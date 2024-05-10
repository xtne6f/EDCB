#ifndef INCLUDE_BON_CTRL_DEF_H
#define INCLUDE_BON_CTRL_DEF_H

#define MUTEX_UDP_PORT_NAME			L"EpgDataCap_Bon_UDP_PORT_" //+IP_ポート番号
#define MUTEX_TCP_PORT_NAME			L"EpgDataCap_Bon_TCP_PORT_" //+IP_ポート番号
#define CHSET_SAVE_EVENT_WAIT		L"EpgTimer_ChSet"

//ネットワーク送信の既定ポート番号
#define BON_UDP_PORT_BEGIN			1234
#define BON_TCP_PORT_BEGIN			2230

//ネットワーク送信のポート番号の増分範囲
#define BON_NW_PORT_RANGE			30

//ネットワーク送信のうち特別に名前付きパイプが使われるIPアドレス
#define BON_NW_SRV_PIPE_IP			L"0.0.0.1"
#define BON_NW_PIPE_IP				L"0.0.0.2"

//間接指定がなければ通常必要でないPID範囲の下限
#define BON_SELECTIVE_PID			0x0030

//ネットワーク送信用設定
struct NW_SEND_INFO {
	wstring ipString;
	DWORD port;
	BOOL broadcastFlag;
	int udpMaxSendSize;
};

#endif
