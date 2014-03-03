#pragma once

#include "../../Common/StructDef.h"
#include "EpgDBManager.h"
#include "ReserveManager.h"

class CRestApiManager
{
public:
	CRestApiManager(void);
	~CRestApiManager(void);

	DWORD AnalyzeCmd(string verb, string url, string param, HTTP_STREAM* sendParam, CEpgDBManager* epgDB, vector<RESERVE_DATA*>* list, CReserveManager* resMng);

private:
	void CheckXMLChar(wstring& text);
	void LoadRecSetData(WORD preset, REC_SETTING_DATA* recSetData);

	DWORD GetEnumService(string param, HTTP_STREAM* sendParam, CEpgDBManager* epgDB);
	static void EnumEventInfoCallback(vector<EPGDB_SERVICE_EVENT_INFO>* pval, void* param);
	DWORD GetEnumEventInfo(string param, HTTP_STREAM* sendParam, CEpgDBManager* epgDB);
	DWORD GetEnumEventInfo(string param, HTTP_STREAM* sendParam, CEpgDBManager* epgDB, vector<EPGDB_SERVICE_EVENT_INFO>* list);
	DWORD GetEnumReserveInfo(string param, HTTP_STREAM* sendParam, vector<RESERVE_DATA*>* list);
	DWORD GetEnumRecPresetInfo(string param, HTTP_STREAM* sendParam);
	DWORD GetReserveDataEPG(string param, CEpgDBManager* epgDB, RESERVE_DATA* reserveData);
	DWORD GetReserveAddResponse(BOOL err, HTTP_STREAM* sendParam);
	DWORD GetSearchEvent(string param, HTTP_STREAM* sendParam, CEpgDBManager* epgDB);
};
