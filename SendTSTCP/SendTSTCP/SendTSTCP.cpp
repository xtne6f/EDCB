// SendTSTCP.cpp : DLL アプリケーション用にエクスポートされる関数を定義します。
//

#include "stdafx.h"

#include "../../Common/Util.h"
#include "../../Common/ErrDef.h"
#include "SendTSTCPMain.h"
#include "../../Common/InstanceManager.h"

CInstanceManager<CSendTSTCPMain> g_instMng;


//DLLの初期化
//戻り値：識別ID（-1でエラー）
int WINAPI InitializeDLL(
	)
{
	int iID = -1;

	try{
		std::shared_ptr<CSendTSTCPMain> ptr = std::make_shared<CSendTSTCPMain>();
		if( ptr->Initialize() != FALSE ){
			iID = (int)g_instMng.push(ptr);
		}
	}catch( std::bad_alloc& ){
	}

	return iID;
}


//DLLの開放
//戻り値：エラーコード
DWORD WINAPI UnInitializeDLL(
	int iID //[IN] InitializeDLLの戻り値
	)
{
	DWORD err = ERR_NOT_INIT;
	{
		std::shared_ptr<CSendTSTCPMain> ptr = g_instMng.pop(iID);
		if( ptr != NULL ){
			ptr->UnInitialize();
			err = NO_ERR;
		}
	}
	return err;
}


//送信先を追加
//戻り値：エラーコード
DWORD WINAPI AddSendAddrDLL(
	int iID, //[IN] InitializeDLLの戻り値
	LPCWSTR lpcwszIP,
	DWORD dwPort
	)
{
	std::shared_ptr<CSendTSTCPMain> ptr = g_instMng.find(iID);
	if( ptr == NULL ){
		return ERR_NOT_INIT;
	}
	DWORD dwRet = NO_ERR;
	dwRet = ptr->AddSendAddr(lpcwszIP, dwPort);
	return dwRet;
}

//送信先クリア
//戻り値：エラーコード
DWORD WINAPI ClearSendAddrDLL(
	int iID //[IN] InitializeDLLの戻り値
	)
{
	std::shared_ptr<CSendTSTCPMain> ptr = g_instMng.find(iID);
	if( ptr == NULL ){
		return ERR_NOT_INIT;
	}
	DWORD dwRet = NO_ERR;
	dwRet = ptr->ClearSendAddr();
	return dwRet;
}

//データ送信を開始
//戻り値：エラーコード
DWORD WINAPI StartSendDLL(
	int iID //[IN] InitializeDLLの戻り値
	)
{
	std::shared_ptr<CSendTSTCPMain> ptr = g_instMng.find(iID);
	if( ptr == NULL ){
		return ERR_NOT_INIT;
	}
	DWORD dwRet = NO_ERR;
	dwRet = ptr->StartSend();
	return dwRet;
}

//データ送信を停止
//戻り値：エラーコード
DWORD WINAPI StopSendDLL(
	int iID //[IN] InitializeDLLの戻り値
	)
{
	std::shared_ptr<CSendTSTCPMain> ptr = g_instMng.find(iID);
	if( ptr == NULL ){
		return ERR_NOT_INIT;
	}
	DWORD dwRet = NO_ERR;
	dwRet = ptr->StopSend();
	return dwRet;
}

//データ送信を開始
//戻り値：エラーコード
DWORD WINAPI AddSendDataDLL(
	int iID, //[IN] InitializeDLLの戻り値
	BYTE* pbData,
	DWORD dwSize
	)
{
	std::shared_ptr<CSendTSTCPMain> ptr = g_instMng.find(iID);
	if( ptr == NULL ){
		return ERR_NOT_INIT;
	}
	DWORD dwRet = NO_ERR;
	dwRet = ptr->AddSendData(pbData, dwSize);
	return dwRet;
}

//送信バッファをクリア
//戻り値：エラーコード
DWORD WINAPI ClearSendBuffDLL(
	int iID //[IN] InitializeDLLの戻り値
	)
{
	std::shared_ptr<CSendTSTCPMain> ptr = g_instMng.find(iID);
	if( ptr == NULL ){
		return ERR_NOT_INIT;
	}
	DWORD dwRet = NO_ERR;
	dwRet = ptr->ClearSendBuff();
	return dwRet;
}
