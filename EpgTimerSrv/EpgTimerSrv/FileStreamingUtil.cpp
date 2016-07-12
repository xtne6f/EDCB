#include "StdAfx.h"
#include "FileStreamingUtil.h"
#include <process.h>

CFileStreamingUtil::CFileStreamingUtil(void)
{
}


CFileStreamingUtil::~CFileStreamingUtil(void)
{
}

BOOL CFileStreamingUtil::OpenFile(LPCWSTR filePath)
{
	return this->timeShiftUtil.OpenTimeShift(filePath, -1, TRUE);
}

BOOL CFileStreamingUtil::OpenTimeShift(LPCWSTR filePath, DWORD processID,DWORD exeCtrlID)
{
	//TODO: ここでfilePathの有効ファイルサイズを調べてfileSizeに格納する
	BOOL ret = FALSE;
	__int64 fileSize = 0;
	{
		ret = this->timeShiftUtil.OpenTimeShift(filePath, fileSize, FALSE);
	}

	return ret;
}

BOOL CFileStreamingUtil::StartSend()
{
	return this->timeShiftUtil.StartTimeShift();
}

BOOL CFileStreamingUtil::StopSend()
{
	return this->timeShiftUtil.StopTimeShift();
}

//ストリーム配信で現在の送信位置と総ファイルサイズを取得する
//戻り値：
// エラーコード
//引数：
// val				[IN/OUT]サイズ情報
BOOL CFileStreamingUtil::GetPos(
	NWPLAY_POS_CMD* val
	)
{
	this->timeShiftUtil.GetFilePos(&val->currentPos, &val->totalPos);
	return TRUE;
}

//ストリーム配信で送信位置をシークする
//戻り値：
// エラーコード
//引数：
// val				[IN]サイズ情報
BOOL CFileStreamingUtil::SetPos(
	NWPLAY_POS_CMD* val
	)
{
	this->timeShiftUtil.SetFilePos(val->currentPos);
	return TRUE;
}

//ストリーム配信で送信先を設定する
//戻り値：
// エラーコード
//引数：
// val				[IN]サイズ情報
BOOL CFileStreamingUtil::SetIP(
	NWPLAY_PLAY_INFO* val
	)
{
	return this->timeShiftUtil.Send(val);
}