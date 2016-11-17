#pragma once

#include "../../Common/StructDef.h"
#include "../../Common/EpgDataCap3Def.h"
#include "../../Common/BlockLock.h"

#import "RegExp.tlb" no_namespace named_guids

class CEpgDBManager
{
public:
	typedef struct _SEARCH_RESULT_EVENT{
		const EPGDB_EVENT_INFO* info;
		wstring findKey;
	}SEARCH_RESULT_EVENT;

	typedef struct _SEARCH_RESULT_EVENT_DATA{
		EPGDB_EVENT_INFO info;
		wstring findKey;
	}SEARCH_RESULT_EVENT_DATA;

public:
	CEpgDBManager(void);
	~CEpgDBManager(void);

	BOOL ReloadEpgData();

	BOOL IsLoadingData();

	BOOL IsInitialLoadingDataDone();

	BOOL CancelLoadData();

	BOOL SearchEpg(vector<EPGDB_SEARCH_KEY_INFO>* key, vector<SEARCH_RESULT_EVENT_DATA>* result);

	//P = [](vector<SEARCH_RESULT_EVENT>&) -> void
	template<class P>
	BOOL SearchEpg(vector<EPGDB_SEARCH_KEY_INFO>* key, P enumProc) {
		CBlockLock lock(&this->epgMapLock);
		vector<SEARCH_RESULT_EVENT> result;
		CoInitialize(NULL);
		{
			IRegExpPtr regExp;
			for( size_t i = 0; i < key->size(); i++ ){
				SearchEvent(&(*key)[i], result, regExp);
			}
		}
		CoUninitialize();
		enumProc(result);
		return TRUE;
	}

	BOOL GetServiceList(vector<EPGDB_SERVICE_INFO>* list);

	//P = [](const vector<EPGDB_EVENT_INFO>&) -> void
	template<class P>
	BOOL EnumEventInfo(LONGLONG serviceKey, P enumProc) const {
		CBlockLock lock(&this->epgMapLock);
		map<LONGLONG, EPGDB_SERVICE_EVENT_INFO>::const_iterator itr = this->epgMap.find(serviceKey);
		if( itr == this->epgMap.end() || itr->second.eventList.empty() ){
			return FALSE;
		}
		enumProc(itr->second.eventList);
		return TRUE;
	}

	//P = [](const map<LONGLONG, EPGDB_SERVICE_EVENT_INFO>&) -> void
	template<class P>
	BOOL EnumEventAll(P enumProc) const {
		CBlockLock lock(&this->epgMapLock);
		if( this->epgMap.empty() ){
			return FALSE;
		}
		enumProc(this->epgMap);
		return TRUE;
	}

	BOOL SearchEpg(
		WORD ONID,
		WORD TSID,
		WORD SID,
		WORD EventID,
		EPGDB_EVENT_INFO* result
		);

	BOOL SearchEpg(
		WORD ONID,
		WORD TSID,
		WORD SID,
		LONGLONG startTime,
		DWORD durationSec,
		EPGDB_EVENT_INFO* result
		);

	BOOL SearchServiceName(
		WORD ONID,
		WORD TSID,
		WORD SID,
		wstring& serviceName
		);

	static void ConvertSearchText(wstring& str);

protected:
	mutable CRITICAL_SECTION epgMapLock;

	HANDLE loadThread;
	BOOL loadStop;
	BOOL initialLoadDone;

	map<LONGLONG, EPGDB_SERVICE_EVENT_INFO> epgMap;
protected:
	static BOOL CALLBACK EnumEpgInfoListProc(DWORD epgInfoListSize, EPG_EVENT_INFO* epgInfoList, LPVOID param);
	void ClearEpgData();
	static UINT WINAPI LoadThread(LPVOID param);

	void SearchEvent(EPGDB_SEARCH_KEY_INFO* key, vector<SEARCH_RESULT_EVENT>& result, IRegExpPtr& regExp);
	BOOL IsEqualContent(vector<EPGDB_CONTENT_DATA>* searchKey, vector<EPGDB_CONTENT_DATA>* eventData);
	static BOOL IsInDateTime(const vector<EPGDB_SEARCH_DATE_INFO>& dateList, const SYSTEMTIME& time);
	static BOOL IsFindKeyword(BOOL regExpFlag, IRegExpPtr& regExp, BOOL caseFlag, const vector<wstring>* keyList, const wstring& word, BOOL andMode, wstring* findKey = NULL);
	static BOOL IsFindLikeKeyword(BOOL caseFlag, const vector<wstring>* keyList, const wstring& word, BOOL andMode, wstring* findKey = NULL);

};

