#pragma once

#include <Windows.h>

//PlugInの名前を取得する
//nameがNULL時は必要なサイズをnameSizeで返す
//通常nameSize=256で呼び出し
//戻り値
// TRUE（成功）、FALSE（失敗）
//引数：
// name						[OUT]名称
// nameSize					[IN/OUT]nameのサイズ(WCHAR単位)
typedef BOOL (WINAPI* GetPlugInNameWP)(
	WCHAR* name,
	DWORD* nameSize
	);

//PlugInで設定が必要な場合、設定用のダイアログなどを表示する
//引数：
// parentWnd				[IN]親ウインドウ
typedef void (WINAPI* SettingWP)(
	HWND parentWnd
	);

//複数保存対応のためインスタンスを新規に作成する
//複数対応できない場合はこの時点でエラーとする
//戻り値
// TRUE（成功）、FALSE（失敗）
//引数：
// id				[OUT]識別ID
typedef BOOL (WINAPI* CreateCtrlWP)(
	DWORD* id
	);

//インスタンスを削除する
//戻り値
// TRUE（成功）、FALSE（失敗）
//引数：
// id				[IN]識別ID
typedef BOOL (WINAPI* DeleteCtrlWP)(
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
typedef BOOL (WINAPI* StartSaveWP)(
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
typedef BOOL (WINAPI* StopSaveWP)(
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
typedef BOOL (WINAPI* GetSaveFilePathWP)(
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
typedef BOOL (WINAPI* AddTSBuffWP)(
	DWORD id,
	BYTE* data,
	DWORD size,
	DWORD* writeSize
	);

class CWritePlugInUtil
{
public:
	CWritePlugInUtil(void);
	~CWritePlugInUtil(void);

	//DLLの初期化
	//戻り値：
	// エラーコード
	//引数：
	// loadDllFilePath		[IN]ロードするDLLパス
	BOOL Initialize(
		LPCWSTR loadDllFilePath
		);

	//DLLの開放
	//戻り値：
	// エラーコード
	void UnInitialize(
		);

	//PlugInの名前を取得する
	//nameがNULL時は必要なサイズをnameSizeで返す
	//通常nameSize=256で呼び出し
	//戻り値
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// name						[OUT]名称
	// nameSize					[IN/OUT]nameのサイズ(WCHAR単位)
	BOOL GetPlugInName(
		WCHAR* name,
		DWORD* nameSize
		);

	//PlugInで設定が必要な場合、設定用のダイアログなどを表示する
	//引数：
	// parentWnd				[IN]親ウインドウ
	void Setting(
		HWND parentWnd
		);

	//ファイル保存を開始する
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// fileName				[IN]保存ファイルフルパス（必要に応じて拡張子変えたりなど行う）
	// overWriteFlag		[IN]同一ファイル名存在時に上書きするかどうか（TRUE：する、FALSE：しない）
	// createSize			[IN]入力予想容量（188バイトTSでの容量。即時録画時など総時間未定の場合は0。延長などの可能性もあるので目安程度）
	BOOL StartSave(
		LPCWSTR fileName,
		BOOL overWriteFlag,
		ULONGLONG createSize
		);

	//ファイル保存を終了する
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	BOOL StopSave(
		);

	//実際に保存しているファイルパスを取得する（再生やバッチ処理に利用される）
	//filePathがNULL時は必要なサイズをfilePathSizeで返す
	//通常filePathSize=512で呼び出し
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// filePath				[OUT]保存ファイルフルパス
	// filePathSize			[IN/OUT]filePathのサイズ(WCHAR単位)
	BOOL GetSaveFilePath(
		WCHAR* filePath,
		DWORD* filePathSize
		);

	//保存用TSデータを送る
	//空き容量不足などで書き出し失敗した場合、writeSizeの値を元に
	//再度保存処理するときの送信開始地点を決める
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// data					[IN]TSデータ
	// size					[IN]dataのサイズ
	// writeSize			[OUT]保存に利用したサイズ
	BOOL AddTSBuff(
		BYTE* data,
		DWORD size,
		DWORD* writeSize
		);

protected:
	HMODULE module;
	DWORD id;

	GetPlugInNameWP				pfnGetPlugInNameWP;
	SettingWP					pfnSettingWP;
	CreateCtrlWP				pfnCreateCtrlWP;
	DeleteCtrlWP				pfnDeleteCtrlWP;
	StartSaveWP					pfnStartSaveWP;
	StopSaveWP					pfnStopSaveWP;
	GetSaveFilePathWP			pfnGetSaveFilePathWP;
	AddTSBuffWP					pfnAddTSBuffWP;

protected:
};

