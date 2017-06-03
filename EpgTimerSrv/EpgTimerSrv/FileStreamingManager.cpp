#include "StdAfx.h"
#include "FileStreamingManager.h"


void CFileStreamingManager::CloseAllFile(
	)
{
	this->utilMng.clear();
}

BOOL CFileStreamingManager::IsStreaming()
{
	return this->utilMng.empty() == false;
}

BOOL CFileStreamingManager::OpenTimeShift(
	LPCWSTR filePath,
	DWORD* ctrlID
	)
{
	std::shared_ptr<CTimeShiftUtil> util = std::make_shared<CTimeShiftUtil>();
	if( util->OpenTimeShift(filePath, FALSE) == FALSE ){
		return FALSE;
	}
	*ctrlID = this->utilMng.push(util);
	return TRUE;
}

BOOL CFileStreamingManager::OpenFile(
	LPCWSTR filePath,
	DWORD* ctrlID
	)
{
	std::shared_ptr<CTimeShiftUtil> util = std::make_shared<CTimeShiftUtil>();
	if( util->OpenTimeShift(filePath, TRUE) == FALSE ){
		return FALSE;
	}
	*ctrlID = this->utilMng.push(util);
	return TRUE;
}

BOOL CFileStreamingManager::CloseFile(
	DWORD ctrlID
	)
{
	std::shared_ptr<CTimeShiftUtil> util = this->utilMng.pop(ctrlID);
	return util != NULL;
}

BOOL CFileStreamingManager::StartSend(
	DWORD ctrlID
	)
{
	std::shared_ptr<CTimeShiftUtil> util = this->utilMng.find(ctrlID);
	if( util == NULL ){
		return FALSE;
	}
	return util->StartTimeShift();
}

BOOL CFileStreamingManager::StopSend(
	DWORD ctrlID
	)
{
	std::shared_ptr<CTimeShiftUtil> util = this->utilMng.find(ctrlID);
	if( util == NULL ){
		return FALSE;
	}
	return util->StopTimeShift();
}

//ストリーム配信で現在の送信位置と総ファイルサイズを取得する
//戻り値：
// エラーコード
//引数：
// val				[IN/OUT]サイズ情報
BOOL CFileStreamingManager::GetPos(
	NWPLAY_POS_CMD* val
	)
{
	std::shared_ptr<CTimeShiftUtil> util = this->utilMng.find(val->ctrlID);
	if( util == NULL ){
		return FALSE;
	}
	util->GetFilePos(&val->currentPos, &val->totalPos);
	return TRUE;
}

//ストリーム配信で送信位置をシークする
//戻り値：
// エラーコード
//引数：
// val				[IN]サイズ情報
BOOL CFileStreamingManager::SetPos(
	NWPLAY_POS_CMD* val
	)
{
	std::shared_ptr<CTimeShiftUtil> util = this->utilMng.find(val->ctrlID);
	if( util == NULL ){
		return FALSE;
	}
	util->SetFilePos(val->currentPos);
	return TRUE;
}

//ストリーム配信で送信先を設定する
//戻り値：
// エラーコード
//引数：
// val				[IN]サイズ情報
BOOL CFileStreamingManager::SetIP(
	NWPLAY_PLAY_INFO* val
	)
{
	std::shared_ptr<CTimeShiftUtil> util = this->utilMng.find(val->ctrlID);
	if( util == NULL ){
		return FALSE;
	}
	return util->Send(val);
}
