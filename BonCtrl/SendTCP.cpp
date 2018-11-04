#include "stdafx.h"
#include "SendTCP.h"

bool CSendTCP::Initialize()
{
	if( m_hModule ){
		return true;
	}
	m_hModule = LoadLibrary(L"SendTSTCP.dll");
	if( m_hModule ){
		InitializeDLL pfnInitializeDLL;
		if( (pfnInitializeDLL = (InitializeDLL)GetProcAddress(m_hModule, "InitializeDLL")) != NULL &&
		    (m_pfnUnInitializeDLL = (UnInitializeDLL)GetProcAddress(m_hModule, "UnInitializeDLL")) != NULL &&
		    (m_pfnAddSendAddrDLL = (AddSendAddrDLL)GetProcAddress(m_hModule, "AddSendAddrDLL")) != NULL &&
		    (m_pfnClearSendAddrDLL = (ClearSendAddrDLL)GetProcAddress(m_hModule, "ClearSendAddrDLL")) != NULL &&
		    (m_pfnStartSendDLL = (StartSendDLL)GetProcAddress(m_hModule, "StartSendDLL")) != NULL &&
		    (m_pfnStopSendDLL = (StopSendDLL)GetProcAddress(m_hModule, "StopSendDLL")) != NULL &&
		    (m_pfnAddSendDataDLL = (AddSendDataDLL)GetProcAddress(m_hModule, "AddSendDataDLL")) != NULL &&
		    (m_pfnClearSendBuffDLL = (ClearSendBuffDLL)GetProcAddress(m_hModule, "ClearSendBuffDLL")) != NULL ){
			m_iID = pfnInitializeDLL();
			if( m_iID != -1 ){
				return true;
			}
		}
		FreeLibrary(m_hModule);
		m_hModule = NULL;
	}
	return false;
}

void CSendTCP::UnInitialize()
{
	if( m_hModule ){
		m_pfnUnInitializeDLL(m_iID);
		FreeLibrary(m_hModule);
		m_hModule = NULL;
	}
}

bool CSendTCP::AddSendAddr(LPCWSTR ip, DWORD dwPort, bool broadcastFlag)
{
	(void)broadcastFlag;
	if( m_hModule ){
		return m_pfnAddSendAddrDLL(m_iID, ip, dwPort) == TRUE;
	}
	return false;
}

void CSendTCP::ClearSendAddr()
{
	if( m_hModule ){
		m_pfnClearSendAddrDLL(m_iID);
	}
}

bool CSendTCP::StartSend()
{
	if( m_hModule ){
		return m_pfnStartSendDLL(m_iID) == TRUE;
	}
	return false;
}

void CSendTCP::StopSend()
{
	if( m_hModule ){
		m_pfnStopSendDLL(m_iID);
	}
}

bool CSendTCP::AddSendData(BYTE* pbBuff, DWORD dwSize)
{
	if( m_hModule ){
		return m_pfnAddSendDataDLL(m_iID, pbBuff, dwSize) == TRUE;
	}
	return false;
}

void CSendTCP::ClearSendBuff()
{
	if( m_hModule ){
		m_pfnClearSendBuffDLL(m_iID);
	}
}
