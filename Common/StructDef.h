#ifndef INCLUDE_STRUCT_DEF_H
#define INCLUDE_STRUCT_DEF_H

#include "EpgDataCap3Def.h"

//転送ファイルデータ
struct FILE_DATA {
	wstring Name;				//ファイル名
	vector<BYTE> Data;			//ファイルデータ
};

//録画フォルダ情報
struct REC_FILE_SET_INFO {
	wstring recFolder;			//録画フォルダ
	wstring writePlugIn;		//出力PlugIn
	wstring recNamePlugIn;		//ファイル名変換PlugInの使用
	wstring recFileName;		//ファイル名個別対応 録画開始処理時に内部で使用。予約情報としては必要なし
};

//録画設定情報
struct REC_SETTING_DATA {
	BYTE recMode;				//録画モード
	BYTE priority;				//優先度
	BYTE tuijyuuFlag;			//イベントリレー追従するかどうか
	DWORD serviceMode;			//処理対象データモード
	BYTE pittariFlag;			//ぴったり？録画
	wstring batFilePath;		//録画後BATファイルパス
	vector<REC_FILE_SET_INFO> recFolderList;		//録画フォルダパス
	BYTE suspendMode;			//休止モード
	BYTE rebootFlag;			//録画後再起動する
	BYTE useMargineFlag;		//録画マージンを個別指定
	int startMargine;			//録画開始時のマージン
	int endMargine;				//録画終了時のマージン
	BYTE continueRecFlag;		//後続同一サービス時、同一ファイルで録画
	BYTE partialRecFlag;		//物理CHに部分受信サービスがある場合、同時録画するかどうか
	DWORD tunerID;				//強制的に使用Tunerを固定
	//CMD_VER 2以降
	vector<REC_FILE_SET_INFO> partialRecFolder;	//部分受信サービス録画のフォルダ
	enum {
		DIV_RECMODE = 5,		//recModeの録画モード情報以外の除数
	};
	//無効かどうか
	bool IsNoRec() const { return recMode / DIV_RECMODE % 2 != 0; }
	//recModeの録画モード情報のみ
	BYTE GetRecMode() const { return (recMode + recMode / DIV_RECMODE % 2) % DIV_RECMODE; }
};

//登録予約情報
struct RESERVE_DATA {
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
	BYTE presentFlag;				//EIT[present]でチェック済み？ 純粋に内部で使用
	BYTE overlapMode;				//かぶり状態 1:かぶってチューナー足りない予約あり 2:チューナー足りなくて予約できない
	//wstring recFilePath;			//録画ファイルパス 旧バージョン互換用 未使用（廃止）
	SYSTEMTIME startTimeEpg;		//予約時の開始時間
	REC_SETTING_DATA recSetting;	//録画設定
	DWORD reserveStatus;			//予約追加状態 内部で使用
	vector<DWORD> ngTunerIDList;	//失敗したTunerIDのリスト。純粋に内部で使用
	//CMD_VER 5以降
	vector<wstring> recFileNameList;	//録画予定ファイル名
	//DWORD param1;					//将来用
};

enum REC_END_STATUS {
	REC_END_STATUS_NORMAL = 1,		//終了・録画終了
	REC_END_STATUS_OPEN_ERR,		//チューナーのオープンに失敗しました
	REC_END_STATUS_ERR_END,			//録画中にキャンセルされた可能性があります
	REC_END_STATUS_NEXT_START_END,	//次の予約開始のためにキャンセルされました
	REC_END_STATUS_START_ERR,		//録画時間に起動していなかった可能性があります
	REC_END_STATUS_CHG_TIME,		//開始時間が変更されました
	REC_END_STATUS_NO_TUNER,		//チューナー不足のため失敗しました
	REC_END_STATUS_NO_RECMODE,		//無効扱いでした
	REC_END_STATUS_NOT_FIND_PF,		//録画中に番組情報を確認できませんでした
	REC_END_STATUS_NOT_FIND_6H,		//指定時間番組情報が見つかりませんでした
	REC_END_STATUS_END_SUBREC,		//録画終了（空き容量不足で別フォルダへの保存が発生）
	REC_END_STATUS_ERR_RECSTART,	//録画開始処理に失敗しました
	REC_END_STATUS_NOT_START_HEAD,	//一部のみ録画が実行された可能性があります
	REC_END_STATUS_ERR_CH_CHG,		//指定チャンネルのデータがBonDriverから出力されなかった可能性があります
	REC_END_STATUS_ERR_END2,		//ファイル保存で致命的なエラーが発生した可能性があります
};

struct REC_FILE_INFO {
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
	LONGLONG drops;				//ドロップ数
	LONGLONG scrambles;			//スクランブル数
	DWORD recStatus;			//録画結果のステータス
	SYSTEMTIME startTimeEpg;	//予約時の開始時間
	wstring programInfo;		//.program.txtファイルの内容
	wstring errInfo;			//.errファイルの内容
	//CMD_VER 4以降
	BYTE protectFlag;
	REC_FILE_INFO & operator= (const RESERVE_DATA & o) {
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
		programInfo = L"";
		errInfo = L"";
		protectFlag = 0;
		return *this;
	};
	LPCWSTR GetComment() const {
		return recStatus == REC_END_STATUS_NORMAL ? (recFilePath.empty() ? L"終了" : L"録画終了") :
			recStatus == REC_END_STATUS_OPEN_ERR ? L"チューナーのオープンに失敗しました" :
			recStatus == REC_END_STATUS_ERR_END ? L"録画中にキャンセルされた可能性があります" :
			recStatus == REC_END_STATUS_NEXT_START_END ? L"次の予約開始のためにキャンセルされました" :
			recStatus == REC_END_STATUS_START_ERR ? L"録画時間に起動していなかった可能性があります" :
			recStatus == REC_END_STATUS_CHG_TIME ? L"開始時間が変更されました" :
			recStatus == REC_END_STATUS_NO_TUNER ? L"チューナー不足のため失敗しました" :
			recStatus == REC_END_STATUS_NO_RECMODE ? L"無効扱いでした" :
			recStatus == REC_END_STATUS_NOT_FIND_PF ? L"録画中に番組情報を確認できませんでした" :
			recStatus == REC_END_STATUS_NOT_FIND_6H ? L"指定時間番組情報が見つかりませんでした" :
			recStatus == REC_END_STATUS_END_SUBREC ? L"録画終了（空き容量不足で別フォルダへの保存が発生）" :
			recStatus == REC_END_STATUS_ERR_RECSTART ? L"録画開始処理に失敗しました" :
			recStatus == REC_END_STATUS_NOT_START_HEAD ? L"一部のみ録画が実行された可能性があります" :
			recStatus == REC_END_STATUS_ERR_CH_CHG ? L"指定チャンネルのデータがBonDriverから出力されなかった可能性があります" :
			recStatus == REC_END_STATUS_ERR_END2 ? L"ファイル保存で致命的なエラーが発生した可能性があります" : L"";
	}
};

struct TUNER_RESERVE_INFO {
	DWORD tunerID;
	wstring tunerName;
	vector<DWORD> reserveList;
};

struct TUNER_PROCESS_STATUS_INFO {
	DWORD tunerID;
	int processID;
	ULONGLONG drop;
	ULONGLONG scramble;
	float signalLv;
	int space;
	int ch;
	int originalNetworkID;
	int transportStreamID;
	BYTE recFlag;
	BYTE epgCapFlag;
	WORD extraFlags;
};

//チューナー毎サービス情報
struct CH_DATA4 {
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
};

//全チューナーで認識したサービス一覧
struct CH_DATA5 {
	WORD originalNetworkID;			//ONID
	WORD transportStreamID;			//TSID
	WORD serviceID;					//サービスID
	WORD serviceType;				//サービスタイプ
	BOOL partialFlag;				//部分受信サービス（ワンセグ）かどうか
	wstring serviceName;			//サービス名
	wstring networkName;			//ts_name or network_name
	BOOL epgCapFlag;				//EPGデータ取得対象かどうか
	BOOL searchFlag;				//検索時のデフォルト検索対象サービスかどうか
	BYTE remoconID;					//リモコンID
};

//EPG基本情報
struct EPGDB_SHORT_EVENT_INFO {
	wstring event_name;			//イベント名
	wstring text_char;			//情報
};

//EPG拡張情報
struct EPGDB_EXTENDED_EVENT_INFO {
	wstring text_char;			//詳細情報
};

//EPGジャンルデータ
typedef EPG_CONTENT EPGDB_CONTENT_DATA;

//EPGジャンル情報
struct EPGDB_CONTEN_INFO {
	vector<EPGDB_CONTENT_DATA> nibbleList;
};

//EPG映像情報
struct EPGDB_COMPONENT_INFO {
	BYTE stream_content;
	BYTE component_type;
	BYTE component_tag;
	wstring text_char;			//情報
};

//EPG音声情報データ
struct EPGDB_AUDIO_COMPONENT_INFO_DATA {
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
};

//EPG音声情報
struct EPGDB_AUDIO_COMPONENT_INFO {
	vector<EPGDB_AUDIO_COMPONENT_INFO_DATA> componentList;
};

//EPGイベントデータ
typedef EPG_EVENT_DATA EPGDB_EVENT_DATA;

//EPGイベントグループ情報
struct EPGDB_EVENTGROUP_INFO {
	vector<EPGDB_EVENT_DATA> eventDataList;
};

struct EPGDB_EVENT_INFO {
	WORD original_network_id;
	WORD transport_stream_id;
	WORD service_id;
	WORD event_id;							//イベントID
	BYTE StartTimeFlag;						//start_timeの値が有効かどうか
	SYSTEMTIME start_time;					//開始時間
	BYTE DurationFlag;						//durationの値が有効かどうか
	DWORD durationSec;						//総時間（単位：秒）
	BYTE freeCAFlag;						//ノンスクランブルフラグ
	bool hasShortInfo;
	bool hasExtInfo;
	bool hasContentInfo;
	bool hasComponentInfo;
	bool hasAudioInfo;
	BYTE eventGroupInfoGroupType;
	BYTE eventRelayInfoGroupType;
	EPGDB_SHORT_EVENT_INFO shortInfo;		//基本情報
	EPGDB_EXTENDED_EVENT_INFO extInfo;		//拡張情報
	EPGDB_CONTEN_INFO contentInfo;			//ジャンル情報
	EPGDB_COMPONENT_INFO componentInfo;		//映像情報
	EPGDB_AUDIO_COMPONENT_INFO audioInfo;	//音声情報
	EPGDB_EVENTGROUP_INFO eventGroupInfo;	//イベントグループ情報
	EPGDB_EVENTGROUP_INFO eventRelayInfo;	//イベントリレー情報
	EPGDB_EVENT_INFO(void) {
		hasShortInfo = false;
		hasExtInfo = false;
		hasContentInfo = false;
		hasComponentInfo = false;
		hasAudioInfo = false;
		eventGroupInfoGroupType = 0;
		eventRelayInfoGroupType = 0;
	};
};

struct EPGDB_SERVICE_INFO {
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
};

struct EPGDB_SERVICE_EVENT_INFO {
	EPGDB_SERVICE_INFO serviceInfo;
	vector<EPGDB_EVENT_INFO> eventList;
};

struct EPGDB_SERVICE_EVENT_INFO_PTR {
	const EPGDB_SERVICE_INFO* serviceInfo;
	vector<const EPGDB_EVENT_INFO*> eventList;
};

struct EPGDB_SEARCH_DATE_INFO {
	BYTE startDayOfWeek;
	WORD startHour;
	WORD startMin;
	BYTE endDayOfWeek;
	WORD endHour;
	WORD endMin;
};

//検索条件
struct EPGDB_SEARCH_KEY_INFO {
	wstring andKey;
	wstring notKey;
	BOOL regExpFlag;
	BOOL titleOnlyFlag;
	vector<EPGDB_CONTENT_DATA> contentList;
	vector<EPGDB_SEARCH_DATE_INFO> dateList;
	vector<LONGLONG> serviceList;
	vector<WORD> videoList;
	vector<WORD> audioList;
	BYTE aimaiFlag;
	BYTE notContetFlag;
	BYTE notDateFlag;
	BYTE freeCAFlag;
	//CMD_VER 3以降
	//自動予約登録の条件専用
	BYTE chkRecEnd;					//録画済かのチェックあり
	WORD chkRecDay;					//録画済かのチェック対象期間（+20000=SID無視,+30000=TS|SID無視,+40000=ON|TS|SID無視）
};

struct SEARCH_PG_PARAM {
	vector<EPGDB_SEARCH_KEY_INFO> keyList;
	LONGLONG enumStart;
	LONGLONG enumEnd;
};

//自動予約登録情報
struct EPG_AUTO_ADD_DATA {
	DWORD dataID;
	EPGDB_SEARCH_KEY_INFO searchInfo;	//検索キー
	REC_SETTING_DATA recSetting;	//録画設定
	DWORD addCount;		//予約登録数
};

struct MANUAL_AUTO_ADD_DATA {
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
};

//Viewアプリのステータス情報
struct VIEW_APP_STATUS_INFO {
	DWORD status;
	int delaySec;
	wstring bonDriver;
	ULONGLONG drop;
	ULONGLONG scramble;
	float signalLv;
	int space;
	int ch;
	int originalNetworkID;
	int transportStreamID;
	int appID;
};

//チャンネル・NetworkTVモード変更情報
struct SET_CH_INFO {
	BOOL useSID;	//ONIDとTSIDとSIDの値が使用できるかどうか
	WORD ONID;
	WORD TSID;
	WORD SID;
	BOOL useBonCh;	//spaceとchの値が使用できるかどうか
	int space;		//チューナー空間（NetworkTV関連ではID）
	int ch;			//物理チャンネル（NetworkTV関連では送信モード）
};

struct SET_CTRL_MODE {
	DWORD ctrlID;
	WORD SID;
	BYTE enableScramble;
	BYTE enableCaption;
	BYTE enableData;
};

struct SET_CTRL_REC_PARAM {
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
};

struct SET_CTRL_REC_STOP_PARAM {
	DWORD ctrlID;
	BOOL saveErrLog;
};

struct SET_CTRL_REC_STOP_RES_PARAM {
	wstring recFilePath;
	ULONGLONG drop;
	ULONGLONG scramble;
	BYTE subRecFlag;
};

struct SEARCH_EPG_INFO_PARAM {
	WORD ONID;
	WORD TSID;
	WORD SID;
	WORD eventID;
	BYTE pfOnlyFlag;
};

struct GET_EPG_PF_INFO_PARAM {
	WORD ONID;
	WORD TSID;
	WORD SID;
	BYTE pfNextFlag;
};

struct TVTEST_CH_CHG_INFO {
	wstring bonDriver;
	SET_CH_INFO chInfo;
};


struct TVTEST_STREAMING_INFO {
	BOOL enableMode;
	DWORD ctrlID;
	DWORD serverIP;
	DWORD serverPort;
	wstring filePath;
	BOOL udpSend;
	BOOL tcpSend;
	BOOL timeShiftMode;
};

struct NWPLAY_PLAY_INFO {
	DWORD ctrlID;
	DWORD ip;
	BYTE udp;
	BYTE tcp;
	DWORD udpPort;//outで実際の開始ポート
	DWORD tcpPort;//outで実際の開始ポート
};

struct NWPLAY_POS_CMD {
	DWORD ctrlID;
	LONGLONG currentPos;
	LONGLONG totalPos;//CMD2_EPG_SRV_NWPLAY_SET_POS時は無視
};

struct NWPLAY_TIMESHIFT_INFO {
	DWORD ctrlID;
	wstring filePath;
};

//情報通知用パラメーター
struct NOTIFY_SRV_INFO {
	DWORD notifyID;		//通知情報の種類
	SYSTEMTIME time;	//通知状態の発生した時間
	DWORD param1;		//パラメーター１（種類によって内容変更）
	DWORD param2;		//パラメーター２（種類によって内容変更）
	DWORD param3;		//パラメーター３（通知の巡回カウンタ）
	wstring param4;		//パラメーター４（種類によって内容変更）
	wstring param5;		//パラメーター５（種類によって内容変更）
	wstring param6;		//パラメーター６（種類によって内容変更）
};

#endif
