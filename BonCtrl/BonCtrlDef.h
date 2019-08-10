#ifndef INCLUDE_BON_CTRL_DEF_H
#define INCLUDE_BON_CTRL_DEF_H

#define MUTEX_UDP_PORT_NAME			L"EpgDataCap_Bon_UDP_PORT_" //+IP_ポート番号
#define MUTEX_TCP_PORT_NAME			L"EpgDataCap_Bon_TCP_PORT_" //+IP_ポート番号
#define CHSET_SAVE_EVENT_WAIT		L"Global\\EpgTimer_ChSet"

//ネットワーク送信の既定ポート番号
#define BON_UDP_PORT_BEGIN			1234
#define BON_TCP_PORT_BEGIN			2230

//ネットワーク送信のポート番号の増分範囲
#define BON_NW_PORT_RANGE			100

//間接指定がなければ通常必要でないPID範囲の下限
#define BON_SELECTIVE_PID			0x0030

//ネットワーク送信用設定
typedef struct {
	wstring ipString;
	DWORD port;
	BOOL broadcastFlag;
}NW_SEND_INFO;

class CSendNW
{
public:
	CSendNW() {}
	virtual ~CSendNW() {}
	virtual bool Initialize() = 0;
	virtual void UnInitialize() = 0;
	virtual bool IsInitialized() const = 0;
	virtual bool AddSendAddr(LPCWSTR ip, DWORD dwPort, bool broadcastFlag) = 0;
	virtual void ClearSendAddr() = 0;
	virtual bool StartSend() = 0;
	virtual void StopSend() = 0;
	virtual bool AddSendData(BYTE* pbBuff, DWORD dwSize) = 0;
	virtual void ClearSendBuff() {}
private:
	CSendNW(const CSendNW&);
	CSendNW& operator=(const CSendNW&);
};

#endif
