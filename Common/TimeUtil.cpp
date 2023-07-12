#include "stdafx.h"
#include "TimeUtil.h"
#ifndef _WIN32
#include <time.h>
#include <chrono>
namespace chrono = std::chrono;
#endif

LPCWSTR GetDayOfWeekName( WORD wDayOfWeek )
{
	static const WCHAR name[7][2] = {
		L"日", L"月", L"火", L"水", L"木", L"金", L"土"
	};
	return name[wDayOfWeek % 7];
}

DWORD GetU32Tick()
{
#ifdef _WIN32
#ifdef _MSC_VER
#pragma warning(push)
// Consider using 'GetTickCount64' instead of 'GetTickCount'
#pragma warning(disable : 28159)
#endif
	return GetTickCount();
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#else
	return (DWORD)chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now().time_since_epoch()).count();
#endif
}

LONGLONG GetNowI64Time()
{
#ifdef _WIN32
	FILETIME fTime;
	GetSystemTimeAsFileTime(&fTime);
	return ((LONGLONG)fTime.dwHighDateTime << 32 | fTime.dwLowDateTime) + I64_UTIL_TIMEZONE;
#else
	return chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now().time_since_epoch()).count() * 10 +
	       116444736000000000 + I64_UTIL_TIMEZONE;
#endif
}

LONGLONG ConvertI64Time(SYSTEMTIME Time)
{
#ifdef _WIN32
	FILETIME fTime;
	if( SystemTimeToFileTime( &Time, &fTime ) ){
		return (LONGLONG)fTime.dwHighDateTime << 32 | fTime.dwLowDateTime;
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
		return (LONGLONG)tt * I64_1SEC + Time.wMilliseconds * 10000 + 116444736000000000;
	}
#endif
	return 0;
}

bool ConvertSystemTime(LONGLONG i64Time, SYSTEMTIME* Time)
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
	WCHAR val[32];
	val[0] = L'\0';
	if( index == 0 ) swprintf_s(val, L"%04d", Time.wYear);
	else if( index == 1 ) swprintf_s(val, L"%02d", Time.wYear % 100);
	else if( index == 2 ) swprintf_s(val, L"%02d", Time.wMonth);
	else if( index == 3 ) swprintf_s(val, L"%d", Time.wMonth);
	else if( index == 4 ) swprintf_s(val, L"%02d", Time.wDay);
	else if( index == 5 ) swprintf_s(val, L"%d", Time.wDay);
	else if( index == 6 ) wcscpy_s(val, GetDayOfWeekName(Time.wDayOfWeek));
	else if( index == 7 ) swprintf_s(val, L"%02d", Time.wHour);
	else if( index == 8 ) swprintf_s(val, L"%d", Time.wHour);
	else if( index == 9 ) swprintf_s(val, L"%02d", Time.wMinute);
	else if( index == 10 ) swprintf_s(val, L"%d", Time.wMinute);
	else if( index == 11 ) swprintf_s(val, L"%02d", Time.wSecond);
	else if( index == 12 ) swprintf_s(val, L"%d", Time.wSecond);
	else{
		WORD wHour28 = Time.wHour;
		if( wHour28 < 4 ){
			ConvertSystemTime(ConvertI64Time(Time) - 24 * 3600 * I64_1SEC, &Time);
			wHour28 += 24;
		}
		if( index == 13 ) swprintf_s(val, L"%04d", Time.wYear);
		else if( index == 14 ) swprintf_s(val, L"%02d", Time.wYear % 100);
		else if( index == 15 ) swprintf_s(val, L"%02d", Time.wMonth);
		else if( index == 16 ) swprintf_s(val, L"%d", Time.wMonth);
		else if( index == 17 ) swprintf_s(val, L"%02d", Time.wDay);
		else if( index == 18 ) swprintf_s(val, L"%d", Time.wDay);
		else if( index == 19 ) wcscpy_s(val, GetDayOfWeekName(Time.wDayOfWeek));
		else if( index == 20 ) swprintf_s(val, L"%02d", wHour28);
		else if( index == 21 ) swprintf_s(val, L"%d", wHour28);
	}
	return val;
}
