#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include "BonCtrlDef.h"
#include "../Common/StringUtil.h"

class CSendUDP
{
public:
	CSendUDP(void);
	~CSendUDP(void);

	BOOL StartUpload( vector<NW_SEND_INFO>* List );
	void SendData(BYTE* pbBuff, DWORD dwSize);
	BOOL CloseUpload();

protected:
	struct SOCKET_DATA {
		SOCKET sock;
		struct sockaddr_storage addr;
		size_t addrlen;
	};
	vector<SOCKET_DATA> SockList;

	UINT m_uiSendSize;

};
