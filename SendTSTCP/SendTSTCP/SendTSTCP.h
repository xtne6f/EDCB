#ifndef INCLUDE_SEND_TS_TCP_H
#define INCLUDE_SEND_TS_TCP_H

//DLLの初期化
//戻り値：識別ID（-1でエラー）
__declspec(dllexport)
int WINAPI InitializeDLL(
	);

//DLLの開放
//戻り値：エラーコード
__declspec(dllexport)
DWORD WINAPI UnInitializeDLL(
	int iID //[IN] InitializeDLLの戻り値
	);

//送信先を追加
//戻り値：エラーコード
__declspec(dllexport)
DWORD WINAPI AddSendAddrDLL(
	int iID, //[IN] InitializeDLLの戻り値
	LPCWSTR lpcwszIP,
	DWORD dwPort
	);

//送信先クリア
//戻り値：エラーコード
__declspec(dllexport)
DWORD WINAPI ClearSendAddrDLL(
	int iID //[IN] InitializeDLLの戻り値
	);

//データ送信を開始
//戻り値：エラーコード
__declspec(dllexport)
DWORD WINAPI StartSendDLL(
	int iID //[IN] InitializeDLLの戻り値
	);

//データ送信を停止
//戻り値：エラーコード
__declspec(dllexport)
DWORD WINAPI StopSendDLL(
	int iID //[IN] InitializeDLLの戻り値
	);

//データ送信を開始
//戻り値：エラーコード
__declspec(dllexport)
DWORD WINAPI AddSendDataDLL(
	int iID, //[IN] InitializeDLLの戻り値
	BYTE* pbData,
	DWORD dwSize
	);

//送信バッファをクリア
//戻り値：エラーコード
__declspec(dllexport)
DWORD WINAPI ClearSendBuffDLL(
	int iID //[IN] InitializeDLLの戻り値
	);


#endif
