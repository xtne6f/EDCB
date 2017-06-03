#pragma once

#include "../../Common/StructDef.h"
#include "../../Common/EpgDataCap3Def.h"
#include "../../Common/BlockLock.h"

#import "RegExp.tlb" no_namespace named_guids

class CEpgDBManager
{
public:
	struct SEARCH_RESULT_EVENT {
		const EPGDB_EVENT_INFO* info;
		wstring findKey;
	};

	struct SEARCH_RESULT_EVENT_DATA {
		EPGDB_EVENT_INFO info;
		wstring findKey;
	};

public:
	CEpgDBManager(void);
	~CEpgDBManager(void);

	void SetArchivePeriod(int periodSec);

	BOOL ReloadEpgData(BOOL foreground = FALSE);

	BOOL IsLoadingData() const;

	BOOL IsInitialLoadingDataDone() const;

	BOOL SearchEpg(const vector<EPGDB_SEARCH_KEY_INFO>* key, vector<SEARCH_RESULT_EVENT_DATA>* result) const;

	//P = [](vector<SEARCH_RESULT_EVENT>&) -> void
	template<class P>
	BOOL SearchEpg(const vector<EPGDB_SEARCH_KEY_INFO>* key, P enumProc) const {
		CRefLock lock(&this->epgMapRefLock);
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

	BOOL GetServiceList(vector<EPGDB_SERVICE_INFO>* list) const;

	//P = [](const vector<EPGDB_EVENT_INFO>&) -> void
	template<class P>
	BOOL EnumEventInfo(LONGLONG serviceKey, P enumProc) const {
		CRefLock lock(&this->epgMapRefLock);
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
		CRefLock lock(&this->epgMapRefLock);
		if( this->epgMap.empty() ){
			return FALSE;
		}
		enumProc(this->epgMap);
		return TRUE;
	}

	//P = [](const vector<EPGDB_EVENT_INFO>&) -> void
	template<class P>
	BOOL EnumArchiveEventInfo(LONGLONG serviceKey, P enumProc) const {
		CRefLock lock(&this->epgMapRefLock);
		map<LONGLONG, EPGDB_SERVICE_EVENT_INFO>::const_iterator itr = this->epgArchive.find(serviceKey);
		if( itr == this->epgArchive.end() ){
			return FALSE;
		}
		enumProc(itr->second.eventList);
		return TRUE;
	}

	//P = [](const map<LONGLONG, EPGDB_SERVICE_EVENT_INFO>&) -> void
	template<class P>
	void EnumArchiveEventAll(P enumProc) const {
		CRefLock lock(&this->epgMapRefLock);
		enumProc(this->epgArchive);
	}

	BOOL SearchEpg(
		WORD ONID,
		WORD TSID,
		WORD SID,
		WORD EventID,
		EPGDB_EVENT_INFO* result
		) const;

	BOOL SearchEpg(
		WORD ONID,
		WORD TSID,
		WORD SID,
		LONGLONG startTime,
		DWORD durationSec,
		EPGDB_EVENT_INFO* result
		) const;

	BOOL SearchServiceName(
		WORD ONID,
		WORD TSID,
		WORD SID,
		wstring& serviceName
		) const;

	static void ConvertSearchText(wstring& str);

protected:
	class CRefLock
	{
	public:
		CRefLock(pair<int, CRITICAL_SECTION*>* ref_) : ref(ref_) {
			CBlockLock lock(ref->second);
			++ref->first;
		}
		~CRefLock() {
			CBlockLock lock(ref->second);
			--ref->first;
		}
	private:
		pair<int, CRITICAL_SECTION*>* ref;
	};

	mutable CRITICAL_SECTION epgMapLock;
	mutable pair<int, CRITICAL_SECTION*> epgMapRefLock;

	HANDLE loadThread;
	BOOL loadStop;
	BOOL loadForeground;
	BOOL initialLoadDone;
	int archivePeriodSec;

	//これらデータベースの読み取りにかぎりepgMapRefLockでアクセスできる。LoadThread以外では変更できない
	map<LONGLONG, EPGDB_SERVICE_EVENT_INFO> epgMap;
	map<LONGLONG, EPGDB_SERVICE_EVENT_INFO> epgArchive;
protected:
	static BOOL CALLBACK EnumEpgInfoListProc(DWORD epgInfoListSize, EPG_EVENT_INFO* epgInfoList, LPVOID param);
	void CancelLoadData(DWORD forceTimeout);
	static UINT WINAPI LoadThread(LPVOID param);

	void SearchEvent(const EPGDB_SEARCH_KEY_INFO* key, vector<SEARCH_RESULT_EVENT>& result, IRegExpPtr& regExp) const;
	static BOOL IsEqualContent(const vector<EPGDB_CONTENT_DATA>& searchKey, const vector<EPGDB_CONTENT_DATA>& eventData);
	static BOOL IsInDateTime(const vector<EPGDB_SEARCH_DATE_INFO>& dateList, const SYSTEMTIME& time);
	static BOOL IsFindKeyword(BOOL regExpFlag, IRegExpPtr& regExp, BOOL caseFlag, const vector<wstring>& keyList, const wstring& word, BOOL andMode, wstring* findKey = NULL);
	static BOOL IsFindLikeKeyword(BOOL caseFlag, const vector<wstring>& keyList, const wstring& word, vector<int>& dist, wstring* findKey = NULL);

};

