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
	void RegistTCP(REGIST_TCP_INFO* info);
	void UnRegistGUI(DWORD processID);
	void UnRegistTCP(REGIST_TCP_INFO* info);

	void GetRegistGUI(map<DWORD, DWORD>* registGUI);
	void GetRegistTCP(map<wstring, REGIST_TCP_INFO>* registTCP);

	void AddNotifyLog(NOTIFY_SRV_INFO info);
	void AddNotify(DWORD notifyID);
	void AddNotifySrvStatus(DWORD status);
	void AddNotifyMsg(DWORD notifyID, wstring msg);

protected:
	HANDLE lockEvent;

	HANDLE lockNotify;
	HANDLE notifyThread;
	HANDLE notifyStopEvent;

	map<DWORD, DWORD> registGUIMap;
	map<wstring, REGIST_TCP_INFO> registTCPMap;

	vector<NOTIFY_SRV_INFO> notifyList;
protected:
	//ÉRÉ}ÉìÉhä÷åWîrëºêßå‰óp
	BOOL Lock(LPCWSTR log = NULL, DWORD timeOut = 60*1000);
	void UnLock(LPCWSTR log = NULL);

	BOOL NotifyLock(LPCWSTR log = NULL, DWORD timeOut = 60*1000);
	void NotifyUnLock(LPCWSTR log = NULL);

	void _SendNotify();
	static UINT WINAPI SendNotifyThread(LPVOID param);
};

