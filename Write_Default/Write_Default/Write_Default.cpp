// Write_Default.cpp : DLL アプリケーション用にエクスポートされる関数を定義します。
//

#include "stdafx.h"

#include "WriteMain.h"
#include "../../Common/InstanceManager.h"
#include "../../Common/PathUtil.h"

CInstanceManager<CWriteMain> g_instMng;

#define PLUGIN_NAME L"デフォルト 188バイトTS出力 PlugIn"

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
		wstring size = GetPrivateProfileToString(L"SET", L"Size", L"770048", iniPath.c_str());
		wstring teeCmd = GetPrivateProfileToString(L"SET", L"TeeCmd", L"", iniPath.c_str());
		wstring teeSize = GetPrivateProfileToString(L"SET", L"TeeSize", L"770048", iniPath.c_str());
		wstring teeDelay = GetPrivateProfileToString(L"SET", L"TeeDelay", L"0", iniPath.c_str());
		CSettingDlg dlg;
		if( dlg.CreateSettingDialog(g_instance, parentWnd, size, teeCmd, teeSize, teeDelay) == IDOK ){
			WritePrivateProfileString(L"SET", L"Size", size.c_str(), iniPath.c_str());
			WritePrivateProfileString(L"SET", L"TeeCmd", (teeCmd.find(L'"') == wstring::npos ? teeCmd : L'"' + teeCmd + L'"').c_str(), iniPath.c_str());
			WritePrivateProfileString(L"SET", L"TeeSize", teeSize.c_str(), iniPath.c_str());
			WritePrivateProfileString(L"SET", L"TeeDelay", teeDelay.c_str(), iniPath.c_str());
		}
	}
}
#endif

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
	)
{
	if( id == NULL ){
		return FALSE;
	}

#ifdef _WIN32
	fs_path iniPath = GetModuleIniPath(g_instance);
#else
	fs_path iniPath = GetModuleIniPath((void*)CreateCtrl);
#endif
	DWORD buffSize = GetPrivateProfileInt(L"SET", L"Size", 770048, iniPath.c_str());
	DWORD teeSize = 0;
	DWORD teeDelay = 0;
	wstring teeCmd = GetPrivateProfileToString(L"SET", L"TeeCmd", L"", iniPath.c_str());
	if( teeCmd.empty() == false ){
		teeSize = GetPrivateProfileInt(L"SET", L"TeeSize", 770048, iniPath.c_str());
		teeDelay = GetPrivateProfileInt(L"SET", L"TeeDelay", 0, iniPath.c_str());
	}

	try{
		std::shared_ptr<CWriteMain> ptr = std::make_shared<CWriteMain>();
		ptr->SetBufferSize(buffSize);
		ptr->SetTeeCommand(teeCmd.c_str(), teeSize, teeDelay);
		*id = g_instMng.push(ptr);
	}catch( std::bad_alloc& ){
		return FALSE;
	}

	return TRUE;
}

//インスタンスを削除する
//戻り値
// TRUE（成功）、FALSE（失敗）
//引数：
// id				[IN]識別ID
DLL_EXPORT
BOOL WINAPI DeleteCtrl(
	DWORD id
	)
{
	if( g_instMng.pop(id) == NULL ){
		return FALSE;
	}

	return TRUE;
}

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
	)
{
	std::shared_ptr<CWriteMain> ptr = g_instMng.find(id);
	if( ptr == NULL ){
		return FALSE;
	}

	return ptr->Start(fileName, overWriteFlag, createSize);
}

//ファイル保存を終了する
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// id					[IN]識別ID
DLL_EXPORT
BOOL WINAPI StopSave(
	DWORD id
	)
{
	std::shared_ptr<CWriteMain> ptr = g_instMng.find(id);
	if( ptr == NULL ){
		return FALSE;
	}

	return ptr->Stop();
}

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
	)
{
	std::shared_ptr<CWriteMain> ptr = g_instMng.find(id);
	if( ptr == NULL ){
		return FALSE;
	}

	if( filePathSize == NULL ){
		return FALSE;
	}else if( filePath == NULL ){
		*filePathSize = (DWORD)ptr->GetSavePath().size() + 1;
	}else if( *filePathSize < (DWORD)ptr->GetSavePath().size() + 1 ){
		*filePathSize = (DWORD)ptr->GetSavePath().size() + 1;
		return FALSE;
	}else{
		wcscpy_s(filePath, *filePathSize, ptr->GetSavePath().c_str());
	}
	return TRUE;
}

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
	)
{
	std::shared_ptr<CWriteMain> ptr = g_instMng.find(id);
	if( ptr == NULL ){
		return FALSE;
	}

	return ptr->Write(data, size, writeSize);
}
