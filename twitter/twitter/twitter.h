#ifndef __TWITTER_H__
#define __TWITTER_H__

#include "../../Common/NWStructDef.h"


//SetAVTransportURI
//戻り値：
// エラーコード(DMR_CALLBACK_ERR_INVALID_ARGS, DMR_CALLBACK_ERR_MIMETYPE, DMR_CALLBACK_ERR_BUSY, DMR_CALLBACK_ERR_NOT_FOUND, DMR_CALLBACK_ERR_INVALID_ID)
//引数：
// param			[IN] DLNAM_INIT_PARAM_DMRのSetAVTransportURIFuncParam
// streamingID		[IN] 識別ID
// json				[IN] json形式の受信データ
typedef int (CALLBACK *TW_CALLBACK_Streaming)(void* param, DWORD streamingID, const char* data, DWORD dataSize);


//DLLの初期化
//戻り値：
// エラーコード
//引数：
// id				[OUT]識別ID
__declspec(dllexport)
DWORD WINAPI InitializeTW(
	DWORD* id
	);

//DLLの開放
//戻り値：
// エラーコード
//引数：
// id		[IN]識別ID
__declspec(dllexport)
DWORD WINAPI UnInitializeTW(
	DWORD id
	);

//IEのProxy設定を取得する
//戻り値：
// TRUE（関数成功）、FALSE（関数失敗）
//引数：
// proxyConfig		[OUT]IEのProxy設定の情報（次回API呼出時までメモリ確保）
__declspec(dllexport)
BOOL WINAPI GetIEProxyConfigTW(
	DWORD id,
	CURRENT_USER_IE_PROXY_CONFIG** proxyConfig
	);

//自動的に検出でProxyのアドレスを取得する
//戻り値：
// TRUE（関数成功）、FALSE（関数失敗）
//引数：
// proxyConfig		[OUT]Proxyの情報（次回API呼出時までメモリ確保）
__declspec(dllexport)
BOOL WINAPI GetProxyAutoDetectTW(
	DWORD id,
	PROXY_CONFIG** proxyConfig
	);

//自動構成スクリプトでProxyのアドレスを取得する
//戻り値：
// TRUE（関数成功）、FALSE（関数失敗）
//引数：
// scriptURL		[IN]自動構成スクリプトのURL
// proxyConfig		[OUT]Proxyの情報（次回API呼出時までメモリ確保）
__declspec(dllexport)
BOOL WINAPI GetProxyAutoScriptTW(
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
__declspec(dllexport)
void WINAPI SetProxyTW(
	DWORD id,
	BOOL useProxy,
	USE_PROXY_INFO* proxyInfo
	);

//認証用ログインURLを取得する
//戻り値：
// エラーコード
//引数：
// url			[OUT]認証用ログインURL（次回API呼出時までメモリ確保）
__declspec(dllexport)
DWORD WINAPI GetAuthorizationUrlTW(
	DWORD id,
	WCHAR** url
	);

//認証結果の暗証番号を設定する
//内部でGetAuthorizationUrlで取得した値を使用するので、一連の流れで設定する必要あり
//戻り値：
// エラーコード
//引数：
// password		[IN]暗証番号
__declspec(dllexport)
DWORD WINAPI SetAuthorizationPWDTW(
	DWORD id,
	LPCWSTR password
	);

//ツイートする
//戻り値：
// エラーコード
//引数：
// asyncMode	[IN]非同期で送信
// text			[IN]ツイート内容
__declspec(dllexport)
DWORD WINAPI SendTweetTW(
	DWORD id,
	BOOL asyncMode,
	LPCWSTR text
	);

//非同期ツイートの残りを取得する
//戻り値：
// 個数
//引数：
__declspec(dllexport)
DWORD WINAPI GetTweetQueTW(
	DWORD id
	);

//ストリーミングを開始する
//戻り値：
// エラーコード
//引数：
// track		[IN]filterのtrack
// streamingID	[OUT]ストリーミング識別ID
__declspec(dllexport)
DWORD WINAPI StartTweetStreamingTW(
	DWORD id,
	LPCWSTR track,
	TW_CALLBACK_Streaming callbackFunc,
	void* callbackFuncParam,
	DWORD* streamingID
	);

//ストリーミングを停止する
//戻り値：
// エラーコード
//引数：
// streamingID	[IN]ストリーミング識別ID
__declspec(dllexport)
DWORD WINAPI StopTweetStreamingTW(
	DWORD id,
	DWORD streamingID
	);

#endif
