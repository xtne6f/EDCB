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
	bool m_bSendingToSomeone;

	struct SEND_INFO {
		string strIP; //空のときUDP
		struct sockaddr_storage udpAddr;
		size_t udpAddrlen;
		int udpMaxSendSize;
		WORD port;
		bool bSuppressHeader;
		SOCKET sock;
#ifdef _WIN32
		HANDLE pipe[2];
		HANDLE olEvent[2];
		OVERLAPPED ol[2];
		bool bPipeWriting[2];
#else
		int pipe[2];
		string strPipe[2];
		DWORD wroteBytes[2];
#endif
		bool bConnect[2];
		DWORD writeAheadCount[2];
	};
	std::list<SEND_INFO> m_SendList;

protected:
	static void SendThread(CSendTSTCPMain* pSys);

};
