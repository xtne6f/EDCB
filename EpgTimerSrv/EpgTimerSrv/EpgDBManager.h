#pragma once

#include "../../Common/StructDef.h"
#include "../../Common/EpgDataCap3Def.h"
#include "../../Common/ThreadUtil.h"
#include <objbase.h>
#include <OleAuto.h>
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

	bool SearchEpg(const vector<EPGDB_SEARCH_KEY_INFO>* key, vector<SEARCH_RESULT_EVENT_DATA>* result) const;

	//P = [](vector<SEARCH_RESULT_EVENT>&) -> void
	template<class P>
	bool SearchEpg(const vector<EPGDB_SEARCH_KEY_INFO>* key, P enumProc) const {
		CRefLock lock(&this->epgMapRefLock);
		vector<SEARCH_RESULT_EVENT> result;
		CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		{
			std::unique_ptr<IRegExp, void(*)(IUnknown*)> regExp(NULL, ComRelease);
			for( size_t i = 0; i < key->size(); i++ ){
				SearchEvent(&(*key)[i], result, regExp);
			}
		}
		CoUninitialize();
		enumProc(result);
		return true;
	}

	bool GetServiceList(vector<EPGDB_SERVICE_INFO>* list) const;

	//P = [](const vector<EPGDB_EVENT_INFO>&) -> void
	template<class P>
	bool EnumEventInfo(LONGLONG serviceKey, P enumProc) const {
		CRefLock lock(&this->epgMapRefLock);
		map<LONGLONG, EPGDB_SERVICE_EVENT_INFO>::const_iterator itr = this->epgMap.find(serviceKey);
		if( itr == this->epgMap.end() || itr->second.eventList.empty() ){
			return false;
		}
		enumProc(itr->second.eventList);
		return true;
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

	//P = [](const vector<EPGDB_EVENT_INFO>&) -> void
	template<class P>
	bool EnumArchiveEventInfo(LONGLONG serviceKey, P enumProc) const {
		CRefLock lock(&this->epgMapRefLock);
		map<LONGLONG, EPGDB_SERVICE_EVENT_INFO>::const_iterator itr = this->epgArchive.find(serviceKey);
		if( itr == this->epgArchive.end() ){
			return false;
		}
		enumProc(itr->second.eventList);
		return true;
	}

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

protected:
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
	bool loadStop;
	bool loadForeground;
	bool initialLoadDone;
	int archivePeriodSec;

	//これらデータベースの読み取りにかぎりepgMapRefLockでアクセスできる。LoadThread以外では変更できない
	map<LONGLONG, EPGDB_SERVICE_EVENT_INFO> epgMap;
	map<LONGLONG, EPGDB_SERVICE_EVENT_INFO> epgArchive;
protected:
	static BOOL CALLBACK EnumEpgInfoListProc(DWORD epgInfoListSize, EPG_EVENT_INFO* epgInfoList, LPVOID param);
	static void LoadThread(CEpgDBManager* sys);

	void SearchEvent(const EPGDB_SEARCH_KEY_INFO* key, vector<SEARCH_RESULT_EVENT>& result, std::unique_ptr<IRegExp, decltype(&ComRelease)>& regExp) const;
	static bool IsEqualContent(const vector<EPGDB_CONTENT_DATA>& searchKey, const vector<EPGDB_CONTENT_DATA>& eventData);
	static bool IsInDateTime(const vector<EPGDB_SEARCH_DATE_INFO>& dateList, const SYSTEMTIME& time);
	static bool IsFindKeyword(bool regExpFlag, std::unique_ptr<IRegExp, decltype(&ComRelease)>& regExp,
	                          bool caseFlag, const vector<wstring>& keyList, const wstring& word, bool andMode, wstring* findKey = NULL);
	static bool IsFindLikeKeyword(bool caseFlag, const vector<wstring>& keyList, const wstring& word, vector<int>& dist, wstring* findKey = NULL);

};

