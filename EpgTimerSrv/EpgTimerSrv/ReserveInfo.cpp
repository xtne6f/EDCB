#include "StdAfx.h"
#include "ReserveInfo.h"


CReserveInfo::CReserveInfo(void)
{
	this->lockEvent = CreateEvent(NULL, FALSE, TRUE, NULL);

	this->recWaitFlag = FALSE;
	this->recWaitTunerID = 0;

	this->continueRecStart = FALSE;
	this->pfInfoCheck = FALSE;
	this->pfInfoAddMode = FALSE;

	this->openErr = FALSE;
}


CReserveInfo::~CReserveInfo(void)
{
	if( this->lockEvent != NULL ){
		UnLock();
		CloseHandle(this->lockEvent);
		this->lockEvent = NULL;
	}
}

BOOL CReserveInfo::Lock(LPCWSTR log, DWORD timeOut)
{
	if( this->lockEvent == NULL ){
		return FALSE;
	}
	if( log != NULL ){
		OutputDebugString(log);
	}
	DWORD dwRet = WaitForSingleObject(this->lockEvent, timeOut);
	if( dwRet == WAIT_ABANDONED || 
		dwRet == WAIT_FAILED){
		return FALSE;
	}
	return TRUE;
}

void CReserveInfo::UnLock(LPCWSTR log)
{
	if( this->lockEvent != NULL ){
		SetEvent(this->lockEvent);
	}
	if( log != NULL ){
		OutputDebugString(log);
	}
}

void CReserveInfo::SetData(RESERVE_DATA* data)
{
	if( Lock() == FALSE ) return;

	this->reserveData = *data;

	UnLock();
}

void CReserveInfo::GetData(RESERVE_DATA* data)
{
	if( Lock() == FALSE ) return;

	*data = this->reserveData;

	UnLock();
}

void CReserveInfo::SetNGChTunerID(vector<DWORD>* idList)
{
	if( Lock() == FALSE ) return;

	this->NGChTunerMap.clear();
	for( size_t i=0; i<idList->size(); i++ ){
		this->NGChTunerMap.insert(pair<DWORD,DWORD>((*idList)[i], (*idList)[i]));
	}

	UnLock();
}

void CReserveInfo::AddNGTunerID(DWORD id)
{
	if( Lock() == FALSE ) return;

	this->NGTunerMap.insert(pair<DWORD,DWORD>(id,id));

	UnLock();
}

void CReserveInfo::ClearAddNGTuner()
{
	if( Lock() == FALSE ) return;

	this->NGTunerMap.clear();

	UnLock();
}

BOOL CReserveInfo::IsNGTuner(DWORD id)
{
	if( Lock() == FALSE ) return TRUE;

	BOOL ret = FALSE;
	map<DWORD, DWORD>::iterator itr;
	itr = this->NGChTunerMap.find(id);
	if( itr != this->NGChTunerMap.end() ){
		ret = TRUE;
	}else{
		itr = this->NGTunerMap.find(id);
		if( itr != this->NGTunerMap.end() ){
			ret = TRUE;
		}
	}

	UnLock();

	return ret;
}

void CReserveInfo::SetRecWaitMode(BOOL recWaitFlag, DWORD tunerID)
{
	if( Lock() == FALSE ) return;

	this->recWaitFlag = recWaitFlag;
	if( recWaitFlag == FALSE ){
		this->recWaitTunerID = 0;
	}else{
		this->recWaitTunerID = tunerID;
	}

	UnLock();
}

void CReserveInfo::GetRecWaitMode(BOOL* recWaitFlag, DWORD* tunerID)
{
	if( Lock() == FALSE ) return;

	*recWaitFlag = this->recWaitFlag;
	*tunerID = this->recWaitTunerID;

	UnLock();
}

void CReserveInfo::GetStartTime(SYSTEMTIME* startTime)
{
	if( Lock() == FALSE ) return;

	if( startTime != NULL ){
		*startTime = this->reserveData.startTime;
	}

	UnLock();
}

void CReserveInfo::GetDuration(DWORD* durationSec)
{
	if( Lock() == FALSE ) return;

	if( durationSec != NULL ){
		*durationSec = this->reserveData.durationSecond;
	}

	UnLock();
}

void CReserveInfo::GetMargine(BOOL* useFlag, int* startMargine, int* endMargine)
{
	if( Lock() == FALSE ) return;

	if( useFlag != NULL ){
		*useFlag = this->reserveData.recSetting.useMargineFlag;
	}
	if( startMargine != NULL ){
		*startMargine = this->reserveData.recSetting.startMargine;
	}
	if( endMargine != NULL ){
		*endMargine = this->reserveData.recSetting.endMargine;
	}

	UnLock();
}

void CReserveInfo::GetPriority(BYTE* priority)
{
	if( Lock() == FALSE ) return;

	if( priority != NULL ){
		*priority = this->reserveData.recSetting.priority;
	}

	UnLock();
}

void CReserveInfo::GetRecMode(BYTE* recMode)
{
	if( Lock() == FALSE ) return;

	if( recMode != NULL ){
		*recMode = this->reserveData.recSetting.recMode;
	}

	UnLock();
}

void CReserveInfo::GetService(WORD* ONID, WORD* TSID, WORD* SID)
{
	if( Lock() == FALSE ) return;

	if( ONID != NULL ){
		*ONID = this->reserveData.originalNetworkID;
	}
	if( TSID != NULL ){
		*TSID = this->reserveData.transportStreamID;
	}
	if( SID != NULL ){
		*SID = this->reserveData.serviceID;
	}

	UnLock();
}

void CReserveInfo::SetOverlapMode(BYTE mode)
{
	if( Lock() == FALSE ) return;

	this->reserveData.overlapMode = mode;

	UnLock();
}

void CReserveInfo::SetContinueRecFlag(BOOL start)
{
	if( Lock() == FALSE ) return;

	this->continueRecStart = start;

	UnLock();
}

BOOL CReserveInfo::IsContinueRec()
{
	BOOL ret = FALSE;
	if( Lock() == FALSE ) return FALSE;

	ret = this->continueRecStart;

	UnLock();
	return ret;
}

void CReserveInfo::SetChkPfInfo(BOOL chk)
{
	if( Lock() == FALSE ) return;

	this->pfInfoCheck = chk;

	UnLock();
}

BOOL CReserveInfo::IsChkPfInfo()
{
	BOOL ret = FALSE;
	if( Lock() == FALSE ) return FALSE;

	ret = this->pfInfoCheck;

	UnLock();
	return ret;
}

void CReserveInfo::SetPfInfoAddMode(BOOL mode)
{
	if( Lock() == FALSE ) return;

	this->pfInfoAddMode = mode;

	UnLock();
}

BOOL CReserveInfo::IsPfInfoAddMode()
{
	BOOL ret = FALSE;
	if( Lock() == FALSE ) return FALSE;

	ret = this->pfInfoAddMode;

	UnLock();
	return ret;
}

DWORD CReserveInfo::GetReserveAddStatus()
{
	DWORD ret = ADD_RESERVE_NORMAL;
	if( Lock() == FALSE ) return FALSE;

	ret = this->reserveData.reserveStatus;

	UnLock();
	return ret;
}

void CReserveInfo::SetOpenErred()
{
	if( Lock() == FALSE ) return ;

	openErr = TRUE;

	UnLock();
}

BOOL CReserveInfo::IsOpenErred()
{
	if( Lock() == FALSE ) return FALSE;

	BOOL ret = openErr;

	UnLock();
	return ret;
}
