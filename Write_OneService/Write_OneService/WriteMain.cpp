#include "stdafx.h"
#include "WriteMain.h"
#include "../../Common/PathUtil.h"
#include "../../Common/TSPacketUtil.h"

extern HINSTANCE g_instance;

CWriteMain::CWriteMain()
{
	this->file = INVALID_HANDLE_VALUE;

	WCHAR dllPath[MAX_PATH];
	DWORD ret = GetModuleFileName(g_instance, dllPath, MAX_PATH);
	if( ret && ret < MAX_PATH ){
		wstring iniPath = wstring(dllPath) + L".ini";
		wstring name = GetPrivateProfileToString(L"SET", L"WritePlugin", L"", iniPath.c_str());
		if( name.empty() == false && name[0] != L';' ){
			//出力プラグインを数珠繋ぎ
			this->writePlugin.reset(new CWritePlugInUtil);
			if( this->writePlugin->Initialize(fs_path(iniPath).replace_filename(name).c_str()) == FALSE ){
				this->writePlugin.reset();
			}
		}
	}
}

CWriteMain::~CWriteMain()
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
	this->targetSID = 0;
	fs_path name = fs_path(this->savePath).filename();
	if( name.c_str()[0] == L'#' ){
		//ファイル名が"#16進数4桁#"で始まるならこれをサービスIDと解釈して取り除く
		WCHAR* endp;
		WORD sid = (WORD)wcstol(name.c_str() + 1, &endp, 16);
		if( endp - name.c_str() == 5 && *endp == L'#' ){
			this->targetSID = sid == 0xFFFF ? 0 : sid;
			this->savePath = fs_path(this->savePath).replace_filename(name.c_str() + 6).native();
		}
	}

	if( this->writePlugin ){
		if( this->writePlugin->Start(this->savePath.c_str(), overWriteFlag, createSize) == FALSE ){
			this->savePath = L"";
			return FALSE;
		}
		vector<WCHAR> path;
		DWORD pathSize = 0;
		if( this->writePlugin->GetSavePath(NULL, &pathSize) && pathSize > 0 ){
			path.resize(pathSize);
			if( this->writePlugin->GetSavePath(&path.front(), &pathSize) == FALSE ){
				path.clear();
			}
		}
		if( path.empty() ){
			this->writePlugin->Stop();
			this->savePath = L"";
			return FALSE;
		}
		this->savePath = &path.front();
	}else{
		_OutputDebugString(L"★CWriteMain::Start CreateFile:%s\r\n", this->savePath.c_str());
		UtilCreateDirectories(fs_path(this->savePath).parent_path());
		this->file = CreateFile(this->savePath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, overWriteFlag ? CREATE_ALWAYS : CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if( this->file == INVALID_HANDLE_VALUE ){
			_OutputDebugString(L"★CWriteMain::Start Err:0x%08X\r\n", GetLastError());
			fs_path pathWoExt = this->savePath;
			fs_path ext = pathWoExt.extension();
			pathWoExt.replace_extension();
			for( int i = 1; ; i++ ){
				Format(this->savePath, L"%s-(%d)%s", pathWoExt.c_str(), i, ext.c_str());
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
	}

	this->lastTSID = 0;
	this->packetInit.ClearBuff();
	this->serviceFilter.SetServiceID(this->targetSID == 0, vector<WORD>(1, this->targetSID));

	return TRUE;
}

BOOL CWriteMain::Stop(
	)
{
	if( this->file != INVALID_HANDLE_VALUE ){
		CloseHandle(this->file);
		this->file = INVALID_HANDLE_VALUE;
	}
	if( this->writePlugin ){
		this->writePlugin->Stop();
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
	if( (this->writePlugin || this->file != INVALID_HANDLE_VALUE) && data != NULL && size > 0 && writeSize != NULL ){
		*writeSize = 0;
		if( this->targetSID == 0 ){
			//全サービスなので何も弄らない
			if( this->writePlugin ){
				if( this->writePlugin->Write(data, size, writeSize) == FALSE ){
					return FALSE;
				}
			}else{
				if( WriteFile(this->file, data, size, writeSize, NULL) == FALSE ){
					_OutputDebugString(L"★WriteFile Err:0x%08X\r\n", GetLastError());
					CloseHandle(this->file);
					this->file = INVALID_HANDLE_VALUE;
					return FALSE;
				}
			}
			return TRUE;
		}else{
			//指定サービス
			BYTE* outData;
			DWORD outSize;
			if( this->packetInit.GetTSData(data, size, &outData, &outSize) == FALSE ){
				outSize = 0;
			}
			this->outBuff.clear();
			for( DWORD i = 0; i < outSize; i += 188 ){
				CTSPacketUtil packet;
				if( packet.Set188TS(outData + i, 188) ){
					if( packet.PID == 0 && packet.payload_unit_start_indicator && 5 < packet.data_byteSize ){
						//TSIDを取得
						WORD tsid = packet.data_byte[4] << 8 | packet.data_byte[5];
						if( this->lastTSID != tsid ){
							this->lastTSID = tsid;
							this->serviceFilter.Clear(tsid);
						}
					}
					if( this->lastTSID != 0 ){
						this->serviceFilter.FilterPacket(this->outBuff, outData + i, packet);
					}
				}
			}
			if( this->outBuff.empty() ){
				*writeSize = size;
				return TRUE;
			}
			DWORD write;
			if( this->writePlugin ){
				if( this->writePlugin->Write(&this->outBuff.front(), (DWORD)this->outBuff.size(), &write) == FALSE ){
					return FALSE;
				}
			}else{
				if( WriteFile(this->file, &this->outBuff.front(), (DWORD)this->outBuff.size(), &write, NULL) == FALSE ){
					_OutputDebugString(L"★WriteFile Err:0x%08X\r\n", GetLastError());
					CloseHandle(this->file);
					this->file = INVALID_HANDLE_VALUE;
					return FALSE;
				}
			}
			*writeSize = size;
			return TRUE;
		}
	}
	return FALSE;
}
