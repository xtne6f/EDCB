#include "stdafx.h"
#include "TimeUtil.h"

BOOL GetI64Time(SYSTEMTIME StartTime, DWORD dwDurationSec, __int64* i64Start, __int64* i64End, SYSTEMTIME* EndTime )
{
	FILETIME fTime;
	SystemTimeToFileTime( &StartTime, &fTime );

	__int64 i64Time = ((__int64)fTime.dwHighDateTime)<<32 | fTime.dwLowDateTime;

	if( i64Start != NULL ){
		*i64Start = i64Time;
	}

	i64Time += I64_1SEC*((__int64)dwDurationSec);

	if( i64End != NULL ){
		*i64End = i64Time;
	}

	if( EndTime != NULL ){
		fTime.dwHighDateTime = (DWORD)(i64Time>>32);
		fTime.dwLowDateTime = (DWORD)(i64Time&0x00000000FFFFFFFF);
		FileTimeToSystemTime(&fTime,EndTime);
	}

	return TRUE;
}

BOOL GetSumTime(SYSTEMTIME StartTime, int iSec, SYSTEMTIME* ResTime )
{
	FILETIME fTime;
	SystemTimeToFileTime( &StartTime, &fTime );

	__int64 i64Time = ((__int64)fTime.dwHighDateTime)<<32 | fTime.dwLowDateTime;

	i64Time += I64_1SEC*((__int64)iSec);

	if( ResTime != NULL ){
		fTime.dwHighDateTime = (DWORD)(i64Time>>32);
		fTime.dwLowDateTime = (DWORD)(i64Time&0x00000000FFFFFFFF);
		FileTimeToSystemTime(&fTime,ResTime);
	}

	return TRUE;
}

BOOL IsBigTime(SYSTEMTIME Time1, SYSTEMTIME Time2 )
{
	FILETIME fTime1;
	FILETIME fTime2;
	SystemTimeToFileTime( &Time1, &fTime1 );
	SystemTimeToFileTime( &Time2, &fTime2 );

	__int64 i64Time1 = ((__int64)fTime1.dwHighDateTime)<<32 | fTime1.dwLowDateTime;
	__int64 i64Time2 = ((__int64)fTime2.dwHighDateTime)<<32 | fTime2.dwLowDateTime;

	if( i64Time1 > i64Time2 ){
		return TRUE;
	}
	return FALSE;
}

BOOL GetTimeString( WORD wYear, WORD wMonth, WORD wDay, WORD wHour, WORD wMin, WORD wSec, wstring& strDay )
{
	SYSTEMTIME sTime;
	sTime.wYear = wYear;
	sTime.wMonth = wMonth;
	sTime.wDay = wDay;
	sTime.wHour = wHour;
	sTime.wMinute = wMin;
	sTime.wSecond = wSec;
	sTime.wMilliseconds = 0;

	wstring strWeek=L"";
	GetDayOfWeekString(sTime, strWeek);

	Format(strDay, L"%04d/%02d/%02d%s %02d:%02d:%02d",
		sTime.wYear,
		sTime.wMonth,
		sTime.wDay,
		strWeek.c_str(),
		sTime.wHour,
		sTime.wMinute,
		sTime.wSecond
		);

	return TRUE;
}

BOOL GetTimeString( SYSTEMTIME Time, wstring& strDay )
{
	wstring strWeek=L"";
	GetDayOfWeekString(Time, strWeek);

	Format(strDay, L"%04d/%02d/%02d%s %02d:%02d:%02d",
		Time.wYear,
		Time.wMonth,
		Time.wDay,
		strWeek.c_str(),
		Time.wHour,
		Time.wMinute,
		Time.wSecond
		);

	return TRUE;
}

BOOL GetTimeString2( SYSTEMTIME StartTime, SYSTEMTIME EndTime, wstring& strDay )
{
	wstring strWeek=L"";
	GetDayOfWeekString(StartTime, strWeek);

	Format(strDay, L"%04d/%02d/%02d%s %02d:%02dÅ`%02d:%02d",
		StartTime.wYear,
		StartTime.wMonth,
		StartTime.wDay,
		strWeek.c_str(),
		StartTime.wHour,
		StartTime.wMinute,
		EndTime.wHour,
		EndTime.wMinute
		);

	return TRUE;
}

BOOL GetTimeString3( SYSTEMTIME StartTime, DWORD dwDureSec, wstring& strDay )
{
	wstring strWeek=L"";
	GetDayOfWeekString(StartTime, strWeek);

	SYSTEMTIME EndTime;
	GetI64Time(StartTime, dwDureSec, NULL, NULL, &EndTime);

	Format(strDay, L"%04d/%02d/%02d%s %02d:%02dÅ`%02d:%02d",
		StartTime.wYear,
		StartTime.wMonth,
		StartTime.wDay,
		strWeek.c_str(),
		StartTime.wHour,
		StartTime.wMinute,
		EndTime.wHour,
		EndTime.wMinute
		);

	return TRUE;
}

BOOL GetTimeString4( SYSTEMTIME Time, wstring& strDay )
{
	wstring strWeek=L"";
	GetDayOfWeekString(Time, strWeek);

	Format(strDay, L"%04d/%02d/%02d%s %02d:%02d",
		Time.wYear,
		Time.wMonth,
		Time.wDay,
		strWeek.c_str(),
		Time.wHour,
		Time.wMinute
		);

	return TRUE;
}

BOOL GetDayOfWeekString( SYSTEMTIME Time, wstring& strWeek )
{
	SYSTEMTIME sTime;
	FILETIME fTime;
	SystemTimeToFileTime( &Time, &fTime );
	FileTimeToSystemTime( &fTime, &sTime );

	switch( sTime.wDayOfWeek ){
		case 0:
			strWeek=L"(ì˙)";
			break;
		case 1:
			strWeek=L"(åé)";
			break;
		case 2:
			strWeek=L"(âŒ)";
			break;
		case 3:
			strWeek=L"(êÖ)";
			break;
		case 4:
			strWeek=L"(ñÿ)";
			break;
		case 5:
			strWeek=L"(ã‡)";
			break;
		case 6:
			strWeek=L"(ìy)";
			break;
		default:
			strWeek=L"";
			break;
	}

	return TRUE;
}

BOOL GetDayOfWeekString2( SYSTEMTIME Time, wstring& strWeek )
{
	SYSTEMTIME sTime;
	FILETIME fTime;
	SystemTimeToFileTime( &Time, &fTime );
	FileTimeToSystemTime( &fTime, &sTime );

	switch( sTime.wDayOfWeek ){
		case 0:
			strWeek=L"ì˙";
			break;
		case 1:
			strWeek=L"åé";
			break;
		case 2:
			strWeek=L"âŒ";
			break;
		case 3:
			strWeek=L"êÖ";
			break;
		case 4:
			strWeek=L"ñÿ";
			break;
		case 5:
			strWeek=L"ã‡";
			break;
		case 6:
			strWeek=L"ìy";
			break;
		default:
			strWeek=L"";
			break;
	}

	return TRUE;
}

BOOL GetDayOfWeekString2( SYSTEMTIME Time, string& strWeek )
{
	SYSTEMTIME sTime;
	FILETIME fTime;
	SystemTimeToFileTime( &Time, &fTime );
	FileTimeToSystemTime( &fTime, &sTime );

	switch( sTime.wDayOfWeek ){
		case 0:
			strWeek="ì˙";
			break;
		case 1:
			strWeek="åé";
			break;
		case 2:
			strWeek="âŒ";
			break;
		case 3:
			strWeek="êÖ";
			break;
		case 4:
			strWeek="ñÿ";
			break;
		case 5:
			strWeek="ã‡";
			break;
		case 6:
			strWeek="ìy";
			break;
		default:
			strWeek="";
			break;
	}

	return TRUE;
}

__int64 GetNowI64Time()
{
	SYSTEMTIME sTime;
	GetLocalTime(&sTime);
	FILETIME fTime;
	SystemTimeToFileTime( &sTime, &fTime );
	__int64 i64Time = 0;
	i64Time = ((__int64)fTime.dwHighDateTime)<<32 | fTime.dwLowDateTime;

	return i64Time;
}

__int64 ConvertI64Time(SYSTEMTIME Time)
{
	FILETIME fTime;
	SystemTimeToFileTime( &Time, &fTime );
	__int64 i64Time = 0;
	i64Time = ((__int64)fTime.dwHighDateTime)<<32 | fTime.dwLowDateTime;

	return i64Time;
}

BOOL ConvertSystemTime( __int64 i64Time, SYSTEMTIME* Time )
{
	if( Time != NULL ){
		FILETIME fTime;
		fTime.dwHighDateTime = (DWORD)(i64Time>>32);
		fTime.dwLowDateTime = (DWORD)(i64Time&0x00000000FFFFFFFF);
		FileTimeToSystemTime(&fTime,Time);
		return TRUE;
	}
	return FALSE;
}

__int64 GetStartTime( SYSTEMTIME Time, int StartMargineSec )
{
	return ConvertI64Time(Time) - (StartMargineSec * I64_1SEC);
}

__int64 GetEndTime( SYSTEMTIME StartTime, DWORD DureSec, int EndMargineSec )
{
	return ConvertI64Time(StartTime) + ((DureSec + EndMargineSec) * I64_1SEC);
}

__int64 GetSumTime( SYSTEMTIME StartTime, DWORD DureSec)
{
	FILETIME fTime;
	SystemTimeToFileTime( &StartTime, &fTime );
	__int64 i64Time = 0;
	i64Time = ((__int64)fTime.dwHighDateTime)<<32 | fTime.dwLowDateTime;
	i64Time += ((__int64)DureSec)*I64_1SEC;

	return i64Time;

}

