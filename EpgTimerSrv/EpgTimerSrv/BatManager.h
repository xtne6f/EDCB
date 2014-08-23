#pragma once
#include "EpgTimerSrvDef.h"
#include "../../Common/StructDef.h"
#include "../../Common/SendCtrlCmd.h"
#include "NotifyManager.h"

class CBatManager
{
public:
	CBatManager(void);
	~CBatManager(void);

	void SetNotifyManager(CNotifyManager* manager);

	void AddBatWork(const BAT_WORK_INFO& info);

	DWORD GetWorkCount();
	BOOL IsWorking();

	void StartWork();
	void PauseWork();

	BOOL GetLastWorkSuspend(BYTE* suspendMode, BYTE* rebootFlag);
protected:
	CRITICAL_SECTION managerLock;

	CNotifyManager* notifyManager;

	vector<BAT_WORK_INFO> workList;

	BOOL pauseFlag;
	BOOL batWorkExitingFlag;
	HANDLE batWorkThread;
	HANDLE batWorkStopEvent;

	BYTE lastSuspendMode;
	BYTE lastRebootFlag;
protected:
	static UINT WINAPI BatWorkThread(LPVOID param);

	static BOOL CreateBatFile(const BAT_WORK_INFO& info, LPCWSTR batSrcFilePath, LPCWSTR batFilePath );
	static BOOL ExpandMacro(const string& var, const BAT_WORK_INFO& info, string& strWrite);
};

