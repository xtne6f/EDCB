#ifndef __NW_STRUCT_DEF_H__
#define __NW_STRUCT_DEF_H__

enum NW_VERB_MODE{
	NW_VERB_GET = 0,
	NW_VERB_POST,
	NW_VERB_PUT,
};

//接続時に使用するProxyの情報
typedef struct _USE_PROXY_INFO{
	WCHAR* serverName;		//サーバー名（アドレス:ポート番号）
	WCHAR* proxyBypass;		//Proxyの例外設定
	WCHAR* userName;		//認証Proxyユーザー名
	WCHAR* password;		//認証Proxyパスワード
	_USE_PROXY_INFO(void){
		serverName = NULL;
		proxyBypass = NULL;
		userName = NULL;
		password = NULL;
	};
	~_USE_PROXY_INFO(void){
		SAFE_DELETE_ARRAY(serverName);
		SAFE_DELETE_ARRAY(proxyBypass);
		SAFE_DELETE_ARRAY(userName);
		SAFE_DELETE_ARRAY(password);
	};
	_USE_PROXY_INFO & operator= (const _USE_PROXY_INFO & o) {
		SAFE_DELETE_ARRAY(serverName);
		SAFE_DELETE_ARRAY(proxyBypass);
		SAFE_DELETE_ARRAY(userName);
		SAFE_DELETE_ARRAY(password);
		size_t len = 0;
		if( o.serverName != NULL ){
			len = wcslen(o.serverName);
			if( len > 0 ){
				serverName = new WCHAR[len+1];
				wcscpy_s(serverName, len+1, o.serverName);
			}
		}
		if( o.proxyBypass != NULL ){
			len = wcslen(o.proxyBypass);
			if( len > 0 ){
				proxyBypass = new WCHAR[len+1];
				wcscpy_s(proxyBypass, len+1, o.proxyBypass);
			}
		}
		if( o.userName != NULL ){
			len = wcslen(o.userName);
			if( len > 0 ){
				userName = new WCHAR[len+1];
				wcscpy_s(userName, len+1, o.userName);
			}
		}
		if( o.password != NULL ){
			len = wcslen(o.password);
			if( len > 0 ){
				password = new WCHAR[len+1];
				wcscpy_s(password, len+1, o.password);
			}
		}
		return *this;
	};
} USE_PROXY_INFO;

//IEのProxyの情報
typedef struct _CURRENT_USER_IE_PROXY_CONFIG{
	BOOL autoDetect;		//設定を自動的に検出するかどうか
	WCHAR* autoConfigUrl;	//自動構成スクリプトのURL
	WCHAR* proxy;			//サーバー名（アドレス:ポート番号）
	WCHAR* proxyBypass;		//Proxyの例外設定
	_CURRENT_USER_IE_PROXY_CONFIG(void){
		autoConfigUrl = NULL;
		proxy = NULL;
		proxyBypass = NULL;
	};
	~_CURRENT_USER_IE_PROXY_CONFIG(void){
		SAFE_DELETE_ARRAY(autoConfigUrl);
		SAFE_DELETE_ARRAY(proxy);
		SAFE_DELETE_ARRAY(proxyBypass);
	};
} CURRENT_USER_IE_PROXY_CONFIG;

//Proxyの情報
typedef struct _PROXY_CONFIG{
	WCHAR* proxy;			//サーバー名（アドレス:ポート番号）
	WCHAR* proxyBypass;		//Proxyの例外設定
	_PROXY_CONFIG(void){
		proxy = NULL;
		proxyBypass = NULL;
	};
	~_PROXY_CONFIG(void){
		SAFE_DELETE_ARRAY(proxy);
		SAFE_DELETE_ARRAY(proxyBypass);
	};
} PROXY_CONFIG;

//アップロードするデータ
typedef struct _UPLOAD_DATA{
	BOOL filePathFlag;		//buffがファイルパスかどうか
	DWORD buffSize;			//buffのサイズ（バイト単位）
	BYTE* buff;				//データのバッファ（ファイルパス時はWCHARで\0で終わるバッファ）
	_UPLOAD_DATA(void){
		filePathFlag = FALSE;
		buffSize = 0;
		buff = NULL;
	};
	~_UPLOAD_DATA(void){
		SAFE_DELETE_ARRAY(buff);
	};
} UPLOAD_DATA;

//アップロードするデータのリスト
//配列の順にアップロードデータを構築する（単純にバイナリ的に結合）
typedef struct _UPLOAD_DATA_LIST{
	DWORD listCount;		//listの個数
	UPLOAD_DATA* list;		//UPLOAD_DATAの配列
	_UPLOAD_DATA_LIST(void){
		listCount = 0;
		list = NULL;
	};
	~_UPLOAD_DATA_LIST(void){
		SAFE_DELETE_ARRAY(list);
	};
} UPLOAD_DATA_LIST;

#endif
