#include "stdafx.h"
#include "WriteTSFile.h"

#include "../Common/PathUtil.h"
#ifdef _WIN32
#include <objbase.h>
#endif

CWriteTSFile::CWriteTSFile(void)
{
	this->overWriteFlag = FALSE;
	this->createSize = 0;
	this->subRecFlag = FALSE;
	this->writeTotalSize = 0;
	this->maxBuffCount = -1;
}

CWriteTSFile::~CWriteTSFile(void)
{
	EndSave();
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
		AddDebugLog(L"CWriteTSFile::StartSave Err saveFolder 0");
		return FALSE;
	}
	
	if( this->outThread.joinable() == false ){
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
				item.writePlugIn = L"Write_Default" EDCB_LIB_EXT;
			}
			item.recFolder = saveFolder[i].recFolder;
			item.recFileName = saveFolder[i].recFileName;
			if( item.recFileName.size() == 0 ){
				item.recFileName = fileName;
			}
		}

		//受信スレッド起動
		this->outStopState = 2;
		this->outStopEvent.Reset();
		this->outThread = thread_(OutThread, this);
		//保存開始まで待つ
		this->outStopEvent.WaitOne();
		//停止状態(1)でなければ開始状態(0)に移す
		if( this->outStopState != 1 ){
			this->outStopState = 0;
			return TRUE;
		}
		this->outThread.join();
	}

	AddDebugLog(L"CWriteTSFile::StartSave Err 1");
	return FALSE;
}

BOOL CWriteTSFile::EndSave(BOOL* subRecFlag_)
{
	if( this->outThread.joinable() ){
		this->outStopState = 1;
		this->outStopEvent.Set();
		this->outThread.join();
		if( subRecFlag_ ){
			*subRecFlag_ = this->subRecFlag;
		}
		this->tsBuffList.clear();
		this->tsFreeList.clear();
		return TRUE;
	}
	return FALSE;
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
	if( data == NULL || size == 0 || this->outThread.joinable() == false ){
		return FALSE;
	}

	BOOL ret = TRUE;

	{
		lock_recursive_mutex lock(this->outThreadLock);
		while( size != 0 ){
			if( this->tsFreeList.empty() ){
				//バッファを増やす
				if( this->maxBuffCount > 0 && this->tsBuffList.size() > (size_t)this->maxBuffCount ){
					AddDebugLog(L"★writeBuffList MaxOver");
					for( auto itr = this->tsBuffList.begin(); itr != this->tsBuffList.end(); (itr++)->clear() );
					this->tsFreeList.splice(this->tsFreeList.end(), this->tsBuffList);
				}else{
					this->tsFreeList.push_back(vector<BYTE>());
					this->tsFreeList.back().reserve(48128);
				}
			}
			DWORD insertSize = min(48128 - (DWORD)this->tsFreeList.front().size(), size);
			this->tsFreeList.front().insert(this->tsFreeList.front().end(), data, data + insertSize);
			if( this->tsFreeList.front().size() == 48128 ){
				this->tsBuffList.splice(this->tsBuffList.end(), this->tsFreeList, this->tsFreeList.begin());
			}
			data += insertSize;
			size -= insertSize;
		}
	}
	return ret;
}

void CWriteTSFile::OutThread(CWriteTSFile* sys)
{
#ifdef _WIN32
	//プラグインがCOMを利用するかもしれないため
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
#endif

	BOOL emptyFlag = TRUE;
	for( size_t i=0; i<sys->fileList.size(); i++ ){
		if( sys->fileList[i]->writeUtil.Initialize(
#ifdef EDCB_LIB_ROOT
		        fs_path(EDCB_LIB_ROOT)
#else
		        GetModulePath().replace_filename(L"Write")
#endif
		        .append(sys->fileList[i]->writePlugIn).native()) == FALSE ){
			AddDebugLog(L"CWriteTSFile::StartSave Err 3");
			sys->fileList[i].reset();
		}else{
			fs_path recFolder = sys->fileList[i]->recFolder;
			//空き容量をあらかじめチェック
			LONGLONG freeBytes = UtilGetStorageFreeBytes(recFolder);
			bool isMainUnknownOrFree = (freeBytes < 0 || freeBytes > (LONGLONG)sys->createSize + FREE_FOLDER_MIN_BYTES);
			if( isMainUnknownOrFree == false ){
				//空きのあるサブフォルダを探してみる
				vector<wstring>::iterator itrFree = std::find_if(sys->saveFolderSub.begin(), sys->saveFolderSub.end(),
					[&](const wstring& a) { return UtilComparePath(a.c_str(), recFolder.c_str()) &&
					                               UtilGetStorageFreeBytes(a) > (LONGLONG)sys->createSize + FREE_FOLDER_MIN_BYTES; });
				if( itrFree != sys->saveFolderSub.end() ){
					sys->subRecFlag = TRUE;
					recFolder = *itrFree;
				}
			}
			//開始
			BOOL startRes = sys->fileList[i]->writeUtil.Start(fs_path(recFolder).append(sys->fileList[i]->recFileName).c_str(),
			                                                  sys->overWriteFlag, sys->createSize);
			if( startRes == FALSE ){
				AddDebugLog(L"CWriteTSFile::StartSave Err 2");
				//エラー時サブフォルダでリトライ
				if( isMainUnknownOrFree ){
					vector<wstring>::iterator itrFree = std::find_if(sys->saveFolderSub.begin(), sys->saveFolderSub.end(),
						[&](const wstring& a) { return UtilComparePath(a.c_str(), recFolder.c_str()) &&
						                               UtilGetStorageFreeBytes(a) > (LONGLONG)sys->createSize + FREE_FOLDER_MIN_BYTES; });
					if( itrFree != sys->saveFolderSub.end() ){
						sys->subRecFlag = TRUE;
						startRes = sys->fileList[i]->writeUtil.Start(fs_path(*itrFree).append(sys->fileList[i]->recFileName).c_str(),
						                                             sys->overWriteFlag, sys->createSize);
					}
				}
			}
			if( startRes == FALSE ){
				sys->fileList[i].reset();
			}else{
				if( i == 0 ){
					DWORD saveFilePathSize = 0;
					if( sys->fileList[i]->writeUtil.GetSavePath(NULL, &saveFilePathSize) && saveFilePathSize > 0 ){
						vector<WCHAR> saveFilePath(saveFilePathSize);
						if( sys->fileList[i]->writeUtil.GetSavePath(saveFilePath.data(), &saveFilePathSize) ){
							sys->mainSaveFilePath = saveFilePath.data();
						}
					}
				}
				sys->fileList[i]->freeChk = emptyFlag;
				emptyFlag = FALSE;
			}
		}
	}
	if( emptyFlag ){
		AddDebugLog(L"CWriteTSFile::StartSave Err fileList 0");
#ifdef _WIN32
		CoUninitialize();
#endif
		sys->outStopState = 1;
		sys->outStopEvent.Set();
		return;
	}
	sys->outStopEvent.Set();
	//中間状態(2)でなくなるまで待つ
	for( ; sys->outStopState == 2; SleepForMsec(100) );
	std::list<vector<BYTE>> data;

	while( sys->outStopState == 0 ){
		//バッファからデータ取り出し
		{
			lock_recursive_mutex lock(sys->outThreadLock);
			if( data.empty() == false ){
				//返却
				data.front().clear();
				sys->tsFreeList.splice(sys->tsFreeList.end(), data);
			}
			if( sys->tsBuffList.empty() == false ){
				data.splice(data.end(), sys->tsBuffList, sys->tsBuffList.begin());
			}
		}

		if( data.empty() == false ){
			DWORD dataSize = (DWORD)data.front().size();
			for( size_t i=0; i<sys->fileList.size(); i++ ){
				{
					if( sys->fileList[i] ){
						DWORD write = 0;
						if( sys->fileList[i]->writeUtil.Write(data.front().data(), dataSize, &write) == FALSE ){
							//空きがなくなった
							if( i == 0 ){
								lock_recursive_mutex lock(sys->outThreadLock);
								if( sys->writeTotalSize >= 0 ){
									//出力サイズの加算を停止する
									sys->writeTotalSize = -(sys->writeTotalSize + 1);
								}
							}
							sys->fileList[i]->writeUtil.Stop();

							if( sys->fileList[i]->freeChk == TRUE ){
								//次の空きを探す
								vector<wstring>::iterator itrFree = std::find_if(sys->saveFolderSub.begin(), sys->saveFolderSub.end(),
									[](const wstring& a) { return UtilGetStorageFreeBytes(a) > FREE_FOLDER_MIN_BYTES; });
								if( itrFree != sys->saveFolderSub.end() ){
									//開始
									if( sys->fileList[i]->writeUtil.Start(fs_path(*itrFree).append(sys->fileList[i]->recFileName).c_str(),
									                                      sys->overWriteFlag, 0) == FALSE ){
										//失敗したので終わり
										sys->fileList[i].reset();
									}else{
										sys->subRecFlag = TRUE;

										if( dataSize > write ){
											sys->fileList[i]->writeUtil.Write(data.front().data()+write, dataSize-write, &write);
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
								lock_recursive_mutex lock(sys->outThreadLock);
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
			sys->outStopEvent.WaitOne(100);
		}
	}

	//残っているバッファを書き出し
	{
		lock_recursive_mutex lock(sys->outThreadLock);
		if( sys->tsFreeList.empty() == false && sys->tsFreeList.front().empty() == false ){
			sys->tsBuffList.splice(sys->tsBuffList.end(), sys->tsFreeList, sys->tsFreeList.begin());
		}
		while( sys->tsBuffList.empty() == false ){
			for( size_t i=0; i<sys->fileList.size(); i++ ){
				if( sys->fileList[i] ){
					DWORD write = 0;
					sys->fileList[i]->writeUtil.Write(sys->tsBuffList.front().data(), (DWORD)sys->tsBuffList.front().size(), &write);
				}
			}
			sys->tsBuffList.pop_front();
		}
	}
	for( size_t i=0; i<sys->fileList.size(); i++ ){
		if( sys->fileList[i] ){
			sys->fileList[i]->writeUtil.Stop();
			sys->fileList[i].reset();
		}
	}

#ifdef _WIN32
	CoUninitialize();
#endif
}

wstring CWriteTSFile::GetSaveFilePath()
{
	return this->mainSaveFilePath;
}

//録画中のファイルの出力サイズを取得する
//引数：
// writeSize			[OUT]保存ファイル名
void CWriteTSFile::GetRecWriteSize(
	LONGLONG* writeSize
	)
{
	if( writeSize != NULL ){
		lock_recursive_mutex lock(this->outThreadLock);
		*writeSize = this->writeTotalSize < 0 ? -(this->writeTotalSize + 1) : this->writeTotalSize;
	}
}
