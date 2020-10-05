
// EpgDataCap_Bon.cpp : アプリケーションのクラス動作を定義します。
//

#include "stdafx.h"
#include "EpgDataCap_Bon.h"
#include "EpgDataCap_BonDlg.h"
#include "../../Common/StackTrace.h"
#include "../../Common/ThreadUtil.h"
#include <objbase.h>
#include <shellapi.h>

namespace
{

FILE* g_debugLog;
recursive_mutex_ g_debugLogLock;

// 唯一の CEpgDataCap_BonApp オブジェクトです。

CEpgDataCap_BonApp theApp;

}

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
	dlg.SetIniMin(FALSE);
	dlg.SetIniView(TRUE);
	dlg.SetIniNW(TRUE);

	// コマンドオプションを解析
	int argc;
	LPWSTR *argv = CommandLineToArgvW(GetCommandLine(), &argc);
	if (argv != NULL) {
		LPCWSTR curr = L"";
		LPCWSTR optUpperD = NULL;
		LPCWSTR optLowerD = NULL;
		for (int i = 1; i < argc; i++) {
			if (argv[i][0] == L'-' || argv[i][0] == L'/') {
				curr = L"";
				if (wcscmp(argv[i] + 1, L"D") == 0 && optUpperD == NULL) {
					curr = argv[i] + 1;
					optUpperD = L"";
				} else if (wcscmp(argv[i] + 1, L"d") == 0 && optLowerD == NULL) {
					curr = argv[i] + 1;
					optLowerD = L"";
				} else if (CompareNoCase(argv[i] + 1, L"min") == 0) {
					dlg.SetIniMin(TRUE);
				} else if (CompareNoCase(argv[i] + 1, L"noview") == 0) {
					dlg.SetIniView(FALSE);
				} else if (CompareNoCase(argv[i] + 1, L"nonw") == 0) {
					dlg.SetIniNW(FALSE);
				} else if (CompareNoCase(argv[i] + 1, L"nwudp") == 0) {
					dlg.SetIniNWUDP(TRUE);
				} else if (CompareNoCase(argv[i] + 1, L"nwtcp") == 0) {
					dlg.SetIniNWTCP(TRUE);
				}
			} else if (wcscmp(curr, L"D") == 0 && optUpperD && optUpperD[0] == L'\0') {
				optUpperD = argv[i];
			} else if (wcscmp(curr, L"d") == 0 && optLowerD && optLowerD[0] == L'\0') {
				optLowerD = argv[i];
			}
		}
		if (optUpperD) {
			dlg.SetInitBon(optUpperD);
			AddDebugLogFormat(L"%ls", optUpperD);
		}
		// 原作の挙動に合わせるため
		if (optLowerD) {
			dlg.SetInitBon(optLowerD);
			AddDebugLogFormat(L"%ls", optLowerD);
		}
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
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	SetDllDirectory(L"");
	SetSaveDebugLog(GetPrivateProfileInt(L"SET", L"SaveDebugLog", 0, GetModuleIniPath().c_str()) != 0);
	//メインスレッドに対するCOMの初期化
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	theApp.InitInstance();
	CoUninitialize();
	SetSaveDebugLog(false);
	return 0;
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
		for( int i = 0; i < 100; i++ ){
			//パスに添え字をつけて書き込み可能な最初のものに記録する
			WCHAR logFileName[64];
			swprintf_s(logFileName, L"EpgDataCap_Bon_DebugLog-%d.txt", i);
			fs_path logPath = GetCommonIniPath().replace_filename(logFileName);
			g_debugLog = UtilOpenFile(logPath, UTIL_O_EXCL_CREAT_APPEND | UTIL_SH_READ);
			if( g_debugLog ){
				fwrite(L"\xFEFF", sizeof(WCHAR), 1, g_debugLog);
			}else{
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
