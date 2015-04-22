#ifndef __UTIL_H__
#define __UTIL_H__

// MFCで使う時用
//#define _MFC
#ifdef _MFC
#ifdef _DEBUG
#undef new
#endif
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#else
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#endif
using std::string;
using std::wstring;
using std::pair;
using std::map;
using std::multimap;
using std::vector;

#include <TCHAR.h>
#include <windows.h>


template<class T> inline void SAFE_DELETE(T*& p) { delete p; p = NULL; }
template<class T> inline void SAFE_DELETE_ARRAY(T*& p) { delete[] p; p = NULL; }

//セキュリティをNULLにして各種作成
HANDLE _CreateEvent(BOOL bManualReset, BOOL bInitialState, LPCTSTR lpName );
HANDLE _CreateMutex( BOOL bInitialOwner, LPCTSTR lpName );
HANDLE _CreateFileMapping( HANDLE hFile, DWORD flProtect, DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, LPCTSTR lpName );
HANDLE _CreateNamedPipe( LPCTSTR lpName, DWORD dwOpenMode, DWORD dwPipeMode, DWORD nMaxInstances, DWORD nOutBufferSize, DWORD nInBufferSize, DWORD nDefaultTimeOut );

HANDLE _CreateDirectoryAndFile( LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpsa, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile );
BOOL _CreateDirectory( LPCTSTR lpPathName );
//ボリュームのマウントを考慮して実ドライブの空きを取得する
BOOL _GetDiskFreeSpaceEx(
  LPCTSTR lpDirectoryName,                 // ディレクトリ名
  PULARGE_INTEGER lpFreeBytesAvailable,    // 呼び出し側が利用できるバイト数
  PULARGE_INTEGER lpTotalNumberOfBytes,    // ディスク全体のバイト数
  PULARGE_INTEGER lpTotalNumberOfFreeBytes // ディスク全体の空きバイト数
);
void _OutputDebugString(const TCHAR *pOutputString, ...);
void GetLastErrMsg(DWORD err, wstring& msg);

#endif
