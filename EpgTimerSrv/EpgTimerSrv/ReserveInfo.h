#pragma once

#include "../../Common/Util.h"
#include "../../Common/CommonDef.h"
#include "../../Common/StructDef.h"
#include "../../Common/EpgTimerUtil.h"
#include "../../Common/TimeUtil.h"

class CReserveInfo
{
public:
	CReserveInfo(void);
	~CReserveInfo(void);

	void SetData(RESERVE_DATA* data);
	void GetData(RESERVE_DATA* data);

	void SetNGChTunerID(vector<DWORD>* idList);

	void AddNGTunerID(DWORD id);
	void ClearAddNGTuner();

	BOOL IsNGTuner(DWORD id);

	void SetRecWaitMode(BOOL recWaitFlag, DWORD tunerID);

	void GetRecWaitMode(BOOL* recWaitFlag, DWORD* tunerID);

	void GetStartTime(SYSTEMTIME* startTime);
	void GetDuration(DWORD* durationSec);
	void GetMargine(BOOL* useFlag, int* startMargine, int* endMargine);
	void GetPriority(BYTE* priority);
	void GetRecMode(BYTE* recMode);
	void GetService(WORD* ONID, WORD* TSID, WORD* SID);

	void SetOverlapMode(BYTE mode);

	void SetContinueRecFlag(BOOL start);
	BOOL IsContinueRec();

	void SetChkPfInfo(BOOL chk);
	BOOL IsChkPfInfo();

	void SetPfInfoAddMode(BOOL mode);
	BOOL IsPfInfoAddMode();

	DWORD GetReserveAddStatus();

	void SetOpenErred();
	BOOL IsOpenErred();
protected:
	HANDLE lockEvent;

	RESERVE_DATA reserveData;

	map<DWORD, DWORD> NGChTunerMap;
	map<DWORD, DWORD> NGTunerMap;

	BOOL recWaitFlag;
	DWORD recWaitTunerID;

	BOOL continueRecStart;

	BOOL pfInfoCheck;
	BOOL pfInfoAddMode;

	BOOL openErr;
protected:
	//PublicAPIîrëºêßå‰óp
	BOOL Lock(LPCWSTR log = NULL, DWORD timeOut = 60*1000);
	void UnLock(LPCWSTR log = NULL);

};

