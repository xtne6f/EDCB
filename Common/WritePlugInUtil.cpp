#include "stdafx.h"
#include "WritePlugInUtil.h"

#include "ErrDef.h"
#ifndef _WIN32
#include "StringUtil.h"
#include <dlfcn.h>
#endif

CWritePlugInUtil::CWritePlugInUtil(void)
{
	module = NULL;

	this->id = 0;
}

CWritePlugInUtil::~CWritePlugInUtil(void)
{
	UnInitialize();
}

BOOL CWritePlugInUtil::Initialize(
	LPCWSTR loadDllFilePath
	)
{
	if( module != NULL ){
		return FALSE;
	}

#ifdef _WIN32
	module = LoadLibrary(loadDllFilePath);
	auto getProcAddr = [=](const char* name) { return GetProcAddress((HMODULE)module, name); };
#else
	string strPath;
	WtoUTF8(loadDllFilePath, strPath);
	module = dlopen(strPath.c_str(), RTLD_LAZY);
	auto getProcAddr = [=](const char* name) { return dlsym(module, name); };
#endif

	if( module == NULL ){
		return FALSE;
	}

	CreateCtrlWP pfnCreateCtrlWP;
	if( (pfnCreateCtrlWP = (CreateCtrlWP)getProcAddr("CreateCtrl")) != NULL &&
	    (pfnGetPlugInNameWP = (GetPlugInNameWP)getProcAddr("GetPlugInName")) != NULL &&
	    (pfnSettingWP = (SettingWP)getProcAddr("Setting")) != NULL &&
	    (pfnDeleteCtrlWP = (DeleteCtrlWP)getProcAddr("DeleteCtrl")) != NULL &&
	    (pfnStartSaveWP = (StartSaveWP)getProcAddr("StartSave")) != NULL &&
	    (pfnStopSaveWP = (StopSaveWP)getProcAddr("StopSave")) != NULL &&
	    (pfnGetSaveFilePathWP = (GetSaveFilePathWP)getProcAddr("GetSaveFilePath")) != NULL &&
	    (pfnAddTSBuffWP = (AddTSBuffWP)getProcAddr("AddTSBuff")) != NULL ){
		if( pfnCreateCtrlWP(&this->id) ){
			return TRUE;
		}
		this->id = 0;
	}
#ifdef _WIN32
	FreeLibrary((HMODULE)module);
#else
	dlclose(module);
#endif
	module = NULL;
	return FALSE;
}

void CWritePlugInUtil::UnInitialize()
{
	if( module != NULL ){
		pfnDeleteCtrlWP(this->id);
		this->id = 0;
#ifdef _WIN32
		FreeLibrary((HMODULE)module);
#else
		dlclose(module);
#endif
	}
	module = NULL;
}

BOOL CWritePlugInUtil::GetName(
	WCHAR* name,
	DWORD* nameSize
	)
{
	if( module == NULL ){
		return ERR_NOT_INIT;
	}

	return pfnGetPlugInNameWP(name, nameSize);
}

void CWritePlugInUtil::ShowSetting(
	HWND parentWnd
	)
{
	if( module == NULL ){
		return ;
	}

	return pfnSettingWP(parentWnd);
}

BOOL CWritePlugInUtil::Start(
	LPCWSTR fileName,
	BOOL overWriteFlag,
	ULONGLONG createSize
	)
{
	if( module == NULL ){
		return ERR_NOT_INIT;
	}

	return pfnStartSaveWP(this->id, fileName, overWriteFlag, createSize);
}

BOOL CWritePlugInUtil::Stop(
	)
{
	if( module == NULL ){
		return ERR_NOT_INIT;
	}

	return pfnStopSaveWP(this->id);
}

BOOL CWritePlugInUtil::GetSavePath(
	WCHAR* filePath,
	DWORD* filePathSize
	)
{
	if( module == NULL ){
		return ERR_NOT_INIT;
	}

	return pfnGetSaveFilePathWP(this->id, filePath, filePathSize);
}

BOOL CWritePlugInUtil::Write(
	BYTE* data,
	DWORD size,
	DWORD* writeSize
	)
{
	if( module == NULL ){
		return ERR_NOT_INIT;
	}

	return pfnAddTSBuffWP(this->id, data, size, writeSize);
}

