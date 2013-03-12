#pragma once

#include <winhttp.h>
#pragma comment (lib, "winhttp.lib")

#include "../../Common/NWStructDef.h"
#include "../../Common/StringUtil.h"
#include "../../Common/ErrDef.h"

#define SEND_BUFF_SIZE 64*1024

typedef int (CALLBACK *RESPONSE_READ)(void* param, BYTE* data, DWORD dataSize);

class CWinHTTPUtil
{
public:
	CWinHTTPUtil(void);
	~CWinHTTPUtil(void);

	//セッションをオープンする
	//戻り値：エラーコード
	DWORD OpenSession(
		LPCWSTR agent,				//[IN] Agentの設定
		BOOL async,					//[IN] 非同期モードで動くかどうか（TRUE：非同期）
		BOOL useProxy,				//[IN] Proxy使うかどうか（TRUE:Proxy使う）
		USE_PROXY_INFO* proxyInfo	//[IN] Proxy使う場合の設定情報
		);

	//セッションを閉じる
	//コネクションとリクエストも閉じる
	//戻り値：エラーコード
	DWORD CloseSession();

	//セッションが開かれているかどうか
	//戻り値：
	// TRUE（オープン済み）、FALSE（未オープン）
	BOOL IsOpenSession();

	//セッションが非同期で開かれているかどうか
	//戻り値：
	// TRUE（非同期）、FALSE（同期）
	BOOL IsOpenASync();

	//リクエストを送る
	//内部メモリバッファにDLする場合はファイル容量に要注意
	//戻り値：エラーコード
	DWORD SendRequest( 
		LPCWSTR url,					//[IN] アクセスするURL
		NW_VERB_MODE verbMode,			//[IN] VERBの種類
		LPCWSTR addHttpHeader,			//[IN] Httpヘッダに追加するものあるなら指定
		LPCWSTR saveFilePath,			//[IN] DLファイル名、callbackFuncもNULL時は内部メモリバッファにDL
		RESPONSE_READ callbackFunc,		//[IN] レスポンス受信時に呼び出すコールバック
		void* callbackFuncParam,		//[IN] RESPONSE_READの第一引数に入る物
		UPLOAD_DATA_LIST* upList		//[IN] サーバーに送信するデータ(PUT or POST)
		);

	//リクエストを閉じる
	//戻り値：エラーコード
	DWORD CloseRequest();

	//内部メモリバッファにDLした場合にデータを取り出す
	//戻り値：エラーコード
	DWORD GetDLBuff(
		BYTE* buff, //[OUT] DLしたデータ NULL指定でサイズ取得可能
		DWORD* size //[IN/OUT] buffのサイズ
		);

protected:
	HINTERNET session;
	HINTERNET connect;
	HINTERNET request;

	BOOL useProxy;
	wstring proxyID;
	wstring proxyPWD;
	BOOL asyncMode;

	wstring lastHost;
	DWORD lastPort;
	wstring saveFilePath;

	vector<UPLOAD_DATA*> upDataList;
	DWORD totalUpSize;
	HANDLE upThread;
	HANDLE upStopEvent;
	HANDLE writeCompEvent;
	HANDLE responseCompEvent;
	DWORD errEndCode;

	wstring responseHTTPHeader;
	DWORD responseStatusCode;
	DWORD responseContentSize;

	typedef struct _DL_DATA{
		DWORD size;
		BYTE* data;
		_DL_DATA(void){
			size = 0;
			data = NULL;
		};
		~_DL_DATA(void){
			SAFE_DELETE_ARRAY(data);
		};
	} DL_DATA;
	vector<DL_DATA*> dlBuffList;

	RESPONSE_READ callbackRevFunc;
	void* callbackRecvFuncParam;

protected:
	static void CALLBACK StatusCallback (
		HINTERNET hInternet,
		DWORD_PTR dwContext,
		DWORD dwInternetStatus,
		LPVOID lpvStatusInformation,
		DWORD dwStatusInformationLength
		);
	static UINT WINAPI UploadThread(LPVOID pParam);

	void StatusSendRequestComplete();
	void StatusWriteComplete(DWORD size);
	void StatusHeadersAvailable();
	void StatusDataAvailable(DWORD size);

	void ClearUpList();
};

