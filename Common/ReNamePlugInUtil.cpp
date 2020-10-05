#include "stdafx.h"
#include "ReNamePlugInUtil.h"
#ifndef _WIN32
#include "StringUtil.h"
#include <dlfcn.h>
#endif

#ifdef _WIN32
BOOL CReNamePlugInUtil::ShowSetting(
	const WCHAR* dllPath,
	HWND parentWnd
	)
{
	BOOL ret = FALSE;
	HMODULE hModule = LoadLibrary(dllPath);
	if( hModule ){
		SettingRNP pfnSetting = (SettingRNP)GetProcAddress(hModule, "Setting");
		if( pfnSetting ){
			pfnSetting(parentWnd);
			ret = TRUE;
		}
		FreeLibrary(hModule);
	}
	return ret;
}
#endif

BOOL CReNamePlugInUtil::Convert(
	PLUGIN_RESERVE_INFO* info,
	const WCHAR* dllPattern,
	const WCHAR* dllFolder,
	WCHAR* recName,
	DWORD* recNamesize
	)
{
	wstring pattern = dllPattern;
	wstring dllPath = dllFolder + pattern.substr(0, pattern.find('?'));
#ifdef _WIN32
	HMODULE hModule = LoadLibrary(dllPath.c_str());
	auto getProcAddr = [=](const char* name) { return GetProcAddress(hModule, name); };
#else
	string strPath;
	WtoUTF8(dllPath, strPath);
	void* hModule = dlopen(strPath.c_str(), RTLD_LAZY);
	auto getProcAddr = [=](const char* name) { return dlsym(hModule, name); };
#endif
	if( hModule == NULL ){
		AddDebugLogFormat(L"%lsのロードに失敗しました", dllPath.c_str());
		return FALSE;
	}
	CloseConvert();
	hModuleConvert = hModule;
	pattern.erase(0, pattern.find('?'));
	pattern.erase(0, 1);
	ConvertRecName3RNP pfnConvertRecName3 = (ConvertRecName3RNP)getProcAddr("ConvertRecName3");
	BOOL ret;
	if( pfnConvertRecName3 ){
		ret = pfnConvertRecName3(info, pattern.empty() ? NULL : pattern.c_str(), recName, recNamesize);
	}else{
		ConvertRecName2RNP pfnConvertRecName2 = (ConvertRecName2RNP)getProcAddr("ConvertRecName2");
		if( pfnConvertRecName2 ){
			ret = pfnConvertRecName2(info, info->epgInfo, recName, recNamesize);
		}else{
			ConvertRecNameRNP pfnConvertRecName = (ConvertRecNameRNP)getProcAddr("ConvertRecName");
			if( pfnConvertRecName ){
				ret = pfnConvertRecName(info, recName, recNamesize);
			}else{
				AddDebugLog(L"ConvertRecNameの GetProcAddress に失敗");
				ret = FALSE;
			}
		}
	}
	return ret;
}

void CReNamePlugInUtil::CloseConvert()
{
	if( hModuleConvert ){
#ifdef _WIN32
		FreeLibrary((HMODULE)hModuleConvert);
#else
		dlclose(hModuleConvert);
#endif
		hModuleConvert = NULL;
	}
}
