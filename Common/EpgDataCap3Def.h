#ifndef __EPG_DATA_CAP3_DEF_H__
#define __EPG_DATA_CAP3_DEF_H__

#include "Util.h"

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
typedef struct _EPG_SHORT_EVENT_INFO{
	WORD event_nameLength;		//event_nameの文字数
	WCHAR* event_name;			//イベント名
	WORD text_charLength;		//text_charの文字数
	WCHAR* text_char;			//情報
	_EPG_SHORT_EVENT_INFO(void){
		event_name = NULL;
		text_char = NULL;
	};
	~_EPG_SHORT_EVENT_INFO(void){
		SAFE_DELETE_ARRAY(event_name);
		SAFE_DELETE_ARRAY(text_char);
	};
	void DeepCopy(const _EPG_SHORT_EVENT_INFO& o){
		event_nameLength = o.event_nameLength;
		SAFE_DELETE_ARRAY(event_name);
		if( o.event_name ){
			event_name = new WCHAR[event_nameLength + 1];
			memcpy(event_name, o.event_name, sizeof(WCHAR) * (event_nameLength + 1));
		}
		text_charLength = o.text_charLength;
		SAFE_DELETE_ARRAY(text_char);
		if( o.text_char ){
			text_char = new WCHAR[text_charLength + 1];
			memcpy(text_char, o.text_char, sizeof(WCHAR) * (text_charLength + 1));
		}
	};
private:
	_EPG_SHORT_EVENT_INFO(const _EPG_SHORT_EVENT_INFO&);
	_EPG_SHORT_EVENT_INFO& operator=(const _EPG_SHORT_EVENT_INFO&);
} EPG_SHORT_EVENT_INFO;

//EPG拡張情報
typedef struct _EPG_EXTENDED_EVENT_INFO{
	WORD text_charLength;		//text_charの文字数
	WCHAR* text_char;			//詳細情報
	_EPG_EXTENDED_EVENT_INFO(void){
		text_char = NULL;
	};
	~_EPG_EXTENDED_EVENT_INFO(void){
		SAFE_DELETE_ARRAY(text_char);
	};
	void DeepCopy(const _EPG_EXTENDED_EVENT_INFO& o){
		text_charLength = o.text_charLength;
		SAFE_DELETE_ARRAY(text_char);
		if( o.text_char ){
			text_char = new WCHAR[text_charLength + 1];
			memcpy(text_char, o.text_char, sizeof(WCHAR) * (text_charLength + 1));
		}
	};
private:
	_EPG_EXTENDED_EVENT_INFO(const _EPG_EXTENDED_EVENT_INFO&);
	_EPG_EXTENDED_EVENT_INFO& operator=(const _EPG_EXTENDED_EVENT_INFO&);
} EPG_EXTENDED_EVENT_INFO;

//EPGジャンルデータ
typedef struct _EPG_CONTENT_DATA{
	BYTE content_nibble_level_1;
	BYTE content_nibble_level_2;
	BYTE user_nibble_1;
	BYTE user_nibble_2;
}EPG_CONTENT;

//EPGジャンル情報
typedef struct _EPG_CONTENT_INFO{
	WORD listSize;
	EPG_CONTENT* nibbleList;
	_EPG_CONTENT_INFO(void){
		listSize = 0;
		nibbleList = NULL;
	};
	~_EPG_CONTENT_INFO(void){
		SAFE_DELETE_ARRAY(nibbleList);
	};
	void DeepCopy(const _EPG_CONTENT_INFO& o){
		listSize = o.listSize;
		SAFE_DELETE_ARRAY(nibbleList);
		if( o.nibbleList ){
			nibbleList = new EPG_CONTENT[listSize];
			memcpy(nibbleList, o.nibbleList, sizeof(EPG_CONTENT) * listSize);
		}
	};
private:
	_EPG_CONTENT_INFO(const _EPG_CONTENT_INFO&);
	_EPG_CONTENT_INFO& operator=(const _EPG_CONTENT_INFO&);
} EPG_CONTEN_INFO;

//EPG映像情報
typedef struct _EPG_COMPONENT_INFO{
	BYTE stream_content;
	BYTE component_type;
	BYTE component_tag;
	WORD text_charLength;		//text_charの文字数
	WCHAR* text_char;			//詳細情報
	_EPG_COMPONENT_INFO(void){
		text_char = NULL;
	};
	~_EPG_COMPONENT_INFO(void){
		SAFE_DELETE_ARRAY(text_char);
	};
	void DeepCopy(const _EPG_COMPONENT_INFO& o){
		stream_content = o.stream_content;
		component_type = o.component_type;
		component_tag = o.component_tag;
		text_charLength = o.text_charLength;
		SAFE_DELETE_ARRAY(text_char);
		if( o.text_char ){
			text_char = new WCHAR[text_charLength + 1];
			memcpy(text_char, o.text_char, sizeof(WCHAR) * (text_charLength + 1));
		}
	};
private:
	_EPG_COMPONENT_INFO(const _EPG_COMPONENT_INFO&);
	_EPG_COMPONENT_INFO& operator=(const _EPG_COMPONENT_INFO&);
} EPG_COMPONENT_INFO;

//EPG音声情報
typedef struct _EPG_AUDIO_COMPONENT_INFO_DATA{
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
	WCHAR* text_char;			//詳細情報
	_EPG_AUDIO_COMPONENT_INFO_DATA(void){
		text_char = NULL;
	};
	~_EPG_AUDIO_COMPONENT_INFO_DATA(void){
		SAFE_DELETE_ARRAY(text_char);
	};
	void DeepCopy(const _EPG_AUDIO_COMPONENT_INFO_DATA& o){
		stream_content = o.stream_content;
		component_type = o.component_type;
		component_tag = o.component_tag;
		stream_type = o.stream_type;
		simulcast_group_tag = o.simulcast_group_tag;
		ES_multi_lingual_flag = o.ES_multi_lingual_flag;
		main_component_flag = o.main_component_flag;
		quality_indicator = o.quality_indicator;
		sampling_rate = o.sampling_rate;
		text_charLength = o.text_charLength;
		SAFE_DELETE_ARRAY(text_char);
		if( o.text_char ){
			text_char = new WCHAR[text_charLength + 1];
			memcpy(text_char, o.text_char, sizeof(WCHAR) * (text_charLength + 1));
		}
	};
private:
	_EPG_AUDIO_COMPONENT_INFO_DATA(const _EPG_AUDIO_COMPONENT_INFO_DATA&);
	_EPG_AUDIO_COMPONENT_INFO_DATA& operator=(const _EPG_AUDIO_COMPONENT_INFO_DATA&);
} EPG_AUDIO_COMPONENT_INFO_DATA;

//EPG音声情報
typedef struct _EPG_AUDIO_COMPONENT_INFO{
	WORD listSize;
	EPG_AUDIO_COMPONENT_INFO_DATA* audioList;
	_EPG_AUDIO_COMPONENT_INFO(void){
		listSize = 0;
		audioList = NULL;
	};
	~_EPG_AUDIO_COMPONENT_INFO(void){
		SAFE_DELETE_ARRAY(audioList);
	};
	void DeepCopy(const _EPG_AUDIO_COMPONENT_INFO& o){
		listSize = o.listSize;
		SAFE_DELETE_ARRAY(audioList);
		if( o.audioList ){
			audioList = new EPG_AUDIO_COMPONENT_INFO_DATA[listSize];
			for( WORD i = 0; i < listSize; i++ ){
				audioList[i].DeepCopy(o.audioList[i]);
			}
		}
	};
private:
	_EPG_AUDIO_COMPONENT_INFO(const _EPG_AUDIO_COMPONENT_INFO&);
	_EPG_AUDIO_COMPONENT_INFO& operator=(const _EPG_AUDIO_COMPONENT_INFO&);
} EPG_AUDIO_COMPONENT_INFO;

//EPGイベントデータ
typedef struct _EPG_EVENT_DATA{
	WORD original_network_id;
	WORD transport_stream_id;
	WORD service_id;
	WORD event_id;
}EPG_EVENT_DATA;

//EPGイベントグループ情報
typedef struct _EPG_EVENTGROUP_INFO{
	BYTE group_type;
	BYTE event_count;
	EPG_EVENT_DATA* eventDataList;
	_EPG_EVENTGROUP_INFO(void){
		eventDataList = NULL;
	};
	~_EPG_EVENTGROUP_INFO(void){
		SAFE_DELETE_ARRAY(eventDataList);
	};
	void DeepCopy(const _EPG_EVENTGROUP_INFO& o){
		group_type = o.group_type;
		event_count = o.event_count;
		SAFE_DELETE_ARRAY(eventDataList);
		if( o.eventDataList ){
			eventDataList = new EPG_EVENT_DATA[event_count];
			memcpy(eventDataList, o.eventDataList, sizeof(EPG_EVENT_DATA) * event_count);
		}
	};
private:
	_EPG_EVENTGROUP_INFO(const _EPG_EVENTGROUP_INFO&);
	_EPG_EVENTGROUP_INFO& operator=(const _EPG_EVENTGROUP_INFO&);
} EPG_EVENTGROUP_INFO;

typedef struct _EPG_EVENT_INFO{
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
	_EPG_EVENT_INFO(void){
		shortInfo = NULL;
		extInfo = NULL;
		contentInfo = NULL;
		componentInfo = NULL;
		audioInfo = NULL;
		eventGroupInfo = NULL;
		eventRelayInfo = NULL;
		freeCAFlag = 0;
	};
	~_EPG_EVENT_INFO(void){
		SAFE_DELETE(shortInfo);
		SAFE_DELETE(extInfo);
		SAFE_DELETE(contentInfo);
		SAFE_DELETE(componentInfo);
		SAFE_DELETE(audioInfo);
		SAFE_DELETE(eventGroupInfo);
		SAFE_DELETE(eventRelayInfo);
	};
private:
	_EPG_EVENT_INFO(const _EPG_EVENT_INFO&);
	_EPG_EVENT_INFO& operator=(const _EPG_EVENT_INFO&);
}EPG_EVENT_INFO;

//サービスの詳細情報
typedef struct _SERVICE_EXT_INFO{
	BYTE service_type;
	BYTE partialReceptionFlag;
	WCHAR* service_provider_name;
	WCHAR* service_name;
	WCHAR* network_name;
	WCHAR* ts_name;
	BYTE remote_control_key_id;
	_SERVICE_EXT_INFO(void){
		service_type = 0;
		partialReceptionFlag = FALSE;
		service_provider_name = NULL;
		service_name = NULL;
		network_name = NULL;
		ts_name = NULL;
		remote_control_key_id = 0;
	};
	~_SERVICE_EXT_INFO(void){
		SAFE_DELETE_ARRAY(service_provider_name);
		SAFE_DELETE_ARRAY(service_name);
		SAFE_DELETE_ARRAY(network_name);
		SAFE_DELETE_ARRAY(ts_name);
	};
private:
	_SERVICE_EXT_INFO(const _SERVICE_EXT_INFO&);
	_SERVICE_EXT_INFO& operator=(const _SERVICE_EXT_INFO&);
}SERVICE_EXT_INFO;

//サービス情報
typedef struct _SERVICE_INFO{
	WORD original_network_id;	//original_network_id
	WORD transport_stream_id;	//transport_stream_id
	WORD service_id;			//service_id
	SERVICE_EXT_INFO* extInfo;	//詳細情報
	_SERVICE_INFO(void){
		extInfo = NULL;
	};
	~_SERVICE_INFO(void){
		SAFE_DELETE(extInfo);
	};
private:
	_SERVICE_INFO(const _SERVICE_INFO&);
	_SERVICE_INFO& operator=(const _SERVICE_INFO&);
}SERVICE_INFO;

#endif
