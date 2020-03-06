#pragma once

//正規表現エンジンをstd::wregexにする場合はこのマクロを定義する
//#define EPGDB_STD_WREGEX

#include "../../Common/StructDef.h"
#include "../../Common/EpgDataCap3Def.h"
#include "../../Common/ThreadUtil.h"
#include <functional>
#if !defined(EPGDB_STD_WREGEX) && defined(_WIN32)
#include <objbase.h>
#include <oleauto.h>
#include "RegExp.h"
#else
#include <regex>
#endif

class CEpgDBManager
{
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

	void SearchEpg(const EPGDB_SEARCH_KEY_INFO* keys, size_t keysSize, __int64 enumStart, __int64 enumEnd,
	               wstring* findKey, const std::function<void(const EPGDB_EVENT_INFO*, wstring*)>& enumProc) const;

	void SearchArchiveEpg(const EPGDB_SEARCH_KEY_INFO* keys, size_t keysSize, __int64 enumStart, __int64 enumEnd, bool deletableBeforeEnumDone,
	                      wstring* findKey, const std::function<void(const EPGDB_EVENT_INFO*, wstring*)>& enumProc) const;

	bool GetServiceList(vector<EPGDB_SERVICE_INFO>* list) const;

	pair<__int64, __int64> GetEventMinMaxTime(__int64 keyMask, __int64 key) const {
		CRefLock lock(&this->epgMapRefLock);
		return GetEventMinMaxTimeProc(keyMask, key, false);
	}

	bool EnumEventInfo(__int64* keys, size_t keysSize, __int64 enumStart, __int64 enumEnd,
	                   const std::function<void(const EPGDB_EVENT_INFO*, const EPGDB_SERVICE_INFO*)>& enumProc) const {
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

	pair<__int64, __int64> GetArchiveEventMinMaxTime(__int64 keyMask, __int64 key) const;

	void EnumArchiveEventInfo(__int64* keys, size_t keysSize, __int64 enumStart, __int64 enumEnd, bool deletableBeforeEnumDone,
	                          const std::function<void(const EPGDB_EVENT_INFO*, const EPGDB_SERVICE_INFO*)>& enumProc) const;

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
#if !defined(EPGDB_STD_WREGEX) && defined(_WIN32)
	static void ComRelease(IUnknown* p) { p->Release(); }

	typedef std::unique_ptr<IRegExp, decltype(&ComRelease)> RegExpPtr;
	typedef std::unique_ptr<OLECHAR, decltype(&SysFreeString)> OleCharPtr;
#else
	typedef std::unique_ptr<std::wregex> RegExpPtr;
#endif

private:
	struct SEARCH_CONTEXT {
		bool caseFlag;
		DWORD chkDurationMinSec;
		DWORD chkDurationMaxSec;
		vector<vector<pair<wstring, RegExpPtr>>> andKeyList;
		vector<pair<wstring, RegExpPtr>> notKeyList;
		const EPGDB_SEARCH_KEY_INFO* key;
		wstring targetWord;
		vector<int> distForFind;
	};

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

	static bool InitializeSearchContext(SEARCH_CONTEXT& ctx, vector<__int64>& enumServiceKey, const EPGDB_SEARCH_KEY_INFO* key);
	static bool IsMatchEvent(SEARCH_CONTEXT* ctxs, size_t ctxsSize, const EPGDB_EVENT_INFO* itrEvent, wstring* findKey);
	static bool IsEqualContent(const vector<EPGDB_CONTENT_DATA>& searchKey, const vector<EPGDB_CONTENT_DATA>& eventData);
	static bool IsInDateTime(const vector<EPGDB_SEARCH_DATE_INFO>& dateList, const SYSTEMTIME& time);
	static bool FindKeyword(const vector<pair<wstring, RegExpPtr>>& keyList, const EPGDB_EVENT_INFO& info, wstring& word,
	                        vector<int>& dist, bool caseFlag, bool aimai, bool andFlag, wstring* findKey = NULL);
	static bool FindLikeKeyword(const wstring& key, size_t keyPos, const wstring& word, vector<int>& dist, bool caseFlag);
	static void AddKeyword(vector<pair<wstring, RegExpPtr>>& keyList, wstring key, bool caseFlag, bool regExp, bool titleOnly);
	pair<__int64, __int64> GetEventMinMaxTimeProc(__int64 keyMask, __int64 key, bool archive) const;
	bool EnumEventInfoProc(__int64* keys, size_t keysSize, __int64 enumStart, __int64 enumEnd,
	                       const std::function<void(const EPGDB_EVENT_INFO*, const EPGDB_SERVICE_INFO*)>& enumProc, bool archive) const;
};

