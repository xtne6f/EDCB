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

	m_hNotifyEvent = WSA_INVALID_EVENT;
	m_stopFlag = FALSE;
	m_hThread = NULL;

	m_sock = INVALID_SOCKET;

	WSAData wsaData;
	WSAStartup(MAKEWORD(2,0), &wsaData);
}

CTCPServer::~CTCPServer(void)
{
	StopServer();
	WSACleanup();
}

BOOL CTCPServer::StartServer(DWORD dwPort, DWORD dwResponseTimeout, LPCWSTR acl, CMD_CALLBACK_PROC pfnCmdProc, void* pParam)
{
	if( pfnCmdProc == NULL || pParam == NULL ){
		return FALSE;
	}
	if( m_hThread != NULL &&
	    m_pCmdProc == pfnCmdProc &&
	    m_pParam == pParam &&
	    m_dwPort == dwPort &&
	    m_dwResponseTimeout == dwResponseTimeout &&
	    m_acl == acl ){
		return TRUE;
	}
	StopServer();
	m_pCmdProc = pfnCmdProc;
	m_pParam = pParam;
	m_dwPort = dwPort;
	m_dwResponseTimeout = dwResponseTimeout;
	m_acl = acl;

	m_sock = socket(AF_INET, SOCK_STREAM, 0);
	if( m_sock == INVALID_SOCKET ){
		return FALSE;
	}
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons((WORD)dwPort);
	addr.sin_addr.S_un.S_addr = INADDR_ANY;
	BOOL b=1;

	setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&b, sizeof(b));

	m_stopFlag = FALSE;
	if( bind(m_sock, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR ||
	    listen(m_sock, 1) == SOCKET_ERROR ||
	    (m_hNotifyEvent = WSACreateEvent()) == WSA_INVALID_EVENT ||
	    (m_hThread = (HANDLE)_beginthreadex(NULL, 0, ServerThread, this, 0, NULL)) == NULL ){
		StopServer();
		return FALSE;
	}

	return TRUE;
}

void CTCPServer::StopServer()
{
	if( m_hThread != NULL ){
		m_stopFlag = TRUE;
		WSASetEvent(m_hNotifyEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(m_hThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(m_hThread, 0xffffffff);
		}
		CloseHandle(m_hThread);
		m_hThread = NULL;
	}
	if( m_hNotifyEvent != WSA_INVALID_EVENT ){
		WSACloseEvent(m_hNotifyEvent);
		m_hNotifyEvent = WSA_INVALID_EVENT;
	}
	
	if( m_sock != INVALID_SOCKET ){
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
	}
}

void CTCPServer::NotifyUpdate()
{
	if( m_hThread != NULL ){
		WSASetEvent(m_hNotifyEvent);
	}
}

static BOOL TestAcl(struct in_addr addr, wstring acl)
{
	//書式例: +192.168.0.0/16,-192.168.0.1
	BOOL ret = FALSE;
	for(;;){
		wstring val;
		BOOL sep = Separate(acl, L",", val, acl);
		if( val.empty() || val[0] != L'+' && val[0] != L'-' ){
			//書式エラー
			return FALSE;
		}
		wstring a, b, c, d, m;
		Separate(val.substr(1), L".", a, b);
		Separate(b, L".", b, c);
		Separate(c, L".", c, d);
		ULONG mask = Separate(d, L"/", d, m) ? _wtoi(m.c_str()) : 32;
		if( a.empty() || b.empty() || c.empty() || d.empty() || mask > 32 ){
			//書式エラー
			return FALSE;
		}
		mask = mask == 0 ? 0 : 0xFFFFFFFFUL << (32 - mask);
		ULONG host = (ULONG)_wtoi(a.c_str()) << 24 | _wtoi(b.c_str()) << 16 | _wtoi(c.c_str()) << 8 | _wtoi(d.c_str());
		if( (ntohl(addr.s_addr) & mask) == (host & mask) ){
			ret = val[0] == L'+';
		}
		if( sep == FALSE ){
			return ret;
		}
	}
}

static int RecvAll(SOCKET sock, char* buf, int len, int flags)
{
	int n = 0;
	while( n < len ){
		int ret = recv(sock, buf + n, len - n, flags);
		if( ret < 0 ){
			return ret;
		}else if( ret <= 0 ){
			break;
		}
		n += ret;
	}
	return n;
}

UINT WINAPI CTCPServer::ServerThread(LPVOID pParam)
{
	CTCPServer* pSys = (CTCPServer*)pParam;

	struct WAIT_INFO {
		SOCKET sock;
		CMD_STREAM* cmd;
		DWORD tick;
	};
	vector<WAIT_INFO> waitList;
	vector<WSAEVENT> hEventList;
	hEventList.push_back(pSys->m_hNotifyEvent);
	hEventList.push_back(WSACreateEvent());
	WSAEventSelect(pSys->m_sock, hEventList.back(), FD_ACCEPT);

	while( pSys->m_stopFlag == FALSE ){
		DWORD result = WSAWaitForMultipleEvents((DWORD)hEventList.size(), &hEventList[0], FALSE, waitList.empty() ? WSA_INFINITE : 2000, FALSE);
		if( result == WSA_WAIT_EVENT_0 || result == WSA_WAIT_TIMEOUT ){
			WSAResetEvent(hEventList[0]);
			for( size_t i = 0; i < waitList.size(); i++ ){
				if( waitList[i].cmd == NULL ){
					continue;
				}
				CMD_STREAM stRes;
				pSys->m_pCmdProc(pSys->m_pParam, waitList[i].cmd, &stRes);
				if( stRes.param == CMD_NO_RES ){
					//応答は保留された
					if( GetTickCount() - waitList[i].tick <= pSys->m_dwResponseTimeout ){
						continue;
					}
				}else{
					DWORD head[256];
					head[0] = stRes.param;
					head[1] = stRes.dataSize;
					DWORD extSize = 0;
					if( stRes.dataSize > 0 ){
						extSize = min(stRes.dataSize, sizeof(head) - sizeof(DWORD)*2);
						memcpy(head + 2, stRes.data.get(), extSize);
					}
					//ブロッキングモードに変更
					WSAEventSelect(waitList[i].sock, NULL, 0);
					ULONG x = 0;
					ioctlsocket(waitList[i].sock, FIONBIO, &x);
					if( send(waitList[i].sock, (const char*)head, sizeof(DWORD)*2 + extSize, 0) != SOCKET_ERROR ){
						if( stRes.dataSize > extSize ){
							send(waitList[i].sock, (const char*)stRes.data.get() + extSize, stRes.dataSize - extSize, 0);
						}
					}
					WSAEventSelect(waitList[i].sock, hEventList[2 + i], FD_READ | FD_CLOSE);
				}
				shutdown(waitList[i].sock, SD_BOTH);
				//タイムアウトか応答済み(ここでは閉じない)
				delete waitList[i].cmd;
				waitList[i].cmd = NULL;
			}
		}else if( WSA_WAIT_EVENT_0 + 2 <= result && result < WSA_WAIT_EVENT_0 + hEventList.size() ){
			size_t i = result - WSA_WAIT_EVENT_0 - 2;
			WSANETWORKEVENTS events;
			if( WSAEnumNetworkEvents(waitList[i].sock, hEventList[2 + i], &events) != SOCKET_ERROR ){
				if( events.lNetworkEvents & FD_CLOSE ){
					//閉じる
					closesocket(waitList[i].sock);
					delete waitList[i].cmd;
					waitList.erase(waitList.begin() + i);
					WSACloseEvent(hEventList[2 + i]);
					hEventList.erase(hEventList.begin() + 2 + i);
				}else if( events.lNetworkEvents & FD_READ ){
					//読み飛ばす
					OutputDebugString(L"Unexpected FD_READ\r\n");
					char buf[1024];
					recv(waitList[i].sock, buf, sizeof(buf), 0);
				}
			}
		}else if( result == WSA_WAIT_EVENT_0 + 1 ){
			struct sockaddr_in client = {};
			int len = sizeof(client);
			SOCKET sock = INVALID_SOCKET;
			WSANETWORKEVENTS events;
			if( WSAEnumNetworkEvents(pSys->m_sock, hEventList[1], &events) != SOCKET_ERROR ){
				if( events.lNetworkEvents & FD_ACCEPT ){
					sock = accept(pSys->m_sock, (struct sockaddr *)&client, &len);
				}
			}
			if( sock != INVALID_SOCKET && TestAcl(client.sin_addr, pSys->m_acl) == FALSE ){
				_OutputDebugString(L"Deny from IP:0x%08x\r\n", ntohl(client.sin_addr.s_addr));
				closesocket(sock);
			}else if( sock != INVALID_SOCKET ){
				//ブロッキングモードに変更
				WSAEventSelect(sock, NULL, 0);
				ULONG x = 0;
				ioctlsocket(sock, FIONBIO, &x);
				for(;;){
					CMD_STREAM stCmd;
					CMD_STREAM stRes;
					DWORD head[256];
					if( RecvAll(sock, (char*)head, sizeof(DWORD)*2, 0) != sizeof(DWORD)*2 ){
						break;
					}
					//第2,3バイトは0でなければならない
					if( HIWORD(head[0]) != 0 ){
						_OutputDebugString(L"Deny TCP cmd:0x%08x\r\n", head[0]);
						break;
					}
					stCmd.param = head[0];
					stCmd.dataSize = head[1];

					if( stCmd.dataSize > 0 ){
						stCmd.data.reset(new BYTE[stCmd.dataSize]);
						if( RecvAll(sock, (char*)stCmd.data.get(), stCmd.dataSize, 0) != (int)stCmd.dataSize ){
							break;
						}
					}

					if( stCmd.param == CMD2_EPG_SRV_REGIST_GUI_TCP || stCmd.param == CMD2_EPG_SRV_UNREGIST_GUI_TCP || stCmd.param == CMD2_EPG_SRV_ISREGIST_GUI_TCP ){
						char ip[64];
						if( getnameinfo((struct sockaddr *)&client, sizeof(client), ip, sizeof(ip), NULL, 0, NI_NUMERICHOST) != 0 ){
							ip[0] = '\0';
						}

						REGIST_TCP_INFO setParam;
						AtoW(ip, setParam.ip);
						ReadVALUE(&setParam.port, stCmd.data, stCmd.dataSize, NULL);

						stCmd.data = NewWriteVALUE(setParam, stCmd.dataSize);
					}

					pSys->m_pCmdProc(pSys->m_pParam, &stCmd, &stRes);
					if( stRes.param == CMD_NO_RES ){
						if( stCmd.param == CMD2_EPG_SRV_GET_STATUS_NOTIFY2 ){
							//保留可能なコマンドは応答待ちリストに移動
							if( hEventList.size() < WSA_MAXIMUM_WAIT_EVENTS ){
								WAIT_INFO waitInfo;
								waitInfo.sock = sock;
								waitInfo.cmd = new CMD_STREAM;
								waitInfo.cmd->param = stCmd.param;
								waitInfo.cmd->dataSize = stCmd.dataSize;
								waitInfo.cmd->data.swap(stCmd.data);
								waitInfo.tick = GetTickCount();
								waitList.push_back(waitInfo);
								hEventList.push_back(WSACreateEvent());
								WSAEventSelect(sock, hEventList.back(), FD_READ | FD_CLOSE);
								sock = INVALID_SOCKET;
							}
						}
						break;
					}
					head[0] = stRes.param;
					head[1] = stRes.dataSize;
					DWORD extSize = 0;
					if( stRes.dataSize > 0 ){
						extSize = min(stRes.dataSize, sizeof(head) - sizeof(DWORD)*2);
						memcpy(head + 2, stRes.data.get(), extSize);
					}
					if( send(sock, (char*)head, sizeof(DWORD)*2 + extSize, 0) == SOCKET_ERROR ||
					    stRes.dataSize > extSize && send(sock, (char*)stRes.data.get() + extSize, stRes.dataSize - extSize, 0) == SOCKET_ERROR ){
						break;
					}
					if( stRes.param != CMD_NEXT && stRes.param != OLD_CMD_NEXT ){
						//Enum用の繰り返しではない
						break;
					}
				}
				if( sock != INVALID_SOCKET ){
					shutdown(sock, SD_BOTH);
					closesocket(sock);
				}
			}
		}else{
			break;
		}
	}

	while( waitList.empty() == false ){
		closesocket(waitList.back().sock);
		delete waitList.back().cmd;
		waitList.pop_back();
		WSACloseEvent(hEventList.back());
		hEventList.pop_back();
	}
	WSAEventSelect(pSys->m_sock, NULL, 0);
	WSACloseEvent(hEventList.back());

	return 0;
}
