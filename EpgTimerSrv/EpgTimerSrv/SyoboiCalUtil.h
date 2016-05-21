#pragma once

#include "../../Common/StructDef.h"

class CSyoboiCalUtil
{
public:
	BOOL SendReserve(const vector<RESERVE_DATA>* reserveList, const vector<TUNER_RESERVE_INFO>* tunerList);

protected:
	BOOL Base64Enc(LPCSTR src, DWORD srcSize, LPWSTR dest, DWORD* destSize);
	__int64 GetTimeStamp(SYSTEMTIME startTime);
	BOOL UrlEncodeUTF8(LPCWSTR src, DWORD srcSize, string& dest);
};


