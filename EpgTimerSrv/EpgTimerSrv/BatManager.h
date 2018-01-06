#pragma once
#include "../../Common/ThreadUtil.h"
#include "NotifyManager.h"

struct BAT_WORK_INFO {
	wstring batFilePath;
	vector<pair<string, wstring>> macroList;
};

class CBatManager
{
public:
	CBatManager(CNotifyManager& notifyManager_, LPCWSTR tmpBatFileName);
	~CBatManager();

	void AddBatWork(const BAT_WORK_INFO& info);
	void SetIdleMargin(DWORD marginSec);

	DWORD GetWorkCount() const;
	bool IsWorking() const;
protected:
	mutable recursive_mutex_ managerLock;

	CNotifyManager& notifyManager;
	wstring tmpBatFilePath;

	vector<BAT_WORK_INFO> workList;

	DWORD idleMargin;
	DWORD nextBatMargin;
	bool batWorkExitingFlag;
	thread_ batWorkThread;
	CAutoResetEvent batWorkStopEvent;
protected:
	void StartWork();
	static void BatWorkThread(CBatManager* sys);

	static bool CreateBatFile(BAT_WORK_INFO& info, LPCWSTR batFilePath, DWORD& exBatMargin, WORD& exSW, wstring& exDirect);
	static bool ExpandMacro(const string& var, const BAT_WORK_INFO& info, wstring& strWrite);
	static wstring CreateEnvironment(const BAT_WORK_INFO& info);
};

