#include "stdafx.h"
#include "WritePlugInUtil.h"

#include "ErrDef.h"

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

	module = ::LoadLibrary(loadDllFilePath);

	if( module == NULL ){
		return FALSE;
	}

	pfnGetPlugInNameWP = ( GetPlugInNameWP ) ::GetProcAddress( module , "GetPlugInName");
	if( !pfnGetPlugInNameWP ){
		goto ERR_END;
	}
	pfnSettingWP = ( SettingWP ) ::GetProcAddress( module , "Setting");
	if( !pfnSettingWP ){
		goto ERR_END;
	}
	pfnCreateCtrlWP = ( CreateCtrlWP ) ::GetProcAddress( module , "CreateCtrl");
	if( !pfnCreateCtrlWP ){
		goto ERR_END;
	}
	pfnDeleteCtrlWP = ( DeleteCtrlWP ) ::GetProcAddress( module , "DeleteCtrl");
	if( !pfnDeleteCtrlWP ){
		goto ERR_END;
	}
	pfnStartSaveWP = ( StartSaveWP ) ::GetProcAddress( module , "StartSave");
	if( !pfnStartSaveWP ){
		goto ERR_END;
	}
	pfnStopSaveWP = ( StopSaveWP ) ::GetProcAddress( module , "StopSave");
	if( !pfnStopSaveWP ){
		goto ERR_END;
	}
	pfnGetSaveFilePathWP = ( GetSaveFilePathWP ) ::GetProcAddress( module , "GetSaveFilePath");
	if( !pfnGetSaveFilePathWP ){
		goto ERR_END;
	}
	pfnAddTSBuffWP = ( AddTSBuffWP ) ::GetProcAddress( module , "AddTSBuff");
	if( !pfnAddTSBuffWP ){
		goto ERR_END;
	}

	if( pfnCreateCtrlWP(&this->id) == FALSE ){
		this->id = 0;
		goto ERR_END;
	}
	return TRUE;

ERR_END:
	::FreeLibrary( module );
	module = NULL;
	return FALSE;
}

void CWritePlugInUtil::UnInitialize()
{
	if( module != NULL ){
		pfnDeleteCtrlWP(this->id);
		this->id = 0;
		::FreeLibrary( module );
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

