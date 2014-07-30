#include "StdAfx.h"
#include "WriteTSFile.h"
#include <process.h>

#include "../Common/PathUtil.h"
#include "../Common/BlockLock.h"

CWriteTSFile::CWriteTSFile(void)
{
	InitializeCriticalSection(&this->outThreadLock);
	this->totalTSBuffSize = 0;

    this->outThread = NULL;
    this->outStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	this->writeTotalSize = 0;
	this->maxBuffCount = -1;
	this->buffOverErr = FALSE;
}

CWriteTSFile::~CWriteTSFile(void)
{
	if( this->outThread != NULL ){
		::SetEvent(this->outStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->outThread, 10000) == WAIT_TIMEOUT ){
			::TerminateThread(this->outThread, 0xffffffff);
		}
		CloseHandle(this->outThread);
		this->outThread = NULL;
	}
	if( this->outStopEvent != NULL ){
		CloseHandle(this->outStopEvent);
		this->outStopEvent = NULL;
	}


	DeleteCriticalSection(&this->outThreadLock);
	for( size_t i=0; i<this->TSBuff.size(); i++ ){
		SAFE_DELETE(this->TSBuff[i])
	}
	this->TSBuff.clear();

	for( size_t i=0; i<this->fileList.size(); i++ ){
		SAFE_DELETE(this->fileList[i]);
	}
	this->fileList.clear();
}

//ファイル保存を開始する
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// fileName				[IN]保存ファイル名
// overWriteFlag		[IN]同一ファイル名存在時に上書きするかどうか（TRUE：する、FALSE：しない）
// createSize			[IN]ファイル作成時にディスクに予約する容量
// saveFolder			[IN]使用するフォルダ一覧
// saveFolderSub		[IN]HDDの空きがなくなった場合に一時的に使用するフォルダ
BOOL CWriteTSFile::StartSave(
	wstring fileName,
	BOOL overWriteFlag,
	ULONGLONG createSize,
	vector<REC_FILE_SET_INFO>* saveFolder,
	vector<wstring>* saveFolderSub,
	int maxBuffCount
)
{
	BOOL ret = TRUE;

	this->exceptionErr = FALSE;
	this->buffOverErr = FALSE;
	this->maxBuffCount = maxBuffCount;

	if( saveFolder->size() == 0 ){
		_OutputDebugString(L"CWriteTSFile::StartSave Err saveFolder 0");
		return FALSE;
	}
	
	if( this->outThread == NULL && this->fileList.size() == 0 ){
		this->writeTotalSize = 0;
		this->subRecFlag = FALSE;
		this->saveFileName = fileName;
		this->overWriteFlag = overWriteFlag;
		this->saveFolder = *saveFolder;
		this->saveFolderSub = *saveFolderSub;
		BOOL firstFreeChek = TRUE;
		for( size_t i=0; i<this->saveFolder.size(); i++ ){
			SAVE_INFO* item = new SAVE_INFO;
			if( this->saveFolder[i].writePlugIn.size() == 0 ){
				this->saveFolder[i].writePlugIn = L"Write_Default.dll";
			}
			wstring plugInPath = L"";
			GetModuleFolderPath(plugInPath);
			plugInPath += L"\\Write\\";
			plugInPath += this->saveFolder[i].writePlugIn.c_str();

			item->writeUtil = new CWritePlugInUtil;
			if(item->writeUtil->Initialize(plugInPath.c_str() ) == FALSE ){
				_OutputDebugString(L"CWriteTSFile::StartSave Err 3");
				SAFE_DELETE(item);
			}else{
				wstring folderPath = this->saveFolder[i].recFolder;
				if( CompareNoCase(this->saveFolder[i].writePlugIn, L"Write_Default.dll" ) == 0 ){
					//デフォルトの場合は空き容量をあらかじめチェック
					if( createSize > 0 ){
						if( ChkFreeFolder(createSize, this->saveFolder[i].recFolder) == FALSE ){
							if( GetFreeFolder(createSize, folderPath) == TRUE ){
								//空きなかったのでサブフォルダに録画
								this->subRecFlag = TRUE;
							}
						}
					}
				}
				ChkFolderPath(folderPath);

				wstring recPath;
				recPath = folderPath;
				recPath += L"\\";
				if( this->saveFolder[i].recFileName.size() == 0 ){
					recPath += fileName;
					item->recFileName = fileName;
				}else{
					recPath += this->saveFolder[i].recFileName;
					item->recFileName = this->saveFolder[i].recFileName;
				}
				//開始
				BOOL startRes = item->writeUtil->StartSave(recPath.c_str(), this->overWriteFlag, createSize);
				if( startRes == FALSE ){
					_OutputDebugString(L"CWriteTSFile::StartSave Err 2");
					//エラー時サブフォルダでリトライ
					if( GetFreeFolder(createSize, folderPath) == TRUE ){
						//空きなかったのでサブフォルダに録画
						this->subRecFlag = TRUE;
					}
					ChkFolderPath(folderPath);
					recPath = folderPath;
					recPath += L"\\";
					if( this->saveFolder[i].recFileName.size() == 0 ){
						recPath += fileName;
						item->recFileName = fileName;
					}else{
						recPath += this->saveFolder[i].recFileName;
						item->recFileName = this->saveFolder[i].recFileName;
					}
					startRes = item->writeUtil->StartSave(recPath.c_str(), this->overWriteFlag, createSize);
				}
				if( startRes == TRUE ){
					WCHAR saveFilePath[512] = L"";
					DWORD saveFilePathSize = 512;
					item->writeUtil->GetSaveFilePath(saveFilePath, &saveFilePathSize);

					item->recFilePath = saveFilePath;
					item->freeChk = firstFreeChek;
					item->overWriteFlag = overWriteFlag;
					this->fileList.push_back(item);
					if( i==0 ){
						this->mainSaveFilePath = saveFilePath;
					}
					firstFreeChek = FALSE;
				}else{
					SAFE_DELETE(item);
				}
			}
		}

		if( this->fileList.size() > 0 ){
			//受信スレッド起動
			ResetEvent(this->outStopEvent);
			this->outThread = (HANDLE)_beginthreadex(NULL, 0, OutThread, (LPVOID)this, CREATE_SUSPENDED, NULL);
			SetThreadPriority( this->outThread, THREAD_PRIORITY_NORMAL );
			ResumeThread(this->outThread);
		}else{
			_OutputDebugString(L"CWriteTSFile::StartSave Err fileList 0");
			ret = FALSE;
		}
	}else{
		_OutputDebugString(L"CWriteTSFile::StartSave Err 1");
		ret = FALSE;
	}

	return ret;
}

//保存サブフォルダから空きのあるフォルダパスを取得
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// needFreeSize			[IN]最低必要な空きサイズ
// freeFolderPath		[OUT]見つかったフォルダ
BOOL CWriteTSFile::GetFreeFolder(
	ULONGLONG needFreeSize,
	wstring& freeFolderPath
)
{
	BOOL ret = FALSE;

	for( int i = 0; i < (int)this->saveFolderSub.size(); i++ ){
		ULARGE_INTEGER stFree;
		ULARGE_INTEGER stTotal;
		ULARGE_INTEGER stTotalFree;
		if( _GetDiskFreeSpaceEx( this->saveFolderSub[i].c_str(), &stFree, &stTotal, &stTotalFree ) == TRUE ){
			if( stFree.QuadPart > (ULONGLONG)needFreeSize ){
				freeFolderPath = this->saveFolderSub[i];
				ChkFolderPath(freeFolderPath);
				ret = TRUE;
				break;
			}
		}
	}
	return ret;
}

//指定フォルダの空きがあるかチェック
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// needFreeSize			[IN]最低必要な空きサイズ
// chkFolderPath		[IN]指定フォルダ
BOOL CWriteTSFile::ChkFreeFolder(
	ULONGLONG needFreeSize,
	wstring chkFolderPath
)
{
	BOOL ret = FALSE;

	ULARGE_INTEGER stFree;
	ULARGE_INTEGER stTotal;
	ULARGE_INTEGER stTotalFree;
	if( _GetDiskFreeSpaceEx( chkFolderPath.c_str(), &stFree, &stTotal, &stTotalFree ) == TRUE ){
		if( stFree.QuadPart > (ULONGLONG)needFreeSize ){
			ret = TRUE;
		}
	}
	return ret;
}

//ファイル保存を終了する
//戻り値：
// TRUE（成功）、FALSE（失敗）
BOOL CWriteTSFile::EndSave()
{
	BOOL ret = TRUE;

	if( this->outThread != NULL ){
		::SetEvent(this->outStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->outThread, 10000) == WAIT_TIMEOUT ){
			::TerminateThread(this->outThread, 0xffffffff);
		}
		CloseHandle(this->outThread);
		this->outThread = NULL;
	}

	//残っているバッファを書き出し
	{
		CBlockLock lock(&this->outThreadLock);
		for( size_t i=0; i<this->TSBuff.size(); i++ ){
			for( size_t j=0; j<this->fileList.size(); j++ ){
				if( this->fileList[j]->writeUtil != NULL ){
					DWORD write = 0;
					this->fileList[j]->writeUtil->AddTSBuff( this->TSBuff[i]->data, this->TSBuff[i]->size, &write);
				}
			}
			SAFE_DELETE(this->TSBuff[i]);
		}
		this->TSBuff.clear();
		this->totalTSBuffSize = 0;
	}

	for( size_t i=0; i<this->fileList.size(); i++ ){
		if( this->fileList[i]->writeUtil != NULL ){
			this->fileList[i]->writeUtil->StopSave();
			this->fileList[i]->writeUtil->UnInitialize();
			SAFE_DELETE(this->fileList[i]);
		}
	}
	this->fileList.clear();

	if( this->buffOverErr == TRUE ){
		ret = FALSE;
	}
	if( this->exceptionErr == TRUE ){
		ret = FALSE;
	}

	return ret;
}

//出力用TSデータを送る
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// data		[IN]TSデータ
// size		[IN]dataのサイズ
BOOL CWriteTSFile::AddTSBuff(
	BYTE* data,
	DWORD size
	)
{
	if( data == NULL || size == 0 || this->outThread == NULL){
		return FALSE;
	}

	BOOL ret = TRUE;

	TS_DATA* item = new TS_DATA;
	item->size = size;
	item->data = new BYTE[size];
	memcpy(item->data, data, size);
	{
		CBlockLock lock(&this->outThreadLock);
		if( this->maxBuffCount > 0 ){
			if( this->totalTSBuffSize / 48128 > (DWORD)this->maxBuffCount ){
				_OutputDebugString(L"★writeBuffList MaxOver");
				while( this->TSBuff.empty() == false && this->totalTSBuffSize / 48128 + 1000 > (DWORD)this->maxBuffCount ){
					this->totalTSBuffSize -= this->TSBuff.back()->size;
					SAFE_DELETE(this->TSBuff.back());
					this->TSBuff.pop_back();
				}
			}
		}
		this->TSBuff.push_back(item);
		this->totalTSBuffSize += this->TSBuff.back()->size;
	}
	return ret;
}

UINT WINAPI CWriteTSFile::OutThread(LPVOID param)
{
	CWriteTSFile* sys = (CWriteTSFile*)param;
	while(1){
		if( ::WaitForSingleObject(sys->outStopEvent, 0) != WAIT_TIMEOUT ){
			//キャンセルされた
			break;
		}
		//バッファからデータ取り出し
		TS_DATA* data = NULL;
		{
			CBlockLock lock(&sys->outThreadLock);
			if( sys->TSBuff.empty() == false ){
				data = sys->TSBuff.front();
				sys->totalTSBuffSize -= data->size;
				sys->TSBuff.erase(sys->TSBuff.begin());
			}
		}

		if( data != NULL ){
			for( size_t i=0; i<sys->fileList.size(); i++ ){
				try{
					if( sys->fileList[i]->writeUtil != NULL ){
						DWORD write = 0;
						if( sys->fileList[i]->writeUtil->AddTSBuff( data->data, data->size, &write) == FALSE ){
							//空きがなくなった
							{
								CBlockLock lock(&sys->outThreadLock);
								sys->writeTotalSize = -1;
							}
							sys->fileList[i]->writeUtil->StopSave();

							if( sys->fileList[i]->freeChk == TRUE ){
								//次の空きを探す
								wstring freeFolderPath = L"";
								if( sys->GetFreeFolder(200*1024*1024, freeFolderPath) == TRUE ){
									wstring recFilePath = freeFolderPath;
									recFilePath += L"\\";
									recFilePath += sys->fileList[i]->recFileName;

									//開始
									if( sys->fileList[i]->writeUtil->StartSave(recFilePath.c_str(), sys->fileList[i]->overWriteFlag, 0) == FALSE ){
										//失敗したので終わり
										SAFE_DELETE(sys->fileList[i]->writeUtil);
									}else{
										WCHAR saveFilePath[512] = L"";
										DWORD saveFilePathSize = 512;
										sys->fileList[i]->writeUtil->GetSaveFilePath(saveFilePath, &saveFilePathSize);
										sys->fileList[i]->subRecPath.push_back(saveFilePath);
										sys->subRecFlag = TRUE;

										if( data->size > write ){
											sys->fileList[i]->writeUtil->AddTSBuff( data->data+write, data->size-write, &write);
										}
									}
								}
							}
						}
					}
				}catch(...){
					sys->fileList[i]->writeUtil = NULL;
					sys->exceptionErr = TRUE;
					_OutputDebugString(L"★★CWriteTSFile::OutThread Exception2");
				}
			}

			{
				CBlockLock lock(&sys->outThreadLock);
				sys->writeTotalSize += data->size;
			}

			SAFE_DELETE(data);
		}else{
			Sleep(10);
		}
	}
	return 0;
}

//録画中のファイルのファイルパスを取得する
//引数：
// filePath			[OUT]保存ファイル名
// subRecFlag		[OUT]サブ録画が発生したかどうか
void CWriteTSFile::GetSaveFilePath(
	wstring* filePath,
	BOOL* subRecFlag
	)
{
	*filePath = this->mainSaveFilePath;
	*subRecFlag = this->subRecFlag;
}

//録画中のファイルの出力サイズを取得する
//引数：
// writeSize			[OUT]保存ファイル名
void CWriteTSFile::GetRecWriteSize(
	__int64* writeSize
	)
{
	if( writeSize != NULL ){
		CBlockLock lock(&this->outThreadLock);
		*writeSize = this->writeTotalSize;
	}
}
