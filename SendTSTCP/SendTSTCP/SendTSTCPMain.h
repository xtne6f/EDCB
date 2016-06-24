#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>

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
	HANDLE m_hStopConnectEvent;
	HANDLE m_hConnectThread;
	HANDLE m_hStopSendEvent;
	HANDLE m_hSendThread;

	CRITICAL_SECTION m_sendLock;
	CRITICAL_SECTION m_buffLock;

	typedef struct _TS_DATA{
		BYTE* pbBuff;
		DWORD dwSize;
		_TS_DATA(void){
			pbBuff = NULL;
			dwSize = 0;
		}
		~_TS_DATA(void){
			SAFE_DELETE_ARRAY(pbBuff);
		}
	}TS_DATA;
	vector<TS_DATA*> m_TSBuff;

	typedef struct _SEND_INFO{
		string strIP;
		DWORD dwPort;
		SOCKET sock;
		BOOL bConnect;
	}SEND_INFO;
	map<wstring, SEND_INFO> m_SendList;

protected:
	static UINT WINAPI SendThread(LPVOID pParam);
	static UINT WINAPI ConnectThread(LPVOID pParam);

};
