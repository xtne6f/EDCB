#include "StdAfx.h"
#include "EpgTimerSrvMain.h"
#include "HttpServer.h"
#include "SyoboiCalUtil.h"
#include "../../UPnPCtrl/UpnpUtil.h"
#include "../../UPnPCtrl/UpnpSsdpUtil.h"
#include "../../Common/PipeServer.h"
#include "../../Common/TCPServer.h"
#include "../../Common/SendCtrlCmd.h"
#include "../../Common/PathUtil.h"
#include "../../Common/TimeUtil.h"
#include "../../Common/BlockLock.h"
#include <tlhelp32.h>

static const char UPNP_URN_DMS_1[] = "urn:schemas-upnp-org:device:MediaServer:1";

CEpgTimerSrvMain::CEpgTimerSrvMain()
	: reserveManager(notifyManager, epgDB)
	, requestStop(false)
	, requestResetServer(false)
	, requestReloadEpgChk(false)
	, requestShutdownMode(0)
	, nwtvUdp(false)
	, nwtvTcp(false)
{
	InitializeCriticalSection(&this->settingLock);
	this->requestEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
}

CEpgTimerSrvMain::~CEpgTimerSrvMain()
{
	CloseHandle(this->requestEvent);
	DeleteCriticalSection(&this->settingLock);
}

bool CEpgTimerSrvMain::Main(bool serviceFlag_)
{
	this->serviceFlag = serviceFlag_;

	DWORD awayMode;
	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	awayMode = GetVersionEx(&osvi) && osvi.dwMajorVersion >= 6 ? ES_AWAYMODE_REQUIRED : 0;

	wstring settingPath;
	GetSettingPath(settingPath);
	this->epgAutoAdd.ParseText((settingPath + L"\\" + EPG_AUTO_ADD_TEXT_NAME).c_str());
	this->manualAutoAdd.ParseText((settingPath + L"\\" + MANUAL_AUTO_ADD_TEXT_NAME).c_str());

	this->reserveManager.Initialize();
	ReloadSetting();

	//Pipeサーバースタート
	this->requestResetServer = true;
	CPipeServer pipeServer;
	if( pipeServer.StartServer(CMD2_EPG_SRV_EVENT_WAIT_CONNECT, CMD2_EPG_SRV_PIPE, CtrlCmdCallback, this, 0, GetCurrentProcessId()) ==  FALSE ){
		this->reserveManager.Finalize();
		return false;
	}
	CTCPServer tcpServer;
	CHttpServer httpServer;
	//UPnPのUDP(Port1900)部分を担当するサーバ
	UPNP_SERVER_HANDLE upnpCtrl = NULL;

	this->epgDB.ReloadEpgData();
	bool reloadEpgChkPending = true;
	WORD shutdownModePending = 0;
	DWORD shutdownTick = 0;

	HANDLE resumeTimer = NULL;
	__int64 resumeTime = 0;

	while( this->requestStop == false ){
		DWORD marginSec;
		bool resetServer;
		WORD shutdownMode;
		{
			CBlockLock lock(&this->settingLock);
			marginSec = this->wakeMarginSec;
			resetServer = this->requestResetServer;
			reloadEpgChkPending = reloadEpgChkPending || this->requestReloadEpgChk;
			shutdownMode = this->requestShutdownMode;
			this->requestResetServer = false;
			this->requestReloadEpgChk = false;
			this->requestShutdownMode = 0;
		}

		if( resetServer ){
			//サーバリセット処理
			unsigned short tcpPort_;
			unsigned short httpPort_;
			wstring httpPublicFolder_;
			wstring httpAcl;
			bool httpSaveLog_ = false;
			bool enableSsdpServer_;
			{
				CBlockLock lock(&this->settingLock);
				tcpPort_ = this->tcpPort;
				httpPort_ = this->httpPort;
				httpPublicFolder_ = this->httpPublicFolder;
				httpAcl = this->httpAccessControlList;
				httpSaveLog_ = this->httpSaveLog;
				enableSsdpServer_ = this->enableSsdpServer;
			}
			if( tcpPort_ == 0 ){
				tcpServer.StopServer();
			}else{
				tcpServer.StartServer(tcpPort_, CtrlCmdCallback, this, 0, GetCurrentProcessId());
			}
			if( upnpCtrl ){
				UPNP_SERVER_RemoveNotifyInfo(upnpCtrl, this->ssdpNotifyUuid.c_str());
				UPNP_SERVER_Stop(upnpCtrl);
				UPNP_SERVER_CloseHandle(&upnpCtrl);
			}
			if( httpPort_ == 0 ){
				httpServer.StopServer();
			}else{
				if( httpServer.StartServer(httpPort_, httpPublicFolder_.c_str(), InitLuaCallback, this, httpSaveLog_, httpAcl.c_str()) ){
					if( enableSsdpServer_ ){
						//"ddd.xml"の先頭から2KB以内に"<UDN>uuid:{UUID}</UDN>"が必要
						char dddBuf[2048] = {};
						HANDLE hFile = CreateFile((httpPublicFolder_ + L"\\dlna\\dms\\ddd.xml").c_str(),
						                          GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
						if( hFile != INVALID_HANDLE_VALUE ){
							DWORD dwRead;
							ReadFile(hFile, dddBuf, sizeof(dddBuf) - 1, &dwRead, NULL);
							CloseHandle(hFile);
						}
						string dddStr = dddBuf;
						size_t udnFrom = dddStr.find("<UDN>uuid:");
						if( udnFrom != string::npos && dddStr.size() > udnFrom + 10 + 36 && dddStr.compare(udnFrom + 10 + 36, 6, "</UDN>") == 0 ){
							this->ssdpNotifyUuid.assign(dddStr, udnFrom + 10, 36);
							this->ssdpNotifyPort = httpPort_;
							upnpCtrl = UPNP_SERVER_CreateHandle(NULL, NULL, UpnpMSearchReqCallback, this);
							UPNP_SERVER_Start(upnpCtrl);
							LPCSTR urnList[] = { UPNP_URN_DMS_1, UPNP_URN_CDS_1, UPNP_URN_CMS_1, UPNP_URN_AVT_1, NULL };
							for( int i = 0; urnList[i]; i++ ){
								UPNP_SERVER_AddNotifyInfo(upnpCtrl, this->ssdpNotifyUuid.c_str(), urnList[i], this->ssdpNotifyPort, "/dlna/dms/ddd.xml");
							}
						}else{
							OutputDebugString(L"Invalid ddd.xml\r\n");
						}
					}
				}
			}
		}

		if( shutdownModePending != 0 ){
			//30秒以内にシャットダウン問い合わせできなければキャンセル
			if( shutdownMode != 0 || GetTickCount() - shutdownTick > 30000 ){
				shutdownModePending = 0;
			}else if( IsSuspendOK() && IsUserWorking() == false && reloadEpgChkPending == false && this->epgDB.ReloadEpgData() != FALSE ){
				reloadEpgChkPending = true;
			}
		}
		if( reloadEpgChkPending && this->epgDB.IsLoadingData() == FALSE ){
			//リロード終わったので自動予約登録処理を行う
			this->reserveManager.CheckTuijyu();
			{
				CBlockLock lock(&this->settingLock);
				for( map<DWORD, EPG_AUTO_ADD_DATA>::const_iterator itr = this->epgAutoAdd.GetMap().begin(); itr != this->epgAutoAdd.GetMap().end(); itr++ ){
					AutoAddReserveEPG(itr->second);
				}
				for( map<DWORD, MANUAL_AUTO_ADD_DATA>::const_iterator itr = this->manualAutoAdd.GetMap().begin(); itr != this->manualAutoAdd.GetMap().end(); itr++ ){
					AutoAddReserveProgram(itr->second);
				}
			}
			reloadEpgChkPending = false;
			this->notifyManager.AddNotify(NOTIFY_UPDATE_EPGDATA);

			if( this->useSyoboi ){
				//しょぼいカレンダー対応
				CSyoboiCalUtil syoboi;
				vector<RESERVE_DATA> reserveList = this->reserveManager.GetReserveDataAll();
				vector<TUNER_RESERVE_INFO> tunerList = this->reserveManager.GetTunerReserveAll();
				syoboi.SendReserve(&reserveList, &tunerList);
			}
			if( shutdownModePending && IsSuspendOK() && IsUserWorking() == false ){
				if( 1 <= LOBYTE(shutdownModePending) && LOBYTE(shutdownModePending) <= 3 ){
					//シャットダウン問い合わせ
					if( QueryShutdown(HIBYTE(shutdownModePending), LOBYTE(shutdownModePending)) == false ){
						shutdownMode = shutdownModePending;
					}
				}
				shutdownModePending = 0;
			}
		}
		if( shutdownMode != 0 ){
			//シャットダウン処理
			if( shutdownMode == 0x01FF ){
				SetShutdown(4);
			}else if( IsSuspendOK() ){
				if( LOBYTE(shutdownMode) == 1 || LOBYTE(shutdownMode) == 2 ){
					//ストリーミングを終了する
					this->streamingManager.CloseAllFile();
					//スリープ抑止解除
					SetThreadExecutionState(ES_CONTINUOUS);
					//rebootFlag時は(指定+5分前)に復帰
					if( SetResumeTimer(&resumeTimer, &resumeTime, marginSec + (HIBYTE(shutdownMode) != 0 ? 300 : 0)) ){
						SetShutdown(LOBYTE(shutdownMode));
						if( HIBYTE(shutdownMode) != 0 ){
							//再起動問い合わせ
							if( QueryShutdown(1, 0) == false ){
								SetShutdown(4);
							}
						}
					}
				}else if( LOBYTE(shutdownMode) == 3 ){
					SetShutdown(3);
				}
			}
		}

		//復帰タイマ更新(powercfg /waketimersでデバッグ可能)
		SetResumeTimer(&resumeTimer, &resumeTime, marginSec);
		//スリープ抑止
		EXECUTION_STATE esFlags = shutdownModePending == 0 && IsSuspendOK() ? ES_CONTINUOUS : ES_CONTINUOUS | ES_SYSTEM_REQUIRED | awayMode;
		if( SetThreadExecutionState(esFlags) != esFlags ){
			_OutputDebugString(L"SetThreadExecutionState(0x%08x)\r\n", (DWORD)esFlags);
		}

		DWORD extra;
		//復帰タイマ更新とスリープ抑止のため、予約変化時か30秒で待機を解除する
		if( this->reserveManager.Wait(this->requestEvent, reloadEpgChkPending || shutdownModePending ? 200 : 30000, &extra) == WAIT_OBJECT_0 + 1 ){
			switch( HIWORD(extra) ){
			case CReserveManager::WAIT_EXTRA_EPGCAP_END:
				if( this->epgDB.ReloadEpgData() != FALSE ){
					//EPGリロード完了後にデフォルトのシャットダウン動作を試みる
					CBlockLock lock(&this->settingLock);
					reloadEpgChkPending = true;
					shutdownModePending = this->defShutdownMode;
					shutdownTick = GetTickCount();
				}
				break;
			case CReserveManager::WAIT_EXTRA_NEED_SHUTDOWN:
				{
					//要求されたシャットダウン動作を試みる
					CBlockLock lock(&this->settingLock);
					shutdownModePending = LOWORD(extra);
					if( LOBYTE(shutdownModePending) == 0 ){
						shutdownModePending = this->defShutdownMode;
					}
					shutdownTick = GetTickCount();
				}
				break;
			case CReserveManager::WAIT_EXTRA_RESERVE_MODIFIED:
				break;
			}
		}
	}

	if( resumeTimer != NULL ){
		CloseHandle(resumeTimer);
	}
	if( upnpCtrl ){
		UPNP_SERVER_Stop(upnpCtrl);
		UPNP_SERVER_CloseHandle(&upnpCtrl);
	}
	httpServer.StopServer();
	tcpServer.StopServer();
	pipeServer.StopServer();
	this->reserveManager.Finalize();
	return true;
}

void CEpgTimerSrvMain::StopMain()
{
	this->requestStop = true;
	SetEvent(this->requestEvent);
}

bool CEpgTimerSrvMain::IsSuspendOK()
{
	DWORD marginSec;
	bool ngFileStreaming_;
	{
		CBlockLock lock(&this->settingLock);
		if( IsFindNoSuspendExe() ){
			return false;
		}
		//rebootFlag時の復帰マージンを基準に3分余裕を加えたもの
		marginSec = this->wakeMarginSec + 300 + 180;
		ngFileStreaming_ = this->ngFileStreaming;
	}
	__int64 now = GetNowI64Time();
	//シャットダウン可能で復帰が間に合うときだけ
	return (ngFileStreaming_ == false || this->streamingManager.IsStreaming() == FALSE) &&
	       this->reserveManager.IsActive() == false &&
	       this->reserveManager.GetSleepReturnTime(now) > now + marginSec * I64_1SEC;
}

void CEpgTimerSrvMain::ReloadSetting()
{
	this->reserveManager.ReloadSetting();

	CBlockLock lock(&this->settingLock);

	wstring iniPath;
	GetModuleIniPath(iniPath);
	this->tcpPort = 0;
	if( GetPrivateProfileInt(L"SET", L"EnableTCPSrv", 0, iniPath.c_str()) != 0 ){
		this->tcpPort = (unsigned short)GetPrivateProfileInt(L"SET", L"TCPPort", 4510, iniPath.c_str());
	}
	this->httpPort = 0;
	int enableHttpSrv = GetPrivateProfileInt(L"SET", L"EnableHttpSrv", 0, iniPath.c_str());
	if( enableHttpSrv != 0 ){
		WCHAR buff[512];
		GetPrivateProfileString(L"SET", L"HttpPublicFolder", L"", buff, 512, iniPath.c_str());
		this->httpPublicFolder = buff;
		if( this->httpPublicFolder.empty() ){
			GetModuleFolderPath(this->httpPublicFolder);
			this->httpPublicFolder += L"\\HttpPublic";
		}
		ChkFolderPath(this->httpPublicFolder);
		if( this->dmsPublicFileList.empty() || CompareNoCase(this->httpPublicFolder, this->dmsPublicFileList[0].second) != 0 ){
			//公開フォルダの場所が変わったのでクリア
			this->dmsPublicFileList.clear();
		}
		GetPrivateProfileString(L"SET", L"HttpAccessControlList", L"+0.0.0.0/0", buff, 512, iniPath.c_str());
		this->httpAccessControlList = buff;
		this->httpPort = (unsigned short)GetPrivateProfileInt(L"SET", L"HttpPort", 5510, iniPath.c_str());
		this->httpSaveLog = enableHttpSrv == 2;
	}
	this->enableSsdpServer = GetPrivateProfileInt(L"SET", L"EnableDMS", 0, iniPath.c_str()) != 0;

	this->requestResetServer = true;
	SetEvent(this->requestEvent);

	this->wakeMarginSec = GetPrivateProfileInt(L"SET", L"WakeTime", 5, iniPath.c_str()) * 60;
	this->autoAddHour = GetPrivateProfileInt(L"SET", L"AutoAddDays", 8, iniPath.c_str()) * 24 +
	                    GetPrivateProfileInt(L"SET", L"AutoAddHour", 0, iniPath.c_str());
	this->chkGroupEvent = GetPrivateProfileInt(L"SET", L"ChkGroupEvent", 1, iniPath.c_str()) != 0;
	this->defShutdownMode = MAKEWORD((GetPrivateProfileInt(L"SET", L"RecEndMode", 0, iniPath.c_str()) + 3) % 4 + 1,
	                                 (GetPrivateProfileInt(L"SET", L"Reboot", 0, iniPath.c_str()) != 0 ? 1 : 0));
	this->ngUsePCTime = 0;
	if( GetPrivateProfileInt(L"NO_SUSPEND", L"NoUsePC", 0, iniPath.c_str()) != 0 ){
		this->ngUsePCTime = GetPrivateProfileInt(L"NO_SUSPEND", L"NoUsePCTime", 3, iniPath.c_str()) * 60 * 1000;
		//閾値が0のときは常に使用中扱い
		if( this->ngUsePCTime == 0 ){
			this->ngUsePCTime = MAXDWORD;
		}
	}
	this->ngFileStreaming = GetPrivateProfileInt(L"NO_SUSPEND", L"NoFileStreaming", 0, iniPath.c_str()) != 0;
	this->useSyoboi = GetPrivateProfileInt(L"SYOBOI", L"use", 0, iniPath.c_str()) != 0;

	this->noSuspendExeList.clear();
	int count = GetPrivateProfileInt(L"NO_SUSPEND", L"Count", 0, iniPath.c_str());
	if( count == 0 ){
		this->noSuspendExeList.push_back(L"EpgDataCap_Bon.exe");
	}
	for( int i = 0; i < count; i++ ){
		WCHAR key[16];
		wsprintf(key, L"%d", i);
		WCHAR buff[256];
		GetPrivateProfileString(L"NO_SUSPEND", key, L"", buff, 256, iniPath.c_str());
		if( buff[0] != L'\0' ){
			this->noSuspendExeList.push_back(buff);
		}
	}

	this->tvtestUseBon.clear();
	count = GetPrivateProfileInt(L"TVTEST", L"Num", 0, iniPath.c_str());
	for( int i = 0; i < count; i++ ){
		WCHAR key[16];
		wsprintf(key, L"%d", i);
		WCHAR buff[256];
		GetPrivateProfileString(L"TVTEST", key, L"", buff, 256, iniPath.c_str());
		if( buff[0] != L'\0' ){
			this->tvtestUseBon.push_back(buff);
		}
	}
}

pair<wstring, REC_SETTING_DATA> CEpgTimerSrvMain::LoadRecSetData(WORD preset) const
{
	wstring iniPath;
	GetModuleIniPath(iniPath);
	WCHAR buff[512];
	WCHAR defName[32];
	WCHAR defFolderName[2][32];
	buff[preset == 0 ? 0 : wsprintf(buff, L"%d", preset)] = L'\0';
	wsprintf(defName, L"REC_DEF%s", buff);
	wsprintf(defFolderName[0], L"REC_DEF_FOLDER%s", buff);
	wsprintf(defFolderName[1], L"REC_DEF_FOLDER_1SEG%s", buff);

	pair<wstring, REC_SETTING_DATA> ret;
	GetPrivateProfileString(defName, L"SetName", L"", buff, 512, iniPath.c_str());
	ret.first = preset == 0 ? L"デフォルト" : buff;
	REC_SETTING_DATA& rs = ret.second;
	rs.recMode = (BYTE)GetPrivateProfileInt(defName, L"RecMode", 1, iniPath.c_str());
	rs.priority = (BYTE)GetPrivateProfileInt(defName, L"Priority", 2, iniPath.c_str());
	rs.tuijyuuFlag = (BYTE)GetPrivateProfileInt(defName, L"TuijyuuFlag", 1, iniPath.c_str());
	rs.serviceMode = (BYTE)GetPrivateProfileInt(defName, L"ServiceMode", 0, iniPath.c_str());
	rs.pittariFlag = (BYTE)GetPrivateProfileInt(defName, L"PittariFlag", 0, iniPath.c_str());
	GetPrivateProfileString(defName, L"BatFilePath", L"", buff, 512, iniPath.c_str());
	rs.batFilePath = buff;
	for( int i = 0; i < 2; i++ ){
		vector<REC_FILE_SET_INFO>& recFolderList = i == 0 ? rs.recFolderList : rs.partialRecFolder;
		int count = GetPrivateProfileInt(defFolderName[i], L"Count", 0, iniPath.c_str());
		for( int j = 0; j < count; j++ ){
			recFolderList.resize(j + 1);
			WCHAR key[32];
			wsprintf(key, L"%d", j);
			GetPrivateProfileString(defFolderName[i], key, L"", buff, 512, iniPath.c_str());
			recFolderList[j].recFolder = buff;
			wsprintf(key, L"WritePlugIn%d", j);
			GetPrivateProfileString(defFolderName[i], key, L"Write_Default.dll", buff, 512, iniPath.c_str());
			recFolderList[j].writePlugIn = buff;
			wsprintf(key, L"RecNamePlugIn%d", j);
			GetPrivateProfileString(defFolderName[i], key, L"", buff, 512, iniPath.c_str());
			recFolderList[j].recNamePlugIn = buff;
		}
	}
	rs.suspendMode = (BYTE)GetPrivateProfileInt(defName, L"SuspendMode", 0, iniPath.c_str());
	rs.rebootFlag = (BYTE)GetPrivateProfileInt(defName, L"RebootFlag", 0, iniPath.c_str());
	rs.useMargineFlag = (BYTE)GetPrivateProfileInt(defName, L"UseMargineFlag", 0, iniPath.c_str());
	rs.startMargine = GetPrivateProfileInt(defName, L"StartMargine", 0, iniPath.c_str());
	rs.endMargine = GetPrivateProfileInt(defName, L"EndMargine", 0, iniPath.c_str());
	rs.continueRecFlag = (BYTE)GetPrivateProfileInt(defName, L"ContinueRec", 0, iniPath.c_str());
	rs.partialRecFlag = (BYTE)GetPrivateProfileInt(defName, L"PartialRec", 0, iniPath.c_str());
	rs.tunerID = GetPrivateProfileInt(defName, L"TunerID", 0, iniPath.c_str());
	return ret;
}

bool CEpgTimerSrvMain::SetResumeTimer(HANDLE* resumeTimer, __int64* resumeTime, DWORD marginSec)
{
	__int64 returnTime = this->reserveManager.GetSleepReturnTime(GetNowI64Time() + marginSec * I64_1SEC);
	if( returnTime == LLONG_MAX ){
		if( *resumeTimer != NULL ){
			CloseHandle(*resumeTimer);
			*resumeTimer = NULL;
		}
		return true;
	}
	__int64 setTime = returnTime - marginSec * I64_1SEC;
	if( *resumeTimer != NULL && *resumeTime == setTime ){
		//同時刻でセット済み
		return true;
	}
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
			return true;
		}
		CloseHandle(*resumeTimer);
		*resumeTimer = NULL;
	}
	return false;
}

void CEpgTimerSrvMain::SetShutdown(BYTE shutdownMode)
{
	HANDLE hToken;
	if ( OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken) ){
		TOKEN_PRIVILEGES tokenPriv;
		LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tokenPriv.Privileges[0].Luid);
		tokenPriv.PrivilegeCount = 1;
		tokenPriv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		AdjustTokenPrivileges(hToken, FALSE, &tokenPriv, 0, NULL, NULL);
		CloseHandle(hToken);
	}
	if( shutdownMode == 1 ){
		//スタンバイ(同期)
		SetSystemPowerState(TRUE, FALSE);
	}else if( shutdownMode == 2 ){
		//休止(同期)
		SetSystemPowerState(FALSE, FALSE);
	}else if( shutdownMode == 3 ){
		//電源断(非同期)
		ExitWindowsEx(EWX_POWEROFF, 0);
	}else if( shutdownMode == 4 ){
		//再起動(非同期)
		ExitWindowsEx(EWX_REBOOT, 0);
	}
}

bool CEpgTimerSrvMain::QueryShutdown(BYTE rebootFlag, BYTE suspendMode)
{
	CSendCtrlCmd ctrlCmd;
	map<DWORD, DWORD> registGUI;
	this->notifyManager.GetRegistGUI(&registGUI);
	for( map<DWORD, DWORD>::iterator itr = registGUI.begin(); itr != registGUI.end(); itr++ ){
		ctrlCmd.SetPipeSetting(CMD2_GUI_CTRL_WAIT_CONNECT, CMD2_GUI_CTRL_PIPE, itr->first);
		//通信できる限り常に成功するので、重複問い合わせを考慮する必要はない
		if( suspendMode == 0 && ctrlCmd.SendGUIQueryReboot(rebootFlag) == CMD_SUCCESS ||
		    suspendMode != 0 && ctrlCmd.SendGUIQuerySuspend(rebootFlag, suspendMode) == CMD_SUCCESS ){
			return true;
		}
	}
	return false;
}

bool CEpgTimerSrvMain::IsUserWorking() const
{
	CBlockLock lock(&this->settingLock);

	//最終入力時刻取得
	LASTINPUTINFO lii;
	lii.cbSize = sizeof(LASTINPUTINFO);
	return this->ngUsePCTime == MAXDWORD || this->ngUsePCTime && GetLastInputInfo(&lii) && GetTickCount() - lii.dwTime < this->ngUsePCTime;
}

bool CEpgTimerSrvMain::IsFindNoSuspendExe() const
{
	CBlockLock lock(&this->settingLock);

	if( this->noSuspendExeList.empty() == false ){
		//Toolhelpスナップショットを作成する
		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if( hSnapshot != INVALID_HANDLE_VALUE ){
			bool found = false;
			PROCESSENTRY32 procent;
			procent.dwSize = sizeof(PROCESSENTRY32);
			if( Process32First(hSnapshot, &procent) != FALSE ){
				do{
					for( size_t i = 0; i < this->noSuspendExeList.size(); i++ ){
						//procent.szExeFileにプロセス名
						wstring strExe = wstring(procent.szExeFile).substr(0, this->noSuspendExeList[i].size());
						if( CompareNoCase(strExe, this->noSuspendExeList[i]) == 0 ){
							_OutputDebugString(L"起動exe:%s\r\n", procent.szExeFile);
							found = true;
							break;
						}
					}
				}while( found == false && Process32Next(hSnapshot, &procent) != FALSE );
			}
			CloseHandle(hSnapshot);
			return found;
		}
	}
	return false;
}

bool CEpgTimerSrvMain::AutoAddReserveEPG(const EPG_AUTO_ADD_DATA& data)
{
	bool modified = false;
	vector<RESERVE_DATA> setList;
	int addCount = 0;
	int autoAddHour_;
	bool chkGroupEvent_;
	{
		CBlockLock lock(&this->settingLock);
		autoAddHour_ = this->autoAddHour;
		chkGroupEvent_ = this->chkGroupEvent;
	}
	__int64 now = GetNowI64Time();

	vector<std::unique_ptr<CEpgDBManager::SEARCH_RESULT_EVENT_DATA>> resultList;
	vector<EPGDB_SEARCH_KEY_INFO> key(1, data.searchInfo);
	this->epgDB.SearchEpg(&key, &resultList);
	for( size_t i = 0; i < resultList.size(); i++ ){
		const EPGDB_EVENT_INFO& info = resultList[i]->info;
		//時間未定でなく対象期間内かどうか
		if( info.StartTimeFlag != 0 && info.DurationFlag != 0 &&
		    now < ConvertI64Time(info.start_time) && ConvertI64Time(info.start_time) < now + autoAddHour_ * 60 * 60 * I64_1SEC ){
			addCount++;
			if( this->reserveManager.IsFindReserve(info.original_network_id, info.transport_stream_id, info.service_id, info.event_id) == false ){
				bool found = false;
				if( info.eventGroupInfo != NULL && chkGroupEvent_ ){
					//イベントグループのチェックをする
					for( size_t j = 0; found == false && j < info.eventGroupInfo->eventDataList.size(); j++ ){
						//group_typeは必ず1(イベント共有)
						const EPGDB_EVENT_DATA& e = info.eventGroupInfo->eventDataList[j];
						if( this->reserveManager.IsFindReserve(e.original_network_id, e.transport_stream_id, e.service_id, e.event_id) ){
							found = true;
							break;
						}
						//追加前予約のチェックをする
						for( size_t k = 0; k < setList.size(); k++ ){
							if( setList[k].originalNetworkID == e.original_network_id &&
							    setList[k].transportStreamID == e.transport_stream_id &&
							    setList[k].serviceID == e.service_id &&
							    setList[k].eventID == e.event_id ){
								found = true;
								break;
							}
						}
					}
				}
				//追加前予約のチェックをする
				for( size_t j = 0; found == false && j < setList.size(); j++ ){
					if( setList[j].originalNetworkID == info.original_network_id &&
					    setList[j].transportStreamID == info.transport_stream_id &&
					    setList[j].serviceID == info.service_id &&
					    setList[j].eventID == info.event_id ){
						found = true;
					}
				}
				if( found == false ){
					//まだ存在しないので追加対象
					setList.resize(setList.size() + 1);
					RESERVE_DATA& item = setList.back();
					if( info.shortInfo != NULL ){
						item.title = info.shortInfo->event_name;
					}
					item.startTime = info.start_time;
					item.startTimeEpg = item.startTime;
					item.durationSecond = info.durationSec;
					this->epgDB.SearchServiceName(info.original_network_id, info.transport_stream_id, info.service_id, item.stationName);
					item.originalNetworkID = info.original_network_id;
					item.transportStreamID = info.transport_stream_id;
					item.serviceID = info.service_id;
					item.eventID = info.event_id;
					item.recSetting = data.recSetting;
					if( data.searchInfo.chkRecEnd != 0 && this->reserveManager.IsFindRecEventInfo(info, data.searchInfo.chkRecDay) ){
						item.recSetting.recMode = RECMODE_NO;
					}
					item.comment = L"EPG自動予約";
					if( resultList[i]->findKey.empty() == false ){
						item.comment += L"(" + resultList[i]->findKey + L")";
						Replace(item.comment, L"\r", L"");
						Replace(item.comment, L"\n", L"");
					}
				}
			}else if( data.searchInfo.chkRecEnd != 0 && this->reserveManager.IsFindRecEventInfo(info, data.searchInfo.chkRecDay) ){
				//録画済みなので無効でない予約は無効にする
				if( this->reserveManager.ChgAutoAddNoRec(info.original_network_id, info.transport_stream_id, info.service_id, info.event_id) ){
					modified = true;
				}
			}
		}
	}
	if( setList.empty() == false && this->reserveManager.AddReserveData(setList, true) ){
		modified = true;
	}
	CBlockLock lock(&this->settingLock);
	//addCountは参考程度の情報。保存もされないので更新を通知する必要はない
	this->epgAutoAdd.SetAddCount(data.dataID, addCount);
	return modified;
}

bool CEpgTimerSrvMain::AutoAddReserveProgram(const MANUAL_AUTO_ADD_DATA& data)
{
	vector<RESERVE_DATA> setList;
	SYSTEMTIME baseTime;
	GetLocalTime(&baseTime);
	__int64 now = ConvertI64Time(baseTime);
	baseTime.wHour = 0;
	baseTime.wMinute = 0;
	baseTime.wSecond = 0;
	baseTime.wMilliseconds = 0;
	__int64 baseStartTime = ConvertI64Time(baseTime);

	for( int i = 0; i < 8; i++ ){
		//今日から8日分を調べる
		if( data.dayOfWeekFlag >> ((i + baseTime.wDayOfWeek) % 7) & 1 ){
			__int64 startTime = baseStartTime + (data.startTime + i * 24 * 60 * 60) * I64_1SEC;
			if( startTime > now ){
				//同一時間の予約がすでにあるかチェック
				if( this->reserveManager.IsFindProgramReserve(
				    data.originalNetworkID, data.transportStreamID, data.serviceID, startTime, data.durationSecond) == false ){
					//見つからなかったので予約追加
					setList.resize(setList.size() + 1);
					RESERVE_DATA& item = setList.back();
					item.title = data.title;
					ConvertSystemTime(startTime, &item.startTime); 
					item.startTimeEpg = item.startTime;
					item.durationSecond = data.durationSecond;
					item.stationName = data.stationName;
					item.originalNetworkID = data.originalNetworkID;
					item.transportStreamID = data.transportStreamID;
					item.serviceID = data.serviceID;
					item.eventID = 0xFFFF;
					item.recSetting = data.recSetting;
				}
			}
		}
	}
	return setList.empty() == false && this->reserveManager.AddReserveData(setList);
}

static void SearchPgCallback(vector<CEpgDBManager::SEARCH_RESULT_EVENT>* pval, void* param)
{
	vector<EPGDB_EVENT_INFO*> valp;
	valp.reserve(pval->size());
	for( size_t i = 0; i < pval->size(); i++ ){
		valp.push_back((*pval)[i].info);
	}
	CMD_STREAM *resParam = (CMD_STREAM*)param;
	resParam->param = CMD_SUCCESS;
	resParam->data = NewWriteVALUE(&valp, resParam->dataSize);
}

static void EnumPgInfoCallback(vector<EPGDB_EVENT_INFO*>* pval, void* param)
{
	CMD_STREAM *resParam = (CMD_STREAM*)param;
	resParam->param = CMD_SUCCESS;
	resParam->data = NewWriteVALUE(pval, resParam->dataSize);
}

static void EnumPgAllCallback(vector<EPGDB_SERVICE_EVENT_INFO>* pval, void* param)
{
	vector<EPGDB_SERVICE_EVENT_INFO*> valp;
	valp.reserve(pval->size());
	for( size_t i = 0; i < pval->size(); i++ ){
		valp.push_back(&(*pval)[i]);
	}
	CMD_STREAM *resParam = (CMD_STREAM*)param;
	resParam->param = CMD_SUCCESS;
	resParam->data = NewWriteVALUE(&valp, resParam->dataSize);
}

int CALLBACK CEpgTimerSrvMain::CtrlCmdCallback(void* param, CMD_STREAM* cmdParam, CMD_STREAM* resParam)
{
	CEpgTimerSrvMain* sys = (CEpgTimerSrvMain*)param;

	resParam->dataSize = 0;
	resParam->param = CMD_ERR;


	switch( cmdParam->param ){
	case CMD2_EPG_SRV_RELOAD_EPG:
		if( sys->epgDB.IsLoadingData() != FALSE ){
			resParam->param = CMD_ERR_BUSY;
		}else if( sys->epgDB.ReloadEpgData() != FALSE ){
			CBlockLock lock(&sys->settingLock);
			sys->requestReloadEpgChk = true;
			SetEvent(sys->requestEvent);
			resParam->param = CMD_SUCCESS;
		}
		break;
	case CMD2_EPG_SRV_RELOAD_SETTING:
		sys->ReloadSetting();
		resParam->param = CMD_SUCCESS;
		break;
	case CMD2_EPG_SRV_CLOSE:
		//サービスは停止できない
		if( sys->serviceFlag == false ){
			sys->StopMain();
			resParam->param = CMD_SUCCESS;
		}
		break;
	case CMD2_EPG_SRV_REGIST_GUI:
		{
			DWORD processID = 0;
			if( ReadVALUE(&processID, cmdParam->data, cmdParam->dataSize, NULL) ){
				//CPipeServerの仕様的にこの時点で相手と通信できるとは限らない。接続待機用イベントが作成されるまで少し待つ
				wstring eventName;
				Format(eventName, L"%s%d", CMD2_GUI_CTRL_WAIT_CONNECT, processID);
				for( int i = 0; i < 100; i++ ){
					HANDLE waitEvent = OpenEvent(SYNCHRONIZE, FALSE, eventName.c_str());
					if( waitEvent ){
						CloseHandle(waitEvent);
						break;
					}
					Sleep(100);
				}
				resParam->param = CMD_SUCCESS;
				sys->notifyManager.RegistGUI(processID);
			}
		}
		break;
	case CMD2_EPG_SRV_UNREGIST_GUI:
		{
			DWORD processID = 0;
			if( ReadVALUE(&processID, cmdParam->data, cmdParam->dataSize, NULL) ){
				resParam->param = CMD_SUCCESS;
				sys->notifyManager.UnRegistGUI(processID);
			}
		}
		break;
	case CMD2_EPG_SRV_REGIST_GUI_TCP:
		{
			REGIST_TCP_INFO val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) ){
				resParam->param = CMD_SUCCESS;
				sys->notifyManager.RegistTCP(val);
			}
		}
		break;
	case CMD2_EPG_SRV_UNREGIST_GUI_TCP:
		{
			REGIST_TCP_INFO val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) ){
				resParam->param = CMD_SUCCESS;
				sys->notifyManager.UnRegistTCP(val);
			}
		}
		break;
	case CMD2_EPG_SRV_ENUM_RESERVE:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ENUM_RESERVE\r\n");
			vector<RESERVE_DATA> list = sys->reserveManager.GetReserveDataAll();
			resParam->data = NewWriteVALUE(&list, resParam->dataSize);
			resParam->param = CMD_SUCCESS;
		}
		break;
	case CMD2_EPG_SRV_GET_RESERVE:
		{
			OutputDebugString(L"CMD2_EPG_SRV_GET_RESERVE\r\n");
			DWORD reserveID;
			if( ReadVALUE(&reserveID, cmdParam->data, cmdParam->dataSize, NULL) ){
				RESERVE_DATA info;
				if( sys->reserveManager.GetReserveData(reserveID, &info) ){
					resParam->data = NewWriteVALUE(&info, resParam->dataSize);
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD2_EPG_SRV_ADD_RESERVE:
		{
			vector<RESERVE_DATA> list;
			if( ReadVALUE(&list, cmdParam->data, cmdParam->dataSize, NULL) &&
			    sys->reserveManager.AddReserveData(list) ){
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_DEL_RESERVE:
		{
			vector<DWORD> list;
			if( ReadVALUE(&list, cmdParam->data, cmdParam->dataSize, NULL) ){
				sys->reserveManager.DelReserveData(list);
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_CHG_RESERVE:
		{
			vector<RESERVE_DATA> list;
			if( ReadVALUE(&list, cmdParam->data, cmdParam->dataSize, NULL) &&
			    sys->reserveManager.ChgReserveData(list) ){
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_ENUM_RECINFO:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ENUM_RECINFO\r\n");
			vector<REC_FILE_INFO> list = sys->reserveManager.GetRecFileInfoAll();
			resParam->data = NewWriteVALUE(&list, resParam->dataSize);
			resParam->param = CMD_SUCCESS;
		}
		break;
	case CMD2_EPG_SRV_DEL_RECINFO:
		{
			vector<DWORD> list;
			if( ReadVALUE(&list, cmdParam->data, cmdParam->dataSize, NULL) ){
				sys->reserveManager.DelRecFileInfo(list);
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_ENUM_SERVICE:
		OutputDebugString(L"CMD2_EPG_SRV_ENUM_SERVICE\r\n");
		if( sys->epgDB.IsInitialLoadingDataDone() == FALSE ){
			resParam->param = CMD_ERR_BUSY;
		}else{
			vector<EPGDB_SERVICE_INFO> list;
			if( sys->epgDB.GetServiceList(&list) != FALSE ){
				resParam->data = NewWriteVALUE(&list, resParam->dataSize);
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_ENUM_PG_INFO:
		OutputDebugString(L"CMD2_EPG_SRV_ENUM_PG_INFO\r\n");
		if( sys->epgDB.IsInitialLoadingDataDone() == FALSE ){
			resParam->param = CMD_ERR_BUSY;
		}else{
			LONGLONG serviceKey;
			if( ReadVALUE(&serviceKey, cmdParam->data, cmdParam->dataSize, NULL) ){
				sys->epgDB.EnumEventInfo(serviceKey, EnumPgInfoCallback, resParam);
			}
		}
		break;
	case CMD2_EPG_SRV_SEARCH_PG:
		OutputDebugString(L"CMD2_EPG_SRV_SEARCH_PG\r\n");
		if( sys->epgDB.IsInitialLoadingDataDone() == FALSE ){
			resParam->param = CMD_ERR_BUSY;
		}else{
			vector<EPGDB_SEARCH_KEY_INFO> key;
			if( ReadVALUE(&key, cmdParam->data, cmdParam->dataSize, NULL) ){
				sys->epgDB.SearchEpg(&key, SearchPgCallback, resParam);
			}
		}
		break;
	case CMD2_EPG_SRV_GET_PG_INFO:
		OutputDebugString(L"CMD2_EPG_SRV_GET_PG_INFO\r\n");
		if( sys->epgDB.IsInitialLoadingDataDone() == FALSE ){
			resParam->param = CMD_ERR_BUSY;
		}else{
			ULONGLONG key;
			EPGDB_EVENT_INFO val;
			if( ReadVALUE(&key, cmdParam->data, cmdParam->dataSize, NULL) &&
			    sys->epgDB.SearchEpg(key>>48&0xFFFF, key>>32&0xFFFF, key>>16&0xFFFF, key&0xFFFF, &val) ){
				resParam->data = NewWriteVALUE(&val, resParam->dataSize);
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_CHK_SUSPEND:
		if( sys->IsSuspendOK() ){
			resParam->param = CMD_SUCCESS;
		}
		break;
	case CMD2_EPG_SRV_SUSPEND:
		{
			WORD val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) && sys->IsSuspendOK() ){
				CBlockLock lock(&sys->settingLock);
				if( HIBYTE(val) == 0xFF ){
					val = MAKEWORD(LOBYTE(val), HIBYTE(sys->defShutdownMode));
				}
				sys->requestShutdownMode = val;
				SetEvent(sys->requestEvent);
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_REBOOT:
		{
			CBlockLock lock(&sys->settingLock);
			sys->requestShutdownMode = 0x01FF;
			SetEvent(sys->requestEvent);
			resParam->param = CMD_SUCCESS;
		}
		break;
	case CMD2_EPG_SRV_EPG_CAP_NOW:
		if( sys->epgDB.IsInitialLoadingDataDone() == FALSE ){
			resParam->param = CMD_ERR_BUSY;
		}else if( sys->reserveManager.RequestStartEpgCap() ){
			resParam->param = CMD_SUCCESS;
		}
		break;
	case CMD2_EPG_SRV_ENUM_AUTO_ADD:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ENUM_AUTO_ADD\r\n");
			vector<EPG_AUTO_ADD_DATA> val;
			{
				CBlockLock lock(&sys->settingLock);
				map<DWORD, EPG_AUTO_ADD_DATA>::const_iterator itr;
				for( itr = sys->epgAutoAdd.GetMap().begin(); itr != sys->epgAutoAdd.GetMap().end(); itr++ ){
					val.push_back(itr->second);
				}
			}
			resParam->data = NewWriteVALUE(&val, resParam->dataSize);
			resParam->param = CMD_SUCCESS;
		}
		break;
	case CMD2_EPG_SRV_ADD_AUTO_ADD:
		{
			vector<EPG_AUTO_ADD_DATA> val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) ){
				{
					CBlockLock lock(&sys->settingLock);
					for( size_t i = 0; i < val.size(); i++ ){
						val[i].dataID = sys->epgAutoAdd.AddData(val[i]);
					}
					sys->epgAutoAdd.SaveText();
				}
				for( size_t i = 0; i < val.size(); i++ ){
					sys->AutoAddReserveEPG(val[i]);
				}
				sys->notifyManager.AddNotify(NOTIFY_UPDATE_AUTOADD_EPG);
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_DEL_AUTO_ADD:
		{
			vector<DWORD> val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) ){
				CBlockLock lock(&sys->settingLock);
				for( size_t i = 0; i < val.size(); i++ ){
					sys->epgAutoAdd.DelData(val[i]);
				}
				sys->epgAutoAdd.SaveText();
				sys->notifyManager.AddNotify(NOTIFY_UPDATE_AUTOADD_EPG);
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_CHG_AUTO_ADD:
		{
			vector<EPG_AUTO_ADD_DATA> val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL ) ){
				{
					CBlockLock lock(&sys->settingLock);
					for( size_t i = 0; i < val.size(); i++ ){
						if( sys->epgAutoAdd.ChgData(val[i]) == false ){
							val.erase(val.begin() + (i--));
						}
					}
					sys->epgAutoAdd.SaveText();
				}
				for( size_t i = 0; i < val.size(); i++ ){
					sys->AutoAddReserveEPG(val[i]);
				}
				sys->notifyManager.AddNotify(NOTIFY_UPDATE_AUTOADD_EPG);
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_ENUM_MANU_ADD:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ENUM_MANU_ADD\r\n");
			vector<MANUAL_AUTO_ADD_DATA> val;
			{
				CBlockLock lock(&sys->settingLock);
				map<DWORD, MANUAL_AUTO_ADD_DATA>::const_iterator itr;
				for( itr = sys->manualAutoAdd.GetMap().begin(); itr != sys->manualAutoAdd.GetMap().end(); itr++ ){
					val.push_back(itr->second);
				}
			}
			resParam->data = NewWriteVALUE(&val, resParam->dataSize);
			resParam->param = CMD_SUCCESS;
		}
		break;
	case CMD2_EPG_SRV_ADD_MANU_ADD:
		{
			vector<MANUAL_AUTO_ADD_DATA> val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) ){
				{
					CBlockLock lock(&sys->settingLock);
					for( size_t i = 0; i < val.size(); i++ ){
						val[i].dataID = sys->manualAutoAdd.AddData(val[i]);
					}
					sys->manualAutoAdd.SaveText();
				}
				for( size_t i = 0; i < val.size(); i++ ){
					sys->AutoAddReserveProgram(val[i]);
				}
				sys->notifyManager.AddNotify(NOTIFY_UPDATE_AUTOADD_MANUAL);
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_DEL_MANU_ADD:
		{
			vector<DWORD> val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) ){
				CBlockLock lock(&sys->settingLock);
				for( size_t i = 0; i < val.size(); i++ ){
					sys->manualAutoAdd.DelData(val[i]);
				}
				sys->manualAutoAdd.SaveText();
				sys->notifyManager.AddNotify(NOTIFY_UPDATE_AUTOADD_MANUAL);
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_CHG_MANU_ADD:
		{
			vector<MANUAL_AUTO_ADD_DATA> val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL ) ){
				{
					CBlockLock lock(&sys->settingLock);
					for( size_t i = 0; i < val.size(); i++ ){
						if( sys->manualAutoAdd.ChgData(val[i]) == false ){
							val.erase(val.begin() + (i--));
						}
					}
					sys->manualAutoAdd.SaveText();
				}
				for( size_t i = 0; i < val.size(); i++ ){
					sys->AutoAddReserveProgram(val[i]);
				}
				sys->notifyManager.AddNotify(NOTIFY_UPDATE_AUTOADD_MANUAL);
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_ENUM_TUNER_RESERVE:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ENUM_TUNER_RESERVE\r\n");
			vector<TUNER_RESERVE_INFO> list = sys->reserveManager.GetTunerReserveAll();
			resParam->data = NewWriteVALUE(&list, resParam->dataSize);
			resParam->param = CMD_SUCCESS;
		}
		break;
	case CMD2_EPG_SRV_FILE_COPY:
		{
			wstring val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) && CompareNoCase(val, L"ChSet5.txt") == 0 ){
				wstring path;
				GetSettingPath(path);
				HANDLE hFile = CreateFile((path + L"\\ChSet5.txt").c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
				if( hFile != INVALID_HANDLE_VALUE ){
					DWORD dwFileSize = GetFileSize(hFile, NULL);
					if( dwFileSize != INVALID_FILE_SIZE && dwFileSize != 0 ){
						BYTE* data = new BYTE[dwFileSize];
						DWORD dwRead;
						if( ReadFile(hFile, data, dwFileSize, &dwRead, NULL) && dwRead != 0 ){
							resParam->dataSize = dwRead;
							resParam->data = data;
						}else{
							delete[] data;
						}
					}
					CloseHandle(hFile);
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD2_EPG_SRV_ENUM_PG_ALL:
		OutputDebugString(L"CMD2_EPG_SRV_ENUM_PG_ALL\r\n");
		if( sys->epgDB.IsInitialLoadingDataDone() == FALSE ){
			resParam->param = CMD_ERR_BUSY;
		}else{
			sys->epgDB.EnumEventAll(EnumPgAllCallback, resParam);
		}
		break;
	case CMD2_EPG_SRV_ENUM_PLUGIN:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ENUM_PLUGIN\r\n");
			WORD mode;
			if( ReadVALUE(&mode, cmdParam->data, cmdParam->dataSize, NULL) && (mode == 1 || mode == 2) ){
				wstring path;
				GetModuleFolderPath(path);
				WIN32_FIND_DATA findData;
				//指定フォルダのファイル一覧取得
				HANDLE hFind = FindFirstFile((path + (mode == 1 ? L"\\RecName\\RecName*.dll" : L"\\Write\\Write*.dll")).c_str(), &findData);
				if( hFind != INVALID_HANDLE_VALUE ){
					vector<wstring> fileList;
					do{
						if( (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 && IsExt(findData.cFileName, L".dll") ){
							fileList.push_back(findData.cFileName);
						}
					}while( FindNextFile(hFind, &findData) );
					FindClose(hFind);
					resParam->data = NewWriteVALUE(&fileList, resParam->dataSize);
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD2_EPG_SRV_GET_CHG_CH_TVTEST:
		{
			OutputDebugString(L"CMD2_EPG_SRV_GET_CHG_CH_TVTEST\r\n");
			LONGLONG key;
			if( ReadVALUE(&key, cmdParam->data, cmdParam->dataSize, NULL) ){
				CBlockLock lock(&sys->settingLock);
				TVTEST_CH_CHG_INFO info;
				info.chInfo.useSID = TRUE;
				info.chInfo.ONID = key >> 32 & 0xFFFF;
				info.chInfo.TSID = key >> 16 & 0xFFFF;
				info.chInfo.SID = key & 0xFFFF;
				info.chInfo.useBonCh = FALSE;
				vector<DWORD> idList = sys->reserveManager.GetSupportServiceTuner(info.chInfo.ONID, info.chInfo.TSID, info.chInfo.SID);
				for( int i = (int)idList.size() - 1; i >= 0; i-- ){
					info.bonDriver = sys->reserveManager.GetTunerBonFileName(idList[i]);
					for( size_t j = 0; j < sys->tvtestUseBon.size(); j++ ){
						if( CompareNoCase(sys->tvtestUseBon[j], info.bonDriver) == 0 ){
							if( sys->reserveManager.IsOpenTuner(idList[i]) == false ){
								info.chInfo.useBonCh = TRUE;
								sys->reserveManager.GetTunerCh(idList[i], info.chInfo.ONID, info.chInfo.TSID, info.chInfo.SID, &info.chInfo.space, &info.chInfo.ch);
							}
							break;
						}
					}
					if( info.chInfo.useBonCh ){
						resParam->data = NewWriteVALUE(&info, resParam->dataSize);
						resParam->param = CMD_SUCCESS;
						break;
					}
				}
			}
		}
		break;
	case CMD2_EPG_SRV_NWTV_SET_CH:
		{
			OutputDebugString(L"CMD2_EPG_SRV_NWTV_SET_CH\r\n");
			SET_CH_INFO val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) && val.useSID ){
				CBlockLock lock(&sys->settingLock);
				vector<DWORD> idList = sys->reserveManager.GetSupportServiceTuner(val.ONID, val.TSID, val.SID);
				vector<DWORD> idUseList;
				for( int i = (int)idList.size() - 1; i >= 0; i-- ){
					wstring bonDriver = sys->reserveManager.GetTunerBonFileName(idList[i]);
					for( size_t j = 0; j < sys->tvtestUseBon.size(); j++ ){
						if( CompareNoCase(sys->tvtestUseBon[j], bonDriver) == 0 ){
							idUseList.push_back(idList[i]);
							break;
						}
					}
				}
				if( sys->reserveManager.SetNWTVCh(sys->nwtvUdp, sys->nwtvTcp, val, idUseList) ){
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD2_EPG_SRV_NWTV_CLOSE:
		{
			OutputDebugString(L"CMD2_EPG_SRV_NWTV_CLOSE\r\n");
			if( sys->reserveManager.CloseNWTV() ){
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_NWTV_MODE:
		{
			OutputDebugString(L"CMD2_EPG_SRV_NWTV_MODE\r\n");
			DWORD val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) ){
				CBlockLock lock(&sys->settingLock);
				sys->nwtvUdp = val == 1 || val == 3;
				sys->nwtvTcp = val == 2 || val == 3;
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_NWPLAY_OPEN:
		{
			OutputDebugString(L"CMD2_EPG_SRV_NWPLAY_OPEN\r\n");
			wstring val;
			DWORD id;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) && sys->streamingManager.OpenFile(val.c_str(), &id) ){
				resParam->data = NewWriteVALUE(id, resParam->dataSize);
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_NWPLAY_CLOSE:
		{
			OutputDebugString(L"CMD2_EPG_SRV_NWPLAY_CLOSE\r\n");
			DWORD val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) && sys->streamingManager.CloseFile(val) ){
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_NWPLAY_PLAY:
		{
			OutputDebugString(L"CMD2_EPG_SRV_NWPLAY_PLAY\r\n");
			DWORD val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) && sys->streamingManager.StartSend(val) ){
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_NWPLAY_STOP:
		{
			OutputDebugString(L"CMD2_EPG_SRV_NWPLAY_STOP\r\n");
			DWORD val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) && sys->streamingManager.StopSend(val) ){
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_NWPLAY_GET_POS:
		{
			OutputDebugString(L"CMD2_EPG_SRV_NWPLAY_GET_POS\r\n");
			NWPLAY_POS_CMD val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) && sys->streamingManager.GetPos(&val) ){
				resParam->data = NewWriteVALUE(&val, resParam->dataSize);
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_NWPLAY_SET_POS:
		{
			OutputDebugString(L"CMD2_EPG_SRV_NWPLAY_SET_POS\r\n");
			NWPLAY_POS_CMD val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) && sys->streamingManager.SetPos(&val) ){
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_NWPLAY_SET_IP:
		{
			OutputDebugString(L"CMD2_EPG_SRV_NWPLAY_SET_IP\r\n");
			NWPLAY_PLAY_INFO val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) && sys->streamingManager.SetIP(&val) ){
				resParam->data = NewWriteVALUE(&val, resParam->dataSize);
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_NWPLAY_TF_OPEN:
		{
			OutputDebugString(L"CMD2_EPG_SRV_NWPLAY_TF_OPEN\r\n");
			DWORD val;
			NWPLAY_TIMESHIFT_INFO resVal;
			DWORD ctrlID;
			DWORD processID;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) &&
			    sys->reserveManager.GetRecFilePath(val, resVal.filePath, &ctrlID, &processID) &&
			    sys->streamingManager.OpenTimeShift(resVal.filePath.c_str(), processID, ctrlID, &resVal.ctrlID) ){
				resParam->data = NewWriteVALUE(&resVal, resParam->dataSize);
				resParam->param = CMD_SUCCESS;
			}
		}
		break;

	////////////////////////////////////////////////////////////
	//CMD_VER対応コマンド
	case CMD2_EPG_SRV_ENUM_RESERVE2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ENUM_RESERVE2\r\n");
			WORD ver;
			if( ReadVALUE(&ver, cmdParam->data, cmdParam->dataSize, NULL) ){
				//ver>=5では録画予定ファイル名も返す
				vector<RESERVE_DATA> list = sys->reserveManager.GetReserveDataAll(ver >= 5);
				resParam->data = NewWriteVALUE2WithVersion(ver, &list, resParam->dataSize);
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_GET_RESERVE2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_GET_RESERVE2\r\n");
			WORD ver;
			DWORD readSize;
			if( ReadVALUE(&ver, cmdParam->data, cmdParam->dataSize, &readSize) ){
				DWORD reserveID;
				if( ReadVALUE2(ver, &reserveID, cmdParam->data + readSize, cmdParam->dataSize - readSize, NULL) ){
					RESERVE_DATA info;
					if( sys->reserveManager.GetReserveData(reserveID, &info) ){
						resParam->data = NewWriteVALUE2WithVersion(ver, &info, resParam->dataSize);
						resParam->param = CMD_SUCCESS;
					}
				}
			}
		}
		break;
	case CMD2_EPG_SRV_ADD_RESERVE2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ADD_RESERVE2\r\n");
			WORD ver;
			DWORD readSize;
			if( ReadVALUE(&ver, cmdParam->data, cmdParam->dataSize, &readSize) ){
				vector<RESERVE_DATA> list;
				if( ReadVALUE2(ver, &list, cmdParam->data + readSize, cmdParam->dataSize - readSize, NULL) &&
				    sys->reserveManager.AddReserveData(list) ){
					resParam->data = NewWriteVALUE(ver, resParam->dataSize);
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD2_EPG_SRV_CHG_RESERVE2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_CHG_RESERVE2\r\n");
			WORD ver;
			DWORD readSize;
			if( ReadVALUE(&ver, cmdParam->data, cmdParam->dataSize, &readSize) ){
				vector<RESERVE_DATA> list;
				if( ReadVALUE2(ver, &list, cmdParam->data + readSize, cmdParam->dataSize - readSize, NULL) &&
				    sys->reserveManager.ChgReserveData(list) ){
					resParam->data = NewWriteVALUE(ver, resParam->dataSize);
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD2_EPG_SRV_ADDCHK_RESERVE2:
		OutputDebugString(L"CMD2_EPG_SRV_ADDCHK_RESERVE2\r\n");
		resParam->param = CMD_NON_SUPPORT;
		break;
	case CMD2_EPG_SRV_GET_EPG_FILETIME2:
		OutputDebugString(L"CMD2_EPG_SRV_GET_EPG_FILETIME2\r\n");
		resParam->param = CMD_NON_SUPPORT;
		break;
	case CMD2_EPG_SRV_GET_EPG_FILE2:
		OutputDebugString(L"CMD2_EPG_SRV_GET_EPG_FILE2\r\n");
		resParam->param = CMD_NON_SUPPORT;
		break;
	case CMD2_EPG_SRV_ENUM_AUTO_ADD2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ENUM_AUTO_ADD2\r\n");
			WORD ver;
			if( ReadVALUE(&ver, cmdParam->data, cmdParam->dataSize, NULL) ){
				vector<EPG_AUTO_ADD_DATA> val;
				{
					CBlockLock lock(&sys->settingLock);
					map<DWORD, EPG_AUTO_ADD_DATA>::const_iterator itr;
					for( itr = sys->epgAutoAdd.GetMap().begin(); itr != sys->epgAutoAdd.GetMap().end(); itr++ ){
						val.push_back(itr->second);
					}
				}
				resParam->data = NewWriteVALUE2WithVersion(ver, &val, resParam->dataSize);
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_ADD_AUTO_ADD2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ADD_AUTO_ADD2\r\n");
			WORD ver;
			DWORD readSize;
			if( ReadVALUE(&ver, cmdParam->data, cmdParam->dataSize, &readSize) ){
				vector<EPG_AUTO_ADD_DATA> val;
				if( ReadVALUE2(ver, &val, cmdParam->data + readSize, cmdParam->dataSize - readSize, NULL) ){
					{
						CBlockLock lock(&sys->settingLock);
						for( size_t i = 0; i < val.size(); i++ ){
							val[i].dataID = sys->epgAutoAdd.AddData(val[i]);
						}
						sys->epgAutoAdd.SaveText();
					}
					for( size_t i = 0; i < val.size(); i++ ){
						sys->AutoAddReserveEPG(val[i]);
					}
					sys->notifyManager.AddNotify(NOTIFY_UPDATE_AUTOADD_EPG);
					resParam->data = NewWriteVALUE(ver, resParam->dataSize);
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD2_EPG_SRV_CHG_AUTO_ADD2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_CHG_AUTO_ADD2\r\n");
			WORD ver;
			DWORD readSize;
			if( ReadVALUE(&ver, cmdParam->data, cmdParam->dataSize, &readSize) ){
				vector<EPG_AUTO_ADD_DATA> val;
				if( ReadVALUE2(ver, &val, cmdParam->data + readSize, cmdParam->dataSize - readSize, NULL) ){
					{
						CBlockLock lock(&sys->settingLock);
						for( size_t i = 0; i < val.size(); i++ ){
							if( sys->epgAutoAdd.ChgData(val[i]) == false ){
								val.erase(val.begin() + (i--));
							}
						}
						sys->epgAutoAdd.SaveText();
					}
					for( size_t i = 0; i < val.size(); i++ ){
						sys->AutoAddReserveEPG(val[i]);
					}
					sys->notifyManager.AddNotify(NOTIFY_UPDATE_AUTOADD_EPG);
					resParam->data = NewWriteVALUE(ver, resParam->dataSize);
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD2_EPG_SRV_ENUM_MANU_ADD2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ENUM_MANU_ADD2\r\n");
			WORD ver;
			if( ReadVALUE(&ver, cmdParam->data, cmdParam->dataSize, NULL) ){
				vector<MANUAL_AUTO_ADD_DATA> val;
				{
					CBlockLock lock(&sys->settingLock);
					map<DWORD, MANUAL_AUTO_ADD_DATA>::const_iterator itr;
					for( itr = sys->manualAutoAdd.GetMap().begin(); itr != sys->manualAutoAdd.GetMap().end(); itr++ ){
						val.push_back(itr->second);
					}
				}
				resParam->data = NewWriteVALUE2WithVersion(ver, &val, resParam->dataSize);
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_ADD_MANU_ADD2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ADD_MANU_ADD2\r\n");
			WORD ver;
			DWORD readSize;
			if( ReadVALUE(&ver, cmdParam->data, cmdParam->dataSize, &readSize) ){
				vector<MANUAL_AUTO_ADD_DATA> val;
				if( ReadVALUE2(ver, &val, cmdParam->data + readSize, cmdParam->dataSize - readSize, NULL) ){
					{
						CBlockLock lock(&sys->settingLock);
						for( size_t i = 0; i < val.size(); i++ ){
							val[i].dataID = sys->manualAutoAdd.AddData(val[i]);
						}
						sys->manualAutoAdd.SaveText();
					}
					for( size_t i = 0; i < val.size(); i++ ){
						sys->AutoAddReserveProgram(val[i]);
					}
					sys->notifyManager.AddNotify(NOTIFY_UPDATE_AUTOADD_MANUAL);
					resParam->data = NewWriteVALUE(ver, resParam->dataSize);
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD2_EPG_SRV_CHG_MANU_ADD2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_CHG_MANU_ADD2\r\n");
			WORD ver;
			DWORD readSize;
			if( ReadVALUE(&ver, cmdParam->data, cmdParam->dataSize, &readSize) ){
				vector<MANUAL_AUTO_ADD_DATA> val;
				if( ReadVALUE2(ver, &val, cmdParam->data + readSize, cmdParam->dataSize - readSize, NULL) ){
					{
						CBlockLock lock(&sys->settingLock);
						for( size_t i = 0; i < val.size(); i++ ){
							if( sys->manualAutoAdd.ChgData(val[i]) == false ){
								val.erase(val.begin() + (i--));
							}
						}
						sys->manualAutoAdd.SaveText();
					}
					for( size_t i = 0; i < val.size(); i++ ){
						sys->AutoAddReserveProgram(val[i]);
					}
					sys->notifyManager.AddNotify(NOTIFY_UPDATE_AUTOADD_MANUAL);
					resParam->data = NewWriteVALUE(ver, resParam->dataSize);
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD2_EPG_SRV_ENUM_RECINFO2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ENUM_RECINFO2\r\n");
			WORD ver;
			if( ReadVALUE(&ver, cmdParam->data, cmdParam->dataSize, NULL) ){
				vector<REC_FILE_INFO> list = sys->reserveManager.GetRecFileInfoAll();
				resParam->data = NewWriteVALUE2WithVersion(ver, &list, resParam->dataSize);
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_CHG_PROTECT_RECINFO2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_CHG_PROTECT_RECINFO2\r\n");
			WORD ver;
			DWORD readSize;
			if( ReadVALUE(&ver, cmdParam->data, cmdParam->dataSize, &readSize) ){
				vector<REC_FILE_INFO> list;
				if( ReadVALUE2(ver, &list, cmdParam->data + readSize, cmdParam->dataSize - readSize, NULL) ){
					sys->reserveManager.ChgProtectRecFileInfo(list);
					resParam->data = NewWriteVALUE(ver, resParam->dataSize);
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
#if 1
	////////////////////////////////////////////////////////////
	//旧バージョン互換コマンド
	case CMD_EPG_SRV_GET_RESERVE_INFO:
		{
			resParam->param = OLD_CMD_ERR;
			{
				DWORD reserveID = 0;
				if( ReadVALUE(&reserveID, cmdParam->data, cmdParam->dataSize, NULL ) != FALSE ){
					RESERVE_DATA info;
					if(sys->reserveManager.GetReserveData(reserveID, &info) ){
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
					if(sys->reserveManager.AddReserveData(list) ){
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
					sys->reserveManager.DelReserveData(list);
					resParam->param = OLD_CMD_SUCCESS;
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
					if(sys->reserveManager.ChgReserveData(list) ){
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
					sys->AutoAddReserveEPG(item);
					sys->notifyManager.AddNotify(NOTIFY_UPDATE_AUTOADD_EPG);
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
					sys->notifyManager.AddNotify(NOTIFY_UPDATE_AUTOADD_EPG);
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
					sys->AutoAddReserveEPG(item);
					sys->notifyManager.AddNotify(NOTIFY_UPDATE_AUTOADD_EPG);
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
						vector<std::unique_ptr<CEpgDBManager::SEARCH_RESULT_EVENT_DATA>> val;
						key.push_back(item);
						if( sys->epgDB.SearchEpg(&key, &val) != FALSE ){
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
	//旧バージョン互換コマンドここまで
#endif
	default:
		_OutputDebugString(L"err default cmd %d\r\n", cmdParam->param);
		resParam->param = CMD_NON_SUPPORT;
		break;
	}

	return 0;
}

int CEpgTimerSrvMain::UpnpMSearchReqCallback(UPNP_MSEARCH_REQUEST_INFO* requestParam, void* param, SORT_LIST_HANDLE resDeviceList)
{
	if( requestParam == NULL || lstrcmpiA(requestParam->man, "\"ssdp:discover\"") != 0 ){
		return -1;
	}
	//ここではUDPで投げられたデバイス検索の要求に対する応答を作成する
	const string& uuid = ((CEpgTimerSrvMain*)param)->ssdpNotifyUuid;
	unsigned short port = ((CEpgTimerSrvMain*)param)->ssdpNotifyPort;
	char ua[128] = "";
	UPNP_UTIL_GetUserAgent(ua, _countof(ua));
	string st = requestParam->st;
	if( CompareNoCase(st, "upnp:rootdevice") == 0 || CompareNoCase(st, "ssdp:all") == 0 ){
		UPNP_MSEARCH_RES_DEV_INFO* resDevInfo = UPNP_MSEARCH_RES_DEV_INFO_New();
		resDevInfo->max_age = 1800;
		resDevInfo->port = port;
		resDevInfo->uri = _strdup("/dlna/dms/ddd.xml");
		resDevInfo->uuid = _strdup(uuid.c_str());
		resDevInfo->usn = _strdup(("uuid:" + uuid + "::urn:rootdevice").c_str());
		resDevInfo->server = _strdup(ua);
		SORT_LIST_AddItem(resDeviceList, resDevInfo->uuid, resDevInfo);
	}
	if( st.compare(0, 5, "uuid:") == 0 && st.find(uuid) != string::npos ){
		UPNP_MSEARCH_RES_DEV_INFO* resDevInfo = UPNP_MSEARCH_RES_DEV_INFO_New();
		resDevInfo->max_age = 1800;
		resDevInfo->port = port;
		resDevInfo->uri = _strdup("/dlna/dms/ddd.xml");
		resDevInfo->uuid = _strdup(uuid.c_str());
		resDevInfo->usn = _strdup(("uuid:" + uuid).c_str());
		resDevInfo->server = _strdup(ua);
		SORT_LIST_AddItem(resDeviceList, resDevInfo->uuid, resDevInfo);
	}
	if( CompareNoCase(st, UPNP_URN_DMS_1) == 0 ||
	    CompareNoCase(st, UPNP_URN_CDS_1) == 0 ||
	    CompareNoCase(st, UPNP_URN_CMS_1) == 0 ||
	    CompareNoCase(st, UPNP_URN_AVT_1) == 0 ){
		UPNP_MSEARCH_RES_DEV_INFO* resDevInfo = UPNP_MSEARCH_RES_DEV_INFO_New();
		resDevInfo->max_age = 1800;
		resDevInfo->port = port;
		resDevInfo->uri = _strdup("/dlna/dms/ddd.xml");
		resDevInfo->uuid = _strdup(uuid.c_str());
		resDevInfo->usn = _strdup(("uuid:" + uuid + "::" + st).c_str());
		resDevInfo->server = _strdup(ua);
		SORT_LIST_AddItem(resDeviceList, resDevInfo->uuid, resDevInfo);	
	}
	return -1;
}

int CEpgTimerSrvMain::InitLuaCallback(lua_State* L)
{
	CEpgTimerSrvMain* sys = (CEpgTimerSrvMain*)lua_touserdata(L, -1);
	lua_newtable(L);
	LuaHelp::reg_function(L, "GetGenreName", LuaGetGenreName, sys);
	LuaHelp::reg_function(L, "GetComponentTypeName", LuaGetComponentTypeName, sys);
	LuaHelp::reg_function(L, "GetChDataList", LuaGetChDataList, sys);
	LuaHelp::reg_function(L, "GetServiceList", LuaGetServiceList, sys);
	LuaHelp::reg_function(L, "EnumEventInfo", LuaEnumEventInfo, sys);
	LuaHelp::reg_function(L, "SearchEpg", LuaSearchEpg, sys);
	LuaHelp::reg_function(L, "AddReserveData", LuaAddReserveData, sys);
	LuaHelp::reg_function(L, "ChgReserveData", LuaChgReserveData, sys);
	LuaHelp::reg_function(L, "DelReserveData", LuaDelReserveData, sys);
	LuaHelp::reg_function(L, "GetReserveData", LuaGetReserveData, sys);
	LuaHelp::reg_function(L, "GetRecFileInfo", LuaGetRecFileInfo, sys);
	LuaHelp::reg_function(L, "DelRecFileInfo", LuaDelRecFileInfo, sys);
	LuaHelp::reg_function(L, "GetTunerReserveAll", LuaGetTunerReserveAll, sys);
	LuaHelp::reg_function(L, "EnumRecPresetInfo", LuaEnumRecPresetInfo, sys);
	LuaHelp::reg_function(L, "EnumAutoAdd", LuaEnumAutoAdd, sys);
	LuaHelp::reg_function(L, "EnumManuAdd", LuaEnumManuAdd, sys);
	LuaHelp::reg_function(L, "DelAutoAdd", LuaDelAutoAdd, sys);
	LuaHelp::reg_function(L, "DelManuAdd", LuaDelManuAdd, sys);
	LuaHelp::reg_function(L, "AddOrChgAutoAdd", LuaAddOrChgAutoAdd, sys);
	LuaHelp::reg_function(L, "AddOrChgManuAdd", LuaAddOrChgManuAdd, sys);
	LuaHelp::reg_function(L, "ListDmsPublicFile", LuaListDmsPublicFile, sys);
	LuaHelp::reg_int(L, "htmlEscape", 0);
	lua_setglobal(L, "edcb");
	return 0;
}

#if 1
//Lua-edcb空間のコールバック

CEpgTimerSrvMain::CLuaWorkspace::CLuaWorkspace(lua_State* L_)
	: L(L_)
	, sys((CEpgTimerSrvMain*)lua_touserdata(L, lua_upvalueindex(1)))
{
	lua_getglobal(L, "edcb");
	this->htmlEscape = LuaHelp::get_int(L, "htmlEscape");
	lua_pop(L, 1);
}

const char* CEpgTimerSrvMain::CLuaWorkspace::WtoUTF8(const wstring& strIn)
{
	this->strOut.resize(strIn.size() * 3 + 1);
	this->strOut.resize(WideCharToMultiByte(CP_UTF8, 0, strIn.c_str(), -1, &this->strOut[0], (int)this->strOut.size(), NULL, NULL));
	if( this->strOut.empty() ){
		//rare case
		this->strOut.resize(WideCharToMultiByte(CP_UTF8, 0, strIn.c_str(), -1, NULL, 0, NULL, NULL));
		if( this->strOut.empty() || WideCharToMultiByte(CP_UTF8, 0, strIn.c_str(), -1, &this->strOut[0], (int)this->strOut.size(), NULL, NULL) == 0 ){
			this->strOut.assign(1, '\0');
		}
	}
	if( this->htmlEscape != 0 ){
		LPCSTR rpl[] = { "&amp;", "<lt;", ">gt;", "\"quot;", "'apos;" };
		for( size_t i = 0; this->strOut[i] != '\0'; i++ ){
			for( int j = 0; j < 5; j++ ){
				if( rpl[j][0] == this->strOut[i] && (this->htmlEscape >> j & 1) ){
					this->strOut[i] = '&';
					this->strOut.insert(this->strOut.begin() + i + 1, rpl[j] + 1, rpl[j] + lstrlenA(rpl[j]));
					break;
				}
			}
		}
	}
	return &this->strOut[0];
}

int CEpgTimerSrvMain::LuaGetGenreName(lua_State* L)
{
	CLuaWorkspace ws(L);
	wstring name;
	if( lua_gettop(L) == 1 ){
		GetGenreName(lua_tointeger(L, -1) >> 8 & 0xFF, lua_tointeger(L, -1) & 0xFF, name);
	}
	lua_pushstring(L, ws.WtoUTF8(name));
	return 1;
}

int CEpgTimerSrvMain::LuaGetComponentTypeName(lua_State* L)
{
	CLuaWorkspace ws(L);
	wstring name;
	if( lua_gettop(L) == 1 ){
		GetComponentTypeName(lua_tointeger(L, -1) >> 8 & 0xFF, lua_tointeger(L, -1) & 0xFF, name);
	}
	lua_pushstring(L, ws.WtoUTF8(name));
	return 1;
}

int CEpgTimerSrvMain::LuaGetChDataList(lua_State* L)
{
	CLuaWorkspace ws(L);
	vector<CH_DATA5> list = ws.sys->reserveManager.GetChDataList();
	lua_newtable(L);
	for( size_t i = 0; i < list.size(); i++ ){
		lua_newtable(L);
		LuaHelp::reg_int(L, "onid", list[i].originalNetworkID);
		LuaHelp::reg_int(L, "tsid", list[i].transportStreamID);
		LuaHelp::reg_int(L, "sid", list[i].serviceID);
		LuaHelp::reg_int(L, "serviceType", list[i].serviceType);
		LuaHelp::reg_boolean(L, "partialFlag", list[i].partialFlag != 0);
		LuaHelp::reg_string(L, "serviceName", ws.WtoUTF8(list[i].serviceName));
		LuaHelp::reg_string(L, "networkName", ws.WtoUTF8(list[i].networkName));
		LuaHelp::reg_boolean(L, "epgCapFlag", list[i].epgCapFlag != 0);
		LuaHelp::reg_boolean(L, "searchFlag", list[i].searchFlag != 0);
		lua_rawseti(L, -2, (int)i + 1);
	}
	return 1;
}

int CEpgTimerSrvMain::LuaGetServiceList(lua_State* L)
{
	CLuaWorkspace ws(L);
	vector<EPGDB_SERVICE_INFO> list;
	if( ws.sys->epgDB.GetServiceList(&list) != FALSE ){
		lua_newtable(L);
		for( size_t i = 0; i < list.size(); i++ ){
			lua_newtable(L);
			LuaHelp::reg_int(L, "onid", list[i].ONID);
			LuaHelp::reg_int(L, "tsid", list[i].TSID);
			LuaHelp::reg_int(L, "sid", list[i].SID);
			LuaHelp::reg_int(L, "service_type", list[i].service_type);
			LuaHelp::reg_boolean(L, "partialReceptionFlag", list[i].partialReceptionFlag != 0);
			LuaHelp::reg_string(L, "service_provider_name", ws.WtoUTF8(list[i].service_provider_name));
			LuaHelp::reg_string(L, "service_name", ws.WtoUTF8(list[i].service_name));
			LuaHelp::reg_string(L, "network_name", ws.WtoUTF8(list[i].network_name));
			LuaHelp::reg_string(L, "ts_name", ws.WtoUTF8(list[i].ts_name));
			LuaHelp::reg_int(L, "remote_control_key_id", list[i].remote_control_key_id);
			lua_rawseti(L, -2, (int)i + 1);
		}
		return 1;
	}
	return 0;
}

void CEpgTimerSrvMain::LuaEnumEventInfoCallback(vector<EPGDB_EVENT_INFO*>* pval, void* param)
{
	CLuaWorkspace& ws = *(CLuaWorkspace*)param;
	lua_newtable(ws.L);
	for( size_t i = 0; i < pval->size(); i++ ){
		lua_newtable(ws.L);
		PushEpgEventInfo(ws, *(*pval)[i]);
		lua_rawseti(ws.L, -2, (int)i + 1);
	}
}

void CEpgTimerSrvMain::LuaEnumEventAllCallback(vector<EPGDB_SERVICE_EVENT_INFO>* pval, void* param)
{
	CLuaWorkspace& ws = *(CLuaWorkspace*)((void**)param)[0];
	vector<int>& key = *(vector<int>*)((void**)param)[1];
	lua_newtable(ws.L);
	int n = 0;
	for( size_t i = 0; i < pval->size(); i++ ){
		for( size_t j = 0; j + 2 < key.size(); j += 3 ){
			if( (key[j] < 0 || key[j] == (*pval)[i].serviceInfo.ONID) &&
			    (key[j+1] < 0 || key[j+1] == (*pval)[i].serviceInfo.TSID) &&
			    (key[j+2] < 0 || key[j+2] == (*pval)[i].serviceInfo.SID) ){
				for( size_t k = 0; k < (*pval)[i].eventList.size(); k++ ){
					lua_newtable(ws.L);
					PushEpgEventInfo(ws, *(*pval)[i].eventList[k]);
					lua_rawseti(ws.L, -2, ++n);
				}
				break;
			}
		}
	}
}

int CEpgTimerSrvMain::LuaEnumEventInfo(lua_State* L)
{
	CLuaWorkspace ws(L);
	if( lua_gettop(L) == 1 && lua_istable(L, -1) ){
		vector<int> key;
		for( int i = 0;; i++ ){
			lua_rawgeti(L, -1, i + 1);
			if( !lua_istable(L, -1) ){
				lua_pop(L, 1);
				break;
			}
			key.push_back(LuaHelp::isnil(L, "onid") ? -1 : LuaHelp::get_int(L, "onid"));
			key.push_back(LuaHelp::isnil(L, "tsid") ? -1 : LuaHelp::get_int(L, "tsid"));
			key.push_back(LuaHelp::isnil(L, "sid") ? -1 : LuaHelp::get_int(L, "sid"));
			lua_pop(L, 1);
		}
		if( key.size() == 3 && key[0] >= 0 && key[1] >= 0 && key[2] >= 0 ){
			if( ws.sys->epgDB.EnumEventInfo(_Create64Key(key[0] & 0xFFFF, key[1] & 0xFFFF, key[2] & 0xFFFF), LuaEnumEventInfoCallback, &ws) != FALSE ){
				return 1;
			}
		}else{
			void* params[] = {&ws, &key};
			if( ws.sys->epgDB.EnumEventAll(LuaEnumEventAllCallback, params) != FALSE ){
				return 1;
			}
		}
	}
	return 0;
}

void CEpgTimerSrvMain::LuaSearchEpgCallback(vector<CEpgDBManager::SEARCH_RESULT_EVENT>* pval, void* param)
{
	CLuaWorkspace& ws = *(CLuaWorkspace*)param;
	SYSTEMTIME now;
	GetLocalTime(&now);
	now.wHour = 0;
	now.wMinute = 0;
	now.wSecond = 0;
	now.wMilliseconds = 0;
	//対象期間
	__int64 chkTime = LuaHelp::get_int(ws.L, "days") * 24 * 60 * 60 * I64_1SEC;
	if( chkTime > 0 ){
		chkTime += ConvertI64Time(now);
	}else{
		//たぶんバグだが互換のため
		chkTime = LuaHelp::get_int(ws.L, "days29") * 29 * 60 * 60 * I64_1SEC;
		if( chkTime > 0 ){
			chkTime += ConvertI64Time(now);
		}
	}
	lua_newtable(ws.L);
	int n = 0;
	for( size_t i = 0; i < pval->size(); i++ ){
		if( (chkTime <= 0 || (*pval)[i].info->StartTimeFlag != 0 && chkTime > ConvertI64Time((*pval)[i].info->start_time)) ){
			//イベントグループはチェックしないので注意
			lua_newtable(ws.L);
			PushEpgEventInfo(ws, *(*pval)[i].info);
			lua_rawseti(ws.L, -2, ++n);
		}
	}
}

int CEpgTimerSrvMain::LuaSearchEpg(lua_State* L)
{
	CLuaWorkspace ws(L);
	if( lua_gettop(L) == 4 ){
		EPGDB_EVENT_INFO info;
		if( ws.sys->epgDB.SearchEpg((WORD)lua_tointeger(L, 1), (WORD)lua_tointeger(L, 2), (WORD)lua_tointeger(L, 3), (WORD)lua_tointeger(L, 4), &info) != FALSE ){
			lua_newtable(ws.L);
			PushEpgEventInfo(ws, info);
			return 1;
		}
	}else if( lua_gettop(L) == 1 && lua_istable(L, -1) ){
		vector<EPGDB_SEARCH_KEY_INFO> keyList(1);
		EPGDB_SEARCH_KEY_INFO& key = keyList.back();
		UTF8toW(LuaHelp::get_string(L, "andKey"), key.andKey);
		UTF8toW(LuaHelp::get_string(L, "notKey"), key.notKey);
		//対象ジャンル
		lua_getfield(L, -1, "contentList");
		if( lua_istable(L, -1) ){
			for( int i = 0;; i++ ){
				lua_rawgeti(L, -1, i + 1);
				if( !lua_istable(L, -1) ){
					lua_pop(L, 1);
					break;
				}
				key.contentList.resize(i + 1);
				key.contentList[i].content_nibble_level_1 = LuaHelp::get_int(L, "content_nibble") >> 8 & 0xFF;
				key.contentList[i].content_nibble_level_2 = LuaHelp::get_int(L, "content_nibble") & 0xFF;
				lua_pop(L, 1);
			}
		}
		lua_pop(L, 1);
		//対象ネットワーク
		vector<EPGDB_SERVICE_INFO> list;
		if( ws.sys->epgDB.GetServiceList(&list) != FALSE ){
			int network = LuaHelp::get_int(L, "network");
			for( size_t i = 0; i < list.size(); i++ ){
				WORD onid = list[i].ONID;
				if( (network & 1) && 0x7880 <= onid && onid <= 0x7FE8 || //地デジ
				    (network & 2) && onid == 4 || //BS
				    (network & 4) && (onid == 6 || onid == 7) || //CS
				    (network & 8) && ((onid < 0x7880 || 0x7FE8 < onid) && onid != 4 && onid != 6 && onid != 7) //その他
				    ){
					key.serviceList.push_back(_Create64Key(onid, list[i].TSID, list[i].SID));
				}
			}
			if( ws.sys->epgDB.SearchEpg(&keyList, LuaSearchEpgCallback, &ws) != FALSE ){
				return 1;
			}
		}
	}
	return 0;
}

int CEpgTimerSrvMain::LuaAddReserveData(lua_State* L)
{
	CLuaWorkspace ws(L);
	if( lua_gettop(L) == 1 && lua_istable(L, -1) ){
		vector<RESERVE_DATA> list(1);
		if( FetchReserveData(ws, list.back()) && ws.sys->reserveManager.AddReserveData(list) ){
			lua_pushboolean(L, true);
			return 1;
		}
	}
	lua_pushboolean(L, false);
	return 1;
}

int CEpgTimerSrvMain::LuaChgReserveData(lua_State* L)
{
	CLuaWorkspace ws(L);
	if( lua_gettop(L) == 1 && lua_istable(L, -1) ){
		vector<RESERVE_DATA> list(1);
		if( FetchReserveData(ws, list.back()) && ws.sys->reserveManager.ChgReserveData(list) ){
			lua_pushboolean(L, true);
			return 1;
		}
	}
	lua_pushboolean(L, false);
	return 1;
}

int CEpgTimerSrvMain::LuaDelReserveData(lua_State* L)
{
	CLuaWorkspace ws(L);
	if( lua_gettop(L) == 1 ){
		vector<DWORD> list(1, (DWORD)lua_tointeger(L, -1));
		ws.sys->reserveManager.DelReserveData(list);
	}
	return 0;
}

int CEpgTimerSrvMain::LuaGetReserveData(lua_State* L)
{
	CLuaWorkspace ws(L);
	if( lua_gettop(L) == 0 ){
		lua_newtable(L);
		vector<RESERVE_DATA> list = ws.sys->reserveManager.GetReserveDataAll();
		for( size_t i = 0; i < list.size(); i++ ){
			lua_newtable(L);
			PushReserveData(ws, list[i]);
			lua_rawseti(L, -2, (int)i + 1);
		}
		return 1;
	}else{
		RESERVE_DATA r;
		if( ws.sys->reserveManager.GetReserveData((DWORD)lua_tointeger(L, 1), &r) ){
			lua_newtable(L);
			PushReserveData(ws, r);
			return 1;
		}
	}
	return 0;
}

int CEpgTimerSrvMain::LuaGetRecFileInfo(lua_State* L)
{
	CLuaWorkspace ws(L);
	bool getAll = lua_gettop(L) == 0;
	DWORD id = 0;
	if( getAll ){
		lua_newtable(L);
	}else{
		id = (DWORD)lua_tointeger(L, 1);
	}
	vector<REC_FILE_INFO> list = ws.sys->reserveManager.GetRecFileInfoAll();
	for( size_t i = 0; i < list.size(); i++ ){
		const REC_FILE_INFO& r = list[i];
		if( getAll || id == r.id ){
			lua_newtable(L);
			LuaHelp::reg_int(L, "id", (int)r.id);
			LuaHelp::reg_string(L, "recFilePath", ws.WtoUTF8(r.recFilePath));
			LuaHelp::reg_string(L, "title", ws.WtoUTF8(r.title));
			LuaHelp::reg_time(L, "startTime", r.startTime);
			LuaHelp::reg_int(L, "durationSecond", (int)r.durationSecond);
			LuaHelp::reg_string(L, "serviceName", ws.WtoUTF8(r.serviceName));
			LuaHelp::reg_int(L, "onid", r.originalNetworkID);
			LuaHelp::reg_int(L, "tsid", r.transportStreamID);
			LuaHelp::reg_int(L, "sid", r.serviceID);
			LuaHelp::reg_int(L, "eid", r.eventID);
			LuaHelp::reg_int(L, "drops", (int)r.drops);
			LuaHelp::reg_int(L, "scrambles", (int)r.scrambles);
			LuaHelp::reg_int(L, "recStatus", (int)r.recStatus);
			LuaHelp::reg_time(L, "startTimeEpg", r.startTimeEpg);
			LuaHelp::reg_string(L, "comment", ws.WtoUTF8(r.comment));
			LuaHelp::reg_string(L, "programInfo", ws.WtoUTF8(r.programInfo));
			LuaHelp::reg_string(L, "errInfo", ws.WtoUTF8(r.errInfo));
			LuaHelp::reg_boolean(L, "protectFlag", r.protectFlag != 0);
			if( getAll == false && id == r.id ){
				return 1;
			}
			lua_rawseti(L, -2, (int)i + 1);
		}
	}
	return getAll ? 1 : 0;
}

int CEpgTimerSrvMain::LuaDelRecFileInfo(lua_State* L)
{
	CLuaWorkspace ws(L);
	if( lua_gettop(L) == 1 ){
		vector<DWORD> list(1, (DWORD)lua_tointeger(L, -1));
		ws.sys->reserveManager.DelRecFileInfo(list);
	}
	return 0;
}

int CEpgTimerSrvMain::LuaGetTunerReserveAll(lua_State* L)
{
	CLuaWorkspace ws(L);
	lua_newtable(L);
	vector<TUNER_RESERVE_INFO> list = ws.sys->reserveManager.GetTunerReserveAll();
	for( size_t i = 0; i < list.size(); i++ ){
		lua_newtable(L);
		LuaHelp::reg_int(L, "tunerID", (int)list[i].tunerID);
		LuaHelp::reg_string(L, "tunerName", ws.WtoUTF8(list[i].tunerName));
		lua_pushstring(L, "reserveList");
		lua_newtable(L);
		for( size_t j = 0; j < list[i].reserveList.size(); j++ ){
			lua_pushinteger(L, (int)list[i].reserveList[j]);
			lua_rawseti(L, -2, (int)j + 1);
		}
		lua_rawset(L, -3);
		lua_rawseti(L, -2, (int)i + 1);
	}
	return 1;
}

int CEpgTimerSrvMain::LuaEnumRecPresetInfo(lua_State* L)
{
	CLuaWorkspace ws(L);
	lua_newtable(L);
	wstring iniPath;
	GetModuleIniPath(iniPath);
	WCHAR buff[512];
	GetPrivateProfileString(L"SET", L"PresetID", L"", buff, 512, iniPath.c_str());
	wstring parseBuff = buff;
	vector<WORD> idList(1, 0);
	while( parseBuff.empty() == false ){
		wstring presetID;
		Separate(parseBuff, L",", presetID, parseBuff);
		idList.push_back((WORD)_wtoi(presetID.c_str()));
	}
	std::sort(idList.begin(), idList.end());
	idList.erase(std::unique(idList.begin(), idList.end()), idList.end());
	for( size_t i = 0; i < idList.size(); i++ ){
		lua_newtable(L);
		pair<wstring, REC_SETTING_DATA> ret = ws.sys->LoadRecSetData(idList[i]);
		LuaHelp::reg_int(L, "id", idList[i]);
		LuaHelp::reg_string(L, "name", ws.WtoUTF8(ret.first));
		lua_pushstring(L, "recSetting");
		lua_newtable(L);
		PushRecSettingData(ws, ret.second);
		lua_rawset(L, -3);
		lua_rawseti(L, -2, (int)i + 1);
	}
	return 1;
}

int CEpgTimerSrvMain::LuaEnumAutoAdd(lua_State* L)
{
	CLuaWorkspace ws(L);
	CBlockLock lock(&ws.sys->settingLock);
	lua_newtable(L);
	int i = 0;
	for( map<DWORD, EPG_AUTO_ADD_DATA>::const_iterator itr = ws.sys->epgAutoAdd.GetMap().begin(); itr != ws.sys->epgAutoAdd.GetMap().end(); itr++, i++ ){
		lua_newtable(L);
		LuaHelp::reg_int(L, "dataID", itr->second.dataID);
		LuaHelp::reg_int(L, "addCount", itr->second.addCount);
		lua_pushstring(L, "searchInfo");
		lua_newtable(L);
		PushEpgSearchKeyInfo(ws, itr->second.searchInfo);
		lua_rawset(L, -3);
		lua_pushstring(L, "recSetting");
		lua_newtable(L);
		PushRecSettingData(ws, itr->second.recSetting);
		lua_rawset(L, -3);
		lua_rawseti(L, -2, (int)i + 1);
	}
	return 1;
}

int CEpgTimerSrvMain::LuaEnumManuAdd(lua_State* L)
{
	CLuaWorkspace ws(L);
	CBlockLock lock(&ws.sys->settingLock);
	lua_newtable(L);
	int i = 0;
	for( map<DWORD, MANUAL_AUTO_ADD_DATA>::const_iterator itr = ws.sys->manualAutoAdd.GetMap().begin(); itr != ws.sys->manualAutoAdd.GetMap().end(); itr++, i++ ){
		lua_newtable(L);
		LuaHelp::reg_int(L, "dataID", itr->second.dataID);
		LuaHelp::reg_int(L, "dayOfWeekFlag", itr->second.dayOfWeekFlag);
		LuaHelp::reg_int(L, "startTime", itr->second.startTime);
		LuaHelp::reg_int(L, "durationSecond", itr->second.durationSecond);
		LuaHelp::reg_string(L, "title", ws.WtoUTF8(itr->second.title));
		LuaHelp::reg_string(L, "stationName", ws.WtoUTF8(itr->second.stationName));
		LuaHelp::reg_int(L, "onid", itr->second.originalNetworkID);
		LuaHelp::reg_int(L, "tsid", itr->second.transportStreamID);
		LuaHelp::reg_int(L, "sid", itr->second.serviceID);
		lua_pushstring(L, "recSetting");
		lua_newtable(L);
		PushRecSettingData(ws, itr->second.recSetting);
		lua_rawset(L, -3);
		lua_rawseti(L, -2, (int)i + 1);
	}
	return 1;
}

int CEpgTimerSrvMain::LuaDelAutoAdd(lua_State* L)
{
	CLuaWorkspace ws(L);
	if( lua_gettop(L) == 1 ){
		CBlockLock lock(&ws.sys->settingLock);
		if( ws.sys->epgAutoAdd.DelData((DWORD)lua_tointeger(L, -1)) ){
			ws.sys->epgAutoAdd.SaveText();
			ws.sys->notifyManager.AddNotify(NOTIFY_UPDATE_AUTOADD_EPG);
		}
	}
	return 0;
}

int CEpgTimerSrvMain::LuaDelManuAdd(lua_State* L)
{
	CLuaWorkspace ws(L);
	if( lua_gettop(L) == 1 ){
		CBlockLock lock(&ws.sys->settingLock);
		if( ws.sys->manualAutoAdd.DelData((DWORD)lua_tointeger(L, -1)) ){
			ws.sys->manualAutoAdd.SaveText();
			ws.sys->notifyManager.AddNotify(NOTIFY_UPDATE_AUTOADD_MANUAL);
		}
	}
	return 0;
}

int CEpgTimerSrvMain::LuaAddOrChgAutoAdd(lua_State* L)
{
	CLuaWorkspace ws(L);
	if( lua_gettop(L) == 1 && lua_istable(L, -1) ){
		EPG_AUTO_ADD_DATA item;
		item.dataID = LuaHelp::get_int(L, "dataID");
		lua_getfield(L, -1, "searchInfo");
		if( lua_istable(L, -1) ){
			FetchEpgSearchKeyInfo(ws, item.searchInfo);
			lua_getfield(L, -2, "recSetting");
			if( lua_istable(L, -1) ){
				FetchRecSettingData(ws, item.recSetting);
				bool modified = true;
				{
					CBlockLock lock(&ws.sys->settingLock);
					if( item.dataID == 0 ){
						item.dataID = ws.sys->epgAutoAdd.AddData(item);
					}else{
						modified = ws.sys->epgAutoAdd.ChgData(item);
					}
					if( modified ){
						ws.sys->epgAutoAdd.SaveText();
					}
				}
				if( modified ){
					ws.sys->AutoAddReserveEPG(item);
					ws.sys->notifyManager.AddNotify(NOTIFY_UPDATE_AUTOADD_EPG);
					lua_pushboolean(L, true);
					return 1;
				}
			}
		}
	}
	lua_pushboolean(L, false);
	return 1;
}

int CEpgTimerSrvMain::LuaAddOrChgManuAdd(lua_State* L)
{
	CLuaWorkspace ws(L);
	if( lua_gettop(L) == 1 && lua_istable(L, -1) ){
		MANUAL_AUTO_ADD_DATA item;
		item.dataID = LuaHelp::get_int(L, "dataID");
		item.dayOfWeekFlag = (BYTE)LuaHelp::get_int(L, "dayOfWeekFlag");
		item.startTime = LuaHelp::get_int(L, "startTime");
		item.durationSecond = LuaHelp::get_int(L, "durationSecond");
		UTF8toW(LuaHelp::get_string(L, "title"), item.title);
		UTF8toW(LuaHelp::get_string(L, "stationName"), item.stationName);
		item.originalNetworkID = (WORD)LuaHelp::get_int(L, "onid");
		item.transportStreamID = (WORD)LuaHelp::get_int(L, "tsid");
		item.serviceID = (WORD)LuaHelp::get_int(L, "sid");
		lua_getfield(L, -1, "recSetting");
		if( lua_istable(L, -1) ){
			FetchRecSettingData(ws, item.recSetting);
			bool modified = true;
			{
				CBlockLock lock(&ws.sys->settingLock);
				if( item.dataID == 0 ){
					item.dataID = ws.sys->manualAutoAdd.AddData(item);
				}else{
					modified = ws.sys->manualAutoAdd.ChgData(item);
				}
				if( modified ){
					ws.sys->manualAutoAdd.SaveText();
				}
			}
			if( modified ){
				ws.sys->AutoAddReserveProgram(item);
				ws.sys->notifyManager.AddNotify(NOTIFY_UPDATE_AUTOADD_MANUAL);
				lua_pushboolean(L, true);
				return 1;
			}
		}
	}
	lua_pushboolean(L, false);
	return 1;
}

int CEpgTimerSrvMain::LuaListDmsPublicFile(lua_State* L)
{
	CLuaWorkspace ws(L);
	CBlockLock lock(&ws.sys->settingLock);
	WIN32_FIND_DATA findData;
	HANDLE hFind = FindFirstFile((ws.sys->httpPublicFolder + L"\\dlna\\dms\\PublicFile\\*").c_str(), &findData);
	vector<pair<int, WIN32_FIND_DATA>> newList;
	if( hFind == INVALID_HANDLE_VALUE ){
		ws.sys->dmsPublicFileList.clear();
	}else{
		if( ws.sys->dmsPublicFileList.empty() ){
			//要素0には公開フォルダの場所と次のIDを格納する
			ws.sys->dmsPublicFileList.push_back(std::make_pair(0, ws.sys->httpPublicFolder));
		}
		do{
			//TODO: 再帰的にリストしほうがいいがとりあえず…
			if( (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 ){
				bool exist = false;
				for( size_t i = 1; i < ws.sys->dmsPublicFileList.size(); i++ ){ 
					if( lstrcmpi(ws.sys->dmsPublicFileList[i].second.c_str(), findData.cFileName) == 0 ){
						newList.push_back(std::make_pair(ws.sys->dmsPublicFileList[i].first, findData));
						exist = true;
						break;
					}
				}
				if( exist == false ){
					newList.push_back(std::make_pair(ws.sys->dmsPublicFileList[0].first++, findData));
				}
			}
		}while( FindNextFile(hFind, &findData) );
		ws.sys->dmsPublicFileList.resize(1);
	}
	lua_newtable(L);
	for( size_t i = 0; i < newList.size(); i++ ){
		//IDとファイル名の対応を記録しておく
		ws.sys->dmsPublicFileList.push_back(std::make_pair(newList[i].first, wstring(newList[i].second.cFileName)));
		lua_newtable(L);
		LuaHelp::reg_int(L, "id", newList[i].first);
		LuaHelp::reg_string(L, "name", ws.WtoUTF8(newList[i].second.cFileName));
		lua_pushstring(L, "size");
		lua_pushnumber(L, (lua_Number)((__int64)newList[i].second.nFileSizeHigh << 32 | newList[i].second.nFileSizeLow));
		lua_rawset(L, -3);
		lua_rawseti(L, -2, (int)i + 1);
	}
	return 1;
}

void CEpgTimerSrvMain::PushEpgEventInfo(CLuaWorkspace& ws, const EPGDB_EVENT_INFO& e)
{
	lua_State* L = ws.L;
	LuaHelp::reg_int(L, "onid", e.original_network_id);
	LuaHelp::reg_int(L, "tsid", e.transport_stream_id);
	LuaHelp::reg_int(L, "sid", e.service_id);
	LuaHelp::reg_int(L, "eid", e.event_id);
	if( e.StartTimeFlag ){
		LuaHelp::reg_time(L, "startTime", e.start_time);
	}
	if( e.DurationFlag ){
		LuaHelp::reg_int(L, "durationSecond", (int)e.durationSec);
	}
	LuaHelp::reg_boolean(L, "freeCAFlag", e.freeCAFlag != 0);
	if( e.shortInfo ){
		lua_pushstring(L, "shortInfo");
		lua_newtable(L);
		LuaHelp::reg_string(L, "event_name", ws.WtoUTF8(e.shortInfo->event_name));
		LuaHelp::reg_string(L, "text_char", ws.WtoUTF8(e.shortInfo->text_char));
		lua_rawset(L, -3);
	}
	if( e.extInfo ){
		lua_pushstring(L, "extInfo");
		lua_newtable(L);
		LuaHelp::reg_string(L, "text_char", ws.WtoUTF8(e.extInfo->text_char));
		lua_rawset(L, -3);
	}
	if( e.contentInfo ){
		lua_pushstring(L, "contentInfoList");
		lua_newtable(L);
		for( size_t i = 0; i < e.contentInfo->nibbleList.size(); i++ ){
			lua_newtable(L);
			LuaHelp::reg_int(L, "content_nibble", e.contentInfo->nibbleList[i].content_nibble_level_1 << 8 | e.contentInfo->nibbleList[i].content_nibble_level_2);
			LuaHelp::reg_int(L, "user_nibble", e.contentInfo->nibbleList[i].user_nibble_1 << 8 | e.contentInfo->nibbleList[i].user_nibble_2);
			lua_rawseti(L, -2, (int)i + 1);
		}
		lua_rawset(L, -3);
	}
	if( e.componentInfo ){
		lua_pushstring(L, "componentInfo");
		lua_newtable(L);
		LuaHelp::reg_int(L, "stream_content", e.componentInfo->stream_content);
		LuaHelp::reg_int(L, "component_type", e.componentInfo->component_type);
		LuaHelp::reg_int(L, "component_tag", e.componentInfo->component_tag);
		LuaHelp::reg_string(L, "text_char", ws.WtoUTF8(e.componentInfo->text_char));
		lua_rawset(L, -3);
	}
	if( e.audioInfo ){
		lua_pushstring(L, "audioInfoList");
		lua_newtable(L);
		for( size_t i = 0; i < e.audioInfo->componentList.size(); i++ ){
			lua_newtable(L);
			LuaHelp::reg_int(L, "stream_content", e.audioInfo->componentList[i].stream_content);
			LuaHelp::reg_int(L, "component_type", e.audioInfo->componentList[i].component_type);
			LuaHelp::reg_int(L, "component_tag", e.audioInfo->componentList[i].component_tag);
			LuaHelp::reg_int(L, "stream_type", e.audioInfo->componentList[i].stream_type);
			LuaHelp::reg_int(L, "simulcast_group_tag", e.audioInfo->componentList[i].simulcast_group_tag);
			LuaHelp::reg_boolean(L, "ES_multi_lingual_flag", e.audioInfo->componentList[i].ES_multi_lingual_flag != 0);
			LuaHelp::reg_boolean(L, "main_component_flag", e.audioInfo->componentList[i].main_component_flag != 0);
			LuaHelp::reg_int(L, "quality_indicator", e.audioInfo->componentList[i].quality_indicator);
			LuaHelp::reg_int(L, "sampling_rate", e.audioInfo->componentList[i].sampling_rate);
			LuaHelp::reg_string(L, "text_char", ws.WtoUTF8(e.audioInfo->componentList[i].text_char));
			lua_rawseti(L, -2, (int)i + 1);
		}
		lua_rawset(L, -3);
	}
	if( e.eventGroupInfo ){
		lua_pushstring(L, "eventGroupInfo");
		lua_newtable(L);
		LuaHelp::reg_int(L, "group_type", e.eventGroupInfo->group_type);
		lua_pushstring(L, "eventDataList");
		lua_newtable(L);
		for( size_t i = 0; i < e.eventGroupInfo->eventDataList.size(); i++ ){
			lua_newtable(L);
			LuaHelp::reg_int(L, "onid", e.eventGroupInfo->eventDataList[i].original_network_id);
			LuaHelp::reg_int(L, "tsid", e.eventGroupInfo->eventDataList[i].transport_stream_id);
			LuaHelp::reg_int(L, "sid", e.eventGroupInfo->eventDataList[i].service_id);
			LuaHelp::reg_int(L, "eid", e.eventGroupInfo->eventDataList[i].event_id);
			lua_rawseti(L, -2, (int)i + 1);
		}
		lua_rawset(L, -3);
		lua_rawset(L, -3);
	}
	if( e.eventRelayInfo ){
		lua_pushstring(L, "eventRelayInfo");
		lua_newtable(L);
		LuaHelp::reg_int(L, "group_type", e.eventRelayInfo->group_type);
		lua_pushstring(L, "eventDataList");
		lua_newtable(L);
		for( size_t i = 0; i < e.eventRelayInfo->eventDataList.size(); i++ ){
			lua_newtable(L);
			LuaHelp::reg_int(L, "onid", e.eventRelayInfo->eventDataList[i].original_network_id);
			LuaHelp::reg_int(L, "tsid", e.eventRelayInfo->eventDataList[i].transport_stream_id);
			LuaHelp::reg_int(L, "sid", e.eventRelayInfo->eventDataList[i].service_id);
			LuaHelp::reg_int(L, "eid", e.eventRelayInfo->eventDataList[i].event_id);
			lua_rawseti(L, -2, (int)i + 1);
		}
		lua_rawset(L, -3);
		lua_rawset(L, -3);
	}
}

void CEpgTimerSrvMain::PushReserveData(CLuaWorkspace& ws, const RESERVE_DATA& r)
{
	lua_State* L = ws.L;
	LuaHelp::reg_string(L, "title", ws.WtoUTF8(r.title));
	LuaHelp::reg_time(L, "startTime", r.startTime);
	LuaHelp::reg_int(L, "durationSecond", (int)r.durationSecond);
	LuaHelp::reg_string(L, "stationName", ws.WtoUTF8(r.stationName));
	LuaHelp::reg_int(L, "onid", r.originalNetworkID);
	LuaHelp::reg_int(L, "tsid", r.transportStreamID);
	LuaHelp::reg_int(L, "sid", r.serviceID);
	LuaHelp::reg_int(L, "eid", r.eventID);
	LuaHelp::reg_string(L, "comment", ws.WtoUTF8(r.comment));
	LuaHelp::reg_int(L, "reserveID", (int)r.reserveID);
	LuaHelp::reg_int(L, "overlapMode", r.overlapMode);
	LuaHelp::reg_time(L, "startTimeEpg", r.startTimeEpg);
	lua_pushstring(L, "recFileNameList");
	lua_newtable(L);
	for( size_t i = 0; i < r.recFileNameList.size(); i++ ){
		lua_pushstring(L, ws.WtoUTF8(r.recFileNameList[i]));
		lua_rawseti(L, -2, (int)i + 1);
	}
	lua_rawset(L, -3);
	lua_pushstring(L, "recSetting");
	lua_newtable(L);
	PushRecSettingData(ws, r.recSetting);
	lua_rawset(L, -3);
}

void CEpgTimerSrvMain::PushRecSettingData(CLuaWorkspace& ws, const REC_SETTING_DATA& rs)
{
	lua_State* L = ws.L;
	LuaHelp::reg_int(L, "recMode", rs.recMode);
	LuaHelp::reg_int(L, "priority", rs.priority);
	LuaHelp::reg_boolean(L, "tuijyuuFlag", rs.tuijyuuFlag != 0);
	LuaHelp::reg_int(L, "serviceMode", (int)rs.serviceMode);
	LuaHelp::reg_boolean(L, "pittariFlag", rs.pittariFlag != 0);
	LuaHelp::reg_string(L, "batFilePath", ws.WtoUTF8(rs.batFilePath));
	LuaHelp::reg_int(L, "suspendMode", rs.suspendMode);
	LuaHelp::reg_boolean(L, "rebootFlag", rs.rebootFlag != 0);
	if( rs.useMargineFlag ){
		LuaHelp::reg_int(L, "startMargin", rs.startMargine);
		LuaHelp::reg_int(L, "endMargin", rs.endMargine);
	}
	LuaHelp::reg_boolean(L, "continueRecFlag", rs.continueRecFlag != 0);
	LuaHelp::reg_int(L, "partialRecFlag", rs.partialRecFlag);
	LuaHelp::reg_int(L, "tunerID", (int)rs.tunerID);
	for( int i = 0; i < 2; i++ ){
		const vector<REC_FILE_SET_INFO>& recFolderList = i == 0 ? rs.recFolderList : rs.partialRecFolder;
		lua_pushstring(L, i == 0 ? "recFolderList" : "partialRecFolder");
		lua_newtable(L);
		for( size_t j = 0; j < recFolderList.size(); j++ ){
			lua_newtable(L);
			LuaHelp::reg_string(L, "recFolder", ws.WtoUTF8(recFolderList[j].recFolder));
			LuaHelp::reg_string(L, "writePlugIn", ws.WtoUTF8(recFolderList[j].writePlugIn));
			LuaHelp::reg_string(L, "recNamePlugIn", ws.WtoUTF8(recFolderList[j].recNamePlugIn));
			lua_rawseti(L, -2, (int)j + 1);
		}
		lua_rawset(L, -3);
	}
}

void CEpgTimerSrvMain::PushEpgSearchKeyInfo(CLuaWorkspace& ws, const EPGDB_SEARCH_KEY_INFO& k)
{
	lua_State* L = ws.L;
	LuaHelp::reg_string(L, "andKey", ws.WtoUTF8(k.andKey));
	LuaHelp::reg_string(L, "notKey", ws.WtoUTF8(k.notKey));
	LuaHelp::reg_boolean(L, "regExpFlag", k.regExpFlag != 0);
	LuaHelp::reg_boolean(L, "titleOnlyFlag", k.titleOnlyFlag != 0);
	LuaHelp::reg_boolean(L, "aimaiFlag", k.aimaiFlag != 0);
	LuaHelp::reg_boolean(L, "notContetFlag", k.notContetFlag != 0);
	LuaHelp::reg_boolean(L, "notDateFlag", k.notDateFlag != 0);
	LuaHelp::reg_int(L, "freeCAFlag", k.freeCAFlag);
	LuaHelp::reg_boolean(L, "chkRecEnd", k.chkRecEnd != 0);
	LuaHelp::reg_int(L, "chkRecDay", k.chkRecDay);
	lua_pushstring(L, "contentList");
	lua_newtable(L);
	for( size_t i = 0; i < k.contentList.size(); i++ ){
		lua_newtable(L);
		LuaHelp::reg_int(L, "content_nibble", k.contentList[i].content_nibble_level_1 << 8 | k.contentList[i].content_nibble_level_2);
		lua_rawseti(L, -2, (int)i + 1);
	}
	lua_rawset(L, -3);
	lua_pushstring(L, "dateList");
	lua_newtable(L);
	for( size_t i = 0; i < k.dateList.size(); i++ ){
		lua_newtable(L);
		LuaHelp::reg_int(L, "startDayOfWeek", k.dateList[i].startDayOfWeek);
		LuaHelp::reg_int(L, "startHour", k.dateList[i].startHour);
		LuaHelp::reg_int(L, "startMin", k.dateList[i].startMin);
		LuaHelp::reg_int(L, "endDayOfWeek", k.dateList[i].endDayOfWeek);
		LuaHelp::reg_int(L, "endHour", k.dateList[i].endHour);
		LuaHelp::reg_int(L, "endMin", k.dateList[i].endMin);
		lua_rawseti(L, -2, (int)i + 1);
	}
	lua_rawset(L, -3);
	lua_pushstring(L, "serviceList");
	lua_newtable(L);
	for( size_t i = 0; i < k.serviceList.size(); i++ ){
		lua_newtable(L);
		LuaHelp::reg_int(L, "onid", k.serviceList[i] >> 32 & 0xFFFF);
		LuaHelp::reg_int(L, "tsid", k.serviceList[i] >> 16 & 0xFFFF);
		LuaHelp::reg_int(L, "sid", k.serviceList[i] & 0xFFFF);
		lua_rawseti(L, -2, (int)i + 1);
	}
	lua_rawset(L, -3);
}

bool CEpgTimerSrvMain::FetchReserveData(CLuaWorkspace& ws, RESERVE_DATA& r)
{
	lua_State* L = ws.L;
	UTF8toW(LuaHelp::get_string(L, "title"), r.title);
	r.startTime = LuaHelp::get_time(L, "startTime");
	r.durationSecond = LuaHelp::get_int(L, "durationSecond");
	UTF8toW(LuaHelp::get_string(L, "stationName"), r.stationName);
	r.originalNetworkID = (WORD)LuaHelp::get_int(L, "onid");
	r.transportStreamID = (WORD)LuaHelp::get_int(L, "tsid");
	r.serviceID = (WORD)LuaHelp::get_int(L, "sid");
	r.eventID = (WORD)LuaHelp::get_int(L, "eid");
	r.reserveID = (WORD)LuaHelp::get_int(L, "reserveID");
	r.startTimeEpg = LuaHelp::get_time(L, "startTimeEpg");
	lua_getfield(L, -1, "recSetting");
	if( r.startTime.wYear && r.startTimeEpg.wYear && lua_istable(L, -1) ){
		FetchRecSettingData(ws, r.recSetting);
		lua_pop(L, 1);
		return true;
	}
	lua_pop(L, 1);
	return false;
}

void CEpgTimerSrvMain::FetchRecSettingData(CLuaWorkspace& ws, REC_SETTING_DATA& rs)
{
	lua_State* L = ws.L;
	rs.recMode = (BYTE)LuaHelp::get_int(L, "recMode");
	rs.priority = (BYTE)LuaHelp::get_int(L, "priority");
	rs.tuijyuuFlag = LuaHelp::get_boolean(L, "tuijyuuFlag");
	rs.serviceMode = (BYTE)LuaHelp::get_int(L, "serviceMode");
	rs.pittariFlag = LuaHelp::get_boolean(L, "pittariFlag");
	UTF8toW(LuaHelp::get_string(L, "batFilePath"), rs.batFilePath);
	rs.suspendMode = (BYTE)LuaHelp::get_int(L, "suspendMode");
	rs.rebootFlag = LuaHelp::get_boolean(L, "rebootFlag");
	rs.useMargineFlag = LuaHelp::isnil(L, "startMargin") == false;
	rs.startMargine = LuaHelp::get_int(L, "startMargin");
	rs.endMargine = LuaHelp::get_int(L, "endMargin");
	rs.continueRecFlag = LuaHelp::get_boolean(L, "continueRecFlag");
	rs.partialRecFlag = (BYTE)LuaHelp::get_int(L, "partialRecFlag");
	rs.tunerID = LuaHelp::get_int(L, "tunerID");
	for( int i = 0; i < 2; i++ ){
		lua_getfield(L, -1, i == 0 ? "recFolderList" : "partialRecFolder");
		if( lua_istable(L, -1) ){
			for( int j = 0;; j++ ){
				lua_rawgeti(L, -1, j + 1);
				if( !lua_istable(L, -1) ){
					lua_pop(L, 1);
					break;
				}
				vector<REC_FILE_SET_INFO>& recFolderList = i == 0 ? rs.recFolderList : rs.partialRecFolder;
				recFolderList.resize(j + 1);
				UTF8toW(LuaHelp::get_string(L, "recFolder"), recFolderList[j].recFolder);
				UTF8toW(LuaHelp::get_string(L, "writePlugIn"), recFolderList[j].writePlugIn);
				UTF8toW(LuaHelp::get_string(L, "recNamePlugIn"), recFolderList[j].recNamePlugIn);
				lua_pop(L, 1);
			}
		}
		lua_pop(L, 1);
	}
}

void CEpgTimerSrvMain::FetchEpgSearchKeyInfo(CLuaWorkspace& ws, EPGDB_SEARCH_KEY_INFO& k)
{
	lua_State* L = ws.L;
	UTF8toW(LuaHelp::get_string(L, "andKey"), k.andKey);
	UTF8toW(LuaHelp::get_string(L, "notKey"), k.notKey);
	k.regExpFlag = LuaHelp::get_boolean(L, "regExpFlag");
	k.titleOnlyFlag = LuaHelp::get_boolean(L, "titleOnlyFlag");
	k.aimaiFlag = LuaHelp::get_boolean(L, "aimaiFlag");
	k.notContetFlag = LuaHelp::get_boolean(L, "notContetFlag");
	k.notDateFlag = LuaHelp::get_boolean(L, "notDateFlag");
	k.freeCAFlag = (BYTE)LuaHelp::get_int(L, "freeCAFlag");
	k.chkRecEnd = LuaHelp::get_boolean(L, "chkRecEnd");
	k.chkRecDay = (WORD)LuaHelp::get_int(L, "chkRecDay");
	lua_getfield(L, -1, "contentList");
	if( lua_istable(L, -1) ){
		for( int i = 0;; i++ ){
			lua_rawgeti(L, -1, i + 1);
			if( !lua_istable(L, -1) ){
				lua_pop(L, 1);
				break;
			}
			k.contentList.resize(i + 1);
			k.contentList[i].content_nibble_level_1 = LuaHelp::get_int(L, "content_nibble") >> 8 & 0xFF;
			k.contentList[i].content_nibble_level_2 = LuaHelp::get_int(L, "content_nibble") & 0xFF;
			lua_pop(L, 1);
		}
	}
	lua_pop(L, 1);
	lua_getfield(L, -1, "dateList");
	if( lua_istable(L, -1) ){
		for( int i = 0;; i++ ){
			lua_rawgeti(L, -1, i + 1);
			if( !lua_istable(L, -1) ){
				lua_pop(L, 1);
				break;
			}
			k.dateList.resize(i + 1);
			k.dateList[i].startDayOfWeek = (BYTE)LuaHelp::get_int(L, "startDayOfWeek");
			k.dateList[i].startHour = (WORD)LuaHelp::get_int(L, "startHour");
			k.dateList[i].startMin = (WORD)LuaHelp::get_int(L, "startMin");
			k.dateList[i].endDayOfWeek = (BYTE)LuaHelp::get_int(L, "endDayOfWeek");
			k.dateList[i].endHour = (WORD)LuaHelp::get_int(L, "endHour");
			k.dateList[i].endMin = (WORD)LuaHelp::get_int(L, "endMin");
			lua_pop(L, 1);
		}
	}
	lua_pop(L, 1);
	lua_getfield(L, -1, "serviceList");
	if( lua_istable(L, -1) ){
		for( int i = 0;; i++ ){
			lua_rawgeti(L, -1, i + 1);
			if( !lua_istable(L, -1) ){
				lua_pop(L, 1);
				break;
			}
			k.serviceList.push_back(
				(__int64)LuaHelp::get_int(L, "onid") << 32 |
				(__int64)LuaHelp::get_int(L, "tsid") << 16 |
				LuaHelp::get_int(L, "sid"));
			lua_pop(L, 1);
		}
	}
	lua_pop(L, 1);
}

//Lua-edcb空間のコールバックここまで
#endif
