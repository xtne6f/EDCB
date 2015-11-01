#ifndef __SEND_TS_TCP_H__
#define __SEND_TS_TCP_H__

namespace sendtstcp{
//DLLの初期化
//戻り値：識別ID（-1でエラー）
typedef int (WINAPI *InitializeDLL)(
	);

//DLLの開放
//戻り値：エラーコード
typedef DWORD (WINAPI *UnInitializeDLL)(
	int iID //[IN] InitializeDLLの戻り値
	);

//送信先を追加
//戻り値：エラーコード
typedef DWORD (WINAPI *AddSendAddrDLL)(
	int iID, //[IN] InitializeDLLの戻り値
	LPCWSTR lpcwszIP,
	DWORD dwPort
	);

//送信先クリア
//戻り値：エラーコード
typedef DWORD (WINAPI *ClearSendAddrDLL)(
	int iID //[IN] InitializeDLLの戻り値
	);

//データ送信を開始
//戻り値：エラーコード
typedef DWORD (WINAPI *StartSendDLL)(
	int iID //[IN] InitializeDLLの戻り値
	);

//データ送信を停止
//戻り値：エラーコード
typedef DWORD (WINAPI *StopSendDLL)(
	int iID //[IN] InitializeDLLの戻り値
	);

//データ送信を開始
//戻り値：エラーコード
typedef DWORD (WINAPI *AddSendDataDLL)(
	int iID, //[IN] InitializeDLLの戻り値
	BYTE* pbData,
	DWORD dwSize
	);

//送信バッファをクリア
//戻り値：エラーコード
typedef DWORD (WINAPI *ClearSendBuffDLL)(
	int iID //[IN] InitializeDLLの戻り値
	);

}
#endif
