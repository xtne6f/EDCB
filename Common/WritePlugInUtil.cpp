#include "stdafx.h"
#include "WritePlugInUtil.h"

#include "ErrDef.h"
#include "PathUtil.h"

CWritePlugInUtil::CWritePlugInUtil(void)
	: module(NULL, UtilFreeLibrary)
{
	this->id = 0;
}

CWritePlugInUtil::~CWritePlugInUtil(void)
{
	UnInitialize();
}

BOOL CWritePlugInUtil::Initialize(
	const wstring& loadDllFilePath
	)
{
	if( module != NULL ){
		return FALSE;
	}

	module.reset(UtilLoadLibrary(loadDllFilePath));

	if( module == NULL ){
		return FALSE;
	}

	CreateCtrlWP pfnCreateCtrlWP;
	if( UtilGetProcAddress(module.get(), "CreateCtrl", pfnCreateCtrlWP) &&
	    UtilGetProcAddress(module.get(), "DeleteCtrl", pfnDeleteCtrlWP) &&
	    UtilGetProcAddress(module.get(), "StartSave", pfnStartSaveWP) &&
	    UtilGetProcAddress(module.get(), "StopSave", pfnStopSaveWP) &&
	    UtilGetProcAddress(module.get(), "GetSaveFilePath", pfnGetSaveFilePathWP) &&
	    UtilGetProcAddress(module.get(), "AddTSBuff", pfnAddTSBuffWP) ){
		if( pfnCreateCtrlWP(&this->id) ){
			return TRUE;
		}
		this->id = 0;
	}
	module.reset();
	return FALSE;
}

void CWritePlugInUtil::UnInitialize()
{
	if( module != NULL ){
		pfnDeleteCtrlWP(this->id);
		this->id = 0;
		module.reset();
	}
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

