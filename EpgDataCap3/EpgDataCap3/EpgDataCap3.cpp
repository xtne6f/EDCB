// EpgDataCap3.cpp : DLL アプリケーション用にエクスポートされる関数を定義します。
//

#include "stdafx.h"

#include "EpgDataCap3Main.h"
#include "../../Common/ErrDef.h"
#include "../../Common/InstanceManager.h"

CInstanceManager<CEpgDataCap3Main> g_instMng;

//DLLの初期化
//戻り値：
// エラーコード
//引数：
// asyncFlag		[IN]予約（必ずFALSEを渡すこと）
// id				[OUT]識別ID
DWORD WINAPI InitializeEP(
	BOOL asyncFlag,
	DWORD* id
	)
{
	if (id == NULL) {
		return ERR_INVALID_ARG;
	}

	DWORD err = ERR_FALSE;
	*id = g_instMng.INVALID_ID;

	try {
		std::shared_ptr<CEpgDataCap3Main> ptr = std::make_shared<CEpgDataCap3Main>();
		*id = g_instMng.push(ptr);
		err = NO_ERR;
	} catch (std::bad_alloc &) {
		err = ERR_FALSE;
	}

	_OutputDebugString(L"EgpDataCap3.dll [InitializeEP : id=%d]\n", *id);

	return err;
}

//DLLの開放
//戻り値：
// エラーコード
//引数：
// id		[IN]識別ID InitializeEPの戻り値
DWORD WINAPI UnInitializeEP(
	DWORD id
	)
{
	_OutputDebugString(L"EgpDataCap3.dll [UnInitializeEP : id=%d]\n", id);

	DWORD err = ERR_NOT_INIT;
	{
		std::shared_ptr<CEpgDataCap3Main> ptr = g_instMng.pop(id);
		if (ptr != NULL) {
			err = NO_ERR;
		}
	}

	return err;
}

//解析対象のTSパケット１つを読み込ませる
//戻り値：
// エラーコード
// id		[IN]識別ID InitializeEPの戻り値
// data		[IN]TSパケット１つ
// size		[IN]dataのサイズ（188でなければならない）
DWORD WINAPI AddTSPacketEP(
	DWORD id,
	BYTE* data,
	DWORD size
	)
{
	std::shared_ptr<CEpgDataCap3Main> ptr = g_instMng.find(id);
	if (ptr == NULL) {
		return ERR_NOT_INIT;
	}
	if (data == NULL || size != 188) {
		return ERR_INVALID_ARG;
	}

	ptr->AddTSPacket(data);
	return NO_ERR;
}

//解析データの現在のストリームＩＤを取得する
//戻り値：
// エラーコード
// id						[IN]識別ID
// originalNetworkID		[OUT]現在のoriginalNetworkID
// transportStreamID		[OUT]現在のtransportStreamID
DWORD WINAPI GetTSIDEP(
	DWORD id,
	WORD* originalNetworkID,
	WORD* transportStreamID
	)
{
	std::shared_ptr<CEpgDataCap3Main> ptr = g_instMng.find(id);
	if (ptr == NULL) {
		return ERR_NOT_INIT;
	}
	if (originalNetworkID == NULL || transportStreamID == NULL) {
		return ERR_INVALID_ARG;
	}

	return ptr->GetTSID(originalNetworkID, transportStreamID);
}

//自ストリームのサービス一覧を取得する
//戻り値：
// エラーコード
//引数：
// id						[IN]識別ID
// serviceListSize			[OUT]serviceListの個数
// serviceList				[OUT]サービス情報のリスト（DLL内で自動的にdeleteする。次に取得を行うまで有効）
DWORD WINAPI GetServiceListActualEP(
	DWORD id,
	DWORD* serviceListSize,
	SERVICE_INFO** serviceList
	)
{
	std::shared_ptr<CEpgDataCap3Main> ptr = g_instMng.find(id);
	if (ptr == NULL) {
		return ERR_NOT_INIT;
	}
	if (serviceListSize == NULL || serviceList == NULL) {
		return ERR_INVALID_ARG;
	}

	return ptr->GetServiceListActual(serviceListSize, serviceList);
}

//蓄積されたEPG情報のあるサービス一覧を取得する
//SERVICE_EXT_INFOの情報はない場合がある
//戻り値：
// エラーコード
//引数：
// id						[IN]識別ID
// serviceListSize			[OUT]serviceListの個数
// serviceList				[OUT]サービス情報のリスト（DLL内で自動的にdeleteする。次に取得を行うまで有効）
DWORD WINAPI GetServiceListEpgDBEP(
	DWORD id,
	DWORD* serviceListSize,
	SERVICE_INFO** serviceList
	)
{
	std::shared_ptr<CEpgDataCap3Main> ptr = g_instMng.find(id);
	if (ptr == NULL) {
		return ERR_NOT_INIT;
	}
	if (serviceListSize == NULL || serviceList == NULL) {
		return ERR_INVALID_ARG;
	}

	ptr->GetServiceListEpgDB(serviceListSize, serviceList);
	return NO_ERR;
}

//指定サービスの全EPG情報を取得する
//戻り値：
// エラーコード
//引数：
// id						[IN]識別ID
// originalNetworkID		[IN]取得対象のoriginalNetworkID
// transportStreamID		[IN]取得対象のtransportStreamID
// serviceID				[IN]取得対象のServiceID
// epgInfoListSize			[OUT]epgInfoListの個数
// epgInfoList				[OUT]EPG情報のリスト（DLL内で自動的にdeleteする。次に取得を行うまで有効）
DWORD WINAPI GetEpgInfoListEP(
	DWORD id,
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	DWORD* epgInfoListSize,
	EPG_EVENT_INFO** epgInfoList
	)
{
	std::shared_ptr<CEpgDataCap3Main> ptr = g_instMng.find(id);
	if (ptr == NULL) {
		return ERR_NOT_INIT;
	}
	if (epgInfoListSize == NULL || epgInfoList == NULL) {
		return ERR_INVALID_ARG;
	}

	if (ptr->GetEpgInfoList(originalNetworkID, transportStreamID, serviceID, epgInfoListSize, epgInfoList) == FALSE) {
		return ERR_NOT_FIND;
	}
	return NO_ERR;
}

//指定サービスの現在or次のEPG情報を取得する
//戻り値：
// エラーコード
//引数：
// originalNetworkID		[IN]取得対象のoriginalNetworkID
// transportStreamID		[IN]取得対象のtransportStreamID
// serviceID				[IN]取得対象のServiceID
// nextFlag					[IN]TRUE（次の番組）、FALSE（現在の番組）
// epgInfo					[OUT]EPG情報（DLL内で自動的にdeleteする。次に取得を行うまで有効）
DWORD WINAPI GetEpgInfoEP(
	DWORD id,
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	BOOL nextFlag,
	EPG_EVENT_INFO** epgInfo
	)
{
	std::shared_ptr<CEpgDataCap3Main> ptr = g_instMng.find(id);
	if (ptr == NULL) {
		return ERR_NOT_INIT;
	}
	if (epgInfo == NULL) {
		return ERR_INVALID_ARG;
	}

	if (ptr->GetEpgInfo(originalNetworkID, transportStreamID, serviceID, nextFlag, epgInfo) == FALSE) {
		return ERR_NOT_FIND;
	}
	return NO_ERR;
}

//指定イベントのEPG情報を取得する
//戻り値：
// エラーコード
//引数：
// id						[IN]識別ID
// originalNetworkID		[IN]取得対象のoriginalNetworkID
// transportStreamID		[IN]取得対象のtransportStreamID
// serviceID				[IN]取得対象のServiceID
// eventID					[IN]取得対象のEventID
// pfOnlyFlag				[IN]p/fからのみ検索するかどうか
// epgInfo					[OUT]EPG情報（DLL内で自動的にdeleteする。次に取得を行うまで有効）
DWORD WINAPI SearchEpgInfoEP(
	DWORD id,
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	WORD eventID,
	BYTE pfOnlyFlag,
	EPG_EVENT_INFO** epgInfo
	)
{
	std::shared_ptr<CEpgDataCap3Main> ptr = g_instMng.find(id);
	if (ptr == NULL) {
		return ERR_NOT_INIT;
	}
	if (epgInfo == NULL) {
		return ERR_INVALID_ARG;
	}

	if (ptr->SearchEpgInfo(originalNetworkID, transportStreamID, serviceID, eventID, pfOnlyFlag, epgInfo) == FALSE) {
		return ERR_NOT_FIND;
	}
	return NO_ERR;
}

//EPGデータの蓄積状態をリセットする
//引数：
// id						[IN]識別ID
void WINAPI ClearSectionStatusEP(
	DWORD id
	)
{
	std::shared_ptr<CEpgDataCap3Main> ptr = g_instMng.find(id);
	if (ptr == NULL) {
		return;
	}

	ptr->ClearSectionStatus();
}

//EPGデータの蓄積状態を取得する
//戻り値：
// ステータス
//引数：
// id						[IN]識別ID
// l_eitFlag				[IN]L-EITのステータスを取得
EPG_SECTION_STATUS WINAPI GetSectionStatusEP(
	DWORD id,
	BOOL l_eitFlag
	)
{
	std::shared_ptr<CEpgDataCap3Main> ptr = g_instMng.find(id);
	if (ptr == NULL) {
		return EpgNoData;
	}

	return ptr->GetSectionStatus(l_eitFlag);
}

//PC時計を元としたストリーム時間との差を取得する
//戻り値：
// 差の秒数
//引数：
// id						[IN]識別ID
int WINAPI GetTimeDelayEP(
	DWORD id
	)
{
	std::shared_ptr<CEpgDataCap3Main> ptr = g_instMng.find(id);
	if (ptr == NULL) {
		return 0;
	}

	return ptr->GetTimeDelay();
}