#pragma once

#include <windows.h>

#include "../Common/Util.h"
#include "../Common/PathUtil.h"
#include "../Common/StringUtil.h"
#include "../Common/ErrDef.h"

#include "IBonDriver.h"
#include "IBonDriver2.h"
#include "BonCtrlDef.h"

class CBonDriverUtil
{
public:
	CBonDriverUtil(void);
	~CBonDriverUtil(void);

	//BonDriverフォルダを指定
	//引数：
	// bonDriverFolderPath		[IN]BonDriverフォルダパス
	void SetBonDriverFolder(
		LPCWSTR bonDriverFolderPath
		);

	//BonDriverフォルダのBonDriver_*.dllを列挙
	//戻り値：
	// エラーコード
	//引数：
	// bonList			[OUT]検索できたBonDriver一覧
	DWORD EnumBonDriver(
		vector<wstring>* bonList
		);

	//BonDriverをロードしてチャンネル情報などを取得（ファイル名で指定）
	//戻り値：
	// エラーコード
	//引数：
	// bonDriverFile	[IN]EnumBonDriverで取得されたBonDriverのファイル名
	DWORD OpenBonDriver(
		LPCWSTR bonDriverFile,
		int openWait = 200
		);

	//ロードしているBonDriverの開放
	//戻り値：
	// エラーコード
	DWORD CloseBonDriver();

	//ロードしたBonDriverの情報取得
	//SpaceとChの一覧を取得する
	//戻り値：
	// エラーコード
	//引数：
	// spaceMap			[OUT] SpaceとChの一覧（mapのキー Space）
	DWORD GetOriginalChList(
		map<DWORD, BON_SPACE_INFO>* spaceMap
	);

	//BonDriverのチューナー名を取得
	//戻り値：
	// チューナー名
	wstring GetTunerName();

	//チャンネル変更
	//戻り値：
	// エラーコード
	//引数：
	// space			[IN]変更チャンネルのSpace
	// ch				[IN]変更チャンネルの物理Ch
	DWORD SetCh(
		DWORD space,
		DWORD ch
		);

	//現在のチャンネル取得
	//戻り値：
	// エラーコード
	//引数：
	// space			[IN]現在のチャンネルのSpace
	// ch				[IN]現在のチャンネルの物理Ch
	DWORD GetNowCh(
		DWORD* space,
		DWORD* ch
		);

	//TSストリームを取得
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// data				[OUT]BonDriver内部バッファのポインタ
	// size				[OUT]取得バッファのサイズ
	// remain			[OUT]未取得バッファのサイズ
	BOOL GetTsStream(
		BYTE **data,
		DWORD *size,
		DWORD *remain
		);

	//シグナルレベルの取得
	//戻り値：
	// シグナルレベル
	float GetSignalLevel();

	//OpenしたBonDriverのファイル名を取得
	//戻り値：
	// BonDriverのファイル名（拡張子含む）（emptyで未Open）
	wstring GetOpenBonDriverFileName();

protected:
	HANDLE lockEvent;

	vector<wstring> bonDllList;

	wstring loadDllPath;
	wstring loadTunerName;
	map<DWORD, BON_SPACE_INFO> loadChMap;
	BOOL initChSetFlag;
	IBonDriver* bonIF;
	IBonDriver2* bon2IF;
	HMODULE module;
protected:
	//PublicAPI排他制御用
	BOOL Lock(LPCWSTR log = NULL, DWORD timeOut = 5*1000);
	void UnLock(LPCWSTR log = NULL);

	//BonDriverをロード時の本体
	//戻り値：
	// エラーコード
	//引数：
	// bonDriverFilePath		[IN] ロードするBonDriverのファイルパス
	DWORD _OpenBonDriver(
		LPCWSTR bonDriverFilePath,
		int openWait
		);

	//ロードしているBonDriverの開放の本体
	//戻り値：
	// エラーコード
	DWORD _CloseBonDriver();
};

