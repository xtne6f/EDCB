#pragma once

#include "StringUtil.h"
#include "CtrlCmdDef.h"
#include "StructDef.h"
#include "ThreadUtil.h"

#include <functional>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

class CTCPServer
{
public:
	//応答が保留されているコマンドを再度呼ぶ(=NotifyUpdate()する)間隔
	static const DWORD NOTIFY_INTERVAL = 2000;
	//送受信タイムアウト
	static const DWORD SND_RCV_TIMEOUT = 30000;

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
	WSAEVENT m_hAcceptEvent;
	bool m_stopFlag;
	thread_ m_thread;

	SOCKET m_sock;
	
protected:
	static void SetBlockingMode(SOCKET sock);
	static void SetNonBlockingMode(SOCKET sock, WSAEVENT hEvent, long lNetworkEvents);
	static void ServerThread(CTCPServer* pSys);

};
