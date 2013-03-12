#pragma once

#include "../../Common/ErrDef.h"
#include "../../Common/NWStructDef.h"
#include "../../Common/ProxyUtil.h"
#include "WinHTTPUtil.h"
#include "twitterDef.h"
#include "twitter.h"

class CTwitterMain
{
public:
	CTwitterMain(void);
	~CTwitterMain(void);

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

	//ストリーミングを開始する
	//戻り値：
	// エラーコード
	//引数：
	// track		[IN]filterのtrack
	// streamingID	[OUT]ストリーミング識別ID
	DWORD StartTweetStreaming(
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
	DWORD StopTweetStreaming(
		DWORD streamingID
		);

protected:
	BOOL useProxy;
	USE_PROXY_INFO proxyInfo;
	BOOL chgProxy;

	CWinHTTPUtil httpUtil;
	CProxyUtil proxyUtil;
	wstring moduleIniPath;

	WCHAR* authorizationUrl;
	wstring authorization_token;
	wstring authorization_token_secret;

	wstring oauth_token;
	wstring oauth_token_secret;

	HANDLE lockEvent;
	vector<wstring> tweetQue;
	HANDLE tweetThread;
	HANDLE tweetStopEvent;
	HANDLE tweetSetEvent;

	wstring lastResponse;

	DWORD sendWaitSec;

	typedef struct _STREAMING_INFO{
		CTwitterMain* sys;
		TW_CALLBACK_Streaming callbackFunc;
		void* callbackFuncParam;
		DWORD streamingID;
		CWinHTTPUtil httpUtil;
	}STREAMING_INFO;
	map<DWORD, STREAMING_INFO*> streamingList;
	DWORD nextStreamingID;

protected:
	BOOL Lock(LPCWSTR log = NULL, DWORD timeOut = 15*1000);
	void UnLock(LPCWSTR log = NULL);
	static UINT WINAPI TweetThread(LPVOID param);

	static int CALLBACK StreamingCallback(void* param, BYTE* data, DWORD dataSize);


	DWORD SendCmd(CWinHTTPUtil* httpUtil, BOOL asyncMode, NW_VERB_MODE verbMode, wstring url, wstring httpHead, UPLOAD_DATA_LIST* upList, wstring saveFilePath, RESPONSE_READ callbackFunc=NULL, void* callbackFuncParam=NULL);
};

