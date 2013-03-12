#pragma once

#include "../../Common/NWStructDef.h"
#include "twitterDef.h"
#include "OAuthUtil.h"

class CRequestUtil
{
public:
	CRequestUtil(void);
	~CRequestUtil(void);

	BOOL CreateRequestToken(
		wstring consumerKey,
		wstring secretKey,
		wstring& url,
		wstring& httpHeader
		);

	BOOL CreateAuthorizationUrl(
		wstring oauthToken,
		wstring& url
		);

	BOOL CreateAccessToken(
		wstring consumerKey,
		wstring secretKey,
		wstring oauthToken,
		wstring oauthVerifier,
		wstring& url,
		wstring& httpHeader
		);

	BOOL CreateSendTweet(
		wstring consumerKey,
		wstring secretKey,
		wstring oauthToken,
		wstring text,
		wstring& url,
		wstring& httpHeader
		);

	BOOL CreateStreaming(
		wstring consumerKey,
		wstring secretKey,
		wstring oauthToken,
		wstring text,
		wstring& url,
		wstring& httpHeader
		);
};
