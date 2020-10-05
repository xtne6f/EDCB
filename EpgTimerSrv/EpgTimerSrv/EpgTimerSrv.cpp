// EpgTimerSrv.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include "EpgTimerSrvMain.h"
#include "../../Common/PathUtil.h"
#include "../../Common/ServiceUtil.h"
#include "../../Common/StackTrace.h"
#include "../../Common/ThreadUtil.h"
#include "../../Common/CommonDef.h"
#include <winsvc.h>
#include <objbase.h>
#include <shellapi.h>

namespace
{
SERVICE_STATUS_HANDLE g_hStatusHandle;
CEpgTimerSrvMain* g_pMain;
FILE* g_debugLog;
recursive_mutex_ g_debugLogLock;

//サービス動作用のメイン
void WINAPI service_main(DWORD dwArgc, LPWSTR* lpszArgv);
}

#ifdef __MINGW32__
__declspec(dllexport) //ASLRを無効にしないため(CVE-2018-5392)
#endif
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	SetDllDirectory(L"");

	WCHAR option[16] = {};
	int argc;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLine(), &argc);
	if( argv ){
		if( argc >= 2 ){
			wcsncpy_s(option, argv[1], _TRUNCATE);
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
			CEpgTimerSrvMain::TaskMain();
			CoUninitialize();
			return 0;
		}
	}

#ifndef SUPPRESS_OUTPUT_STACK_TRACE
	SetOutputStackTraceOnUnhandledException(GetModulePath().concat(L".err").c_str());
#endif

	if( IsInstallService(SERVICE_NAME) == FALSE ){
		//普通にexeとして起動を行う
		HANDLE hMutex = CreateMutex(NULL, FALSE, EPG_TIMER_BON_SRV_MUTEX);
		if( hMutex != NULL ){
			if( GetLastError() != ERROR_ALREADY_EXISTS ){
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
			CloseHandle(hMutex);
		}
	}else if( IsStopService(SERVICE_NAME) == FALSE ){
		//サービスとして実行
		HANDLE hMutex = CreateMutex(NULL, FALSE, EPG_TIMER_BON_SRV_MUTEX);
		if( hMutex != NULL ){
			if( GetLastError() != ERROR_ALREADY_EXISTS ){
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
			CloseHandle(hMutex);
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
	g_hStatusHandle = RegisterServiceCtrlHandlerEx(SERVICE_NAME, service_ctrl, NULL);
	if( g_hStatusHandle != NULL ){
		ReportServiceStatus(SERVICE_START_PENDING, 0, 1, 10000);

		//メインスレッドに対するCOMの初期化
		CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		//ここでは単純な(時間のかからない)初期化のみ行う
		g_pMain = new CEpgTimerSrvMain;

		ReportServiceStatus(SERVICE_RUNNING, SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_POWEREVENT, 0, 0);

		if( g_pMain->Main(true) == false ){
			AddDebugLog(L"service_main(): Failed to start");
		}
		delete g_pMain;
		g_pMain = NULL;
		CoUninitialize();

		ReportServiceStatus(SERVICE_STOPPED, 0, 0, 0);
	}
}

DWORD WINAPI service_ctrl(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext)
{
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

void AddDebugLogNoNewline(const wchar_t* lpOutputString, bool suppressDebugOutput)
{
	{
		//デバッグ出力ログ保存
		CBlockLock lock(&g_debugLogLock);
		if( g_debugLog ){
			SYSTEMTIME st;
			GetLocalTime(&st);
			WCHAR t[128];
			int n = swprintf_s(t, L"[%02d%02d%02d%02d%02d%02d.%03d] ",
			                   st.wYear % 100, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
			fwrite(t, sizeof(WCHAR), n, g_debugLog);
			size_t m = lpOutputString ? wcslen(lpOutputString) : 0;
			if( m > 0 ){
				fwrite(lpOutputString, sizeof(WCHAR), m, g_debugLog);
			}
			if( m == 0 || lpOutputString[m - 1] != L'\n' ){
				fwrite(L"<NOBR>" UTIL_NEWLINE, sizeof(WCHAR), array_size(L"<NOBR>" UTIL_NEWLINE) - 1, g_debugLog);
			}
			fflush(g_debugLog);
		}
	}
	if( suppressDebugOutput == false ){
		OutputDebugString(lpOutputString);
	}
}

void SetSaveDebugLog(bool saveDebugLog)
{
	CBlockLock lock(&g_debugLogLock);
	if( g_debugLog == NULL && saveDebugLog ){
		fs_path logPath = GetCommonIniPath().replace_filename(L"EpgTimerSrvDebugLog.txt");
		g_debugLog = UtilOpenFile(logPath, UTIL_O_EXCL_CREAT_APPEND | UTIL_SH_READ);
		if( g_debugLog ){
			fwrite(L"\xFEFF", sizeof(WCHAR), 1, g_debugLog);
		}else{
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
