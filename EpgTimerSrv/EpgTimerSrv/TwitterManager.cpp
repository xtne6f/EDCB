#include "StdAfx.h"
#include "TwitterManager.h"
#include "../../Common/PathUtil.h"
#include "../../Common/StringUtil.h"
#include "../../Common/TimeUtil.h"

CTwitterManager::CTwitterManager(void)
{
	this->twitterUtil.Initialize();
	this->lockEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
}


CTwitterManager::~CTwitterManager(void)
{
	if( this->lockEvent != NULL ){
		UnLock();
		CloseHandle(this->lockEvent);
		this->lockEvent = NULL;
	}
	this->twitterUtil.UnInitialize();
}
BOOL CTwitterManager::Lock(LPCWSTR log, DWORD timeOut)
{
	if( this->lockEvent == NULL ){
		return FALSE;
	}
	//if( log != NULL ){
	//	_OutputDebugString(L"Ÿ%s",log);
	//}
	DWORD dwRet = WaitForSingleObject(this->lockEvent, timeOut);
	if( dwRet == WAIT_ABANDONED || 
		dwRet == WAIT_FAILED ||
		dwRet == WAIT_TIMEOUT){
			OutputDebugString(L"ŸCTwitterManager::Lock FALSE");
			if( log != NULL ){
				OutputDebugString(log);
			}
		return FALSE;
	}
	return TRUE;
}

void CTwitterManager::UnLock(LPCWSTR log)
{
	if( this->lockEvent != NULL ){
		SetEvent(this->lockEvent);
	}
	if( log != NULL ){
		OutputDebugString(log);
	}
}

//ProxyŽg—p‚ðÝ’è
//–ß‚è’lF
// ƒGƒ‰[ƒR[ƒh
//ˆø”F
// useProxy			[IN]ProxyŽg‚¤‚©‚Ç‚¤‚©iTRUE:ProxyŽg‚¤j
// proxyInfo		[IN]ProxyŽg‚¤ê‡‚ÌÝ’èî•ñ
void CTwitterManager::SetProxy(
	BOOL useProxy,
	USE_PROXY_INFO* proxyInfo
	)
{
	if( Lock(L"SetProxy") == FALSE ) return;

	twitterUtil.SetProxy(useProxy, proxyInfo);

	UnLock();
}

//”ñ“¯ŠúƒcƒC[ƒg‚ÌŽc‚è‚ðŽæ“¾‚·‚é
//–ß‚è’lF
// ŒÂ”
DWORD CTwitterManager::GetTweetQue(
	)
{
	if( Lock(L"SendTweet") == FALSE ) return 0;

	DWORD ret = twitterUtil.GetTweetQue();

	UnLock();
	return ret;
}

void CTwitterManager::SendTweet(
	SEND_TWEET_MODE mode,
	void* param1,
	void* param2,
	void* param3
	)
{
	if( Lock(L"SendTweet") == FALSE ) return;

	wstring text;

	switch(mode){
	case TW_TEXT:
		{
			text = (WCHAR*)param1;
		}
		break;
	case TW_CHG_RESERVE_RELOADEPG:
		{
			CreateTextChgResReload(text, (RESERVE_DATA*)param1, (RESERVE_DATA*)param2);
		}
		break;
	case TW_CHG_RESERVE_CHK_REC:
		{
			CreateTextChgResRec(text, (RESERVE_DATA*)param1, (RESERVE_DATA*)param2, (EPGDB_EVENT_INFO*)param3);
		}
		break;
	case TW_ADD_RESERVE:
		{
			CreateTextAddRes(text, (RESERVE_DATA*)param1);
		}
		break;
	case TW_REC_START:
		{
			CreateTextRecStart(text, (RESERVE_DATA*)param1);
		}
		break;
	case TW_REC_END:
		{
			CreateTextRecStop(text, (REC_FILE_INFO*)param1);
		}
		break;
	default:
		break;
	}

	if( text.size() > 0 ){
		this->twitterUtil.SendTweet(TRUE, text.c_str());
	}

	UnLock();
}

void CTwitterManager::CreateTextChgResReload(wstring& text, RESERVE_DATA* oldInfo, RESERVE_DATA* newInfo)
{
	wstring iniAppPath = L"";
	GetModuleIniPath(iniAppPath);

	if( GetPrivateProfileInt(L"TWITTER_MACRO", L"Use1", 0, iniAppPath.c_str() ) == 0 ){
		return ;
	}
	WCHAR buff[1024] = L"";
	GetPrivateProfileString(L"TWITTER_MACRO", L"MACRO1", L"œEPG“Ç‚Ýž‚ÝF$ServiceNameOLD$ $SDYYYYOLD$/$SDMOLD$/$SDDOLD$ $STHOLD$:$STMOLD$ ` $ETHOLD$:$ETMOLD$ EventID:0x$EID16OLD$ ¨ $SDYYYYNEW$/$SDMNEW$/$SDDNEW$ $STHNEW$:$STMNEW$ ` $ETHNEW$:$ETMNEW$ EventID:0x$EID16NEW$ $TitleNEW$", buff, 1024, iniAppPath.c_str());

	text = buff;
	MACRO_DATA macroData;

	macroData.oldTitle = oldInfo->title;
	Format(macroData.oldSDYYYY, L"%04d", oldInfo->startTime.wYear);
	Format(macroData.oldSDYY, L"%02d", oldInfo->startTime.wYear%100);
	Format(macroData.oldSDMM, L"%02d", oldInfo->startTime.wMonth);
	Format(macroData.oldSDM, L"%d", oldInfo->startTime.wMonth);
	Format(macroData.oldSDDD, L"%02d", oldInfo->startTime.wDay);
	Format(macroData.oldSDD, L"%d", oldInfo->startTime.wDay);
	Format(macroData.oldSTHH, L"%02d", oldInfo->startTime.wHour);
	Format(macroData.oldSTH, L"%d", oldInfo->startTime.wHour);
	Format(macroData.oldSTMM, L"%02d", oldInfo->startTime.wMinute);
	Format(macroData.oldSTM, L"%d", oldInfo->startTime.wMinute);
	Format(macroData.oldSTSS, L"%02d", oldInfo->startTime.wSecond);
	Format(macroData.oldSTS, L"%d", oldInfo->startTime.wSecond);

	SYSTEMTIME t28TimeSOld;
	if( 0 <= oldInfo->startTime.wHour && oldInfo->startTime.wHour < 4 ){
		GetSumTime(oldInfo->startTime, -24*60*60, &t28TimeSOld);
		t28TimeSOld.wHour+=24;
	}else{
		t28TimeSOld = oldInfo->startTime;
	}
	Format(macroData.oldSDYYYY28, L"%04d", t28TimeSOld.wYear);
	Format(macroData.oldSDYY28, L"%02d", t28TimeSOld.wYear%100);
	Format(macroData.oldSDMM28, L"%02d", t28TimeSOld.wMonth);
	Format(macroData.oldSDM28, L"%d", t28TimeSOld.wMonth);
	Format(macroData.oldSDDD28, L"%02d", t28TimeSOld.wDay);
	Format(macroData.oldSDD28, L"%d", t28TimeSOld.wDay);
	Format(macroData.oldSTHH28, L"%02d", t28TimeSOld.wHour);
	Format(macroData.oldSTH28, L"%d", t28TimeSOld.wHour);

	SYSTEMTIME endOld;
	GetI64Time(oldInfo->startTime, oldInfo->durationSecond, NULL, NULL, &endOld);
	Format(macroData.oldEDYYYY, L"%04d", endOld.wYear);
	Format(macroData.oldEDYY, L"%02d", endOld.wYear%100);
	Format(macroData.oldEDMM, L"%02d", endOld.wMonth);
	Format(macroData.oldEDM, L"%d", endOld.wMonth);
	Format(macroData.oldEDDD, L"%02d", endOld.wDay);
	Format(macroData.oldEDD, L"%d", endOld.wDay);
	Format(macroData.oldETHH, L"%02d", endOld.wHour);
	Format(macroData.oldETH, L"%d", endOld.wHour);
	Format(macroData.oldETMM, L"%02d", endOld.wMinute);
	Format(macroData.oldETM, L"%d", endOld.wMinute);
	Format(macroData.oldETSS, L"%02d", endOld.wSecond);
	Format(macroData.oldETS, L"%d", endOld.wSecond);

	SYSTEMTIME t28TimeEOld;
	if( 0 <= endOld.wHour && endOld.wHour < 4 ){
		GetSumTime(endOld, -24*60*60, &t28TimeEOld);
		t28TimeEOld.wHour+=24;
	}else{
		t28TimeEOld = endOld;
	}
	Format(macroData.oldEDYYYY28, L"%04d", t28TimeEOld.wYear);
	Format(macroData.oldEDYY28, L"%02d", t28TimeEOld.wYear%100);
	Format(macroData.oldEDMM28, L"%02d", t28TimeEOld.wMonth);
	Format(macroData.oldEDM28, L"%d", t28TimeEOld.wMonth);
	Format(macroData.oldEDDD28, L"%02d", t28TimeEOld.wDay);
	Format(macroData.oldEDD28, L"%d", t28TimeEOld.wDay);
	Format(macroData.oldETHH28, L"%02d", t28TimeEOld.wHour);
	Format(macroData.oldETH28, L"%d", t28TimeEOld.wHour);

	macroData.oldServiceName = oldInfo->stationName;
	Format(macroData.oldONID10, L"%d", oldInfo->originalNetworkID);
	Format(macroData.oldTSID10, L"%d", oldInfo->transportStreamID);
	Format(macroData.oldSID10, L"%d", oldInfo->serviceID);
	Format(macroData.oldEID10, L"%d", oldInfo->eventID);
	Format(macroData.oldONID16, L"%04X", oldInfo->originalNetworkID);
	Format(macroData.oldTSID16, L"%04X", oldInfo->transportStreamID);
	Format(macroData.oldSID16, L"%04X", oldInfo->serviceID);
	Format(macroData.oldEID16, L"%04X", oldInfo->eventID);

	Format(macroData.oldDUHH, L"%02d", oldInfo->durationSecond/(60*60));
	Format(macroData.oldDUH, L"%d", oldInfo->durationSecond/(60*60));
	Format(macroData.oldDUMM, L"%02d", (oldInfo->durationSecond%(60*60))/60);
	Format(macroData.oldDUM, L"%d", (oldInfo->durationSecond%(60*60))/60);
	Format(macroData.oldDUSS, L"%02d", oldInfo->durationSecond%60);
	Format(macroData.oldDUS, L"%d", oldInfo->durationSecond%60);	

	macroData.newTitle = newInfo->title;
	Format(macroData.newSDYYYY, L"%04d", newInfo->startTime.wYear);
	Format(macroData.newSDYY, L"%02d", newInfo->startTime.wYear%100);
	Format(macroData.newSDMM, L"%02d", newInfo->startTime.wMonth);
	Format(macroData.newSDM, L"%d", newInfo->startTime.wMonth);
	Format(macroData.newSDDD, L"%02d", newInfo->startTime.wDay);
	Format(macroData.newSDD, L"%d", newInfo->startTime.wDay);
	Format(macroData.newSTHH, L"%02d", newInfo->startTime.wHour);
	Format(macroData.newSTH, L"%d", newInfo->startTime.wHour);
	Format(macroData.newSTMM, L"%02d", newInfo->startTime.wMinute);
	Format(macroData.newSTM, L"%d", newInfo->startTime.wMinute);
	Format(macroData.newSTSS, L"%02d", newInfo->startTime.wSecond);
	Format(macroData.newSTS, L"%d", newInfo->startTime.wSecond);

	SYSTEMTIME t28TimeSNew;
	if( 0 <= newInfo->startTime.wHour && newInfo->startTime.wHour < 4 ){
		GetSumTime(newInfo->startTime, -24*60*60, &t28TimeSNew);
		t28TimeSNew.wHour+=24;
	}else{
		t28TimeSNew = newInfo->startTime;
	}
	Format(macroData.newSDYYYY28, L"%04d", t28TimeSNew.wYear);
	Format(macroData.newSDYY28, L"%02d", t28TimeSNew.wYear%100);
	Format(macroData.newSDMM28, L"%02d", t28TimeSNew.wMonth);
	Format(macroData.newSDM28, L"%d", t28TimeSNew.wMonth);
	Format(macroData.newSDDD28, L"%02d", t28TimeSNew.wDay);
	Format(macroData.newSDD28, L"%d", t28TimeSNew.wDay);
	Format(macroData.newSTHH28, L"%02d", t28TimeSNew.wHour);
	Format(macroData.newSTH28, L"%d", t28TimeSNew.wHour);

	SYSTEMTIME endNew;
	GetI64Time(newInfo->startTime, newInfo->durationSecond, NULL, NULL, &endNew);
	Format(macroData.newEDYYYY, L"%04d", endNew.wYear);
	Format(macroData.newEDYY, L"%02d", endNew.wYear%100);
	Format(macroData.newEDMM, L"%02d", endNew.wMonth);
	Format(macroData.newEDM, L"%d", endNew.wMonth);
	Format(macroData.newEDDD, L"%02d", endNew.wDay);
	Format(macroData.newEDD, L"%d", endNew.wDay);
	Format(macroData.newETHH, L"%02d", endNew.wHour);
	Format(macroData.newETH, L"%d", endNew.wHour);
	Format(macroData.newETMM, L"%02d", endNew.wMinute);
	Format(macroData.newETM, L"%d", endNew.wMinute);
	Format(macroData.newETSS, L"%02d", endNew.wSecond);
	Format(macroData.newETS, L"%d", endNew.wSecond);

	SYSTEMTIME t28TimeENew;
	if( 0 <= endNew.wHour && endNew.wHour < 4 ){
		GetSumTime(endNew, -24*60*60, &t28TimeENew);
		t28TimeENew.wHour+=24;
	}else{
		t28TimeENew = endNew;
	}
	Format(macroData.newEDYYYY28, L"%04d", t28TimeENew.wYear);
	Format(macroData.newEDYY28, L"%02d", t28TimeENew.wYear%100);
	Format(macroData.newEDMM28, L"%02d", t28TimeENew.wMonth);
	Format(macroData.newEDM28, L"%d", t28TimeENew.wMonth);
	Format(macroData.newEDDD28, L"%02d", t28TimeENew.wDay);
	Format(macroData.newEDD28, L"%d", t28TimeENew.wDay);
	Format(macroData.newETHH28, L"%02d", t28TimeENew.wHour);
	Format(macroData.newETH28, L"%d", t28TimeENew.wHour);

	macroData.newServiceName = newInfo->stationName;
	Format(macroData.newONID10, L"%d", newInfo->originalNetworkID);
	Format(macroData.newTSID10, L"%d", newInfo->transportStreamID);
	Format(macroData.newSID10, L"%d", newInfo->serviceID);
	Format(macroData.newEID10, L"%d", newInfo->eventID);
	Format(macroData.newONID16, L"%04X", newInfo->originalNetworkID);
	Format(macroData.newTSID16, L"%04X", newInfo->transportStreamID);
	Format(macroData.newSID16, L"%04X", newInfo->serviceID);
	Format(macroData.newEID16, L"%04X", newInfo->eventID);

	Format(macroData.newDUHH, L"%02d", newInfo->durationSecond/(60*60));
	Format(macroData.newDUH, L"%d", newInfo->durationSecond/(60*60));
	Format(macroData.newDUMM, L"%02d", (newInfo->durationSecond%(60*60))/60);
	Format(macroData.newDUM, L"%d", (newInfo->durationSecond%(60*60))/60);
	Format(macroData.newDUSS, L"%02d", newInfo->durationSecond%60);
	Format(macroData.newDUS, L"%d", newInfo->durationSecond%60);

	ConvertText(text, &macroData);
}

void CTwitterManager::CreateTextChgResRec(wstring& text, RESERVE_DATA* oldInfo, RESERVE_DATA* newInfo, EPGDB_EVENT_INFO* eventInfo)
{
	wstring iniAppPath = L"";
	GetModuleIniPath(iniAppPath);

	if( GetPrivateProfileInt(L"TWITTER_MACRO", L"Use2", 0, iniAppPath.c_str() ) == 0 ){
		return ;
	}

	WCHAR buff[1024] = L"";
	GetPrivateProfileString(L"TWITTER_MACRO", L"MACRO2", L"œ’Ç]F$ServiceNameOLD$ $SDYYYYOLD$/$SDMOLD$/$SDDOLD$ $STHOLD$:$STMOLD$ ` $ETHOLD$:$ETMOLD$ ¨ $SEYMDHMNEW$ $TitleNEW$", buff, 1024, iniAppPath.c_str());

	text = buff;

	MACRO_DATA macroData;

	macroData.oldTitle = oldInfo->title;
	Format(macroData.oldSDYYYY, L"%04d", oldInfo->startTime.wYear);
	Format(macroData.oldSDYY, L"%02d", oldInfo->startTime.wYear%100);
	Format(macroData.oldSDMM, L"%02d", oldInfo->startTime.wMonth);
	Format(macroData.oldSDM, L"%d", oldInfo->startTime.wMonth);
	Format(macroData.oldSDDD, L"%02d", oldInfo->startTime.wDay);
	Format(macroData.oldSDD, L"%d", oldInfo->startTime.wDay);
	Format(macroData.oldSTHH, L"%02d", oldInfo->startTime.wHour);
	Format(macroData.oldSTH, L"%d", oldInfo->startTime.wHour);
	Format(macroData.oldSTMM, L"%02d", oldInfo->startTime.wMinute);
	Format(macroData.oldSTM, L"%d", oldInfo->startTime.wMinute);
	Format(macroData.oldSTSS, L"%02d", oldInfo->startTime.wSecond);
	Format(macroData.oldSTS, L"%d", oldInfo->startTime.wSecond);

	SYSTEMTIME t28TimeSOld;
	if( 0 <= oldInfo->startTime.wHour && oldInfo->startTime.wHour < 4 ){
		GetSumTime(oldInfo->startTime, -24*60*60, &t28TimeSOld);
		t28TimeSOld.wHour+=24;
	}else{
		t28TimeSOld = oldInfo->startTime;
	}
	Format(macroData.oldSDYYYY28, L"%04d", t28TimeSOld.wYear);
	Format(macroData.oldSDYY28, L"%02d", t28TimeSOld.wYear%100);
	Format(macroData.oldSDMM28, L"%02d", t28TimeSOld.wMonth);
	Format(macroData.oldSDM28, L"%d", t28TimeSOld.wMonth);
	Format(macroData.oldSDDD28, L"%02d", t28TimeSOld.wDay);
	Format(macroData.oldSDD28, L"%d", t28TimeSOld.wDay);
	Format(macroData.oldSTHH28, L"%02d", t28TimeSOld.wHour);
	Format(macroData.oldSTH28, L"%d", t28TimeSOld.wHour);

	SYSTEMTIME endOld;
	GetI64Time(oldInfo->startTime, oldInfo->durationSecond, NULL, NULL, &endOld);
	Format(macroData.oldEDYYYY, L"%04d", endOld.wYear);
	Format(macroData.oldEDYY, L"%02d", endOld.wYear%100);
	Format(macroData.oldEDMM, L"%02d", endOld.wMonth);
	Format(macroData.oldEDM, L"%d", endOld.wMonth);
	Format(macroData.oldEDDD, L"%02d", endOld.wDay);
	Format(macroData.oldEDD, L"%d", endOld.wDay);
	Format(macroData.oldETHH, L"%02d", endOld.wHour);
	Format(macroData.oldETH, L"%d", endOld.wHour);
	Format(macroData.oldETMM, L"%02d", endOld.wMinute);
	Format(macroData.oldETM, L"%d", endOld.wMinute);
	Format(macroData.oldETSS, L"%02d", endOld.wSecond);
	Format(macroData.oldETS, L"%d", endOld.wSecond);

	SYSTEMTIME t28TimeEOld;
	if( 0 <= endOld.wHour && endOld.wHour < 4 ){
		GetSumTime(endOld, -24*60*60, &t28TimeEOld);
		t28TimeEOld.wHour+=24;
	}else{
		t28TimeEOld = endOld;
	}
	Format(macroData.oldEDYYYY28, L"%04d", t28TimeEOld.wYear);
	Format(macroData.oldEDYY28, L"%02d", t28TimeEOld.wYear%100);
	Format(macroData.oldEDMM28, L"%02d", t28TimeEOld.wMonth);
	Format(macroData.oldEDM28, L"%d", t28TimeEOld.wMonth);
	Format(macroData.oldEDDD28, L"%02d", t28TimeEOld.wDay);
	Format(macroData.oldEDD28, L"%d", t28TimeEOld.wDay);
	Format(macroData.oldETHH28, L"%02d", t28TimeEOld.wHour);
	Format(macroData.oldETH28, L"%d", t28TimeEOld.wHour);

	macroData.oldServiceName = oldInfo->stationName;
	Format(macroData.oldONID10, L"%d", oldInfo->originalNetworkID);
	Format(macroData.oldTSID10, L"%d", oldInfo->transportStreamID);
	Format(macroData.oldSID10, L"%d", oldInfo->serviceID);
	Format(macroData.oldEID10, L"%d", oldInfo->eventID);
	Format(macroData.oldONID16, L"%04X", oldInfo->originalNetworkID);
	Format(macroData.oldTSID16, L"%04X", oldInfo->transportStreamID);
	Format(macroData.oldSID16, L"%04X", oldInfo->serviceID);
	Format(macroData.oldEID16, L"%04X", oldInfo->eventID);

	Format(macroData.oldDUHH, L"%02d", oldInfo->durationSecond/(60*60));
	Format(macroData.oldDUH, L"%d", oldInfo->durationSecond/(60*60));
	Format(macroData.oldDUMM, L"%02d", (oldInfo->durationSecond%(60*60))/60);
	Format(macroData.oldDUM, L"%d", (oldInfo->durationSecond%(60*60))/60);
	Format(macroData.oldDUSS, L"%02d", oldInfo->durationSecond%60);
	Format(macroData.oldDUS, L"%d", oldInfo->durationSecond%60);	

	macroData.newTitle = newInfo->title;
	Format(macroData.newSDYYYY, L"%04d", newInfo->startTime.wYear);
	Format(macroData.newSDYY, L"%02d", newInfo->startTime.wYear%100);
	Format(macroData.newSDMM, L"%02d", newInfo->startTime.wMonth);
	Format(macroData.newSDM, L"%d", newInfo->startTime.wMonth);
	Format(macroData.newSDDD, L"%02d", newInfo->startTime.wDay);
	Format(macroData.newSDD, L"%d", newInfo->startTime.wDay);
	Format(macroData.newSTHH, L"%02d", newInfo->startTime.wHour);
	Format(macroData.newSTH, L"%d", newInfo->startTime.wHour);
	Format(macroData.newSTMM, L"%02d", newInfo->startTime.wMinute);
	Format(macroData.newSTM, L"%d", newInfo->startTime.wMinute);
	Format(macroData.newSTSS, L"%02d", newInfo->startTime.wSecond);
	Format(macroData.newSTS, L"%d", newInfo->startTime.wSecond);

	SYSTEMTIME t28TimeSNew;
	if( 0 <= newInfo->startTime.wHour && newInfo->startTime.wHour < 4 ){
		GetSumTime(newInfo->startTime, -24*60*60, &t28TimeSNew);
		t28TimeSNew.wHour+=24;
	}else{
		t28TimeSNew = newInfo->startTime;
	}
	Format(macroData.newSDYYYY28, L"%04d", t28TimeSNew.wYear);
	Format(macroData.newSDYY28, L"%02d", t28TimeSNew.wYear%100);
	Format(macroData.newSDMM28, L"%02d", t28TimeSNew.wMonth);
	Format(macroData.newSDM28, L"%d", t28TimeSNew.wMonth);
	Format(macroData.newSDDD28, L"%02d", t28TimeSNew.wDay);
	Format(macroData.newSDD28, L"%d", t28TimeSNew.wDay);
	Format(macroData.newSTHH28, L"%02d", t28TimeSNew.wHour);
	Format(macroData.newSTH28, L"%d", t28TimeSNew.wHour);

	SYSTEMTIME endNew;
	GetI64Time(newInfo->startTime, newInfo->durationSecond, NULL, NULL, &endNew);
	Format(macroData.newEDYYYY, L"%04d", endNew.wYear);
	Format(macroData.newEDYY, L"%02d", endNew.wYear%100);
	Format(macroData.newEDMM, L"%02d", endNew.wMonth);
	Format(macroData.newEDM, L"%d", endNew.wMonth);
	Format(macroData.newEDDD, L"%02d", endNew.wDay);
	Format(macroData.newEDD, L"%d", endNew.wDay);
	Format(macroData.newETHH, L"%02d", endNew.wHour);
	Format(macroData.newETH, L"%d", endNew.wHour);
	Format(macroData.newETMM, L"%02d", endNew.wMinute);
	Format(macroData.newETM, L"%d", endNew.wMinute);
	Format(macroData.newETSS, L"%02d", endNew.wSecond);
	Format(macroData.newETS, L"%d", endNew.wSecond);

	SYSTEMTIME t28TimeENew;
	if( 0 <= endNew.wHour && endNew.wHour < 4 ){
		GetSumTime(endNew, -24*60*60, &t28TimeENew);
		t28TimeENew.wHour+=24;
	}else{
		t28TimeENew = endNew;
	}
	Format(macroData.newEDYYYY28, L"%04d", t28TimeENew.wYear);
	Format(macroData.newEDYY28, L"%02d", t28TimeENew.wYear%100);
	Format(macroData.newEDMM28, L"%02d", t28TimeENew.wMonth);
	Format(macroData.newEDM28, L"%d", t28TimeENew.wMonth);
	Format(macroData.newEDDD28, L"%02d", t28TimeENew.wDay);
	Format(macroData.newEDD28, L"%d", t28TimeENew.wDay);
	Format(macroData.newETHH28, L"%02d", t28TimeENew.wHour);
	Format(macroData.newETH28, L"%d", t28TimeENew.wHour);

	macroData.newServiceName = newInfo->stationName;
	Format(macroData.newONID10, L"%d", newInfo->originalNetworkID);
	Format(macroData.newTSID10, L"%d", newInfo->transportStreamID);
	Format(macroData.newSID10, L"%d", newInfo->serviceID);
	Format(macroData.newEID10, L"%d", newInfo->eventID);
	Format(macroData.newONID16, L"%04X", newInfo->originalNetworkID);
	Format(macroData.newTSID16, L"%04X", newInfo->transportStreamID);
	Format(macroData.newSID16, L"%04X", newInfo->serviceID);
	Format(macroData.newEID16, L"%04X", newInfo->eventID);

	Format(macroData.newDUHH, L"%02d", newInfo->durationSecond/(60*60));
	Format(macroData.newDUH, L"%d", newInfo->durationSecond/(60*60));
	Format(macroData.newDUMM, L"%02d", (newInfo->durationSecond%(60*60))/60);
	Format(macroData.newDUM, L"%d", (newInfo->durationSecond%(60*60))/60);
	Format(macroData.newDUSS, L"%02d", newInfo->durationSecond%60);
	Format(macroData.newDUS, L"%d", newInfo->durationSecond%60);

	if( eventInfo->StartTimeFlag == 0 ){
		macroData.newSYMDHM = L"–¢’è";
		macroData.newSYMDHM28 = L"–¢’è";
		macroData.newEYMDHM = L"–¢’è";
		macroData.newEYMDHM28 = L"–¢’è";
		macroData.newSEYMDHM = L"–¢’è";
		macroData.newSEYMDHM28 = L"–¢’è";
	}else{
		Format(macroData.newSYMDHM, L"%d/%d/%d %d:%d",
			eventInfo->start_time.wYear,
			eventInfo->start_time.wMonth,
			eventInfo->start_time.wDay,
			eventInfo->start_time.wHour,
			eventInfo->start_time.wMinute
			);

		SYSTEMTIME t28TimeSPG;
		if( 0 <= eventInfo->start_time.wHour && eventInfo->start_time.wHour < 4 ){
			GetSumTime(eventInfo->start_time, -24*60*60, &t28TimeSPG);
			t28TimeSPG.wHour+=24;
		}else{
			t28TimeSPG = eventInfo->start_time;
		}
		Format(macroData.newSYMDHM28, L"%d/%d/%d %d:%d",
			t28TimeSPG.wYear,
			t28TimeSPG.wMonth,
			t28TimeSPG.wDay,
			t28TimeSPG.wHour,
			t28TimeSPG.wMinute
			);

		if( eventInfo->DurationFlag == 0 ){
			macroData.newEYMDHM = L"–¢’è";
			macroData.newEYMDHM28 = L"–¢’è";

			Format(macroData.newSEYMDHM, L"%d/%d/%d %d:%d ` –¢’è",
				eventInfo->start_time.wYear,
				eventInfo->start_time.wMonth,
				eventInfo->start_time.wDay,
				eventInfo->start_time.wHour,
				eventInfo->start_time.wMinute
				);
			Format(macroData.newSEYMDHM28, L"%d/%d/%d %d:%d ` –¢’è",
				t28TimeSPG.wYear,
				t28TimeSPG.wMonth,
				t28TimeSPG.wDay,
				t28TimeSPG.wHour,
				t28TimeSPG.wMinute
				);
		}else{
			SYSTEMTIME endPG;
			GetI64Time(eventInfo->start_time, eventInfo->durationSec, NULL, NULL, &endPG);

			Format(macroData.newEYMDHM, L"%d/%d/%d %d:%d",
				endPG.wYear,
				endPG.wMonth,
				endPG.wDay,
				endPG.wHour,
				endPG.wMinute
				);

			SYSTEMTIME t28TimeEPG;
			if( 0 <= endPG.wHour && endPG.wHour < 4 ){
				GetSumTime(endPG, -24*60*60, &t28TimeEPG);
				t28TimeEPG.wHour+=24;
			}else{
				t28TimeEPG = endPG;
			}
			Format(macroData.newEYMDHM28, L"%d/%d/%d %d:%d",
				t28TimeEPG.wYear,
				t28TimeEPG.wMonth,
				t28TimeEPG.wDay,
				t28TimeEPG.wHour,
				t28TimeEPG.wMinute
				);


			Format(macroData.newSEYMDHM, L"%d/%d/%d %d:%d ` %d:%d",
				eventInfo->start_time.wYear,
				eventInfo->start_time.wMonth,
				eventInfo->start_time.wDay,
				eventInfo->start_time.wHour,
				eventInfo->start_time.wMinute,
				endPG.wHour,
				endPG.wMinute
				);
			Format(macroData.newSEYMDHM28, L"%d/%d/%d %d:%d ` %d:%d",
				t28TimeSPG.wYear,
				t28TimeSPG.wMonth,
				t28TimeSPG.wDay,
				t28TimeSPG.wHour,
				t28TimeSPG.wMinute,
				t28TimeEPG.wHour,
				t28TimeEPG.wMinute
				);
		}
	}

	ConvertText(text, &macroData);
}

void CTwitterManager::CreateTextAddRes(wstring& text, RESERVE_DATA* resInfo)
{
	wstring iniAppPath = L"";
	GetModuleIniPath(iniAppPath);

	if( GetPrivateProfileInt(L"TWITTER_MACRO", L"Use3", 0, iniAppPath.c_str() ) == 0 ){
		return ;
	}

	WCHAR buff[1024] = L"";
	GetPrivateProfileString(L"TWITTER_MACRO", L"MACRO3", L"œ—\–ñ’Ç‰ÁF$ServiceNameNEW$ $SDYYYYNEW$/$SDMNEW$/$SDDNEW$ $STHNEW$:$STMNEW$ ` $ETHNEW$:$ETMNEW$ $TitleNEW$", buff, 1024, iniAppPath.c_str());

	text = buff;

	MACRO_DATA macroData;

	macroData.newTitle = resInfo->title;
	Format(macroData.newSDYYYY, L"%04d", resInfo->startTime.wYear);
	Format(macroData.newSDYY, L"%02d", resInfo->startTime.wYear%100);
	Format(macroData.newSDMM, L"%02d", resInfo->startTime.wMonth);
	Format(macroData.newSDM, L"%d", resInfo->startTime.wMonth);
	Format(macroData.newSDDD, L"%02d", resInfo->startTime.wDay);
	Format(macroData.newSDD, L"%d", resInfo->startTime.wDay);
	Format(macroData.newSTHH, L"%02d", resInfo->startTime.wHour);
	Format(macroData.newSTH, L"%d", resInfo->startTime.wHour);
	Format(macroData.newSTMM, L"%02d", resInfo->startTime.wMinute);
	Format(macroData.newSTM, L"%d", resInfo->startTime.wMinute);
	Format(macroData.newSTSS, L"%02d", resInfo->startTime.wSecond);
	Format(macroData.newSTS, L"%d", resInfo->startTime.wSecond);

	SYSTEMTIME t28TimeSNew;
	if( 0 <= resInfo->startTime.wHour && resInfo->startTime.wHour < 4 ){
		GetSumTime(resInfo->startTime, -24*60*60, &t28TimeSNew);
		t28TimeSNew.wHour+=24;
	}else{
		t28TimeSNew = resInfo->startTime;
	}
	Format(macroData.newSDYYYY28, L"%04d", t28TimeSNew.wYear);
	Format(macroData.newSDYY28, L"%02d", t28TimeSNew.wYear%100);
	Format(macroData.newSDMM28, L"%02d", t28TimeSNew.wMonth);
	Format(macroData.newSDM28, L"%d", t28TimeSNew.wMonth);
	Format(macroData.newSDDD28, L"%02d", t28TimeSNew.wDay);
	Format(macroData.newSDD28, L"%d", t28TimeSNew.wDay);
	Format(macroData.newSTHH28, L"%02d", t28TimeSNew.wHour);
	Format(macroData.newSTH28, L"%d", t28TimeSNew.wHour);

	SYSTEMTIME endNew;
	GetI64Time(resInfo->startTime, resInfo->durationSecond, NULL, NULL, &endNew);
	Format(macroData.newEDYYYY, L"%04d", endNew.wYear);
	Format(macroData.newEDYY, L"%02d", endNew.wYear%100);
	Format(macroData.newEDMM, L"%02d", endNew.wMonth);
	Format(macroData.newEDM, L"%d", endNew.wMonth);
	Format(macroData.newEDDD, L"%02d", endNew.wDay);
	Format(macroData.newEDD, L"%d", endNew.wDay);
	Format(macroData.newETHH, L"%02d", endNew.wHour);
	Format(macroData.newETH, L"%d", endNew.wHour);
	Format(macroData.newETMM, L"%02d", endNew.wMinute);
	Format(macroData.newETM, L"%d", endNew.wMinute);
	Format(macroData.newETSS, L"%02d", endNew.wSecond);
	Format(macroData.newETS, L"%d", endNew.wSecond);

	SYSTEMTIME t28TimeENew;
	if( 0 <= endNew.wHour && endNew.wHour < 4 ){
		GetSumTime(endNew, -24*60*60, &t28TimeENew);
		t28TimeENew.wHour+=24;
	}else{
		t28TimeENew = endNew;
	}
	Format(macroData.newEDYYYY28, L"%04d", t28TimeENew.wYear);
	Format(macroData.newEDYY28, L"%02d", t28TimeENew.wYear%100);
	Format(macroData.newEDMM28, L"%02d", t28TimeENew.wMonth);
	Format(macroData.newEDM28, L"%d", t28TimeENew.wMonth);
	Format(macroData.newEDDD28, L"%02d", t28TimeENew.wDay);
	Format(macroData.newEDD28, L"%d", t28TimeENew.wDay);
	Format(macroData.newETHH28, L"%02d", t28TimeENew.wHour);
	Format(macroData.newETH28, L"%d", t28TimeENew.wHour);

	macroData.newServiceName = resInfo->stationName;
	Format(macroData.newONID10, L"%d", resInfo->originalNetworkID);
	Format(macroData.newTSID10, L"%d", resInfo->transportStreamID);
	Format(macroData.newSID10, L"%d", resInfo->serviceID);
	Format(macroData.newEID10, L"%d", resInfo->eventID);
	Format(macroData.newONID16, L"%04X", resInfo->originalNetworkID);
	Format(macroData.newTSID16, L"%04X", resInfo->transportStreamID);
	Format(macroData.newSID16, L"%04X", resInfo->serviceID);
	Format(macroData.newEID16, L"%04X", resInfo->eventID);

	Format(macroData.newDUHH, L"%02d", resInfo->durationSecond/(60*60));
	Format(macroData.newDUH, L"%d", resInfo->durationSecond/(60*60));
	Format(macroData.newDUMM, L"%02d", (resInfo->durationSecond%(60*60))/60);
	Format(macroData.newDUM, L"%d", (resInfo->durationSecond%(60*60))/60);
	Format(macroData.newDUSS, L"%02d", resInfo->durationSecond%60);
	Format(macroData.newDUS, L"%d", resInfo->durationSecond%60);

	ConvertText(text, &macroData);
}

void CTwitterManager::CreateTextRecStart(wstring& text, RESERVE_DATA* resInfo)
{
	wstring iniAppPath = L"";
	GetModuleIniPath(iniAppPath);

	if( GetPrivateProfileInt(L"TWITTER_MACRO", L"Use4", 0, iniAppPath.c_str() ) == 0 ){
		return ;
	}

	WCHAR buff[1024] = L"";
	GetPrivateProfileString(L"TWITTER_MACRO", L"MACRO4", L"œ˜^‰æŠJŽnF$ServiceNameNEW$ $SDYYYYNEW$/$SDMNEW$/$SDDNEW$ $STHNEW$:$STMNEW$ ` $ETHNEW$:$ETMNEW$ $TitleNEW$", buff, 1024, iniAppPath.c_str());

	text = buff;

	MACRO_DATA macroData;

	macroData.newTitle = resInfo->title;
	Format(macroData.newSDYYYY, L"%04d", resInfo->startTime.wYear);
	Format(macroData.newSDYY, L"%02d", resInfo->startTime.wYear%100);
	Format(macroData.newSDMM, L"%02d", resInfo->startTime.wMonth);
	Format(macroData.newSDM, L"%d", resInfo->startTime.wMonth);
	Format(macroData.newSDDD, L"%02d", resInfo->startTime.wDay);
	Format(macroData.newSDD, L"%d", resInfo->startTime.wDay);
	Format(macroData.newSTHH, L"%02d", resInfo->startTime.wHour);
	Format(macroData.newSTH, L"%d", resInfo->startTime.wHour);
	Format(macroData.newSTMM, L"%02d", resInfo->startTime.wMinute);
	Format(macroData.newSTM, L"%d", resInfo->startTime.wMinute);
	Format(macroData.newSTSS, L"%02d", resInfo->startTime.wSecond);
	Format(macroData.newSTS, L"%d", resInfo->startTime.wSecond);

	SYSTEMTIME t28TimeSNew;
	if( 0 <= resInfo->startTime.wHour && resInfo->startTime.wHour < 4 ){
		GetSumTime(resInfo->startTime, -24*60*60, &t28TimeSNew);
		t28TimeSNew.wHour+=24;
	}else{
		t28TimeSNew = resInfo->startTime;
	}
	Format(macroData.newSDYYYY28, L"%04d", t28TimeSNew.wYear);
	Format(macroData.newSDYY28, L"%02d", t28TimeSNew.wYear%100);
	Format(macroData.newSDMM28, L"%02d", t28TimeSNew.wMonth);
	Format(macroData.newSDM28, L"%d", t28TimeSNew.wMonth);
	Format(macroData.newSDDD28, L"%02d", t28TimeSNew.wDay);
	Format(macroData.newSDD28, L"%d", t28TimeSNew.wDay);
	Format(macroData.newSTHH28, L"%02d", t28TimeSNew.wHour);
	Format(macroData.newSTH28, L"%d", t28TimeSNew.wHour);

	SYSTEMTIME endNew;
	GetI64Time(resInfo->startTime, resInfo->durationSecond, NULL, NULL, &endNew);
	Format(macroData.newEDYYYY, L"%04d", endNew.wYear);
	Format(macroData.newEDYY, L"%02d", endNew.wYear%100);
	Format(macroData.newEDMM, L"%02d", endNew.wMonth);
	Format(macroData.newEDM, L"%d", endNew.wMonth);
	Format(macroData.newEDDD, L"%02d", endNew.wDay);
	Format(macroData.newEDD, L"%d", endNew.wDay);
	Format(macroData.newETHH, L"%02d", endNew.wHour);
	Format(macroData.newETH, L"%d", endNew.wHour);
	Format(macroData.newETMM, L"%02d", endNew.wMinute);
	Format(macroData.newETM, L"%d", endNew.wMinute);
	Format(macroData.newETSS, L"%02d", endNew.wSecond);
	Format(macroData.newETS, L"%d", endNew.wSecond);

	SYSTEMTIME t28TimeENew;
	if( 0 <= endNew.wHour && endNew.wHour < 4 ){
		GetSumTime(endNew, -24*60*60, &t28TimeENew);
		t28TimeENew.wHour+=24;
	}else{
		t28TimeENew = endNew;
	}
	Format(macroData.newEDYYYY28, L"%04d", t28TimeENew.wYear);
	Format(macroData.newEDYY28, L"%02d", t28TimeENew.wYear%100);
	Format(macroData.newEDMM28, L"%02d", t28TimeENew.wMonth);
	Format(macroData.newEDM28, L"%d", t28TimeENew.wMonth);
	Format(macroData.newEDDD28, L"%02d", t28TimeENew.wDay);
	Format(macroData.newEDD28, L"%d", t28TimeENew.wDay);
	Format(macroData.newETHH28, L"%02d", t28TimeENew.wHour);
	Format(macroData.newETH28, L"%d", t28TimeENew.wHour);

	macroData.newServiceName = resInfo->stationName;
	Format(macroData.newONID10, L"%d", resInfo->originalNetworkID);
	Format(macroData.newTSID10, L"%d", resInfo->transportStreamID);
	Format(macroData.newSID10, L"%d", resInfo->serviceID);
	Format(macroData.newEID10, L"%d", resInfo->eventID);
	Format(macroData.newONID16, L"%04X", resInfo->originalNetworkID);
	Format(macroData.newTSID16, L"%04X", resInfo->transportStreamID);
	Format(macroData.newSID16, L"%04X", resInfo->serviceID);
	Format(macroData.newEID16, L"%04X", resInfo->eventID);

	Format(macroData.newDUHH, L"%02d", resInfo->durationSecond/(60*60));
	Format(macroData.newDUH, L"%d", resInfo->durationSecond/(60*60));
	Format(macroData.newDUMM, L"%02d", (resInfo->durationSecond%(60*60))/60);
	Format(macroData.newDUM, L"%d", (resInfo->durationSecond%(60*60))/60);
	Format(macroData.newDUSS, L"%02d", resInfo->durationSecond%60);
	Format(macroData.newDUS, L"%d", resInfo->durationSecond%60);

	ConvertText(text, &macroData);
}

void CTwitterManager::CreateTextRecStop(wstring& text, REC_FILE_INFO* resInfo)
{
	wstring iniAppPath = L"";
	GetModuleIniPath(iniAppPath);

	if( GetPrivateProfileInt(L"TWITTER_MACRO", L"Use5", 0, iniAppPath.c_str() ) == 0 ){
		return ;
	}

	WCHAR buff[1024] = L"";
	GetPrivateProfileString(L"TWITTER_MACRO", L"MACRO5", L"œ˜^‰æI—¹F$ServiceNameNEW$ $SDYYYYNEW$/$SDMNEW$/$SDDNEW$ $STHNEW$:$STMNEW$ ` $ETHNEW$:$ETMNEW$ $Result$ Drops:$Drops$ Scrambles:$Scrambles$ $TitleNEW$", buff, 1024, iniAppPath.c_str());

	text = buff;

	MACRO_DATA macroData;

	macroData.newTitle = resInfo->title;
	Format(macroData.newSDYYYY, L"%04d", resInfo->startTime.wYear);
	Format(macroData.newSDYY, L"%02d", resInfo->startTime.wYear%100);
	Format(macroData.newSDMM, L"%02d", resInfo->startTime.wMonth);
	Format(macroData.newSDM, L"%d", resInfo->startTime.wMonth);
	Format(macroData.newSDDD, L"%02d", resInfo->startTime.wDay);
	Format(macroData.newSDD, L"%d", resInfo->startTime.wDay);
	Format(macroData.newSTHH, L"%02d", resInfo->startTime.wHour);
	Format(macroData.newSTH, L"%d", resInfo->startTime.wHour);
	Format(macroData.newSTMM, L"%02d", resInfo->startTime.wMinute);
	Format(macroData.newSTM, L"%d", resInfo->startTime.wMinute);
	Format(macroData.newSTSS, L"%02d", resInfo->startTime.wSecond);
	Format(macroData.newSTS, L"%d", resInfo->startTime.wSecond);

	SYSTEMTIME t28TimeSNew;
	if( 0 <= resInfo->startTime.wHour && resInfo->startTime.wHour < 4 ){
		GetSumTime(resInfo->startTime, -24*60*60, &t28TimeSNew);
		t28TimeSNew.wHour+=24;
	}else{
		t28TimeSNew = resInfo->startTime;
	}
	Format(macroData.newSDYYYY28, L"%04d", t28TimeSNew.wYear);
	Format(macroData.newSDYY28, L"%02d", t28TimeSNew.wYear%100);
	Format(macroData.newSDMM28, L"%02d", t28TimeSNew.wMonth);
	Format(macroData.newSDM28, L"%d", t28TimeSNew.wMonth);
	Format(macroData.newSDDD28, L"%02d", t28TimeSNew.wDay);
	Format(macroData.newSDD28, L"%d", t28TimeSNew.wDay);
	Format(macroData.newSTHH28, L"%02d", t28TimeSNew.wHour);
	Format(macroData.newSTH28, L"%d", t28TimeSNew.wHour);

	SYSTEMTIME endNew;
	GetI64Time(resInfo->startTime, resInfo->durationSecond, NULL, NULL, &endNew);
	Format(macroData.newEDYYYY, L"%04d", endNew.wYear);
	Format(macroData.newEDYY, L"%02d", endNew.wYear%100);
	Format(macroData.newEDMM, L"%02d", endNew.wMonth);
	Format(macroData.newEDM, L"%d", endNew.wMonth);
	Format(macroData.newEDDD, L"%02d", endNew.wDay);
	Format(macroData.newEDD, L"%d", endNew.wDay);
	Format(macroData.newETHH, L"%02d", endNew.wHour);
	Format(macroData.newETH, L"%d", endNew.wHour);
	Format(macroData.newETMM, L"%02d", endNew.wMinute);
	Format(macroData.newETM, L"%d", endNew.wMinute);
	Format(macroData.newETSS, L"%02d", endNew.wSecond);
	Format(macroData.newETS, L"%d", endNew.wSecond);

	SYSTEMTIME t28TimeENew;
	if( 0 <= endNew.wHour && endNew.wHour < 4 ){
		GetSumTime(endNew, -24*60*60, &t28TimeENew);
		t28TimeENew.wHour+=24;
	}else{
		t28TimeENew = endNew;
	}
	Format(macroData.newEDYYYY28, L"%04d", t28TimeENew.wYear);
	Format(macroData.newEDYY28, L"%02d", t28TimeENew.wYear%100);
	Format(macroData.newEDMM28, L"%02d", t28TimeENew.wMonth);
	Format(macroData.newEDM28, L"%d", t28TimeENew.wMonth);
	Format(macroData.newEDDD28, L"%02d", t28TimeENew.wDay);
	Format(macroData.newEDD28, L"%d", t28TimeENew.wDay);
	Format(macroData.newETHH28, L"%02d", t28TimeENew.wHour);
	Format(macroData.newETH28, L"%d", t28TimeENew.wHour);

	macroData.newServiceName = resInfo->serviceName;
	Format(macroData.newONID10, L"%d", resInfo->originalNetworkID);
	Format(macroData.newTSID10, L"%d", resInfo->transportStreamID);
	Format(macroData.newSID10, L"%d", resInfo->serviceID);
	Format(macroData.newEID10, L"%d", resInfo->eventID);
	Format(macroData.newONID16, L"%04X", resInfo->originalNetworkID);
	Format(macroData.newTSID16, L"%04X", resInfo->transportStreamID);
	Format(macroData.newSID16, L"%04X", resInfo->serviceID);
	Format(macroData.newEID16, L"%04X", resInfo->eventID);

	Format(macroData.newDUHH, L"%02d", resInfo->durationSecond/(60*60));
	Format(macroData.newDUH, L"%d", resInfo->durationSecond/(60*60));
	Format(macroData.newDUMM, L"%02d", (resInfo->durationSecond%(60*60))/60);
	Format(macroData.newDUM, L"%d", (resInfo->durationSecond%(60*60))/60);
	Format(macroData.newDUSS, L"%02d", resInfo->durationSecond%60);
	Format(macroData.newDUS, L"%d", resInfo->durationSecond%60);

	Format(macroData.drops, L"%I64d", resInfo->drops);
	Format(macroData.scrambles, L"%I64d", resInfo->scrambles);
	macroData.result = resInfo->comment;

	ConvertText(text, &macroData);
}

void CTwitterManager::ConvertText(wstring& text, MACRO_DATA* macroData)
{
	if(text.find(L"$TitleOLD$") != string::npos ){
		Replace(text, L"$TitleOLD$", macroData->oldTitle);
	}
	if(text.find(L"$SDYYYYOLD$") != string::npos ){
		Replace(text, L"$SDYYYYOLD$", macroData->oldSDYYYY);
	}
	if(text.find(L"$SDYYOLD$") != string::npos ){
		Replace(text, L"$SDYYOLD$", macroData->oldSDYY);
	}
	if(text.find(L"$SDMMOLD$") != string::npos ){
		Replace(text, L"$SDMMOLD$", macroData->oldSDMM);
	}
	if(text.find(L"$SDMOLD$") != string::npos ){
		Replace(text, L"$SDMOLD$", macroData->oldSDM);
	}
	if(text.find(L"$SDDDOLD$") != string::npos ){
		Replace(text, L"$SDDDOLD$", macroData->oldSDDD);
	}
	if(text.find(L"$SDDOLD$") != string::npos ){
		Replace(text, L"$SDDOLD$", macroData->oldSDD);
	}
	if(text.find(L"$STHHOLD$") != string::npos ){
		Replace(text, L"$STHHOLD$", macroData->oldSTHH);
	}
	if(text.find(L"$STHOLD$") != string::npos ){
		Replace(text, L"$STHOLD$", macroData->oldSTH);
	}
	if(text.find(L"$STMMOLD$") != string::npos ){
		Replace(text, L"$STMMOLD$", macroData->oldSTMM);
	}
	if(text.find(L"$STMOLD$") != string::npos ){
		Replace(text, L"$STMOLD$", macroData->oldSTM);
	}
	if(text.find(L"$STSSOLD$") != string::npos ){
		Replace(text, L"$STSSOLD$", macroData->oldSTSS);
	}
	if(text.find(L"$STSOLD$") != string::npos ){
		Replace(text, L"$STSOLD$", macroData->oldSTS);
	}
	if(text.find(L"$EDYYYYOLD$") != string::npos ){
		Replace(text, L"$EDYYYYOLD$", macroData->oldEDYYYY);
	}
	if(text.find(L"$EDYYOLD$") != string::npos ){
		Replace(text, L"$EDYYOLD$", macroData->oldEDYY);
	}
	if(text.find(L"$EDMMOLD$") != string::npos ){
		Replace(text, L"$EDMMOLD$", macroData->oldEDMM);
	}
	if(text.find(L"$EDMOLD$") != string::npos ){
		Replace(text, L"$EDMOLD$", macroData->oldEDM);
	}
	if(text.find(L"$EDDDOLD$") != string::npos ){
		Replace(text, L"$EDDDOLD$", macroData->oldEDDD);
	}
	if(text.find(L"$EDDOLD$") != string::npos ){
		Replace(text, L"$EDDOLD$", macroData->oldEDD);
	}
	if(text.find(L"$ETHHOLD$") != string::npos ){
		Replace(text, L"$ETHHOLD$", macroData->oldETHH);
	}
	if(text.find(L"$ETHOLD$") != string::npos ){
		Replace(text, L"$ETHOLD$", macroData->oldETH);
	}
	if(text.find(L"$ETMMOLD$") != string::npos ){
		Replace(text, L"$ETMMOLD$", macroData->oldETMM);
	}
	if(text.find(L"$ETMOLD$") != string::npos ){
		Replace(text, L"$ETMOLD$", macroData->oldETM);
	}
	if(text.find(L"$ETSSOLD$") != string::npos ){
		Replace(text, L"$ETSSOLD$", macroData->oldETSS);
	}
	if(text.find(L"$ETSOLD$") != string::npos ){
		Replace(text, L"$ETSOLD$", macroData->oldETS);
	}
	if(text.find(L"$ONID10OLD$") != string::npos ){
		Replace(text, L"$ONID10OLD$", macroData->oldONID10);
	}
	if(text.find(L"$TSID10OLD$") != string::npos ){
		Replace(text, L"$TSID10OLD$", macroData->oldTSID10);
	}
	if(text.find(L"$SID10OLD$") != string::npos ){
		Replace(text, L"$SID10OLD$", macroData->oldSID10);
	}
	if(text.find(L"$EID10OLD$") != string::npos ){
		Replace(text, L"$EID10OLD$", macroData->oldEID10);
	}
	if(text.find(L"$ONID16OLD$") != string::npos ){
		Replace(text, L"$ONID16OLD$", macroData->oldONID16);
	}
	if(text.find(L"$TSID16OLD$") != string::npos ){
		Replace(text, L"$TSID16OLD$", macroData->oldTSID16);
	}
	if(text.find(L"$SID16OLD$") != string::npos ){
		Replace(text, L"$SID16OLD$", macroData->oldSID16);
	}
	if(text.find(L"$EID16OLD$") != string::npos ){
		Replace(text, L"$EID16OLD$", macroData->oldEID16);
	}
	if(text.find(L"$ServiceNameOLD$") != string::npos ){
		Replace(text, L"$ServiceNameOLD$", macroData->oldServiceName);
	}
	if(text.find(L"$SDYYYYOLD28$") != string::npos ){
		Replace(text, L"$SDYYYYOLD28$", macroData->oldSDYYYY28);
	}
	if(text.find(L"$SDYYOLD28$") != string::npos ){
		Replace(text, L"$SDYYOLD28$", macroData->oldSDYY28);
	}
	if(text.find(L"$SDMMOLD28$") != string::npos ){
		Replace(text, L"$SDMMOLD28$", macroData->oldSDMM28);
	}
	if(text.find(L"$SDMOLD28$") != string::npos ){
		Replace(text, L"$SDMOLD28$", macroData->oldSDM28);
	}
	if(text.find(L"$SDDDOLD28$") != string::npos ){
		Replace(text, L"$SDDDOLD28$", macroData->oldSDDD28);
	}
	if(text.find(L"$SDDOLD28$") != string::npos ){
		Replace(text, L"$SDDOLD28$", macroData->oldSDD28);
	}
	if(text.find(L"$STHHOLD28$") != string::npos ){
		Replace(text, L"$STHHOLD28$", macroData->oldSTHH28);
	}
	if(text.find(L"$STHOLD28$") != string::npos ){
		Replace(text, L"$STHOLD28$", macroData->oldSTH28);
	}
	if(text.find(L"$EDYYYYOLD28$") != string::npos ){
		Replace(text, L"$EDYYYYOLD28$", macroData->oldEDYYYY28);
	}
	if(text.find(L"$EDYYOLD28$") != string::npos ){
		Replace(text, L"$EDYYOLD28$", macroData->oldEDYY28);
	}
	if(text.find(L"$EDMMOLD28$") != string::npos ){
		Replace(text, L"$EDMMOLD28$", macroData->oldEDMM28);
	}
	if(text.find(L"$EDMOLD28$") != string::npos ){
		Replace(text, L"$EDMOLD28$", macroData->oldEDM28);
	}
	if(text.find(L"$EDDDOLD28$") != string::npos ){
		Replace(text, L"$EDDDOLD28$", macroData->oldEDDD28);
	}
	if(text.find(L"$EDDOLD28$") != string::npos ){
		Replace(text, L"$EDDOLD28$", macroData->oldEDD28);
	}
	if(text.find(L"$ETHHOLD28$") != string::npos ){
		Replace(text, L"$ETHHOLD28$", macroData->oldETHH28);
	}
	if(text.find(L"$ETHOLD28$") != string::npos ){
		Replace(text, L"$ETHOLD28$", macroData->oldETH28);
	}
	if(text.find(L"$DUHHOLD$") != string::npos ){
		Replace(text, L"$DUHHOLD$", macroData->oldDUHH);
	}
	if(text.find(L"$DUHOLD$") != string::npos ){
		Replace(text, L"$DUHOLD$", macroData->oldDUH);
	}
	if(text.find(L"$DUMMOLD$") != string::npos ){
		Replace(text, L"$DUMMOLD$", macroData->oldDUMM);
	}
	if(text.find(L"$DUMOLD$") != string::npos ){
		Replace(text, L"$DUMOLD$", macroData->oldDUM);
	}
	if(text.find(L"$DUSSOLD$") != string::npos ){
		Replace(text, L"$DUSSOLD$", macroData->oldDUSS);
	}
	if(text.find(L"$DUSOLD$") != string::npos ){
		Replace(text, L"$DUSOLD$", macroData->oldDUS);
	}

	if(text.find(L"$TitleNEW$") != string::npos ){
		Replace(text, L"$TitleNEW$", macroData->newTitle);
	}
	if(text.find(L"$SDYYYYNEW$") != string::npos ){
		Replace(text, L"$SDYYYYNEW$", macroData->newSDYYYY);
	}
	if(text.find(L"$SDYYNEW$") != string::npos ){
		Replace(text, L"$SDYYNEW$", macroData->newSDYY);
	}
	if(text.find(L"$SDMMNEW$") != string::npos ){
		Replace(text, L"$SDMMNEW$", macroData->newSDMM);
	}
	if(text.find(L"$SDMNEW$") != string::npos ){
		Replace(text, L"$SDMNEW$", macroData->newSDM);
	}
	if(text.find(L"$SDDDNEW$") != string::npos ){
		Replace(text, L"$SDDDNEW$", macroData->newSDDD);
	}
	if(text.find(L"$SDDNEW$") != string::npos ){
		Replace(text, L"$SDDNEW$", macroData->newSDD);
	}
	if(text.find(L"$STHHNEW$") != string::npos ){
		Replace(text, L"$STHHNEW$", macroData->newSTHH);
	}
	if(text.find(L"$STHNEW$") != string::npos ){
		Replace(text, L"$STHNEW$", macroData->newSTH);
	}
	if(text.find(L"$STMMNEW$") != string::npos ){
		Replace(text, L"$STMMNEW$", macroData->newSTMM);
	}
	if(text.find(L"$STMNEW$") != string::npos ){
		Replace(text, L"$STMNEW$", macroData->newSTM);
	}
	if(text.find(L"$STSSNEW$") != string::npos ){
		Replace(text, L"$STSSNEW$", macroData->newSTSS);
	}
	if(text.find(L"$STSNEW$") != string::npos ){
		Replace(text, L"$STSNEW$", macroData->newSTS);
	}
	if(text.find(L"$EDYYYYNEW$") != string::npos ){
		Replace(text, L"$EDYYYYNEW$", macroData->newEDYYYY);
	}
	if(text.find(L"$EDYYNEW$") != string::npos ){
		Replace(text, L"$EDYYNEW$", macroData->newEDYY);
	}
	if(text.find(L"$EDMMNEW$") != string::npos ){
		Replace(text, L"$EDMMNEW$", macroData->newEDMM);
	}
	if(text.find(L"$EDMNEW$") != string::npos ){
		Replace(text, L"$EDMNEW$", macroData->newEDM);
	}
	if(text.find(L"$EDDDNEW$") != string::npos ){
		Replace(text, L"$EDDDNEW$", macroData->newEDDD);
	}
	if(text.find(L"$EDDNEW$") != string::npos ){
		Replace(text, L"$EDDNEW$", macroData->newEDD);
	}
	if(text.find(L"$ETHHNEW$") != string::npos ){
		Replace(text, L"$ETHHNEW$", macroData->newETHH);
	}
	if(text.find(L"$ETHNEW$") != string::npos ){
		Replace(text, L"$ETHNEW$", macroData->newETH);
	}
	if(text.find(L"$ETMMNEW$") != string::npos ){
		Replace(text, L"$ETMMNEW$", macroData->newETMM);
	}
	if(text.find(L"$ETMNEW$") != string::npos ){
		Replace(text, L"$ETMNEW$", macroData->newETM);
	}
	if(text.find(L"$ETSSNEW$") != string::npos ){
		Replace(text, L"$ETSSNEW$", macroData->newETSS);
	}
	if(text.find(L"$ETSNEW$") != string::npos ){
		Replace(text, L"$ETSNEW$", macroData->newETS);
	}
	if(text.find(L"$ONID10NEW$") != string::npos ){
		Replace(text, L"$ONID10NEW$", macroData->newONID10);
	}
	if(text.find(L"$TSID10NEW$") != string::npos ){
		Replace(text, L"$TSID10NEW$", macroData->newTSID10);
	}
	if(text.find(L"$SID10NEW$") != string::npos ){
		Replace(text, L"$SID10NEW$", macroData->newSID10);
	}
	if(text.find(L"$EID10NEW$") != string::npos ){
		Replace(text, L"$EID10NEW$", macroData->newEID10);
	}
	if(text.find(L"$ONID16NEW$") != string::npos ){
		Replace(text, L"$ONID16NEW$", macroData->newONID16);
	}
	if(text.find(L"$TSID16NEW$") != string::npos ){
		Replace(text, L"$TSID16NEW$", macroData->newTSID16);
	}
	if(text.find(L"$SID16NEW$") != string::npos ){
		Replace(text, L"$SID16NEW$", macroData->newSID16);
	}
	if(text.find(L"$EID16NEW$") != string::npos ){
		Replace(text, L"$EID16NEW$", macroData->newEID16);
	}
	if(text.find(L"$ServiceNameNEW$") != string::npos ){
		Replace(text, L"$ServiceNameNEW$", macroData->newServiceName);
	}
	if(text.find(L"$SDYYYYNEW28$") != string::npos ){
		Replace(text, L"$SDYYYYNEW28$", macroData->newSDYYYY28);
	}
	if(text.find(L"$SDYYNEW28$") != string::npos ){
		Replace(text, L"$SDYYNEW28$", macroData->newSDYY28);
	}
	if(text.find(L"$SDMMNEW28$") != string::npos ){
		Replace(text, L"$SDMMNEW28$", macroData->newSDMM28);
	}
	if(text.find(L"$SDMNEW28$") != string::npos ){
		Replace(text, L"$SDMNEW28$", macroData->newSDM28);
	}
	if(text.find(L"$SDDDNEW28$") != string::npos ){
		Replace(text, L"$SDDDNEW28$", macroData->newSDDD28);
	}
	if(text.find(L"$SDDNEW28$") != string::npos ){
		Replace(text, L"$SDDNEW28$", macroData->newSDD28);
	}
	if(text.find(L"$STHHNEW28$") != string::npos ){
		Replace(text, L"$STHHNEW28$", macroData->newSTHH28);
	}
	if(text.find(L"$STHNEW28$") != string::npos ){
		Replace(text, L"$STHNEW28$", macroData->newSTH28);
	}
	if(text.find(L"$EDYYYYNEW28$") != string::npos ){
		Replace(text, L"$EDYYYYNEW28$", macroData->newEDYYYY28);
	}
	if(text.find(L"$EDYYNEW28$") != string::npos ){
		Replace(text, L"$EDYYNEW28$", macroData->newEDYY28);
	}
	if(text.find(L"$EDMMNEW28$") != string::npos ){
		Replace(text, L"$EDMMNEW28$", macroData->newEDMM28);
	}
	if(text.find(L"$EDMNEW28$") != string::npos ){
		Replace(text, L"$EDMNEW28$", macroData->newEDM28);
	}
	if(text.find(L"$EDDDNEW28$") != string::npos ){
		Replace(text, L"$EDDDNEW28$", macroData->newEDDD28);
	}
	if(text.find(L"$EDDNEW28$") != string::npos ){
		Replace(text, L"$EDDNEW28$", macroData->newEDD28);
	}
	if(text.find(L"$ETHHNEW28$") != string::npos ){
		Replace(text, L"$ETHHNEW28$", macroData->newETHH28);
	}
	if(text.find(L"$ETHNEW28$") != string::npos ){
		Replace(text, L"$ETHNEW28$", macroData->newETH28);
	}
	if(text.find(L"$DUHHNEW$") != string::npos ){
		Replace(text, L"$DUHHNEW$", macroData->newDUHH);
	}
	if(text.find(L"$DUHNEW$") != string::npos ){
		Replace(text, L"$DUHNEW$", macroData->newDUH);
	}
	if(text.find(L"$DUMMNEW$") != string::npos ){
		Replace(text, L"$DUMMNEW$", macroData->newDUMM);
	}
	if(text.find(L"$DUMNEW$") != string::npos ){
		Replace(text, L"$DUMNEW$", macroData->newDUM);
	}
	if(text.find(L"$DUSSNEW$") != string::npos ){
		Replace(text, L"$DUSSNEW$", macroData->newDUSS);
	}
	if(text.find(L"$DUSNEW$") != string::npos ){
		Replace(text, L"$DUSNEW$", macroData->newDUS);
	}
	if(text.find(L"$SYMDHMNEW$") != string::npos ){
		Replace(text, L"$SYMDHMNEW$", macroData->newSYMDHM);
	}
	if(text.find(L"$SYMDHM28NEW$") != string::npos ){
		Replace(text, L"$SYMDHM28NEW$", macroData->newSYMDHM28);
	}
	if(text.find(L"$EYMDHMNEW$") != string::npos ){
		Replace(text, L"$EYMDHMNEW$", macroData->newEYMDHM);
	}
	if(text.find(L"$EYMDHM28NEW$") != string::npos ){
		Replace(text, L"$EYMDHM28NEW$", macroData->newEYMDHM28);
	}
	if(text.find(L"$SEYMDHMNEW$") != string::npos ){
		Replace(text, L"$SEYMDHMNEW$", macroData->newSEYMDHM);
	}
	if(text.find(L"$SEYMDHM28NEW$") != string::npos ){
		Replace(text, L"$SEYMDHM28NEW$", macroData->newSEYMDHM28);
	}

	if(text.find(L"$Drops$") != string::npos ){
		Replace(text, L"$Drops$", macroData->drops);
	}
	if(text.find(L"$Scrambles$") != string::npos ){
		Replace(text, L"$Scrambles$", macroData->scrambles);
	}
	if(text.find(L"$Result$") != string::npos ){
		Replace(text, L"$Result$", macroData->result);
	}


}
