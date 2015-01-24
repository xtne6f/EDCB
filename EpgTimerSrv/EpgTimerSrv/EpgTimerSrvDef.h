#ifndef __EPG_TIMER_SRV_DEF_H__
#define __EPG_TIMER_SRV_DEF_H__

#include "ReserveInfo.h"

typedef struct _END_RESERVE_INFO{
	CReserveInfo* reserveInfo;
	DWORD reserveID;
	DWORD tunerID;
	DWORD endType;
	wstring recFilePath;
	ULONGLONG drop;
	ULONGLONG scramble;
	wstring epgEventName;
	WORD epgOriginalNetworkID;
	WORD epgTransportStreamID;
	WORD epgServiceID;
	SYSTEMTIME epgStartTime;
}END_RESERVE_INFO;

typedef struct _BAT_WORK_INFO{
	DWORD tunerID;
	RESERVE_DATA reserveInfo;
	REC_FILE_INFO recFileInfo;
}BAT_WORK_INFO;

#endif
