#pragma once
#include "NotifyManager.h"

struct BAT_WORK_INFO {
	wstring batFilePath;
	vector<pair<string, wstring>> macroList;
};

class CBatManager
{
public:
	CBatManager(CNotifyManager& notifyManager_, LPCWSTR tmpBatFileName);
	~CBatManager(void);

	void AddBatWork(const BAT_WORK_INFO& info);
	void SetIdleMargin(DWORD marginSec);

	DWORD GetWorkCount() const;
	BOOL IsWorking() const;
protected:
	mutable CRITICAL_SECTION managerLock;

	CNotifyManager& notifyManager;
	wstring tmpBatFilePath;

	vector<BAT_WORK_INFO> workList;

	DWORD idleMargin;
	DWORD nextBatMargin;
	BOOL batWorkExitingFlag;
	HANDLE batWorkThread;
	HANDLE batWorkStopEvent;
protected:
	void StartWork();
	static UINT WINAPI BatWorkThread(LPVOID param);

	static BOOL CreateBatFile(const BAT_WORK_INFO& info, LPCWSTR batSrcFilePath, LPCWSTR batFilePath, DWORD& exBatMargin, WORD& exSW, wstring& exDirect);
	static BOOL ExpandMacro(const string& var, const BAT_WORK_INFO& info, wstring& strWrite);
	static wstring CreateEnvironment(const BAT_WORK_INFO& info);
};

