#include "StdAfx.h"
#include "TwitterMain.h"
#include "RequestUtil.h"
#include <process.h>

extern HINSTANCE g_Instance;

CTwitterMain::CTwitterMain(void)
{
	this->lockEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
	this->authorizationUrl = NULL;
	this->useProxy = FALSE;
	this->chgProxy = TRUE;

    this->tweetThread = NULL;
    this->tweetStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	this->tweetSetEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	WCHAR filePath[512] = L"";
	GetModuleFileName(g_Instance, filePath, 512);
	this->moduleIniPath = filePath;
	this->moduleIniPath += L".ini";

	this->sendWaitSec = 5;
}


CTwitterMain::~CTwitterMain(void)
{
	if( this->tweetThread != NULL ){
		::SetEvent(this->tweetStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->tweetThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->tweetThread, 0xffffffff);
		}
		CloseHandle(this->tweetThread);
		this->tweetThread = NULL;
	}
	if( this->tweetStopEvent != NULL ){
		CloseHandle(this->tweetStopEvent);
		this->tweetStopEvent = NULL;
	}
	if( this->tweetSetEvent != NULL ){
		CloseHandle(this->tweetSetEvent);
		this->tweetSetEvent = NULL;
	}

	map<DWORD, STREAMING_INFO*>::iterator itr;
	for( itr = streamingList.begin(); itr != streamingList.end(); itr++ ){
		itr->second->httpUtil.CloseSession();
		SAFE_DELETE(itr->second);
	}

	SAFE_DELETE_ARRAY(this->authorizationUrl);
	this->httpUtil.CloseSession();
	if( this->lockEvent != NULL ){
		UnLock();
		CloseHandle(this->lockEvent);
		this->lockEvent = NULL;
	}
}

BOOL CTwitterMain::Lock(LPCWSTR log, DWORD timeOut)
{
	if( this->lockEvent == NULL ){
		return FALSE;
	}
	if( log != NULL ){
		OutputDebugString(log);
	}
	DWORD dwRet = WaitForSingleObject(this->lockEvent, timeOut);
	if( dwRet == WAIT_ABANDONED || 
		dwRet == WAIT_FAILED ||
		dwRet == WAIT_TIMEOUT){
			OutputDebugString(L"◆CTwitterMain::Lock FALSE");
		return FALSE;
	}
	return TRUE;
}

void CTwitterMain::UnLock(LPCWSTR log)
{
	if( this->lockEvent != NULL ){
		SetEvent(this->lockEvent);
	}
	if( log != NULL ){
		OutputDebugString(log);
	}
}

//DLLの初期化
//戻り値：
// エラーコード
DWORD CTwitterMain::Initialize(
	)
{
	WCHAR buff[512] = L"";
	GetPrivateProfileString(L"SET", L"token", L"", buff, 512, this->moduleIniPath.c_str());
	this->oauth_token = buff;
	ZeroMemory(buff, sizeof(WCHAR)*512);
	GetPrivateProfileString(L"SET", L"token_secret", L"", buff, 512, this->moduleIniPath.c_str());
	this->oauth_token_secret = buff;

	this->sendWaitSec = GetPrivateProfileInt(L"SET", L"SendWaitSec", 1, this->moduleIniPath.c_str());
	
	return NO_ERR;
}

//DLLの開放
//戻り値：
// エラーコード
DWORD CTwitterMain::UnInitialize(
	)
{
	if( this->tweetThread != NULL ){
		::SetEvent(this->tweetStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->tweetThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->tweetThread, 0xffffffff);
		}
		CloseHandle(this->tweetThread);
		this->tweetThread = NULL;
	}
	this->httpUtil.CloseSession();

	this->tweetQue.clear();
	return NO_ERR;
}

//IEのProxy設定を取得する
//戻り値：
// TRUE（関数成功）、FALSE（関数失敗）
//引数：
// proxyConfig		[OUT]IEのProxy設定の情報（次回API呼出時までメモリ確保）
BOOL CTwitterMain::GetIEProxyConfig(
	CURRENT_USER_IE_PROXY_CONFIG** proxyConfig
	)
{
	return this->proxyUtil.GetIEProxyConfig(proxyConfig);
}

//自動的に検出でProxyのアドレスを取得する
//戻り値：
// TRUE（関数成功）、FALSE（関数失敗）
//引数：
// proxyConfig		[OUT]Proxyの情報（次回API呼出時までメモリ確保）
BOOL CTwitterMain::GetProxyAutoDetect(
	PROXY_CONFIG** proxyConfig
	)
{
	return this->proxyUtil.GetProxyAutoDetect(PROXY_CHK_URL, proxyConfig);
}

//自動構成スクリプトでProxyのアドレスを取得する
//戻り値：
// TRUE（関数成功）、FALSE（関数失敗）
//引数：
// scriptURL		[IN]自動構成スクリプトのURL
// proxyConfig		[OUT]Proxyの情報（次回API呼出時までメモリ確保）
BOOL CTwitterMain::GetProxyAutoScript(
	LPCWSTR scriptURL, //[IN]自動構成スクリプトのURL
	PROXY_CONFIG** proxyConfig //[OUT]Proxyの情報
	)
{
	return this->proxyUtil.GetProxyAutoScript(PROXY_CHK_URL, scriptURL, proxyConfig);
}

//Proxy使用を設定
//戻り値：
// エラーコード
//引数：
// useProxy			[IN]Proxy使うかどうか（TRUE:Proxy使う）
// proxyInfo		[IN]Proxy使う場合の設定情報
void CTwitterMain::SetProxy(
	BOOL useProxy,
	USE_PROXY_INFO* proxyInfo
	)
{
	this->useProxy = useProxy;
	this->proxyInfo = *proxyInfo;
	this->chgProxy = TRUE;
}

//認証用ログインURLを取得する
//戻り値：
// エラーコード
//引数：
// url			[OUT]認証用ログインURL（次回API呼出時までメモリ確保）
DWORD CTwitterMain::GetAuthorizationUrl(
	WCHAR** url
	)
{
	if( Lock() == FALSE ) return ERR_FALSE;
	SAFE_DELETE_ARRAY(this->authorizationUrl);
	DWORD ret = NO_ERR;

	CRequestUtil requestUtil;

	wstring requestUrl;
	wstring httpHeader;

	CRequestUtil request;
	wstring key = CONSUMER_SECRET_KEY;
	key += L"&";
	requestUtil.CreateRequestToken(CONSUMER_KEY, key, requestUrl, httpHeader);

	ret = SendCmd(&this->httpUtil, FALSE, NW_VERB_POST, requestUrl.c_str(), httpHeader.c_str(), NULL, L"");
	if( ret == NO_ERR ){
		//レスポンス取得して解析
		wstring buff = this->lastResponse;
		Replace(buff, L"oauth_token=", L"");
		Separate(buff, L"&oauth_token_secret=", this->authorization_token, this->authorization_token_secret);

		wstring authUrl;
		request.CreateAuthorizationUrl(this->authorization_token.c_str(), authUrl);
		authorizationUrl = new WCHAR[authUrl.size() + 1];
		wcscpy_s(authorizationUrl, authUrl.size() + 1, authUrl.c_str());
		*url = authorizationUrl;
	}

	UnLock();
	return ret;
}

//認証結果の暗証番号を設定する
//内部でGetAuthorizationUrlで取得した値を使用するので、一連の流れで設定する必要あり
//戻り値：
// エラーコード
//引数：
// password		[IN]暗証番号
DWORD CTwitterMain::SetAuthorizationPWD(
	LPCWSTR password
	)
{
	if( Lock() == FALSE ) return ERR_FALSE;
	DWORD ret = NO_ERR;
	CRequestUtil requestUtil;

	wstring requestUrl;
	wstring httpHeader;

	wstring key = CONSUMER_SECRET_KEY;
	key += L"&";
	key += this->authorization_token_secret.c_str();

	requestUtil.CreateAccessToken(CONSUMER_KEY, key, this->authorization_token.c_str(), password, requestUrl, httpHeader);
	ret = SendCmd(&this->httpUtil, FALSE, NW_VERB_POST, requestUrl.c_str(), httpHeader.c_str(), NULL, L"");
	if( ret == NO_ERR ){
		//レスポンス取得して解析
		wstring buff = this->lastResponse;
		Replace(buff, L"oauth_token=", L"");
		Separate(buff, L"&oauth_token_secret=", this->oauth_token, buff);
		Separate(buff, L"&", this->oauth_token_secret, buff);

		WritePrivateProfileString(L"SET", L"token", this->oauth_token.c_str(), this->moduleIniPath.c_str() );
		WritePrivateProfileString(L"SET", L"token_secret", this->oauth_token_secret.c_str(), this->moduleIniPath.c_str() );
	}

	UnLock();
	return ret;
}

//ツイートする
//戻り値：
// エラーコード
//引数：
// asyncMode	[IN]非同期で送信
// text			[IN]ツイート内容
DWORD CTwitterMain::SendTweet(
	BOOL asyncMode,
	LPCWSTR text
	)
{
	if( Lock() == FALSE ) return ERR_FALSE;

	DWORD ret = NO_ERR;
	if( this->oauth_token.size() > 0 && this->oauth_token_secret.size() > 0 ){
		wstring buff = text;
		if( buff.size() > 140 ){
			buff = buff.substr(0,139);
			buff+=L"…";
		}
		if( asyncMode == TRUE ){
			this->tweetQue.push_back(buff);

			SetEvent(this->tweetSetEvent);
			if( this->tweetThread == NULL ){
				ResetEvent(this->tweetStopEvent);
				this->tweetThread = (HANDLE)_beginthreadex(NULL, 0, TweetThread, (LPVOID)this, CREATE_SUSPENDED, NULL);
				SetThreadPriority( this->tweetThread, THREAD_PRIORITY_NORMAL );
				ResumeThread(this->tweetThread);
			}
		}else{
			CRequestUtil requestUtil;

			wstring requestUrl;
			wstring httpHeader;

			wstring key = CONSUMER_SECRET_KEY;
			key += L"&";
			key += this->oauth_token_secret.c_str();
			requestUtil.CreateSendTweet(CONSUMER_KEY, key, this->oauth_token.c_str(), buff.c_str(), requestUrl, httpHeader);
			if(SendCmd(&this->httpUtil, FALSE, NW_VERB_POST, requestUrl.c_str(), httpHeader.c_str(), NULL, L"") != NO_ERR ){
				_OutputDebugString(L"★tweet send Err:%s", this->lastResponse.c_str());
				OutputDebugString(buff.c_str());
				ret = ERR_FALSE;
			}
		}
	}else{
		ret = ERR_INIT;
	}

	UnLock();
	return ret;
}

UINT WINAPI CTwitterMain::TweetThread(LPVOID param)
{
	CTwitterMain* sys = (CTwitterMain*)param;
	HANDLE eventArray[2];
	eventArray[0] = sys->tweetStopEvent;
	eventArray[1] = sys->tweetSetEvent;

	DWORD nextWait = sys->sendWaitSec * 1000;
	while(1){
		DWORD res = WaitForMultipleObjects(2, eventArray, FALSE, INFINITE);
		if( res == WAIT_OBJECT_0 ){
			//キャンセルされた
			break;
		}else if( res == WAIT_OBJECT_0+1 ){
			while(1){
				BOOL end = FALSE;
				wstring text = L"";
				if( sys->Lock() == TRUE ){
					if( sys->tweetQue.size() > 0 ){
						text = sys->tweetQue[0];
						sys->tweetQue.erase(sys->tweetQue.begin());
					}else{
						end = TRUE;
					}
					sys->UnLock();
				}
				if( text.size() > 0 ){
					if( sys->oauth_token.size() > 0 && sys->oauth_token_secret.size() > 0 ){
						CRequestUtil requestUtil;

						wstring requestUrl;
						wstring httpHeader;

						wstring key = CONSUMER_SECRET_KEY;
						key += L"&";
						key += sys->oauth_token_secret.c_str();
						requestUtil.CreateSendTweet(CONSUMER_KEY, key, sys->oauth_token.c_str(), text, requestUrl, httpHeader);
						if(sys->SendCmd(&sys->httpUtil, FALSE, NW_VERB_POST, requestUrl.c_str(), httpHeader.c_str(), NULL, L"") != NO_ERR ){
							_OutputDebugString(L"★tweet send Err:%s", sys->lastResponse.c_str());
						}
					}
				}
				if(end == TRUE ){
					break;
				}
				if( WaitForSingleObject(sys->tweetStopEvent, nextWait) == WAIT_OBJECT_0 ){
					//キャンセルされた
					return 0;
				}
			}
		}
	}
	return 0;
}

DWORD CTwitterMain::SendCmd(CWinHTTPUtil* httpUtil, BOOL asyncMode, NW_VERB_MODE verbMode, wstring url, wstring httpHead, UPLOAD_DATA_LIST* upList, wstring saveFilePath, RESPONSE_READ callbackFunc, void* callbackFuncParam)
{
	DWORD ret = NO_ERR;

	if( asyncMode != httpUtil->IsOpenASync() ){
		httpUtil->CloseSession();
	}else
	if( this->chgProxy == TRUE ){
		httpUtil->CloseSession();
		this->chgProxy = FALSE;
	}
	if( httpUtil->IsOpenSession() == FALSE ){
		ret = httpUtil->OpenSession(L"", asyncMode, this->useProxy, &this->proxyInfo);
		if( ret != NO_ERR ){
			httpUtil->CloseSession();
			return ret;
		}
	}

	ret = httpUtil->SendRequest(url.c_str(), verbMode, httpHead.c_str(), saveFilePath.c_str(), callbackFunc, callbackFuncParam, upList);
	//エラーチェック
	if( ret == 0x8197 ){
		ret = ERR_NW_PROXY_LOGIN;
	}

	//同期接続の場合、レスポンスをここでメンバに取得しておく
	if( asyncMode == FALSE ){
		this->lastResponse = L"";

		DWORD dlSize = 0;
		httpUtil->GetDLBuff(NULL,&dlSize);
		if( dlSize > 0 ){
			BYTE* responseBuff = new BYTE[dlSize+1];
			ZeroMemory(responseBuff, dlSize+1);
			httpUtil->GetDLBuff(responseBuff,&dlSize);

			UTF8toW((char*)responseBuff, this->lastResponse);

			delete[] responseBuff;
		}
	}

	return ret;
}

//非同期ツイートの残りを取得する
//戻り値：
// 個数
DWORD CTwitterMain::GetTweetQue(
	)
{
	if( Lock() == FALSE ) return 0;

	DWORD ret = (DWORD)this->tweetQue.size();

	UnLock();
	return ret;
}

//ストリーミングを開始する
//戻り値：
// エラーコード
//引数：
// track		[IN]filterのtrack
// streamingID	[OUT]ストリーミング識別ID
DWORD CTwitterMain::StartTweetStreaming(
	LPCWSTR track,
	TW_CALLBACK_Streaming callbackFunc,
	void* callbackFuncParam,
	DWORD* streamingID
	)
{
	if( Lock() == FALSE ) return ERR_FALSE;

	if(streamingID == NULL || callbackFunc == NULL ){
		UnLock();
		return ERR_FALSE;
	}

	DWORD ret = NO_ERR;
	if( this->oauth_token.size() > 0 && this->oauth_token_secret.size() > 0 ){
		wstring buff = track;

		CRequestUtil requestUtil;
		STREAMING_INFO* info = new STREAMING_INFO();
		info->sys = this;
		info->streamingID = nextStreamingID;
		info->callbackFunc = callbackFunc;
		info->callbackFuncParam = callbackFuncParam;

		*streamingID = nextStreamingID;

		wstring requestUrl;
		wstring httpHeader;

		wstring key = CONSUMER_SECRET_KEY;
		key += L"&";
		key += this->oauth_token_secret.c_str();
		requestUtil.CreateStreaming(CONSUMER_KEY, key, this->oauth_token.c_str(), buff.c_str(), requestUrl, httpHeader);
		if(SendCmd(&info->httpUtil, TRUE, NW_VERB_POST, requestUrl.c_str(), httpHeader.c_str(), NULL, L"", StreamingCallback, info) != NO_ERR ){
			_OutputDebugString(L"★tweet send Err:%s", this->lastResponse.c_str());
			OutputDebugString(buff.c_str());
			ret = ERR_FALSE;
			SAFE_DELETE(info);
		}else{
			streamingList.insert(pair<DWORD, STREAMING_INFO*>(info->streamingID, info));
		}

	}else{
		ret = ERR_INIT;
	}

	UnLock();
	return ret;
}

//ストリーミングを停止する
//戻り値：
// エラーコード
//引数：
// streamingID	[IN]ストリーミング識別ID
DWORD CTwitterMain::StopTweetStreaming(
	DWORD streamingID
	)
{
	if( Lock() == FALSE ) return 0;

	DWORD ret = NO_ERR;

	map<DWORD, STREAMING_INFO*>::iterator itr;
	itr = streamingList.find(streamingID);
	if( itr != streamingList.end() ){
		itr->second->httpUtil.CloseSession();
		SAFE_DELETE(itr->second);

		streamingList.erase(itr);
	}

	UnLock();
	return ret;
}

int CALLBACK CTwitterMain::StreamingCallback(void* param, BYTE* data, DWORD dataSize)
{
	STREAMING_INFO* info = (STREAMING_INFO*)param;
	if( info == NULL ){
		return -1;
	}
	CTwitterMain* sys = (CTwitterMain*)info->sys;

	if( info->callbackFunc != NULL ){
		info->callbackFunc(info->callbackFuncParam, info->streamingID, (const char*)data, dataSize);
	}

	return 0;
}
