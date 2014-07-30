#include "StdAfx.h"
#include "EpgDataCap3Util.h"

#include "ErrDef.h"

CEpgDataCap3Util::CEpgDataCap3Util(void)
{
	module = NULL;
	id = 0;
}

CEpgDataCap3Util::~CEpgDataCap3Util(void)
{
	UnLoadDll();
}

BOOL CEpgDataCap3Util::LoadDll(void)
{
	if( module != NULL ){
		return FALSE;
	}

	pfnInitializeEP3 = NULL;
	pfnUnInitializeEP3 = NULL;
	pfnAddTSPacketEP3 = NULL;
	pfnGetTSIDEP3 = NULL;
	pfnGetEpgInfoListEP3 = NULL;
	pfnClearSectionStatusEP3 = NULL;
	pfnGetSectionStatusEP3 = NULL;
	pfnGetServiceListActualEP3 = NULL;
	pfnGetServiceListEpgDBEP3 = NULL;
	pfnGetEpgInfoEP3 = NULL;
	pfnSearchEpgInfoEP3 = NULL;
	pfnGetTimeDelayEP3 = NULL;


	BOOL ret = TRUE;

	module = ::LoadLibrary(L"EpgDataCap3.dll");

	if( module == NULL ){
		OutputDebugString(L"EpgDataCap3.dllのロードに失敗しました\r\n");
		return FALSE;
	}

	pfnInitializeEP3 = ( InitializeEP3 ) ::GetProcAddress( module , "InitializeEP");
	if( !pfnInitializeEP3 ){
		OutputDebugString(L"InitializeEPの GetProcAddress に失敗\r\n");
		ret = FALSE;
		goto ERR_END;
	}
	pfnUnInitializeEP3 = ( UnInitializeEP3 ) ::GetProcAddress( module , "UnInitializeEP");
	if( !pfnUnInitializeEP3 ){
		OutputDebugString(L"UnInitializeEPの GetProcAddress に失敗\r\n");
		ret = FALSE;
		goto ERR_END;
	}
	pfnAddTSPacketEP3 = ( AddTSPacketEP3 ) ::GetProcAddress( module , "AddTSPacketEP");
	if( !pfnAddTSPacketEP3 ){
		OutputDebugString(L"AddTSPacketEPの GetProcAddress に失敗\r\n");
		ret = FALSE;
		goto ERR_END;
	}
	pfnGetTSIDEP3 = ( GetTSIDEP3 ) ::GetProcAddress( module , "GetTSIDEP");
	if( !pfnGetTSIDEP3 ){
		OutputDebugString(L"GetTSIDEPの GetProcAddress に失敗\r\n");
		ret = FALSE;
		goto ERR_END;
	}
	pfnGetEpgInfoListEP3 = ( GetEpgInfoListEP3 ) ::GetProcAddress( module , "GetEpgInfoListEP");
	if( !pfnGetEpgInfoListEP3 ){
		OutputDebugString(L"GetEpgInfoListEPの GetProcAddress に失敗\r\n");
		ret = FALSE;
		goto ERR_END;
	}
	pfnClearSectionStatusEP3 = ( ClearSectionStatusEP3 ) ::GetProcAddress( module , "ClearSectionStatusEP");
	if( !pfnClearSectionStatusEP3 ){
		OutputDebugString(L"ClearSectionStatusEPの GetProcAddress に失敗\r\n");
		ret = FALSE;
		goto ERR_END;
	}
	pfnGetSectionStatusEP3 = ( GetSectionStatusEP3 ) ::GetProcAddress( module , "GetSectionStatusEP");
	if( !pfnGetSectionStatusEP3 ){
		OutputDebugString(L"GetSectionStatusEPの GetProcAddress に失敗\r\n");
		ret = FALSE;
		goto ERR_END;
	}
	pfnGetServiceListActualEP3 = ( GetServiceListActualEP3 ) ::GetProcAddress( module , "GetServiceListActualEP");
	if( !pfnGetServiceListActualEP3 ){
		OutputDebugString(L"GetServiceListActualEPの GetProcAddress に失敗\r\n");
		ret = FALSE;
		goto ERR_END;
	}
	pfnGetServiceListEpgDBEP3 = ( GetServiceListEpgDBEP3 ) ::GetProcAddress( module , "GetServiceListEpgDBEP");
	if( !pfnGetServiceListEpgDBEP3 ){
		OutputDebugString(L"GetServiceListEpgDBEPの GetProcAddress に失敗\r\n");
		ret = FALSE;
		goto ERR_END;
	}
	pfnGetEpgInfoEP3 = ( GetEpgInfoEP3 ) ::GetProcAddress( module , "GetEpgInfoEP");
	if( !pfnGetEpgInfoEP3 ){
		OutputDebugString(L"GetEpgInfoEPの GetProcAddress に失敗\r\n");
		ret = FALSE;
		goto ERR_END;
	}
	pfnSearchEpgInfoEP3 = ( SearchEpgInfoEP3 ) ::GetProcAddress( module , "SearchEpgInfoEP");
	if( !pfnSearchEpgInfoEP3 ){
		OutputDebugString(L"SearchEpgInfoEPの GetProcAddress に失敗\r\n");
		ret = FALSE;
		goto ERR_END;
	}
	pfnGetTimeDelayEP3 = ( GetTimeDelayEP3 ) ::GetProcAddress( module , "GetTimeDelayEP");
	if( !pfnGetTimeDelayEP3 ){
		OutputDebugString(L"GetTimeDelayEPの GetProcAddress に失敗\r\n");
		ret = FALSE;
		goto ERR_END;
	}


ERR_END:
	if( ret == FALSE ){
		::FreeLibrary( module );
		module=NULL;
	}
	return ret;
}

BOOL CEpgDataCap3Util::UnLoadDll(void)
{
	if( module != NULL ){
		if( id != 0 ){
			pfnUnInitializeEP3(id);
		}
		::FreeLibrary( module );
		id = 0;
	}
	module = NULL;

	pfnInitializeEP3 = NULL;
	pfnUnInitializeEP3 = NULL;
	pfnAddTSPacketEP3 = NULL;
	pfnGetTSIDEP3 = NULL;
	pfnGetEpgInfoListEP3 = NULL;
	pfnClearSectionStatusEP3 = NULL;
	pfnGetSectionStatusEP3 = NULL;
	pfnGetServiceListActualEP3 = NULL;
	pfnGetServiceListEpgDBEP3 = NULL;
	pfnGetEpgInfoEP3 = NULL;
	pfnSearchEpgInfoEP3 = NULL;
	pfnGetTimeDelayEP3 = NULL;

	return TRUE;
}

//DLLの初期化
//戻り値：
// エラーコード
//引数：
// asyncMode		[IN]TRUE:非同期モード、FALSE:同期モード
DWORD CEpgDataCap3Util::Initialize(
	BOOL asyncFlag
	)
{
	if( LoadDll() == FALSE ){
		return ERR_INIT;
	}
	DWORD err = pfnInitializeEP3(asyncFlag, &id);
	if( err != NO_ERR ){
		id = 0;
	}
	return err;
}

//DLLの開放
//戻り値：
// エラーコード
DWORD CEpgDataCap3Util::UnInitialize(
	)
{
	if( module == NULL || id == 0 ){
		return ERR_NOT_INIT;
	}
	DWORD err = pfnUnInitializeEP3(id);
	id = 0; // ← これがないと下の UnLoadDll で再度 UnInitializeEP が呼ばれる
	UnLoadDll();
	return err;
}

//解析対象のTSパケット１つを読み込ませる
//戻り値：
// エラーコード
//引数：
// data		[IN]TSパケット１つ
// size		[IN]dataのサイズ（188、192あたりになるはず）
DWORD CEpgDataCap3Util::AddTSPacket(
	BYTE* data,
	DWORD size
	)
{
	if( module == NULL || id == 0 ){
		return ERR_NOT_INIT;
	}
	return pfnAddTSPacketEP3(id, data, size);
}

//解析データの現在のストリームＩＤを取得する
//戻り値：
// エラーコード
//引数：
// originalNetworkID		[OUT]現在のoriginalNetworkID
// transportStreamID		[OUT]現在のtransportStreamID
DWORD CEpgDataCap3Util::GetTSID(
	WORD* originalNetworkID,
	WORD* transportStreamID
	)
{
	if( module == NULL || id == 0 ){
		return ERR_NOT_INIT;
	}
	return pfnGetTSIDEP3(id, originalNetworkID, transportStreamID);
}

//自ストリームのサービス一覧を取得する
//戻り値：
// エラーコード
//引数：
// serviceListSize			[OUT]serviceListの個数
// serviceList				[OUT]サービス情報のリスト（DLL内で自動的にdeleteする。次に取得を行うまで有効）
DWORD CEpgDataCap3Util::GetServiceListActual(
	DWORD* serviceListSize,
	SERVICE_INFO** serviceList
	)
{
	if( module == NULL || id == 0 ){
		return ERR_NOT_INIT;
	}
	return pfnGetServiceListActualEP3(id, serviceListSize, serviceList);
}

//蓄積されたEPG情報のあるサービス一覧を取得する
//SERVICE_EXT_INFOの情報はない
//戻り値：
// エラーコード
//引数：
// serviceListSize			[OUT]serviceListの個数
// serviceList				[OUT]サービス情報のリスト（DLL内で自動的にdeleteする。次に取得を行うまで有効）
DWORD CEpgDataCap3Util::GetServiceListEpgDB(
	DWORD* serviceListSize,
	SERVICE_INFO** serviceList
	)
{
	if( module == NULL || id == 0 ){
		return ERR_NOT_INIT;
	}
	return pfnGetServiceListEpgDBEP3(id, serviceListSize, serviceList);
}

//指定サービスの全EPG情報を取得する
//戻り値：
// エラーコード
//引数：
// originalNetworkID		[IN]取得対象のoriginalNetworkID
// transportStreamID		[IN]取得対象のtransportStreamID
// serviceID				[IN]取得対象のServiceID
// epgInfoListSize			[OUT]epgInfoListの個数
// epgInfoList				[OUT]EPG情報のリスト（DLL内で自動的にdeleteする。次に取得を行うまで有効）
DWORD CEpgDataCap3Util::GetEpgInfoList(
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	DWORD* epgInfoListSize,
	EPG_EVENT_INFO** epgInfoList
	)
{
	if( module == NULL || id == 0 ){
		return ERR_NOT_INIT;
	}
	return pfnGetEpgInfoListEP3(id, originalNetworkID, transportStreamID, serviceID, epgInfoListSize, epgInfoList);
}

//EPGデータの蓄積状態をリセットする
void CEpgDataCap3Util::ClearSectionStatus()
{
	if( module == NULL || id == 0 ){
		return ;
	}
	pfnClearSectionStatusEP3(id);
}

//EPGデータの蓄積状態を取得する
//戻り値：
// ステータス
//引数：
// l_eitFlag		[IN]L-EITのステータスを取得
EPG_SECTION_STATUS CEpgDataCap3Util::GetSectionStatus(
	BOOL l_eitFlag
	)
{
	if( module == NULL || id == 0 ){
		return EpgNoData;
	}
	return pfnGetSectionStatusEP3(id, l_eitFlag);
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
DWORD CEpgDataCap3Util::GetEpgInfo(
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	BOOL nextFlag,
	EPG_EVENT_INFO** epgInfo
	)
{
	if( module == NULL || id == 0 ){
		return ERR_NOT_INIT;
	}
	return pfnGetEpgInfoEP3(id, originalNetworkID, transportStreamID, serviceID, nextFlag, epgInfo);
}

//指定イベントのEPG情報を取得する
//戻り値：
// エラーコード
//引数：
// originalNetworkID		[IN]取得対象のoriginalNetworkID
// transportStreamID		[IN]取得対象のtransportStreamID
// serviceID				[IN]取得対象のServiceID
// EventID					[IN]取得対象のEventID
// pfOnlyFlag				[IN]p/fからのみ検索するかどうか
// epgInfo					[OUT]EPG情報（DLL内で自動的にdeleteする。次に取得を行うまで有効）
DWORD CEpgDataCap3Util::SearchEpgInfo(
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	WORD eventID,
	BYTE pfOnlyFlag,
	EPG_EVENT_INFO** epgInfo
	)
{
	if( module == NULL || id == 0 ){
		return ERR_NOT_INIT;
	}
	return pfnSearchEpgInfoEP3(id, originalNetworkID, transportStreamID, serviceID, eventID, pfOnlyFlag, epgInfo);
}

//PC時計を元としたストリーム時間との差を取得する
//戻り値：
// 差の秒数
int CEpgDataCap3Util::GetTimeDelay(
	)
{
	if( module == NULL || id == 0 ){
		return 0;
	}
	return pfnGetTimeDelayEP3(id);
}
