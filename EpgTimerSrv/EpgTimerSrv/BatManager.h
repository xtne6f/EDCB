#pragma once
#include "../../Common/StructDef.h"
#include "NotifyManager.h"

typedef struct _BAT_WORK_INFO{
	wstring batFilePath;
	BYTE suspendMode;
	BYTE rebootFlag;
	wstring addKey;
	REC_FILE_INFO recFileInfo;
}BAT_WORK_INFO;

class CBatManager
{
public:
	CBatManager(CNotifyManager& notifyManager_);
	~CBatManager(void);

	void AddBatWork(const BAT_WORK_INFO& info);

	DWORD GetWorkCount() const;
	BOOL IsWorking() const;

	void StartWork();
	void PauseWork();

	BOOL PopLastWorkSuspend(BYTE* suspendMode, BYTE* rebootFlag);
protected:
	mutable CRITICAL_SECTION managerLock;

	CNotifyManager& notifyManager;

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

