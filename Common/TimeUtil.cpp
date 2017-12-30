#include "stdafx.h"
#include "TimeUtil.h"

LPCWSTR GetDayOfWeekName( WORD wDayOfWeek )
{
	static const WCHAR name[7][2] = {
		L"ì˙", L"åé", L"âŒ", L"êÖ", L"ñÿ", L"ã‡", L"ìy"
	};
	return name[wDayOfWeek % 7];
}

__int64 GetNowI64Time()
{
	FILETIME fTime;
	GetSystemTimeAsFileTime(&fTime);
	return ((__int64)fTime.dwHighDateTime << 32 | fTime.dwLowDateTime) + I64_UTIL_TIMEZONE;
}

__int64 ConvertI64Time(SYSTEMTIME Time)
{
	FILETIME fTime;
	__int64 i64Time = 0;
	if( SystemTimeToFileTime( &Time, &fTime ) ){
		i64Time = ((__int64)fTime.dwHighDateTime)<<32 | fTime.dwLowDateTime;
	}

	return i64Time;
}

BOOL ConvertSystemTime( __int64 i64Time, SYSTEMTIME* Time )
{
	if( Time != NULL ){
		FILETIME fTime;
		fTime.dwHighDateTime = (DWORD)(i64Time>>32);
		fTime.dwLowDateTime = (DWORD)(i64Time&0x00000000FFFFFFFF);
		if( FileTimeToSystemTime(&fTime,Time) ){
			return TRUE;
		}
		SYSTEMTIME sTime = {};
		*Time = sTime;
	}
	return FALSE;
}

LPCSTR GetTimeMacroName(int index)
{
	static const LPCSTR name[22] = {
		"DYYYY", "DYY", "DMM", "DM", "DDD", "DD", "DW", "THH", "TH", "TMM", "TM", "TSS", "TS",
		"DYYYY28", "DYY28", "DMM28", "DM28", "DDD28", "DD28", "DW28", "THH28", "TH28"
	};
	return index < 0 || index > 21 ? NULL : name[index];
}

wstring GetTimeMacroValue(int index, SYSTEMTIME Time)
{
	wstring val;
	if( index == 0 ) Format(val, L"%04d", Time.wYear);
	else if( index == 1 ) Format(val, L"%02d", Time.wYear % 100);
	else if( index == 2 ) Format(val, L"%02d", Time.wMonth);
	else if( index == 3 ) Format(val, L"%d", Time.wMonth);
	else if( index == 4 ) Format(val, L"%02d", Time.wDay);
	else if( index == 5 ) Format(val, L"%d", Time.wDay);
	else if( index == 6 ) val = GetDayOfWeekName(Time.wDayOfWeek);
	else if( index == 7 ) Format(val, L"%02d", Time.wHour);
	else if( index == 8 ) Format(val, L"%d", Time.wHour);
	else if( index == 9 ) Format(val, L"%02d", Time.wMinute);
	else if( index == 10 ) Format(val, L"%d", Time.wMinute);
	else if( index == 11 ) Format(val, L"%02d", Time.wSecond);
	else if( index == 12 ) Format(val, L"%d", Time.wSecond);
	else{
		WORD wHour28 = Time.wHour;
		if( wHour28 < 4 ){
			ConvertSystemTime(ConvertI64Time(Time) - 24 * 3600 * I64_1SEC, &Time);
			wHour28 += 24;
		}
		if( index == 13 ) Format(val, L"%04d", Time.wYear);
		else if( index == 14 ) Format(val, L"%02d", Time.wYear % 100);
		else if( index == 15 ) Format(val, L"%02d", Time.wMonth);
		else if( index == 16 ) Format(val, L"%d", Time.wMonth);
		else if( index == 17 ) Format(val, L"%02d", Time.wDay);
		else if( index == 18 ) Format(val, L"%d", Time.wDay);
		else if( index == 19 ) val = GetDayOfWeekName(Time.wDayOfWeek);
		else if( index == 20 ) Format(val, L"%02d", wHour28);
		else if( index == 21 ) Format(val, L"%d", wHour28);
	}
	return val;
}
