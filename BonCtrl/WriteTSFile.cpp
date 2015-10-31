#include "StdAfx.h"
#include "WriteTSFile.h"
#include <process.h>

#include "../Common/PathUtil.h"
#include "../Common/BlockLock.h"

CWriteTSFile::CWriteTSFile(void)
{
	InitializeCriticalSection(&this->outThreadLock);
	this->TSBuffOffset = 0;

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
	wstring fileName,
	BOOL overWriteFlag,
	ULONGLONG createSize,
	vector<REC_FILE_SET_INFO>* saveFolder,
	vector<wstring>* saveFolderSub,
	int maxBuffCount
)
{
	if( saveFolder->size() == 0 ){
		OutputDebugString(L"CWriteTSFile::StartSave Err saveFolder 0\r\n");
		return FALSE;
	}
	
	if( this->outThread == NULL ){
		this->fileList.clear();
		this->mainSaveFilePath = L"";
		this->overWriteFlag = overWriteFlag;
		this->createSize = createSize;
		this->maxBuffCount = maxBuffCount;
		this->writeTotalSize = 0;
		this->subRecFlag = FALSE;
		vector<REC_FILE_SET_INFO> saveFolder_ = *saveFolder;
		this->saveFolderSub = *saveFolderSub;
		for( size_t i=0; i<saveFolder_.size(); i++ ){
			SAVE_INFO item;
			item.writeUtil = NULL;
			item.freeChk = FALSE;
			item.writePlugIn = saveFolder_[i].writePlugIn;
			if( item.writePlugIn.size() == 0 ){
				item.writePlugIn = L"Write_Default.dll";
			}
			item.recFolder = saveFolder_[i].recFolder;
			item.recFileName = saveFolder_[i].recFileName;
			if( item.recFileName.size() == 0 ){
				item.recFileName = fileName;
			}
			this->fileList.push_back(item);
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
		if( _GetDiskFreeSpaceEx( this->saveFolderSub[i].c_str(), &stFree, NULL, NULL ) != FALSE ){
			if( stFree.QuadPart > needFreeSize ){
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
	if( _GetDiskFreeSpaceEx( chkFolderPath.c_str(), &stFree, NULL, NULL ) != FALSE ){
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

	this->TSBuff.clear();
	this->TSBuffOffset = 0;

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
		this->TSBuff.erase(this->TSBuff.begin(), this->TSBuff.begin() + this->TSBuffOffset);
		this->TSBuffOffset = 0;
		if( this->maxBuffCount > 0 ){
			if( this->TSBuff.size() / 48128 > (size_t)this->maxBuffCount ){
				_OutputDebugString(L"★writeBuffList MaxOver");
				this->TSBuff.clear();
			}
		}
		this->TSBuff.insert(this->TSBuff.end(), data, data + size);
	}
	return ret;
}

UINT WINAPI CWriteTSFile::OutThread(LPVOID param)
{
	//プラグインがCOMを利用するかもしれないため
	CoInitialize(NULL);

	CWriteTSFile* sys = (CWriteTSFile*)param;
	BOOL emptyFlag = TRUE;
	for( size_t i=0; i<sys->fileList.size(); i++ ){
		sys->fileList[i].writeUtil = new CWritePlugInUtil;
		wstring moduleFolder;
		GetModuleFolderPath(moduleFolder);
		if( sys->fileList[i].writeUtil->Initialize((moduleFolder + L"\\Write\\" + sys->fileList[i].writePlugIn).c_str()) == FALSE ){
			OutputDebugString(L"CWriteTSFile::StartSave Err 3\r\n");
			SAFE_DELETE(sys->fileList[i].writeUtil);
		}else{
			wstring folderPath = sys->fileList[i].recFolder;
			ChkFolderPath(folderPath);
			if( CompareNoCase(sys->fileList[i].writePlugIn, L"Write_Default.dll") == 0 ){
				//デフォルトの場合は空き容量をあらかじめチェック
				if( sys->createSize > 0 ){
					if( sys->ChkFreeFolder(sys->createSize, sys->fileList[i].recFolder) == FALSE ){
						if( sys->GetFreeFolder(sys->createSize, folderPath) ){
							//空きなかったのでサブフォルダに録画
							sys->subRecFlag = TRUE;
						}
					}
				}
			}
			//開始
			BOOL startRes = sys->fileList[i].writeUtil->StartSave(
				(folderPath + L'\\' + sys->fileList[i].recFileName).c_str(), sys->overWriteFlag, sys->createSize);
			if( startRes == FALSE ){
				OutputDebugString(L"CWriteTSFile::StartSave Err 2\r\n");
				//エラー時サブフォルダでリトライ
				if( sys->GetFreeFolder(sys->createSize, folderPath) ){
					//空きなかったのでサブフォルダに録画
					sys->subRecFlag = TRUE;
					startRes = sys->fileList[i].writeUtil->StartSave(
						(folderPath + L'\\' + sys->fileList[i].recFileName).c_str(), sys->overWriteFlag, sys->createSize);
				}
			}
			if( startRes == FALSE ){
				SAFE_DELETE(sys->fileList[i].writeUtil);
			}else{
				if( i == 0 ){
					WCHAR saveFilePath[512] = L"";
					DWORD saveFilePathSize = 512;
					sys->fileList[i].writeUtil->GetSaveFilePath(saveFilePath, &saveFilePathSize);
					sys->mainSaveFilePath = saveFilePath;
				}
				sys->fileList[i].freeChk = emptyFlag;
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

	while( sys->outStopFlag == FALSE ){
		//バッファからデータ取り出し
		BYTE data[48128];
		DWORD dataSize = 0;
		{
			CBlockLock lock(&sys->outThreadLock);
			if( sys->TSBuff.size() - sys->TSBuffOffset >= sizeof(data) ){
				//必ず188の倍数で取り出さなければならない
				dataSize = sizeof(data);
				memcpy(data, &sys->TSBuff[sys->TSBuffOffset], dataSize);
				sys->TSBuffOffset += dataSize;
			}
		}

		if( dataSize != 0 ){
			for( size_t i=0; i<sys->fileList.size(); i++ ){
				{
					if( sys->fileList[i].writeUtil != NULL ){
						DWORD write = 0;
						if( sys->fileList[i].writeUtil->AddTSBuff( data, dataSize, &write) == FALSE ){
							//空きがなくなった
							if( i == 0 ){
								CBlockLock lock(&sys->outThreadLock);
								if( sys->writeTotalSize >= 0 ){
									//出力サイズの加算を停止する
									sys->writeTotalSize = -(sys->writeTotalSize + 1);
								}
							}
							sys->fileList[i].writeUtil->StopSave();

							if( sys->fileList[i].freeChk == TRUE ){
								//次の空きを探す
								wstring freeFolderPath = L"";
								if( sys->GetFreeFolder(200*1024*1024, freeFolderPath) == TRUE ){
									wstring recFilePath = freeFolderPath;
									recFilePath += L"\\";
									recFilePath += sys->fileList[i].recFileName;

									//開始
									if( sys->fileList[i].writeUtil->StartSave(recFilePath.c_str(), sys->overWriteFlag, 0) == FALSE ){
										//失敗したので終わり
										SAFE_DELETE(sys->fileList[i].writeUtil);
									}else{
										sys->subRecFlag = TRUE;

										if( dataSize > write ){
											sys->fileList[i].writeUtil->AddTSBuff( data+write, dataSize-write, &write);
										}
									}
								}
							}else{
								//失敗したので終わり
								SAFE_DELETE(sys->fileList[i].writeUtil);
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
		sys->TSBuff.erase(sys->TSBuff.begin(), sys->TSBuff.begin() + sys->TSBuffOffset);
		sys->TSBuffOffset = 0;
		while( sys->TSBuff.empty() == false ){
			DWORD dataSize = (DWORD)min(sys->TSBuff.size(), 48128);
			for( size_t i=0; i<sys->fileList.size(); i++ ){
				if( sys->fileList[i].writeUtil ){
					DWORD write = 0;
					sys->fileList[i].writeUtil->AddTSBuff( &sys->TSBuff.front(), dataSize, &write);
				}
			}
			sys->TSBuff.erase(sys->TSBuff.begin(), sys->TSBuff.begin() + dataSize);
		}
	}
	for( size_t i=0; i<sys->fileList.size(); i++ ){
		if( sys->fileList[i].writeUtil ){
			sys->fileList[i].writeUtil->StopSave();
			SAFE_DELETE(sys->fileList[i].writeUtil);
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
		*writeSize = this->writeTotalSize < 0 ? -(this->writeTotalSize + 1) : this->writeTotalSize;
	}
}
