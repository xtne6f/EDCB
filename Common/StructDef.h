#ifndef __STRUCT_DEF_H__
#define __STRUCT_DEF_H__

#include "Util.h"

//録画フォルダ情報
typedef struct _REC_FILE_SET_INFO{
	wstring recFolder;			//録画フォルダ
	wstring writePlugIn;		//出力PlugIn
	wstring recNamePlugIn;		//ファイル名変換PlugInの使用
	wstring recFileName;		//ファイル名個別対応 録画開始処理時に内部で使用。予約情報としては必要なし
}REC_FILE_SET_INFO;

//録画設定情報
typedef struct _REC_SETTING_DATA{
	BYTE recMode;				//録画モード
	BYTE priority;				//優先度
	BYTE tuijyuuFlag;			//追従モード
	DWORD serviceMode;			//処理対象データモード
	BYTE pittariFlag;			//ぴったり？録画
	wstring batFilePath;		//録画後BATファイルパス
	vector<REC_FILE_SET_INFO> recFolderList;		//録画フォルダパス
	BYTE suspendMode;			//休止モード
	BYTE rebootFlag;			//録画後再起動する
	BYTE useMargineFlag;		//録画マージンを個別指定
	INT startMargine;			//録画開始時のマージン
	INT endMargine;				//録画終了時のマージン
	BYTE continueRecFlag;		//後続同一サービス時、同一ファイルで録画
	BYTE partialRecFlag;		//物理CHに部分受信サービスがある場合、同時録画するかどうか
	DWORD tunerID;				//強制的に使用Tunerを固定
	//CMD_VER 2以降
	vector<REC_FILE_SET_INFO> partialRecFolder;	//部分受信サービス録画のフォルダ
	_REC_SETTING_DATA(void){
		recMode = 1;
		priority = 1;
		tuijyuuFlag = 1;
		serviceMode = 0;
		pittariFlag = FALSE;
		batFilePath = L"";
		suspendMode = 0;
		rebootFlag = FALSE;
		useMargineFlag = FALSE;
		startMargine = 10;
		endMargine = 5;
		continueRecFlag = 0;
		partialRecFlag = 0;
		tunerID = 0;
	};
} REC_SETTING_DATA;

//登録予約情報
typedef struct _RESERVE_DATA{
	wstring title;					//番組名
	SYSTEMTIME startTime;			//録画開始時間
	DWORD durationSecond;			//録画総時間
	wstring stationName;			//サービス名
	WORD originalNetworkID;			//ONID
	WORD transportStreamID;			//TSID
	WORD serviceID;					//SID
	WORD eventID;					//EventID
	wstring comment;				//コメント
	DWORD reserveID;				//予約識別ID 予約登録時は0
	//BYTE recWaitFlag;				//予約待機入った？ 内部で使用（廃止）
	BYTE overlapMode;				//かぶり状態 1:かぶってチューナー足りない予約あり 2:チューナー足りなくて予約できない
	//wstring recFilePath;			//録画ファイルパス 旧バージョン互換用 未使用（廃止）
	SYSTEMTIME startTimeEpg;		//予約時の開始時間
	REC_SETTING_DATA recSetting;	//録画設定
	DWORD reserveStatus;			//予約追加状態 内部で使用
	//CMD_VER 5以降
	vector<wstring> recFileNameList;	//録画予定ファイル名
	//DWORD param1;					//将来用
	_RESERVE_DATA(void){
		title=L"";
		ZeroMemory(&startTime, sizeof(SYSTEMTIME));
		durationSecond = 0;
		stationName = L"";
		originalNetworkID = 0;
		transportStreamID = 0;
		serviceID = 0;
		eventID = 0;
		comment = L"";
		reserveID = 0;
		overlapMode = 0;
		ZeroMemory(&startTimeEpg, sizeof(SYSTEMTIME));
		reserveStatus = 0;
	};
} RESERVE_DATA;

typedef struct _REC_FILE_INFO{
	DWORD id;					//ID
	wstring recFilePath;		//録画ファイルパス
	wstring title;				//番組名
	SYSTEMTIME startTime;		//開始時間
	DWORD durationSecond;		//録画時間
	wstring serviceName;		//サービス名
	WORD originalNetworkID;		//ONID
	WORD transportStreamID;		//TSID
	WORD serviceID;				//SID
	WORD eventID;				//EventID
	__int64 drops;				//ドロップ数
	__int64 scrambles;			//スクランブル数
	DWORD recStatus;			//録画結果のステータス
	SYSTEMTIME startTimeEpg;	//予約時の開始時間
	wstring comment;			//コメント
	wstring programInfo;		//.program.txtファイルの内容
	wstring errInfo;			//.errファイルの内容
	//CMD_VER 4以降
	BYTE protectFlag;
	_REC_FILE_INFO(void){
		id = 0;
		recFilePath = L"";
		title = L"";
		ZeroMemory(&startTime, sizeof(SYSTEMTIME));
		durationSecond = 0;
		serviceName = L"";
		originalNetworkID = 0;
		transportStreamID = 0;
		serviceID = 0;
		eventID = 0;
		drops = 0;
		scrambles = 0;
		recStatus = 0;
		ZeroMemory(&startTimeEpg, sizeof(SYSTEMTIME));
		comment = L"";
		programInfo = L"";
		errInfo = L"";
		protectFlag = 0;
	};
	_REC_FILE_INFO & operator= (const _RESERVE_DATA & o) {
		id = 0;
		recFilePath = L"";
		title = o.title;
		startTime = o.startTime;
		durationSecond = o.durationSecond;
		serviceName = o.stationName;
		originalNetworkID = o.originalNetworkID;
		transportStreamID = o.transportStreamID;
		serviceID = o.serviceID;
		eventID = o.eventID;
		drops = 0;
		scrambles = 0;
		recStatus = 0;
		startTimeEpg = o.startTimeEpg;
		comment = o.comment;
		programInfo = L"";
		errInfo = L"";
		protectFlag = 0;
		return *this;
	};
} REC_FILE_INFO;

typedef struct _TUNER_RESERVE_INFO{
	DWORD tunerID;
	wstring tunerName;
	vector<DWORD> reserveList;
} TUNER_RESERVE_INFO;

//チューナー毎サービス情報
typedef struct _CH_DATA4{
	int space;						//チューナー空間
	int ch;							//物理チャンネル
	WORD originalNetworkID;			//ONID
	WORD transportStreamID;			//TSID
	WORD serviceID;					//サービスID
	WORD serviceType;				//サービスタイプ
	BOOL partialFlag;				//部分受信サービス（ワンセグ）かどうか
	BOOL useViewFlag;				//一覧表示に使用するかどうか
	wstring serviceName;			//サービス名
	wstring chName;					//チャンネル名
	wstring networkName;			//ts_name or network_name
	BYTE remoconID;					//リモコンID
	_CH_DATA4(void){
		space = 0;
		ch = 0;
		originalNetworkID = 0;
		transportStreamID = 0;
		serviceID = 0;
		serviceType = 0;
		partialFlag = FALSE;
		useViewFlag = TRUE;
		serviceName = L"";
		chName = L"";
		networkName = L"";
		remoconID = 0;
	};
} CH_DATA4;

//全チューナーで認識したサービス一覧
typedef struct _CH_DATA5{
	WORD originalNetworkID;			//ONID
	WORD transportStreamID;			//TSID
	WORD serviceID;					//サービスID
	WORD serviceType;				//サービスタイプ
	BOOL partialFlag;				//部分受信サービス（ワンセグ）かどうか
	wstring serviceName;			//サービス名
	wstring networkName;			//ts_name or network_name
	BOOL epgCapFlag;				//EPGデータ取得対象かどうか
	BOOL searchFlag;				//検索時のデフォルト検索対象サービスかどうか
	_CH_DATA5(void){
		originalNetworkID = 0;
		transportStreamID = 0;
		serviceID = 0;
		serviceType = 0;
		partialFlag = FALSE;
		serviceName = L"";
		networkName = L"";
		epgCapFlag = TRUE;
		searchFlag = TRUE;
	};
} CH_DATA5;

typedef struct _REGIST_TCP_INFO{
	wstring ip;
	DWORD port;
}REGIST_TCP_INFO;

//コマンド送受信ストリーム
typedef struct _CMD_STREAM{
	DWORD param;	//送信時コマンド、受信時エラーコード
	DWORD dataSize;	//dataのサイズ（BYTE単位）
	BYTE* data;		//送受信するバイナリデータ
	_CMD_STREAM(void){
		param = 0;
		dataSize = 0;
		data = NULL;
	}
	~_CMD_STREAM(void){
		delete[] data;
	}
private:
	_CMD_STREAM(const _CMD_STREAM &);
	_CMD_STREAM & operator= (const _CMD_STREAM &);
} CMD_STREAM;

typedef struct _HTTP_STREAM{
	string httpHeader;	//送信時コマンド、受信時エラーコード
	DWORD dataSize;	//dataのサイズ（BYTE単位）
	BYTE* data;		//送受信するバイナリデータ
	_HTTP_STREAM(void){
		httpHeader = "";
		dataSize = 0;
		data = NULL;
	}
	~_HTTP_STREAM(void){
		delete[] data;
	}
private:
	_HTTP_STREAM(const _HTTP_STREAM &);
	_HTTP_STREAM & operator= (const _HTTP_STREAM &);
} HTTP_STREAM;

//EPG基本情報
typedef struct _EPGDB_SHORT_EVENT_INFO{
	wstring event_name;			//イベント名
	wstring text_char;			//情報
} EPGDB_SHORT_EVENT_INFO;

//EPG拡張情報
typedef struct _EPGDB_EXTENDED_EVENT_INFO{
	wstring text_char;			//詳細情報
} EPGDB_EXTENDED_EVENT_INFO;

//EPGジャンルデータ
typedef struct _EPGDB_CONTENT_DATA{
	BYTE content_nibble_level_1;
	BYTE content_nibble_level_2;
	BYTE user_nibble_1;
	BYTE user_nibble_2;
}EPGDB_CONTENT_DATA;

//EPGジャンル情報
typedef struct _EPGDB_CONTENT_INFO{
	vector<EPGDB_CONTENT_DATA> nibbleList;
} EPGDB_CONTEN_INFO;

//EPG映像情報
typedef struct _EPGDB_COMPONENT_INFO{
	BYTE stream_content;
	BYTE component_type;
	BYTE component_tag;
	wstring text_char;			//情報
} EPGDB_COMPONENT_INFO;

//EPG音声情報データ
typedef struct _EPGDB_AUDIO_COMPONENT_INFO_DATA{
	BYTE stream_content;
	BYTE component_type;
	BYTE component_tag;
	BYTE stream_type;
	BYTE simulcast_group_tag;
	BYTE ES_multi_lingual_flag;
	BYTE main_component_flag;
	BYTE quality_indicator;
	BYTE sampling_rate;
	wstring text_char;			//詳細情報
} EPGDB_AUDIO_COMPONENT_INFO_DATA;

//EPG音声情報
typedef struct _EPGDB_AUDIO_COMPONENT_INFO{
	vector<EPGDB_AUDIO_COMPONENT_INFO_DATA> componentList;
} EPGDB_AUDIO_COMPONENT_INFO;

//EPGイベントデータ
typedef struct _EPGDB_EVENT_DATA{
	WORD original_network_id;
	WORD transport_stream_id;
	WORD service_id;
	WORD event_id;
}EPGDB_EVENT_DATA;

//EPGイベントグループ情報
typedef struct _EPGDB_EVENTGROUP_INFO{
	BYTE group_type;
	vector<EPGDB_EVENT_DATA> eventDataList;
} EPGDB_EVENTGROUP_INFO;

typedef struct _EPGDB_EVENT_INFO{
	WORD original_network_id;
	WORD transport_stream_id;
	WORD service_id;
	WORD event_id;							//イベントID
	BYTE StartTimeFlag;						//start_timeの値が有効かどうか
	SYSTEMTIME start_time;					//開始時間
	BYTE DurationFlag;						//durationの値が有効かどうか
	DWORD durationSec;						//総時間（単位：秒）

	EPGDB_SHORT_EVENT_INFO* shortInfo;		//基本情報
	EPGDB_EXTENDED_EVENT_INFO* extInfo;		//拡張情報
	EPGDB_CONTEN_INFO* contentInfo;			//ジャンル情報
	EPGDB_COMPONENT_INFO* componentInfo;		//映像情報
	EPGDB_AUDIO_COMPONENT_INFO* audioInfo;	//音声情報
	EPGDB_EVENTGROUP_INFO* eventGroupInfo;	//イベントグループ情報
	EPGDB_EVENTGROUP_INFO* eventRelayInfo;	//イベントリレー情報

	BYTE freeCAFlag;						//ノンスクランブルフラグ
	_EPGDB_EVENT_INFO(void){
		shortInfo = NULL;
		extInfo = NULL;
		contentInfo = NULL;
		componentInfo = NULL;
		audioInfo = NULL;
		eventGroupInfo = NULL;
		eventRelayInfo = NULL;
	};
	~_EPGDB_EVENT_INFO(void){
		delete shortInfo;
		delete extInfo;
		delete contentInfo;
		delete componentInfo;
		delete audioInfo;
		delete eventGroupInfo;
		delete eventRelayInfo;
	};
	void DeepCopy(const _EPGDB_EVENT_INFO & o){
		original_network_id = o.original_network_id;
		transport_stream_id = o.transport_stream_id;
		service_id = o.service_id;
		event_id = o.event_id;
		StartTimeFlag = o.StartTimeFlag;
		start_time = o.start_time;
		DurationFlag = o.DurationFlag;
		durationSec = o.durationSec;
		freeCAFlag = o.freeCAFlag;
		SAFE_DELETE(shortInfo);
		SAFE_DELETE(extInfo);
		SAFE_DELETE(contentInfo);
		SAFE_DELETE(componentInfo);
		SAFE_DELETE(audioInfo);
		SAFE_DELETE(eventGroupInfo);
		SAFE_DELETE(eventRelayInfo);
		if( o.shortInfo ) shortInfo = new EPGDB_SHORT_EVENT_INFO(*o.shortInfo);
		if( o.extInfo ) extInfo = new EPGDB_EXTENDED_EVENT_INFO(*o.extInfo);
		if( o.contentInfo ) contentInfo = new EPGDB_CONTEN_INFO(*o.contentInfo);
		if( o.componentInfo ) componentInfo = new EPGDB_COMPONENT_INFO(*o.componentInfo);
		if( o.audioInfo ) audioInfo = new EPGDB_AUDIO_COMPONENT_INFO(*o.audioInfo);
		if( o.eventGroupInfo ) eventGroupInfo = new EPGDB_EVENTGROUP_INFO(*o.eventGroupInfo);
		if( o.eventRelayInfo ) eventRelayInfo = new EPGDB_EVENTGROUP_INFO(*o.eventRelayInfo);
	};
private:
	_EPGDB_EVENT_INFO(const _EPGDB_EVENT_INFO &);
	_EPGDB_EVENT_INFO & operator= (const _EPGDB_EVENT_INFO &);
}EPGDB_EVENT_INFO;

typedef struct _EPGDB_SERVICE_INFO{
	WORD ONID;
	WORD TSID;
	WORD SID;
	BYTE service_type;
	BYTE partialReceptionFlag;
	wstring service_provider_name;
	wstring service_name;
	wstring network_name;
	wstring ts_name;
	BYTE remote_control_key_id;
	_EPGDB_SERVICE_INFO(void){
		ONID = 0;
		TSID = 0;
		SID = 0;
		service_type = 0;
		partialReceptionFlag = 0;
		service_provider_name = L"";
		service_name = L"";
		network_name = L"";
		ts_name = L"";
		remote_control_key_id = 0;
	};
}EPGDB_SERVICE_INFO;

typedef struct _EPGDB_SERVICE_EVENT_INFO{
	EPGDB_SERVICE_INFO serviceInfo;
	vector<EPGDB_EVENT_INFO*> eventList;
}EPGDB_SERVICE_EVENT_INFO;

typedef struct _EPGDB_SEARCH_DATE_INFO{
	BYTE startDayOfWeek;
	WORD startHour;
	WORD startMin;
	BYTE endDayOfWeek;
	WORD endHour;
	WORD endMin;
} EPGDB_SEARCH_DATE_INFO;

//検索条件
typedef struct _EPGDB_SEARCH_KEY_INFO{
	wstring andKey;
	wstring notKey;
	BOOL regExpFlag;
	BOOL titleOnlyFlag;
	vector<EPGDB_CONTENT_DATA> contentList;
	vector<EPGDB_SEARCH_DATE_INFO> dateList;
	vector<__int64> serviceList;
	vector<WORD> videoList;
	vector<WORD> audioList;
	BYTE aimaiFlag;
	BYTE notContetFlag;
	BYTE notDateFlag;
	BYTE freeCAFlag;
	//CMD_VER 3以降
	//自動予約登録の条件専用
	BYTE chkRecEnd;					//録画済かのチェックあり
	WORD chkRecDay;					//録画済かのチェック対象期間
	_EPGDB_SEARCH_KEY_INFO(void){
		andKey = L"";
		notKey = L"";
		regExpFlag = FALSE;
		titleOnlyFlag = FALSE;
		aimaiFlag = 0;
		notContetFlag = 0;
		notDateFlag = 0;
		freeCAFlag = 0;
		chkRecEnd = 0;
		chkRecDay = 6;
	};
}EPGDB_SEARCH_KEY_INFO;

//自動予約登録情報
typedef struct _EPG_AUTO_ADD_DATA{
	DWORD dataID;
	EPGDB_SEARCH_KEY_INFO searchInfo;	//検索キー
	REC_SETTING_DATA recSetting;	//録画設定
	DWORD addCount;		//予約登録数
} EPG_AUTO_ADD_DATA;

typedef struct _MANUAL_AUTO_ADD_DATA{
	DWORD dataID;
	BYTE dayOfWeekFlag;				//対象曜日
	DWORD startTime;				//録画開始時間（00:00を0として秒単位）
	DWORD durationSecond;			//録画総時間
	wstring title;					//番組名
	wstring stationName;			//サービス名
	WORD originalNetworkID;			//ONID
	WORD transportStreamID;			//TSID
	WORD serviceID;					//SID
	REC_SETTING_DATA recSetting;	//録画設定
} MANUAL_AUTO_ADD_DATA;

//コマンド送信用
//チャンネル変更情報
typedef struct _SET_CH_INFO{
	BOOL useSID;//wONIDとwTSIDとwSIDの値が使用できるかどうか
	WORD ONID;
	WORD TSID;
	WORD SID;
	BOOL useBonCh;//dwSpaceとdwChの値が使用できるかどうか
	DWORD space;
	DWORD ch;
}SET_CH_INFO;

typedef struct _SET_CTRL_MODE{
	DWORD ctrlID;
	WORD SID;
	BYTE enableScramble;
	BYTE enableCaption;
	BYTE enableData;
} SET_CTRL_MODE;

typedef struct _SET_CTRL_REC_PARAM{
	DWORD ctrlID;
	wstring fileName;
	BYTE overWriteFlag;
	ULONGLONG createSize;
	vector<REC_FILE_SET_INFO> saveFolder;
	BYTE pittariFlag;
	WORD pittariONID;
	WORD pittariTSID;
	WORD pittariSID;
	WORD pittariEventID;
} SET_CTRL_REC_PARAM;

typedef struct _SET_CTRL_REC_STOP_PARAM{
	DWORD ctrlID;
	BOOL saveErrLog;
} SET_CTRL_REC_STOP_PARAM;

typedef struct _SET_CTRL_REC_STOP_RES_PARAM{
	wstring recFilePath;
	ULONGLONG drop;
	ULONGLONG scramble;
	BYTE subRecFlag;
} SET_CTRL_REC_STOP_RES_PARAM;

typedef struct _SEARCH_EPG_INFO_PARAM{
	WORD ONID;
	WORD TSID;
	WORD SID;
	WORD eventID;
	BYTE pfOnlyFlag;
} SEARCH_EPG_INFO_PARAM;

typedef struct _GET_EPG_PF_INFO_PARAM{
	WORD ONID;
	WORD TSID;
	WORD SID;
	BYTE pfNextFlag;
} GET_EPG_PF_INFO_PARAM;

typedef struct _TVTEST_CH_CHG_INFO{
	wstring bonDriver;
	SET_CH_INFO chInfo;
} TVTEST_CH_CHG_INFO;


typedef struct _TVTEST_STREAMING_INFO{
	BOOL enableMode;
	DWORD ctrlID;
	DWORD serverIP;
	DWORD serverPort;
	wstring filePath;
	BOOL udpSend;
	BOOL tcpSend;
	BOOL timeShiftMode;
} TVTEST_STREAMING_INFO;

typedef struct _NWPLAY_PLAY_INFO{
	DWORD ctrlID;
	DWORD ip;
	BYTE udp;
	BYTE tcp;
	DWORD udpPort;//outで実際の開始ポート
	DWORD tcpPort;//outで実際の開始ポート
} NWPLAY_PLAY_INFO;

typedef struct _NWPLAY_POS_CMD{
	DWORD ctrlID;
	__int64 currentPos;
	__int64 totalPos;//CMD2_EPG_SRV_NWPLAY_SET_POS時は無視
} NWPLAY_POS_CMD;

typedef struct _NWPLAY_TIMESHIFT_INFO{
	DWORD ctrlID;
	wstring filePath;
} NWPLAY_TIMESHIFT_INFO;

//情報通知用パラメーター
typedef struct _NOTIFY_SRV_INFO{
	DWORD notifyID;		//通知情報の種類
	SYSTEMTIME time;	//通知状態の発生した時間
	DWORD param1;		//パラメーター１（種類によって内容変更）
	DWORD param2;		//パラメーター２（種類によって内容変更）
	DWORD param3;		//パラメーター３（種類によって内容変更）
	wstring param4;		//パラメーター４（種類によって内容変更）
	wstring param5;		//パラメーター５（種類によって内容変更）
	wstring param6;		//パラメーター６（種類によって内容変更）
	_NOTIFY_SRV_INFO(void){
		notifyID= 0;
		param1 = 0;
		param2 = 0;
		param3 = 0;
	};
} NOTIFY_SRV_INFO;


typedef struct _GENRU_INFO{
	BYTE nibble1;
	BYTE nibble2;
	WORD key;
	wstring name;
	_GENRU_INFO(void){
		nibble1= 0xFF;
		nibble2 = 0xFF;
		key = 0xFFFF;
		name = L"";
	};
} GENRU_INFO;


////////////////////////////////////////////////////////////////////////////////////////////
//旧バージョンコマンド送信用
typedef struct _OLD_RESERVE_DATA{
	wstring strTitle;
	SYSTEMTIME StartTime;
	DWORD dwDurationSec;
	wstring strStationName;
	unsigned short usONID;
	unsigned short usTSID;
	unsigned short usServiceID;
	unsigned short usEventID;
	unsigned char ucPriority;
	unsigned char ucTuijyuu;
	wstring strComment;
	DWORD dwRecMode;
	BOOL bPittari;
	wstring strBatPath;
	DWORD dwReserveID; //同一番組判別用ID
	BOOL bSetWait; //予約待機入った？
	DWORD dwPiledUpMode; //かぶり状態 1:かぶってチューナー足りない予約あり 2:チューナー足りなくて予約できない
	wstring strRecFolder;
	WORD wSuspendMode;
	BOOL bReboot;
	wstring strRecFilePath;
	BOOL bUseMargine;
	int iStartMargine;
	int iEndMargine;
	DWORD dwServiceMode;
	_OLD_RESERVE_DATA & operator= (const _RESERVE_DATA & o) {
		strTitle=o.title;
		StartTime = o.startTime;
		dwDurationSec = o.durationSecond;
		strStationName = o.stationName;
		usONID = o.originalNetworkID;
		usTSID = o.transportStreamID;
		usServiceID = o.serviceID;
		usEventID = o.eventID;
		ucPriority = o.recSetting.priority;
		ucTuijyuu = o.recSetting.tuijyuuFlag;
		strComment = o.comment;
		dwRecMode = o.recSetting.recMode;
		bPittari = o.recSetting.pittariFlag;
		strBatPath = o.recSetting.batFilePath;
		dwReserveID = o.reserveID;
		bSetWait = 0;
		dwPiledUpMode = o.overlapMode;
		if( o.recSetting.recFolderList.size() >0 ){
			strRecFolder = o.recSetting.recFolderList[0].recFolder;
		}else{
			strRecFolder = L"";
		}
		wSuspendMode = o.recSetting.suspendMode;
		bReboot = o.recSetting.rebootFlag;
		strRecFilePath = L"";
		bUseMargine = o.recSetting.useMargineFlag;
		iStartMargine = o.recSetting.startMargine;
		iEndMargine = o.recSetting.endMargine;
		dwServiceMode = o.recSetting.serviceMode;
		return *this;
	};
} OLD_RESERVE_DATA;


typedef struct _OLD_SEARCH_KEY{
	wstring strAnd;
	wstring strNot;
	BOOL bTitle;
	int iJanru;
	int iSH;
	int iSM;
	int iEH;
	int iEM;
	BOOL bChkMon;
	BOOL bChkTue;
	BOOL bChkWed;
	BOOL bChkThu;
	BOOL bChkFri;
	BOOL bChkSat;
	BOOL bChkSun;
	vector<__int64> CHIDList; //ONID<<24 | TSID<<16 | SID
	//以下自動予約登録時関係のみ使用
	int iAutoAddID; //自動予約登録一覧の識別用キー
	int iPriority;
	int iTuijyuu;
	int iRecMode;
	int iPittari;
	wstring strBat;
	wstring strRecFolder;
	WORD wSuspendMode;
	BOOL bReboot;
	BOOL bUseMargine;
	int iStartMargine;
	int iEndMargine;
	DWORD dwServiceMode;

	BOOL bRegExp;
	wstring strPattern;
} OLD_SEARCH_KEY;

typedef struct _OLD_EVENT_ID_INFO{
	DWORD dwOriginalNID;
	DWORD dwTSID;
	DWORD dwServiceID;
	DWORD dwEventID;
}OLD_EVENT_ID_INFO;

typedef struct _OLD_NIBBLE_DATA{
	unsigned char ucContentNibbleLv1; //content_nibble_level_1
	unsigned char ucContentNibbleLv2; //content_nibble_level_2
	unsigned char ucUserNibbleLv1; //user_nibble
	unsigned char ucUserNibbleLv2; //user_nibble
}OLD_NIBBLE_DATA;

typedef struct _OLD_EVENT_INFO_DATA3{
	WORD wOriginalNID;
	WORD wTSID;
	WORD wServiceID;
	WORD wEventID;
	wstring strEventName;
	wstring strEventText;
	wstring strEventExtText;
	SYSTEMTIME stStartTime;
	DWORD dwDurationSec;
	unsigned char ucComponentType;
	wstring strComponentTypeText;
	unsigned char ucAudioComponentType;
	unsigned char ucESMultiLangFlag;
	unsigned char ucMainComponentFlag;
	unsigned char ucSamplingRate;
	wstring strAudioComponentTypeText;
	vector<OLD_NIBBLE_DATA> NibbleList;
	vector<OLD_EVENT_ID_INFO> EventRelayList;

	wstring strSearchTitle;
	wstring strSearchInfo;
	vector<OLD_EVENT_ID_INFO> EventGroupList;

	_OLD_EVENT_INFO_DATA3 & operator= (const _EPGDB_EVENT_INFO & o) {
		wOriginalNID = o.original_network_id;
		wTSID = o.transport_stream_id;
		wServiceID = o.service_id;
		wEventID = o.event_id;
		if( o.shortInfo != NULL ){
			strEventName = o.shortInfo->event_name;
		}else{
			strEventName = L"";
		}
		if( o.shortInfo != NULL ){
			strEventText = o.shortInfo->text_char;
		}else{
			strEventText = L"";
		}
		if( o.extInfo != NULL ){
			strEventText = o.extInfo->text_char;
		}else{
			strEventText = L"";
		}
		stStartTime = o.start_time;
		dwDurationSec = o.durationSec;
		if( o.componentInfo != NULL ){
			ucComponentType = o.componentInfo->component_type;
			strComponentTypeText = o.componentInfo->text_char;
		}else{
			ucComponentType = 0;
			strComponentTypeText = L"";
		}
		if( o.audioInfo != NULL ){
			if( o.audioInfo->componentList.size() > 0 ){
				ucAudioComponentType = o.audioInfo->componentList[0].component_type;
				ucESMultiLangFlag = o.audioInfo->componentList[0].ES_multi_lingual_flag;
				ucMainComponentFlag = o.audioInfo->componentList[0].main_component_flag;
				ucSamplingRate = o.audioInfo->componentList[0].sampling_rate;
				strAudioComponentTypeText = o.audioInfo->componentList[0].text_char;
			}
		}
		if( o.contentInfo != NULL ){
			NibbleList.clear();
			for( size_t i=0; i<o.contentInfo->nibbleList.size(); i++ ){
				OLD_NIBBLE_DATA item;
				item.ucContentNibbleLv1 = o.contentInfo->nibbleList[i].content_nibble_level_1;
				item.ucContentNibbleLv2 = o.contentInfo->nibbleList[i].content_nibble_level_2;
				item.ucUserNibbleLv1 = o.contentInfo->nibbleList[i].user_nibble_1;
				item.ucUserNibbleLv2 = o.contentInfo->nibbleList[i].user_nibble_2;
				NibbleList.push_back(item);
			}
		}
		if( o.eventGroupInfo != NULL ){
			EventGroupList.clear();
			for( size_t i=0; i<o.eventGroupInfo->eventDataList.size(); i++ ){
				OLD_EVENT_ID_INFO item;
				item.dwOriginalNID = o.eventGroupInfo->eventDataList[i].original_network_id;
				item.dwTSID = o.eventGroupInfo->eventDataList[i].transport_stream_id;
				item.dwServiceID = o.eventGroupInfo->eventDataList[i].service_id;
				item.dwEventID = o.eventGroupInfo->eventDataList[i].event_id;
				EventGroupList.push_back(item);
			}
		}
		if( o.eventRelayInfo != NULL ){
			EventRelayList.clear();
			for( size_t i=0; i<o.eventRelayInfo->eventDataList.size(); i++ ){
				OLD_EVENT_ID_INFO item;
				item.dwOriginalNID = o.eventRelayInfo->eventDataList[i].original_network_id;
				item.dwTSID = o.eventRelayInfo->eventDataList[i].transport_stream_id;
				item.dwServiceID = o.eventRelayInfo->eventDataList[i].service_id;
				item.dwEventID = o.eventRelayInfo->eventDataList[i].event_id;
				EventRelayList.push_back(item);
			}
		}

		return *this;
	};
}OLD_EVENT_INFO_DATA3;

#endif
