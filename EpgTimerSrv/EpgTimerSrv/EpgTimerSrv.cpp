// EpgTimerSrv.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include "EpgTimerSrv.h"
#include "EpgTimerSrvMain.h"
#include "../../Common/PathUtil.h"
#include "../../Common/ServiceUtil.h"
#include "../../Common/ThreadUtil.h"

#include "../../Common/CommonDef.h"
#include <WinSvc.h>
#include <ObjBase.h>

namespace
{
SERVICE_STATUS_HANDLE g_hStatusHandle;
CEpgTimerSrvMain* g_pMain;
FILE* g_debugLog;
recursive_mutex_ g_debugLogLock;
bool g_saveDebugLog;

void StartDebugLog()
{
	if( GetPrivateProfileInt(L"SET", L"SaveDebugLog", 0, GetModuleIniPath().c_str()) != 0 ){
		fs_path logPath = GetModulePath().replace_filename(L"EpgTimerSrvDebugLog.txt");
		g_debugLog = shared_wfopen(logPath.c_str(), L"abN");
		if( g_debugLog ){
			_fseeki64(g_debugLog, 0, SEEK_END);
			if( _ftelli64(g_debugLog) == 0 ){
				fputwc(L'\xFEFF', g_debugLog);
			}
			g_saveDebugLog = true;
			OutputDebugString(L"****** LOG START ******\r\n");
		}
	}
}

void StopDebugLog()
{
	if( g_saveDebugLog ){
		OutputDebugString(L"****** LOG STOP ******\r\n");
		g_saveDebugLog = false;
		fclose(g_debugLog);
	}
}
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	SetDllDirectory(L"");

	WCHAR szTask[] = L"/task";
	if( _wcsicmp(GetModulePath().stem().c_str(), L"EpgTimerTask") == 0 ){
		//Taskモードを強制する
		lpCmdLine = szTask;
	}
	if( lpCmdLine[0] == L'-' || lpCmdLine[0] == L'/' ){
		if( _wcsicmp(L"install", lpCmdLine + 1) == 0 ){
			return 0;
		}else if( _wcsicmp(L"remove", lpCmdLine + 1) == 0 ){
			return 0;
		}else if( _wcsicmp(L"setting", lpCmdLine + 1) == 0 ){
			//設定ダイアログを表示する
			CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
			CEpgTimerSrvSetting setting;
			setting.ShowDialog();
			CoUninitialize();
			return 0;
		}else if( _wcsicmp(L"task", lpCmdLine + 1) == 0 ){
			//Taskモード
			CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
			CEpgTimerSrvMain::TaskMain();
			CoUninitialize();
			return 0;
		}
	}


	if( IsInstallService(SERVICE_NAME) == FALSE ){
		//普通にexeとして起動を行う
		HANDLE hMutex = CreateMutex(NULL, FALSE, EPG_TIMER_BON_SRV_MUTEX);
		if( hMutex != NULL ){
			if( GetLastError() != ERROR_ALREADY_EXISTS ){
				StartDebugLog();
				//メインスレッドに対するCOMの初期化
				CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
				CEpgTimerSrvMain* pMain = new CEpgTimerSrvMain;
				if( pMain->Main(false) == false ){
					OutputDebugString(L"_tWinMain(): Failed to start\r\n");
				}
				delete pMain;
				CoUninitialize();
				StopDebugLog();
			}
			CloseHandle(hMutex);
		}
	}else if( IsStopService(SERVICE_NAME) == FALSE ){
		//サービスとして実行
		HANDLE hMutex = CreateMutex(NULL, FALSE, EPG_TIMER_BON_SRV_MUTEX);
		if( hMutex != NULL ){
			if( GetLastError() != ERROR_ALREADY_EXISTS ){
				StartDebugLog();
				SERVICE_TABLE_ENTRY dispatchTable[] = {
					{ SERVICE_NAME, service_main },
					{ NULL, NULL }
				};
				if( StartServiceCtrlDispatcher(dispatchTable) == FALSE ){
					OutputDebugString(L"_tWinMain(): StartServiceCtrlDispatcher failed\r\n");
				}
				StopDebugLog();
			}
			CloseHandle(hMutex);
		}
	}else{
		//Stop状態なのでサービスの開始を要求
		bool started = false;
		SC_HANDLE hScm = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
		if( hScm != NULL ){
			SC_HANDLE hSrv = OpenService(hScm, SERVICE_NAME, SERVICE_START);
			if( hSrv != NULL ){
				started = StartService(hSrv, 0, NULL) != FALSE;
				CloseServiceHandle(hSrv);
			}
			CloseServiceHandle(hScm);
		}
		if( started == false ){
			OutputDebugString(L"_tWinMain(): Failed to start\r\n");
		}
	}

	return 0;
}

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
			OutputDebugString(L"service_main(): Failed to start\r\n");
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
				OutputDebugString(L"PBT_APMQUERYSUSPEND\r\n");
				if( g_pMain->IsSuspendOK() == false ){
					OutputDebugString(L"BROADCAST_QUERY_DENY\r\n");
					return BROADCAST_QUERY_DENY;
				}
			}else if( dwEventType == PBT_APMRESUMESUSPEND ){
				OutputDebugString(L"PBT_APMRESUMESUSPEND\r\n");
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

void OutputDebugStringWrapper(LPCWSTR lpOutputString)
{
	if( g_saveDebugLog ){
		//デバッグ出力ログ保存
		CBlockLock lock(&g_debugLogLock);
		SYSTEMTIME st;
		GetLocalTime(&st);
		fwprintf(g_debugLog, L"[%02d%02d%02d%02d%02d%02d.%03d] %s%s",
		         st.wYear % 100, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
		         lpOutputString ? lpOutputString : L"",
		         lpOutputString && lpOutputString[0] && lpOutputString[wcslen(lpOutputString) - 1] == L'\n' ? L"" : L"<NOBR>\r\n");
		fflush(g_debugLog);
	}
	OutputDebugStringW(lpOutputString);
}
