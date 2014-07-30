#include "StdAfx.h"
#include "OneServiceUtil.h"


COneServiceUtil::COneServiceUtil(void)
{
	this->id = 0;
	this->SID = 0xFFFF;

	this->sendUdp = NULL;
	this->sendTcp = NULL;
	this->writeFile = NULL;

	this->pmtPID = 0xFFFF;

	this->enableScramble = TRUE;
	this->epgUtil = NULL;

	this->pittariStart = FALSE;
	this->pittariEndChk = FALSE;
	this->maxBuffCount = -1;
}


COneServiceUtil::~COneServiceUtil(void)
{
	SAFE_DELETE(this->sendUdp);
	SAFE_DELETE(this->sendTcp);
	SAFE_DELETE(this->writeFile);
}

void COneServiceUtil::SetEpgUtil(
	CEpgDataCap3Util* epgUtil
	)
{
	this->epgUtil = epgUtil;
}


//識別IDの設定
//引数：
// id			[IN]識別ID
void COneServiceUtil::SetID(
	DWORD id
	)
{
	this->id = id;
}

//識別IDの取得
//戻り値：
// 識別ID
DWORD COneServiceUtil::GetID()
{
	return this->id;
}

//処理対象ServiceIDを設定
//引数：
// SID			[IN]ServiceID
void COneServiceUtil::SetSID(
	WORD SID
)
{
	if( this->SID != SID ){
		this->pmtPID = 0xFFFF;
		this->emmPIDMap.clear();

		this->dropCount.Clear();
	}
	this->SID = SID;
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
// id			[IN]制御識別ID
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
			::ReleaseMutex(udpPortMutex[i]);
			::CloseHandle(udpPortMutex[i]);
		}
		udpPortMutex.clear();
	}

	if( sendList != NULL ){
		if( this->sendUdp == NULL ){
			this->sendUdp = new CSendUDP;
		}
		for( size_t i=0; i<sendList->size(); i++ ){
			wstring key = L"";

			while(1){
				Format(key, L"%s%d_%d", MUTEX_UDP_PORT_NAME, (*sendList)[i].ip, (*sendList)[i].port );
				HANDLE mutex = ::OpenMutex(MUTEX_ALL_ACCESS, FALSE, key.c_str());
		
				if(mutex){
					::CloseHandle(mutex);
					(*sendList)[i].port++;
				}else{
					break;
				}
			}

			HANDLE portMutex = _CreateMutex( TRUE, key.c_str());
			_OutputDebugString(L"%s\r\n", key.c_str());
			udpPortMutex.push_back(portMutex);
		}

		this->sendUdp->StartUpload(sendList);
	}else{
		SAFE_DELETE(this->sendUdp);
	}

	return TRUE;
}

//TCPで送信を行う
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// id			[IN]制御識別ID
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
			::ReleaseMutex(tcpPortMutex[i]);
			::CloseHandle(tcpPortMutex[i]);
		}
		tcpPortMutex.clear();
	}

	if( sendList != NULL ){
		if( this->sendTcp == NULL ){
			this->sendTcp = new CSendTCP;
		}
		for( size_t i=0; i<sendList->size(); i++ ){
			wstring key = L"";

			while(1){
				Format(key, L"%s%d_%d", MUTEX_TCP_PORT_NAME, (*sendList)[i].ip, (*sendList)[i].port );
				HANDLE mutex = ::OpenMutex(MUTEX_ALL_ACCESS, FALSE, key.c_str());
		
				if(mutex){
					::CloseHandle(mutex);
					(*sendList)[i].port++;
				}else{
					break;
				}
			}

			HANDLE portMutex = _CreateMutex( TRUE, key.c_str());
			_OutputDebugString(L"%s\r\n", key.c_str());
			tcpPortMutex.push_back(portMutex);
		}

		this->sendTcp->StartUpload(sendList);
	}else{
		SAFE_DELETE(this->sendTcp);
	}

	return TRUE;
}

//出力用TSデータを送る
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// data		[IN]TSデータ
// size		[IN]dataのサイズ
BOOL COneServiceUtil::AddTSBuff(
	BYTE* data,
	DWORD size
	)
{
	BOOL ret = TRUE;
	if( this->SID == 0xFFFF || this->sendTcp != NULL || this->sendUdp != NULL){
		//全サービス扱い
		if( data != NULL ){
			ret = WriteData(data, size);
		}
		for( DWORD i=0; i<size; i+=188 ){
			CTSPacketUtil packet;
			if( packet.Set188TS(data + i, 188) == TRUE ){
				if( packet.PID == this->pmtPID ){
					DWORD err = createPmt.AddData(&packet);
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
					if( err == NO_ERR || err == ERR_NO_CHAGE ){
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
							map<WORD,WORD>::iterator itr;
							itr = this->emmPIDMap.find(packet.PID);
							if( itr != this->emmPIDMap.end() ){
								this->buff.insert(this->buff.end(), data + i, data + i + 188);
							}
						}
					}
				}
			}
		}

		if( this->buff.empty() == false ){
			ret = WriteData(&this->buff.front(), (DWORD)this->buff.size());
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
		if( this->epgUtil != NULL ){
			EPG_EVENT_INFO* epgInfo;
			if( this->epgUtil->GetEpgInfo( this->pittariONID, this->pittariTSID, this->pittariSID, FALSE, &epgInfo ) == NO_ERR ){
				if( epgInfo->event_id == this->pittariEventID ){
					//ぴったり開始
					StratPittariRec();
					this->pittariStart = FALSE;
					this->pittariEndChk = TRUE;
				}
			}
		}
	}
	if( this->pittariEndChk == TRUE ){
		if( this->epgUtil != NULL ){
			EPG_EVENT_INFO* epgInfo;
			if( this->epgUtil->GetEpgInfo( this->pittariONID, this->pittariTSID, this->pittariSID, FALSE, &epgInfo ) == NO_ERR ){
				if( epgInfo->event_id != this->pittariEventID ){
					//ぴったり終了
					StopPittariRec();
					this->pittariEndChk = FALSE;
				}
			}
		}
	}

	return ret;
}

BOOL COneServiceUtil::WriteData(BYTE* data, DWORD size)
{
	if( this->sendUdp != NULL ){
		this->sendUdp->SendData(data, size);
	}
	if( this->sendTcp != NULL ){
		this->sendTcp->SendData(data, size);
	}
	if( this->writeFile != NULL ){
		this->writeFile->AddTSBuff(data, size);
	}

	dropCount.AddData(data, size);

	return TRUE;
}

void COneServiceUtil::SetPmtPID(
	WORD TSID,
	WORD pmtPID
	)
{
	if( this->pmtPID != pmtPID && this->SID != 0xFFFF){
		_OutputDebugString(L"COneServiceUtil::SetPmtPID 0x%04x => 0x%04x", this->pmtPID, pmtPID);
		map<WORD, CCreatePATPacket::PROGRAM_PID_INFO> PIDMap;

		CCreatePATPacket::PROGRAM_PID_INFO item;
		item.PMTPID = pmtPID;
		item.SID = this->SID;
		PIDMap.insert(pair<WORD, CCreatePATPacket::PROGRAM_PID_INFO>(item.PMTPID,item));
		
		item.PMTPID = 0x0010;
		item.SID = 0x00;
		PIDMap.insert(pair<WORD, CCreatePATPacket::PROGRAM_PID_INFO>(item.PMTPID,item));
		
		createPat.SetParam(TSID, &PIDMap);

		this->pmtPID = pmtPID;
	}
}

void COneServiceUtil::SetEmmPID(
	map<WORD,WORD>* PIDMap
	)
{
	this->emmPIDMap = *PIDMap;
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
	wstring fileName,
	BOOL overWriteFlag,
	BOOL pittariFlag,
	WORD pittariONID,
	WORD pittariTSID,
	WORD pittariSID,
	WORD pittariEventID,
	ULONGLONG createSize,
	vector<REC_FILE_SET_INFO>* saveFolder,
	vector<wstring>* saveFolderSub,
	int maxBuffCount
)
{
	if( pittariFlag == FALSE ){
		if( this->writeFile == NULL ){
			OutputDebugString(L"*:StartSave");
			this->pittariRecFilePath = L"";
			this->pittariStart = FALSE;
			this->pittariEndChk = FALSE;

			this->writeFile = new CWriteTSFile;
			return this->writeFile->StartSave(fileName, overWriteFlag, createSize, saveFolder, saveFolderSub, maxBuffCount);
		}
	}else{
		if( this->writeFile == NULL ){
			OutputDebugString(L"*:StartSave pittariFlag");
			this->pittariRecFilePath = L"";
			this->fileName = fileName;
			this->overWriteFlag = overWriteFlag;
			this->createSize = createSize;
			this->saveFolder = *saveFolder;
			this->saveFolderSub = *saveFolderSub;
			this->maxBuffCount = maxBuffCount;
			this->pittariONID = pittariONID;
			this->pittariTSID = pittariTSID;
			this->pittariSID = pittariSID;
			this->pittariEventID = pittariEventID;

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
		this->writeFile = new CWriteTSFile;
		this->writeFile->StartSave(this->fileName, this->overWriteFlag, this->createSize, &this->saveFolder, &this->saveFolderSub, this->maxBuffCount);
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
	BOOL ret = this->writeFile->EndSave();
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
	SAFE_DELETE(this->writeFile);
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
	wstring filePath
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
// writeSize			[OUT]保存ファイル名
void COneServiceUtil::GetRecWriteSize(
	__int64* writeSize
	)
{
	if( this->writeFile != NULL ){
		this->writeFile->GetRecWriteSize(writeSize);
	}
}

void COneServiceUtil::SetBonDriver(
	wstring bonDriver
	)
{
	this->dropCount.SetBonDriver(bonDriver);
}

void COneServiceUtil::SetPIDName(
	map<WORD, string>* pidName
	)
{
	this->dropCount.SetPIDName(pidName);
}
