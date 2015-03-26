#ifndef __EPG_DATA_CAP3_H__
#define __EPG_DATA_CAP3_H__

#include "../../Common/EpgDataCap3Def.h"

//DLLの初期化
//戻り値：
// エラーコード
//引数：
// asyncFlag		[IN]予約（必ずFALSEを渡すこと）
// id				[OUT]識別ID
__declspec(dllexport)
DWORD WINAPI InitializeEP(
	BOOL asyncFlag,
	DWORD* id
	);

//DLLの開放
//戻り値：
// エラーコード
//引数：
// id		[IN]識別ID
__declspec(dllexport)
DWORD WINAPI UnInitializeEP(
	DWORD id
	);

//解析対象のTSパケット１つを読み込ませる
//戻り値：
// エラーコード
//引数：
// id		[IN]識別ID
// data		[IN]TSパケット１つ
// size		[IN]dataのサイズ（188でなければならない）
__declspec(dllexport)
DWORD WINAPI AddTSPacketEP(
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
__declspec(dllexport)
DWORD WINAPI GetTSIDEP(
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
__declspec(dllexport)
DWORD WINAPI GetServiceListActualEP(
	DWORD id,
	DWORD* serviceListSize,
	SERVICE_INFO** serviceList
	);

//蓄積されたEPG情報のあるサービス一覧を取得する
//SERVICE_EXT_INFOの情報はない場合がある
//戻り値：
// エラーコード
//引数：
// id						[IN]識別ID
// serviceListSize			[OUT]serviceListの個数
// serviceList				[OUT]サービス情報のリスト（DLL内で自動的にdeleteする。次に取得を行うまで有効）
__declspec(dllexport)
DWORD WINAPI GetServiceListEpgDBEP(
	DWORD id,
	DWORD* serviceListSize,
	SERVICE_INFO** serviceList
	);

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
__declspec(dllexport)
DWORD WINAPI GetEpgInfoListEP(
	DWORD id,
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	DWORD* epgInfoListSize,
	EPG_EVENT_INFO** epgInfoList
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
__declspec(dllexport)
DWORD WINAPI GetEpgInfoEP(
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
__declspec(dllexport)
DWORD WINAPI SearchEpgInfoEP(
	DWORD id,
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	WORD eventID,
	BYTE pfOnlyFlag,
	EPG_EVENT_INFO** epgInfo
	);

//EPGデータの蓄積状態をリセットする
//引数：
// id						[IN]識別ID
__declspec(dllexport)
void WINAPI ClearSectionStatusEP(
	DWORD id
	);

//EPGデータの蓄積状態を取得する
//戻り値：
// ステータス
//引数：
// id						[IN]識別ID
// l_eitFlag				[IN]L-EITのステータスを取得
__declspec(dllexport)
EPG_SECTION_STATUS WINAPI GetSectionStatusEP(
	DWORD id,
	BOOL l_eitFlag
	);

//PC時計を元としたストリーム時間との差を取得する
//戻り値：
// 差の秒数
//引数：
// id						[IN]識別ID
__declspec(dllexport)
int WINAPI GetTimeDelayEP(
	DWORD id
	);

#endif
