#include "StdAfx.h"
#include "SendUDP.h"
#include "../Common/PathUtil.h"

static const int SNDBUF_SIZE = 3 * 1024 * 1024;

CSendUDP::CSendUDP(void)
{
	WSAData wsaData;
	WSAStartup(MAKEWORD(2,0), &wsaData);
}

CSendUDP::~CSendUDP(void)
{
	CloseUpload();

	WSACleanup();

}

BOOL CSendUDP::StartUpload( vector<NW_SEND_INFO>* List )
{
	if( SockList.size() > 0 ){
		return FALSE;
	}

	for( int i=0; i<(int)List->size(); i++ ){
		SOCKET_DATA Item;
		string ipA, strPort;
		WtoA((*List)[i].ipString, ipA);
		Format(strPort, "%d", (WORD)(*List)[i].port);
		struct addrinfo hints = {};
		hints.ai_flags = AI_NUMERICHOST;
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_protocol = IPPROTO_UDP;
		struct addrinfo* result;
		if( getaddrinfo(ipA.c_str(), strPort.c_str(), &hints, &result) != 0 ){
			continue;
		}
		Item.addrlen = min(result->ai_addrlen, sizeof(Item.addr));
		memcpy(&Item.addr, result->ai_addr, Item.addrlen);
		Item.sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		freeaddrinfo(result);
		if( Item.sock == INVALID_SOCKET ){
			CloseUpload();
			return FALSE;
		}
		//ノンブロッキングモードへ
		ULONG x = 1;
		if( ioctlsocket(Item.sock,FIONBIO, &x) == SOCKET_ERROR ||
		    setsockopt(Item.sock, SOL_SOCKET, SO_SNDBUF, (const char *)&SNDBUF_SIZE, sizeof(SNDBUF_SIZE)) == SOCKET_ERROR ){
			closesocket(Item.sock);
			CloseUpload();
			return FALSE;
		}

		if( (*List)[i].broadcastFlag ){
			BOOL b=1;
			setsockopt(Item.sock,SOL_SOCKET, SO_BROADCAST, (char *)&b, sizeof(b));
		}
		SockList.push_back(Item);
	}
	wstring iniPath;
	GetModuleIniPath(iniPath);
	m_uiSendSize = GetPrivateProfileInt(L"Set", L"UDPPacket", 128, iniPath.c_str()) * 188;

	return TRUE;
}

BOOL CSendUDP::CloseUpload()
{
	for( int i=0; i<(int)SockList.size(); i++ ){
		closesocket(SockList[i].sock);
	}
	SockList.clear();

	return TRUE;
}

void CSendUDP::SendData(BYTE* pbBuff, DWORD dwSize)
{
	for( DWORD dwRead=0; dwRead<dwSize; ){
		//ペイロード分割。BonDriver_UDPに送る場合は受信サイズ48128以下でなければならない
		int iSendSize = min(max(m_uiSendSize, 188), dwSize - dwRead);
		for( size_t i=0; i<SockList.size(); i++ ){
			int iRet = sendto(SockList[i].sock, (char*)(pbBuff + dwRead), iSendSize, 0, (struct sockaddr *)&SockList[i].addr, (int)SockList[i].addrlen);
			if( iRet == SOCKET_ERROR ){
				if( WSAGetLastError() == WSAEWOULDBLOCK ){
					//送信処理が追いつかずSNDBUF_SIZEで指定したバッファも尽きてしまった
					//帯域が足りないときはどう足掻いてもドロップするしかないので、Sleep()によるフロー制御はしない
					OutputDebugString(L"Dropped\r\n");
				}
			}
		}
		dwRead += iSendSize;
	}
}