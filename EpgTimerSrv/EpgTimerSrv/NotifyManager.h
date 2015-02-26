#pragma once

#include "../../Common/Util.h"
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

	vector<NOTIFY_SRV_INFO> notifyList;
protected:
	void _SendNotify();
	static UINT WINAPI SendNotifyThread(LPVOID param);
};

