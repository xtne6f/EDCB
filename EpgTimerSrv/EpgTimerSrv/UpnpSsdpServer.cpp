#include "stdafx.h"
#include "UpnpSsdpServer.h"
#include "../../Common/StringUtil.h"
#include "../../Common/TimeUtil.h"
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#else
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

CUpnpSsdpServer::~CUpnpSsdpServer()
{
	Stop();
}

bool CUpnpSsdpServer::Start(const vector<SSDP_TARGET_INFO>& targetList_, int ifTypes, int initialWaitSec_)
{
	Stop();
	this->targetList = targetList_;
	this->ssdpIfTypes = ifTypes;
	this->initialWaitSec = initialWaitSec_;
	this->stopFlag = false;
	this->ssdpThread = thread_(SsdpThread, this);
	return true;
}

void CUpnpSsdpServer::Stop()
{
	if( this->ssdpThread.joinable() ){
		this->stopFlag = true;
		this->ssdpThread.join();
	}
}

namespace
{
struct SSDP_NIC_INFO {
	sockaddr_in addr;
	string name;
	SOCKET sock;
};

vector<SSDP_NIC_INFO> GetNICList()
{
	vector<SSDP_NIC_INFO> nicList;
#ifdef _WIN32
	//MSDN: "The recommended method ~ is to pre-allocate a 15KB working buffer pointed to by the AdapterAddresses parameter."
	IP_ADAPTER_ADDRESSES adpts[16384 / sizeof(IP_ADAPTER_ADDRESSES)];
	ULONG len = sizeof(adpts);
	if( GetAdaptersAddresses(AF_INET, 0, 0, adpts, &len) == ERROR_SUCCESS ){
		for( PIP_ADAPTER_ADDRESSES adpt = adpts; adpt; adpt = adpt->Next ){
			for( PIP_ADAPTER_UNICAST_ADDRESS uni = adpt->FirstUnicastAddress; uni; uni = uni->Next ){
				char host[NI_MAXHOST];
				if( (uni->Flags & IP_ADAPTER_ADDRESS_TRANSIENT) == 0 &&
				    uni->Address.lpSockaddr->sa_family == AF_INET &&
				    getnameinfo(uni->Address.lpSockaddr, uni->Address.iSockaddrLength, host, sizeof(host), NULL, 0, NI_NUMERICHOST) == 0 ){
					nicList.resize(nicList.size() + 1);
					nicList.back().addr = *(sockaddr_in*)uni->Address.lpSockaddr;
					nicList.back().name = host;
				}
			}
		}
	}
#else
	ifaddrs* ifap;
	if( getifaddrs(&ifap) == 0 ){
		for( ifaddrs* p = ifap; p; p = p->ifa_next ){
			char host[NI_MAXHOST];
			if( p->ifa_addr->sa_family == AF_INET &&
			    getnameinfo(p->ifa_addr, sizeof(sockaddr_in), host, sizeof(host), NULL, 0, NI_NUMERICHOST) == 0 ){
				nicList.resize(nicList.size() + 1);
				nicList.back().addr = *(sockaddr_in*)p->ifa_addr;
				nicList.back().name = host;
			}
		}
		freeifaddrs(ifap);
	}
#endif
	return nicList;
}

string GetMSearchReply(const char* header, const char* host, const vector<CUpnpSsdpServer::SSDP_TARGET_INFO>& targetList);
void SendNotifyAliveOrByebye(bool byebyeFlag, const vector<SSDP_NIC_INFO>& nicList, const vector<CUpnpSsdpServer::SSDP_TARGET_INFO>& targetList);
}

void CUpnpSsdpServer::SsdpThread(CUpnpSsdpServer* sys)
{
	for( int i = 0; i < sys->initialWaitSec; i++ ){
		SleepForMsec(1000);
		if( sys->stopFlag ){
			return;
		}
	}
#ifdef _WIN32
	WSAData wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

	vector<SSDP_NIC_INFO> nicList = GetNICList();
	string debug = "SSDP watching:";
	//見つかったNIC全てで受信できるようにする
	for( size_t i = 0; i < nicList.size(); ){
		int ifType = ntohl(nicList[i].addr.sin_addr.s_addr) == INADDR_LOOPBACK ? SSDP_IF_LOOPBACK :
		             (ntohl(nicList[i].addr.sin_addr.s_addr) >> 16) == 0xC0A8 ? SSDP_IF_C_PRIVATE :
		             (ntohl(nicList[i].addr.sin_addr.s_addr) >> 20) == 0xAC1 ? SSDP_IF_B_PRIVATE :
		             (ntohl(nicList[i].addr.sin_addr.s_addr) >> 24) == 0x0A ? SSDP_IF_A_PRIVATE :
		             (ntohl(nicList[i].addr.sin_addr.s_addr) >> 16) == 0xA9FE ? SSDP_IF_LINKLOCAL : SSDP_IF_GLOBAL;
		//SSDP待ち受けポート(UDP 1900)の作成
		SOCKET sock = INVALID_SOCKET;
		if( (sys->ssdpIfTypes & ifType) != 0 ){
#ifdef _WIN32
			sock = socket(AF_INET, SOCK_DGRAM, 0);
			if( sock != INVALID_SOCKET ){
				//ノンブロッキングモードへ
				unsigned long x = 1;
				if( ioctlsocket(sock, FIONBIO, &x) != 0 ){
					closesocket(sock);
					sock = INVALID_SOCKET;
				}
			}
#else
			sock = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
#endif
		}
		if( sock == INVALID_SOCKET ){
			nicList.erase(nicList.begin() + i);
		}else{
			int opt = 1;
			if( setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) != 0 ){
				AddDebugLog(L"SSDP setsockopt(SO_REUSEADDR) failed.");
			}
			nicList[i].addr.sin_port = htons(1900);
			ip_mreq mreq = {};
			mreq.imr_multiaddr.s_addr = htonl(0xEFFFFFFA); //239.255.255.250
			mreq.imr_interface = nicList[i].addr.sin_addr;
			if( bind(sock, (const sockaddr*)&nicList[i].addr, sizeof(nicList[i].addr)) != 0 ||
			    setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*)&mreq, sizeof(mreq)) != 0 ){
				closesocket(sock);
				nicList.erase(nicList.begin() + i);
			}else{
				debug += ' ' + nicList[i].name;
				nicList[i].sock = sock;
				i++;
			}
		}
	}
	wstring debugW;
	UTF8toW(debug, debugW);
	AddDebugLogFormat(L"%ls", debugW.c_str());

	struct SSDP_REPLY_INFO {
		string msg;
		sockaddr_in addr;
		DWORD jitter;
	};
	vector<SSDP_REPLY_INFO> replyList;
	DWORD random = 0;
	DWORD notifyTick = GetU32Tick() - 1000 * NOTIFY_INTERVAL_SEC;
	while( sys->stopFlag == false ){
		fd_set ready;
		FD_ZERO(&ready);
		int maxfd = -1;
		for( size_t i = 0; i < nicList.size(); i++ ){
#ifdef _WIN32
			maxfd++;
#else
			maxfd = max(maxfd, nicList[i].sock);
#endif
			if( maxfd < FD_SETSIZE ){
				FD_SET(nicList[i].sock, &ready);
			}
		}
		timeval to = {};
		to.tv_usec = replyList.empty() ? 500000 : 100000;
		if( maxfd >= FD_SETSIZE || select(maxfd + 1, &ready, NULL, NULL, &to) < 0 ){
			SleepForMsec(replyList.empty() ? 500 : 100);
		}
		DWORD tick = GetU32Tick();
		random = (random << 31 | random >> 1) ^ tick;
		for( size_t i = 0; i < nicList.size(); i++ ){
			char recvData[RECV_BUFF_SIZE];
			SSDP_REPLY_INFO info;
#ifdef _WIN32
			int fromLen = sizeof(info.addr);
#else
			socklen_t fromLen = sizeof(info.addr);
#endif
			int recvLen = (int)recvfrom(nicList[i].sock, recvData, RECV_BUFF_SIZE - 1, 0, (sockaddr*)&info.addr, &fromLen);
			if( recvLen >= 0 ){
				if( fromLen != sizeof(info.addr) ){
					AddDebugLog(L"SSDP recvfrom() failed.");
				}else{
					recvData[recvLen] = '\0';
					if( strncmp(recvData, "M-SEARCH ", 9) == 0 && replyList.size() < 100 ){
						info.msg = GetMSearchReply(recvData, nicList[i].name.c_str(), sys->targetList);
						if( info.msg.empty() == false ){
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
				SOCKET sock = socket(AF_INET, SOCK_DGRAM
#ifndef _WIN32
				                         | SOCK_CLOEXEC
#endif
				                     , 0);
				if( sock != INVALID_SOCKET ){
					if( sendto(sock, itr->msg.c_str(), (int)itr->msg.size(), 0, (const sockaddr*)&itr->addr, sizeof(itr->addr)) != (int)itr->msg.size() ){
						AddDebugLog(L"SSDP sendto() failed.");
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
			SendNotifyAliveOrByebye(false, nicList, sys->targetList);
		}
	}
	SendNotifyAliveOrByebye(true, nicList, sys->targetList);

	while( nicList.empty() == false ){
		closesocket(nicList.back().sock);
		nicList.pop_back();
	}
#ifdef _WIN32
	WSACleanup();
#endif
}

namespace
{
string GetMSearchReply(const char* header, const char* host, const vector<CUpnpSsdpServer::SSDP_TARGET_INFO>& targetList)
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
			string val(header, tail);
			if( val.find(':') == string::npos ){
				val.clear();
			}else{
				prop.assign(val, 0, val.find(':'));
				val.erase(0, prop.size() + 1);
			}
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
			for( vector<CUpnpSsdpServer::SSDP_TARGET_INFO>::const_iterator itr = targetList.begin(); itr != targetList.end(); itr++ ){
				if( CompareNoCase(itr->target, st) == 0 ){
					string location = "http://" + string(host) + itr->location;
					resMsg =
						"HTTP/1.1 200 OK\r\n"
						"CACHE-CONTROL: max-age = 1800\r\n"
						"EXT:\r\n"
						"LOCATION: " + location + "\r\n"
						"SERVER: UnknownOS/1.0 UPnP/1.1 EpgTimerSrv/0.10\r\n"
						"ST: " + itr->target + "\r\n"
						"USN: " + itr->usn + "\r\n\r\n";
					break;
				}
			}
		}
	}
	return resMsg;
}

void SendNotifyAliveOrByebye(bool byebyeFlag, const vector<SSDP_NIC_INFO>& nicList, const vector<CUpnpSsdpServer::SSDP_TARGET_INFO>& targetList)
{
	sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(0xEFFFFFFA); //239.255.255.250
	addr.sin_port = htons(1900);
	for( size_t i = 0; i < nicList.size(); i++ ){
		SOCKET sock = socket(AF_INET, SOCK_DGRAM
#ifndef _WIN32
		                         | SOCK_CLOEXEC
#endif
		                     , 0);
		if( sock != INVALID_SOCKET ){
			int ttl = 3;
			if( setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, (const char*)&nicList[i].addr.sin_addr, sizeof(nicList[i].addr.sin_addr)) == 0 &&
			    setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (const char*)&ttl, sizeof(ttl)) == 0 ){
				for( vector<CUpnpSsdpServer::SSDP_TARGET_INFO>::const_iterator itr = targetList.begin(); itr != targetList.end(); itr++ ){
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
							string location = "http://" + nicList[i].name + itr->location;
							sendMsg +=
								"CACHE-CONTROL: max-age = 1800\r\n"
								"LOCATION: " + location + "\r\n"
								"NT: " + itr->target + "\r\n"
								"NTS: ssdp:alive\r\n"
								"SERVER: UnknownOS/1.0 UPnP/1.1 EpgTimerSrv/0.10\r\n"
								"USN: " + itr->usn + "\r\n\r\n";
						}
						if( sendto(sock, sendMsg.c_str(), (int)sendMsg.size(), 0, (const sockaddr*)&addr, sizeof(addr)) != (int)sendMsg.size() ){
							AddDebugLog(L"SSDP sendto() failed.");
							break;
						}
					}
				}
			}
			closesocket(sock);
		}
	}
}
}
