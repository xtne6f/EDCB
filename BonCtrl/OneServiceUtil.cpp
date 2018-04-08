#include "stdafx.h"
#include "OneServiceUtil.h"


COneServiceUtil::COneServiceUtil(BOOL sendUdpTcp_)
{
	this->sendUdpTcp = sendUdpTcp_;
	this->SID = 0xFFFF;

	this->pmtPID = 0xFFFF;

	this->enableScramble = TRUE;

	this->pittariStart = FALSE;
	this->pittariEndChk = FALSE;
}


COneServiceUtil::~COneServiceUtil(void)
{
	SendUdp(NULL);
	SendTcp(NULL);
}

//処理対象ServiceIDを設定
//引数：
// SID			[IN]ServiceID
void COneServiceUtil::SetSID(
	WORD SID_
)
{
	if( this->SID != SID_ ){
		this->pmtPID = 0xFFFF;
		this->emmPIDList.clear();

		this->dropCount.Clear();
	}
	this->SID = SID_;
}

//設定されてる処理対象のServiceIDを取得
//戻り値：
// ServiceID
WORD COneServiceUtil::GetSID()
{
	return this->SID;
}

//UDPで送信を行う
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// sendList		[IN/OUT]送信先リスト。NULLで停止。Portは実際に送信に使用したPortが返る。
BOOL COneServiceUtil::SendUdp(
	vector<NW_SEND_INFO>* sendList
	)
{
	if( this->sendUdp != NULL ){
		this->sendUdp->CloseUpload();
	}

	if(udpPortMutex.size() != 0){
		for( int i=0; i<(int)udpPortMutex.size(); i++ ){
			::CloseHandle(udpPortMutex[i]);
		}
		udpPortMutex.clear();
	}

	if( sendList != NULL ){
		if( this->sendUdp == NULL ){
			this->sendUdp.reset(new CSendUDP);
		}
		for( size_t i=0; i<sendList->size(); i++ ){
			wstring key = L"";
			HANDLE portMutex;

			//生成できなくても深刻ではないのでほどほどに打ち切る
			for( int j = 0; j < 100; j++ ){
				UINT u[4];
				if( swscanf_s((*sendList)[i].ipString.c_str(), L"%u.%u.%u.%u", &u[0], &u[1], &u[2], &u[3]) == 4 ){
					Format(key, L"%s%d_%d", MUTEX_UDP_PORT_NAME, u[0] << 24 | u[1] << 16 | u[2] << 8 | u[3], (*sendList)[i].port);
				}else{
					Format(key, L"%s%s_%d", MUTEX_UDP_PORT_NAME, (*sendList)[i].ipString.c_str(), (*sendList)[i].port);
				}
				portMutex = CreateMutex(NULL, FALSE, key.c_str());
		
				if( portMutex == NULL ){
					(*sendList)[i].port++;
				}else if( GetLastError() == ERROR_ALREADY_EXISTS ){
					CloseHandle(portMutex);
					(*sendList)[i].port++;
				}else{
					_OutputDebugString(L"%s\r\n", key.c_str());
					this->udpPortMutex.push_back(portMutex);
					break;
				}
			}
		}

		this->sendUdp->StartUpload(sendList);
	}else{
		this->sendUdp.reset();
	}

	return TRUE;
}

//TCPで送信を行う
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// sendList		[IN/OUT]送信先リスト。NULLで停止。Portは実際に送信に使用したPortが返る。
BOOL COneServiceUtil::SendTcp(
	vector<NW_SEND_INFO>* sendList
	)
{
	if( this->sendTcp != NULL ){
		this->sendTcp->CloseUpload();
	}

	if(tcpPortMutex.size() != 0){
		for( int i=0; i<(int)tcpPortMutex.size(); i++ ){
			::CloseHandle(tcpPortMutex[i]);
		}
		tcpPortMutex.clear();
	}

	if( sendList != NULL ){
		if( this->sendTcp == NULL ){
			this->sendTcp.reset(new CSendTCP);
		}
		for( size_t i=0; i<sendList->size(); i++ ){
			wstring key = L"";
			HANDLE portMutex;

			//生成できなくても深刻ではないのでほどほどに打ち切る
			for( int j = 0; j < 100; j++ ){
				UINT u[4];
				if( swscanf_s((*sendList)[i].ipString.c_str(), L"%u.%u.%u.%u", &u[0], &u[1], &u[2], &u[3]) == 4 ){
					Format(key, L"%s%d_%d", MUTEX_TCP_PORT_NAME, u[0] << 24 | u[1] << 16 | u[2] << 8 | u[3], (*sendList)[i].port);
				}else{
					Format(key, L"%s%s_%d", MUTEX_TCP_PORT_NAME, (*sendList)[i].ipString.c_str(), (*sendList)[i].port);
				}
				portMutex = CreateMutex(NULL, FALSE, key.c_str());
		
				if( portMutex == NULL ){
					(*sendList)[i].port++;
				}else if( GetLastError() == ERROR_ALREADY_EXISTS ){
					CloseHandle(portMutex);
					(*sendList)[i].port++;
				}else{
					_OutputDebugString(L"%s\r\n", key.c_str());
					this->tcpPortMutex.push_back(portMutex);
					break;
				}
			}
		}

		this->sendTcp->StartUpload(sendList);
	}else{
		this->sendTcp.reset();
	}

	return TRUE;
}

//出力用TSデータを送る
//引数：
// data		[IN]TSデータ
// size		[IN]dataのサイズ
// funcGetPresent	[IN]EPGの現在番組IDを調べる関数
void COneServiceUtil::AddTSBuff(
	BYTE* data,
	DWORD size,
	const std::function<int(WORD, WORD, WORD)>& funcGetPresent
	)
{
	if( this->sendUdpTcp ){
		if( size > 0 ){
			if( this->sendUdp ){
				this->sendUdp->SendData(data, size);
			}
			if( this->sendTcp ){
				this->sendTcp->SendData(data, size);
			}
		}
		this->dropCount.AddData(data, size);
	}else if( this->SID == 0xFFFF ){
		//全サービス扱い
		if( this->writeFile ){
			this->writeFile->AddTSBuff(data, size);
		}
		this->dropCount.AddData(data, size);

		for( DWORD i=0; i<size; i+=188 ){
			CTSPacketUtil packet;
			if( packet.Set188TS(data + i, 188) == TRUE ){
				if( packet.PID == this->pmtPID ){
					createPmt.AddData(&packet);
				}
			}
		}
	}else{
		this->buff.clear();

		for( DWORD i=0; i<size; i+=188 ){
			CTSPacketUtil packet;
			if( packet.Set188TS(data + i, 188) == TRUE ){
				if( packet.PID == 0x0000 ){
					//PAT
					BYTE* patBuff = NULL;
					DWORD patBuffSize = 0;
					if( createPat.GetPacket(&patBuff, &patBuffSize) == TRUE ){
						if( packet.payload_unit_start_indicator == 1 ){
							this->buff.insert(this->buff.end(), patBuff, patBuff + patBuffSize);
						}
					}
				}else if( packet.PID == this->pmtPID ){
					//PMT
					DWORD err = createPmt.AddData(&packet);
					if( err == NO_ERR || err == CCreatePMTPacket::ERR_NO_CHAGE ){
						BYTE* pmtBuff = NULL;
						DWORD pmtBuffSize = 0;
						if( createPmt.GetPacket(&pmtBuff, &pmtBuffSize) == TRUE ){
							this->buff.insert(this->buff.end(), pmtBuff, pmtBuff + pmtBuffSize);
						}else{
							_OutputDebugString(L"createPmt.GetPacket Err");
							//そのまま
							this->buff.insert(this->buff.end(), data + i, data + i + 188);
						}
					}else if( err == FALSE ){
						_OutputDebugString(L"createPmt.AddData Err");
						//そのまま
						this->buff.insert(this->buff.end(), data + i, data + i + 188);
					}
				}else{
					//その他
					if( packet.PID < 0x0030 ){
						//そのまま
						this->buff.insert(this->buff.end(), data + i, data + i + 188);
					}else{
						if( createPmt.IsNeedPID(packet.PID) == TRUE ){
							//PMTで定義されてる
							this->buff.insert(this->buff.end(), data + i, data + i + 188);
						}else{
							//EMMなら必要
							if( std::binary_search(this->emmPIDList.begin(), this->emmPIDList.end(), packet.PID) ){
								this->buff.insert(this->buff.end(), data + i, data + i + 188);
							}
						}
					}
				}
			}
		}

		if( this->buff.empty() == false ){
			if( this->writeFile ){
				this->writeFile->AddTSBuff(this->buff.data(), (DWORD)this->buff.size());
			}
			this->dropCount.AddData(this->buff.data(), (DWORD)this->buff.size());
		}
	}

	if( this->pittariStart == TRUE ){
		if( this->lastPMTVer == 0xFFFF ){
			this->lastPMTVer = createPmt.GetVersion();
		}else if(this->lastPMTVer != createPmt.GetVersion()){
			//ぴったり開始
			StratPittariRec();
			this->lastPMTVer = createPmt.GetVersion();
		}
		if( funcGetPresent ){
			int eventID = funcGetPresent(this->pittariONID, this->pittariTSID, this->pittariSID);
			if( eventID >= 0 ){
				if( eventID == this->pittariEventID ){
					//ぴったり開始
					StratPittariRec();
					this->pittariStart = FALSE;
					this->pittariEndChk = TRUE;
				}
			}
		}
	}
	if( this->pittariEndChk == TRUE ){
		if( funcGetPresent ){
			int eventID = funcGetPresent(this->pittariONID, this->pittariTSID, this->pittariSID);
			if( eventID >= 0 ){
				if( eventID != this->pittariEventID ){
					//ぴったり終了
					StopPittariRec();
					this->pittariEndChk = FALSE;
				}
			}
		}
	}
}

void COneServiceUtil::SetPmtPID(
	WORD TSID,
	WORD pmtPID_
	)
{
	if( this->pmtPID != pmtPID_ && this->SID != 0xFFFF){
		_OutputDebugString(L"COneServiceUtil::SetPmtPID 0x%04x => 0x%04x", this->pmtPID, pmtPID_);
		vector<pair<WORD, WORD>> pidList;
		pidList.push_back(pair<WORD, WORD>(0x10, 0));
		pidList.push_back(pair<WORD, WORD>(pmtPID_, this->SID));
		this->createPat.SetParam(TSID, pidList);

		this->pmtPID = pmtPID_;
	}
}

void COneServiceUtil::SetEmmPID(
	const vector<WORD>& pidList
	)
{
	this->emmPIDList = pidList;
	std::sort(this->emmPIDList.begin(), this->emmPIDList.end());
}

//ファイル保存を開始する
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// fileName				[IN]保存ファイルパス
// overWriteFlag		[IN]同一ファイル名存在時に上書きするかどうか（TRUE：する、FALSE：しない）
// pittariFlag			[IN]ぴったりモード（TRUE：する、FALSE：しない）
// pittariONID			[IN]ぴったりモードで録画するONID
// pittariTSID			[IN]ぴったりモードで録画するTSID
// pittariSID			[IN]ぴったりモードで録画するSID
// pittariEventID		[IN]ぴったりモードで録画するイベントID
// createSize			[IN]ファイル作成時にディスクに予約する容量
// saveFolder			[IN]使用するフォルダ一覧
// saveFolderSub		[IN]HDDの空きがなくなった場合に一時的に使用するフォルダ
BOOL COneServiceUtil::StartSave(
	const wstring& fileName,
	BOOL overWriteFlag,
	BOOL pittariFlag,
	WORD pittariONID_,
	WORD pittariTSID_,
	WORD pittariSID_,
	WORD pittariEventID_,
	ULONGLONG createSize,
	const vector<REC_FILE_SET_INFO>& saveFolder,
	const vector<wstring>& saveFolderSub,
	int maxBuffCount
)
{
	if( pittariFlag == FALSE ){
		if( this->writeFile == NULL ){
			OutputDebugString(L"*:StartSave");
			this->pittariRecFilePath = L"";
			this->pittariStart = FALSE;
			this->pittariEndChk = FALSE;

			this->writeFile.reset(new CWriteTSFile);
			return this->writeFile->StartSave(fileName, overWriteFlag, createSize, saveFolder, saveFolderSub, maxBuffCount);
		}
	}else{
		if( this->writeFile == NULL ){
			OutputDebugString(L"*:StartSave pittariFlag");
			this->pittariRecFilePath = L"";
			this->pittariFileName = fileName;
			this->pittariOverWriteFlag = overWriteFlag;
			this->pittariCreateSize = createSize;
			this->pittariSaveFolder = saveFolder;
			this->pittariSaveFolderSub = saveFolderSub;
			this->pittariMaxBuffCount = maxBuffCount;
			this->pittariONID = pittariONID_;
			this->pittariTSID = pittariTSID_;
			this->pittariSID = pittariSID_;
			this->pittariEventID = pittariEventID_;

			this->lastPMTVer = 0xFFFF;

			this->pittariStart = TRUE;
			this->pittariEndChk = FALSE;

			return TRUE;
		}
	}

	return FALSE;
}

void COneServiceUtil::StratPittariRec()
{
	if( this->writeFile == NULL ){
		OutputDebugString(L"*:StratPittariRec");
		this->writeFile.reset(new CWriteTSFile);
		this->writeFile->StartSave(this->pittariFileName, this->pittariOverWriteFlag, this->pittariCreateSize,
		                           this->pittariSaveFolder, this->pittariSaveFolderSub, this->pittariMaxBuffCount);
	}
}

void COneServiceUtil::StopPittariRec()
{
	if( this->writeFile == NULL ){
		return ;
	}
	OutputDebugString(L"*:StopPittariRec");
	BOOL subRec;
	this->writeFile->GetSaveFilePath(&this->pittariRecFilePath, &subRec);
	this->writeFile->EndSave();
}

//ファイル保存を終了する
//戻り値：
// TRUE（成功）、FALSE（失敗）
BOOL COneServiceUtil::EndSave()
{
	this->pittariRecFilePath = L"";
	this->pittariStart = FALSE;
	this->pittariEndChk = FALSE;

	if( this->writeFile == NULL ){
		return FALSE;
	}
	BOOL ret = this->writeFile->EndSave();
	this->writeFile.reset();
	OutputDebugString(L"*:EndSave");
	return ret;
}

//録画中かどうか
//戻り値：
// TRUE（録画中）、FALSE（していない）
BOOL COneServiceUtil::IsRec()
{
	if( this->writeFile == NULL ){
		if( this->pittariStart == TRUE || this->pittariEndChk == TRUE ){
			return TRUE;
		}
		return FALSE;
	}else{
		return TRUE;
	}
}

//スクランブル解除処理の動作設定
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// enable		[IN] TRUE（処理する）、FALSE（処理しない）
void COneServiceUtil::SetScramble(
	BOOL enable
	)
{
	this->enableScramble = enable;
}

//スクランブル解除処理を行うかどうか
//戻り値：
// TRUE（処理する）、FALSE（処理しない）
BOOL COneServiceUtil::GetScramble(
	)
{
	return this->enableScramble;
}

//字幕とデータ放送含めるかどうか
//引数：
// enableCaption		[IN]字幕を TRUE（含める）、FALSE（含めない）
// enableData			[IN]データ放送を TRUE（含める）、FALSE（含めない）
void COneServiceUtil::SetServiceMode(
	BOOL enableCaption,
	BOOL enableData
	)
{
	createPmt.SetCreateMode(enableCaption, enableData);
}

//エラーカウントをクリアする
void COneServiceUtil::ClearErrCount()
{
	this->dropCount.Clear();
}

//ドロップとスクランブルのカウントを取得する
//引数：
// drop				[OUT]ドロップ数
// scramble			[OUT]スクランブル数
void COneServiceUtil::GetErrCount(ULONGLONG* drop, ULONGLONG* scramble)
{
	this->dropCount.GetCount(drop, scramble);
}

//録画中のファイルのファイルパスを取得する
//引数：
// filePath			[OUT]保存ファイル名
// subRecFlag		[OUT]サブ録画が発生したかどうか
void COneServiceUtil::GetSaveFilePath(
	wstring* filePath,
	BOOL* subRecFlag
	)
{
	if( this->writeFile != NULL ){
		this->writeFile->GetSaveFilePath(filePath, subRecFlag);
		if( filePath->size() == 0 ){
			*filePath = this->pittariRecFilePath;
		}
	}
}

//ドロップとスクランブルのカウントを保存する
//引数：
// filePath			[IN]保存ファイル名
void COneServiceUtil::SaveErrCount(
	const wstring& filePath
	)
{
	this->dropCount.SaveLog(filePath);
}


void COneServiceUtil::SetSignalLevel(
	float signalLv
	)
{
	this->dropCount.SetSignal(signalLv);
}

//録画中のファイルの出力サイズを取得する
//引数：
// writeSize			[OUT]出力サイズ
void COneServiceUtil::GetRecWriteSize(
	__int64* writeSize
	)
{
	if( this->writeFile != NULL ){
		this->writeFile->GetRecWriteSize(writeSize);
	}
}

void COneServiceUtil::SetBonDriver(
	const wstring& bonDriver
	)
{
	this->dropCount.SetBonDriver(bonDriver);
}

void COneServiceUtil::SetPIDName(
	WORD pid,
	LPCSTR name
	)
{
	this->dropCount.SetPIDName(pid, name);
}

void COneServiceUtil::SetNoLogScramble(
	BOOL noLog
	)
{
	this->dropCount.SetNoLog(FALSE, noLog);
}
