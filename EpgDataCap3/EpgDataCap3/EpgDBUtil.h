#pragma once

#include "../../Common/Util.h"
#include "./Table/TableUtil.h"
#include "../../Common/EpgDataCap3Def.h"

typedef struct _NIBBLE_DATA{
	BYTE content_nibble_level_1;
	BYTE content_nibble_level_2;
	BYTE user_nibble_1;
	BYTE user_nibble_2;
}NIBBLE_DATA;

typedef struct _EVENT_DATA2{
	WORD original_network_id;
	WORD transport_stream_id;
	WORD service_id;
	WORD event_id;
}EVENT_DATA2;

typedef struct _SHORT_EVENT_INFO{
	BYTE tableID;		//データ追加時のtable_id （優先度 0x4E > 0x50-0x5F > 0x4F > 0x60-0x6F)
	BYTE version;		//データ追加時のバージョン
	wstring event_name;
	wstring text_char;
} SHORT_EVENT_INFO;

typedef struct _EXTENDED_EVENT_INFO{
	BYTE tableID;		//データ追加時のtable_id （優先度 0x4E > 0x50-0x5F > 0x4F > 0x60-0x6F)
	BYTE version;		//データ追加時のバージョン
	wstring text_char;
} EXTENDED_EVENT_INFO;

typedef struct _CONTENT_INFO{
	BYTE tableID;		//データ追加時のtable_id （優先度 0x4E > 0x50-0x5F > 0x4F > 0x60-0x6F)
	BYTE version;		//データ追加時のバージョン
	vector<NIBBLE_DATA> nibbleList;
} CONTEN_INFO;

typedef struct _COMPONENT_INFO{
	BYTE tableID;		//データ追加時のtable_id （優先度 0x4E > 0x50-0x5F > 0x4F > 0x60-0x6F)
	BYTE version;		//データ追加時のバージョン
	BYTE stream_content;
	BYTE component_type;
	BYTE component_tag;
	wstring text_char;
} COMPONENT_INFO;

typedef struct _AUDIO_COMPONENT_INFO_DATA{
	BYTE stream_content;
	BYTE component_type;
	BYTE component_tag;
	BYTE stream_type;
	BYTE simulcast_group_tag;
	BYTE ES_multi_lingual_flag;
	BYTE main_component_flag;
	BYTE quality_indicator;
	BYTE sampling_rate;
	wstring text_char;
}AUDIO_COMPONENT_INFO_DATA;

typedef struct _AUDIO_COMPONENT_INFO{
	BYTE tableID;		//データ追加時のtable_id （優先度 0x4E > 0x50-0x5F > 0x4F > 0x60-0x6F)
	BYTE version;		//データ追加時のバージョン
	vector<AUDIO_COMPONENT_INFO_DATA> componentList;
} AUDIO_COMPONENT_INFO;

typedef struct _EVENTGROUP_INFO{
	BYTE tableID;		//データ追加時のtable_id （優先度 0x4E > 0x50-0x5F > 0x4F > 0x60-0x6F)
	BYTE version;		//データ追加時のバージョン
	BYTE group_type;
	BYTE event_count;
	vector<EVENT_DATA2> eventData2List;
} EVENTGROUP_INFO;

typedef struct _EVENT_INFO{
	WORD ONID;
	WORD TSID;
	WORD SID;
	WORD event_id;
	BYTE StartTimeFlag;	//start_timeの値が有効かどうか
	SYSTEMTIME start_time;
	BYTE DurationFlag; //durationの値が有効かどうか
	DWORD durationSec;

	SHORT_EVENT_INFO* shortInfo;
	EXTENDED_EVENT_INFO* extInfo;
	CONTEN_INFO* contentInfo;
	COMPONENT_INFO* componentInfo;
	AUDIO_COMPONENT_INFO* audioInfo;
	EVENTGROUP_INFO* eventGroupInfo;
	EVENTGROUP_INFO* eventRelayInfo;

	BYTE freeCAFlag;
	BOOL pfFlag;
	_EVENT_INFO(void){
		shortInfo = NULL;
		extInfo = NULL;
		contentInfo = NULL;
		componentInfo = NULL;
		audioInfo = NULL;
		eventGroupInfo = NULL;
		eventRelayInfo = NULL;
		freeCAFlag = 0;
		pfFlag = FALSE;
	};
	~_EVENT_INFO(void){
		SAFE_DELETE(shortInfo);
		SAFE_DELETE(extInfo);
		SAFE_DELETE(contentInfo);
		SAFE_DELETE(componentInfo);
		SAFE_DELETE(audioInfo);
		SAFE_DELETE(eventGroupInfo);
		SAFE_DELETE(eventRelayInfo);
	};
} EVENT_INFO;

typedef struct _SERVICE_EVENT_INFO{
	map<WORD, EVENT_INFO*> eventMap;
	EVENT_INFO* nowEvent;	//map側でdeleteされるのでdeleteする必要なし
	EVENT_INFO* nextEvent;	//map側でdeleteされるのでdeleteする必要なし
	_SERVICE_EVENT_INFO(void){
		nowEvent = NULL;
		nextEvent = NULL;
	};
	~_SERVICE_EVENT_INFO(void){
		map<WORD, EVENT_INFO*>::iterator itr;
		for( itr = eventMap.begin(); itr != eventMap.end(); itr++ ){
			SAFE_DELETE(itr->second);
		}
	};
} SERVICE_EVENT_INFO;

typedef struct _SECTION_FLAG_INFO{
	DWORD sectionFlag;
	DWORD maxFlag;
	_SECTION_FLAG_INFO(void){
		sectionFlag = 0;
		maxFlag = 0;
	};
}SECTION_FLAG_INFO;

typedef struct _SECTION_STATUS_INFO{
	BYTE HEITFlag;
	BYTE last_section_numberBasic;
	BYTE last_table_idBasic;
	BYTE last_section_numberExt;
	BYTE last_table_idExt;
	map<WORD, SECTION_FLAG_INFO> sectionBasicMap;	//キー TableID、フラグ
	map<WORD, SECTION_FLAG_INFO> sectionExtMap;	//キー TableID、フラグ
	_SECTION_STATUS_INFO(void){
		HEITFlag = TRUE;
		last_section_numberBasic = 0;
		last_table_idBasic = 0;
		last_section_numberExt = 0;
		last_table_idExt = 0;
	};
}SECTION_STATUS_INFO;

class CEpgDBUtil
{
public:
	CEpgDBUtil(void);
	~CEpgDBUtil(void);

	BOOL AddEIT(WORD PID, CEITTable* eit);
	BOOL AddEIT_SD(WORD PID, CEITTable_SD* eit);
	BOOL AddEIT_SD2(WORD PID, CEITTable_SD2* eit);

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
	// nowTime					[IN]現在の時間
	// epgInfo					[OUT]EPG情報（DLL内で自動的にdeleteする。次に取得を行うまで有効）
	BOOL GetEpgInfo(
		WORD originalNetworkID,
		WORD transportStreamID,
		WORD serviceID,
		BOOL nextFlag,
		SYSTEMTIME nowTime,
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

	map<ULONGLONG, SERVICE_EVENT_INFO*> serviceEventMap;
	map<ULONGLONG, SECTION_STATUS_INFO*> sectionMap;
	map<ULONGLONG, BYTE> serviceList;

	map<ULONGLONG, SERVICE_EVENT_INFO*> serviceEventMapSD;


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
		map<WORD,DB_SERVICE_INFO*> serviceList;
		_DB_TS_INFO(void){
			network_name = L"";
			ts_name = L"";
			remote_control_key_id = 0;
		};
		~_DB_TS_INFO(void){
			map<WORD,DB_SERVICE_INFO*>::iterator itr;
			for( itr=serviceList.begin(); itr != serviceList.end(); itr++ ){
				SAFE_DELETE(itr->second);
			}
			serviceList.clear();
		};
	}DB_TS_INFO;
	map<DWORD, DB_TS_INFO*> serviceInfoList;

	DWORD sectionNowFlag;

	DWORD epgInfoListSize;
	EPG_EVENT_INFO* epgInfoList;

	EPG_EVENT_INFO* epgInfo;

	EPG_EVENT_INFO* searchEpgInfo;

	DWORD epgSearchListSize;
	EPG_EVENT_INFO* epgSearchList;

	DWORD serviceDBListSize;
	SERVICE_INFO* serviceDBList;
protected:
	void Clear();
	
	static BOOL AddShortEvent(BYTE table_id, BYTE version_number, EVENT_INFO* eventInfo, AribDescriptor::CDescriptor* shortEvent, BOOL skySDFlag);
	static BOOL AddExtEvent(BYTE table_id, BYTE version_number, EVENT_INFO* eventInfo, vector<AribDescriptor::CDescriptor*>* descriptorList, BOOL skySDFlag);
	static BOOL AddContent(BYTE table_id, BYTE version_number, EVENT_INFO* eventInfo, AribDescriptor::CDescriptor* content, BOOL skySDFlag);
	static BOOL AddComponent(BYTE table_id, BYTE version_number, EVENT_INFO* eventInfo, AribDescriptor::CDescriptor* component, BOOL skySDFlag);
	static BOOL AddAudioComponent(BYTE table_id, BYTE version_number, EVENT_INFO* eventInfo, vector<AribDescriptor::CDescriptor*>* descriptorList, BOOL skySDFlag);
	static BOOL AddEventGroup(CEITTable* eit, EVENT_INFO* eventInfo, AribDescriptor::CDescriptor* eventGroup);
	static BOOL AddEventRelay(CEITTable* eit, EVENT_INFO* eventInfo, AribDescriptor::CDescriptor* eventGroup);
	static BOOL CheckUpdate(BYTE eit_table_id, BYTE eit_version_number, BYTE tableID, BYTE version, BOOL skySDFlag);

	static BOOL CheckUpdate_SD(BYTE eit_table_id, BYTE eit_version_number, BYTE tableID, BYTE version);

	BOOL AddSDEventMap(CEITTable_SD* eit);

	BOOL CheckSectionAll(map<WORD, SECTION_FLAG_INFO>* sectionMap, BOOL leitFlag = FALSE);

	void CopyEpgInfo(EPG_EVENT_INFO* destInfo, EVENT_INFO* srcInfo);
};
