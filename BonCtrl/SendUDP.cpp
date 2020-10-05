#include "stdafx.h"
#include "SendUDP.h"
#include "../Common/PathUtil.h"
#include "../Common/StringUtil.h"

static const int SNDBUF_SIZE = 3 * 1024 * 1024;

bool CSendUDP::Initialize()
{
	if( m_initialized ){
		return true;
	}
#ifdef _WIN32
	WSAData wsaData;
	if( WSAStartup(MAKEWORD(2, 2), &wsaData) != 0 ){
		return false;
	}
#endif
	m_initialized = true;
	m_sending = false;
	return true;
}

void CSendUDP::UnInitialize()
{
	if( m_initialized ){
		ClearSendAddr();
#ifdef _WIN32
		WSACleanup();
#endif
		m_initialized = false;
	}
}

bool CSendUDP::AddSendAddr(LPCWSTR ip, DWORD dwPort, bool broadcastFlag)
{
	if( m_initialized ){
		SOCKET_DATA Item;
		string ipA;
		WtoUTF8(ip, ipA);
		char szPort[16];
		sprintf_s(szPort, "%d", (WORD)dwPort);
		struct addrinfo hints = {};
		hints.ai_flags = AI_NUMERICHOST;
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_protocol = IPPROTO_UDP;
		struct addrinfo* result;
		if( getaddrinfo(ipA.c_str(), szPort, &hints, &result) != 0 ){
			return false;
		}
		Item.addrlen = min((size_t)result->ai_addrlen, sizeof(Item.addr));
		memcpy(&Item.addr, result->ai_addr, Item.addrlen);
		Item.sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		freeaddrinfo(result);
#ifdef _WIN32
		if( Item.sock == INVALID_SOCKET ){
#else
		if( Item.sock < 0 ){
#endif
			return false;
		}
		//ノンブロッキングモードへ
#ifdef _WIN32
		ULONG x = 1;
		if( ioctlsocket(Item.sock, FIONBIO, &x) != 0 ||
		    setsockopt(Item.sock, SOL_SOCKET, SO_SNDBUF, (const char *)&SNDBUF_SIZE, sizeof(SNDBUF_SIZE)) != 0 ){
			closesocket(Item.sock);
#else
		int x = 1;
		if( ioctl(Item.sock, FIONBIO, &x) != 0 ||
		    setsockopt(Item.sock, SOL_SOCKET, SO_SNDBUF, (const char *)&SNDBUF_SIZE, sizeof(SNDBUF_SIZE)) != 0 ){
			close(Item.sock);
#endif
			return false;
		}

		if( broadcastFlag ){
			BOOL b=1;
			setsockopt(Item.sock,SOL_SOCKET, SO_BROADCAST, (char *)&b, sizeof(b));
		}
		m_sockList.push_back(Item);
		return true;
	}
	return false;
}

void CSendUDP::ClearSendAddr()
{
	while( m_sockList.empty() == false ){
#ifdef _WIN32
		ULONG x = 0;
		ioctlsocket(m_sockList.back().sock, FIONBIO, &x);
		closesocket(m_sockList.back().sock);
#else
		int x = 0;
		ioctl(m_sockList.back().sock, FIONBIO, &x);
		close(m_sockList.back().sock);
#endif
		m_sockList.pop_back();
	}
}

bool CSendUDP::StartSend()
{
	if( m_initialized ){
		m_sendSize = GetPrivateProfileInt(L"SET", L"UDPPacket", 128, GetModuleIniPath().c_str()) * 188;
		m_sending = true;
		return true;
	}
	return false;
}

bool CSendUDP::AddSendData(BYTE* pbBuff, DWORD dwSize)
{
	if( m_initialized == false || m_sending == false ){
		return false;
	}
	for( DWORD dwRead=0; dwRead<dwSize; ){
		//ペイロード分割。BonDriver_UDPに送る場合は受信サイズ48128以下でなければならない
		int iSendSize = (int)min((DWORD)max(m_sendSize, 188), dwSize - dwRead);
		for( size_t i=0; i<m_sockList.size(); i++ ){
			int iRet = (int)sendto(m_sockList[i].sock, (char*)(pbBuff + dwRead), iSendSize, 0, (sockaddr*)&m_sockList[i].addr, (int)m_sockList[i].addrlen);
			if( iRet < 0 ){
#ifdef _WIN32
				if( WSAGetLastError() == WSAEWOULDBLOCK ){
#else
				if( errno == EAGAIN || errno == EWOULDBLOCK ){
#endif
					//送信処理が追いつかずSNDBUF_SIZEで指定したバッファも尽きてしまった
					//帯域が足りないときはどう足掻いてもドロップするしかないので、Sleep()によるフロー制御はしない
					AddDebugLog(L"Dropped");
				}
			}
		}
		dwRead += iSendSize;
	}
	return true;
}
