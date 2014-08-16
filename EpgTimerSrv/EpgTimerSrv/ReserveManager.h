#pragma once

#include "../../Common/Util.h"
#include "../../Common/EpgTimerUtil.h"
#include "../../Common/PathUtil.h"
#include "../../Common/StringUtil.h"
#include "../../Common/ParseTextInstances.h"

#include "TwitterManager.h"

#include "NotifyManager.h"
#include "ReserveInfo.h"
#include "TunerManager.h"
#include "BatManager.h"

class CReserveManager
{
public:
	CReserveManager(void);
	~CReserveManager(void);
/*
	void SetRegistGUI(map<DWORD, DWORD> registGUIMap);
	void SetRegistTCP(map<wstring, REGIST_TCP_INFO> registTCPMap);
*/
	void SetNotifyManager(CNotifyManager* manager);
	void SetEpgDBManager(CEpgDBManager* epgDBManager);
	void ChangeRegist();
	void ReloadSetting();

	//予約情報の読み込みを行う
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	BOOL ReloadReserveData();

	//予約情報を取得する
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// reserveList		[OUT]予約情報一覧（呼び出し元で解放する必要あり）
	BOOL GetReserveDataAll(
		vector<RESERVE_DATA*>* reserveList
		);

	//チューナー毎の予約情報を取得する
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// reserveList		[OUT]予約情報一覧
	BOOL GetTunerReserveAll(
		vector<TUNER_RESERVE_INFO>* list
		);

	//予約情報を取得する
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// id				[IN]予約ID
	// reserveData		[OUT]予約情報
	BOOL GetReserveData(
		DWORD id,
		RESERVE_DATA* reserveData
		);

	//予約情報を追加する
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// reserveList		[IN]予約情報
	BOOL AddReserveData(
		vector<RESERVE_DATA>* reserveList,
		BOOL tweet = FALSE
		);

	//予約情報を変更する
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// reserveList		[IN]予約情報
	BOOL ChgReserveData(
		vector<RESERVE_DATA>* reserveList,
		BOOL timeChg = FALSE
		);

	//予約情報を削除する
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// reserveList		[IN]予約IDリスト
	BOOL DelReserveData(
		vector<DWORD>* reserveList
		);

	//予約の振り分けを行う
	void ReloadBankMap(BOOL notify);
	
	//録画済み情報一覧を取得する
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// infoList			[OUT]録画済み情報一覧
	BOOL GetRecFileInfoAll(
		vector<REC_FILE_INFO>* infoList
		);

	//録画済み情報を削除する
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// idList			[IN]IDリスト
	BOOL DelRecFileInfo(
		vector<DWORD>* idList
		);

	//録画済み情報のプロテクトを変更する
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// idList			[IN]IDリスト
	BOOL ChgProtectRecFileInfo(
		vector<REC_FILE_INFO>* infoList
		);


	BOOL IsEnableSuspend(
		BYTE* suspendMode,
		BYTE* rebootFlag
		);

	BOOL IsEnableReloadEPG(
		);

	BOOL IsSuspendOK();

	BOOL GetSleepReturnTime(
		LONGLONG* returnTime
		);

	BOOL StartEpgCap();
	void StopEpgCap();
	BOOL IsEpgCap();

	BOOL IsFindReserve(
		WORD ONID,
		WORD TSID,
		WORD SID,
		WORD eventID
		);

	BOOL IsFindReserve(
		WORD ONID,
		WORD TSID,
		WORD SID,
		LONGLONG startTime,
		DWORD durationSec
		);

	void SendNotifyUpdate(DWORD notifyId);
	void SendNotifyChgReserveAutoAdd(RESERVE_DATA* oldInfo, RESERVE_DATA* newInfo);

	BOOL GetTVTestChgCh(
		LONGLONG chID,
		TVTEST_CH_CHG_INFO* chInfo
		);

	BOOL SetNWTVCh(
		SET_CH_INFO* chInfo
		);

	BOOL CloseNWTV(
		);

	void SetNWTVMode(
		DWORD mode
		);

	void SendTweet(
		SEND_TWEET_MODE mode,
		void* param1,
		void* param2,
		void* param3
		);

	BOOL GetRecFilePath(
		DWORD reserveID,
		wstring& filePath,
		DWORD* ctrlID,
		DWORD* processID
		);

	//予約追加可能かチェックする
	BOOL ChkAddReserve(RESERVE_DATA* chkData, WORD* status);

	//6日以内の録画結果に同じ番組名あるかチェックする
	BOOL IsFindRecEventInfo(EPGDB_EVENT_INFO* info, WORD chkDay);
	void ChgAutoAddNoRec(EPGDB_EVENT_INFO* info);

	BOOL IsRecInfoChg();
protected:
	HANDLE lockEvent;

	HANDLE bankCheckThread;
	HANDLE bankCheckStopEvent;

	WORD notifyStatus;

	/*
	map<DWORD, DWORD> registGUIMap;
	map<wstring, REGIST_TCP_INFO> registTCPMap;
	HANDLE lockNotify;
	HANDLE notifyThread;
	HANDLE notifyStopEvent;
	HANDLE notifyStatusThread;
	HANDLE notifyStatusStopEvent;

	HANDLE notifyEpgReloadThread;
	HANDLE notifyEpgReloadStopEvent;
	*/
	CNotifyManager* notifyManager;

	CParseReserveText reserveText;
	map<DWORD, CReserveInfo*> reserveInfoMap; //キー　reserveID
	map<LONGLONG, DWORD> reserveInfoIDMap; //キー　ONID<<48|TSID<<32|SID<<16|EventID
	CParseRecInfoText recInfoText;
	CParseRecInfo2Text recInfo2Text;
	wstring recInfo2RegExp;
	int recInfo2DropChk;

	CParseChText5 chUtil;

	CTunerManager tunerManager;
	CBatManager batManager;
	CTwitterManager* twitterManager;

	CEpgDBManager* epgDBManager;

	typedef struct _BANK_WORK_INFO{
		CReserveInfo* reserveInfo;
		LONGLONG startTime;//マージン考慮した開始時間
		LONGLONG endTime;//マージン考慮した終了時間
		BYTE priority;
		BOOL recWaitFlag;
		wstring sortKey;
		DWORD reserveID;
		DWORD chID;		//originalNetworkID<<16 | transportStreamID
		DWORD preTunerID;
		DWORD useTunerID;
		WORD ONID;
		WORD TSID;
		WORD SID;
	}BANK_WORK_INFO;
	typedef struct _BANK_INFO{
		DWORD tunerID;
		map<DWORD, BANK_WORK_INFO*> reserveList; //キー 予約ID
	}BANK_INFO;
	map<DWORD, BANK_INFO*> bankMap; //キー チューナーID
	map<DWORD, BANK_WORK_INFO*> NGReserveMap;

	int defStartMargine;
	int defEndMargine;

	BOOL backPriorityFlag;
	BOOL sameChPriorityFlag;

	map<DWORD, CTunerBankCtrl*> tunerBankMap; //キー bonID<<16 | tunerID

	BYTE enableSetSuspendMode;
	BYTE enableSetRebootFlag;
	BYTE enableEpgReload;

	BOOL epgCapCheckFlag;


	BOOL BSOnly;
	BOOL CS1Only;
	BOOL CS2Only;
	LONGLONG ngCapMin;
	LONGLONG ngCapTunerMin;
	typedef struct _EPGTIME_INFO{
		DWORD time;
		int wday;
		int basicOnlyFlags;
	}EPGTIME_INFO;
	vector<EPGTIME_INFO> epgCapTimeList;
	int wakeTime;
	BYTE defSuspendMode;
	BYTE defRebootFlag;
	int batMargin;
	vector<wstring> noStandbyExeList;
	DWORD noStandbyTime;
	BOOL ngShareFile;
	BOOL autoDel;
	vector<wstring> delExtList;
	vector<wstring> delFolderList;
	BOOL eventRelay;
	BOOL useTweet;
	BOOL useProxy;
	wstring proxySrv;
	wstring proxyID;
	wstring proxyPWD;
	BOOL recEndTweetErr;
	DWORD recEndTweetDrop;

	vector<wstring> tvtestUseBon;

	int duraChgMarginMin;
	int notFindTuijyuHour;
	int noEpgTuijyuMin;

	BOOL autoDelRecInfo;
	int autoDelRecInfoNum;
	BOOL timeSync;
	BOOL setTimeSync;

	DWORD NWTVPID;
	wstring recExePath;
	CSendCtrlCmd sendCtrlNWTV;
	BOOL NWTVUDP;
	BOOL NWTVTCP;

	BOOL ngAddResSrvCoop;

	BOOL errEndBatRun;

	int reloadBankMapAlgo;
	BOOL useRecNamePlugIn;
	wstring recNamePlugInFilePath;

	BOOL chgRecInfo;
protected:
	//PublicAPI排他制御用
	BOOL Lock(LPCWSTR log = NULL, DWORD timeOut = 60*1000);
	void UnLock(LPCWSTR log = NULL);
/*
	BOOL NotifyLock(LPCWSTR log = NULL, DWORD timeOut = 60*1000);
	void NotifyUnLock(LPCWSTR log = NULL);
	*/

	BOOL _AddReserveData(RESERVE_DATA* reserve, BOOL tweet = FALSE);
	BOOL _ChgReserveData(RESERVE_DATA* reserve, BOOL chgTime);

	void _ReloadBankMap();
	void _ReloadBankMapAlgo(BOOL do2Pass, BOOL ignoreUseTunerID, BOOL backPriority, BOOL noTuner);
	void CalcEntireReserveTime(LONGLONG* startTime, LONGLONG* endTime, const RESERVE_DATA& data);
	void CheckOverTimeReserve();
	void CreateWorkData(CReserveInfo* reserveInfo, BANK_WORK_INFO* workInfo, BOOL backPriority, DWORD reserveCount, DWORD reserveNum, BOOL noTuner = FALSE);
	DWORD ChkInsertStatus(BANK_INFO* bank, BANK_WORK_INFO* inItem, BOOL reCheck = FALSE, BOOL mustOverlap = FALSE);
	DWORD ReChkInsertStatus(BANK_INFO* bank, BANK_WORK_INFO* inItem);
	DWORD ChkInsertNGStatus(BANK_INFO* bank, BANK_WORK_INFO* inItem);
	BOOL ChangeNGReserve(BANK_WORK_INFO* inItem);
	DWORD ChkInsertSameChStatus(BANK_INFO* bank, BANK_WORK_INFO* inItem);

	void _SendNotifyUpdate(DWORD notifyId);
	void _SendNotifyStatus(WORD status);
	void _SendNotifyRecEnd(REC_FILE_INFO* item);
	void _SendNotifyChgReserve(DWORD notifyId, RESERVE_DATA* oldInfo, RESERVE_DATA* newInfo);
/*	static UINT WINAPI SendNotifyThread(LPVOID param);
	static UINT WINAPI SendNotifyStatusThread(LPVOID param);
	void _SendNotifyEpgReload();
	static UINT WINAPI SendNotifyEpgReloadThread(LPVOID param);
	*/
	BOOL _DelReserveData(
		vector<DWORD>* reserveList
	);

	static UINT WINAPI BankCheckThread(LPVOID param);
	void CheckEndReserve();
	void CheckErrReserve();
	void CheckBatWork();
	void CheckTuijyu();
	BOOL CheckEventRelay(EPGDB_EVENT_INFO* info, RESERVE_DATA* data, BOOL errEnd = FALSE);

	BOOL CheckChgEvent(EPGDB_EVENT_INFO* info, RESERVE_DATA* data, BYTE* chgMode = NULL);
	BOOL CheckChgEventID(EPGDB_EVENT_INFO* info, RESERVE_DATA* data);
	BOOL CheckNotFindChgEvent(RESERVE_DATA* data, CTunerBankCtrl* ctrl, vector<DWORD>* deleteList);
	BOOL ChgDurationChk(EPGDB_EVENT_INFO* info);

	void EnableSuspendWork(BYTE suspendMode, BYTE rebootFlag, BYTE epgReload);
	BOOL _IsSuspendOK(BOOL rebootFlag);
	BOOL IsFindNoSuspendExe();
	BOOL IsFindShareTSFile();

	BOOL GetNextEpgcapTime(LONGLONG* capTime, LONGLONG chkMargineMin, int* basicOnlyFlags = NULL);

	BOOL _StartEpgCap();
	BOOL _IsEpgCap();

	void _SendTweet(
		SEND_TWEET_MODE mode,
		void* param1,
		void* param2,
		void* param3
		);

	//TSファイルを削除して必要な空き領域を作る
	static void CreateDiskFreeSpace(
		const vector<RESERVE_DATA>& chkReserve,
		const wstring& defRecFolder,
		const map<wstring, wstring>& protectFile,
		const vector<wstring>& delFolderList,
		const vector<wstring>& delExtList
		);
};

