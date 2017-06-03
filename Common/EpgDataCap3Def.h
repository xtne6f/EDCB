#ifndef __EPG_DATA_CAP3_DEF_H__
#define __EPG_DATA_CAP3_DEF_H__

//EPGデータ取得ステータス
typedef enum{
	EpgNoData			= 0x0000,	//データがない
	EpgNeedData			= 0x0001,	//たまっていない
	EpgBasicAll			= 0x0010,	//基本情報はたまった
	EpgExtendAll		= 0x0020,	//拡張情報はたまった
	EpgHEITAll			= 0x0040,	//基本、拡張共にたまった
	EpgLEITAll			= 0x0080,	//ワンセグがたまった
} EPG_SECTION_STATUS;

//EPG基本情報
typedef struct {
	WORD event_nameLength;		//event_nameの文字数
	LPCWSTR event_name;			//イベント名
	WORD text_charLength;		//text_charの文字数
	LPCWSTR text_char;			//情報
} EPG_SHORT_EVENT_INFO;

//EPG拡張情報
typedef struct {
	WORD text_charLength;		//text_charの文字数
	LPCWSTR text_char;			//詳細情報
} EPG_EXTENDED_EVENT_INFO;

//EPGジャンルデータ
typedef struct {
	BYTE content_nibble_level_1;
	BYTE content_nibble_level_2;
	BYTE user_nibble_1;
	BYTE user_nibble_2;
}EPG_CONTENT;

//EPGジャンル情報
typedef struct {
	WORD listSize;
	EPG_CONTENT* nibbleList;
} EPG_CONTEN_INFO;

//EPG映像情報
typedef struct {
	BYTE stream_content;
	BYTE component_type;
	BYTE component_tag;
	WORD text_charLength;		//text_charの文字数
	LPCWSTR text_char;			//詳細情報
} EPG_COMPONENT_INFO;

//EPG音声情報
typedef struct {
	BYTE stream_content;
	BYTE component_type;
	BYTE component_tag;
	BYTE stream_type;
	BYTE simulcast_group_tag;
	BYTE ES_multi_lingual_flag;
	BYTE main_component_flag;
	BYTE quality_indicator;
	BYTE sampling_rate;
	WORD text_charLength;		//text_charの文字数
	LPCWSTR text_char;			//詳細情報
} EPG_AUDIO_COMPONENT_INFO_DATA;

//EPG音声情報
typedef struct {
	WORD listSize;
	EPG_AUDIO_COMPONENT_INFO_DATA* audioList;
} EPG_AUDIO_COMPONENT_INFO;

//EPGイベントデータ
typedef struct {
	WORD original_network_id;
	WORD transport_stream_id;
	WORD service_id;
	WORD event_id;
}EPG_EVENT_DATA;

//EPGイベントグループ情報
typedef struct {
	BYTE group_type;
	BYTE event_count;
	EPG_EVENT_DATA* eventDataList;
} EPG_EVENTGROUP_INFO;

typedef struct {
	WORD event_id;							//イベントID
	BYTE StartTimeFlag;						//start_timeの値が有効かどうか
	SYSTEMTIME start_time;					//開始時間
	BYTE DurationFlag;						//durationの値が有効かどうか
	DWORD durationSec;						//総時間（単位：秒）

	EPG_SHORT_EVENT_INFO* shortInfo;		//基本情報
	EPG_EXTENDED_EVENT_INFO* extInfo;		//拡張情報
	EPG_CONTEN_INFO* contentInfo;			//ジャンル情報
	EPG_COMPONENT_INFO* componentInfo;		//映像情報
	EPG_AUDIO_COMPONENT_INFO* audioInfo;	//音声情報
	EPG_EVENTGROUP_INFO* eventGroupInfo;	//イベントグループ情報
	EPG_EVENTGROUP_INFO* eventRelayInfo;	//イベントリレー情報

	BYTE freeCAFlag;						//ノンスクランブルフラグ
}EPG_EVENT_INFO;

//サービスの詳細情報
typedef struct {
	BYTE service_type;
	BYTE partialReceptionFlag;
	LPCWSTR service_provider_name;
	LPCWSTR service_name;
	LPCWSTR network_name;
	LPCWSTR ts_name;
	BYTE remote_control_key_id;
}SERVICE_EXT_INFO;

//サービス情報
typedef struct {
	WORD original_network_id;	//original_network_id
	WORD transport_stream_id;	//transport_stream_id
	WORD service_id;			//service_id
	SERVICE_EXT_INFO* extInfo;	//詳細情報
}SERVICE_INFO;

#endif
