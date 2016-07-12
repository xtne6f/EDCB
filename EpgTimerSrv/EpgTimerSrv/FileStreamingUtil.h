#pragma once

#include "../../Common/TimeShiftUtil.h"
#include "../../Common/StringUtil.h"
#include "../../Common/PathUtil.h"
#include "../../Common/SendCtrlCmd.h"
#include "../../BonCtrl/SendUDP.h"
#include "../../BonCtrl/SendTCP.h"
#include "../../Common/SendCtrlCmd.h"

class CFileStreamingUtil
{
public:
	CFileStreamingUtil(void);
	~CFileStreamingUtil(void);

	BOOL OpenFile(LPCWSTR filePath);
	BOOL OpenTimeShift(LPCWSTR filePath, DWORD processID,DWORD exeCtrlID);
	BOOL StartSend();
	BOOL StopSend();

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
	CTimeShiftUtil timeShiftUtil;
};

