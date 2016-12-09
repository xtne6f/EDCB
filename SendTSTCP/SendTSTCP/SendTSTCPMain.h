#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <list>

#include "../../Common/StringUtil.h"
#pragma comment(lib, "Ws2_32.lib")

class CSendTSTCPMain
{
public:
	CSendTSTCPMain(void);
	~CSendTSTCPMain(void);

	//DLLの初期化
	//戻り値：TRUE:成功、FALSE:失敗
	BOOL Initialize(
		);

	//DLLの開放
	//戻り値：なし
	void UnInitialize(
		);

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
	HANDLE m_hStopSendEvent;
	HANDLE m_hSendThread;

	CRITICAL_SECTION m_sendLock;
	CRITICAL_SECTION m_buffLock;

	std::list<vector<BYTE>> m_TSBuff;

	typedef struct _SEND_INFO{
		string strIP;
		DWORD dwPort;
		SOCKET sock;
		BOOL bConnect;
	}SEND_INFO;
	map<wstring, SEND_INFO> m_SendList;

protected:
	static UINT WINAPI SendThread(LPVOID pParam);

};
