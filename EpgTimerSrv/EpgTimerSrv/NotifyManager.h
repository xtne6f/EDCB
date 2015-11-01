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

	void GetRegistGUI(map<DWORD, DWORD>* registGUI) const;
	void GetRegistTCP(map<wstring, REGIST_TCP_INFO>* registTCP) const;

	void AddNotify(DWORD notifyID);
	void SetNotifySrvStatus(DWORD status);
	void AddNotifyMsg(DWORD notifyID, wstring msg);

protected:
	mutable CRITICAL_SECTION managerLock;

	HANDLE notifyEvent;
	HANDLE notifyThread;
	BOOL notifyStopFlag;
	DWORD srvStatus;

	map<DWORD, DWORD> registGUIMap;
	map<wstring, REGIST_TCP_INFO> registTCPMap;
	HWND hwndNotify;
	UINT msgIDNotify;

	vector<NOTIFY_SRV_INFO> notifyList;
	vector<NOTIFY_SRV_INFO> notifySentList;
protected:
	void _SendNotify();
	static UINT WINAPI SendNotifyThread(LPVOID param);
};

