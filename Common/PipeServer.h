#pragma once

#include "StructDef.h"
#include "ThreadUtil.h"
#include <functional>

class CPipeServer
{
public:
	CPipeServer(void);
	~CPipeServer(void);

	BOOL StartServer(
		LPCWSTR eventName, 
		LPCWSTR pipeName, 
		const std::function<void(CMD_STREAM*, CMD_STREAM*)>& cmdProc_,
		BOOL insecureFlag = FALSE
		);
	BOOL StopServer(BOOL checkOnlyFlag = FALSE);

	//SERVICE_NAMEのサービスセキュリティ識別子(Service-specific SID)に対するアクセス許可を追加する
	static BOOL GrantServerAccessToKernelObject(HANDLE handle, DWORD permissions);

protected:
	std::function<void(CMD_STREAM*, CMD_STREAM*)> cmdProc;

	CAutoResetEvent stopEvent;
	HANDLE hEventConnect;
	HANDLE hPipe;
	thread_ workThread;

protected:
	static BOOL GrantAccessToKernelObject(HANDLE handle, WCHAR* trusteeName, DWORD permissions);
	static void ServerThread(CPipeServer* pSys);

};
