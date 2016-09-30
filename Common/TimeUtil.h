#ifndef __TIME_UTIL_H__
#define __TIME_UTIL_H__

#include "StringUtil.h"

#define I64_1SEC ((__int64)10000000)

BOOL GetSumTime(SYSTEMTIME StartTime, int iSec, SYSTEMTIME* ResTime );

void GetTimeString( SYSTEMTIME Time, wstring& strDay );

void GetTimeString2( SYSTEMTIME StartTime, SYSTEMTIME EndTime, wstring& strDay );

void GetTimeString3( SYSTEMTIME StartTime, DWORD dwDureSec, wstring& strDay );

void GetTimeString4( SYSTEMTIME Time, wstring& strDay );

void GetDayOfWeekString2( SYSTEMTIME Time, wstring& strWeek );

__int64 GetNowI64Time();

__int64 ConvertI64Time( SYSTEMTIME Time );

BOOL ConvertSystemTime( __int64 i64Time, SYSTEMTIME* Time );

LPCSTR GetTimeMacroName(int index);

wstring GetTimeMacroValue(int index, SYSTEMTIME Time);

#endif
