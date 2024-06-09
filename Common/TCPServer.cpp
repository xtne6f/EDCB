#include "stdafx.h"
#include "TCPServer.h"
#include "StringUtil.h"
#include "TimeUtil.h"
#include "CtrlCmdDef.h"
#include "ErrDef.h"
#ifndef _WIN32
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
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
                             const std::function<void(const CCmdStream&, CCmdStream&, LPCWSTR)>& cmdProc,
                             const std::function<void(const CCmdStream&, CCmdStream&, RESPONSE_THREAD_STATE, void*&)>& responseThreadProc)
{
	if( !cmdProc ){
		return false;
	}
	if( m_thread.joinable() &&
	    m_port == port &&
	    m_ipv6 == ipv6 &&
	    m_dwResponseTimeout == dwResponseTimeout &&
	    m_acl == acl ){
		//cmdProcとresponseThreadProcの変化は想定していない
		return true;
	}
	StopServer();
	m_cmdProc = cmdProc;
	m_responseThreadProc = responseThreadProc;
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
	CCmdStream cmd;
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
	vector<std::unique_ptr<RESPONSE_THREAD_INFO>> resThreadList;
	vector<WAIT_INFO> waitList;

	while( pSys->m_stopFlag == false ){
#ifdef _WIN32
		vector<WSAEVENT> hEventList(2 + waitList.size());
		hEventList[0] = pSys->m_notifyEvent.Handle();
		hEventList[1] = pSys->m_hAcceptEvent;
		for( size_t i = 0; i < waitList.size(); i++ ){
			hEventList[2 + i] = waitList[i].hEvent;
		}
		DWORD result = WSAWaitForMultipleEvents((DWORD)hEventList.size(), &hEventList[0], FALSE, waitList.empty() ? WSA_INFINITE : NOTIFY_INTERVAL, FALSE);
		if( WSA_WAIT_EVENT_0 + 2 <= result && result < WSA_WAIT_EVENT_0 + hEventList.size() ){
			size_t i = result - WSA_WAIT_EVENT_0 - 2;
			WSANETWORKEVENTS events;
			if( WSAEnumNetworkEvents(waitList[i].sock, waitList[i].hEvent, &events) != SOCKET_ERROR ){
				if( events.lNetworkEvents & FD_CLOSE ){
					//閉じる
					closesocket(waitList[i].sock);
					WSACloseEvent(waitList[i].hEvent);
					waitList.erase(waitList.begin() + i);
				}else if( events.lNetworkEvents & FD_READ ){
					//読み飛ばす
					AddDebugLog(L"Unexpected FD_READ");
					char buf[1024];
					recv(waitList[i].sock, buf, sizeof(buf), 0);
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
			pfdList[2 + i].fd = waitList[i].sock;
			pfdList[2 + i].events = POLLIN;
		}
		int result = poll(&pfdList[0], pfdList.size(), waitList.empty() ? -1 : (int)NOTIFY_INTERVAL);
		if( result < 0 ){
			if( errno == EINTR ){
				continue;
			}
			break;
		}
		for( size_t i = 0; i < waitList.size(); ){
			if( pfdList[2 + i].revents & POLLIN ){
				//閉じる
				close(waitList[i].sock);
				waitList.erase(waitList.begin() + i);
				pfdList.erase(pfdList.begin() + 2 + i);
			}else{
				i++;
			}
		}
		if( result == 0 || (pfdList[0].revents & POLLIN) ){
			pSys->m_notifyEvent.Reset();
#endif
			for( size_t i = 0; i < waitList.size(); i++ ){
				if( waitList[i].closing ){
					continue;
				}
				CCmdStream res;
				pSys->m_cmdProc(waitList[i].cmd, res, NULL);
				if( res.GetParam() == CMD_NO_RES ){
					//応答は保留された
					if( GetU32Tick() - waitList[i].tick <= pSys->m_dwResponseTimeout ){
						continue;
					}
				}else{
					//ブロッキングモードに変更
					SetBlockingMode(waitList[i].sock);
					send(waitList[i].sock, (const char*)res.GetStream(), res.GetStreamSize(), 0);
					SetNonBlockingMode(waitList[i]);
				}
				shutdown(waitList[i].sock, 2); //SD_BOTH
				//タイムアウトか応答済み(ここでは閉じない)
				waitList[i].closing = true;
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
					BYTE head[8];
					if( RecvAll(sock, (char*)head, sizeof(head), 0) != (int)sizeof(head) ){
						break;
					}
					CCmdStream cmd(head[0] | head[1] << 8 | head[2] << 16 | (DWORD)head[3] << 24);
					//第2,3バイトは0でなければならない
					if( cmd.GetParam() & 0xFFFF0000 ){
						AddDebugLogFormat(L"Deny TCP cmd:0x%08x", cmd.GetParam());
						break;
					}
					cmd.Resize(head[4] | head[5] << 8 | head[6] << 16 | (DWORD)head[7] << 24);
					if( RecvAll(sock, (char*)cmd.GetData(), cmd.GetDataSize(), 0) != (int)cmd.GetDataSize() ){
						break;
					}

					wstring clientIP;
					if( cmd.GetParam() == CMD2_EPG_SRV_REGIST_GUI_TCP ||
					    cmd.GetParam() == CMD2_EPG_SRV_UNREGIST_GUI_TCP ||
					    cmd.GetParam() == CMD2_EPG_SRV_ISREGIST_GUI_TCP ||
					    cmd.GetParam() == CMD2_EPG_SRV_NWPLAY_SET_IP ){
						//接続元IPを引数に添付
						char ip[NI_MAXHOST];
						if( getnameinfo((struct sockaddr*)&client, clientLen, ip, NI_MAXHOST, NULL, 0, NI_NUMERICHOST) == 0 ){
							UTF8toW(ip, clientIP);
						}
					}

					CCmdStream res;
					pSys->m_cmdProc(cmd, res, clientIP.empty() ? NULL : clientIP.c_str());
					if( res.GetParam() == CMD_NO_RES ){
						if( cmd.GetParam() == CMD2_EPG_SRV_GET_STATUS_NOTIFY2 ){
							//保留可能なコマンドは応答待ちリストに移動
#ifdef _WIN32
							WSAEVENT hEvent;
							if( hEventList.size() < WSA_MAXIMUM_WAIT_EVENTS && (hEvent = WSACreateEvent()) != WSA_INVALID_EVENT )
#endif
							{
								waitList.resize(waitList.size() + 1);
								waitList.back().sock = sock;
								std::swap(waitList.back().cmd, cmd);
								waitList.back().tick = GetU32Tick();
								waitList.back().closing = false;
#ifdef _WIN32
								waitList.back().hEvent = hEvent;
#endif
								SetNonBlockingMode(waitList.back());
								sock = INVALID_SOCKET;
							}
						}
						break;
					}else if( res.GetParam() == CMD_NO_RES_THREAD ){
						//できるだけ回収
						for( auto itr = resThreadList.begin(); itr != resThreadList.end(); ){
							if( (*itr)->completed ){
								(*itr)->th.join();
								itr = resThreadList.erase(itr);
							}else{
								itr++;
							}
						}
						if( resThreadList.size() >= RESPONSE_THREADS_MAX ){
							AddDebugLog(L"Too many TCP threads");
							break;
						}
						//応答用スレッドを追加
						resThreadList.push_back(std::unique_ptr<RESPONSE_THREAD_INFO>(new RESPONSE_THREAD_INFO));
						RESPONSE_THREAD_INFO& info = *resThreadList.back();
						info.sock = sock;
						std::swap(info.cmd, cmd);
						info.sys = pSys;
						info.completed = false;
						info.th = thread_(ResponseThread, &info);
						sock = INVALID_SOCKET;
						break;
					}
					if( send(sock, (const char*)res.GetStream(), res.GetStreamSize(), 0) != (int)res.GetStreamSize() ){
						break;
					}
					if( res.GetParam() != OLD_CMD_NEXT ){
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
		SetBlockingMode(waitList.back().sock);
		closesocket(waitList.back().sock);
#ifdef _WIN32
		WSACloseEvent(waitList.back().hEvent);
#endif
		waitList.pop_back();
	}
	while( resThreadList.empty() == false ){
		resThreadList.back()->th.join();
		resThreadList.pop_back();
	}
}

void CTCPServer::ResponseThread(RESPONSE_THREAD_INFO* info)
{
	CCmdStream res(CMD_ERR);
	void* param = NULL;
	if( info->sys->m_responseThreadProc ){
		info->sys->m_responseThreadProc(info->cmd, res, RESPONSE_THREAD_INIT, param);
	}
	if( send(info->sock, (const char*)res.GetStream(), res.GetStreamSize(), 0) != (int)res.GetStreamSize() ){
		if( res.GetParam() == CMD_SUCCESS ){
			info->sys->m_responseThreadProc(info->cmd, res, RESPONSE_THREAD_FIN, param);
		}
	}else if( res.GetParam() == CMD_SUCCESS ){
		DWORD tick = GetU32Tick();
		while( res.GetParam() == CMD_SUCCESS && info->sys->m_stopFlag == false &&
		       GetU32Tick() - tick <= info->sys->m_dwResponseTimeout ){
			res.SetParam(CMD_ERR);
			res.Resize(0);
			info->sys->m_responseThreadProc(info->cmd, res, RESPONSE_THREAD_PROC, param);
			if( info->sys->m_stopFlag == false && res.GetDataSize() != 0 ){
				if( send(info->sock, (const char*)res.GetData(), res.GetDataSize(), 0) != (int)res.GetDataSize() ){
					break;
				}
				tick = GetU32Tick();
			}
		}
		info->sys->m_responseThreadProc(info->cmd, res, RESPONSE_THREAD_FIN, param);
	}
	closesocket(info->sock);
	info->completed = true;
}
