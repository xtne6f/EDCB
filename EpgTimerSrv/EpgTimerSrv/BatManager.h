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
	void SetIdleMargin(DWORD marginSec);

	DWORD GetWorkCount() const;
	BOOL IsWorking() const;

	BOOL PopLastWorkSuspend(BYTE* suspendMode, BYTE* rebootFlag);
protected:
	mutable CRITICAL_SECTION managerLock;

	CNotifyManager& notifyManager;

	vector<BAT_WORK_INFO> workList;

	DWORD idleMargin;
	DWORD nextBatMargin;
	BOOL batWorkExitingFlag;
	HANDLE batWorkThread;
	HANDLE batWorkStopEvent;

	BYTE lastSuspendMode;
	BYTE lastRebootFlag;
protected:
	void StartWork();
	static UINT WINAPI BatWorkThread(LPVOID param);

	static BOOL CreateBatFile(const BAT_WORK_INFO& info, LPCWSTR batSrcFilePath, LPCWSTR batFilePath, DWORD& exBatMargin, WORD& exSW, wstring& exDirect);
	static BOOL ExpandMacro(const string& var, const BAT_WORK_INFO& info, wstring& strWrite);
	static wstring CreateEnvironment(const BAT_WORK_INFO& info);
};

