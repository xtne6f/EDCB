#include "stdafx.h"
#include "SendTSTCPDllUtil.h"
#include "PathUtil.h"

CSendTSTCPDllUtil::CSendTSTCPDllUtil()
	: m_module(NULL, UtilFreeLibrary)
{
}

bool CSendTSTCPDllUtil::Initialize()
{
	if( m_module ){
		return true;
	}
	m_module.reset(UtilLoadLibrary(
#ifdef EDCB_LIB_ROOT
		fs_path(EDCB_LIB_ROOT).append(
#else
		GetModulePath().replace_filename(
#endif
		L"SendTSTCP" EDCB_LIB_EXT
		)));
	if( m_module ){
		InitializeDLL pfnInitializeDLL;
		if( UtilGetProcAddress(m_module.get(), "InitializeDLL", pfnInitializeDLL) &&
		    UtilGetProcAddress(m_module.get(), "UnInitializeDLL", m_pfnUnInitializeDLL) &&
		    UtilGetProcAddress(m_module.get(), "AddSendAddrDLL", m_pfnAddSendAddrDLL) &&
		    UtilGetProcAddress(m_module.get(), "ClearSendAddrDLL", m_pfnClearSendAddrDLL) &&
		    UtilGetProcAddress(m_module.get(), "StartSendDLL", m_pfnStartSendDLL) &&
		    UtilGetProcAddress(m_module.get(), "StopSendDLL", m_pfnStopSendDLL) &&
		    UtilGetProcAddress(m_module.get(), "AddSendDataDLL", m_pfnAddSendDataDLL) &&
		    UtilGetProcAddress(m_module.get(), "ClearSendBuffDLL", m_pfnClearSendBuffDLL) ){
			UtilGetProcAddress(m_module.get(), "AddSendAddrUdpDLL", m_pfnAddSendAddrUdpDLL);
			m_iID = pfnInitializeDLL();
			if( m_iID != -1 ){
				return true;
			}
		}
		m_module.reset();
	}
	return false;
}

void CSendTSTCPDllUtil::UnInitialize()
{
	if( m_module ){
		m_pfnUnInitializeDLL(m_iID);
		m_module.reset();
	}
}

bool CSendTSTCPDllUtil::AddSendAddr(LPCWSTR ip, DWORD dwPort)
{
	if( m_module ){
		return m_pfnAddSendAddrDLL(m_iID, ip, dwPort) == TRUE;
	}
	return false;
}

bool CSendTSTCPDllUtil::AddSendAddrUdp(LPCWSTR ip, DWORD dwPort, bool broadcastFlag, int maxSendSize)
{
	if( m_module && m_pfnAddSendAddrUdpDLL ){
		return m_pfnAddSendAddrUdpDLL(m_iID, ip, dwPort, broadcastFlag, maxSendSize) == TRUE;
	}
	return false;
}

void CSendTSTCPDllUtil::ClearSendAddr()
{
	if( m_module ){
		m_pfnClearSendAddrDLL(m_iID);
	}
}

bool CSendTSTCPDllUtil::StartSend()
{
	if( m_module ){
		return m_pfnStartSendDLL(m_iID) == TRUE;
	}
	return false;
}

void CSendTSTCPDllUtil::StopSend()
{
	if( m_module ){
		m_pfnStopSendDLL(m_iID);
	}
}

bool CSendTSTCPDllUtil::AddSendData(BYTE* pbBuff, DWORD dwSize)
{
	if( m_module ){
		return m_pfnAddSendDataDLL(m_iID, pbBuff, dwSize) == TRUE;
	}
	return false;
}

void CSendTSTCPDllUtil::ClearSendBuff()
{
	if( m_module ){
		m_pfnClearSendBuffDLL(m_iID);
	}
}
