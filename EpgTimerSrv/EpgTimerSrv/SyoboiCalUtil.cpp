#include "StdAfx.h"
#include "SyoboiCalUtil.h"

#include <winhttp.h>
#pragma comment (lib, "winhttp.lib")
#include <wincrypt.h>
#pragma comment (lib, "Crypt32.lib")

#include "../../Common/CommonDef.h"
#include "../../Common/PathUtil.h"
#include "../../Common/StringUtil.h"
#include "../../Common/TimeUtil.h"
#include "../../Common/ParseTextInstances.h"

#define SYOBOI_UP_URL L"http://cal.syoboi.jp/sch_upload"

__int64 CSyoboiCalUtil::GetTimeStamp(SYSTEMTIME startTime)
{
	SYSTEMTIME keyTime;
	keyTime.wYear = 1970;
	keyTime.wMonth = 1;
	keyTime.wDay = 1;
	//UTC+9
	keyTime.wHour = 9;
	keyTime.wMinute = 0;
	keyTime.wSecond = 0;
	keyTime.wMilliseconds = 0;

	return (ConvertI64Time(startTime) - ConvertI64Time(keyTime)) / I64_1SEC;
}

BOOL CSyoboiCalUtil::UrlEncodeUTF8(LPCWSTR src, DWORD srcSize, string& dest)
{
	if( src == NULL || srcSize == 0 ){
		return FALSE;
	}

	WtoUTF8(src, dest);

	for( size_t i=0; i<dest.size(); ){
		if( ( dest[i] >= 'A' && dest[i] <= 'Z' ) ||
			( dest[i] >= 'a' && dest[i] <= 'z' ) ||
			( dest[i] >= '0' && dest[i] <= '9' ) 
			)
		{
			i++;
		}else{
			char cEnc[4]="";
			sprintf_s( cEnc, "%%%02X", (BYTE)dest[i] );
			dest.replace(i, 1, cEnc, 3);
			i += 3;
		}
	}

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

BOOL CSyoboiCalUtil::SendReserve(const vector<RESERVE_DATA>* reserveList, const vector<TUNER_RESERVE_INFO>* tunerList)
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
	_OutputDebugString(L"★SyoboiCalUtil:SendReserve");

	wstring textPath;
	GetModuleFolderPath(textPath);
	textPath += L"\\SyoboiCh.txt";
	CParseServiceChgText srvChg;
	srvChg.ParseText(textPath.c_str());

	wstring proxyServerName;
	wstring proxyUserName;
	wstring proxyPassword;
	if( GetPrivateProfileInt(L"SYOBOI", L"useProxy", 0, iniAppPath.c_str()) != 0 ){
		proxyServerName = GetPrivateProfileToString(L"SYOBOI", L"ProxyServer", L"", iniAppPath.c_str());
		proxyUserName = GetPrivateProfileToString(L"SYOBOI", L"ProxyID", L"", iniAppPath.c_str());
		proxyPassword = GetPrivateProfileToString(L"SYOBOI", L"ProxyPWD", L"", iniAppPath.c_str());
	}

	wstring id=GetPrivateProfileToString(L"SYOBOI", L"userID", L"", iniAppPath.c_str());

	wstring pass=GetPrivateProfileToString(L"SYOBOI", L"PWD", L"", iniAppPath.c_str());

	int slot = GetPrivateProfileInt(L"SYOBOI", L"slot", 0, iniAppPath.c_str());

	wstring devcolors=GetPrivateProfileToString(L"SYOBOI", L"devcolors", L"", iniAppPath.c_str());
	
	wstring epgurl=GetPrivateProfileToString(L"SYOBOI", L"epgurl", L"", iniAppPath.c_str());

	if( id.size() == 0 ){
		_OutputDebugString(L"★SyoboiCalUtil:NoUserID");
		return FALSE;
	}

	//Authorization
	wstring auth = L"";
	auth = id;
	auth += L":";
	auth += pass;
	string authA;
	WtoA(auth, authA);

	DWORD destSize = 0;
	Base64Enc(authA.c_str(), (DWORD)authA.size(), NULL, &destSize);
	vector<WCHAR> base64(destSize + 1, L'\0');
	Base64Enc(authA.c_str(), (DWORD)authA.size(), &base64.front(), &destSize);
	//無駄なCRLFが混じることがあるため
	std::replace(base64.begin(), base64.end(), L'\r', L'\0');
	std::replace(base64.begin(), base64.end(), L'\n', L'\0');

	wstring authHead = L"";
	Format(authHead, L"Authorization: Basic %s\r\nContent-type: application/x-www-form-urlencoded\r\n", &base64.front());

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
		const RESERVE_DATA* info = &(*reserveList)[i];
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
		_OutputDebugString(L"★SyoboiCalUtil:NoReserve");
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
	vector<char> dataBuff(data.begin(), data.end());

	//URLの分解
	URL_COMPONENTS stURL = {};
	stURL.dwStructSize = sizeof(stURL);
	stURL.dwSchemeLength = (DWORD)-1;
	stURL.dwHostNameLength = (DWORD)-1;
	stURL.dwUrlPathLength = (DWORD)-1;
	stURL.dwExtraInfoLength = (DWORD)-1;
	if( WinHttpCrackUrl(SYOBOI_UP_URL, 0, 0, &stURL) == FALSE || stURL.dwHostNameLength == 0 ){
		return FALSE;
	}
	wstring host(stURL.lpszHostName, stURL.dwHostNameLength);
	wstring sendUrl(stURL.lpszUrlPath, stURL.dwUrlPathLength + stURL.dwExtraInfoLength);

	HINTERNET session;
	if( proxyServerName.empty() ){
		session = WinHttpOpen(L"EpgTimerSrv", WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	}else{
		session = WinHttpOpen(L"EpgTimerSrv", WINHTTP_ACCESS_TYPE_NAMED_PROXY, proxyServerName.c_str(), WINHTTP_NO_PROXY_BYPASS, 0);
	}
	if( session == NULL ){
		return FALSE;
	}

	LPCWSTR result = L"1";
	HINTERNET connect = NULL;
	HINTERNET request = NULL;

	if( WinHttpSetTimeouts(session, 15000, 15000, 15000, 15000) == FALSE ){
		result = L"0 SetTimeouts";
		goto EXIT;
	}
	//コネクションオープン
	connect = WinHttpConnect(session, host.c_str(), stURL.nPort, 0);
	if( connect == NULL ){
		result = L"0 Connect";
		goto EXIT;
	}
	//リクエストオープン
	request = WinHttpOpenRequest(connect, L"POST", sendUrl.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
	                             stURL.nPort == INTERNET_DEFAULT_HTTPS_PORT ? WINHTTP_FLAG_SECURE : 0);
	if( request == NULL ){
		result = L"0 OpenRequest";
		goto EXIT;
	}
	if( proxyServerName.empty() == false ){
		//ProxyのIDかパスワードがあったらセット
		if( proxyUserName.empty() == false || proxyPassword.empty() == false ){
			if( WinHttpSetCredentials(request, WINHTTP_AUTH_TARGET_PROXY, WINHTTP_AUTH_SCHEME_BASIC,
			                          proxyUserName.c_str(), proxyPassword.c_str(), NULL) == FALSE ){
				result = L"0 SetCredentials";
				goto EXIT;
			}
		}
	}
	if( WinHttpSendRequest(request, authHead.c_str(), (DWORD)-1, &dataBuff.front(), (DWORD)dataBuff.size(), (DWORD)dataBuff.size(), 0) == FALSE ){
		result = L"0 SendRequest";
		goto EXIT;
	}
	if( WinHttpReceiveResponse(request, NULL) == FALSE ){
		result = L"0 ReceiveResponse";
		goto EXIT;
	}
	//HTTPのステータスコード確認
	DWORD statusCode;
	DWORD statusCodeSize = sizeof(statusCode);
	if( WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX,
	                        &statusCode, &statusCodeSize, WINHTTP_NO_HEADER_INDEX) == FALSE ){
		statusCode = 0;
	}
	if( statusCode != 200 && statusCode != 201 ){
		result = L"0 StatusNotOK";
		goto EXIT;
	}

EXIT:
	if( request != NULL ){
		WinHttpCloseHandle(request);
	}
	if( connect != NULL ){
		WinHttpCloseHandle(connect);
	}
	if( session != NULL ){
		WinHttpCloseHandle(session);
	}

	_OutputDebugString(L"★SyoboiCalUtil:SendRequest res:%s", result);

	if( result[0] != L'1' ){
		return FALSE;
	}
	return TRUE;
}
