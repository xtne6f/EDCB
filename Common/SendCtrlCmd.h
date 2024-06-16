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

	//EPG再読み込みを開始する
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

	//EpgTimerSrvのパイプ接続GUIとしてプロセスを登録する（通知が受信可能になる）
	//戻り値：
	// エラーコード
	//引数：
	// processID			[IN]プロセスID
	DWORD SendRegistGUI(DWORD processID){
		return SendCmdData(CMD2_EPG_SRV_REGIST_GUI, processID);
	}

	//EpgTimerSrvのパイプ接続GUI登録を解除する
	//戻り値：
	// エラーコード
	//引数：
	// processID			[IN]プロセスID
	DWORD SendUnRegistGUI(DWORD processID){
		return SendCmdData(CMD2_EPG_SRV_UNREGIST_GUI, processID);
	}

	//予約一覧取得
	//戻り値：
	// エラーコード
	//引数：
	// val			[OUT]予約一覧
	DWORD SendEnumReserve(
		vector<RESERVE_DATA>* val
		){
		return ReceiveCmdData(CMD2_EPG_SRV_ENUM_RESERVE, val);
	}

	//予約削除
	//戻り値：
	// エラーコード
	//引数：
	// val				[IN]削除する予約ID一覧
	DWORD SendDelReserve(const vector<DWORD>& val){
		return SendCmdData(CMD2_EPG_SRV_DEL_RESERVE, val);
	}

	//スタンバイ、休止、シャットダウンを行っていいかの確認
	//戻り値：
	// エラーコード
	DWORD SendChkSuspend(){
		return SendCmdWithoutData(CMD2_EPG_SRV_CHK_SUSPEND);
	}

	//スタンバイ、休止、シャットダウンに移行する
	//戻り値：
	// エラーコード
	//引数：
	// val				[IN]1:スタンバイ 2:休止 3:シャットダウン | 0x0100:復帰後再起動
	DWORD SendSuspend(
		WORD val
		){
		return SendCmdData(CMD2_EPG_SRV_SUSPEND, val);
	}

	//PC再起動を行う
	//戻り値：
	// エラーコード
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

	// ***** 以下、タイマーGUI（EpgTimerなど）用 *****

	//予約情報などが更新されたことを通知する（旧仕様との互換用）
	//戻り値：
	// エラーコード
	DWORD SendGUIUpdateReserve(
		){
		return SendCmdWithoutData(CMD2_TIMER_GUI_UPDATE_RESERVE);
	}

	//EPGデータが更新されたことを通知する（旧仕様との互換用）
	//戻り値：
	// エラーコード
	DWORD SendGUIUpdateEpgData(
		){
		return SendCmdWithoutData(CMD2_TIMER_GUI_UPDATE_EPGDATA);
	}

	//サーバーの情報変更を通知する
	//戻り値：
	// エラーコード
	//引数：
	// val				[IN]通知情報
	DWORD SendGUINotifyInfo2(const NOTIFY_SRV_INFO& val){
		return SendCmdData2(CMD2_TIMER_GUI_SRV_STATUS_NOTIFY2, val);
	}

	//Viewアプリ（EpgDataCap_Bonなど）を起動
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

	//スタンバイ、休止、シャットダウンに入っていいかユーザーに確認する（了解ならタイマーGUIはCMD_EPG_SRV_SUSPENDを送る）
	//戻り値：
	// エラーコード
	DWORD SendGUIQuerySuspend(
		BYTE rebootFlag,
		BYTE suspendMode
		){
		return SendCmdData(CMD2_TIMER_GUI_QUERY_SUSPEND, (WORD)(rebootFlag<<8|suspendMode));
	}

	//PC再起動に入っていいかユーザーに確認する（了解ならタイマーGUIはCMD_EPG_SRV_REBOOTを送る）
	//戻り値：
	// エラーコード
	DWORD SendGUIQueryReboot(
		BYTE rebootFlag
		){
		return SendCmdData(CMD2_TIMER_GUI_QUERY_REBOOT, (WORD)(rebootFlag<<8));
	}

	//サーバーの動作状況の変更を通知する（旧仕様との互換用）
	//戻り値：
	// エラーコード
	//引数：
	// status			[IN]1:通常、2:EPGデータ取得開始、3:予約録画開始
	DWORD SendGUIStatusChg(
		WORD status
		){
		return SendCmdData(CMD2_TIMER_GUI_SRV_STATUS_CHG, status);
	}

	// ***** 以下、Viewアプリ（EpgDataCap_Bonなど）用 *****

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

	//現在の状態を詳細に取得
	//戻り値：
	// エラーコード
	//引数：
	// flags				[IN]VIEW_APP_FLAG_GET_*
	// info					[OUT]ステータス情報
	DWORD SendViewGetStatusDetails(
		DWORD flags,
		VIEW_APP_STATUS_INFO* info
		){
		return SendAndReceiveCmdData(CMD2_VIEW_APP_GET_STATUS_DETAILS, flags, info);
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

	//コントロールの動作を設定
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

	//EPG取得停止
	//戻り値：
	// エラーコード
	DWORD SendViewEpgCapStop(
		){
		return SendCmdWithoutData(CMD2_VIEW_APP_EPGCAP_STOP);
	}

	//番組情報の検索
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

	//Viewボタン登録アプリの起動
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
	DWORD SendCmdStream(const CCmdStream& cmd, CCmdStream* res);
	DWORD SendCmdWithoutData(DWORD param, CCmdStream* res = NULL);
	DWORD SendCmdWithoutData2(DWORD param, CCmdStream* res = NULL);
	template<class T> DWORD SendCmdData(DWORD param, const T& val, CCmdStream* res = NULL);
	template<class T> DWORD SendCmdData2(DWORD param, const T& val, CCmdStream* res = NULL);
	template<class T> DWORD ReceiveCmdData(DWORD param, T* resVal);
	template<class T> DWORD ReceiveCmdData2(DWORD param, T* resVal);
	template<class T, class U> DWORD SendAndReceiveCmdData(DWORD param, const T& val, U* resVal);
	template<class T, class U> DWORD SendAndReceiveCmdData2(DWORD param, const T& val, U* resVal);
};

#if 1 //インライン/テンプレート定義

inline DWORD CSendCtrlCmd::SendCmdWithoutData(DWORD param, CCmdStream* res)
{
	return SendCmdStream(CCmdStream(param), res);
}

inline DWORD CSendCtrlCmd::SendCmdWithoutData2(DWORD param, CCmdStream* res)
{
	return SendCmdData(param, (WORD)CMD_VER, res);
}

template<class T>
DWORD CSendCtrlCmd::SendCmdData(DWORD param, const T& val, CCmdStream* res)
{
	CCmdStream cmd(param);
	cmd.WriteVALUE(val);
	return SendCmdStream(cmd, res);
}

template<class T>
DWORD CSendCtrlCmd::SendCmdData2(DWORD param, const T& val, CCmdStream* res)
{
	WORD ver = CMD_VER;
	CCmdStream cmd(param);
	cmd.WriteVALUE2WithVersion(ver, val);
	return SendCmdStream(cmd, res);
}

template<class T>
DWORD CSendCtrlCmd::ReceiveCmdData(DWORD param, T* resVal)
{
	CCmdStream res;
	DWORD ret = SendCmdWithoutData(param, &res);

	if( ret == CMD_SUCCESS ){
		if( !res.ReadVALUE(resVal) ){
			ret = CMD_ERR;
		}
	}
	return ret;
}

template<class T>
DWORD CSendCtrlCmd::ReceiveCmdData2(DWORD param, T* resVal)
{
	CCmdStream res;
	DWORD ret = SendCmdWithoutData2(param, &res);

	if( ret == CMD_SUCCESS ){
		WORD ver = 0;
		if( !res.ReadVALUE2WithVersion(&ver, resVal) ){
			ret = CMD_ERR;
		}
	}
	return ret;
}

template<class T, class U>
DWORD CSendCtrlCmd::SendAndReceiveCmdData(DWORD param, const T& val, U* resVal)
{
	CCmdStream res;
	DWORD ret = SendCmdData(param, val, &res);

	if( ret == CMD_SUCCESS ){
		if( !res.ReadVALUE(resVal) ){
			ret = CMD_ERR;
		}
	}
	return ret;
}

template<class T, class U>
DWORD CSendCtrlCmd::SendAndReceiveCmdData2(DWORD param, const T& val, U* resVal)
{
	CCmdStream res;
	DWORD ret = SendCmdData2(param, val, &res);

	if( ret == CMD_SUCCESS ){
		WORD ver = 0;
		if( !res.ReadVALUE2WithVersion(&ver, resVal) ){
			ret = CMD_ERR;
		}
	}
	return ret;
}

#endif
