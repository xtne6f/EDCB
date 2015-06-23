#include "stdafx.h"
#include "PathUtil.h"
#include "StringUtil.h"
#include <shlobj.h>

void GetDefSettingPath(wstring& strPath)
{
/*	strPath = L"";

	WCHAR szPathM[_MAX_PATH];
	::SHGetFolderPath(NULL, CSIDL_PERSONAL|CSIDL_FLAG_CREATE, NULL, 0, szPathM);

	strPath = szPathM;
	strPath += SAVE_FOLDER;
	ChkFolderPath(strPath);
*/
	WCHAR strExePath[512] = L"";
	GetModuleFileName(NULL, strExePath, 512);

	WCHAR szPath[_MAX_PATH];	// パス
	WCHAR szDrive[_MAX_DRIVE];
	WCHAR szDir[_MAX_DIR];
	WCHAR szFname[_MAX_FNAME];
	WCHAR szExt[_MAX_EXT];
	_wsplitpath_s( strExePath, szDrive, _MAX_DRIVE, szDir, _MAX_DIR, szFname, _MAX_FNAME, szExt, _MAX_EXT );
	_wmakepath_s(  szPath, _MAX_PATH, szDrive, szDir, NULL, NULL );
	
	strPath = L"";
	strPath += szPath;
	strPath += L"Setting";
}

void GetSettingPath(wstring& strPath)
{
	strPath = L"";
	wstring strIni = L"";
	GetCommonIniPath(strIni);
	
	WCHAR wPath[512]=L"";
	GetPrivateProfileString( L"Set", L"DataSavePath", L"", wPath, 512, strIni.c_str() );
	strPath = wPath;
	if( strPath.empty() == true ){
		GetDefSettingPath(strPath);
	}
	ChkFolderPath(strPath);
}

void GetModuleFolderPath(wstring& strPath)
{
	WCHAR strExePath[512] = L"";
	GetModuleFileName(NULL, strExePath, 512);

	WCHAR szPath[_MAX_PATH];	// パス
	WCHAR szDrive[_MAX_DRIVE];
	WCHAR szDir[_MAX_DIR];
	WCHAR szFname[_MAX_FNAME];
	WCHAR szExt[_MAX_EXT];
	_wsplitpath_s( strExePath, szDrive, _MAX_DRIVE, szDir, _MAX_DIR, szFname, _MAX_FNAME, szExt, _MAX_EXT );
	_wmakepath_s(  szPath, _MAX_PATH, szDrive, szDir, NULL, NULL );
	int iLen = lstrlen(szPath);
	szPath[iLen-1] = '\0';
	
	strPath = L"";
	strPath += szPath;
	ChkFolderPath(strPath);
}

void GetModuleIniPath(wstring& strPath)
{
	WCHAR strExePath[512] = L"";
	GetModuleFileName(NULL, strExePath, 512);

	WCHAR szPath[_MAX_PATH];	// パス
	WCHAR szDrive[_MAX_DRIVE];
	WCHAR szDir[_MAX_DIR];
	WCHAR szFname[_MAX_FNAME];
	WCHAR szExt[_MAX_EXT];
	_wsplitpath_s( strExePath, szDrive, _MAX_DRIVE, szDir, _MAX_DIR, szFname, _MAX_FNAME, szExt, _MAX_EXT );
	_wmakepath_s(  szPath, _MAX_PATH, szDrive, szDir, NULL, NULL );
	
	strPath = L"";
	strPath += szPath;
	strPath += szFname;
	strPath += L".ini";
}

void GetCommonIniPath(wstring& strPath)
{
	WCHAR strExePath[512] = L"";
	GetModuleFileName(NULL, strExePath, 512);

	WCHAR szPath[_MAX_PATH];	// パス
	WCHAR szDrive[_MAX_DRIVE];
	WCHAR szDir[_MAX_DIR];
	WCHAR szFname[_MAX_FNAME];
	WCHAR szExt[_MAX_EXT];
	_wsplitpath_s( strExePath, szDrive, _MAX_DRIVE, szDir, _MAX_DIR, szFname, _MAX_FNAME, szExt, _MAX_EXT );
	_wmakepath_s(  szPath, _MAX_PATH, szDrive, szDir, NULL, NULL );
	
	strPath = L"";
	strPath += szPath;
	strPath += L"Common.ini";
}

void GetEpgTimerSrvIniPath(wstring& strPath)
{
	WCHAR strExePath[512] = L"";
	GetModuleFileName(NULL, strExePath, 512);

	WCHAR szPath[_MAX_PATH];	// パス
	WCHAR szDrive[_MAX_DRIVE];
	WCHAR szDir[_MAX_DIR];
	WCHAR szFname[_MAX_FNAME];
	WCHAR szExt[_MAX_EXT];
	_wsplitpath_s( strExePath, szDrive, _MAX_DRIVE, szDir, _MAX_DIR, szFname, _MAX_FNAME, szExt, _MAX_EXT );
	_wmakepath_s(  szPath, _MAX_PATH, szDrive, szDir, NULL, NULL );
	
	strPath = L"";
	strPath += szPath;
	strPath += L"EpgTimerSrv.ini";
}

void GetEpgSavePath(wstring& strPath)
{
	strPath = L"";
	GetSettingPath(strPath);
	strPath += EPG_SAVE_FOLDER;
}

void GetLogoSavePath(wstring& strPath)
{
	strPath = L"";
	GetSettingPath(strPath);
	strPath += LOGO_SAVE_FOLDER;
}

void GetRecFolderPath(wstring& strPath)
{
	strPath = L"";
	wstring strIni = L"";
	GetCommonIniPath(strIni);
	
	WCHAR wPath[512]=L"";
	GetPrivateProfileString( L"Set", L"RecFolderPath0", L"", wPath, 512, strIni.c_str() );
	strPath = wPath;
	if( strPath.empty() || GetPrivateProfileInt(L"SET", L"RecFolderNum", 0, strIni.c_str()) <= 0 ){
		GetSettingPath(strPath);
	}
	ChkFolderPath(strPath);
}

void GetFileTitle(wstring strPath, wstring& strTitle)
{
	WCHAR szDrive[_MAX_DRIVE];
	WCHAR szDir[_MAX_DIR];
	WCHAR szFname[_MAX_FNAME];
	WCHAR szExt[_MAX_EXT];
	_wsplitpath_s( strPath.c_str(), szDrive, _MAX_DRIVE, szDir, _MAX_DIR, szFname, _MAX_FNAME, szExt, _MAX_EXT );

	strTitle = szFname;
	return ;
}

void GetFileName(wstring strPath, wstring& strName)
{
	WCHAR strFileName[512] = L"";
	WCHAR szDrive[_MAX_DRIVE];
	WCHAR szDir[_MAX_DIR];
	WCHAR szFname[_MAX_FNAME];
	WCHAR szExt[_MAX_EXT];
	_wsplitpath_s( strPath.c_str(), szDrive, _MAX_DRIVE, szDir, _MAX_DIR, szFname, _MAX_FNAME, szExt, _MAX_EXT );
	_wmakepath_s( strFileName, _MAX_PATH, NULL, NULL, szFname, szExt );

	strName = strFileName;
	return ;
}

void GetFileExt(wstring strPath, wstring& strExt)
{
	WCHAR strFileName[512] = L"";
	WCHAR szDrive[_MAX_DRIVE];
	WCHAR szDir[_MAX_DIR];
	WCHAR szFname[_MAX_FNAME];
	WCHAR szExt[_MAX_EXT];
	_wsplitpath_s( strPath.c_str(), szDrive, _MAX_DRIVE, szDir, _MAX_DIR, szFname, _MAX_FNAME, szExt, _MAX_EXT );

	strExt = szExt;
	return ;
}

void GetFileFolder(wstring strPath, wstring& strFolder)
{
	WCHAR szPath[512] = L"";
	WCHAR szDrive[_MAX_DRIVE];
	WCHAR szDir[_MAX_DIR];
	WCHAR szFname[_MAX_FNAME];
	WCHAR szExt[_MAX_EXT];
	_wsplitpath_s( strPath.c_str(), szDrive, _MAX_DRIVE, szDir, _MAX_DIR, szFname, _MAX_FNAME, szExt, _MAX_EXT );
	_wmakepath_s( szPath, _MAX_PATH, szDrive, szDir, NULL, NULL );

	strFolder = szPath;
	ChkFolderPath(strFolder);
	return ;
}


BOOL IsExt(wstring filePath, wstring ext)
{
	if( filePath.empty() == true ){
		return FALSE;
	}
	WCHAR szDrive[_MAX_DRIVE];
	WCHAR szDir[_MAX_DIR];
	WCHAR szFname[_MAX_FNAME];
	WCHAR szExt[_MAX_EXT];
	_wsplitpath_s( filePath.c_str(), szDrive, _MAX_DRIVE, szDir, _MAX_DIR, szFname, _MAX_FNAME, szExt, _MAX_EXT );

	if( CompareNoCase( szExt, ext ) != 0 ){
		return FALSE;
	}

	return TRUE;
}

void CheckFileName(wstring& fileName, BOOL noChkYen)
{
	if( noChkYen == FALSE ){
		Replace(fileName, L"\\",L"￥");
	}
	Replace(fileName, L"/",L"／");
	Replace(fileName, L":",L"：");
	Replace(fileName, L"*",L"＊");
	Replace(fileName, L"?",L"？");
	Replace(fileName, L"\"",L"”");
	Replace(fileName, L"<",L"＜");
	Replace(fileName, L">",L"＞");
	Replace(fileName, L"|",L"｜");
}

void CheckFileName(string& fileName, BOOL noChkYen)
{
	if( noChkYen == FALSE ){
		Replace(fileName, "\\","￥");
	}
	Replace(fileName, "/","／");
	Replace(fileName, ":","：");
	Replace(fileName, "*","＊");
	Replace(fileName, "?","？");
	Replace(fileName, "\"","”");
	Replace(fileName, "<","＜");
	Replace(fileName, ">","＞");
	Replace(fileName, "|","｜");
}

