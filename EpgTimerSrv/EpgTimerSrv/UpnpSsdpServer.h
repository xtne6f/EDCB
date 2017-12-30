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
	static const unsigned int NOTIFY_FIRST_DELAY_SEC = 5;
	struct SSDP_TARGET_INFO {
		string target;
		string location;
		string usn;
		bool notifyFlag;
	};
	~CUpnpSsdpServer();
	bool Start(const vector<SSDP_TARGET_INFO>& targetList_);
	void Stop();
	static string GetUserAgent();
private:
	static vector<string> GetNICList();
	static void SsdpThread(CUpnpSsdpServer* sys);
	string GetMSearchReply(const char* header, const char* host) const;
	void SendNotifyAliveOrByebye(bool byebyeFlag, const vector<string>& nicList);
	thread_ ssdpThread;
	bool stopFlag;
	vector<SSDP_TARGET_INFO> targetList;
};
