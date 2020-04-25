#pragma once

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netdb.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#endif
#include "BonCtrlDef.h"

//final
class CSendUDP : public CSendNW
{
public:
	CSendUDP() : m_initialized(false) {}
	~CSendUDP() { UnInitialize(); }
	bool Initialize();
	void UnInitialize();
	bool IsInitialized() const { return m_initialized; }
	bool AddSendAddr(LPCWSTR ip, DWORD dwPort, bool broadcastFlag);
	void ClearSendAddr();
	bool StartSend();
	void StopSend() { m_sending = false; }
	bool AddSendData(BYTE* pbBuff, DWORD dwSize);

private:
	struct SOCKET_DATA {
#ifdef _WIN32
		SOCKET sock;
#else
		int sock;
#endif
		struct sockaddr_storage addr;
		size_t addrlen;
	};
	vector<SOCKET_DATA> m_sockList;

	int m_sendSize;
	bool m_initialized;
	bool m_sending;
};
