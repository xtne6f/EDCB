#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include "BonCtrlDef.h"
#include "../Common/StringUtil.h"

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
		SOCKET sock;
		struct sockaddr_storage addr;
		size_t addrlen;
	};
	vector<SOCKET_DATA> SockList;

	UINT m_uiSendSize;
	bool m_initialized;
	bool m_sending;
};
