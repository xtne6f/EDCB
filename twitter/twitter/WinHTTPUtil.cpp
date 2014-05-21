#include "StdAfx.h"
#include "WinHTTPUtil.h"
#include <process.h>

CWinHTTPUtil::CWinHTTPUtil(void)
{
	this->session = NULL;
	this->connect = NULL;
	this->request = NULL;

	this->useProxy = FALSE;
	this->asyncMode = FALSE;
	this->lastHost = L"";
	this->lastPort = 0;

    this->upThread = NULL;
    this->upStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	this->writeCompEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	this->responseCompEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	this->callbackRevFunc = NULL;
	this->callbackRecvFuncParam = NULL;
}


CWinHTTPUtil::~CWinHTTPUtil(void)
{
	CloseSession();
	ClearUpList();
	for( size_t i=0; i<this->dlBuffList.size(); i++ ){
		SAFE_DELETE(this->dlBuffList[i]);
	}
	this->dlBuffList.clear();


	if( this->upStopEvent != NULL ){
		CloseHandle(this->upStopEvent);
		this->upStopEvent = NULL;
	}
	if( this->writeCompEvent != NULL ){
		CloseHandle(this->writeCompEvent);
		this->writeCompEvent = NULL;
	}
	if( this->responseCompEvent != NULL ){
		CloseHandle(this->responseCompEvent);
		this->responseCompEvent = NULL;
	}
}

void CWinHTTPUtil::ClearUpList()
{
	for( size_t i=0; i<this->upDataList.size(); i++ ){
		SAFE_DELETE(this->upDataList[i]);
	}
	this->upDataList.clear();
}

//セッションをオープンする
//戻り値：エラーコード
DWORD CWinHTTPUtil::OpenSession(
	LPCWSTR agent,				//[IN] Agentの設定
	BOOL async,					//[IN] 非同期モードで動くかどうか（TRUE：非同期）
	BOOL useProxy,				//[IN] Proxy使うかどうか（TRUE:Proxy使う）
	USE_PROXY_INFO* proxyInfo	//[IN] Proxy使う場合の設定情報
	)
{
	if( this->session != NULL ){
		//すでにセッション張ってる
		return ERR_NW_ALREADY_SESSION;
	}


	this->useProxy = useProxy;
	this->asyncMode = async;
	if( useProxy == TRUE ){
		if( proxyInfo->userName != NULL ){
			this->proxyID = proxyInfo->userName;
		}
		if( proxyInfo->password != NULL ){
			this->proxyPWD = proxyInfo->password;
		}
	}else{
		this->proxyID = L"";
		this->proxyPWD = L"";
	}

	if( this->useProxy == FALSE ){
		this->session = WinHttpOpen( agent, WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, WINHTTP_FLAG_ASYNC );
	}else{
		if( proxyInfo == NULL ){
			return ERR_INVALID_ARG;
		}
		this->session = WinHttpOpen( agent, WINHTTP_ACCESS_TYPE_NAMED_PROXY, proxyInfo->serverName, proxyInfo->proxyBypass, WINHTTP_FLAG_ASYNC );
	}
	
	if( this->session == NULL ){
		return ERR_NW_OPEN_SESSION;
	}else{
		WinHttpSetTimeouts( this->session, 1000*60, 1000*60*2, 1000*60*2, 1000*30*4 ); 
	}

	return NO_ERR;
}

//セッションを閉じる
//コネクションとリクエストも閉じる
//戻り値：エラーコード
DWORD CWinHTTPUtil::CloseSession()
{
	if( this->upThread != NULL ){
		::SetEvent(this->upStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->upThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->upThread, 0xffffffff);
		}
		CloseHandle(this->upThread);
		this->upThread = NULL;
	}

	//コネクション系のクローズ
	if( this->request != NULL ){
		WinHttpSetStatusCallback( this->request, NULL, NULL, NULL );
		WinHttpCloseHandle(this->request);
		this->request = NULL;
	}
	if( this->connect != NULL ){
		WinHttpCloseHandle(this->connect);
		this->connect = NULL;
	}
	if( this->session != NULL ){
		WinHttpCloseHandle(this->session);
		this->session = NULL;
	}
	return NO_ERR;
}

//セッションが開かれているかどうか
//戻り値：
// TRUE（オープン済み）、FALSE（未オープン）
BOOL CWinHTTPUtil::IsOpenSession()
{
	if( this->session != NULL ){
		return TRUE;
	}else{
		return FALSE;
	}
}

//セッションが非同期で開かれているかどうか
//戻り値：
// TRUE（非同期）、FALSE（同期）
BOOL CWinHTTPUtil::IsOpenASync()
{
	return this->asyncMode;
}

//リクエストを送る
//内部メモリバッファにDLする場合はファイル容量に要注意
//戻り値：エラーコード
DWORD CWinHTTPUtil::SendRequest( 
	LPCWSTR url,					//[IN] アクセスするURL
	NW_VERB_MODE verbMode,			//[IN] VERBの種類
	LPCWSTR addHttpHeader,			//[IN] Httpヘッダに追加するものあるなら指定
	LPCWSTR saveFilePath,			//[IN] DLファイル名、NULL時は内部メモリバッファにDL
	RESPONSE_READ callbackFunc,		//[IN] レスポンス受信時に呼び出すコールバック
	void* callbackFuncParam,		//[IN] RESPONSE_READの第一引数に入る物
	UPLOAD_DATA_LIST* upList		//[IN] サーバーに送信するデータ(PUT or POST)
	)
{
	if( url == NULL ){
		return ERR_INVALID_ARG;
	}
	if( this->session == NULL ){
		return ERR_NW_NO_SESSION;
	}
	//リクエストクローズ
	if( this->request != NULL ){
		WinHttpSetStatusCallback( this->request, NULL, NULL, NULL );
		WinHttpCloseHandle(this->request);
		this->request = NULL;
	}
	ClearUpList();
	for( size_t i=0; i<this->dlBuffList.size(); i++ ){
		SAFE_DELETE(this->dlBuffList[i]);
	}
	this->dlBuffList.clear();

	//URLの分解
	URL_COMPONENTS stURL;
	ZeroMemory(&stURL, sizeof(URL_COMPONENTS));
	stURL.dwStructSize = sizeof(URL_COMPONENTS);

	stURL.dwSchemeLength    = -1;
	stURL.dwHostNameLength  = -1;
	stURL.dwUrlPathLength   = -1;
	stURL.dwExtraInfoLength = -1;

	if( WinHttpCrackUrl( url, (DWORD)wcslen(url), 0, &stURL) == NULL ){
		return ERR_FALSE;
	}
	if( stURL.dwHostNameLength <= 0 ){
		return ERR_FALSE;
	}

	wstring host = L"";
	host.insert(0,stURL.lpszHostName, stURL.dwHostNameLength );
	host += L"\0";

	if( CompareNoCase(host, stURL.lpszHostName) != 0 || this->lastPort != stURL.nPort){
		//前回と違うコネクションなのでやりなおし
		if( this->connect != NULL ){
			WinHttpCloseHandle(this->connect);
			this->connect = NULL;
		}
	}

	//コネクションオープン
	if( this->connect == NULL ){
		this->connect = WinHttpConnect( this->session, host.c_str(), stURL.nPort, 0 );
		if( this->connect == NULL ){
			return ERR_NW_OPEN_CONNECT;
		}
	}

	//Verbの設定
	wstring verb = L"GET";
	if( verbMode == NW_VERB_GET ){
		verb = L"GET";
	}else if( verbMode == NW_VERB_POST ){
		verb = L"POST";
	}else if( verbMode == NW_VERB_PUT ){
		verb = L"PUT";
	}

	wstring sendUrl = L"";
	sendUrl.insert(0, stURL.lpszUrlPath, stURL.dwUrlPathLength+stURL.dwExtraInfoLength);
	sendUrl += L"\0";

	//リクエストオープン
	if( stURL.nPort == INTERNET_DEFAULT_HTTPS_PORT ){
		this->request = WinHttpOpenRequest( this->connect, verb.c_str(), sendUrl.c_str(),
			NULL, WINHTTP_NO_REFERER, 
			WINHTTP_DEFAULT_ACCEPT_TYPES, 
			WINHTTP_FLAG_SECURE);
	}else{
		this->request = WinHttpOpenRequest( this->connect, verb.c_str(), sendUrl.c_str(),
			NULL, WINHTTP_NO_REFERER, 
			WINHTTP_DEFAULT_ACCEPT_TYPES, 
			0 );
	}
	if( this->request == NULL ){
		return ERR_NW_OPEN_REQUEST;
	}

	if( this->useProxy == TRUE ){
		//ProxyのIDとパスワードがあったらセット
		if( this->proxyID.size() > 0 || this->proxyPWD.size() > 0 ){
			if( WinHttpSetCredentials( this->request,
				WINHTTP_AUTH_TARGET_PROXY,
				WINHTTP_AUTH_SCHEME_BASIC,
				this->proxyID.c_str(),
				this->proxyPWD.c_str(),
				NULL )  == FALSE ){
				return ERR_NW_PROXY_LOGIN;
			}
		}
	}

	//HTTPヘッダを設定
	if( addHttpHeader != NULL ){
		WinHttpAddRequestHeaders( this->request, addHttpHeader, -1, WINHTTP_ADDREQ_FLAG_ADD );
	}

	//コールバック設定
	WinHttpSetStatusCallback(this->request, StatusCallback, WINHTTP_CALLBACK_FLAG_ALL_COMPLETIONS, NULL );

	//アップロードデータのコピー
	this->totalUpSize = 0;
	if( upList != NULL ){
		for( DWORD i=0; i<upList->listCount; i++ ){
			UPLOAD_DATA* item = new UPLOAD_DATA;
			item->filePathFlag = upList->list[i].filePathFlag;
			item->buffSize = upList->list[i].buffSize;
			if( item->buffSize > 0 ){
				item->buff = new BYTE[item->buffSize];
				memcpy(item->buff, upList->list[i].buff, item->buffSize);
			}
			upDataList.push_back(item);

			if( item->filePathFlag == TRUE ){
				HANDLE file = CreateFileW( (WCHAR*)item->buff, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
				if( file == INVALID_HANDLE_VALUE ){
					//ファイルにアクセスできないのでエラー
					return ERR_NW_OPEN_FILE;
				}
				DWORD HSize=0;
				DWORD LSize=0;
				LSize = GetFileSize(file, &HSize);
				CloseHandle(file);
				if( HSize > 0 ){
					//DWORD以上は未サポート
					return ERR_SIZE;
				}
				this->totalUpSize += LSize;
			}else{
				this->totalUpSize  += item->buffSize;
			}
		}
	}

	this->lastHost = host;
	this->lastPort = stURL.nPort;
	if( saveFilePath != NULL ){
		this->saveFilePath = saveFilePath;
	}else{
		this->saveFilePath = L"";
	}

	this->callbackRevFunc = callbackFunc;
	this->callbackRecvFuncParam = callbackFuncParam;

	ResetEvent(this->upStopEvent);
	ResetEvent(this->responseCompEvent);
	this->errEndCode = NO_ERR;
	if( WinHttpSendRequest( this->request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, totalUpSize, (DWORD_PTR)this) == FALSE ){
		return ERR_NW_SEND_REQUEST;
	}
	if( this->asyncMode == FALSE ){
		//同期なので完了するまで待つ
		WaitForSingleObject(this->responseCompEvent, INFINITE);
		return this->errEndCode;
	}
	return NO_ERR;
}

//リクエストを閉じる
//戻り値：エラーコード
DWORD CWinHTTPUtil::CloseRequest()
{
	if( this->upThread != NULL ){
		::SetEvent(this->upStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->upThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->upThread, 0xffffffff);
		}
		CloseHandle(this->upThread);
		this->upThread = NULL;
	}
	if( this->request != NULL ){
		WinHttpSetStatusCallback( this->request, NULL, NULL, NULL );
		WinHttpCloseHandle(this->request);
		this->request = NULL;
	}
	if( this->connect != NULL ){
		WinHttpCloseHandle(this->connect);
		this->connect = NULL;
	}

	return NO_ERR;
}



void CALLBACK CWinHTTPUtil::StatusCallback (
		HINTERNET hInternet,
		DWORD_PTR dwContext,
		DWORD dwInternetStatus,
		LPVOID lpvStatusInformation,
		DWORD dwStatusInformationLength
		)
{
	CWinHTTPUtil* sys = (CWinHTTPUtil*)dwContext;

	switch(dwInternetStatus){
		case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE:
			//WinHttpSendRequest
			sys->StatusSendRequestComplete();
			break;
		case WINHTTP_CALLBACK_STATUS_WRITE_COMPLETE:
			//WinHttpWriteData
			sys->StatusWriteComplete(*((DWORD *)lpvStatusInformation));
			break;
		case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE:
			//WinHttpReceiveResponse
			sys->StatusHeadersAvailable();
			break;
		case WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE:
			//WinHttpQueryDataAvailable
			sys->StatusDataAvailable(*((DWORD*)lpvStatusInformation));
			break;
		case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR:
			//エラー発生
			{
				WinHttpSetStatusCallback( sys->request, NULL, NULL, NULL );
				sys->errEndCode = ERR_NW_FALSE;
				SetEvent(sys->responseCompEvent);
			}
			break;
		default:
			break;
	}
	
}

void CWinHTTPUtil::StatusSendRequestComplete()
{
	if( this->totalUpSize > 0 ){
		//アップ処理
		this->upThread = (HANDLE)_beginthreadex( NULL, 0, UploadThread, this, CREATE_SUSPENDED, NULL );
		if( this->upThread == NULL ){
			return ;
		}
		ResumeThread(this->upThread);
	}else{
		WinHttpReceiveResponse(this->request, NULL);
	}

}

void CWinHTTPUtil::StatusWriteComplete(DWORD size)
{
	SetEvent(this->writeCompEvent);
}

void CWinHTTPUtil::StatusHeadersAvailable()
{
	DWORD size = sizeof(DWORD);

	//HTTPのステータスコード確認
	WinHttpQueryHeaders( this->request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &this->responseStatusCode, &size, NULL );

	//レスポンスヘッダを全部取得
	WinHttpQueryHeaders( this->request, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, NULL, &size, WINHTTP_NO_HEADER_INDEX);
	WCHAR* rawHeader = new WCHAR[size/sizeof(WCHAR) + 1];
	ZeroMemory(rawHeader, sizeof(WCHAR) * (size/sizeof(WCHAR) + 1));
	if( WinHttpQueryHeaders( this->request, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, rawHeader, &size, WINHTTP_NO_HEADER_INDEX) == TRUE ){
		this->responseHTTPHeader = rawHeader;
	}else{
		this->responseHTTPHeader = L"";
	}
	SAFE_DELETE_ARRAY(rawHeader)

	//ContentLengthを取得してみる
	DWORD fileLength = 0;
	size = sizeof(DWORD);
	if( WinHttpQueryHeaders( this->request, WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER , NULL, &fileLength, &size, NULL ) == TRUE ){
		this->responseContentSize = fileLength;
	}else{
		this->responseContentSize = 0;
	}

	if( this->responseStatusCode == 200 || this->responseStatusCode == 201){
		//サイズ０のファイル作成
		if( this->saveFilePath.size() > 0 ){
			HANDLE file = CreateFile( this->saveFilePath.c_str(), GENERIC_READ|GENERIC_WRITE , FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
			if( file != INVALID_HANDLE_VALUE){
				CloseHandle(file);
			}
		}
	}else{
		this->errEndCode = ERR_NW_FALSE;
	}

	WinHttpQueryDataAvailable( this->request, NULL);

}

void CWinHTTPUtil::StatusDataAvailable(DWORD size)
{
	if( size > 0 ){
		DL_DATA* item = new DL_DATA;
		item->size = size;
		item->data = new BYTE[item->size];
		ZeroMemory(item->data, item->size);

		WinHttpReadData( this->request, item->data, item->size, NULL);

		if( this->saveFilePath.size() > 0 ){
			HANDLE file = CreateFile( this->saveFilePath.c_str(), GENERIC_READ|GENERIC_WRITE , FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
			if( file != INVALID_HANDLE_VALUE){
				SetFilePointer(file, 0,NULL, FILE_END );
				DWORD write;
				WriteFile( file, item->data, item->size, &write, NULL );
				CloseHandle(file);
			}

			SAFE_DELETE(item);
		}else{
			if( this->callbackRevFunc != NULL ){
				this->callbackRevFunc(this->callbackRecvFuncParam, item->data, item->size);
				SAFE_DELETE(item);
			}else{
				this->dlBuffList.push_back(item);
			}
		}

		WinHttpQueryDataAvailable( this->request, NULL);
	}else{
		//DL終了
		SetEvent(this->responseCompEvent);
	}
}

UINT CWinHTTPUtil::UploadThread(LPVOID pParam)
{
	if( pParam == NULL ){
		return 0;
	}
	CWinHTTPUtil* sys = (CWinHTTPUtil*)pParam;

	HANDLE eventArray[2];
	eventArray[0] = sys->upStopEvent;
	eventArray[1] = sys->writeCompEvent;

	SetEvent(sys->writeCompEvent);
	for( size_t i=0; i<sys->upDataList.size(); i++ ){
		UPLOAD_DATA* data = sys->upDataList[i];
		if( data->filePathFlag == FALSE ){
			DWORD res = WaitForMultipleObjects(2, eventArray, FALSE, INFINITE);
			if( res == WAIT_OBJECT_0 ){
				//STOP
				break;
			}else if( res == WAIT_OBJECT_0+1 ){
				WinHttpWriteData( sys->request, data->buff, data->buffSize, NULL );
			}
		}else{
			HANDLE file;
			file = CreateFile( (WCHAR*)data->buff, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
			if( file == INVALID_HANDLE_VALUE ){
				sys->errEndCode = ERR_NW_FILE_OPEN;
				WinHttpSetStatusCallback( sys->request, NULL, NULL, NULL );
				SetEvent(sys->responseCompEvent);
				return 0;
			}
			DWORD readSize = 0;
			DWORD readBuffSize = 0;
			BYTE buff[SEND_BUFF_SIZE];
			DWORD fileSize = GetFileSize(file, NULL);
			while( readSize < fileSize ){
				DWORD res = WaitForMultipleObjects(2, eventArray, FALSE, INFINITE);
				if( res == WAIT_OBJECT_0 ){
					//STOP
					break;
				}else if( res == WAIT_OBJECT_0+1 ){
					DWORD buffSize = 0;
					if( SEND_BUFF_SIZE+readSize < fileSize ){
						buffSize = SEND_BUFF_SIZE;
					}else{
						buffSize = fileSize - readSize;
					}
					if( ReadFile(file, buff, SEND_BUFF_SIZE, &readBuffSize, NULL ) == FALSE ){
						sys->errEndCode = ERR_NW_FILE_OPEN;
						WinHttpSetStatusCallback( sys->request, NULL, NULL, NULL );
						SetEvent(sys->responseCompEvent);
						return 0;
					}else{
						DWORD retryCount = 0;
						while( WinHttpWriteData( sys->request, buff, readBuffSize, NULL ) == FALSE ){
							Sleep(200);
							if( retryCount > 100 ){
								sys->errEndCode = ERR_NW_FALSE;
								WinHttpSetStatusCallback( sys->request, NULL, NULL, NULL );
								SetEvent(sys->responseCompEvent);
								return 0;
							}
							retryCount++;
						}
					}

					readSize+=buffSize;
				}
			}
			CloseHandle(file);
		}
	}

	sys->ClearUpList();
	return 0;
}

DWORD CWinHTTPUtil::GetDLBuff( 
	BYTE* buff, //[OUT] DLしたデータ
	DWORD* size //[IN/OUT] pbBuffのサイズ
	)
{
	DWORD totalSize = 0;
	vector<DL_DATA*>::iterator itr;
	for( itr=this->dlBuffList.begin(); itr!=this->dlBuffList.end(); itr++ ){
		totalSize += (*itr)->size;
	}
	
	if( buff == NULL ){
		*size = totalSize;
		return NO_ERR;
	}
	if( *size < totalSize ){
		return ERR_FALSE;
	}
	DWORD writeSize = 0;
	for( itr=this->dlBuffList.begin(); itr!=this->dlBuffList.end(); itr++ ){
		memcpy( buff+writeSize, (*itr)->data, (*itr)->size );
		writeSize += (*itr)->size;
	}
	
	return NO_ERR;
}

