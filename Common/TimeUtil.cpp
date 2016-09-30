#include "stdafx.h"
#include "TimeUtil.h"

BOOL GetSumTime(SYSTEMTIME StartTime, int iSec, SYSTEMTIME* ResTime )
{
	return ConvertSystemTime(ConvertI64Time(StartTime) + iSec * I64_1SEC, ResTime);
}

void GetTimeString( SYSTEMTIME Time, wstring& strDay )
{
	wstring strBase;
	GetTimeString4(Time, strBase);

	Format(strDay, L"%s:%02d",
		strBase.c_str(),
		Time.wSecond
		);
}

void GetTimeString2( SYSTEMTIME StartTime, SYSTEMTIME EndTime, wstring& strDay )
{
	wstring strBase;
	GetTimeString4(StartTime, strBase);

	Format(strDay, L"%sÅ`%02d:%02d",
		strBase.c_str(),
		EndTime.wHour,
		EndTime.wMinute
		);
}

void GetTimeString3( SYSTEMTIME StartTime, DWORD dwDureSec, wstring& strDay )
{
	SYSTEMTIME EndTime;
	ConvertSystemTime(ConvertI64Time(StartTime) + dwDureSec * I64_1SEC, &EndTime);
	GetTimeString2(StartTime, EndTime, strDay);
}

void GetTimeString4( SYSTEMTIME Time, wstring& strDay )
{
	wstring strWeek=L"";
	GetDayOfWeekString2(Time, strWeek);

	Format(strDay, L"%04d/%02d/%02d(%s) %02d:%02d",
		Time.wYear,
		Time.wMonth,
		Time.wDay,
		strWeek.c_str(),
		Time.wHour,
		Time.wMinute
		);
}

void GetDayOfWeekString2( SYSTEMTIME Time, wstring& strWeek )
{
	SYSTEMTIME sTime;
	ConvertSystemTime(ConvertI64Time(Time), &sTime);

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
}

__int64 GetNowI64Time()
{
	SYSTEMTIME sTime;
	GetLocalTime(&sTime);
	return ConvertI64Time(sTime);
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
	else if( index == 6 ) GetDayOfWeekString2(Time, val);
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
		else if( index == 19 ) GetDayOfWeekString2(Time, val);
		else if( index == 20 ) Format(val, L"%02d", wHour28);
		else if( index == 21 ) Format(val, L"%d", wHour28);
	}
	return val;
}
