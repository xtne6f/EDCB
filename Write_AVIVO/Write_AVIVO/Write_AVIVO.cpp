// Write_AVIVO.cpp : DLL アプリケーション用にエクスポートされる関数を定義します。
//

#include "stdafx.h"

#include "Write_PlugIn.h"
#include "WriteMain.h"
#include "SettingDlg.h"
#include "../../Common/InstanceManager.h"

#include <tchar.h>
using namespace std;

CInstanceManager<CWriteMain> g_instMng;
extern HINSTANCE g_Instance;

#define PLUGIN_NAME L"ATI AVIVO Encode PlugIn"


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
	CSettingDlg dlg;
	dlg.CreateSettingDialog(g_Instance);
}

//複数保存対応のためインスタンスを新規に作成する
//複数対応できない場合はこの時点でエラーとする
//戻り値
// TRUE（成功）、FALSE（失敗）
//引数：
// id				[OUT]識別ID
BOOL WINAPI CreateCtrl(
	DWORD* id
	)
{
	if( id == NULL ){
		return FALSE;
	}

	try{
		std::shared_ptr<CWriteMain> ptr = std::make_shared<CWriteMain>();
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

	return ptr->_StartSave(fileName, overWriteFlag, createSize, g_Instance);
}

//ファイル保存を終了する
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// id					[IN]識別ID
BOOL WINAPI StopSave(
	DWORD id
	)
{
	std::shared_ptr<CWriteMain> ptr = g_instMng.find(id);
	if( ptr == NULL ){
		return FALSE;
	}

	return ptr->_StopSave();
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

	return ptr->_GetSaveFilePath(filePath, filePathSize);
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

	return ptr->_AddTSBuff(data, size, writeSize);
}



