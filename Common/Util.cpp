#include "stdafx.h"
#include "Util.h"
#include <aclapi.h>

#pragma comment(lib, "advapi32.lib")

HANDLE _CreateEvent(BOOL bManualReset, BOOL bInitialState, LPCTSTR lpName)
{
	SECURITY_DESCRIPTOR sd;
	SECURITY_ATTRIBUTES sa;
	 
	memset(&sd,0,sizeof(sd));
	InitializeSecurityDescriptor(&sd,SECURITY_DESCRIPTOR_REVISION);
	SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
	memset(&sa,0,sizeof(sa));
	sa.nLength=sizeof(sa);
	sa.lpSecurityDescriptor=&sd;

	return ::CreateEvent(&sa, bManualReset, bInitialState, lpName);
}

HANDLE _CreateMutex( BOOL bInitialOwner, LPCTSTR lpName )
{
	SECURITY_DESCRIPTOR sd;
	SECURITY_ATTRIBUTES sa;
	 
	memset(&sd,0,sizeof(sd));
	InitializeSecurityDescriptor(&sd,SECURITY_DESCRIPTOR_REVISION);
	SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
	memset(&sa,0,sizeof(sa));
	sa.nLength=sizeof(sa);
	sa.lpSecurityDescriptor=&sd;

	return ::CreateMutex( &sa, bInitialOwner, lpName );
}

HANDLE _CreateFileMapping( HANDLE hFile, DWORD flProtect, DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, LPCTSTR lpName)
{
	SECURITY_DESCRIPTOR sd;
	SECURITY_ATTRIBUTES sa;
	 
	memset(&sd,0,sizeof(sd));
	InitializeSecurityDescriptor(&sd,SECURITY_DESCRIPTOR_REVISION);
	SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
	memset(&sa,0,sizeof(sa));
	sa.nLength=sizeof(sa);
	sa.lpSecurityDescriptor=&sd;

	return ::CreateFileMapping( hFile, &sa, flProtect, dwMaximumSizeHigh, dwMaximumSizeLow, lpName);
}

HANDLE _CreateNamedPipe( LPCTSTR lpName, DWORD dwOpenMode, DWORD dwPipeMode, DWORD nMaxInstances, DWORD nOutBufferSize, DWORD nInBufferSize, DWORD nDefaultTimeOut )
{
	SECURITY_DESCRIPTOR sd;
	SECURITY_ATTRIBUTES sa;
	 
	memset(&sd,0,sizeof(sd));
	InitializeSecurityDescriptor(&sd,SECURITY_DESCRIPTOR_REVISION);
	SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
	memset(&sa,0,sizeof(sa));
	sa.nLength=sizeof(sa);
	sa.lpSecurityDescriptor=&sd;

	return ::CreateNamedPipe( lpName, dwOpenMode, dwPipeMode, nMaxInstances, nOutBufferSize, nInBufferSize, nDefaultTimeOut, &sa );
}

BOOL _CreateDirectory( LPCTSTR lpPathName )
{
/*
	SECURITY_DESCRIPTOR sd;
	SECURITY_ATTRIBUTES sa;
	 
	memset(&sd,0,sizeof(sd));
	InitializeSecurityDescriptor(&sd,SECURITY_DESCRIPTOR_REVISION);
	SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
	memset(&sa,0,sizeof(sa));
	sa.nLength=sizeof(sa);
	sa.lpSecurityDescriptor=&sd;

	PACL                pDacl;
	EXPLICIT_ACCESS     explicitAccess[3];
	SECURITY_ATTRIBUTES sa;
	SECURITY_DESCRIPTOR sd;

	InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);

	BuildExplicitAccessWithName(&explicitAccess[0], TEXT("SYSTEM"), FILE_ALL_ACCESS, GRANT_ACCESS, 0);
	BuildExplicitAccessWithName(&explicitAccess[1], TEXT("Administrators"), FILE_ALL_ACCESS, GRANT_ACCESS, 0);
	BuildExplicitAccessWithName(&explicitAccess[2], TEXT("Everyone"), FILE_ALL_ACCESS, GRANT_ACCESS, 0);
	SetEntriesInAcl(3, explicitAccess, NULL, &pDacl);

	SetSecurityDescriptorDacl(&sd, TRUE, pDacl, FALSE);

	sa.nLength              = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = &sd;
	sa.bInheritHandle       = FALSE;
*/
	BOOL bRet = FALSE;
	if( _tcslen(lpPathName) > 2 ){
		TCHAR szCreatePath[MAX_PATH+1] = _T("");
		szCreatePath[0] = lpPathName[0];
		szCreatePath[1] = lpPathName[1];
		
		for (int i = 2; i < (int)_tcslen(lpPathName); i++) {
			szCreatePath[i] = lpPathName[i];
			if (szCreatePath[i] == '\\') {
				szCreatePath[i+1] = '\0';
				if ( GetFileAttributes(szCreatePath) == 0xFFFFFFFF ) {
					bRet = ::CreateDirectory( szCreatePath, NULL );
				}
			}
		}
		if ( GetFileAttributes(szCreatePath) == 0xFFFFFFFF ) {
			bRet = ::CreateDirectory( szCreatePath, NULL );
		}
	}

	return bRet;
}

HANDLE _CreateFile( LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile )
{
	SECURITY_DESCRIPTOR sd;
	SECURITY_ATTRIBUTES sa;
	 
	memset(&sd,0,sizeof(sd));
	InitializeSecurityDescriptor(&sd,SECURITY_DESCRIPTOR_REVISION);
	SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
	memset(&sa,0,sizeof(sa));
	sa.nLength=sizeof(sa);
	sa.lpSecurityDescriptor=&sd;
/*
	PACL                pDacl;
	EXPLICIT_ACCESS     explicitAccess[3];
	SECURITY_ATTRIBUTES sa;
	SECURITY_DESCRIPTOR sd;

	InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);

	BuildExplicitAccessWithName(&explicitAccess[0], TEXT("SYSTEM"), FILE_ALL_ACCESS, GRANT_ACCESS, 0);
	BuildExplicitAccessWithName(&explicitAccess[1], TEXT("Administrators"), FILE_ALL_ACCESS, GRANT_ACCESS, 0);
	BuildExplicitAccessWithName(&explicitAccess[2], TEXT("Everyone"), FILE_ALL_ACCESS, GRANT_ACCESS, 0);
	SetEntriesInAcl(3, explicitAccess, NULL, &pDacl);

	SetSecurityDescriptorDacl(&sd, TRUE, pDacl, FALSE);

	sa.nLength              = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = &sd;
	sa.bInheritHandle       = FALSE;
*/
	return ::CreateFile( lpFileName, dwDesiredAccess, dwShareMode, &sa, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile );
}

HANDLE _CreateDirectoryAndFile( LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpsa, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile )
{
	HANDLE hFile =  ::CreateFile( lpFileName, dwDesiredAccess, dwShareMode, lpsa, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile );
	if( hFile == INVALID_HANDLE_VALUE ){
		TCHAR* p = (TCHAR*)_tcsrchr(lpFileName, '\\');
		TCHAR* szDirPath = NULL;
		if( p != NULL ){
			int iSize = (int)(p - lpFileName);
			szDirPath = new TCHAR[iSize+1];
			_tcsncpy_s(szDirPath, iSize+1, lpFileName, iSize);
		}
		if( szDirPath != NULL ){
			_CreateDirectory(szDirPath);
			delete[] szDirPath;
			hFile =  ::CreateFile( lpFileName, dwDesiredAccess, dwShareMode, lpsa, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile );
		}
	}
	return hFile;
}

BOOL _GetDiskFreeSpaceEx(
  LPCTSTR lpDirectoryName,                 // ディレクトリ名
  PULARGE_INTEGER lpFreeBytesAvailable,    // 呼び出し側が利用できるバイト数
  PULARGE_INTEGER lpTotalNumberOfBytes,    // ディスク全体のバイト数
  PULARGE_INTEGER lpTotalNumberOfFreeBytes // ディスク全体の空きバイト数
)
{
	if( lpFreeBytesAvailable == NULL ||
		lpTotalNumberOfBytes == NULL ||
		lpTotalNumberOfFreeBytes == NULL 
		){
		return FALSE;
	}
	TCHAR szVolumePathName[MAX_PATH] = _T("");
	if( GetVolumePathName( lpDirectoryName, szVolumePathName, MAX_PATH) == FALSE ){
		return GetDiskFreeSpaceEx( lpDirectoryName, lpFreeBytesAvailable, lpTotalNumberOfBytes, lpTotalNumberOfFreeBytes );
	}
	TCHAR szMount[MAX_PATH] = _T("");
	if( GetVolumeNameForVolumeMountPoint(szVolumePathName, szMount, MAX_PATH) == FALSE ){
		return GetDiskFreeSpaceEx( szVolumePathName, lpFreeBytesAvailable, lpTotalNumberOfBytes, lpTotalNumberOfFreeBytes );
	}
	return GetDiskFreeSpaceEx( szMount, lpFreeBytesAvailable, lpTotalNumberOfBytes, lpTotalNumberOfFreeBytes );
}

void _OutputDebugString(const TCHAR *format, ...)
{
	va_list params;

	va_start(params, format);
	int iResult;
	TCHAR *buff;
	int length = _vsctprintf(format, params);
	buff = new TCHAR [length + 1];
	iResult = _vstprintf_s(buff, length + 1, format, params);
	buff[length] = '\0';
	if (buff != NULL) {
		OutputDebugString(buff);
		delete[] buff;
	}

	va_end(params);
}

void GetLastErrMsg(DWORD err, wstring& msg)
{
	LPVOID lpMsgBuf;
	FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		err,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR)&lpMsgBuf,
		0,
		NULL );
	msg = (LPWSTR)lpMsgBuf;
	LocalFree( lpMsgBuf );
}
