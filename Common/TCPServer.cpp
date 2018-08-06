#include "stdafx.h"
#include "TCPServer.h"
#include "ErrDef.h"
#include "CtrlCmdUtil.h"

CTCPServer::CTCPServer(void)
{
	m_hNotifyEvent = WSA_INVALID_EVENT;
	m_hAcceptEvent = WSA_INVALID_EVENT;
	m_sock = INVALID_SOCKET;

	WSAData wsaData;
	WSAStartup(MAKEWORD(2,0), &wsaData);
}

CTCPServer::~CTCPServer(void)
{
	StopServer();
	WSACleanup();
}

bool CTCPServer::StartServer(unsigned short port, bool ipv6, DWORD dwResponseTimeout, LPCWSTR acl,
                             const std::function<void(CMD_STREAM*, CMD_STREAM*)>& cmdProc)
{
	if( !cmdProc ){
		return false;
	}
	string aclU;
	WtoUTF8(acl, aclU);
	if( m_thread.joinable() &&
	    m_port == port &&
	    m_ipv6 == ipv6 &&
	    m_dwResponseTimeout == dwResponseTimeout &&
	    m_acl == aclU ){
		//cmdProcの変化は想定していない
		return true;
	}
	StopServer();
	m_cmdProc = cmdProc;
	m_port = port;
	m_ipv6 = ipv6;
	m_dwResponseTimeout = dwResponseTimeout;
	m_acl = aclU;

	string strPort;
	Format(strPort, "%d", m_port);
	struct addrinfo hints = {};
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = m_ipv6 ? AF_INET6 : AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	struct addrinfo* result;
	if( getaddrinfo(NULL, strPort.c_str(), &hints, &result) != 0 ){
		return false;
	}

	m_sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if( m_sock != INVALID_SOCKET ){
		BOOL b = TRUE;
		setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&b, sizeof(b));
		if( m_ipv6 ){
			//デュアルスタックにはしない
			b = TRUE;
			setsockopt(m_sock, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&b, sizeof(b));
		}
		if( bind(m_sock, result->ai_addr, (int)result->ai_addrlen) != SOCKET_ERROR && listen(m_sock, 1) != SOCKET_ERROR ){
			m_hNotifyEvent = WSACreateEvent();
			m_hAcceptEvent = WSACreateEvent();
			if( m_hNotifyEvent != WSA_INVALID_EVENT && m_hAcceptEvent != WSA_INVALID_EVENT ){
				m_stopFlag = false;
				m_thread = thread_(ServerThread, this);
				freeaddrinfo(result);
				return true;
			}
		}
		StopServer();
	}
	freeaddrinfo(result);

	return false;
}

void CTCPServer::StopServer()
{
	if( m_thread.joinable() ){
		m_stopFlag = true;
		WSASetEvent(m_hNotifyEvent);
		m_thread.join();
	}
	if( m_hAcceptEvent != WSA_INVALID_EVENT ){
		WSACloseEvent(m_hAcceptEvent);
		m_hAcceptEvent = WSA_INVALID_EVENT;
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
	if( m_thread.joinable() ){
		WSASetEvent(m_hNotifyEvent);
	}
}

static bool TestAcl(struct sockaddr* addr, string acl)
{
	//書式例: +192.168.0.0/16,-192.168.0.1
	//書式例(IPv6): +fe80::/64,-::1
	bool ret = false;
	for(;;){
		string val;
		BOOL sep = Separate(acl, ",", val, acl);
		if( val.empty() || val[0] != '+' && val[0] != '-' ){
			//書式エラー
			return false;
		}
		string m;
		int mm = Separate(val, "/", val, m) ? atoi(m.c_str()) : addr->sa_family == AF_INET6 ? 128 : 32;
		if( val.empty() || mm < 0 || mm > (addr->sa_family == AF_INET6 ? 128 : 32) ){
			//書式エラー
			return false;
		}
		struct addrinfo hints = {};
		hints.ai_flags = AI_NUMERICHOST;
		hints.ai_family = addr->sa_family;
		struct addrinfo* result;
		if( getaddrinfo(val.c_str() + 1, NULL, &hints, &result) != 0 ){
			//書式エラー
			return false;
		}
		if( result->ai_family == AF_INET6 ){
			int i = 0;
			for( ; i < 16; i++ ){
				UCHAR mask = (UCHAR)(8 * i + 8 < mm ? 0xFF : 8 * i > mm ? 0 : 0xFF << (8 * i + 8 - mm));
				if( (((struct sockaddr_in6*)addr)->sin6_addr.s6_addr[i] & mask) !=
				    (((struct sockaddr_in6*)result->ai_addr)->sin6_addr.s6_addr[i] & mask) ){
					break;
				}
			}
			if( i == 16 ){
				ret = val[0] == '+';
			}
		}else{
			ULONG mask = mm == 0 ? 0 : 0xFFFFFFFFUL << (32 - mm);
			if( (ntohl(((struct sockaddr_in*)addr)->sin_addr.s_addr) & mask) ==
			    (ntohl(((struct sockaddr_in*)result->ai_addr)->sin_addr.s_addr) & mask) ){
				ret = val[0] == '+';
			}
		}
		freeaddrinfo(result);
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

void CTCPServer::SetBlockingMode(SOCKET sock)
{
	WSAEventSelect(sock, NULL, 0);
	unsigned long x = 0;
	ioctlsocket(sock, FIONBIO, &x);
	DWORD to = SND_RCV_TIMEOUT;
	setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&to, sizeof(to));
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&to, sizeof(to));
}

void CTCPServer::SetNonBlockingMode(SOCKET sock, WSAEVENT hEvent, long lNetworkEvents)
{
	DWORD noTimeout = 0;
	setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&noTimeout, sizeof(noTimeout));
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&noTimeout, sizeof(noTimeout));
	WSAEventSelect(sock, hEvent, lNetworkEvents);
}

void CTCPServer::ServerThread(CTCPServer* pSys)
{
	struct WAIT_INFO {
		SOCKET sock;
		CMD_STREAM* cmd;
		DWORD tick;
	};
	vector<WAIT_INFO> waitList;
	vector<WSAEVENT> hEventList;
	hEventList.push_back(pSys->m_hNotifyEvent);
	hEventList.push_back(pSys->m_hAcceptEvent);
	WSAEventSelect(pSys->m_sock, hEventList.back(), FD_ACCEPT);

	while( pSys->m_stopFlag == false ){
		DWORD result = WSAWaitForMultipleEvents((DWORD)hEventList.size(), &hEventList[0], FALSE, waitList.empty() ? WSA_INFINITE : NOTIFY_INTERVAL, FALSE);
		if( result == WSA_WAIT_EVENT_0 || result == WSA_WAIT_TIMEOUT ){
			WSAResetEvent(hEventList[0]);
			for( size_t i = 0; i < waitList.size(); i++ ){
				if( waitList[i].cmd == NULL ){
					continue;
				}
				CMD_STREAM stRes;
				pSys->m_cmdProc(waitList[i].cmd, &stRes);
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
						extSize = min(stRes.dataSize, (DWORD)(sizeof(head) - sizeof(DWORD)*2));
						memcpy(head + 2, stRes.data.get(), extSize);
					}
					//ブロッキングモードに変更
					SetBlockingMode(waitList[i].sock);
					if( send(waitList[i].sock, (const char*)head, sizeof(DWORD)*2 + extSize, 0) == (int)(sizeof(DWORD)*2 + extSize) ){
						if( stRes.dataSize > extSize ){
							send(waitList[i].sock, (const char*)stRes.data.get() + extSize, stRes.dataSize - extSize, 0);
						}
					}
					SetNonBlockingMode(waitList[i].sock, hEventList[2 + i], FD_READ | FD_CLOSE);
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
			struct sockaddr_storage client = {};
			int clientLen = sizeof(client);
			SOCKET sock = INVALID_SOCKET;
			WSANETWORKEVENTS events;
			if( WSAEnumNetworkEvents(pSys->m_sock, hEventList[1], &events) != SOCKET_ERROR ){
				if( events.lNetworkEvents & FD_ACCEPT ){
					sock = accept(pSys->m_sock, (struct sockaddr*)&client, &clientLen);
				}
			}
			if( sock != INVALID_SOCKET ){
				if( (pSys->m_ipv6 && client.ss_family != AF_INET6) || (pSys->m_ipv6 == false && client.ss_family != AF_INET) ){
					OutputDebugString(L"IP protocol mismatch\r\n");
					closesocket(sock);
					sock = INVALID_SOCKET;
				}else if( TestAcl((struct sockaddr*)&client, pSys->m_acl) == false ){
					wstring ipW;
					char ip[NI_MAXHOST];
					if( getnameinfo((struct sockaddr*)&client, clientLen, ip, NI_MAXHOST, NULL, 0, NI_NUMERICHOST) == 0 ){
						UTF8toW(ip, ipW);
					}
					OutputDebugString((L"Deny from IP: " + ipW + L"\r\n").c_str());
					closesocket(sock);
					sock = INVALID_SOCKET;
				}
			}
			if( sock != INVALID_SOCKET ){
				//ブロッキングモードに変更
				SetBlockingMode(sock);
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
						REGIST_TCP_INFO setParam;
						char ip[NI_MAXHOST];
						if( getnameinfo((struct sockaddr*)&client, clientLen, ip, NI_MAXHOST, NULL, 0, NI_NUMERICHOST) == 0 &&
						    ReadVALUE(&setParam.port, stCmd.data, stCmd.dataSize, NULL) ){
							UTF8toW(ip, setParam.ip);
							stCmd.data = NewWriteVALUE(setParam, stCmd.dataSize);
						}else{
							//接続元IPの添付に失敗した
							stCmd.dataSize = 0;
							stCmd.data.reset();
						}
					}

					pSys->m_cmdProc(&stCmd, &stRes);
					if( stRes.param == CMD_NO_RES ){
						if( stCmd.param == CMD2_EPG_SRV_GET_STATUS_NOTIFY2 ){
							//保留可能なコマンドは応答待ちリストに移動
							WSAEVENT hEvent;
							if( hEventList.size() < WSA_MAXIMUM_WAIT_EVENTS && (hEvent = WSACreateEvent()) != WSA_INVALID_EVENT ){
								WAIT_INFO waitInfo;
								waitInfo.sock = sock;
								waitInfo.cmd = new CMD_STREAM;
								waitInfo.cmd->param = stCmd.param;
								waitInfo.cmd->dataSize = stCmd.dataSize;
								waitInfo.cmd->data.swap(stCmd.data);
								waitInfo.tick = GetTickCount();
								waitList.push_back(waitInfo);
								hEventList.push_back(hEvent);
								SetNonBlockingMode(sock, hEvent, FD_READ | FD_CLOSE);
								sock = INVALID_SOCKET;
							}
						}
						break;
					}
					head[0] = stRes.param;
					head[1] = stRes.dataSize;
					DWORD extSize = 0;
					if( stRes.dataSize > 0 ){
						extSize = min(stRes.dataSize, (DWORD)(sizeof(head) - sizeof(DWORD)*2));
						memcpy(head + 2, stRes.data.get(), extSize);
					}
					if( send(sock, (char*)head, sizeof(DWORD)*2 + extSize, 0) != (int)(sizeof(DWORD)*2 + extSize) ||
					    (stRes.dataSize > extSize &&
					     send(sock, (char*)stRes.data.get() + extSize, stRes.dataSize - extSize, 0) != (int)(stRes.dataSize - extSize)) ){
						break;
					}
					if( stRes.param != CMD_NEXT && stRes.param != OLD_CMD_NEXT ){
						//Enum用の繰り返しではない
						shutdown(sock, SD_BOTH);
						break;
					}
				}
				if( sock != INVALID_SOCKET ){
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
}
