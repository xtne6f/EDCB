#pragma once

#include "EpgDataCap3Def.h"

class CReNamePlugInUtil
{
public:
	CReNamePlugInUtil();

#ifdef _WIN32
	//PlugInで設定が必要な場合、設定用のダイアログなどを表示する
	//戻り値
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// dllPath					[IN]プラグインDLLパス
	// parentWnd				[IN]親ウインドウ
	static BOOL ShowSetting(
		const wstring& dllPath,
		HWND parentWnd
		);
#endif

	//入力された予約情報と変換パターンを元に、録画時のファイル名を作成する（拡張子含む）
	//recNameがNULL時は必要なサイズをrecNamesizeで返す
	//通常recNamesize=256で呼び出し
	//プラグインのバージョンに応じてConvertRecName3→2→1の順に互換呼び出しを行う
	//スレッドセーフではない
	//戻り値
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// info						[IN]予約情報
	// dllPattern				[IN]プラグインDLL名、および最初の'?'に続けて変換パターン
	// dllFolder				[IN]プラグインDLLフォルダパス(dllPatternがそのまま連結される)
	// recName					[OUT]名称
	// recNamesize				[IN/OUT]nameのサイズ(WCHAR単位)
	BOOL Convert(
		PLUGIN_RESERVE_INFO* info,
		const WCHAR* dllPattern,
		const WCHAR* dllFolder,
		WCHAR* recName,
		DWORD* recNamesize
		);

	//Convert()で開いたリソースがあれば閉じる
	void CloseConvert();

private:
#ifdef _WIN32
	typedef void (WINAPI* SettingRNP)(HWND parentWnd);
#endif
	typedef BOOL (WINAPI* ConvertRecNameRNP)(PLUGIN_RESERVE_INFO* info, WCHAR* recName, DWORD* recNamesize);
	typedef BOOL (WINAPI* ConvertRecName2RNP)(PLUGIN_RESERVE_INFO* info, EPG_EVENT_INFO* epgInfo, WCHAR* recName, DWORD* recNamesize);
	typedef BOOL (WINAPI* ConvertRecName3RNP)(PLUGIN_RESERVE_INFO* info, const WCHAR* pattern, WCHAR* recName, DWORD* recNamesize);

	std::unique_ptr<void, void(*)(void*)> m_moduleHolder;
};
