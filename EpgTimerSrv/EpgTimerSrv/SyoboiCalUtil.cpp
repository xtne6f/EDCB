#include "StdAfx.h"
#include "SyoboiCalUtil.h"

#include <wincrypt.h>
#pragma comment (lib, "Crypt32.lib")

#include "../../Common/PathUtil.h"
#include "../../Common/ProxyUtil.h"
#include "../../Common/StringUtil.h"
#include "../../Common/TimeUtil.h"
#include "../../Common/ParseTextInstances.h"

#define SYOBOI_UP_URL L"http://cal.syoboi.jp/sch_upload"

CSyoboiCalUtil::CSyoboiCalUtil(void)
{
	this->proxyInfo = NULL;
}


CSyoboiCalUtil::~CSyoboiCalUtil(void)
{
	SAFE_DELETE(this->proxyInfo);
}

__int64 CSyoboiCalUtil::GetTimeStamp(SYSTEMTIME startTime)
{
	SYSTEMTIME keyTime;
	keyTime.wYear = 1970;
	keyTime.wMonth = 1;
	keyTime.wDay = 1;
	keyTime.wHour = 0;
	keyTime.wMinute = 0;
	keyTime.wSecond = 0;
	keyTime.wMilliseconds = 0;

	FILETIME keyFTime;
	SystemTimeToFileTime( &keyTime, &keyFTime );
	FILETIME startFTime;
	SystemTimeToFileTime( &startTime, &startFTime );

	__int64 key64Time = ((__int64)keyFTime.dwHighDateTime)<<32 | keyFTime.dwLowDateTime;
	__int64 start64Time = ((__int64)startFTime.dwHighDateTime)<<32 | startFTime.dwLowDateTime;
	__int64 totalSec = (start64Time-key64Time)/(I64_1SEC);
	//UTCÇÃÇΩÇﬂÇ…-9éûä‘
	totalSec -= 9*60*60;

	return totalSec;
}

BOOL CSyoboiCalUtil::UrlEncodeUTF8(LPCWSTR src, DWORD srcSize, string& dest)
{
	if( src == NULL || srcSize == 0 ){
		return FALSE;
	}

	char* pBuff = NULL;
	int iLen = 0;

	iLen = WideCharToMultiByte(CP_UTF8, 0, src, -1, NULL, 0, NULL, NULL);
	pBuff = new char[iLen+1];
	ZeroMemory(pBuff, iLen+1);
	WideCharToMultiByte(CP_UTF8, 0, src, -1, pBuff, iLen, NULL, NULL);

	for( int i=0; i<iLen; i++ ){
		if( ( pBuff[i] >= 'A' && pBuff[i] <= 'Z' ) ||
			( pBuff[i] >= 'a' && pBuff[i] <= 'z' ) ||
			( pBuff[i] >= '0' && pBuff[i] <= '9' ) 
			)
		{
			dest+=pBuff[i];
		}else if( pBuff[i] == ' ' ){
			dest+="%20";
		}else if( pBuff[i] == '\0' ){
		}else{
			char cEnc[4]="";
			sprintf_s( cEnc, 4, "%%%02X", (BYTE)pBuff[i] );
			dest+=cEnc;
		}
	}

	delete[] pBuff;

	return TRUE;
}

BOOL CSyoboiCalUtil::UrlEncodeUTF8(LPCWSTR src, DWORD srcSize, wstring& dest)
{
	if( src == NULL || srcSize == 0 ){
		return FALSE;
	}

	char* pBuff = NULL;
	int iLen = 0;

	iLen = WideCharToMultiByte(CP_UTF8, 0, src, -1, NULL, 0, NULL, NULL);
	pBuff = new char[iLen+1];
	ZeroMemory(pBuff, iLen+1);
	WideCharToMultiByte(CP_UTF8, 0, src, -1, pBuff, iLen, NULL, NULL);

	string destBuff;

	for( int i=0; i<iLen; i++ ){
		if( ( pBuff[i] >= 'A' && pBuff[i] <= 'Z' ) ||
			( pBuff[i] >= 'a' && pBuff[i] <= 'z' ) ||
			( pBuff[i] >= '0' && pBuff[i] <= '9' ) 
			)
		{
			destBuff+=pBuff[i];
		}else if( pBuff[i] == ' ' ){
			destBuff+="%20";
		}else if( pBuff[i] == '\0' ){
		}else{
			char cEnc[4]="";
			sprintf_s( cEnc, 4, "%%%02X", (BYTE)pBuff[i] );
			destBuff+=cEnc;
		}
	}

	delete[] pBuff;

	AtoW(destBuff, dest);

	return TRUE;
}

BOOL CSyoboiCalUtil::Base64Enc(LPCSTR src, DWORD srcSize, LPWSTR dest, DWORD* destSize)
{
	if( destSize == NULL ){
		return FALSE;
	}
	DWORD dwDst = 0;
	if( CryptBinaryToStringW( (BYTE*)src, srcSize, CRYPT_STRING_BASE64, NULL, &dwDst ) == FALSE){
		return FALSE;
	}
	if( *destSize < dwDst+1 ){
		*destSize = dwDst+1;
		return FALSE;
	}
	if( dest != NULL ){
		if( CryptBinaryToStringW( (BYTE*)src, srcSize, CRYPT_STRING_BASE64, dest, destSize ) ){
		}
	}
	return TRUE;
}

BOOL CSyoboiCalUtil::SendReserve(vector<RESERVE_DATA*>* reserveList, vector<TUNER_RESERVE_INFO>* tunerList)
{
	if( reserveList == NULL || tunerList == NULL ){
		return FALSE;
	}
	if( reserveList->size() == 0 ){
		return FALSE;
	}

	wstring iniAppPath = L"";
	GetModuleIniPath(iniAppPath);
	if( GetPrivateProfileInt(L"SYOBOI", L"use", 0, iniAppPath.c_str()) == 0 ){
		return FALSE;
	}
	_OutputDebugString(L"ÅöSyoboiCalUtil:SendReserve");

	wstring textPath;
	GetModuleFolderPath(textPath);
	textPath += L"\\SyoboiCh.txt";
	CParseServiceChgText srvChg;
	srvChg.ParseText(textPath.c_str());

	SAFE_DELETE(this->proxyInfo);
	WCHAR buff[512] = L"";
	int length = 0;
	BOOL useProxy = (BOOL)GetPrivateProfileInt(L"SYOBOI", L"useProxy", 0, iniAppPath.c_str());
	if( useProxy == TRUE ){
		this->proxyInfo = new USE_PROXY_INFO;

		GetPrivateProfileString(L"SYOBOI", L"ProxyServer", L"", buff, 512, iniAppPath.c_str());
		this->proxyInfo->serverName = new WCHAR[wcslen(buff)+1];
		wcscpy_s(this->proxyInfo->serverName, wcslen(buff)+1, buff);

		ZeroMemory(buff, (sizeof(WCHAR)*512));
		GetPrivateProfileString(L"SYOBOI", L"ProxyID", L"", buff, 512, iniAppPath.c_str());
		length = (int)wcslen(buff);
		if( length > 0 ){
			this->proxyInfo->userName = new WCHAR[length+1];
			wcscpy_s(this->proxyInfo->userName, length+1, buff);
		}else{
			this->proxyInfo->userName = NULL;
		}

		ZeroMemory(buff, (sizeof(WCHAR)*512));
		GetPrivateProfileString(L"SYOBOI", L"ProxyPWD", L"", buff, 512, iniAppPath.c_str());
		length = (int)wcslen(buff);
		if( length > 0 ){
			this->proxyInfo->password = new WCHAR[length+1];
			wcscpy_s(this->proxyInfo->password, length+1, buff);
		}else{
			this->proxyInfo->password = NULL;
		}

	}

	ZeroMemory(buff, (sizeof(WCHAR)*512));
	GetPrivateProfileString(L"SYOBOI", L"userID", L"", buff, 512, iniAppPath.c_str());
	this->id=buff;

	ZeroMemory(buff, (sizeof(WCHAR)*512));
	GetPrivateProfileString(L"SYOBOI", L"PWD", L"", buff, 512, iniAppPath.c_str());
	this->pass=buff;

	int slot = GetPrivateProfileInt(L"SYOBOI", L"slot", 0, iniAppPath.c_str());

	ZeroMemory(buff, (sizeof(WCHAR)*512));
	GetPrivateProfileString(L"SYOBOI", L"devcolors", L"", buff, 512, iniAppPath.c_str());
	wstring devcolors=buff;
	
	ZeroMemory(buff, (sizeof(WCHAR)*512));
	GetPrivateProfileString(L"SYOBOI", L"epgurl", L"", buff, 512, iniAppPath.c_str());
	wstring epgurl=buff;

	if( this->id.size() == 0 ){
		_OutputDebugString(L"ÅöSyoboiCalUtil:NoUserID");
		return FALSE;
	}

	//Authorization
	wstring auth = L"";
	auth = this->id;
	auth += L":";
	auth += this->pass;
	string authA;
	WtoA(auth, authA);

	DWORD destSize = 0;
	Base64Enc(authA.c_str(), (DWORD)authA.size(), NULL, &destSize);
	WCHAR* base64 = new WCHAR[destSize];
	ZeroMemory(base64, destSize*sizeof(WCHAR));
	Base64Enc(authA.c_str(), (DWORD)authA.size(), base64, &destSize);

	wstring authHead = L"";
	Format(authHead, L"Authorization: Basic %s\r\nContent-type: application/x-www-form-urlencoded\r\n", base64);

	//data
	wstring dataParam;
	wstring param;
	map<DWORD, wstring> tunerMap;
	for( size_t i=0; i<tunerList->size(); i++ ){
		for( size_t j=0; j<(*tunerList)[i].reserveList.size(); j++ ){
			tunerMap.insert(pair<DWORD, wstring>((*tunerList)[i].reserveList[j], (*tunerList)[i].tunerName));
		}
	}
	map<DWORD, wstring>::iterator itrTuner;
	DWORD dataCount = 0;
	for(size_t i=0; i<reserveList->size(); i++ ){
		if( dataCount>=200 ){
			break;
		}
		RESERVE_DATA* info = (*reserveList)[i];
		if( info->recSetting.recMode == RECMODE_NO || info->recSetting.recMode == RECMODE_VIEW ){
			continue;
		}
		wstring device=L"";
		itrTuner = tunerMap.find(info->reserveID);
		if( itrTuner != tunerMap.end() ){
			device = itrTuner->second;
		}

		wstring stationName = info->stationName;
		srvChg.ChgText(stationName);

		__int64 startTime = GetTimeStamp(info->startTime);
		Format(param, L"%I64d\t%I64d\t%s\t%s\t%s\t\t0\t%d\n", startTime, startTime+info->durationSecond, device.c_str(), info->title.c_str(), stationName.c_str(), info->reserveID );
		dataParam+=param;
	}

	if(dataParam.size() == 0 ){
		_OutputDebugString(L"ÅöSyoboiCalUtil:NoReserve");
		return FALSE;
	}

	string utf8;
	UrlEncodeUTF8(dataParam.c_str(), (DWORD)dataParam.size(), utf8);
	string data;
	Format(data, "slot=%d&data=%s",slot, utf8.c_str());

	if( devcolors.size() > 0){
		utf8 = "";
		UrlEncodeUTF8(devcolors.c_str(), (DWORD)devcolors.size(), utf8);
		data += "&devcolors=";
		data += utf8;
	}
	if( epgurl.size() > 0){
		utf8 = "";
		UrlEncodeUTF8(epgurl.c_str(), (DWORD)epgurl.size(), utf8);
		data += "&epgurl=";
		data += utf8;
	}

	UPLOAD_DATA_LIST upList;
	upList.listCount = 1;
	upList.list = new UPLOAD_DATA[1];
	upList.list->filePathFlag = 0;
	upList.list->buffSize = (DWORD)data.size();
	upList.list->buff = new BYTE[data.size()+1];
	ZeroMemory(upList.list->buff, data.size()+1);
	memcpy(upList.list->buff, data.c_str(), data.size());

	wstring url;
	Format(url,L"%s",SYOBOI_UP_URL);

	CWinHTTPUtil http;
	DWORD result = NO_ERR;
	result = http.OpenSession(L"EpgTimerSrv", FALSE, useProxy, this->proxyInfo);
	if( result != NO_ERR ){
		return FALSE;
	}
	result = http.SendRequest(url.c_str(), NW_VERB_POST, authHead.c_str(), NULL, &upList);


	DWORD dlSize = 0;
	http.GetDLBuff(NULL, &dlSize);
	if( dlSize > 0 ){
		BYTE* dlBuff = new BYTE[dlSize+1];
		ZeroMemory(dlBuff, dlSize+1);
		http.GetDLBuff(dlBuff, &dlSize);

		string response = (char*)dlBuff;
		SAFE_DELETE_ARRAY(dlBuff);
	}

	http.CloseRequest();

	_OutputDebugString(L"ÅöSyoboiCalUtil:SendRequest res:%d", result);

	if( result != NO_ERR ){
		return FALSE;
	}
	return TRUE;
}
