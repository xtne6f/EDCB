#pragma once

#include <Windows.h>
#include "NWStructDef.h"


//DLLの初期化
//戻り値：
// エラーコード
//引数：
// id				[OUT]識別ID
typedef DWORD (WINAPI *InitializeTW)(
	DWORD* id
	);

//DLLの開放
//戻り値：
// エラーコード
//引数：
// id		[IN]識別ID
typedef DWORD (WINAPI *UnInitializeTW)(
	DWORD id
	);

//IEのProxy設定を取得する
//戻り値：
// TRUE（関数成功）、FALSE（関数失敗）
//引数：
// proxyConfig		[OUT]IEのProxy設定の情報（次回API呼出時までメモリ確保）
typedef BOOL (WINAPI *GetIEProxyConfigTW)(
	DWORD id,
	CURRENT_USER_IE_PROXY_CONFIG** proxyConfig
	);

//自動的に検出でProxyのアドレスを取得する
//戻り値：
// TRUE（関数成功）、FALSE（関数失敗）
//引数：
// proxyConfig		[OUT]Proxyの情報（次回API呼出時までメモリ確保）
typedef BOOL (WINAPI *GetProxyAutoDetectTW)(
	DWORD id,
	PROXY_CONFIG** proxyConfig
	);

//自動構成スクリプトでProxyのアドレスを取得する
//戻り値：
// TRUE（関数成功）、FALSE（関数失敗）
//引数：
// scriptURL		[IN]自動構成スクリプトのURL
// proxyConfig		[OUT]Proxyの情報（次回API呼出時までメモリ確保）
typedef BOOL (WINAPI *GetProxyAutoScriptTW)(
	DWORD id,
	LPCWSTR scriptURL,
	PROXY_CONFIG** proxyConfig
	);

//Proxy使用を設定
//戻り値：
// エラーコード
//引数：
// useProxy			[IN]Proxy使うかどうか（TRUE:Proxy使う）
// proxyInfo		[IN]Proxy使う場合の設定情報
typedef void (WINAPI *SetProxyTW)(
	DWORD id,
	BOOL useProxy,
	USE_PROXY_INFO* proxyInfo
	);

//認証用ログインURLを取得する
//戻り値：
// エラーコード
//引数：
// url			[OUT]認証用ログインURL（次回API呼出時までメモリ確保）
typedef DWORD (WINAPI *GetAuthorizationUrlTW)(
	DWORD id,
	WCHAR** url
	);

//認証結果の暗証番号を設定する
//内部でGetAuthorizationUrlで取得した値を使用するので、一連の流れで設定する必要あり
//戻り値：
// エラーコード
//引数：
// password		[IN]暗証番号
typedef DWORD (WINAPI *SetAuthorizationPWDTW)(
	DWORD id,
	LPCWSTR password
	);

//ツイートする
//戻り値：
// エラーコード
//引数：
// asyncMode	[IN]非同期で送信
// text			[IN]ツイート内容
typedef DWORD (WINAPI *SendTweetTW)(
	DWORD id,
	BOOL asyncMode,
	LPCWSTR text
	);

//非同期ツイートの残りを取得する
//戻り値：
// 個数
//引数：
typedef DWORD (WINAPI *GetTweetQueTW)(
	DWORD id
	);

class CTwitterUtil
{
public:
	CTwitterUtil(void);
	~CTwitterUtil(void);

	//DLLの初期化
	//戻り値：
	// エラーコード
	DWORD Initialize(
		);

	//DLLの開放
	//戻り値：
	// エラーコード
	DWORD UnInitialize(
		);

	//IEのProxy設定を取得する
	//戻り値：
	// TRUE（関数成功）、FALSE（関数失敗）
	//引数：
	// proxyConfig		[OUT]IEのProxy設定の情報（次回API呼出時までメモリ確保）
	BOOL GetIEProxyConfig(
		CURRENT_USER_IE_PROXY_CONFIG** proxyConfig
		);

	//自動的に検出でProxyのアドレスを取得する
	//戻り値：
	// TRUE（関数成功）、FALSE（関数失敗）
	//引数：
	// proxyConfig		[OUT]Proxyの情報（次回API呼出時までメモリ確保）
	BOOL GetProxyAutoDetect(
		PROXY_CONFIG** proxyConfig
		);

	//自動構成スクリプトでProxyのアドレスを取得する
	//戻り値：
	// TRUE（関数成功）、FALSE（関数失敗）
	//引数：
	// scriptURL		[IN]自動構成スクリプトのURL
	// proxyConfig		[OUT]Proxyの情報（次回API呼出時までメモリ確保）
	BOOL GetProxyAutoScript(
		LPCWSTR scriptURL,
		PROXY_CONFIG** proxyConfig
		);

	//Proxy使用を設定
	//戻り値：
	// エラーコード
	//引数：
	// useProxy			[IN]Proxy使うかどうか（TRUE:Proxy使う）
	// proxyInfo		[IN]Proxy使う場合の設定情報
	void SetProxy(
		BOOL useProxy,
		USE_PROXY_INFO* proxyInfo
		);

	//認証用ログインURLを取得する
	//戻り値：
	// エラーコード
	//引数：
	// url			[OUT]認証用ログインURL（次回API呼出時までメモリ確保）
	DWORD GetAuthorizationUrl(
		WCHAR** url
		);

	//認証結果の暗証番号を設定する
	//内部でGetAuthorizationUrlで取得した値を使用するので、一連の流れで設定する必要あり
	//戻り値：
	// エラーコード
	//引数：
	// password		[IN]暗証番号
	DWORD SetAuthorizationPWD(
		LPCWSTR password
		);

	//ツイートする
	//戻り値：
	// エラーコード
	//引数：
	// asyncMode	[IN]非同期で送信
	// text			[IN]ツイート内容
	DWORD SendTweet(
		BOOL asyncMode,
		LPCWSTR text
		);

	//非同期ツイートの残りを取得する
	//戻り値：
	// 個数
	DWORD GetTweetQue(
		);

protected:
	HMODULE module;
	DWORD id;

	InitializeTW				pfnInitializeTW;
	UnInitializeTW				pfnUnInitializeTW;
	GetIEProxyConfigTW			pfnGetIEProxyConfigTW;
	GetProxyAutoDetectTW		pfnGetProxyAutoDetectTW;
	GetProxyAutoScriptTW		pfnGetProxyAutoScriptTW;
	SetProxyTW					pfnSetProxyTW;
	GetAuthorizationUrlTW		pfnGetAuthorizationUrlTW;
	SetAuthorizationPWDTW		pfnSetAuthorizationPWDTW;
	SendTweetTW					pfnSendTweetTW;
	GetTweetQueTW				pfnGetTweetQueTW;

protected:

};

