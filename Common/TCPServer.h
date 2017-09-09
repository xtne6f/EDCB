#pragma once

#include "StringUtil.h"
#include "CtrlCmdDef.h"
#include "StructDef.h"

#include <functional>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

class CTCPServer
{
public:
	CTCPServer(void);
	~CTCPServer(void);

	bool StartServer(unsigned short port, bool ipv6, DWORD dwResponseTimeout, LPCWSTR acl,
	                 const std::function<void(CMD_STREAM*, CMD_STREAM*)>& cmdProc);
	void StopServer();
	void NotifyUpdate();

protected:
	std::function<void(CMD_STREAM*, CMD_STREAM*)> m_cmdProc;
	unsigned short m_port;
	bool m_ipv6;
	DWORD m_dwResponseTimeout;
	string m_acl;

	WSAEVENT m_hNotifyEvent;
	BOOL m_stopFlag;
	HANDLE m_hThread;

	SOCKET m_sock;
	
protected:
	static UINT WINAPI ServerThread(LPVOID pParam);

};
