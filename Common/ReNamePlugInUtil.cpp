#include "stdafx.h"
#include "ReNamePlugInUtil.h"

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

BOOL CReNamePlugInUtil::Convert(
	PLUGIN_RESERVE_INFO* info,
	const WCHAR* dllPattern,
	const WCHAR* dllFolder,
	WCHAR* recName,
	DWORD* recNamesize
	)
{
	wstring pattern = dllPattern;
	HMODULE hModule = LoadLibrary((dllFolder + pattern.substr(0, pattern.find('?'))).c_str());
	if( hModule == NULL ){
		OutputDebugString(L"dl‚Ìƒ[ƒh‚ÉŽ¸”s‚µ‚Ü‚µ‚½\r\n");
		return FALSE;
	}
	pattern.erase(0, pattern.find('?'));
	pattern.erase(0, 1);
	ConvertRecName3RNP pfnConvertRecName3 = (ConvertRecName3RNP)GetProcAddress(hModule, "ConvertRecName3");
	BOOL ret;
	if( pfnConvertRecName3 ){
		ret = pfnConvertRecName3(info, pattern.empty() ? NULL : pattern.c_str(), recName, recNamesize);
	}else{
		ConvertRecName2RNP pfnConvertRecName2 = (ConvertRecName2RNP)GetProcAddress(hModule, "ConvertRecName2");
		if( pfnConvertRecName2 ){
			ret = pfnConvertRecName2(info, info->epgInfo, recName, recNamesize);
		}else{
			ConvertRecNameRNP pfnConvertRecName = (ConvertRecNameRNP)GetProcAddress(hModule, "ConvertRecName");
			if( pfnConvertRecName ){
				ret = pfnConvertRecName(info, recName, recNamesize);
			}else{
				OutputDebugString(L"ConvertRecName‚Ì GetProcAddress ‚ÉŽ¸”s\r\n");
				ret = FALSE;
			}
		}
	}
	FreeLibrary(hModule);
	return ret;
}

