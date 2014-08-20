#pragma once

#include "../../Common/Util.h"
#include "../../Common/ErrDef.h"
#include "../../Common/EpgTimerUtil.h"
#include "../../Common/StringUtil.h"
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

	void GetRegistGUI(map<DWORD, DWORD>* registGUI);
	void GetRegistTCP(map<wstring, REGIST_TCP_INFO>* registTCP);

	void AddNotify(DWORD notifyID);
	void AddNotifySrvStatus(DWORD status);
	void AddNotifyMsg(DWORD notifyID, wstring msg);

protected:
	CRITICAL_SECTION managerLock;

	HANDLE notifyEvent;
	HANDLE notifyThread;
	BOOL notifyStopFlag;

	map<DWORD, DWORD> registGUIMap;
	map<wstring, REGIST_TCP_INFO> registTCPMap;

	vector<NOTIFY_SRV_INFO> notifyList;
protected:
	void _SendNotify();
	static UINT WINAPI SendNotifyThread(LPVOID param);
};

