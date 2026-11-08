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
		ST_STOP = -4,		//停止中
		ST_COMPLETE = -3,	//完了
		ST_CANCEL = -2,		//キャンセルされた
		ST_WORKING = -1,	//実行中
	};

	CBonCtrl(void);
	~CBonCtrl(void);

	void ReloadSetting(
		bool enableEmm,
		bool noLogScramble,
		bool parseEpgPostProcess,
		bool enableScramble,
		bool needCaption,
		bool needData,
		bool allService,
		DWORD logoTypeFlags
		);

	//ネットワーク送信と統計の対象サービスIDを取得する
	WORD GetNWCtrlServiceID() { return this->nwCtrlServiceID; }

	//ネットワーク送信と統計の対象サービスIDを設定する
	//※GetStreamID()で受動的なチャンネル変化を検出した時などに使う
	void SetNWCtrlServiceID(
		WORD serviceID
		);

	//EPG取得などの状態を更新する
	//※概ね1秒ごとに呼ぶ
	void Check();

	//BonDriverをロードしてチャンネル情報などを取得（ファイル名で指定）
	//戻り値：
	// true（成功）、false（失敗）
	//引数：
	// bonDriverFile	[IN]BonDriverのファイル名
	bool OpenBonDriver(
		LPCWSTR bonDriverFile,
		int traceLevel,
		int openWait,
		DWORD tsBuffMaxCount
		);

	//ロードしているBonDriverの開放
	void CloseBonDriver();

	//ロード中のBonDriverのファイル名を取得する（ロード成功しているかの判定）
	//※スレッドセーフ
	//戻り値：
	// true（成功）、false（Openに失敗している）
	//引数：
	// bonDriverFile		[OUT]BonDriverのファイル名(nullptr可)
	bool GetOpenBonDriver(
		wstring* bonDriverFile
		);

	//チャンネル変更
	//戻り値：
	// true（成功）、false（失敗）
	//引数：
	// chData		[IN]変更チャンネルのサービス情報
	bool SetCh(
		const CH_DATA4& chData
		);

	//チャンネル変更中かどうか
	//※スレッドセーフ
	//戻り値：
	// true（変更中）、false（完了）
	bool IsChChanging(bool* chChgErr);

	//現在のストリームのIDを取得する
	//戻り値：
	// true（成功）、false（失敗）
	//引数：
	// ONID		[OUT]originalNetworkID
	// TSID		[OUT]transportStreamID
	bool GetStreamID(
		WORD* ONID,
		WORD* TSID
		) {
		return this->tsOut.GetStreamID(ONID, TSID);
	}

	//サービス一覧を取得する
	//戻り値：
	// サービス一覧。mapのキーは読み込み順番号。次に非スレッドセーフメソッドを呼び出すまで有効
	const map<DWORD, CH_DATA4>& GetServiceList(
		) {
		return this->chUtil.GetServiceList();
	}

	//TSストリーム制御用コントロールを作成する
	//戻り値：
	// 制御識別ID
	//引数：
	// duplicateNWCtrl		[IN]ネットワーク送信と統計用のものと同じ初期値を適用するかどうか
	DWORD CreateServiceCtrl(
		bool duplicateNWCtrl
		);

	//TSストリーム制御用コントロールを削除する
	//戻り値：
	// true（成功）、false（失敗）
	//引数：
	// id			[IN]制御識別ID
	bool DeleteServiceCtrl(
		DWORD id
		) {
		return this->tsOut.DeleteServiceCtrl(id);
	}

	//制御対象のサービスを設定する
	//戻り値：
	// true（成功）、false（失敗）
	//引数：
	// id			[IN]制御識別ID
	// serviceID	[IN]対象サービスID、0xFFFFで全サービス対象
	bool SetServiceID(
		DWORD id,
		WORD serviceID
		) {
		return this->tsOut.SetServiceID(id, serviceID);
	}

	bool GetServiceID(
		DWORD id,
		WORD* serviceID
		) {
		return this->tsOut.GetServiceID(id, serviceID);
	}

	//UDPで送信を行う
	//引数：
	// sendList		[IN/OUT]送信先リスト。nullptrで停止。送信に使用したポート(失敗のものは0x10000)がportにセットされる。
	void SendUdp(
		vector<NW_SEND_INFO>* sendList
		) {
		this->tsOut.SendUdp(this->nwCtrlID, sendList);
	}

	//TCPで送信を行う
	//引数：
	// sendList		[IN/OUT]送信先リスト。nullptrで停止。送信に使用したポート(失敗のものは0x10000)がportにセットされる。
	void SendTcp(
		vector<NW_SEND_INFO>* sendList
		) {
		this->tsOut.SendTcp(this->nwCtrlID, sendList);
	}

	//ファイル保存を開始する
	//戻り値：
	// true（成功）、false（失敗）
	//引数：
	// recParam				[IN]保存パラメータ
	// saveFolderSub		[IN]HDDの空きがなくなった場合に一時的に使用するフォルダ
	// writeBuffMaxCount	[IN]出力バッファ上限
	bool StartSave(
		const SET_CTRL_REC_PARAM& recParam,
		const vector<wstring>& saveFolderSub,
		int writeBuffMaxCount
	);

	//ファイル保存を終了する
	//戻り値：
	// true（成功）、false（失敗）
	//引数：
	// id			[IN]制御識別ID
	// subRecFlag	[OUT]成功のとき、サブ録画が発生したかどうか
	bool EndSave(
		DWORD id,
		bool* subRecFlag = nullptr
		) {
		return this->tsOut.EndSave(id, subRecFlag);
	}

	//スクランブル解除処理の動作設定
	//戻り値：
	// true（成功）、false（失敗）
	//引数：
	// id			[IN]制御識別ID
	// enable		[IN] true（処理する）、false（処理しない）
	bool SetScramble(
		DWORD id,
		bool enable
		) {
		return this->tsOut.SetScramble(id, enable);
	}

	//字幕とデータ放送含めるかどうか
	//引数：
	// id					[IN]制御識別ID
	// enableCaption		[IN]字幕を true（含める）、false（含めない）
	// enableData			[IN]データ放送を true（含める）、false（含めない）
	void SetServiceMode(
		DWORD id,
		bool enableCaption,
		bool enableData
		) {
		this->tsOut.SetServiceMode(id, enableCaption, enableData);
	}

	//エラーカウントをクリアする
	//引数：
	// id					[IN]制御識別ID
	void ClearErrCount(
		DWORD id
		) {
		this->tsOut.ClearErrCount(id);
	}

	//ドロップとスクランブルのカウントを取得する
	//引数：
	// id					[IN]制御識別ID
	// drop					[OUT]ドロップ数
	// scramble				[OUT]スクランブル数
	void GetErrCount(
		DWORD id,
		ULONGLONG* drop,
		ULONGLONG* scramble
		) {
		this->tsOut.GetErrCount(id, drop, scramble);
	}

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
	// asUtf8				[IN]UTF-8で保存するか
	// dropSaveThresh		[IN]ドロップ数がこれ以上なら保存する
	// drop					[OUT]ドロップ数
	void SaveErrCount(
		DWORD id,
		const wstring& filePath,
		bool asUtf8,
		int dropSaveThresh,
		int scrambleSaveThresh,
		ULONGLONG& drop,
		ULONGLONG& scramble
		) {
		this->tsOut.SaveErrCount(id, filePath, asUtf8, dropSaveThresh, scrambleSaveThresh, drop, scramble);
	}

	//録画中のファイルの出力サイズを取得する
	//引数：
	// id					[IN]制御識別ID
	// writeSize			[OUT]出力サイズ
	void GetRecWriteSize(
		DWORD id,
		LONGLONG* writeSize
		) {
		this->tsOut.GetRecWriteSize(id, writeSize);
	}

	//指定サービスの現在or次のEPG情報を取得する
	//※スレッドセーフ
	//戻り値：
	// true（成功）、false（失敗）
	//引数：
	// originalNetworkID		[IN]取得対象のoriginalNetworkID
	// transportStreamID		[IN]取得対象のtransportStreamID
	// serviceID				[IN]取得対象のServiceID
	// nextFlag					[IN]true（次の番組）、false（現在の番組）
	// epgInfo					[OUT]EPG情報
	bool GetEpgInfo(
		WORD originalNetworkID,
		WORD transportStreamID,
		WORD serviceID,
		bool nextFlag,
		EPGDB_EVENT_INFO* epgInfo
		) {
		return this->tsOut.GetEpgInfo(originalNetworkID, transportStreamID, serviceID, nextFlag, epgInfo);
	}

	//指定イベントのEPG情報を取得する
	//※スレッドセーフ
	//戻り値：
	// true（成功）、false（失敗）
	//引数：
	// originalNetworkID		[IN]取得対象のoriginalNetworkID
	// transportStreamID		[IN]取得対象のtransportStreamID
	// serviceID				[IN]取得対象のServiceID
	// eventID					[IN]取得対象のEventID
	// pfOnlyFlag				[IN]p/fからのみ検索するかどうか
	// epgInfo					[OUT]EPG情報
	bool SearchEpgInfo(
		WORD originalNetworkID,
		WORD transportStreamID,
		WORD serviceID,
		WORD eventID,
		bool pfOnlyFlag,
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
	// true（録画中）、false（録画していない）
	bool IsRec() { return this->tsOut.IsRec(); }

	//チャンネルスキャンを開始する
	//戻り値：
	// true（成功）、false（失敗）
	bool StartChScan();

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
	// true（成功）、false（失敗）
	//引数：
	// chList		[IN]EPG取得するチャンネル一覧(nullptr可)
	bool StartEpgCap(
		const vector<SET_CH_INFO>* chList
		);

	//EPG取得を停止する
	void StopEpgCap(
		);

	//EPG取得のステータスを取得する
	//※info==nullptrの場合に限りスレッドセーフ
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
	// *Basic		[IN]１チャンネルから基本情報のみ取得するかどうか
	// backStartWaitSec	[IN]Ch切り替え、録画開始後、バックグラウンドでのEPG取得を開始するまでの秒数
	void SetBackGroundEpgCap(
		bool enableLive,
		bool enableRec,
		bool BSBasic,
		bool CS1Basic,
		bool CS2Basic,
		bool CS3Basic,
		DWORD backStartWaitSec
		);

	//現在のストリームの表示用のステータスを取得する
	//引数：
	// signalLv		[OUT]シグナルレベル
	// space		[OUT]物理CHのspace(不明のとき負)
	// ch			[OUT]物理CHのch(不明のとき負)
	// drop			[OUT]ドロップ数
	// scramble		[OUT]スクランブル数
	void GetViewStatusInfo(
		float* signalLv,
		int* space,
		int* ch,
		ULONGLONG* drop,
		ULONGLONG* scramble
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

	DWORD nwCtrlID;
	bool nwCtrlEnableScramble;
	bool nwCtrlNeedCaption;
	bool nwCtrlNeedData;
	bool nwCtrlAllService;
	WORD nwCtrlServiceID;

	thread_ analyzeThread;
	CAutoResetEvent analyzeEvent;
	atomic_bool_ analyzeStopFlag;

	//チャンネルスキャン用
	struct CHK_CH_INFO {
		DWORD space;
		DWORD ch;
		wstring spaceName;
		wstring chName;
	};
	vector<CHK_CH_INFO> chScanChkList;
	int chScanIndexOrStatus;
	DWORD chScanChChgTimeOut;
	DWORD chScanServiceChkTimeOut;
	bool chScanChkNext;
	DWORD chScanTick;

	//EPG取得用
	//取得中はconst操作のみ
	vector<SET_CH_INFO> epgCapChList;
	atomic_int_ epgCapIndexOrStatus;
	bool epgCapBSBasic;
	bool epgCapCS1Basic;
	bool epgCapCS2Basic;
	bool epgCapCS3Basic;
	bool epgCapChkBS;
	bool epgCapChkCS1;
	bool epgCapChkCS2;
	bool epgCapChkCS3;
	bool epgCapChkNext;
	int epgCapSetChState;
	DWORD epgCapTimeOut;
	bool epgCapSaveTimeOut;
	DWORD epgCapTick;
	DWORD epgCapLastChkTick;

	int epgCapBackIndexOrStatus;
	bool enableLiveEpgCap;
	bool enableRecEpgCap;

	bool epgCapBackBSBasic;
	bool epgCapBackCS1Basic;
	bool epgCapBackCS2Basic;
	bool epgCapBackCS3Basic;
	DWORD epgCapBackStartWaitSec;

	DWORD tsOutLogoTypeFlags;
protected:
	bool ProcessSetCh(
		DWORD space,
		DWORD ch,
		int onidOrChScan
		);

	static void GetEpgDataFilePath(WORD ONID, WORD TSID, wstring& epgDataFilePath);

	void RecvCallback(BYTE* data, DWORD size, DWORD remain, DWORD tsBuffMaxCount);
	void StatusCallback(float signalLv, int space, int ch);
	static void AnalyzeThread(CBonCtrl* sys);

	void CheckChScan();
	void CheckEpgCap();

	void StartBackgroundEpgCap();
	void StopBackgroundEpgCap();
	void CheckEpgCapBack();
	void CheckLogo();
};

