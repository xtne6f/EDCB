#include "stdafx.h"
#include "PathUtil.h"
#include <wchar.h>

void GetDefSettingPath(wstring& strPath)
{
	GetModuleFolderPath(strPath);
	strPath += L"\\Setting";
}

void GetSettingPath(wstring& strPath)
{
	wstring strIni = L"";
	GetCommonIniPath(strIni);
	
	WCHAR wPath[MAX_PATH + 8];
	GetPrivateProfileString( L"Set", L"DataSavePath", L"", wPath, MAX_PATH + 8, strIni.c_str() );
	strPath = wPath;
	ChkFolderPath(strPath);
	if( strPath.size() >= MAX_PATH ){
		throw std::runtime_error("");
	}
	if( strPath.empty() == true ){
		GetDefSettingPath(strPath);
	}
}

void GetModuleFolderPath(wstring& strPath)
{
	WCHAR strExePath[MAX_PATH];
	DWORD len = GetModuleFileName(NULL, strExePath, MAX_PATH);
	if( len == 0 || len >= MAX_PATH ){
		throw std::runtime_error("");
	}

	WCHAR szPath[_MAX_DRIVE + _MAX_DIR + 8];
	WCHAR szDrive[_MAX_DRIVE];
	WCHAR szDir[_MAX_DIR];
	_wsplitpath_s( strExePath, szDrive, _MAX_DRIVE, szDir, _MAX_DIR, NULL, 0, NULL, 0 );
	_wmakepath_s(  szPath, szDrive, szDir, NULL, NULL );
	strPath = szPath;
	ChkFolderPath(strPath);
}

void GetModuleIniPath(wstring& strPath)
{
	WCHAR strExePath[MAX_PATH];
	DWORD len = GetModuleFileName(NULL, strExePath, MAX_PATH);
	if( len == 0 || len >= MAX_PATH ){
		throw std::runtime_error("");
	}

	WCHAR szPath[_MAX_DRIVE + _MAX_DIR + _MAX_FNAME + 4 + 8];
	WCHAR szDrive[_MAX_DRIVE];
	WCHAR szDir[_MAX_DIR];
	WCHAR szFname[_MAX_FNAME];
	_wsplitpath_s( strExePath, szDrive, _MAX_DRIVE, szDir, _MAX_DIR, szFname, _MAX_FNAME, NULL, 0 );
	_wmakepath_s(  szPath, szDrive, szDir, szFname, L".ini" );
	strPath = szPath;
}

void GetCommonIniPath(wstring& strPath)
{
	GetModuleFolderPath(strPath);
	strPath += L"\\Common.ini";
}

void GetEpgTimerSrvIniPath(wstring& strPath)
{
	GetModuleFolderPath(strPath);
	strPath += L"\\EpgTimerSrv.ini";
}

void GetRecFolderPath(wstring& strPath)
{
	wstring strIni = L"";
	GetCommonIniPath(strIni);
	
	WCHAR wPath[MAX_PATH + 8];
	GetPrivateProfileString( L"Set", L"RecFolderPath0", L"", wPath, MAX_PATH + 8, strIni.c_str() );
	strPath = wPath;
	ChkFolderPath(strPath);
	if( strPath.size() >= MAX_PATH ){
		throw std::runtime_error("");
	}
	if( strPath.empty() || GetPrivateProfileInt(L"SET", L"RecFolderNum", 0, strIni.c_str()) <= 0 ){
		GetSettingPath(strPath);
	}
}

void GetFileTitle(const wstring& strPath, wstring& strTitle)
{
	WCHAR szFname[_MAX_FNAME];
	_wsplitpath_s( strPath.c_str(), NULL, 0, NULL, 0, szFname, _MAX_FNAME, NULL, 0 );

	strTitle = szFname;
	return ;
}

void GetFileName(const wstring& strPath, wstring& strName)
{
	WCHAR strFileName[_MAX_FNAME + _MAX_EXT + 8];
	WCHAR szFname[_MAX_FNAME];
	WCHAR szExt[_MAX_EXT];
	_wsplitpath_s( strPath.c_str(), NULL, 0, NULL, 0, szFname, _MAX_FNAME, szExt, _MAX_EXT );
	_wmakepath_s( strFileName, NULL, NULL, szFname, szExt );

	strName = strFileName;
	return ;
}

void GetFileExt(const wstring& strPath, wstring& strExt)
{
	WCHAR szExt[_MAX_EXT];
	_wsplitpath_s( strPath.c_str(), NULL, 0, NULL, 0, NULL, 0, szExt, _MAX_EXT );

	strExt = szExt;
	return ;
}

void GetFileFolder(const wstring& strPath, wstring& strFolder)
{
	WCHAR szPath[_MAX_DRIVE + _MAX_DIR + 8];
	WCHAR szDrive[_MAX_DRIVE];
	WCHAR szDir[_MAX_DIR];
	_wsplitpath_s( strPath.c_str(), szDrive, _MAX_DRIVE, szDir, _MAX_DIR, NULL, 0, NULL, 0 );
	_wmakepath_s( szPath, szDrive, szDir, NULL, NULL );

	strFolder = szPath;
	ChkFolderPath(strFolder);
	return ;
}


BOOL IsExt(const WCHAR* filePath, const WCHAR* ext)
{
	WCHAR szExt[_MAX_EXT];
	_wsplitpath_s( filePath, NULL, 0, NULL, 0, NULL, 0, szExt, _MAX_EXT );

	if( _wcsicmp( szExt, ext ) != 0 ){
		return FALSE;
	}

	return TRUE;
}

void CheckFileName(wstring& fileName, BOOL noChkYen)
{
	const WCHAR s[] = { L'/', L':', L'*', L'?', L'"', L'<', L'>', L'|', (noChkYen ? L'\0' : L'\\'), L'\0' };
	const WCHAR r[] = { L'Å^', L'ÅF', L'Åñ', L'ÅH', L'Åh', L'ÅÉ', L'ÅÑ', L'Åb', L'Åè', L'\0' };
	for( size_t i = 0; i < fileName.size(); i++ ){
		const WCHAR* p = wcschr(s, fileName[i]);
		if( p ){
			fileName[i] = r[p - s];
		}
	}
}

void ChkFolderPath(wstring& strPath)
{
	if( strPath.empty() == false && strPath.back() == L'\\' ){
		strPath.pop_back();
	}
}

