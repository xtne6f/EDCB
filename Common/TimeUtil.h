#ifndef INCLUDE_TIME_UTIL_H
#define INCLUDE_TIME_UTIL_H

#include "StringUtil.h"

#define I64_1SEC ((LONGLONG)10000000)
#define I64_UTIL_TIMEZONE (9 * 3600 * I64_1SEC)

LPCWSTR GetDayOfWeekName( WORD wDayOfWeek );

LONGLONG GetNowI64Time();

LONGLONG ConvertI64Time(SYSTEMTIME Time);

bool ConvertSystemTime(LONGLONG i64Time, SYSTEMTIME* Time);

LPCSTR GetTimeMacroName(int index);

wstring GetTimeMacroValue(int index, SYSTEMTIME Time);

#endif
