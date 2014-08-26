#include "StdAfx.h"
#include "EpgTimerSrvMain.h"
#include "HTMLManager.h"
#include "RestApiManager.h"

#include "../../Common/CommonDef.h"
#include "../../Common/CtrlCmdDef.h"
#include "../../Common/CtrlCmdUtil.h"
#include "../../Common/CtrlCmdUtil2.h"
#include "../../Common/StringUtil.h"
#include "../../Common/BlockLock.h"

#include "HttpPublicFileSend.h"
#include "HttpRecFileSend.h"
#include "HttpRequestReader.h"

#include "SyoboiCalUtil.h"

#include <process.h>

CEpgTimerSrvMain::CEpgTimerSrvMain(void)
{
	InitializeCriticalSection(&settingLock);

	this->stopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	this->sleepEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	this->resetServerEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	this->reloadEpgChkEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	this->dlnaManager = NULL;

	this->enableTCPSrv = FALSE;
	this->tcpPort = 4510;
	this->autoAddDays = 8;
	this->autoAddHour = 0;
	this->chkGroupEvent = TRUE;
	this->rebootDef = 0;
	this->ngEpgFileSrvCoop = FALSE;

	this->awayMode = FALSE;

	this->httpPublicFolder = L"";
	this->enableHttpPublic = FALSE;

	OSVERSIONINFO osvi;
	ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx((OSVERSIONINFO*)&osvi);
	if( VER_PLATFORM_WIN32_NT==osvi.dwPlatformId ){
		if( osvi.dwMajorVersion >= 6 ){
			//Vista以降
			this->awayMode = TRUE;
		}
	}

}


CEpgTimerSrvMain::~CEpgTimerSrvMain(void)
{
	CloseHandle(this->reloadEpgChkEvent);
	CloseHandle(this->resetServerEvent);
	CloseHandle(this->sleepEvent);
	CloseHandle(this->stopEvent);

	if( this->dlnaManager != NULL ){
		this->dlnaManager->StopDMS();
		this->dlnaManager->StopSSDPServer();
		SAFE_DELETE(this->dlnaManager);
	}

	DeleteCriticalSection(&settingLock);
}

//メインループ処理
//引数：
// serviceFlag			[IN]サービスとしての起動かどうか
void CEpgTimerSrvMain::StartMain(
	BOOL serviceFlag
	)
{
	ReloadSetting();

	this->reserveManager.ReloadReserveData();
	wstring epgAutoAddFilePath;
	GetSettingPath(epgAutoAddFilePath);
	epgAutoAddFilePath += L"\\";
	epgAutoAddFilePath += EPG_AUTO_ADD_TEXT_NAME;

	wstring manualAutoAddFilePath;
	GetSettingPath(manualAutoAddFilePath);
	manualAutoAddFilePath += L"\\";
	manualAutoAddFilePath += MANUAL_AUTO_ADD_TEXT_NAME;

	{
		CBlockLock lock(&this->settingLock);
		this->epgAutoAdd.ParseText(epgAutoAddFilePath.c_str());
		this->manualAutoAdd.ParseText(manualAutoAddFilePath.c_str());
	}

	this->reserveManager.SetNotifyManager(&this->notifyManager);
	this->reserveManager.SetEpgDBManager(&this->epgDB);

	//Pipeサーバースタート
	CPipeServer pipeServer;
	pipeServer.StartServer(CMD2_EPG_SRV_EVENT_WAIT_CONNECT, CMD2_EPG_SRV_PIPE, CtrlCmdCallback, this, 0, GetCurrentProcessId());

	this->epgDB.ReloadEpgData();
	this->reserveManager.ReloadBankMap(FALSE);
	SetEvent(this->reloadEpgChkEvent);

	CTCPServer tcpServer;
	CHttpServer httpServer;
	CTCPServerUtil tcpSrvUtil;

	HANDLE resumeTimer = NULL;
	LONGLONG resumeTime = 0;

	CSendCtrlCmd sendCtrl;
	DWORD countChkSuspend = 11;
	BYTE suspendMode = 0xFF;
	BYTE rebootFlag = 0xFF;
	BOOL reloadEpgChkFlag = FALSE;
	
	while(1){
		HANDLE events[] = {this->stopEvent, this->sleepEvent, this->resetServerEvent, this->reloadEpgChkEvent};
		DWORD retWait = WaitForMultipleObjects(4, events, FALSE, reloadEpgChkFlag ? 200 : 1000);
		if( retWait == WAIT_OBJECT_0 ){
			break;
		}else if( retWait == WAIT_OBJECT_0 + 1 ){
			BYTE rebootFlagWork_;
			BYTE suspendModeWork_;
			{
				CBlockLock lock(&this->settingLock);
				rebootFlagWork_ = this->rebootFlagWork;
				suspendModeWork_ = this->suspendModeWork;
			}
			if( rebootFlagWork_ == 1 && suspendModeWork_ == 0xFF ){
				SetShutdown(4);
			}else{
				//ストリーミングを終了する
				this->streamingManager.CloseAllFile();
				SetThreadExecutionState(ES_CONTINUOUS);
				//タイマを正常にセットしたor不要だったときだけ続行
				if( SetResumeTimer(&resumeTimer, &resumeTime, rebootFlagWork_ == 1) == TRUE ){
					if( suspendModeWork_ == 1 || suspendModeWork_ == 2 ){
						SetShutdown(suspendModeWork_);
						if( rebootFlagWork_ == 1 ){
							if( QueryReboot(1) == FALSE ){
								SetShutdown(4);
							}
						}
					}else if( suspendModeWork_ == 3 ){
						SetShutdown(3);
					}
				}
			}
			ResetEvent(this->sleepEvent);
		}else if( retWait == WAIT_OBJECT_0 + 2 ){
			//コールバック関数とデッドロックする可能性があるのでここで排他してはいけない
			ResetServer(tcpServer, httpServer, tcpSrvUtil);
		}else if( retWait == WAIT_OBJECT_0 + 3 ){
			reloadEpgChkFlag = TRUE;
		}

		if( reloadEpgChkFlag == TRUE ){
			if( this->epgDB.IsLoadingData() == FALSE ){
				//リロード終わったので自動予約登録処理を行う
				{
					CheckTuijyu();
					AutoAddReserveEPG();
					AutoAddReserveProgram();
					this->reserveManager.ReloadBankMap(TRUE);

					//しょぼいカレンダー対応
					CSyoboiCalUtil syoboi;
					vector<RESERVE_DATA*> reserveList;
					reserveManager.GetReserveDataAll(&reserveList);
					vector<TUNER_RESERVE_INFO> tunerList;
					reserveManager.GetTunerReserveAll(&tunerList);

					syoboi.SendReserve(&reserveList, &tunerList);

					for( size_t i=0; i<reserveList.size(); i++ ){
						SAFE_DELETE(reserveList[i]);
					}
					reserveList.clear();
				}
				reloadEpgChkFlag = FALSE;
				this->reserveManager.SendNotifyUpdate(NOTIFY_UPDATE_EPGDATA);

				//リロードタイミングで予約始まったかもしれないのでチェック
				BOOL streamingChk = TRUE;
				if( this->ngFileStreaming == TRUE ){
					if( this->streamingManager.IsStreaming() == TRUE ){
						streamingChk = FALSE;
					}
				}

				BOOL userWorkingChk = TRUE;
				if (this->ngUsePC == TRUE) {
					if (IsUserWorking() == TRUE) {
						userWorkingChk = FALSE;
					}
				}

				if( this->reserveManager.IsSuspendOK() == TRUE && userWorkingChk == TRUE && streamingChk == TRUE){
					if( suspendMode != 0xFF && rebootFlag != 0xFF ){
						//問い合わせ
						if( suspendMode != 0 && suspendMode != 4 ){
							if( QuerySleep(rebootFlag, suspendMode) == FALSE ){
								CBlockLock lock(&this->settingLock);
								if( WaitForSingleObject(&this->sleepEvent, 0) != WAIT_OBJECT_0 ){
									this->suspendModeWork = suspendMode;
									this->rebootFlagWork = rebootFlag;
									SetEvent(this->sleepEvent);
								}
							}
						}
					}
				}
				countChkSuspend = 11;
				suspendMode = 0xFF;
				rebootFlag = 0xFF;
			}
		}
		//予約終了後の動作チェック
		if( this->reserveManager.IsEnableSuspend(&suspendMode, &rebootFlag ) == TRUE ){
			OutputDebugString(L"★IsEnableSuspend");
			SetEvent(this->reloadEpgChkEvent);
			this->epgDB.ReloadEpgData();
		}else{
			if( this->reserveManager.IsEnableReloadEPG() == TRUE ){
				SetEvent(this->reloadEpgChkEvent);
				this->epgDB.ReloadEpgData();
			}
		}

		if( this->reserveManager.IsRecInfoChg() == TRUE ){
			AddRecFileDMS();
		}

		if( countChkSuspend > 10 ){
			BOOL streamingChk = TRUE;
			if( this->ngFileStreaming == TRUE ){
				if( this->streamingManager.IsStreaming() == TRUE ){
					streamingChk = FALSE;
				}
			}
			if( this->reserveManager.IsSuspendOK() == FALSE || streamingChk == FALSE){
				DWORD esMode = ES_SYSTEM_REQUIRED|ES_CONTINUOUS;
				if( this->awayMode == TRUE ){
					esMode |= ES_AWAYMODE_REQUIRED;
				}
				SetThreadExecutionState(esMode);
			}else{
				SetThreadExecutionState(ES_CONTINUOUS);
			}
			countChkSuspend = 0;
		}
		countChkSuspend++;

		SetResumeTimer(&resumeTimer, &resumeTime, FALSE);
	}

	if( resumeTimer != NULL ){
		CloseHandle(resumeTimer);
	}

	tcpSrvUtil.StopServer();
	httpServer.StopServer();
	tcpServer.StopServer();
	pipeServer.StopServer();
}

BOOL CEpgTimerSrvMain::SetResumeTimer(HANDLE* resumeTimer, LONGLONG* resumeTime, BOOL rebootFlag)
{
	BOOL ret = TRUE;
	LONGLONG returnTime = 0;
	if( this->reserveManager.GetSleepReturnTime(&returnTime) == TRUE ){
		CBlockLock lock(&this->settingLock);
		ret = FALSE;
		//rebootFlag時は(指定+5分前)に復帰
		LONGLONG setTime = returnTime - 60 * I64_1SEC * (this->wakeMargin + (rebootFlag ? 5 : 0));
		if( setTime > GetNowI64Time() ){
			if( *resumeTimer != NULL && *resumeTime == setTime ){
				//同時刻でセット済み
				ret = TRUE;
			}else{
				if( *resumeTimer == NULL ){
					*resumeTimer = CreateWaitableTimer(NULL, FALSE, NULL);
				}
				if( *resumeTimer != NULL ){
					FILETIME locTime;
					locTime.dwLowDateTime = (DWORD)setTime;
					locTime.dwHighDateTime = (DWORD)(setTime >> 32);
					FILETIME utcTime = {};
					LocalFileTimeToFileTime(&locTime, &utcTime);
					LARGE_INTEGER liTime;
					liTime.QuadPart = (LONGLONG)utcTime.dwHighDateTime << 32 | utcTime.dwLowDateTime;
					if( SetWaitableTimer(*resumeTimer, &liTime, 0, NULL, NULL, TRUE) != FALSE ){
						*resumeTime = setTime;
						ret = TRUE;
					}else{
						CloseHandle(*resumeTimer);
						*resumeTimer = NULL;
					}
				}
			}
		}else if( *resumeTimer != NULL ){
			CloseHandle(*resumeTimer);
			*resumeTimer = NULL;
		}
	}else if( *resumeTimer != NULL ){
		CloseHandle(*resumeTimer);
		*resumeTimer = NULL;
	}
	return ret;
}

void CEpgTimerSrvMain::ResetServer(CTCPServer& tcpServer, CHttpServer& httpServer, CTCPServerUtil& tcpSrvUtil)
{
	BOOL enableTCPSrv_;
	DWORD tcpPort_;
	BOOL enableHttpSrv_;
	DWORD httpPort_;
	BOOL enableDMS_;
	{
		CBlockLock lock(&this->settingLock);
		enableTCPSrv_ = this->enableTCPSrv;
		tcpPort_ = this->tcpPort;
		enableHttpSrv_ = this->enableHttpSrv;
		httpPort_ = this->httpPort;
		enableDMS_ = this->enableDMS;
	}
	if( enableTCPSrv_ == FALSE ){
		tcpServer.StopServer();
	}else{
		tcpServer.StartServer(tcpPort_, CtrlCmdCallback, this, 0, GetCurrentProcessId());
	}
	if( enableHttpSrv_ == FALSE ){
		httpServer.StopServer();
	}else{
		httpServer.StartServer(httpPort_, HttpCallback, this, 0, GetCurrentProcessId());
	}
	if( enableDMS_ == FALSE ){
		tcpSrvUtil.StopServer();
		{
			CBlockLock lock(&this->settingLock);
			if( dlnaManager != NULL ){
				dlnaManager->StopDMS();
				dlnaManager->StopSSDPServer();
				SAFE_DELETE(dlnaManager);
			}
		}
	}else{
		{
			CBlockLock lock(&this->settingLock);
			if( dlnaManager == NULL ){
				dlnaManager = new CDLNAManager;
				dlnaManager->StartSSDPServer(httpPort_ + 1);
				dlnaManager->LoadPublicFolder();
				AddRecFileDMS();
				dlnaManager->StartDMS();
			}else{
				dlnaManager->LoadPublicFolder();
				AddRecFileDMS();
			}
		}
		tcpSrvUtil.StartServer(httpPort_ + 1, TcpAcceptCallback, this);
	}
}

void CEpgTimerSrvMain::ReloadSetting()
{
	CBlockLock lock(&this->settingLock);

	wstring iniPath = L"";
	GetModuleIniPath(iniPath);
	this->enableTCPSrv = GetPrivateProfileInt(L"SET", L"EnableTCPSrv", 0, iniPath.c_str());
	this->tcpPort = GetPrivateProfileInt(L"SET", L"TCPPort", 4510, iniPath.c_str());
	this->enableHttpSrv = GetPrivateProfileInt(L"SET", L"EnableHttpSrv", 0, iniPath.c_str());
	this->httpPort = GetPrivateProfileInt(L"SET", L"HttpPort", 5510, iniPath.c_str());
	this->enableDMS = GetPrivateProfileInt(L"SET", L"EnableDMS", 0, iniPath.c_str());
	SetEvent(this->resetServerEvent);

	this->wakeMargin = GetPrivateProfileInt(L"SET", L"WakeTime", 5, iniPath.c_str());
	this->autoAddDays = GetPrivateProfileInt(L"SET", L"AutoAddDays", 8, iniPath.c_str());
	this->autoAddHour = GetPrivateProfileInt(L"SET", L"AutoAddHour", 0, iniPath.c_str());
	this->chkGroupEvent = GetPrivateProfileInt(L"SET", L"ChkGroupEvent", 1, iniPath.c_str());
	this->rebootDef = (BYTE)GetPrivateProfileInt(L"SET", L"Reboot", 0, iniPath.c_str());
	this->ngUsePC = GetPrivateProfileInt(L"NO_SUSPEND", L"NoUsePC", 0, iniPath.c_str());
	this->ngUsePCTime = GetPrivateProfileInt(L"NO_SUSPEND", L"NoUsePCTime", 3, iniPath.c_str());
	this->ngFileStreaming = (BYTE)GetPrivateProfileInt(L"NO_SUSPEND", L"NoFileStreaming", 0, iniPath.c_str());
	if( GetPrivateProfileInt(L"SET", L"UseSrvCoop", 0, iniPath.c_str()) == 1 ){
		this->ngEpgFileSrvCoop = GetPrivateProfileInt(L"SET", L"NgEpgFileSrvCoop", 0, iniPath.c_str());
	}else{
		this->ngEpgFileSrvCoop = TRUE;
	}

	WCHAR buff[512] = L"";
	GetPrivateProfileString( L"SET", L"HttpPublicFolder", L"", buff, 512, iniPath.c_str() );
	this->httpPublicFolder = buff;
	if( this->httpPublicFolder.size() == 0 ){
		GetModuleFolderPath(this->httpPublicFolder);
		this->httpPublicFolder += L"\\httpPublic";
	}
	this->enableHttpPublic = GetPrivateProfileInt(L"SET", L"EnableHttpPublic", 0, iniPath.c_str());
}

//メイン処理停止
void CEpgTimerSrvMain::StopMain()
{
	this->epgDB.CancelLoadData();
	if( this->stopEvent != NULL ){
		SetEvent(this->stopEvent);
	}
}

void CEpgTimerSrvMain::SetShutdown(BYTE shutdownMode)
{
	TOKEN_PRIVILEGES TokenPri;
	HANDLE hToken;
	if ( OpenProcessToken(GetCurrentProcess(),(TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY),&hToken) ){
		LookupPrivilegeValue(NULL,SE_SHUTDOWN_NAME,&TokenPri.Privileges[0].Luid);

		TokenPri.PrivilegeCount = 1;
		TokenPri.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		AdjustTokenPrivileges( hToken, FALSE, &TokenPri, 0, NULL, NULL );
		CloseHandle(hToken);
	}
	if( shutdownMode == 1 ){
		SetSystemPowerState(TRUE, FALSE);
	}else if( shutdownMode == 2 ){
		SetSystemPowerState(FALSE, FALSE);
	}else if( shutdownMode == 3 ){
		ExitWindowsEx(EWX_POWEROFF, 0);
	}else if( shutdownMode == 4 ){
		ExitWindowsEx(EWX_REBOOT, 0);
	}
}

BOOL CEpgTimerSrvMain::QuerySleep(BYTE rebootFlag, BYTE suspendMode)
{
	CSendCtrlCmd sendCtrl;
	BOOL ret = FALSE;

	map<DWORD,DWORD>::iterator itrReg;
	map<DWORD,DWORD> registGUI;
	notifyManager.GetRegistGUI(&registGUI);

	for( itrReg = registGUI.begin(); itrReg != registGUI.end(); itrReg++){
		if( _FindOpenExeProcess(itrReg->first) == TRUE ){
			sendCtrl.SetPipeSetting(CMD2_GUI_CTRL_WAIT_CONNECT, CMD2_GUI_CTRL_PIPE, itrReg->first);
			sendCtrl.SetConnectTimeOut(5*1000);
			if( sendCtrl.SendGUIQuerySuspend(rebootFlag, suspendMode) == CMD_SUCCESS ){
				ret = TRUE;
				break;
			}
		}
	}

	return ret;
}

BOOL CEpgTimerSrvMain::QueryReboot(BYTE rebootFlag)
{
	CSendCtrlCmd sendCtrl;
	BOOL ret = FALSE;

	map<DWORD,DWORD>::iterator itrReg;
	map<DWORD,DWORD> registGUI;
	notifyManager.GetRegistGUI(&registGUI);

	for( itrReg = registGUI.begin(); itrReg != registGUI.end(); itrReg++){
		if( _FindOpenExeProcess(itrReg->first) == TRUE ){
			sendCtrl.SetPipeSetting(CMD2_GUI_CTRL_WAIT_CONNECT, CMD2_GUI_CTRL_PIPE, itrReg->first);
			sendCtrl.SetConnectTimeOut(5*1000);
			if( sendCtrl.SendGUIQueryReboot(rebootFlag) == CMD_SUCCESS ){
				ret = TRUE;
				break;
			}
		}
	}

	return ret;
}

//休止／スタンバイ移行処理中かどうか
//戻り値：
// TRUE（移行中）、FALSE
BOOL CEpgTimerSrvMain::IsSuspending()
{
	return WaitForSingleObject(this->sleepEvent, 0) == WAIT_OBJECT_0 ? TRUE : FALSE;
}

//休止／スタンバイに移行して構わない状況かどうか
//戻り値：
// TRUE（構わない）、FALSE（移行しては駄目）
BOOL CEpgTimerSrvMain::ChkSuspend()
{
	BOOL ret = FALSE;
	BOOL streamingChk = TRUE;
	if( this->ngFileStreaming == TRUE ){
		if( this->streamingManager.IsStreaming() == TRUE ){
			streamingChk = FALSE;
		}
	}
	if( streamingChk == TRUE ){
		ret = this->reserveManager.IsSuspendOK();
	}
	return ret;
}

BOOL CEpgTimerSrvMain::CheckTuijyu()
{
	BOOL ret = FALSE;

	wstring iniAppPath = L"";
	GetModuleIniPath(iniAppPath);

	BOOL chgTitle = GetPrivateProfileInt(L"SET", L"ResAutoChgTitle", 1, iniAppPath.c_str());
	BOOL chkTime = GetPrivateProfileInt(L"SET", L"ResAutoChkTime", 1, iniAppPath.c_str());

	vector<RESERVE_DATA*> reserveList;
	vector<RESERVE_DATA> chgList;
	this->reserveManager.GetReserveDataAll(&reserveList);
	for( size_t i=0; i<reserveList.size(); i++ ){
		if( reserveList[i]->recSetting.recMode == RECMODE_NO ){
			continue;
		}
		if( reserveList[i]->eventID == 0xFFFF ){
			continue;
		}
		if( reserveList[i]->recSetting.tuijyuuFlag == 0 ){
			continue;
		}
		if( reserveList[i]->reserveStatus != ADD_RESERVE_NORMAL ){
			continue;
		}
		if( ConvertI64Time(reserveList[i]->startTime) < GetNowI64Time() + I64_1SEC*15 ){
			//録画開始15秒前のものはチェックしない
			continue;
		}

		RESERVE_DATA oldData = *(reserveList[i]);
		EPGDB_EVENT_INFO info;
		if( this->epgDB.SearchEpg(
			reserveList[i]->originalNetworkID,
			reserveList[i]->transportStreamID,
			reserveList[i]->serviceID,
			reserveList[i]->eventID,
			&info
			) == TRUE){

				BOOL chgRes = FALSE;
				if( info.StartTimeFlag == 1 ){
					if( ConvertI64Time(reserveList[i]->startTime) != ConvertI64Time(info.start_time) ){
						reserveList[i]->startTime = info.start_time;
						chgRes = TRUE;
					}
				}
				if( info.DurationFlag == 1 ){
					if( reserveList[i]->durationSecond != info.durationSec ){
						reserveList[i]->durationSecond = info.durationSec;
						chgRes = TRUE;
					}
				}
				if( chgTitle == TRUE ){
					if( info.shortInfo != NULL ){
						if( CompareNoCase(reserveList[i]->title, info.shortInfo->event_name) != 0 ){
							reserveList[i]->title = info.shortInfo->event_name;
							chgRes = TRUE;
						}
					}
				}
				if( chgRes == TRUE ){
					chgList.push_back(*(reserveList[i]));
					this->reserveManager.SendTweet(TW_CHG_RESERVE_RELOADEPG, &oldData, reserveList[i], NULL);
					this->reserveManager.SendNotifyChgReserveAutoAdd(&oldData, reserveList[i]);
				}
		}else{
			//IDで見つからなかったので時間で検索してみる
			if( this->epgDB.SearchEpg(
				reserveList[i]->originalNetworkID,
				reserveList[i]->transportStreamID,
				reserveList[i]->serviceID,
				ConvertI64Time(reserveList[i]->startTime),
				reserveList[i]->durationSecond,
				&info
				) == TRUE){

					reserveList[i]->eventID = info.event_id;

					if( chkTime == FALSE ){
						//番組名も同じか確認
						if( info.shortInfo != NULL ){
							if( CompareNoCase(reserveList[i]->title, info.shortInfo->event_name) == 0 ){
								chgList.push_back(*(reserveList[i]));

								this->reserveManager.SendTweet(TW_CHG_RESERVE_RELOADEPG, &oldData, reserveList[i], NULL);
							}
						}
					}else{
						//時間のみで判断
						if( chgTitle == TRUE ){
							if( info.shortInfo != NULL ){
								if( CompareNoCase(reserveList[i]->title, info.shortInfo->event_name) != 0 ){
									reserveList[i]->title = info.shortInfo->event_name;
								}
							}
						}
						chgList.push_back(*(reserveList[i]));

						this->reserveManager.SendTweet(TW_CHG_RESERVE_RELOADEPG, &oldData, reserveList[i], NULL);

					}
			}
		}
	}
	if( chgList.size() > 0 ){
		this->reserveManager.ChgReserveData(&chgList, TRUE);
		ret = TRUE;
	}
	for( size_t i=0; i<reserveList.size(); i++ ){
		SAFE_DELETE(reserveList[i]);
	}

	return ret;
}

BOOL CEpgTimerSrvMain::IsUserWorking()
{
	CBlockLock lock(&this->settingLock);

	if (this->ngUsePCTime == 0) {
		return TRUE;	// 閾値が0のときは常に使用中扱い
	}

	// 最終入力時刻取得
	LASTINPUTINFO lii;
	lii.cbSize = sizeof(LASTINPUTINFO);
	GetLastInputInfo(&lii);

	// 現在時刻取得
	ULONGLONG dwNow = GetTickCount();

	// GetTickCount()は49.7日周期でリセットされるので桁上りさせる
	if (lii.dwTime > dwNow) {
		dwNow += 0x100000000;
	}

	DWORD threshold = this->ngUsePCTime * 60 * 1000;
	return (dwNow - lii.dwTime < threshold);
}

BOOL CEpgTimerSrvMain::AutoAddReserveEPG(int targetSize, EPG_AUTO_ADD_DATA* target)
{
	BOOL ret = TRUE;

	map<ULONGLONG, RESERVE_DATA*> addMap;
	map<ULONGLONG, RESERVE_DATA*>::iterator itrAdd;

	LONGLONG nowTime = GetNowI64Time();
	BOOL chgRecEnd = FALSE;

	{ //CBlockLock
	CBlockLock lock(&this->settingLock);

	map<DWORD, EPG_AUTO_ADD_DATA>::const_iterator itrKey;
	for( itrKey = this->epgAutoAdd.GetMap().begin(); itrKey != this->epgAutoAdd.GetMap().end(); itrKey++ ){
		if( targetSize >= 0 ){
			BOOL findData = FALSE;
			for( int i=0; i < targetSize; i++ ){
				if( target[i].dataID == itrKey->second.dataID ){
					findData = TRUE;
					break;
				}
			}
			if( findData == FALSE ){
				continue;
			}
		}

		this->epgAutoAdd.SetAddCount(itrKey->first, 0);

		vector<unique_ptr<CEpgDBManager::SEARCH_RESULT_EVENT_DATA>> resultList;
		vector<EPGDB_SEARCH_KEY_INFO> key(1, itrKey->second.searchInfo);
		this->epgDB.SearchEpg(&key, &resultList);
		for( size_t i=0; i<resultList.size(); i++ ){
			EPGDB_EVENT_INFO* result = &resultList[i]->info;
			if( result->StartTimeFlag == 0 || result->DurationFlag == 0 ){
				//時間未定なので対象外
				continue;
			}
			if( ConvertI64Time(result->start_time) < nowTime ){
				//開始時間過ぎているので対象外
				continue;
			}
			if( nowTime + ((LONGLONG)this->autoAddDays)*24*60*60*I64_1SEC + ((LONGLONG)this->autoAddHour)*60*60*I64_1SEC < ConvertI64Time(result->start_time)){
				//対象期間外
				continue;
			}

			this->epgAutoAdd.SetAddCount(itrKey->first, itrKey->second.addCount + 1);

			if(this->reserveManager.IsFindReserve(
				result->original_network_id,
				result->transport_stream_id,
				result->service_id,
				result->event_id
				) == FALSE ){
					ULONGLONG eventKey = _Create64Key2(
						result->original_network_id,
						result->transport_stream_id,
						result->service_id,
						result->event_id
						);

					itrAdd = addMap.find(eventKey);
					if( itrAdd == addMap.end() ){
						//まだ存在しないので追加対象
						if(result->eventGroupInfo != NULL && this->chkGroupEvent == TRUE){
							//イベントグループのチェックをする
							BOOL findGroup = FALSE;
							for(size_t j=0; j<result->eventGroupInfo->eventDataList.size(); j++ ){
								EPGDB_EVENT_DATA groupData = result->eventGroupInfo->eventDataList[j];
								if(this->reserveManager.IsFindReserve(
									groupData.original_network_id,
									groupData.transport_stream_id,
									groupData.service_id,
									groupData.event_id
									) == TRUE ){
										findGroup = TRUE;
										break;
								}
					
								ULONGLONG eventKey = _Create64Key2(
									groupData.original_network_id,
									groupData.transport_stream_id,
									groupData.service_id,
									groupData.event_id
									);

								itrAdd = addMap.find(eventKey);
								if( itrAdd != addMap.end() ){
									findGroup = TRUE;
									break;
								}
							}
							if( findGroup == TRUE ){
								continue;
							}
						}
						//まだ存在しないので追加対象
						RESERVE_DATA* addItem = new RESERVE_DATA;
						if( result->shortInfo != NULL ){
							addItem->title = result->shortInfo->event_name;
						}
						addItem->startTime = result->start_time;
						addItem->startTimeEpg = result->start_time;
						addItem->durationSecond = result->durationSec;
						this->epgDB.SearchServiceName(
							result->original_network_id,
							result->transport_stream_id,
							result->service_id,
							addItem->stationName
							);
						addItem->originalNetworkID = result->original_network_id;
						addItem->transportStreamID = result->transport_stream_id;
						addItem->serviceID = result->service_id;
						addItem->eventID = result->event_id;

						addItem->recSetting = itrKey->second.recSetting;
						if( itrKey->second.searchInfo.chkRecEnd == 1 ){
							if( this->reserveManager.IsFindRecEventInfo(result, itrKey->second.searchInfo.chkRecDay) == TRUE ){
								addItem->recSetting.recMode = RECMODE_NO;
							}
						}
						if( resultList[i]->findKey.size() > 0 ){
							Format(addItem->comment, L"EPG自動予約(%s)", resultList[i]->findKey.c_str());
							Replace(addItem->comment, L"\r", L"");
							Replace(addItem->comment, L"\n", L"");
						}else{
							addItem->comment = L"EPG自動予約";
						}

						addMap.insert(pair<ULONGLONG, RESERVE_DATA*>(eventKey, addItem));
					}else{
						//無効ならそれを優先
						if( itrKey->second.recSetting.recMode == RECMODE_NO ){
							itrAdd->second->recSetting.recMode = RECMODE_NO;
						}
					}
			}else if( itrKey->second.searchInfo.chkRecEnd == 1 ){
				if( this->reserveManager.IsFindRecEventInfo(result, itrKey->second.searchInfo.chkRecDay) == TRUE ){
					this->reserveManager.ChgAutoAddNoRec(result);
					chgRecEnd = TRUE;
				}
			}
		}
	}

	} //CBlockLock

	vector<RESERVE_DATA> setList;
	for( itrAdd = addMap.begin(); itrAdd != addMap.end(); itrAdd++ ){
		setList.push_back(*(itrAdd->second));
		SAFE_DELETE(itrAdd->second);
	}
	addMap.clear();
	if( setList.size() > 0 ){
		this->reserveManager.AddReserveData(&setList, TRUE);
		setList.clear();
	}else if(chgRecEnd == TRUE){
		this->reserveManager.SendNotifyUpdate(NOTIFY_UPDATE_RESERVE_INFO);
	}
	this->reserveManager.SendNotifyUpdate(NOTIFY_UPDATE_AUTOADD_EPG);


	return ret;
}

BOOL CEpgTimerSrvMain::AutoAddReserveProgram()
{
	BOOL ret = TRUE;

	vector<RESERVE_DATA> setList;
	vector<RESERVE_DATA*> reserveList;

	SYSTEMTIME nowTime;
	GetLocalTime(&nowTime);
	LONGLONG now = GetNowI64Time();

	SYSTEMTIME baseTime = nowTime;
	baseTime.wHour = 0;
	baseTime.wMinute = 0;
	baseTime.wSecond = 0;
	baseTime.wMilliseconds = 0;

	LONGLONG baseStartTime = ConvertI64Time(baseTime);

	this->reserveManager.GetReserveDataAll(&reserveList);

	{ //CBlockLock
	CBlockLock lock(&this->settingLock);

	map<DWORD, MANUAL_AUTO_ADD_DATA>::const_iterator itr;
	for( itr = this->manualAutoAdd.GetMap().begin(); itr != this->manualAutoAdd.GetMap().end(); itr++){
		BYTE weekChkFlag = (BYTE)(1<<nowTime.wDayOfWeek);
		for( BYTE i=0; i<8; i++ ){
			if( (itr->second.dayOfWeekFlag & weekChkFlag) != 0 ){
				LONGLONG startTime = baseStartTime + ((LONGLONG)itr->second.startTime) * I64_1SEC + (((LONGLONG)i) * 24*60*60*I64_1SEC);

				if( startTime > now ){
					//時間的に予約追加候補
					BOOL find = FALSE;
					for( size_t j=0; j<reserveList.size(); j++ ){
						//同一時間の予約がすでにあるかチェック
						if( reserveList[j]->eventID != 0xFFFF ){
							continue;
						}
						if( reserveList[j]->originalNetworkID != itr->second.originalNetworkID ||
							reserveList[j]->transportStreamID != itr->second.transportStreamID ||
							reserveList[j]->serviceID != itr->second.serviceID 
							){
							continue;
						}
						if( ConvertI64Time(reserveList[j]->startTime) == startTime &&
							reserveList[j]->durationSecond == itr->second.durationSecond
							){
							find = TRUE;
							break;
						}
					}
					if( find == FALSE ){
						//見つからなかったので予約追加
						RESERVE_DATA item;
						item.title = itr->second.title;
						ConvertSystemTime(startTime, &item.startTime); 
						item.startTimeEpg = item.startTime;
						item.durationSecond = itr->second.durationSecond;
						item.stationName = itr->second.stationName;
						item.originalNetworkID = itr->second.originalNetworkID;
						item.transportStreamID = itr->second.transportStreamID;
						item.serviceID = itr->second.serviceID;
						item.eventID = 0xFFFF;
						item.recSetting = itr->second.recSetting;

						setList.push_back(item);
					}
				}
			}

			weekChkFlag = weekChkFlag<<1;
			if( weekChkFlag == 0x80){
				weekChkFlag = 1;
			}
		}
	}

	} //CBlockLock

	if( setList.size() > 0 ){
		this->reserveManager.AddReserveData(&setList);
	}
	for( size_t i=0; i<reserveList.size(); i++ ){
		SAFE_DELETE(reserveList[i]);
	}
	reserveList.clear();

	return ret;
}

static void SearchPgCallback(vector<CEpgDBManager::SEARCH_RESULT_EVENT>* pval, void* param)
{
	vector<EPGDB_EVENT_INFO*> valp;
	for( size_t i = 0; i < pval->size(); i++ ){
		valp.push_back((*pval)[i].info);
	}
	CMD_STREAM *resParam = (CMD_STREAM*)param;
	resParam->param = CMD_SUCCESS;
	resParam->dataSize = GetVALUESize(&valp);
	resParam->data = new BYTE[resParam->dataSize];
	if( WriteVALUE(&valp, resParam->data, resParam->dataSize, NULL) == FALSE ){
		_OutputDebugString(L"err Write res CMD2_EPG_SRV_SEARCH_PG\r\n");
		resParam->dataSize = 0;
		resParam->param = CMD_ERR;
	}
}

static void EnumPgInfoCallback(vector<EPGDB_EVENT_INFO*>* pval, void* param)
{
	CMD_STREAM *resParam = (CMD_STREAM*)param;
	resParam->param = CMD_SUCCESS;
	resParam->dataSize = GetVALUESize(pval);
	resParam->data = new BYTE[resParam->dataSize];
	if( WriteVALUE(pval, resParam->data, resParam->dataSize, NULL) == FALSE ){
		_OutputDebugString(L"err Write res CMD2_EPG_SRV_ENUM_PG_INFO\r\n");
		resParam->dataSize = 0;
		resParam->param = CMD_ERR;
	}
}

static void EnumPgAllCallback(vector<EPGDB_SERVICE_EVENT_INFO>* pval, void* param)
{
	vector<EPGDB_SERVICE_EVENT_INFO*> valp;
	for( size_t i = 0; i < pval->size(); i++ ){
		valp.push_back(&(*pval)[i]);
	}
	CMD_STREAM *resParam = (CMD_STREAM*)param;
	resParam->param = CMD_SUCCESS;
	resParam->dataSize = GetVALUESize(&valp);
	resParam->data = new BYTE[resParam->dataSize];
	if( WriteVALUE(&valp, resParam->data, resParam->dataSize, NULL) == FALSE ){
		_OutputDebugString(L"err Write res CMD2_EPG_SRV_ENUM_PG_ALL\r\n");
		resParam->dataSize = 0;
		resParam->param = CMD_ERR;
	}
}

int CALLBACK CEpgTimerSrvMain::CtrlCmdCallback(void* param, CMD_STREAM* cmdParam, CMD_STREAM* resParam)
{
	CEpgTimerSrvMain* sys = (CEpgTimerSrvMain*)param;

	resParam->dataSize = 0;
	resParam->param = CMD_ERR;


	switch( cmdParam->param ){
	case CMD2_EPG_SRV_RELOAD_EPG:
		if( sys->epgDB.IsLoadingData() == TRUE ){
			resParam->param = CMD_ERR_BUSY;
		}else{
			{
				if( sys->epgDB.ReloadEpgData() == TRUE ){
					SetEvent(sys->reloadEpgChkEvent);
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD2_EPG_SRV_RELOAD_SETTING:
		{
			resParam->param = CMD_SUCCESS;
			sys->ReloadSetting();
			sys->reserveManager.ReloadSetting();
			sys->reserveManager.ReloadBankMap(TRUE);
		}
		break;
	case CMD2_EPG_SRV_CLOSE:
		{
			sys->StopMain();
			resParam->param = CMD_SUCCESS;
		}
		break;
	case CMD2_EPG_SRV_REGIST_GUI:
		{
			DWORD processID = 0;
			if( ReadVALUE( &processID, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
				resParam->param = CMD_SUCCESS;
				sys->notifyManager.RegistGUI(processID);
				sys->reserveManager.ChangeRegist();
			}
		}
		break;
	case CMD2_EPG_SRV_UNREGIST_GUI:
		{
			DWORD processID = 0;
			if( ReadVALUE( &processID, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
				resParam->param = CMD_SUCCESS;
				sys->notifyManager.UnRegistGUI(processID);
			}
		}
		break;
	case CMD2_EPG_SRV_REGIST_GUI_TCP:
		{
			REGIST_TCP_INFO val;
			if( ReadVALUE( &val, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
				resParam->param = CMD_SUCCESS;
				sys->notifyManager.RegistTCP(val);

				sys->reserveManager.ChangeRegist();
			}
		}
		break;
	case CMD2_EPG_SRV_UNREGIST_GUI_TCP:
		{
			REGIST_TCP_INFO val;
			if( ReadVALUE( &val, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
				resParam->param = CMD_SUCCESS;
				sys->notifyManager.UnRegistTCP(val);
			}
		}
		break;

	case CMD2_EPG_SRV_ENUM_RESERVE:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ENUM_RESERVE");
			{
				vector<RESERVE_DATA*> list;
				if(sys->reserveManager.GetReserveDataAll(&list) == TRUE ){
					resParam->param = CMD_SUCCESS;
					resParam->dataSize = GetVALUESize(&list);
					resParam->data = new BYTE[resParam->dataSize];
					if( WriteVALUE(&list, resParam->data, resParam->dataSize, NULL) == FALSE ){
						_OutputDebugString(L"err Write res CMD2_EPG_SRV_ENUM_RESERVE\r\n");
						resParam->dataSize = 0;
						resParam->param = CMD_ERR;
					}
					for( size_t i=0; i<list.size(); i++ ){
						SAFE_DELETE(list[i]);
					}
					list.clear();
				}
			}
		}
		break;
	case CMD2_EPG_SRV_GET_RESERVE:
		{
			OutputDebugString(L"CMD2_EPG_SRV_GET_RESERVE");
			{
				DWORD reserveID = 0;
				if( ReadVALUE( &reserveID, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
					RESERVE_DATA info;
					if(sys->reserveManager.GetReserveData(reserveID, &info) == TRUE ){
						resParam->param = CMD_SUCCESS;
						resParam->dataSize = GetVALUESize(&info);
						resParam->data = new BYTE[resParam->dataSize];
						if( WriteVALUE(&info, resParam->data, resParam->dataSize, NULL) == FALSE ){
							_OutputDebugString(L"err Write res CMD2_EPG_SRV_GET_RESERVE\r\n");
							resParam->dataSize = 0;
							resParam->param = CMD_ERR;
						}
					}
				}
			}
		}
		break;
	case CMD2_EPG_SRV_ADD_RESERVE:
		{
			{
				vector<RESERVE_DATA> list;
				if( ReadVALUE( &list, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
					if(sys->reserveManager.AddReserveData(&list) == TRUE ){
						resParam->param = CMD_SUCCESS;
					}
				}
			}
		}
		break;
	case CMD2_EPG_SRV_DEL_RESERVE:
		{
			{
				vector<DWORD> list;
				if( ReadVALUE( &list, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
					if(sys->reserveManager.DelReserveData(&list) == TRUE ){
						resParam->param = CMD_SUCCESS;
					}
				}
			}
		}
		break;
	case CMD2_EPG_SRV_CHG_RESERVE:
		{
			{
				vector<RESERVE_DATA> list;
				if( ReadVALUE( &list, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
					if(sys->reserveManager.ChgReserveData(&list) == TRUE ){
						resParam->param = CMD_SUCCESS;
					}
				}
			}
		}
		break;
	case CMD2_EPG_SRV_ENUM_RECINFO:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ENUM_RECINFO");
			{
				vector<REC_FILE_INFO> list;
				if(sys->reserveManager.GetRecFileInfoAll(&list) == TRUE ){
					resParam->param = CMD_SUCCESS;
					resParam->dataSize = GetVALUESize(&list);
					resParam->data = new BYTE[resParam->dataSize];
					if( WriteVALUE(&list, resParam->data, resParam->dataSize, NULL) == FALSE ){
						_OutputDebugString(L"err Write res CMD2_EPG_SRV_ENUM_RECINFO\r\n");
						resParam->dataSize = 0;
						resParam->param = CMD_ERR;
					}
				}
			}
		}
		break;
	case CMD2_EPG_SRV_DEL_RECINFO:
		{
			{
				vector<DWORD> list;
				if( ReadVALUE( &list, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
					if(sys->reserveManager.DelRecFileInfo(&list) == TRUE ){
						resParam->param = CMD_SUCCESS;
					}
				}
			}
		}
		break;
	case CMD2_EPG_SRV_ENUM_SERVICE:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ENUM_SERVICE");
			if( sys->epgDB.IsInitialLoadingDataDone() == FALSE ){
				resParam->param = CMD_ERR_BUSY;
			}else{
				{
					vector<EPGDB_SERVICE_INFO> list;
					if( sys->epgDB.GetServiceList(&list) == TRUE ){
						resParam->param = CMD_SUCCESS;
						resParam->dataSize = GetVALUESize(&list);
						resParam->data = new BYTE[resParam->dataSize];
						if( WriteVALUE(&list, resParam->data, resParam->dataSize, NULL) == FALSE ){
							_OutputDebugString(L"err Write res CMD2_EPG_SRV_ENUM_SERVICE\r\n");
							resParam->dataSize = 0;
							resParam->param = CMD_ERR;
						}
					}
				}
			}
		}
		break;
	case CMD2_EPG_SRV_ENUM_PG_INFO:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ENUM_PG_INFO");
			if( sys->epgDB.IsInitialLoadingDataDone() == FALSE ){
				resParam->param = CMD_ERR_BUSY;
			}else{
				{
					resParam->param = CMD_ERR;
					LONGLONG serviceKey = 0;

					if( ReadVALUE(&serviceKey, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
						sys->epgDB.EnumEventInfo(serviceKey, EnumPgInfoCallback, resParam);
					}
				}
			}
		}
		break;
	case CMD2_EPG_SRV_SEARCH_PG:
		{
			OutputDebugString(L"CMD2_EPG_SRV_SEARCH_PG");
			if( sys->epgDB.IsInitialLoadingDataDone() == FALSE ){
				resParam->param = CMD_ERR_BUSY;
			}else{
				{
					vector<EPGDB_SEARCH_KEY_INFO> key;

					if( ReadVALUE( &key, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
						sys->epgDB.SearchEpg(&key, SearchPgCallback, resParam);
					}
				}
			}
		}
		break;
	case CMD2_EPG_SRV_GET_PG_INFO:
		{
			OutputDebugString(L"CMD2_EPG_SRV_GET_PG_INFO");
			if( sys->epgDB.IsInitialLoadingDataDone() == FALSE ){
				resParam->param = CMD_ERR_BUSY;
			}else{
				{
					ULONGLONG key;
					EPGDB_EVENT_INFO val;

					if( ReadVALUE( &key, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
						WORD ONID = (WORD)(key>>48);
						WORD TSID = (WORD)((key&0x0000FFFF00000000)>>32);
						WORD SID = (WORD)((key&0x00000000FFFF0000)>>16);
						WORD eventID = (WORD)(key&0x000000000000FFFF);
						if( sys->epgDB.SearchEpg(ONID, TSID, SID, eventID, &val) == TRUE ){
							resParam->param = CMD_SUCCESS;
							resParam->dataSize = GetVALUESize(&val);
							resParam->data = new BYTE[resParam->dataSize];
							if( WriteVALUE(&val, resParam->data, resParam->dataSize, NULL) == FALSE ){
								_OutputDebugString(L"err Write res CMD2_EPG_SRV_GET_PG_INFO\r\n");
								resParam->dataSize = 0;
								resParam->param = CMD_ERR;
							}
						}
					}
				}
			}
		}
		break;
	case CMD2_EPG_SRV_CHK_SUSPEND:
		{
			BOOL streamingChk = TRUE;
			if( sys->ngFileStreaming == TRUE ){
				if( sys->streamingManager.IsStreaming() == TRUE ){
					streamingChk = FALSE;
				}
			}
			if( sys->reserveManager.IsSuspendOK() == TRUE && streamingChk == TRUE){
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_SUSPEND:
		{
			WORD val = 0;
			if( ReadVALUE( &val, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
				BYTE reboot = val>>8;
				if( reboot == 0xFF ){
					CBlockLock lock(&sys->settingLock);
					reboot = sys->rebootDef;
				}
				BYTE suspendMode = val&0x00FF;

				BOOL streamingChk = TRUE;
				if( sys->ngFileStreaming == TRUE ){
					if( sys->streamingManager.IsStreaming() == TRUE ){
						streamingChk = FALSE;
					}
				}
				if( sys->reserveManager.IsSuspendOK() == TRUE && streamingChk == TRUE){
					CBlockLock lock(&sys->settingLock);
					if( WaitForSingleObject(&sys->sleepEvent, 0) != WAIT_OBJECT_0 ){
						sys->suspendModeWork = suspendMode;
						sys->rebootFlagWork = reboot;
						SetEvent(sys->sleepEvent);
					}
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD2_EPG_SRV_REBOOT:
		{
			CBlockLock lock(&sys->settingLock);
			if( WaitForSingleObject(&sys->sleepEvent, 0) != WAIT_OBJECT_0 ){
				sys->suspendModeWork = 0xFF;
				sys->rebootFlagWork = 1;
				SetEvent(sys->sleepEvent);
			}
			resParam->param = CMD_SUCCESS;
		}
		break;
	case CMD2_EPG_SRV_EPG_CAP_NOW:
		{
			if( sys->epgDB.IsLoadingData() == TRUE ){
				resParam->param = CMD_ERR_BUSY;
			}else{
				if( sys->reserveManager.StartEpgCap() == TRUE ){
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD2_EPG_SRV_ENUM_AUTO_ADD:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ENUM_AUTO_ADD");
			{
				vector<EPG_AUTO_ADD_DATA> val;
				{
					CBlockLock lock(&sys->settingLock);
					map<DWORD, EPG_AUTO_ADD_DATA>::const_iterator itr;
					for( itr = sys->epgAutoAdd.GetMap().begin(); itr != sys->epgAutoAdd.GetMap().end(); itr++ ){
						val.push_back(itr->second);
					}
				}
				resParam->param = CMD_SUCCESS;
				resParam->dataSize = GetVALUESize(&val);
				resParam->data = new BYTE[resParam->dataSize];
				if( WriteVALUE(&val, resParam->data, resParam->dataSize, NULL) == FALSE ){
					_OutputDebugString(L"err Write res CMD2_EPG_SRV_ENUM_AUTO_ADD\r\n");
					resParam->dataSize = 0;
					resParam->param = CMD_ERR;
				}
			}
		}
		break;
	case CMD2_EPG_SRV_ADD_AUTO_ADD:
		{
			{
				vector<EPG_AUTO_ADD_DATA> val;
				if( ReadVALUE( &val, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
					{
						CBlockLock lock(&sys->settingLock);
						for( size_t i=0; i<val.size(); i++ ){
							val[i].dataID = sys->epgAutoAdd.AddData(val[i]);
						}
						sys->epgAutoAdd.SaveText();
					}

					resParam->param = CMD_SUCCESS;

					sys->AutoAddReserveEPG((int)val.size(), val.data());
				}

				sys->reserveManager.SendNotifyUpdate(NOTIFY_UPDATE_AUTOADD_EPG);

			}

		}
		break;
	case CMD2_EPG_SRV_DEL_AUTO_ADD:
		{
			{
				vector<DWORD> val;
				if( ReadVALUE( &val, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
					{
						CBlockLock lock(&sys->settingLock);
						for( size_t i=0; i<val.size(); i++ ){
							sys->epgAutoAdd.DelData(val[i]);
						}
						sys->epgAutoAdd.SaveText();
					}

					resParam->param = CMD_SUCCESS;
				}

				sys->reserveManager.SendNotifyUpdate(NOTIFY_UPDATE_AUTOADD_EPG);
			}

		}
		break;
	case CMD2_EPG_SRV_CHG_AUTO_ADD:
		{
			{
				vector<EPG_AUTO_ADD_DATA> val;
				if( ReadVALUE( &val, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
					{
						CBlockLock lock(&sys->settingLock);
						for( size_t i=0; i<val.size(); i++ ){
							sys->epgAutoAdd.ChgData(val[i]);
						}
						sys->epgAutoAdd.SaveText();
					}

					resParam->param = CMD_SUCCESS;

					sys->AutoAddReserveEPG((int)val.size(), val.data());
				}

				sys->reserveManager.SendNotifyUpdate(NOTIFY_UPDATE_AUTOADD_EPG);
			}

		}
		break;
	case CMD2_EPG_SRV_ENUM_MANU_ADD:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ENUM_MANU_ADD");
			{
				vector<MANUAL_AUTO_ADD_DATA> val;
				{
					CBlockLock lock(&sys->settingLock);
					map<DWORD, MANUAL_AUTO_ADD_DATA>::const_iterator itr;
					for( itr = sys->manualAutoAdd.GetMap().begin(); itr != sys->manualAutoAdd.GetMap().end(); itr++ ){
						val.push_back(itr->second);
					}
				}
				resParam->param = CMD_SUCCESS;
				resParam->dataSize = GetVALUESize(&val);
				resParam->data = new BYTE[resParam->dataSize];
				if( WriteVALUE(&val, resParam->data, resParam->dataSize, NULL) == FALSE ){
					_OutputDebugString(L"err Write res CMD2_EPG_SRV_ENUM_MANU_ADD\r\n");
					resParam->dataSize = 0;
					resParam->param = CMD_ERR;
				}
			}

		}
		break;
	case CMD2_EPG_SRV_ADD_MANU_ADD:
		{
			{
				vector<MANUAL_AUTO_ADD_DATA> val;
				if( ReadVALUE( &val, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
					{
						CBlockLock lock(&sys->settingLock);
						for( size_t i=0; i<val.size(); i++ ){
							sys->manualAutoAdd.AddData(val[i]);
						}
						sys->manualAutoAdd.SaveText();
					}

					resParam->param = CMD_SUCCESS;

					sys->AutoAddReserveProgram();
				}

				sys->reserveManager.SendNotifyUpdate(NOTIFY_UPDATE_AUTOADD_MANUAL);

			}

		}
		break;
	case CMD2_EPG_SRV_DEL_MANU_ADD:
		{
			{
				vector<DWORD> val;
				if( ReadVALUE( &val, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
					{
						CBlockLock lock(&sys->settingLock);
						for( size_t i=0; i<val.size(); i++ ){
							sys->manualAutoAdd.DelData(val[i]);
						}
						sys->manualAutoAdd.SaveText();
					}

					resParam->param = CMD_SUCCESS;
				}

				sys->reserveManager.SendNotifyUpdate(NOTIFY_UPDATE_AUTOADD_MANUAL);
			}

		}
		break;
	case CMD2_EPG_SRV_CHG_MANU_ADD:
		{
			{
				vector<MANUAL_AUTO_ADD_DATA> val;
				if( ReadVALUE( &val, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
					{
						CBlockLock lock(&sys->settingLock);
						for( size_t i=0; i<val.size(); i++ ){
							sys->manualAutoAdd.ChgData(val[i]);
						}
						sys->manualAutoAdd.SaveText();
					}

					resParam->param = CMD_SUCCESS;

					sys->AutoAddReserveProgram();
				}

				sys->reserveManager.SendNotifyUpdate(NOTIFY_UPDATE_AUTOADD_MANUAL);
			}

		}
		break;
	case CMD2_EPG_SRV_ENUM_TUNER_RESERVE:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ENUM_TUNER_RESERVE");
			vector<TUNER_RESERVE_INFO> list;
			{
				if(sys->reserveManager.GetTunerReserveAll(&list) == TRUE ){
					resParam->param = CMD_SUCCESS;
					resParam->dataSize = GetVALUESize(&list);
					resParam->data = new BYTE[resParam->dataSize];
					if( WriteVALUE(&list, resParam->data, resParam->dataSize, NULL) == FALSE ){
						_OutputDebugString(L"err Write res CMD2_EPG_SRV_ENUM_TUNER_RESERVE\r\n");
						resParam->dataSize = 0;
						resParam->param = CMD_ERR;
					}
				}
			}
		}
		break;
	case CMD2_EPG_SRV_FILE_COPY:
		{
			wstring val;
			if( ReadVALUE( &val, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
				if( CompareNoCase(val, L"ChSet5.txt") == 0){
					wstring path = L"";
					GetSettingPath(path);
					path += L"\\ChSet5.txt";

					HANDLE file = CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
					if( file != INVALID_HANDLE_VALUE){
						DWORD dwFileSize = GetFileSize( file, NULL );
						if( dwFileSize > 0 ){
							resParam->dataSize = dwFileSize;
							resParam->data = new BYTE[resParam->dataSize];

							DWORD dwRead=0;
							ReadFile( file, resParam->data, resParam->dataSize, &dwRead, NULL );
						}
						CloseHandle(file);
						resParam->param = CMD_SUCCESS;
					}
				}
			}
		}
		break;
	case CMD2_EPG_SRV_ENUM_PG_ALL:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ENUM_PG_ALL");
			if( sys->epgDB.IsInitialLoadingDataDone() == FALSE ){
				resParam->param = CMD_ERR_BUSY;
			}else{
				resParam->param = CMD_ERR;
				sys->epgDB.EnumEventAll(EnumPgAllCallback, resParam);
			}
		}
		break;
	case CMD2_EPG_SRV_ENUM_PLUGIN:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ENUM_PLUGIN");
			WORD mode = 0;
			if( ReadVALUE( &mode, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
				if( mode == 1 || mode == 2 ){
					wstring path = L"";
					GetModuleFolderPath(path);

					wstring searchKey = path;

					if( mode == 1 ){
						searchKey += L"\\RecName\\RecName*.dll";
					}else if( mode == 2 ){
						searchKey += L"\\Write\\Write*.dll";
					}

					WIN32_FIND_DATA findData;
					HANDLE find;

					//指定フォルダのファイル一覧取得
					find = FindFirstFile( searchKey.c_str(), &findData);
					if ( find != INVALID_HANDLE_VALUE ) {
						vector<wstring> fileList;
						do{
							if( (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 ){
								//本当に拡張子DLL?
								if( IsExt(findData.cFileName, L".dll") == TRUE ){
									fileList.push_back(findData.cFileName);
								}
							}
						}while(FindNextFile(find, &findData));

						FindClose(find);

						resParam->param = CMD_SUCCESS;
						resParam->dataSize = GetVALUESize(&fileList);
						resParam->data = new BYTE[resParam->dataSize];
						if( WriteVALUE(&fileList, resParam->data, resParam->dataSize, NULL) == FALSE ){
							_OutputDebugString(L"err Write res CMD2_EPG_SRV_ENUM_PLUGIN\r\n");
							resParam->dataSize = 0;
							resParam->param = CMD_ERR;
						}
					}

				}
			}
		}
		break;
	case CMD2_EPG_SRV_GET_CHG_CH_TVTEST:
		{
			OutputDebugString(L"CMD2_EPG_SRV_GET_CHG_CH_TVTEST");
			LONGLONG key = 0;

			if( ReadVALUE(&key, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
				TVTEST_CH_CHG_INFO chInfo;
				if( sys->reserveManager.GetTVTestChgCh(key, &chInfo) == TRUE ){
					resParam->param = CMD_SUCCESS;
					resParam->dataSize = GetVALUESize(&chInfo);
					resParam->data = new BYTE[resParam->dataSize];
					if( WriteVALUE(&chInfo, resParam->data, resParam->dataSize, NULL) == FALSE ){
						_OutputDebugString(L"err Write res CMD2_EPG_SRV_GET_CHG_CH_TVTEST\r\n");
						resParam->dataSize = 0;
						resParam->param = CMD_ERR;
					}
				}
			}
		}
		break;
	case CMD2_EPG_SRV_NWTV_SET_CH:
		{
			OutputDebugString(L"CMD2_EPG_SRV_NWTV_SET_CH");
			SET_CH_INFO val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
				if( sys->reserveManager.SetNWTVCh(&val) == TRUE ){
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD2_EPG_SRV_NWTV_CLOSE:
		{
			OutputDebugString(L"CMD2_EPG_SRV_NWTV_CLOSE");
			if( sys->reserveManager.CloseNWTV() == TRUE ){
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_NWTV_MODE:
		{
			OutputDebugString(L"CMD2_EPG_SRV_NWTV_MODE");
			DWORD val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
				sys->reserveManager.SetNWTVMode(val);
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_NWPLAY_OPEN:
		{
			OutputDebugString(L"CMD2_EPG_SRV_NWPLAY_OPEN");
			wstring val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
				DWORD id=0;
				if( sys->streamingManager.OpenFile(val.c_str(), &id) == TRUE ){
					resParam->param = CMD_SUCCESS;
					resParam->dataSize = GetVALUESize(id);
					resParam->data = new BYTE[resParam->dataSize];
					if( WriteVALUE(id, resParam->data, resParam->dataSize, NULL) == FALSE ){
						_OutputDebugString(L"err Write res CMD2_EPG_SRV_NWPLAY_OPEN\r\n");
						resParam->dataSize = 0;
						resParam->param = CMD_ERR;
					}
				}
			}
		}
		break;
	case CMD2_EPG_SRV_NWPLAY_CLOSE:
		{
			OutputDebugString(L"CMD2_EPG_SRV_NWPLAY_CLOSE");
			DWORD val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
				if( sys->streamingManager.CloseFile(val) == TRUE ){
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD2_EPG_SRV_NWPLAY_PLAY:
		{
			OutputDebugString(L"CMD2_EPG_SRV_NWPLAY_PLAY");
			DWORD val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
				if( sys->streamingManager.StartSend(val) == TRUE ){
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD2_EPG_SRV_NWPLAY_STOP:
		{
			OutputDebugString(L"CMD2_EPG_SRV_NWPLAY_STOP");
			DWORD val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
				if( sys->streamingManager.StopSend(val) == TRUE ){
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD2_EPG_SRV_NWPLAY_GET_POS:
		{
			OutputDebugString(L"CMD2_EPG_SRV_NWPLAY_GET_POS");
			NWPLAY_POS_CMD val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
				if( sys->streamingManager.GetPos(&val) == TRUE ){
					resParam->param = CMD_SUCCESS;
					resParam->dataSize = GetVALUESize(&val);
					resParam->data = new BYTE[resParam->dataSize];
					if( WriteVALUE(&val, resParam->data, resParam->dataSize, NULL) == FALSE ){
						_OutputDebugString(L"err Write res CMD2_EPG_SRV_NWPLAY_GET_POS\r\n");
						resParam->dataSize = 0;
						resParam->param = CMD_ERR;
					}
				}
			}
		}
		break;
	case CMD2_EPG_SRV_NWPLAY_SET_POS:
		{
			OutputDebugString(L"CMD2_EPG_SRV_NWPLAY_SET_POS");
			NWPLAY_POS_CMD val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
				if( sys->streamingManager.SetPos(&val) == TRUE ){
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD2_EPG_SRV_NWPLAY_SET_IP:
		{
			OutputDebugString(L"CMD2_EPG_SRV_NWPLAY_SET_IP");
			NWPLAY_PLAY_INFO val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
				if( sys->streamingManager.SetIP(&val) == TRUE ){
					resParam->param = CMD_SUCCESS;
					resParam->dataSize = GetVALUESize(&val);
					resParam->data = new BYTE[resParam->dataSize];
					if( WriteVALUE(&val, resParam->data, resParam->dataSize, NULL) == FALSE ){
						_OutputDebugString(L"err Write res CMD2_EPG_SRV_NWPLAY_SET_IP\r\n");
						resParam->dataSize = 0;
						resParam->param = CMD_ERR;
					}
				}
			}
		}
		break;
	case CMD2_EPG_SRV_NWPLAY_TF_OPEN:
		{
			OutputDebugString(L"CMD2_EPG_SRV_NWPLAY_TF_OPEN");
			DWORD val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
				NWPLAY_TIMESHIFT_INFO resVal;
				DWORD ctrlID = 0;
				DWORD processID = 0;
				if( sys->reserveManager.GetRecFilePath(val, resVal.filePath, &ctrlID, &processID) == TRUE ){
					if( sys->streamingManager.OpenTimeShift(resVal.filePath.c_str(), processID, ctrlID, &resVal.ctrlID) == TRUE ){
						resParam->param = CMD_SUCCESS;
						resParam->dataSize = GetVALUESize(&resVal);
						resParam->data = new BYTE[resParam->dataSize];
						if( WriteVALUE(&resVal, resParam->data, resParam->dataSize, NULL) == FALSE ){
							_OutputDebugString(L"err Write res CMD2_EPG_SRV_NWPLAY_TF_OPEN\r\n");
							resParam->dataSize = 0;
							resParam->param = CMD_ERR;
						}
					}
				}
			}
		}
		break;

	////////////////////////////////////////////////////////////
	//CMD_VER対応コマンド
	case CMD2_EPG_SRV_ENUM_RESERVE2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ENUM_RESERVE2");
			{
				vector<RESERVE_DATA*> list;
				if(sys->reserveManager.GetReserveDataAll(&list) == TRUE ){
					WORD ver = (WORD)CMD_VER;

					if( ReadVALUE2(ver, &ver, cmdParam->data, cmdParam->dataSize, NULL) == TRUE ){
						DWORD writeSize = 0;
						resParam->param = CMD_SUCCESS;
						resParam->dataSize = GetVALUESize2(ver, &list)+GetVALUESize2(ver, ver);
						resParam->data = new BYTE[resParam->dataSize];
						if( WriteVALUE2(ver, ver, resParam->data, resParam->dataSize, &writeSize) == FALSE ){
							_OutputDebugString(L"err Write res CMD2_EPG_SRV_ENUM_RESERVE2\r\n");
							resParam->dataSize = 0;
							resParam->param = CMD_ERR;
						}else
						if( WriteVALUE2(ver, &list, resParam->data+writeSize, resParam->dataSize-writeSize, NULL) == FALSE ){
							_OutputDebugString(L"err Write res CMD2_EPG_SRV_ENUM_RESERVE2\r\n");
							resParam->dataSize = 0;
							resParam->param = CMD_ERR;
						}
						for( size_t i=0; i<list.size(); i++ ){
							SAFE_DELETE(list[i]);
						}
						list.clear();
					}
				}
			}
		}
		break;
	case CMD2_EPG_SRV_GET_RESERVE2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_GET_RESERVE2");
			{
				WORD ver = (WORD)CMD_VER;
				DWORD readSize = 0;
				if( ReadVALUE2(ver, &ver, cmdParam->data, cmdParam->dataSize, &readSize) == TRUE ){

					DWORD reserveID = 0;
					if( ReadVALUE2(ver, &reserveID, cmdParam->data+readSize, cmdParam->dataSize-readSize, NULL ) == TRUE ){
						RESERVE_DATA info;
						if(sys->reserveManager.GetReserveData(reserveID, &info) == TRUE ){
							DWORD writeSize = 0;
							resParam->param = CMD_SUCCESS;
							resParam->dataSize = GetVALUESize2(ver, &info)+GetVALUESize2(ver, ver);
							resParam->data = new BYTE[resParam->dataSize];
							if( WriteVALUE2(ver, ver, resParam->data, resParam->dataSize, &writeSize) == FALSE ){
								_OutputDebugString(L"err Write res CMD2_EPG_SRV_GET_RESERVE2\r\n");
								resParam->dataSize = 0;
								resParam->param = CMD_ERR;
							}else
							if( WriteVALUE2(ver, &info, resParam->data+writeSize, resParam->dataSize-writeSize, NULL) == FALSE ){
								_OutputDebugString(L"err Write res CMD2_EPG_SRV_GET_RESERVE2\r\n");
								resParam->dataSize = 0;
								resParam->param = CMD_ERR;
							}
						}
					}
				}
			}
		}
		break;
	case CMD2_EPG_SRV_ADD_RESERVE2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ADD_RESERVE2");
			{
				WORD ver = (WORD)CMD_VER;
				DWORD readSize = 0;
				if( ReadVALUE2(ver, &ver, cmdParam->data, cmdParam->dataSize, &readSize) == TRUE ){

					vector<RESERVE_DATA> list;
					if( ReadVALUE2(ver, &list, cmdParam->data+readSize, cmdParam->dataSize-readSize, NULL ) == TRUE ){
						if(sys->reserveManager.AddReserveData(&list) == TRUE ){
							DWORD writeSize = 0;
							resParam->param = CMD_SUCCESS;
							resParam->dataSize = GetVALUESize2(ver, ver);
							resParam->data = new BYTE[resParam->dataSize];
							if( WriteVALUE2(ver, ver, resParam->data, resParam->dataSize, &writeSize) == FALSE ){
								_OutputDebugString(L"err Write res CMD2_EPG_SRV_ADD_RESERVE2\r\n");
								resParam->dataSize = 0;
								resParam->param = CMD_ERR;
							}
						}
					}
				}
			}
		}
		break;
	case CMD2_EPG_SRV_CHG_RESERVE2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_CHG_RESERVE2");
			{
				WORD ver = (WORD)CMD_VER;
				DWORD readSize = 0;
				if( ReadVALUE2(ver, &ver, cmdParam->data, cmdParam->dataSize, &readSize) == TRUE ){
					vector<RESERVE_DATA> list;
					if( ReadVALUE2(ver, &list, cmdParam->data+readSize, cmdParam->dataSize-readSize, NULL ) == TRUE ){
						if(sys->reserveManager.ChgReserveData(&list) == TRUE ){
							DWORD writeSize = 0;
							resParam->param = CMD_SUCCESS;
							resParam->dataSize = GetVALUESize2(ver, ver);
							resParam->data = new BYTE[resParam->dataSize];
							if( WriteVALUE2(ver, ver, resParam->data, resParam->dataSize, &writeSize) == FALSE ){
								_OutputDebugString(L"err Write res CMD2_EPG_SRV_CHG_RESERVE2\r\n");
								resParam->dataSize = 0;
								resParam->param = CMD_ERR;
							}
						}
					}
				}
			}
		}
		break;
	case CMD2_EPG_SRV_ADDCHK_RESERVE2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ADDCHK_RESERVE2");
			{
				WORD ver = (WORD)CMD_VER;
				DWORD readSize = 0;
				if( ReadVALUE2(ver, &ver, cmdParam->data, cmdParam->dataSize, &readSize) == TRUE ){
					RESERVE_DATA reserveInfo;
					if( ReadVALUE2(ver, &reserveInfo, cmdParam->data+readSize, cmdParam->dataSize-readSize, NULL ) == TRUE ){
						WORD chkStatus = 0;
						if(sys->reserveManager.ChkAddReserve(&reserveInfo, &chkStatus) == TRUE ){
							DWORD writeSize = 0;
							resParam->param = CMD_SUCCESS;
							resParam->dataSize = GetVALUESize2(ver, chkStatus)+GetVALUESize2(ver, ver);
							resParam->data = new BYTE[resParam->dataSize];
							if( WriteVALUE2(ver, ver, resParam->data, resParam->dataSize, &writeSize) == FALSE ){
								_OutputDebugString(L"err Write res CMD2_EPG_SRV_ADDCHK_RESERVE2\r\n");
								resParam->dataSize = 0;
								resParam->param = CMD_ERR;
							}else
							if( WriteVALUE2(ver, chkStatus, resParam->data+writeSize, resParam->dataSize-writeSize, NULL) == FALSE ){
								_OutputDebugString(L"err Write res CMD2_EPG_SRV_ADDCHK_RESERVE2\r\n");
								resParam->dataSize = 0;
								resParam->param = CMD_ERR;
							}

						}
					}
				}
			}
		}
		break;
	case CMD2_EPG_SRV_GET_EPG_FILETIME2:
		{
			if(sys->reserveManager.IsEpgCap() == FALSE && sys->ngEpgFileSrvCoop == FALSE ){
				WORD ver = (WORD)CMD_VER;
				DWORD readSize = 0;
				if( ReadVALUE2(ver, &ver, cmdParam->data, cmdParam->dataSize, &readSize) == TRUE ){
					wstring val;
					if( ReadVALUE2(ver, &val, cmdParam->data+readSize, cmdParam->dataSize-readSize, NULL ) == TRUE ){
						wstring epgDataPath = L"";
						GetSettingPath(epgDataPath);
						epgDataPath += EPG_SAVE_FOLDER;
						epgDataPath += L"\\";
						epgDataPath += val;

						HANDLE file = CreateFile(epgDataPath.c_str(), GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
						if( file != INVALID_HANDLE_VALUE){
							FILETIME CreationTime;
							FILETIME LastAccessTime;
							FILETIME LastWriteTime;
							GetFileTime(file, &CreationTime, &LastAccessTime, &LastWriteTime);

							LONGLONG fileTime = ((LONGLONG)LastWriteTime.dwHighDateTime)<<32 | (LONGLONG)LastWriteTime.dwLowDateTime;

							CloseHandle(file);

							DWORD writeSize = 0;
							resParam->param = CMD_SUCCESS;
							resParam->dataSize = GetVALUESize2(ver, fileTime)+GetVALUESize2(ver, ver);
							resParam->data = new BYTE[resParam->dataSize];
							if( WriteVALUE2(ver, ver, resParam->data, resParam->dataSize, &writeSize) == FALSE ){
								_OutputDebugString(L"err Write res CMD2_EPG_SRV_GET_EPG_FILETIME2\r\n");
								resParam->dataSize = 0;
								resParam->param = CMD_ERR;
							}else
							if( WriteVALUE2(ver, fileTime, resParam->data+writeSize, resParam->dataSize-writeSize, NULL) == FALSE ){
								_OutputDebugString(L"err Write res CMD2_EPG_SRV_GET_EPG_FILETIME2\r\n");
								resParam->dataSize = 0;
								resParam->param = CMD_ERR;
							}
						}
					}
				}
			}
		}
		break;
	case CMD2_EPG_SRV_GET_EPG_FILE2:
		{
			if(sys->reserveManager.IsEpgCap() == FALSE && sys->ngEpgFileSrvCoop == FALSE){
				WORD ver = (WORD)CMD_VER;
				DWORD readSize = 0;
				if( ReadVALUE2(ver, &ver, cmdParam->data, cmdParam->dataSize, &readSize) == TRUE ){
					wstring val;
					if( ReadVALUE2(ver, &val, cmdParam->data+readSize, cmdParam->dataSize-readSize, NULL ) == TRUE ){
						wstring epgDataPath = L"";
						GetSettingPath(epgDataPath);
						epgDataPath += EPG_SAVE_FOLDER;
						epgDataPath += L"\\";
						epgDataPath += val;

						HANDLE file = CreateFile(epgDataPath.c_str(), GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
						if( file != INVALID_HANDLE_VALUE){
							DWORD dwFileSize = GetFileSize( file, NULL );
							if( dwFileSize > 0 ){
								resParam->dataSize = dwFileSize+GetVALUESize2(ver, ver);
								resParam->data = new BYTE[resParam->dataSize];
								DWORD writeSize = 0;
								if( WriteVALUE2(ver, ver, resParam->data, resParam->dataSize, &writeSize) == FALSE ){
									_OutputDebugString(L"err Write res CMD2_EPG_SRV_GET_EPG_FILE2\r\n");
									resParam->dataSize = 0;
									resParam->param = CMD_ERR;
								}else{
									DWORD dwRead=0;
									ReadFile( file, resParam->data+writeSize, resParam->dataSize-writeSize, &dwRead, NULL );
								}
							}
							CloseHandle(file);
							resParam->param = CMD_SUCCESS;
						}
					}
				}
			}
		}
		break;
	case CMD2_EPG_SRV_ENUM_AUTO_ADD2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ENUM_AUTO_ADD2");
			{
				vector<EPG_AUTO_ADD_DATA> val;
				{
					CBlockLock lock(&sys->settingLock);
					map<DWORD, EPG_AUTO_ADD_DATA>::const_iterator itr;
					for( itr = sys->epgAutoAdd.GetMap().begin(); itr != sys->epgAutoAdd.GetMap().end(); itr++ ){
						val.push_back(itr->second);
					}
				}
				
				WORD ver = (WORD)CMD_VER;

				if( ReadVALUE2(ver, &ver, cmdParam->data, cmdParam->dataSize, NULL) == TRUE ){
					DWORD writeSize = 0;
					resParam->param = CMD_SUCCESS;
					resParam->dataSize = GetVALUESize2(ver, &val)+GetVALUESize2(ver, ver);
					resParam->data = new BYTE[resParam->dataSize];
					if( WriteVALUE2(ver, ver, resParam->data, resParam->dataSize, &writeSize) == FALSE ){
						_OutputDebugString(L"err Write res CMD2_EPG_SRV_ENUM_AUTO_ADD2\r\n");
						resParam->dataSize = 0;
						resParam->param = CMD_ERR;
					}else
					if( WriteVALUE2(ver, &val, resParam->data+writeSize, resParam->dataSize-writeSize, NULL) == FALSE ){
						_OutputDebugString(L"err Write res CMD2_EPG_SRV_ENUM_AUTO_ADD2\r\n");
						resParam->dataSize = 0;
						resParam->param = CMD_ERR;
					}
				}
			}
		}
		break;
	case CMD2_EPG_SRV_ADD_AUTO_ADD2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ADD_AUTO_ADD2");
			{
				WORD ver = (WORD)CMD_VER;
				DWORD readSize = 0;
				if( ReadVALUE2(ver, &ver, cmdParam->data, cmdParam->dataSize, &readSize) == TRUE ){

					vector<EPG_AUTO_ADD_DATA> list;
					if( ReadVALUE2(ver, &list, cmdParam->data+readSize, cmdParam->dataSize-readSize, NULL ) == TRUE ){
						{
							CBlockLock lock(&sys->settingLock);
							for( size_t i=0; i<list.size(); i++ ){
								list[i].dataID = sys->epgAutoAdd.AddData(list[i]);
							}
							sys->epgAutoAdd.SaveText();
						}

						resParam->param = CMD_SUCCESS;

						sys->AutoAddReserveEPG((int)list.size(), list.data());

						DWORD writeSize = 0;
						resParam->param = CMD_SUCCESS;
						resParam->dataSize = GetVALUESize2(ver, ver);
						resParam->data = new BYTE[resParam->dataSize];
						if( WriteVALUE2(ver, ver, resParam->data, resParam->dataSize, &writeSize) == FALSE ){
							_OutputDebugString(L"err Write res CMD2_EPG_SRV_ADD_AUTO_ADD2\r\n");
							resParam->dataSize = 0;
							resParam->param = CMD_ERR;
						}
					}
				}

				sys->reserveManager.SendNotifyUpdate(NOTIFY_UPDATE_AUTOADD_EPG);
			}
		}
		break;
	case CMD2_EPG_SRV_CHG_AUTO_ADD2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_CHG_AUTO_ADD2");
			{
				WORD ver = (WORD)CMD_VER;
				DWORD readSize = 0;
				if( ReadVALUE2(ver, &ver, cmdParam->data, cmdParam->dataSize, &readSize) == TRUE ){

					vector<EPG_AUTO_ADD_DATA> list;
					if( ReadVALUE2(ver, &list, cmdParam->data+readSize, cmdParam->dataSize-readSize, NULL ) == TRUE ){
						{
							CBlockLock lock(&sys->settingLock);
							for( size_t i=0; i<list.size(); i++ ){
								sys->epgAutoAdd.ChgData(list[i]);
							}
							sys->epgAutoAdd.SaveText();
						}

						resParam->param = CMD_SUCCESS;

						sys->AutoAddReserveEPG((int)list.size(), list.data());

						DWORD writeSize = 0;
						resParam->param = CMD_SUCCESS;
						resParam->dataSize = GetVALUESize2(ver, ver);
						resParam->data = new BYTE[resParam->dataSize];
						if( WriteVALUE2(ver, ver, resParam->data, resParam->dataSize, &writeSize) == FALSE ){
							_OutputDebugString(L"err Write res CMD2_EPG_SRV_CHG_AUTO_ADD2\r\n");
							resParam->dataSize = 0;
							resParam->param = CMD_ERR;
						}
					}
				}

				sys->reserveManager.SendNotifyUpdate(NOTIFY_UPDATE_AUTOADD_EPG);
			}
		}
		break;
	case CMD2_EPG_SRV_ENUM_MANU_ADD2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ENUM_MANU_ADD2");
			{
				vector<MANUAL_AUTO_ADD_DATA> val;
				{
					CBlockLock lock(&sys->settingLock);
					map<DWORD, MANUAL_AUTO_ADD_DATA>::const_iterator itr;
					for( itr = sys->manualAutoAdd.GetMap().begin(); itr != sys->manualAutoAdd.GetMap().end(); itr++ ){
						val.push_back(itr->second);
					}
				}
				
				WORD ver = (WORD)CMD_VER;

				if( ReadVALUE2(ver, &ver, cmdParam->data, cmdParam->dataSize, NULL) == TRUE ){
					DWORD writeSize = 0;
					resParam->param = CMD_SUCCESS;
					resParam->dataSize = GetVALUESize2(ver, &val)+GetVALUESize2(ver, ver);
					resParam->data = new BYTE[resParam->dataSize];
					if( WriteVALUE2(ver, ver, resParam->data, resParam->dataSize, &writeSize) == FALSE ){
						_OutputDebugString(L"err Write res CMD2_EPG_SRV_ENUM_MANU_ADD2\r\n");
						resParam->dataSize = 0;
						resParam->param = CMD_ERR;
					}else
					if( WriteVALUE2(ver, &val, resParam->data+writeSize, resParam->dataSize-writeSize, NULL) == FALSE ){
						_OutputDebugString(L"err Write res CMD2_EPG_SRV_ENUM_MANU_ADD2\r\n");
						resParam->dataSize = 0;
						resParam->param = CMD_ERR;
					}
				}
			}
		}
		break;
	case CMD2_EPG_SRV_ADD_MANU_ADD2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ADD_MANU_ADD2");
			{
				WORD ver = (WORD)CMD_VER;
				DWORD readSize = 0;
				if( ReadVALUE2(ver, &ver, cmdParam->data, cmdParam->dataSize, &readSize) == TRUE ){

					vector<MANUAL_AUTO_ADD_DATA> list;
					if( ReadVALUE2(ver, &list, cmdParam->data+readSize, cmdParam->dataSize-readSize, NULL ) == TRUE ){
						{
							CBlockLock lock(&sys->settingLock);
							for( size_t i=0; i<list.size(); i++ ){
								sys->manualAutoAdd.AddData(list[i]);
							}
							sys->manualAutoAdd.SaveText();
						}

						resParam->param = CMD_SUCCESS;

						sys->AutoAddReserveProgram();

						DWORD writeSize = 0;
						resParam->param = CMD_SUCCESS;
						resParam->dataSize = GetVALUESize2(ver, ver);
						resParam->data = new BYTE[resParam->dataSize];
						if( WriteVALUE2(ver, ver, resParam->data, resParam->dataSize, &writeSize) == FALSE ){
							_OutputDebugString(L"err Write res CMD2_EPG_SRV_ADD_MANU_ADD2\r\n");
							resParam->dataSize = 0;
							resParam->param = CMD_ERR;
						}
					}
				}

				sys->reserveManager.SendNotifyUpdate(NOTIFY_UPDATE_AUTOADD_MANUAL);
			}
		}
		break;
	case CMD2_EPG_SRV_CHG_MANU_ADD2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_CHG_MANU_ADD2");
			{
				WORD ver = (WORD)CMD_VER;
				DWORD readSize = 0;
				if( ReadVALUE2(ver, &ver, cmdParam->data, cmdParam->dataSize, &readSize) == TRUE ){

					vector<MANUAL_AUTO_ADD_DATA> list;
					if( ReadVALUE2(ver, &list, cmdParam->data+readSize, cmdParam->dataSize-readSize, NULL ) == TRUE ){
						{
							CBlockLock lock(&sys->settingLock);
							for( size_t i=0; i<list.size(); i++ ){
								sys->manualAutoAdd.ChgData(list[i]);
							}
							sys->manualAutoAdd.SaveText();
						}

						resParam->param = CMD_SUCCESS;

						sys->AutoAddReserveProgram();

						DWORD writeSize = 0;
						resParam->param = CMD_SUCCESS;
						resParam->dataSize = GetVALUESize2(ver, ver);
						resParam->data = new BYTE[resParam->dataSize];
						if( WriteVALUE2(ver, ver, resParam->data, resParam->dataSize, &writeSize) == FALSE ){
							_OutputDebugString(L"err Write res CMD2_EPG_SRV_CHG_MANU_ADD2\r\n");
							resParam->dataSize = 0;
							resParam->param = CMD_ERR;
						}
					}
				}

				sys->reserveManager.SendNotifyUpdate(NOTIFY_UPDATE_AUTOADD_MANUAL);
			}
		}
		break;
	case CMD2_EPG_SRV_ENUM_RECINFO2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ENUM_RECINFO2");
			{
				vector<REC_FILE_INFO> list;
				WORD ver = (WORD)CMD_VER;
				if( ReadVALUE2(ver, &ver, cmdParam->data, cmdParam->dataSize, NULL) == TRUE ){
					if(sys->reserveManager.GetRecFileInfoAll(&list) == TRUE ){
						DWORD writeSize = 0;
						resParam->param = CMD_SUCCESS;
						resParam->dataSize = GetVALUESize2(ver, &list)+GetVALUESize2(ver, ver);
						resParam->data = new BYTE[resParam->dataSize];
						if( WriteVALUE2(ver, ver, resParam->data, resParam->dataSize, &writeSize) == FALSE ){
							_OutputDebugString(L"err Write res CMD2_EPG_SRV_ENUM_RECINFO2\r\n");
							resParam->dataSize = 0;
							resParam->param = CMD_ERR;
						}else
						if( WriteVALUE2(ver, &list, resParam->data+writeSize, resParam->dataSize-writeSize, NULL) == FALSE ){
							_OutputDebugString(L"err Write res CMD2_EPG_SRV_ENUM_RECINFO2\r\n");
							resParam->dataSize = 0;
							resParam->param = CMD_ERR;
						}
					}
				}
			}
		}
		break;
	case CMD2_EPG_SRV_CHG_PROTECT_RECINFO2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_CHG_PROTECT_RECINFO2");
			{
				WORD ver = (WORD)CMD_VER;
				DWORD readSize = 0;
				if( ReadVALUE2(ver, &ver, cmdParam->data, cmdParam->dataSize, &readSize) == TRUE ){

					vector<REC_FILE_INFO> list;
					if( ReadVALUE2(ver, &list, cmdParam->data+readSize, cmdParam->dataSize-readSize, NULL ) == TRUE ){
						sys->reserveManager.ChgProtectRecFileInfo(&list);

						resParam->param = CMD_SUCCESS;

						DWORD writeSize = 0;
						resParam->param = CMD_SUCCESS;
						resParam->dataSize = GetVALUESize2(ver, ver);
						resParam->data = new BYTE[resParam->dataSize];
						if( WriteVALUE2(ver, ver, resParam->data, resParam->dataSize, &writeSize) == FALSE ){
							_OutputDebugString(L"err Write res CMD2_EPG_SRV_CHG_PROTECT_RECINFO2\r\n");
							resParam->dataSize = 0;
							resParam->param = CMD_ERR;
						}
					}
				}
			}
		}
		break;
	////////////////////////////////////////////////////////////
	//旧バージョン互換コマンド
	case CMD_EPG_SRV_GET_RESERVE_INFO:
		{
			resParam->param = OLD_CMD_ERR;
			{
				DWORD reserveID = 0;
				if( ReadVALUE(&reserveID, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
					RESERVE_DATA info;
					if(sys->reserveManager.GetReserveData(reserveID, &info) == TRUE ){
						OLD_RESERVE_DATA oldInfo;
						oldInfo = info;
						CreateReserveDataStream(&oldInfo, resParam);
						resParam->param = OLD_CMD_SUCCESS;
					}
				}
			}
		}
		break;
	case CMD_EPG_SRV_ADD_RESERVE:
		{
			resParam->param = OLD_CMD_ERR;
			{
				OLD_RESERVE_DATA oldItem;
				if( CopyReserveData(&oldItem, cmdParam) == TRUE){
					RESERVE_DATA item;
					CopyOldNew(&oldItem, &item);

					vector<RESERVE_DATA> list;
					list.push_back(item);
					if(sys->reserveManager.AddReserveData(&list) == TRUE ){
						resParam->param = OLD_CMD_SUCCESS;
					}
				}
			}
		}
		break;
	case CMD_EPG_SRV_DEL_RESERVE:
		{
			resParam->param = OLD_CMD_ERR;
			{
				OLD_RESERVE_DATA oldItem;
				if( CopyReserveData(&oldItem, cmdParam) == TRUE){
					vector<DWORD> list;
					list.push_back(oldItem.dwReserveID);
					if(sys->reserveManager.DelReserveData(&list) == TRUE ){
						resParam->param = OLD_CMD_SUCCESS;
					}
				}
			}
		}
		break;
	case CMD_EPG_SRV_CHG_RESERVE:
		{
			resParam->param = OLD_CMD_ERR;
			{
				OLD_RESERVE_DATA oldItem;
				if( CopyReserveData(&oldItem, cmdParam) == TRUE){
					RESERVE_DATA item;
					CopyOldNew(&oldItem, &item);

					vector<RESERVE_DATA> list;
					list.push_back(item);
					if(sys->reserveManager.ChgReserveData(&list) == TRUE ){
						resParam->param = OLD_CMD_SUCCESS;
					}
				}
			}
		}

		break;
	case CMD_EPG_SRV_ADD_AUTO_ADD:
		{
			resParam->param = OLD_CMD_ERR;
			{
				OLD_SEARCH_KEY oldItem;
				if( CopySearchKeyData(&oldItem, cmdParam) == TRUE){
					EPG_AUTO_ADD_DATA item;
					CopyOldNew(&oldItem, &item);

					{
						CBlockLock lock(&sys->settingLock);
						item.dataID = sys->epgAutoAdd.AddData(item);
						sys->epgAutoAdd.SaveText();
					}
					resParam->param = OLD_CMD_SUCCESS;
					sys->AutoAddReserveEPG(1, &item);
					sys->reserveManager.SendNotifyUpdate(NOTIFY_UPDATE_AUTOADD_EPG);
				}
			}
		}
		break;
	case CMD_EPG_SRV_DEL_AUTO_ADD:
		{
			resParam->param = OLD_CMD_ERR;
			{
				OLD_SEARCH_KEY oldItem;
				if( CopySearchKeyData(&oldItem, cmdParam) == TRUE){
					{
						CBlockLock lock(&sys->settingLock);
						sys->epgAutoAdd.DelData((DWORD)oldItem.iAutoAddID);
						sys->epgAutoAdd.SaveText();
					}
					resParam->param = OLD_CMD_SUCCESS;
					sys->reserveManager.SendNotifyUpdate(NOTIFY_UPDATE_AUTOADD_EPG);
				}
			}
		}
		break;
	case CMD_EPG_SRV_CHG_AUTO_ADD:
		{
			resParam->param = OLD_CMD_ERR;
			{
				OLD_SEARCH_KEY oldItem;
				if( CopySearchKeyData(&oldItem, cmdParam) == TRUE){
					EPG_AUTO_ADD_DATA item;
					CopyOldNew(&oldItem, &item);

					{
						CBlockLock lock(&sys->settingLock);
						sys->epgAutoAdd.ChgData(item);
						sys->epgAutoAdd.SaveText();
					}
					resParam->param = OLD_CMD_SUCCESS;
					sys->AutoAddReserveEPG(1, &item);
					sys->reserveManager.SendNotifyUpdate(NOTIFY_UPDATE_AUTOADD_EPG);
				}
			}
		}
		break;
	case CMD_EPG_SRV_SEARCH_PG_FIRST:
		{
			resParam->param = OLD_CMD_ERR;
			if( sys->epgDB.IsInitialLoadingDataDone() == FALSE ){
				resParam->param = CMD_ERR_BUSY;
			}else{
				{
					OLD_SEARCH_KEY oldItem;
					if( CopySearchKeyData(&oldItem, cmdParam) == TRUE){
						EPGDB_SEARCH_KEY_INFO item;
						CopyOldNew(&oldItem, &item);

						vector<EPGDB_SEARCH_KEY_INFO> key;
						vector<unique_ptr<CEpgDBManager::SEARCH_RESULT_EVENT_DATA>> val;
						key.push_back(item);
						if( sys->epgDB.SearchEpg(&key, &val) == TRUE ){
							CBlockLock lock(&sys->settingLock);

							sys->oldSearchList.clear();
							for( size_t i=0; i<val.size(); i++ ){
								OLD_EVENT_INFO_DATA3 add;
								add = val[i]->info;
								sys->oldSearchList.push_back(add);
							}
							if( sys->oldSearchList.size() == 0 ){
								resParam->param = OLD_CMD_ERR;
							}else{
								if( sys->oldSearchList.size() == 1 ){
									resParam->param = OLD_CMD_SUCCESS;
								}else{
									resParam->param = OLD_CMD_NEXT;
								}
								CreateEventInfoData3Stream(&sys->oldSearchList[0], resParam);
								sys->oldSearchList.erase(sys->oldSearchList.begin());
								vector<OLD_EVENT_INFO_DATA3>(sys->oldSearchList).swap(sys->oldSearchList);
							}
						}
					}
				}
			}
		}
		break;
	case CMD_EPG_SRV_SEARCH_PG_NEXT:
		{
			CBlockLock lock(&sys->settingLock);

			resParam->param = OLD_CMD_ERR;
			if( sys->oldSearchList.size() == 0 ){
				resParam->param = OLD_CMD_ERR;
			}else{
				{
					if( sys->oldSearchList.size() == 1 ){
						resParam->param = OLD_CMD_SUCCESS;
					}else{
						resParam->param = OLD_CMD_NEXT;
					}
					CreateEventInfoData3Stream(&sys->oldSearchList[0], resParam);
					sys->oldSearchList.erase(sys->oldSearchList.begin());
				}
			}
		}
		break;
	default:
		_OutputDebugString(L"err default cmd %d\r\n", cmdParam->param);
		resParam->param = CMD_NON_SUPPORT;
		break;
	}

	return 0;
}

int CALLBACK CEpgTimerSrvMain::HttpCallback(void* param, HTTP_STREAM* recvParam, HTTP_STREAM* sendParam)
{
	CEpgTimerSrvMain* sys = (CEpgTimerSrvMain*)param;
	CHTMLManager htmlManager;
	if( sendParam != NULL ){
		sendParam->httpHeader = "HTTP/1.0 404 Not Found\r\nConnection: close\r\n\r\n";
	}
	OutputDebugStringA(recvParam->httpHeader.c_str());
	if( recvParam != NULL ){
		string verb = "";
		string httpHeader = recvParam->httpHeader;
		Separate(httpHeader, " ", verb, httpHeader);

		string url = "";
		Separate(httpHeader, " ", url, httpHeader);
		OutputDebugStringA(url.c_str());
		if(url.find("/api/") == 0 ){
			string param = "";
			if( recvParam->dataSize > 0 ){
				param.append((char*)recvParam->data, 0, recvParam->dataSize);
			}
			vector<RESERVE_DATA*> list;
			sys->reserveManager.GetReserveDataAll(&list);

			CRestApiManager restApi;
			restApi.AnalyzeCmd(verb, url, param, sendParam, &sys->epgDB, &list, &sys->reserveManager);

			for( size_t i=0; i<list.size(); i++ ){
				SAFE_DELETE(list[i]);
			}
			list.clear();
		}else
		if( CompareNoCase(verb, "GET") == 0 ){
			if( url.compare("/") == 0 || url.compare("/index.html") == 0 ){
				htmlManager.GetIndexPage(sendParam);
			}
			else if(url.find("/reserve.html") == 0 ){
				string page = "";
				Separate(url, "page=", url, page);
				int pageIndex = atoi(page.c_str());
				vector<RESERVE_DATA*> list;
				if(sys->reserveManager.GetReserveDataAll(&list) == TRUE ){
					htmlManager.GetReservePage(&list, pageIndex, sendParam);
					for( size_t i=0; i<list.size(); i++ ){
						SAFE_DELETE(list[i]);
					}
					list.clear();
				}
			}
			else if(url.find("/reserveinfo.html") == 0 ){
				string id = "";
				string preset = "";
				Separate(url, "id=", url, id);
				int reserveID = atoi(id.c_str());
				WORD presetID = 0xFFFF;
				if( recvParam->dataSize > 0 ){
					string param = "";
					param.append((char*)recvParam->data, 0, recvParam->dataSize);
					Separate(param, "presetID=", param, preset);
					presetID = atoi(preset.c_str());
				}
				RESERVE_DATA reserveData;
				if(sys->reserveManager.GetReserveData(reserveID, &reserveData) == TRUE ){
					wstring eventText = L"";
					if( reserveData.eventID != 0xFFFF ){
						EPGDB_EVENT_INFO eventData;
						if(sys->epgDB.SearchEpg(reserveData.originalNetworkID, reserveData.transportStreamID, reserveData.serviceID, reserveData.eventID, &eventData) == TRUE ){
							_ConvertEpgInfoText2(&eventData, eventText, reserveData.stationName);
						}
					}
					vector<TUNER_RESERVE_INFO> tunerList;
					sys->reserveManager.GetTunerReserveAll(&tunerList);
					htmlManager.GetReserveInfoPage(&reserveData, eventText, presetID, &tunerList, sendParam);
				}
			}
			else if(url.find("/recinfo.html") == 0 ){
				string page = "";
				Separate(url, "page=", url, page);
				int pageIndex = atoi(page.c_str());
				vector<REC_FILE_INFO> list;
				if(sys->reserveManager.GetRecFileInfoAll(&list) == TRUE ){
					htmlManager.GetRecInfoPage(&list, pageIndex, sendParam);
				}
			}
			else if(url.find("/recinfodesc.html") == 0 ){
				string id = "";
				Separate(url, "id=", url, id);
				int infoID = atoi(id.c_str());
				vector<REC_FILE_INFO> list;
				if(sys->reserveManager.GetRecFileInfoAll(&list) == TRUE ){
					for( size_t i=0; i<list.size(); i++ ){
						if( list[i].id == infoID ){
							htmlManager.GetRecInfoDescPage(&list[i], sendParam);
							break;
						}
					}
				}
			}
			else if(url.find("/epg.html") == 0 ){
				vector<RESERVE_DATA*> reserveList;
				sys->reserveManager.GetReserveDataAll(&reserveList);
				htmlManager.GetEpgPage(&sys->epgDB, &reserveList, url, sendParam);
				for( size_t i=0; i<reserveList.size(); i++ ){
					SAFE_DELETE(reserveList[i]);
				}
				reserveList.clear();
			}
			else if(url.find("/epginfo.html") == 0 ){
				vector<RESERVE_DATA*> reserveList;
				sys->reserveManager.GetReserveDataAll(&reserveList);
				vector<TUNER_RESERVE_INFO> tunerList;
				sys->reserveManager.GetTunerReserveAll(&tunerList);

				string param = "";
				Separate(url, "?", url, param);
				htmlManager.GetEpgInfoPage(&sys->epgDB, &reserveList, &tunerList, param, sendParam);
				for( size_t i=0; i<reserveList.size(); i++ ){
					SAFE_DELETE(reserveList[i]);
				}
				reserveList.clear();
			}else if(url.find("/addprogres.html") == 0 ){
				vector<TUNER_RESERVE_INFO> tunerList;
				sys->reserveManager.GetTunerReserveAll(&tunerList);

				string param = "";
				htmlManager.GetAddProgramReservePage(&sys->epgDB, &tunerList, param, sendParam);
			}
			else if(url.find("/autoaddepg.html") == 0 ){
				string page = "";
				Separate(url, "page=", url, page);
				int pageIndex = atoi(page.c_str());

				vector<EPG_AUTO_ADD_DATA> list;
				{
					CBlockLock lock(&sys->settingLock);
					map<DWORD, EPG_AUTO_ADD_DATA>::const_iterator itr;
					for( itr = sys->epgAutoAdd.GetMap().begin(); itr != sys->epgAutoAdd.GetMap().end(); itr++ ){
						list.push_back(itr->second);
					}
				}

				htmlManager.GetAutoAddEpgPage(&list, pageIndex, sendParam);
			}
			else if(url.find("/autoaddepgadd.html") == 0 ){
				string param = "";
				Separate(url, "?", url, param);
				vector<TUNER_RESERVE_INFO> tunerList;
				sys->reserveManager.GetTunerReserveAll(&tunerList);

				EPG_AUTO_ADD_DATA val;
				htmlManager.GetAddAutoEpgPage(&val, param, &tunerList, sendParam);
			}
			else if(url.find("/autoaddepginfo.html") == 0 ){
				string param = "";
				Separate(url, "id=", url, param);
				vector<TUNER_RESERVE_INFO> tunerList;
				sys->reserveManager.GetTunerReserveAll(&tunerList);

				{
					CBlockLock lock(&sys->settingLock);
					map<DWORD, EPG_AUTO_ADD_DATA>::const_iterator itr;
					itr = sys->epgAutoAdd.GetMap().find(atoi(param.c_str()));
					if( itr != sys->epgAutoAdd.GetMap().end() ){
						EPG_AUTO_ADD_DATA val = itr->second;
						htmlManager.GetChgAutoEpgPage(&val, "", &tunerList, sendParam);
					}
				}
			}
			
		}else if( CompareNoCase(verb, "POST") == 0 ){
			if(url.find("/reserveinfo.html") == 0 ){
				string id = "";
				string preset = "";
				Separate(url, "id=", url, id);
				int reserveID = atoi(id.c_str());
				WORD presetID = 0xFFFF;
				if( recvParam->dataSize > 0 ){
					string param = "";
					param.append((char*)recvParam->data, 0, recvParam->dataSize);
					Separate(param, "preset=", param, preset);
					presetID = atoi(preset.c_str());
				}
				RESERVE_DATA reserveData;
				if(sys->reserveManager.GetReserveData(reserveID, &reserveData) == TRUE ){
					wstring eventText = L"";
					if( reserveData.eventID != 0xFFFF ){
						EPGDB_EVENT_INFO eventData;
						if(sys->epgDB.SearchEpg(reserveData.originalNetworkID, reserveData.transportStreamID, reserveData.serviceID, reserveData.eventID, &eventData) == TRUE ){
							_ConvertEpgInfoText2(&eventData, eventText, reserveData.stationName);
						}
					}
					vector<TUNER_RESERVE_INFO> tunerList;
					sys->reserveManager.GetTunerReserveAll(&tunerList);
					htmlManager.GetReserveInfoPage(&reserveData, eventText, presetID, &tunerList, sendParam);
				}
			}
			else if(url.find("/reservechg.html") == 0 ){
				string id = "";
				string preset = "";
				Separate(url, "id=", url, id);
				int reserveID = atoi(id.c_str());
				RESERVE_DATA reserveData;
				if(sys->reserveManager.GetReserveData(reserveID, &reserveData) == TRUE ){
					if(htmlManager.GetReserveParam(&reserveData, recvParam) == TRUE ){
						vector<RESERVE_DATA> chgList;
						chgList.push_back(reserveData);
						sys->reserveManager.ChgReserveData(&chgList);
						htmlManager.GetReserveChgPage(sendParam);
					}else{
						htmlManager.GetReserveChgPage(sendParam, TRUE);
					}
				}
			}
			else if(url.find("/reservedel.html") == 0 ){
				string id = "";
				string preset = "";
				Separate(url, "id=", url, id);
				int reserveID = atoi(id.c_str());
				vector<DWORD> delList;
				delList.push_back(reserveID);
				sys->reserveManager.DelReserveData(&delList);
				htmlManager.GetReserveDelPage(sendParam);
			}
			else if(url.find("/recinfodel.html") == 0 ){
				string id = "";
				string preset = "";
				Separate(url, "id=", url, id);
				int reserveID = atoi(id.c_str());
				vector<DWORD> delList;
				delList.push_back(reserveID);
				sys->reserveManager.DelRecFileInfo(&delList);
				htmlManager.GetRecInfoDelPage(sendParam);
			}
			else if(url.find("/epginfo.html") == 0 ){
				vector<RESERVE_DATA*> reserveList;
				sys->reserveManager.GetReserveDataAll(&reserveList);
				vector<TUNER_RESERVE_INFO> tunerList;
				sys->reserveManager.GetTunerReserveAll(&tunerList);
				string param = "";
				param.append((char*)recvParam->data, 0, recvParam->dataSize);
				htmlManager.GetEpgInfoPage(&sys->epgDB, &reserveList, &tunerList, param, sendParam);
				for( size_t i=0; i<reserveList.size(); i++ ){
					SAFE_DELETE(reserveList[i]);
				}
				reserveList.clear();
			}
			else if(url.find("/reserveadd.html") == 0 ){
				RESERVE_DATA reserveData;
				string param = "";
				param.append((char*)recvParam->data, 0, recvParam->dataSize);
				if( htmlManager.GetAddReserveData(&sys->epgDB, &reserveData, param) == TRUE ){
					vector<RESERVE_DATA> chgList;
					chgList.push_back(reserveData);
					if( sys->reserveManager.AddReserveData(&chgList) == TRUE ){
						htmlManager.GetReserveAddPage(sendParam);
					}else{
						htmlManager.GetReserveAddPage(sendParam, TRUE);
					}
				}
			}
			else if(url.find("/addprogres.html") == 0 ){
				vector<TUNER_RESERVE_INFO> tunerList;
				sys->reserveManager.GetTunerReserveAll(&tunerList);

				string param = "";
				if( recvParam->dataSize > 0 ){
					param.append((char*)recvParam->data, 0, recvParam->dataSize);
				}
				htmlManager.GetAddProgramReservePage(&sys->epgDB, &tunerList, param, sendParam);
			}
			else if(url.find("/reservepgadd.html") == 0 ){
				RESERVE_DATA reserveData;
				string param = "";
				param.append((char*)recvParam->data, 0, recvParam->dataSize);
				if( htmlManager.GetAddReservePgData(&sys->epgDB, &reserveData, param) == TRUE ){
					vector<RESERVE_DATA> chgList;
					chgList.push_back(reserveData);
					if( sys->reserveManager.AddReserveData(&chgList) == TRUE ){
						htmlManager.GetReserveAddPage(sendParam);
					}else{
						htmlManager.GetReserveAddPage(sendParam, TRUE);
					}
				}
			}
			else if(url.find("/autoaddepgadd.html") == 0 ){
				string param = "";
				param.append((char*)recvParam->data, 0, recvParam->dataSize);

				OutputDebugStringA(param.c_str());

				vector<TUNER_RESERVE_INFO> tunerList;
				sys->reserveManager.GetTunerReserveAll(&tunerList);

				EPG_AUTO_ADD_DATA val;
				htmlManager.GetAddAutoEpgPage(&val, param, &tunerList, sendParam);
			}
			else if(url.find("/autoaddepgaddkey.html") == 0 ){
				string param = "";
				param.append((char*)recvParam->data, 0, recvParam->dataSize);

				OutputDebugStringA(param.c_str());

				EPG_AUTO_ADD_DATA val;
				if( htmlManager.GetAutoEpgParam(&val, recvParam) == TRUE ){
					{
						CBlockLock lock(&sys->settingLock);
						val.dataID = sys->epgAutoAdd.AddData(val);
						sys->epgAutoAdd.SaveText();
					}

					sys->AutoAddReserveEPG(1, &val);

					htmlManager.GetAddAutoEpgPage(sendParam);
				}
			}
			else if(url.find("/autoaddepginfo.html") == 0 ){
				string param = "";
				param.append((char*)recvParam->data, 0, recvParam->dataSize);

				OutputDebugStringA(param.c_str());

				string id;
				Separate(url, "id=", url, id);
				vector<TUNER_RESERVE_INFO> tunerList;
				sys->reserveManager.GetTunerReserveAll(&tunerList);

				{
					CBlockLock lock(&sys->settingLock);
					map<DWORD, EPG_AUTO_ADD_DATA>::const_iterator itr;
					itr = sys->epgAutoAdd.GetMap().find(atoi(id.c_str()));
					if( itr != sys->epgAutoAdd.GetMap().end() ){
						EPG_AUTO_ADD_DATA val = itr->second;
						htmlManager.GetChgAutoEpgPage(&val, param, &tunerList, sendParam);
					}
				}
			}
			else if(url.find("/autoaddepgchgkey.html") == 0 ){
				string param = "";
				param.append((char*)recvParam->data, 0, recvParam->dataSize);

				OutputDebugStringA(param.c_str());

				EPG_AUTO_ADD_DATA val;
				if( htmlManager.GetAutoEpgParam(&val, recvParam) == TRUE ){
					{
						CBlockLock lock(&sys->settingLock);
						sys->epgAutoAdd.ChgData(val);
						sys->epgAutoAdd.SaveText();
					}

					sys->AutoAddReserveEPG(1, &val);

					htmlManager.GetChgAutoEpgPage(sendParam);
				}
			}
			else if(url.find("/autoaddepgdelkey.html") == 0 ){
				string id = "";
				string preset = "";
				Separate(url, "id=", url, id);
				int dataID = atoi(id.c_str());

				{
					CBlockLock lock(&sys->settingLock);
					sys->epgAutoAdd.DelData(dataID);
					sys->epgAutoAdd.SaveText();
				}

				htmlManager.GetDelAutoEpgPage(sendParam);

				sys->reserveManager.SendNotifyUpdate(NOTIFY_UPDATE_AUTOADD_EPG);
			}
		}
	}
	return 0;
}


int CALLBACK CEpgTimerSrvMain::TcpAcceptCallback(void* param, SOCKET clientSock, struct sockaddr_in* client, HANDLE stopEvent)
{
	CEpgTimerSrvMain* sys = (CEpgTimerSrvMain*)param;

	CHttpRequestReader reqReader;

	string method = "";
	string uri = "";
	string httpVersion = "";
	nocase::map<string, string> headerList;

	reqReader.SetSocket(clientSock, stopEvent);
	if(reqReader.ReadHeader(method, uri, httpVersion, &headerList) != NO_ERR){
		goto Err_End;
	}

	//各処理に振り分け
	if(uri.find("/api/") == 0 ){
	}
	else
	if(uri.find("/file/") == 0 && sys->enableHttpPublic == TRUE){
		BOOL enableHttpPublic_;
		wstring httpPublicFolder_;
		{
			CBlockLock lock(&sys->settingLock);
			enableHttpPublic_ = sys->enableHttpPublic;
			httpPublicFolder_ = sys->httpPublicFolder;
		}
		if( enableHttpPublic_ == TRUE ){
			CHttpPublicFileSend send;
			send.SetPublicFolder(L"/file", httpPublicFolder_);
			send.HttpRequest(method, uri, &headerList, clientSock, stopEvent);
		}
	}
	else
	if(uri.find("/recfile/") == 0 ){
		vector<REC_FILE_INFO> infoList;
		sys->reserveManager.GetRecFileInfoAll(&infoList);

		CHttpRecFileSend send;
		send.SetRootUri(L"/recfile");
		send.SetRecInfo(&infoList);
		send.HttpRequest(method, uri, &headerList, clientSock, stopEvent);
	}
	else
	if(uri.find("/dlna/") == 0 && sys->dlnaManager != NULL){
		CBlockLock lock(&sys->settingLock);
		if( sys->dlnaManager != NULL ){
			sys->dlnaManager->HttpRequest(method, uri, &headerList, &reqReader, clientSock, stopEvent);
		}
	}
	else
	{
		string sendHeader = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
		send(clientSock, sendHeader.c_str(), (int)sendHeader.size(), 0);
	}

Err_End:

	return 0;
}

void CEpgTimerSrvMain::AddRecFileDMS()
{
	CBlockLock lock(&this->settingLock);

	if( dlnaManager == NULL || enableDMS == FALSE ){
		return;
	}
	vector<REC_FILE_INFO> list;
	if(this->reserveManager.GetRecFileInfoAll(&list) == TRUE ){
		for( size_t i= 0; i<list.size(); i++ ){
			if( list[i].recFilePath.size() > 0 ){
				dlnaManager->AddDMSRecFile(list[i].recFilePath);
			}
		}
	}
}

