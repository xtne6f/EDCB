#include "stdafx.h"
#include "WriteTSFile.h"
#include <process.h>

#include "../Common/PathUtil.h"
#include "../Common/BlockLock.h"

CWriteTSFile::CWriteTSFile(void)
{
	InitializeCriticalSection(&this->outThreadLock);

    this->outThread = NULL;
    this->outStopFlag = FALSE;
	this->outStartFlag = FALSE;
	this->overWriteFlag = FALSE;
	this->createSize = 0;
	this->subRecFlag = FALSE;
	this->writeTotalSize = 0;
	this->maxBuffCount = -1;
}

CWriteTSFile::~CWriteTSFile(void)
{
	EndSave();
	DeleteCriticalSection(&this->outThreadLock);
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
	const wstring& fileName,
	BOOL overWriteFlag_,
	ULONGLONG createSize_,
	const vector<REC_FILE_SET_INFO>& saveFolder,
	const vector<wstring>& saveFolderSub_,
	int maxBuffCount_
)
{
	if( saveFolder.size() == 0 ){
		OutputDebugString(L"CWriteTSFile::StartSave Err saveFolder 0\r\n");
		return FALSE;
	}
	
	if( this->outThread == NULL ){
		this->fileList.clear();
		this->mainSaveFilePath = L"";
		this->overWriteFlag = overWriteFlag_;
		this->createSize = createSize_;
		this->maxBuffCount = maxBuffCount_;
		this->writeTotalSize = 0;
		this->subRecFlag = FALSE;
		this->saveFolderSub = saveFolderSub_;
		for( size_t i=0; i<saveFolder.size(); i++ ){
			this->fileList.push_back(std::unique_ptr<SAVE_INFO>(new SAVE_INFO));
			SAVE_INFO& item = *this->fileList.back();
			item.freeChk = FALSE;
			item.writePlugIn = saveFolder[i].writePlugIn;
			if( item.writePlugIn.size() == 0 ){
				item.writePlugIn = L"Write_Default.dll";
			}
			item.recFolder = saveFolder[i].recFolder;
			item.recFileName = saveFolder[i].recFileName;
			if( item.recFileName.size() == 0 ){
				item.recFileName = fileName;
			}
		}

		//受信スレッド起動
		this->outStopFlag = FALSE;
		this->outStartFlag = FALSE;
		this->outThread = (HANDLE)_beginthreadex(NULL, 0, OutThread, this, 0, NULL);
		if( this->outThread ){
			//保存開始まで待つ
			while( WaitForSingleObject(this->outThread, 10) == WAIT_TIMEOUT && this->outStartFlag == FALSE );
			if( this->outStartFlag ){
				return TRUE;
			}
			CloseHandle(this->outThread);
			this->outThread = NULL;
		}
	}

	OutputDebugString(L"CWriteTSFile::StartSave Err 1\r\n");
	return FALSE;
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
		if( GetDiskFreeSpaceEx( GetChkDrivePath(this->saveFolderSub[i]).c_str(), &stFree, NULL, NULL ) != FALSE ){
			if( stFree.QuadPart > needFreeSize ){
				freeFolderPath = this->saveFolderSub[i];
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
	const wstring& chkFolderPath
)
{
	BOOL ret = FALSE;

	ULARGE_INTEGER stFree;
	if( GetDiskFreeSpaceEx( GetChkDrivePath(chkFolderPath).c_str(), &stFree, NULL, NULL ) != FALSE ){
		if( stFree.QuadPart > needFreeSize ){
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
		this->outStopFlag = TRUE;
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->outThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->outThread, 0xffffffff);
		}
		CloseHandle(this->outThread);
		this->outThread = NULL;
	}

	this->tsBuffList.clear();

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

	{
		CBlockLock lock(&this->outThreadLock);
		for( std::list<vector<BYTE>>::iterator itr = this->tsBuffList.begin(); size != 0; itr++ ){
			if( itr == this->tsBuffList.end() ){
				//バッファを増やす
				if( this->maxBuffCount > 0 && this->tsBuffList.size() > (size_t)this->maxBuffCount ){
					_OutputDebugString(L"★writeBuffList MaxOver");
					for( itr = this->tsBuffList.begin(); itr != this->tsBuffList.end(); (itr++)->clear() );
					itr = this->tsBuffList.begin();
				}else{
					this->tsBuffList.push_back(vector<BYTE>());
					itr = this->tsBuffList.end();
					(--itr)->reserve(48128);
				}
			}
			DWORD insertSize = min(48128 - (DWORD)itr->size(), size);
			itr->insert(itr->end(), data, data + insertSize);
			data += insertSize;
			size -= insertSize;
		}
	}
	return ret;
}

UINT WINAPI CWriteTSFile::OutThread(LPVOID param)
{
	//プラグインがCOMを利用するかもしれないため
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	CWriteTSFile* sys = (CWriteTSFile*)param;
	BOOL emptyFlag = TRUE;
	for( size_t i=0; i<sys->fileList.size(); i++ ){
		if( sys->fileList[i]->writeUtil.Initialize(GetModulePath().replace_filename(L"Write").append(sys->fileList[i]->writePlugIn).c_str()) == FALSE ){
			OutputDebugString(L"CWriteTSFile::StartSave Err 3\r\n");
			sys->fileList[i].reset();
		}else{
			fs_path path = sys->fileList[i]->recFolder;
			ChkFolderPath(path);
			if( CompareNoCase(sys->fileList[i]->writePlugIn, L"Write_Default.dll") == 0 ){
				//デフォルトの場合は空き容量をあらかじめチェック
				if( sys->createSize > 0 ){
					if( sys->ChkFreeFolder(sys->createSize, path.native()) == FALSE ){
						wstring folderPath;
						if( sys->GetFreeFolder(sys->createSize, folderPath) ){
							//空きなかったのでサブフォルダに録画
							sys->subRecFlag = TRUE;
							path = folderPath;
						}
					}
				}
			}
			//開始
			path.append(sys->fileList[i]->recFileName);
			BOOL startRes = sys->fileList[i]->writeUtil.StartSave(path.c_str(), sys->overWriteFlag, sys->createSize);
			if( startRes == FALSE ){
				OutputDebugString(L"CWriteTSFile::StartSave Err 2\r\n");
				//エラー時サブフォルダでリトライ
				wstring folderPath;
				if( sys->GetFreeFolder(sys->createSize, folderPath) ){
					//空きなかったのでサブフォルダに録画
					sys->subRecFlag = TRUE;
					path = fs_path(folderPath).append(sys->fileList[i]->recFileName);
					startRes = sys->fileList[i]->writeUtil.StartSave(path.c_str(), sys->overWriteFlag, sys->createSize);
				}
			}
			if( startRes == FALSE ){
				sys->fileList[i].reset();
			}else{
				if( i == 0 ){
					DWORD saveFilePathSize = 0;
					if( sys->fileList[i]->writeUtil.GetSaveFilePath(NULL, &saveFilePathSize) && saveFilePathSize > 0 ){
						vector<WCHAR> saveFilePath(saveFilePathSize);
						if( sys->fileList[i]->writeUtil.GetSaveFilePath(&saveFilePath.front(), &saveFilePathSize) ){
							sys->mainSaveFilePath = &saveFilePath.front();
						}
					}
				}
				sys->fileList[i]->freeChk = emptyFlag;
				emptyFlag = FALSE;
			}
		}
	}
	if( emptyFlag ){
		OutputDebugString(L"CWriteTSFile::StartSave Err fileList 0\r\n");
		CoUninitialize();
		return 0;
	}
	sys->outStartFlag = TRUE;
	std::list<vector<BYTE>> data;

	while( sys->outStopFlag == FALSE ){
		//バッファからデータ取り出し
		{
			CBlockLock lock(&sys->outThreadLock);
			if( data.empty() == false ){
				//返却
				data.front().clear();
				std::list<vector<BYTE>>::iterator itr;
				for( itr = sys->tsBuffList.begin(); itr != sys->tsBuffList.end() && itr->empty() == false; itr++ );
				sys->tsBuffList.splice(itr, data);
			}
			if( sys->tsBuffList.empty() == false && sys->tsBuffList.front().size() == 48128 ){
				data.splice(data.end(), sys->tsBuffList, sys->tsBuffList.begin());
			}
		}

		if( data.empty() == false ){
			DWORD dataSize = (DWORD)data.front().size();
			for( size_t i=0; i<sys->fileList.size(); i++ ){
				{
					if( sys->fileList[i] ){
						DWORD write = 0;
						if( sys->fileList[i]->writeUtil.AddTSBuff(&data.front().front(), dataSize, &write) == FALSE ){
							//空きがなくなった
							if( i == 0 ){
								CBlockLock lock(&sys->outThreadLock);
								if( sys->writeTotalSize >= 0 ){
									//出力サイズの加算を停止する
									sys->writeTotalSize = -(sys->writeTotalSize + 1);
								}
							}
							sys->fileList[i]->writeUtil.StopSave();

							if( sys->fileList[i]->freeChk == TRUE ){
								//次の空きを探す
								wstring freeFolderPath = L"";
								if( sys->GetFreeFolder(200*1024*1024, freeFolderPath) == TRUE ){
									fs_path recFilePath = fs_path(freeFolderPath).append(sys->fileList[i]->recFileName);

									//開始
									if( sys->fileList[i]->writeUtil.StartSave(recFilePath.c_str(), sys->overWriteFlag, 0) == FALSE ){
										//失敗したので終わり
										sys->fileList[i].reset();
									}else{
										sys->subRecFlag = TRUE;

										if( dataSize > write ){
											sys->fileList[i]->writeUtil.AddTSBuff(&data.front().front()+write, dataSize-write, &write);
										}
									}
								}
							}else{
								//失敗したので終わり
								sys->fileList[i].reset();
							}
						}else{
							//原作では成否にかかわらずwriteTotalSizeにdataSizeを加算しているが
							//出力サイズの利用ケース的にはmainSaveFilePathと一致させないとおかしいと思うので、そのように変更した
							if( i == 0 ){
								CBlockLock lock(&sys->outThreadLock);
								if( sys->writeTotalSize >= 0 ){
									sys->writeTotalSize += dataSize;
								}
							}
						}
					}
				}
			}
		}else{
			//TODO: 厳密にはメッセージをディスパッチすべき(スレッド内で単純なCOMオブジェクトを扱う限りは(多分)問題ない)
			Sleep(100);
		}
	}

	//残っているバッファを書き出し
	{
		CBlockLock lock(&sys->outThreadLock);
		for( std::list<vector<BYTE>>::iterator itr = sys->tsBuffList.begin(); itr != sys->tsBuffList.end() && itr->empty() == false; itr++ ){
			for( size_t i=0; i<sys->fileList.size(); i++ ){
				if( sys->fileList[i] ){
					DWORD write = 0;
					sys->fileList[i]->writeUtil.AddTSBuff(&itr->front(), (DWORD)itr->size(), &write);
				}
			}
			itr->clear();
		}
	}
	for( size_t i=0; i<sys->fileList.size(); i++ ){
		if( sys->fileList[i] ){
			sys->fileList[i]->writeUtil.StopSave();
			sys->fileList[i].reset();
		}
	}

	CoUninitialize();
	return 0;
}

//録画中のファイルのファイルパスを取得する
//引数：
// filePath			[OUT]保存ファイル名
// subRecFlag		[OUT]サブ録画が発生したかどうか
void CWriteTSFile::GetSaveFilePath(
	wstring* filePath,
	BOOL* subRecFlag_
	)
{
	*filePath = this->mainSaveFilePath;
	*subRecFlag_ = this->subRecFlag;
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
		*writeSize = this->writeTotalSize < 0 ? -(this->writeTotalSize + 1) : this->writeTotalSize;
	}
}
