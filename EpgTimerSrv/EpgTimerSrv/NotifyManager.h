#pragma once

#include "../../Common/ErrDef.h"
#include "../../Common/CommonDef.h"
#include "../../Common/ThreadUtil.h"
#include <functional>

class CNotifyManager
{
public:
	CNotifyManager();
	~CNotifyManager();

	void RegistGUI(DWORD processID);
	void RegistTCP(LPCWSTR ip, DWORD port);
	void UnRegistGUI(DWORD processID);
	void UnRegistTCP(LPCWSTR ip, DWORD port);
	void SetNotifyCallback(const std::function<void()>& proc);
	void SetLogFilePath(LPCWSTR path);
	bool GetNotify(NOTIFY_SRV_INFO* info, DWORD targetCount) const;
	bool WaitForIdle(DWORD timeoutMsec) const;
	int GetNotifyUpdateCount(DWORD notifyID) const;
	static pair<LPCWSTR, LPCWSTR> ExtractTitleFromInfo(const NOTIFY_SRV_INFO* info);

	vector<DWORD> GetRegistGUI() const;
	vector<pair<wstring, DWORD>> GetRegistTCP() const;

	void AddNotify(DWORD notifyID);
	void SetNotifySrvStatus(DWORD status);
	void AddNotifyMsg(DWORD notifyID, const wstring& msg);

	bool IsGUI() const { return guiFlag; }
	void SetGUI(bool f) { guiFlag = f; }

protected:
	mutable recursive_mutex_ managerLock;

	CAutoResetEvent notifyEvent;
	thread_ notifyThread;
	atomic_bool_ notifyStopFlag;
	DWORD srvStatus;
	DWORD notifyCount;
	DWORD activeOrIdleCount;
	DWORD notifyUpdateCount[6];
	vector<DWORD> registGUIList;
	vector<pair<wstring, DWORD>> registTCPList;
	std::function<void()> notifyProc;
	wstring logFilePath;
	bool guiFlag;

	vector<NOTIFY_SRV_INFO> notifyList;
	vector<NOTIFY_SRV_INFO> notifySentList;
protected:
	void SendNotify();
	static void SendNotifyThread(CNotifyManager* sys);
};

