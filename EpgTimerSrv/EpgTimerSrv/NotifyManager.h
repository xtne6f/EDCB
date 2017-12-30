#pragma once

#include "../../Common/ErrDef.h"
#include "../../Common/CommonDef.h"
#include "../../Common/ThreadUtil.h"

class CNotifyManager
{
public:
	CNotifyManager();
	~CNotifyManager();

	void RegistGUI(DWORD processID);
	void RegistTCP(const REGIST_TCP_INFO& info);
	void UnRegistGUI(DWORD processID);
	void UnRegistTCP(const REGIST_TCP_INFO& info);
	void SetNotifyWindow(HWND hwnd, UINT msgID);
	vector<NOTIFY_SRV_INFO> RemoveSentList();
	bool GetNotify(NOTIFY_SRV_INFO* info, DWORD targetCount) const;

	vector<DWORD> GetRegistGUI() const;
	vector<REGIST_TCP_INFO> GetRegistTCP() const;

	void AddNotify(DWORD notifyID);
	void SetNotifySrvStatus(DWORD status);
	void AddNotifyMsg(DWORD notifyID, wstring msg);

	bool IsGUI() const { return guiFlag; }
	void SetGUI(bool f) { guiFlag = f; }

protected:
	mutable recursive_mutex_ managerLock;

	CAutoResetEvent notifyEvent;
	thread_ notifyThread;
	bool notifyStopFlag;
	DWORD srvStatus;
	DWORD notifyCount;
	size_t notifyRemovePos;

	vector<pair<DWORD, HANDLE>> registGUIList;
	vector<REGIST_TCP_INFO> registTCPList;
	HWND hwndNotify;
	UINT msgIDNotify;
	bool guiFlag;

	vector<NOTIFY_SRV_INFO> notifyList;
	vector<NOTIFY_SRV_INFO> notifySentList;
protected:
	void SendNotify();
	static void SendNotifyThread(CNotifyManager* sys);
};

