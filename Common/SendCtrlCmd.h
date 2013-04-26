#pragma once

#include "Util.h"
#include "StructDef.h"

#include "CtrlCmdDef.h"
#include "ErrDef.h"
#include "CtrlCmdUtil.h"
#include "CtrlCmdUtil2.h"

class CSendCtrlCmd
{
public:
	CSendCtrlCmd(void);
	~CSendCtrlCmd(void);

	//コマンド送信方法の設定
	//引数：
	// tcpFlag		[IN] TRUE：TCP/IPモード、FALSE：名前付きパイプモード
	void SetSendMode(
		BOOL tcpFlag
		);

	//名前付きパイプモード時の接続先を設定
	//EpgTimerSrv.exeに対するコマンドは設定しなくても可（デフォルト値になっている）
	//引数：
	// eventName	[IN]排他制御用Eventの名前
	// pipeName		[IN]接続パイプの名前
	void SetPipeSetting(
		wstring eventName,
		wstring pipeName
		);

	//TCP/IPモード時の接続先を設定
	//引数：
	// ip			[IN]接続先IP
	// port			[IN]接続先ポート
	void SetNWSetting(
		wstring ip,
		DWORD port
		);

	//接続処理時のタイムアウト設定
	// timeOut		[IN]タイムアウト値（単位：ms）
	void SetConnectTimeOut(
		DWORD timeOut
		);

	//Program.txtを追加で再読み込みする
	//戻り値：
	// エラーコード
	DWORD SendAddloadReserve();

	//EPGデータを再読み込みする
	//戻り値：
	// エラーコード
	DWORD SendReloadEpg();

	//設定情報を再読み込みする
	//戻り値：
	// エラーコード
	DWORD SendReloadSetting();

	//EpgTimerSrv.exeを終了する
	//戻り値：
	// エラーコード
	DWORD SendClose();

	//EpgTimerSrv.exeのパイプ接続GUIとしてプロセスを登録する
	//戻り値：
	// エラーコード
	//引数：
	// processID			[IN]プロセスID
	DWORD SendRegistGUI(DWORD processID);

	//EpgTimerSrv.exeのパイプ接続GUI登録を解除する
	//戻り値：
	// エラーコード
	//引数：
	// processID			[IN]プロセスID
	DWORD SendUnRegistGUI(DWORD processID);
	
	//EpgTimerSrv.exeのTCP接続GUIとしてプロセスを登録する
	//戻り値：
	// エラーコード
	//引数：
	// port					[IN]ポート
	DWORD SendRegistTCP(DWORD port);

	//EpgTimerSrv.exeのTCP接続GUI登録を解除する
	//戻り値：
	// エラーコード
	//引数：
	// port					[IN]ポート
	DWORD SendUnRegistTCP(DWORD port);

	//予約一覧を取得する
	//戻り値：
	// エラーコード
	//引数：
	// val			[OUT]予約一覧
	DWORD SendEnumReserve(
		vector<RESERVE_DATA>* val
		);

	//予約情報を取得する
	//戻り値：
	// エラーコード
	//引数：
	// reserveID		[IN]取得する情報の予約ID
	// val				[OUT]予約情報
	DWORD SendGetReserve(DWORD reserveID, RESERVE_DATA* val);

	//予約を追加する
	//戻り値：
	// エラーコード
	//引数：
	// val				[IN]追加する予約一覧
	DWORD SendAddReserve(vector<RESERVE_DATA>* val);

	//予約を削除する
	//戻り値：
	// エラーコード
	//引数：
	// val				[IN]削除する予約ID一覧
	DWORD SendDelReserve(vector<DWORD>* val);

	//予約を変更する
	//戻り値：
	// エラーコード
	//引数：
	// val				[IN]変更する予約一覧
	DWORD SendChgReserve(vector<RESERVE_DATA>* val);

	//チューナーごとの予約一覧を取得する
	//戻り値：
	// エラーコード
	//引数：
	// val				[IN]予約一覧
	DWORD SendEnumTunerReserve(vector<TUNER_RESERVE_INFO>* val);

	//録画済み情報一覧取得
	//戻り値：
	// エラーコード
	//引数：
	// val			[OUT]録画済み情報一覧
	DWORD SendEnumRecInfo(
		vector<REC_FILE_INFO>* val
		);
	
	//録画済み情報を削除する
	//戻り値：
	// エラーコード
	//引数：
	// val				[IN]削除するID一覧
	DWORD SendDelRecInfo(vector<DWORD>* val);

	//サービス一覧を取得する
	//戻り値：
	// エラーコード
	//引数：
	// val				[OUT]サービス一覧
	DWORD SendEnumService(
		vector<EPGDB_SERVICE_INFO>* val
		);

	//サービス指定で番組情報を一覧を取得する
	//戻り値：
	// エラーコード
	//引数：
	// service			[IN]ONID<<32 | TSID<<16 | SIDとしたサービスID
	// val				[OUT]番組情報一覧
	DWORD SendEnumPgInfo(
		ULONGLONG service,
		vector<EPGDB_EVENT_INFO*>* val
		);

	//指定イベントの番組情報を取得する
	//戻り値：
	// エラーコード
	//引数：
	// pgID				[IN]ONID<<48 | TSID<<32 | SID<<16 | EventIDとしたID
	// val				[OUT]番組情報
	DWORD SendGetPgInfo(
		ULONGLONG pgID,
		EPGDB_EVENT_INFO* val
		);

	//指定キーワードで番組情報を検索する
	//戻り値：
	// エラーコード
	//引数：
	// key				[IN]検索キー（複数指定時はまとめて検索結果が返る）
	// val				[OUT]番組情報一覧
	DWORD SendSearchPg(
		vector<EPGDB_SEARCH_KEY_INFO>* key,
		vector<EPGDB_EVENT_INFO*>* val
		);

	//番組情報一覧を取得する
	//戻り値：
	// エラーコード
	//引数：
	// val				[OUT]番組情報一覧
	DWORD SendEnumPgAll(
		vector<EPGDB_SERVICE_EVENT_INFO*>* val
		);

	//自動予約登録条件一覧を取得する
	//戻り値：
	// エラーコード
	//引数：
	// val			[OUT]条件一覧
	DWORD SendEnumEpgAutoAdd(
		vector<EPG_AUTO_ADD_DATA>* val
		);

	//自動予約登録条件を追加する
	//戻り値：
	// エラーコード
	//引数：
	// val			[IN]条件一覧
	DWORD SendAddEpgAutoAdd(
		vector<EPG_AUTO_ADD_DATA>* val
		);

	//自動予約登録条件を削除する
	//戻り値：
	// エラーコード
	//引数：
	// val			[IN]条件一覧
	DWORD SendDelEpgAutoAdd(
		vector<DWORD>* val
		);

	//自動予約登録条件を変更する
	//戻り値：
	// エラーコード
	//引数：
	// val			[IN]条件一覧
	DWORD SendChgEpgAutoAdd(
		vector<EPG_AUTO_ADD_DATA>* val
		);

	//自動予約登録条件一覧を取得する
	//戻り値：
	// エラーコード
	//引数：
	// val			[OUT]条件一覧	
	DWORD SendEnumManualAdd(
		vector<MANUAL_AUTO_ADD_DATA>* val
		);

	//自動予約登録条件を追加する
	//戻り値：
	// エラーコード
	//引数：
	// val			[IN]条件一覧
	DWORD SendAddManualAdd(
		vector<MANUAL_AUTO_ADD_DATA>* val
		);

	//プログラム予約自動登録の条件削除
	//戻り値：
	// エラーコード
	//引数：
	// val			[IN]条件一覧
	DWORD SendDelManualAdd(
		vector<DWORD>* val
		);

	//プログラム予約自動登録の条件変更
	//戻り値：
	// エラーコード
	//引数：
	// val			[IN]条件一覧
	DWORD SendChgManualAdd(
		vector<MANUAL_AUTO_ADD_DATA>* val
		);


	DWORD SendChkSuspend();

	DWORD SendSuspend(
		WORD val
		);

	DWORD SendReboot();

	DWORD SendEpgCapNow();

	//指定ファイルを転送する
	//戻り値：
	// エラーコード
	//引数：
	// val			[IN]ファイル名
	// resVal		[OUT]ファイルのバイナリデータ
	// resValSize	[OUT]resValのサイズ
	DWORD SendFileCopy(
		wstring val,
		BYTE** resVal,
		DWORD* resValSize
		);

	//PlugInファイルの一覧を取得する
	//戻り値：
	// エラーコード
	//引数：
	// val			[IN]1:ReName、2:Write
	// resVal		[OUT]ファイル名一覧
	DWORD SendEnumPlugIn(
		WORD val,
		vector<wstring>* resVal
		);

	//TVTestのチャンネル切り替え用の情報を取得する
	//戻り値：
	// エラーコード
	//引数：
	// val			[IN]ONID<<32 | TSID<<16 | SIDとしたサービスID
	// resVal		[OUT]チャンネル情報
	DWORD SendGetChgChTVTest(
		ULONGLONG val,
		TVTEST_CH_CHG_INFO* resVal
		);

	//ネットワークモードのEpgDataCap_Bonのチャンネルを切り替え
	//戻り値：
	// エラーコード
	//引数：
	// chInfo				[OUT]チャンネル情報
	DWORD SendNwTVSetCh(
		SET_CH_INFO* val
		);

	//ネットワークモードで起動中のEpgDataCap_Bonを終了
	//戻り値：
	// エラーコード
	//引数：
	// chInfo				[OUT]チャンネル情報
	DWORD SendNwTVClose(
		);

	//ネットワークモードで起動するときのモード
	//戻り値：
	// エラーコード
	//引数：
	// val				[OUT]モード（1:UDP 2:TCP 3:UDP+TCP）
	DWORD SendNwTVMode(
		DWORD val
		);

	//ストリーム配信用ファイルを開く
	//戻り値：
	// エラーコード
	//引数：
	// val				[IN]開くファイルのサーバー側ファイルパス
	// resVal			[OUT]制御用CtrlID
	DWORD SendNwPlayOpen(
		wstring val,
		DWORD* resVal
		);

	//ストリーム配信用ファイルを閉じる
	//戻り値：
	// エラーコード
	//引数：
	// val				[IN]制御用CtrlID
	DWORD SendNwPlayClose(
		DWORD val
		);

	//ストリーム配信開始
	//戻り値：
	// エラーコード
	//引数：
	// val				[IN]制御用CtrlID
	DWORD SendNwPlayStart(
		DWORD val
		);

	//ストリーム配信停止
	//戻り値：
	// エラーコード
	//引数：
	// val				[IN]制御用CtrlID
	DWORD SendNwPlayStop(
		DWORD val
		);

	//ストリーム配信で現在の送信位置と総ファイルサイズを取得する
	//戻り値：
	// エラーコード
	//引数：
	// val				[IN/OUT]サイズ情報
	DWORD SendNwPlayGetPos(
		NWPLAY_POS_CMD* val
		);

	//ストリーム配信で送信位置をシークする
	//戻り値：
	// エラーコード
	//引数：
	// val				[IN]サイズ情報
	DWORD SendNwPlaySetPos(
		NWPLAY_POS_CMD* val
		);

	//ストリーム配信で送信先を設定する
	//戻り値：
	// エラーコード
	//引数：
	// val				[IN]サイズ情報
	DWORD SendNwPlaySetIP(
		NWPLAY_PLAY_INFO* val
		);

	//ストリーム配信用ファイルをタイムシフトモードで開く
	//戻り値：
	// エラーコード
	//引数：
	// val				[IN]予約ID
	// resVal			[OUT]ファイルパスとCtrlID
	DWORD SendNwTimeShiftOpen(
		DWORD val,
		NWPLAY_TIMESHIFT_INFO* resVal
		);

//コマンドバージョン対応版
	//予約一覧を取得する
	//戻り値：
	// エラーコード
	//引数：
	// val			[OUT]予約一覧
	DWORD SendEnumReserve2(
		vector<RESERVE_DATA>* val
		);

	//予約情報を取得する
	//戻り値：
	// エラーコード
	//引数：
	// reserveID		[IN]取得する情報の予約ID
	// val				[OUT]予約情報
	DWORD SendGetReserve2(DWORD reserveID, RESERVE_DATA* val);

	//予約を追加する
	//戻り値：
	// エラーコード
	//引数：
	// val				[IN]追加する予約一覧
	DWORD SendAddReserve2(vector<RESERVE_DATA>* val);

	//予約を変更する
	//戻り値：
	// エラーコード
	//引数：
	// val				[IN]変更する予約一覧
	DWORD SendChgReserve2(vector<RESERVE_DATA>* val);

	//予約追加が可能か確認する
	//戻り値：
	// エラーコード
	//引数：
	// val				[IN]予約情報
	// resVal			[OUT]追加可能かのステータス
	DWORD SendAddChkReserve2(RESERVE_DATA* val, WORD* resVal);


	//EPGデータファイルのタイムスタンプ取得
	//戻り値：
	// エラーコード
	//引数：
	// val				[IN]取得ファイル名
	// resVal			[OUT]タイムスタンプ
	DWORD SendGetEpgFileTime2(wstring val, LONGLONG* resVal);

	//EPGデータファイル取得
	//戻り値：
	// エラーコード
	//引数：
	// val			[IN]ファイル名
	// resVal		[OUT]ファイルのバイナリデータ
	// resValSize	[OUT]resValのサイズ
	DWORD SendGetEpgFile2(
		wstring val,
		BYTE** resVal,
		DWORD* resValSize
		);

	//自動予約登録条件一覧を取得する
	//戻り値：
	// エラーコード
	//引数：
	// val			[OUT]条件一覧
	DWORD SendEnumEpgAutoAdd2(
		vector<EPG_AUTO_ADD_DATA>* val
		);

	//自動予約登録条件を追加する
	//戻り値：
	// エラーコード
	//引数：
	// val			[IN]条件一覧
	DWORD SendAddEpgAutoAdd2(
		vector<EPG_AUTO_ADD_DATA>* val
		);

	//自動予約登録条件を変更する
	//戻り値：
	// エラーコード
	//引数：
	// val			[IN]条件一覧
	DWORD SendChgEpgAutoAdd2(
		vector<EPG_AUTO_ADD_DATA>* val
		);

	//自動予約登録条件一覧を取得する
	//戻り値：
	// エラーコード
	//引数：
	// val			[OUT]条件一覧	
	DWORD SendEnumManualAdd2(
		vector<MANUAL_AUTO_ADD_DATA>* val
		);

	//自動予約登録条件を追加する
	//戻り値：
	// エラーコード
	//引数：
	// val			[IN]条件一覧
	DWORD SendAddManualAdd2(
		vector<MANUAL_AUTO_ADD_DATA>* val
		);

	//プログラム予約自動登録の条件変更
	//戻り値：
	// エラーコード
	//引数：
	// val			[IN]条件一覧
	DWORD SendChgManualAdd2(
		vector<MANUAL_AUTO_ADD_DATA>* val
		);

	//録画済み情報一覧取得
	//戻り値：
	// エラーコード
	//引数：
	// val			[OUT]録画済み情報一覧
	DWORD SendEnumRecInfo2(
		vector<REC_FILE_INFO>* val
		);

	//録画済み情報一覧取得
	//戻り値：
	// エラーコード
	//引数：
	// val			[OUT]録画済み情報一覧
	DWORD SendChgProtectRecInfo2(
		vector<REC_FILE_INFO>* val
		);

//タイマーGUI（EpgTimer_Bon.exe）用

	//ダイアログを前面に表示
	//戻り値：
	// エラーコード
	DWORD SendGUIShowDlg(
		);

	//予約一覧の情報が更新された
	//戻り値：
	// エラーコード
	DWORD SendGUIUpdateReserve(
		);

	//EPGデータの再読み込みが完了した
	//戻り値：
	// エラーコード
	DWORD SendGUIUpdateEpgData(
		);

	//情報更新を通知する
	//戻り値：
	// エラーコード
	//引数：
	// val				[IN]通知情報
	DWORD SendGUINotifyInfo2(NOTIFY_SRV_INFO* val);

//Viewアプリ（EpgDataCap_Bon.exe）を起動
	//戻り値：
	// エラーコード
	//引数：
	// exeCmd			[IN]コマンドライン
	// PID				[OUT]起動したexeのPID
	DWORD SendGUIExecute(
		wstring exeCmd,
		DWORD* PID
		);

	//スタンバイ、休止、シャットダウンに入っていいかの確認をユーザーに行う
	//戻り値：
	// エラーコード
	DWORD SendGUIQuerySuspend(
		BYTE rebootFlag,
		BYTE suspendMode
		);

	//PC再起動に入っていいかの確認をユーザーに行う
	//戻り値：
	// エラーコード
	DWORD SendGUIQueryReboot(
		BYTE rebootFlag
		);

	//サーバーのステータス変更通知
	//戻り値：
	// エラーコード
	//引数：
	// status			[IN]ステータス
	DWORD SendGUIStatusChg(
		WORD status
		);


//Viewアプリ（EpgDataCap_Bon.exe）用

	//BonDriverの切り替え
	//戻り値：
	// エラーコード
	//引数：
	// bonDriver			[IN]BonDriverファイル名
	DWORD SendViewSetBonDrivere(
		wstring bonDriver
		);

	//使用中のBonDriverのファイル名を取得
	//戻り値：
	// エラーコード
	//引数：
	// bonDriver			[OUT]BonDriverファイル名
	DWORD SendViewGetBonDrivere(
		wstring* bonDriver
		);

	//チャンネル切り替え
	//戻り値：
	// エラーコード
	//引数：
	// chInfo				[OUT]チャンネル情報
	DWORD SendViewSetCh(
		SET_CH_INFO* chInfo
		);

	//放送波の時間とPC時間の誤差取得
	//戻り値：
	// エラーコード
	//引数：
	// delaySec				[OUT]誤差（秒）
	DWORD SendViewGetDelay(
		int* delaySec
		);

	//現在の状態を取得
	//戻り値：
	// エラーコード
	//引数：
	// status				[OUT]状態
	DWORD SendViewGetStatus(
		DWORD* status
		);

	//現在の状態を取得
	//戻り値：
	// エラーコード
	DWORD SendViewAppClose(
		);

	//識別用IDの設定
	//戻り値：
	// エラーコード
	//引数：
	// id				[IN]ID
	DWORD SendViewSetID(
		int id
		);

	//識別用IDの取得
	//戻り値：
	// エラーコード
	//引数：
	// id				[OUT]ID
	DWORD SendViewGetID(
		int* id
		);

	//予約録画用にGUIキープ
	//戻り値：
	// エラーコード
	DWORD SendViewSetStandbyRec(
		DWORD keepFlag
		);

	//ストリーム制御用コントロール作成
	//戻り値：
	// エラーコード
	//引数：
	// ctrlID				[OUT]制御ID
	DWORD SendViewCreateCtrl(
		DWORD* ctrlID
		);

	//ストリーム制御用コントロール削除
	//戻り値：
	// エラーコード
	//引数：
	// ctrlID				[IN]制御ID
	DWORD SendViewDeleteCtrl(
		DWORD ctrlID
		);

	//制御コントロールの設定
	//戻り値：
	// エラーコード
	//引数：
	// val					[IN]設定値
	DWORD SendViewSetCtrlMode(
		SET_CTRL_MODE val
		);

	//録画処理開始
	//戻り値：
	// エラーコード
	//引数：
	// val					[IN]設定値
	DWORD SendViewStartRec(
		SET_CTRL_REC_PARAM val
		);

	//録画処理開始
	//戻り値：
	// エラーコード
	//引数：
	// val					[IN]設定値
	// resVal				[OUT]ドロップ数
	DWORD SendViewStopRec(
		SET_CTRL_REC_STOP_PARAM val,
		SET_CTRL_REC_STOP_RES_PARAM* resVal
		);

	//録画中のファイルパスを取得
	//戻り値：
	// エラーコード
	//引数：
	// val					[OUT]ファイルパス
	DWORD SendViewGetRecFilePath(
		DWORD ctrlID,
		wstring* resVal
		);

	//録画処理開始
	//戻り値：
	// エラーコード
	DWORD SendViewStopRecAll(
		);

	//ファイル出力したサイズを取得
	//戻り値：
	// エラーコード
	//引数：
	// resVal					[OUT]ファイル出力したサイズ
	DWORD SendViewGetWriteSize(
		DWORD ctrlID,
		__int64* resVal
		);

	//EPG取得開始
	//戻り値：
	// エラーコード
	//引数：
	// val					[IN]取得チャンネルリスト
	DWORD SendViewEpgCapStart(
		vector<SET_CH_INFO>* val
		);

	//EPG取得キャンセル
	//戻り値：
	// エラーコード
	DWORD SendViewEpgCapStop(
		);

	//EPGデータの検索
	//戻り値：
	// エラーコード
	// val					[IN]取得番組
	// resVal				[OUT]番組情報
	DWORD SendViewSearchEvent(
		SEARCH_EPG_INFO_PARAM* val,
		EPGDB_EVENT_INFO* resVal
		);

	//現在or次の番組情報を取得する
	//戻り値：
	// エラーコード
	// val					[IN]取得番組
	// resVal				[OUT]番組情報
	DWORD SendViewGetEventPF(
		GET_EPG_PF_INFO_PARAM* val,
		EPGDB_EVENT_INFO* resVal
		);

	//Viewボタン登録アプリ起動
	//戻り値：
	// エラーコード
	DWORD SendViewExecViewApp(
		);

//TVTest連携のストリーミング配信専用
	//ストリーミング配信制御IDの設定
	//戻り値：
	// エラーコード
	DWORD SendViewSetStreamingInfo(
		TVTEST_STREAMING_INFO* val
		);

protected:
	HANDLE lockEvent;

	BOOL tcpFlag;
	DWORD connectTimeOut;
	wstring eventName;
	wstring pipeName;
	wstring ip;
	DWORD port;

protected:
	//PublicAPI排他制御用
	BOOL Lock(LPCWSTR log = NULL, DWORD timeOut = 60*1000);
	void UnLock(LPCWSTR log = NULL);

	DWORD SendPipe(LPCWSTR pipeName, LPCWSTR eventName, DWORD timeOut, CMD_STREAM* send, CMD_STREAM* res);
	DWORD SendTCP(wstring ip, DWORD port, DWORD timeOut, CMD_STREAM* sendCmd, CMD_STREAM* resCmd);

	DWORD SendCmdStream(CMD_STREAM* send, CMD_STREAM* res);
	DWORD SendCmdWithoutData(DWORD param, CMD_STREAM* res = NULL);
	DWORD SendCmdWithoutData2(DWORD param, CMD_STREAM* res = NULL);
	template<class T> DWORD SendCmdData(DWORD param, const T& val, CMD_STREAM* res = NULL);
	template<class T> DWORD SendCmdData2(DWORD param, const T& val, CMD_STREAM* res = NULL);
	template<class T> DWORD ReceiveCmdData(DWORD param, T* resVal);
	template<class T> DWORD ReceiveCmdData2(DWORD param, T* resVal);
	template<class T, class U> DWORD SendAndReceiveCmdData(DWORD param, const T& val, U* resVal);
	template<class T, class U> DWORD SendAndReceiveCmdData2(DWORD param, const T& val, U* resVal);
};

#if 1 //インライン/テンプレート定義

inline DWORD CSendCtrlCmd::SendCmdWithoutData(DWORD param, CMD_STREAM* res)
{
	CMD_STREAM send;
	send.param = param;
	return SendCmdStream(&send, res);
}

inline DWORD CSendCtrlCmd::SendCmdWithoutData2(DWORD param, CMD_STREAM* res)
{
	return SendCmdData(param, (WORD)CMD_VER, res);
}

template<class T>
DWORD CSendCtrlCmd::SendCmdData(DWORD param, const T& val, CMD_STREAM* res)
{
	CMD_STREAM send;
	send.param = param;
	send.dataSize = GetVALUESize(val);
	send.data = new BYTE[send.dataSize];

	if( WriteVALUE(val, send.data, send.dataSize, NULL) == FALSE ){
		return CMD_ERR;
	}
	return SendCmdStream(&send, res);
}

template<class T>
DWORD CSendCtrlCmd::SendCmdData2(DWORD param, const T& val, CMD_STREAM* res)
{
	WORD ver = CMD_VER;
	CMD_STREAM send;
	send.param = param;
	send.dataSize = GetVALUESize(ver) + GetVALUESize2(ver, val);
	send.data = new BYTE[send.dataSize];

	DWORD writeSize = 0;
	if( WriteVALUE(ver, send.data, send.dataSize, &writeSize) == FALSE ||
		WriteVALUE2(ver, val, send.data + writeSize, send.dataSize - writeSize, NULL) == FALSE ){
		return CMD_ERR;
	}
	return SendCmdStream(&send, res);
}

template<class T>
DWORD CSendCtrlCmd::ReceiveCmdData(DWORD param, T* resVal)
{
	CMD_STREAM res;
	DWORD ret = SendCmdWithoutData(param, &res);

	if( ret == CMD_SUCCESS ){
		if( ReadVALUE(resVal, res.data, res.dataSize, NULL) == FALSE ){
			ret = CMD_ERR;
		}
	}
	return ret;
}

template<class T>
DWORD CSendCtrlCmd::ReceiveCmdData2(DWORD param, T* resVal)
{
	CMD_STREAM res;
	DWORD ret = SendCmdWithoutData2(param, &res);

	if( ret == CMD_SUCCESS ){
		WORD ver = 0;
		DWORD readSize = 0;
		if( ReadVALUE(&ver, res.data, res.dataSize, &readSize) == FALSE ||
			ReadVALUE2(ver, resVal, res.data + readSize, res.dataSize - readSize, NULL) == FALSE ){
			ret = CMD_ERR;
		}
	}
	return ret;
}

template<class T, class U>
DWORD CSendCtrlCmd::SendAndReceiveCmdData(DWORD param, const T& val, U* resVal)
{
	CMD_STREAM res;
	DWORD ret = SendCmdData(param, val, &res);

	if( ret == CMD_SUCCESS ){
		if( ReadVALUE(resVal, res.data, res.dataSize, NULL) == FALSE ){
			ret = CMD_ERR;
		}
	}
	return ret;
}

template<class T, class U>
DWORD CSendCtrlCmd::SendAndReceiveCmdData2(DWORD param, const T& val, U* resVal)
{
	CMD_STREAM res;
	DWORD ret = SendCmdData2(param, val, &res);

	if( ret == CMD_SUCCESS ){
		WORD ver = 0;
		DWORD readSize = 0;
		if( ReadVALUE(&ver, res.data, res.dataSize, &readSize) == FALSE ||
			ReadVALUE2(ver, resVal, res.data + readSize, res.dataSize - readSize, NULL) == FALSE ){
			ret = CMD_ERR;
		}
	}
	return ret;
}

#endif
