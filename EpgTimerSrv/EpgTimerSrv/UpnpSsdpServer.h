#pragma once

#include "../../Common/ThreadUtil.h"

//UPnPのUDP(Port1900)部分を担当するサーバ
//UPnPCtrlフォルダにあるC言語ベース(?)のコードをC++で再実装したもの
//※UPnPCtrlフォルダは不要のため削除済み。必要なら以前のコミットを参照
//  UPnP(DLNA)のHTTP応答や文字列処理などがほぼスタンドアロンで実装されていた
class CUpnpSsdpServer
{
public:
	static const int RECV_BUFF_SIZE = 2048;
	static const unsigned int NOTIFY_INTERVAL_SEC = 1000;
	static const int SSDP_IF_LOOPBACK = 1;
	static const int SSDP_IF_C_PRIVATE = 2;
	static const int SSDP_IF_B_PRIVATE = 4;
	static const int SSDP_IF_A_PRIVATE = 8;
	static const int SSDP_IF_LINKLOCAL = 16;
	static const int SSDP_IF_GLOBAL = 32;
	struct SSDP_TARGET_INFO {
		string target;
		string location;
		string usn;
		bool notifyFlag;
	};
	~CUpnpSsdpServer();
	bool Start(const vector<SSDP_TARGET_INFO>& targetList_, int ifTypes, int initialWaitSec_);
	void Stop();
private:
	static void SsdpThread(CUpnpSsdpServer* sys);
	thread_ ssdpThread;
	atomic_bool_ stopFlag;
	vector<SSDP_TARGET_INFO> targetList;
	int ssdpIfTypes;
	int initialWaitSec;
};
