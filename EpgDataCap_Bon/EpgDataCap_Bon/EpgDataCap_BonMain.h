#pragma once

#include "../../BonCtrl/BonCtrl.h"
#include "../../Common/PathUtil.h"
#include "../../Common/TimeUtil.h"
#include "../../Common/PipeServer.h"

class CEpgDataCap_BonMain
{
public:
	CEpgDataCap_BonMain(void);
	~CEpgDataCap_BonMain(void);

	//通知用HWNDを設定
	void SetHwnd(HWND wnd);

	//設定を行う
	void ReloadSetting();
	
	//BonDriverフォルダのBonDriver_*.dllを列挙
	//戻り値：
	// エラーコード
	//引数：
	// bonList			[OUT]検索できたBonDriver一覧
	DWORD EnumBonDriver(
		vector<wstring>* bonList
		);

	//BonDriverをロードしてチャンネル情報などを取得（ファイル名で指定）
	//戻り値：
	// エラーコード
	//引数：
	// bonDriverFile	[IN]EnumBonDriverで取得されたBonDriverのファイル名
	DWORD OpenBonDriver(
		LPCWSTR bonDriverFile
		);

	//ロードしているBonDriverの開放
	//戻り値：
	// エラーコード
	DWORD CloseBonDriver();

	//サービス一覧を取得する
	//戻り値：
	// エラーコード
	//引数：
	// serviceList				[OUT]サービス情報のリスト
	DWORD GetServiceList(
		vector<CH_DATA4>* serviceList
		);

	//ロード中のBonDriverのファイル名を取得する（ロード成功しているかの判定）
	//戻り値：
	// TRUE（成功）：FALSE（Openに失敗している）
	//引数：
	// bonDriverFile		[OUT]BonDriverのファイル名(NULL可)
	BOOL GetOpenBonDriver(
		wstring* bonDriverFile
		);

	//チャンネル変更
	//戻り値：
	// エラーコード
	//引数：
	// ONID			[IN]変更チャンネルのorignal_network_id
	// TSID			[IN]変更チャンネルのtransport_stream_id
	// SID			[IN]変更チャンネルのservice_id
	DWORD SetCh(
		WORD ONID,
		WORD TSID,
		WORD SID
		);

	//チャンネル変更
	//戻り値：
	// エラーコード
	//引数：
	// ONID			[IN]変更チャンネルのorignal_network_id
	// TSID			[IN]変更チャンネルのtransport_stream_id
	// SID			[IN]変更チャンネルのservice_id
	// space		[IN]変更チャンネルのspace
	// ch			[IN]変更チャンネルのch
	DWORD SetCh(
		WORD ONID,
		WORD TSID,
		WORD SID,
		DWORD space,
		DWORD ch
		);

	//現在のサービス取得
	//戻り値：
	// エラーコード
	//引数：
	// ONID			[IN]現在のorignal_network_id
	// TSID			[IN]現在のtransport_stream_id
	// SID			[IN]現在のservice_id
	void GetCh(
		WORD* ONID,
		WORD* TSID,
		WORD* SID
		);

	//チャンネル変更中かどうか
	//戻り値：
	// TRUE（変更中）、FALSE（完了）
	BOOL IsChChanging(BOOL* chChgErr);

	//外部制御などでCH変更された場合にSIDのみ設定
	void SetSID(
		WORD SID
		);

	//UDPで送信を行う
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// enableFlag		[IN]TRUE（開始）、FALSE（停止）
	BOOL SendUDP(
		BOOL enableFlag
		);

	//TCPで送信を行う
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// enableFlag		[IN]TRUE（開始）、FALSE（停止）
	BOOL SendTCP(
		BOOL enableFlag
		);

	//UDP送信の設定数を取得
	DWORD GetCountUDP();
	//TCP送信の設定数を取得
	DWORD GetCountTCP();
	//UDPの送信中先一覧取得
	BOOL GetSendUDPList(
		vector<NW_SEND_INFO>* sendList
		);
	//TCPの送信中先一覧取得
	BOOL GetSendTCPList(
		vector<NW_SEND_INFO>* sendList
		);

	//指定サービスの現在or次のEPG情報を取得する
	//戻り値：
	// エラーコード
	//引数：
	// nextFlag					[IN]TRUE（次の番組）、FALSE（現在の番組）
	// epgInfo					[OUT]EPG情報
	DWORD GetEpgInfo(
		BOOL nextFlag,
		wstring* epgInfo
		);

	//シグナルレベルの取得
	//戻り値：
	// シグナルレベル
	float GetSignalLevel();

	//エラーカウントをクリアする
	void ClearErrCount(
		);

	//ドロップとスクランブルのカウントを取得する
	//引数：
	// drop				[OUT]ドロップ数
	// scramble			[OUT]スクランブル数
	void GetErrCount(
		ULONGLONG* drop,
		ULONGLONG* scramble
		);

	//録画を開始する
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	BOOL StartRec(
		);

	//録画を停止する
	// TRUE（成功）、FALSE（失敗）
	BOOL StopRec();

	//録画中かどうかを取得する
	// TRUE（録画中）、FALSE（録画していない）
	BOOL IsRec();

	//予約録画を停止する
	void StopReserveRec();

	//チャンネルスキャンを開始する
	//戻り値：
	// エラーコード
	DWORD StartChScan();

	//チャンネルスキャンをキャンセルする
	//戻り値：
	// エラーコード
	DWORD StopChScan();

	//チャンネルスキャンの状態を取得する
	//戻り値：
	// エラーコード
	//引数：
	// space		[OUT]スキャン中の物理CHのspace
	// ch			[OUT]スキャン中の物理CHのch
	// chName		[OUT]スキャン中の物理CHの名前
	// chkNum		[OUT]チェック済みの数
	// totalNum		[OUT]チェック対象の総数
	DWORD GetChScanStatus(
		DWORD* space,
		DWORD* ch,
		wstring* chName,
		DWORD* chkNum,
		DWORD* totalNum
		);

	//EPG取得を開始する
	//戻り値：
	// エラーコード
	DWORD StartEpgCap(
		);

	//EPG取得を停止する
	//戻り値：
	// エラーコード
	DWORD StopEpgCap(
		);

	//EPG取得のステータスを取得する
	//戻り値：
	// エラーコード
	//引数：
	// info			[OUT]取得中のサービス
	DWORD GetEpgCapStatus(EPGCAP_SERVICE_INFO* info);

	//Viewアプリの起動を行う
	void ViewAppOpen();

	void StartServer();
	void StopServer();

	BOOL GetViewStatusInfo(
		float* signal,
		DWORD* space,
		DWORD* ch,
		ULONGLONG* drop,
		ULONGLONG* scramble,
		vector<NW_SEND_INFO>* sendUdpList,
		vector<NW_SEND_INFO>* sendTcpList
		);

	void CtrlCmdCallbackInvoked();

protected:
	HWND msgWnd;

	CBonCtrl bonCtrl;
	CPipeServer pipeServer;

	vector<wstring> recFolderList;

	DWORD nwCtrlID;
	BOOL sendUdpFlag;
	BOOL sendTcpFlag;
	DWORD udpCount;
	DWORD tcpCount;
	vector<NW_SEND_INFO> udpSendList;
	vector<NW_SEND_INFO> tcpSendList;

	int startMargine;
	int endMargine;
	BOOL overWriteFlag;

	BOOL enableScrambleFlag;
	BOOL enableEMMFlag;

	BOOL allService;
	BOOL needCaption;
	BOOL needData;

	int openWait;

	wstring viewPath;
	wstring viewOpt;

	WORD lastONID;
	WORD lastTSID;
	WORD lastSID;

	DWORD recCtrlID;

	wstring currentBonDriver;
	int outCtrlID;
	map<DWORD, DWORD> ctrlMap;

	CMD_STREAM* cmdCapture;
	CMD_STREAM* resCapture;

protected:
	//外部制御コマンド関係
	static int CALLBACK CtrlCmdCallback(void* param, CMD_STREAM* cmdParam, CMD_STREAM* resParam);
};

