#pragma once

#include "../Common/StructDef.h"
#include "../Common/EpgTimerUtil.h"
#include "../Common/StringUtil.h"
#include "../Common/ThreadUtil.h"

#include "BonDriverUtil.h"
#include "PacketInit.h"
#include "TSOut.h"
#include "ChSetUtil.h"
#include <list>

class CBonCtrl
{
public:
	//チャンネルスキャン、EPG取得のステータス用
	enum JOB_STATUS {
		ST_STOP = -4,	//停止中
		ST_WORKING,		//実行中
		ST_COMPLETE,	//完了
		ST_CANCEL,		//キャンセルされた
	};

	CBonCtrl(void);
	~CBonCtrl(void);

	//BonDriverフォルダを指定
	//引数：
	// bonDriverFolderPath		[IN]BonDriverフォルダパス
	void SetBonDriverFolder(
		LPCWSTR bonDriverFolderPath
		);

	void SetEMMMode(BOOL enable) { this->tsOut.SetEmm(enable); }

	void SetNoLogScramble(BOOL noLog) { this->tsOut.SetNoLogScramble(noLog); }

	void SetParseEpgPostProcess(BOOL parsePost) { this->tsOut.SetParseEpgPostProcess(parsePost); }

	//BonDriverフォルダのBonDriver_*.dllを列挙
	//戻り値：
	// 検索できたBonDriver一覧
	vector<wstring> EnumBonDriver();

	//BonDriverをロードしてチャンネル情報などを取得（ファイル名で指定）
	//戻り値：
	// エラーコード
	//引数：
	// bonDriverFile	[IN]EnumBonDriverで取得されたBonDriverのファイル名
	DWORD OpenBonDriver(
		LPCWSTR bonDriverFile,
		int openWait,
		DWORD tsBuffMaxCount
		);

	//ロードしているBonDriverの開放
	void CloseBonDriver();

	//ロード中のBonDriverのファイル名を取得する（ロード成功しているかの判定）
	//※スレッドセーフ
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
	// space			[IN]変更チャンネルのSpace
	// ch				[IN]変更チャンネルの物理Ch
	DWORD SetCh(
		DWORD space,
		DWORD ch
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

	//チャンネル変更中かどうか
	//※スレッドセーフ
	//戻り値：
	// TRUE（変更中）、FALSE（完了）
	BOOL IsChChanging(BOOL* chChgErr);

	//現在のストリームのIDを取得する
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// ONID		[OUT]originalNetworkID
	// TSID		[OUT]transportStreamID
	BOOL GetStreamID(
		WORD* ONID,
		WORD* TSID
		);

	//サービス一覧を取得する
	//戻り値：
	// エラーコード
	//引数：
	// serviceList				[OUT]サービス情報のリスト
	DWORD GetServiceList(
		vector<CH_DATA4>* serviceList
		);

	//TSストリーム制御用コントロールを作成する
	//戻り値：
	// 制御識別ID
	//引数：
	// sendUdpTcp	[IN]UDP/TCP送信用にする
	DWORD CreateServiceCtrl(
		BOOL sendUdpTcp
		);

	//TSストリーム制御用コントロールを作成する
	//戻り値：
	// エラーコード
	//引数：
	// id			[IN]制御識別ID
	BOOL DeleteServiceCtrl(
		DWORD id
		);

	//制御対象のサービスを設定する
	//戻り値：
	// TRUE（成功）、FALSE（失敗
	//引数：
	// id			[IN]制御識別ID
	// serviceID	[IN]対象サービスID、0xFFFFで全サービス対象
	BOOL SetServiceID(
		DWORD id,
		WORD serviceID
		);

	BOOL GetServiceID(
		DWORD id,
		WORD* serviceID
		);

	//UDPで送信を行う
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// id			[IN]制御識別ID
	// sendList		[IN/OUT]送信先リスト。NULLで停止。Portは実際に送信に使用したPortが返る。
	BOOL SendUdp(
		DWORD id,
		vector<NW_SEND_INFO>* sendList
		);

	//TCPで送信を行う
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// id			[IN]制御識別ID
	// sendList		[IN/OUT]送信先リスト。NULLで停止。Portは実際に送信に使用したPortが返る。
	BOOL SendTcp(
		DWORD id,
		vector<NW_SEND_INFO>* sendList
		);

	//ファイル保存を開始する
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// recParam				[IN]保存パラメータ
	// saveFolderSub		[IN]HDDの空きがなくなった場合に一時的に使用するフォルダ
	// writeBuffMaxCount	[IN]出力バッファ上限
	BOOL StartSave(
		const SET_CTRL_REC_PARAM& recParam,
		const vector<wstring>& saveFolderSub,
		int writeBuffMaxCount
	);

	//ファイル保存を終了する
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// id			[IN]制御識別ID
	// subRecFlag	[OUT]成功のとき、サブ録画が発生したかどうか
	BOOL EndSave(
		DWORD id,
		BOOL* subRecFlag = NULL
		);

	//スクランブル解除処理の動作設定
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// id			[IN]制御識別ID
	// enable		[IN] TRUE（処理する）、FALSE（処理しない）
	BOOL SetScramble(
		DWORD id,
		BOOL enable
		);

	//字幕とデータ放送含めるかどうか
	//引数：
	// id					[IN]制御識別ID
	// enableCaption		[IN]字幕を TRUE（含める）、FALSE（含めない）
	// enableData			[IN]データ放送を TRUE（含める）、FALSE（含めない）
	void SetServiceMode(
		DWORD id,
		BOOL enableCaption,
		BOOL enableData
		);

	//エラーカウントをクリアする
	//引数：
	// id					[IN]制御識別ID
	void ClearErrCount(
		DWORD id
		);

	//ドロップとスクランブルのカウントを取得する
	//引数：
	// id					[IN]制御識別ID
	// drop					[OUT]ドロップ数
	// scramble				[OUT]スクランブル数
	void GetErrCount(
		DWORD id,
		ULONGLONG* drop,
		ULONGLONG* scramble
		);

	//録画中のファイルのファイルパスを取得する
	//※スレッドセーフ
	//戻り値：
	// ファイルパス
	//引数：
	// id					[IN]制御識別ID
	wstring GetSaveFilePath(
		DWORD id
		) {
		return this->tsOut.GetSaveFilePath(id);
	}

	//ドロップとスクランブルのカウントを保存する
	//引数：
	// id					[IN]制御識別ID
	// filePath				[IN]保存ファイル名
	void SaveErrCount(
		DWORD id,
		wstring filePath
		);

	//録画中のファイルの出力サイズを取得する
	//引数：
	// id					[IN]制御識別ID
	// writeSize			[OUT]保存ファイル名
	void GetRecWriteSize(
		DWORD id,
		__int64* writeSize
		);

	//指定サービスの現在or次のEPG情報を取得する
	//※スレッドセーフ
	//戻り値：
	// エラーコード
	//引数：
	// originalNetworkID		[IN]取得対象のoriginalNetworkID
	// transportStreamID		[IN]取得対象のtransportStreamID
	// serviceID				[IN]取得対象のServiceID
	// nextFlag					[IN]TRUE（次の番組）、FALSE（現在の番組）
	// epgInfo					[OUT]EPG情報
	DWORD GetEpgInfo(
		WORD originalNetworkID,
		WORD transportStreamID,
		WORD serviceID,
		BOOL nextFlag,
		EPGDB_EVENT_INFO* epgInfo
		) {
		return this->tsOut.GetEpgInfo(originalNetworkID, transportStreamID, serviceID, nextFlag, epgInfo);
	}

	//指定イベントのEPG情報を取得する
	//※スレッドセーフ
	//戻り値：
	// エラーコード
	//引数：
	// originalNetworkID		[IN]取得対象のoriginalNetworkID
	// transportStreamID		[IN]取得対象のtransportStreamID
	// serviceID				[IN]取得対象のServiceID
	// eventID					[IN]取得対象のEventID
	// pfOnlyFlag				[IN]p/fからのみ検索するかどうか
	// epgInfo					[OUT]EPG情報
	DWORD SearchEpgInfo(
		WORD originalNetworkID,
		WORD transportStreamID,
		WORD serviceID,
		WORD eventID,
		BYTE pfOnlyFlag,
		EPGDB_EVENT_INFO* epgInfo
		) {
		return this->tsOut.SearchEpgInfo(originalNetworkID, transportStreamID, serviceID, eventID, pfOnlyFlag, epgInfo);
	}
	
	//PC時計を元としたストリーム時間との差を取得する
	//※スレッドセーフ
	//戻り値：
	// 差の秒数
	int GetTimeDelay() { return this->tsOut.GetTimeDelay(); }

	//録画中かどうかを取得する
	//※スレッドセーフ
	// TRUE（録画中）、FALSE（録画していない）
	BOOL IsRec() { return this->tsOut.IsRec(); }

	//チャンネルスキャンを開始する
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	BOOL StartChScan();

	//チャンネルスキャンをキャンセルする
	void StopChScan();

	//チャンネルスキャンの状態を取得する
	//戻り値：
	// ステータス
	//引数：
	// space		[OUT]スキャン中の物理CHのspace
	// ch			[OUT]スキャン中の物理CHのch
	// chName		[OUT]スキャン中の物理CHの名前
	// chkNum		[OUT]チェック済みの数
	// totalNum		[OUT]チェック対象の総数
	JOB_STATUS GetChScanStatus(
		DWORD* space,
		DWORD* ch,
		wstring* chName,
		DWORD* chkNum,
		DWORD* totalNum
		);

	//EPG取得を開始する
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// chList		[IN]EPG取得するチャンネル一覧(NULL可)
	BOOL StartEpgCap(
		const vector<SET_CH_INFO>* chList
		);

	//EPG取得を停止する
	void StopEpgCap(
		);

	//EPG取得のステータスを取得する
	//※info==NULLの場合に限りスレッドセーフ
	//戻り値：
	// ステータス
	//引数：
	// info			[OUT]取得中のサービス
	JOB_STATUS GetEpgCapStatus(
		SET_CH_INFO* info
		);

	//バックグラウンドでのEPG取得設定
	//引数：
	// enableLive	[IN]視聴中に取得する
	// enableRec	[IN]録画中に取得する
	// enableRec	[IN]EPG取得するチャンネル一覧
	// *Basic		[IN]１チャンネルから基本情報のみ取得するかどうか
	// backStartWaitSec	[IN]Ch切り替え、録画開始後、バックグラウンドでのEPG取得を開始するまでの秒数
	void SetBackGroundEpgCap(
		BOOL enableLive,
		BOOL enableRec,
		BOOL BSBasic,
		BOOL CS1Basic,
		BOOL CS2Basic,
		BOOL CS3Basic,
		DWORD backStartWaitSec
		);

	//現在のストリームの表示用のステータスを取得する
	//※スレッドセーフ
	//引数：
	// signalLv		[OUT]シグナルレベル
	// space		[OUT]物理CHのspace(不明のとき負)
	// ch			[OUT]物理CHのch(不明のとき負)
	void GetViewStatusInfo(
		float* signalLv,
		int* space,
		int* ch
		);

protected:
	CBonDriverUtil bonUtil;
	CPacketInit packetInit;
	CTSOut tsOut;
	CChSetUtil chUtil;

	recursive_mutex_ buffLock;
	std::list<vector<BYTE>> tsBuffList;
	std::list<vector<BYTE>> tsFreeList;
	float statusSignalLv;
	int viewSpace;
	int viewCh;

	thread_ analyzeThread;
	CAutoResetEvent analyzeEvent;
	atomic_bool_ analyzeStopFlag;

	//チャンネルスキャン用
	thread_ chScanThread;
	CAutoResetEvent chScanStopEvent;
	struct CHK_CH_INFO {
		DWORD space;
		DWORD ch;
		wstring spaceName;
		wstring chName;
	};
	//スキャン中はconst操作のみ
	vector<CHK_CH_INFO> chScanChkList;
	atomic_int_ chScanIndexOrStatus;

	//EPG取得用
	thread_ epgCapThread;
	CAutoResetEvent epgCapStopEvent;
	//取得中はconst操作のみ
	vector<SET_CH_INFO> epgCapChList;
	atomic_int_ epgCapIndexOrStatus;

	thread_ epgCapBackThread;
	CAutoResetEvent epgCapBackStopEvent;
	BOOL enableLiveEpgCap;
	BOOL enableRecEpgCap;

	BOOL epgCapBackBSBasic;
	BOOL epgCapBackCS1Basic;
	BOOL epgCapBackCS2Basic;
	BOOL epgCapBackCS3Basic;
	DWORD epgCapBackStartWaitSec;
protected:
	DWORD ProcessSetCh(
		DWORD space,
		DWORD ch,
		BOOL chScan,
		BOOL restartEpgCapBack
		);

	static void GetEpgDataFilePath(WORD ONID, WORD TSID, wstring& epgDataFilePath);

	void RecvCallback(BYTE* data, DWORD size, DWORD remain, DWORD tsBuffMaxCount);
	void StatusCallback(float signalLv, int space, int ch);
	static void AnalyzeThread(CBonCtrl* sys);

	static void ChScanThread(CBonCtrl* sys);
	static void EpgCapThread(CBonCtrl* sys);

	void StartBackgroundEpgCap();
	void StopBackgroundEpgCap();
	static void EpgCapBackThread(CBonCtrl* sys);
};

