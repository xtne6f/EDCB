#include "stdafx.h"
#include "WritePlugInUtil.h"
#include "PathUtil.h"

CWritePlugInUtil::CWritePlugInUtil(void)
	: module(nullptr, UtilFreeLibrary)
{
	this->id = 0;
}

CWritePlugInUtil::~CWritePlugInUtil(void)
{
	UnInitialize();
}

bool CWritePlugInUtil::Initialize(
	const wstring& loadDllFilePath
	)
{
	if( module != nullptr ){
		return false;
	}

	module.reset(UtilLoadLibrary(loadDllFilePath));

	if( module == nullptr ){
		return false;
	}

	CreateCtrlWP pfnCreateCtrlWP;
	if( UtilGetProcAddress(module.get(), "CreateCtrl", pfnCreateCtrlWP) &&
	    UtilGetProcAddress(module.get(), "DeleteCtrl", pfnDeleteCtrlWP) &&
	    UtilGetProcAddress(module.get(), "StartSave", pfnStartSaveWP) &&
	    UtilGetProcAddress(module.get(), "StopSave", pfnStopSaveWP) &&
	    UtilGetProcAddress(module.get(), "GetSaveFilePath", pfnGetSaveFilePathWP) &&
	    UtilGetProcAddress(module.get(), "AddTSBuff", pfnAddTSBuffWP) ){
		if( pfnCreateCtrlWP(&this->id) ){
			return true;
		}
		this->id = 0;
	}
	module.reset();
	return false;
}

void CWritePlugInUtil::UnInitialize()
{
	if( module != nullptr ){
		pfnDeleteCtrlWP(this->id);
		this->id = 0;
		module.reset();
	}
}

bool CWritePlugInUtil::Start(
	LPCWSTR fileName,
	bool overWriteFlag,
	ULONGLONG createSize
	)
{
	return module && pfnStartSaveWP(this->id, fileName, overWriteFlag, createSize);
}

bool CWritePlugInUtil::Stop(
	)
{
	return module && pfnStopSaveWP(this->id);
}

bool CWritePlugInUtil::GetSavePath(
	WCHAR* filePath,
	DWORD* filePathSize
	)
{
	return module && pfnGetSaveFilePathWP(this->id, filePath, filePathSize);
}

bool CWritePlugInUtil::Write(
	BYTE* data,
	DWORD size,
	DWORD* writeSize
	)
{
	return module && pfnAddTSBuffWP(this->id, data, size, writeSize);
}
