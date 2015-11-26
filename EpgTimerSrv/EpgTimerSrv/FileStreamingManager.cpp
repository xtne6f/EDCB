#include "StdAfx.h"
#include "FileStreamingManager.h"


CFileStreamingManager::CFileStreamingManager(void)
{
	this->lockEvent = CreateEvent(NULL, FALSE, TRUE, NULL);

	this->nextCtrlID = 1;
}


CFileStreamingManager::~CFileStreamingManager(void)
{
	map<DWORD, CFileStreamingUtil*>::iterator itr;
	for(itr = this->utilMap.begin(); itr != this->utilMap.end(); itr++ ){
		itr->second->StopSend();
		SAFE_DELETE(itr->second);
	}

	if( this->lockEvent != NULL ){
		UnLock();
		CloseHandle(this->lockEvent);
		this->lockEvent = NULL;
	}
}

BOOL CFileStreamingManager::Lock(LPCWSTR log, DWORD timeOut)
{
	if( this->lockEvent == NULL ){
		return FALSE;
	}
	//if( log != NULL ){
	//	_OutputDebugString(L"◆%s",log);
	//}
	DWORD dwRet = WaitForSingleObject(this->lockEvent, timeOut);
	if( dwRet == WAIT_ABANDONED || 
		dwRet == WAIT_FAILED ||
		dwRet == WAIT_TIMEOUT){
			OutputDebugString(L"◆CFileStreamingManager::Lock FALSE");
			if( log != NULL ){
				OutputDebugString(log);
			}
		return FALSE;
	}
	return TRUE;
}

void CFileStreamingManager::UnLock(LPCWSTR log)
{
	if( this->lockEvent != NULL ){
		SetEvent(this->lockEvent);
	}
	if( log != NULL ){
		OutputDebugString(log);
	}
}

//次に使用する制御IDを取得する
//戻り値：
// id
DWORD CFileStreamingManager::GetNextID()
{
	DWORD nextID = 0xFFFFFFFF;

	map<DWORD, CFileStreamingUtil*>::iterator itr;
	itr = this->utilMap.find(this->nextCtrlID);
	if( itr == this->utilMap.end() ){
		//存在しないIDなのでそのまま使用
		nextID = this->nextCtrlID;
		this->nextCtrlID++;
		if( this->nextCtrlID == 0 || this->nextCtrlID == 0xFFFFFFFF){
			this->nextCtrlID = 1;
		}
	}else{
		//一周した？
		for( DWORD i=1; i<0xFFFFFFFF; i++ ){
			//１から順番に存在しないIDを確認
			itr = this->utilMap.find(this->nextCtrlID);
			if( itr == this->utilMap.end() ){
				nextID = this->nextCtrlID;
				this->nextCtrlID++;
				if( this->nextCtrlID == 0 || this->nextCtrlID == 0xFFFFFFFF){
					this->nextCtrlID = 1;
				}
				break;
			}else{
				this->nextCtrlID++;
			}
			if( this->nextCtrlID == 0 || this->nextCtrlID == 0xFFFFFFFF){
				this->nextCtrlID = 1;
			}
		}
	}

	return nextID;
}

BOOL CFileStreamingManager::CloseAllFile(
	)
{
	if( Lock() == FALSE ) return FALSE;

	map<DWORD, CFileStreamingUtil*>::iterator itr;
	for( itr = this->utilMap.begin(); itr != this->utilMap.end(); itr++ ){
		itr->second->StopSend();
		SAFE_DELETE(itr->second);
	}
	this->utilMap.clear();

	UnLock();
	return TRUE;
}

BOOL CFileStreamingManager::IsStreaming()
{
	if( Lock() == FALSE ) return FALSE;

	BOOL ret = FALSE;
	if( this->utilMap.size() > 0 ){
		ret = TRUE;
	}

	UnLock();
	return ret;
}

BOOL CFileStreamingManager::OpenTimeShift(
	LPCWSTR filePath,
	DWORD processID,
	DWORD exeCtrlID,
	DWORD* ctrlID
	)
{
	if( Lock() == FALSE ) return FALSE;
	BOOL ret = TRUE;

	CFileStreamingUtil* util = new CFileStreamingUtil;
	if( util->OpenTimeShift(filePath, processID, exeCtrlID) == FALSE ){
		SAFE_DELETE(util);
		UnLock();
		return FALSE;
	}else{
		*ctrlID = GetNextID();
		this->utilMap.insert(pair<DWORD, CFileStreamingUtil*>(*ctrlID, util));
	}

	UnLock();
	return ret;
}

BOOL CFileStreamingManager::OpenFile(
	LPCWSTR filePath,
	DWORD* ctrlID
	)
{
	if( Lock() == FALSE ) return FALSE;
	BOOL ret = TRUE;

	CFileStreamingUtil* util = new CFileStreamingUtil;
	if( util->OpenFile(filePath) == FALSE ){
		SAFE_DELETE(util);
		UnLock();
		return FALSE;
	}else{
		*ctrlID = GetNextID();
		this->utilMap.insert(pair<DWORD, CFileStreamingUtil*>(*ctrlID, util));
	}

	UnLock();
	return ret;
}

BOOL CFileStreamingManager::CloseFile(
	DWORD ctrlID
	)
{
	if( Lock() == FALSE ) return FALSE;

	map<DWORD, CFileStreamingUtil*>::iterator itr;
	itr = this->utilMap.find(ctrlID);
	if( itr == this->utilMap.end() ){
		UnLock();
		return FALSE;
	}

	itr->second->StopSend();

	SAFE_DELETE(itr->second);
	this->utilMap.erase(itr);

	UnLock();
	return TRUE;
}

BOOL CFileStreamingManager::StartSend(
	DWORD ctrlID
	)
{
	if( Lock() == FALSE ) return FALSE;

	BOOL ret = TRUE;
	map<DWORD, CFileStreamingUtil*>::iterator itr;
	itr = this->utilMap.find(ctrlID);
	if( itr == this->utilMap.end() ){
		UnLock();
		return FALSE;
	}

	ret = itr->second->StartSend();

	UnLock();
	return ret;
}

BOOL CFileStreamingManager::StopSend(
	DWORD ctrlID
	)
{
	if( Lock() == FALSE ) return FALSE;

	BOOL ret = TRUE;
	map<DWORD, CFileStreamingUtil*>::iterator itr;
	itr = this->utilMap.find(ctrlID);
	if( itr == this->utilMap.end() ){
		UnLock();
		return FALSE;
	}

	ret = itr->second->StopSend();

	UnLock();
	return ret;
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
	if( Lock() == FALSE ) return FALSE;

	BOOL ret = TRUE;
	map<DWORD, CFileStreamingUtil*>::iterator itr;
	itr = this->utilMap.find(val->ctrlID);
	if( itr == this->utilMap.end() ){
		UnLock();
		return FALSE;
	}

	ret = itr->second->GetPos(val);

	UnLock();
	return ret;
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
	if( Lock() == FALSE ) return FALSE;

	BOOL ret = TRUE;
	map<DWORD, CFileStreamingUtil*>::iterator itr;
	itr = this->utilMap.find(val->ctrlID);
	if( itr == this->utilMap.end() ){
		UnLock();
		return FALSE;
	}

	ret = itr->second->SetPos(val);

	UnLock();
	return ret;
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
	if( Lock() == FALSE ) return FALSE;

	BOOL ret = TRUE;
	map<DWORD, CFileStreamingUtil*>::iterator itr;
	itr = this->utilMap.find(val->ctrlID);
	if( itr == this->utilMap.end() ){
		UnLock();
		return FALSE;
	}

	ret = itr->second->SetIP(val);

	UnLock();
	return ret;
}
