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

BOOL CConvertMacro2::Convert(wstring macro, PLUGIN_RESERVE_INFO* info, EPG_EVENT_INFO* epgInfo, wstring& convert)
{
	convert = macro;
	wstring strTitle=L"";
	wstring strSDYYYY=L"";
	wstring strSDYY=L"";
	wstring strSDMM=L"";
	wstring strSDM=L"";
	wstring strSDDD=L"";
	wstring strSDD=L"";
	wstring strSDW=L"";
	wstring strSTHH=L"";
	wstring strSTH=L"";
	wstring strSTMM=L"";
	wstring strSTM=L"";
	wstring strSTSS=L"";
	wstring strSTS=L"";
	wstring strEDYYYY=L"";
	wstring strEDYY=L"";
	wstring strEDMM=L"";
	wstring strEDM=L"";
	wstring strEDDD=L"";
	wstring strEDD=L"";
	wstring strEDW=L"";
	wstring strETHH=L"";
	wstring strETH=L"";
	wstring strETMM=L"";
	wstring strETM=L"";
	wstring strETSS=L"";
	wstring strETS=L"";
	wstring strONID10=L"";
	wstring strTSID10=L"";
	wstring strSID10=L"";
	wstring strEID10=L"";
	wstring strONID16=L"";
	wstring strTSID16=L"";
	wstring strSID16=L"";
	wstring strEID16=L"";
	wstring strServiceName=L"";
	wstring strSDYYYY28=L"";
	wstring strSDYY28=L"";
	wstring strSDMM28=L"";
	wstring strSDM28=L"";
	wstring strSDDD28=L"";
	wstring strSDD28=L"";
	wstring strSDW28=L"";
	wstring strSTHH28=L"";
	wstring strSTH28=L"";
	wstring strEDYYYY28=L"";
	wstring strEDYY28=L"";
	wstring strEDMM28=L"";
	wstring strEDM28=L"";
	wstring strEDDD28=L"";
	wstring strEDD28=L"";
	wstring strEDW28=L"";
	wstring strETHH28=L"";
	wstring strETH28=L"";
	wstring strDUH=L"";
	wstring strDUHH=L"";
	wstring strDUM=L"";
	wstring strDUMM=L"";
	wstring strDUS=L"";
	wstring strDUSS=L"";
	wstring strTitle2=L"";
	wstring strGenre=L"";
	wstring strGenre2=L"";
	wstring strSubTitle=L"";
	wstring strSubTitle2=L"";

	strTitle = info->eventName;

	Format(strSDYYYY, L"%04d", info->startTime.wYear);
	Format(strSDYY, L"%02d", info->startTime.wYear%100);
	Format(strSDMM, L"%02d", info->startTime.wMonth);
	Format(strSDM, L"%d", info->startTime.wMonth);
	Format(strSDDD, L"%02d", info->startTime.wDay);
	Format(strSDD, L"%d", info->startTime.wDay);
	GetDayOfWeekString2(info->startTime, strSDW);
	Format(strSTHH, L"%02d", info->startTime.wHour);
	Format(strSTH, L"%d", info->startTime.wHour);
	Format(strSTMM, L"%02d", info->startTime.wMinute);
	Format(strSTM, L"%d", info->startTime.wMinute);
	Format(strSTSS, L"%02d", info->startTime.wSecond);
	Format(strSTS, L"%d", info->startTime.wSecond);

	SYSTEMTIME t28TimeS;
	if( 0 <= info->startTime.wHour && info->startTime.wHour < 4 ){
		GetSumTime(info->startTime, -24*60*60, &t28TimeS);
		GetDayOfWeekString2(t28TimeS, strSDW28);
		t28TimeS.wHour+=24;
	}else{
		t28TimeS = info->startTime;
		GetDayOfWeekString2(t28TimeS, strSDW28);
	}

	Format(strSDYYYY28, L"%04d", t28TimeS.wYear);
	Format(strSDYY28, L"%02d", t28TimeS.wYear%100);
	Format(strSDMM28, L"%02d", t28TimeS.wMonth);
	Format(strSDM28, L"%d", t28TimeS.wMonth);
	Format(strSDDD28, L"%02d", t28TimeS.wDay);
	Format(strSDD28, L"%d", t28TimeS.wDay);
	Format(strSTHH28, L"%02d", t28TimeS.wHour);
	Format(strSTH28, L"%d", t28TimeS.wHour);

	SYSTEMTIME tEnd;
	GetI64Time(info->startTime, info->durationSec, NULL, NULL, &tEnd);

	Format(strEDYYYY, L"%04d", tEnd.wYear);
	Format(strEDYY, L"%02d", tEnd.wYear%100);
	Format(strEDMM, L"%02d", tEnd.wMonth);
	Format(strEDM, L"%d", tEnd.wMonth);
	Format(strEDDD, L"%02d", tEnd.wDay);
	Format(strEDD, L"%d", tEnd.wDay);
	GetDayOfWeekString2(tEnd, strEDW);
	Format(strETHH, L"%02d", tEnd.wHour);
	Format(strETH, L"%d", tEnd.wHour);
	Format(strETMM, L"%02d", tEnd.wMinute);
	Format(strETM, L"%d", tEnd.wMinute);
	Format(strETSS, L"%02d", tEnd.wSecond);
	Format(strETS, L"%d", tEnd.wSecond);

	SYSTEMTIME t28TimeE;
	if( 0 <= tEnd.wHour && tEnd.wHour < 4 ){
		GetSumTime(tEnd, -24*60*60, &t28TimeE);
		GetDayOfWeekString2(t28TimeE, strEDW28);
		t28TimeE.wHour+=24;
	}else{
		t28TimeE = tEnd;
		GetDayOfWeekString2(t28TimeE, strEDW28);
	}

	Format(strEDYYYY28, L"%04d", t28TimeE.wYear);
	Format(strEDYY28, L"%02d", t28TimeE.wYear%100);
	Format(strEDMM28, L"%02d", t28TimeE.wMonth);
	Format(strEDM28, L"%d", t28TimeE.wMonth);
	Format(strEDDD28, L"%02d", t28TimeE.wDay);
	Format(strEDD28, L"%d", t28TimeE.wDay);
	Format(strETHH28, L"%02d", t28TimeE.wHour);
	Format(strETH28, L"%d", t28TimeE.wHour);

	Format(strONID10, L"%d", info->ONID);
	Format(strTSID10, L"%d", info->TSID);
	Format(strSID10, L"%d", info->SID);
	Format(strEID10, L"%d", info->EventID);
	Format(strONID16, L"%04X", info->ONID);
	Format(strTSID16, L"%04X", info->TSID);
	Format(strSID16, L"%04X", info->SID);
	Format(strEID16, L"%04X", info->EventID);

	strServiceName = info->serviceName;

	Format(strDUHH, L"%02d", info->durationSec/(60*60));
	Format(strDUH, L"%d", info->durationSec/(60*60));
	Format(strDUMM, L"%02d", (info->durationSec%(60*60))/60);
	Format(strDUM, L"%d", (info->durationSec%(60*60))/60);
	Format(strDUSS, L"%02d", info->durationSec%60);
	Format(strDUS, L"%d", info->durationSec%60);

	strTitle2 = info->eventName;
	while( (strTitle2.find(L"[") != string::npos) && (strTitle2.find(L"]") != string::npos) ){
		wstring strSep1=L"";
		wstring strSep2=L"";
		Separate(strTitle2, L"[", strSep1, strTitle2);
		Separate(strTitle2, L"]", strSep2, strTitle2);
		strSep1 += strTitle2;
		strTitle2 = strSep1;
	}

	if(epgInfo != NULL ){
		if( epgInfo->contentInfo != NULL ){
			if( epgInfo->contentInfo->listSize > 0 ){
				GetGenreName(epgInfo->contentInfo->nibbleList[0].content_nibble_level_1, 0xFF, strGenre);
				GetGenreName(epgInfo->contentInfo->nibbleList[0].content_nibble_level_1, epgInfo->contentInfo->nibbleList[0].content_nibble_level_2, strGenre2);
			}
		}
		if( epgInfo->shortInfo != NULL ){
			strSubTitle = epgInfo->shortInfo->text_char;
			strSubTitle2 = epgInfo->shortInfo->text_char;
			wstring r;
			Separate(strSubTitle2, L"\r\n", strSubTitle2, r);
			LPCWSTR startsWith[] = { L"#ÅîëÊ", L"0123456789ÇOÇPÇQÇRÇSÇTÇUÇVÇWÇX", NULL };
			BOOL found = FALSE;
			for( size_t j, i = 0; i < strSubTitle2.size(); i++ ){
				for( j = 0; startsWith[i][j] && startsWith[i][j] != strSubTitle2[i]; j++ );
				if( startsWith[i][j] == L'\0' ){
					break;
				}
				if( startsWith[i+1] == NULL ){
					found = TRUE;
					break;
				}
			}
			if( found == FALSE ){
				strSubTitle2 = L"";
			}
		}
	}

	Replace(convert, L"$Title$", strTitle);
	Replace(convert, L"$SDYYYY$", strSDYYYY);
	Replace(convert, L"$SDYY$", strSDYY);
	Replace(convert, L"$SDMM$", strSDMM);
	Replace(convert, L"$SDM$", strSDM);
	Replace(convert, L"$SDDD$", strSDDD);
	Replace(convert, L"$SDD$", strSDD);
	Replace(convert, L"$SDW$", strSDW);
	Replace(convert, L"$STHH$", strSTHH);
	Replace(convert, L"$STH$", strSTH);
	Replace(convert, L"$STMM$", strSTMM);
	Replace(convert, L"$STM$", strSTM);
	Replace(convert, L"$STSS$", strSTSS);
	Replace(convert, L"$STS$", strSTS);
	Replace(convert, L"$EDYYYY$", strEDYYYY);
	Replace(convert, L"$EDYY$", strEDYY);
	Replace(convert, L"$EDMM$", strEDMM);
	Replace(convert, L"$EDM$", strEDM);
	Replace(convert, L"$EDDD$", strEDDD);
	Replace(convert, L"$EDD$", strEDD);
	Replace(convert, L"$EDW$", strEDW);
	Replace(convert, L"$ETHH$", strETHH);
	Replace(convert, L"$ETH$", strETH);
	Replace(convert, L"$ETMM$", strETMM);
	Replace(convert, L"$ETM$", strETM);
	Replace(convert, L"$ETSS$", strETSS);
	Replace(convert, L"$ETS$", strETS);
	Replace(convert, L"$ONID10$", strONID10);
	Replace(convert, L"$TSID10$", strTSID10);
	Replace(convert, L"$SID10$", strSID10);
	Replace(convert, L"$EID10$", strEID10);
	Replace(convert, L"$ONID16$", strONID16);
	Replace(convert, L"$TSID16$", strTSID16);
	Replace(convert, L"$SID16$", strSID16);
	Replace(convert, L"$EID16$", strEID16);
	Replace(convert, L"$ServiceName$", strServiceName);
	Replace(convert, L"$SDYYYY28$", strSDYYYY28);
	Replace(convert, L"$SDYY28$", strSDYY28);
	Replace(convert, L"$SDMM28$", strSDMM28);
	Replace(convert, L"$SDM28$", strSDM28);
	Replace(convert, L"$SDDD28$", strSDDD28);
	Replace(convert, L"$SDD28$", strSDD28);
	Replace(convert, L"$SDW28$", strSDW28);
	Replace(convert, L"$STHH28$", strSTHH28);
	Replace(convert, L"$STH28$", strSTH28);
	Replace(convert, L"$EDYYYY28$", strEDYYYY28);
	Replace(convert, L"$EDYY28$", strEDYY28);
	Replace(convert, L"$EDMM28$", strEDMM28);
	Replace(convert, L"$EDM28$", strEDM28);
	Replace(convert, L"$EDDD28$", strEDDD28);
	Replace(convert, L"$EDD28$", strEDD28);
	Replace(convert, L"$EDW28$", strEDW28);
	Replace(convert, L"$ETHH28$", strETHH28);
	Replace(convert, L"$ETH28$", strETH28);
	Replace(convert, L"$DUHH$", strDUHH);
	Replace(convert, L"$DUH$", strDUH);
	Replace(convert, L"$DUMM$", strDUMM);
	Replace(convert, L"$DUM$", strDUM);
	Replace(convert, L"$DUSS$", strDUSS);
	Replace(convert, L"$DUS$", strDUS);
	Replace(convert, L"$Title2$", strTitle2);
	Replace(convert, L"$Genre$", strGenre);
	Replace(convert, L"$Genre2$", strGenre2);
	Replace(convert, L"$SubTitle$", strSubTitle);
	Replace(convert, L"$SubTitle2$", strSubTitle2);
	Replace(convert, L"\r\n", L"");

	return TRUE;
}
