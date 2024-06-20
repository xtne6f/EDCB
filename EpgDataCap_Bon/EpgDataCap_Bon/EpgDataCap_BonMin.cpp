#include "stdafx.h"
#include "EpgDataCap_BonMin.h"
#include "../../Common/CommonDef.h"
#include "../../Common/CommonResource.h"
#include "../../Common/CtrlCmdDef.h"
#include "../../Common/TimeUtil.h"

namespace
{
enum {
	TIMER_STATUS_UPDATE = 1,
	TIMER_START_REC,
	TIMER_TRY_STOP_SERVER,
};

enum {
	ID_APP_INVOKE_CTRL_CMD = CMessageManager::ID_APP,
};
}

CEpgDataCap_BonMin::CEpgDataCap_BonMin()
	: iniNetwork(true)
	, iniUDP(false)
	, iniTCP(false)
	, iniONID(-1)
	, iniTSID(-1)
	, iniSID(-1)
	, iniChScan(false)
	, iniEpgCap(false)
	, iniRec(false)
	, msgManager(OnMessage, this)
	, cmdCapture(NULL)
	, resCapture(NULL)
	, recCtrlID(0)
	, chScanWorking(false)
	, epgCapWorking(false)
	, printStatusSpace(0)
{
	VIEW_APP_STATUS_INFO info = {};
	info.space = -1;
	info.ch = -1;
	info.originalNetworkID = -1;
	info.transportStreamID = -1;
	info.appID = -1;
	this->statusInfo = info;
}

void CEpgDataCap_BonMin::Main()
{
	if( this->msgManager.MessageLoop(true) == false ){
		AddDebugLog(L"Failed to enter message loop");
	}
}

void CEpgDataCap_BonMin::ParseCommandLine(char** argv, int argc)
{
	for( int i = 1; i < argc; i++ ){
		if( strcmp(argv[i], "-nonw") == 0 ){
			this->iniNetwork = false;
		}else if( strcmp(argv[i], "-nwudp") == 0 ){
			this->iniUDP = true;
		}else if( strcmp(argv[i], "-nwtcp") == 0 ){
			this->iniTCP = true;
		}else if( i + 1 < argc && strcmp(argv[i], "-d") == 0 ){
			UTF8toW(argv[++i], this->iniBonDriver);
		}else if( i + 1 < argc && strcmp(argv[i], "-nid") == 0 ){
			this->iniONID = (int)strtol(argv[++i], NULL, 10);
		}else if( i + 1 < argc && strcmp(argv[i], "-tsid") == 0 ){
			this->iniTSID = (int)strtol(argv[++i], NULL, 10);
		}else if( i + 1 < argc && strcmp(argv[i], "-sid") == 0 ){
			this->iniSID = (int)strtol(argv[++i], NULL, 10);
		}else if( strcmp(argv[i], "-chscan") == 0 ){
			this->iniChScan = true;
		}else if( strcmp(argv[i], "-epgcap") == 0 ){
			this->iniEpgCap = true;
		}else if( strcmp(argv[i], "-rec") == 0 ){
			this->iniRec = true;
		}
	}
}

bool CEpgDataCap_BonMin::ValidateCommandLine(char** argv, int argc)
{
	if( argc >= 2 && strcmp(argv[1], "-h") == 0 ){
		printf("Ver.%s\nUsage: %s [-nonw][-nwudp][-nwtcp][-d BonDriver*][-nid x][-tsid y][-sid z][-chscan][-epgcap][-rec]\n",
#ifdef EDCB_VERSION_TEXT
		       EDCB_VERSION_TEXT
#else
		       " ?"
#endif
		       , argv[0]);
		return false;
	}
	return true;
}

int CEpgDataCap_BonMin::ReloadServiceList(int selONID, int selTSID, int selSID)
{
	const map<DWORD, CH_DATA4>& nextServices = this->bonCtrl.GetServiceList();
	this->serviceList.clear();
	this->serviceList.reserve(nextServices.size());
	int selectIndex = -1;
	for( auto itr = nextServices.begin(); itr != nextServices.end(); ++itr ){
		if( selectIndex < 0 ||
		    (itr->second.originalNetworkID == selONID &&
		     itr->second.transportStreamID == selTSID &&
		     itr->second.serviceID == selSID) ){
			selectIndex = (int)this->serviceList.size();
		}
		this->serviceList.push_back(itr->second);
	}
	return selectIndex;
}

void CEpgDataCap_BonMin::PrintStatusLog(LPCWSTR log)
{
	if( log[0] != L'\r' ){
		this->printStatusSpace = 0;
	}
	for( size_t i = 0; log[i]; i++ ){
		char dest[4];
		fwrite(dest, 1, codepoint_to_utf8(log[i], dest), stdout);
	}
}

void CEpgDataCap_BonMin::OnInit()
{
	this->setting = APP_SETTING::Load(GetModuleIniPath().c_str());

	for( int i = 0; ; i++ ){
		this->recFolderList.push_back(GetRecFolderPath(i).native());
		if( this->recFolderList.back().empty() ){
			this->recFolderList.pop_back();
			break;
		}
	}

	vector<NW_SEND_INFO> udpSendList;
	vector<NW_SEND_INFO> tcpSendList;
	udpSendList.swap(this->setting.udpSendList);
	tcpSendList.swap(this->setting.tcpSendList);
	if( this->iniNetwork == false || this->iniUDP == false ){
		udpSendList.clear();
	}
	if( this->iniNetwork == false || this->iniTCP == false ){
		tcpSendList.clear();
	}

	this->bonCtrl.SetBackGroundEpgCap(this->setting.epgCapLive,
	                                  this->setting.epgCapRec,
	                                  this->setting.epgCapBackBSBasic,
	                                  this->setting.epgCapBackCS1Basic,
	                                  this->setting.epgCapBackCS2Basic,
	                                  this->setting.epgCapBackCS3Basic,
	                                  this->setting.epgCapBackStartWaitSec);

	this->bonCtrl.ReloadSetting(this->setting.emm,
	                            this->setting.noLogScramble,
	                            this->setting.parseEpgPostProcess,
	                            this->setting.scramble,
	                            this->setting.enableCaption,
	                            this->setting.enableData,
	                            this->setting.allService,
	                            this->setting.saveLogo ? this->setting.saveLogoTypeFlags : 0);

	if( this->iniBonDriver.empty() == false ){
		if( this->bonCtrl.OpenBonDriver(this->iniBonDriver.c_str(), this->setting.traceBonDriverLevel, this->setting.openWait, this->setting.tsBuffMaxCount) ){
			int selectIndex = ReloadServiceList(this->iniONID, this->iniTSID, this->iniSID);
			if( selectIndex >= 0 && this->bonCtrl.SetCh(this->serviceList[selectIndex]) ){
				lock_recursive_mutex lock(this->statusInfoLock);
				this->statusInfo.originalNetworkID = this->serviceList[selectIndex].originalNetworkID;
				this->statusInfo.transportStreamID = this->serviceList[selectIndex].transportStreamID;
			}
		}
	}

	if( udpSendList.empty() == false ){
		this->bonCtrl.SendUdp(&udpSendList);
		if( udpSendList.empty() == false ){
			PrintStatusLog(L"UDP:");
			for( size_t i = 0; i < udpSendList.size(); i++ ){
				PrintStatusLog(L" ");
				if( udpSendList[i].ipString.find(L':') == wstring::npos ){
					PrintStatusLog(udpSendList[i].ipString.c_str());
				}else{
					PrintStatusLog(L"[");
					PrintStatusLog(udpSendList[i].ipString.c_str());
					PrintStatusLog(L"]");
				}
				WCHAR buff[32];
				swprintf_s(buff, L":%d%ls", udpSendList[i].port, udpSendList[i].broadcastFlag ? L"(Broadcast)" : L"");
				PrintStatusLog(buff);
			}
			PrintStatusLog(L"\n");
		}
	}
	if( tcpSendList.empty() == false ){
		this->bonCtrl.SendTcp(&tcpSendList);
		if( tcpSendList.empty() == false ){
			PrintStatusLog(L"TCP:");
			for( size_t i = 0; i < tcpSendList.size(); i++ ){
				PrintStatusLog(L" ");
				if( tcpSendList[i].ipString.find(L':') == wstring::npos ){
					PrintStatusLog(tcpSendList[i].ipString.c_str());
				}else{
					PrintStatusLog(L"[");
					PrintStatusLog(tcpSendList[i].ipString.c_str());
					PrintStatusLog(L"]");
				}
				WCHAR buff[32];
				swprintf_s(buff, L":%d", tcpSendList[i].port);
				PrintStatusLog(buff);
			}
			PrintStatusLog(L"\n");
		}
	}

	this->msgManager.SetTimer(TIMER_STATUS_UPDATE, 1000);

	StartPipeServer();

	if( this->iniChScan ){
		if( this->bonCtrl.StartChScan() ){
			this->chScanWorking = true;
		}else{
			PrintStatusLog(L"Failed to start channel scan\n");
			this->msgManager.Send(CMessageManager::ID_CLOSE);
			return;
		}
	}else if( this->iniEpgCap ){
		if( this->bonCtrl.StartEpgCap(NULL) ){
			this->epgCapWorking = true;
		}else{
			PrintStatusLog(L"Failed to start EPG capture\n");
			this->msgManager.Send(CMessageManager::ID_CLOSE);
			return;
		}
	}else if( this->iniRec ){
		if( this->statusInfo.originalNetworkID >= 0 ){
			this->msgManager.SetTimer(TIMER_START_REC, 3000);
		}else{
			PrintStatusLog(L"Failed to open or set channel\n");
			this->msgManager.Send(CMessageManager::ID_CLOSE);
			return;
		}
	}
}

void CEpgDataCap_BonMin::OnDestroy()
{
	if( this->chScanWorking ){
		this->bonCtrl.StopChScan();
		PrintStatusLog(L"Canceled\n");
	}
	if( this->epgCapWorking ){
		this->bonCtrl.StopEpgCap();
		PrintStatusLog(L"Canceled\n");
	}
	if( this->recCtrlID != 0 ){
		this->bonCtrl.DeleteServiceCtrl(this->recCtrlID);
		this->recCtrlID = 0;
	}
	this->pipeServer.StopServer();
	this->bonCtrl.CloseBonDriver();
}

void CEpgDataCap_BonMin::OnStartRec()
{
	if( this->bonCtrl.IsRec() || this->recCtrlID != 0 ){
		return;
	}

	//即時録画
	this->recCtrlID = this->bonCtrl.CreateServiceCtrl(TRUE);
	wstring serviceName;
	Format(serviceName, L"%04X", this->bonCtrl.GetNWCtrlServiceID());
	for( size_t i = 0; i < this->serviceList.size(); i++ ){
		if( this->serviceList[i].originalNetworkID == this->statusInfo.originalNetworkID &&
		    this->serviceList[i].transportStreamID == this->statusInfo.transportStreamID &&
		    this->serviceList[i].serviceID == this->bonCtrl.GetNWCtrlServiceID() ){
			serviceName = this->serviceList[i].serviceName;
			break;
		}
	}
	wstring fileName = this->setting.recFileName;
	SYSTEMTIME now;
	ConvertSystemTime(GetNowI64Time(), &now);
	for( int i = 0; GetTimeMacroName(i); i++ ){
		wstring name;
		UTF8toW(GetTimeMacroName(i), name);
		Replace(fileName, L'$' + name + L'$', GetTimeMacroValue(i, now));
	}
	Replace(fileName, L"$ServiceName$", serviceName);
	CheckFileName(fileName);

	SET_CTRL_REC_PARAM recParam;
	recParam.ctrlID = this->recCtrlID;
	recParam.fileName = L"padding.ts";
	recParam.overWriteFlag = this->setting.overWrite;
	recParam.createSize = 0;
	recParam.saveFolder.resize(1);
	recParam.saveFolder.back().recFolder = this->recFolderList[0];
	recParam.saveFolder.back().recFileName = fileName;
	recParam.pittariFlag = FALSE;
	if( this->bonCtrl.StartSave(recParam, this->recFolderList, this->setting.writeBuffMaxCount) == FALSE ){
		PrintStatusLog(L"Failed to start rec\n");
		this->msgManager.Send(CMessageManager::ID_CLOSE);
	}
}

bool CEpgDataCap_BonMin::OnMessage(CMessageManager::PARAMS& pa)
{
	CEpgDataCap_BonMin* sys = (CEpgDataCap_BonMin*)pa.ctx;
	switch( pa.id ){
	case CMessageManager::ID_INITIALIZED:
		sys->OnInit();
		return true;
	case CMessageManager::ID_SIGNAL:
		AddDebugLogFormat(L"Received signal %d", (int)pa.param1);
		break;
	case CMessageManager::ID_CLOSE:
		//デッドロック回避のためメッセージポンプを維持しつつサーバを終わらせる
		sys->pipeServer.StopServer(true);
		sys->msgManager.SetTimer(TIMER_TRY_STOP_SERVER, 20);
		return true;
	case CMessageManager::ID_DESTROY:
		sys->OnDestroy();
		return true;
	case CMessageManager::ID_TIMER:
		if( pa.param1 == TIMER_STATUS_UPDATE ){
			sys->OnTimerStatusUpdate();
			return true;
		}else if( pa.param1 == TIMER_START_REC ){
			sys->msgManager.KillTimer(TIMER_START_REC);
			sys->OnStartRec();
			return true;
		}else if( pa.param1 == TIMER_TRY_STOP_SERVER ){
			if( sys->pipeServer.StopServer(true) ){
				AddDebugLog(L"CmdServer stopped");
				sys->msgManager.Send(CMessageManager::ID_DESTROY);
			}
			return true;
		}
		break;
	case ID_APP_INVOKE_CTRL_CMD:
		sys->CtrlCmdCallbackInvoked();
		return true;
	}
	return false;
}

void CEpgDataCap_BonMin::StartPipeServer()
{
	CPipeServer::DeleteRemainingFiles(CMD2_VIEW_CTRL_PIPE);

	wstring pipeName;
	Format(pipeName, L"%ls%d", CMD2_VIEW_CTRL_PIPE, (int)getpid());
	AddDebugLogFormat(L"%ls", pipeName.c_str());
	if( this->pipeServer.StartServer(pipeName, [this](CCmdStream& cmd, CCmdStream& res) {
		res.SetParam(CMD_ERR);
		//同期呼び出しが不要なコマンドはここで処理する
		switch( cmd.GetParam() ){
		case CMD2_VIEW_APP_GET_BONDRIVER:
			{
				wstring bonFile;
				if( this->bonCtrl.GetOpenBonDriver(&bonFile) ){
					res.WriteVALUE(bonFile);
					res.SetParam(CMD_SUCCESS);
				}
			}
			return;
		case CMD2_VIEW_APP_GET_DELAY:
			res.WriteVALUE(this->bonCtrl.GetTimeDelay());
			res.SetParam(CMD_SUCCESS);
			return;
		case CMD2_VIEW_APP_GET_STATUS:
			{
				BOOL chChgErr;
				DWORD val =
					this->bonCtrl.GetOpenBonDriver(NULL) == FALSE ? VIEW_APP_ST_ERR_BON :
					this->bonCtrl.IsRec() ? VIEW_APP_ST_REC :
					this->bonCtrl.GetEpgCapStatus(NULL) == CBonCtrl::ST_WORKING ? VIEW_APP_ST_GET_EPG :
					this->bonCtrl.IsChChanging(&chChgErr) == FALSE && chChgErr ? VIEW_APP_ST_ERR_CH_CHG : VIEW_APP_ST_NORMAL;
				res.WriteVALUE(val);
				res.SetParam(CMD_SUCCESS);
			}
			return;
		case CMD2_VIEW_APP_CLOSE:
			AddDebugLog(L"CMD2_VIEW_APP_CLOSE");
			this->msgManager.Post(CMessageManager::ID_CLOSE);
			res.SetParam(CMD_SUCCESS);
			return;
		case CMD2_VIEW_APP_SET_ID:
			AddDebugLog(L"CMD2_VIEW_APP_SET_ID");
			if( cmd.ReadVALUE(&this->statusInfo.appID) ){
				res.SetParam(CMD_SUCCESS);
			}
			return;
		case CMD2_VIEW_APP_GET_ID:
			AddDebugLog(L"CMD2_VIEW_APP_GET_ID");
			res.WriteVALUE(this->statusInfo.appID);
			res.SetParam(CMD_SUCCESS);
			return;
		case CMD2_VIEW_APP_GET_STATUS_DETAILS:
			{
				DWORD flags;
				if( cmd.ReadVALUE(&flags) ){
					VIEW_APP_STATUS_INFO info;
					{
						lock_recursive_mutex lock(this->statusInfoLock);
						info = this->statusInfo;
					}
					if( flags & VIEW_APP_FLAG_GET_STATUS ){
						BOOL chChgErr;
						info.status =
							this->bonCtrl.GetOpenBonDriver(NULL) == FALSE ? VIEW_APP_ST_ERR_BON :
							this->bonCtrl.IsRec() ? VIEW_APP_ST_REC :
							this->bonCtrl.GetEpgCapStatus(NULL) == CBonCtrl::ST_WORKING ? VIEW_APP_ST_GET_EPG :
							this->bonCtrl.IsChChanging(&chChgErr) == FALSE && chChgErr ? VIEW_APP_ST_ERR_CH_CHG : VIEW_APP_ST_NORMAL;
					}
					if( flags & VIEW_APP_FLAG_GET_DELAY ){
						info.delaySec = this->bonCtrl.GetTimeDelay();
					}
					if( flags & VIEW_APP_FLAG_GET_BONDRIVER ){
						this->bonCtrl.GetOpenBonDriver(&info.bonDriver);
					}
					res.WriteVALUE(info);
					res.SetParam(CMD_SUCCESS);
				}
			}
			return;
		case CMD2_VIEW_APP_REC_FILE_PATH:
			AddDebugLog(L"CMD2_VIEW_APP_REC_FILE_PATH");
			{
				DWORD id;
				if( cmd.ReadVALUE(&id) ){
					wstring saveFile = this->bonCtrl.GetSaveFilePath(id);
					if( saveFile.size() > 0 ){
						res.WriteVALUE(saveFile);
						res.SetParam(CMD_SUCCESS);
					}
				}
			}
			return;
		case CMD2_VIEW_APP_SEARCH_EVENT:
			{
				SEARCH_EPG_INFO_PARAM key;
				EPGDB_EVENT_INFO epgInfo;
				if( cmd.ReadVALUE(&key) &&
				    this->bonCtrl.SearchEpgInfo(key.ONID, key.TSID, key.SID, key.eventID, key.pfOnlyFlag, &epgInfo) == NO_ERR ){
					res.WriteVALUE(epgInfo);
					res.SetParam(CMD_SUCCESS);
				}
			}
			return;
		case CMD2_VIEW_APP_GET_EVENT_PF:
			{
				GET_EPG_PF_INFO_PARAM key;
				EPGDB_EVENT_INFO epgInfo;
				if( cmd.ReadVALUE(&key) &&
				    this->bonCtrl.GetEpgInfo(key.ONID, key.TSID, key.SID, key.pfNextFlag, &epgInfo) == NO_ERR ){
					res.WriteVALUE(epgInfo);
					res.SetParam(CMD_SUCCESS);
				}
			}
			return;
		case CMD2_VIEW_APP_EXEC_VIEW_APP:
			res.SetParam(CMD_NON_SUPPORT);
			return;
		}
		//CtrlCmdCallbackInvoked()をメインスレッドで呼ぶ
		//注意: CPipeServerがアクティブな間、メッセージループは確実に存在しなければならない
		this->cmdCapture = &cmd;
		this->resCapture = &res;
		this->msgManager.Send(ID_APP_INVOKE_CTRL_CMD);
		this->cmdCapture = NULL;
		this->resCapture = NULL;
	}) == false ){
		AddDebugLog(L"Failed to start CmdServer");
	}
}

void CEpgDataCap_BonMin::OnTimerStatusUpdate()
{
	this->bonCtrl.Check();

	float signalLv;
	int space;
	int ch;
	ULONGLONG drop = 0;
	ULONGLONG scramble = 0;
	this->bonCtrl.GetViewStatusInfo(&signalLv, &space, &ch, &drop, &scramble);
	{
		lock_recursive_mutex lock(this->statusInfoLock);
		this->statusInfo.drop = drop;
		this->statusInfo.scramble = scramble;
		this->statusInfo.signalLv = signalLv;
		this->statusInfo.space = space;
		this->statusInfo.ch = ch;
	}

	wstring statusLog;
	if( space >= 0 && ch >= 0 ){
		Format(statusLog, L"\rSig:%.02f D:%lld S:%lld sp:%d ch:%d ", signalLv, drop, scramble, space, ch);
	}else{
		Format(statusLog, L"\rSig:%.02f D:%lld S:%lld ", signalLv, drop, scramble);
	}
	if( this->bonCtrl.IsRec() ){
		statusLog += L"Rec ";
	}
	if( this->chScanWorking ){
		statusLog += L"ChScan ";
	}
	if( this->epgCapWorking ){
		statusLog += L"EpgCap ";
	}

	//行頭復帰で前回出力をすべて上書きするためスペースを補う
	this->printStatusSpace = max(printStatusSpace, statusLog.size());
	statusLog.append(this->printStatusSpace - statusLog.size(), L' ');
	PrintStatusLog(statusLog.c_str());
	fflush(stdout);

	WORD onid;
	WORD tsid;
	//チャンネルスキャン中はサービス一覧などが安定しないため
	//EPG取得中は別の検出ロジックがある
	if( this->chScanWorking == false && this->epgCapWorking == false &&
	    this->bonCtrl.GetStreamID(&onid, &tsid) &&
	    (this->statusInfo.originalNetworkID != onid || this->statusInfo.transportStreamID != tsid) ){
		//チャンネルが変化した
		for( size_t i = 0; i < this->serviceList.size(); i++ ){
			if( this->serviceList[i].originalNetworkID == onid &&
			    this->serviceList[i].transportStreamID == tsid ){
				int index = ReloadServiceList(onid, tsid, this->serviceList[i].serviceID);
				if( index >= 0 ){
					{
						lock_recursive_mutex lock(this->statusInfoLock);
						this->statusInfo.originalNetworkID = onid;
						this->statusInfo.transportStreamID = tsid;
					}
					this->bonCtrl.SetNWCtrlServiceID(this->serviceList[index].serviceID);
				}
				break;
			}
		}
	}

	if( this->chScanWorking ){
		DWORD space;
		DWORD ch;
		wstring chName;
		DWORD chkNum;
		DWORD totalNum;
		CBonCtrl::JOB_STATUS status = this->bonCtrl.GetChScanStatus(&space, &ch, &chName, &chkNum, &totalNum);
		if( status == CBonCtrl::ST_WORKING ){
			wstring log;
			Format(log, L"\"%ls\" %d/%d remain %d sec\n", chName.c_str(), chkNum, totalNum, (totalNum - chkNum) * 10);
			if( this->lastChScanLog != log ){
				this->lastChScanLog = log;
				PrintStatusLog(log.c_str());
			}
		}else{
			this->chScanWorking = false;
			if( status == CBonCtrl::ST_CANCEL ){
				PrintStatusLog(L"Canceled\n");
			}else if( status == CBonCtrl::ST_COMPLETE ){
				PrintStatusLog(L"Completed\n");
			}
			this->msgManager.Send(CMessageManager::ID_CLOSE);
			return;
		}
	}

	if( this->epgCapWorking ){
		SET_CH_INFO info;
		CBonCtrl::JOB_STATUS status = this->bonCtrl.GetEpgCapStatus(&info);
		if( status == CBonCtrl::ST_WORKING ){
			ReloadServiceList(info.ONID, info.TSID, info.SID);
			this->bonCtrl.SetNWCtrlServiceID(info.SID);
			{
				lock_recursive_mutex lock(this->statusInfoLock);
				this->statusInfo.originalNetworkID = info.ONID;
				this->statusInfo.transportStreamID = info.TSID;
			}
		}else{
			this->epgCapWorking = false;
			if( status == CBonCtrl::ST_CANCEL ){
				PrintStatusLog(L"Canceled\n");
			}else if( status == CBonCtrl::ST_COMPLETE ){
				PrintStatusLog(L"Completed\n");
			}
			if( this->iniEpgCap ){
				this->msgManager.Send(CMessageManager::ID_CLOSE);
				return;
			}
		}
	}
}

void CEpgDataCap_BonMin::CtrlCmdCallbackInvoked()
{
	const CCmdStream& cmd = *this->cmdCapture;
	CCmdStream& res = *this->resCapture;

	switch( cmd.GetParam() ){
	case CMD2_VIEW_APP_SET_BONDRIVER:
		AddDebugLog(L"CMD2_VIEW_APP_SET_BONDRIVER");
		{
			wstring val;
			if( cmd.ReadVALUE(&val) ){
				{
					lock_recursive_mutex lock(this->statusInfoLock);
					this->statusInfo.originalNetworkID = -1;
					this->statusInfo.transportStreamID = -1;
				}
				if( this->bonCtrl.OpenBonDriver(val.c_str(), this->setting.traceBonDriverLevel, this->setting.openWait, this->setting.tsBuffMaxCount) ){
					ReloadServiceList();
					res.SetParam(CMD_SUCCESS);
				}else{
					this->serviceList.clear();
				}
			}
		}
		break;
	case CMD2_VIEW_APP_SET_CH:
		AddDebugLog(L"CMD2_VIEW_APP_SET_CH");
		{
			SET_CH_INFO val;
			if( cmd.ReadVALUE(&val) ){
				if( val.useSID ){
					int index = ReloadServiceList(val.ONID, val.TSID, val.SID);
					if( index >= 0 && this->bonCtrl.SetCh(this->serviceList[index]) ){
						lock_recursive_mutex lock(this->statusInfoLock);
						this->statusInfo.originalNetworkID = this->serviceList[index].originalNetworkID;
						this->statusInfo.transportStreamID = this->serviceList[index].transportStreamID;
						res.SetParam(CMD_SUCCESS);
					}
				}else if( val.useBonCh ){
					for( size_t i = 0; i < this->serviceList.size(); i++ ){
						if( this->serviceList[i].space == val.space &&
						    this->serviceList[i].ch == val.ch ){
							int index = ReloadServiceList(this->serviceList[i].originalNetworkID,
							                              this->serviceList[i].transportStreamID,
							                              this->serviceList[i].serviceID);
							if( index >= 0 && this->bonCtrl.SetCh(this->serviceList[index]) ){
								lock_recursive_mutex lock(this->statusInfoLock);
								this->statusInfo.originalNetworkID = this->serviceList[index].originalNetworkID;
								this->statusInfo.transportStreamID = this->serviceList[index].transportStreamID;
								res.SetParam(CMD_SUCCESS);
							}
							break;
						}
					}
				}
			}
		}
		break;
	case CMD2_VIEW_APP_SET_STANDBY_REC:
		AddDebugLog(L"CMD2_VIEW_APP_SET_STANDBY_REC");
		res.SetParam(CMD_SUCCESS);
		break;
	case CMD2_VIEW_APP_CREATE_CTRL:
		AddDebugLog(L"CMD2_VIEW_APP_CREATE_CTRL");
		{
			DWORD val = this->bonCtrl.CreateServiceCtrl(FALSE);
			this->cmdCtrlList.push_back(val);
			res.WriteVALUE(val);
			res.SetParam(CMD_SUCCESS);
		}
		break;
	case CMD2_VIEW_APP_DELETE_CTRL:
		AddDebugLog(L"CMD2_VIEW_APP_DELETE_CTRL");
		{
			DWORD val;
			if( cmd.ReadVALUE(&val) ){
				auto itr = std::find(this->cmdCtrlList.begin(), this->cmdCtrlList.end(), val);
				if( itr != this->cmdCtrlList.end() ){
					this->cmdCtrlList.erase(itr);
					if( this->bonCtrl.DeleteServiceCtrl(val) ){
						WORD sid;
						if( this->cmdCtrlList.empty() == false &&
						    this->bonCtrl.GetServiceID(this->cmdCtrlList.front(), &sid) && sid != 0xFFFF ){
							this->bonCtrl.SetNWCtrlServiceID(sid);
							ReloadServiceList(this->statusInfo.originalNetworkID, this->statusInfo.transportStreamID, sid);
						}
						res.SetParam(CMD_SUCCESS);
					}
				}
			}
		}
		break;
	case CMD2_VIEW_APP_SET_CTRLMODE:
		AddDebugLog(L"CMD2_VIEW_APP_SET_CTRLMODE");
		{
			SET_CTRL_MODE val;
			if( cmd.ReadVALUE(&val) ){
				this->bonCtrl.SetScramble(val.ctrlID, val.enableScramble);
				this->bonCtrl.SetServiceMode(val.ctrlID, val.enableCaption, val.enableData);
				this->bonCtrl.SetServiceID(val.ctrlID, val.SID);
				res.SetParam(CMD_SUCCESS);
			}
		}
		break;
	case CMD2_VIEW_APP_REC_START_CTRL:
		AddDebugLog(L"CMD2_VIEW_APP_REC_START_CTRL");
		{
			SET_CTRL_REC_PARAM val;
			if( cmd.ReadVALUE(&val) ){
				if( val.overWriteFlag == 2 ){
					val.overWriteFlag = this->setting.overWrite;
				}
				this->bonCtrl.ClearErrCount(val.ctrlID);
				if( this->bonCtrl.StartSave(val, this->recFolderList, this->setting.writeBuffMaxCount) ){
					res.SetParam(CMD_SUCCESS);
				}
			}
		}
		break;
	case CMD2_VIEW_APP_REC_STOP_CTRL:
		AddDebugLog(L"CMD2_VIEW_APP_REC_STOP_CTRL");
		{
			SET_CTRL_REC_STOP_PARAM val;
			if( cmd.ReadVALUE(&val) ){
				SET_CTRL_REC_STOP_RES_PARAM resVal;
				resVal.recFilePath = this->bonCtrl.GetSaveFilePath(val.ctrlID);
				resVal.drop = 0;
				resVal.scramble = 0;
				if( resVal.recFilePath.empty() == false && val.saveErrLog ){
					fs_path infoPath = GetPrivateProfileToString(L"SET", L"RecInfoFolder", L"", GetCommonIniPath().c_str());
					if( infoPath.empty() ){
						infoPath = resVal.recFilePath + L".err";
					}else{
						infoPath.append(fs_path(resVal.recFilePath).filename().concat(L".err").native());
					}
					this->bonCtrl.SaveErrCount(val.ctrlID, infoPath.native(), true, this->setting.dropSaveThresh,
					                           this->setting.scrambleSaveThresh, resVal.drop, resVal.scramble);
				}else{
					this->bonCtrl.GetErrCount(val.ctrlID, &resVal.drop, &resVal.scramble);
				}
				BOOL subRec;
				if( this->bonCtrl.EndSave(val.ctrlID, &subRec) ){
					resVal.subRecFlag = subRec != FALSE;
					res.WriteVALUE(resVal);
					res.SetParam(CMD_SUCCESS);
				}
			}
		}
		break;
	case CMD2_VIEW_APP_EPGCAP_START:
		AddDebugLog(L"CMD2_VIEW_APP_EPGCAP_START");
		{
			vector<SET_CH_INFO> val;
			if( cmd.ReadVALUE(&val) ){
				if( this->bonCtrl.StartEpgCap(&val) ){
					this->epgCapWorking = true;
					res.SetParam(CMD_SUCCESS);
				}
			}
		}
		break;
	case CMD2_VIEW_APP_EPGCAP_STOP:
		AddDebugLog(L"CMD2_VIEW_APP_EPGCAP_STOP");
		this->bonCtrl.StopEpgCap();
		res.SetParam(CMD_SUCCESS);
		break;
	case CMD2_VIEW_APP_REC_STOP_ALL:
		AddDebugLog(L"CMD2_VIEW_APP_REC_STOP_ALL");
		while( this->cmdCtrlList.empty() == false ){
			this->bonCtrl.DeleteServiceCtrl(this->cmdCtrlList.back());
			this->cmdCtrlList.pop_back();
		}
		if( this->recCtrlID != 0 ){
			this->bonCtrl.DeleteServiceCtrl(this->recCtrlID);
			this->recCtrlID = 0;
		}
		res.SetParam(CMD_SUCCESS);
		break;
	case CMD2_VIEW_APP_REC_WRITE_SIZE:
		{
			DWORD val;
			if( cmd.ReadVALUE(&val) ){
				LONGLONG writeSize = -1;
				this->bonCtrl.GetRecWriteSize(val, &writeSize);
				res.WriteVALUE(writeSize);
				res.SetParam(CMD_SUCCESS);
			}
		}
		break;
	default:
		AddDebugLogFormat(L"err default cmd %d", cmd.GetParam());
		res.SetParam(CMD_NON_SUPPORT);
		break;
	}
}
