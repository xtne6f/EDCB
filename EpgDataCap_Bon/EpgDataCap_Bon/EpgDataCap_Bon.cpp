
// EpgDataCap_Bon.cpp : アプリケーションのクラス動作を定義します。
//

#include "stdafx.h"
#include "EpgDataCap_Bon.h"
#include "../../Common/PathUtil.h"
#include "../../Common/ThreadUtil.h"
#include "../../Common/TimeUtil.h"
#ifdef _WIN32
#include "EpgDataCap_BonDlg.h"
#include "../../Common/StackTrace.h"
#include <objbase.h>
#include <shellapi.h>
#else
#include "EpgDataCap_BonMin.h"
#include <signal.h>
#endif

namespace
{
FILE* g_debugLog;
recursive_mutex_ g_debugLogLock;
}

#ifdef _WIN32

// CEpgDataCap_BonApp コンストラクション

CEpgDataCap_BonApp::CEpgDataCap_BonApp()
{
	// TODO: この位置に構築用コードを追加してください。
	// ここに InitInstance 中の重要な初期化処理をすべて記述してください。
}

// CEpgDataCap_BonApp 初期化

BOOL CEpgDataCap_BonApp::InitInstance()
{
	// アプリケーション マニフェストが visual スタイルを有効にするために、
	// ComCtl32.dll Version 6 以降の使用を指定する場合は、
	// Windows XP に InitCommonControlsEx() が必要です。さもなければ、ウィンドウ作成はすべて失敗します。
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// アプリケーションで使用するすべてのコモン コントロール クラスを含めるには、
	// これを設定します。
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	SetProcessShutdownParameters(0x300, 0);

#ifndef SUPPRESS_OUTPUT_STACK_TRACE
	SetOutputStackTraceOnUnhandledException(GetModulePath().concat(L".err").c_str());
#endif

	CEpgDataCap_BonDlg dlg;

	// コマンドオプションを解析
	int argc;
	LPWSTR *argv = CommandLineToArgvW(GetCommandLine(), &argc);
	if (argv != NULL) {
		dlg.ParseCommandLine(argv, argc);
		LocalFree(argv);
	}


	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: ダイアログが <OK> で消された時のコードを
		//  記述してください。
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: ダイアログが <キャンセル> で消された時のコードを
		//  記述してください。
	}

	// ダイアログは閉じられました。アプリケーションのメッセージ ポンプを開始しないで
	//  アプリケーションを終了するために FALSE を返してください。
	return FALSE;
}

#ifdef __MINGW32__
__declspec(dllexport) //ASLRを無効にしないため(CVE-2018-5392)
#endif
int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
	SetDllDirectory(L"");
	SetSaveDebugLog(GetPrivateProfileInt(L"SET", L"SaveDebugLog", 0, GetModuleIniPath().c_str()) != 0);
	//メインスレッドに対するCOMの初期化
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	CEpgDataCap_BonApp().InitInstance();
	CoUninitialize();
	SetSaveDebugLog(false);
	return 0;
}

#else

int main(int argc, char** argv)
{
	if( CEpgDataCap_BonMin::ValidateCommandLine(argv, argc) == false ){
		return 2;
	}
	struct sigaction sigact = {};
	sigact.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sigact, NULL);

	SetSaveDebugLog(GetPrivateProfileInt(L"SET", L"SaveDebugLog", 0, GetModuleIniPath().c_str()) != 0);
	CEpgDataCap_BonMin app;
	app.ParseCommandLine(argv, argc);
	app.Main();
	SetSaveDebugLog(false);
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
		for( int i = 0; i < 100; i++ ){
			//パスに添え字をつけて書き込み可能な最初のものに記録する
			WCHAR logFileName[64];
			swprintf_s(logFileName, L"EpgDataCap_Bon_DebugLog-%d.txt", i);
			fs_path logPath = GetCommonIniPath().replace_filename(logFileName);
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
				break;
			}
		}
	}else if( g_debugLog && saveDebugLog == false ){
		AddDebugLog(L"****** LOG STOP ******");
		fclose(g_debugLog);
		g_debugLog = NULL;
	}
}
