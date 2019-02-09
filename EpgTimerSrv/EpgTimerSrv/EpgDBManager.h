#pragma once

#include "../../Common/StructDef.h"
#include "../../Common/EpgDataCap3Def.h"
#include "../../Common/ThreadUtil.h"
#include <functional>
#include <objbase.h>
#include <oleauto.h>
#include "RegExp.h"

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
	CEpgDBManager();
	~CEpgDBManager();

	void SetArchivePeriod(int periodSec);

	//同期的に呼び出すこと
	void ReloadEpgData(bool foreground = false);
	//同期的に呼び出すこと
	bool IsLoadingData();
	//同期的に呼び出すこと
	void CancelLoadData();

	bool IsInitialLoadingDataDone() const { return this->initialLoadDone; }

	void SearchEpg(const EPGDB_SEARCH_KEY_INFO* keys, size_t keysSize, vector<SEARCH_RESULT_EVENT_DATA>* result) const;

	//P = [](vector<SEARCH_RESULT_EVENT>&) -> void
	template<class P>
	void SearchEpg(const EPGDB_SEARCH_KEY_INFO* keys, size_t keysSize, P enumProc) const {
		CRefLock lock(&this->epgMapRefLock);
		vector<SEARCH_RESULT_EVENT> result;
		CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		for( size_t i = 0; i < keysSize; i++ ){
			SearchEvent(keys[i], result);
		}
		CoUninitialize();
		enumProc(result);
	}

	bool GetServiceList(vector<EPGDB_SERVICE_INFO>* list) const;

	pair<__int64, __int64> GetEventMinMaxTime(WORD onid, WORD tsid, WORD sid) const {
		CRefLock lock(&this->epgMapRefLock);
		return GetEventMinMaxTimeProc(onid, tsid, sid, false);
	}

	bool EnumEventInfo(int* keys, size_t keysSize, __int64 enumStart, __int64 enumEnd,
	                   const std::function<void(const EPGDB_EVENT_INFO*)>& enumProc) const {
		CRefLock lock(&this->epgMapRefLock);
		return EnumEventInfoProc(keys, keysSize, enumStart, enumEnd, enumProc, false);
	}

	//P = [](const map<LONGLONG, EPGDB_SERVICE_EVENT_INFO>&) -> void
	template<class P>
	bool EnumEventAll(P enumProc) const {
		CRefLock lock(&this->epgMapRefLock);
		if( this->epgMap.empty() ){
			return false;
		}
		enumProc(this->epgMap);
		return true;
	}

	pair<__int64, __int64> GetArchiveEventMinMaxTime(WORD onid, WORD tsid, WORD sid) const;

	bool EnumArchiveEventInfo(int* keys, size_t keysSize, __int64 enumStart, __int64 enumEnd, bool inmemory,
	                          const std::function<void(const EPGDB_EVENT_INFO*)>& enumProc) const;

	//P = [](const map<LONGLONG, EPGDB_SERVICE_EVENT_INFO>&) -> void
	template<class P>
	void EnumArchiveEventAll(P enumProc) const {
		CRefLock lock(&this->epgMapRefLock);
		enumProc(this->epgArchive);
	}

	bool SearchEpg(
		WORD ONID,
		WORD TSID,
		WORD SID,
		WORD EventID,
		EPGDB_EVENT_INFO* result
		) const;

	bool SearchEpg(
		WORD ONID,
		WORD TSID,
		WORD SID,
		LONGLONG startTime,
		DWORD durationSec,
		EPGDB_EVENT_INFO* result
		) const;

	bool SearchServiceName(
		WORD ONID,
		WORD TSID,
		WORD SID,
		wstring& serviceName
		) const;

	static void ConvertSearchText(wstring& str);
	static void ComRelease(IUnknown* p) { p->Release(); }

	typedef std::unique_ptr<IRegExp, decltype(&ComRelease)> RegExpPtr;
	typedef std::unique_ptr<OLECHAR, decltype(&SysFreeString)> OleCharPtr;

private:
	class CRefLock
	{
	public:
		CRefLock(pair<int, recursive_mutex_*>* ref_) : ref(ref_) {
			CBlockLock lock(ref->second);
			++ref->first;
		}
		~CRefLock() {
			CBlockLock lock(ref->second);
			--ref->first;
		}
	private:
		pair<int, recursive_mutex_*>* ref;
	};

	mutable recursive_mutex_ epgMapLock;
	mutable pair<int, recursive_mutex_*> epgMapRefLock;

	thread_ loadThread;
	atomic_bool_ loadStop;
	bool loadForeground;
	atomic_bool_ initialLoadDone;
	int archivePeriodSec;

	//これらデータベースの読み取りにかぎりepgMapRefLockでアクセスできる。LoadThread以外では変更できない
	map<LONGLONG, EPGDB_SERVICE_EVENT_INFO> epgMap;
	map<LONGLONG, EPGDB_SERVICE_EVENT_INFO> epgArchive;
	vector<vector<__int64>> epgOldIndexCache;

	static BOOL CALLBACK EnumEpgInfoListProc(DWORD epgInfoListSize, EPG_EVENT_INFO* epgInfoList, LPVOID param);
	static void LoadThread(CEpgDBManager* sys);

	void SearchEvent(const EPGDB_SEARCH_KEY_INFO& key, vector<SEARCH_RESULT_EVENT>& result) const;
	static bool IsEqualContent(const vector<EPGDB_CONTENT_DATA>& searchKey, const vector<EPGDB_CONTENT_DATA>& eventData);
	static bool IsInDateTime(const vector<EPGDB_SEARCH_DATE_INFO>& dateList, const SYSTEMTIME& time);
	static bool FindKeyword(const vector<pair<wstring, RegExpPtr>>& keyList, const EPGDB_EVENT_INFO& info, wstring& word,
	                        vector<int>& dist, bool caseFlag, bool aimai, bool andFlag, wstring* findKey = NULL);
	static bool FindLikeKeyword(const wstring& key, size_t keyPos, const wstring& word, vector<int>& dist, bool caseFlag);
	static void AddKeyword(vector<pair<wstring, RegExpPtr>>& keyList, wstring key, bool caseFlag, bool regExp, bool titleOnly);
	pair<__int64, __int64> GetEventMinMaxTimeProc(WORD onid, WORD tsid, WORD sid, bool archive) const;
	bool EnumEventInfoProc(int* keys, size_t keysSize, __int64 enumStart, __int64 enumEnd,
	                       const std::function<void(const EPGDB_EVENT_INFO*)>& enumProc, bool archive) const;
};

