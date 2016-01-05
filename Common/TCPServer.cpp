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

	bind(m_sock, (struct sockaddr *)&addr, sizeof(addr));

	listen(m_sock, 1);

	m_stopFlag = FALSE;
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, ServerThread, (LPVOID)this, CREATE_SUSPENDED, NULL);
	ResumeThread(m_hThread);

	return TRUE;
}

void CTCPServer::StopServer()
{
	if( m_hThread != NULL ){
		m_stopFlag = TRUE;
		// スレッド終了待ち
		if ( ::WaitForSingleObject(m_hThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(m_hThread, 0xffffffff);
		}
		CloseHandle(m_hThread);
		m_hThread = NULL;
	}
	
	if( m_sock != INVALID_SOCKET ){
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
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

	while( pSys->m_stopFlag == FALSE ){
		fd_set ready;
		struct timeval to;
		if( waitList.empty() ){
			//StopServer()が固まらない程度
			to.tv_sec = 1;
			to.tv_usec = 0;
		}else{
			//適度に迅速
			to.tv_sec = 0;
			to.tv_usec = 200000;
		}
		FD_ZERO(&ready);
		FD_SET(pSys->m_sock, &ready);
		for( size_t i = 0; i < waitList.size(); i++ ){
			FD_SET(waitList[i].sock, &ready);
		}

		if( select(0, &ready, NULL, NULL, &to ) == SOCKET_ERROR ){
			break;
		}
		for( size_t i = 0; i < waitList.size(); i++ ){
			if( FD_ISSET(waitList[i].sock, &ready) == 0 ){
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
						memcpy(head + 2, stRes.data, extSize);
					}
					if( send(waitList[i].sock, (const char*)head, sizeof(DWORD)*2 + extSize, 0) != SOCKET_ERROR ){
						if( stRes.dataSize > extSize ){
							send(waitList[i].sock, (const char*)stRes.data + extSize, stRes.dataSize - extSize, 0);
						}
					}
				}
				shutdown(waitList[i].sock, SD_BOTH);
			}
			//応答待ちソケットは応答済みかタイムアウトか切断を含むなんらかの受信があれば閉じる
			closesocket(waitList[i].sock);
			delete waitList[i].cmd;
			waitList.erase(waitList.begin() + i--);
		}

		if( FD_ISSET(pSys->m_sock, &ready) ){
			struct sockaddr_in client;
			int len = sizeof(client);
			SOCKET sock = accept(pSys->m_sock, (struct sockaddr *)&client, &len);
			if( sock == INVALID_SOCKET ){
				closesocket(pSys->m_sock);
				pSys->m_sock = INVALID_SOCKET;
				break;
			}else if( TestAcl(client.sin_addr, pSys->m_acl) == FALSE ){
				_OutputDebugString(L"Deny from IP:0x%08x\r\n", ntohl(client.sin_addr.s_addr));
				closesocket(sock);
			}else{
				for(;;){
					CMD_STREAM stCmd;
					CMD_STREAM stRes;
					DWORD head[256];
					if( RecvAll(sock, (char*)head, sizeof(DWORD)*2, 0) != sizeof(DWORD)*2 ){
						break;
					}
					stCmd.param = head[0];
					stCmd.dataSize = head[1];

					if( stCmd.dataSize > 0 ){
						stCmd.data = new BYTE[stCmd.dataSize];
						if( RecvAll(sock, (char*)stCmd.data, stCmd.dataSize, 0) != stCmd.dataSize ){
							break;
						}
					}

					if( stCmd.param == CMD2_EPG_SRV_REGIST_GUI_TCP || stCmd.param == CMD2_EPG_SRV_UNREGIST_GUI_TCP ){
						string ip = inet_ntoa(client.sin_addr);

						REGIST_TCP_INFO setParam;
						AtoW(ip, setParam.ip);
						ReadVALUE(&setParam.port, stCmd.data, stCmd.dataSize, NULL);

						SAFE_DELETE_ARRAY(stCmd.data);
						stCmd.data = NewWriteVALUE(&setParam, stCmd.dataSize);
					}

					pSys->m_pCmdProc(pSys->m_pParam, &stCmd, &stRes);
					if( stRes.param == CMD_NO_RES ){
						if( stCmd.param == CMD2_EPG_SRV_GET_STATUS_NOTIFY2 ){
							//保留可能なコマンドは応答待ちリストに移動
							if( waitList.size() < FD_SETSIZE - 1 ){
								WAIT_INFO waitInfo;
								waitInfo.sock = sock;
								waitInfo.cmd = new CMD_STREAM;
								waitInfo.cmd->param = stCmd.param;
								waitInfo.cmd->dataSize = stCmd.dataSize;
								waitInfo.cmd->data = stCmd.data;
								waitInfo.tick = GetTickCount();
								waitList.push_back(waitInfo);
								sock = INVALID_SOCKET;
								stCmd.data = NULL;
							}
						}
						break;
					}
					head[0] = stRes.param;
					head[1] = stRes.dataSize;
					DWORD extSize = 0;
					if( stRes.dataSize > 0 ){
						extSize = min(stRes.dataSize, sizeof(head) - sizeof(DWORD)*2);
						memcpy(head + 2, stRes.data, extSize);
					}
					if( send(sock, (char*)head, sizeof(DWORD)*2 + extSize, 0) == SOCKET_ERROR ||
					    stRes.dataSize > extSize && send(sock, (char*)stRes.data + extSize, stRes.dataSize - extSize, 0) == SOCKET_ERROR ){
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
		}
	}

	while( waitList.empty() == false ){
		closesocket(waitList.back().sock);
		delete waitList.back().cmd;
		waitList.pop_back();
	}

	return 0;
}
