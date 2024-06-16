#pragma once

#include "NotifyManager.h"
#include "EpgDBManager.h"
#include "EpgTimerSrvSetting.h"
#include "../../Common/ReNamePlugInUtil.h"
#include "../../Common/ThreadUtil.h"

//1つのチューナ(EpgDataCap_Bon.exe)を管理する
//必ずオブジェクト生成→ReloadSetting()→…→破棄の順番で利用しなければならない
//スレッドセーフではない
class CTunerBankCtrl
{
public:
	//録画開始前に録画制御を作成するタイミング(秒)
	static const int READY_MARGIN = 20;

	enum TR_STATE {
		TR_IDLE,
		TR_OPEN,	//チューナ起動済み(GetState()のみ使用)
		TR_READY,	//録画制御作成済み
		TR_REC,		//録画中
		TR_EPGCAP,	//EPG取得中(GetState()とspecialStateで使用)
		TR_NWTV,	//ネットワークモードで起動中(GetState()とspecialStateで使用)
	};
	enum {
		CHECK_END = 1,				//正常終了
		CHECK_END_CANCEL,			//キャンセルにより録画が中断した
		CHECK_END_NOT_FIND_PF,		//p/fに番組情報確認できなかった
		CHECK_END_NEXT_START_END,	//次の予約開始のため終了
		CHECK_END_END_SUBREC,		//サブフォルダへの録画が発生した
		CHECK_END_NOT_START_HEAD,	//一部のみ録画された
		CHECK_ERR_RECEND,			//録画終了処理に失敗した
		CHECK_ERR_REC,				//予期せず録画が中断した
		CHECK_ERR_RECSTART,			//録画開始に失敗した
		CHECK_ERR_CTRL,				//録画制御の作成に失敗した
		CHECK_ERR_OPEN,				//チューナのオープンができなかった
		CHECK_ERR_PASS,				//終了時間が過ぎていた
	};
	struct CHECK_RESULT {
		DWORD type;
		DWORD reserveID;
		//以下はtype<=CHECK_END_NOT_START_HEADのとき有効
		wstring recFilePath;
		bool continueRec;
		//continueRec(連続録画開始による終了)のときdropsとscramblesは常に0
		LONGLONG drops;
		LONGLONG scrambles;
		//以下はtype==CHECK_ENDのとき有効
		SYSTEMTIME epgStartTime;
		wstring epgEventName;
	};
	struct TUNER_RESERVE {
		DWORD reserveID;
		wstring title;
		LONGLONG startTime;
		DWORD durationSecond;
		wstring stationName;
		WORD onid;
		WORD tsid;
		WORD sid;
		WORD eid;
		BYTE recMode; //RECMODE_ALL～RECMODE_VIEW
		BYTE priority;
		bool enableCaption;
		bool enableData;
		bool pittari;
		BYTE partialRecMode;
		bool continueRecFlag;
		//マージンはデフォルト値適用済みとすること
		LONGLONG startMargin;
		LONGLONG endMargin;
		vector<REC_FILE_SET_INFO> recFolder;
		vector<REC_FILE_SET_INFO> partialRecFolder;
	};

	CTunerBankCtrl(DWORD tunerID_, LPCWSTR bonFileName_, WORD epgCapMax, const map<DWORD, CH_DATA4>& chMap_, CNotifyManager& notifyManager_, CEpgDBManager& epgDBManager_);
	~CTunerBankCtrl();
	void ReloadSetting(const CEpgTimerSrvSetting::SETTING& s);

	//ドライバファイル名を取得(不変値)
	const wstring& GetBonFileName() const { return this->bonFileName; }
	//このチューナと同じドライバのチューナをEPG取得に使える最大数を取得(不変値)
	WORD GetEpgCapMaxOfThisBon() const { return this->epgCapMaxOfThisBon; }
	//サービス情報を取得(不変値)
	const CH_DATA4* GetCh(WORD onid, WORD tsid, WORD sid) const;
	//予約を追加する
	bool AddReserve(const TUNER_RESERVE& reserve);
	//待機状態に入っている予約を変更する
	//変更できないフィールドは適宜修正される
	//開始時間の後方移動は注意が必要。後方移動の結果待機状態を明らかに抜けてしまう場合はstartTimeとstartMarginが修正される
	bool ChgCtrlReserve(TUNER_RESERVE* reserve);
	//予約を削除する
	//retList: 録画中であれば終了結果を追加
	bool DelReserve(DWORD reserveID, vector<CHECK_RESULT>* retList = NULL);
	//開始時間がstartTime以上の待機状態に入っていないすべての予約をクリアする
	void ClearNoCtrl(LONGLONG startTime = 0);
	//予約ID一覧を取得する(ソート済み)
	vector<DWORD> GetReserveIDList() const;
	//チューナの状態遷移をおこない、終了した予約を取得する
	//概ね1秒ごとに呼ぶ
	//startedReserveIDList: TR_RECに遷移した予約ID一覧
	vector<CHECK_RESULT> Check(vector<DWORD>* startedReserveIDList = NULL);
	//チューナ全体としての状態を取得する
	TR_STATE GetState() const;
	//プロセスIDを取得する(GetState()がTR_IDLEのとき不定)
	int GetProcessID() const { return this->tunerPid; }
	//ネットワークモードIDを取得する(GetState()がTR_NWTVでないとき不定)
	int GetNWTVID() const { return this->nwtvID; }
	//起動中のチューナから得たステータス情報(GetState()がTR_IDLEのとき不定)
	TUNER_PROCESS_STATUS_INFO GetProcessStatusInfo() const;
	//予約開始の最小時刻を取得する
	LONGLONG GetNearestReserveTime() const;
	//EPG取得を開始する
	bool StartEpgCap(const vector<SET_CH_INFO>& setChList);
	//起動中のチューナのチャンネルを取得する
	bool GetCurrentChID(WORD* onid, WORD* tsid) const;
	//起動中のチューナからEPGデータの検索
	bool SearchEpgInfo(WORD sid, WORD eid, EPGDB_EVENT_INFO* resVal) const;
	//起動中のチューナから現在or次の番組情報を取得する
	//戻り値: 0=成功,1=失敗(番組情報はない),2=失敗(取得できない)
	int GetEventPF(WORD sid, bool pfNextFlag, EPGDB_EVENT_INFO* resVal) const;
	//放送波時刻に対するシステム時刻の遅延時間を取得する
	LONGLONG DelayTime() const;
	//ネットワークモードでチューナを起動しチャンネル設定する
	bool OpenNWTV(int id, bool nwUdp, bool nwTcp, const SET_CH_INFO& chInfo);
	//ネットワークモードのチューナを閉じる
	void CloseNWTV();
	//予約が録画中であればその録画ファイル名を取得する
	bool GetRecFilePath(DWORD reserveID, wstring& filePath) const;
	//予約情報をもとにファイル名を生成する
	static wstring ConvertRecName(
		LPCWSTR recNamePlugIn, const SYSTEMTIME& startTime, DWORD durationSec, LPCWSTR eventName, WORD onid, WORD tsid, WORD sid, WORD eid,
		LPCWSTR serviceName, LPCWSTR bonDriverName, DWORD tunerID, DWORD reserveID, CEpgDBManager& epgDBManager_,
		const SYSTEMTIME& startTimeForDefault, DWORD ctrlID, LPCWSTR ext, bool noChkYen, CReNamePlugInUtil& util);
	//バンクを監視して必要ならチューナを強制終了する
	//概ね2秒ごとにワーカスレッドから呼ぶ
	void Watch();
private:
	struct TUNER_RESERVE_WORK : TUNER_RESERVE {
		LONGLONG startOrder; //開始順(予約の前後関係を決める)
		LONGLONG effectivePriority; //実効優先度(予約の優先度を決める。小さいほうが高優先度)
		TR_STATE state;
		int retryOpenCount;
		//以下はstate!=TR_IDLEのとき有効
		DWORD ctrlID[2]; //要素1は部分受信録画制御
		//以下はstate==TR_RECのとき有効
		wstring recFilePath[2];
		bool notStartHead;
		bool appendPgInfo;
		bool savedPgInfo;
		SYSTEMTIME epgStartTime;
		wstring epgEventName;
	};
	//チューナを閉じてはいけない状態かどうか
	bool IsNeedOpenTuner() const;
	//部分受信サービスを探す
	bool FindPartialService(WORD onid, WORD tsid, WORD sid, WORD* partialSID, wstring* serviceName) const;
	//チューナに録画制御を作成する
	bool CreateCtrl(DWORD* ctrlID, DWORD* partialCtrlID, const TUNER_RESERVE& reserve) const;
	//録画ファイルに対応する番組情報ファイルを保存する
	void SaveProgramInfo(LPCWSTR recPath, const EPGDB_EVENT_INFO& info, bool append) const;
	//チューナに録画を開始させる
	bool RecStart(const TUNER_RESERVE_WORK& reserve, LONGLONG now) const;
	//チューナを起動する
	bool OpenTuner(bool minWake, bool noView, bool nwUdp, bool nwTcp, bool standbyRec, const SET_CH_INFO* initCh);
	//チューナを閉じる
	void CloseTuner();
	//このバンクのBonDriverを使用しているプロセスを1つだけ閉じる
	bool CloseOtherTuner();

	const DWORD tunerID;
	const wstring bonFileName;
	const WORD epgCapMaxOfThisBon;
	map<LONGLONG, CH_DATA4> chMap;
	CNotifyManager& notifyManager;
	CEpgDBManager& epgDBManager;
	map<DWORD, TUNER_RESERVE_WORK> reserveMap;
	DWORD tunerPid;
#ifdef _WIN32
	//tunerPidが非0で有効、0で無効
	HANDLE hTunerProcess;
#endif
	VIEW_APP_STATUS_INFO statusInfo;
	bool getStatusDetailsNonSupport;
	WORD tunerONID;
	WORD tunerTSID;
	bool tunerChLocked;
	bool tunerResetLock;
	DWORD tunerChChgTick;
	//EPG取得中かネットワークモードか否か
	TR_STATE specialState;
	//放送波時刻に対するシステム時刻の遅延時間
	LONGLONG delayTime;
	LONGLONG epgCapDelayTime;
	//ネットワークモードID
	int nwtvID;

	LONGLONG recWakeTime;
	bool recMinWake;
	bool openViewForViewing;
	bool openViewForRec;
	bool openViewAlways;
	bool recNW;
	bool backPriority;
	bool saveProgramInfo;
	bool saveProgramInfoAsUtf8;
	bool saveErrLog;
	bool recOverWrite;
	int processPriority;
	bool keepDisk;
	bool recNameNoChkYen;
	wstring recNamePlugInFileName;
	wstring tsExt;

	mutable struct WATCH_CONTEXT {
		recursive_mutex_ lock;
		DWORD count;
		DWORD tick;
	} watchContext;

	class CWatchBlock {
	public:
		CWatchBlock(WATCH_CONTEXT* context_);
		~CWatchBlock();
	private:
		WATCH_CONTEXT* context;
	};
};
