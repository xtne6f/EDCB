#include "stdafx.h"
#include "TimeShiftUtil.h"

CTimeShiftUtil::CTimeShiftUtil(void)
{
	this->readFile = INVALID_HANDLE_VALUE;
	this->seekFile = INVALID_HANDLE_VALUE;

	this->PCR_PID = 0xFFFF;
	this->fileMode = FALSE;
	this->seekJitter = 1;
	this->currentFilePos = 0;
}


CTimeShiftUtil::~CTimeShiftUtil(void)
{
	StopTimeShift();

	NWPLAY_PLAY_INFO val = {};
	Send(&val);
}

BOOL CTimeShiftUtil::Send(
	NWPLAY_PLAY_INFO* val
	)
{
	CBlockLock lock(&this->utilLock);
	CBlockLock lock2(&this->ioLock);

	//送信先を設定する
	WCHAR ip[64];
	swprintf_s(ip, L"%d.%d.%d.%d", val->ip >> 24, val->ip >> 16 & 0xFF, val->ip >> 8 & 0xFF, val->ip & 0xFF);

	if( this->sendUdpIP.empty() == false && (val->udp == 0 || this->sendUdpIP != ip) ){
		this->sendUdpIP.clear();
		this->sendUdp.CloseUpload();
		CloseHandle(this->udpPortMutex);
	}
	if( this->sendUdpIP.empty() && val->udp != 0 ){
		NW_SEND_INFO item;
		item.port = 1234;
		item.ipString = ip;
		item.broadcastFlag = FALSE;
		wstring mutexKey;
		for( ; item.port < 1234 + 100; item.port++ ){
			Format(mutexKey, L"%s%d_%d", MUTEX_UDP_PORT_NAME, val->ip, item.port);
			this->udpPortMutex = CreateMutex(NULL, FALSE, mutexKey.c_str());
			if( this->udpPortMutex ){
				if( GetLastError() != ERROR_ALREADY_EXISTS ){
					break;
				}
				CloseHandle(this->udpPortMutex);
				this->udpPortMutex = NULL;
			}
		}
		if( this->udpPortMutex ){
			OutputDebugString((mutexKey + L"\r\n").c_str());
			vector<NW_SEND_INFO> sendList(1, item);
			this->sendUdp.StartUpload(&sendList);
			this->sendUdpIP = ip;
			this->sendUdpPort = item.port;
		}
	}
	if( this->sendUdpIP.empty() == false ){
		val->udpPort = this->sendUdpPort;
	}

	if( this->sendTcpIP.empty() == false && (val->tcp == 0 || this->sendTcpIP != ip) ){
		this->sendTcpIP.clear();
		this->sendTcp.CloseUpload();
		CloseHandle(this->tcpPortMutex);
	}
	if( this->sendTcpIP.empty() && val->tcp != 0 ){
		NW_SEND_INFO item;
		item.port = 2230;
		item.ipString = ip;
		item.broadcastFlag = FALSE;
		wstring mutexKey;
		for( ; item.port < 2230 + 100; item.port++ ){
			Format(mutexKey, L"%s%d_%d", MUTEX_TCP_PORT_NAME, val->ip, item.port);
			this->tcpPortMutex = CreateMutex(NULL, FALSE, mutexKey.c_str());
			if( this->tcpPortMutex ){
				if( GetLastError() != ERROR_ALREADY_EXISTS ){
					break;
				}
				CloseHandle(this->tcpPortMutex);
				this->tcpPortMutex = NULL;
			}
		}
		if( this->tcpPortMutex ){
			OutputDebugString((mutexKey + L"\r\n").c_str());
			vector<NW_SEND_INFO> sendList(1, item);
			this->sendTcp.StartUpload(&sendList);
			this->sendTcpIP = ip;
			this->sendTcpPort = item.port;
		}
	}
	if( this->sendTcpIP.empty() == false ){
		val->tcpPort = this->sendTcpPort;
	}
	return TRUE;
}

BOOL CTimeShiftUtil::OpenTimeShift(
	LPCWSTR filePath_,
	BOOL fileMode_
	)
{
	CBlockLock lock(&this->utilLock);

	StopTimeShift();

	this->PCR_PID = 0xFFFF;
	if( GetFileAttributes(filePath_) == INVALID_FILE_ATTRIBUTES ){
		return FALSE;
	}

	this->filePath = filePath_;
	this->fileMode = fileMode_;
	this->seekJitter = GetTickCount() / 100 % 8 + 1;
	this->currentFilePos = 0;

	return TRUE;
}

BOOL CTimeShiftUtil::StartTimeShift()
{
	CBlockLock lock(&this->utilLock);

	if( this->filePath.size() == 0 ){
		return FALSE;
	}else{
		if( this->readThread.joinable() == false ){
			//受信スレッド起動
			this->readStopFlag = FALSE;
			this->readThread = thread_(ReadThread, this);
		}
	}

	return TRUE;
}

BOOL CTimeShiftUtil::StopTimeShift()
{
	CBlockLock lock(&this->utilLock);

	if( this->readThread.joinable() ){
		this->readStopFlag = TRUE;
		this->readThread.join();
	}
	if( this->seekFile != INVALID_HANDLE_VALUE ){
		CloseHandle(this->seekFile);
		this->seekFile = INVALID_HANDLE_VALUE;
	}
	if( this->readFile != INVALID_HANDLE_VALUE ){
		CloseHandle(this->readFile);
		this->readFile = INVALID_HANDLE_VALUE;
	}
	return TRUE;
}

void CTimeShiftUtil::ReadThread(CTimeShiftUtil* sys)
{
	BYTE buff[188*256];
	CPacketInit packetInit;

	{
		CBlockLock lock(&sys->ioLock);
		sys->readFile = CreateFile(sys->filePath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
		                           NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
		if( sys->readFile == INVALID_HANDLE_VALUE ){
			return;
		}
		sys->seekFile = CreateFile(sys->filePath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
		                           NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if( sys->seekFile == INVALID_HANDLE_VALUE ){
			CloseHandle(sys->readFile);
			sys->readFile = INVALID_HANDLE_VALUE;
			return;
		}
	}

	__int64 initTime = -1;
	__int64 base = -1;
	DWORD initTick = 0;
	vector<WORD> pcrPidList;
	DWORD errCount = 0;

	for(;;){
		{
			__int64 wait = 0;
			if( base >= 0 ){
				//レート調整
				wait = ((base + 0x200000000LL - initTime) & 0x1FFFFFFFFLL) / 90 - (GetTickCount() - initTick);
				base = -1;
			}else if( errCount > 0 ){
				//終端監視中
				wait = 200;
			}
			for( ; wait > 0 && sys->readStopFlag == FALSE; wait -= 20 ){
				Sleep(20);
			}
			if( sys->readStopFlag ){
				break;
			}
		}
		CBlockLock lock(&sys->ioLock);

		LARGE_INTEGER liMove = {};
		if( SetFilePointerEx(sys->readFile, liMove, &liMove, FILE_CURRENT) == FALSE ){
			break;
		}
		if( liMove.QuadPart != sys->currentFilePos ){
			//シークされた
			if( sys->currentFilePos >= sys->GetAvailableFileSize() ){
				//有効なデータの終端に達した
				if( sys->fileMode || ++errCount > 50 ){
					break;
				}
				continue;
			}
			liMove.QuadPart = sys->currentFilePos;
			if( SetFilePointerEx(sys->readFile, liMove, NULL, FILE_BEGIN) == FALSE ){
				break;
			}
			packetInit.ClearBuff();
			initTime = -1;
		}
		DWORD readSize;
		if( ReadFile(sys->readFile, buff, sizeof(buff), &readSize, NULL) == FALSE ||
		    readSize < (sys->fileMode ? 1 : sizeof(buff)) ){
			//ファイル終端に達した
			if( sys->fileMode || ++errCount > 50 ){
				break;
			}
			liMove.QuadPart = sys->currentFilePos;
			if( SetFilePointerEx(sys->readFile, liMove, NULL, FILE_BEGIN) == FALSE ){
				break;
			}
			continue;
		}
		BYTE* data;
		DWORD dataSize;
		if( packetInit.GetTSData(buff, readSize, &data, &dataSize) == FALSE || dataSize <= 0 ){
			if( sys->fileMode == FALSE && sys->currentFilePos + (__int64)sizeof(buff) > sys->GetAvailableFileSize() ){
				//無効なデータ領域を読んでいる可能性がある
				if( ++errCount > 50 ){
					break;
				}
				liMove.QuadPart = sys->currentFilePos;
				if( SetFilePointerEx(sys->readFile, liMove, NULL, FILE_BEGIN) == FALSE ){
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
				if( packet.adaptation_field_length > 0 && packet.PCR_flag == 1 ){
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
							initTick = GetTickCount();
						}
					}
				}
			}
		}
		if( sys->sendUdpIP.empty() == false ){
			sys->sendUdp.SendData(data, dataSize);
		}
		if( sys->sendTcpIP.empty() == false ){
			sys->sendTcp.SendData(data, dataSize);
		}
	}

	CBlockLock lock(&sys->ioLock);
	CloseHandle(sys->seekFile);
	sys->seekFile = INVALID_HANDLE_VALUE;
	CloseHandle(sys->readFile);
	sys->readFile = INVALID_HANDLE_VALUE;

	if( sys->readStopFlag == FALSE ){
		return;
	}
	//無効PAT送って次回送信時にリセットされるようにする
	memset(buff, 0xFF, sizeof(buff));
	CCreatePATPacket patUtil;
	patUtil.SetParam(1, vector<pair<WORD, WORD>>());
	BYTE* patBuff;
	DWORD patSize=0;
	patUtil.GetPacket(&patBuff, &patSize);

	memcpy(buff, patBuff, patSize);
	for( DWORD i=patSize; i+3<sizeof(buff); i+=188 ){
		buff[i] = 0x47;
		buff[i+1] = 0x1F;
		buff[i+2] = 0xFF;
		buff[i+3] = 0x10;
	}

	if( sys->sendUdpIP.empty() == false ){
		sys->sendUdp.SendData(buff, sizeof(buff));
	}
	if( sys->sendTcpIP.empty() == false ){
		sys->sendTcp.SendData(buff, sizeof(buff));
	}
}

static BOOL IsDataAvailable(HANDLE file, __int64 pos, CPacketInit* packetInit)
{
	LARGE_INTEGER liPos;
	liPos.QuadPart = pos;
	if( SetFilePointerEx(file, liPos, NULL, FILE_BEGIN) ){
		BYTE buff[188 * 16];
		DWORD readSize;
		if( ReadFile(file, buff, sizeof(buff), &readSize, NULL) ){
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

__int64 CTimeShiftUtil::GetAvailableFileSize() const
{
	if( this->filePath.empty() == false ){
		if( this->fileMode ){
			//単純にファイルサイズを返す
			if( this->seekFile == INVALID_HANDLE_VALUE ){
				WIN32_FIND_DATA findData;
				HANDLE find = FindFirstFile(this->filePath.c_str(), &findData);
				if( find != INVALID_HANDLE_VALUE ){
					FindClose(find);
					return (__int64)findData.nFileSizeHigh << 32 | findData.nFileSizeLow;
				}
			}else{
				LARGE_INTEGER liSize;
				if( GetFileSizeEx(this->seekFile, &liSize) ){
					return liSize.QuadPart;
				}
			}
		}else{
			//有効なデータのある範囲を調べる
			HANDLE file = this->seekFile;
			if( file == INVALID_HANDLE_VALUE ){
				file = CreateFile(this->filePath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
				                  NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			}
			if( file != INVALID_HANDLE_VALUE ){
				LARGE_INTEGER liSize;
				CPacketInit packetInit;
				if( GetFileSizeEx(file, &liSize) == FALSE || liSize.QuadPart < 188 * 16 * 8 ){
					liSize.QuadPart = 0;
				}else if( IsDataAvailable(file, liSize.QuadPart - 188 * 16 * this->seekJitter, &packetInit) == FALSE ){
					//終端部分が無効なので有効なデータの境目を探す
					//seekJitterは調査箇所がたまたま壊れている場合への対処
					__int64 range = liSize.QuadPart - 188 * 16 * this->seekJitter;
					__int64 pos = range / 2 / 188 * 188;
					//ここは頻繁に呼ばれると高負荷に見えるが、ファイルキャッシュがよく効く条件なのでさほどでもない
					for( ; range > 256 * 1024; range /= 2 ){
						if( IsDataAvailable(file, pos, &packetInit) ){
							pos += range / 4 / 188 * 188;
						}else{
							pos -= range / 4 / 188 * 188;
						}
					}
					//安定のため有効なデータの境目からさらに512KBだけ手前にする
					liSize.QuadPart = max(pos - range / 2 - 512 * 1024, 0LL);
				}
				if( file != this->seekFile ){
					CloseHandle(file);
				}
				return liSize.QuadPart;
			}
		}
	}
	return 0;
}

void CTimeShiftUtil::GetFilePos(__int64* filePos, __int64* fileSize)
{
	CBlockLock lock(&this->utilLock);
	CBlockLock lock2(&this->ioLock);

	if( filePos != NULL ){
		*filePos = this->currentFilePos;
	}
	if( fileSize != NULL ){
		*fileSize = GetAvailableFileSize();
	}
}

void CTimeShiftUtil::SetFilePos(__int64 filePos)
{
	CBlockLock lock(&this->utilLock);
	CBlockLock lock2(&this->ioLock);

	this->currentFilePos = filePos;
}
