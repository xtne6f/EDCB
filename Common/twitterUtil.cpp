#include "StdAfx.h"
#include "twitterUtil.h"

#include "Util.h"
#include "ErrDef.h"

CTwitterUtil::CTwitterUtil(void)
{
	module = NULL;

	this->id = 0;
}

CTwitterUtil::~CTwitterUtil(void)
{
	UnInitialize();
}

//DLLの初期化
//戻り値：
// エラーコード
DWORD CTwitterUtil::Initialize()
{
	if( module != NULL ){
		return FALSE;
	}

	pfnInitializeTW = NULL;
	pfnUnInitializeTW = NULL;
	pfnGetIEProxyConfigTW = NULL;
	pfnGetProxyAutoDetectTW = NULL;
	pfnGetProxyAutoScriptTW = NULL;
	pfnSetProxyTW = NULL;
	pfnGetAuthorizationUrlTW = NULL;
	pfnSetAuthorizationPWDTW = NULL;
	pfnSendTweetTW = NULL;
	pfnGetTweetQueTW = NULL;


	BOOL ret = TRUE;

	module = ::LoadLibrary(L"twitter.dll");

	if( module == NULL ){
		return FALSE;
	}

	pfnInitializeTW = ( InitializeTW ) ::GetProcAddress( module , "InitializeTW");
	if( !pfnInitializeTW ){
		OutputDebugString(L"InitializeTWの GetProcAddress に失敗\r\n");
		ret = FALSE;
		goto ERR_END;
	}
	pfnUnInitializeTW = ( UnInitializeTW ) ::GetProcAddress( module , "UnInitializeTW");
	if( !pfnUnInitializeTW ){
		OutputDebugString(L"UnInitializeTWの GetProcAddress に失敗\r\n");
		ret = FALSE;
		goto ERR_END;
	}
	pfnGetIEProxyConfigTW = ( GetIEProxyConfigTW ) ::GetProcAddress( module , "GetIEProxyConfigTW");
	if( !pfnGetIEProxyConfigTW ){
		OutputDebugString(L"GetIEProxyConfigTWの GetProcAddress に失敗\r\n");
		ret = FALSE;
		goto ERR_END;
	}
	pfnGetProxyAutoDetectTW = ( GetProxyAutoDetectTW ) ::GetProcAddress( module , "GetProxyAutoDetectTW");
	if( !pfnGetProxyAutoDetectTW ){
		OutputDebugString(L"GetProxyAutoDetectTWの GetProcAddress に失敗\r\n");
		ret = FALSE;
		goto ERR_END;
	}
	pfnGetProxyAutoScriptTW = ( GetProxyAutoScriptTW ) ::GetProcAddress( module , "GetProxyAutoScriptTW");
	if( !pfnGetProxyAutoScriptTW ){
		OutputDebugString(L"GetProxyAutoScriptTWの GetProcAddress に失敗\r\n");
		ret = FALSE;
		goto ERR_END;
	}
	pfnSetProxyTW = ( SetProxyTW ) ::GetProcAddress( module , "SetProxyTW");
	if( !pfnSetProxyTW ){
		OutputDebugString(L"SetProxyTWの GetProcAddress に失敗\r\n");
		ret = FALSE;
		goto ERR_END;
	}
	pfnGetAuthorizationUrlTW = ( GetAuthorizationUrlTW ) ::GetProcAddress( module , "GetAuthorizationUrlTW");
	if( !pfnGetAuthorizationUrlTW ){
		OutputDebugString(L"GetAuthorizationUrlTWの GetProcAddress に失敗\r\n");
		ret = FALSE;
		goto ERR_END;
	}
	pfnSetAuthorizationPWDTW = ( SetAuthorizationPWDTW ) ::GetProcAddress( module , "SetAuthorizationPWDTW");
	if( !pfnSetAuthorizationPWDTW ){
		OutputDebugString(L"SetAuthorizationPWDTWの GetProcAddress に失敗\r\n");
		ret = FALSE;
		goto ERR_END;
	}

	pfnSendTweetTW = ( SendTweetTW ) ::GetProcAddress( module , "SendTweetTW");
	if( !pfnSendTweetTW ){
		OutputDebugString(L"SendTweetTWの GetProcAddress に失敗\r\n");
		ret = FALSE;
		goto ERR_END;
	}

	pfnGetTweetQueTW = ( GetTweetQueTW ) ::GetProcAddress( module , "GetTweetQueTW");
	if( !pfnGetTweetQueTW ){
		OutputDebugString(L"GetTweetQueTWの GetProcAddress に失敗\r\n");
		ret = FALSE;
		goto ERR_END;
	}

	if( pfnInitializeTW(&this->id) == FALSE ){
		this->id = 0;
		ret = FALSE;
		goto ERR_END;
	}

ERR_END:
	if( ret == FALSE ){
		::FreeLibrary( module );
		module=NULL;
	}
	return ret;
}

//DLLの開放
//戻り値：
// エラーコード
DWORD CTwitterUtil::UnInitialize()
{
	if( module != NULL ){
		pfnUnInitializeTW(this->id);
		this->id = 0;
		::FreeLibrary( module );
	}
	module = NULL;
	
	pfnInitializeTW = NULL;
	pfnUnInitializeTW = NULL;
	pfnGetIEProxyConfigTW = NULL;
	pfnGetProxyAutoDetectTW = NULL;
	pfnGetProxyAutoScriptTW = NULL;
	pfnSetProxyTW = NULL;
	pfnGetAuthorizationUrlTW = NULL;
	pfnSetAuthorizationPWDTW = NULL;
	pfnSendTweetTW = NULL;
	pfnGetTweetQueTW = NULL;

	return NO_ERR;
}

//IEのProxy設定を取得する
//戻り値：
// TRUE（関数成功）、FALSE（関数失敗）
//引数：
// proxyConfig		[OUT]IEのProxy設定の情報（次回API呼出時までメモリ確保）
BOOL CTwitterUtil::GetIEProxyConfig(
	CURRENT_USER_IE_PROXY_CONFIG** proxyConfig
	)
{
	if( module == NULL ){
		return ERR_NOT_INIT;
	}

	return pfnGetIEProxyConfigTW(this->id, proxyConfig);
}

//自動的に検出でProxyのアドレスを取得する
//戻り値：
// TRUE（関数成功）、FALSE（関数失敗）
//引数：
// proxyConfig		[OUT]Proxyの情報（次回API呼出時までメモリ確保）
BOOL CTwitterUtil::GetProxyAutoDetect(
	PROXY_CONFIG** proxyConfig
	)
{
	if( module == NULL ){
		return ERR_NOT_INIT;
	}

	return pfnGetProxyAutoDetectTW(this->id, proxyConfig);
}

//自動構成スクリプトでProxyのアドレスを取得する
//戻り値：
// TRUE（関数成功）、FALSE（関数失敗）
//引数：
// scriptURL		[IN]自動構成スクリプトのURL
// proxyConfig		[OUT]Proxyの情報（次回API呼出時までメモリ確保）
BOOL CTwitterUtil::GetProxyAutoScript(
	LPCWSTR scriptURL,
	PROXY_CONFIG** proxyConfig
	)
{
	if( module == NULL ){
		return ERR_NOT_INIT;
	}

	return pfnGetProxyAutoScriptTW(this->id, scriptURL, proxyConfig);
}

//Proxy使用を設定
//戻り値：
// エラーコード
//引数：
// useProxy			[IN]Proxy使うかどうか（TRUE:Proxy使う）
// proxyInfo		[IN]Proxy使う場合の設定情報
void CTwitterUtil::SetProxy(
	BOOL useProxy,
	USE_PROXY_INFO* proxyInfo
	)
{
	if( module == NULL ){
		return ;
	}

	return pfnSetProxyTW(this->id, useProxy, proxyInfo);
}

//認証用ログインURLを取得する
//戻り値：
// エラーコード
//引数：
// url			[OUT]認証用ログインURL（次回API呼出時までメモリ確保）
DWORD CTwitterUtil::GetAuthorizationUrl(
	WCHAR** url
	)
{
	if( module == NULL ){
		return ERR_NOT_INIT;
	}

	return pfnGetAuthorizationUrlTW(this->id, url);
}

//認証結果の暗証番号を設定する
//内部でGetAuthorizationUrlで取得した値を使用するので、一連の流れで設定する必要あり
//戻り値：
// エラーコード
//引数：
// password		[IN]暗証番号
DWORD CTwitterUtil::SetAuthorizationPWD(
	LPCWSTR password
	)
{
	if( module == NULL ){
		return ERR_NOT_INIT;
	}

	return pfnSetAuthorizationPWDTW(this->id, password);
}

//ツイートする
//戻り値：
// エラーコード
//引数：
// asyncMode	[IN]非同期で送信
// text			[IN]ツイート内容
DWORD CTwitterUtil::SendTweet(
	BOOL asyncMode,
	LPCWSTR text
	)
{
	if( module == NULL ){
		return ERR_NOT_INIT;
	}

	return pfnSendTweetTW(this->id, asyncMode, text);
}

//非同期ツイートの残りを取得する
//戻り値：
// 個数
DWORD CTwitterUtil::GetTweetQue(
	)
{
	if( module == NULL ){
		return 0;
	}

	return pfnGetTweetQueTW(this->id);
}
