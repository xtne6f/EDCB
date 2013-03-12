// twitter.cpp : DLL アプリケーション用にエクスポートされる関数を定義します。
//

#include "stdafx.h"
#include "twitter.h"
#include "../../Common/Util.h"
#include "TwitterMain.h"

map<DWORD, CTwitterMain*> g_List;
DWORD g_nextID = 1;

DWORD GetNextID()
{
	DWORD nextID = 0xFFFFFFFF;

	map<DWORD, CTwitterMain*>::iterator itr;
	itr = g_List.find(g_nextID);
	if( itr == g_List.end() ){
		nextID = g_nextID;
		g_nextID++;
		if( g_nextID == 0 || g_nextID == 0xFFFFFFFF){
			g_nextID = 1;
		}
	}else{
		for( DWORD i=1; i<0xFFFFFFFF; i++ ){
			itr = g_List.find(g_nextID);
			if( itr == g_List.end() ){
				nextID = g_nextID;
				g_nextID++;
				if( g_nextID == 0 || g_nextID == 0xFFFFFFFF){
					g_nextID = 1;
				}
				break;
			}else{
				g_nextID++;
			}
			if( g_nextID == 0 || g_nextID == 0xFFFFFFFF){
				g_nextID = 1;
			}
		}
	}

	return nextID;
}

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

	CTwitterMain* main = new CTwitterMain;
	DWORD err = main->Initialize();
	if( err == NO_ERR ){
		*id = GetNextID();
		g_List.insert(pair<DWORD, CTwitterMain*>(*id, main));
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
	map<DWORD, CTwitterMain*>::iterator itr;
	itr = g_List.find(id);
	if( itr == g_List.end() ){
		return ERR_NOT_INIT;
	}

	DWORD err = itr->second->UnInitialize();

	SAFE_DELETE(itr->second);

	g_List.erase(itr);

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
	map<DWORD, CTwitterMain*>::iterator itr;
	itr = g_List.find(id);
	if( itr == g_List.end() ){
		return FALSE;
	}

	return itr->second->GetIEProxyConfig(proxyConfig);
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
	map<DWORD, CTwitterMain*>::iterator itr;
	itr = g_List.find(id);
	if( itr == g_List.end() ){
		return FALSE;
	}

	return itr->second->GetProxyAutoDetect(proxyConfig);
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
	map<DWORD, CTwitterMain*>::iterator itr;
	itr = g_List.find(id);
	if( itr == g_List.end() ){
		return FALSE;
	}

	return itr->second->GetProxyAutoScript(scriptURL, proxyConfig);
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
	map<DWORD, CTwitterMain*>::iterator itr;
	itr = g_List.find(id);
	if( itr == g_List.end() ){
		return ;
	}

	itr->second->SetProxy(useProxy, proxyInfo);
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
	map<DWORD, CTwitterMain*>::iterator itr;
	itr = g_List.find(id);
	if( itr == g_List.end() ){
		return FALSE;
	}

	return itr->second->GetAuthorizationUrl(url);
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
	map<DWORD, CTwitterMain*>::iterator itr;
	itr = g_List.find(id);
	if( itr == g_List.end() ){
		return FALSE;
	}

	return itr->second->SetAuthorizationPWD(password);
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
	map<DWORD, CTwitterMain*>::iterator itr;
	itr = g_List.find(id);
	if( itr == g_List.end() ){
		return FALSE;
	}

	return itr->second->SendTweet(asyncMode, text);
}

//非同期ツイートの残りを取得する
//戻り値：
// 個数
//引数：
DWORD WINAPI GetTweetQueTW(
	DWORD id
	)
{
	map<DWORD, CTwitterMain*>::iterator itr;
	itr = g_List.find(id);
	if( itr == g_List.end() ){
		return FALSE;
	}

	return itr->second->GetTweetQue();
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
	map<DWORD, CTwitterMain*>::iterator itr;
	itr = g_List.find(id);
	if( itr == g_List.end() ){
		return FALSE;
	}

	return itr->second->StartTweetStreaming(track, callbackFunc, callbackFuncParam, streamingID);
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
	map<DWORD, CTwitterMain*>::iterator itr;
	itr = g_List.find(id);
	if( itr == g_List.end() ){
		return FALSE;
	}

	return itr->second->StopTweetStreaming(streamingID);
}
