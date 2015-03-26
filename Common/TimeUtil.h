#ifndef __TIME_UTIL_H__
#define __TIME_UTIL_H__

#include "StringUtil.h"

#define I64_1SEC ((__int64)10000000)

BOOL GetI64Time(SYSTEMTIME StartTime, DWORD dwDurationSec, __int64* i64Start, __int64* i64End, SYSTEMTIME* EndTime );

BOOL GetSumTime(SYSTEMTIME StartTime, int iSec, SYSTEMTIME* ResTime );

BOOL IsBigTime(SYSTEMTIME Time1, SYSTEMTIME Time2 );

BOOL GetTimeString( WORD wYear, WORD wMonth, WORD wDay, WORD wHour, WORD wMin, WORD wSec, wstring& strDay );

BOOL GetTimeString( SYSTEMTIME Time, wstring& strDay );

BOOL GetTimeString2( SYSTEMTIME StartTime, SYSTEMTIME EndTime, wstring& strDay );

BOOL GetTimeString3( SYSTEMTIME StartTime, DWORD dwDureSec, wstring& strDay );

BOOL GetTimeString4( SYSTEMTIME Time, wstring& strDay );

BOOL GetDayOfWeekString( SYSTEMTIME Time, wstring& strWeek );
BOOL GetDayOfWeekString2( SYSTEMTIME Time, wstring& strWeek );
BOOL GetDayOfWeekString2( SYSTEMTIME Time, string& strWeek );

__int64 GetNowI64Time();

__int64 ConvertI64Time( SYSTEMTIME Time );

BOOL ConvertSystemTime( __int64 i64Time, SYSTEMTIME* Time );

__int64 GetStartTime( SYSTEMTIME Time, int StartMargineSec );

__int64 GetEndTime( SYSTEMTIME StartTime, DWORD DureSec, int EndMargineSec );

__int64 GetSumTime( SYSTEMTIME StartTime, DWORD DureSec);

#endif
