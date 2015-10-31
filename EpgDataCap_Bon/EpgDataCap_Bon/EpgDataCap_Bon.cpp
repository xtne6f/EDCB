
// EpgDataCap_Bon.cpp : アプリケーションのクラス動作を定義します。
//

#include "stdafx.h"
#include "EpgDataCap_Bon.h"
#include "EpgDataCap_BonDlg.h"

#include "CmdLineUtil.h"
#include "../../Common/BlockLock.h"

static HANDLE g_hDebugLog;
static CRITICAL_SECTION g_debugLogLock;
static bool g_saveDebugLog;

static void StartDebugLog()
{
	wstring iniPath;
	GetModuleIniPath(iniPath);
	if( GetPrivateProfileInt(L"SET", L"SaveDebugLog", 0, iniPath.c_str()) != 0 ){
		wstring logFolder;
		GetModuleFolderPath(logFolder);
		for( int i = 0; i < 100; i++ ){
			//パスに添え字をつけて書き込み可能な最初のものに記録する
			WCHAR logFileName[64];
			wsprintf(logFileName, L"\\EpgDataCap_Bon_DebugLog-%d.txt", i);
			g_hDebugLog = CreateFile((logFolder + logFileName).c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if( g_hDebugLog != INVALID_HANDLE_VALUE ){
				if( GetLastError() == ERROR_SUCCESS ){
					DWORD dwWritten;
					WriteFile(g_hDebugLog, "\xFF\xFE", sizeof(char) * 2, &dwWritten, NULL);
				}else{
					LARGE_INTEGER liPos = {};
					SetFilePointerEx(g_hDebugLog, liPos, NULL, FILE_END);
				}
				InitializeCriticalSection(&g_debugLogLock);
				g_saveDebugLog = true;
				OutputDebugString(L"****** LOG START ******\r\n");
				break;
			}
		}
	}
}

static void StopDebugLog()
{
	if( g_saveDebugLog ){
		OutputDebugString(L"****** LOG STOP ******\r\n");
		g_saveDebugLog = false;
		DeleteCriticalSection(&g_debugLogLock);
		CloseHandle(g_hDebugLog);
	}
}

#ifndef SUPPRESS_OUTPUT_STACK_TRACE
// 例外によってアプリケーションが終了する直前にスタックトレースを"実行ファイル名.exe.err"に出力する
// デバッグ情報(.pdbファイル)が存在すれば出力はより詳細になる

#include <tlhelp32.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")

static void OutputStackTrace(DWORD exceptionCode, const PVOID* addrOffsets)
{
	WCHAR path[MAX_PATH + 4];
	path[GetModuleFileName(NULL, path, MAX_PATH)] = L'\0';
	lstrcat(path, L".err");
	HANDLE hFile = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if( hFile != INVALID_HANDLE_VALUE ){
		char buff[384];
		DWORD written;
		int len = wsprintfA(buff, "ExceptionCode = 0x%08X\r\n", exceptionCode);
		WriteFile(hFile, buff, len, &written, NULL);
		for( int i = 0; addrOffsets[i]; i++ ){
			SYMBOL_INFO symbol[1 + (256 + sizeof(SYMBOL_INFO)) / sizeof(SYMBOL_INFO)];
			symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
			symbol->MaxNameLen = 256;
			DWORD64 displacement;
			if( SymFromAddr(GetCurrentProcess(), (DWORD64)addrOffsets[i], &displacement, symbol) ){
				len = wsprintfA(buff, "Trace%02d 0x%p = 0x%p(%s) + 0x%X\r\n", i, addrOffsets[i], (PVOID)symbol->Address, symbol->Name, (DWORD)displacement);
			}else{
				len = wsprintfA(buff, "Trace%02d 0x%p = ?\r\n", i, addrOffsets[i]);
			}
			WriteFile(hFile, buff, len, &written, NULL);
		}
		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);
		if( hSnapshot != INVALID_HANDLE_VALUE ){
			MODULEENTRY32W modent;
			modent.dwSize = sizeof(modent);
			if( Module32FirstW(hSnapshot, &modent) ){
				do{
					len = wsprintfA(buff, "0x%p - 0x%p = %S\r\n", modent.modBaseAddr, modent.modBaseAddr + modent.modBaseSize - 1, modent.szModule);
					WriteFile(hFile, buff, len, &written, NULL);
				}while( Module32NextW(hSnapshot, &modent) );
			}
			CloseHandle(hSnapshot);
		}
		CloseHandle(hFile);
	}
}

static LONG WINAPI TopLevelExceptionFilter(_EXCEPTION_POINTERS* exceptionInfo)
{
	static struct {
		LONG used;
		CONTEXT contextRecord;
		STACKFRAME64 stackFrame;
		PVOID addrOffsets[32];
	} work;

	if( InterlockedExchange(&work.used, 1) == 0 ){
		SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
		if( SymInitialize(GetCurrentProcess(), NULL, TRUE) ){
			work.addrOffsets[0] = exceptionInfo->ExceptionRecord->ExceptionAddress;
			work.contextRecord = *exceptionInfo->ContextRecord;
			work.stackFrame.AddrPC.Mode = AddrModeFlat;
			work.stackFrame.AddrFrame.Mode = AddrModeFlat;
			work.stackFrame.AddrStack.Mode = AddrModeFlat;
#if defined(_M_IX86) || defined(_M_X64)
#ifdef _M_X64
			work.stackFrame.AddrPC.Offset = work.contextRecord.Rip;
			work.stackFrame.AddrFrame.Offset = work.contextRecord.Rbp;
			work.stackFrame.AddrStack.Offset = work.contextRecord.Rsp;
#else
			work.stackFrame.AddrPC.Offset = work.contextRecord.Eip;
			work.stackFrame.AddrFrame.Offset = work.contextRecord.Ebp;
			work.stackFrame.AddrStack.Offset = work.contextRecord.Esp;
#endif
			for( int i = 1; i < _countof(work.addrOffsets) - 1 && StackWalk64(
#ifdef _M_X64
				IMAGE_FILE_MACHINE_AMD64,
#else
				IMAGE_FILE_MACHINE_I386,
#endif
				GetCurrentProcess(), GetCurrentThread(), &work.stackFrame, &work.contextRecord,
				NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL); i++ ){
				work.addrOffsets[i] = (PVOID)work.stackFrame.AddrPC.Offset;
			}
#endif
			OutputStackTrace(exceptionInfo->ExceptionRecord->ExceptionCode, work.addrOffsets);
			SymCleanup(GetCurrentProcess());
		}
	}
	return EXCEPTION_CONTINUE_SEARCH;
}

#endif // SUPPRESS_OUTPUT_STACK_TRACE


// CEpgDataCap_BonApp

// CEpgDataCap_BonApp コンストラクション

CEpgDataCap_BonApp::CEpgDataCap_BonApp()
{
	// TODO: この位置に構築用コードを追加してください。
	// ここに InitInstance 中の重要な初期化処理をすべて記述してください。
}


// 唯一の CEpgDataCap_BonApp オブジェクトです。

CEpgDataCap_BonApp theApp;


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
	SetUnhandledExceptionFilter(TopLevelExceptionFilter);
#endif

	// コマンドオプションを解析
	CCmdLineUtil cCmdUtil;
	int argc;
	LPWSTR *argv = CommandLineToArgvW(GetCommandLine(), &argc);
	if (argv != NULL) {
		for (int i = 1; i < argc; i++) {
			BOOL bFlag = argv[i][0] == L'-' || argv[i][0] == L'/' ? TRUE : FALSE;
			cCmdUtil.ParseParam(&argv[i][bFlag ? 1 : 0], bFlag, i == argc - 1 ? TRUE : FALSE);
		}
		LocalFree(argv);
	}

	CEpgDataCap_BonDlg dlg;

	map<wstring, wstring>::iterator itr;
	dlg.SetIniMin(FALSE);
	dlg.SetIniView(TRUE);
	dlg.SetIniNW(TRUE);
	for( itr = cCmdUtil.m_CmdList.begin(); itr != cCmdUtil.m_CmdList.end(); itr++ ){
		if( lstrcmpi(itr->first.c_str(), L"d") == 0 ){
			dlg.SetInitBon(itr->second.c_str());
			OutputDebugString(itr->second.c_str());
		}else if( lstrcmpi(itr->first.c_str(), L"min") == 0 ){
			dlg.SetIniMin(TRUE);
		}else if( lstrcmpi(itr->first.c_str(), L"noview") == 0 ){
			dlg.SetIniView(FALSE);
		}else if( lstrcmpi(itr->first.c_str(), L"nonw") == 0 ){
			dlg.SetIniNW(FALSE);
		}else if( lstrcmpi(itr->first.c_str(), L"nwudp") == 0 ){
			dlg.SetIniNWUDP(TRUE);
		}else if( lstrcmpi(itr->first.c_str(), L"nwtcp") == 0 ){
			dlg.SetIniNWTCP(TRUE);
		}
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

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	StartDebugLog();
	//メインスレッドに対するCOMの初期化
	CoInitialize(NULL);
	theApp.InitInstance();
	CoUninitialize();
	StopDebugLog();
	return 0;
}

BOOL WritePrivateProfileInt(LPCTSTR lpAppName, LPCTSTR lpKeyName, int value, LPCTSTR lpFileName)
{
	TCHAR sz[32];
	wsprintf(sz, TEXT("%d"), value);
	return WritePrivateProfileString(lpAppName, lpKeyName, sz, lpFileName);
}

void OutputDebugStringWrapper(LPCWSTR lpOutputString)
{
	if( g_saveDebugLog ){
		//デバッグ出力ログ保存
		CBlockLock lock(&g_debugLogLock);
		SYSTEMTIME st;
		GetLocalTime(&st);
		WCHAR header[64];
		int len = wsprintf(header, L"[%02d%02d%02d%02d%02d%02d.%03d] ",
		                   st.wYear % 100, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
		DWORD dwWritten;
		WriteFile(g_hDebugLog, header, sizeof(WCHAR) * len, &dwWritten, NULL);
		if( lpOutputString ){
			len = lstrlen(lpOutputString);
			WriteFile(g_hDebugLog, lpOutputString, sizeof(WCHAR) * len, &dwWritten, NULL);
			if( len == 0 || lpOutputString[len - 1] != L'\n' ){
				WriteFile(g_hDebugLog, L"<NOBR>\r\n", sizeof(WCHAR) * 8, &dwWritten, NULL);
			}
		}
	}
	OutputDebugStringW(lpOutputString);
}
