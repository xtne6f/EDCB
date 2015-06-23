#ifndef __REC_NAME_PLUGIN_H__
#define __REC_NAME_PLUGIN_H__

#include <Windows.h>
#include "../../Common/EpgDataCap3Def.h"

typedef struct _PLUGIN_RESERVE_INFO{
	SYSTEMTIME startTime;		//開始時間
	DWORD durationSec;			//総時間（単位秒）
	WCHAR eventName[512];		//番組名
	WORD ONID;					//originai_network_id
	WORD TSID;					//transport_stream_id
	WORD SID;					//service_id
	WORD EventID;				//evend_id（プログラム予約扱い時、0xFFFF）
	WCHAR serviceName[256];		//サービス名
	WCHAR bonDriverName[256];	//使用BonDriverファイル名
	DWORD bonDriverID;			//EpgTimerSrv内部でのBonDriver識別ID
	DWORD tunerID;				//EpgTimerSrv内部でのチューナー識別ID
	DWORD reserveID;			//予約ID（ConvertRecName3で必須）
	EPG_EVENT_INFO* epgInfo;	//番組情報（存在しないときNULL）（ConvertRecName3で必須）
	DWORD sizeOfStruct;			//未使用（0または構造体サイズで初期化）（ConvertRecName3で必須）
}PLUGIN_RESERVE_INFO;

//PlugInの名前を取得する
//nameがNULL時は必要なサイズをnameSizeで返す
//通常nameSize=256で呼び出し
//戻り値
// TRUE（成功）、FALSE（失敗）
//引数：
// name						[OUT]名称
// nameSize					[IN/OUT]nameのサイズ(WCHAR単位)
__declspec(dllexport)
BOOL WINAPI GetPlugInName(
	WCHAR* name,
	DWORD* nameSize
	);

//PlugInで設定が必要な場合、設定用のダイアログなどを表示する
//引数：
// parentWnd				[IN]親ウインドウ
__declspec(dllexport)
void WINAPI Setting(
	HWND parentWnd
	);

//入力された予約情報を元に、録画時のファイル名を作成する（拡張子含む）
//recNameがNULL時は必要なサイズをrecNamesizeで返す
//通常recNamesize=256で呼び出し
//戻り値
// TRUE（成功）、FALSE（失敗）
//引数：
// info						[IN]予約情報
// recName					[OUT]名称
// recNamesize				[IN/OUT]nameのサイズ(WCHAR単位)
__declspec(dllexport)
BOOL WINAPI ConvertRecName(
	PLUGIN_RESERVE_INFO* info,
	WCHAR* recName,
	DWORD* recNamesize
	);

//入力された予約情報を元に、録画時のファイル名を作成する（拡張子含む）
//recNameがNULL時は必要なサイズをrecNamesizeで返す
//通常recNamesize=256で呼び出し
//戻り値
// TRUE（成功）、FALSE（失敗）
//引数：
// info						[IN]予約情報
// epgInfo					[IN]番組情報（EPG予約で番組情報が存在する時、存在しない場合はNULL）
// recName					[OUT]名称
// recNamesize				[IN/OUT]nameのサイズ(WCHAR単位)
__declspec(dllexport)
BOOL WINAPI ConvertRecName2(
	PLUGIN_RESERVE_INFO* info,
	EPG_EVENT_INFO* epgInfo,
	WCHAR* recName,
	DWORD* recNamesize
	);

//入力された予約情報と変換パターンを元に、録画時のファイル名を作成する（拡張子含む）
//recNameがNULL時は必要なサイズをrecNamesizeで返す
//通常recNamesize=256で呼び出し
//戻り値
// TRUE（成功）、FALSE（失敗）
//引数：
// info						[IN]予約情報
// pattern					[IN]変換パターン（デフォルトのときNULL）
// recName					[OUT]名称
// recNamesize				[IN/OUT]nameのサイズ(WCHAR単位)
__declspec(dllexport)
BOOL WINAPI ConvertRecName3(
	PLUGIN_RESERVE_INFO* info,
	const WCHAR* pattern,
	WCHAR* recName,
	DWORD* recNamesize
	);

#endif
