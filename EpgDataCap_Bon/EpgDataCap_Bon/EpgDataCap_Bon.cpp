
// EpgDataCap_Bon.cpp : アプリケーションのクラス動作を定義します。
//

#include "stdafx.h"
#include "EpgDataCap_Bon.h"
#include "EpgDataCap_BonDlg.h"

#include "../../Common/ThreadUtil.h"
#include <io.h>
#include <fcntl.h>
#include <share.h>
#include <sys/stat.h>

#ifndef SUPPRESS_OUTPUT_STACK_TRACE
#include <tlhelp32.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")
#endif

namespace
{

FILE* g_debugLog;
recursive_mutex_ g_debugLogLock;
bool g_saveDebugLog;

void StartDebugLog()
{
	if( GetPrivateProfileInt(L"SET", L"SaveDebugLog", 0, GetModuleIniPath().c_str()) != 0 ){
		for( int i = 0; i < 100; i++ ){
			//パスに添え字をつけて書き込み可能な最初のものに記録する
			WCHAR logFileName[64];
			swprintf_s(logFileName, L"EpgDataCap_Bon_DebugLog-%d.txt", i);
			fs_path logPath = GetModulePath().replace_filename(logFileName);
			//やりたいことは_wfsopen(L"abN",_SH_DENYWR)だが_wfsopenには"N"オプションがなさそうなので低水準で開く
			int fd;
			if( _wsopen_s(&fd, logPath.c_str(), _O_APPEND | _O_BINARY | _O_CREAT | _O_NOINHERIT | _O_WRONLY, _SH_DENYWR, _S_IWRITE) == 0 ){
				g_debugLog = _wfdopen(fd, L"ab");
				if( g_debugLog == NULL ){
					_close(fd);
				}
			}
			if( g_debugLog ){
				_fseeki64(g_debugLog, 0, SEEK_END);
				if( _ftelli64(g_debugLog) == 0 ){
					fputwc(L'\xFEFF', g_debugLog);
				}
				g_saveDebugLog = true;
				OutputDebugString(L"****** LOG START ******\r\n");
				break;
			}
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

#ifndef SUPPRESS_OUTPUT_STACK_TRACE
// 例外によってアプリケーションが終了する直前にスタックトレースを"実行ファイル名.exe.err"に出力する
// デバッグ情報(.pdbファイル)が存在すれば出力はより詳細になる

void OutputStackTrace(DWORD exceptionCode, const PVOID* addrOffsets)
{
	WCHAR path[MAX_PATH + 4];
	path[GetModuleFileName(NULL, path, MAX_PATH)] = L'\0';
	wcscat_s(path, L".err");
	HANDLE hFile = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if( hFile != INVALID_HANDLE_VALUE ){
		char buff[384];
		DWORD written;
		int len = sprintf_s(buff, "ExceptionCode = 0x%08X\r\n", exceptionCode);
		WriteFile(hFile, buff, len, &written, NULL);
		for( int i = 0; addrOffsets[i]; i++ ){
			SYMBOL_INFO symbol[1 + (256 + sizeof(SYMBOL_INFO)) / sizeof(SYMBOL_INFO)];
			symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
			symbol->MaxNameLen = 256;
			DWORD64 displacement;
			if( SymFromAddr(GetCurrentProcess(), (DWORD64)addrOffsets[i], &displacement, symbol) ){
				len = sprintf_s(buff, "Trace%02d 0x%p = 0x%p(%s) + 0x%X\r\n", i, addrOffsets[i], (PVOID)symbol->Address, symbol->Name, (DWORD)displacement);
			}else{
				len = sprintf_s(buff, "Trace%02d 0x%p = ?\r\n", i, addrOffsets[i]);
			}
			WriteFile(hFile, buff, len, &written, NULL);
		}
		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);
		if( hSnapshot != INVALID_HANDLE_VALUE ){
			MODULEENTRY32W modent;
			modent.dwSize = sizeof(modent);
			if( Module32FirstW(hSnapshot, &modent) ){
				do{
					char moduleA[256] = {};
					for( int i = 0; i == 0 || i < 255 && moduleA[i - 1]; i++ ){
						//文字化けしても構わない
						moduleA[i] = (char)modent.szModule[i];
					}
					len = sprintf_s(buff, "0x%p - 0x%p = %s\r\n", modent.modBaseAddr, modent.modBaseAddr + modent.modBaseSize - 1, moduleA);
					WriteFile(hFile, buff, len, &written, NULL);
				}while( Module32NextW(hSnapshot, &modent) );
			}
			CloseHandle(hSnapshot);
		}
		CloseHandle(hFile);
	}
}

LONG WINAPI TopLevelExceptionFilter(_EXCEPTION_POINTERS* exceptionInfo)
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
	SetUnhandledExceptionFilter(TopLevelExceptionFilter);
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
				} else if (_wcsicmp(argv[i] + 1, L"min") == 0) {
					dlg.SetIniMin(TRUE);
				} else if (_wcsicmp(argv[i] + 1, L"noview") == 0) {
					dlg.SetIniView(FALSE);
				} else if (_wcsicmp(argv[i] + 1, L"nonw") == 0) {
					dlg.SetIniNW(FALSE);
				} else if (_wcsicmp(argv[i] + 1, L"nwudp") == 0) {
					dlg.SetIniNWUDP(TRUE);
				} else if (_wcsicmp(argv[i] + 1, L"nwtcp") == 0) {
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
			OutputDebugString(optUpperD);
		}
		// 原作の挙動に合わせるため
		if (optLowerD) {
			dlg.SetInitBon(optLowerD);
			OutputDebugString(optLowerD);
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

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	SetDllDirectory(L"");
	StartDebugLog();
	//メインスレッドに対するCOMの初期化
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	theApp.InitInstance();
	CoUninitialize();
	StopDebugLog();
	return 0;
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
