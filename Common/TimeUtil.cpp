#include "stdafx.h"
#include "TimeUtil.h"
#ifndef _WIN32
#include <time.h>
#include <stdexcept>
#endif

LPCWSTR GetDayOfWeekName( WORD wDayOfWeek )
{
	static const WCHAR name[7][2] = {
		L"日", L"月", L"火", L"水", L"木", L"金", L"土"
	};
	return name[wDayOfWeek % 7];
}

__int64 GetNowI64Time()
{
#ifdef _WIN32
	FILETIME fTime;
	GetSystemTimeAsFileTime(&fTime);
	return ((__int64)fTime.dwHighDateTime << 32 | fTime.dwLowDateTime) + I64_UTIL_TIMEZONE;
#else
	timespec ts;
	if( clock_gettime(CLOCK_REALTIME, &ts) ){
		throw std::runtime_error("");
	}
	return (__int64)ts.tv_sec * I64_1SEC + ts.tv_nsec / 100 + 116444736000000000 + I64_UTIL_TIMEZONE;
#endif
}

__int64 ConvertI64Time(SYSTEMTIME Time)
{
#ifdef _WIN32
	FILETIME fTime;
	if( SystemTimeToFileTime( &Time, &fTime ) ){
		return (__int64)fTime.dwHighDateTime << 32 | fTime.dwLowDateTime;
	}
#else
	tm t = {};
	t.tm_year = Time.wYear - 1900;
	t.tm_mon = Time.wMonth - 1;
	t.tm_mday = Time.wDay;
	t.tm_hour = Time.wHour;
	t.tm_min = Time.wMinute;
	t.tm_sec = Time.wSecond;
	time_t tt = timegm(&t);
	if( tt != (time_t)-1 ){
		return (__int64)tt * I64_1SEC + Time.wMilliseconds * 10000 + 116444736000000000;
	}
#endif
	return 0;
}

bool ConvertSystemTime(__int64 i64Time, SYSTEMTIME* Time)
{
	if( Time != NULL ){
#ifdef _WIN32
		FILETIME fTime;
		fTime.dwHighDateTime = (DWORD)(i64Time>>32);
		fTime.dwLowDateTime = (DWORD)(i64Time&0x00000000FFFFFFFF);
		if( FileTimeToSystemTime(&fTime,Time) ){
			return true;
		}
#else
		time_t tt = (time_t)((i64Time - 116444736000000000) / I64_1SEC);
		tm t;
		if( gmtime_r(&tt, &t) ){
			Time->wYear = (WORD)(t.tm_year + 1900);
			Time->wMonth = (WORD)(t.tm_mon + 1);
			Time->wDayOfWeek = (WORD)t.tm_wday;
			Time->wDay = (WORD)t.tm_mday;
			Time->wHour = (WORD)t.tm_hour;
			Time->wMinute = (WORD)t.tm_min;
			Time->wSecond = (WORD)t.tm_sec;
			Time->wMilliseconds = (WORD)(i64Time / 10000 % 1000);
			return true;
		}
#endif
		SYSTEMTIME sTime = {};
		*Time = sTime;
	}
	return false;
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
