#include "StdAfx.h"
#include "SendTSTCPMain.h"
#include "../../Common/BlockLock.h"

#include <process.h>

//SendTSTCPプロトコルのヘッダの送信を抑制する既定のポート範囲
#define SEND_TS_TCP_NOHEAD_PORT_MIN 22000
#define SEND_TS_TCP_NOHEAD_PORT_MAX 22999

CSendTSTCPMain::CSendTSTCPMain(void)
{
	m_hStopSendEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hSendThread = NULL;
	m_hStopConnectEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hConnectThread = NULL;

	InitializeCriticalSection(&m_sendLock);
	InitializeCriticalSection(&m_buffLock);

	WSAData wsaData;
	WSAStartup(MAKEWORD(2,0), &wsaData);
}

CSendTSTCPMain::~CSendTSTCPMain(void)
{
	if( m_hConnectThread != NULL ){
		::SetEvent(m_hStopConnectEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(m_hConnectThread, 2000) == WAIT_TIMEOUT ){
			::TerminateThread(m_hConnectThread, 0xffffffff);
		}
		CloseHandle(m_hConnectThread);
		m_hConnectThread = NULL;
	}
	::CloseHandle(m_hStopConnectEvent);
	m_hStopConnectEvent = NULL;

	if( m_hSendThread != NULL ){
		::SetEvent(m_hStopSendEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(m_hSendThread, 2000) == WAIT_TIMEOUT ){
			::TerminateThread(m_hSendThread, 0xffffffff);
		}
		CloseHandle(m_hSendThread);
		m_hSendThread = NULL;
	}
	::CloseHandle(m_hStopSendEvent);
	m_hStopSendEvent = NULL;

	DeleteCriticalSection(&m_buffLock);
	DeleteCriticalSection(&m_sendLock);

	map<wstring, SEND_INFO>::iterator itr;
	for( itr = m_SendList.begin(); itr != m_SendList.end(); itr){
		if( itr->second.sock != INVALID_SOCKET ){
			closesocket(itr->second.sock);
		}
	}
	m_SendList.clear();

	WSACleanup();
}

//DLLの初期化
//戻り値：TRUE:成功、FALSE:失敗
BOOL CSendTSTCPMain::Initialize(
	)
{
	return TRUE;
}

//DLLの開放
//戻り値：なし
void CSendTSTCPMain::UnInitialize(
	)
{
	StopSend();
	ClearSendAddr();
	ClearSendBuff();
}

//送信先を追加
//戻り値：エラーコード
DWORD CSendTSTCPMain::AddSendAddr(
	LPCWSTR lpcwszIP,
	DWORD dwPort
	)
{
	if( lpcwszIP == NULL ){
		return FALSE;
	}
	SEND_INFO Item;
	WtoA(lpcwszIP, Item.strIP);
	Item.dwPort = dwPort;
	if( SEND_TS_TCP_NOHEAD_PORT_MIN <= dwPort && dwPort <= SEND_TS_TCP_NOHEAD_PORT_MAX ){
		//上位ワードが1のときはヘッダの送信が抑制される
		Item.dwPort |= 0x10000;
	}
	Item.sock = INVALID_SOCKET;
	Item.bConnect = FALSE;
	wstring strKey=L"";
	Format(strKey, L"%s:%d", lpcwszIP, dwPort);

	CBlockLock lock(&m_sendLock);
	m_SendList.insert(pair<wstring, SEND_INFO>(strKey, Item));

	return TRUE;

}

//送信先クリア
//戻り値：エラーコード
DWORD CSendTSTCPMain::ClearSendAddr(
	)
{
	CBlockLock lock(&m_sendLock);

	map<wstring, SEND_INFO>::iterator itr;
	for( itr = m_SendList.begin(); itr != m_SendList.end(); itr++){
		if( itr->second.sock != INVALID_SOCKET ){
			closesocket(itr->second.sock);
			itr->second.sock = INVALID_SOCKET;
			itr->second.bConnect = FALSE;
		}
	}
	m_SendList.clear();

	return TRUE;

}

//データ送信を開始
//戻り値：エラーコード
DWORD CSendTSTCPMain::StartSend(
	)
{
	if( m_hSendThread != NULL || m_hConnectThread != NULL){
		return FALSE;
	}

	ResetEvent(m_hStopConnectEvent);
	ResetEvent(m_hStopSendEvent);
	m_hConnectThread = (HANDLE)_beginthreadex(NULL, 0, ConnectThread, (LPVOID)this, CREATE_SUSPENDED, NULL);
	m_hSendThread = (HANDLE)_beginthreadex(NULL, 0, SendThread, (LPVOID)this, CREATE_SUSPENDED, NULL);
	ResumeThread(m_hConnectThread);
	ResumeThread(m_hSendThread);

	return TRUE;
}

//データ送信を停止
//戻り値：エラーコード
DWORD CSendTSTCPMain::StopSend(
	)
{
	if( m_hConnectThread != NULL ){
		::SetEvent(m_hStopConnectEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(m_hConnectThread, 5000) == WAIT_TIMEOUT ){
			::TerminateThread(m_hConnectThread, 0xffffffff);
		}
		CloseHandle(m_hConnectThread);
		m_hConnectThread = NULL;
	}

	if( m_hSendThread != NULL ){
		::SetEvent(m_hStopSendEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(m_hSendThread, 5000) == WAIT_TIMEOUT ){
			::TerminateThread(m_hSendThread, 0xffffffff);
		}
		CloseHandle(m_hSendThread);
		m_hSendThread = NULL;
	}

	CBlockLock lock(&m_sendLock);
	map<wstring, SEND_INFO>::iterator itr;
	for( itr = m_SendList.begin(); itr != m_SendList.end(); itr++){
		if( itr->second.sock != INVALID_SOCKET ){
			shutdown(itr->second.sock,SD_BOTH);
			closesocket(itr->second.sock);
			itr->second.sock = INVALID_SOCKET;
			itr->second.bConnect = FALSE;
		}
	}

	return TRUE;
}

//データ送信を開始
//戻り値：エラーコード
DWORD CSendTSTCPMain::AddSendData(
	BYTE* pbData,
	DWORD dwSize
	)
{

	if( m_hSendThread != NULL || m_hConnectThread != NULL){
		CBlockLock lock(&m_buffLock);
		m_TSBuff.push_back(vector<BYTE>());
		m_TSBuff.back().assign(pbData, pbData + dwSize);
		if( m_TSBuff.size() > 500 ){
			m_TSBuff.pop_front();
		}
	}
	return TRUE;
}

//送信バッファをクリア
//戻り値：エラーコード
DWORD CSendTSTCPMain::ClearSendBuff(
	)
{
	CBlockLock lock(&m_buffLock);
	m_TSBuff.clear();

	return TRUE;
}

UINT WINAPI CSendTSTCPMain::ConnectThread(LPVOID pParam)
{
	CSendTSTCPMain* pSys = (CSendTSTCPMain*)pParam;
	while(1){
		if( ::WaitForSingleObject(pSys->m_hStopConnectEvent, 500) != WAIT_TIMEOUT ){
			//キャンセルされた
			break;
		}

		CBlockLock lock(&pSys->m_sendLock);

		map<wstring, SEND_INFO>::iterator itr;
		for( itr = pSys->m_SendList.begin(); itr != pSys->m_SendList.end(); itr++){
			if( itr->second.bConnect == FALSE ){
				if( itr->second.sock != INVALID_SOCKET && itr->second.bConnect == FALSE ){
					fd_set rmask,wmask;
					FD_ZERO(&rmask);
					FD_SET(itr->second.sock,&rmask);
					wmask=rmask;
					struct timeval tv={0,0};
					int rc=select((int)itr->second.sock+1,&rmask, &wmask, NULL, &tv);
					if(rc==SOCKET_ERROR || rc == 0){
						closesocket(itr->second.sock);
						itr->second.sock = INVALID_SOCKET;
					}else{
						ULONG x = 0;
						ioctlsocket(itr->second.sock,FIONBIO, &x);
						itr->second.bConnect = TRUE;
					}
				}else{
					string strPort;
					Format(strPort, "%d", (WORD)itr->second.dwPort);
					struct addrinfo hints = {};
					hints.ai_flags = AI_NUMERICHOST;
					hints.ai_socktype = SOCK_STREAM;
					hints.ai_protocol = IPPROTO_TCP;
					struct addrinfo* result;
					if( getaddrinfo(itr->second.strIP.c_str(), strPort.c_str(), &hints, &result) != 0 ){
						continue;
					}
					itr->second.sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
					if( itr->second.sock == INVALID_SOCKET ){
						freeaddrinfo(result);
						continue;
					}
					ULONG x = 1;
					ioctlsocket(itr->second.sock,FIONBIO, &x);

					if( connect(itr->second.sock, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR ){
						if( WSAGetLastError() != WSAEWOULDBLOCK ){
							closesocket(itr->second.sock);
							itr->second.sock = INVALID_SOCKET;
						}
					}
					freeaddrinfo(result);
				}
			}
		}
	}
	return 0;
}

UINT WINAPI CSendTSTCPMain::SendThread(LPVOID pParam)
{
	CSendTSTCPMain* pSys = (CSendTSTCPMain*)pParam;
	DWORD dwWait = 0;
	DWORD dwCount = 0;
	while(1){
		if( ::WaitForSingleObject(pSys->m_hStopSendEvent, dwWait) != WAIT_TIMEOUT ){
			//キャンセルされた
			break;
		}

		vector<BYTE> buffSend;
		{
		CBlockLock lock(&pSys->m_buffLock);

		if( pSys->m_TSBuff.size() >= 2 ){
			std::list<vector<BYTE>>::iterator itrNext = pSys->m_TSBuff.begin();
			DWORD dwCmd[2] = {0,0};
			dwCmd[0]=dwCount;
			dwCmd[1]=(DWORD)(pSys->m_TSBuff.front().size() + (++itrNext)->size());
			buffSend.reserve(sizeof(dwCmd) + dwCmd[1]);
			buffSend.assign((BYTE*)dwCmd, (BYTE*)dwCmd + sizeof(dwCmd));
			buffSend.insert(buffSend.end(), pSys->m_TSBuff.front().begin(), pSys->m_TSBuff.front().end());
			buffSend.insert(buffSend.end(), itrNext->begin(), itrNext->end());

			pSys->m_TSBuff.pop_front();
			pSys->m_TSBuff.pop_front();
		}
		if( pSys->m_TSBuff.size() < 2 ){
			dwWait = 10;
		}
		//実際にやりたかったのはこっちかもしれない(挙動を変えるのは若干危険を感じるので保留)
		//dwWait = pSys->m_TSBuff.size() < 2 ? 10 : 0;

		} //m_buffLock
		if( buffSend.empty() == false ){
			CBlockLock lock(&pSys->m_sendLock);

			map<wstring, SEND_INFO>::iterator itr;
			for( itr = pSys->m_SendList.begin(); itr != pSys->m_SendList.end(); itr++){
				if( itr->second.bConnect == TRUE ){
					size_t adjust = HIWORD(itr->second.dwPort) == 1 ? buffSend.size() - sizeof(DWORD)*2 : buffSend.size();
					if( adjust > 0 && send(itr->second.sock, 
						(char*)&buffSend.front() + (buffSend.size() - adjust),
						(int)adjust,
						0
						) == INVALID_SOCKET){
							closesocket(itr->second.sock);
							itr->second.sock = INVALID_SOCKET;
							itr->second.bConnect = FALSE;
					}
					dwCount++;
				}
			}
		}
	}
	return 0;
}
