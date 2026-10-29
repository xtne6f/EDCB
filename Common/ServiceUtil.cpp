#include "stdafx.h"
#include "ServiceUtil.h"
#include <winsvc.h>

BOOL IsInstallService(LPCWSTR lpcwszName)
{
	SC_HANDLE hScm = nullptr;
	SC_HANDLE hSrv = nullptr;
	hScm = OpenSCManagerW(0, 0, SC_MANAGER_CONNECT);
	if(hScm == nullptr){
		AddDebugLog(L"OpenSCManager failed");
		return FALSE;
	}
	hSrv = OpenServiceW(hScm, lpcwszName, SERVICE_QUERY_STATUS);
	if(hSrv == nullptr){
		AddDebugLog(L"OpenService failed");
		CloseServiceHandle(hScm);
		return FALSE;
	}
	CloseServiceHandle(hSrv);
	CloseServiceHandle(hScm);

	return TRUE;
}

BOOL IsStopService(LPCWSTR lpcwszName)
{
	BOOL bRet = FALSE;
	SC_HANDLE hScm = nullptr;
	SC_HANDLE hSrv = nullptr;
	hScm = OpenSCManagerW(0, 0, SC_MANAGER_CONNECT);
	if(hScm == nullptr){
		AddDebugLog(L"OpenSCManager failed");
		return FALSE;
	}
	hSrv = OpenServiceW(hScm, lpcwszName, SERVICE_QUERY_STATUS);
	if(hSrv == nullptr){
		AddDebugLog(L"OpenService failed");
		CloseServiceHandle(hScm);
		return FALSE;
	}
	SERVICE_STATUS stStatus;
	if( QueryServiceStatus(hSrv, &stStatus) != FALSE ){
		if(stStatus.dwCurrentState == SERVICE_STOPPED){
			bRet = TRUE;
		}
	}else{
		AddDebugLog(L"QueryServiceStatus failed");
	}

	CloseServiceHandle(hSrv);
	CloseServiceHandle(hScm);

	return bRet;
}
