#pragma once

#include "../../Common/Util.h"
#include "../../Common/EpgTimerUtil.h"
#include "../../Common/PathUtil.h"
#include "../../Common/StringUtil.h"
#include "../../Common/ParseTextInstances.h"
#include "../../Common/SendCtrlCmd.h"

#include "ReserveInfo.h"
#include "EpgTimerSrvDef.h"
#include "TwitterManager.h"
#include "NotifyManager.h"
#include "EpgDBManager.h"

class CTunerBankCtrl
{
public:
	CTunerBankCtrl(void);
	~CTunerBankCtrl(void);

	void SetTwitterCtrl(CTwitterManager* twitterManager);
	void SetEpgDBManager(CEpgDBManager* epgDBManager);
	void ReloadSetting();
	void SetNotifyManager(CNotifyManager* manager);

	void SetAutoDel(
		BOOL autoDel,
		vector<wstring>* delExtList,
		vector<wstring>* delFolderList
		);

	void SetTunerInfo(
		WORD bonID,
		WORD tunerID,
		wstring bonFileName,
		wstring chSet4FilePath
		);

	void AddReserve(
		vector<CReserveInfo*>* reserveInfo
		);

	void ChgReserve(
		RESERVE_DATA* reserve
		);

	void DeleteReserve(
		DWORD reserveID
		);

	void ClearNoCtrl();

	void GetEndReserve(map<DWORD, END_RESERVE_INFO*>* reserveMap); //キー　reserveID

	BOOL IsOpenErr();
	void GetOpenErrReserve(vector<CReserveInfo*>* reserveInfo);
	void ResetOpenErr();

	BOOL IsRecWork();
	BOOL IsOpenTuner();
	BOOL GetCurrentChID(DWORD* currentChID);
	BOOL IsSuspendOK();
	BOOL IsEpgCapOK(LONGLONG ngCapMin);
	BOOL IsEpgCapWorking();
	void ClearEpgCapItem();
	void AddEpgCapItem(SET_CH_INFO info);
	void StartEpgCap();
	void StopEpgCap();

	//起動中のチューナーからEPGデータの検索
	//戻り値：
	// エラーコード
	// val					[IN]取得番組
	// resVal				[OUT]番組情報
	BOOL SearchEpgInfo(
		SEARCH_EPG_INFO_PARAM* val,
		EPGDB_EVENT_INFO* resVal
		);

	//起動中のチューナーから現在or次の番組情報を取得する
	//戻り値：
	// エラーコード
	// val					[IN]取得番組
	// resVal				[OUT]番組情報
	DWORD GetEventPF(
		GET_EPG_PF_INFO_PARAM* val,
		EPGDB_EVENT_INFO* resVal
		);

	LONGLONG DelayTime();

	BOOL ReRec(DWORD reserveID, BOOL deleteFile);

	BOOL GetRecFilePath(
		DWORD reserveID,
		wstring& filePath,
		DWORD* ctrlID,
		DWORD* processID
		);

	static BOOL OpenTunerExe(
		LPCWSTR exePath,
		LPCWSTR bonDriver,
		DWORD id,
		BOOL minWake, BOOL noView, BOOL noNW, BOOL nwUdp, BOOL nwTcp,
		DWORD priority,
		const map<DWORD, DWORD>& registGUIMap,
		DWORD* pid
		);

	static void CloseTunerExe(
		DWORD pid
		);
protected:
	HANDLE lockEvent;

	CTwitterManager* twitterManager;
	CNotifyManager* notifyManager;
	DWORD tunerID;
	wstring bonFileName;
	CParseChText4 chUtil;
	CEpgDBManager* epgDBManager;

	typedef struct _RESERVE_WORK{
		CReserveInfo* reserveInfo;
		DWORD reserveID;
		DWORD mainCtrlID;
		DWORD partialCtrlID;
		vector<DWORD> ctrlID;
		BOOL recStartFlag;

		LONGLONG stratTime;
		LONGLONG endTime;
		LONGLONG startMargine;
		LONGLONG endMargine;
		DWORD chID;
		BYTE priority;

		BYTE enableScramble;
		BYTE enableCaption;
		BYTE enableData;

		BOOL savedPgInfo;

		BYTE partialRecFlag;
		BYTE continueRecFlag;

		WORD ONID;
		WORD TSID;
		WORD SID;

		BYTE notStartHeadFlag;
		EPGDB_EVENT_INFO* eventInfo;
		//=オペレーターの処理
		_RESERVE_WORK(void){
			reserveInfo = NULL;
			reserveID = 0;
			mainCtrlID = 0;
			partialCtrlID = 0;
			recStartFlag = FALSE;
			stratTime = 0;
			endTime = 0;
			startMargine = 0;
			endMargine = 0;
			chID = 0;
			priority = 0;
			enableScramble = 2;
			enableCaption = 2;
			enableData = 2;

			savedPgInfo = FALSE;

			partialRecFlag = 0;
			continueRecFlag = 0;
			ONID = 0xFFFF;
			TSID = 0xFFFF;
			SID = 0xFFFF;

			notStartHeadFlag = FALSE;
			eventInfo = NULL;
		};
		~_RESERVE_WORK(void){
			SAFE_DELETE(eventInfo);
		};
	}RESERVE_WORK;
	map<DWORD, RESERVE_WORK*> reserveWork; //キーreserveID
	map<DWORD, RESERVE_WORK*> createCtrlList; //キーreserveID
	map<DWORD, RESERVE_WORK*> openErrReserveList; //キーreserveID
	vector<END_RESERVE_INFO*> endList;

	BOOL openTuner;
	DWORD processID;
	BOOL openErrFlag;
	BOOL useOpendTuner;
	DWORD currentChID;
	CSendCtrlCmd sendCtrl;

	HANDLE checkThread;
	HANDLE checkStopEvent;

	//map<DWORD, DWORD> registGUIMap;
	LONGLONG defStartMargine;
	LONGLONG defEndMargine;
	LONGLONG recWakeTime;
	BOOL recMinWake;
	BOOL recView;
	BOOL recNW;
	BOOL backPriority;
	BOOL saveProgramInfo;
	BOOL saveErrLog;
	BOOL recOverWrite;
	BOOL useRecNamePlugIn;
	wstring recNamePlugInFilePath;
	wstring recFolderPath;
	wstring recWritePlugIn;
	wstring recExePath;
	BYTE enableCaption;
	BYTE enableData;
	DWORD processPriority;
	BOOL keepDisk;

	LONGLONG delayTime;

	BOOL epgCapWork;
	vector<SET_CH_INFO> epgCapItem;

	BOOL autoDel;
	vector<wstring> delExtList;
	vector<wstring> delFolderList;
	DWORD chkSpaceCount;

protected:
	//PublicAPI排他制御用
	BOOL Lock(LPCWSTR log = NULL, DWORD timeOut = 60*1000);
	void UnLock(LPCWSTR log = NULL);

	static UINT WINAPI CheckReserveThread(LPVOID param);

	void GetCheckList(multimap<LONGLONG, RESERVE_WORK*>* sortList);
	BOOL IsNeedOpenTuner(multimap<LONGLONG, RESERVE_WORK*>* sortList, BOOL* viewMode, SET_CH_INFO* initCh);

	BOOL OpenTuner(BOOL viewMode, SET_CH_INFO* initCh);
	void CreateCtrl(multimap<LONGLONG, RESERVE_WORK*>* sortList, LONGLONG delay);
	void CreateCtrl(RESERVE_WORK* info);
	BOOL CheckOtherChCreate(LONGLONG nowTime, RESERVE_WORK* reserve);
	void StopAllRec();
	void ErrStop();
	void AddEndReserve(RESERVE_WORK* reserve, DWORD endType, SET_CTRL_REC_STOP_RES_PARAM resVal);
	void CheckRec(LONGLONG delay, BOOL* needShortCheck, DWORD wait);
	BOOL RecStart(LONGLONG nowTime, RESERVE_WORK* reserve, BOOL sendNoyify);
	BOOL CloseTuner();

	BOOL ContinueRec(RESERVE_WORK* info);
	BOOL FindPartialService(WORD ONID, WORD TSID, WORD SID, WORD* partialSID, wstring* serviceName);

	BOOL IsFindContinueReserve(RESERVE_WORK* reserve, DWORD* continueSec);

	void SaveProgramInfo(wstring savePath, EPGDB_EVENT_INFO* info, BYTE mode, BOOL addMode = FALSE);
	//void CopyEpgInfo(EPG_EVENT_INFO* destInfo, EPGDB_EVENT_INFO* srcInfo);
};

