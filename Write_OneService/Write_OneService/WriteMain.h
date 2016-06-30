#pragma once

#include "../../Common/Util.h"
#include "../../Common/StringUtil.h"
#include "../../BonCtrl/PacketInit.h"
#include "../../BonCtrl/CreatePATPacket.h"
#include "../../BonCtrl/CATUtil.h"
#include "../../BonCtrl/PMTUtil.h"

class CWriteMain
{
public:
	CWriteMain();
	~CWriteMain();

	//ファイル保存を開始する
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// fileName				[IN]保存ファイルフルパス（必要に応じて拡張子変えたりなど行う）
	// overWriteFlag		[IN]同一ファイル名存在時に上書きするかどうか（TRUE：する、FALSE：しない）
	// createSize			[IN]入力予想容量（188バイトTSでの容量。即時録画時など総時間未定の場合は0。延長などの可能性もあるので目安程度）
	BOOL Start(
		LPCWSTR fileName,
		BOOL overWriteFlag,
		ULONGLONG createSize
		);

	//ファイル保存を終了する
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	BOOL Stop(
		);

	//実際に保存しているファイルパスを取得する（再生やバッチ処理に利用される）
	//戻り値：
	// 保存ファイルフルパス
	wstring GetSavePath(
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
	BOOL Write(
		BYTE* data,
		DWORD size,
		DWORD* writeSize
		);

private:
	struct WRITE_PLUGIN {
		HMODULE hDll;
		DWORD id;
		BOOL (WINAPI *pfnCreateCtrl)(DWORD*);
		BOOL (WINAPI *pfnDeleteCtrl)(DWORD);
		BOOL (WINAPI *pfnStartSave)(DWORD,LPCWSTR,BOOL,ULONGLONG);
		BOOL (WINAPI *pfnStopSave)(DWORD);
		BOOL (WINAPI *pfnGetSaveFilePath)(DWORD,WCHAR*,DWORD*);
		BOOL (WINAPI *pfnAddTSBuff)(DWORD,BYTE*,DWORD,DWORD*);
		BOOL Initialize(LPCWSTR path) {
			hDll = LoadLibrary(path);
			if( hDll ){
				if( (pfnCreateCtrl = (BOOL (WINAPI*)(DWORD*))(GetProcAddress(hDll, "CreateCtrl"))) != NULL &&
				    (pfnDeleteCtrl = (BOOL (WINAPI*)(DWORD))(GetProcAddress(hDll, "DeleteCtrl"))) != NULL &&
				    (pfnStartSave = (BOOL (WINAPI*)(DWORD,LPCWSTR,BOOL,ULONGLONG))(GetProcAddress(hDll, "StartSave"))) != NULL &&
				    (pfnStopSave = (BOOL (WINAPI*)(DWORD))(GetProcAddress(hDll, "StopSave"))) != NULL &&
				    (pfnGetSaveFilePath = (BOOL (WINAPI*)(DWORD,WCHAR*,DWORD*))(GetProcAddress(hDll, "GetSaveFilePath"))) != NULL &&
				    (pfnAddTSBuff = (BOOL (WINAPI*)(DWORD,BYTE*,DWORD,DWORD*))(GetProcAddress(hDll, "AddTSBuff"))) != NULL ){
					if( pfnCreateCtrl(&id) ){
						return TRUE;
					}
				}
				FreeLibrary(hDll);
				hDll = NULL;
			}
			return FALSE;
		}
	};
	HANDLE file;
	WRITE_PLUGIN writePlugin;
	wstring savePath;
	WORD targetSID;
	WORD lastTSID;
	CPacketInit packetInit;
	CCreatePATPacket patUtil;
	CCATUtil catUtil;
	map<WORD, CPMTUtil> pmtUtilMap;
	vector<WORD> needPIDList;
	vector<BYTE> outBuff;

	void AddNeedPID(WORD pid);
	void CheckNeedPID();
};

