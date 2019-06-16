#pragma once

#include "BonCtrlDef.h"

//final
class CSendTCP : public CSendNW
{
public:
	CSendTCP() : m_hModule(NULL) {}
	~CSendTCP() { UnInitialize(); }
	bool Initialize();
	void UnInitialize();
	bool IsInitialized() const { return m_hModule != NULL; }
	bool AddSendAddr(LPCWSTR ip, DWORD dwPort, bool broadcastFlag);
	void ClearSendAddr();
	bool StartSend();
	void StopSend();
	bool AddSendData(BYTE* pbBuff, DWORD dwSize);
	void ClearSendBuff();

private:
	typedef int (WINAPI *InitializeDLL)();
	typedef DWORD (WINAPI *UnInitializeDLL)(int iID);
	typedef DWORD (WINAPI *AddSendAddrDLL)(int iID, LPCWSTR lpcwszIP, DWORD dwPort);
	typedef DWORD (WINAPI *ClearSendAddrDLL)(int iID);
	typedef DWORD (WINAPI *StartSendDLL)(int iID);
	typedef DWORD (WINAPI *StopSendDLL)(int iID);
	typedef DWORD (WINAPI *AddSendDataDLL)(int iID, BYTE* pbData, DWORD dwSize);
	typedef DWORD (WINAPI *ClearSendBuffDLL)(int iID);

	void* m_hModule;
	int m_iID;
	UnInitializeDLL m_pfnUnInitializeDLL;
	AddSendAddrDLL m_pfnAddSendAddrDLL;
	ClearSendAddrDLL m_pfnClearSendAddrDLL;
	StartSendDLL m_pfnStartSendDLL;
	StopSendDLL m_pfnStopSendDLL;
	AddSendDataDLL m_pfnAddSendDataDLL;
	ClearSendBuffDLL m_pfnClearSendBuffDLL;
};
