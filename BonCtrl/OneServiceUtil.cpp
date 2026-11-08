#include "stdafx.h"
#include "OneServiceUtil.h"


COneServiceUtil::COneServiceUtil(bool sendUdpTcp_)
	: serviceFilter(true)
{
	this->sendUdpTcp = sendUdpTcp_;
	this->SID = 0xFFFF;
	this->enableScramble = -1;
	this->serviceFilter.SetServiceID(true, vector<WORD>());
	this->pittariState = PITTARI_NONE;
}


COneServiceUtil::~COneServiceUtil(void)
{
	if( IsRec() ){
		EndSave();
	}
	SendUdp(nullptr);
	SendTcp(nullptr);
}

//処理対象ServiceIDを設定
//引数：
// SID			[IN]ServiceID。0xFFFFで全サービス対象。
void COneServiceUtil::SetSID(
	WORD SID_
)
{
	if( this->SID != SID_ ){
		AddDebugLogFormat(L"COneServiceUtil::SetSID 0x%04X => 0x%04X", this->SID, SID_);
		//ネットワーク送信モードのフィルタ動作は全サービス相当
		if( SID_ == 0xFFFF || this->sendUdpTcp ){
			this->serviceFilter.SetServiceID(true, vector<WORD>());
		}else{
			this->serviceFilter.SetServiceID(false, vector<WORD>(1, SID_));
		}
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

void COneServiceUtil::SendUdpTcp(
	vector<NW_SEND_INFO>* sendList,
	bool tcpFlag,
	CSendTSTCPDllUtil& sendNW,
	vector<util_unique_handle>& portMutexList,
	LPCWSTR mutexName
	)
{
	sendNW.StopSend();
	sendNW.UnInitialize();
	portMutexList.clear();

	if( sendList != nullptr ){
		sendNW.Initialize();
		for( vector<NW_SEND_INFO>::iterator itr = sendList->begin(); itr != sendList->end(); itr++ ){
			//IPアドレスであること
			if( std::find_if(itr->ipString.begin(), itr->ipString.end(), [](WCHAR c) {
			        return (c < L'0' || L'9' < c) && (c < L'A' || L'Z' < c) && (c < L'a' || L'z' < c) && c != L'%' && c != L'.' && c != L':'; }) != itr->ipString.end() ){
				//失敗
				itr->port = 0x10000;
				continue;
			}
			bool created = false;
			for( int i = 0; i < BON_NW_PORT_RANGE && itr->port < 0x10000; i++, itr->port++ ){
				wstring key;
				int n;
				if( ParseIPv4Address(itr->ipString.c_str(), n) ){
					Format(key, L"%ls%d_%d", mutexName, n, itr->port);
				}else{
					Format(key, L"%ls%ls_%d", mutexName, itr->ipString.c_str(), itr->port);
				}
				util_unique_handle portMutex = UtilCreateGlobalMutex(key.c_str());
				if( portMutex ){
					AddDebugLogFormat(L"Global\\%ls", key.c_str());
					portMutexList.push_back(std::move(portMutex));
					created = true;
					break;
				}
			}
			//ポート番号増分用のミューテックスを生成できたものだけ追加
			if( created ){
				if( tcpFlag ){
					sendNW.AddSendAddr(itr->ipString.c_str(), itr->port);
				}else{
					sendNW.AddSendAddrUdp(itr->ipString.c_str(), itr->port, itr->broadcastFlag != FALSE, itr->udpMaxSendSize);
				}
			}else{
				//失敗
				itr->port = 0x10000;
			}
		}
		sendNW.StartSend();
	}
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
	this->buff.clear();
	for( DWORD i = 0; i < size; i += 188 ){
		CTSPacketUtil packet;
		if( packet.Set188TS(data + i, 188) ){
			this->serviceFilter.FilterPacket(this->buff, data + i, packet);
			if( this->serviceFilter.CatOrPmtUpdated() ){
				//各PIDに名前をつける
				for( WORD pid : this->serviceFilter.CatUtil().GetPIDList() ){
					this->dropCount.SetPIDName(pid, L"EMM");
				}
				for( const auto& pmtUtil : this->serviceFilter.PmtUtilMap() ){
					WORD programNumber = pmtUtil.second.GetProgramNumber();
					if( programNumber != 0 ){
						this->dropCount.SetPIDName(pmtUtil.second.GetPcrPID(), L"PCR");
						wstring name;
						for( pair<WORD, BYTE> pidType : pmtUtil.second.GetPIDTypeList() ){
							name = pidType.second == 0x00 ? L"ECM" :
							       pidType.second == 0x02 ? L"MPEG2 VIDEO" :
							       pidType.second == 0x0F ? L"MPEG2 AAC" :
							       pidType.second == 0x1B ? L"MPEG4 VIDEO" :
							       pidType.second == 0x04 ? L"MPEG2 AUDIO" :
							       pidType.second == 0x24 ? L"HEVC VIDEO" :
							       pidType.second == 0x11 ? L"LATM AUDIO" :
							       pidType.second == 0x06 ? L"字幕" :
							       pidType.second == 0x0D ? L"データカルーセル" : L"";
							if( name.empty() ){
								Format(name, L"stream_type 0x%0X", pidType.second);
							}
							this->dropCount.SetPIDName(pidType.first, name);
						}
						Format(name, L"PMT(ServiceID 0x%04X)", programNumber);
						this->dropCount.SetPIDName(pmtUtil.first, name);
					}
				}
			}
		}
	}
	if( this->buff.empty() == false ){
		if( this->sendUdpTcp ){
			this->sendUdp.AddSendData(this->buff.data(), (DWORD)this->buff.size());
			this->sendTcp.AddSendData(this->buff.data(), (DWORD)this->buff.size());
		}else{
			this->writeFile.AddTSBuff(this->buff.data(), (DWORD)this->buff.size());
		}
		this->dropCount.AddData(this->buff.data(), (DWORD)this->buff.size());
	}

	if( this->pittariState == PITTARI_START ){
		WORD pmtVersion = 0xFFFF;
		for( const auto& pmtUtil : this->serviceFilter.PmtUtilMap() ){
			WORD programNumber = pmtUtil.second.GetProgramNumber();
			if( programNumber != 0 && programNumber == this->pittariRecParam.pittariSID ){
				pmtVersion = pmtUtil.second.GetVersion();
				break;
			}
		}
		if( this->lastPMTVer == 0xFFFF ){
			this->lastPMTVer = pmtVersion;
		}else if( pmtVersion != 0xFFFF && this->lastPMTVer != pmtVersion ){
			//ぴったり開始
			StratPittariRec();
			this->lastPMTVer = pmtVersion;
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

//出力状態をクリアする（チャンネル変更など）
//引数：
// tsid		[IN]TransportStreamID
void COneServiceUtil::Clear(
	WORD tsid
	)
{
	this->serviceFilter.Clear(tsid);
	this->dropCount.Clear();
}

//ファイル保存を開始する
//戻り値：
// true（成功）、false（失敗）
//引数：
// recParam				[IN]保存パラメータ（ctrlIDフィールドは無視）
// saveFolderSub		[IN]HDDの空きがなくなった場合に一時的に使用するフォルダ
// maxBuffCount			[IN]出力バッファ上限
bool COneServiceUtil::StartSave(
	const SET_CTRL_REC_PARAM& recParam,
	const vector<wstring>& saveFolderSub,
	int maxBuffCount
)
{
	if( this->writeFile.IsRec() == false && this->pittariState == PITTARI_NONE ){
		if( recParam.pittariFlag == 0 ){
			AddDebugLog(L"*:StartSave");
			return this->writeFile.StartSave(recParam.fileName, recParam.overWriteFlag != 0, recParam.createSize,
			                                 recParam.saveFolder, saveFolderSub, maxBuffCount);
		}else{
			AddDebugLog(L"*:StartSave pittariFlag");
			this->pittariRecParam = recParam;
			this->pittariSaveFolderSub = saveFolderSub;
			this->pittariMaxBuffCount = maxBuffCount;

			this->lastPMTVer = 0xFFFF;
			this->pittariState = PITTARI_START;

			return true;
		}
	}

	return false;
}

void COneServiceUtil::StratPittariRec()
{
	if( this->writeFile.IsRec() == false && this->pittariState == PITTARI_START ){
		AddDebugLog(L"*:StratPittariRec");
		if( this->writeFile.StartSave(this->pittariRecParam.fileName, this->pittariRecParam.overWriteFlag != 0, this->pittariRecParam.createSize,
		                              this->pittariRecParam.saveFolder, this->pittariSaveFolderSub, this->pittariMaxBuffCount) == false ){
			this->pittariState = PITTARI_END;
			this->pittariRecParam.fileName.clear();
			this->pittariSubRec = false;
		}
	}
}

void COneServiceUtil::StopPittariRec()
{
	AddDebugLog(L"*:StopPittariRec");
	this->pittariState = PITTARI_END;
	//ここでファイルパスを取得しておく
	this->pittariRecParam.fileName = this->writeFile.GetSaveFilePath();
	this->writeFile.EndSave(&this->pittariSubRec);
}

//ファイル保存を終了する
//戻り値：
// true（成功）、false（失敗）
//引数：
// subRecFlag	[OUT]成功のとき、サブ録画が発生したかどうか
bool COneServiceUtil::EndSave(bool* subRecFlag)
{
	bool ret = false;
	if( this->writeFile.IsRec() ){
		ret = this->writeFile.EndSave(subRecFlag);
	}else if( this->pittariState != PITTARI_NONE ){
		//ぴったりモードでは内部的な開始終了とは一致しない
		if( subRecFlag ){
			*subRecFlag = this->pittariState == PITTARI_END && this->pittariSubRec;
		}
		ret = true;
	}
	this->pittariState = PITTARI_NONE;
	AddDebugLog(L"*:EndSave");
	return ret;
}

//録画中かどうか
//戻り値：
// true（録画中）、false（していない）
bool COneServiceUtil::IsRec()
{
	return this->writeFile.IsRec() || this->pittariState != PITTARI_NONE;
}

//字幕とデータ放送含めるかどうか
//引数：
// enableCaption		[IN]字幕を true（含める）、false（含めない）
// enableData			[IN]データ放送を true（含める）、false（含めない）
void COneServiceUtil::SetServiceMode(
	bool enableCaption,
	bool enableData
	)
{
	this->serviceFilter.SetPmtCreateMode(!!enableCaption, !!enableData);
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

//録画中のファイルのファイルパスを取得する
//戻り値：
// ファイルパス
wstring COneServiceUtil::GetSaveFilePath()
{
	if( this->writeFile.IsRec() ){
		return this->writeFile.GetSaveFilePath();
	}else if( this->pittariState == PITTARI_END ){
		return this->pittariRecParam.fileName;
	}
	return wstring();
}

//ドロップとスクランブルのカウントを保存する
//引数：
// filePath			[IN]保存ファイル名
// asUtf8			[IN]UTF-8で保存するか
// dropSaveThresh	[IN]ドロップ数がこれ以上なら保存する
// drop				[OUT]ドロップ数
void COneServiceUtil::SaveErrCount(
	const wstring& filePath,
	bool asUtf8,
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
	LONGLONG* writeSize
	)
{
	if( this->writeFile.IsRec() || this->pittariState == PITTARI_END ){
		if( writeSize ){
			*writeSize = this->writeFile.GetRecWriteSize();
		}
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

void COneServiceUtil::SetNoLogScramble(
	bool noLog
	)
{
	this->dropCount.SetNoLog(false, noLog);
}
