#ifndef INCLUDE_TIME_UTIL_H
#define INCLUDE_TIME_UTIL_H

#include "StringUtil.h"

#define I64_1SEC ((__int64)10000000)
#define I64_UTIL_TIMEZONE (9 * 3600 * I64_1SEC)

LPCWSTR GetDayOfWeekName( WORD wDayOfWeek );

__int64 GetNowI64Time();

__int64 ConvertI64Time( SYSTEMTIME Time );

BOOL ConvertSystemTime( __int64 i64Time, SYSTEMTIME* Time );

LPCSTR GetTimeMacroName(int index);

wstring GetTimeMacroValue(int index, SYSTEMTIME Time);

#endif
