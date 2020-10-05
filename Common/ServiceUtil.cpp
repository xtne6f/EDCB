#include "stdafx.h"
#include "ServiceUtil.h"
#include <winsvc.h>

BOOL IsInstallService(LPCWSTR lpcwszName)
{
	SC_HANDLE hScm = NULL;
	SC_HANDLE hSrv = NULL;
	hScm = OpenSCManagerW(0, 0, SC_MANAGER_CONNECT);
	if(hScm == NULL){
		AddDebugLog(L"OpenSCManager failed");
		return FALSE;
	}
	hSrv = OpenServiceW(hScm, lpcwszName, SERVICE_QUERY_STATUS);
	if(hSrv == NULL){
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
	SC_HANDLE hScm = NULL;
	SC_HANDLE hSrv = NULL;
	hScm = OpenSCManagerW(0, 0, SC_MANAGER_CONNECT);
	if(hScm == NULL){
		AddDebugLog(L"OpenSCManager failed");
		return FALSE;
	}
	hSrv = OpenServiceW(hScm, lpcwszName, SERVICE_QUERY_STATUS);
	if(hSrv == NULL){
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
