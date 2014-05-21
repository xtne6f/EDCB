#include "StdAfx.h"
#include "HttpServer.h"
#include <process.h>
#include "ErrDef.h"
#include "CtrlCmdUtil.h"

#define CONTENT_LENGTH_MAX	(1024 * 1024 * 10)

CHttpServer::CHttpServer(void)
{
	m_pHttpProc = NULL;
	m_pParam = NULL;
	m_dwPort = 8081;

	m_hStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hThread = NULL;

	m_sock = NULL;

	WSAData wsaData;
	WSAStartup(MAKEWORD(2,0), &wsaData);
}

CHttpServer::~CHttpServer(void)
{
	if( m_hThread != NULL ){
		::SetEvent(m_hStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(m_hThread, 2000) == WAIT_TIMEOUT ){
			::TerminateThread(m_hThread, 0xffffffff);
		}
		CloseHandle(m_hThread);
		m_hThread = NULL;
	}
	::CloseHandle(m_hStopEvent);
	m_hStopEvent = NULL;
	
	if( m_sock != NULL ){
		shutdown(m_sock,SD_RECEIVE);
		closesocket(m_sock);
		m_sock = NULL;
	}
	WSACleanup();
}

BOOL CHttpServer::StartServer(DWORD dwPort, HTTP_CALLBACK_PROC pfnHttpProc, void* pParam, int iThreadPriority, int iCtrlCmdEventID)
{
	if( pfnHttpProc == NULL || pParam == NULL ){
		return FALSE;
	}
	if( m_hThread != NULL ){
		return FALSE;
	}
	m_pHttpProc = pfnHttpProc;
	m_pParam = pParam;
	m_dwPort = dwPort;
	m_iThreadPriority = iThreadPriority;
	m_iCtrlCmdEventID = iCtrlCmdEventID;

	m_sock = socket(AF_INET, SOCK_STREAM, 0);
	if( m_sock == INVALID_SOCKET ){
		m_sock = NULL;
		return FALSE;
	}
	m_addr.sin_family = AF_INET;
	m_addr.sin_port = htons((WORD)dwPort);
	m_addr.sin_addr.S_un.S_addr = INADDR_ANY;
	BOOL b=1;

	setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&b, sizeof(b));
	DWORD socketBuffSize = 1024*1024;
	setsockopt(m_sock, SOL_SOCKET, SO_SNDBUF, (const char*)&socketBuffSize, sizeof(socketBuffSize));
	setsockopt(m_sock, SOL_SOCKET, SO_SNDBUF, (const char*)&socketBuffSize, sizeof(socketBuffSize));

	bind(m_sock, (struct sockaddr *)&m_addr, sizeof(m_addr));

	listen(m_sock, 1);

	ResetEvent(m_hStopEvent);
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, ServerThread, (LPVOID)this, CREATE_SUSPENDED, NULL);
	ResumeThread(m_hThread);

	return TRUE;
}

void CHttpServer::StopServer()
{
	if( m_hThread != NULL ){
		::SetEvent(m_hStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(m_hThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(m_hThread, 0xffffffff);
		}
		CloseHandle(m_hThread);
		m_hThread = NULL;
	}
	
	if( m_sock != NULL ){
		shutdown(m_sock,SD_RECEIVE);
		closesocket(m_sock);
		m_sock = NULL;
	}
}

UINT WINAPI CHttpServer::ServerThread(LPVOID pParam)
{
	CHttpServer* pSys = (CHttpServer*)pParam;

	HANDLE hCurThread = GetCurrentThread();
	SetThreadPriority(hCurThread, pSys->m_iThreadPriority);

	SOCKET sock = NULL;
	SOCKET sock2 = NULL;
	struct sockaddr_in client;
	
	HANDLE hEventCmdWait = NULL;
	if( pSys->m_iCtrlCmdEventID != -1 ){
		wstring strCmdEvent;
		Format(strCmdEvent, L"%s%d", CMD2_CTRL_EVENT_WAIT, pSys->m_iCtrlCmdEventID);
		hEventCmdWait = _CreateEvent(FALSE, TRUE, strCmdEvent.c_str());
	}
	
	fd_set ready;
	struct timeval to;

	while(1){
		if( WaitForSingleObject( pSys->m_hStopEvent, 0 ) != WAIT_TIMEOUT ){
			//中止
			break;
		}

		to.tv_sec = 1;
		to.tv_usec = 0;
		FD_ZERO(&ready);
		FD_SET(pSys->m_sock, &ready);

		if( select(0, &ready, NULL, NULL, &to ) == SOCKET_ERROR ){
			break;
		}
		if( sock == NULL ){
			if ( FD_ISSET(pSys->m_sock, &ready) ){
				int len = sizeof(client);
				sock = accept(pSys->m_sock, (struct sockaddr *)&client, &len);
				if (sock == INVALID_SOCKET) {
					closesocket(pSys->m_sock);
					pSys->m_sock = NULL;
					break;
				}
			}
		}
		if( sock != NULL ){
			//ほかのサーバーで処理中？
			if( hEventCmdWait != NULL ){
				WaitForSingleObject(hEventCmdWait, INFINITE);
			}
			DWORD socketBuffSize = 1024*1024;
			setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (const char*)&socketBuffSize, sizeof(socketBuffSize));
			setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (const char*)&socketBuffSize, sizeof(socketBuffSize));
			
			to.tv_sec = 1;
			to.tv_usec = 0;
			FD_ZERO(&ready);
			FD_SET(sock, &ready);
			if( select(0, &ready, NULL, NULL, &to ) == SOCKET_ERROR ){
				shutdown(sock,SD_RECEIVE);
				sock = NULL;
				continue;
			}
			if ( FD_ISSET(sock, &ready) ){
				HTTP_STREAM recvData;
				HTTP_STREAM sendData;
				string httpHeader = "";
				BOOL chkLine = FALSE;
				BOOL chkBody = FALSE;
				DWORD readBody = 0;
				while(1){
					int offset = 0;
					int iRet = 1;
					char recvBuff[1025] = {0};
					iRet = recv(sock, recvBuff, 1024, 0);
					if( iRet == SOCKET_ERROR ){
						break;
					}else if( iRet == 0 ){
						break;
					}
					if( chkLine == TRUE ){
						chkLine = FALSE;
						if(recvBuff[0] == '\n'){
							httpHeader += "\n";
							chkBody = TRUE;
							offset=1;
							recvData.dataSize = pSys->ChkContentLength(httpHeader);
							if( recvData.dataSize > 0 ){
								recvData.data = new BYTE[recvData.dataSize];
							}
						}else if(recvBuff[0] == '\r' && recvBuff[1] == '\n' ){
							httpHeader += "\r\n";
							chkBody = TRUE;
							offset=2;
							recvData.dataSize = pSys->ChkContentLength(httpHeader);
							if( recvData.dataSize > 0 ){
								recvData.data = new BYTE[recvData.dataSize];
							}
						}
					}
					if( chkBody == FALSE ){
						for( int i=0; i<iRet; i++ ){
							if( recvBuff[i] == '\n'){
								if( i+2 < iRet ){
									if(recvBuff[i+1] == '\r' && recvBuff[i+2] == '\n' ){
										char* buff = new char[i+3];
										ZeroMemory(buff, i+3);
										memcpy(buff, recvBuff, i+2);
										httpHeader+=buff;
										chkBody = TRUE;
										offset=i+3;
										recvData.dataSize = pSys->ChkContentLength(httpHeader);
										if( recvData.dataSize > 0 ){
											recvData.data = new BYTE[recvData.dataSize];
										}
										SAFE_DELETE_ARRAY(buff);
										break;
									}
								}else{
									chkLine = TRUE;
								}
							}
						}
						if( chkBody == FALSE ){
							httpHeader += recvBuff;
						}
					}
					if( chkBody == TRUE ){
						if( recvData.dataSize > 0 ){
							memcpy(recvData.data + readBody, recvBuff+offset, min(iRet-offset, (int)(recvData.dataSize-readBody)));
							readBody+=iRet-offset;
						}
						if( recvData.dataSize <= readBody ){
							recvData.httpHeader = httpHeader;
							break;
						}
					}
				}
				if( recvData.httpHeader.size() > 0 ){
					int iRet = 1;
					pSys->m_pHttpProc(pSys->m_pParam, &recvData, &sendData);
					iRet = send(sock, (char*)sendData.httpHeader.c_str(), (int)sendData.httpHeader.size(), 0);
					if( iRet != SOCKET_ERROR && sendData.dataSize > 0){
						iRet = send(sock, (char*)sendData.data, sendData.dataSize, 0);
					}
				}
			}
			shutdown(sock,SD_RECEIVE);
			closesocket(sock);
			sock = NULL;
			if( hEventCmdWait != NULL ){
				SetEvent(hEventCmdWait);
			}
		}
	}

	if( sock != NULL ){
		shutdown(sock,SD_RECEIVE);
		closesocket(sock);
	}
	if( hEventCmdWait != NULL ){
		CloseHandle(hEventCmdWait);
	}
	
	return 0;
}

DWORD CHttpServer::ChkContentLength(string httpHeader)
{
	DWORD ret = 0;
	if( httpHeader.find("Content-Length: ") != string::npos){
		int pos1 = (int)httpHeader.find("Content-Length: ");
		ret = atoi(httpHeader.c_str()+pos1+16);
	}
	return min(ret, CONTENT_LENGTH_MAX);
}

