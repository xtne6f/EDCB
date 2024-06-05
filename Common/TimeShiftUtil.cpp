#include "stdafx.h"
#include "TimeShiftUtil.h"
#include "StringUtil.h"
#include "TimeUtil.h"
#include "TSPacketUtil.h"
#include "../BonCtrl/BonCtrlDef.h"
#include "../BonCtrl/PacketInit.h"
#include "../BonCtrl/CreatePATPacket.h"
#ifndef _WIN32
#include <sys/stat.h>
#endif

CTimeShiftUtil::CTimeShiftUtil(void)
	: udpMutex(UtilCreateGlobalMutex())
	, tcpMutex(UtilCreateGlobalMutex())
{
	this->PCR_PID = 0xFFFF;
	this->fileMode = FALSE;
	this->seekJitter = 1;
	this->currentFilePos = 0;
}


CTimeShiftUtil::~CTimeShiftUtil(void)
{
	StopTimeShift();

	Send(L"", NULL, NULL);
}

void CTimeShiftUtil::Send(
	LPCWSTR ip,
	DWORD* udpPort,
	DWORD* tcpPort
	)
{
	lock_recursive_mutex lock(this->utilLock);
	lock_recursive_mutex lock2(this->ioLock);

	for( int tcp = 0; tcp < 2; tcp++ ){
		util_unique_handle& mutex = (tcp ? this->tcpMutex : this->udpMutex);
		CSendTSTCPDllUtil* sendNW = (tcp ? &this->sendTcp : &this->sendUdp);
		SEND_INFO* info = this->sendInfo + tcp;
		DWORD* port = tcp ? tcpPort : udpPort;
		if( info->ip.empty() == false && (port == NULL || info->ip != ip) ){
			//終了
			info->ip.clear();
			sendNW->StopSend();
			sendNW->UnInitialize();
			mutex.reset();
		}
		if( port == NULL ){
			continue;
		}
		if( info->ip.empty() == false ){
			//開始済み。ポート番号を返す
			*port = info->port;
			continue;
		}
		//IPアドレスであること
		if( std::find_if(ip, ip + wcslen(ip), [](WCHAR c) {
		        return (c < L'0' || L'9' < c) && (c < L'A' || L'Z' < c) && (c < L'a' || L'z' < c) && c != L'%' && c != L'.' && c != L':'; }) != ip + wcslen(ip) ){
			continue;
		}

		int n;
		bool parsed = ParseIPv4Address(ip, n);
		//引数のポート番号は使わない(原作挙動)。ip:0.0.0.1-255は特別扱い
		info->port = (tcp ? (parsed && 1 <= n && n <= 255 ? 0 : BON_TCP_PORT_BEGIN) : BON_UDP_PORT_BEGIN);
		for( int i = 0; i < BON_NW_PORT_RANGE; i++, info->port++ ){
			LPCWSTR mutexName = tcp ? MUTEX_TCP_PORT_NAME : MUTEX_UDP_PORT_NAME;
			if( parsed ){
				Format(info->key, L"%ls%d_%d", mutexName, n, info->port);
			}else{
				Format(info->key, L"%ls%ls_%d", mutexName, ip, info->port);
			}
			mutex = UtilCreateGlobalMutex(info->key.c_str());
			if( mutex ){
				break;
			}
		}
		if( mutex ){
			//開始
			AddDebugLogFormat(L"Global\\%ls", info->key.c_str());
			sendNW->Initialize();
			if( tcp ){
				sendNW->AddSendAddr(ip, info->port);
			}else{
				int maxSendSize = GetPrivateProfileInt(L"SET", L"UDPPacket", 128, GetModuleIniPath().c_str()) * 188;
				sendNW->AddSendAddrUdp(ip, info->port, false, maxSendSize);
			}
			sendNW->StartSend();
			info->ip = ip;
			*port = info->port;
		}
	}
}

BOOL CTimeShiftUtil::OpenTimeShift(
	LPCWSTR filePath_,
	BOOL fileMode_
	)
{
	lock_recursive_mutex lock(this->utilLock);

	StopTimeShift();

	this->PCR_PID = 0xFFFF;
	if( UtilFileExists(filePath_).first == false ){
		return FALSE;
	}

	this->filePath = filePath_;
	this->fileMode = fileMode_;
	this->seekJitter = GetU32Tick() / 100 % 8 + 1;
	this->currentFilePos = 0;

	return TRUE;
}

BOOL CTimeShiftUtil::StartTimeShift()
{
	lock_recursive_mutex lock(this->utilLock);

	if( this->filePath.size() == 0 ){
		return FALSE;
	}else{
		if( this->readThread.joinable() == false ){
			//受信スレッド起動
			this->readStopFlag = false;
			this->readThread = thread_(ReadThread, this);
		}
	}

	return TRUE;
}

void CTimeShiftUtil::StopTimeShift()
{
	lock_recursive_mutex lock(this->utilLock);

	if( this->readThread.joinable() ){
		this->readStopFlag = true;
		this->readThread.join();
	}
	this->seekFile.reset();
	this->readFile.reset();
}

void CTimeShiftUtil::ReadThread(CTimeShiftUtil* sys)
{
	BYTE buff[188*256];
	CPacketInit packetInit;

	{
		lock_recursive_mutex lock(sys->ioLock);
		sys->readFile.reset(UtilOpenFile(sys->filePath, UTIL_SHARED_READ | UTIL_F_SEQUENTIAL));
		if( !sys->readFile ){
			return;
		}
		sys->seekFile.reset(UtilOpenFile(sys->filePath, UTIL_SHARED_READ));
		if( !sys->seekFile ){
			sys->readFile.reset();
			return;
		}
	}

	LONGLONG initTime = -1;
	LONGLONG base = -1;
	DWORD initTick = 0;
	vector<WORD> pcrPidList;
	DWORD errCount = 0;

	for(;;){
		{
			LONGLONG wait = 0;
			if( base >= 0 ){
				//レート調整
				wait = ((base + 0x200000000LL - initTime) & 0x1FFFFFFFFLL) / 90 - (GetU32Tick() - initTick);
				base = -1;
			}else if( errCount > 0 ){
				//終端監視中
				wait = 200;
			}
			for( ; wait > 0 && sys->readStopFlag == false; wait -= 20 ){
				SleepForMsec(20);
			}
			if( sys->readStopFlag ){
				break;
			}
		}
		lock_recursive_mutex lock(sys->ioLock);

		LONGLONG pos = my_ftell(sys->readFile.get());
		if( pos < 0 ){
			break;
		}
		if( pos != sys->currentFilePos ){
			//シークされた
			if( sys->currentFilePos >= sys->GetAvailableFileSize() ){
				//有効なデータの終端に達した
				if( sys->fileMode || ++errCount > 50 ){
					break;
				}
				continue;
			}
			if( my_fseek(sys->readFile.get(), sys->currentFilePos, SEEK_SET) != 0 ){
				break;
			}
			packetInit.ClearBuff();
			initTime = -1;
		}
		DWORD readSize = (DWORD)fread(buff, 1, sizeof(buff), sys->readFile.get());
		if( readSize < (sys->fileMode ? 1 : sizeof(buff)) ){
			//ファイル終端に達した
			if( sys->fileMode || ++errCount > 50 ){
				break;
			}
			if( my_fseek(sys->readFile.get(), sys->currentFilePos, SEEK_SET) != 0 ){
				break;
			}
			continue;
		}
		BYTE* data;
		DWORD dataSize;
		if( packetInit.GetTSData(buff, readSize, &data, &dataSize) == FALSE || dataSize <= 0 ){
			if( sys->fileMode == FALSE && sys->currentFilePos + (LONGLONG)sizeof(buff) > sys->GetAvailableFileSize() ){
				//無効なデータ領域を読んでいる可能性がある
				if( ++errCount > 50 ){
					break;
				}
				if( my_fseek(sys->readFile.get(), sys->currentFilePos, SEEK_SET) != 0 ){
					break;
				}
			}else{
				//不正なデータを読み飛ばす
				sys->currentFilePos += readSize;
				errCount = 0;
			}
			continue;
		}
		sys->currentFilePos += readSize;
		errCount = 0;

		for( DWORD i = 0; i < dataSize; i += 188 ){
			CTSPacketUtil packet;
			if( packet.Set188TS(data + i, 188) ){
				if( packet.has_adaptation_field_flags && packet.PCR_flag ){
					//最初に3回PCRが出現したPIDをPCR_PIDとする
					//PCR_PIDが現れることなく5回別のPCRが出現すればPCR_PIDを変更する
					if( packet.PID != sys->PCR_PID ){
						pcrPidList.push_back(packet.PID);
						if( std::count(pcrPidList.begin(), pcrPidList.end(), packet.PID) >= (sys->PCR_PID == 0xFFFF ? 3 : 5) ){
							sys->PCR_PID = packet.PID;
							initTime = -1;
						}
					}
					if( packet.PID == sys->PCR_PID ){
						pcrPidList.clear();
						base = packet.program_clock_reference_base;
						if( initTime < 0 ){
							initTime = base;
							initTick = GetU32Tick();
						}
					}
				}
			}
		}
		sys->sendUdp.AddSendData(data, dataSize);
		sys->sendTcp.AddSendData(data, dataSize);
	}

	lock_recursive_mutex lock(sys->ioLock);
	sys->seekFile.reset();
	sys->readFile.reset();

	if( sys->readStopFlag == false ){
		return;
	}
	//無効PAT送って次回送信時にリセットされるようにする
	std::fill_n(buff, sizeof(buff), (BYTE)0xFF);
	CCreatePATPacket patUtil;
	patUtil.SetParam(1, vector<pair<WORD, WORD>>());
	BYTE* patBuff;
	DWORD patSize=0;
	patUtil.GetPacket(&patBuff, &patSize);

	std::copy(patBuff, patBuff + patSize, buff);
	for( DWORD i=patSize; i+3<sizeof(buff); i+=188 ){
		buff[i] = 0x47;
		buff[i+1] = 0x1F;
		buff[i+2] = 0xFF;
		buff[i+3] = 0x10;
	}

	sys->sendUdp.AddSendData(buff, sizeof(buff));
	sys->sendTcp.AddSendData(buff, sizeof(buff));
}

static BOOL IsDataAvailable(FILE* fp, LONGLONG pos, CPacketInit* packetInit)
{
	if( my_fseek(fp, pos, SEEK_SET) == 0 ){
		BYTE buff[188 * 16];
		DWORD readSize = (DWORD)fread(buff, 1, sizeof(buff), fp);
		if( readSize > 0 ){
			packetInit->ClearBuff();
			BYTE* data;
			DWORD dataSize;
			if( packetInit->GetTSData(buff, readSize, &data, &dataSize) && dataSize > 0 ){
				return TRUE;
			}
		}
	}
	return FALSE;
}

LONGLONG CTimeShiftUtil::GetAvailableFileSize() const
{
	if( this->filePath.empty() == false ){
		std::unique_ptr<FILE, fclose_deleter> tmpFile;
		FILE* fp = this->seekFile.get();
		if( fp == NULL ){
			tmpFile.reset(UtilOpenFile(this->filePath, UTIL_SHARED_READ | UTIL_SH_DELETE));
			fp = tmpFile.get();
		}
		LONGLONG fileSize = -1;
		if( fp && my_fseek(fp, 0, SEEK_END) == 0 ){
			fileSize = my_ftell(fp);
		}
		if( this->fileMode ){
			//単純にファイルサイズを返す
			if( fileSize >= 0 ){
				return fileSize;
			}
		}else{
			//有効なデータのある範囲を調べる
			if( fileSize >= 188 * 16 * 8 ){
				CPacketInit packetInit;
				if( IsDataAvailable(fp, fileSize - 188 * 16 * this->seekJitter, &packetInit) == FALSE ){
					//終端部分が無効なので有効なデータの境目を探す
					//seekJitterは調査箇所がたまたま壊れている場合への対処
					LONGLONG range = fileSize - 188 * 16 * this->seekJitter;
					LONGLONG pos = range / 2 / 188 * 188;
					//ここは頻繁に呼ばれると高負荷に見えるが、ファイルキャッシュがよく効く条件なのでさほどでもない
					for( ; range > 256 * 1024; range /= 2 ){
						if( IsDataAvailable(fp, pos, &packetInit) ){
							pos += range / 4 / 188 * 188;
						}else{
							pos -= range / 4 / 188 * 188;
						}
					}
					//安定のため有効なデータの境目からさらに512KBだけ手前にする
					fileSize = max(pos - range / 2 - 512 * 1024, 0LL);
				}
				return fileSize;
			}
		}
	}
	return 0;
}

void CTimeShiftUtil::GetFilePos(LONGLONG* filePos, LONGLONG* fileSize)
{
	lock_recursive_mutex lock(this->utilLock);
	lock_recursive_mutex lock2(this->ioLock);

	if( filePos != NULL ){
		*filePos = this->currentFilePos;
	}
	if( fileSize != NULL ){
		*fileSize = GetAvailableFileSize();
	}
}

void CTimeShiftUtil::SetFilePos(LONGLONG filePos)
{
	lock_recursive_mutex lock(this->utilLock);
	lock_recursive_mutex lock2(this->ioLock);

	this->currentFilePos = filePos;
}
