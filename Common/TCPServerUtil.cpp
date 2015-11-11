#include "StdAfx.h"
#include "TCPServerUtil.h"
#include <process.h>

CTCPServerUtil::CTCPServerUtil(void)
{
	this->lockEvent = CreateEvent(NULL, FALSE, TRUE, NULL);

	this->tcpAcceptProc = NULL;
	this->funcParam = NULL;
	this->port = 8081;

	this->stopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	this->serverThread = NULL;

	this->srvSock = INVALID_SOCKET;

	WSAData wsaData;
	WSAStartup(MAKEWORD(2,0), &wsaData);
}

CTCPServerUtil::~CTCPServerUtil(void)
{
	if( this->srvSock != INVALID_SOCKET ){
		shutdown(this->srvSock,SD_BOTH);
		closesocket(this->srvSock);
		this->srvSock = INVALID_SOCKET;
	}

	map<HANDLE, TCP_ACCEPT_PARAM*>::iterator itr;
	itr = acceptThreadMap.begin();
	while(itr != acceptThreadMap.end() )
	{
		if( WaitForSingleObject(itr->second->thread, 0) == WAIT_OBJECT_0 ){
			CloseHandle(itr->second->stopEvent);
			CloseHandle(itr->second->thread);
		}else{
			::SetEvent(itr->second->stopEvent);
			// スレッド終了待ち
			if ( ::WaitForSingleObject(itr->second->thread, 15000) == WAIT_TIMEOUT ){
				::TerminateThread(itr->second->thread, 0xffffffff);
			}
		}
		SAFE_DELETE(itr->second);
		itr++;
	}
	acceptThreadMap.clear();

	if( this->serverThread != NULL ){
		::SetEvent(this->stopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->serverThread, 2000) == WAIT_TIMEOUT ){
			::TerminateThread(this->serverThread, 0xffffffff);
		}
		CloseHandle(this->serverThread);
		this->serverThread = NULL;
	}
	::CloseHandle(this->stopEvent);
	this->stopEvent = NULL;

	if( this->lockEvent != NULL ){
		UnLock();
		CloseHandle(this->lockEvent);
		this->lockEvent = NULL;
	}

	WSACleanup();
}

BOOL CTCPServerUtil::Lock(LPCWSTR log, DWORD timeOut)
{
	if( this->lockEvent == NULL ){
		return FALSE;
	}
	//if( log != NULL ){
	//	_OutputDebugString(L"◆%s",log);
	//}
	DWORD dwRet = WaitForSingleObject(this->lockEvent, timeOut);
	if( dwRet == WAIT_ABANDONED || 
		dwRet == WAIT_FAILED ||
		dwRet == WAIT_TIMEOUT){
			_OutputDebugString(L"◆CTCPServerUtil::Lock FALSE (last lock : %s)", lastLockLog.c_str());
			if( log != NULL ){
				OutputDebugString(log);
			}
		return FALSE;
	}
	if( log != NULL ){
		lastLockLog = log;
	}else{
		lastLockLog = L"";
	}
	
	return TRUE;
}

void CTCPServerUtil::UnLock(LPCWSTR log)
{
	if( this->lockEvent != NULL ){
		SetEvent(this->lockEvent);
	}
	if( log != NULL ){
		OutputDebugString(log);
	}
}

BOOL CTCPServerUtil::StartServer(DWORD port, TCP_ACCEPT_CALLBACK_PROC tcpAcceptProc, void* funcParam, int threadPriority)
{
	if( tcpAcceptProc == NULL ){
		return FALSE;
	}
	if( this->serverThread != NULL ){
		return FALSE;
	}
	this->tcpAcceptProc = tcpAcceptProc;
	this->funcParam = funcParam;
	this->port = port;
	this->threadPriority = threadPriority;

	this->srvSock = socket(AF_INET, SOCK_STREAM, 0);
	if( this->srvSock == INVALID_SOCKET ){
		return FALSE;
	}
	this->srvAddr.sin_family = AF_INET;
	this->srvAddr.sin_port = htons((WORD)port);
	this->srvAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	BOOL b=1;

	setsockopt(this->srvSock, SOL_SOCKET, SO_REUSEADDR, (const char *)&b, sizeof(b));
	DWORD socketBuffSize = 1024*1024;
	setsockopt(this->srvSock, SOL_SOCKET, SO_SNDBUF, (const char*)&socketBuffSize, sizeof(socketBuffSize));

	if( bind(this->srvSock, (struct sockaddr *)&this->srvAddr, sizeof(this->srvAddr)) == SOCKET_ERROR){
		return FALSE;
	}

	if( listen(this->srvSock, 0) == SOCKET_ERROR){
		return FALSE;
	}

	ResetEvent(this->stopEvent);
	this->serverThread = (HANDLE)_beginthreadex(NULL, 0, ServerThread, (LPVOID)this, CREATE_SUSPENDED, NULL);
	ResumeThread(this->serverThread);

	return TRUE;
}

void CTCPServerUtil::StopServer()
{
	if( this->srvSock != INVALID_SOCKET ){
		shutdown(this->srvSock,SD_BOTH);
		closesocket(this->srvSock);
		this->srvSock = INVALID_SOCKET;
	}

	map<HANDLE, TCP_ACCEPT_PARAM*>::iterator itr;
	itr = acceptThreadMap.begin();
	while(itr != acceptThreadMap.end() )
	{
		if( WaitForSingleObject(itr->second->thread, 0) == WAIT_OBJECT_0 ){
			CloseHandle(itr->second->stopEvent);
			CloseHandle(itr->second->thread);
		}else{
			::SetEvent(itr->second->stopEvent);
			// スレッド終了待ち
			if ( ::WaitForSingleObject(itr->second->thread, 15000) == WAIT_TIMEOUT ){
				::TerminateThread(itr->second->thread, 0xffffffff);
			}
		}
		SAFE_DELETE(itr->second);
		itr++;
	}
	acceptThreadMap.clear();

	if( this->serverThread != NULL ){
		::SetEvent(this->stopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->serverThread, 2000) == WAIT_TIMEOUT ){
			::TerminateThread(this->serverThread, 0xffffffff);
		}
		CloseHandle(this->serverThread);
		this->serverThread = NULL;
	}
}

UINT WINAPI CTCPServerUtil::ServerThread(LPVOID pParam)
{
	CTCPServerUtil* pSys = (CTCPServerUtil*)pParam;

	HANDLE curThread = GetCurrentThread();
	SetThreadPriority(curThread, pSys->threadPriority);

	struct sockaddr_in client;
	
	fd_set ready;
	struct timeval to;

	int result = 0;

	while(1){
		if( WaitForSingleObject( pSys->stopEvent, 0 ) != WAIT_TIMEOUT ){
			//中止
			break;
		}

		to.tv_sec = 0;
		to.tv_usec = 500*1000;
		FD_ZERO(&ready);
		FD_SET(pSys->srvSock, &ready);

		result = select(0, &ready, NULL, NULL, &to );
		if( result == SOCKET_ERROR ){
			break;
		}else if( result == 0 ){
			pSys->Lock();
			map<HANDLE, TCP_ACCEPT_PARAM*>::iterator itr;
			itr = pSys->acceptThreadMap.begin();
			while(itr != pSys->acceptThreadMap.end() )
			{
				if( WaitForSingleObject(itr->second->thread, 0) == WAIT_OBJECT_0 ){
					CloseHandle(itr->second->stopEvent);
					CloseHandle(itr->second->thread);
					SAFE_DELETE(itr->second);
					pSys->acceptThreadMap.erase(itr++);
				}else{
					itr++;
				}
			}
			pSys->UnLock();

			continue;
		}
		if ( FD_ISSET(pSys->srvSock, &ready) ){
			int len = sizeof(client);
			SOCKET sock = accept(pSys->srvSock, (struct sockaddr *)&client, &len);
			if (sock == INVALID_SOCKET) {
				continue;
			}else{
				string aa;
				Format(aa, "accept  %s:%u\r\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
				//OutputDebugStringA(aa.c_str());

				pSys->Lock();
				TCP_ACCEPT_PARAM* param = new TCP_ACCEPT_PARAM;
				param->param = pSys;
				param->sock = sock;
				param->client = client;
				param->stopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

				param->thread = (HANDLE)_beginthreadex(NULL, 0, AcceptThread, (LPVOID)param, CREATE_SUSPENDED, NULL);
				pSys->acceptThreadMap.insert(pair<HANDLE,TCP_ACCEPT_PARAM*>(param->thread, param));
				ResumeThread(param->thread);
				pSys->UnLock();
			}
		}

	}
	
	return 0;
}

UINT WINAPI CTCPServerUtil::AcceptThread(LPVOID pParam)
{
	TCP_ACCEPT_PARAM* pTemp = (TCP_ACCEPT_PARAM*)pParam;
	CTCPServerUtil* pSys = (CTCPServerUtil*)pTemp->param;

	pSys->tcpAcceptProc(pSys->funcParam, pTemp->sock, &pTemp->client, pTemp->stopEvent);
	
//Err_End:
	shutdown(pTemp->sock,SD_SEND);
	closesocket(pTemp->sock);
	pTemp->sock = INVALID_SOCKET;

	return 0;
}
