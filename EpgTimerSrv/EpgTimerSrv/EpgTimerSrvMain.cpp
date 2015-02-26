#include "StdAfx.h"
#include "EpgTimerSrvMain.h"
#include "../../Common/PipeServer.h"
#include "../../Common/TCPServer.h"
#include "../../Common/SendCtrlCmd.h"
#include "../../Common/PathUtil.h"
#include "../../Common/TimeUtil.h"
#include "../../Common/BlockLock.h"
#include <tlhelp32.h>

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
			{
				CBlockLock lock(&this->settingLock);
				tcpPort_ = this->tcpPort;
			}
			if( tcpPort_ == 0 ){
				tcpServer.StopServer();
			}else{
				tcpServer.StartServer(tcpPort_, CtrlCmdCallback, this, 0, GetCurrentProcessId());
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
