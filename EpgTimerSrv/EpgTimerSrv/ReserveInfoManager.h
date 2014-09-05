#pragma once

#include <map>
#include "../../Common/BlockLock.h"

class CReserveInfoManager
{
public:
	CReserveInfoManager() {
		InitializeCriticalSection(&managerLock_);
	}
	~CReserveInfoManager() {
		DeleteCriticalSection(&managerLock_);
	}
	void AddNGTunerID(DWORD reserveID, DWORD tunerID) {
		CBlockLock lock(&managerLock_);
		reserveInfoMap_[reserveID].ngTunerIDMap.insert(std::pair<DWORD, DWORD>(tunerID, 0));
	}
	void ClearNGTunerID(DWORD reserveID) {
		CBlockLock lock(&managerLock_);
		reserveInfoMap_[reserveID].ngTunerIDMap.clear();
	}
	BOOL IsNGTunerID(DWORD reserveID, DWORD tunerID) {
		CBlockLock lock(&managerLock_);
		return reserveInfoMap_.count(reserveID) && reserveInfoMap_[reserveID].ngTunerIDMap.count(tunerID) ? TRUE : FALSE;
	}
	void SetRecWaitMode(DWORD reserveID, BOOL recWaitFlag, DWORD tunerID) {
		CBlockLock lock(&managerLock_);
		reserveInfoMap_[reserveID].recWaitFlag = recWaitFlag;
		reserveInfoMap_[reserveID].recWaitTunerID = recWaitFlag != FALSE ? tunerID : 0;
	}
	void GetRecWaitMode(DWORD reserveID, BOOL* recWaitFlag, DWORD* tunerID) {
		CBlockLock lock(&managerLock_);
		*recWaitFlag = reserveInfoMap_.count(reserveID) ? reserveInfoMap_[reserveID].recWaitFlag : FALSE;
		*tunerID = reserveInfoMap_.count(reserveID) ? reserveInfoMap_[reserveID].recWaitTunerID : 0;
	}
	void SetChkPfInfo(DWORD reserveID) {
		CBlockLock lock(&managerLock_);
		reserveInfoMap_[reserveID].chkPfInfoFlag = TRUE;
	}
	BOOL IsChkPfInfo(DWORD reserveID) {
		CBlockLock lock(&managerLock_);
		return reserveInfoMap_.count(reserveID) ? reserveInfoMap_[reserveID].chkPfInfoFlag : FALSE;
	}
	void SetOpenErred(DWORD reserveID) {
		CBlockLock lock(&managerLock_);
		reserveInfoMap_[reserveID].openErrFlag = TRUE;
	}
	BOOL IsOpenErred(DWORD reserveID) {
		CBlockLock lock(&managerLock_);
		return reserveInfoMap_.count(reserveID) ? reserveInfoMap_[reserveID].openErrFlag : FALSE;
	}
private:
	struct RESERVE_INFO
	{
		std::map<DWORD, DWORD> ngTunerIDMap;
		BOOL recWaitFlag;
		DWORD recWaitTunerID;
		BOOL chkPfInfoFlag;
		BOOL openErrFlag;
		RESERVE_INFO()
			: recWaitFlag(FALSE)
			, recWaitTunerID(0)
			, chkPfInfoFlag(FALSE)
			, openErrFlag(FALSE) {}
	};
	CRITICAL_SECTION managerLock_;
	std::map<DWORD, RESERVE_INFO> reserveInfoMap_;
};
