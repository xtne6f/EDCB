#pragma once

#include "../../Common/ErrDef.h"
#include "../../Common/CommonDef.h"

class CNotifyManager
{
public:
	CNotifyManager(void);
	~CNotifyManager(void);

	void RegistGUI(DWORD processID);
	void RegistTCP(const REGIST_TCP_INFO& info);
	void UnRegistGUI(DWORD processID);
	void UnRegistTCP(const REGIST_TCP_INFO& info);
	void SetNotifyWindow(HWND hwnd, UINT msgID);
	vector<NOTIFY_SRV_INFO> RemoveSentList();
	BOOL GetNotify(NOTIFY_SRV_INFO* info, DWORD targetCount);

	vector<DWORD> GetRegistGUI() const;
	vector<REGIST_TCP_INFO> GetRegistTCP() const;

	void AddNotify(DWORD notifyID);
	void SetNotifySrvStatus(DWORD status);
	void AddNotifyMsg(DWORD notifyID, wstring msg);

protected:
	mutable CRITICAL_SECTION managerLock;

	HANDLE notifyEvent;
	HANDLE notifyThread;
	BOOL notifyStopFlag;
	DWORD srvStatus;
	DWORD notifyCount;
	size_t notifyRemovePos;

	vector<pair<DWORD, HANDLE>> registGUIList;
	vector<REGIST_TCP_INFO> registTCPList;
	HWND hwndNotify;
	UINT msgIDNotify;

	vector<NOTIFY_SRV_INFO> notifyList;
	vector<NOTIFY_SRV_INFO> notifySentList;
protected:
	void SendNotify();
	static UINT WINAPI SendNotifyThread(LPVOID param);
};

