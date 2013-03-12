#include "StdAfx.h"
#include "RequestUtil.h"


CRequestUtil::CRequestUtil(void)
{
}

CRequestUtil::~CRequestUtil(void)
{
}

BOOL CRequestUtil::CreateRequestToken(
	wstring consumerKey,
	wstring secretKey,
	wstring& url,
	wstring& httpHeader
	)
{
	wstring timeStamp = L"";
	GetTimeStamp(timeStamp);

	wstring createUrl = OAUTH_API_URL;
	createUrl += L"oauth/request_token";

	map<wstring, wstring> paramSort;
	paramSort.insert(pair<wstring, wstring>(L"oauth_consumer_key", consumerKey));
	paramSort.insert(pair<wstring, wstring>(L"oauth_nonce", timeStamp));
	paramSort.insert(pair<wstring, wstring>(L"oauth_signature_method", L"HMAC-SHA1"));
	paramSort.insert(pair<wstring, wstring>(L"oauth_timestamp", timeStamp));
	paramSort.insert(pair<wstring, wstring>(L"oauth_version", L"1.0"));
	paramSort.insert(pair<wstring, wstring>(L"oauth_callback", L"oob"));

	wstring param = L"";
	map<wstring, wstring>::iterator itr;
	for( itr = paramSort.begin(); itr != paramSort.end(); itr++ ){
		if( param.size() != 0 ){
			param += L"&";
		}
		param += itr->first;
		param += L"=";
		param += itr->second;
	}

	wstring urlEnc;
	UrlEncodeUTF8(createUrl.c_str(), (DWORD)createUrl.size(), urlEnc);
	wstring paramEnc;
	UrlEncodeUTF8(param.c_str(), (DWORD)param.size(), paramEnc);

	wstring sigSrc = L"";
	Format(sigSrc, L"POST&%s&%s", urlEnc.c_str(), paramEnc.c_str() );

	wstring signature = L"";
	if( CreateSignatureHMACSHA1(sigSrc, secretKey, signature ) == FALSE ){
		return FALSE;
	}

	paramSort.insert(pair<wstring, wstring>(L"oauth_signature", signature));

	param = L"";
	for( itr = paramSort.begin(); itr != paramSort.end(); itr++ ){
		if( param.size() != 0 ){
			param += L"&";
		}

		wstring valEnc;
		UrlEncodeUTF8(itr->second.c_str(), (DWORD)itr->second.size(), valEnc);

		param += itr->first;
		param += L"=";
		param += valEnc;
	}

	//URL
	url = createUrl.c_str();

	//HTTPƒwƒbƒ_
	httpHeader = L"Authorization: OAuth ";
	wstring headBuff = L"";
	Format(headBuff, L"oauth_consumer_key=\"%s\", ", consumerKey.c_str());
	httpHeader += headBuff.c_str();
	httpHeader += L"oauth_signature_method=\"HMAC-SHA1\", ";
	wstring valEnc;
	UrlEncodeUTF8(signature.c_str(), (DWORD)signature.size(), valEnc);
	Format(headBuff, L"oauth_signature=\"%s\", ", valEnc.c_str());
	httpHeader += headBuff.c_str();
	httpHeader += L"oauth_callback=\"oob\", ";
	Format(headBuff, L"oauth_timestamp=\"%s\", ", timeStamp.c_str());
	httpHeader += headBuff.c_str();
	Format(headBuff, L"oauth_nonce=\"%s\", ", timeStamp.c_str());
	httpHeader += headBuff.c_str();
	httpHeader += L"oauth_version=\"1.0\"";

	return TRUE;
}

BOOL CRequestUtil::CreateAuthorizationUrl(
	wstring oauthToken,
	wstring& url
	)
{
	wstring successUrlEnc;
	wstring tokenEnc;
	UrlEncodeUTF8(oauthToken.c_str(), (DWORD)oauthToken.size(), tokenEnc);

	Format(url, L"%soauth/authorize?oauth_token=%s",
		OAUTH_API_URL,
		oauthToken.c_str() );

	return TRUE;
}

BOOL CRequestUtil::CreateAccessToken(
	wstring consumerKey,
	wstring secretKey,
	wstring oauthToken,
	wstring oauthVerifier,
	wstring& url,
	wstring& httpHeader
	)
{
	wstring timeStamp = L"";
	GetTimeStamp(timeStamp);

	wstring createUrl = OAUTH_API_URL;
	createUrl += L"oauth/access_token";

	map<wstring, wstring> paramSort;
	paramSort.insert(pair<wstring, wstring>(L"oauth_consumer_key", consumerKey));
	paramSort.insert(pair<wstring, wstring>(L"oauth_token", oauthToken));
	paramSort.insert(pair<wstring, wstring>(L"oauth_verifier", oauthVerifier));
	paramSort.insert(pair<wstring, wstring>(L"oauth_nonce", timeStamp));
	paramSort.insert(pair<wstring, wstring>(L"oauth_signature_method", L"HMAC-SHA1"));
	paramSort.insert(pair<wstring, wstring>(L"oauth_timestamp", timeStamp));
	paramSort.insert(pair<wstring, wstring>(L"oauth_version", L"1.0"));

	wstring param = L"";
	map<wstring, wstring>::iterator itr;
	for( itr = paramSort.begin(); itr != paramSort.end(); itr++ ){
		if( param.size() != 0 ){
			param += L"&";
		}
		param += itr->first;
		param += L"=";
		param += itr->second;
	}

	wstring urlEnc;
	UrlEncodeUTF8(createUrl.c_str(), (DWORD)createUrl.size(), urlEnc);
	wstring paramEnc;
	UrlEncodeUTF8(param.c_str(), (DWORD)param.size(), paramEnc);

	wstring sigSrc = L"";
	Format(sigSrc, L"POST&%s&%s", urlEnc.c_str(), paramEnc.c_str() );

	//OutputDebugString(sigSrc.c_str());
	wstring signature = L"";
	if( CreateSignatureHMACSHA1(sigSrc, secretKey, signature ) == FALSE ){
		return FALSE;
	}

	//OutputDebugString(L"\r\n");
	//OutputDebugString(signature.c_str());
	paramSort.insert(pair<wstring, wstring>(L"oauth_signature", signature));

	param = L"";
	for( itr = paramSort.begin(); itr != paramSort.end(); itr++ ){
		if( param.size() != 0 ){
			param += L"&";
		}
		if( CompareNoCase(itr->first, L"oauth_token" ) == 0 ){
			//token‚ÍurlencodeÏ‚Ý
			param += itr->first;
			param += L"=";
			param += itr->second;
		}else{
			wstring valEnc;
			UrlEncodeUTF8(itr->second.c_str(), (DWORD)itr->second.size(), valEnc);

			param += itr->first;
			param += L"=";
			param += valEnc;
		}

	}

	//URL
	url = createUrl.c_str();

	//HTTPƒwƒbƒ_
	httpHeader = L"Authorization: OAuth ";
	wstring headBuff = L"";
	Format(headBuff, L"oauth_consumer_key=\"%s\", ", consumerKey.c_str());
	httpHeader += headBuff.c_str();
	httpHeader += L"oauth_signature_method=\"HMAC-SHA1\", ";
	wstring valEnc;
	UrlEncodeUTF8(signature.c_str(), (DWORD)signature.size(), valEnc);
	Format(headBuff, L"oauth_signature=\"%s\", ", valEnc.c_str());
	httpHeader += headBuff.c_str();
	Format(headBuff, L"oauth_timestamp=\"%s\", ", timeStamp.c_str());
	httpHeader += headBuff.c_str();
	Format(headBuff, L"oauth_token=\"%s\", ", oauthToken.c_str());
	httpHeader += headBuff.c_str();
	Format(headBuff, L"oauth_verifier=\"%s\", ", oauthVerifier.c_str());
	httpHeader += headBuff.c_str();
	Format(headBuff, L"oauth_nonce=\"%s\", ", timeStamp.c_str());
	httpHeader += headBuff.c_str();
	httpHeader += L"oauth_version=\"1.0\"";

	return TRUE;
}

BOOL CRequestUtil::CreateSendTweet(
	wstring consumerKey,
	wstring secretKey,
	wstring oauthToken,
	wstring text,
	wstring& url,
	wstring& httpHeader
	)
{
	wstring timeStamp = L"";
	GetTimeStamp(timeStamp);

	wstring createUrl = SEND_API_URL;
	createUrl += L"1.1/statuses/update.json";

	map<wstring, wstring> paramSort;
	paramSort.insert(pair<wstring, wstring>(L"oauth_consumer_key", consumerKey));
	paramSort.insert(pair<wstring, wstring>(L"oauth_token", oauthToken));
	paramSort.insert(pair<wstring, wstring>(L"oauth_nonce", timeStamp));
	paramSort.insert(pair<wstring, wstring>(L"oauth_signature_method", L"HMAC-SHA1"));
	paramSort.insert(pair<wstring, wstring>(L"oauth_timestamp", timeStamp));
	paramSort.insert(pair<wstring, wstring>(L"oauth_version", L"1.0"));
	wstring msgEnc;
	UrlEncodeUTF8(text.c_str(), (DWORD)text.size(), msgEnc);
	paramSort.insert(pair<wstring, wstring>(L"status", msgEnc));

	wstring param = L"";
	map<wstring, wstring>::iterator itr;
	for( itr = paramSort.begin(); itr != paramSort.end(); itr++ ){
		if( param.size() != 0 ){
			param += L"&";
		}
		param += itr->first;
		param += L"=";
		param += itr->second;
	}

	wstring urlEnc;
	UrlEncodeUTF8(createUrl.c_str(), (DWORD)createUrl.size(), urlEnc);
	wstring paramEnc;
	UrlEncodeUTF8(param.c_str(), (DWORD)param.size(), paramEnc);

	wstring sigSrc = L"";
	Format(sigSrc, L"POST&%s&%s", urlEnc.c_str(), paramEnc.c_str() );

	//OutputDebugString(sigSrc.c_str());
	wstring signature = L"";
	if( CreateSignatureHMACSHA1(sigSrc, secretKey, signature ) == FALSE ){
		return FALSE;
	}

	//OutputDebugString(L"\r\n");
	//OutputDebugString(signature.c_str());
	paramSort.insert(pair<wstring, wstring>(L"oauth_signature", signature));

	param = L"";
	for( itr = paramSort.begin(); itr != paramSort.end(); itr++ ){
		if( param.size() != 0 ){
			param += L"&";
		}
		if( CompareNoCase(itr->first, L"oauth_token" ) == 0 ){
			//token‚ÍurlencodeÏ‚Ý
			param += itr->first;
			param += L"=";
			param += itr->second;
		}else{
			wstring valEnc;
			UrlEncodeUTF8(itr->second.c_str(), (DWORD)itr->second.size(), valEnc);

			param += itr->first;
			param += L"=";
			param += valEnc;
		}

	}

	//URL
	Format(url, L"%s?status=%s", createUrl.c_str(), msgEnc.c_str() );

	//HTTPƒwƒbƒ_
	httpHeader = L"Authorization: OAuth ";
	wstring headBuff = L"";
	Format(headBuff, L"oauth_consumer_key=\"%s\", ", consumerKey.c_str());
	httpHeader += headBuff.c_str();
	httpHeader += L"oauth_signature_method=\"HMAC-SHA1\", ";
	wstring valEnc;
	UrlEncodeUTF8(signature.c_str(), (DWORD)signature.size(), valEnc);
	Format(headBuff, L"oauth_signature=\"%s\", ", valEnc.c_str());
	httpHeader += headBuff.c_str();
	Format(headBuff, L"oauth_timestamp=\"%s\", ", timeStamp.c_str());
	httpHeader += headBuff.c_str();
	Format(headBuff, L"oauth_token=\"%s\", ", oauthToken.c_str());
	httpHeader += headBuff.c_str();
	Format(headBuff, L"oauth_nonce=\"%s\", ", timeStamp.c_str());
	httpHeader += headBuff.c_str();
	httpHeader += L"oauth_version=\"1.0\"";

	return TRUE;
}

BOOL CRequestUtil::CreateStreaming(
wstring consumerKey,
wstring secretKey,
wstring oauthToken,
wstring text,
wstring& url,
wstring& httpHeader
)

{
	wstring timeStamp = L"";
	GetTimeStamp(timeStamp);

	wstring createUrl = SEND_STREAMING_API_URL;
	createUrl += L"1.1/statuses/filter.json";

	map<wstring, wstring> paramSort;
	paramSort.insert(pair<wstring, wstring>(L"oauth_consumer_key", consumerKey));
	paramSort.insert(pair<wstring, wstring>(L"oauth_token", oauthToken));
	paramSort.insert(pair<wstring, wstring>(L"oauth_nonce", timeStamp));
	paramSort.insert(pair<wstring, wstring>(L"oauth_signature_method", L"HMAC-SHA1"));
	paramSort.insert(pair<wstring, wstring>(L"oauth_timestamp", timeStamp));
	paramSort.insert(pair<wstring, wstring>(L"oauth_version", L"1.0"));
	wstring msgEnc;
	UrlEncodeUTF8(text.c_str(), (DWORD)text.size(), msgEnc);
	paramSort.insert(pair<wstring, wstring>(L"track", msgEnc));

	wstring param = L"";
	map<wstring, wstring>::iterator itr;
	for( itr = paramSort.begin(); itr != paramSort.end(); itr++ ){
		if( param.size() != 0 ){
			param += L"&";
		}
		param += itr->first;
		param += L"=";
		param += itr->second;
	}

	wstring urlEnc;
	UrlEncodeUTF8(createUrl.c_str(), (DWORD)createUrl.size(), urlEnc);
	wstring paramEnc;
	UrlEncodeUTF8(param.c_str(), (DWORD)param.size(), paramEnc);

	wstring sigSrc = L"";
	Format(sigSrc, L"POST&%s&%s", urlEnc.c_str(), paramEnc.c_str() );

	//OutputDebugString(sigSrc.c_str());
	wstring signature = L"";
	if( CreateSignatureHMACSHA1(sigSrc, secretKey, signature ) == FALSE ){
		return FALSE;
	}

	//OutputDebugString(L"\r\n");
	//OutputDebugString(signature.c_str());
	paramSort.insert(pair<wstring, wstring>(L"oauth_signature", signature));

	param = L"";
	for( itr = paramSort.begin(); itr != paramSort.end(); itr++ ){
		if( param.size() != 0 ){
			param += L"&";
		}
		if( CompareNoCase(itr->first, L"oauth_token" ) == 0 ){
			//token‚ÍurlencodeÏ‚Ý
			param += itr->first;
			param += L"=";
			param += itr->second;
		}else{
			wstring valEnc;
			UrlEncodeUTF8(itr->second.c_str(), (DWORD)itr->second.size(), valEnc);

			param += itr->first;
			param += L"=";
			param += valEnc;
		}

	}

	//URL
	Format(url, L"%s?track=%s", createUrl.c_str(), msgEnc.c_str() );

	//HTTPƒwƒbƒ_
	httpHeader = L"Authorization: OAuth ";
	wstring headBuff = L"";
	Format(headBuff, L"oauth_consumer_key=\"%s\", ", consumerKey.c_str());
	httpHeader += headBuff.c_str();
	httpHeader += L"oauth_signature_method=\"HMAC-SHA1\", ";
	wstring valEnc;
	UrlEncodeUTF8(signature.c_str(), (DWORD)signature.size(), valEnc);
	Format(headBuff, L"oauth_signature=\"%s\", ", valEnc.c_str());
	httpHeader += headBuff.c_str();
	Format(headBuff, L"oauth_timestamp=\"%s\", ", timeStamp.c_str());
	httpHeader += headBuff.c_str();
	Format(headBuff, L"oauth_token=\"%s\", ", oauthToken.c_str());
	httpHeader += headBuff.c_str();
	Format(headBuff, L"oauth_nonce=\"%s\", ", timeStamp.c_str());
	httpHeader += headBuff.c_str();
	httpHeader += L"oauth_version=\"1.0\"";

	return TRUE;
}
