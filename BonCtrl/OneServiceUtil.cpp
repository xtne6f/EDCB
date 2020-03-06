#include "stdafx.h"
#include "OneServiceUtil.h"


COneServiceUtil::COneServiceUtil(BOOL sendUdpTcp_)
{
	this->sendUdpTcp = sendUdpTcp_;
	this->SID = 0xFFFF;

	this->pmtPID = 0xFFFF;

	this->enableScramble = -1;

	this->pittariState = PITTARI_NONE;
}


COneServiceUtil::~COneServiceUtil(void)
{
	if( IsRec() ){
		EndSave();
	}
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

BOOL COneServiceUtil::SendUdpTcp(
	vector<NW_SEND_INFO>* sendList,
	CSendNW& sendNW,
	vector<HANDLE>& portMutexList,
	LPCWSTR mutexName
	)
{
	sendNW.StopSend();
	sendNW.UnInitialize();
	while( portMutexList.empty() == false ){
		CloseHandle(portMutexList.back());
		portMutexList.pop_back();
	}

	if( sendList != NULL ){
		sendNW.Initialize();
		for( size_t i=0; i<sendList->size(); i++ ){
			wstring key = L"";
			HANDLE portMutex;

			//生成できなくても深刻ではないのでほどほどに打ち切る
			for( int j = 0; j < BON_NW_PORT_RANGE; j++ ){
				UINT u[4];
				if( swscanf_s((*sendList)[i].ipString.c_str(), L"%u.%u.%u.%u", &u[0], &u[1], &u[2], &u[3]) == 4 ){
					Format(key, L"Global\\%ls%d_%d", mutexName, u[0] << 24 | u[1] << 16 | u[2] << 8 | u[3], (*sendList)[i].port);
				}else{
					Format(key, L"Global\\%ls%ls_%d", mutexName, (*sendList)[i].ipString.c_str(), (*sendList)[i].port);
				}
				portMutex = CreateMutex(NULL, FALSE, key.c_str());
		
				if( portMutex == NULL ){
					(*sendList)[i].port++;
				}else if( GetLastError() == ERROR_ALREADY_EXISTS ){
					CloseHandle(portMutex);
					(*sendList)[i].port++;
				}else{
					_OutputDebugString(L"%ls\r\n", key.c_str());
					portMutexList.push_back(portMutex);
					break;
				}
			}
			sendNW.AddSendAddr((*sendList)[i].ipString.c_str(), (*sendList)[i].port, (*sendList)[i].broadcastFlag != FALSE);
		}
		sendNW.StartSend();
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
			this->sendUdp.AddSendData(data, size);
			this->sendTcp.AddSendData(data, size);
		}
		this->dropCount.AddData(data, size);
	}else if( this->SID == 0xFFFF ){
		//全サービス扱い
		this->writeFile.AddTSBuff(data, size);
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
					if( packet.PID < BON_SELECTIVE_PID || createPmt.IsNeedPID(packet.PID) ||
					    std::binary_search(this->emmPIDList.begin(), this->emmPIDList.end(), packet.PID) ){
						//PMTで定義されてるかEMMなら必要
						this->buff.insert(this->buff.end(), data + i, data + i + 188);
					}
				}
			}
		}

		if( this->buff.empty() == false ){
			this->writeFile.AddTSBuff(this->buff.data(), (DWORD)this->buff.size());
			this->dropCount.AddData(this->buff.data(), (DWORD)this->buff.size());
		}
	}

	if( this->pittariState == PITTARI_START ){
		if( this->lastPMTVer == 0xFFFF ){
			this->lastPMTVer = createPmt.GetVersion();
		}else if(this->lastPMTVer != createPmt.GetVersion()){
			//ぴったり開始
			StratPittariRec();
			this->lastPMTVer = createPmt.GetVersion();
		}
		if( funcGetPresent ){
			int eventID = funcGetPresent(this->pittariRecParam.pittariONID, this->pittariRecParam.pittariTSID, this->pittariRecParam.pittariSID);
			if( eventID >= 0 ){
				if( eventID == this->pittariRecParam.pittariEventID ){
					//ぴったり開始
					StratPittariRec();
					if( this->pittariState == PITTARI_START ){
						this->pittariState = PITTARI_END_CHK;
					}
				}
			}
		}
	}
	if( this->pittariState == PITTARI_END_CHK ){
		if( funcGetPresent ){
			int eventID = funcGetPresent(this->pittariRecParam.pittariONID, this->pittariRecParam.pittariTSID, this->pittariRecParam.pittariSID);
			if( eventID >= 0 ){
				if( eventID != this->pittariRecParam.pittariEventID ){
					//ぴったり終了
					StopPittariRec();
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
		pidList.push_back(std::make_pair((WORD)0x10, (WORD)0));
		pidList.push_back(std::make_pair(pmtPID_, this->SID));
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

BOOL COneServiceUtil::StartSave(
	const SET_CTRL_REC_PARAM& recParam,
	const vector<wstring>& saveFolderSub,
	int maxBuffCount
)
{
	if( this->writeFile.IsRec() == FALSE && this->pittariState == PITTARI_NONE ){
		if( recParam.pittariFlag == FALSE ){
			OutputDebugString(L"*:StartSave");
			return this->writeFile.StartSave(recParam.fileName, recParam.overWriteFlag, recParam.createSize,
			                                 recParam.saveFolder, saveFolderSub, maxBuffCount);
		}else{
			OutputDebugString(L"*:StartSave pittariFlag");
			this->pittariRecParam = recParam;
			this->pittariSaveFolderSub = saveFolderSub;
			this->pittariMaxBuffCount = maxBuffCount;

			this->lastPMTVer = 0xFFFF;
			this->pittariState = PITTARI_START;

			return TRUE;
		}
	}

	return FALSE;
}

void COneServiceUtil::StratPittariRec()
{
	if( this->writeFile.IsRec() == FALSE && this->pittariState == PITTARI_START ){
		OutputDebugString(L"*:StratPittariRec");
		if( this->writeFile.StartSave(this->pittariRecParam.fileName, this->pittariRecParam.overWriteFlag, this->pittariRecParam.createSize,
		                              this->pittariRecParam.saveFolder, this->pittariSaveFolderSub, this->pittariMaxBuffCount) == FALSE ){
			this->pittariState = PITTARI_END;
			this->pittariRecParam.fileName.clear();
			this->pittariSubRec = FALSE;
		}
	}
}

void COneServiceUtil::StopPittariRec()
{
	OutputDebugString(L"*:StopPittariRec");
	this->pittariState = PITTARI_END;
	//ここでファイルパスを取得しておく
	this->pittariRecParam.fileName = this->writeFile.GetSaveFilePath();
	this->writeFile.EndSave(&this->pittariSubRec);
}

BOOL COneServiceUtil::EndSave(BOOL* subRecFlag)
{
	BOOL ret = FALSE;
	if( this->writeFile.IsRec() ){
		ret = this->writeFile.EndSave(subRecFlag);
	}else if( this->pittariState != PITTARI_NONE ){
		//ぴったりモードでは内部的な開始終了とは一致しない
		if( subRecFlag ){
			*subRecFlag = this->pittariState == PITTARI_END && this->pittariSubRec;
		}
		ret = TRUE;
	}
	this->pittariState = PITTARI_NONE;
	OutputDebugString(L"*:EndSave");
	return ret;
}

//録画中かどうか
//戻り値：
// TRUE（録画中）、FALSE（していない）
BOOL COneServiceUtil::IsRec()
{
	return this->writeFile.IsRec() || this->pittariState != PITTARI_NONE;
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
	if( drop ){
		*drop = this->dropCount.GetDropCount();
	}
	if( scramble ){
		*scramble = this->dropCount.GetScrambleCount();
	}
}

wstring COneServiceUtil::GetSaveFilePath()
{
	if( this->writeFile.IsRec() ){
		return this->writeFile.GetSaveFilePath();
	}else if( this->pittariState == PITTARI_END ){
		return this->pittariRecParam.fileName;
	}
	return wstring();
}

void COneServiceUtil::SaveErrCount(
	const wstring& filePath,
	BOOL asUtf8,
	int dropSaveThresh,
	int scrambleSaveThresh,
	ULONGLONG& drop,
	ULONGLONG& scramble
	)
{
	GetErrCount(&drop, &scramble);
	if( (dropSaveThresh >= 0 && drop >= (ULONGLONG)dropSaveThresh) ||
	    (scrambleSaveThresh >= 0 && scramble >= (ULONGLONG)scrambleSaveThresh) ){
		this->dropCount.SaveLog(filePath, asUtf8);
	}
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
	if( this->writeFile.IsRec() || this->pittariState == PITTARI_END ){
		this->writeFile.GetRecWriteSize(writeSize);
	}else if( this->pittariState != PITTARI_NONE ){
		if( writeSize ){
			*writeSize = 0;
		}
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
	const wstring& name
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
