// RecName_Macro.cpp : DLL アプリケーション用にエクスポートされる関数を定義します。
//

#include "stdafx.h"
#include "ConvertMacro2.h"
#include "../../Common/PathUtil.h"

#define PLUGIN_NAME L"マクロ PlugIn"
#ifdef _WIN32
#include "SettingDlg.h"
#define DLL_EXPORT extern "C" __declspec(dllexport)
extern HINSTANCE g_instance;
#else
#define DLL_EXPORT extern "C"
#endif

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
	)
{
	if( name == NULL ){
		if( nameSize == NULL ){
			return FALSE;
		}else{
			*nameSize = (DWORD)wcslen(PLUGIN_NAME)+1;
		}
	}else{
		if( nameSize == NULL ){
			return FALSE;
		}else{
			if( *nameSize < (DWORD)wcslen(PLUGIN_NAME)+1 ){
				*nameSize = (DWORD)wcslen(PLUGIN_NAME)+1;
				return FALSE;
			}else{
				wcscpy_s(name, *nameSize, PLUGIN_NAME);
			}
		}
	}
	return TRUE;
}

#ifdef _WIN32
//PlugInで設定が必要な場合、設定用のダイアログなどを表示する
//引数：
// parentWnd				[IN]親ウインドウ
DLL_EXPORT
void WINAPI Setting(
	HWND parentWnd
	)
{
	{
		fs_path iniPath = GetModuleIniPath(g_instance);
		wstring macro = GetPrivateProfileToString(L"SET", L"Macro", L"$Title$.ts", iniPath.c_str());
		CSettingDlg dlg;
		if( dlg.CreateSettingDialog(g_instance, parentWnd, macro) == IDOK ){
			TouchFileAsUnicode(iniPath);
			WritePrivateProfileString(L"SET", L"Macro", macro.c_str(), iniPath.c_str());
		}
	}
}
#endif

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
DLL_EXPORT
BOOL WINAPI ConvertRecName3(
	PLUGIN_RESERVE_INFO* info,
	const WCHAR* pattern,
	WCHAR* recName,
	DWORD* recNamesize
	)
{
	if( recNamesize == NULL ){
		return FALSE;
	}
	wstring buff;
	if( pattern == NULL ){
#ifdef _WIN32
		fs_path iniPath = GetModuleIniPath(g_instance);
#else
		fs_path iniPath = GetModuleIniPath((void*)ConvertRecName3);
#endif
		buff = GetPrivateProfileToString(L"SET", L"Macro", L"$Title$.ts", iniPath.c_str());
		pattern = buff.c_str();
	}

	wstring convert = CConvertMacro2::Convert(pattern, info);
	if( recName == NULL ){
		*recNamesize = (DWORD)convert.size()+1;
	}else{
		if( *recNamesize < (DWORD)convert.size()+1 ){
			*recNamesize = (DWORD)convert.size()+1;
			return FALSE;
		}else{
			wcscpy_s(recName, *recNamesize, convert.c_str());
		}
	}

	return TRUE;
}

//入力された予約情報を元に、録画時のファイル名を作成する（拡張子含む）
//recNameがNULL時は必要なサイズをrecNamesizeで返す
//通常recNamesize=256で呼び出し
//戻り値
// TRUE（成功）、FALSE（失敗）
//引数：
// info						[IN]予約情報
// epgInfo					[IN]番組情報（EPG予約で番組情報が存在する時、存在しない場合のNULL）
// recName					[OUT]名称
// recNamesize				[IN/OUT]nameのサイズ(WCHAR単位)
DLL_EXPORT
BOOL WINAPI ConvertRecName2(
	PLUGIN_RESERVE_INFO* info,
	EPG_EVENT_INFO* epgInfo,
	WCHAR* recName,
	DWORD* recNamesize
	)
{
	PLUGIN_RESERVE_INFO infoEx;
	memcpy(&infoEx, info, offsetof(PLUGIN_RESERVE_INFO, tunerID) + sizeof(infoEx.tunerID));
	infoEx.reserveID = 0;
	infoEx.epgInfo = epgInfo;
	infoEx.sizeOfStruct = 0;
	return ConvertRecName3(&infoEx, NULL, recName, recNamesize);
}

//入力された予約情報を元に、録画時のファイル名を作成する（拡張子含む）
//recNameがNULL時は必要なサイズをrecNamesizeで返す
//通常recNamesize=256で呼び出し
//戻り値
// TRUE（成功）、FALSE（失敗）
//引数：
// info						[IN]予約情報
// recName					[OUT]名称
// recNamesize				[IN/OUT]nameのサイズ(WCHAR単位)
DLL_EXPORT
BOOL WINAPI ConvertRecName(
	PLUGIN_RESERVE_INFO* info,
	WCHAR* recName,
	DWORD* recNamesize
	)
{
	return ConvertRecName2(info, NULL, recName, recNamesize);
}
