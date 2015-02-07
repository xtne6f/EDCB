#include "StdAfx.h"
#include "TCPServer.h"
#include <process.h>
#include "ErrDef.h"
#include "CtrlCmdUtil.h"

CTCPServer::CTCPServer(void)
{
	m_pCmdProc = NULL;
	m_pParam = NULL;
	m_dwPort = 8081;

	m_hStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hThread = NULL;

	m_sock = NULL;

	WSAData wsaData;
	WSAStartup(MAKEWORD(2,0), &wsaData);
}

CTCPServer::~CTCPServer(void)
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

BOOL CTCPServer::StartServer(DWORD dwPort, CMD_CALLBACK_PROC pfnCmdProc, void* pParam, int iThreadPriority, int iCtrlCmdEventID)
{
	if( pfnCmdProc == NULL || pParam == NULL ){
		return FALSE;
	}
	if( m_hThread != NULL ){
		return FALSE;
	}
	m_pCmdProc = pfnCmdProc;
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

void CTCPServer::StopServer()
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

UINT WINAPI CTCPServer::ServerThread(LPVOID pParam)
{
	CTCPServer* pSys = (CTCPServer*)pParam;

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
				CMD_STREAM stCmd;
				CMD_STREAM stRes;
				DWORD dwRead = 0;
				DWORD dwWrite = 0;
				DWORD head[2];
				do{
					int iRet = 1;
					iRet = recv(sock, (char*)head, sizeof(DWORD)*2, 0);
					if( iRet == SOCKET_ERROR ){
						break;
					}else if( iRet == 0 ){
						break;
					}
					stCmd.param = head[0];
					stCmd.dataSize = head[1];

					// 明らかに不正なパケットを除外
					// 本来はparamもチェックすべきなんだろうけど量が多すぎて断念
					if (stCmd.dataSize > 1024){
						break;
					}

					if( stCmd.dataSize > 0 ){
						stCmd.data = new BYTE[stCmd.dataSize];

						while(iRet>0){
							iRet = recv(sock, (char*)(stCmd.data+dwRead), stCmd.dataSize-dwRead, 0);
							if( iRet == SOCKET_ERROR ){
								break;
							}else if( iRet == 0 ){
								break;
							}
							dwRead+=iRet;
							if( dwRead == stCmd.dataSize ){
								break;
							}
						}
					}

					if( stCmd.param == CMD2_EPG_SRV_REGIST_GUI_TCP || stCmd.param == CMD2_EPG_SRV_UNREGIST_GUI_TCP ){
						string ip = inet_ntoa(client.sin_addr);

						REGIST_TCP_INFO setParam;
						AtoW(ip, setParam.ip);
						ReadVALUE(&setParam.port, stCmd.data, stCmd.dataSize, NULL);

						SAFE_DELETE_ARRAY(stCmd.data);
						stCmd.dataSize = GetVALUESize(&setParam);
						stCmd.data = new BYTE[stCmd.dataSize];
						WriteVALUE(&setParam, stCmd.data, stCmd.dataSize, NULL);
					}

					pSys->m_pCmdProc(pSys->m_pParam, &stCmd, &stRes);
					if( stRes.param == CMD_NO_RES ){
						break;
					}
					head[0] = stRes.param;
					head[1] = stRes.dataSize;

					iRet = send(sock, (char*)head, sizeof(DWORD)*2, 0);
					if( iRet == SOCKET_ERROR ){
						break;
					}
					if( stRes.dataSize > 0 ){
						if( stRes.data == NULL ){
							break;
						}
						iRet = send(sock, (char*)(stRes.data), stRes.dataSize, 0);
						if( iRet == SOCKET_ERROR ){
							break;
						}
					}

					SAFE_DELETE_ARRAY(stCmd.data);
					SAFE_DELETE_ARRAY(stRes.data);
					stCmd.dataSize = 0;
					stRes.dataSize = 0;
				}while(stRes.param == CMD_NEXT || stRes.param == OLD_CMD_NEXT); //Emun用の繰り返し
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
