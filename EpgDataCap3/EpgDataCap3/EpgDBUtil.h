#pragma once

#include "./Table/TableUtil.h"
#include "../../Common/EpgDataCap3Def.h"

class CEpgDBUtil
{
public:
	CEpgDBUtil(void);
	~CEpgDBUtil(void);

	BOOL AddEIT(WORD PID, const CEITTable* eit, __int64 streamTime);

	BOOL AddServiceList(const CNITTable* nit);
	BOOL AddServiceList(WORD TSID, const CSITTable* sit);
	BOOL AddSDT(const CSDTTable* sdt);

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
		EPG_EVENT_INFO** epgInfoList
		);

	//指定サービスの全EPG情報を列挙する
	BOOL EnumEpgInfoList(
		WORD originalNetworkID,
		WORD transportStreamID,
		WORD serviceID,
		BOOL (CALLBACK *enumEpgInfoListProc)(DWORD, EPG_EVENT_INFO*, LPVOID),
		LPVOID param
		);

	//蓄積されたEPG情報のあるサービス一覧を取得する
	//SERVICE_EXT_INFOの情報はない場合がある
	//引数：
	// serviceListSize			[OUT]serviceListの個数
	// serviceList				[OUT]サービス情報のリスト（DLL内で自動的にdeleteする。次に取得を行うまで有効）
	void GetServiceListEpgDB(
		DWORD* serviceListSize,
		SERVICE_INFO** serviceList
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
		EPG_EVENT_INFO** epgInfo
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
		EPG_EVENT_INFO** epgInfo
		);

protected:
	CRITICAL_SECTION dbLock;

	struct SI_TAG{
		BYTE tableID;		//データ追加時のtable_id
		BYTE version;		//データ追加時のバージョン
		DWORD time;			//データのタイムスタンプ(単位は10秒)
	};
	struct EVENT_INFO : EPG_EVENT_INFO{
		//デストラクタは非仮想なので注意
		DWORD time;
		SI_TAG tagBasic;
		SI_TAG tagExt;
	};
	struct SECTION_FLAG_INFO{
		BYTE version;
		BYTE flags[32];			//セグメント(0～31)毎の受信済みセクション(0～7)のフラグ
		BYTE ignoreFlags[32];	//無視する(送出されない)セクションのフラグ
	};
	struct SERVICE_EVENT_INFO{
		map<WORD, std::unique_ptr<EVENT_INFO>> eventMap;
		std::unique_ptr<EVENT_INFO> nowEvent;
		std::unique_ptr<EVENT_INFO> nextEvent;
		BYTE lastTableID;
		BYTE lastTableIDExt;
		vector<SECTION_FLAG_INFO> sectionList;	//添え字はテーブル番号(0～7)
		vector<SECTION_FLAG_INFO> sectionExtList;
		SERVICE_EVENT_INFO(void){
			lastTableID = 0;
			lastTableIDExt = 0;
			sectionList.resize(8);
		}
		SERVICE_EVENT_INFO(SERVICE_EVENT_INFO&& o){
			*this = std::move(o);
		}
		SERVICE_EVENT_INFO& operator=(SERVICE_EVENT_INFO&& o){
			eventMap.swap(o.eventMap);
			nowEvent.swap(o.nowEvent);
			nextEvent.swap(o.nextEvent);
			lastTableID = o.lastTableID;
			lastTableIDExt = o.lastTableIDExt;
			sectionList.swap(o.sectionList);
			sectionExtList.swap(o.sectionExtList);
			return *this;
		}
	};
	map<ULONGLONG, SERVICE_EVENT_INFO> serviceEventMap;
	map<ULONGLONG, BYTE> serviceList;

	typedef struct _DB_SERVICE_INFO{
		WORD original_network_id;	//original_network_id
		WORD transport_stream_id;	//transport_stream_id
		WORD service_id;			//service_id
		BYTE service_type;
		BYTE partialReceptionFlag;
		wstring service_provider_name;
		wstring service_name;
		_DB_SERVICE_INFO(void){
			service_type = 0;
			partialReceptionFlag = FALSE;
			service_provider_name = L"";
			service_name = L"";
		};
	}DB_SERVICE_INFO;
	typedef struct _DB_TS_INFO{
		WORD original_network_id;	//original_network_id
		WORD transport_stream_id;	//transport_stream_id
		wstring network_name;
		wstring ts_name;
		BYTE remote_control_key_id;
		map<WORD,DB_SERVICE_INFO> serviceList;
		_DB_TS_INFO(void){
			network_name = L"";
			ts_name = L"";
			remote_control_key_id = 0;
		};
	}DB_TS_INFO;
	map<DWORD, DB_TS_INFO> serviceInfoList;

	std::unique_ptr<EPG_EVENT_INFO[]> epgInfoList;

	std::unique_ptr<EPG_EVENT_INFO> epgInfo;

	std::unique_ptr<EPG_EVENT_INFO> searchEpgInfo;

	std::unique_ptr<SERVICE_INFO[]> serviceDBList;
protected:
	void Clear();
	
	static void AddBasicInfo(EVENT_INFO* eventInfo, const vector<AribDescriptor::CDescriptor>* descriptorList, WORD onid, WORD tsid);
	static void AddShortEvent(EVENT_INFO* eventInfo, const AribDescriptor::CDescriptor* shortEvent);
	static BOOL AddExtEvent(EVENT_INFO* eventInfo, const vector<AribDescriptor::CDescriptor>* descriptorList);
	static void AddContent(EVENT_INFO* eventInfo, const AribDescriptor::CDescriptor* content);
	static void AddComponent(EVENT_INFO* eventInfo, const AribDescriptor::CDescriptor* component);
	static BOOL AddAudioComponent(EVENT_INFO* eventInfo, const vector<AribDescriptor::CDescriptor>* descriptorList);
	static void AddEventGroup(EVENT_INFO* eventInfo, const AribDescriptor::CDescriptor* eventGroup, WORD onid, WORD tsid);
	static void AddEventRelay(EVENT_INFO* eventInfo, const AribDescriptor::CDescriptor* eventGroup, WORD onid, WORD tsid);

	static BOOL CheckSectionAll(const vector<SECTION_FLAG_INFO>& sectionList);

	void CopyEpgInfo(EPG_EVENT_INFO* destInfo, EVENT_INFO* srcInfo);
};
