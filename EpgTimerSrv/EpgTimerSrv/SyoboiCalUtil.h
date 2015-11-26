#pragma once

#include "../../Common/StringUtil.h"
#include "../../Common/StructDef.h"
#include "../../Common/WinHTTPUtil.h"

class CSyoboiCalUtil
{
public:
	CSyoboiCalUtil(void);
	~CSyoboiCalUtil(void);

	BOOL SendReserve(vector<RESERVE_DATA>* reserveList, vector<TUNER_RESERVE_INFO>* tunerList);

protected:
	wstring id;
	wstring pass;

	USE_PROXY_INFO* proxyInfo;

protected:
	BOOL Base64Enc(LPCSTR src, DWORD srcSize, LPWSTR dest, DWORD* destSize);
	__int64 GetTimeStamp(SYSTEMTIME startTime);
	BOOL UrlEncodeUTF8(LPCWSTR src, DWORD srcSize, wstring& dest);
	BOOL UrlEncodeUTF8(LPCWSTR src, DWORD srcSize, string& dest);
};


