#include "StdAfx.h"
#include "EpgTimerSrvMain.h"
#include "HTMLManager.h"
#include "RestApiManager.h"

#include "../../Common/CommonDef.h"
#include "../../Common/CtrlCmdDef.h"
#include "../../Common/CtrlCmdUtil.h"
#include "../../Common/CtrlCmdUtil2.h"
#include "../../Common/StringUtil.h"

#include "HttpPublicFileSend.h"
#include "HttpRecFileSend.h"
#include "HttpRequestReader.h"

#include "SyoboiCalUtil.h"

#include <process.h>

CEpgTimerSrvMain::CEpgTimerSrvMain(void)
{
	this->lockEvent = _CreateEvent(FALSE, TRUE, NULL);

	this->stopEvent = _CreateEvent(TRUE, FALSE,NULL);

	this->reloadEpgChkFlag = FALSE;

	this->suspendMode = 0xFF;
	this->rebootFlag= 0xFF;
	this->sleepThread = NULL;

	this->suspending = FALSE;

	this->pipeServer = NULL;
	this->tcpServer = NULL;
	this->httpServer = NULL;
	this->tcpSrvUtil = NULL;

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
	if( this->stopEvent != NULL ){
		CloseHandle(this->stopEvent);
	}

	if( this->httpServer != NULL ){
		this->httpServer->StopServer();
		SAFE_DELETE(this->httpServer);
	}
	if( this->tcpServer != NULL ){
		this->tcpServer->StopServer();
		SAFE_DELETE(this->tcpServer);
	}
	if( this->pipeServer != NULL ){
		this->pipeServer->StopServer();
		SAFE_DELETE(this->pipeServer);
	}
	if( this->tcpSrvUtil != NULL ){
		this->tcpSrvUtil->StopServer();
		SAFE_DELETE(this->tcpSrvUtil);
	}
	if( this->dlnaManager != NULL ){
		this->dlnaManager->StopDMS();
		this->dlnaManager->StopSSDPServer();
		SAFE_DELETE(this->dlnaManager);
	}

	if( this->lockEvent != NULL ){
		UnLock();
		CloseHandle(this->lockEvent);
		this->lockEvent = NULL;
	}
}

BOOL CEpgTimerSrvMain::Lock(LPCWSTR log, DWORD timeOut)
{
	if( this->lockEvent == NULL ){
		return FALSE;
	}
	if( log != NULL ){
		OutputDebugString(log);
	}
	DWORD dwRet = WaitForSingleObject(this->lockEvent, timeOut);
	if( dwRet == WAIT_ABANDONED || 
		dwRet == WAIT_FAILED ||
		dwRet == WAIT_TIMEOUT){
			OutputDebugString(L"◆CEpgTimerSrvMain::Lock FALSE");
		return FALSE;
	}
	return TRUE;
}

void CEpgTimerSrvMain::UnLock(LPCWSTR log)
{
	if( this->lockEvent != NULL ){
		SetEvent(this->lockEvent);
	}
	if( log != NULL ){
		OutputDebugString(log);
	}
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
	this->reserveManager.ReloadRecInfoData();
	wstring epgAutoAddFilePath;
	GetSettingPath(epgAutoAddFilePath);
	epgAutoAddFilePath += L"\\";
	epgAutoAddFilePath += EPG_AUTO_ADD_TEXT_NAME;

	wstring manualAutoAddFilePath;
	GetSettingPath(manualAutoAddFilePath);
	manualAutoAddFilePath += L"\\";
	manualAutoAddFilePath += MANUAL_AUTO_ADD_TEXT_NAME;

	this->epgAutoAdd.ParseText(epgAutoAddFilePath.c_str());
	this->manualAutoAdd.ParseText(manualAutoAddFilePath.c_str());

	this->reserveManager.SetNotifyManager(&this->notifyManager);
	this->reserveManager.SetEpgDBManager(&this->epgDB);

	//Pipeサーバースタート
	if( this->pipeServer == NULL ){
		pipeServer = new CPipeServer;
	}
	pipeServer->StartServer(CMD2_EPG_SRV_EVENT_WAIT_CONNECT, CMD2_EPG_SRV_PIPE, CtrlCmdCallback, this, 0, GetCurrentProcessId());

	this->epgDB.ReloadEpgData();
	this->reserveManager.ReloadBankMap(FALSE);
	this->reloadEpgChkFlag = TRUE;

	CSendCtrlCmd sendCtrl;
	DWORD countChkSuspend = 11;
	
	while(1){
		if( WaitForSingleObject(this->stopEvent, 1*1000) == WAIT_OBJECT_0 ){
			break;
		}
		if( this->reloadEpgChkFlag == TRUE ){
			if( this->epgDB.IsLoadingData() == FALSE ){
				//リロード終わったので自動予約登録処理を行う
				if( Lock() == TRUE ){
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

					UnLock();
				}
				this->reloadEpgChkFlag = FALSE;
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
					if( this->suspendMode != 0xFF && this->rebootFlag != 0xFF ){
						//問い合わせ
						if( this->suspendMode != 0 && this->suspendMode != 4 ){
							if( QuerySleep(this->rebootFlag, this->suspendMode) == FALSE ){
								StartSleep(this->rebootFlag, this->suspendMode);
							}
						}
					}
				}
				countChkSuspend = 11;
				this->suspendMode = 0xFF;
				this->rebootFlag = 0xFF;
			}
		}
		//予約終了後の動作チェック
		if( this->reserveManager.IsEnableSuspend(&this->suspendMode, &this->rebootFlag ) == TRUE ){
			OutputDebugString(L"★IsEnableSuspend");
			this->reloadEpgChkFlag = TRUE;
			this->epgDB.ReloadEpgData();
		}else{
			if( this->reserveManager.IsEnableReloadEPG() == TRUE ){
				this->reloadEpgChkFlag = TRUE;
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

			LONGLONG returnTime = 0;
			if( reserveManager.GetSleepReturnTime(&returnTime) == TRUE ){
				if( sleepUtil.SetReturnTime(returnTime, this->rebootFlagWork, this->wakeMargin) == TRUE ){
				}
			}
		}
		countChkSuspend++;
	}
	pipeServer->StopServer();
}

void CEpgTimerSrvMain::ReloadSetting()
{
	wstring iniPath = L"";
	GetModuleIniPath(iniPath);
	this->enableTCPSrv = GetPrivateProfileInt(L"SET", L"EnableTCPSrv", 0, iniPath.c_str());
	this->tcpPort = GetPrivateProfileInt(L"SET", L"TCPPort", 4510, iniPath.c_str());

	if( this->enableTCPSrv == FALSE){
		if( this->tcpServer != NULL ){
			this->tcpServer->StopServer();
			SAFE_DELETE(this->tcpServer);
		}
	}else{
		if( this->tcpServer == NULL ){
			this->tcpServer = new CTCPServer;
			this->tcpServer->StartServer(this->tcpPort, CtrlCmdCallback, this, 0, GetCurrentProcessId());
		}
	}

	this->enableHttpSrv = GetPrivateProfileInt(L"SET", L"EnableHttpSrv", 0, iniPath.c_str());
	this->httpPort = GetPrivateProfileInt(L"SET", L"HttpPort", 5510, iniPath.c_str());

	if( this->enableHttpSrv == FALSE){
		if( this->httpServer != NULL ){
			this->httpServer->StopServer();
			SAFE_DELETE(this->httpServer);
		}
	}else{
		if( this->httpServer == NULL ){
			this->httpServer = new CHttpServer;
			this->httpServer->StartServer(this->httpPort, HttpCallback, this, 0, GetCurrentProcessId());
		}
	}
	
	this->enableDMS = GetPrivateProfileInt(L"SET", L"EnableDMS", 0, iniPath.c_str());
	if( this->enableDMS == FALSE ){
		if( this->tcpSrvUtil != NULL ){
			this->tcpSrvUtil->StopServer();
			SAFE_DELETE(this->tcpSrvUtil);
		}
		if( this->dlnaManager != NULL ){
			this->dlnaManager->StopDMS();
			this->dlnaManager->StopSSDPServer();
			SAFE_DELETE(this->dlnaManager);
		}
	}else{
		if( this->tcpSrvUtil == NULL ){
			this->tcpSrvUtil = new CTCPServerUtil;
			this->tcpSrvUtil->StartServer(this->httpPort+1, TcpAcceptCallback, this);
		}
		if( this->dlnaManager == NULL ){
			this->dlnaManager = new CDLNAManager;
			this->dlnaManager->StartSSDPServer(this->httpPort+1);
			this->dlnaManager->LoadPublicFolder();
			AddRecFileDMS();
			this->dlnaManager->StartDMS();
		}else{
			this->dlnaManager->LoadPublicFolder();
			AddRecFileDMS();
		}
	}

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

void CEpgTimerSrvMain::StartSleep(BYTE rebootFlag, BYTE suspendMode)
{
	if( this->sleepThread != NULL ){
		if( ::WaitForSingleObject(this->sleepThread, 0) == WAIT_OBJECT_0 ){
			CloseHandle(this->sleepThread);
			this->sleepThread = NULL;
		}
	}
	if( this->sleepThread == NULL ){
		this->rebootFlagWork = rebootFlag;
		this->suspendModeWork = suspendMode;
		this->sleepThread = (HANDLE)_beginthreadex(NULL, 0, SleepThread, (LPVOID)this, CREATE_SUSPENDED, NULL);
		SetThreadPriority( this->sleepThread, THREAD_PRIORITY_NORMAL );
		ResumeThread(this->sleepThread);
	}
}

UINT WINAPI CEpgTimerSrvMain::SleepThread(void* param)
{
	CEpgTimerSrvMain* sys = (CEpgTimerSrvMain*)param;
	sys->suspending = TRUE;

	if( sys->rebootFlagWork == 1 && sys->suspendModeWork == 0xFF ){
		sys->StartReboot();
		return 0;
	}

	SetThreadExecutionState(ES_CONTINUOUS);

	LONGLONG returnTime = 0;
	if( sys->reserveManager.GetSleepReturnTime(&returnTime) == TRUE ){
		SYSTEMTIME retTime;
		ConvertSystemTime(returnTime, &retTime);
		wstring strTime;
		GetTimeString(retTime, strTime);
		_OutputDebugString(L"ReturnTime: %s", strTime.c_str());
		if( sys->sleepUtil.SetReturnTime(returnTime, sys->rebootFlagWork, sys->wakeMargin) == TRUE ){
			//ストリーミングを終了する
			sys->streamingManager.CloseAllFile();

			if( sys->suspendModeWork == 1 ){
				sys->sleepUtil.SetStandby(TRUE);
				if( sys->rebootFlagWork == 1 ){
					if( sys->QueryReboot(1) == FALSE ){
						sys->StartReboot();
					}
				}
			}else if( sys->suspendModeWork == 2 ){
				sys->sleepUtil.SetStandby(FALSE);
				if( sys->rebootFlagWork == 1 ){
					if( sys->QueryReboot(1) == FALSE ){
						sys->StartReboot();
					}
				}
			}else if( sys->suspendModeWork == 3 ){
				TOKEN_PRIVILEGES TokenPri;
				HANDLE hToken;

				if ( OpenProcessToken(GetCurrentProcess(),(TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY),&hToken) ){
					LookupPrivilegeValue(NULL,SE_SHUTDOWN_NAME,&TokenPri.Privileges[0].Luid);

					TokenPri.PrivilegeCount = 1;
					TokenPri.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
					AdjustTokenPrivileges( hToken, FALSE, &TokenPri, 0, NULL, NULL );
					if ( GetLastError() == ERROR_SUCCESS ){
						ExitWindowsEx(EWX_POWEROFF,0);
					}
				}
			}
		}
	}else{
		//ストリーミングを終了する
		sys->streamingManager.CloseAllFile();

		if( sys->suspendModeWork == 1 ){
			sys->sleepUtil.SetStandby(TRUE);
			if( sys->rebootFlagWork == 1 ){
				if( sys->QueryReboot(1) == FALSE ){
					sys->StartReboot();
				}
			}
		}else if( sys->suspendModeWork == 2 ){
			sys->sleepUtil.SetStandby(FALSE);
			if( sys->rebootFlagWork == 1 ){
				if( sys->QueryReboot(1) == FALSE ){
					sys->StartReboot();
				}
			}
		}else if( sys->suspendModeWork == 3 ){
			TOKEN_PRIVILEGES TokenPri;
			HANDLE hToken;

			if ( OpenProcessToken(GetCurrentProcess(),(TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY),&hToken) ){
				LookupPrivilegeValue(NULL,SE_SHUTDOWN_NAME,&TokenPri.Privileges[0].Luid);

				TokenPri.PrivilegeCount = 1;
				TokenPri.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
				AdjustTokenPrivileges( hToken, FALSE, &TokenPri, 0, NULL, NULL );
				if ( GetLastError() == ERROR_SUCCESS ){
					ExitWindowsEx(EWX_POWEROFF,0);
				}
			}
		}
	}

	sys->suspending = FALSE;

	return 0;
}

void CEpgTimerSrvMain::StartReboot()
{
	TOKEN_PRIVILEGES TokenPri;
	HANDLE hToken;
	if ( OpenProcessToken(GetCurrentProcess(),(TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY),&hToken) ){
		LookupPrivilegeValue(NULL,SE_SHUTDOWN_NAME,&TokenPri.Privileges[0].Luid);

		TokenPri.PrivilegeCount = 1;
		TokenPri.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		AdjustTokenPrivileges( hToken, FALSE, &TokenPri, 0, NULL, NULL );
		if ( GetLastError() == ERROR_SUCCESS ){
			ExitWindowsEx(EWX_REBOOT,0);
		}
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
			wstring pipe;
			wstring waitEvent;
			Format(pipe, L"%s%d", CMD2_GUI_CTRL_PIPE, itrReg->first);
			Format(waitEvent, L"%s%d", CMD2_GUI_CTRL_WAIT_CONNECT, itrReg->first);

			sendCtrl.SetPipeSetting(waitEvent, pipe);
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
			wstring pipe;
			wstring waitEvent;
			Format(pipe, L"%s%d", CMD2_GUI_CTRL_PIPE, itrReg->first);
			Format(waitEvent, L"%s%d", CMD2_GUI_CTRL_WAIT_CONNECT, itrReg->first);

			sendCtrl.SetPipeSetting(waitEvent, pipe);
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
	BOOL ret = FALSE;
	ret = this->suspending;
	return ret;
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
		EPGDB_EVENT_INFO* info;
		if( this->epgDB.SearchEpg(
			reserveList[i]->originalNetworkID,
			reserveList[i]->transportStreamID,
			reserveList[i]->serviceID,
			reserveList[i]->eventID,
			&info
			) == TRUE){

				BOOL chgRes = FALSE;
				if( info->StartTimeFlag == 1 ){
					if( ConvertI64Time(reserveList[i]->startTime) != ConvertI64Time(info->start_time) ){
						reserveList[i]->startTime = info->start_time;
						chgRes = TRUE;
					}
				}
				if( info->DurationFlag == 1 ){
					if( reserveList[i]->durationSecond != info->durationSec ){
						reserveList[i]->durationSecond = info->durationSec;
						chgRes = TRUE;
					}
				}
				if( chgTitle == TRUE ){
					if( info->shortInfo != NULL ){
						if( CompareNoCase(reserveList[i]->title, info->shortInfo->event_name) != 0 ){
							reserveList[i]->title = info->shortInfo->event_name;
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

					reserveList[i]->eventID = info->event_id;

					if( chkTime == FALSE ){
						//番組名も同じか確認
						if( info->shortInfo != NULL ){
							if( CompareNoCase(reserveList[i]->title, info->shortInfo->event_name) == 0 ){
								chgList.push_back(*(reserveList[i]));

								this->reserveManager.SendTweet(TW_CHG_RESERVE_RELOADEPG, &oldData, reserveList[i], NULL);
							}
						}
					}else{
						//時間のみで判断
						if( chgTitle == TRUE ){
							if( info->shortInfo != NULL ){
								if( CompareNoCase(reserveList[i]->title, info->shortInfo->event_name) != 0 ){
									reserveList[i]->title = info->shortInfo->event_name;
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

BOOL CEpgTimerSrvMain::AutoAddReserveEPG()
{
	BOOL ret = TRUE;

	map<ULONGLONG, RESERVE_DATA*> addMap;
	map<ULONGLONG, RESERVE_DATA*>::iterator itrAdd;

	LONGLONG nowTime = GetNowI64Time();
	BOOL chgRecEnd = FALSE;
	map<DWORD, EPG_AUTO_ADD_DATA*>::iterator itrKey;
	for( itrKey = this->epgAutoAdd.dataIDMap.begin(); itrKey != this->epgAutoAdd.dataIDMap.end(); itrKey++ ){
		itrKey->second->addCount = 0;

		vector<CEpgDBManager::SEARCH_RESULT_EVENT> resultList;
		this->epgDB.SearchEpg(&itrKey->second->searchInfo, &resultList);
		for( size_t i=0; i<resultList.size(); i++ ){
			EPGDB_EVENT_INFO* result = resultList[i].info;
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

			itrKey->second->addCount++;

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

						addItem->recSetting = itrKey->second->recSetting;
						if( itrKey->second->searchInfo.chkRecEnd == 1 ){
							if( this->reserveManager.IsFindRecEventInfo(result, itrKey->second->searchInfo.chkRecDay) == TRUE ){
								addItem->recSetting.recMode = RECMODE_NO;
							}
						}
						if( resultList[i].findKey.size() > 0 ){
							Format(addItem->comment, L"EPG自動予約(%s)", resultList[i].findKey.c_str());
						}else{
							addItem->comment = L"EPG自動予約";
						}

						addMap.insert(pair<ULONGLONG, RESERVE_DATA*>(eventKey, addItem));
					}else{
						//無効ならそれを優先
						if( itrKey->second->recSetting.recMode == RECMODE_NO ){
							itrAdd->second->recSetting.recMode = RECMODE_NO;
						}
					}
			}else if( itrKey->second->searchInfo.chkRecEnd == 1 ){
				if( this->reserveManager.IsFindRecEventInfo(result, itrKey->second->searchInfo.chkRecDay) == TRUE ){
					this->reserveManager.ChgAutoAddNoRec(result);
					chgRecEnd = TRUE;
				}
			}
		}
	}
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

BOOL CEpgTimerSrvMain::AutoAddReserveEPG(vector<EPG_AUTO_ADD_DATA>* val)
{
	BOOL ret = TRUE;

	map<ULONGLONG, RESERVE_DATA*> addMap;
	map<ULONGLONG, RESERVE_DATA*>::iterator itrAdd;

	LONGLONG nowTime = GetNowI64Time();
	BOOL chgRecEnd = FALSE;

	if( val != NULL ){
		map<DWORD, EPG_AUTO_ADD_DATA*>::iterator itrKey;
		for( itrKey = this->epgAutoAdd.dataIDMap.begin(); itrKey != this->epgAutoAdd.dataIDMap.end(); itrKey++ ){
			BOOL findData = FALSE;
			for( size_t i=0; i < val->size(); i++ ){
				if( (*val)[i].dataID == itrKey->second->dataID ){
					findData = TRUE;
					break;
				}
			}
			if( findData == FALSE ){
				continue;
			}

			itrKey->second->addCount = 0;

			vector<CEpgDBManager::SEARCH_RESULT_EVENT> resultList;
			this->epgDB.SearchEpg(&itrKey->second->searchInfo, &resultList);
			for( size_t i=0; i<resultList.size(); i++ ){
				EPGDB_EVENT_INFO* result = resultList[i].info;
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

				itrKey->second->addCount++;

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

							addItem->recSetting = itrKey->second->recSetting;
							if( itrKey->second->searchInfo.chkRecEnd == 1 ){
								if( this->reserveManager.IsFindRecEventInfo(result, itrKey->second->searchInfo.chkRecDay) == TRUE ){
									addItem->recSetting.recMode = RECMODE_NO;
								}
							}
							if( resultList[i].findKey.size() > 0 ){
								Format(addItem->comment, L"EPG自動予約(%s)", resultList[i].findKey.c_str());
							}else{
								addItem->comment = L"EPG自動予約";
							}

							addMap.insert(pair<ULONGLONG, RESERVE_DATA*>(eventKey, addItem));
						}else{
							//無効ならそれを優先
							if( itrKey->second->recSetting.recMode == RECMODE_NO ){
								itrAdd->second->recSetting.recMode = RECMODE_NO;
							}
						}
				}else if( itrKey->second->searchInfo.chkRecEnd == 1 ){
					if( this->reserveManager.IsFindRecEventInfo(result, itrKey->second->searchInfo.chkRecDay) == TRUE ){
						this->reserveManager.ChgAutoAddNoRec(result);
						chgRecEnd = TRUE;
					}
				}
			}
		}
	}
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

BOOL CEpgTimerSrvMain::AutoAddReserveEPG(EPG_AUTO_ADD_DATA* item)
{
	if( item == NULL ){
		return FALSE;
	}

	vector<EPG_AUTO_ADD_DATA> list;
	list.push_back(*item);

	return AutoAddReserveEPG(&list);
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

	map<DWORD, MANUAL_AUTO_ADD_DATA*>::iterator itr;
	for( itr = this->manualAutoAdd.dataIDMap.begin(); itr != this->manualAutoAdd.dataIDMap.end(); itr++){
		BYTE weekChkFlag = (BYTE)(1<<nowTime.wDayOfWeek);
		for( BYTE i=0; i<8; i++ ){
			if( (itr->second->dayOfWeekFlag & weekChkFlag) != 0 ){
				LONGLONG startTime = baseStartTime + ((LONGLONG)itr->second->startTime) * I64_1SEC + (((LONGLONG)i) * 24*60*60*I64_1SEC);

				if( startTime > now ){
					//時間的に予約追加候補
					BOOL find = FALSE;
					for( size_t j=0; j<reserveList.size(); j++ ){
						//同一時間の予約がすでにあるかチェック
						if( reserveList[j]->eventID != 0xFFFF ){
							continue;
						}
						if( reserveList[j]->originalNetworkID != itr->second->originalNetworkID ||
							reserveList[j]->transportStreamID != itr->second->transportStreamID ||
							reserveList[j]->serviceID != itr->second->serviceID 
							){
							continue;
						}
						if( ConvertI64Time(reserveList[j]->startTime) == startTime &&
							reserveList[j]->durationSecond == itr->second->durationSecond
							){
							find = TRUE;
							break;
						}
					}
					if( find == FALSE ){
						//見つからなかったので予約追加
						RESERVE_DATA item;
						item.title = itr->second->title;
						ConvertSystemTime(startTime, &item.startTime); 
						item.startTimeEpg = item.startTime;
						item.durationSecond = itr->second->durationSecond;
						item.stationName = itr->second->stationName;
						item.originalNetworkID = itr->second->originalNetworkID;
						item.transportStreamID = itr->second->transportStreamID;
						item.serviceID = itr->second->serviceID;
						item.eventID = 0xFFFF;
						item.recSetting = itr->second->recSetting;

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

	if( setList.size() > 0 ){
		this->reserveManager.AddReserveData(&setList);
	}
	for( size_t i=0; i<reserveList.size(); i++ ){
		SAFE_DELETE(reserveList[i]);
	}
	reserveList.clear();

	return ret;
}

int CALLBACK CEpgTimerSrvMain::CtrlCmdCallback(void* param, CMD_STREAM* cmdParam, CMD_STREAM* resParam)
{
	CEpgTimerSrvMain* sys = (CEpgTimerSrvMain*)param;

	resParam->dataSize = 0;
	resParam->param = CMD_ERR;


	switch( cmdParam->param ){
	case CMD2_EPG_SRV_ADDLOAD_RESERVE:
		if( sys->Lock() == TRUE ){
			if( sys->reserveManager.AddLoadReserveData() == TRUE ){
				resParam->param = CMD_SUCCESS;
			}
			sys->UnLock();
		}else{
			resParam->param = CMD_ERR_BUSY;
		}
		break;
	case CMD2_EPG_SRV_RELOAD_EPG:
		if( sys->epgDB.IsLoadingData() == TRUE ){
			resParam->param = CMD_ERR_BUSY;
		}else{
			if( sys->Lock() == TRUE ){
				if( sys->epgDB.ReloadEpgData() == TRUE ){
					sys->reloadEpgChkFlag = TRUE;
					resParam->param = CMD_SUCCESS;
				}
				sys->UnLock();
			}else{
				resParam->param = CMD_ERR_BUSY;
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
				sys->notifyManager.RegistTCP(&val);

				sys->reserveManager.ChangeRegist();
			}
		}
		break;
	case CMD2_EPG_SRV_UNREGIST_GUI_TCP:
		{
			REGIST_TCP_INFO val;
			if( ReadVALUE( &val, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
				resParam->param = CMD_SUCCESS;
				sys->notifyManager.UnRegistTCP(&val);
			}
		}
		break;

	case CMD2_EPG_SRV_ENUM_RESERVE:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ENUM_RESERVE");
			if( sys->Lock() == TRUE ){
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
				sys->UnLock();
			}else{
				resParam->param = CMD_ERR_BUSY;
			}
		}
		break;
	case CMD2_EPG_SRV_GET_RESERVE:
		{
			OutputDebugString(L"CMD2_EPG_SRV_GET_RESERVE");
			if( sys->Lock() == TRUE ){
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
				sys->UnLock();
			}else{
				resParam->param = CMD_ERR_BUSY;
			}
		}
		break;
	case CMD2_EPG_SRV_ADD_RESERVE:
		{
			if( sys->Lock() == TRUE ){
				vector<RESERVE_DATA> list;
				if( ReadVALUE( &list, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
					if(sys->reserveManager.AddReserveData(&list) == TRUE ){
						resParam->param = CMD_SUCCESS;
					}
				}
				sys->UnLock();
			}else{
				resParam->param = CMD_ERR_BUSY;
			}
		}
		break;
	case CMD2_EPG_SRV_DEL_RESERVE:
		{
			if( sys->Lock() == TRUE ){
				vector<DWORD> list;
				if( ReadVALUE( &list, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
					if(sys->reserveManager.DelReserveData(&list) == TRUE ){
						resParam->param = CMD_SUCCESS;
					}
				}
				sys->UnLock();
			}else{
				resParam->param = CMD_ERR_BUSY;
			}
		}
		break;
	case CMD2_EPG_SRV_CHG_RESERVE:
		{
			if( sys->Lock() == TRUE ){
				vector<RESERVE_DATA> list;
				if( ReadVALUE( &list, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
					if(sys->reserveManager.ChgReserveData(&list) == TRUE ){
						resParam->param = CMD_SUCCESS;
					}
				}
				sys->UnLock();
			}else{
				resParam->param = CMD_ERR_BUSY;
			}
		}
		break;
	case CMD2_EPG_SRV_ENUM_RECINFO:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ENUM_RECINFO");
			if( sys->Lock() == TRUE ){
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
				sys->UnLock();
			}else{
				resParam->param = CMD_ERR_BUSY;
			}
		}
		break;
	case CMD2_EPG_SRV_DEL_RECINFO:
		{
			if( sys->Lock() == TRUE ){
				vector<DWORD> list;
				if( ReadVALUE( &list, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
					if(sys->reserveManager.DelRecFileInfo(&list) == TRUE ){
						resParam->param = CMD_SUCCESS;
					}
				}
				sys->UnLock();
			}else{
				resParam->param = CMD_ERR_BUSY;
			}
		}
		break;
	case CMD2_EPG_SRV_ENUM_SERVICE:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ENUM_SERVICE");
			if( sys->epgDB.IsLoadingData() == TRUE ){
				resParam->param = CMD_ERR_BUSY;
			}else{
				if( sys->Lock() == TRUE ){
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
					sys->UnLock();
				}else{
					resParam->param = CMD_ERR_BUSY;
				}
			}
		}
		break;
	case CMD2_EPG_SRV_ENUM_PG_INFO:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ENUM_PG_INFO");
			if( sys->epgDB.IsLoadingData() == TRUE ){
				resParam->param = CMD_ERR_BUSY;
			}else{
				if( sys->Lock() == TRUE ){
					resParam->param = CMD_ERR;
					vector<EPGDB_EVENT_INFO*> val;
					LONGLONG serviceKey = 0;

					if( ReadVALUE(&serviceKey, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
						if( sys->epgDB.EnumEventInfo(serviceKey, &val) == TRUE){
							resParam->param = CMD_SUCCESS;
							resParam->dataSize = GetVALUESize(&val);
							resParam->data = new BYTE[resParam->dataSize];
							if( WriteVALUE(&val, resParam->data, resParam->dataSize, NULL) == FALSE ){
								_OutputDebugString(L"err Write res CMD2_EPG_SRV_ENUM_PG_INFO\r\n");
								resParam->dataSize = 0;
								resParam->param = CMD_ERR;
							}
						}
					}
					sys->UnLock();
				}else{
					resParam->param = CMD_ERR_BUSY;
				}
			}
		}
		break;
	case CMD2_EPG_SRV_SEARCH_PG:
		{
			OutputDebugString(L"CMD2_EPG_SRV_SEARCH_PG");
			if( sys->epgDB.IsLoadingData() == TRUE ){
				resParam->param = CMD_ERR_BUSY;
			}else{
				if( sys->Lock() == TRUE ){
					vector<EPGDB_SEARCH_KEY_INFO> key;
					vector<EPGDB_EVENT_INFO*> val;

					if( ReadVALUE( &key, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
						if( sys->epgDB.SearchEpg(&key, &val) == TRUE ){
							resParam->param = CMD_SUCCESS;
							resParam->dataSize = GetVALUESize(&val);
							resParam->data = new BYTE[resParam->dataSize];
							if( WriteVALUE(&val, resParam->data, resParam->dataSize, NULL) == FALSE ){
								_OutputDebugString(L"err Write res CMD2_EPG_SRV_SEARCH_PG\r\n");
								resParam->dataSize = 0;
								resParam->param = CMD_ERR;
							}
						}
					}
					sys->UnLock();
				}else{
					resParam->param = CMD_ERR_BUSY;
				}
			}
		}
		break;
	case CMD2_EPG_SRV_GET_PG_INFO:
		{
			OutputDebugString(L"CMD2_EPG_SRV_GET_PG_INFO");
			if( sys->epgDB.IsLoadingData() == TRUE ){
				resParam->param = CMD_ERR_BUSY;
			}else{
				if( sys->Lock() == TRUE ){
					ULONGLONG key;
					EPGDB_EVENT_INFO* val;

					if( ReadVALUE( &key, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
						WORD ONID = (WORD)(key>>48);
						WORD TSID = (WORD)((key&0x0000FFFF00000000)>>32);
						WORD SID = (WORD)((key&0x00000000FFFF0000)>>16);
						WORD eventID = (WORD)(key&0x000000000000FFFF);
						if( sys->epgDB.SearchEpg(ONID, TSID, SID, eventID, &val) == TRUE ){
							resParam->param = CMD_SUCCESS;
							resParam->dataSize = GetVALUESize(val);
							resParam->data = new BYTE[resParam->dataSize];
							if( WriteVALUE(val, resParam->data, resParam->dataSize, NULL) == FALSE ){
								_OutputDebugString(L"err Write res CMD2_EPG_SRV_GET_PG_INFO\r\n");
								resParam->dataSize = 0;
								resParam->param = CMD_ERR;
							}
						}
					}
					sys->UnLock();
				}else{
					resParam->param = CMD_ERR_BUSY;
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
					if( sys->Lock() == TRUE ){
						sys->StartSleep(reboot, suspendMode);
						sys->UnLock();
					}
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD2_EPG_SRV_REBOOT:
		{
			resParam->param = CMD_SUCCESS;
			sys->StartSleep(1, 0xFF);
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
			if( sys->Lock() == TRUE ){
				vector<EPG_AUTO_ADD_DATA> val;
				map<DWORD, EPG_AUTO_ADD_DATA*>::iterator itr;
				for( itr = sys->epgAutoAdd.dataIDMap.begin(); itr != sys->epgAutoAdd.dataIDMap.end(); itr++ ){
					val.push_back(*(itr->second));
				}
				resParam->param = CMD_SUCCESS;
				resParam->dataSize = GetVALUESize(&val);
				resParam->data = new BYTE[resParam->dataSize];
				if( WriteVALUE(&val, resParam->data, resParam->dataSize, NULL) == FALSE ){
					_OutputDebugString(L"err Write res CMD2_EPG_SRV_ENUM_AUTO_ADD\r\n");
					resParam->dataSize = 0;
					resParam->param = CMD_ERR;
				}

				sys->UnLock();
			}else{
				resParam->param = CMD_ERR_BUSY;
			}
		}
		break;
	case CMD2_EPG_SRV_ADD_AUTO_ADD:
		{
			if( sys->Lock() == TRUE ){
				vector<EPG_AUTO_ADD_DATA> val;
				if( ReadVALUE( &val, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
					for( size_t i=0; i<val.size(); i++ ){
						sys->epgAutoAdd.AddData(&val[i]);
					}

					wstring savePath = L"";
					GetSettingPath(savePath);
					savePath += L"\\";
					savePath += EPG_AUTO_ADD_TEXT_NAME;

					sys->epgAutoAdd.SaveText(savePath.c_str());

					resParam->param = CMD_SUCCESS;

					sys->AutoAddReserveEPG(&val);
				}

				sys->UnLock();

				sys->reserveManager.SendNotifyUpdate(NOTIFY_UPDATE_AUTOADD_EPG);

			}else{
				resParam->param = CMD_ERR_BUSY;
			}

		}
		break;
	case CMD2_EPG_SRV_DEL_AUTO_ADD:
		{
			if( sys->Lock() == TRUE ){
				vector<DWORD> val;
				if( ReadVALUE( &val, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
					for( size_t i=0; i<val.size(); i++ ){
						sys->epgAutoAdd.DelData(val[i]);
					}

					wstring savePath = L"";
					GetSettingPath(savePath);
					savePath += L"\\";
					savePath += EPG_AUTO_ADD_TEXT_NAME;

					sys->epgAutoAdd.SaveText(savePath.c_str());

					resParam->param = CMD_SUCCESS;
				}

				sys->UnLock();

				sys->reserveManager.SendNotifyUpdate(NOTIFY_UPDATE_AUTOADD_EPG);
			}else{
				resParam->param = CMD_ERR_BUSY;
			}

		}
		break;
	case CMD2_EPG_SRV_CHG_AUTO_ADD:
		{
			if( sys->Lock() == TRUE ){
				vector<EPG_AUTO_ADD_DATA> val;
				if( ReadVALUE( &val, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
					for( size_t i=0; i<val.size(); i++ ){
						sys->epgAutoAdd.ChgData(&val[i]);
					}

					wstring savePath = L"";
					GetSettingPath(savePath);
					savePath += L"\\";
					savePath += EPG_AUTO_ADD_TEXT_NAME;

					sys->epgAutoAdd.SaveText(savePath.c_str());

					resParam->param = CMD_SUCCESS;

					sys->AutoAddReserveEPG(&val);
				}

				sys->UnLock();

				sys->reserveManager.SendNotifyUpdate(NOTIFY_UPDATE_AUTOADD_EPG);
			}else{
				resParam->param = CMD_ERR_BUSY;
			}

		}
		break;
	case CMD2_EPG_SRV_ENUM_MANU_ADD:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ENUM_MANU_ADD");
			if( sys->Lock() == TRUE ){
				vector<MANUAL_AUTO_ADD_DATA> val;
				map<DWORD, MANUAL_AUTO_ADD_DATA*>::iterator itr;
				for( itr = sys->manualAutoAdd.dataIDMap.begin(); itr != sys->manualAutoAdd.dataIDMap.end(); itr++ ){
					val.push_back(*(itr->second));
				}
				resParam->param = CMD_SUCCESS;
				resParam->dataSize = GetVALUESize(&val);
				resParam->data = new BYTE[resParam->dataSize];
				if( WriteVALUE(&val, resParam->data, resParam->dataSize, NULL) == FALSE ){
					_OutputDebugString(L"err Write res CMD2_EPG_SRV_ENUM_MANU_ADD\r\n");
					resParam->dataSize = 0;
					resParam->param = CMD_ERR;
				}

				sys->UnLock();
			}else{
				resParam->param = CMD_ERR_BUSY;
			}

		}
		break;
	case CMD2_EPG_SRV_ADD_MANU_ADD:
		{
			if( sys->Lock() == TRUE ){
				vector<MANUAL_AUTO_ADD_DATA> val;
				if( ReadVALUE( &val, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
					for( size_t i=0; i<val.size(); i++ ){
						sys->manualAutoAdd.AddData(&val[i]);
					}

					wstring savePath = L"";
					GetSettingPath(savePath);
					savePath += L"\\";
					savePath += MANUAL_AUTO_ADD_TEXT_NAME;

					sys->manualAutoAdd.SaveText(savePath.c_str());

					resParam->param = CMD_SUCCESS;

					sys->AutoAddReserveProgram();
				}

				sys->UnLock();

				sys->reserveManager.SendNotifyUpdate(NOTIFY_UPDATE_AUTOADD_MANUAL);

			}else{
				resParam->param = CMD_ERR_BUSY;
			}

		}
		break;
	case CMD2_EPG_SRV_DEL_MANU_ADD:
		{
			if( sys->Lock() == TRUE ){
				vector<DWORD> val;
				if( ReadVALUE( &val, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
					for( size_t i=0; i<val.size(); i++ ){
						sys->manualAutoAdd.DelData(val[i]);
					}

					wstring savePath = L"";
					GetSettingPath(savePath);
					savePath += L"\\";
					savePath += MANUAL_AUTO_ADD_TEXT_NAME;

					sys->manualAutoAdd.SaveText(savePath.c_str());

					resParam->param = CMD_SUCCESS;
				}

				sys->UnLock();

				sys->reserveManager.SendNotifyUpdate(NOTIFY_UPDATE_AUTOADD_MANUAL);
			}else{
				resParam->param = CMD_ERR_BUSY;
			}

		}
		break;
	case CMD2_EPG_SRV_CHG_MANU_ADD:
		{
			if( sys->Lock() == TRUE ){
				vector<MANUAL_AUTO_ADD_DATA> val;
				if( ReadVALUE( &val, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
					for( size_t i=0; i<val.size(); i++ ){
						sys->manualAutoAdd.ChgData(&val[i]);
					}

					wstring savePath = L"";
					GetSettingPath(savePath);
					savePath += L"\\";
					savePath += MANUAL_AUTO_ADD_TEXT_NAME;

					sys->manualAutoAdd.SaveText(savePath.c_str());

					resParam->param = CMD_SUCCESS;

					sys->AutoAddReserveProgram();
				}

				sys->UnLock();

				sys->reserveManager.SendNotifyUpdate(NOTIFY_UPDATE_AUTOADD_MANUAL);
			}else{
				resParam->param = CMD_ERR_BUSY;
			}

		}
		break;
	case CMD2_EPG_SRV_ENUM_TUNER_RESERVE:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ENUM_TUNER_RESERVE");
			vector<TUNER_RESERVE_INFO> list;
			if( sys->Lock() == TRUE ){
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
				sys->UnLock();
			}else{
				resParam->param = CMD_ERR_BUSY;
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

					HANDLE file = _CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
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
			if( sys->epgDB.IsLoadingData() == TRUE ){
				resParam->param = CMD_ERR_BUSY;
			}else{
				if( sys->Lock() == TRUE ){
					resParam->param = CMD_ERR;
					vector<EPGDB_SERVICE_EVENT_INFO*> val;

					if( sys->epgDB.EnumEventAll(&val) == TRUE){
						resParam->param = CMD_SUCCESS;
						resParam->dataSize = GetVALUESize(&val);
						resParam->data = new BYTE[resParam->dataSize];
						if( WriteVALUE(&val, resParam->data, resParam->dataSize, NULL) == FALSE ){
							_OutputDebugString(L"err Write res CMD2_EPG_SRV_ENUM_PG_ALL\r\n");
							resParam->dataSize = 0;
							resParam->param = CMD_ERR;
						}
						for( size_t i=0;i<val.size(); i++ ){
							SAFE_DELETE(val[i]);
						}
					}
					sys->UnLock();
				}else{
					resParam->param = CMD_ERR_BUSY;
				}
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
			if( sys->Lock() == TRUE ){
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
				sys->UnLock();
			}else{
				resParam->param = CMD_ERR_BUSY;
			}
		}
		break;
	case CMD2_EPG_SRV_GET_RESERVE2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_GET_RESERVE2");
			if( sys->Lock() == TRUE ){
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
				sys->UnLock();
			}else{
				resParam->param = CMD_ERR_BUSY;
			}
		}
		break;
	case CMD2_EPG_SRV_ADD_RESERVE2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ADD_RESERVE2");
			if( sys->Lock() == TRUE ){
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
				sys->UnLock();
			}else{
				resParam->param = CMD_ERR_BUSY;
			}
		}
		break;
	case CMD2_EPG_SRV_CHG_RESERVE2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_CHG_RESERVE2");
			if( sys->Lock() == TRUE ){
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
				sys->UnLock();
			}else{
				resParam->param = CMD_ERR_BUSY;
			}
		}
		break;
	case CMD2_EPG_SRV_ADDCHK_RESERVE2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ADDCHK_RESERVE2");
			if( sys->Lock() == TRUE ){
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
				sys->UnLock();
			}else{
				resParam->param = CMD_ERR_BUSY;
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

						HANDLE file = _CreateFile(epgDataPath.c_str(), GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
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

						HANDLE file = _CreateFile(epgDataPath.c_str(), GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
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
			if( sys->Lock() == TRUE ){
				vector<EPG_AUTO_ADD_DATA> val;
				map<DWORD, EPG_AUTO_ADD_DATA*>::iterator itr;
				for( itr = sys->epgAutoAdd.dataIDMap.begin(); itr != sys->epgAutoAdd.dataIDMap.end(); itr++ ){
					val.push_back(*(itr->second));
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
				
				sys->UnLock();
			}else{
				resParam->param = CMD_ERR_BUSY;
			}
		}
		break;
	case CMD2_EPG_SRV_ADD_AUTO_ADD2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ADD_AUTO_ADD2");
			if( sys->Lock() == TRUE ){
				WORD ver = (WORD)CMD_VER;
				DWORD readSize = 0;
				if( ReadVALUE2(ver, &ver, cmdParam->data, cmdParam->dataSize, &readSize) == TRUE ){

					vector<EPG_AUTO_ADD_DATA> list;
					if( ReadVALUE2(ver, &list, cmdParam->data+readSize, cmdParam->dataSize-readSize, NULL ) == TRUE ){
						for( size_t i=0; i<list.size(); i++ ){
							sys->epgAutoAdd.AddData(&list[i]);
						}

						wstring savePath = L"";
						GetSettingPath(savePath);
						savePath += L"\\";
						savePath += EPG_AUTO_ADD_TEXT_NAME;

						sys->epgAutoAdd.SaveText(savePath.c_str());

						resParam->param = CMD_SUCCESS;

						sys->AutoAddReserveEPG(&list);

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
				sys->UnLock();

				sys->reserveManager.SendNotifyUpdate(NOTIFY_UPDATE_AUTOADD_EPG);
			}else{
				resParam->param = CMD_ERR_BUSY;
			}
		}
		break;
	case CMD2_EPG_SRV_CHG_AUTO_ADD2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_CHG_AUTO_ADD2");
			if( sys->Lock() == TRUE ){
				WORD ver = (WORD)CMD_VER;
				DWORD readSize = 0;
				if( ReadVALUE2(ver, &ver, cmdParam->data, cmdParam->dataSize, &readSize) == TRUE ){

					vector<EPG_AUTO_ADD_DATA> list;
					if( ReadVALUE2(ver, &list, cmdParam->data+readSize, cmdParam->dataSize-readSize, NULL ) == TRUE ){
						for( size_t i=0; i<list.size(); i++ ){
							sys->epgAutoAdd.ChgData(&list[i]);
						}

						wstring savePath = L"";
						GetSettingPath(savePath);
						savePath += L"\\";
						savePath += EPG_AUTO_ADD_TEXT_NAME;

						sys->epgAutoAdd.SaveText(savePath.c_str());

						resParam->param = CMD_SUCCESS;

						sys->AutoAddReserveEPG(&list);

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
				sys->UnLock();

				sys->reserveManager.SendNotifyUpdate(NOTIFY_UPDATE_AUTOADD_EPG);
			}else{
				resParam->param = CMD_ERR_BUSY;
			}
		}
		break;
	case CMD2_EPG_SRV_ENUM_MANU_ADD2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ENUM_MANU_ADD2");
			if( sys->Lock() == TRUE ){
				vector<MANUAL_AUTO_ADD_DATA> val;
				map<DWORD, MANUAL_AUTO_ADD_DATA*>::iterator itr;
				for( itr = sys->manualAutoAdd.dataIDMap.begin(); itr != sys->manualAutoAdd.dataIDMap.end(); itr++ ){
					val.push_back(*(itr->second));
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
				
				sys->UnLock();
			}else{
				resParam->param = CMD_ERR_BUSY;
			}
		}
		break;
	case CMD2_EPG_SRV_ADD_MANU_ADD2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ADD_MANU_ADD2");
			if( sys->Lock() == TRUE ){
				WORD ver = (WORD)CMD_VER;
				DWORD readSize = 0;
				if( ReadVALUE2(ver, &ver, cmdParam->data, cmdParam->dataSize, &readSize) == TRUE ){

					vector<MANUAL_AUTO_ADD_DATA> list;
					if( ReadVALUE2(ver, &list, cmdParam->data+readSize, cmdParam->dataSize-readSize, NULL ) == TRUE ){
						for( size_t i=0; i<list.size(); i++ ){
							sys->manualAutoAdd.AddData(&list[i]);
						}

						wstring savePath = L"";
						GetSettingPath(savePath);
						savePath += L"\\";
						savePath += MANUAL_AUTO_ADD_TEXT_NAME;

						sys->manualAutoAdd.SaveText(savePath.c_str());

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
				sys->UnLock();

				sys->reserveManager.SendNotifyUpdate(NOTIFY_UPDATE_AUTOADD_MANUAL);
			}else{
				resParam->param = CMD_ERR_BUSY;
			}
		}
		break;
	case CMD2_EPG_SRV_CHG_MANU_ADD2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_CHG_MANU_ADD2");
			if( sys->Lock() == TRUE ){
				WORD ver = (WORD)CMD_VER;
				DWORD readSize = 0;
				if( ReadVALUE2(ver, &ver, cmdParam->data, cmdParam->dataSize, &readSize) == TRUE ){

					vector<MANUAL_AUTO_ADD_DATA> list;
					if( ReadVALUE2(ver, &list, cmdParam->data+readSize, cmdParam->dataSize-readSize, NULL ) == TRUE ){
						for( size_t i=0; i<list.size(); i++ ){
							sys->manualAutoAdd.ChgData(&list[i]);
						}

						wstring savePath = L"";
						GetSettingPath(savePath);
						savePath += L"\\";
						savePath += MANUAL_AUTO_ADD_TEXT_NAME;

						sys->manualAutoAdd.SaveText(savePath.c_str());

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
				sys->UnLock();

				sys->reserveManager.SendNotifyUpdate(NOTIFY_UPDATE_AUTOADD_MANUAL);
			}else{
				resParam->param = CMD_ERR_BUSY;
			}
		}
		break;
	case CMD2_EPG_SRV_ENUM_RECINFO2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ENUM_RECINFO2");
			if( sys->Lock() == TRUE ){
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
				sys->UnLock();
			}else{
				resParam->param = CMD_ERR_BUSY;
			}
		}
		break;
	case CMD2_EPG_SRV_CHG_PROTECT_RECINFO2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_CHG_PROTECT_RECINFO2");
			if( sys->Lock() == TRUE ){
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
				sys->UnLock();
			}else{
				resParam->param = CMD_ERR_BUSY;
			}
		}
		break;
	////////////////////////////////////////////////////////////
	//旧バージョン互換コマンド
	case CMD_EPG_SRV_GET_RESERVE_INFO:
		{
			resParam->param = OLD_CMD_ERR;
			if( sys->Lock() == TRUE ){
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
				sys->UnLock();
			}
		}
		break;
	case CMD_EPG_SRV_ADD_RESERVE:
		{
			resParam->param = OLD_CMD_ERR;
			if( sys->Lock() == TRUE ){
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
				sys->UnLock();
			}
		}
		break;
	case CMD_EPG_SRV_DEL_RESERVE:
		{
			resParam->param = OLD_CMD_ERR;
			if( sys->Lock() == TRUE ){
				OLD_RESERVE_DATA oldItem;
				if( CopyReserveData(&oldItem, cmdParam) == TRUE){
					vector<DWORD> list;
					list.push_back(oldItem.dwReserveID);
					if(sys->reserveManager.DelReserveData(&list) == TRUE ){
						resParam->param = OLD_CMD_SUCCESS;
					}
				}
				sys->UnLock();
			}
		}
		break;
	case CMD_EPG_SRV_CHG_RESERVE:
		{
			resParam->param = OLD_CMD_ERR;
			if( sys->Lock() == TRUE ){
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
				sys->UnLock();
			}
		}

		break;
	case CMD_EPG_SRV_ADD_AUTO_ADD:
		{
			resParam->param = OLD_CMD_ERR;
			if( sys->Lock() == TRUE ){
				OLD_SEARCH_KEY oldItem;
				if( CopySearchKeyData(&oldItem, cmdParam) == TRUE){
					EPG_AUTO_ADD_DATA item;
					CopyOldNew(&oldItem, &item);

					if( sys->Lock() == TRUE ){
						sys->epgAutoAdd.AddData(&item);

						wstring savePath = L"";
						GetSettingPath(savePath);
						savePath += L"\\";
						savePath += EPG_AUTO_ADD_TEXT_NAME;

						sys->epgAutoAdd.SaveText(savePath.c_str());

						resParam->param = OLD_CMD_SUCCESS;

						sys->AutoAddReserveEPG(&item);
						sys->UnLock();
						sys->reserveManager.SendNotifyUpdate(NOTIFY_UPDATE_AUTOADD_EPG);
					}
				}
				sys->UnLock();
			}
		}
		break;
	case CMD_EPG_SRV_DEL_AUTO_ADD:
		{
			resParam->param = OLD_CMD_ERR;
			if( sys->Lock() == TRUE ){
				OLD_SEARCH_KEY oldItem;
				if( CopySearchKeyData(&oldItem, cmdParam) == TRUE){
					if( sys->Lock() == TRUE ){
						sys->epgAutoAdd.DelData((DWORD)oldItem.iAutoAddID);

						wstring savePath = L"";
						GetSettingPath(savePath);
						savePath += L"\\";
						savePath += EPG_AUTO_ADD_TEXT_NAME;

						sys->epgAutoAdd.SaveText(savePath.c_str());

						resParam->param = OLD_CMD_SUCCESS;
						sys->UnLock();
						sys->reserveManager.SendNotifyUpdate(NOTIFY_UPDATE_AUTOADD_EPG);
					}
				}
				sys->UnLock();
			}
		}
		break;
	case CMD_EPG_SRV_CHG_AUTO_ADD:
		{
			resParam->param = OLD_CMD_ERR;
			if( sys->Lock() == TRUE ){
				OLD_SEARCH_KEY oldItem;
				if( CopySearchKeyData(&oldItem, cmdParam) == TRUE){
					EPG_AUTO_ADD_DATA item;
					CopyOldNew(&oldItem, &item);

					if( sys->Lock() == TRUE ){
						sys->epgAutoAdd.ChgData(&item);

						wstring savePath = L"";
						GetSettingPath(savePath);
						savePath += L"\\";
						savePath += EPG_AUTO_ADD_TEXT_NAME;

						sys->epgAutoAdd.SaveText(savePath.c_str());

						resParam->param = OLD_CMD_SUCCESS;

						sys->AutoAddReserveEPG(&item);
						sys->UnLock();
						sys->reserveManager.SendNotifyUpdate(NOTIFY_UPDATE_AUTOADD_EPG);
					}
				}
				sys->UnLock();
			}
		}
		break;
	case CMD_EPG_SRV_SEARCH_PG_FIRST:
		{
			sys->oldSearchList.clear();
			resParam->param = OLD_CMD_ERR;
			if( sys->epgDB.IsLoadingData() == TRUE ){
				resParam->param = CMD_ERR_BUSY;
			}else{
				if( sys->Lock() == TRUE ){
					OLD_SEARCH_KEY oldItem;
					if( CopySearchKeyData(&oldItem, cmdParam) == TRUE){
						EPGDB_SEARCH_KEY_INFO item;
						CopyOldNew(&oldItem, &item);

						vector<EPGDB_SEARCH_KEY_INFO> key;
						vector<EPGDB_EVENT_INFO*> val;
						key.push_back(item);
						if( sys->epgDB.SearchEpg(&key, &val) == TRUE ){
							for( size_t i=0; i<val.size(); i++ ){
								OLD_EVENT_INFO_DATA3 add;
								add = *val[i];
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
					sys->UnLock();
				}
			}
		}
		break;
	case CMD_EPG_SRV_SEARCH_PG_NEXT:
		{
			resParam->param = OLD_CMD_ERR;
			if( sys->oldSearchList.size() == 0 ){
				resParam->param = OLD_CMD_ERR;
			}else{
				if( sys->Lock() == TRUE ){
					if( sys->oldSearchList.size() == 1 ){
						resParam->param = OLD_CMD_SUCCESS;
					}else{
						resParam->param = OLD_CMD_NEXT;
					}
					CreateEventInfoData3Stream(&sys->oldSearchList[0], resParam);
					sys->oldSearchList.erase(sys->oldSearchList.begin());
					sys->UnLock();
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
						EPGDB_EVENT_INFO* eventData;
						if(sys->epgDB.SearchEpg(reserveData.originalNetworkID, reserveData.transportStreamID, reserveData.serviceID, reserveData.eventID, &eventData) == TRUE ){
							_ConvertEpgInfoText2(eventData, eventText, reserveData.stationName);
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
				map<DWORD, EPG_AUTO_ADD_DATA*>::iterator itr;
				for( itr = sys->epgAutoAdd.dataIDMap.begin(); itr != sys->epgAutoAdd.dataIDMap.end(); itr++ ){
					list.push_back(*(itr->second));
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

				map<DWORD, EPG_AUTO_ADD_DATA*>::iterator itr;
				itr = sys->epgAutoAdd.dataIDMap.find(atoi(param.c_str()));
				if( itr != sys->epgAutoAdd.dataIDMap.end() ){
					htmlManager.GetChgAutoEpgPage(itr->second, "", &tunerList, sendParam);
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
						EPGDB_EVENT_INFO* eventData;
						if(sys->epgDB.SearchEpg(reserveData.originalNetworkID, reserveData.transportStreamID, reserveData.serviceID, reserveData.eventID, &eventData) == TRUE ){
							_ConvertEpgInfoText2(eventData, eventText, reserveData.stationName);
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
					sys->epgAutoAdd.AddData(&val);

					wstring savePath = L"";
					GetSettingPath(savePath);
					savePath += L"\\";
					savePath += EPG_AUTO_ADD_TEXT_NAME;

					sys->epgAutoAdd.SaveText(savePath.c_str());

					sys->AutoAddReserveEPG(&val);

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

				map<DWORD, EPG_AUTO_ADD_DATA*>::iterator itr;
				itr = sys->epgAutoAdd.dataIDMap.find(atoi(id.c_str()));
				if( itr != sys->epgAutoAdd.dataIDMap.end() ){
					htmlManager.GetChgAutoEpgPage(itr->second, param, &tunerList, sendParam);
				}
			}
			else if(url.find("/autoaddepgchgkey.html") == 0 ){
				string param = "";
				param.append((char*)recvParam->data, 0, recvParam->dataSize);

				OutputDebugStringA(param.c_str());

				EPG_AUTO_ADD_DATA val;
				if( htmlManager.GetAutoEpgParam(&val, recvParam) == TRUE ){
					sys->epgAutoAdd.ChgData(&val);

					wstring savePath = L"";
					GetSettingPath(savePath);
					savePath += L"\\";
					savePath += EPG_AUTO_ADD_TEXT_NAME;

					sys->epgAutoAdd.SaveText(savePath.c_str());

					sys->AutoAddReserveEPG(&val);

					htmlManager.GetChgAutoEpgPage(sendParam);
				}
			}
			else if(url.find("/autoaddepgdelkey.html") == 0 ){
				string id = "";
				string preset = "";
				Separate(url, "id=", url, id);
				int dataID = atoi(id.c_str());

				sys->epgAutoAdd.DelData(dataID);

				wstring savePath = L"";
				GetSettingPath(savePath);
				savePath += L"\\";
				savePath += EPG_AUTO_ADD_TEXT_NAME;

				sys->epgAutoAdd.SaveText(savePath.c_str());

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
		CHttpPublicFileSend send;
		send.SetPublicFolder(L"/file", sys->httpPublicFolder);
		send.HttpRequest(method, uri, &headerList, clientSock, stopEvent);
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
		sys->dlnaManager->HttpRequest(method, uri, &headerList, &reqReader, clientSock, stopEvent);
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

