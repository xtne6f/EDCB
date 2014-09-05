#include "StdAfx.h"
#include "ReserveInfo.h"


CReserveInfo::CReserveInfo(void)
{
	this->lockEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
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

void CReserveInfo::SetData(const RESERVE_DATA& data)
{
	if( Lock() == FALSE ) return;

	this->reserveData = data;

	UnLock();
}

void CReserveInfo::GetData(RESERVE_DATA* data)
{
	if( Lock() == FALSE ) return;

	*data = this->reserveData;

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
