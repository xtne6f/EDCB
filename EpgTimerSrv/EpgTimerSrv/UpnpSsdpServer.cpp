#include "StdAfx.h"
#include "UpnpSsdpServer.h"
#include "../../Common/StringUtil.h"
#include <process.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")

CUpnpSsdpServer::CUpnpSsdpServer()
	: ssdpThread(NULL)
{
}

CUpnpSsdpServer::~CUpnpSsdpServer()
{
	Stop();
}

bool CUpnpSsdpServer::Start(const vector<SSDP_TARGET_INFO>& targetList_)
{
	Stop();
	this->targetList = targetList_;
	this->stopFlag = false;
	this->ssdpThread = (HANDLE)_beginthreadex(NULL, 0, SsdpThread, this, 0, NULL);
	return this->ssdpThread != NULL;
}

void CUpnpSsdpServer::Stop()
{
	if( this->ssdpThread ){
		this->stopFlag = true;
		WaitForSingleObject(this->ssdpThread, INFINITE);
		CloseHandle(this->ssdpThread);
		this->ssdpThread = NULL;
	}
}

string CUpnpSsdpServer::GetUserAgent()
{
	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize = sizeof(osvi);
	GetVersionEx(&osvi);
	string ua;
	Format(ua, "Windows/%d.%d UPnP/1.1 EpgTimerSrv/0.10", osvi.dwMajorVersion, osvi.dwMinorVersion);
	return ua;
}

vector<string> CUpnpSsdpServer::GetNICList()
{
	vector<string> nicList(1, string("127.0.0.1"));
	//MSDN: "The recommended method ~ is to pre-allocate a 15KB working buffer pointed to by the AdapterAddresses parameter."
	IP_ADAPTER_ADDRESSES adpts[16384 / sizeof(IP_ADAPTER_ADDRESSES)];
	ULONG len = sizeof(adpts);
	if( GetAdaptersAddresses(AF_INET, 0, 0, adpts, &len) == ERROR_SUCCESS ){
		for( PIP_ADAPTER_ADDRESSES adpt = adpts; adpt; adpt = adpt->Next ){
			if( adpt->PhysicalAddressLength != 0 &&
			    adpt->IfType != IF_TYPE_SOFTWARE_LOOPBACK ){
				for( PIP_ADAPTER_UNICAST_ADDRESS uni = adpt->FirstUnicastAddress; uni; uni = uni->Next ){
					if( (uni->Flags & IP_ADAPTER_ADDRESS_DNS_ELIGIBLE) != 0 &&
					    (uni->Flags & IP_ADAPTER_ADDRESS_TRANSIENT) == 0 ){
						char host[NI_MAXHOST];
						if( getnameinfo(uni->Address.lpSockaddr, uni->Address.iSockaddrLength, host, sizeof(host), NULL, 0, NI_NUMERICHOST) == 0 ){
							if( std::find(nicList.begin(), nicList.end(), host) == nicList.end() ){
								nicList.push_back(host);
							}
						}
					}
				}
			}
		}
	}
	return nicList;
}

static bool GetInetAddr(unsigned long* addr, const char* host)
{
	*addr = htonl(INADDR_NONE);
	addrinfo hints = {};
	hints.ai_flags = AI_NUMERICHOST;
	hints.ai_family = AF_INET;
	addrinfo* result;
	if( getaddrinfo(host, NULL, &hints, &result) == 0 ){
		if( result->ai_addrlen >= sizeof(sockaddr_in) ){
			*addr = ((sockaddr_in*)result->ai_addr)->sin_addr.s_addr;
			freeaddrinfo(result);
			return true;
		}
		freeaddrinfo(result);
	}
	return false;
}

UINT WINAPI CUpnpSsdpServer::SsdpThread(LPVOID param)
{
	CUpnpSsdpServer* sys = (CUpnpSsdpServer*)param;
	WSAData wsaData;
	WSAStartup(MAKEWORD(2, 0), &wsaData);

	vector<string> nicList = GetNICList();
	SOCKET sockList[FD_SETSIZE];
	string debug = "SSDP watching:";
	//見つかったNIC全てで受信できるようにする
	for( size_t i = 0; i < nicList.size(); ){
		//SSDP待ち受けポート(UDP 1900)の作成
		SOCKET sock;
		if( i >= FD_SETSIZE || (sock = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET ){
			nicList.erase(nicList.begin() + i);
		}else{
			int opt = 1;
			if( setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) == SOCKET_ERROR ){
				OutputDebugString(L"SSDP setsockopt(SO_REUSEADDR) failed.\r\n");
			}
			sockaddr_in addr = {};
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = htonl(INADDR_ANY);
			addr.sin_port = htons(1900);
			ip_mreq_source mreq = {};
			mreq.imr_interface.s_addr = htonl(INADDR_ANY);
			if( GetInetAddr(&mreq.imr_sourceaddr.s_addr, nicList[i].c_str()) == false ||
			    GetInetAddr(&mreq.imr_multiaddr.s_addr, "239.255.255.250") == false ||
			    bind(sock, (const sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR ||
			    setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*)&mreq, sizeof(mreq)) == SOCKET_ERROR ){
				closesocket(sock);
				nicList.erase(nicList.begin() + i);
			}else{
				debug += ' ' + nicList[i];
				sockList[i] = sock;
				i++;
			}
		}
	}
	debug += "\r\n";
	wstring debugW;
	AtoW(debug, debugW);
	OutputDebugString(debugW.c_str());

	struct SSDP_REPLY_INFO {
		string msg;
		sockaddr_in addr;
		int addrLen;
		DWORD jitter;
	};
	vector<SSDP_REPLY_INFO> replyList;
	DWORD random = 0;
	DWORD notifyTick = GetTickCount() - 1000 * (NOTIFY_INTERVAL_SEC - NOTIFY_FIRST_DELAY_SEC);
	while( sys->stopFlag == false ){
		fd_set ready;
		FD_ZERO(&ready);
		for( size_t i = 0; i < nicList.size(); i++ ){
			FD_SET(sockList[i], &ready);
		}
		timeval to;
		to.tv_sec = replyList.empty() ? 1 : 0;
		to.tv_usec = 100000;
		if( select(0, &ready, NULL, NULL, &to) == SOCKET_ERROR ){
			break;
		}
		DWORD tick = GetTickCount();
		random = (random << 31 | random >> 1) ^ tick;
		for( size_t i = 0; i < nicList.size(); i++ ){
			if( FD_ISSET(sockList[i], &ready) ){
				char recvData[RECV_BUFF_SIZE];
				sockaddr_in from;
				int fromLen = sizeof(from);
				int recvLen = recvfrom(sockList[i], recvData, RECV_BUFF_SIZE - 1, 0, (sockaddr*)&from, &fromLen);
				if( recvLen < 0 || fromLen > sizeof(from) ){
					OutputDebugString(L"SSDP recvfrom() failed.\r\n");
				}else{
					recvData[recvLen] = '\0';
					if( strncmp(recvData, "M-SEARCH ", 9) == 0 && replyList.size() < 100 ){
						SSDP_REPLY_INFO info;
						info.msg = sys->GetMSearchReply(recvData, nicList[i].c_str());
						if( info.msg.empty() == false ){
							info.addr = from;
							info.addrLen = fromLen;
							//応答は1秒以内の揺らぎを入れる
							info.jitter = tick - random % 1000;
							replyList.push_back(info);
						}
					}
				}
			}
		}
		for( vector<SSDP_REPLY_INFO>::const_iterator itr = replyList.begin(); itr != replyList.end(); ){
			if( tick - itr->jitter > 1000 ){
				SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
				if( sock != INVALID_SOCKET ){
					if( sendto(sock, itr->msg.c_str(), (int)itr->msg.size(), 0, (const sockaddr*)&itr->addr, itr->addrLen) != (int)itr->msg.size() ){
						OutputDebugString(L"SSDP sendto() failed.\r\n");
					}
					closesocket(sock);
				}
				itr = replyList.erase(itr);
			}else{
				itr++;
			}
		}
		if( tick - notifyTick > 1000 * NOTIFY_INTERVAL_SEC ){
			notifyTick = tick;
			sys->SendNotifyAliveOrByebye(false, nicList);
		}
	}
	sys->SendNotifyAliveOrByebye(true, nicList);

	WSACleanup();
	return 0;
}

string CUpnpSsdpServer::GetMSearchReply(const char* header, const char* host) const
{
	string resMsg;
	header = strstr(header, "\r\n");
	if( header ){
		header += 2;
		string man;
		string st;
		for(;;){
			const char* tail = strstr(header, "\r\n");
			if( tail == NULL || tail == header ){
				break;
			}
			string prop;
			string val;
			Separate(string(header, tail), ":", prop, val);
			prop.erase(prop.find_last_not_of(' ') == string::npos ? 0 : prop.find_last_not_of(' ') + 1);
			prop.erase(0, prop.find_first_not_of(' '));
			val.erase(val.find_last_not_of(' ') == string::npos ? 0 : val.find_last_not_of(' ') + 1);
			val.erase(0, val.find_first_not_of(' '));
			if( CompareNoCase(prop, "MAN") == 0 ){
				man = val;
			}else if( CompareNoCase(prop, "ST") == 0 ){
				st = val;
			}
			header = tail + 2;
		}
		if( CompareNoCase(man, "\"ssdp:discover\"") == 0 ){
			for( vector<SSDP_TARGET_INFO>::const_iterator itr = this->targetList.begin(); itr != this->targetList.end(); itr++ ){
				if( CompareNoCase(itr->target, st) == 0 ){
					string location = itr->location;
					Replace(location, "$HOST$", host);
					resMsg =
						"HTTP/1.1 200 OK\r\n"
						"CACHE-CONTROL: max-age = 1800\r\n"
						"EXT:\r\n"
						"LOCATION: " + location + "\r\n"
						"SERVER: " + GetUserAgent() + "\r\n"
						"ST: " + itr->target + "\r\n"
						"USN: " + itr->usn + "\r\n\r\n";
					break;
				}
			}
		}
	}
	return resMsg;
}

void CUpnpSsdpServer::SendNotifyAliveOrByebye(bool byebyeFlag, const vector<string>& nicList)
{
	sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	GetInetAddr(&addr.sin_addr.s_addr, "239.255.255.250");
	addr.sin_port = htons(1900);
	for( size_t i = 0; i < nicList.size(); i++ ){
		SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
		if( sock != INVALID_SOCKET ){
			unsigned long ipv4;
			GetInetAddr(&ipv4, nicList[i].c_str());
			int ttl = 3;
			if( setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, (const char*)&ipv4, sizeof(ipv4)) != SOCKET_ERROR &&
			    setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (const char*)&ttl, sizeof(ttl)) != SOCKET_ERROR ){
				for( vector<SSDP_TARGET_INFO>::const_iterator itr = this->targetList.begin(); itr != this->targetList.end(); itr++ ){
					if( itr->notifyFlag ){
						string sendMsg =
							"NOTIFY * HTTP/1.1\r\n"
							"HOST: 239.255.255.250:1900\r\n";
						if( byebyeFlag ){
							sendMsg +=
								"NT: " + itr->target + "\r\n"
								"NTS: ssdp:byebye\r\n"
								"USN: " + itr->usn + "\r\n\r\n";
						}else{
							string location = itr->location;
							Replace(location, "$HOST$", nicList[i]);
							sendMsg +=
								"CACHE-CONTROL: max-age = 1800\r\n"
								"LOCATION: " + location + "\r\n"
								"NT: " + itr->target + "\r\n"
								"NTS: ssdp:alive\r\n"
								"SERVER: " + GetUserAgent() + "\r\n"
								"USN: " + itr->usn + "\r\n\r\n";
						}
						if( sendto(sock, sendMsg.c_str(), (int)sendMsg.size(), 0, (const sockaddr*)&addr, sizeof(addr)) != (int)sendMsg.size() ){
							OutputDebugString(L"SSDP sendto() failed.\r\n");
							break;
						}
					}
				}
			}
			closesocket(sock);
		}
	}
}
