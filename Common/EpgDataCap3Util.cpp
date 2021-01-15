#include "stdafx.h"
#include "EpgDataCap3Util.h"

#include "ErrDef.h"
#include "PathUtil.h"
#ifndef _WIN32
#include "StringUtil.h"
#include <dlfcn.h>
#endif

CEpgDataCap3Util::CEpgDataCap3Util(void)
{
	module = NULL;
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
#ifdef _WIN32
	fs_path path = loadDllFilePath ? loadDllFilePath : GetModulePath().replace_filename(L"EpgDataCap3.dll");
	module = LoadLibrary(path.c_str());
	auto getProcAddr = [=](const char* name) { return GetProcAddress((HMODULE)module, name); };
#else
	fs_path path = loadDllFilePath ? loadDllFilePath : GetModulePath().replace_filename(L"EpgDataCap3.so");
	string strPath;
	WtoUTF8(path.native(), strPath);
	module = dlopen(strPath.c_str(), RTLD_LAZY);
	auto getProcAddr = [=](const char* name) { return dlsym(module, name); };
#endif
	DWORD err = FALSE;
	if( module != NULL ){
		pfnSetDebugLogCallbackEP3 = (SetDebugLogCallbackEP3)getProcAddr("SetDebugLogCallbackEP");
		if( pfnSetDebugLogCallbackEP3 ){
			pfnSetDebugLogCallbackEP3(DebugLogCallback);
		}
		InitializeEP3 pfnInitializeEP3;
		if( (pfnInitializeEP3 = (InitializeEP3)getProcAddr("InitializeEP")) != NULL &&
		    (pfnUnInitializeEP3 = (UnInitializeEP3)getProcAddr("UnInitializeEP")) != NULL &&
		    (pfnAddTSPacketEP3 = (AddTSPacketEP3)getProcAddr("AddTSPacketEP")) != NULL &&
		    (pfnGetTSIDEP3 = (GetTSIDEP3)getProcAddr("GetTSIDEP")) != NULL &&
		    (pfnGetEpgInfoListEP3 = (GetEpgInfoListEP3)getProcAddr("GetEpgInfoListEP")) != NULL &&
		    (pfnClearSectionStatusEP3 = (ClearSectionStatusEP3)getProcAddr("ClearSectionStatusEP")) != NULL &&
		    (pfnGetSectionStatusEP3 = (GetSectionStatusEP3)getProcAddr("GetSectionStatusEP")) != NULL &&
		    (pfnGetServiceListActualEP3 = (GetServiceListActualEP3)getProcAddr("GetServiceListActualEP")) != NULL &&
		    (pfnGetServiceListEpgDBEP3 = (GetServiceListEpgDBEP3)getProcAddr("GetServiceListEpgDBEP")) != NULL &&
		    (pfnGetEpgInfoEP3 = (GetEpgInfoEP3)getProcAddr("GetEpgInfoEP")) != NULL &&
		    (pfnSearchEpgInfoEP3 = (SearchEpgInfoEP3)getProcAddr("SearchEpgInfoEP")) != NULL &&
		    (pfnGetTimeDelayEP3 = (GetTimeDelayEP3)getProcAddr("GetTimeDelayEP")) != NULL ){
			pfnEnumEpgInfoListEP3 = (EnumEpgInfoListEP3)getProcAddr("EnumEpgInfoListEP");
			pfnGetSectionStatusServiceEP3 = (GetSectionStatusServiceEP3)getProcAddr("GetSectionStatusServiceEP");
			pfnSetLogoTypeFlagsEP3 = (SetLogoTypeFlagsEP3)getProcAddr("SetLogoTypeFlagsEP");
			pfnEnumLogoListEP3 = (EnumLogoListEP3)getProcAddr("EnumLogoListEP");
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
#ifdef _WIN32
	FreeLibrary((HMODULE)module);
#else
	dlclose(module);
#endif
	module = NULL;
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
	BOOL (CALLBACK *enumEpgInfoListProc)(DWORD epgInfoListSize, EPG_EVENT_INFO* epgInfoList, LPVOID param),
	LPVOID param
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
	BOOL (CALLBACK *enumLogoListProc)(DWORD logoListSize, const LOGO_INFO* logoList, LPVOID param),
	LPVOID param
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
#ifdef WRAP_DEBUG_OUTPUT
	AddDebugLogNoNewline(s, true);
#endif
}
