#ifndef __BON_CTRL_DEF_H__
#define __BON_CTRL_DEF_H__

#define MUTEX_UDP_PORT_NAME			TEXT("Global\\EpgDataCap_Bon_UDP_PORT_")
#define MUTEX_TCP_PORT_NAME			TEXT("Global\\EpgDataCap_Bon_TCP_PORT_")
#define CHSET_SAVE_EVENT_WAIT		 _T("Global\\EpgTimer_ChSet")


//ネットワーク送信用設定
typedef struct _NW_SEND_INFO{
	wstring ipString;
	DWORD ip;
	DWORD port;
	BOOL broadcastFlag;
}NW_SEND_INFO;

//EPG取得用サービス情報
typedef struct _EPGCAP_SERVICE_INFO{
	WORD ONID;
	WORD TSID;
	WORD SID;
}EPGCAP_SERVICE_INFO;


#endif
