#pragma once

#include "StringUtil.h"
#include "CtrlCmdDef.h"
#include "StructDef.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "wsock32.lib")

typedef int (CALLBACK *CMD_CALLBACK_PROC)(void* pParam, CMD_STREAM* pCmdParam, CMD_STREAM* pResParam);

class CTCPServer
{
public:
	CTCPServer(void);
	~CTCPServer(void);

	BOOL StartServer(
		DWORD dwPort, 
		LPCWSTR acl,
		CMD_CALLBACK_PROC pfnCmdProc, 
		void* pParam
		);
	void StopServer();

protected:
	CMD_CALLBACK_PROC m_pCmdProc;
	void* m_pParam;
	DWORD m_dwPort;
	wstring m_acl;

	HANDLE m_hStopEvent;
	HANDLE m_hThread;

	SOCKET m_sock;
	struct sockaddr_in m_addr;
	
protected:
	static UINT WINAPI ServerThread(LPVOID pParam);

};
