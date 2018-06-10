#include "stdafx.h"
#include "EpgDataCap3Util.h"

#include "ErrDef.h"

CEpgDataCap3Util::CEpgDataCap3Util(void)
{
	module = NULL;
	id = 0;
}

CEpgDataCap3Util::~CEpgDataCap3Util(void)
{
	UnLoadDll();
}

BOOL CEpgDataCap3Util::LoadDll(LPCWSTR loadDllFilePath)
{
	if( module != NULL ){
		return FALSE;
	}

	module = ::LoadLibrary(loadDllFilePath);

	if( module == NULL ){
		_OutputDebugString(L"%sのロードに失敗しました\r\n", loadDllFilePath);
		return FALSE;
	}

	pfnInitializeEP3 = ( InitializeEP3 ) ::GetProcAddress( module , "InitializeEP");
	if( !pfnInitializeEP3 ){
		goto ERR_END;
	}
	pfnUnInitializeEP3 = ( UnInitializeEP3 ) ::GetProcAddress( module , "UnInitializeEP");
	if( !pfnUnInitializeEP3 ){
		goto ERR_END;
	}
	pfnAddTSPacketEP3 = ( AddTSPacketEP3 ) ::GetProcAddress( module , "AddTSPacketEP");
	if( !pfnAddTSPacketEP3 ){
		goto ERR_END;
	}
	pfnGetTSIDEP3 = ( GetTSIDEP3 ) ::GetProcAddress( module , "GetTSIDEP");
	if( !pfnGetTSIDEP3 ){
		goto ERR_END;
	}
	pfnGetEpgInfoListEP3 = ( GetEpgInfoListEP3 ) ::GetProcAddress( module , "GetEpgInfoListEP");
	if( !pfnGetEpgInfoListEP3 ){
		goto ERR_END;
	}
	pfnEnumEpgInfoListEP3 = ( EnumEpgInfoListEP3 ) ::GetProcAddress( module , "EnumEpgInfoListEP");

	pfnClearSectionStatusEP3 = ( ClearSectionStatusEP3 ) ::GetProcAddress( module , "ClearSectionStatusEP");
	if( !pfnClearSectionStatusEP3 ){
		goto ERR_END;
	}
	pfnGetSectionStatusEP3 = ( GetSectionStatusEP3 ) ::GetProcAddress( module , "GetSectionStatusEP");
	if( !pfnGetSectionStatusEP3 ){
		goto ERR_END;
	}
	pfnGetSectionStatusServiceEP3 = ( GetSectionStatusServiceEP3 ) ::GetProcAddress( module , "GetSectionStatusServiceEP");

	pfnGetServiceListActualEP3 = ( GetServiceListActualEP3 ) ::GetProcAddress( module , "GetServiceListActualEP");
	if( !pfnGetServiceListActualEP3 ){
		goto ERR_END;
	}
	pfnGetServiceListEpgDBEP3 = ( GetServiceListEpgDBEP3 ) ::GetProcAddress( module , "GetServiceListEpgDBEP");
	if( !pfnGetServiceListEpgDBEP3 ){
		goto ERR_END;
	}
	pfnGetEpgInfoEP3 = ( GetEpgInfoEP3 ) ::GetProcAddress( module , "GetEpgInfoEP");
	if( !pfnGetEpgInfoEP3 ){
		goto ERR_END;
	}
	pfnSearchEpgInfoEP3 = ( SearchEpgInfoEP3 ) ::GetProcAddress( module , "SearchEpgInfoEP");
	if( !pfnSearchEpgInfoEP3 ){
		goto ERR_END;
	}
	pfnGetTimeDelayEP3 = ( GetTimeDelayEP3 ) ::GetProcAddress( module , "GetTimeDelayEP");
	if( !pfnGetTimeDelayEP3 ){
		goto ERR_END;
	}
	return TRUE;

ERR_END:
	_OutputDebugString(L"%sのロード中 GetProcAddress に失敗\r\n", loadDllFilePath);
	::FreeLibrary( module );
	module = NULL;
	return FALSE;
}

BOOL CEpgDataCap3Util::UnLoadDll(void)
{
	if( module != NULL ){
		if( id != 0 ){
			pfnUnInitializeEP3(id);
		}
		::FreeLibrary( module );
		id = 0;
	}
	module = NULL;

	return TRUE;
}

DWORD CEpgDataCap3Util::Initialize(
	BOOL asyncFlag,
	LPCWSTR loadDllFilePath
	)
{
	if( LoadDll(loadDllFilePath) == FALSE ){
		return ERR_INIT;
	}
	DWORD err = pfnInitializeEP3(asyncFlag, &id);
	if( err != NO_ERR ){
		id = 0;
	}
	return err;
}

DWORD CEpgDataCap3Util::UnInitialize(
	)
{
	if( module == NULL || id == 0 ){
		return ERR_NOT_INIT;
	}
	DWORD err = pfnUnInitializeEP3(id);
	id = 0; // ← これがないと下の UnLoadDll で再度 UnInitializeEP が呼ばれる
	UnLoadDll();
	return err;
}

DWORD CEpgDataCap3Util::AddTSPacket(
	BYTE* data,
	DWORD size
	)
{
	if( module == NULL || id == 0 ){
		return ERR_NOT_INIT;
	}
	return pfnAddTSPacketEP3(id, data, size);
}

DWORD CEpgDataCap3Util::GetTSID(
	WORD* originalNetworkID,
	WORD* transportStreamID
	)
{
	if( module == NULL || id == 0 ){
		return ERR_NOT_INIT;
	}
	return pfnGetTSIDEP3(id, originalNetworkID, transportStreamID);
}

DWORD CEpgDataCap3Util::GetServiceListActual(
	DWORD* serviceListSize,
	SERVICE_INFO** serviceList
	)
{
	if( module == NULL || id == 0 ){
		return ERR_NOT_INIT;
	}
	return pfnGetServiceListActualEP3(id, serviceListSize, serviceList);
}

DWORD CEpgDataCap3Util::GetServiceListEpgDB(
	DWORD* serviceListSize,
	SERVICE_INFO** serviceList
	)
{
	if( module == NULL || id == 0 ){
		return ERR_NOT_INIT;
	}
	return pfnGetServiceListEpgDBEP3(id, serviceListSize, serviceList);
}

DWORD CEpgDataCap3Util::GetEpgInfoList(
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	DWORD* epgInfoListSize,
	EPG_EVENT_INFO** epgInfoList
	)
{
	if( module == NULL || id == 0 ){
		return ERR_NOT_INIT;
	}
	return pfnGetEpgInfoListEP3(id, originalNetworkID, transportStreamID, serviceID, epgInfoListSize, epgInfoList);
}

DWORD CEpgDataCap3Util::EnumEpgInfoList(
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	BOOL (CALLBACK *enumEpgInfoListProc)(DWORD epgInfoListSize, EPG_EVENT_INFO* epgInfoList, LPVOID param),
	LPVOID param
	)
{
	if( module == NULL || id == 0 ){
		return ERR_NOT_INIT;
	}
	if( pfnEnumEpgInfoListEP3 == NULL ){
		DWORD epgInfoListSize;
		EPG_EVENT_INFO* epgInfoList;
		DWORD ret = pfnGetEpgInfoListEP3(id, originalNetworkID, transportStreamID, serviceID, &epgInfoListSize, &epgInfoList);
		if( ret == NO_ERR && enumEpgInfoListProc(epgInfoListSize, NULL, param) != FALSE ){
			enumEpgInfoListProc(epgInfoListSize, epgInfoList, param);
		}
		return ret;
	}
	return pfnEnumEpgInfoListEP3(id, originalNetworkID, transportStreamID, serviceID, enumEpgInfoListProc, param);
}

void CEpgDataCap3Util::ClearSectionStatus()
{
	if( module == NULL || id == 0 ){
		return ;
	}
	pfnClearSectionStatusEP3(id);
}

EPG_SECTION_STATUS CEpgDataCap3Util::GetSectionStatus(
	BOOL l_eitFlag
	)
{
	if( module == NULL || id == 0 ){
		return EpgNoData;
	}
	return pfnGetSectionStatusEP3(id, l_eitFlag);
}

pair<EPG_SECTION_STATUS, BOOL> CEpgDataCap3Util::GetSectionStatusService(
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	BOOL l_eitFlag
	)
{
	if( module == NULL || id == 0 || pfnGetSectionStatusServiceEP3 == NULL ){
		return pair<EPG_SECTION_STATUS, BOOL>(EpgNoData, FALSE);
	}
	return pair<EPG_SECTION_STATUS, BOOL>(
		pfnGetSectionStatusServiceEP3(id, originalNetworkID, transportStreamID, serviceID, l_eitFlag), TRUE);
}

DWORD CEpgDataCap3Util::GetEpgInfo(
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	BOOL nextFlag,
	EPG_EVENT_INFO** epgInfo
	)
{
	if( module == NULL || id == 0 ){
		return ERR_NOT_INIT;
	}
	return pfnGetEpgInfoEP3(id, originalNetworkID, transportStreamID, serviceID, nextFlag, epgInfo);
}

DWORD CEpgDataCap3Util::SearchEpgInfo(
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	WORD eventID,
	BYTE pfOnlyFlag,
	EPG_EVENT_INFO** epgInfo
	)
{
	if( module == NULL || id == 0 ){
		return ERR_NOT_INIT;
	}
	return pfnSearchEpgInfoEP3(id, originalNetworkID, transportStreamID, serviceID, eventID, pfOnlyFlag, epgInfo);
}

int CEpgDataCap3Util::GetTimeDelay(
	)
{
	if( module == NULL || id == 0 ){
		return 0;
	}
	return pfnGetTimeDelayEP3(id);
}
