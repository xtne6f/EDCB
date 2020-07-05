#include "stdafx.h"

#ifndef SUPPRESS_OUTPUT_STACK_TRACE
#include <tlhelp32.h>
#include <dbghelp.h>

namespace
{
WCHAR g_path[MAX_PATH];

struct {
	LONG used;
	CONTEXT contextRecord;
	STACKFRAME64 stackFrame;
	PVOID addrOffsets[32];
} g_work;

void OutputStackTrace(DWORD exceptionCode, const PVOID* addrOffsets)
{
	HANDLE hFile = CreateFile(g_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
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
					for( size_t i = 0; i == 0 || (i < 255 && moduleA[i - 1]); i++ ){
						// 文字化けしても構わない
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
	if( InterlockedExchange(&g_work.used, 1) == 0 ){
		SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
		if( SymInitialize(GetCurrentProcess(), NULL, TRUE) ){
			g_work.addrOffsets[0] = exceptionInfo->ExceptionRecord->ExceptionAddress;
			g_work.contextRecord = *exceptionInfo->ContextRecord;
			g_work.stackFrame.AddrPC.Mode = AddrModeFlat;
			g_work.stackFrame.AddrFrame.Mode = AddrModeFlat;
			g_work.stackFrame.AddrStack.Mode = AddrModeFlat;
#if defined(_M_IX86) || defined(_M_X64)
#ifdef _M_X64
			g_work.stackFrame.AddrPC.Offset = g_work.contextRecord.Rip;
			g_work.stackFrame.AddrFrame.Offset = g_work.contextRecord.Rbp;
			g_work.stackFrame.AddrStack.Offset = g_work.contextRecord.Rsp;
#else
			g_work.stackFrame.AddrPC.Offset = g_work.contextRecord.Eip;
			g_work.stackFrame.AddrFrame.Offset = g_work.contextRecord.Ebp;
			g_work.stackFrame.AddrStack.Offset = g_work.contextRecord.Esp;
#endif
			for( size_t i = 1; i < array_size(g_work.addrOffsets) - 1 && StackWalk64(
#ifdef _M_X64
				IMAGE_FILE_MACHINE_AMD64,
#else
				IMAGE_FILE_MACHINE_I386,
#endif
				GetCurrentProcess(), GetCurrentThread(), &g_work.stackFrame, &g_work.contextRecord,
				NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL); i++ ){
				g_work.addrOffsets[i] = (PVOID)g_work.stackFrame.AddrPC.Offset;
			}
#endif
			OutputStackTrace(exceptionInfo->ExceptionRecord->ExceptionCode, g_work.addrOffsets);
			SymCleanup(GetCurrentProcess());
		}
	}
	return EXCEPTION_CONTINUE_SEARCH;
}
}

void SetOutputStackTraceOnUnhandledException(LPCWSTR pathOrNull)
{
	if( pathOrNull == NULL ){
		SetUnhandledExceptionFilter(NULL);
	}else if( wcslen(pathOrNull) < MAX_PATH ){
		wcscpy_s(g_path, pathOrNull);
		SetUnhandledExceptionFilter(TopLevelExceptionFilter);
	}
}
#endif // SUPPRESS_OUTPUT_STACK_TRACE
