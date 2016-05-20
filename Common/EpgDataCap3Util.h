#pragma once

#include <Windows.h>
#include "EpgDataCap3Def.h"
#include "StructDef.h"

//DLLの初期化
//戻り値：
// エラーコード
//引数：
// asyncMode		[IN]TRUE:非同期モード、FALSE:同期モード
// id				[OUT]識別ID
typedef DWORD (WINAPI *InitializeEP3)(
	BOOL asyncFlag,
	DWORD* id
	);

//DLLの開放
//戻り値：
// エラーコード
//引数：
// id		[IN]識別ID
typedef DWORD (WINAPI *UnInitializeEP3)(
	DWORD id
	);

//解析対象のTSパケット１つを読み込ませる
//戻り値：
// エラーコード
//引数：
// id		[IN]識別ID
// data		[IN]TSパケット１つ
// size		[IN]dataのサイズ（188、192あたりになるはず）
typedef DWORD (WINAPI *AddTSPacketEP3)(
	DWORD id,
	BYTE* data,
	DWORD size
	);

//解析データの現在のストリームＩＤを取得する
//戻り値：
// エラーコード
//引数：
// id						[IN]識別ID
// originalNetworkID		[OUT]現在のoriginalNetworkID
// transportStreamID		[OUT]現在のtransportStreamID
typedef DWORD (WINAPI *GetTSIDEP3)(
	DWORD id,
	WORD* originalNetworkID,
	WORD* transportStreamID
	);

//自ストリームのサービス一覧を取得する
//戻り値：
// エラーコード
//引数：
// id						[IN]識別ID
// serviceListSize			[OUT]serviceListの個数
// serviceList				[OUT]サービス情報のリスト（DLL内で自動的にdeleteする。次に取得を行うまで有効）
typedef DWORD (WINAPI *GetServiceListActualEP3)(
	DWORD id,
	DWORD* serviceListSize,
	SERVICE_INFO** serviceList
	);

//蓄積されたEPG情報のあるサービス一覧を取得する
//SERVICE_EXT_INFOの情報はなし
//戻り値：
// エラーコード
//引数：
// id						[IN]識別ID
// serviceListSize			[OUT]serviceListの個数
// serviceList				[OUT]サービス情報のリスト（DLL内で自動的にdeleteする。次に取得を行うまで有効）
typedef DWORD (WINAPI *GetServiceListEpgDBEP3)(
	DWORD id,
	DWORD* serviceListSize,
	SERVICE_INFO** serviceList
	);

//指定サービスの全EPG情報を取得する
//戻り値：
// エラーコード
// id						[IN]識別ID
//引数：
// originalNetworkID		[IN]取得対象のoriginalNetworkID
// transportStreamID		[IN]取得対象のtransportStreamID
// serviceID				[IN]取得対象のServiceID
// epgInfoListSize			[OUT]epgInfoListの個数
// epgInfoList				[OUT]EPG情報のリスト（DLL内で自動的にdeleteする。次に取得を行うまで有効）
typedef DWORD (WINAPI *GetEpgInfoListEP3)(
	DWORD id,
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	DWORD* epgInfoListSize,
	EPG_EVENT_INFO** epgInfoList
	);

//指定サービスの全EPG情報を列挙する
//引数：
// enumEpgInfoListEP3Proc	[IN]EPG情報のリストを取得するコールバック関数
// param					[IN]コールバック引数
typedef DWORD (WINAPI *EnumEpgInfoListEP3)(
	DWORD id,
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	BOOL (CALLBACK *enumEpgInfoListEP3Proc)(DWORD epgInfoListSize, EPG_EVENT_INFO* epgInfoList, LPVOID param),
	LPVOID param
	);

//EPGデータの蓄積状態をリセットする
//引数：
// id						[IN]識別ID
typedef void (WINAPI *ClearSectionStatusEP3)(
	DWORD id
	);

//EPGデータの蓄積状態を取得する
//戻り値：
// ステータス
//引数：
// id						[IN]識別ID
// l_eitFlag				[IN]L-EITのステータスを取得
typedef EPG_SECTION_STATUS (WINAPI *GetSectionStatusEP3)(
	DWORD id,
	BOOL l_eitFlag
	);

//指定サービスのEPGデータの蓄積状態を取得する
//戻り値：
// ステータス
//引数：
// id						[IN]識別ID
// originalNetworkID		[IN]取得対象のOriginalNetworkID
// transportStreamID		[IN]取得対象のTransportStreamID
// serviceID				[IN]取得対象のServiceID
// l_eitFlag				[IN]L-EITのステータスを取得
typedef EPG_SECTION_STATUS (WINAPI *GetSectionStatusServiceEP3)(
	DWORD id,
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	BOOL l_eitFlag
	);

//指定サービスの現在or次のEPG情報を取得する
//戻り値：
// エラーコード
//引数：
// id						[IN]識別ID
// originalNetworkID		[IN]取得対象のoriginalNetworkID
// transportStreamID		[IN]取得対象のtransportStreamID
// serviceID				[IN]取得対象のServiceID
// nextFlag					[IN]TRUE（次の番組）、FALSE（現在の番組）
// epgInfo					[OUT]EPG情報（DLL内で自動的にdeleteする。次に取得を行うまで有効）
typedef DWORD (WINAPI *GetEpgInfoEP3)(
	DWORD id,
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	BOOL nextFlag,
	EPG_EVENT_INFO** epgInfo
	);

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
typedef DWORD (WINAPI *SearchEpgInfoEP3)(
	DWORD id,
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	WORD eventID,
	BYTE pfOnlyFlag,
	EPG_EVENT_INFO** epgInfo
	);

//PC時計を元としたストリーム時間との差を取得する
//戻り値：
// 差の秒数
//引数：
// id						[IN]識別ID
typedef int (WINAPI *GetTimeDelayEP3)(
	DWORD id
	);

class CEpgDataCap3Util
{
public:
	CEpgDataCap3Util(void);
	~CEpgDataCap3Util(void);

	//DLLの初期化
	//戻り値：
	// エラーコード
	//引数：
	// asyncMode		[IN]TRUE:非同期モード、FALSE:同期モード
	// loadDllFilePath	[IN]ロードするDLLパス
	DWORD Initialize(
		BOOL asyncFlag,
		LPCWSTR loadDllFilePath = L"EpgDataCap3.dll"
		);

	//DLLの開放
	//戻り値：
	// エラーコード
	DWORD UnInitialize(
		);

	//解析対象のTSパケット１つを読み込ませる
	//戻り値：
	// エラーコード
	// data		[IN]TSパケット１つ
	// size		[IN]dataのサイズ（188、192あたりになるはず）
	DWORD AddTSPacket(
		BYTE* data,
		DWORD size
		);

	//解析データの現在のストリームＩＤを取得する
	//戻り値：
	// エラーコード
	// originalNetworkID		[OUT]現在のoriginalNetworkID
	// transportStreamID		[OUT]現在のtransportStreamID
	DWORD GetTSID(
		WORD* originalNetworkID,
		WORD* transportStreamID
		);

	//自ストリームのサービス一覧を取得する
	//戻り値：
	// エラーコード
	//引数：
	// serviceListSize			[OUT]serviceListの個数
	// serviceList				[OUT]サービス情報のリスト（DLL内で自動的にdeleteする。次に取得を行うまで有効）
	DWORD GetServiceListActual(
		DWORD* serviceListSize,
		SERVICE_INFO** serviceList
		);

	//蓄積されたEPG情報のあるサービス一覧を取得する
	//SERVICE_EXT_INFOの情報はない
	//戻り値：
	// エラーコード
	//引数：
	// serviceListSize			[OUT]serviceListの個数
	// serviceList				[OUT]サービス情報のリスト（DLL内で自動的にdeleteする。次に取得を行うまで有効）
	DWORD GetServiceListEpgDB(
		DWORD* serviceListSize,
		SERVICE_INFO** serviceList
		);

	//指定サービスの全EPG情報を取得する
	//戻り値：
	// エラーコード
	// originalNetworkID		[IN]取得対象のoriginalNetworkID
	// transportStreamID		[IN]取得対象のtransportStreamID
	// serviceID				[IN]取得対象のServiceID
	// epgInfoListSize			[OUT]epgInfoListの個数
	// epgInfoList				[OUT]EPG情報のリスト（DLL内で自動的にdeleteする。次に取得を行うまで有効）
	DWORD GetEpgInfoList(
		WORD originalNetworkID,
		WORD transportStreamID,
		WORD serviceID,
		DWORD* epgInfoListSize,
		EPG_EVENT_INFO** epgInfoList
		);

	//指定サービスの全EPG情報を列挙する
	//引数：
	// enumEpgInfoListProc		[IN]EPG情報のリストを取得するコールバック関数
	// param					[IN]コールバック引数
	DWORD EnumEpgInfoList(
		WORD originalNetworkID,
		WORD transportStreamID,
		WORD serviceID,
		BOOL (CALLBACK *enumEpgInfoListProc)(DWORD epgInfoListSize, EPG_EVENT_INFO* epgInfoList, LPVOID param),
		LPVOID param
		);

	//EPGデータの蓄積状態をリセットする
	void ClearSectionStatus();

	//EPGデータの蓄積状態を取得する
	//戻り値：
	// ステータス
	//引数：
	// l_eitFlag		[IN]L-EITのステータスを取得
	EPG_SECTION_STATUS GetSectionStatus(
		BOOL l_eitFlag
		);

	//指定サービスのEPGデータの蓄積状態を取得する
	//戻り値：
	// ステータス,取得できたかどうか
	//引数：
	// originalNetworkID		[IN]取得対象のOriginalNetworkID
	// transportStreamID		[IN]取得対象のTransportStreamID
	// serviceID				[IN]取得対象のServiceID
	// l_eitFlag				[IN]L-EITのステータスを取得
	pair<EPG_SECTION_STATUS, BOOL> GetSectionStatusService(
		WORD originalNetworkID,
		WORD transportStreamID,
		WORD serviceID,
		BOOL l_eitFlag
		);

	//指定サービスの現在or次のEPG情報を取得する
	//戻り値：
	// エラーコード
	//引数：
	// originalNetworkID		[IN]取得対象のoriginalNetworkID
	// transportStreamID		[IN]取得対象のtransportStreamID
	// serviceID				[IN]取得対象のServiceID
	// nextFlag					[IN]TRUE（次の番組）、FALSE（現在の番組）
	// epgInfo					[OUT]EPG情報（DLL内で自動的にdeleteする。次に取得を行うまで有効）
	DWORD GetEpgInfo(
		WORD originalNetworkID,
		WORD transportStreamID,
		WORD serviceID,
		BOOL nextFlag,
		EPG_EVENT_INFO** epgInfo
		);

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
	DWORD SearchEpgInfo(
		WORD originalNetworkID,
		WORD transportStreamID,
		WORD serviceID,
		WORD eventID,
		BYTE pfOnlyFlag,
		EPG_EVENT_INFO** epgInfo
		);

	//PC時計を元としたストリーム時間との差を取得する
	//戻り値：
	// 差の秒数
	int GetTimeDelay(
		);

protected:
	HMODULE module;
	DWORD id;

	InitializeEP3			pfnInitializeEP3;
	UnInitializeEP3			pfnUnInitializeEP3;
	AddTSPacketEP3			pfnAddTSPacketEP3;
	GetTSIDEP3				pfnGetTSIDEP3;
	GetEpgInfoListEP3		pfnGetEpgInfoListEP3;
	EnumEpgInfoListEP3		pfnEnumEpgInfoListEP3;
	ClearSectionStatusEP3	pfnClearSectionStatusEP3;
	GetSectionStatusEP3		pfnGetSectionStatusEP3;
	GetSectionStatusServiceEP3	pfnGetSectionStatusServiceEP3;
	GetServiceListActualEP3	pfnGetServiceListActualEP3;
	GetServiceListEpgDBEP3	pfnGetServiceListEpgDBEP3;
	GetEpgInfoEP3			pfnGetEpgInfoEP3;
	SearchEpgInfoEP3		pfnSearchEpgInfoEP3;
	GetTimeDelayEP3			pfnGetTimeDelayEP3;

protected:
	BOOL LoadDll(LPCWSTR loadDllFilePath);
	BOOL UnLoadDll(void);
};

