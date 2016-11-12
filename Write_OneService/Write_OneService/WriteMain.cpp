#include "StdAfx.h"
#include "WriteMain.h"
#include <process.h>
#include "../../Common/PathUtil.h"
#include "../../Common/TSPacketUtil.h"

extern HINSTANCE g_instance;

CWriteMain::CWriteMain()
{
	this->file = INVALID_HANDLE_VALUE;
	this->writePlugin.hDll = NULL;

	WCHAR dllPath[MAX_PATH];
	DWORD ret = GetModuleFileName(g_instance, dllPath, MAX_PATH);
	if( ret && ret < MAX_PATH ){
		wstring iniPath = wstring(dllPath) + L".ini";
		wstring name = GetPrivateProfileToString(L"SET", L"WritePlugin", L"", iniPath.c_str());
		if( name.empty() == false && name[0] != L';' ){
			//出力プラグインを数珠繋ぎ
			wstring dir;
			GetFileFolder(iniPath, dir);
			this->writePlugin.Initialize((dir + L'\\' + name).c_str());
		}
	}
}

CWriteMain::~CWriteMain()
{
	Stop();
	if( this->writePlugin.hDll ){
		this->writePlugin.pfnDeleteCtrl(this->writePlugin.id);
		FreeLibrary(this->writePlugin.hDll);
	}
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
	wstring name;
	GetFileName(this->savePath, name);
	if( name.c_str()[0] == L'#' ){
		//ファイル名が"#16進数4桁#"で始まるならこれをサービスIDと解釈して取り除く
		WCHAR* endp;
		WORD sid = (WORD)wcstol(name.c_str() + 1, &endp, 16);
		if( endp - name.c_str() == 5 && *endp == L'#' ){
			this->targetSID = sid == 0xFFFF ? 0 : sid;
			GetFileFolder(this->savePath, this->savePath);
			this->savePath += L'\\' + name.substr(6);
		}
	}

	if( this->writePlugin.hDll ){
		if( this->writePlugin.pfnStartSave(this->writePlugin.id, this->savePath.c_str(), overWriteFlag, createSize) == FALSE ){
			this->savePath = L"";
			return FALSE;
		}
		WCHAR path[512];
		DWORD pathSize = 512;
		if( this->writePlugin.pfnGetSaveFilePath(this->writePlugin.id, path, &pathSize) == FALSE ){
			this->writePlugin.pfnStopSave(this->writePlugin.id);
			this->savePath = L"";
			return FALSE;
		}
		this->savePath = path;
	}else{
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
	}

	this->lastTSID = 0;
	this->packetInit.ClearBuff();
	this->catUtil = CCATUtil();
	this->pmtUtilMap.clear();
	CheckNeedPID();

	return TRUE;
}

BOOL CWriteMain::Stop(
	)
{
	if( this->file != INVALID_HANDLE_VALUE ){
		CloseHandle(this->file);
		this->file = INVALID_HANDLE_VALUE;
	}
	if( this->writePlugin.hDll ){
		this->writePlugin.pfnStopSave(this->writePlugin.id);
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
	if( (this->writePlugin.hDll || this->file != INVALID_HANDLE_VALUE) && data != NULL && size > 0 && writeSize != NULL ){
		*writeSize = 0;
		if( this->targetSID == 0 ){
			//全サービスなので何も弄らない
			if( this->writePlugin.hDll ){
				if( this->writePlugin.pfnAddTSBuff(this->writePlugin.id, data, size, writeSize) == FALSE ){
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
			//※ここからBonCtrl/CTSOut::AddTSBuff()とほとんど同じ作業
			this->outBuff.clear();
			for( DWORD i = 0; i < outSize; i += 188 ){
				CTSPacketUtil packet;
				if( packet.Set188TS(outData + i, 188) ){
					//指定サービスに必要なPIDを解析
					if( packet.transport_scrambling_control == 0 ){
						if( packet.PID == 1 ){
							//CAT
							if( this->catUtil.AddPacket(&packet) ){
								CheckNeedPID();
							}
						}
						if( packet.payload_unit_start_indicator && packet.data_byteSize > 0 ){
							BYTE pointer = packet.data_byte[0];
							if( 1 + pointer < packet.data_byteSize && packet.data_byte[1 + pointer] == 2 ){
								//PMT
								if( this->pmtUtilMap.count(packet.PID) == 0 ){
									this->pmtUtilMap[packet.PID] = CPMTUtil();
								}
								if( this->pmtUtilMap[packet.PID].AddPacket(&packet) ){
									CheckNeedPID();
								}
							}
						}else{
							//PMTの2パケット目かチェック
							if( this->pmtUtilMap.count(packet.PID) != 0 ){
								if( this->pmtUtilMap[packet.PID].AddPacket(&packet) ){
									CheckNeedPID();
								}
							}
						}
					}
					if( *std::lower_bound(this->needPIDList.begin(), this->needPIDList.end(), packet.PID) == packet.PID ){
						if( packet.PID == 0 ){
							//PATなので必要なサービスのみに絞る
							if( packet.payload_unit_start_indicator ){
								if( 5 < packet.data_byteSize ){
									//TSIDを取得
									this->lastTSID = packet.data_byte[4] << 8 | packet.data_byte[5];
									CheckNeedPID();
								}
								BYTE* patBuff;
								DWORD patBuffSize;
								if( this->lastTSID != 0 && this->patUtil.GetPacket(&patBuff, &patBuffSize) ){
									this->outBuff.insert(this->outBuff.end(), patBuff, patBuff + patBuffSize);
								}
							}
						}else{
							this->outBuff.insert(this->outBuff.end(), outData + i, outData + i + 188);
						}
					}
				}
			}
			if( this->outBuff.empty() ){
				*writeSize = size;
				return TRUE;
			}
			DWORD write;
			if( this->writePlugin.hDll ){
				if( this->writePlugin.pfnAddTSBuff(this->writePlugin.id, &this->outBuff.front(), (DWORD)this->outBuff.size(), &write) == FALSE ){
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

void CWriteMain::AddNeedPID(WORD pid)
{
	vector<WORD>::iterator itr = std::lower_bound(this->needPIDList.begin(), this->needPIDList.end(), pid);
	if( *itr != pid ){
		this->needPIDList.insert(itr, pid);
	}
}

void CWriteMain::CheckNeedPID()
{
	//0xFFFFは番兵
	this->needPIDList.assign(1, 0xFFFF);
	for( WORD i = 0; i <= 0x30; AddNeedPID(i++) );

	//PAT作成用のPMTリスト
	map<WORD, CCreatePATPacket::PROGRAM_PID_INFO> pidMap;
	//NITのPID追加しておく
	CCreatePATPacket::PROGRAM_PID_INFO item;
	item.PMTPID = 0x10;
	item.SID = 0;
	pidMap[item.PMTPID] = item;

	//EMMのPID
	for( map<WORD, WORD>::const_iterator itr = this->catUtil.PIDList.begin(); itr != this->catUtil.PIDList.end(); itr++ ){
		AddNeedPID(itr->first);
	}
	for( map<WORD, CPMTUtil>::const_iterator itr = this->pmtUtilMap.begin(); itr != this->pmtUtilMap.end(); itr++ ){
		if( itr->second.program_number == this->targetSID ){
			//PAT作成用のPMTリスト作成
			item.PMTPID = itr->first;
			item.SID = itr->second.program_number;
			pidMap[item.PMTPID] = item;
			//指定サービスのPMT発見。PMT記載のPIDを登録
			AddNeedPID(itr->first);
			AddNeedPID(itr->second.PCR_PID);
			for( map<WORD,WORD>::const_iterator jtr = itr->second.PIDList.begin(); jtr != itr->second.PIDList.end(); jtr++ ){
				AddNeedPID(jtr->first);
			}
		}
	}
	this->patUtil.SetParam(this->lastTSID, &pidMap);
}
