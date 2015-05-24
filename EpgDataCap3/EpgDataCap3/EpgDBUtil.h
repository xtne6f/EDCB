#pragma once

#include "../../Common/Util.h"
#include "./Table/TableUtil.h"
#include "../../Common/EpgDataCap3Def.h"

class CEpgDBUtil
{
public:
	CEpgDBUtil(void);
	~CEpgDBUtil(void);

	BOOL AddEIT(WORD PID, CEITTable* eit, __int64 streamTime);

	BOOL AddServiceList(CNITTable* nit);
	BOOL AddServiceList(WORD TSID, CSITTable* sit);
	BOOL AddSDT(CSDTTable* sdt);

	void SetStreamChangeEvent();

	//EPGデータの蓄積状態をリセットする
	void ClearSectionStatus();

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
	struct SERVICE_EVENT_INFO{
		map<WORD, EVENT_INFO*> eventMap;
		EVENT_INFO* nowEvent;
		EVENT_INFO* nextEvent;
		SERVICE_EVENT_INFO(void){
			nowEvent = NULL;
			nextEvent = NULL;
		}
	};
	struct SECTION_FLAG_INFO{
		DWORD sectionFlag;
		DWORD maxFlag;
		SECTION_FLAG_INFO(void){
			sectionFlag = 0;
			maxFlag = 0;
		}
	};
	struct SECTION_STATUS_INFO{
		BYTE HEITFlag;
		BYTE last_section_numberBasic;
		BYTE last_table_idBasic;
		BYTE last_section_numberExt;
		BYTE last_table_idExt;
		map<WORD, SECTION_FLAG_INFO> sectionBasicMap;	//キー TableID、フラグ
		map<WORD, SECTION_FLAG_INFO> sectionExtMap;	//キー TableID、フラグ
		SECTION_STATUS_INFO(void){
			HEITFlag = TRUE;
			last_section_numberBasic = 0;
			last_table_idBasic = 0;
			last_section_numberExt = 0;
			last_table_idExt = 0;
		}
	};
	map<ULONGLONG, SERVICE_EVENT_INFO> serviceEventMap;
	map<ULONGLONG, SECTION_STATUS_INFO> sectionMap;
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
		~_DB_SERVICE_INFO(void){
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
		~_DB_TS_INFO(void){
		};
	}DB_TS_INFO;
	map<DWORD, DB_TS_INFO> serviceInfoList;

	DWORD sectionNowFlag;

	EPG_EVENT_INFO* epgInfoList;

	EPG_EVENT_INFO* epgInfo;

	EPG_EVENT_INFO* searchEpgInfo;

	SERVICE_INFO* serviceDBList;
protected:
	void Clear();
	
	static void AddBasicInfo(EVENT_INFO* eventInfo, vector<AribDescriptor::CDescriptor*>* descriptorList, WORD onid, WORD tsid);
	static void AddShortEvent(EVENT_INFO* eventInfo, AribDescriptor::CDescriptor* shortEvent);
	static BOOL AddExtEvent(EVENT_INFO* eventInfo, vector<AribDescriptor::CDescriptor*>* descriptorList);
	static void AddContent(EVENT_INFO* eventInfo, AribDescriptor::CDescriptor* content);
	static void AddComponent(EVENT_INFO* eventInfo, AribDescriptor::CDescriptor* component);
	static BOOL AddAudioComponent(EVENT_INFO* eventInfo, vector<AribDescriptor::CDescriptor*>* descriptorList);
	static void AddEventGroup(EVENT_INFO* eventInfo, AribDescriptor::CDescriptor* eventGroup, WORD onid, WORD tsid);
	static void AddEventRelay(EVENT_INFO* eventInfo, AribDescriptor::CDescriptor* eventGroup, WORD onid, WORD tsid);

	BOOL CheckSectionAll(map<WORD, SECTION_FLAG_INFO>* sectionMap, BOOL leitFlag = FALSE);

	void CopyEpgInfo(EPG_EVENT_INFO* destInfo, EVENT_INFO* srcInfo);
};
