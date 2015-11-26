// RecName_Macro.cpp : DLL アプリケーション用にエクスポートされる関数を定義します。
//

#include "stdafx.h"


#include "RecName_PlugIn.h"
#include "SettingDlg.h"
#include "ConvertMacro2.h"
#include <tchar.h>

#define PLUGIN_NAME L"マクロ PlugIn"

extern HINSTANCE g_instance;

//PlugInの名前を取得する
//nameがNULL時は必要なサイズをnameSizeで返す
//通常nameSize=256で呼び出し
//戻り値
// TRUE（成功）、FALSE（失敗）
//引数：
// name						[OUT]名称
// nameSize					[IN/OUT]nameのサイズ(WCHAR単位)
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

//PlugInで設定が必要な場合、設定用のダイアログなどを表示する
//引数：
// parentWnd				[IN]親ウインドウ
void WINAPI Setting(
	HWND parentWnd
	)
{
	WCHAR dllPath[512] = L"";
	GetModuleFileName(g_instance, dllPath, 512);

	wstring iniPath = dllPath;
	iniPath += L".ini";

	WCHAR buff[1024] = L"";
	GetPrivateProfileString(L"SET", L"Macro", L"$Title$.ts", buff, 1024, iniPath.c_str());

	CSettingDlg dlg;
	dlg.macro = buff;
	if( dlg.CreateSettingDialog(g_instance, parentWnd) == IDOK ){
		WritePrivateProfileString(L"SET", L"Macro", dlg.macro.c_str(), iniPath.c_str());
	}
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
BOOL WINAPI ConvertRecName(
	PLUGIN_RESERVE_INFO* info,
	WCHAR* recName,
	DWORD* recNamesize
	)
{
	return ConvertRecName2(info, NULL, recName, recNamesize);
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
	WCHAR buff[1024];
	if( pattern == NULL ){
		wcscpy_s(buff, L"$Title$.ts");
		pattern = buff;
		WCHAR dllPath[512];
		DWORD dwRet = GetModuleFileName(g_instance, dllPath, 512);
		if( dwRet && dwRet < 512 ){
			GetPrivateProfileString(L"SET", L"Macro", L"$Title$.ts", buff, 1024, (wstring(dllPath) + L".ini").c_str());
		}
	}

	wstring convert = L"";
	CConvertMacro2 convertMacro;
	BOOL ret = convertMacro.Convert(pattern, info, convert);
	if( recName == NULL ){
		*recNamesize = (DWORD)convert.size()+1;
	}else{
		if( *recNamesize < (DWORD)convert.size()+1 ){
			*recNamesize = (DWORD)convert.size()+1;
			ret = FALSE;
		}else{
			wcscpy_s(recName, *recNamesize, convert.c_str());
		}
	}

	return ret;
}
