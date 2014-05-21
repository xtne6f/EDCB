#include "StdAfx.h"
#include "SendTSTCPMain.h"

#include <process.h>

CSendTSTCPMain::CSendTSTCPMain(void)
{
	m_hStopSendEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hSendThread = NULL;
	m_hStopConnectEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hConnectThread = NULL;

	m_hEvent = CreateEvent(NULL, FALSE, TRUE, NULL );

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

	Lock();
	if( m_hEvent != NULL ){
		CloseHandle(m_hEvent);
		m_hEvent = NULL;
	}

	map<wstring, SEND_INFO>::iterator itr;
	for( itr = m_SendList.begin(); itr != m_SendList.end(); itr){
		if( itr->second.sock != INVALID_SOCKET ){
			closesocket(itr->second.sock);
		}
	}
	m_SendList.clear();

	for( int i=0; i<(int)m_TSBuff.size(); i++ ){
		SAFE_DELETE(m_TSBuff[i])
	}
	m_TSBuff.clear();

	WSACleanup();
}

BOOL CSendTSTCPMain::Lock()
{
	if( m_hEvent == NULL ){
		return FALSE;
	}
	WaitForSingleObject(m_hEvent, INFINITE);
	return TRUE;
}

void CSendTSTCPMain::UnLock()
{
	if( m_hEvent != NULL ){
		SetEvent(m_hEvent);
	}
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
	Item.strIP = lpcwszIP;
	string strA = "";
	WtoA(Item.strIP, strA);
	Item.dwIP = inet_addr(strA.c_str());
	Item.dwPort = dwPort;
	Item.sock = INVALID_SOCKET;
	Item.bConnect = FALSE;
	wstring strKey=L"";
	Format(strKey, L"%s:%d", lpcwszIP, dwPort);

	if( Lock() == FALSE ) return FALSE;
	m_SendList.insert(pair<wstring, SEND_INFO>(strKey, Item));
	UnLock();

	return TRUE;

}

//送信先クリア
//戻り値：エラーコード
DWORD CSendTSTCPMain::ClearSendAddr(
	)
{
	if( Lock() == FALSE ) return FALSE;

	map<wstring, SEND_INFO>::iterator itr;
	for( itr = m_SendList.begin(); itr != m_SendList.end(); itr++){
		if( itr->second.sock != INVALID_SOCKET ){
			closesocket(itr->second.sock);
			itr->second.sock = INVALID_SOCKET;
			itr->second.bConnect = FALSE;
		}
	}
	m_SendList.clear();

	UnLock();
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

	if( Lock() == FALSE ) return FALSE;
	map<wstring, SEND_INFO>::iterator itr;
	for( itr = m_SendList.begin(); itr != m_SendList.end(); itr++){
		if( itr->second.sock != INVALID_SOCKET ){
			shutdown(itr->second.sock,SD_BOTH);
			closesocket(itr->second.sock);
			itr->second.sock = INVALID_SOCKET;
			itr->second.bConnect = FALSE;
		}
	}
	UnLock();

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
		if( Lock() == FALSE ) return FALSE;
		TS_DATA* pItem = new TS_DATA;
		pItem->pbBuff = new BYTE[dwSize];
		ZeroMemory( pItem->pbBuff, dwSize );
		memcpy( pItem->pbBuff, pbData, dwSize );
		pItem->dwSize = dwSize;

		m_TSBuff.push_back(pItem);
		UnLock();
	}
	return TRUE;
}

//送信バッファをクリア
//戻り値：エラーコード
DWORD CSendTSTCPMain::ClearSendBuff(
	)
{
	if( Lock() == FALSE ) return FALSE;
	for( int i=0; i<(int)m_TSBuff.size(); i++ ){
		SAFE_DELETE(m_TSBuff[i])
	}
	m_TSBuff.clear();
	UnLock();

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

		if( pSys->Lock() == FALSE ) return FALSE;

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
					itr->second.sock = socket(AF_INET, SOCK_STREAM, 0);
					if( itr->second.sock == INVALID_SOCKET ){
						continue;
					}
					ULONG x = 1;
					ioctlsocket(itr->second.sock,FIONBIO, &x);

					itr->second.addr.sin_family = AF_INET;
					itr->second.addr.sin_port = htons((WORD)itr->second.dwPort);
					itr->second.addr.sin_addr.S_un.S_addr = itr->second.dwIP;

					if( connect(itr->second.sock, (struct sockaddr *)&itr->second.addr, sizeof(itr->second.addr)) == SOCKET_ERROR ){
						if( WSAGetLastError() != WSAEWOULDBLOCK ){
							closesocket(itr->second.sock);
							itr->second.sock = INVALID_SOCKET;
						}
					}
				}
			}
		}

		pSys->UnLock();
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

		if( pSys->Lock() == FALSE ) return FALSE;

		if(pSys->m_TSBuff.size()>500){
			for( int i=(int)pSys->m_TSBuff.size()-500; i>=0; i-- ){
				SAFE_DELETE(pSys->m_TSBuff[i]);
				vector<TS_DATA*>::iterator itr;
				itr = pSys->m_TSBuff.begin();
				advance(itr,i);
				pSys->m_TSBuff.erase( itr );
			}
		}

		DWORD dwSend = 0;
		BYTE* pbSend = NULL;
		DWORD dwCmd[2] = {0,0};
/*
		if( pSys->m_TSBuff.size() >= 3 ){

			dwSend = sizeof(DWORD)*2 + pSys->m_TSBuff[0]->dwSize + pSys->m_TSBuff[1]->dwSize + pSys->m_TSBuff[2]->dwSize;
			pbSend = new BYTE[dwSend];
			DWORD dwCmd[2] = {0,0};
			dwCmd[0]=dwCount;
			dwCmd[1]=pSys->m_TSBuff[0]->dwSize + pSys->m_TSBuff[1]->dwSize + pSys->m_TSBuff[2]->dwSize;
			memcpy(pbSend, (BYTE*)&dwCmd, sizeof(DWORD)*2);
			memcpy(pbSend+sizeof(DWORD)*2, pSys->m_TSBuff[0]->pbBuff, pSys->m_TSBuff[0]->dwSize);
			memcpy(pbSend+sizeof(DWORD)*2+pSys->m_TSBuff[0]->dwSize, pSys->m_TSBuff[1]->pbBuff, pSys->m_TSBuff[1]->dwSize);
			memcpy(pbSend+sizeof(DWORD)*2+pSys->m_TSBuff[0]->dwSize+pSys->m_TSBuff[1]->dwSize, pSys->m_TSBuff[2]->pbBuff, pSys->m_TSBuff[2]->dwSize);

			SAFE_DELETE(pSys->m_TSBuff[0]);
			SAFE_DELETE(pSys->m_TSBuff[1]);
			SAFE_DELETE(pSys->m_TSBuff[2]);
			pSys->m_TSBuff.erase( pSys->m_TSBuff.begin() );
			pSys->m_TSBuff.erase( pSys->m_TSBuff.begin() );
			pSys->m_TSBuff.erase( pSys->m_TSBuff.begin() );
		}
*/

		if( pSys->m_TSBuff.size() >= 2 ){

			dwSend = sizeof(DWORD)*2 + pSys->m_TSBuff[0]->dwSize + pSys->m_TSBuff[1]->dwSize;
			pbSend = new BYTE[dwSend];
			DWORD dwCmd[2] = {0,0};
			dwCmd[0]=dwCount;
			dwCmd[1]=pSys->m_TSBuff[0]->dwSize + pSys->m_TSBuff[1]->dwSize;
			memcpy(pbSend, (BYTE*)&dwCmd, sizeof(DWORD)*2);
			memcpy(pbSend+sizeof(DWORD)*2, pSys->m_TSBuff[0]->pbBuff, pSys->m_TSBuff[0]->dwSize);
			memcpy(pbSend+sizeof(DWORD)*2+pSys->m_TSBuff[0]->dwSize, pSys->m_TSBuff[1]->pbBuff, pSys->m_TSBuff[1]->dwSize);

			SAFE_DELETE(pSys->m_TSBuff[0]);
			SAFE_DELETE(pSys->m_TSBuff[1]);
			pSys->m_TSBuff.erase( pSys->m_TSBuff.begin() );
			pSys->m_TSBuff.erase( pSys->m_TSBuff.begin() );
		}

		pSys->UnLock();
		if( pbSend != NULL && pSys->m_SendList.size() > 0 ){
			if( pSys->Lock() == FALSE ) return FALSE;

			map<wstring, SEND_INFO>::iterator itr;
			for( itr = pSys->m_SendList.begin(); itr != pSys->m_SendList.end(); itr++){
				if( itr->second.bConnect == TRUE ){
					if( sendto(itr->second.sock, 
						(char*)pbSend,
						dwSend,
						0,
						(struct sockaddr *)&itr->second.addr,
						sizeof(itr->second.addr)
						) == INVALID_SOCKET){
							closesocket(itr->second.sock);
							itr->second.sock = NULL;
							itr->second.bConnect = FALSE;
					}
					dwCount++;
				}
			}

			pSys->UnLock();
		}
		if( pbSend != NULL ){
			delete[] pbSend;
		}
		if( pSys->m_TSBuff.size() == 0 ){
			dwWait = 10;
		}
	}
	return 0;
}
