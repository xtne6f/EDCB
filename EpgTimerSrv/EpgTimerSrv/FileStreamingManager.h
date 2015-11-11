#pragma once

#include "FileStreamingUtil.h"

class CFileStreamingManager
{
public:
	CFileStreamingManager(void);
	~CFileStreamingManager(void);

	BOOL CloseAllFile(
		);
	BOOL IsStreaming();

	BOOL OpenTimeShift(
		LPCWSTR filePath,
		DWORD processID,
		DWORD exeCtrlID,
		DWORD* ctrlID
		);
	BOOL OpenFile(
		LPCWSTR filePath,
		DWORD* ctrlID
		);
	BOOL CloseFile(
		DWORD ctrlID
		);
	BOOL StartSend(
		DWORD ctrlID
		);
	BOOL StopSend(
		DWORD ctrlID
		);

	//ストリーム配信で現在の送信位置と総ファイルサイズを取得する
	//戻り値：
	// エラーコード
	//引数：
	// val				[IN/OUT]サイズ情報
	BOOL GetPos(
		NWPLAY_POS_CMD* val
		);

	//ストリーム配信で送信位置をシークする
	//戻り値：
	// エラーコード
	//引数：
	// val				[IN]サイズ情報
	BOOL SetPos(
		NWPLAY_POS_CMD* val
		);

	//ストリーム配信で送信先を設定する
	//戻り値：
	// エラーコード
	//引数：
	// val				[IN]サイズ情報
	BOOL SetIP(
		NWPLAY_PLAY_INFO* val
		);

protected:
	HANDLE lockEvent;

	DWORD nextCtrlID;
	map<DWORD, CFileStreamingUtil*> utilMap;

protected:
	//PublicAPI排他制御用
	BOOL Lock(LPCWSTR log = NULL, DWORD timeOut = 20*1000);
	void UnLock(LPCWSTR log = NULL);

	//次に使用する制御IDを取得する
	//戻り値：
	// id
	DWORD GetNextID();
};

