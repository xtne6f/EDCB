#include "StdAfx.h"
#include "WriteMain.h"

extern HINSTANCE g_instance;

CWriteMain::CWriteMain(void)
{
	this->file = INVALID_HANDLE_VALUE;
	this->writeBuffSize = 0;

	WCHAR dllPath[MAX_PATH];
	DWORD ret = GetModuleFileName(g_instance, dllPath, MAX_PATH);
	if( ret && ret < MAX_PATH ){
		wstring iniPath = wstring(dllPath) + L".ini";
		this->writeBuffSize = GetPrivateProfileInt(L"SET", L"Size", 770048, iniPath.c_str());
		this->writeBuff.reserve(this->writeBuffSize);
	}
}


CWriteMain::~CWriteMain(void)
{
	Stop();
}

BOOL CWriteMain::Start(
	LPCWSTR fileName,
	BOOL overWriteFlag,
	ULONGLONG createSize
	)
{
	Stop();

	this->savePath = fileName;
	_OutputDebugString(L"★CWriteMain::Start CreateFile:%s\r\n", this->savePath.c_str());
	this->file = _CreateDirectoryAndFile(this->savePath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, overWriteFlag ? CREATE_ALWAYS : CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if( this->file == INVALID_HANDLE_VALUE ){
		_OutputDebugString(L"★CWriteMain::Start Err:0x%08X\r\n", GetLastError());
		WCHAR szPath[_MAX_PATH];
		WCHAR szDrive[_MAX_DRIVE];
		WCHAR szDir[_MAX_DIR];
		WCHAR szFname[_MAX_FNAME];
		WCHAR szExt[_MAX_EXT];
		_wsplitpath_s(fileName, szDrive, szDir, szFname, szExt);
		_wmakepath_s(szPath, szDrive, szDir, szFname, NULL);
		for( int i = 1; ; i++ ){
			Format(this->savePath, L"%s-(%d)%s", szPath, i, szExt);
			this->file = CreateFile(this->savePath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, overWriteFlag ? CREATE_ALWAYS : CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
			if( this->file != INVALID_HANDLE_VALUE || i >= 999 ){
				DWORD err = GetLastError();
				_OutputDebugString(L"★CWriteMain::Start CreateFile:%s\r\n", this->savePath.c_str());
				if( this->file != INVALID_HANDLE_VALUE ){
					break;
				}
				_OutputDebugString(L"★CWriteMain::Start Err:0x%08X\r\n", err);
				this->savePath = L"";
				return FALSE;
			}
		}
	}

	//ディスクに容量を確保
	if( createSize > 0 ){
		LARGE_INTEGER stPos;
		stPos.QuadPart = createSize;
		SetFilePointerEx( this->file, stPos, NULL, FILE_BEGIN );
		SetEndOfFile( this->file );
		SetFilePointer( this->file, 0, NULL, FILE_BEGIN );
	}

	return TRUE;
}

BOOL CWriteMain::Stop(
	)
{
	if( this->file != INVALID_HANDLE_VALUE ){
		if( this->writeBuff.empty() == false ){
			DWORD write;
			if( WriteFile(this->file, &this->writeBuff.front(), (DWORD)this->writeBuff.size(), &write, NULL) == FALSE ){
				_OutputDebugString(L"★WriteFile Err:0x%08X\r\n", GetLastError());
			}else{
				this->writeBuff.erase(this->writeBuff.begin(), this->writeBuff.begin() + write);
			}
			//未出力のバッファは再Start()に備えて繰り越す
		}
		SetEndOfFile(this->file);
		CloseHandle(this->file);
		this->file = INVALID_HANDLE_VALUE;
	}

	return TRUE;
}

wstring CWriteMain::GetSavePath(
	)
{
	return this->savePath;
}

BOOL CWriteMain::Write(
	BYTE* data,
	DWORD size,
	DWORD* writeSize
	)
{
	if( this->file != INVALID_HANDLE_VALUE && data != NULL && size > 0 ){
		*writeSize = 0;
		if( this->writeBuff.empty() == false ){
			//できるだけバッファにコピー。コピー済みデータは呼び出し側にとっては「保存済み」となる
			*writeSize = min(size, this->writeBuffSize - (DWORD)this->writeBuff.size());
			this->writeBuff.insert(this->writeBuff.end(), data, data + *writeSize);
			data += *writeSize;
			size -= *writeSize;
			if( this->writeBuff.size() >= this->writeBuffSize ){
				//バッファが埋まったので出力
				DWORD write;
				if( WriteFile(this->file, &this->writeBuff.front(), (DWORD)this->writeBuff.size(), &write, NULL) == FALSE ){
					_OutputDebugString(L"★WriteFile Err:0x%08X\r\n", GetLastError());
					SetEndOfFile(this->file);
					CloseHandle(this->file);
					this->file = INVALID_HANDLE_VALUE;
					return FALSE;
				}
				this->writeBuff.erase(this->writeBuff.begin(), this->writeBuff.begin() + write);
			}
			if( this->writeBuff.empty() == false || size == 0 ){
				return TRUE;
			}
		}
		if( size > this->writeBuffSize ){
			//バッファサイズより大きいのでそのまま出力
			DWORD write;
			if( WriteFile(this->file, data, size, &write, NULL) == FALSE ){
				_OutputDebugString(L"★WriteFile Err:0x%08X\r\n", GetLastError());
				SetEndOfFile(this->file);
				CloseHandle(this->file);
				this->file = INVALID_HANDLE_VALUE;
				return FALSE;
			}
			*writeSize += write;
		}else{
			//バッファにコピー
			*writeSize += size;
			this->writeBuff.insert(this->writeBuff.end(), data, data + size);
		}
		return TRUE;
	}
	return FALSE;
}
