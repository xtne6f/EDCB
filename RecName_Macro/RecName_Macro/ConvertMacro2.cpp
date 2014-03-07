#include "StdAfx.h"
#include "ConvertMacro2.h"

#include "../../Common/StringUtil.h"
#include "../../Common/TimeUtil.h"
#include "../../Common/EpgTimerUtil.h"


CConvertMacro2::CConvertMacro2(void)
{
}


CConvertMacro2::~CConvertMacro2(void)
{
}

static BOOL ExpandMacro(wstring var, PLUGIN_RESERVE_INFO* info, EPG_EVENT_INFO* epgInfo, wstring& convert);

BOOL CConvertMacro2::Convert(wstring macro, PLUGIN_RESERVE_INFO* info, EPG_EVENT_INFO* epgInfo, wstring& convert)
{
	convert = L"";

	for( size_t pos = 0;; ){
		size_t next = macro.find(L'$', pos);
		if( next == wstring::npos ){
			convert.append(macro, pos, wstring::npos);
			break;
		}
		convert.append(macro, pos, next - pos);
		pos = next;

		next = macro.find(L'$', pos + 1);
		if( next == wstring::npos ){
			convert.append(macro, pos, wstring::npos);
			break;
		}
		if( ExpandMacro(macro.substr(pos + 1, next - pos - 1), info, epgInfo, convert) == FALSE ){
			convert += L'$';
			pos++;
		}else{
			pos = next + 1;
		}
	}
	Replace(convert, L"\r", L"");
	Replace(convert, L"\n", L"");

	return TRUE;
}

static BOOL ExpandMacro(wstring var, PLUGIN_RESERVE_INFO* info, EPG_EVENT_INFO* epgInfo, wstring& convert)
{
	wstring strSDW28;
	SYSTEMTIME t28TimeS;
	if( 0 <= info->startTime.wHour && info->startTime.wHour < 4 ){
		GetSumTime(info->startTime, -24*60*60, &t28TimeS);
		GetDayOfWeekString2(t28TimeS, strSDW28);
		t28TimeS.wHour+=24;
	}else{
		t28TimeS = info->startTime;
		GetDayOfWeekString2(t28TimeS, strSDW28);
	}

	SYSTEMTIME tEnd;
	GetI64Time(info->startTime, info->durationSec, NULL, NULL, &tEnd);

	wstring strEDW28;
	SYSTEMTIME t28TimeE;
	if( 0 <= tEnd.wHour && tEnd.wHour < 4 ){
		GetSumTime(tEnd, -24*60*60, &t28TimeE);
		GetDayOfWeekString2(t28TimeE, strEDW28);
		t28TimeE.wHour+=24;
	}else{
		t28TimeE = tEnd;
		GetDayOfWeekString2(t28TimeE, strEDW28);
	}

	wstring ret;
	if( var == L"Title" )	ret = info->eventName;
	else if( var == L"SDYYYY" )	Format(ret, L"%04d", info->startTime.wYear);
	else if( var == L"SDYY" )	Format(ret, L"%02d", info->startTime.wYear%100);
	else if( var == L"SDMM" )	Format(ret, L"%02d", info->startTime.wMonth);
	else if( var == L"SDM" )	Format(ret, L"%d", info->startTime.wMonth);
	else if( var == L"SDDD" )	Format(ret, L"%02d", info->startTime.wDay);
	else if( var == L"SDD" )	Format(ret, L"%d", info->startTime.wDay);
	else if( var == L"SDW" )	GetDayOfWeekString2(info->startTime, ret);
	else if( var == L"STHH" )	Format(ret, L"%02d", info->startTime.wHour);
	else if( var == L"STH" )	Format(ret, L"%d", info->startTime.wHour);
	else if( var == L"STMM" )	Format(ret, L"%02d", info->startTime.wMinute);
	else if( var == L"STM" )	Format(ret, L"%d", info->startTime.wMinute);
	else if( var == L"STSS" )	Format(ret, L"%02d", info->startTime.wSecond);
	else if( var == L"STS" )	Format(ret, L"%d", info->startTime.wSecond);
	else if( var == L"EDYYYY" )	Format(ret, L"%04d", tEnd.wYear);
	else if( var == L"EDYY" )	Format(ret, L"%02d", tEnd.wYear%100);
	else if( var == L"EDMM" )	Format(ret, L"%02d", tEnd.wMonth);
	else if( var == L"EDM" )	Format(ret, L"%d", tEnd.wMonth);
	else if( var == L"EDDD" )	Format(ret, L"%02d", tEnd.wDay);
	else if( var == L"EDD" )	Format(ret, L"%d", tEnd.wDay);
	else if( var == L"EDW" )	GetDayOfWeekString2(tEnd, ret);
	else if( var == L"ETHH" )	Format(ret, L"%02d", tEnd.wHour);
	else if( var == L"ETH" )	Format(ret, L"%d", tEnd.wHour);
	else if( var == L"ETMM" )	Format(ret, L"%02d", tEnd.wMinute);
	else if( var == L"ETM" )	Format(ret, L"%d", tEnd.wMinute);
	else if( var == L"ETSS" )	Format(ret, L"%02d", tEnd.wSecond);
	else if( var == L"ETS" )	Format(ret, L"%d", tEnd.wSecond);
	else if( var == L"ONID10" )	Format(ret, L"%d", info->ONID);
	else if( var == L"TSID10" )	Format(ret, L"%d", info->TSID);
	else if( var == L"SID10" )	Format(ret, L"%d", info->SID);
	else if( var == L"EID10" )	Format(ret, L"%d", info->EventID);
	else if( var == L"ONID16" )	Format(ret, L"%04X", info->ONID);
	else if( var == L"TSID16" )	Format(ret, L"%04X", info->TSID);
	else if( var == L"SID16" )	Format(ret, L"%04X", info->SID);
	else if( var == L"EID16" )	Format(ret, L"%04X", info->EventID);
	else if( var == L"ServiceName" )	ret = info->serviceName;
	else if( var == L"SDYYYY28" )	Format(ret, L"%04d", t28TimeS.wYear);
	else if( var == L"SDYY28" )	Format(ret, L"%02d", t28TimeS.wYear%100);
	else if( var == L"SDMM28" )	Format(ret, L"%02d", t28TimeS.wMonth);
	else if( var == L"SDM28" )	Format(ret, L"%d", t28TimeS.wMonth);
	else if( var == L"SDDD28" )	Format(ret, L"%02d", t28TimeS.wDay);
	else if( var == L"SDD28" )	Format(ret, L"%d", t28TimeS.wDay);
	else if( var == L"SDW28" )	ret = strSDW28;
	else if( var == L"STHH28" )	Format(ret, L"%02d", t28TimeS.wHour);
	else if( var == L"STH28" )	Format(ret, L"%d", t28TimeS.wHour);
	else if( var == L"EDYYYY28" )	Format(ret, L"%04d", t28TimeE.wYear);
	else if( var == L"EDYY28" )	Format(ret, L"%02d", t28TimeE.wYear%100);
	else if( var == L"EDMM28" )	Format(ret, L"%02d", t28TimeE.wMonth);
	else if( var == L"EDM28" )	Format(ret, L"%d", t28TimeE.wMonth);
	else if( var == L"EDDD28" )	Format(ret, L"%02d", t28TimeE.wDay);
	else if( var == L"EDD28" )	Format(ret, L"%d", t28TimeE.wDay);
	else if( var == L"EDW28" )	ret = strEDW28;
	else if( var == L"ETHH28" )	Format(ret, L"%02d", t28TimeE.wHour);
	else if( var == L"ETH28" )	Format(ret, L"%d", t28TimeE.wHour);
	else if( var == L"DUHH" )	Format(ret, L"%02d", info->durationSec/(60*60));
	else if( var == L"DUH" )	Format(ret, L"%d", info->durationSec/(60*60));
	else if( var == L"DUMM" )	Format(ret, L"%02d", (info->durationSec%(60*60))/60);
	else if( var == L"DUM" )	Format(ret, L"%d", (info->durationSec%(60*60))/60);
	else if( var == L"DUSS" )	Format(ret, L"%02d", info->durationSec%60);
	else if( var == L"DUS" )	Format(ret, L"%d", info->durationSec%60);
	else if( var == L"Title2" ){
		ret = info->eventName;
		while( ret.find(L"[") != wstring::npos && ret.find(L"]") != wstring::npos ){
			wstring strSep1;
			wstring strSep2;
			Separate(ret, L"[", ret, strSep1);
			Separate(strSep1, L"]", strSep2, strSep1);
			ret += strSep1;
		}
	}else if( var == L"Genre" ){
		if( epgInfo->contentInfo != NULL && epgInfo->contentInfo != NULL && epgInfo->contentInfo->listSize > 0 ){
			GetGenreName(epgInfo->contentInfo->nibbleList[0].content_nibble_level_1, 0xFF, ret);
		}
	}else if( var == L"Genre2" ){
		if( epgInfo->contentInfo != NULL && epgInfo->contentInfo != NULL && epgInfo->contentInfo->listSize > 0 ){
			GetGenreName(epgInfo->contentInfo->nibbleList[0].content_nibble_level_1, epgInfo->contentInfo->nibbleList[0].content_nibble_level_2, ret);
		}
	}else if( var == L"SubTitle" ){
		if( epgInfo != NULL && epgInfo->shortInfo != NULL ){
			ret = epgInfo->shortInfo->text_char;
		}
	}else if( var == L"SubTitle2" ){
		if( epgInfo != NULL && epgInfo->shortInfo != NULL ){
			wstring strSubTitle2 = epgInfo->shortInfo->text_char;
			strSubTitle2 = strSubTitle2.substr(0, strSubTitle2.find(L"\r\n"));
			LPCWSTR startsWith[] = { L"#ÅîëÊ", L"0123456789ÇOÇPÇQÇRÇSÇTÇUÇVÇWÇX", NULL };
			for( size_t j, i = 0; i < strSubTitle2.size(); i++ ){
				for( j = 0; startsWith[i][j] && startsWith[i][j] != strSubTitle2[i]; j++ );
				if( startsWith[i][j] == L'\0' ){
					break;
				}
				if( startsWith[i+1] == NULL ){
					ret = strSubTitle2;
					break;
				}
			}
		}
	}else{
		return FALSE;
	}

	convert += ret;

	return TRUE;
}
