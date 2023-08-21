#include "stdafx.h"
#include "ReNamePlugInUtil.h"
#include "PathUtil.h"

CReNamePlugInUtil::CReNamePlugInUtil()
	: m_moduleHolder(NULL, UtilFreeLibrary)
{
}

#ifdef _WIN32
BOOL CReNamePlugInUtil::ShowSetting(
	const wstring& dllPath,
	HWND parentWnd
	)
{
	std::unique_ptr<void, decltype(&UtilFreeLibrary)> module(UtilLoadLibrary(dllPath), UtilFreeLibrary);
	SettingRNP pfnSetting;
	if( UtilGetProcAddress(module.get(), "Setting", pfnSetting) ){
		pfnSetting(parentWnd);
		return TRUE;
	}
	return FALSE;
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
	void* hModule = UtilLoadLibrary(dllPath);
	if( hModule == NULL ){
		AddDebugLogFormat(L"%lsのロードに失敗しました", dllPath.c_str());
		return FALSE;
	}
	m_moduleHolder.reset(hModule);
	pattern.erase(0, pattern.find('?'));
	pattern.erase(0, 1);
	ConvertRecName3RNP pfnConvertRecName3;
	BOOL ret;
	if( UtilGetProcAddress(hModule, "ConvertRecName3", pfnConvertRecName3) ){
		ret = pfnConvertRecName3(info, pattern.empty() ? NULL : pattern.c_str(), recName, recNamesize);
	}else{
		ConvertRecName2RNP pfnConvertRecName2;
		if( UtilGetProcAddress(hModule, "ConvertRecName2", pfnConvertRecName2) ){
			ret = pfnConvertRecName2(info, info->epgInfo, recName, recNamesize);
		}else{
			ConvertRecNameRNP pfnConvertRecName;
			if( UtilGetProcAddress(hModule, "ConvertRecName", pfnConvertRecName) ){
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
	m_moduleHolder.reset();
}
