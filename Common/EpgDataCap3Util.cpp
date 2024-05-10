#include "stdafx.h"
#include "EpgDataCap3Util.h"

#include "ErrDef.h"
#include "PathUtil.h"

CEpgDataCap3Util::CEpgDataCap3Util(void)
	: module(NULL, UtilFreeLibrary)
{
	id = 0;
}

CEpgDataCap3Util::~CEpgDataCap3Util(void)
{
	UnInitialize();
}

DWORD CEpgDataCap3Util::Initialize(
	BOOL asyncFlag,
	LPCWSTR loadDllFilePath
	)
{
	if( module != NULL ){
		return FALSE;
	}
	fs_path path = loadDllFilePath ? loadDllFilePath :
#ifdef EDCB_LIB_ROOT
		fs_path(EDCB_LIB_ROOT).append(
#else
		GetModulePath().replace_filename(
#endif
		L"EpgDataCap3" EDCB_LIB_EXT
		);
	module.reset(UtilLoadLibrary(path));
	DWORD err = FALSE;
	if( module != NULL ){
		if( UtilGetProcAddress(module.get(), "SetDebugLogCallbackEP", pfnSetDebugLogCallbackEP3) ){
			pfnSetDebugLogCallbackEP3(DebugLogCallback);
		}
		InitializeEP3 pfnInitializeEP3;
		if( UtilGetProcAddress(module.get(), "InitializeEP", pfnInitializeEP3) &&
		    UtilGetProcAddress(module.get(), "UnInitializeEP", pfnUnInitializeEP3) &&
		    UtilGetProcAddress(module.get(), "AddTSPacketEP", pfnAddTSPacketEP3) &&
		    UtilGetProcAddress(module.get(), "GetTSIDEP", pfnGetTSIDEP3) &&
		    UtilGetProcAddress(module.get(), "GetEpgInfoListEP", pfnGetEpgInfoListEP3) &&
		    UtilGetProcAddress(module.get(), "ClearSectionStatusEP", pfnClearSectionStatusEP3) &&
		    UtilGetProcAddress(module.get(), "GetSectionStatusEP", pfnGetSectionStatusEP3) &&
		    UtilGetProcAddress(module.get(), "GetServiceListActualEP", pfnGetServiceListActualEP3) &&
		    UtilGetProcAddress(module.get(), "GetServiceListEpgDBEP", pfnGetServiceListEpgDBEP3) &&
		    UtilGetProcAddress(module.get(), "GetEpgInfoEP", pfnGetEpgInfoEP3) &&
		    UtilGetProcAddress(module.get(), "SearchEpgInfoEP", pfnSearchEpgInfoEP3) &&
		    UtilGetProcAddress(module.get(), "GetTimeDelayEP", pfnGetTimeDelayEP3) ){
			UtilGetProcAddress(module.get(), "EnumEpgInfoListEP", pfnEnumEpgInfoListEP3);
			UtilGetProcAddress(module.get(), "GetSectionStatusServiceEP", pfnGetSectionStatusServiceEP3);
			UtilGetProcAddress(module.get(), "SetLogoTypeFlagsEP", pfnSetLogoTypeFlagsEP3);
			UtilGetProcAddress(module.get(), "EnumLogoListEP", pfnEnumLogoListEP3);
			err = pfnInitializeEP3(asyncFlag, &id);
			if( err == NO_ERR ){
				if( id != 0 ){
					return err;
				}
				err = FALSE;
			}
			id = 0;
		}
		UnInitialize();
	}
	AddDebugLogFormat(L"%lsのロードに失敗しました", path.c_str());
	return err;
}

DWORD CEpgDataCap3Util::UnInitialize(
	)
{
	if( module == NULL ){
		return ERR_NOT_INIT;
	}
	DWORD err = NO_ERR;
	if( id != 0 ){
		err = pfnUnInitializeEP3(id);
		id = 0;
	}
	if( pfnSetDebugLogCallbackEP3 ){
		pfnSetDebugLogCallbackEP3(NULL);
	}
	module.reset();
	return err;
}

DWORD CEpgDataCap3Util::AddTSPacket(
	BYTE* data,
	DWORD size
	)
{
	if( module == NULL ){
		return ERR_NOT_INIT;
	}
	if( size > 188 && pfnAddTSPacketEP3(id, data, 0) != ERR_FALSE ){
		for( ; size > 188; size -= 188, data += 188 ){
			pfnAddTSPacketEP3(id, data, 188);
		}
	}
	return pfnAddTSPacketEP3(id, data, size);
}

DWORD CEpgDataCap3Util::GetTSID(
	WORD* originalNetworkID,
	WORD* transportStreamID
	)
{
	if( module == NULL ){
		return ERR_NOT_INIT;
	}
	if( originalNetworkID == NULL && pfnEnumEpgInfoListEP3 == NULL ){
		return ERR_INVALID_ARG;
	}
	return pfnGetTSIDEP3(id, originalNetworkID, transportStreamID);
}

DWORD CEpgDataCap3Util::GetServiceListActual(
	DWORD* serviceListSize,
	SERVICE_INFO** serviceList
	)
{
	if( module == NULL ){
		return ERR_NOT_INIT;
	}
	return pfnGetServiceListActualEP3(id, serviceListSize, serviceList);
}

DWORD CEpgDataCap3Util::GetServiceListEpgDB(
	DWORD* serviceListSize,
	SERVICE_INFO** serviceList
	)
{
	if( module == NULL ){
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
	if( module == NULL ){
		return ERR_NOT_INIT;
	}
	return pfnGetEpgInfoListEP3(id, originalNetworkID, transportStreamID, serviceID, epgInfoListSize, epgInfoList);
}

DWORD CEpgDataCap3Util::EnumEpgInfoList(
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	BOOL (CALLBACK *enumEpgInfoListProc)(DWORD epgInfoListSize, EPG_EVENT_INFO* epgInfoList, void* param),
	void* param
	)
{
	if( module == NULL ){
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
	if( module == NULL ){
		return ;
	}
	pfnClearSectionStatusEP3(id);
}

EPG_SECTION_STATUS CEpgDataCap3Util::GetSectionStatus(
	BOOL l_eitFlag
	)
{
	if( module == NULL ){
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
	if( module == NULL || pfnGetSectionStatusServiceEP3 == NULL ){
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
	if( module == NULL ){
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
	if( module == NULL ){
		return ERR_NOT_INIT;
	}
	return pfnSearchEpgInfoEP3(id, originalNetworkID, transportStreamID, serviceID, eventID, pfOnlyFlag, epgInfo);
}

void CEpgDataCap3Util::SetLogoTypeFlags(
	DWORD flags,
	const WORD** additionalNeededPids
	)
{
	if( module == NULL || pfnSetLogoTypeFlagsEP3 == NULL ){
		return;
	}
	return pfnSetLogoTypeFlagsEP3(id, flags, additionalNeededPids);
}

DWORD CEpgDataCap3Util::EnumLogoList(
	BOOL (CALLBACK *enumLogoListProc)(DWORD logoListSize, const LOGO_INFO* logoList, void* param),
	void* param
	)
{
	if( module == NULL ){
		return ERR_NOT_INIT;
	}
	if( pfnEnumLogoListEP3 == NULL ){
		return ERR_FALSE;
	}
	return pfnEnumLogoListEP3(id, enumLogoListProc, param);
}

int CEpgDataCap3Util::GetTimeDelay(
	)
{
	if( module == NULL ){
		return 0;
	}
	return pfnGetTimeDelayEP3(id);
}

void CALLBACK CEpgDataCap3Util::DebugLogCallback(
	const WCHAR* s
	)
{
	(void)s;
#ifdef WRAP_DEBUG_OUTPUT
	AddDebugLogNoNewline(s, true);
#endif
}
