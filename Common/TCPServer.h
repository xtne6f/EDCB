#pragma once

#include "StringUtil.h"
#include "CtrlCmdDef.h"
#include "StructDef.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

typedef int (CALLBACK *CMD_CALLBACK_PROC)(void* pParam, CMD_STREAM* pCmdParam, CMD_STREAM* pResParam);

class CTCPServer
{
public:
	CTCPServer(void);
	~CTCPServer(void);

	BOOL StartServer(
		DWORD dwPort, 
		DWORD dwResponseTimeout,
		LPCWSTR acl,
		CMD_CALLBACK_PROC pfnCmdProc, 
		void* pParam
		);
	void StopServer();
	void NotifyUpdate();

protected:
	CMD_CALLBACK_PROC m_pCmdProc;
	void* m_pParam;
	DWORD m_dwPort;
	DWORD m_dwResponseTimeout;
	wstring m_acl;

	WSAEVENT m_hNotifyEvent;
	BOOL m_stopFlag;
	HANDLE m_hThread;

	SOCKET m_sock;
	
protected:
	static UINT WINAPI ServerThread(LPVOID pParam);

};
