#include "stdafx.h"
#include "EpgDataCap_BonMin.h"
#include "../../Common/CommonDef.h"
#include "../../Common/CtrlCmdDef.h"

namespace
{
enum {
	TIMER_STATUS_UPDATE = 1,
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
	, msgManager(OnMessage, this)
	, outCtrlID(-1)
	, cmdCapture(NULL)
	, resCapture(NULL)
	, lastONID(-1)
	, lastTSID(-1)
	, recCtrlID(0)
	, chScanWorking(false)
	, epgCapWorking(false)
	, printStatusSpace(0)
{
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
		}
	}
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
	fs_path appIniPath = GetModuleIniPath();

	this->overWriteFlag       = GetPrivateProfileInt(L"SET", L"OverWrite", 0, appIniPath.c_str()) != 0;
	this->dropSaveThresh      = GetPrivateProfileInt(L"SET", L"DropSaveThresh", 0, appIniPath.c_str());
	this->scrambleSaveThresh  = GetPrivateProfileInt(L"SET", L"ScrambleSaveThresh", -1, appIniPath.c_str());
	this->tsBuffMaxCount      = (DWORD)GetPrivateProfileInt(L"SET", L"TsBuffMaxCount", 5000, appIniPath.c_str());
	this->writeBuffMaxCount   = GetPrivateProfileInt(L"SET", L"WriteBuffMaxCount", -1, appIniPath.c_str());
	this->traceBonDriverLevel = GetPrivateProfileInt(L"SET", L"TraceBonDriverLevel", 0, appIniPath.c_str());
	this->openWait            = GetPrivateProfileInt(L"SET", L"OpenWait", 200, appIniPath.c_str());

	for( int i = 0; ; i++ ){
		this->recFolderList.push_back(GetRecFolderPath(i).native());
		if( this->recFolderList.back().empty() ){
			this->recFolderList.pop_back();
			break;
		}
	}

	vector<NW_SEND_INFO> udpSendList;
	vector<NW_SEND_INFO> tcpSendList;
	for( int tcp = 0; tcp < 2; tcp++ ){
		if( this->iniNetwork == false || (tcp ? this->iniTCP : this->iniUDP) == false ){
			continue;
		}
		LPCWSTR section = tcp ? L"SET_TCP" : L"SET_UDP";
		int count = GetPrivateProfileInt(section, L"Count", 0, appIniPath.c_str());
		for( int i = 0; i < count; i++ ){
			NW_SEND_INFO item;
			WCHAR key[64];
			swprintf_s(key, L"IP%d", i);
			item.ipString = GetPrivateProfileToString(section, key, L"2130706433", appIniPath.c_str());
			if( item.ipString.size() >= 2 && item.ipString[0] == L'[' ){
				item.ipString.erase(0, 1).pop_back();
			}else{
				DWORD ip = (int)wcstol(item.ipString.c_str(), NULL, 10);
				Format(item.ipString, L"%d.%d.%d.%d", ip >> 24, ip >> 16 & 0xFF, ip >> 8 & 0xFF, ip & 0xFF);
			}
			swprintf_s(key, L"Port%d", i);
			item.port = 0;
			if( item.ipString != BON_NW_SRV_PIPE_IP ){
				item.port = GetPrivateProfileInt(section, key, tcp ? BON_TCP_PORT_BEGIN : BON_UDP_PORT_BEGIN, appIniPath.c_str());
			}
			swprintf_s(key, L"BroadCast%d", i);
			item.broadcastFlag = tcp ? 0 : GetPrivateProfileInt(section, key, 0, appIniPath.c_str());
			item.udpMaxSendSize = tcp ? 0 : GetPrivateProfileInt(L"SET", L"UDPPacket", 128, appIniPath.c_str()) * 188;
			(tcp ? tcpSendList : udpSendList).push_back(item);
		}
	}

	this->bonCtrl.SetBackGroundEpgCap(GetPrivateProfileInt(L"SET", L"EpgCapLive", 1, appIniPath.c_str()) != 0,
	                                  GetPrivateProfileInt(L"SET", L"EpgCapRec", 1, appIniPath.c_str()) != 0,
	                                  GetPrivateProfileInt(L"SET", L"EpgCapBackBSBasicOnly", 1, appIniPath.c_str()) != 0,
	                                  GetPrivateProfileInt(L"SET", L"EpgCapBackCS1BasicOnly", 1, appIniPath.c_str()) != 0,
	                                  GetPrivateProfileInt(L"SET", L"EpgCapBackCS2BasicOnly", 1, appIniPath.c_str()) != 0,
	                                  GetPrivateProfileInt(L"SET", L"EpgCapBackCS3BasicOnly", 0, appIniPath.c_str()) != 0,
	                                  (DWORD)GetPrivateProfileInt(L"SET", L"EpgCapBackStartWaitSec", 30, appIniPath.c_str()));

	this->bonCtrl.ReloadSetting(GetPrivateProfileInt(L"SET", L"EMM", 0, appIniPath.c_str()) != 0,
	                            GetPrivateProfileInt(L"SET", L"NoLogScramble", 0, appIniPath.c_str()) != 0,
	                            GetPrivateProfileInt(L"SET", L"ParseEpgPostProcess", 0, appIniPath.c_str()) != 0,
	                            GetPrivateProfileInt(L"SET", L"Scramble", 1, appIniPath.c_str()) != 0,
	                            GetPrivateProfileInt(L"SET", L"Caption", 1, appIniPath.c_str()) != 0,
	                            GetPrivateProfileInt(L"SET", L"Data", 0, appIniPath.c_str()) != 0,
	                            GetPrivateProfileInt(L"SET", L"AllService", 0, appIniPath.c_str()) != 0,
	                            (DWORD)(GetPrivateProfileInt(L"SET", L"SaveLogo", 0, appIniPath.c_str()) == 0 ? 0 :
	                                        GetPrivateProfileInt(L"SET", L"SaveLogoTypeFlags", 32, appIniPath.c_str())));

	if( this->iniBonDriver.empty() == false ){
		if( this->bonCtrl.OpenBonDriver(this->iniBonDriver.c_str(), this->traceBonDriverLevel, this->openWait, this->tsBuffMaxCount) ){
			int selectIndex = ReloadServiceList(this->iniONID, this->iniTSID, this->iniSID);
			if( selectIndex >= 0 && this->bonCtrl.SetCh(this->serviceList[selectIndex]) ){
				this->lastONID = this->serviceList[selectIndex].originalNetworkID;
				this->lastTSID = this->serviceList[selectIndex].transportStreamID;
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
	this->pipeServer.StopServer();
	this->bonCtrl.CloseBonDriver();
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
				DWORD val = VIEW_APP_ST_NORMAL;
				BOOL chChgErr;
				if( this->bonCtrl.GetOpenBonDriver(NULL) == FALSE ){
					val = VIEW_APP_ST_ERR_BON;
				}else if( this->bonCtrl.IsRec() ){
					val = VIEW_APP_ST_REC;
				}else if( this->bonCtrl.GetEpgCapStatus(NULL) == CBonCtrl::ST_WORKING ){
					val = VIEW_APP_ST_GET_EPG;
				}else if( this->bonCtrl.IsChChanging(&chChgErr) == FALSE && chChgErr ){
					val = VIEW_APP_ST_ERR_CH_CHG;
				}
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
			if( cmd.ReadVALUE(&this->outCtrlID) ){
				res.SetParam(CMD_SUCCESS);
			}
			return;
		case CMD2_VIEW_APP_GET_ID:
			AddDebugLog(L"CMD2_VIEW_APP_GET_ID");
			res.WriteVALUE(this->outCtrlID);
			res.SetParam(CMD_SUCCESS);
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
	    (this->lastONID != onid || this->lastTSID != tsid) ){
		//チャンネルが変化した
		for( size_t i = 0; i < this->serviceList.size(); i++ ){
			if( this->serviceList[i].originalNetworkID == onid &&
			    this->serviceList[i].transportStreamID == tsid ){
				int index = ReloadServiceList(onid, tsid, this->serviceList[i].serviceID);
				if( index >= 0 ){
					this->lastONID = onid;
					this->lastTSID = tsid;
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
			if( this->lastONID != info.ONID || this->lastTSID != info.TSID ){
				this->lastONID = info.ONID;
				this->lastTSID = info.TSID;
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
				this->lastONID = -1;
				this->lastTSID = -1;
				if( this->bonCtrl.OpenBonDriver(val.c_str(), this->traceBonDriverLevel, this->openWait, this->tsBuffMaxCount) ){
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
						this->lastONID = this->serviceList[index].originalNetworkID;
						this->lastTSID = this->serviceList[index].transportStreamID;
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
								this->lastONID = this->serviceList[index].originalNetworkID;
								this->lastTSID = this->serviceList[index].transportStreamID;
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
							ReloadServiceList(this->lastONID, this->lastTSID, sid);
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
					val.overWriteFlag = this->overWriteFlag != FALSE;
				}
				this->bonCtrl.ClearErrCount(val.ctrlID);
				if( this->bonCtrl.StartSave(val, this->recFolderList, this->writeBuffMaxCount) ){
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
					this->bonCtrl.SaveErrCount(val.ctrlID, infoPath.native(), true, this->dropSaveThresh,
					                           this->scrambleSaveThresh, resVal.drop, resVal.scramble);
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
