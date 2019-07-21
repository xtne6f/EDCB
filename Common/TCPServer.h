#pragma once

#include "StructDef.h"
#include "ThreadUtil.h"

#include <functional>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#endif

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
	                 const std::function<void(CMD_STREAM*, CMD_STREAM*, LPCWSTR)>& cmdProc);
	void StopServer();
	void NotifyUpdate();

protected:
	std::function<void(CMD_STREAM*, CMD_STREAM*, LPCWSTR)> m_cmdProc;
	unsigned short m_port;
	bool m_ipv6;
	DWORD m_dwResponseTimeout;
	wstring m_acl;

	CAutoResetEvent m_notifyEvent;
	atomic_bool_ m_stopFlag;
	thread_ m_thread;
#ifdef _WIN32
	WSAEVENT m_hAcceptEvent;
	SOCKET m_sock;
#else
	int m_sock;
#endif

	static void ServerThread(CTCPServer* pSys);

};
