#include "stdafx.h"
#include "EpgDataCap3Util.h"

#include "ErrDef.h"
#include "PathUtil.h"

CEpgDataCap3Util::CEpgDataCap3Util(void)
	: module(nullptr, UtilFreeLibrary)
{
	id = 0;
}

CEpgDataCap3Util::~CEpgDataCap3Util(void)
{
	UnInitialize();
}

bool CEpgDataCap3Util::Initialize(
	LPCWSTR loadDllFilePath
	)
{
	if( module != nullptr ){
		return false;
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
	if( module != nullptr ){
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
			if( pfnInitializeEP3(FALSE, &id) == NO_ERR ){
				if( id != 0 ){
					return true;
				}
			}
			id = 0;
		}
		UnInitialize();
	}
	AddDebugLogFormat(L"%lsのロードに失敗しました", path.c_str());
	return false;
}

void CEpgDataCap3Util::UnInitialize(
	)
{
	if( module == nullptr ){
		return;
	}
	if( id != 0 ){
		pfnUnInitializeEP3(id);
		id = 0;
	}
	if( pfnSetDebugLogCallbackEP3 ){
		pfnSetDebugLogCallbackEP3(nullptr);
	}
	module.reset();
}

bool CEpgDataCap3Util::AddTSPacket(
	BYTE* data,
	DWORD size
	)
{
	if( module == nullptr ){
		return false;
	}
	if( size > 188 && pfnAddTSPacketEP3(id, data, 0) != ERR_FALSE ){
		for( ; size > 188; size -= 188, data += 188 ){
			pfnAddTSPacketEP3(id, data, 188);
		}
	}
	return pfnAddTSPacketEP3(id, data, size) == NO_ERR;
}

bool CEpgDataCap3Util::GetTSID(
	WORD* originalNetworkID,
	WORD* transportStreamID
	)
{
	if( module == nullptr || (originalNetworkID == nullptr && pfnEnumEpgInfoListEP3 == nullptr) ){
		return false;
	}
	return pfnGetTSIDEP3(id, originalNetworkID, transportStreamID) == NO_ERR;
}

bool CEpgDataCap3Util::GetServiceListActual(
	DWORD* serviceListSize,
	SERVICE_INFO** serviceList
	)
{
	return module && pfnGetServiceListActualEP3(id, serviceListSize, serviceList) == NO_ERR;
}

bool CEpgDataCap3Util::GetServiceListEpgDB(
	DWORD* serviceListSize,
	SERVICE_INFO** serviceList
	)
{
	return module && pfnGetServiceListEpgDBEP3(id, serviceListSize, serviceList) == NO_ERR;
}

bool CEpgDataCap3Util::GetEpgInfoList(
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	DWORD* epgInfoListSize,
	EPG_EVENT_INFO** epgInfoList
	)
{
	return module && pfnGetEpgInfoListEP3(id, originalNetworkID, transportStreamID, serviceID, epgInfoListSize, epgInfoList) == NO_ERR;
}

bool CEpgDataCap3Util::EnumEpgInfoList(
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	const std::function<bool(DWORD epgInfoListSize, EPG_EVENT_INFO* epgInfoList)>& enumEpgInfoListProc
	)
{
	if( module == nullptr || !enumEpgInfoListProc ){
		return false;
	}
	if( pfnEnumEpgInfoListEP3 == nullptr ){
		DWORD epgInfoListSize;
		EPG_EVENT_INFO* epgInfoList;
		DWORD ret = pfnGetEpgInfoListEP3(id, originalNetworkID, transportStreamID, serviceID, &epgInfoListSize, &epgInfoList);
		if( ret == NO_ERR && enumEpgInfoListProc(epgInfoListSize, nullptr) ){
			enumEpgInfoListProc(epgInfoListSize, epgInfoList);
		}
		return ret == NO_ERR;
	}
	return pfnEnumEpgInfoListEP3(id, originalNetworkID, transportStreamID, serviceID, EnumEpgInfoListProc, (void*)&enumEpgInfoListProc) == NO_ERR;
}

void CEpgDataCap3Util::ClearSectionStatus()
{
	if( module ){
		pfnClearSectionStatusEP3(id);
	}
}

EPG_SECTION_STATUS CEpgDataCap3Util::GetSectionStatus(
	bool l_eitFlag
	)
{
	if( module == nullptr ){
		return EpgNoData;
	}
	return pfnGetSectionStatusEP3(id, l_eitFlag);
}

pair<EPG_SECTION_STATUS, bool> CEpgDataCap3Util::GetSectionStatusService(
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	bool l_eitFlag
	)
{
	if( module == nullptr || pfnGetSectionStatusServiceEP3 == nullptr ){
		return std::make_pair(EpgNoData, false);
	}
	return std::make_pair(
		pfnGetSectionStatusServiceEP3(id, originalNetworkID, transportStreamID, serviceID, l_eitFlag), true);
}

bool CEpgDataCap3Util::GetEpgInfo(
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	bool nextFlag,
	EPG_EVENT_INFO** epgInfo
	)
{
	return module && pfnGetEpgInfoEP3(id, originalNetworkID, transportStreamID, serviceID, nextFlag, epgInfo) == NO_ERR;
}

bool CEpgDataCap3Util::SearchEpgInfo(
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	WORD eventID,
	bool pfOnlyFlag,
	EPG_EVENT_INFO** epgInfo
	)
{
	return module && pfnSearchEpgInfoEP3(id, originalNetworkID, transportStreamID, serviceID, eventID, pfOnlyFlag, epgInfo) == NO_ERR;
}

void CEpgDataCap3Util::SetLogoTypeFlags(
	DWORD flags,
	const WORD** additionalNeededPids
	)
{
	if( module && pfnSetLogoTypeFlagsEP3 ){
		pfnSetLogoTypeFlagsEP3(id, flags, additionalNeededPids);
	}
}

bool CEpgDataCap3Util::EnumLogoList(
	const std::function<bool(DWORD logoListSize, const LOGO_INFO* logoList)>& enumLogoListProc
	)
{
	if( module == nullptr || pfnEnumLogoListEP3 == nullptr || !enumLogoListProc ){
		return false;
	}
	return pfnEnumLogoListEP3(id, EnumLogoListProc, (void*)&enumLogoListProc) == NO_ERR;
}

int CEpgDataCap3Util::GetTimeDelay(
	)
{
	if( module == nullptr ){
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

BOOL CALLBACK CEpgDataCap3Util::EnumEpgInfoListProc(DWORD epgInfoListSize, EPG_EVENT_INFO* epgInfoList, void* param)
{
	return (*(const std::function<bool(DWORD, EPG_EVENT_INFO*)>*)param)(epgInfoListSize, epgInfoList);
}

BOOL CALLBACK CEpgDataCap3Util::EnumLogoListProc(DWORD logoListSize, const LOGO_INFO* logoList, void* param)
{
	return (*(const std::function<bool(DWORD, const LOGO_INFO*)>*)param)(logoListSize, logoList);
}
