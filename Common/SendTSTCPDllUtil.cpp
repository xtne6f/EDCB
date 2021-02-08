#include "stdafx.h"
#include "SendTSTCPDllUtil.h"
#include "PathUtil.h"
#ifndef _WIN32
#include "StringUtil.h"
#include <dlfcn.h>
#endif

bool CSendTSTCPDllUtil::Initialize()
{
	if( m_hModule ){
		return true;
	}
#ifdef _WIN32
	m_hModule = LoadLibrary(GetModulePath().replace_filename(L"SendTSTCP.dll").c_str());
	auto getProcAddr = [=](const char* name) { return GetProcAddress((HMODULE)m_hModule, name); };
#else
	string strPath;
	WtoUTF8(GetModulePath().replace_filename(L"SendTSTCP.so").native(), strPath);
	m_hModule = dlopen(strPath.c_str(), RTLD_LAZY);
	auto getProcAddr = [=](const char* name) { return dlsym(m_hModule, name); };
#endif
	if( m_hModule ){
		InitializeDLL pfnInitializeDLL;
		if( (pfnInitializeDLL = (InitializeDLL)getProcAddr("InitializeDLL")) != NULL &&
		    (m_pfnUnInitializeDLL = (UnInitializeDLL)getProcAddr("UnInitializeDLL")) != NULL &&
		    (m_pfnAddSendAddrDLL = (AddSendAddrDLL)getProcAddr("AddSendAddrDLL")) != NULL &&
		    (m_pfnClearSendAddrDLL = (ClearSendAddrDLL)getProcAddr("ClearSendAddrDLL")) != NULL &&
		    (m_pfnStartSendDLL = (StartSendDLL)getProcAddr("StartSendDLL")) != NULL &&
		    (m_pfnStopSendDLL = (StopSendDLL)getProcAddr("StopSendDLL")) != NULL &&
		    (m_pfnAddSendDataDLL = (AddSendDataDLL)getProcAddr("AddSendDataDLL")) != NULL &&
		    (m_pfnClearSendBuffDLL = (ClearSendBuffDLL)getProcAddr("ClearSendBuffDLL")) != NULL ){
			m_pfnAddSendAddrUdpDLL = (AddSendAddrUdpDLL)getProcAddr("AddSendAddrUdpDLL");
			m_iID = pfnInitializeDLL();
			if( m_iID != -1 ){
				return true;
			}
		}
#ifdef _WIN32
		FreeLibrary((HMODULE)m_hModule);
#else
		dlclose(m_hModule);
#endif
		m_hModule = NULL;
	}
	return false;
}

void CSendTSTCPDllUtil::UnInitialize()
{
	if( m_hModule ){
		m_pfnUnInitializeDLL(m_iID);
#ifdef _WIN32
		FreeLibrary((HMODULE)m_hModule);
#else
		dlclose(m_hModule);
#endif
		m_hModule = NULL;
	}
}

bool CSendTSTCPDllUtil::AddSendAddr(LPCWSTR ip, DWORD dwPort)
{
	if( m_hModule ){
		return m_pfnAddSendAddrDLL(m_iID, ip, dwPort) == TRUE;
	}
	return false;
}

bool CSendTSTCPDllUtil::AddSendAddrUdp(LPCWSTR ip, DWORD dwPort, bool broadcastFlag, int maxSendSize)
{
	if( m_hModule && m_pfnAddSendAddrUdpDLL ){
		return m_pfnAddSendAddrUdpDLL(m_iID, ip, dwPort, broadcastFlag, maxSendSize) == TRUE;
	}
	return false;
}

void CSendTSTCPDllUtil::ClearSendAddr()
{
	if( m_hModule ){
		m_pfnClearSendAddrDLL(m_iID);
	}
}

bool CSendTSTCPDllUtil::StartSend()
{
	if( m_hModule ){
		return m_pfnStartSendDLL(m_iID) == TRUE;
	}
	return false;
}

void CSendTSTCPDllUtil::StopSend()
{
	if( m_hModule ){
		m_pfnStopSendDLL(m_iID);
	}
}

bool CSendTSTCPDllUtil::AddSendData(BYTE* pbBuff, DWORD dwSize)
{
	if( m_hModule ){
		return m_pfnAddSendDataDLL(m_iID, pbBuff, dwSize) == TRUE;
	}
	return false;
}

void CSendTSTCPDllUtil::ClearSendBuff()
{
	if( m_hModule ){
		m_pfnClearSendBuffDLL(m_iID);
	}
}
