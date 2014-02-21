#include "stdafx.h"

#include "MPInclude.h"

#include "UpnpUtil.h"
#include "UpnpSsdpUtil.h"
#include "ThreadUtil.h"
#include "NICUtil.h"
#include "CharUtil.h"
#include "HttpUtil.h"

#define UPNP_SSDP_RECV_BUFF_SIZE (2048+1)

typedef struct _UPNP_SSDP_RECV_PARAM{
	char endFlag;
	char stopFlag;

	char key[64];

	char recvData[UPNP_SSDP_RECV_BUFF_SIZE];

	struct sockaddr_in client;
	int addr_len;

	MP_NIC_INFO* nicInfo;

	void* ssdpHandle;
	MP_THREAD_HANDLE threadHandle;
}UPNP_SSDP_RECV_PARAM;

UPNP_SSDP_RECV_PARAM* UPNP_SSDP_RECV_PARAM_New();
void UPNP_SSDP_RECV_PARAM_Delete(void* item);

UPNP_SSDP_RECV_PARAM* UPNP_SSDP_RECV_PARAM_New()
{
	UPNP_SSDP_RECV_PARAM* item = NULL;
	item = (UPNP_SSDP_RECV_PARAM*)malloc(sizeof(UPNP_SSDP_RECV_PARAM));
	if( item != NULL ){
		item->endFlag = 0;
		item->stopFlag = 0;

		memset(item->key, 0, 64);
		memset(item->recvData, 0, UPNP_SSDP_RECV_BUFF_SIZE);

		item->addr_len = sizeof(item->client);
		item->nicInfo = NULL;

		item->ssdpHandle = NULL;
		item->threadHandle = NULL;
	}
	return item;
}

void UPNP_SSDP_RECV_PARAM_Delete(void* item)
{
	UPNP_SSDP_RECV_PARAM* info = (UPNP_SSDP_RECV_PARAM*)item;
	if( info == NULL ){
		return ;
	}
	if( info->threadHandle != NULL ){
		MP_THREAD_WaitEndThread(&info->threadHandle);
	}
	if( info->nicInfo != NULL ){
		MP_NIC_INFO_Delete(info->nicInfo);
	}
	free(info);
}

typedef struct _UPNP_SSDP_SOCKET_ITEM{
	MP_NIC_INFO* nicInfo;
	SOCKET ssdpSock;
}UPNP_SSDP_SOCKET_ITEM;

UPNP_SSDP_SOCKET_ITEM* UPNP_SSDP_SOCKET_ITEM_New();
void UPNP_SSDP_SOCKET_ITEM_Delete(void* item);

UPNP_SSDP_SOCKET_ITEM* UPNP_SSDP_SOCKET_ITEM_New()
{
	UPNP_SSDP_SOCKET_ITEM* item = NULL;
	item = (UPNP_SSDP_SOCKET_ITEM*)malloc(sizeof(UPNP_SSDP_SOCKET_ITEM));
	if( item != NULL ){
		item->nicInfo = NULL;
		item->ssdpSock = -1;
	}
	return item;
}

void UPNP_SSDP_SOCKET_ITEM_Delete(void* item)
{
	UPNP_SSDP_SOCKET_ITEM* info = (UPNP_SSDP_SOCKET_ITEM*)item;
	if( info == NULL ){
		return ;
	}
	if( info->nicInfo != NULL ){
		MP_NIC_INFO_Delete(info->nicInfo);
	}
	if( info->ssdpSock >= 0 ){
		shutdown(info->ssdpSock,SD_BOTH);
#ifdef _WIN32
		closesocket(info->ssdpSock);
#else
		close(info->ssdpSock);
#endif
		info->ssdpSock = -1;
	}
	free(info);
}

typedef struct _UPNP_SSDP_NOTIFY_DEVICE_INFO{
	char* uuid;					//uuid「uuid:」は省く
	int locationPort;			//Locationのポート
	char* locationUri;			//Locationの「http://ip:port/」より後ろの部分
	SORT_LIST_HANDLE urnList;	//urn「uuid:」は省いたもののリスト（キ－char*、値char*）
}UPNP_SSDP_NOTIFY_DEVICE_INFO;

UPNP_SSDP_NOTIFY_DEVICE_INFO* UPNP_SSDP_NOTIFY_DEVICE_INFO_New();
void UPNP_SSDP_NOTIFY_DEVICE_INFO_Delete(void* item);

UPNP_SSDP_NOTIFY_DEVICE_INFO* UPNP_SSDP_NOTIFY_DEVICE_INFO_New()
{
	UPNP_SSDP_NOTIFY_DEVICE_INFO* item = NULL;

	item = (UPNP_SSDP_NOTIFY_DEVICE_INFO*)malloc(sizeof(UPNP_SSDP_NOTIFY_DEVICE_INFO));
	if( item != NULL ){
		item->uuid = NULL;
		item->locationPort = 0;
		item->locationUri = NULL;
		item->urnList = SORT_LIST_CreateHandle(0, SORT_LIST_COMPDEF_strcmp, NULL, SORT_LIST_DELDEF_free);
	}
	return item;
}

void UPNP_SSDP_NOTIFY_DEVICE_INFO_Delete(void* item)
{
	UPNP_SSDP_NOTIFY_DEVICE_INFO* info = (UPNP_SSDP_NOTIFY_DEVICE_INFO*)item;
	if( info == NULL ){
		return ;
	}

	if( info->uuid != NULL ){
		free(info->uuid);
	}
	if( info->locationUri != NULL ){
		free(info->locationUri);
	}

	SORT_LIST_CloseHandle(&info->urnList);

	free(item);
}

typedef struct _UPNP_SSDP_HANDLE_ITEM{
	UPNP_SSDP_CALLBACK ssdpCallBack;	//[IN]SSDPのNotify通知用コールバック
	void* ssdpCallBackParam;			//[IN]SSDPのNotify通知用コールバック呼出時に入れるパラメーター
	UPNP_MSEARCH_REQUEST_CALLBACK msearchCallBack;	//[IN]SSDPのM-SEARCH通知用コールバック
	void* msearchCallBackParam;			//[IN]SSDPのM-SEARCH通知用コールバック呼出時に入れるパラメーター

	MP_LOCK_HANDLE lockHandle;
	MP_THREAD_HANDLE ssdpSrvThread;
	int ssdpSrvStopFlag;
	MP_THREAD_HANDLE notifySendThread;
	int notifySendStopFlag;

	SORT_LIST_HANDLE ssdpSockList;

	SORT_LIST_HANDLE recvThreadList;

	SORT_LIST_HANDLE notifyDeviceList;	//（キ－char*(uuid)、値UPNP_SSDP_NOTIFY_DEVICE_INFO*）
}UPNP_SSDP_HANDLE_ITEM;

typedef struct _UPNP_MSEARCH_HANDLE_ITEM{
	UPNP_MSEARCH_REQUEST_CALLBACK msearchCallBack;	//[IN]SSDPのM-SEARCH通知用コールバック
	void* msearchCallBackParam;			//[IN]SSDPのM-SEARCH通知用コールバック呼出時に入れるパラメーター

	MP_LOCK_HANDLE lockHandle;
	MP_THREAD_HANDLE msearchThread;
	int msearchStopFlag;

	SORT_LIST_HANDLE resultList;
}UPNP_MSEARCH_HANDLE_ITEM;

int _UPNP_SERVER_StartSSDP(UPNP_SSDP_HANDLE_ITEM* handle);
int _UPNP_SERVER_StopSSDP(UPNP_SSDP_HANDLE_ITEM* handle);

int _UPNP_SERVER_AddNotifyInfo(UPNP_SSDP_HANDLE_ITEM* handle, const char* uuid, const char* urn, int locationPort, const char* locationUri );
int _UPNP_SERVER_RemoveNotifyInfo(UPNP_SSDP_HANDLE_ITEM* handle, const char* uuid);
int _UPNP_SERVER_ClearNotifyInfo(UPNP_SSDP_HANDLE_ITEM* handle);

int UPNP_SERVER_StartNotifySend(UPNP_SSDP_HANDLE_ITEM* handle);
int UPNP_SERVER_StopNotifySend(UPNP_SSDP_HANDLE_ITEM* handle);

int UPNP_SERVER_SendNotifyAlive(UPNP_SSDP_HANDLE_ITEM* handle, UPNP_SSDP_NOTIFY_DEVICE_INFO* devInfo);
int UPNP_SERVER_SendNotifyByebye(UPNP_SSDP_HANDLE_ITEM* handle, UPNP_SSDP_NOTIFY_DEVICE_INFO* devInfo);

#ifdef _WIN32
static unsigned int WINAPI UPNP_SERVER_SSDPThread(void* param);
static unsigned int WINAPI UPNP_SERVER_SSDPRecvThread(void* param);
static unsigned int WINAPI UPNP_SERVER_NotifySendThread(void* param);
#else
//Linux系
static void* UPNP_SERVER_SSDPThread(void* param);
static void* UPNP_SERVER_SSDPRecvThread(void* param);
static void* UPNP_SERVER_NotifyThread(void* param);
#endif


void UPNP_SERVER_SSDPNotifyMsg(UPNP_SSDP_RECV_PARAM* param, HTTP_REQUEST_HEADER* httpReqHeader, char* src);
void UPNP_SERVER_SSDPMSearchMsg(UPNP_SSDP_RECV_PARAM* param, HTTP_REQUEST_HEADER* httpReqHeader, char* src);

//ハンドルの作成
//戻り値：ハンドル、エラー時NULL
UPNP_SERVER_HANDLE UPNP_SERVER_CreateHandle(
	UPNP_SSDP_CALLBACK ssdpCallBack,	//[IN]SSDPのNotify通知用コールバック
	void* ssdpCallBackParam,			//[IN]SSDPのNotify通知用コールバック呼出時に入れるパラメーター
	UPNP_MSEARCH_REQUEST_CALLBACK msearchCallBack,	//[IN]SSDPのM-SEARCHリクエスト通知用コールバック
	void* msearchCallBackParam			//[IN]SSDPのM-SEARCHリクエスト通知用コールバック呼出時に入れるパラメーター
)
{
	UPNP_SSDP_HANDLE_ITEM* item = NULL;

#ifdef _WIN32
	WSADATA wsaData;
#endif

	item = (UPNP_SSDP_HANDLE_ITEM*)malloc(sizeof(UPNP_SSDP_HANDLE_ITEM));
	if( item != NULL ){
		item->ssdpCallBack = ssdpCallBack;
		item->ssdpCallBackParam = ssdpCallBackParam;
		item->msearchCallBack = msearchCallBack;
		item->msearchCallBackParam = msearchCallBackParam;

		item->lockHandle = MP_LOCK_CreateHandle();
		item->ssdpSrvThread = NULL;
		item->ssdpSrvStopFlag = 0;
		item->notifySendThread = NULL;
		item->notifySendStopFlag = 0;

		item->ssdpSockList = SORT_LIST_CreateHandle(1, SORT_LIST_COMPDEF_strcmp, NULL, UPNP_SSDP_SOCKET_ITEM_Delete);

		item->recvThreadList = SORT_LIST_CreateHandle(1, SORT_LIST_COMPDEF_strcmp, NULL, UPNP_SSDP_RECV_PARAM_Delete);

		item->notifyDeviceList = SORT_LIST_CreateHandle(0, SORT_LIST_COMPDEF_strcmp, NULL, UPNP_SSDP_NOTIFY_DEVICE_INFO_Delete);
#ifdef _WIN32
		ZeroMemory(&wsaData, sizeof(wsaData));
		WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
	}

	return (void*)item;
}

//ハンドルの解放
//解放後handleはNULLに
void UPNP_SERVER_CloseHandle(
	UPNP_SERVER_HANDLE* handle			//[IN]UPNP_SERVER_CreateHandleの戻り値
)
{
	UPNP_SSDP_HANDLE_ITEM* item = NULL;
	if( handle == NULL ){
		return;
	}
	item = (UPNP_SSDP_HANDLE_ITEM*)*handle;
	if(item != NULL ){
		if( item->notifySendThread != NULL ){
			UPNP_SERVER_StopNotifySend(item);
		}
		if( item->ssdpSrvThread != NULL ){
			_UPNP_SERVER_StopSSDP(item);
		}
		MP_LOCK_CloseHandle(&item->lockHandle);

		SORT_LIST_CloseHandle(&item->ssdpSockList);
		SORT_LIST_CloseHandle(&item->recvThreadList);
		SORT_LIST_CloseHandle(&item->notifyDeviceList);
		
#ifdef _WIN32
		WSACleanup();
#endif
		free(item);
	}
	*handle = NULL;
}

//UPnPサーバーの起動
//戻り値：エラーコード
int UPNP_SERVER_Start(
	UPNP_SERVER_HANDLE handle			//[IN]UPNP_SERVER_CreateHandleの戻り値
)
{
	UPNP_SSDP_HANDLE_ITEM* item = NULL;
	int ret = 0;

	if( handle == NULL ){
		return -1;
	}
	item = (UPNP_SSDP_HANDLE_ITEM*)handle;

	MP_LOCK_Lock(item->lockHandle);

	ret = _UPNP_SERVER_StartSSDP(item);
	UPNP_SERVER_StartNotifySend(item);

	MP_LOCK_UnLock(item->lockHandle);

	return ret;
}

//UPnPサーバーの停止
//戻り値：エラーコード
int UPNP_SERVER_Stop(
	UPNP_SERVER_HANDLE handle			//[IN]UPNP_SERVER_CreateHandleの戻り値
)
{
	UPNP_SSDP_HANDLE_ITEM* item = NULL;
	int ret = 0;

	if( handle == NULL ){
		return -1;
	}
	item = (UPNP_SSDP_HANDLE_ITEM*)handle;

	MP_LOCK_Lock(item->lockHandle);

	UPNP_SERVER_StopNotifySend(item);
	ret = _UPNP_SERVER_StopSSDP(item);

	MP_LOCK_UnLock(item->lockHandle);

	return ret;
}

//Notify用のdeviceとserviceを登録する
int UPNP_SERVER_AddNotifyInfo(
	UPNP_SERVER_HANDLE handle,			//[IN]UPNP_SERVER_CreateHandleの戻り値
	const char* uuid,							//[IN]uuid「uuid:」は省く
	const char* urn,							//[IN]urn「urn:」を含む
	int locationPort,					//[IN]Locationのポート
	const char* locationUri					//[IN]Locationの「http://ip:port」より後ろの部分(「/」から始まる)
)
{
	UPNP_SSDP_HANDLE_ITEM* item = NULL;
	int ret = 0;

	if( handle == NULL ){
		return -1;
	}
	item = (UPNP_SSDP_HANDLE_ITEM*)handle;

	MP_LOCK_Lock(item->lockHandle);

	ret = _UPNP_SERVER_AddNotifyInfo(item, uuid, urn, locationPort, locationUri);

	MP_LOCK_UnLock(item->lockHandle);

	return ret;
}

//Notify用のdeviceとserviceの登録を解除する
int UPNP_SERVER_RemoveNotifyInfo(
	UPNP_SERVER_HANDLE handle,			//[IN]UPNP_SERVER_CreateHandleの戻り値
	const char* uuid							//[IN]uuid「uuid:」は省く
)
{
	UPNP_SSDP_HANDLE_ITEM* item = NULL;
	int ret = 0;

	if( handle == NULL ){
		return -1;
	}
	item = (UPNP_SSDP_HANDLE_ITEM*)handle;

	MP_LOCK_Lock(item->lockHandle);

	ret = _UPNP_SERVER_RemoveNotifyInfo(item, uuid);

	MP_LOCK_UnLock(item->lockHandle);

	return ret;
}

//Notify用のdeviceとserviceの全登録を解除する
int UPNP_SERVER_ClearNotifyInfo(
	UPNP_SERVER_HANDLE handle			//[IN]UPNP_SERVER_CreateHandleの戻り値
)
{
	UPNP_SSDP_HANDLE_ITEM* item = NULL;
	int ret = 0;

	if( handle == NULL ){
		return -1;
	}
	item = (UPNP_SSDP_HANDLE_ITEM*)handle;

	MP_LOCK_Lock(item->lockHandle);

	ret = _UPNP_SERVER_ClearNotifyInfo(item);

	MP_LOCK_UnLock(item->lockHandle);

	return ret;
}

int _UPNP_SERVER_StartSSDP(UPNP_SSDP_HANDLE_ITEM* handle)
{
	int ret = 0;
	struct sockaddr_in addr;
	struct ip_mreq_source mreq;
	int opt = 1;
	SORT_LIST_HANDLE nicList = NULL;
	int listCount = 0;
	MP_NIC_INFO* nicInfo = NULL;
	int success = 0;
	int i=0;
	UPNP_SSDP_SOCKET_ITEM* sockItem = NULL;


	if( handle == NULL ){
		return -1;
	}
	if( handle->ssdpSrvThread != NULL ){
		return -2;
	}

	nicList = MP_NIC_GetNICInfo();
	if( nicList == NULL ){
		ret = -3;
		goto Err_End;
	}

	nicInfo = MP_NIC_INFO_New();
	nicInfo->ipv4 = (char*)malloc(strlen("127.0.0.1")+1);
	nicInfo->name = (char*)malloc(strlen("Loop Back")+1);
	nicInfo->macAddress = (char*)malloc(strlen("00-00-00-00-00-00")+1);
#ifdef _WIN32
	strcpy_s(nicInfo->ipv4 , strlen("127.0.0.1")+1, "127.0.0.1");
	strcpy_s(nicInfo->name , strlen("Loop Back")+1, "Loop Back");
	strcpy_s(nicInfo->macAddress, strlen("00-00-00-00-00-00")+1, "00-00-00-00-00-00");
#else
	strcpy(nicInfo->ipv4 , "127.0.0.1");
	strcpy(nicInfo->name , "Loop Back");
	strcpy(nicInfo->macAddress , "00-00-00-00-00-00");
#endif
	memset(nicInfo->macAddressValue, 0, 6);
	SORT_LIST_AddItem(nicList, nicInfo->name, nicInfo);

	listCount = SORT_LIST_GetCount(nicList);
	if( listCount <= 0 ){
		ret = -4;
		goto Err_End;
	}
	//見つかったNIC全てで受信できるようにする
	for( i=0; i<listCount; i++ ){
		nicInfo = (MP_NIC_INFO*)SORT_LIST_GetItemByIndex(nicList, i);

		sockItem = UPNP_SSDP_SOCKET_ITEM_New();
		sockItem->nicInfo = MP_NIC_INFO_Clone(nicInfo);

		//SSDP待ち受けポート（UDP 1900）の作成
		sockItem->ssdpSock = socket(AF_INET, SOCK_DGRAM, 0);
		if( sockItem->ssdpSock < 0 ){
			ret = -5;
			UPNP_SSDP_SOCKET_ITEM_Delete(sockItem);
			goto Err_End;
		}

		memset((char*)&addr, 0, sizeof(struct sockaddr_in)); 
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		addr.sin_port = htons(1900);

		if( setsockopt(sockItem->ssdpSock, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt)) < 0 ){
			ret = -6;
			UPNP_SSDP_SOCKET_ITEM_Delete(sockItem);
			goto Err_End;
		}
		if( bind(sockItem->ssdpSock, (struct sockaddr *)&addr, sizeof(addr)) < 0 ){
			ret = -7;
			UPNP_SSDP_SOCKET_ITEM_Delete(sockItem);
			goto Err_End;
		}

		memset(&mreq, 0, sizeof(mreq));
		mreq.imr_interface.S_un.S_addr = INADDR_ANY;
		mreq.imr_sourceaddr.S_un.S_addr = inet_addr(nicInfo->ipv4);
		mreq.imr_multiaddr.S_un.S_addr = inet_addr("239.255.255.250");

		if(setsockopt(sockItem->ssdpSock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof(mreq)) < 0){
			UPNP_SSDP_SOCKET_ITEM_Delete(sockItem);
		}else{
			SORT_LIST_AddItem(handle->ssdpSockList, sockItem->nicInfo->name, sockItem);
		}
	}
	if( SORT_LIST_GetCount(handle->ssdpSockList) <= 0 ){
		ret =  -8;
		goto Err_End;
	}

	handle->ssdpSrvStopFlag = 0;
	handle->ssdpSrvThread = MP_THREAD_BegineThread(UPNP_SERVER_SSDPThread, handle);

	if( nicList != NULL){
		SORT_LIST_CloseHandle(&nicList);
	}
	return 0;

Err_End:
	if( nicList != NULL){
		SORT_LIST_CloseHandle(&nicList);
	}
	SORT_LIST_Clear(handle->ssdpSockList);

	return ret;
}

int _UPNP_SERVER_StopSSDP(UPNP_SSDP_HANDLE_ITEM* handle)
{
	int ret = 0;

	if( handle == NULL ){
		return -1;
	}

	handle->ssdpSrvStopFlag = 1;
	if( handle->ssdpSrvThread != NULL ){
		MP_THREAD_WaitEndThread(&(handle->ssdpSrvThread));
	}
	SORT_LIST_Clear(handle->ssdpSockList);

	return ret;
}

#ifdef _WIN32
unsigned int WINAPI UPNP_SERVER_SSDPThread(void* param)
#else
//Linux系
void* UPNP_SERVER_SSDPThread(void* param)
#endif
{
	UPNP_SSDP_HANDLE_ITEM* handle = NULL;
	fd_set ready;
	struct timeval to;
	SOCKET sockClient = -1;
	UPNP_SSDP_SOCKET_ITEM* sockItem = NULL;

	UPNP_SSDP_RECV_PARAM* recvParam = NULL;
	int recv_len = 0;
	int threadCount = 0;
	int i=0;
	int j=0;
	int ssdpSockMax = 0;

	handle = (UPNP_SSDP_HANDLE_ITEM*)param;
	if( handle == NULL ){
		return -1;
	}

	while(1){
		if( handle->ssdpSrvStopFlag == 1 ){
			break;
		}

		to.tv_sec = 1;
		to.tv_usec = 0;
		FD_ZERO(&ready);

		ssdpSockMax = 0;

		for( i=0; i<SORT_LIST_GetCount(handle->ssdpSockList); i++ ){
			sockItem = (UPNP_SSDP_SOCKET_ITEM*)SORT_LIST_GetItemByIndex(handle->ssdpSockList, i);
			FD_SET(sockItem->ssdpSock, &ready);
#ifdef _WIN32
#else
			if( ssdpSockMax < sockItem->ssdpSock){
				ssdpSockMax = sockItem->ssdpSock;
			}
#endif
		}

#ifdef _WIN32
		if( select(0, &ready, NULL, NULL, &to ) == SOCKET_ERROR ){
#else
		if( select(ssdpSockMax+1, &ready, NULL, NULL, &to ) == SOCKET_ERROR ){
#endif
			break;
		}
		for( i=0; i<SORT_LIST_GetCount(handle->ssdpSockList); i++ ){
			sockItem = (UPNP_SSDP_SOCKET_ITEM*)SORT_LIST_GetItemByIndex(handle->ssdpSockList, i);

			if ( FD_ISSET(sockItem->ssdpSock, &ready) ){
				recvParam = UPNP_SSDP_RECV_PARAM_New();
				if( recvParam == NULL ){
					break;
				}
				recvParam->nicInfo = MP_NIC_INFO_Clone(sockItem->nicInfo);
			
				recv_len = recvfrom(sockItem->ssdpSock, recvParam->recvData, UPNP_SSDP_RECV_BUFF_SIZE-1, 0, (sockaddr*)&recvParam->client, &recvParam->addr_len);
				if( recv_len < 0 ){
					UPNP_SSDP_RECV_PARAM_Delete(recvParam);
					break;
				}else if( recv_len == 0 ){
					UPNP_SSDP_RECV_PARAM_Delete(recvParam);
					continue;
				}else{
#ifdef _WIN32
					sprintf_s(recvParam->key, sizeof(recvParam->key), "%s:%u", inet_ntoa(recvParam->client.sin_addr), ntohs(recvParam->client.sin_port));
#else
					sprintf(recvParam->key, "%s:%u", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
#endif
					recvParam->ssdpHandle = handle;
					recvParam->threadHandle = MP_THREAD_BegineThread(UPNP_SERVER_SSDPRecvThread, recvParam);

					SORT_LIST_AddItem(handle->recvThreadList, (void*)recvParam->key, (void*)recvParam);
				}
			}else{
				threadCount = SORT_LIST_GetCount(handle->recvThreadList);
				for( j=threadCount-1; j>=0; j-- ){
					recvParam = (UPNP_SSDP_RECV_PARAM*)SORT_LIST_GetItemByIndex(handle->recvThreadList, j);
					if( recvParam->endFlag == 1 ){
						MP_THREAD_WaitEndThread(&recvParam->threadHandle);
						SORT_LIST_DeleteItemByIndex(handle->recvThreadList, j);
					}
				}
			}
		}
	}

	SORT_LIST_Clear(handle->recvThreadList);

	return 0;
}

#ifdef _WIN32
unsigned int WINAPI UPNP_SERVER_SSDPRecvThread(void* param)
#else
//Linux系
void* UPNP_SERVER_SSDPRecvThread(void* param)
#endif
{
	HTTP_REQUEST_HEADER* httpReqHeader = NULL;
	UPNP_SSDP_RECV_PARAM* info = NULL;

	info = (UPNP_SSDP_RECV_PARAM*)param;
	if( info == NULL ){
		return -1;
	}

	httpReqHeader = HTTP_REQUEST_HEADER_New();
#ifdef _WIN32
	//OutputDebugStringA(info->recvData);
#else
	printf(info->recvData);
#endif

	if( MP_HTTP_RequestHeaderParse(info->recvData, httpReqHeader) < 0 ){
		HTTP_REQUEST_HEADER_Delete(&httpReqHeader);
		return -1;
	}

	if( MP_STR_CompNoCase(httpReqHeader->method, "NOTIFY") == 0 ){
		UPNP_SERVER_SSDPNotifyMsg((UPNP_SSDP_RECV_PARAM*)info, httpReqHeader, info->recvData);
	}else if( MP_STR_CompNoCase(httpReqHeader->method, "M-SEARCH") == 0 ){
		UPNP_SERVER_SSDPMSearchMsg((UPNP_SSDP_RECV_PARAM*)info, httpReqHeader, info->recvData);
	}else{
		goto END_THREAD;
	}


END_THREAD:
	HTTP_REQUEST_HEADER_Delete(httpReqHeader);
	info->endFlag = 1;

	return 0;
}

void UPNP_SERVER_SSDPNotifyMsg(UPNP_SSDP_RECV_PARAM* param, HTTP_REQUEST_HEADER* httpReqHeader, char* src)
{
	UPNP_SSDP_RECV_PARAM* info = NULL;
	UPNP_SSDP_HANDLE_ITEM* handle = NULL;
	UPNP_SSDP_NOTIFY_INFO* notifyInfo = NULL;
	SSDP_NOTIFY_STATUS notifyType = SSDP_unknown;

	info = (UPNP_SSDP_RECV_PARAM*)param;
	if( info == NULL ){
		return ;
	}
	handle = (UPNP_SSDP_HANDLE_ITEM*)info->ssdpHandle;
	if( handle == NULL ){
		return ;
	}

	notifyInfo = UPNP_SSDP_NOTIFY_INFO_New();
	if( notifyInfo == NULL ){
		return ;
	}

	if( handle->ssdpCallBack != NULL ){
		handle->ssdpCallBack(notifyType, (void*)notifyInfo, handle->ssdpCallBackParam);
	}
	UPNP_SSDP_NOTIFY_INFO_Delete(notifyInfo);
}

void UPNP_SERVER_SSDPMSearchMsg(UPNP_SSDP_RECV_PARAM* param, HTTP_REQUEST_HEADER* httpReqHeader, char* src)
{
	UPNP_SSDP_RECV_PARAM* info = NULL;
	UPNP_SSDP_HANDLE_ITEM* handle = NULL;
	UPNP_MSEARCH_REQUEST_INFO* requestParam = NULL;
	SORT_LIST_HANDLE resDeviceList = NULL;
	HTTP_HEADER_FIELD_INFO* headerItem = NULL;
	UPNP_MSEARCH_RES_DEV_INFO* devInfo = NULL;
	int length = 0;
	int i=0;
	char nowTime[30] = "";
	char resMsg[1024] = "";
	char buff[256] = "";
	SOCKET sendSock;
	char ua[128] = "";

	info = (UPNP_SSDP_RECV_PARAM*)param;
	if( info == NULL ){
		return ;
	}
	handle = (UPNP_SSDP_HANDLE_ITEM*)info->ssdpHandle;
	if( handle == NULL ){
		return ;
	}
	if( handle->msearchCallBack == NULL ){
		return ;
	}
	requestParam = UPNP_MSEARCH_REQUEST_INFO_New();
	resDeviceList = SORT_LIST_CreateHandle(1, SORT_LIST_COMPDEF_stricmp, NULL, UPNP_MSEARCH_RES_DEV_INFO_Delete);

	//リクエスト全体
	length = (int)strlen(param->recvData);
	requestParam->res = (char*)malloc(length+1);
#ifdef _WIN32
	strcpy_s(requestParam->res, length+1, param->recvData);
#else
	strcpy(requestParam->res, param->recvData);
#endif
	//ヘッダをコピー
	headerItem = (HTTP_HEADER_FIELD_INFO*)SORT_LIST_FindItem(httpReqHeader->headerList, (void*)"HOST" );
	if( headerItem != NULL ){
		length = (int)strlen(headerItem->value);
		requestParam->host = (char*)malloc(length+1);
#ifdef _WIN32
		strcpy_s(requestParam->host, length+1, headerItem->value);
#else
		strcpy(requestParam->host, headerItem->value);
#endif
	}
	headerItem = (HTTP_HEADER_FIELD_INFO*)SORT_LIST_FindItem(httpReqHeader->headerList, (void*)"MAN" );
	if( headerItem != NULL ){
		length = (int)strlen(headerItem->value);
		requestParam->man = (char*)malloc(length+1);
#ifdef _WIN32
		strcpy_s(requestParam->man, length+1, headerItem->value);
#else
		strcpy(requestParam->man, headerItem->value);
#endif
	}
	headerItem = (HTTP_HEADER_FIELD_INFO*)SORT_LIST_FindItem(httpReqHeader->headerList, (void*)"MX" );
	if( headerItem != NULL ){
		length = (int)strlen(headerItem->value);
		requestParam->mx = (char*)malloc(length+1);
#ifdef _WIN32
		strcpy_s(requestParam->mx, length+1, headerItem->value);
#else
		strcpy(requestParam->mx, headerItem->value);
#endif
	}
	headerItem = (HTTP_HEADER_FIELD_INFO*)SORT_LIST_FindItem(httpReqHeader->headerList, (void*)"ST" );
	if( headerItem != NULL ){
		length = (int)strlen(headerItem->value);
		requestParam->st = (char*)malloc(length+1);
#ifdef _WIN32
		strcpy_s(requestParam->st, length+1, headerItem->value);
#else
		strcpy(requestParam->st, headerItem->value);
#endif
	}
	headerItem = (HTTP_HEADER_FIELD_INFO*)SORT_LIST_FindItem(httpReqHeader->headerList, (void*)"USER-AGENT" );
	if( headerItem != NULL ){
		length = (int)strlen(headerItem->value);
		requestParam->userAgent = (char*)malloc(length+1);
#ifdef _WIN32
		strcpy_s(requestParam->userAgent, length+1, headerItem->value);
#else
		strcpy(requestParam->userAgent, headerItem->value);
#endif
	}
	//通知
	handle->msearchCallBack(requestParam, handle->msearchCallBackParam, resDeviceList);

	//MP_Sleep(500);

	MP_TIME_GetNowTime_RFC1123(nowTime, sizeof(nowTime));
	//デバイスごとに送信
	for(i=0; i<SORT_LIST_GetCount(resDeviceList); i++ ){
		devInfo = (UPNP_MSEARCH_RES_DEV_INFO*)SORT_LIST_GetItemByIndex(resDeviceList, i);

		if( param->nicInfo->ipv4 == NULL || devInfo->uri == NULL ){
			continue;
		}
#ifdef _WIN32
		strcpy_s(resMsg, sizeof(resMsg), "HTTP/1.1 200 OK\r\n");

		sprintf_s(buff, sizeof(buff), "CACHE-CONTROL: max-age=%d\r\n", devInfo->max_age);
		strcat_s(resMsg, sizeof(resMsg), buff);

//		sprintf_s(buff, sizeof(buff), "DATE: %s\r\n", nowTime);
//		strcat_s(resMsg, sizeof(resMsg), buff);

		sprintf_s(buff, sizeof(buff), "LOCATION: http://%s:%d%s\r\n", param->nicInfo->ipv4, devInfo->port, devInfo->uri);
		strcat_s(resMsg, sizeof(resMsg), buff);

		sprintf_s(buff, sizeof(buff), "ST: %s\r\n", requestParam->st);
		strcat_s(resMsg, sizeof(resMsg), buff);

		if(devInfo->usn != NULL ){
			sprintf_s(buff, sizeof(buff), "USN: %s\r\n", devInfo->usn);
			strcat_s(resMsg, sizeof(resMsg), buff);
		}

		UPNP_UTIL_GetUserAgent(ua, sizeof(ua));
		sprintf_s(buff, sizeof(buff), "SERVER: %s\r\n", ua);
		strcat_s(resMsg, sizeof(resMsg), buff);

		strcat_s(resMsg, sizeof(resMsg), "EXT:\r\n");

		strcat_s(resMsg, sizeof(resMsg), "\r\n");
#else
		strcpy(resMsg, "HTTP/1.1 200 OK\r\n");

		sprintf(buff, "CACHE-CONTROL: max-age=%d\r\n", devInfo->max_age);
		strcat(resMsg, buff);

//		sprintf(buff, "DATE: %s\r\n", nowTime);
//		strcat(resMsg, buff);

		sprintf(buff, "LOCATION: http://%s:%d%s\r\n", param->nicInfo->ipv4, devInfo->port, devInfo->uri);
		strcat(resMsg, buff);

		sprintf(buff, "ST: %s\r\n", requestParam->st);
		strcat(resMsg, buff);

		if(devInfo->usn != NULL ){
			sprintf(buff, "USN: %s\r\n", devInfo->usn);
			strcat(resMsg, buff);
		}

		UPNP_UTIL_GetUserAgent(ua, sizeof(ua));
		sprintf_s(buff, sizeof(buff), "SERVER: %s\r\n", ua);
		strcat_s(resMsg, sizeof(resMsg), buff);
		strcat(resMsg, "EXT:\r\n");

		strcat(resMsg,  "\r\n");
#endif
		//レスポンス送信
		MP_Sleep(10);
		sendSock = socket(AF_INET, SOCK_DGRAM, 0);
		//OutputDebugStringA(resMsg);
		sendto(sendSock, resMsg, (int)strlen(resMsg), 0, (struct sockaddr *)&param->client, sizeof(param->client));
		shutdown(sendSock, SD_BOTH);
		closesocket(sendSock);
	}

	SORT_LIST_CloseHandle(&resDeviceList);
	UPNP_MSEARCH_REQUEST_INFO_Delete(requestParam);
}

int _UPNP_SERVER_AddNotifyInfo(UPNP_SSDP_HANDLE_ITEM* handle, const char* uuid, const char* urn, int locationPort, const char* locationUri )
{
	int ret = 0;
	UPNP_SSDP_NOTIFY_DEVICE_INFO* devInfo = NULL;
	char* urnFind = NULL;

	devInfo = (UPNP_SSDP_NOTIFY_DEVICE_INFO*)SORT_LIST_FindItem(handle->notifyDeviceList, (void*)uuid);
	if( devInfo != NULL ){
		urnFind = (char*)SORT_LIST_FindItem(devInfo->urnList, (void*)urn);
		if( urnFind != NULL ){
			//登録済み
			return 2;
		}else{
			urnFind = (char*)malloc(strlen(urn)+1);
#ifdef _WIN32
			strcpy_s(urnFind, strlen(urn)+1, urn);
#else
			strcpy(urnFind, urn);
#endif
			SORT_LIST_AddItem(devInfo->urnList, urnFind, urnFind);
		}
	}else{
		//新規登録
		devInfo = UPNP_SSDP_NOTIFY_DEVICE_INFO_New();
		devInfo->uuid = (char*)malloc(strlen(uuid)+1);
		devInfo->locationUri = (char*)malloc(strlen(locationUri)+1);
		devInfo->locationPort = locationPort;
		urnFind = (char*)malloc(strlen(urn)+1);
#ifdef _WIN32
		strcpy_s(devInfo->uuid, strlen(uuid)+1, uuid);
		strcpy_s(devInfo->locationUri, strlen(locationUri)+1, locationUri);
		strcpy_s(urnFind, strlen(urn)+1, urn);
#else
		strcpy(devInfo->uuid, uuid);
		strcpy(devInfo->locationUri, locationUri);
		strcpy(urnFind, urn);
#endif
		SORT_LIST_AddItem(devInfo->urnList, urnFind, urnFind);

		SORT_LIST_AddItem(handle->notifyDeviceList, devInfo->uuid, devInfo);
	}

	UPNP_SERVER_SendNotifyAlive(handle, devInfo);
	ret = 1;

	return ret;
}

int _UPNP_SERVER_RemoveNotifyInfo(UPNP_SSDP_HANDLE_ITEM* handle, const char* uuid )
{
	int ret = 0;
	UPNP_SSDP_NOTIFY_DEVICE_INFO* devInfo = NULL;
	int i=0;
	int devCount = 0;

	devCount = SORT_LIST_GetCount(handle->notifyDeviceList);
	for( i=0; i<devCount; i++ ){
		devInfo = (UPNP_SSDP_NOTIFY_DEVICE_INFO*)SORT_LIST_GetItemByIndex(handle->notifyDeviceList, i);
		if( strcmp(devInfo->uuid, uuid) == 0 ){
			UPNP_SERVER_SendNotifyByebye(handle, devInfo);

			SORT_LIST_DeleteItemByIndex(handle->notifyDeviceList, i);
			ret = 1;
			break;
		}
	}

	return ret;
}

int _UPNP_SERVER_ClearNotifyInfo( UPNP_SSDP_HANDLE_ITEM* handle	)
{
	int ret = 0;
	UPNP_SSDP_NOTIFY_DEVICE_INFO* devInfo = NULL;
	int i=0;
	int devCount = 0;

	devCount = SORT_LIST_GetCount(handle->notifyDeviceList);
	for( i=0; i<devCount; i++ ){
		devInfo = (UPNP_SSDP_NOTIFY_DEVICE_INFO*)SORT_LIST_GetItemByIndex(handle->notifyDeviceList, i);
		UPNP_SERVER_SendNotifyByebye(handle, devInfo);
	}
	SORT_LIST_Clear(handle->notifyDeviceList);
	ret = 1;

	return ret;
}

int UPNP_SERVER_StartNotifySend(UPNP_SSDP_HANDLE_ITEM* handle)
{
	if( handle == NULL ){
		return -1;
	}
	if( handle->notifySendThread == NULL ){
		handle->notifySendStopFlag = 0;
		handle->notifySendThread = MP_THREAD_BegineThread(UPNP_SERVER_NotifySendThread, handle);
	}

	return 1;
}

int UPNP_SERVER_StopNotifySend(UPNP_SSDP_HANDLE_ITEM* handle)
{
	int ret = 0;
	int devCount = 0;
	int i=0;
	int j=0;
	UPNP_SSDP_NOTIFY_DEVICE_INFO* devInfo = NULL;

	if( handle == NULL ){
		return -1;
	}

	handle->notifySendStopFlag = 1;
	if( handle->notifySendThread != NULL ){
		MP_THREAD_WaitEndThread(&(handle->notifySendThread));
	}

	devCount = SORT_LIST_GetCount(handle->notifyDeviceList);
	for( i=0; i<devCount; i++ ){
		devInfo = (UPNP_SSDP_NOTIFY_DEVICE_INFO*)SORT_LIST_GetItemByIndex(handle->notifyDeviceList, i);
		UPNP_SERVER_SendNotifyByebye(handle, devInfo);
	}

	SORT_LIST_Clear(handle->notifyDeviceList);

	return ret;
}

int UPNP_SERVER_SendNotifyAlive(UPNP_SSDP_HANDLE_ITEM* handle, UPNP_SSDP_NOTIFY_DEVICE_INFO* devInfo)
{
	int ret = 0;
	int urnCount = 0;
	char* urn = NULL;
	int i=0;
	int j=0;
	char sendMsg[2048] = "";
	SORT_LIST_HANDLE nicList = NULL;
	int nicCount = 0;
	MP_NIC_INFO* nicInfo = NULL;

	SOCKET sock;
	struct sockaddr_in addr;
	struct sockaddr_in send_addr;
	int opt = 1;
	unsigned long ipAdrr = 0;
	int ttl = 3;

	char sendMsgFormat[] = "NOTIFY * HTTP/1.1\r\nHOST: 239.255.255.250:1900\r\nCACHE-CONTROL: max-age = 1800\r\nLOCATION: http://%s:%d%s\r\nNT: %s\r\nNTS: ssdp:alive\r\nSERVER: %s\r\nUSN: %s\r\n\r\n";
	char nt[512] = "";
	char usn[512] = "";
	char ua[128] = "";

	if( handle == NULL || devInfo == NULL){
		ret = -1;
		goto Err_End;
	}

	nicList = MP_NIC_GetNICInfo();
	if( nicList == NULL ){
		ret = -3;
		goto Err_End;
	}

	UPNP_UTIL_GetUserAgent(ua, sizeof(ua));

	nicInfo = MP_NIC_INFO_New();
	nicInfo->ipv4 = (char*)malloc(strlen("127.0.0.1")+1);
	nicInfo->name = (char*)malloc(strlen("Loop Back")+1);
	nicInfo->macAddress = (char*)malloc(strlen("00-00-00-00-00-00")+1);
#ifdef _WIN32
	strcpy_s(nicInfo->ipv4 , strlen("127.0.0.1")+1, "127.0.0.1");
	strcpy_s(nicInfo->name , strlen("Loop Back")+1, "Loop Back");
	strcpy_s(nicInfo->macAddress, strlen("00-00-00-00-00-00")+1, "00-00-00-00-00-00");
#else
	strcpy(nicInfo->ipv4 , "127.0.0.1");
	strcpy(nicInfo->name , "Loop Back");
	strcpy(nicInfo->macAddress , "00-00-00-00-00-00");
#endif
	memset(nicInfo->macAddressValue, 0, 6);
	SORT_LIST_AddItem(nicList, nicInfo->name, nicInfo);

	nicCount = SORT_LIST_GetCount(nicList);
	if( nicCount <= 0 ){
		ret = -4;
		goto Err_End;
	}

	memset((char*)&send_addr, 0, sizeof(struct sockaddr_in)); 
	send_addr.sin_family = AF_INET;
	send_addr.sin_addr.s_addr = inet_addr("239.255.255.250");
	send_addr.sin_port = htons(1900);

	//見つかったNIC全てで送信
	for( i=0; i<nicCount; i++ ){
		nicInfo = (MP_NIC_INFO*)SORT_LIST_GetItemByIndex(nicList, i);

		ipAdrr = inet_addr(nicInfo->ipv4);

		memset((char*)&addr, 0, sizeof(struct sockaddr_in)); 
		addr.sin_family = AF_INET;
		addr.sin_port = 0;
		addr.sin_addr.S_un.S_addr = ipAdrr;

		sock = socket(AF_INET, SOCK_DGRAM, 0);
		if( sock < 0 ){
			continue;
		}

		if( setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt)) < 0 ){
			closesocket(sock);
			continue;
		}

		if(setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, (char *)&ipAdrr, sizeof(ipAdrr)) < 0){
			closesocket(sock);
			continue;
		}
		if(setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&ttl, sizeof(ttl)) < 0){
			closesocket(sock);
			continue;
		}

		//upnp:rootdevice
		memset(sendMsg, 0, 2048);
#ifdef _WIN32
		sprintf_s(usn, sizeof(usn), "uuid:%s::upnp:rootdevice", devInfo->uuid);
		sprintf_s(sendMsg, sizeof(sendMsg), sendMsgFormat,
			nicInfo->ipv4,
			devInfo->locationPort,
			devInfo->locationUri,
			"upnp:rootdevice",
			ua,
			usn
			);
#else
		sprintf(usn, "uuid:%s::upnp:rootdevice", devInfo->uuid);
		sprintf(sendMsg, sendMsgFormat,
			nicInfo->ipv4,
			devInfo->locationPort,
			devInfo->locationUri,
			"upnp:rootdevice",
			ua,
			usn
			);
#endif
		if( sendto(sock, sendMsg, (int)strlen(sendMsg), 0, (struct sockaddr *)&send_addr, sizeof(send_addr)) < 0 ){
			closesocket(sock);
			continue;
		}

		//uuid:device-UUID
		memset(sendMsg, 0, 2048);
#ifdef _WIN32
		sprintf_s(nt, sizeof(usn), "uuid:%s", devInfo->uuid);
		sprintf_s(usn, sizeof(usn), "uuid:%s", devInfo->uuid);
		sprintf_s(sendMsg, sizeof(sendMsg), sendMsgFormat,
			nicInfo->ipv4,
			devInfo->locationPort,
			devInfo->locationUri,
			nt,
			ua,
			usn
			);
#else
		sprintf(nt, "uuid:%s", devInfo->uuid);
		sprintf(usn, "uuid:%s", devInfo->uuid);
		sprintf(sendMsg, sendMsgFormat,
			nicInfo->ipv4,
			devInfo->locationPort,
			devInfo->locationUri,
			nt,
			ua,
			usn
			);
#endif
		if( sendto(sock, sendMsg, (int)strlen(sendMsg), 0, (struct sockaddr *)&send_addr, sizeof(send_addr)) < 0 ){
			closesocket(sock);
			continue;
		}

		//urn:schemas-upnp-org:device:deviceType:ver
		//urn:schemas-upnp-org:service:serviceType:ver
		urnCount = SORT_LIST_GetCount(devInfo->urnList);
		for( j=0; j<urnCount; j++ ){
			urn = (char*)SORT_LIST_GetItemByIndex(devInfo->urnList, j);
			memset(sendMsg, 0, 2048);
#ifdef _WIN32
			sprintf_s(nt, sizeof(usn), "%s", urn);
			sprintf_s(usn, sizeof(usn), "uuid:%s::%s", devInfo->uuid, urn);
			sprintf_s(sendMsg, sizeof(sendMsg), sendMsgFormat,
				nicInfo->ipv4,
				devInfo->locationPort,
				devInfo->locationUri,
				nt,
				ua,
				usn
				);
#else
			sprintf(nt, "%s", urn);
			sprintf(usn, "uuid:%s::%s", devInfo->uuid, urn);
			sprintf(sendMsg, sendMsgFormat,
				nicInfo->ipv4,
				devInfo->locationPort,
				devInfo->locationUri,
				nt,
				ua,
				usn
				);
#endif
			if( sendto(sock, sendMsg, (int)strlen(sendMsg), 0, (struct sockaddr *)&send_addr, sizeof(send_addr)) < 0 ){
				break;
			}
		}
		closesocket(sock);
	}

Err_End:
	if( nicList != NULL){
		SORT_LIST_CloseHandle(&nicList);
	}
	return ret;
}

int UPNP_SERVER_SendNotifyByebye(UPNP_SSDP_HANDLE_ITEM* handle, UPNP_SSDP_NOTIFY_DEVICE_INFO* devInfo)
{
	int ret = 0;
	int urnCount = 0;
	char* urn = NULL;
	int i=0;
	int j=0;
	char sendMsg[2048] = "";
	SORT_LIST_HANDLE nicList = NULL;
	int nicCount = 0;
	MP_NIC_INFO* nicInfo = NULL;

	SOCKET sock;
	struct sockaddr_in addr;
	struct sockaddr_in send_addr;
	int opt = 1;
	unsigned long ipAdrr = 0;
	int ttl = 3;

	char sendMsgFormat[] = "NOTIFY * HTTP/1.1\r\nHOST: 239.255.255.250:1900\r\nNT: %s\r\nNTS: ssdp:byebye\r\nUSN: %s\r\n\r\n";
	char nt[512] = "";
	char usn[512] = "";

	if( handle == NULL || devInfo == NULL){
		ret = -1;
		goto Err_End;
	}

	nicList = MP_NIC_GetNICInfo();
	if( nicList == NULL ){
		ret = -3;
		goto Err_End;
	}

	nicInfo = MP_NIC_INFO_New();
	nicInfo->ipv4 = (char*)malloc(strlen("127.0.0.1")+1);
	nicInfo->name = (char*)malloc(strlen("Loop Back")+1);
	nicInfo->macAddress = (char*)malloc(strlen("00-00-00-00-00-00")+1);
#ifdef _WIN32
	strcpy_s(nicInfo->ipv4 , strlen("127.0.0.1")+1, "127.0.0.1");
	strcpy_s(nicInfo->name , strlen("Loop Back")+1, "Loop Back");
	strcpy_s(nicInfo->macAddress, strlen("00-00-00-00-00-00")+1, "00-00-00-00-00-00");
#else
	strcpy(nicInfo->ipv4 , "127.0.0.1");
	strcpy(nicInfo->name , "Loop Back");
	strcpy(nicInfo->macAddress , "00-00-00-00-00-00");
#endif
	memset(nicInfo->macAddressValue, 0, 6);
	SORT_LIST_AddItem(nicList, nicInfo->name, nicInfo);

	nicCount = SORT_LIST_GetCount(nicList);
	if( nicCount <= 0 ){
		ret = -4;
		goto Err_End;
	}

	memset((char*)&send_addr, 0, sizeof(struct sockaddr_in)); 
	send_addr.sin_family = AF_INET;
	send_addr.sin_addr.s_addr = inet_addr("239.255.255.250");
	send_addr.sin_port = htons(1900);

	//見つかったNIC全てで送信
	for( i=0; i<nicCount; i++ ){
		nicInfo = (MP_NIC_INFO*)SORT_LIST_GetItemByIndex(nicList, i);

		ipAdrr = inet_addr(nicInfo->ipv4);

		memset((char*)&addr, 0, sizeof(struct sockaddr_in)); 
		addr.sin_family = AF_INET;
		addr.sin_port = 0;
		addr.sin_addr.S_un.S_addr = ipAdrr;

		sock = socket(AF_INET, SOCK_DGRAM, 0);
		if( sock < 0 ){
			continue;
		}

		if( setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt)) < 0 ){
			closesocket(sock);
			continue;
		}

		if(setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, (char *)&ipAdrr, sizeof(ipAdrr)) < 0){
			closesocket(sock);
			continue;
		}
		if(setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&ttl, sizeof(ttl)) < 0){
			closesocket(sock);
			continue;
		}

		//upnp:rootdevice
		memset(sendMsg, 0, 2048);
#ifdef _WIN32
		sprintf_s(usn, sizeof(usn), "uuid:%s::upnp:rootdevice", devInfo->uuid);
		sprintf_s(sendMsg, sizeof(sendMsg), sendMsgFormat,
			"upnp:rootdevice",
			usn
			);
#else
		sprintf(usn, "uuid:%s::upnp:rootdevice", devInfo->uuid);
		sprintf(sendMsg, sendMsgFormat,
			"upnp:rootdevice",
			usn
			);
#endif
		if( sendto(sock, sendMsg, (int)strlen(sendMsg), 0, (struct sockaddr *)&send_addr, sizeof(send_addr)) < 0 ){
			closesocket(sock);
			continue;
		}

		//uuid:device-UUID
		memset(sendMsg, 0, 2048);
#ifdef _WIN32
		sprintf_s(nt, sizeof(usn), "uuid:%s", devInfo->uuid);
		sprintf_s(usn, sizeof(usn), "uuid:%s", devInfo->uuid);
		sprintf_s(sendMsg, sizeof(sendMsg), sendMsgFormat,
			nt,
			usn
			);
#else
		sprintf(nt, "uuid:%s", devInfo->uuid);
		sprintf(usn, "uuid:%s", devInfo->uuid);
		sprintf(sendMsg, sendMsgFormat,
			nt,
			usn
			);
#endif
		if( sendto(sock, sendMsg, (int)strlen(sendMsg), 0, (struct sockaddr *)&send_addr, sizeof(send_addr)) < 0 ){
			closesocket(sock);
			continue;
		}

		//urn:schemas-upnp-org:device:deviceType:ver
		//urn:schemas-upnp-org:service:serviceType:ver
		urnCount = SORT_LIST_GetCount(devInfo->urnList);
		for( j=0; j<urnCount; j++ ){
			urn = (char*)SORT_LIST_GetItemByIndex(devInfo->urnList, j);
			memset(sendMsg, 0, 2048);
#ifdef _WIN32
			sprintf_s(nt, sizeof(usn), "%s", urn);
			sprintf_s(usn, sizeof(usn), "uuid:%s::%s", devInfo->uuid, urn);
			sprintf_s(sendMsg, sizeof(sendMsg), sendMsgFormat,
				nt,
				usn
				);
#else
			sprintf(nt, "%s", urn);
			sprintf(usn, "uuid:%s::%s", devInfo->uuid, urn);
			sprintf(sendMsg, sendMsgFormat,
				nt,
				usn
				);
#endif
			if( sendto(sock, sendMsg, (int)strlen(sendMsg), 0, (struct sockaddr *)&send_addr, sizeof(send_addr)) < 0 ){
				break;
			}
		}
		closesocket(sock);
	}

Err_End:
	if( nicList != NULL){
		SORT_LIST_CloseHandle(&nicList);
	}
	return ret;
}

#ifdef _WIN32
unsigned int WINAPI UPNP_SERVER_NotifySendThread(void* param)
#else
//Linux系
void* UPNP_SERVER_NotifySendThread(void* param)
#endif
{
	UPNP_SSDP_HANDLE_ITEM* handle = NULL;
	UPNP_SSDP_NOTIFY_DEVICE_INFO* devInfo = NULL;
	int i=0;
	int devCount = 0;
	int timeCount = 0;
	handle = (UPNP_SSDP_HANDLE_ITEM*)param;
	if( handle == NULL ){
		return -1;
	}

	while(1){
		if( handle->notifySendStopFlag == 1 ){
			break;
		}

		if( timeCount > 1000 ){
			MP_LOCK_Lock(handle->lockHandle);

			devCount = SORT_LIST_GetCount(handle->notifyDeviceList);
			for( i=0; i<devCount; i++ ){
				devInfo = (UPNP_SSDP_NOTIFY_DEVICE_INFO*)SORT_LIST_GetItemByIndex(handle->notifyDeviceList, i);
				UPNP_SERVER_SendNotifyAlive(handle, devInfo);
			}

			MP_LOCK_UnLock(handle->lockHandle);

			timeCount = 0;
		}else{
			timeCount++;
			MP_Sleep(1000);
		}
	}

	return 0;
}

UPNP_MSEARCH_HANDLE_ITEM* UPNP_MSEARCH_HANDLE_ITEM_Create(UPNP_SERVER_HANDLE handle)
{
	UPNP_MSEARCH_HANDLE_ITEM* item = NULL;
	UPNP_SSDP_HANDLE_ITEM* srvHandle = NULL;
	int ret = 0;

	if( handle == NULL ){
		return NULL;
	}
	srvHandle = (UPNP_SSDP_HANDLE_ITEM*)handle;

	item = (UPNP_MSEARCH_HANDLE_ITEM*)malloc(sizeof(UPNP_MSEARCH_HANDLE_ITEM));
	if( item != NULL ){
		item->msearchCallBack = srvHandle->msearchCallBack;
		item->msearchCallBackParam = srvHandle->msearchCallBackParam;

		item->lockHandle = MP_LOCK_CreateHandle();
		item->msearchThread = NULL;
		item->msearchStopFlag = 0;

		item->resultList = SORT_LIST_CreateHandle(0, SORT_LIST_COMPDEF_strcmp, NULL, UPNP_SSDP_SOCKET_ITEM_Delete);
	}

	return item;
}

void UPNP_MSEARCH_HANDLE_ITEM_Close(UPNP_MSEARCH_HANDLE_ITEM* item)
{
	if( item == NULL ){
		return;
	}

	if( item->msearchThread != NULL ){
		item->msearchStopFlag = 1;
		MP_THREAD_WaitEndThread(&(item->msearchThread));
	}
	MP_LOCK_CloseHandle(&item->lockHandle);

	SORT_LIST_CloseHandle(&item->resultList);

	free(item);
}

//M-Search 機器の検索を実行（同期）
//戻り値：ハンドル、エラー時NULL
UPNP_MSEARCH_HANDLE UPNP_MSearch_Exec(
	UPNP_SERVER_HANDLE handle,			//[IN]UPNP_SERVER_CreateHandleの戻り値
	const char* searchTarget,			//[IN]実行時のSTの値（検索キー）
	short mx,							//[IN]実行時のMXの値(単位：秒)（タイムアウト）
	SORT_LIST_HANDLE* resultList		//[OUT]検索結果のリスト、キー：char* のUUID、要素：UPNP_MSEARCH_INFO*　UPNP_MSearch_CloseHandleで開放するされる
);

//M-Search 機器の検索を実行（非同期）
//検索結果はコールバックでの通知のみ
//戻り値：ハンドル、エラー時NULL
UPNP_MSEARCH_HANDLE UPNP_MSearch_ExecAsync(
	UPNP_SERVER_HANDLE handle,			//[IN]UPNP_SERVER_CreateHandleの戻り値
	const char* searchTarget,			//[IN]実行時のSTの値（検索キー）
	short mx							//[IN]実行時のMXの値(単位：秒)（タイムアウト）
);

//M-Searchハンドルの解放
//非同期実行時は動作のキャンセルもされる
//解放後handleはNULLに
void UPNP_MSearch_CloseHandle(
	UPNP_MSEARCH_HANDLE* handle			//[IN]UPNP_MSearch_Exec or UPNP_MSearch_ExecAsyncの戻り値
)
{
	UPNP_MSEARCH_HANDLE_ITEM* item = NULL;
	int ret = 0;

	if( handle == NULL ){
		return;
	}
	item = (UPNP_MSEARCH_HANDLE_ITEM*)handle;

	UPNP_MSEARCH_HANDLE_ITEM_Close(item);

	*handle = NULL;

	return ;
}

