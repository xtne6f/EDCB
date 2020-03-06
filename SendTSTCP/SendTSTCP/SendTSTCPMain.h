#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <list>

#include "../../Common/StringUtil.h"
#include "../../Common/ThreadUtil.h"
#pragma comment(lib, "Ws2_32.lib")

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

	std::list<vector<BYTE>> m_TSBuff;

	struct SEND_INFO {
		string strIP;
		DWORD dwPort;
		SOCKET sock;
		HANDLE pipe;
		HANDLE olEvent;
		OVERLAPPED ol;
		BOOL bConnect;
	};
	std::list<SEND_INFO> m_SendList;

protected:
	static void SendThread(CSendTSTCPMain* pSys);

};
