// twitter.cpp : DLL アプリケーション用にエクスポートされる関数を定義します。
//

#include "stdafx.h"
#include "twitter.h"
#include "../../Common/Util.h"
#include "TwitterMain.h"
#include "../../Common/InstanceManager.h"

CInstanceManager<CTwitterMain> g_instMng;

//DLLの初期化
//戻り値：
// エラーコード
//引数：
// id				[OUT]識別ID
DWORD WINAPI InitializeTW(
	DWORD* id
	)
{
	if( id == NULL ){
		return ERR_INVALID_ARG;
	}

	DWORD err = ERR_FALSE;

	try{
		std::shared_ptr<CTwitterMain> ptr = std::make_shared<CTwitterMain>();
		err = ptr->Initialize();
		if( err == NO_ERR ){
			*id = g_instMng.push(ptr);
		}
	}catch( std::bad_alloc& ){
		err = ERR_FALSE;
	}
	return err;
}

//DLLの開放
//戻り値：
// エラーコード
//引数：
// id		[IN]識別ID InitializeEPの戻り値
DWORD WINAPI UnInitializeTW(
	DWORD id
	)
{
	DWORD err = ERR_NOT_INIT;
	{
		std::shared_ptr<CTwitterMain> ptr = g_instMng.pop(id);
		if( ptr != NULL ){
			err = ptr->UnInitialize();
		}
	}

	return err;
}


//IEのProxy設定を取得する
//戻り値：
// TRUE（関数成功）、FALSE（関数失敗）
//引数：
// proxyConfig		[OUT]IEのProxy設定の情報（次回API呼出時までメモリ確保）
BOOL WINAPI GetIEProxyConfigTW(
	DWORD id,
	CURRENT_USER_IE_PROXY_CONFIG** proxyConfig
	)
{
	std::shared_ptr<CTwitterMain> ptr = g_instMng.find(id);
	if( ptr == NULL ){
		return FALSE;
	}

	return ptr->GetIEProxyConfig(proxyConfig);
}

//自動的に検出でProxyのアドレスを取得する
//戻り値：
// TRUE（関数成功）、FALSE（関数失敗）
//引数：
// proxyConfig		[OUT]Proxyの情報（次回API呼出時までメモリ確保）
BOOL WINAPI GetProxyAutoDetectTW(
	DWORD id,
	PROXY_CONFIG** proxyConfig
	)
{
	std::shared_ptr<CTwitterMain> ptr = g_instMng.find(id);
	if( ptr == NULL ){
		return FALSE;
	}

	return ptr->GetProxyAutoDetect(proxyConfig);
}

//自動構成スクリプトでProxyのアドレスを取得する
//戻り値：
// TRUE（関数成功）、FALSE（関数失敗）
//引数：
// scriptURL		[IN]自動構成スクリプトのURL
// proxyConfig		[OUT]Proxyの情報（次回API呼出時までメモリ確保）
BOOL WINAPI GetProxyAutoScriptTW(
	DWORD id,
	LPCWSTR scriptURL,
	PROXY_CONFIG** proxyConfig
	)
{
	std::shared_ptr<CTwitterMain> ptr = g_instMng.find(id);
	if( ptr == NULL ){
		return FALSE;
	}

	return ptr->GetProxyAutoScript(scriptURL, proxyConfig);
}

//Proxy使用を設定
//戻り値：
// エラーコード
//引数：
// useProxy			[IN]Proxy使うかどうか（TRUE:Proxy使う）
// proxyInfo		[IN]Proxy使う場合の設定情報
void WINAPI SetProxyTW(
	DWORD id,
	BOOL useProxy,
	USE_PROXY_INFO* proxyInfo
	)
{
	std::shared_ptr<CTwitterMain> ptr = g_instMng.find(id);
	if( ptr == NULL ){
		return ;
	}

	ptr->SetProxy(useProxy, proxyInfo);
}

//認証用ログインURLを取得する
//戻り値：
// エラーコード
//引数：
// url			[OUT]認証用ログインURL（次回API呼出時までメモリ確保）
DWORD WINAPI GetAuthorizationUrlTW(
	DWORD id,
	WCHAR** url
	)
{
	std::shared_ptr<CTwitterMain> ptr = g_instMng.find(id);
	if( ptr == NULL ){
		return FALSE;
	}

	return ptr->GetAuthorizationUrl(url);
}

//認証結果の暗証番号を設定する
//内部でGetAuthorizationUrlで取得した値を使用するので、一連の流れで設定する必要あり
//戻り値：
// エラーコード
//引数：
// password		[IN]暗証番号
DWORD WINAPI SetAuthorizationPWDTW(
	DWORD id,
	LPCWSTR password
	)
{
	std::shared_ptr<CTwitterMain> ptr = g_instMng.find(id);
	if( ptr == NULL ){
		return FALSE;
	}

	return ptr->SetAuthorizationPWD(password);
}

//ツイートする
//戻り値：
// エラーコード
//引数：
// asyncMode	[IN]非同期で送信
// text			[IN]ツイート内容
DWORD WINAPI SendTweetTW(
	DWORD id,
	BOOL asyncMode,
	LPCWSTR text
	)
{
	std::shared_ptr<CTwitterMain> ptr = g_instMng.find(id);
	if( ptr == NULL ){
		return FALSE;
	}

	return ptr->SendTweet(asyncMode, text);
}

//非同期ツイートの残りを取得する
//戻り値：
// 個数
//引数：
DWORD WINAPI GetTweetQueTW(
	DWORD id
	)
{
	std::shared_ptr<CTwitterMain> ptr = g_instMng.find(id);
	if( ptr == NULL ){
		return FALSE;
	}

	return ptr->GetTweetQue();
}

//ストリーミングを開始する
//戻り値：
// エラーコード
//引数：
// track		[IN]filterのtrack
// streamingID	[OUT]ストリーミング識別ID
DWORD WINAPI StartTweetStreamingTW(
	DWORD id,
	LPCWSTR track,
	TW_CALLBACK_Streaming callbackFunc,
	void* callbackFuncParam,
	DWORD* streamingID
	)
{
	std::shared_ptr<CTwitterMain> ptr = g_instMng.find(id);
	if( ptr == NULL ){
		return FALSE;
	}

	return ptr->StartTweetStreaming(track, callbackFunc, callbackFuncParam, streamingID);
}

//ストリーミングを停止する
//戻り値：
// エラーコード
//引数：
// streamingID	[IN]ストリーミング識別ID
DWORD WINAPI StopTweetStreamingTW(
	DWORD id,
	DWORD streamingID
	)
{
	std::shared_ptr<CTwitterMain> ptr = g_instMng.find(id);
	if( ptr == NULL ){
		return FALSE;
	}

	return ptr->StopTweetStreaming(streamingID);
}
