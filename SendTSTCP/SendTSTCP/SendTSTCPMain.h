#pragma once

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#endif
#include <list>

#include "../../Common/ThreadUtil.h"

class CSendTSTCPMain
{
public:
	CSendTSTCPMain(void);
	~CSendTSTCPMain(void);

	//送信先を追加
	//戻り値：エラーコード
	DWORD AddSendAddr(
		LPCWSTR lpcwszIP,
		DWORD dwPort
		);

	//送信先を追加(UDP)
	//戻り値：エラーコード
	DWORD AddSendAddrUdp(
		LPCWSTR lpcwszIP,
		DWORD dwPort,
		BOOL broadcastFlag,
		int maxSendSize
		);

	//送信先クリア
	//戻り値：エラーコード
	DWORD ClearSendAddr(
		);

	//データ送信を開始
	//戻り値：エラーコード
	DWORD StartSend(
		);

	//データ送信を停止
	//戻り値：エラーコード
	DWORD StopSend(
		);

	//データ送信を開始
	//戻り値：エラーコード
	DWORD AddSendData(
		BYTE* pbData,
		DWORD dwSize
		);

	//送信バッファをクリア
	//戻り値：エラーコード
	DWORD ClearSendBuff(
		);


protected:
	CAutoResetEvent m_stopSendEvent;
	thread_ m_sendThread;
	recursive_mutex_ m_sendLock;
#ifdef _WIN32
	int m_wsaStartupResult;
#endif
	std::list<vector<BYTE>> m_TSBuff;

	struct SEND_INFO {
		string strIP;
		WORD port;
		bool bSuppressHeader;
#ifdef _WIN32
		SOCKET sock;
		HANDLE pipe[2];
		HANDLE olEvent[2];
		OVERLAPPED ol[2];
		bool bPipeWriting[2];
#else
		int sock;
		int pipe[2];
		string strPipe[2];
		DWORD wroteBytes[2];
#endif
		bool bConnect[2];
		DWORD writeAheadCount[2];
	};
	std::list<SEND_INFO> m_SendList;

	struct SOCKET_DATA {
#ifdef _WIN32
		SOCKET sock;
#else
		int sock;
#endif
		struct sockaddr_storage addr;
		size_t addrlen;
		int maxSendSize;
	};
	vector<SOCKET_DATA> m_udpSockList;

protected:
	static void SendThread(CSendTSTCPMain* pSys);

};
