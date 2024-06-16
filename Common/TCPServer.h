#pragma once

#include "CtrlCmdUtil.h"
#include "ThreadUtil.h"

#include <functional>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

class CTCPServer
{
public:
	//応答が保留されているコマンドを再度呼ぶ(=NotifyUpdate()する)間隔
	static const DWORD NOTIFY_INTERVAL = 2000;
	//送受信タイムアウト
	static const DWORD SND_RCV_TIMEOUT = 30000;
	//応答用スレッドの最大数
	static const DWORD RESPONSE_THREADS_MAX = 20;

	enum RESPONSE_THREAD_STATE {
		RESPONSE_THREAD_INIT,
		RESPONSE_THREAD_PROC,
		RESPONSE_THREAD_FIN,
	};

	CTCPServer(void);
	~CTCPServer(void);

	bool StartServer(unsigned short port, bool ipv6, DWORD dwResponseTimeout, LPCWSTR acl,
	                 const std::function<void(const CCmdStream&, CCmdStream&, LPCWSTR)>& cmdProc,
	                 const std::function<void(const CCmdStream&, CCmdStream&, RESPONSE_THREAD_STATE, void*&)>& responseThreadProc);
	void StopServer();
	void NotifyUpdate();

protected:
	std::function<void(const CCmdStream&, CCmdStream&, LPCWSTR)> m_cmdProc;
	std::function<void(const CCmdStream&, CCmdStream&, RESPONSE_THREAD_STATE, void*&)> m_responseThreadProc;
	unsigned short m_port;
	bool m_ipv6;
	DWORD m_dwResponseTimeout;
	wstring m_acl;

	CAutoResetEvent m_notifyEvent;
	atomic_bool_ m_stopFlag;
	thread_ m_thread;
#ifdef _WIN32
	WSAEVENT m_hAcceptEvent;
#endif
	SOCKET m_sock;

	struct RESPONSE_THREAD_INFO {
		thread_ th;
		atomic_bool_ completed;
		SOCKET sock;
		CCmdStream cmd;
		CTCPServer* sys;
	};

	static void ServerThread(CTCPServer* pSys);
	static void ResponseThread(RESPONSE_THREAD_INFO* info);
};
