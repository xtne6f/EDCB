#include "StdAfx.h"
#include "WritePlugInUtil.h"

#include "Util.h"
#include "ErrDef.h"

CWritePlugInUtil::CWritePlugInUtil(void)
{
	module = NULL;

	this->id = 0;
}

CWritePlugInUtil::~CWritePlugInUtil(void)
{
	UnInitialize();
}

//DLLの初期化
//戻り値：
// エラーコード
//引数：
// loadDllFilePath		[IN]ロードするDLLパス
BOOL CWritePlugInUtil::Initialize(
	LPCWSTR loadDllFilePath
	)
{
	if( module != NULL ){
		return FALSE;
	}

	pfnGetPlugInNameWP = NULL;
	pfnSettingWP = NULL;
	pfnCreateCtrlWP = NULL;
	pfnDeleteCtrlWP = NULL;
	pfnStartSaveWP = NULL;
	pfnStopSaveWP = NULL;
	pfnGetSaveFilePathWP = NULL;
	pfnAddTSBuffWP = NULL;


	BOOL ret = TRUE;

	module = ::LoadLibrary(loadDllFilePath);

	if( module == NULL ){
		OutputDebugString(L"dlのロードに失敗しました\r\n");
		return FALSE;
	}

	pfnGetPlugInNameWP = ( GetPlugInNameWP ) ::GetProcAddress( module , "GetPlugInName");
	if( !pfnGetPlugInNameWP ){
		OutputDebugString(L"GetPlugInNameの GetProcAddress に失敗\r\n");
		ret = FALSE;
		goto ERR_END;
	}
	pfnSettingWP = ( SettingWP ) ::GetProcAddress( module , "Setting");
	if( !pfnSettingWP ){
		OutputDebugString(L"Settingの GetProcAddress に失敗\r\n");
		ret = FALSE;
		goto ERR_END;
	}
	pfnCreateCtrlWP = ( CreateCtrlWP ) ::GetProcAddress( module , "CreateCtrl");
	if( !pfnCreateCtrlWP ){
		OutputDebugString(L"CreateCtrlの GetProcAddress に失敗\r\n");
		ret = FALSE;
		goto ERR_END;
	}
	pfnDeleteCtrlWP = ( DeleteCtrlWP ) ::GetProcAddress( module , "DeleteCtrl");
	if( !pfnDeleteCtrlWP ){
		OutputDebugString(L"DeleteCtrlの GetProcAddress に失敗\r\n");
		ret = FALSE;
		goto ERR_END;
	}
	pfnStartSaveWP = ( StartSaveWP ) ::GetProcAddress( module , "StartSave");
	if( !pfnStartSaveWP ){
		OutputDebugString(L"StartSaveの GetProcAddress に失敗\r\n");
		ret = FALSE;
		goto ERR_END;
	}
	pfnStopSaveWP = ( StopSaveWP ) ::GetProcAddress( module , "StopSave");
	if( !pfnStopSaveWP ){
		OutputDebugString(L"StopSaveの GetProcAddress に失敗\r\n");
		ret = FALSE;
		goto ERR_END;
	}
	pfnGetSaveFilePathWP = ( GetSaveFilePathWP ) ::GetProcAddress( module , "GetSaveFilePath");
	if( !pfnGetSaveFilePathWP ){
		OutputDebugString(L"GetSaveFilePathの GetProcAddress に失敗\r\n");
		ret = FALSE;
		goto ERR_END;
	}
	pfnAddTSBuffWP = ( AddTSBuffWP ) ::GetProcAddress( module , "AddTSBuff");
	if( !pfnAddTSBuffWP ){
		OutputDebugString(L"AddTSBuffの GetProcAddress に失敗\r\n");
		ret = FALSE;
		goto ERR_END;
	}

	if( pfnCreateCtrlWP(&this->id) == FALSE ){
		this->id = 0;
		ret = FALSE;
		goto ERR_END;
	}

ERR_END:
	if( ret == FALSE ){
		::FreeLibrary( module );
		module=NULL;
	}
	return ret;
}

//DLLの開放
//戻り値：
// エラーコード
void CWritePlugInUtil::UnInitialize()
{
	if( module != NULL ){
		pfnDeleteCtrlWP(this->id);
		this->id = 0;
		::FreeLibrary( module );
	}
	module = NULL;
	
	pfnGetPlugInNameWP = NULL;
	pfnSettingWP = NULL;
	pfnCreateCtrlWP = NULL;
	pfnDeleteCtrlWP = NULL;
	pfnStartSaveWP = NULL;
	pfnStopSaveWP = NULL;
	pfnGetSaveFilePathWP = NULL;
	pfnAddTSBuffWP = NULL;

}

//PlugInの名前を取得する
//nameがNULL時は必要なサイズをnameSizeで返す
//通常nameSize=256で呼び出し
//戻り値
// TRUE（成功）、FALSE（失敗）
//引数：
// name						[OUT]名称
// nameSize					[IN/OUT]nameのサイズ(WCHAR単位)
BOOL CWritePlugInUtil::GetPlugInName(
	WCHAR* name,
	DWORD* nameSize
	)
{
	if( module == NULL ){
		return ERR_NOT_INIT;
	}

	return pfnGetPlugInNameWP(name, nameSize);
}

//PlugInで設定が必要な場合、設定用のダイアログなどを表示する
//引数：
// parentWnd				[IN]親ウインドウ
void CWritePlugInUtil::Setting(
	HWND parentWnd
	)
{
	if( module == NULL ){
		return ;
	}

	return pfnSettingWP(parentWnd);
}

//ファイル保存を開始する
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// fileName				[IN]保存ファイルフルパス（必要に応じて拡張子変えたりなど行う）
// overWriteFlag		[IN]同一ファイル名存在時に上書きするかどうか（TRUE：する、FALSE：しない）
// createSize			[IN]入力予想容量（188バイトTSでの容量。即時録画時など総時間未定の場合は0。延長などの可能性もあるので目安程度）
BOOL CWritePlugInUtil::StartSave(
	LPCWSTR fileName,
	BOOL overWriteFlag,
	ULONGLONG createSize
	)
{
	if( module == NULL ){
		return ERR_NOT_INIT;
	}

	BOOL ret = pfnStartSaveWP(this->id, fileName, overWriteFlag, createSize);

	return ret;
}

//ファイル保存を終了する
//戻り値：
// TRUE（成功）、FALSE（失敗）
BOOL CWritePlugInUtil::StopSave(
	)
{
	if( module == NULL ){
		return ERR_NOT_INIT;
	}

	BOOL ret = pfnStopSaveWP(this->id);

	return ret;
}

//実際に保存しているファイルパスを取得する（再生やバッチ処理に利用される）
//filePathがNULL時は必要なサイズをfilePathSizeで返す
//通常filePathSize=512で呼び出し
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// filePath				[OUT]保存ファイルフルパス
// filePathSize			[IN/OUT]filePathのサイズ(WCHAR単位)
BOOL CWritePlugInUtil::GetSaveFilePath(
	WCHAR* filePath,
	DWORD* filePathSize
	)
{
	if( module == NULL ){
		return ERR_NOT_INIT;
	}

	BOOL ret = pfnGetSaveFilePathWP(this->id, filePath, filePathSize);

	return ret;
}

//保存用TSデータを送る
//空き容量不足などで書き出し失敗した場合、writeSizeの値を元に
//再度保存処理するときの送信開始地点を決める
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// data					[IN]TSデータ
// size					[IN]dataのサイズ
// writeSize			[OUT]保存に利用したサイズ
BOOL CWritePlugInUtil::AddTSBuff(
	BYTE* data,
	DWORD size,
	DWORD* writeSize
	)
{
	if( module == NULL ){
		return ERR_NOT_INIT;
	}

	BOOL ret = pfnAddTSBuffWP(this->id, data, size, writeSize);

	return ret;
}

