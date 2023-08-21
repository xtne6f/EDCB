#pragma once

class CSendTSTCPDllUtil
{
public:
	CSendTSTCPDllUtil();
	~CSendTSTCPDllUtil() { UnInitialize(); }
	bool Initialize();
	void UnInitialize();
	bool IsInitialized() const { return m_module != NULL; }
	bool AddSendAddr(LPCWSTR ip, DWORD dwPort);
	bool AddSendAddrUdp(LPCWSTR ip, DWORD dwPort, bool broadcastFlag, int maxSendSize);
	void ClearSendAddr();
	bool StartSend();
	void StopSend();
	bool AddSendData(BYTE* pbBuff, DWORD dwSize);
	void ClearSendBuff();

private:
	typedef int (WINAPI *InitializeDLL)();
	typedef DWORD (WINAPI *UnInitializeDLL)(int iID);
	typedef DWORD (WINAPI *AddSendAddrDLL)(int iID, LPCWSTR lpcwszIP, DWORD dwPort);
	typedef DWORD (WINAPI *AddSendAddrUdpDLL)(int iID, LPCWSTR lpcwszIP, DWORD dwPort, BOOL broadcastFlag, int maxSendSize);
	typedef DWORD (WINAPI *ClearSendAddrDLL)(int iID);
	typedef DWORD (WINAPI *StartSendDLL)(int iID);
	typedef DWORD (WINAPI *StopSendDLL)(int iID);
	typedef DWORD (WINAPI *AddSendDataDLL)(int iID, BYTE* pbData, DWORD dwSize);
	typedef DWORD (WINAPI *ClearSendBuffDLL)(int iID);

	std::unique_ptr<void, void(*)(void*)> m_module;
	int m_iID;
	UnInitializeDLL m_pfnUnInitializeDLL;
	AddSendAddrDLL m_pfnAddSendAddrDLL;
	AddSendAddrUdpDLL m_pfnAddSendAddrUdpDLL;
	ClearSendAddrDLL m_pfnClearSendAddrDLL;
	StartSendDLL m_pfnStartSendDLL;
	StopSendDLL m_pfnStopSendDLL;
	AddSendDataDLL m_pfnAddSendDataDLL;
	ClearSendBuffDLL m_pfnClearSendBuffDLL;
};
