#ifndef __PATH_UTIL_H__
#define __PATH_UTIL_H__

void GetDefSettingPath(wstring& strPath);

void GetSettingPath(wstring& strPath);

void GetModuleFolderPath(wstring& strPath);

void GetModuleIniPath(wstring& strPath);

void GetCommonIniPath(wstring& strPath);

void GetEpgTimerSrvIniPath(wstring& strPath);

void GetRecFolderPath(wstring& strPath);

void GetFileTitle(const wstring& strPath, wstring& strTitle);

void GetFileName(const wstring& strPath, wstring& strName);

void GetFileExt(const wstring& strPath, wstring& strExt);

void GetFileFolder(const wstring& strPath, wstring& strFolder);

BOOL IsExt(const WCHAR* filePath, const WCHAR* ext);

void CheckFileName(wstring& fileName, BOOL noChkYen = FALSE);

void ChkFolderPath(wstring& strPath);

#endif
