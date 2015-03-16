#pragma once

#include "../../Common/ParseTextInstances.h"
#include "EpgDBManager.h"
#include "NotifyManager.h"
#include "TunerManager.h"
#include "BatManager.h"

//予約を管理しチューナに割り当てる
//必ずオブジェクト生成→Initialize()→…→Finalize()→破棄の順番で利用しなければならない
class CReserveManager
{
public:
	enum {
		WAIT_EXTRA_EPGCAP_END = 1,		//EPG取得が完了した
		WAIT_EXTRA_NEED_SHUTDOWN,		//システムシャットダウンを試みる必要がある
		WAIT_EXTRA_RESERVE_MODIFIED,	//予約になんらかの変化があった
	};
	CReserveManager(CNotifyManager& notifyManager_, CEpgDBManager& epgDBManager_);
	~CReserveManager();
	void Initialize();
	void Finalize();
	void ReloadSetting();
	//予約情報一覧を取得する
	vector<RESERVE_DATA> GetReserveDataAll(bool getRecFileName = false) const;
	//チューナ毎の予約情報を取得する
	vector<TUNER_RESERVE_INFO> GetTunerReserveAll() const;
	//予約情報を取得する
	bool GetReserveData(DWORD id, RESERVE_DATA* reserveData, bool getRecFileName = false) const;
	//予約情報を追加する
	bool AddReserveData(const vector<RESERVE_DATA>& reserveList, bool setComment = false);
	//予約情報を変更する
	bool ChgReserveData(const vector<RESERVE_DATA>& reserveList, bool setReserveStatus = false);
	//予約情報を削除する
	void DelReserveData(const vector<DWORD>& idList);
	//録画済み情報一覧を取得する
	vector<REC_FILE_INFO> GetRecFileInfoAll() const;
	//録画済み情報を削除する
	void DelRecFileInfo(const vector<DWORD>& idList);
	//録画済み情報のプロテクトを変更する
	//infoList: 録画済み情報一覧(idとprotectFlagのみ参照)
	void ChgProtectRecFileInfo(const vector<REC_FILE_INFO>& infoList);
	//EIT[schedule]をもとに追従処理する
	void CheckTuijyu();
	//予約管理のために待機する
	//スレッドセーフではない
	//extra: 追加の戻り値(HIWORDにWAIT_EXTRA_*)
	//戻り値: WaitForSingleObject(hEvent)と同じ。ただしextraを渡すために待機を解除するときWAIT_OBJECT_0+1
	DWORD Wait(HANDLE hEvent, DWORD timeout, DWORD* extra);
	//EPG取得開始を要求する
	bool RequestStartEpgCap();
	//チューナ起動やEPG取得やバッチ処理が行われているか
	bool IsActive() const;
	//baseTime以後に録画またはEPG取得を開始する最小時刻を取得する
	__int64 GetSleepReturnTime(__int64 baseTime) const;
	//指定イベントの予約が存在するかどうか
	bool IsFindReserve(WORD onid, WORD tsid, WORD sid, WORD eid) const;
	//指定時刻のプログラム予約があるかどうか
	bool IsFindProgramReserve(WORD onid, WORD tsid, WORD sid, __int64 startTime, DWORD durationSec) const;
	//指定サービスを利用できるチューナID一覧を取得する
	vector<DWORD> GetSupportServiceTuner(WORD onid, WORD tsid, WORD sid) const;
	bool GetTunerCh(DWORD tunerID, WORD onid, WORD tsid, WORD sid, DWORD* space, DWORD* ch) const;
	wstring GetTunerBonFileName(DWORD tunerID) const;
	bool IsOpenTuner(DWORD tunerID) const;
	//ネットワークモードでチューナを起動しチャンネル設定する
	//tunerIDList: 起動させるときはこのリストにあるチューナを候補にする
	bool SetNWTVCh(bool nwUdp, bool nwTcp, const SET_CH_INFO& chInfo, const vector<DWORD>& tunerIDList);
	//ネットワークモードのチューナを閉じる
	bool CloseNWTV();
	//予約が録画中であればその録画ファイル名などを取得する
	bool GetRecFilePath(DWORD reserveID, wstring& filePath, DWORD* ctrlID, DWORD* processID) const;
	//指定EPGイベントは録画済みかどうか
	bool IsFindRecEventInfo(const EPGDB_EVENT_INFO& info, WORD chkDay) const;
	//自動予約によって作成された指定イベントの予約を無効にする
	bool ChgAutoAddNoRec(WORD onid, WORD tsid, WORD sid, WORD eid);
	//チャンネル情報を取得する
	vector<CH_DATA5> GetChDataList() const;
private:
	struct CHK_RESERVE_DATA {
		__int64 cutStartTime;
		__int64 cutEndTime;
		__int64 startOrder;
		__int64 effectivePriority;
		bool started;
		const RESERVE_DATA* r;
	};
	//チューナに割り当てられていない予約一覧を取得する
	vector<DWORD> GetNoTunerReserveAll() const;
	//予約をチューナに割り当てる
	//reloadTime: なんらかの変更があった最小予約位置
	void ReloadBankMap(__int64 reloadTime = 0);
	//ある予約をバンクに追加したときに発生するコスト(単位:10秒)を計算する
	//戻り値: 重なりが無ければ0、別チャンネルの重なりがあれば重なりの秒数だけ加算、同一チャンネルのみの重なりがあれば-1
	__int64 ChkInsertStatus(vector<CHK_RESERVE_DATA>& bank, CHK_RESERVE_DATA& inItem, bool modifyBank) const;
	//マージンを考慮した予約時刻を計算する(常にendTime>=startTime)
	void CalcEntireReserveTime(__int64* startTime, __int64* endTime, const RESERVE_DATA& data) const;
	//最新EPG(チューナからの情報)をもとに追従処理する
	void CheckTuijyuTuner();
	//チューナ割り当てされていない古い予約を終了処理する
	void CheckOverTimeReserve();
	//EPG取得可能なチューナIDのリストを取得する
	vector<DWORD> GetEpgCapTunerIDList(__int64 now) const;
	//EPG取得処理を管理する
	//isEpgCap: EPG取得中のチューナが無ければfalse
	//戻り値: EPG取得が完了した瞬間にtrue
	bool CheckEpgCap(bool isEpgCap);
	//次のEPG取得時刻を取得する
	__int64 GetNextEpgCapTime(__int64 now, int* basicOnlyFlags = NULL) const;
	//必要ならreserveTextCacheListを再構築する
	void ReCacheReserveText() const;
	//バンクを監視して必要ならチューナを強制終了するスレッド
	static UINT WINAPI WatchdogThread(LPVOID param);

	mutable CRITICAL_SECTION managerLock;

	CNotifyManager& notifyManager;
	CEpgDBManager& epgDBManager;

	CParseReserveText reserveText;
	CParseRecInfoText recInfoText;
	CParseRecInfo2Text recInfo2Text;
	CParseChText5 chUtil;

	CTunerManager tunerManager;
	CBatManager batManager;

	map<DWORD, CTunerBankCtrl*> tunerBankMap;
	//reserveTextをONID<<48|TSID<<32|SID<<16|EID,予約IDでソートした検索用キャッシュ
	mutable vector<pair<ULONGLONG, DWORD>> reserveTextCache;

	DWORD ngCapTimeSec;
	DWORD ngCapTunerTimeSec;
	bool epgCapTimeSync;
	//LOWORDに取得時刻の日曜日からのオフセット(分)、HIWORDに取得種別
	vector<DWORD> epgCapTimeList;
	int defStartMargin;
	int defEndMargin;
	bool backPriority;
	int recInfo2DropChk;
	wstring recInfo2RegExp;
	bool defEnableCaption;
	bool defEnableData;
	wstring recNamePlugInFileName;
	bool recNameNoChkYen;

	DWORD waitTick;
	DWORD waitCount;
	__int64 lastCheckEpgCap;
	bool epgCapRequested;
	bool epgCapWork;
	bool epgCapSetTimeSync;
	int epgCapBasicOnlyFlags;
	bool reserveModified;

	HANDLE watchdogStopEvent;
	HANDLE watchdogThread;
};
