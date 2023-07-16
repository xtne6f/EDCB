#pragma once

#include "ARIB8CharDecode.h"
#include "AribDescriptor.h"
#include "../../Common/EpgDataCap3Def.h"
#include "../../Common/EpgTimerUtil.h"

class CEpgDBUtil
{
public:
	BOOL AddEIT(WORD PID, const AribDescriptor::CDescriptor& eit, LONGLONG streamTime);

	BOOL AddServiceListNIT(const AribDescriptor::CDescriptor& nit);
	BOOL AddServiceListSIT(WORD TSID, const AribDescriptor::CDescriptor& sit);
	BOOL AddSDT(const AribDescriptor::CDescriptor& sdt);

	void SetStreamChangeEvent();

	//EPGデータの蓄積状態をリセットする
	void ClearSectionStatus();

	//指定サービスのEPGデータの蓄積状態を取得する
	//戻り値：
	// ステータス
	//引数：
	// originalNetworkID		[IN]取得対象のOriginalNetworkID
	// transportStreamID		[IN]取得対象のTransportStreamID
	// serviceID				[IN]取得対象のServiceID
	// l_eitFlag				[IN]L-EITのステータスを取得
	EPG_SECTION_STATUS GetSectionStatusService(
		WORD originalNetworkID,
		WORD transportStreamID,
		WORD serviceID,
		BOOL l_eitFlag
		);

	//EPGデータの蓄積状態を取得する
	//戻り値：
	// ステータス
	//引数：
	// l_eitFlag		[IN]L-EITのステータスを取得
	EPG_SECTION_STATUS GetSectionStatus(BOOL l_eitFlag);

	//指定サービスの全EPG情報を取得する
	//引数：
	// originalNetworkID		[IN]取得対象のoriginalNetworkID
	// transportStreamID		[IN]取得対象のtransportStreamID
	// serviceID				[IN]取得対象のServiceID
	// epgInfoListSize			[OUT]epgInfoListの個数
	// epgInfoList				[OUT]EPG情報のリスト（DLL内で自動的にdeleteする。次に取得を行うまで有効）
	BOOL GetEpgInfoList(
		WORD originalNetworkID,
		WORD transportStreamID,
		WORD serviceID,
		DWORD* epgInfoListSize,
		EPG_EVENT_INFO** epgInfoList_
		);

	//指定サービスの全EPG情報を列挙する
	BOOL EnumEpgInfoList(
		WORD originalNetworkID,
		WORD transportStreamID,
		WORD serviceID,
		BOOL (CALLBACK *enumEpgInfoListProc)(DWORD, EPG_EVENT_INFO*, void*),
		void* param
		);

	//蓄積されたEPG情報のあるサービス一覧を取得する
	//SERVICE_EXT_INFOの情報はない場合がある
	//引数：
	// serviceListSize			[OUT]serviceListの個数
	// serviceList				[OUT]サービス情報のリスト（DLL内で自動的にdeleteする。次に取得を行うまで有効）
	void GetServiceListEpgDB(
		DWORD* serviceListSize,
		SERVICE_INFO** serviceList_
		);

	//指定サービスの現在or次のEPG情報を取得する
	//引数：
	// originalNetworkID		[IN]取得対象のoriginalNetworkID
	// transportStreamID		[IN]取得対象のtransportStreamID
	// serviceID				[IN]取得対象のServiceID
	// nextFlag					[IN]TRUE（次の番組）、FALSE（現在の番組）
	// epgInfo					[OUT]EPG情報（DLL内で自動的にdeleteする。次に取得を行うまで有効）
	BOOL GetEpgInfo(
		WORD originalNetworkID,
		WORD transportStreamID,
		WORD serviceID,
		BOOL nextFlag,
		EPG_EVENT_INFO** epgInfo_
		);

	//指定イベントのEPG情報を取得する
	//引数：
	// originalNetworkID		[IN]取得対象のoriginalNetworkID
	// transportStreamID		[IN]取得対象のtransportStreamID
	// serviceID				[IN]取得対象のServiceID
	// EventID					[IN]取得対象のEventID
	// pfOnlyFlag				[IN]p/fからのみ検索するかどうか
	// epgInfo					[OUT]EPG情報（DLL内で自動的にdeleteする。次に取得を行うまで有効）
	BOOL SearchEpgInfo(
		WORD originalNetworkID,
		WORD transportStreamID,
		WORD serviceID,
		WORD eventID,
		BYTE pfOnlyFlag,
		EPG_EVENT_INFO** epgInfo_
		);

protected:
	struct SI_TAG{
		BYTE tableID;		//データ追加時のtable_id
		BYTE version;		//データ追加時のバージョン
		DWORD time;			//データのタイムスタンプ(単位は10秒)
	};
	struct EVENT_INFO{
		DWORD time;
		SI_TAG tagBasic;
		SI_TAG tagExt;
		EPGDB_EVENT_INFO db;	//ONID,TSID,SIDは未使用
	};
	struct SECTION_FLAG_INFO{
		BYTE version;
		BYTE flags[32];			//セグメント(0～31)毎の受信済みセクション(0～7)のフラグ
		BYTE ignoreFlags[32];	//無視する(送出されない)セクションのフラグ
	};
	struct SERVICE_EVENT_INFO{
		map<WORD, EVENT_INFO> eventMap;
		vector<EVENT_INFO> nowEvent;
		vector<EVENT_INFO> nextEvent;
		BYTE lastTableID;
		BYTE lastTableIDExt;
		SECTION_FLAG_INFO sectionList[8];	//添え字はテーブル番号(0～7)
		SECTION_FLAG_INFO sectionExtList[8];
		SERVICE_EVENT_INFO(void){
			lastTableID = 0;
			lastTableIDExt = 0;
		}
	};
	map<ULONGLONG, SERVICE_EVENT_INFO> serviceEventMap;
	map<ULONGLONG, BYTE> serviceList;

	struct DB_TS_INFO{
		wstring network_name;
		wstring ts_name;
		BYTE remote_control_key_id;
		map<WORD, EPGDB_SERVICE_INFO> serviceList;	//network_name,ts_name,remote_control_key_idは未使用
	};
	map<DWORD, DB_TS_INFO> serviceInfoList;

	class CEventInfoStore
	{
	public:
		void Update() {
			data.reset(new EPG_EVENT_INFO[db.size()]);
			adapter.reset(new CEpgEventInfoAdapter[db.size()]);
			for( size_t i = 0; i < db.size(); i++ ){
				data[i] = adapter[i].Create(&db[i]);
			}
		}
		vector<EPGDB_EVENT_INFO> db;
		std::unique_ptr<EPG_EVENT_INFO[]> data;
	private:
		std::unique_ptr<CEpgEventInfoAdapter[]> adapter;
	};
	CEventInfoStore epgInfoList;

	CEventInfoStore epgInfo;

	CEventInfoStore searchEpgInfo;

	std::unique_ptr<SERVICE_INFO[]> serviceDataList;
	std::unique_ptr<EPGDB_SERVICE_INFO[]> serviceDBList;
	std::unique_ptr<CServiceInfoAdapter[]> serviceAdapterList;

	CARIB8CharDecode arib;
protected:
	void Clear();

	int GetCaptionInfo(const AribDescriptor::CDescriptor& eit, AribDescriptor::CDescriptor::CLoopPointer lp);
	void AddBasicInfo(EPGDB_EVENT_INFO* eventInfo, const AribDescriptor::CDescriptor& eit, AribDescriptor::CDescriptor::CLoopPointer lpParent, WORD onid, WORD tsid);
	void AddShortEvent(EPGDB_EVENT_INFO* eventInfo, const AribDescriptor::CDescriptor& eit, AribDescriptor::CDescriptor::CLoopPointer lp);
	static void EscapeHyphenSpaceAndAppend(wstring& text, const wstring& appendText);
	BOOL AddExtEvent(EPGDB_EVENT_INFO* eventInfo, const AribDescriptor::CDescriptor& eit, AribDescriptor::CDescriptor::CLoopPointer lpParent);
	static void AddContent(EPGDB_EVENT_INFO* eventInfo, const AribDescriptor::CDescriptor& eit, AribDescriptor::CDescriptor::CLoopPointer lp);
	void AddComponent(EPGDB_EVENT_INFO* eventInfo, const AribDescriptor::CDescriptor& eit, AribDescriptor::CDescriptor::CLoopPointer lp);
	BOOL AddAudioComponent(EPGDB_EVENT_INFO* eventInfo, const AribDescriptor::CDescriptor& eit, AribDescriptor::CDescriptor::CLoopPointer lpParent);
	static void AddEventGroup(EPGDB_EVENT_INFO* eventInfo, const AribDescriptor::CDescriptor& eit, AribDescriptor::CDescriptor::CLoopPointer lp, WORD onid, WORD tsid);
	static void AddEventRelay(EPGDB_EVENT_INFO* eventInfo, const AribDescriptor::CDescriptor& eit, AribDescriptor::CDescriptor::CLoopPointer lp, WORD onid, WORD tsid);

	static BOOL CheckSectionAll(const SECTION_FLAG_INFO (&sectionList)[8]);
};
