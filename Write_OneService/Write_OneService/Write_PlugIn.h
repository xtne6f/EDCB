#ifndef __WRITE_PLUGIN_H__
#define __WRITE_PLUGIN_H__

#include <Windows.h>

#define DLL_EXPORT extern "C" __declspec(dllexport)

//PlugInの名前を取得する
//nameがNULL時は必要なサイズをnameSizeで返す
//通常nameSize=256で呼び出し
//戻り値
// TRUE（成功）、FALSE（失敗）
//引数：
// name						[OUT]名称
// nameSize					[IN/OUT]nameのサイズ(WCHAR単位)
DLL_EXPORT
BOOL WINAPI GetPlugInName(
	WCHAR* name,
	DWORD* nameSize
	);

//PlugInで設定が必要な場合、設定用のダイアログなどを表示する
//引数：
// parentWnd				[IN]親ウインドウ
DLL_EXPORT
void WINAPI Setting(
	HWND parentWnd
	);

//////////////////////////////////////////////////////////
//基本的な保存時のAPIの呼ばれ方
//CreateCtrl
//↓
//StartSave
//↓
//GetSaveFilePath
//↓
//AddTSBuff（ループ）
//↓（録画時間終わった）
//StopSave
//↓
//DeleteCtrl
//
//AddTSBuffでFALSEが返ってきた場合（空き容量なくなったなど）
//AddTSBuff
//↓（FALSE）
//StopSave
//↓
//StartSave
//↓
//GetSaveFilePath
//↓
//AddTSBuff（ループ）
//↓（録画時間終わった）
//StopSave
//↓
//DeleteCtrl


//複数保存対応のためインスタンスを新規に作成する
//複数対応できない場合はこの時点でエラーとする
//戻り値
// TRUE（成功）、FALSE（失敗）
//引数：
// id				[OUT]識別ID
DLL_EXPORT
BOOL WINAPI CreateCtrl(
	DWORD* id
	);

//インスタンスを削除する
//戻り値
// TRUE（成功）、FALSE（失敗）
//引数：
// id				[IN]識別ID
DLL_EXPORT
BOOL WINAPI DeleteCtrl(
	DWORD id
	);

//ファイル保存を開始する
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// id					[IN]識別ID
// fileName				[IN]保存ファイルフルパス（必要に応じて拡張子変えたりなど行う）
// overWriteFlag		[IN]同一ファイル名存在時に上書きするかどうか（TRUE：する、FALSE：しない）
// createSize			[IN]入力予想容量（188バイトTSでの容量。即時録画時など総時間未定の場合は0。延長などの可能性もあるので目安程度）
DLL_EXPORT
BOOL WINAPI StartSave(
	DWORD id,
	LPCWSTR fileName,
	BOOL overWriteFlag,
	ULONGLONG createSize
	);

//ファイル保存を終了する
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// id					[IN]識別ID
DLL_EXPORT
BOOL WINAPI StopSave(
	DWORD id
	);

//実際に保存しているファイルパスを取得する（再生やバッチ処理に利用される）
//filePathがNULL時は必要なサイズをfilePathSizeで返す
//通常filePathSize=512で呼び出し
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// id					[IN]識別ID
// filePath				[OUT]保存ファイルフルパス
// filePathSize			[IN/OUT]filePathのサイズ(WCHAR単位)
DLL_EXPORT
BOOL WINAPI GetSaveFilePath(
	DWORD id,
	WCHAR* filePath,
	DWORD* filePathSize
	);

//保存用TSデータを送る
//空き容量不足などで書き出し失敗した場合、writeSizeの値を元に
//再度保存処理するときの送信開始地点を決める
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// id					[IN]識別ID
// data					[IN]TSデータ
// size					[IN]dataのサイズ
// writeSize			[OUT]保存に利用したサイズ
DLL_EXPORT
BOOL WINAPI AddTSBuff(
	DWORD id,
	BYTE* data,
	DWORD size,
	DWORD* writeSize
	);

#endif
