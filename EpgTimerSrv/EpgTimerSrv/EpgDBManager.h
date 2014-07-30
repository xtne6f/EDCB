#pragma once

#include "../../Common/Util.h"
#include "../../Common/StructDef.h"
#include "../../Common/StringUtil.h"
#include "../../Common/PathUtil.h"
#include "../../Common/EpgTimerUtil.h"
#include "../../Common/EpgDataCap3Util.h"
#include "../../Common/EpgDataCap3Def.h"
#include <memory>

#import "RegExp.tlb" no_namespace named_guids

class CEpgDBManager
{
public:
	typedef struct _SEARCH_RESULT_EVENT{
		EPGDB_EVENT_INFO* info;
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

	BOOL SearchEpg(vector<EPGDB_SEARCH_KEY_INFO>* key, vector<unique_ptr<SEARCH_RESULT_EVENT_DATA>>* result);

	BOOL SearchEpg(vector<EPGDB_SEARCH_KEY_INFO>* key, void (*enumProc)(vector<SEARCH_RESULT_EVENT>*, void*), void* param);

	BOOL GetServiceList(vector<EPGDB_SERVICE_INFO>* list);

	BOOL EnumEventInfo(LONGLONG serviceKey, vector<unique_ptr<EPGDB_EVENT_INFO>>* result);

	BOOL EnumEventInfo(LONGLONG serviceKey, void (*enumProc)(vector<EPGDB_EVENT_INFO*>*, void*), void* param);

	BOOL EnumEventAll(void (*enumProc)(vector<EPGDB_SERVICE_EVENT_INFO>*, void*), void* param);

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
	CRITICAL_SECTION epgMapLock;

	HANDLE loadThread;
	BOOL loadStop;
	BOOL initialLoadDone;

	typedef struct _EPGDB_SERVICE_DATA{
		EPGDB_SERVICE_INFO serviceInfo;
		map<WORD, EPGDB_EVENT_INFO*> eventMap;
		~_EPGDB_SERVICE_DATA(void){
			map<WORD, EPGDB_EVENT_INFO*>::iterator itr;
			for( itr = eventMap.begin(); itr != eventMap.end(); itr++ ){
				SAFE_DELETE(itr->second);
			}
		};
	}EPGDB_SERVICE_DATA;

	typedef struct _TIME_SEARCH{
		BYTE week;
		DWORD start;
		DWORD end;
	}TIME_SEARCH;

	map<LONGLONG, EPGDB_SERVICE_DATA*> epgMap;
protected:
	BOOL ConvertEpgInfo(WORD ONID, WORD TSID, WORD SID, EPG_EVENT_INFO* src, EPGDB_EVENT_INFO* dest);
	void ClearEpgData();
	static UINT WINAPI LoadThread(LPVOID param);

	void SearchEvent(EPGDB_SEARCH_KEY_INFO* key, map<ULONGLONG, SEARCH_RESULT_EVENT>* resultMap, IRegExpPtr& regExp);
	BOOL IsEqualContent(vector<EPGDB_CONTENT_DATA>* searchKey, vector<EPGDB_CONTENT_DATA>* eventData);
	BOOL IsInDateTime(vector<TIME_SEARCH>* timeList, SYSTEMTIME startTime);
	static BOOL IsFindKeyword(BOOL regExpFlag, IRegExpPtr& regExp, BOOL titleOnlyFlag, BOOL caseFlag, vector<wstring>* keyList, EPGDB_SHORT_EVENT_INFO* shortInfo, EPGDB_EXTENDED_EVENT_INFO* extInfo, BOOL andMode, wstring* findKey = NULL);
	BOOL IsFindLikeKeyword(BOOL titleOnlyFlag, BOOL caseFlag, vector<wstring>* keyList, EPGDB_SHORT_EVENT_INFO* shortInfo, EPGDB_EXTENDED_EVENT_INFO* extInfo, BOOL andMode, wstring* findKey = NULL);

};

