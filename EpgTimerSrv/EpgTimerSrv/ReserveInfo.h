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

	void SetData(const RESERVE_DATA& data);
	void GetData(RESERVE_DATA* data);

	void GetRecMode(BYTE* recMode);
	void GetService(WORD* ONID, WORD* TSID, WORD* SID);

	void SetOverlapMode(BYTE mode);
protected:
	HANDLE lockEvent;

	RESERVE_DATA reserveData;
protected:
	//PublicAPIîrëºêßå‰óp
	BOOL Lock(LPCWSTR log = NULL, DWORD timeOut = 60*1000);
	void UnLock(LPCWSTR log = NULL);

};

