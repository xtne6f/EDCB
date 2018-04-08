#ifndef INCLUDE_BON_CTRL_DEF_H
#define INCLUDE_BON_CTRL_DEF_H

#define MUTEX_UDP_PORT_NAME			L"Global\\EpgDataCap_Bon_UDP_PORT_"
#define MUTEX_TCP_PORT_NAME			L"Global\\EpgDataCap_Bon_TCP_PORT_"
#define CHSET_SAVE_EVENT_WAIT		L"Global\\EpgTimer_ChSet"


//ネットワーク送信用設定
typedef struct {
	wstring ipString;
	DWORD port;
	BOOL broadcastFlag;
}NW_SEND_INFO;

//EPG取得用サービス情報
typedef struct {
	WORD ONID;
	WORD TSID;
	WORD SID;
}EPGCAP_SERVICE_INFO;


#endif
