#ifndef __BON_CTRL_DEF_H__
#define __BON_CTRL_DEF_H__

#include "../Common/Util.h"

#define MUTEX_UDP_PORT_NAME			TEXT("Global\\EpgDataCap_Bon_UDP_PORT_")
#define MUTEX_TCP_PORT_NAME			TEXT("Global\\EpgDataCap_Bon_TCP_PORT_")
#define CHSET_SAVE_EVENT_WAIT		 _T("Global\\EpgTimer_ChSet")


//BonDriverのチューナー空間情報
typedef struct _BON_SPACE_INFO{
	wstring spaceName;					//チューナー空間名
	map<DWORD, wstring> chMap;			//チャンネルリスト（キー CH）
}BON_SPACE_INFO;

//受信データのバッファリング用
typedef struct _TS_DATA{
	BYTE* data;				//TSデータ
	DWORD size;				//dataのサイズ
	_TS_DATA(void){
		data = NULL;
		size = 0;
	}
	~_TS_DATA(void){
		SAFE_DELETE_ARRAY(data);
	}
} TS_DATA;

//ネットワーク送信用設定
typedef struct _NW_SEND_INFO{
	wstring ipString;
	DWORD ip;
	DWORD port;
	BOOL broadcastFlag;
}NW_SEND_INFO;

//サービス情報
typedef struct _TS_SERVICE_INFO{
	WORD ONID;
	WORD TSID;
	WORD SID;
	BYTE serviceType;
	BOOL partialFlag;
	wstring serviceName;
	wstring networkName;
	BYTE remoteControlKeyID;
}TS_SERVICE_INFO;

//EPG取得用サービス情報
typedef struct _EPGCAP_SERVICE_INFO{
	WORD ONID;
	WORD TSID;
	WORD SID;
}EPGCAP_SERVICE_INFO;


#endif
