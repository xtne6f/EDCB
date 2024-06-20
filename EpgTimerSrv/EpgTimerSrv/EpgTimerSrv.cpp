// EpgTimerSrv.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include "EpgTimerSrvMain.h"
#include "../../Common/PathUtil.h"
#include "../../Common/ThreadUtil.h"
#include "../../Common/TimeUtil.h"
#include "../../Common/CommonDef.h"
#ifdef _WIN32
#include "EpgTimerTask.h"
#include "../../Common/ServiceUtil.h"
#include "../../Common/StackTrace.h"
#include <winsvc.h>
#include <objbase.h>
#include <shellapi.h>
#else
#include "../../Common/CommonResource.h"
#include <signal.h>
#endif

namespace
{
FILE* g_debugLog;
recursive_mutex_ g_debugLogLock;
}

#ifdef _WIN32

namespace
{
SERVICE_STATUS_HANDLE g_hStatusHandle;
CEpgTimerSrvMain* g_pMain;

//サービス動作用のメイン
void WINAPI service_main(DWORD dwArgc, LPWSTR* lpszArgv);
}

#ifdef __MINGW32__
__declspec(dllexport) //ASLRを無効にしないため(CVE-2018-5392)
#endif
int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
	SetDllDirectory(L"");

	WCHAR option[16] = {};
	vector<WCHAR> param;
	int argc;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLine(), &argc);
	if( argv ){
		if( argc >= 2 ){
			wcsncpy_s(option, argv[1], _TRUNCATE);
		}
		if( argc >= 3 ){
			param.assign(argv[2], argv[2] + wcslen(argv[2]));
		}
		LocalFree(argv);
	}

	if( CompareNoCase(GetModulePath().stem().c_str(), L"EpgTimerTask") == 0 ){
		//Taskモードを強制する
		wcscpy_s(option, L"/task");
	}
	if( option[0] == L'-' || option[0] == L'/' ){
		if( CompareNoCase(L"install", option + 1) == 0 ){
			return 0;
		}else if( CompareNoCase(L"remove", option + 1) == 0 ){
			return 0;
		}else if( CompareNoCase(L"setting", option + 1) == 0 ){
			//設定ダイアログを表示する
			CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
			CEpgTimerSrvSetting setting;
			setting.ShowDialog();
			CoUninitialize();
			return 0;
		}else if( CompareNoCase(L"task", option + 1) == 0 ){
			//Taskモード
			CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
			CEpgTimerTask().Main();
			CoUninitialize();
			return 0;
		}else if( CompareNoCase(L"luapost", option + 1) == 0 ){
			if( param.empty() == false ){
				COPYDATASTRUCT cds;
				cds.dwData = COPYDATA_TYPE_LUAPOST;
				cds.cbData = (DWORD)(param.size() * sizeof(WCHAR));
				cds.lpData = param.data();
				HWND hwnd = NULL;
				while( (hwnd = FindWindowEx(NULL, hwnd, SERVICE_NAME, NULL)) != NULL ){
					//ウィンドウスレッドはさほどビジーにならないので想定する最悪値として10秒タイムアウトとする
					DWORD_PTR dwResult;
					if( SendMessageTimeout(hwnd, WM_COPYDATA, 0, (LPARAM)&cds, SMTO_NORMAL, 10000, &dwResult) && dwResult ){
						break;
					}
				}
			}
			//コンソールアプリではないので戻り値は成否によらない
			return 0;
		}
	}

#ifndef SUPPRESS_OUTPUT_STACK_TRACE
	SetOutputStackTraceOnUnhandledException(GetModulePath().concat(L".err").c_str());
#endif

	if( IsInstallService(SERVICE_NAME) == FALSE ){
		//普通にexeとして起動を行う
		util_unique_handle mutex = UtilCreateGlobalMutex(EPG_TIMER_BON_SRV_MUTEX);
		if( mutex ){
			SetSaveDebugLog(GetPrivateProfileInt(L"SET", L"SaveDebugLog", 0, GetModuleIniPath().c_str()) != 0);
			//メインスレッドに対するCOMの初期化
			CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
			CEpgTimerSrvMain* pMain = new CEpgTimerSrvMain;
			if( pMain->Main(false) == false ){
				AddDebugLog(L"_tWinMain(): Failed to start");
			}
			delete pMain;
			CoUninitialize();
			SetSaveDebugLog(false);
		}
	}else if( IsStopService(SERVICE_NAME) == FALSE ){
		//サービスとして実行
		util_unique_handle mutex = UtilCreateGlobalMutex(EPG_TIMER_BON_SRV_MUTEX);
		if( mutex ){
			SetSaveDebugLog(GetPrivateProfileInt(L"SET", L"SaveDebugLog", 0, GetModuleIniPath().c_str()) != 0);
			WCHAR serviceName[] = SERVICE_NAME;
			SERVICE_TABLE_ENTRY dispatchTable[] = {
				{ serviceName, service_main },
				{ NULL, NULL }
			};
			if( StartServiceCtrlDispatcher(dispatchTable) == FALSE ){
				AddDebugLog(L"_tWinMain(): StartServiceCtrlDispatcher failed");
			}
			SetSaveDebugLog(false);
		}
	}

	return 0;
}

namespace
{
//サービスからのコマンドのコールバック
DWORD WINAPI service_ctrl(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext);

//サービスのステータス通知用
void ReportServiceStatus(DWORD dwCurrentState, DWORD dwControlsAccepted, DWORD dwCheckPoint, DWORD dwWaitHint);

void WINAPI service_main(DWORD dwArgc, LPWSTR* lpszArgv)
{
	(void)dwArgc;
	(void)lpszArgv;
	g_hStatusHandle = RegisterServiceCtrlHandlerEx(SERVICE_NAME, service_ctrl, NULL);
	if( g_hStatusHandle != NULL ){
		ReportServiceStatus(SERVICE_START_PENDING, 0, 1, 10000);

		//メインスレッドに対するCOMの初期化
		CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		{
			//ここでは単純な(時間のかからない)初期化のみ行う
			CEpgTimerSrvMain appMain;
			g_pMain = &appMain;

			ReportServiceStatus(SERVICE_RUNNING, SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_POWEREVENT, 0, 0);

			if( appMain.Main(true) == false ){
				AddDebugLog(L"service_main(): Failed to start");
			}
			g_pMain = NULL;
		}
		CoUninitialize();

		ReportServiceStatus(SERVICE_STOPPED, 0, 0, 0);
	}
}

DWORD WINAPI service_ctrl(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext)
{
	(void)lpEventData;
	(void)lpContext;
	switch (dwControl){
		case SERVICE_CONTROL_STOP:
		case SERVICE_CONTROL_SHUTDOWN:
			ReportServiceStatus(SERVICE_STOP_PENDING, 0, 0, 10000);
			g_pMain->StopMain();
			return NO_ERROR;
		case SERVICE_CONTROL_POWEREVENT:
			if( dwEventType == PBT_APMQUERYSUSPEND ){
				//Vista以降は呼ばれない
				AddDebugLog(L"PBT_APMQUERYSUSPEND");
				if( g_pMain->IsSuspendOK() == false ){
					AddDebugLog(L"BROADCAST_QUERY_DENY");
					return BROADCAST_QUERY_DENY;
				}
			}else if( dwEventType == PBT_APMRESUMESUSPEND ){
				AddDebugLog(L"PBT_APMRESUMESUSPEND");
			}
			return NO_ERROR;
		default:
			break;
	}
	return ERROR_CALL_NOT_IMPLEMENTED;
}

void ReportServiceStatus(DWORD dwCurrentState, DWORD dwControlsAccepted, DWORD dwCheckPoint, DWORD dwWaitHint)
{
	SERVICE_STATUS ss;

	ss.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	ss.dwCurrentState = dwCurrentState;
	ss.dwControlsAccepted = dwControlsAccepted;
	ss.dwWin32ExitCode = NO_ERROR;
	ss.dwServiceSpecificExitCode = 0;
	ss.dwCheckPoint = dwCheckPoint;
	ss.dwWaitHint = dwWaitHint;

	SetServiceStatus(g_hStatusHandle, &ss);
}
}

#else

int main(int argc, char** argv)
{
	if( argc >= 2 && strcmp(argv[1], "-h") == 0 ){
		printf("Ver.%s\n",
#ifdef EDCB_VERSION_TEXT
		       EDCB_VERSION_TEXT
#else
		       " ?"
#endif
		       );
		return 2;
	}
	struct sigaction sigact = {};
	sigact.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sigact, NULL);

	util_unique_handle mutex = UtilCreateGlobalMutex(EPG_TIMER_BON_SRV_MUTEX);
	if( mutex ){
		SetSaveDebugLog(GetPrivateProfileInt(L"SET", L"SaveDebugLog", 0, GetModuleIniPath().c_str()) != 0);
		{
			CEpgTimerSrvMain appMain;
			if( appMain.Main(true) == false ){
				AddDebugLog(L"main(): Failed to start");
			}
		}
		SetSaveDebugLog(false);
	}
	return 0;
}

#endif

void AddDebugLogNoNewline(const wchar_t* lpOutputString, bool suppressDebugOutput)
{
	if( lpOutputString[0] ){
		//デバッグ出力ログ保存
		lock_recursive_mutex lock(g_debugLogLock);
		if( g_debugLog ){
			SYSTEMTIME st;
			ConvertSystemTime(GetNowI64Time(), &st);
			WCHAR t[128];
			int n = swprintf_s(t, L"[%02d%02d%02d%02d%02d%02d.%03d] ",
			                   st.wYear % 100, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
#if WCHAR_MAX > 0xFFFF
			for( int i = 0; i < n; i++ ){
				char dest[4];
				fwrite(dest, 1, codepoint_to_utf8(t[i], dest), g_debugLog);
			}
			for( size_t i = 0; lpOutputString[i]; i++ ){
				char dest[4];
				fwrite(dest, 1, codepoint_to_utf8(lpOutputString[i], dest), g_debugLog);
			}
#else
			fwrite(t, sizeof(WCHAR), n, g_debugLog);
			fwrite(lpOutputString, sizeof(WCHAR), wcslen(lpOutputString), g_debugLog);
#endif
			fflush(g_debugLog);
		}
	}
	if( suppressDebugOutput == false ){
#ifdef _WIN32
		OutputDebugString(lpOutputString);
#elif WCHAR_MAX > 0xFFFF && 0
		for( size_t i = 0; lpOutputString[i]; i++ ){
			char dest[4];
			fwrite(dest, 1, codepoint_to_utf8(lpOutputString[i], dest), stderr);
		}
#endif
	}
}

void SetSaveDebugLog(bool saveDebugLog)
{
	lock_recursive_mutex lock(g_debugLogLock);
	if( g_debugLog == NULL && saveDebugLog ){
		fs_path logPath = GetCommonIniPath().replace_filename(L"EpgTimerSrvDebugLog.txt");
#if WCHAR_MAX <= 0xFFFF
		g_debugLog = UtilOpenFile(logPath, UTIL_O_EXCL_CREAT_APPEND | UTIL_SH_READ);
		if( g_debugLog ){
			fwrite(L"\xFEFF", sizeof(WCHAR), 1, g_debugLog);
		}else
#endif
		{
			g_debugLog = UtilOpenFile(logPath, UTIL_O_CREAT_APPEND | UTIL_SH_READ);
		}
		if( g_debugLog ){
			AddDebugLog(L"****** LOG START ******");
		}
	}else if( g_debugLog && saveDebugLog == false ){
		AddDebugLog(L"****** LOG STOP ******");
		fclose(g_debugLog);
		g_debugLog = NULL;
	}
}
