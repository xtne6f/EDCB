#ifndef INCLUDE_SEND_TS_TCP_DLL_UTIL_H
#define INCLUDE_SEND_TS_TCP_DLL_UTIL_H

#include "ErrDef.h"

class CSendTSTCPDllUtil
{
public:
	CSendTSTCPDllUtil(void);
	~CSendTSTCPDllUtil(void);

	//DLLの初期化
	//戻り値：エラーコード
	DWORD Initialize(
		);

	//DLLの開放
	//戻り値：なし
	void UnInitialize(
		);

	//送信先を追加
	//戻り値：エラーコード
	DWORD AddSendAddr(
		LPCWSTR lpcwszIP,
		DWORD dwPort
		);

	//送信先クリア
	//戻り値：エラーコード
	DWORD ClearSendAddr(
		);

	//データ送信を開始
	//戻り値：エラーコード
	DWORD StartSend(
		);

	//データ送信を停止
	//戻り値：エラーコード
	DWORD StopSend(
		);

	//データ送信を開始
	//戻り値：エラーコード
	DWORD AddSendData(
		BYTE* pbData,
		DWORD dwSize
		);

	//送信バッファをクリア
	//戻り値：エラーコード
	DWORD ClearSendBuff(
		);
		
protected:
	typedef int (WINAPI *InitializeDLL)();
	typedef DWORD (WINAPI *UnInitializeDLL)(int iID);
	typedef DWORD (WINAPI *AddSendAddrDLL)(int iID, LPCWSTR lpcwszIP, DWORD dwPort);
	typedef DWORD (WINAPI *ClearSendAddrDLL)(int iID);
	typedef DWORD (WINAPI *StartSendDLL)(int iID);
	typedef DWORD (WINAPI *StopSendDLL)(int iID);
	typedef DWORD (WINAPI *AddSendDataDLL)(int iID, BYTE* pbData, DWORD dwSize);
	typedef DWORD (WINAPI *ClearSendBuffDLL)(int iID);

	HMODULE m_hModule;
	int m_iID;

	InitializeDLL pfnInitializeDLL;
	UnInitializeDLL pfnUnInitializeDLL;
	AddSendAddrDLL pfnAddSendAddrDLL;
	ClearSendAddrDLL pfnClearSendAddrDLL;
	StartSendDLL pfnStartSendDLL;
	StopSendDLL pfnStopSendDLL;
	AddSendDataDLL pfnAddSendDataDLL;
	ClearSendBuffDLL pfnClearSendBuffDLL;

protected:
	BOOL LoadDll(void);
	void UnLoadDll(void);
};

#endif
