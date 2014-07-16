#include "StdAfx.h"
#include "WriteMain.h"

extern HINSTANCE g_instance;

CWriteMain::CWriteMain(void)
{
	this->file = NULL;
	this->writeBuff = NULL;
	this->writeBuffSize = 0;
	this->writeBuffPos = 0;

	WCHAR dllPath[512] = L"";
	GetModuleFileName(g_instance, dllPath, 512);

	wstring iniPath = dllPath;
	iniPath += L".ini";

	this->writeBuffSize = (DWORD)GetPrivateProfileIntW(L"SET", L"Size", 770048, iniPath.c_str());
	SAFE_DELETE_ARRAY(this->writeBuff);
	if( this->writeBuffSize > 0 ){
		this->writeBuff = new BYTE[this->writeBuffSize];
	}
	this->writeBuffPos = 0;
}


CWriteMain::~CWriteMain(void)
{
	if( this->file != NULL ){
		_StopSave();
	}

	SAFE_DELETE_ARRAY(this->writeBuff);
	this->writeBuffSize = 0;
	this->writeBuffPos = 0;
}

//ファイル保存を開始する
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// fileName				[IN]保存ファイルフルパス（必要に応じて拡張子変えたりなど行う）
// overWriteFlag		[IN]同一ファイル名存在時に上書きするかどうか（TRUE：する、FALSE：しない）
// createSize			[IN]入力予想容量（188バイトTSでの容量。即時録画時など総時間未定の場合は0。延長などの可能性もあるので目安程度）
BOOL CWriteMain::_StartSave(
	LPCWSTR fileName,
	BOOL overWriteFlag,
	ULONGLONG createSize
	)
{
	this->savePath = L"";

	wstring errMsg = L"";
	DWORD err = 0;

	wstring recFilePath = fileName;
	if( overWriteFlag == TRUE ){
		_OutputDebugString(L"★_StartSave CreateFile:%s", recFilePath.c_str());
		this->file = _CreateDirectoryAndFile( recFilePath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
		if( this->file == INVALID_HANDLE_VALUE ){
			err = GetLastError();
			GetLastErrMsg(err, errMsg);
			_OutputDebugString(L"★_StartSave Err:0x%08X %s", err, errMsg.c_str());
			if( GetNextFileName(fileName, recFilePath) == TRUE ){
				_OutputDebugString(L"★_StartSave CreateFile:%s", recFilePath.c_str());
				this->file = _CreateDirectoryAndFile( recFilePath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
			}
		}
	}else{
		_OutputDebugString(L"★_StartSave CreateFile:%s", recFilePath.c_str());
		this->file = _CreateDirectoryAndFile( recFilePath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL );
		if( this->file == INVALID_HANDLE_VALUE ){
			err = GetLastError();
			GetLastErrMsg(err, errMsg);
			_OutputDebugString(L"★_StartSave Err:0x%08X %s", err, errMsg.c_str());
			if( GetNextFileName(fileName, recFilePath) == TRUE ){
				_OutputDebugString(L"★_StartSave CreateFile:%s", recFilePath.c_str());
				this->file = _CreateDirectoryAndFile( recFilePath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL );
			}
		}
	}
	if( this->file == INVALID_HANDLE_VALUE ){
		err = GetLastError();
		GetLastErrMsg(err, errMsg);
		_OutputDebugString(L"★_StartSave Err:0x%08X %s", err, errMsg.c_str());
		this->file = NULL;
		return FALSE;
	}

	//ディスクに容量を確保
	if( createSize > 0 ){
		LARGE_INTEGER stPos;
		stPos.QuadPart = createSize;
		SetFilePointerEx( this->file, stPos, NULL, FILE_BEGIN );
		SetEndOfFile( this->file );
		FlushFileBuffers( this->file );
		SetFilePointer( this->file, 0, NULL, FILE_BEGIN );
	}

	this->savePath = recFilePath;

	return TRUE;
}

//ファイル保存を終了する
//戻り値：
// TRUE（成功）、FALSE（失敗）
BOOL CWriteMain::_StopSave(
	)
{
	if( this->file != NULL ){
		if( this->writeBuffPos > 0 ){
			DWORD writeSize = 0;
			DWORD err = 0;
			wstring errMsg = L"";
			if( WriteFile(this->file, this->writeBuff, this->writeBuffPos, &writeSize, NULL) == FALSE ){
				err = GetLastError();
				GetLastErrMsg(err, errMsg);
				_OutputDebugString(L"★_StopSave WriteFile Err:0x%08X %s", err, errMsg.c_str());
			}else{
				this->writeBuffPos = 0;
			}
		}
		SetEndOfFile(this->file);
		CloseHandle(this->file);
		this->file = NULL;
	}

	return TRUE;
}

//実際に保存しているファイルパスを取得する（再生やバッチ処理に利用される）
//filePathがNULL時は必要なサイズをfilePathSizeで返す
//通常filePathSize=512で呼び出し
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// filePath				[OUT]保存ファイルフルパス
// filePathSize			[IN/OUT]filePathのサイズ(WCHAR単位)
BOOL CWriteMain::_GetSaveFilePath(
	WCHAR* filePath,
	DWORD* filePathSize
	)
{
	if( filePath == NULL ){
		if( filePathSize == NULL ){
			return FALSE;
		}else{
			*filePathSize = (DWORD)this->savePath.size()+1;
		}
	}else{
		if( filePathSize == NULL ){
			return FALSE;
		}else{
			if( *filePathSize < (DWORD)this->savePath.size()+1 ){
				*filePathSize = (DWORD)this->savePath.size()+1;
				return FALSE;
			}else{
				wcscpy_s(filePath, *filePathSize, this->savePath.c_str());
			}
		}
	}
	return TRUE;
}

//保存用TSデータを送る
//空き容量不足などで書き出し失敗した場合、writeSizeの値を元に
//再度保存処理するときの送信開始地点を決める
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// data					[IN]TSデータ
// size					[IN]dataのサイズ
// writeSize			[OUT]保存に利用したサイズ
BOOL CWriteMain::_AddTSBuff(
	BYTE* data,
	DWORD size,
	DWORD* writeSize
	)
{
	BOOL ret = FALSE;
	wstring errMsg = L"";
	DWORD err = 0;
	if( this->file != NULL && data != NULL && size > 0 ){
		if( this->writeBuffSize > 0 && this->writeBuff != NULL){
			if( size > this->writeBuffSize ){
				//バッファサイズより大きいのでそのまま出力
				if( this->writeBuffPos > 0 ){
					//前回のバッファあるので先に出力
					DWORD write = 0;
					wstring errMsg = L"";
					if( WriteFile(this->file, this->writeBuff, this->writeBuffPos, &write, NULL) == FALSE ){
						//エラーが出た
						err = GetLastError();
						GetLastErrMsg(err, errMsg);
						_OutputDebugString(L"★_StopSave WriteFile Err:0x%08X %s", err, errMsg.c_str());
						*writeSize = 0;
						//書き込めなかった部分をバッファの先頭へコピー
						DWORD tempBuffSize = this->writeBuffPos-write;
						if( tempBuffSize > 0 ){
							BYTE* tempBuff = new BYTE[tempBuffSize];
							memcpy( tempBuff, this->writeBuff+write, tempBuffSize);
							this->writeBuffPos = tempBuffSize;
							memcpy(this->writeBuff, tempBuff, tempBuffSize);
							SAFE_DELETE_ARRAY(tempBuff);
						}

						SetEndOfFile(this->file);
						CloseHandle(this->file);
						this->file = NULL;

						return FALSE;
					}else{
						this->writeBuffPos = 0;
					}
				}
				//ファイル出力
				if( WriteFile(this->file, data, size, writeSize, NULL) == FALSE ){
					//エラーが出た
					err = GetLastError();
					GetLastErrMsg(err, errMsg);
					_OutputDebugString(L"★WriteFile Err:0x%08X %s", err, errMsg.c_str());

					SetEndOfFile(this->file);
					CloseHandle(this->file);
					this->file = NULL;

					return FALSE;
				}else{
					ret = TRUE;
				}
			}else{
				if(this->writeBuffSize < this->writeBuffPos + size){
					//バッファ埋まるので出力
					DWORD tempSize = this->writeBuffSize - this->writeBuffPos;
					memcpy( this->writeBuff+this->writeBuffPos, data, tempSize );
					DWORD tempWriteSize = 0;
					if( WriteFile(this->file, this->writeBuff, this->writeBuffSize, &tempWriteSize, NULL) == FALSE ){
						//エラー
						err = GetLastError();
						GetLastErrMsg(err, errMsg);
						_OutputDebugString(L"★_StopSave WriteFile Err:0x%08X %s", err, errMsg.c_str());
						*writeSize = 0;
						//ファイルポインタ戻す
						LONG lpos = (LONG)tempWriteSize;
						SetFilePointer(this->file, -lpos, NULL, FILE_CURRENT);

						SetEndOfFile(this->file);
						CloseHandle(this->file);
						this->file = NULL;

						return FALSE;
					}else{
						//バッファにコピー
						memcpy( this->writeBuff, data+tempSize, size-tempSize );
						this->writeBuffPos = size-tempSize;
						*writeSize = size;
						ret = TRUE;
					}
				}else{
					//バッファにコピー
					memcpy( this->writeBuff+this->writeBuffPos, data, size );
					this->writeBuffPos += size;
					*writeSize = size;
					ret = TRUE;
				}
			}
		}else{
			ret = WriteFile(this->file, data, size, writeSize, NULL);
			if( ret == FALSE ){
				err = GetLastError();
				GetLastErrMsg(err, errMsg);
				_OutputDebugString(L"★WriteFile Err:0x%08X %s", err, errMsg.c_str());

				SetEndOfFile(this->file);
				CloseHandle(this->file);
				this->file = NULL;
			}
		}
	}
	return ret;
}

BOOL CWriteMain::GetNextFileName(wstring filePath, wstring& newPath)
{
	WCHAR szPath[_MAX_PATH];
	WCHAR szDrive[_MAX_DRIVE];
	WCHAR szDir[_MAX_DIR];
	WCHAR szFname[_MAX_FNAME];
	WCHAR szExt[_MAX_EXT];
	_wsplitpath_s( filePath.c_str(), szDrive, _MAX_DRIVE, szDir, _MAX_DIR, szFname, _MAX_FNAME, szExt, _MAX_EXT );
	_wmakepath_s(  szPath, _MAX_PATH, szDrive, szDir, NULL, NULL );

	BOOL findFlag = FALSE;
	for( int i=1; i<1000; i++ ){
		WIN32_FIND_DATA findData;
		HANDLE find;

		wstring name;
		Format(name, L"%s%s-(%d)%s", szPath, szFname, i, szExt);
		newPath = name;
		
		find = FindFirstFile( newPath.c_str(), &findData);
		if ( find == INVALID_HANDLE_VALUE ) {
			findFlag = TRUE;
			break;
		}
		FindClose(find);
	}
	return findFlag;
}
