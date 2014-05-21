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
using namespace std;

#include <TCHAR.h>
#include <windows.h>


#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }

//セキュリティをNULLにして各種作成
HANDLE _CreateEvent(BOOL bManualReset, BOOL bInitialState, LPCTSTR lpName );
HANDLE _CreateFile( LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile );
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


namespace nocase {
template<class T> struct less : public std::less<T> {};
template<> struct less<std::string> : public std::binary_function<std::string,std::string,bool> {
	bool operator()( const std::string&	lhs, const std::string&	rhs ) const {
		return	( &lhs[0] == &rhs[0] ) ? false : ( _stricmp( &lhs[0], &rhs[0] ) < 0 );
	}
};
template<> struct less<std::wstring> : public std::binary_function<std::wstring,std::wstring,bool> {
	bool operator()( const std::wstring&	lhs, const std::wstring&	rhs ) const {
		return	( &lhs[0] == &rhs[0] ) ? false : ( _wcsicmp( &lhs[0], &rhs[0] ) < 0 );
	}
};
template<class _key_type, class _value_type, class _less_type = less<_key_type>, class _allocator_type = std::allocator<std::pair<const _key_type, _value_type> > > struct map : public std::map<_key_type,_value_type,_less_type,_allocator_type> {};
}

#endif
