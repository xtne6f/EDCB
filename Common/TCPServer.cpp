#include "stdafx.h"
#include "TCPServer.h"
#include "StringUtil.h"
#include "CtrlCmdDef.h"
#include "ErrDef.h"
#ifndef _WIN32
#include <netdb.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

typedef int SOCKET;
static const int INVALID_SOCKET = -1;
#define closesocket(sock) close(sock)
#endif

CTCPServer::CTCPServer(void)
{
	m_sock = INVALID_SOCKET;
#ifdef _WIN32
	m_hAcceptEvent = WSA_INVALID_EVENT;

	WSAData wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

CTCPServer::~CTCPServer(void)
{
	StopServer();
#ifdef _WIN32
	WSACleanup();
#endif
}

bool CTCPServer::StartServer(unsigned short port, bool ipv6, DWORD dwResponseTimeout, LPCWSTR acl,
                             const std::function<void(CMD_STREAM*, CMD_STREAM*, LPCWSTR)>& cmdProc)
{
	if( !cmdProc ){
		return false;
	}
	if( m_thread.joinable() &&
	    m_port == port &&
	    m_ipv6 == ipv6 &&
	    m_dwResponseTimeout == dwResponseTimeout &&
	    m_acl == acl ){
		//cmdProcの変化は想定していない
		return true;
	}
	StopServer();
	m_cmdProc = cmdProc;
	m_port = port;
	m_ipv6 = ipv6;
	m_dwResponseTimeout = dwResponseTimeout;
	m_acl = acl;

	char szPort[16];
	sprintf_s(szPort, "%d", m_port);
	struct addrinfo hints = {};
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = m_ipv6 ? AF_INET6 : AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	struct addrinfo* result;
	if( getaddrinfo(NULL, szPort, &hints, &result) != 0 ){
		return false;
	}

	m_sock = socket(result->ai_family, result->ai_socktype
#ifndef _WIN32
	                    | SOCK_CLOEXEC | SOCK_NONBLOCK
#endif
	                , result->ai_protocol);
	if( m_sock != INVALID_SOCKET ){
		BOOL b = TRUE;
		setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&b, sizeof(b));
		if( m_ipv6 ){
			//デュアルスタックにはしない
			b = TRUE;
			setsockopt(m_sock, IPPROTO_IPV6, IPV6_V6ONLY, (const char*)&b, sizeof(b));
		}
#ifdef _WIN32
		m_hAcceptEvent = WSACreateEvent();
		if( m_hAcceptEvent != WSA_INVALID_EVENT && WSAEventSelect(m_sock, m_hAcceptEvent, FD_ACCEPT) == 0 )
#endif
		{
			if( bind(m_sock, result->ai_addr, (int)result->ai_addrlen) == 0 && listen(m_sock, SOMAXCONN) == 0 ){
				m_stopFlag = false;
				m_notifyEvent.Reset();
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
		m_notifyEvent.Set();
		m_thread.join();
	}
	if( m_sock != INVALID_SOCKET ){
#ifdef _WIN32
		WSAEventSelect(m_sock, NULL, 0);
		unsigned long x = 0;
		ioctlsocket(m_sock, FIONBIO, &x);
#else
		int x = 0;
		ioctl(m_sock, FIONBIO, &x);
#endif
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
	}
#ifdef _WIN32
	if( m_hAcceptEvent != WSA_INVALID_EVENT ){
		WSACloseEvent(m_hAcceptEvent);
		m_hAcceptEvent = WSA_INVALID_EVENT;
	}
#endif
}

void CTCPServer::NotifyUpdate()
{
	if( m_thread.joinable() ){
		m_notifyEvent.Set();
	}
}

namespace
{
bool TestAcl(struct sockaddr* addr, wstring acl)
{
	//書式例: +192.168.0.0/16,-192.168.0.1
	//書式例(IPv6): +fe80::/64,-::1
	bool ret = false;
	for(;;){
		wstring val;
		bool sep = Separate(acl, L",", val, acl);
		if( val.empty() || (val[0] != L'+' && val[0] != L'-') ){
			//書式エラー
			return false;
		}
		wstring m;
		int mm = Separate(val, L"/", val, m) ? (int)wcstol(m.c_str(), NULL, 10) : addr->sa_family == AF_INET6 ? 128 : 32;
		if( val.empty() || mm < 0 || mm > (addr->sa_family == AF_INET6 ? 128 : 32) ){
			//書式エラー
			return false;
		}
		string valU;
		WtoUTF8(val, valU);
		struct addrinfo hints = {};
		hints.ai_flags = AI_NUMERICHOST;
		hints.ai_family = addr->sa_family;
		struct addrinfo* result;
		if( getaddrinfo(valU.c_str() + 1, NULL, &hints, &result) != 0 ){
			//書式エラー
			return false;
		}
		if( result->ai_family == AF_INET6 ){
			int i = 0;
			for( ; i < 16; i++ ){
				BYTE mask = (BYTE)(8 * i + 8 < mm ? 0xFF : 8 * i > mm ? 0 : 0xFF << (8 * i + 8 - mm));
				if( (((struct sockaddr_in6*)addr)->sin6_addr.s6_addr[i] & mask) !=
				    (((struct sockaddr_in6*)result->ai_addr)->sin6_addr.s6_addr[i] & mask) ){
					break;
				}
			}
			if( i == 16 ){
				ret = val[0] == '+';
			}
		}else{
			DWORD mask = mm == 0 ? 0 : 0xFFFFFFFF << (32 - mm);
			if( (ntohl(((struct sockaddr_in*)addr)->sin_addr.s_addr) & mask) ==
			    (ntohl(((struct sockaddr_in*)result->ai_addr)->sin_addr.s_addr) & mask) ){
				ret = val[0] == '+';
			}
		}
		freeaddrinfo(result);
		if( sep == false ){
			return ret;
		}
	}
}

int RecvAll(SOCKET sock, char* buf, int len, int flags)
{
	int n = 0;
	while( n < len ){
		int ret = (int)recv(sock, buf + n, len - n, flags);
		if( ret < 0 ){
			return ret;
		}else if( ret <= 0 ){
			break;
		}
		n += ret;
	}
	return n;
}

void SetBlockingMode(SOCKET sock)
{
#ifdef _WIN32
	WSAEventSelect(sock, NULL, 0);
	unsigned long x = 0;
	ioctlsocket(sock, FIONBIO, &x);
	DWORD to = CTCPServer::SND_RCV_TIMEOUT;
#else
	int x = 0;
	ioctl(sock, FIONBIO, &x);
	timeval to = {};
	to.tv_sec = CTCPServer::SND_RCV_TIMEOUT / 1000;
#endif
	setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&to, sizeof(to));
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&to, sizeof(to));
}

struct WAIT_INFO {
	SOCKET sock;
	CMD_STREAM cmd;
	DWORD tick;
	bool closing;
#ifdef _WIN32
	WSAEVENT hEvent;
#endif
};

void SetNonBlockingMode(const WAIT_INFO& info)
{
#ifdef _WIN32
	WSAEventSelect(info.sock, info.hEvent, FD_READ | FD_CLOSE);
	DWORD noTimeout = 0;
#else
	int x = 1;
	ioctl(info.sock, FIONBIO, &x);
	timeval noTimeout = {};
#endif
	setsockopt(info.sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&noTimeout, sizeof(noTimeout));
	setsockopt(info.sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&noTimeout, sizeof(noTimeout));
}
}

void CTCPServer::ServerThread(CTCPServer* pSys)
{
	vector<std::unique_ptr<WAIT_INFO>> waitList;

	while( pSys->m_stopFlag == false ){
#ifdef _WIN32
		vector<WSAEVENT> hEventList(2 + waitList.size());
		hEventList[0] = pSys->m_notifyEvent.Handle();
		hEventList[1] = pSys->m_hAcceptEvent;
		for( size_t i = 0; i < waitList.size(); i++ ){
			hEventList[2 + i] = waitList[i]->hEvent;
		}
		DWORD result = WSAWaitForMultipleEvents((DWORD)hEventList.size(), &hEventList[0], FALSE, waitList.empty() ? WSA_INFINITE : NOTIFY_INTERVAL, FALSE);
		if( WSA_WAIT_EVENT_0 + 2 <= result && result < WSA_WAIT_EVENT_0 + hEventList.size() ){
			size_t i = result - WSA_WAIT_EVENT_0 - 2;
			WSANETWORKEVENTS events;
			if( WSAEnumNetworkEvents(waitList[i]->sock, waitList[i]->hEvent, &events) != SOCKET_ERROR ){
				if( events.lNetworkEvents & FD_CLOSE ){
					//閉じる
					closesocket(waitList[i]->sock);
					WSACloseEvent(waitList[i]->hEvent);
					waitList.erase(waitList.begin() + i);
				}else if( events.lNetworkEvents & FD_READ ){
					//読み飛ばす
					AddDebugLog(L"Unexpected FD_READ");
					char buf[1024];
					recv(waitList[i]->sock, buf, sizeof(buf), 0);
				}
			}
		}else if( result == WSA_WAIT_EVENT_0 || result == WSA_WAIT_TIMEOUT ){
#else
		vector<pollfd> pfdList(2 + waitList.size());
		pfdList[0].fd = pSys->m_notifyEvent.Handle();
		pfdList[0].events = POLLIN;
		pfdList[1].fd = pSys->m_sock;
		pfdList[1].events = POLLIN;
		for( size_t i = 0; i < waitList.size(); i++ ){
			pfdList[2 + i].fd = waitList[i]->sock;
			pfdList[2 + i].events = POLLIN;
		}
		int result = poll(&pfdList[0], pfdList.size(), waitList.empty() ? -1 : (int)NOTIFY_INTERVAL);
		if( result < 0 ){
			break;
		}
		for( size_t i = 0; i < waitList.size(); ){
			if( pfdList[2 + i].revents & POLLIN ){
				//閉じる
				close(waitList[i]->sock);
				waitList.erase(waitList.begin() + i);
				pfdList.erase(pfdList.begin() + 2 + i);
			}else{
				i++;
			}
		}
		if( result == 0 || (pfdList[0].revents & POLLIN) ){
#endif
			for( size_t i = 0; i < waitList.size(); i++ ){
				if( waitList[i]->closing ){
					continue;
				}
				CMD_STREAM stRes;
				pSys->m_cmdProc(&waitList[i]->cmd, &stRes, NULL);
				if( stRes.param == CMD_NO_RES ){
					//応答は保留された
					if( GetTickCount() - waitList[i]->tick <= pSys->m_dwResponseTimeout ){
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
					SetBlockingMode(waitList[i]->sock);
					if( send(waitList[i]->sock, (const char*)head, sizeof(DWORD)*2 + extSize, 0) == (int)(sizeof(DWORD)*2 + extSize) ){
						if( stRes.dataSize > extSize ){
							send(waitList[i]->sock, (const char*)stRes.data.get() + extSize, stRes.dataSize - extSize, 0);
						}
					}
					SetNonBlockingMode(*waitList[i]);
				}
				shutdown(waitList[i]->sock, 2); //SD_BOTH
				//タイムアウトか応答済み(ここでは閉じない)
				waitList[i]->closing = true;
			}
		}
#ifdef _WIN32
		else if( result == WSA_WAIT_EVENT_0 + 1 ){
			struct sockaddr_storage client = {};
			int clientLen = sizeof(client);
			SOCKET sock = INVALID_SOCKET;
			WSANETWORKEVENTS events;
			if( WSAEnumNetworkEvents(pSys->m_sock, hEventList[1], &events) != SOCKET_ERROR ){
				if( events.lNetworkEvents & FD_ACCEPT ){
					sock = accept(pSys->m_sock, (struct sockaddr*)&client, &clientLen);
				}
			}
#else
		if( pfdList[1].revents & POLLIN ){
			struct sockaddr_storage client = {};
			socklen_t clientLen = sizeof(client);
			int sock = accept4(pSys->m_sock, (struct sockaddr*)&client, &clientLen, SOCK_CLOEXEC);
#endif
			if( sock != INVALID_SOCKET ){
				if( (pSys->m_ipv6 && client.ss_family != AF_INET6) || (pSys->m_ipv6 == false && client.ss_family != AF_INET) ){
					AddDebugLog(L"IP protocol mismatch");
					closesocket(sock);
					sock = INVALID_SOCKET;
				}else if( TestAcl((struct sockaddr*)&client, pSys->m_acl) == false ){
					wstring ipW;
					char ip[NI_MAXHOST];
					if( getnameinfo((struct sockaddr*)&client, clientLen, ip, NI_MAXHOST, NULL, 0, NI_NUMERICHOST) == 0 ){
						UTF8toW(ip, ipW);
					}
					AddDebugLogFormat(L"Deny from IP: %ls", ipW.c_str());
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
					if( RecvAll(sock, (char*)head, sizeof(DWORD)*2, 0) != (int)(sizeof(DWORD)*2) ){
						break;
					}
					//第2,3バイトは0でなければならない
					if( head[0] & 0xFFFF0000 ){
						AddDebugLogFormat(L"Deny TCP cmd:0x%08x", head[0]);
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

					wstring clientIP;
					if( stCmd.param == CMD2_EPG_SRV_REGIST_GUI_TCP ||
					    stCmd.param == CMD2_EPG_SRV_UNREGIST_GUI_TCP ||
					    stCmd.param == CMD2_EPG_SRV_ISREGIST_GUI_TCP ){
						//接続元IPを引数に添付
						char ip[NI_MAXHOST];
						if( getnameinfo((struct sockaddr*)&client, clientLen, ip, NI_MAXHOST, NULL, 0, NI_NUMERICHOST) == 0 ){
							UTF8toW(ip, clientIP);
						}
					}

					pSys->m_cmdProc(&stCmd, &stRes, clientIP.empty() ? NULL : clientIP.c_str());
					if( stRes.param == CMD_NO_RES ){
						if( stCmd.param == CMD2_EPG_SRV_GET_STATUS_NOTIFY2 ){
							//保留可能なコマンドは応答待ちリストに移動
#ifdef _WIN32
							WSAEVENT hEvent;
							if( hEventList.size() < WSA_MAXIMUM_WAIT_EVENTS && (hEvent = WSACreateEvent()) != WSA_INVALID_EVENT )
#endif
							{
								waitList.push_back(std::unique_ptr<WAIT_INFO>(new WAIT_INFO));
								waitList.back()->sock = sock;
								waitList.back()->cmd.param = stCmd.param;
								waitList.back()->cmd.dataSize = stCmd.dataSize;
								waitList.back()->cmd.data.swap(stCmd.data);
								waitList.back()->tick = GetTickCount();
								waitList.back()->closing = false;
#ifdef _WIN32
								waitList.back()->hEvent = hEvent;
#endif
								SetNonBlockingMode(*waitList.back());
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
						shutdown(sock, 2); //SD_BOTH
						break;
					}
				}
				if( sock != INVALID_SOCKET ){
					closesocket(sock);
				}
			}
		}
#ifdef _WIN32
		else{
			break;
		}
#endif
	}

	while( waitList.empty() == false ){
		SetBlockingMode(waitList.back()->sock);
		closesocket(waitList.back()->sock);
#ifdef _WIN32
		WSACloseEvent(waitList.back()->hEvent);
#endif
		waitList.pop_back();
	}
}
