#pragma once

#include "StructDef.h"

#include "CtrlCmdDef.h"
#include "ErrDef.h"
#include "CtrlCmdUtil.h"

class CSendCtrlCmd
{
public:
	CSendCtrlCmd(void);
	~CSendCtrlCmd(void);

#if !defined(SEND_CTRL_CMD_NO_TCP) && defined(_WIN32)
	//送受信タイムアウト（接続先が要求を処理するのにかかる時間よりも十分に長く）
	static const DWORD SND_RCV_TIMEOUT = 30000;

	//コマンド送信方法の設定
	//引数：
	// tcpFlag		[IN] TRUE：TCP/IPモード、FALSE：名前付きパイプモード
	void SetSendMode(
		BOOL tcpFlag_
		);
#endif

	//名前付きパイプモード時の接続先を設定
	//EpgTimerSrv.exeに対するコマンドは設定しなくても可（デフォルト値になっている）
	//引数：
	// pipeName		[IN]接続パイプの名前
	void SetPipeSetting(
		LPCWSTR pipeName_
		);

	//名前付きパイプモード時の接続先を設定（接尾にプロセスIDを伴うタイプ）
	//引数：
	// pid			[IN]プロセスID
	void SetPipeSetting(
		LPCWSTR pipeName_,
		DWORD pid
		);

	//接続先パイプが存在するか調べる
	bool PipeExists();

	//TCP/IPモード時の接続先を設定
	//引数：
	// ip			[IN]接続先IP
	// port			[IN]接続先ポート
	void SetNWSetting(
		const wstring& ip,
		DWORD port
		);

	//接続処理時のタイムアウト設定
	// timeOut		[IN]タイムアウト値（単位：ms）
	void SetConnectTimeOut(
		DWORD timeOut
		);

	//EPGデータを再読み込みする
	//戻り値：
	// エラーコード
	DWORD SendReloadEpg(){
		return SendCmdWithoutData(CMD2_EPG_SRV_RELOAD_EPG);
	}

	//設定情報を再読み込みする
	//戻り値：
	// エラーコード
	DWORD SendReloadSetting(){
		return SendCmdWithoutData(CMD2_EPG_SRV_RELOAD_SETTING);
	}

	//EpgTimerSrv.exeのパイプ接続GUIとしてプロセスを登録する
	//戻り値：
	// エラーコード
	//引数：
	// processID			[IN]プロセスID
	DWORD SendRegistGUI(DWORD processID){
		return SendCmdData(CMD2_EPG_SRV_REGIST_GUI, processID);
	}

	//EpgTimerSrv.exeのパイプ接続GUI登録を解除する
	//戻り値：
	// エラーコード
	//引数：
	// processID			[IN]プロセスID
	DWORD SendUnRegistGUI(DWORD processID){
		return SendCmdData(CMD2_EPG_SRV_UNREGIST_GUI, processID);
	}

	//予約一覧を取得する
	//戻り値：
	// エラーコード
	//引数：
	// val			[OUT]予約一覧
	DWORD SendEnumReserve(
		vector<RESERVE_DATA>* val
		){
		return ReceiveCmdData(CMD2_EPG_SRV_ENUM_RESERVE, val);
	}

	//予約を削除する
	//戻り値：
	// エラーコード
	//引数：
	// val				[IN]削除する予約ID一覧
	DWORD SendDelReserve(const vector<DWORD>& val){
		return SendCmdData(CMD2_EPG_SRV_DEL_RESERVE, val);
	}

	DWORD SendChkSuspend(){
		return SendCmdWithoutData(CMD2_EPG_SRV_CHK_SUSPEND);
	}

	DWORD SendSuspend(
		WORD val
		){
		return SendCmdData(CMD2_EPG_SRV_SUSPEND, val);
	}

	DWORD SendReboot(){
		return SendCmdWithoutData(CMD2_EPG_SRV_REBOOT);
	}

	//設定ファイル(ini)の更新を通知させる
	//戻り値：
	//引数：
	// val			[IN]Sender
	// エラーコード
	DWORD SendProfileUpdate(
		const wstring& val
		){
		return SendCmdData(CMD2_EPG_SRV_PROFILE_UPDATE, val);
	}

	//ストリーム配信用ファイルを閉じる
	//戻り値：
	// エラーコード
	//引数：
	// val				[IN]制御用CtrlID
	DWORD SendNwPlayClose(
		DWORD val
		){
		return SendCmdData(CMD2_EPG_SRV_NWPLAY_CLOSE, val);
	}

	//ストリーム配信開始
	//戻り値：
	// エラーコード
	//引数：
	// val				[IN]制御用CtrlID
	DWORD SendNwPlayStart(
		DWORD val
		){
		return SendCmdData(CMD2_EPG_SRV_NWPLAY_PLAY, val);
	}

	//ストリーム配信停止
	//戻り値：
	// エラーコード
	//引数：
	// val				[IN]制御用CtrlID
	DWORD SendNwPlayStop(
		DWORD val
		){
		return SendCmdData(CMD2_EPG_SRV_NWPLAY_STOP, val);
	}

	//ストリーム配信で現在の送信位置と総ファイルサイズを取得する
	//戻り値：
	// エラーコード
	//引数：
	// val				[IN/OUT]サイズ情報
	DWORD SendNwPlayGetPos(
		NWPLAY_POS_CMD* val
		){
		return SendAndReceiveCmdData(CMD2_EPG_SRV_NWPLAY_GET_POS, *val, val);
	}

	//ストリーム配信で送信位置をシークする
	//戻り値：
	// エラーコード
	//引数：
	// val				[IN]サイズ情報
	DWORD SendNwPlaySetPos(
		const NWPLAY_POS_CMD* val
		){
		return SendCmdData(CMD2_EPG_SRV_NWPLAY_SET_POS, *val);
	}

	//ストリーム配信で送信先を設定する
	//戻り値：
	// エラーコード
	//引数：
	// val				[IN/OUT]サイズ情報
	DWORD SendNwPlaySetIP(
		NWPLAY_PLAY_INFO* val
		){
		return SendAndReceiveCmdData(CMD2_EPG_SRV_NWPLAY_SET_IP, *val, val);
	}

//タイマーGUI（EpgTimer_Bon.exe）用

	//予約一覧の情報が更新された
	//戻り値：
	// エラーコード
	DWORD SendGUIUpdateReserve(
		){
		return SendCmdWithoutData(CMD2_TIMER_GUI_UPDATE_RESERVE);
	}

	//EPGデータの再読み込みが完了した
	//戻り値：
	// エラーコード
	DWORD SendGUIUpdateEpgData(
		){
		return SendCmdWithoutData(CMD2_TIMER_GUI_UPDATE_EPGDATA);
	}

	//情報更新を通知する
	//戻り値：
	// エラーコード
	//引数：
	// val				[IN]通知情報
	DWORD SendGUINotifyInfo2(const NOTIFY_SRV_INFO& val){
		return SendCmdData2(CMD2_TIMER_GUI_SRV_STATUS_NOTIFY2, val);
	}

	//Viewアプリ（EpgDataCap_Bon.exe）を起動
	//戻り値：
	// エラーコード
	//引数：
	// exeCmd			[IN]コマンドライン
	// PID				[OUT]起動したexeのPID
	DWORD SendGUIExecute(
		const wstring& exeCmd,
		DWORD* PID
		){
		return SendAndReceiveCmdData(CMD2_TIMER_GUI_VIEW_EXECUTE, exeCmd, PID);
	}

	//スタンバイ、休止、シャットダウンに入っていいかの確認をユーザーに行う
	//戻り値：
	// エラーコード
	DWORD SendGUIQuerySuspend(
		BYTE rebootFlag,
		BYTE suspendMode
		){
		return SendCmdData(CMD2_TIMER_GUI_QUERY_SUSPEND, (WORD)(rebootFlag<<8|suspendMode));
	}

	//PC再起動に入っていいかの確認をユーザーに行う
	//戻り値：
	// エラーコード
	DWORD SendGUIQueryReboot(
		BYTE rebootFlag
		){
		return SendCmdData(CMD2_TIMER_GUI_QUERY_REBOOT, (WORD)(rebootFlag<<8));
	}

	//サーバーのステータス変更通知
	//戻り値：
	// エラーコード
	//引数：
	// status			[IN]ステータス
	DWORD SendGUIStatusChg(
		WORD status
		){
		return SendCmdData(CMD2_TIMER_GUI_SRV_STATUS_CHG, status);
	}


//Viewアプリ（EpgDataCap_Bon.exe）用

	//使用中のBonDriverのファイル名を取得
	//戻り値：
	// エラーコード
	//引数：
	// bonDriver			[OUT]BonDriverファイル名
	DWORD SendViewGetBonDrivere(
		wstring* bonDriver
		){
		return ReceiveCmdData(CMD2_VIEW_APP_GET_BONDRIVER, bonDriver);
	}

	//チャンネル切り替え
	//戻り値：
	// エラーコード
	//引数：
	// chInfo				[IN]チャンネル情報
	DWORD SendViewSetCh(
		const SET_CH_INFO& chInfo
		){
		return SendCmdData(CMD2_VIEW_APP_SET_CH, chInfo);
	}

	//放送波の時間とPC時間の誤差取得
	//戻り値：
	// エラーコード
	//引数：
	// delaySec				[OUT]誤差（秒）
	DWORD SendViewGetDelay(
		int* delaySec
		){
		return ReceiveCmdData(CMD2_VIEW_APP_GET_DELAY, delaySec);
	}

	//現在の状態を取得
	//戻り値：
	// エラーコード
	//引数：
	// status				[OUT]状態
	DWORD SendViewGetStatus(
		DWORD* status
		){
		return ReceiveCmdData(CMD2_VIEW_APP_GET_STATUS, status);
	}

	//アプリケーションの終了
	//戻り値：
	// エラーコード
	DWORD SendViewAppClose(
		){
		return SendCmdWithoutData(CMD2_VIEW_APP_CLOSE);
	}

	//識別用IDの設定
	//戻り値：
	// エラーコード
	//引数：
	// id				[IN]ID
	DWORD SendViewSetID(
		int id
		){
		return SendCmdData(CMD2_VIEW_APP_SET_ID, id);
	}

	//識別用IDの取得
	//戻り値：
	// エラーコード
	//引数：
	// id				[OUT]ID
	DWORD SendViewGetID(
		int* id
		){
		return ReceiveCmdData(CMD2_VIEW_APP_GET_ID, id);
	}

	//予約録画用にGUIキープ
	//戻り値：
	// エラーコード
	DWORD SendViewSetStandbyRec(
		DWORD keepFlag
		){
		return SendCmdData(CMD2_VIEW_APP_SET_STANDBY_REC, keepFlag);
	}

	//ストリーム制御用コントロール作成
	//戻り値：
	// エラーコード
	//引数：
	// ctrlID				[OUT]制御ID
	DWORD SendViewCreateCtrl(
		DWORD* ctrlID
		){
		return ReceiveCmdData(CMD2_VIEW_APP_CREATE_CTRL, ctrlID);
	}

	//ストリーム制御用コントロール削除
	//戻り値：
	// エラーコード
	//引数：
	// ctrlID				[IN]制御ID
	DWORD SendViewDeleteCtrl(
		DWORD ctrlID
		){
		return SendCmdData(CMD2_VIEW_APP_DELETE_CTRL, ctrlID);
	}

	//制御コントロールの設定
	//戻り値：
	// エラーコード
	//引数：
	// val					[IN]設定値
	DWORD SendViewSetCtrlMode(
		const SET_CTRL_MODE& val
		){
		return SendCmdData(CMD2_VIEW_APP_SET_CTRLMODE, val);
	}

	//録画処理開始
	//戻り値：
	// エラーコード
	//引数：
	// val					[IN]設定値
	DWORD SendViewStartRec(
		const SET_CTRL_REC_PARAM& val
		){
		return SendCmdData(CMD2_VIEW_APP_REC_START_CTRL, val);
	}

	//録画処理停止
	//戻り値：
	// エラーコード
	//引数：
	// val					[IN]設定値
	// resVal				[OUT]ドロップ数
	DWORD SendViewStopRec(
		const SET_CTRL_REC_STOP_PARAM& val,
		SET_CTRL_REC_STOP_RES_PARAM* resVal
		){
		return SendAndReceiveCmdData(CMD2_VIEW_APP_REC_STOP_CTRL, val, resVal);
	}

	//録画中のファイルパスを取得
	//戻り値：
	// エラーコード
	//引数：
	// val					[OUT]ファイルパス
	DWORD SendViewGetRecFilePath(
		DWORD ctrlID,
		wstring* resVal
		){
		return SendAndReceiveCmdData(CMD2_VIEW_APP_REC_FILE_PATH, ctrlID, resVal);
	}

	//EPG取得開始
	//戻り値：
	// エラーコード
	//引数：
	// val					[IN]取得チャンネルリスト
	DWORD SendViewEpgCapStart(
		const vector<SET_CH_INFO>& val
		){
		return SendCmdData(CMD2_VIEW_APP_EPGCAP_START, val);
	}

	//EPG取得キャンセル
	//戻り値：
	// エラーコード
	DWORD SendViewEpgCapStop(
		){
		return SendCmdWithoutData(CMD2_VIEW_APP_EPGCAP_STOP);
	}

	//EPGデータの検索
	//戻り値：
	// エラーコード
	// val					[IN]取得番組
	// resVal				[OUT]番組情報
	DWORD SendViewSearchEvent(
		const SEARCH_EPG_INFO_PARAM& val,
		EPGDB_EVENT_INFO* resVal
		){
		return SendAndReceiveCmdData(CMD2_VIEW_APP_SEARCH_EVENT, val, resVal);
	}

	//現在or次の番組情報を取得する
	//戻り値：
	// エラーコード
	// val					[IN]取得番組
	// resVal				[OUT]番組情報
	DWORD SendViewGetEventPF(
		const GET_EPG_PF_INFO_PARAM& val,
		EPGDB_EVENT_INFO* resVal
		){
		return SendAndReceiveCmdData(CMD2_VIEW_APP_GET_EVENT_PF, val, resVal);
	}

	//Viewボタン登録アプリ起動
	//戻り値：
	// エラーコード
	DWORD SendViewExecViewApp(
		){
		return SendCmdWithoutData(CMD2_VIEW_APP_EXEC_VIEW_APP);
	}

private:
	BOOL tcpFlag;
	DWORD connectTimeOut;
	wstring pipeName;
	wstring sendIP;
	DWORD sendPort;

	CSendCtrlCmd(const CSendCtrlCmd&);
	CSendCtrlCmd& operator=(const CSendCtrlCmd&);
	DWORD SendCmdStream(const CMD_STREAM* cmd, CMD_STREAM* res);
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
	CMD_STREAM cmd;
	cmd.param = param;
	return SendCmdStream(&cmd, res);
}

inline DWORD CSendCtrlCmd::SendCmdWithoutData2(DWORD param, CMD_STREAM* res)
{
	return SendCmdData(param, (WORD)CMD_VER, res);
}

template<class T>
DWORD CSendCtrlCmd::SendCmdData(DWORD param, const T& val, CMD_STREAM* res)
{
	CMD_STREAM cmd;
	cmd.param = param;
	cmd.data = NewWriteVALUE(val, cmd.dataSize);
	if( cmd.data == NULL ){
		return CMD_ERR;
	}
	return SendCmdStream(&cmd, res);
}

template<class T>
DWORD CSendCtrlCmd::SendCmdData2(DWORD param, const T& val, CMD_STREAM* res)
{
	WORD ver = CMD_VER;
	CMD_STREAM cmd;
	cmd.param = param;
	cmd.data = NewWriteVALUE2WithVersion(ver, val, cmd.dataSize);
	if( cmd.data == NULL ){
		return CMD_ERR;
	}
	return SendCmdStream(&cmd, res);
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
			ReadVALUE2(ver, resVal, res.data.get() + readSize, res.dataSize - readSize, NULL) == FALSE ){
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
			ReadVALUE2(ver, resVal, res.data.get() + readSize, res.dataSize - readSize, NULL) == FALSE ){
			ret = CMD_ERR;
		}
	}
	return ret;
}

#endif
