#include "StdAfx.h"
#include "TSOut.h"

#include "../Common/TimeUtil.h"
#include "../Common/EpgTimerUtil.h"
#include "../Common/BlockLock.h"

CTSOut::CTSOut(void)
{
	InitializeCriticalSection(&this->objLock);

	this->chChangeState = CH_ST_INIT;
	this->chChangeTime = 0;
	this->lastONID = 0xFFFF;
	this->lastTSID = 0xFFFF;

	this->epgUtil.Initialize(FALSE);

	this->enableDecodeFlag = TRUE;
	this->emmEnableFlag = FALSE;
	this->serviceOnlyFlag = FALSE;

	this->nextCtrlID = 1;

	this->epgFile = NULL;
}


CTSOut::~CTSOut(void)
{
	StopSaveEPG(FALSE);
	DeleteCriticalSection(&this->objLock);
}

void CTSOut::SetChChangeEvent(BOOL resetEpgUtil)
{
	CBlockLock lock(&this->objLock);

	this->chChangeState = CH_ST_WAIT_PAT;
	this->chChangeTime = GetTickCount();

	this->decodeUtil.UnLoadDll();

	this->catUtil = CCATUtil();

	if( resetEpgUtil == TRUE ){
		this->epgUtil.UnInitialize();
		this->epgUtil.Initialize(FALSE);
	}
}

BOOL CTSOut::IsChUnknown(DWORD* elapsedTime)
{
	CBlockLock lock(&this->objLock);

	if( this->chChangeState != CH_ST_DONE ){
		if( elapsedTime != NULL ){
			*elapsedTime = this->chChangeState == CH_ST_INIT ? MAXDWORD : GetTickCount() - this->chChangeTime;
		}
		return TRUE;
	}
	return FALSE;
}

BOOL CTSOut::GetStreamID(WORD* ONID, WORD* TSID)
{
	CBlockLock lock(&this->objLock);

	if( this->chChangeState == CH_ST_DONE ){
		*ONID = this->lastONID;
		*TSID = this->lastTSID;
		return TRUE;
	}
	return FALSE;
}

void CTSOut::OnChChanged(WORD onid, WORD tsid)
{
	this->chChangeState = CH_ST_DONE;
	this->lastONID = onid;
	this->lastTSID = tsid;
	this->epgUtil.ClearSectionStatus();

	if( this->enableDecodeFlag != FALSE || this->emmEnableFlag != FALSE ){
		//スクランブル解除かEMM処理が設定されている場合だけ実行
		if( this->decodeUtil.SetNetwork(onid, tsid) == FALSE ){
			OutputDebugString(L"★★Decode DLL load err [CTSOut::OnChChanged()]\r\n");
			//再試行は意味がなさそうなので廃止
		}
		this->decodeUtil.SetEmm(this->emmEnableFlag);
	}
	ResetErrCount();

	this->pmtUtilMap.clear();
}

void CTSOut::AddTSBuff(BYTE* data, DWORD dataSize)
{
	//dataは同期済みかつそのサイズは188の整数倍であること

	CBlockLock lock(&this->objLock);
	if( dataSize == 0 || data == NULL ){
		return;
	}
	DWORD tick = GetTickCount();
	if( this->chChangeState == CH_ST_WAIT_PAT && tick - this->chChangeTime < 1000 ){
		//1秒間はチャンネル切り替え前のパケット来る可能性を考慮して無視する
		return;
	}
	this->decodeBuff.clear();

	BYTE* decodeData = NULL;
	DWORD decodeSize = 0;
	{
		for( DWORD i=0; i<dataSize; i+=188 ){
			CTSPacketUtil packet;
			if( packet.Set188TS(data + i, 188) == TRUE ){
				if( this->chChangeState != CH_ST_DONE ){
					//チャンネル切り替え中
					if( packet.transport_scrambling_control != 0 ){
						//スクランブルパケットなので解析できない
						continue;
					}
					this->epgUtil.AddTSPacket(data + i, 188);
					//GetTSID()が切り替え前の値を返さないようにPATを待つ
					if( this->chChangeState == CH_ST_WAIT_PAT ){
						if( packet.PID == 0 && packet.payload_unit_start_indicator ){
							this->chChangeState = CH_ST_WAIT_PAT2;
						}
					}else if( this->chChangeState == CH_ST_WAIT_PAT2 ){
						if( packet.PID == 0 && packet.payload_unit_start_indicator ){
							this->chChangeState = CH_ST_WAIT_ID;
						}
					}
					if( this->chChangeState == CH_ST_INIT || this->chChangeState == CH_ST_WAIT_ID ){
						WORD onid;
						WORD tsid;
						if( this->epgUtil.GetTSID(&onid, &tsid) == NO_ERR ){
							if( this->chChangeState == CH_ST_INIT ){
								_OutputDebugString(L"★Ch Init 0x%04X 0x%04X\r\n", onid, tsid);
								OnChChanged(onid, tsid);
							}else if( onid != this->lastONID || tsid != this->lastTSID ){
								_OutputDebugString(L"★Ch Change 0x%04X 0x%04X => 0x%04X 0x%04X\r\n", this->lastONID, this->lastTSID, onid, tsid);
								OnChChanged(onid, tsid);
							}else if( tick - this->chChangeTime > 7000 ){
								OutputDebugString(L"★Ch NoChange\r\n");
								OnChChanged(onid, tsid);
							}
						}
					}
				}else{
					//指定サービスに必要なPIDを解析
					if( packet.transport_scrambling_control == 0 ){
						//CAT
						if( packet.PID == 0x0001 ){
							if(this->catUtil.AddPacket(&packet) == TRUE){
								CheckNeedPID();
							}
						}
						//PMT
						if( packet.payload_unit_start_indicator == 1 && packet.data_byteSize > 0){
							BYTE pointer = packet.data_byte[0];
							if( pointer+1 < packet.data_byteSize ){
								if( packet.data_byte[1+pointer] == 0x02 ){
									//PMT
									map<WORD, CPMTUtil>::iterator itrPmt;
									itrPmt = this->pmtUtilMap.find(packet.PID);
									if( itrPmt == this->pmtUtilMap.end() ){
										itrPmt = this->pmtUtilMap.insert(std::make_pair(packet.PID, CPMTUtil())).first;
									}
									if( itrPmt->second.AddPacket(&packet) == TRUE ){
										CheckNeedPID();
									}
								}
							}
						}else{
							//PMTの2パケット目かチェック
							map<WORD, CPMTUtil>::iterator itrPmt;
							itrPmt = this->pmtUtilMap.find(packet.PID);
							if( itrPmt != this->pmtUtilMap.end() ){
								if( itrPmt->second.AddPacket(&packet) == TRUE ){
									CheckNeedPID();
								}
							}
						}
					}

					//デコード用のバッファ作成
					if( this->serviceOnlyFlag == FALSE ){
						//全サービス
						this->decodeBuff.insert(this->decodeBuff.end(), data + i, data + i + 188);
					}else{
						//指定サービス
						if( packet.PID <= 0x30 || this->needPIDMap.count(packet.PID) != 0 ){
							if( packet.PID == 0x0000 ){
								//PATなので必要なサービスのみに絞る
								BYTE* patBuff = NULL;
								DWORD patBuffSize = 0;
								if( patUtil.GetPacket(&patBuff, &patBuffSize) == TRUE ){
									if( packet.payload_unit_start_indicator == 1 ){
										this->decodeBuff.insert(this->decodeBuff.end(), patBuff, patBuff + patBuffSize);
									}
								}
							}else{
								this->decodeBuff.insert(this->decodeBuff.end(), data + i, data + i + 188);
							}
						}
					}
					if( this->epgFile != NULL ){
						DWORD write;
						if( packet.PID == 0 && packet.payload_unit_start_indicator ){
							if( this->epgFileState == EPG_FILE_ST_NONE ){
								this->epgFileState = EPG_FILE_ST_PAT;
							}else if( this->epgFileState == EPG_FILE_ST_PAT ){
								this->epgFileState = EPG_FILE_ST_TOT;
								//番組情報が不足しないよう改めて蓄積状態をリセット
								this->epgUtil.ClearSectionStatus();
								//TOTを前倒しで書き込むための場所を確保
								BYTE nullData[188] = { 0x47, 0x1F, 0xFF, 0x10 };
								memset(nullData + 4, 0xFF, 184);
								this->epgFileTotPos = SetFilePointer(this->epgFile, 0, NULL, FILE_CURRENT);
								WriteFile(this->epgFile, nullData, 188, &write, NULL);
							}
						}
						//まずPAT、次に(あれば)TOTを書き込む。この処理は必須ではないが番組情報をより確実かつ効率的に読み出せる
						if( packet.PID == 0x14 && this->epgFileState == EPG_FILE_ST_TOT ){
							this->epgFileState = EPG_FILE_ST_ALL;
							if( this->epgFileTotPos != INVALID_SET_FILE_POINTER ){
								SetFilePointer(this->epgFile, this->epgFileTotPos, NULL, FILE_BEGIN);
							}
							WriteFile(this->epgFile, data + i, 188, &write, NULL);
							LONG posHigh = 0;
							SetFilePointer(this->epgFile, 0, &posHigh, FILE_END);
						}else if( packet.PID == 0 && this->epgFileState >= EPG_FILE_ST_PAT || packet.PID <= 0x30 && this->epgFileState >= EPG_FILE_ST_TOT ){
							WriteFile(this->epgFile, data + i, 188, &write, NULL);
						}
					}
				}
			}
		}
		if( this->chChangeState == CH_ST_DONE ){
			WORD onid;
			WORD tsid;
			if( this->epgUtil.GetTSID(&onid, &tsid) == NO_ERR ){
				if( onid != this->lastONID || tsid != this->lastTSID ){
					_OutputDebugString(L"★Ch Unexpected Change 0x%04X 0x%04X => 0x%04X 0x%04X\r\n", this->lastONID, this->lastTSID, onid, tsid);
					OnChChanged(onid, tsid);
				}
			}
		}
	}
	try{
		if( this->decodeBuff.empty() == false ){
			if( this->enableDecodeFlag ){
				//デコード必要

				if( decodeUtil.Decode(&this->decodeBuff.front(), (DWORD)this->decodeBuff.size(), &decodeData, &decodeSize) == FALSE ){
					//デコード失敗
					decodeData = &this->decodeBuff.front();
					decodeSize = (DWORD)this->decodeBuff.size();
				}else{
					if( decodeData == NULL || decodeSize == 0 ){
						decodeData = NULL;
						decodeSize = 0;
					}
				}
			}else{
				//デコードの必要なし
				decodeData = &this->decodeBuff.front();
				decodeSize = (DWORD)this->decodeBuff.size();
			}
		}
	}catch(...){
		_OutputDebugString(L"★★CTSOut::AddTSBuff Exception2");
		//デコード失敗
		decodeData = &this->decodeBuff.front();
		decodeSize = (DWORD)this->decodeBuff.size();
	}
	
	//デコード済みのデータを解析させる
	{
		for( DWORD i=0; i<decodeSize; i+=188 ){
			this->epgUtil.AddTSPacket(decodeData + i, 188);
		}
	}

	//各サービス処理にデータ渡す
	{
		for( auto itrService = serviceUtilMap.begin(); itrService != serviceUtilMap.end(); itrService++ ){
			itrService->second->AddTSBuff(decodeData, decodeSize);
		}
	}
}

void CTSOut::CheckNeedPID()
{
	this->needPIDMap.clear();
	this->serviceOnlyFlag = TRUE;
	//PAT作成用のPMTリスト
	map<WORD, CCreatePATPacket::PROGRAM_PID_INFO> PIDMap;
	//NITのPID追加しておく
	PIDMap[0x10].PMTPID = 0x10;
	PIDMap[0x10].SID = 0;

	map<WORD, string> pidName;
	map<WORD, CPMTUtil>::iterator itrPmt;
	for( itrPmt = pmtUtilMap.begin(); itrPmt != pmtUtilMap.end(); itrPmt++ ){
		string name = "";
		Format(name, "PMT(ServiceID 0x%04X)", itrPmt->second.program_number);
		pidName.insert(pair<WORD, string>(itrPmt->first, name));
		map<WORD, WORD>::iterator itrPID;
		for( itrPID = itrPmt->second.PIDList.begin(); itrPID != itrPmt->second.PIDList.end(); itrPID++ ){
			switch(itrPID->second){
			case 0x00:
				name = "ECM";
				break;
			case 0x02:
				name = "MPEG2 VIDEO";
				break;
			case 0x0F:
				name = "MPEG2 AAC";
				break;
			case 0x1B:
				name = "MPEG4 VIDEO";
				break;
			case 0x04:
				name = "MPEG2 AUDIO";
				break;
			case 0x24:
				name = "HEVC VIDEO";
				break;
			case 0x06:
				name = "字幕";
				break;
			case 0x0D:
				name = "データカルーセル";
				break;
			default:
				Format(name, "stream_type 0x%0X", itrPID->second);
				break;
			}
			pidName.insert(pair<WORD, string>(itrPID->first, name));
		}
		pidName.insert(pair<WORD, string>(itrPmt->second.PCR_PID, "PCR"));
		
	}

	//EMMのPID
	{
		map<WORD,WORD>::iterator itrPID;
		for( itrPID = catUtil.PIDList.begin(); itrPID != catUtil.PIDList.end(); itrPID++ ){
			this->needPIDMap.insert(pair<WORD,WORD>(itrPID->first, itrPID->second));
			pidName.insert(pair<WORD, string>(itrPID->first, "EMM"));
		}
	}


	//各サービスのPMTを探す
	for( auto itrService = serviceUtilMap.begin(); itrService != serviceUtilMap.end(); itrService++ ){
		if( itrService->second->GetSID() == 0xFFFF ){
			//全サービス対象
			this->serviceOnlyFlag = FALSE;
			for( itrPmt = pmtUtilMap.begin(); itrPmt != pmtUtilMap.end(); itrPmt++ ){
				//PAT作成用のPMTリスト作成
				CCreatePATPacket::PROGRAM_PID_INFO item;
				item.PMTPID = itrPmt->first;
				item.SID = itrPmt->second.program_number;
				PIDMap.insert(pair<WORD, CCreatePATPacket::PROGRAM_PID_INFO>(item.PMTPID,item));

				//PMT記載のPIDを登録
				this->needPIDMap.insert(pair<WORD,WORD>(itrPmt->first, 0));
				map<WORD,WORD>::iterator itrPID;
				for( itrPID = itrPmt->second.PIDList.begin(); itrPID != itrPmt->second.PIDList.end(); itrPID++ ){
					this->needPIDMap.insert(pair<WORD,WORD>(itrPID->first, itrPID->second));
				}
			}
		}else{
			for( itrPmt = pmtUtilMap.begin(); itrPmt != pmtUtilMap.end(); itrPmt++ ){
				if( itrService->second->GetSID() == itrPmt->second.program_number ){
					//PMT発見
					itrService->second->SetPmtPID(this->lastTSID, itrPmt->first);
					itrService->second->SetEmmPID(catUtil.PIDList);


					//PAT作成用のPMTリスト作成
					CCreatePATPacket::PROGRAM_PID_INFO item2;
					item2.PMTPID = itrPmt->first;
					item2.SID = itrPmt->second.program_number;
					PIDMap.insert(pair<WORD, CCreatePATPacket::PROGRAM_PID_INFO>(item2.PMTPID,item2));
					//_OutputDebugString(L"0x%04x, 0x%04x", itrPmt->first,itrPmt->second->program_number);
					//PMT記載のPIDを登録
					this->needPIDMap.insert(pair<WORD,WORD>(itrPmt->first, 0));
					this->needPIDMap.insert(pair<WORD,WORD>(itrPmt->second.PCR_PID, 0));
					map<WORD,WORD>::iterator itrPID;
					for( itrPID = itrPmt->second.PIDList.begin(); itrPID != itrPmt->second.PIDList.end(); itrPID++ ){
						this->needPIDMap.insert(pair<WORD,WORD>(itrPID->first, itrPID->second));
					}
				}
			}
		}
		itrService->second->SetPIDName(pidName);
	}
	this->patUtil.SetParam(this->lastTSID, &PIDMap);
}

//EPGデータの保存を開始する
//戻り値：
// TRUE（成功）、FALSE（失敗）
BOOL CTSOut::StartSaveEPG(
	const wstring& epgFilePath
	)
{
	CBlockLock lock(&this->objLock);
	if( this->epgFile != NULL ){
		return FALSE;
	}
	this->epgFilePath = epgFilePath;
	this->epgTempFilePath = epgFilePath;
	this->epgTempFilePath += L".tmp";

	_OutputDebugString(L"★%s\r\n", this->epgFilePath.c_str());
	_OutputDebugString(L"★%s\r\n", this->epgTempFilePath.c_str());

	this->epgUtil.ClearSectionStatus();
	this->epgFileState = EPG_FILE_ST_NONE;

	this->epgFile = _CreateDirectoryAndFile(this->epgTempFilePath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if( this->epgFile == INVALID_HANDLE_VALUE ){
		this->epgFile = NULL;
		OutputDebugString(L"err\r\n");
		return FALSE;
	}

	return TRUE;
}

//EPGデータの保存を終了する
void CTSOut::StopSaveEPG(
	BOOL copy
	)
{
	CBlockLock lock(&this->objLock);
	if( this->epgFile == NULL ){
		return;
	}

	CloseHandle(this->epgFile);
	this->epgFile = NULL;

	if( copy == TRUE ){
		CopyFile(this->epgTempFilePath.c_str(), this->epgFilePath.c_str(), FALSE );
	}
	DeleteFile(this->epgTempFilePath.c_str());
}

//EPGデータの蓄積状態を取得する
//戻り値：
// ステータス
//引数：
// l_eitFlag		[IN]L-EITのステータスを取得
EPG_SECTION_STATUS CTSOut::GetSectionStatus(
	BOOL l_eitFlag
	)
{
	CBlockLock lock(&this->objLock);

	return this->epgUtil.GetSectionStatus(l_eitFlag);
}

//指定サービスのEPGデータの蓄積状態を取得する
pair<EPG_SECTION_STATUS, BOOL> CTSOut::GetSectionStatusService(
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	BOOL l_eitFlag
	)
{
	CBlockLock lock(&this->objLock);

	return this->epgUtil.GetSectionStatusService(originalNetworkID, transportStreamID, serviceID, l_eitFlag);
}

//EMM処理の動作設定
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// enable		[IN] TRUE（処理する）、FALSE（処理しない）
BOOL CTSOut::SetEmm(
	BOOL enable
	)
{
	CBlockLock lock(&this->objLock);

	try{
		if( this->chChangeState == CH_ST_DONE ){
			//チューニング済みで
			if( enable != FALSE && this->enableDecodeFlag == FALSE && this->emmEnableFlag == FALSE ){
				//最初に EMM 処理が設定される場合は DLL を読み込む
				//スクランブル解除が設定されている場合は読み込み済みなので除外
				if( this->decodeUtil.SetNetwork(this->lastONID, this->lastTSID) == FALSE ){
					OutputDebugString(L"★★Decode DLL load err [CTSOut::SetEmm()]\r\n");
				}
			}
		}
	}catch(...){
		return FALSE;
	}

	this->emmEnableFlag = enable;
	return this->decodeUtil.SetEmm(enable);
}

//EMM処理を行った数
//戻り値：
// 処理数
DWORD CTSOut::GetEmmCount()
{
	CBlockLock lock(&this->objLock);

	return this->decodeUtil.GetEmmCount();
}

//DLLのロード状態を取得
//戻り値：
// TRUE（ロードに成功している）、FALSE（ロードに失敗している）
//引数：
// loadErrDll		[OUT]ロードに失敗したDLLファイル名
BOOL CTSOut::GetLoadStatus(
	wstring& loadErrDll
	)
{
	CBlockLock lock(&this->objLock);

	return this->decodeUtil.GetLoadStatus(loadErrDll);
}

//自ストリームのサービス一覧を取得する
//戻り値：
// エラーコード
//引数：
// serviceListSize			[OUT]serviceListの個数
// serviceList				[OUT]サービス情報のリスト（DLL内で自動的にdeleteする。次に取得を行うまで有効）
DWORD CTSOut::GetServiceListActual(
	DWORD* serviceListSize,
	SERVICE_INFO** serviceList
	)
{
	CBlockLock lock(&this->objLock);

	return this->epgUtil.GetServiceListActual(serviceListSize, serviceList);
}

//次に使用する制御IDを取得する
//戻り値：
// id
DWORD CTSOut::GetNextID()
{
	DWORD nextID = 0xFFFFFFFF;

	auto itr = this->serviceUtilMap.find(this->nextCtrlID);
	if( itr == this->serviceUtilMap.end() ){
		//存在しないIDなのでそのまま使用
		nextID = this->nextCtrlID;
		this->nextCtrlID++;
		if( this->nextCtrlID == 0 || this->nextCtrlID == 0xFFFFFFFF){
			this->nextCtrlID = 1;
		}
	}else{
		//一周した？
		for( DWORD i=1; i<0xFFFFFFFF; i++ ){
			//１から順番に存在しないIDを確認
			itr = this->serviceUtilMap.find(this->nextCtrlID);
			if( itr == this->serviceUtilMap.end() ){
				nextID = this->nextCtrlID;
				this->nextCtrlID++;
				if( this->nextCtrlID == 0 || this->nextCtrlID == 0xFFFFFFFF){
					this->nextCtrlID = 1;
				}
				break;
			}else{
				this->nextCtrlID++;
			}
			if( this->nextCtrlID == 0 || this->nextCtrlID == 0xFFFFFFFF){
				this->nextCtrlID = 1;
			}
		}
	}

	return nextID;
}

//TSストリーム制御用コントロールを作成する
//戻り値：
// エラーコード
//引数：
// id			[OUT]制御識別ID
BOOL CTSOut::CreateServiceCtrl(
	DWORD* id
	)
{
	CBlockLock lock(&this->objLock);

	*id = GetNextID();
	auto itr = this->serviceUtilMap.insert(std::make_pair(*id, std::unique_ptr<COneServiceUtil>(new COneServiceUtil))).first;
	itr->second->SetEpgUtil(&this->epgUtil);
	itr->second->SetBonDriver(bonFile);

	return TRUE;
}

//TSストリーム制御用コントロールを削除する
//戻り値：
// エラーコード
//引数：
// id			[IN]制御識別ID
BOOL CTSOut::DeleteServiceCtrl(
	DWORD id
	)
{
	CBlockLock lock(&this->objLock);

	if( serviceUtilMap.erase(id) == 0 ){
		return FALSE;
	}

	CheckNeedPID();

	return TRUE;
}

//制御対象のサービスを設定する
//戻り値：
// TRUE（成功）、FALSE（失敗
//引数：
// id			[IN]制御識別ID
// serviceID	[IN]対象サービスID
BOOL CTSOut::SetServiceID(
	DWORD id,
	WORD serviceID
	)
{
	CBlockLock lock(&this->objLock);

	auto itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		return FALSE;
	}

	itr->second->SetSID(serviceID);
	CheckNeedPID();

	return TRUE;
}

BOOL CTSOut::GetServiceID(
	DWORD id,
	WORD* serviceID
	)
{
	CBlockLock lock(&this->objLock);

	auto itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		return FALSE;
	}
	if( serviceID != NULL ){
		*serviceID = itr->second->GetSID();
	}

	return TRUE;
}

//UDPで送信を行う
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// id			[IN]制御識別ID
// sendList		[IN/OUT]送信先リスト。NULLで停止。Portは実際に送信に使用したPortが返る。
BOOL CTSOut::SendUdp(
	DWORD id,
	vector<NW_SEND_INFO>* sendList
	)
{
	CBlockLock lock(&this->objLock);

	auto itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		return FALSE;
	}

	itr->second->SendUdp(sendList);

	return TRUE;
}

//TCPで送信を行う
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// id			[IN]制御識別ID
// sendList		[IN/OUT]送信先リスト。NULLで停止。Portは実際に送信に使用したPortが返る。
BOOL CTSOut::SendTcp(
	DWORD id,
	vector<NW_SEND_INFO>* sendList
	)
{
	CBlockLock lock(&this->objLock);

	auto itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		return FALSE;
	}

	itr->second->SendTcp(sendList);

	return TRUE;
}

//指定サービスの現在or次のEPG情報を取得する
//戻り値：
// エラーコード
//引数：
// originalNetworkID		[IN]取得対象のoriginalNetworkID
// transportStreamID		[IN]取得対象のtransportStreamID
// serviceID				[IN]取得対象のServiceID
// nextFlag					[IN]TRUE（次の番組）、FALSE（現在の番組）
// epgInfo					[OUT]EPG情報
DWORD CTSOut::GetEpgInfo(
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	BOOL nextFlag,
	EPGDB_EVENT_INFO* epgInfo
	)
{
	CBlockLock lock(&this->objLock);

	EPG_EVENT_INFO* _epgInfo;
	DWORD err = this->epgUtil.GetEpgInfo(originalNetworkID, transportStreamID, serviceID, nextFlag, &_epgInfo);
	if( err == NO_ERR ){
		ConvertEpgInfo(originalNetworkID, transportStreamID, serviceID, _epgInfo, epgInfo);
	}

	return err;
}

//指定イベントのEPG情報を取得する
//戻り値：
// エラーコード
//引数：
// originalNetworkID		[IN]取得対象のoriginalNetworkID
// transportStreamID		[IN]取得対象のtransportStreamID
// serviceID				[IN]取得対象のServiceID
// eventID					[IN]取得対象のEventID
// pfOnlyFlag				[IN]p/fからのみ検索するかどうか
// epgInfo					[OUT]EPG情報
DWORD CTSOut::SearchEpgInfo(
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	WORD eventID,
	BYTE pfOnlyFlag,
	EPGDB_EVENT_INFO* epgInfo
	)
{
	CBlockLock lock(&this->objLock);

	EPG_EVENT_INFO* _epgInfo;
	DWORD err = this->epgUtil.SearchEpgInfo(originalNetworkID, transportStreamID, serviceID, eventID, pfOnlyFlag, &_epgInfo);
	if( err == NO_ERR ){
		ConvertEpgInfo(originalNetworkID, transportStreamID, serviceID, _epgInfo, epgInfo);
	}

	return err;
}

//PC時計を元としたストリーム時間との差を取得する
//戻り値：
// 差の秒数
int CTSOut::GetTimeDelay(
	)
{
	CBlockLock lock(&this->objLock);

	return this->epgUtil.GetTimeDelay();
}

//録画中かどうか
//戻り値：
// TRUE（録画中）、FALSE（していない）
BOOL CTSOut::IsRec()
{
	CBlockLock lock(&this->objLock);

	for( auto itr = this->serviceUtilMap.begin(); itr != this->serviceUtilMap.end(); itr++ ){
		if( itr->second->IsRec() == TRUE ){
			return TRUE;
		}
	}

	return FALSE;
}

//ファイル保存を開始する
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// id					[IN]制御識別ID
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
BOOL CTSOut::StartSave(
	DWORD id,
	const wstring& fileName,
	BOOL overWriteFlag,
	BOOL pittariFlag,
	WORD pittariONID,
	WORD pittariTSID,
	WORD pittariSID,
	WORD pittariEventID,
	ULONGLONG createSize,
	const vector<REC_FILE_SET_INFO>* saveFolder,
	const vector<wstring>* saveFolderSub,
	int maxBuffCount
)
{
	CBlockLock lock(&this->objLock);

	auto itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		return FALSE;
	}

	return itr->second->StartSave(fileName, overWriteFlag, pittariFlag, pittariONID, pittariTSID, pittariSID, pittariEventID, createSize, saveFolder, saveFolderSub, maxBuffCount);
}

//ファイル保存を終了する
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// id			[IN]制御識別ID
BOOL CTSOut::EndSave(
	DWORD id
	)
{
	CBlockLock lock(&this->objLock);
	auto itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		return FALSE;
	}

	return itr->second->EndSave();
}

//スクランブル解除処理の動作設定
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// enable		[IN] TRUE（処理する）、FALSE（処理しない）
BOOL CTSOut::SetScramble(
	DWORD id,
	BOOL enable
	)
{
	CBlockLock lock(&this->objLock);

	try{
		if( this->chChangeState == CH_ST_DONE ){
			//チューニング済みで
			if( enable != FALSE && this->enableDecodeFlag == FALSE && this->emmEnableFlag == FALSE ){
				//最初にスクランブル解除が設定される場合は DLL を再読み込みする
				//EMM 処理が設定されている場合は読み込み済みなので除外
				if( this->decodeUtil.SetNetwork(this->lastONID, this->lastTSID) == FALSE ){
					OutputDebugString(L"★★Decode DLL load err [CTSOut::SetScramble()]\r\n");
				}
			}
		}
	}catch(...){
		return FALSE;
	}

	auto itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		return FALSE;
	}

	itr->second->SetScramble(enable);

	BOOL enableScramble = FALSE;
	for( itr = this->serviceUtilMap.begin(); itr != this->serviceUtilMap.end(); itr++ ){
		if( itr->second->GetScramble() == TRUE ){
			enableScramble = TRUE;
			break;
		}
	}

	this->enableDecodeFlag = enableScramble;

	return TRUE;
}

//字幕とデータ放送含めるかどうか
//引数：
// id					[IN]制御識別ID
// enableCaption		[IN]字幕を TRUE（含める）、FALSE（含めない）
// enableData			[IN]データ放送を TRUE（含める）、FALSE（含めない）
void CTSOut::SetServiceMode(
	DWORD id,
	BOOL enableCaption,
	BOOL enableData
	)
{
	CBlockLock lock(&this->objLock);

	auto itr = serviceUtilMap.find(id);
	if( itr != serviceUtilMap.end() ){
		itr->second->SetServiceMode(enableCaption, enableData);
	}
}

//エラーカウントをクリアする
//引数：
// id				[IN]制御識別ID
void CTSOut::ClearErrCount(
	DWORD id
	)
{
	CBlockLock lock(&this->objLock);

	auto itr = serviceUtilMap.find(id);
	if( itr != serviceUtilMap.end() ){
		itr->second->ClearErrCount();
	}
}

//ドロップとスクランブルのカウントを取得する
//引数：
// id				[IN]制御識別ID
// drop				[OUT]ドロップ数
// scramble			[OUT]スクランブル数
void CTSOut::GetErrCount(
	DWORD id,
	ULONGLONG* drop,
	ULONGLONG* scramble
	)
{
	CBlockLock lock(&this->objLock);

	auto itr = serviceUtilMap.find(id);
	if( itr != serviceUtilMap.end() ){
		itr->second->GetErrCount(drop, scramble);
	}
}


//録画中のファイルの出力サイズを取得する
//引数：
// id					[IN]制御識別ID
// writeSize			[OUT]出力サイズ
void CTSOut::GetRecWriteSize(
	DWORD id,
	__int64* writeSize
	)
{
	CBlockLock lock(&this->objLock);

	auto itr = serviceUtilMap.find(id);
	if( itr != serviceUtilMap.end() ){
		itr->second->GetRecWriteSize(writeSize);
	}
}

void CTSOut::ResetErrCount()
{
	for( auto itr = serviceUtilMap.begin(); itr != serviceUtilMap.end(); itr++ ){
		itr->second->ClearErrCount();
	}
}

//録画中のファイルのファイルパスを取得する
//引数：
// id					[IN]制御識別ID
// filePath				[OUT]保存ファイル名
// subRecFlag			[OUT]サブ録画が発生したかどうか
void CTSOut::GetSaveFilePath(
	DWORD id,
	wstring* filePath,
	BOOL* subRecFlag
	)
{
	CBlockLock lock(&this->objLock);

	auto itr = serviceUtilMap.find(id);
	if( itr != serviceUtilMap.end() ){
		itr->second->GetSaveFilePath(filePath, subRecFlag);
	}
}

//ドロップとスクランブルのカウントを保存する
//引数：
// id					[IN]制御識別ID
// filePath				[IN]保存ファイル名
void CTSOut::SaveErrCount(
	DWORD id,
	const wstring& filePath
	)
{
	CBlockLock lock(&this->objLock);

	auto itr = serviceUtilMap.find(id);
	if( itr != serviceUtilMap.end() ){
		itr->second->SaveErrCount(filePath);
	}
}

void CTSOut::SetSignalLevel(
	float signalLv
	)
{
	CBlockLock lock(&this->objLock);

	for( auto itr = serviceUtilMap.begin(); itr != serviceUtilMap.end(); itr++ ){
		itr->second->SetSignalLevel(signalLv);
	}
}


void CTSOut::SetBonDriver(
	const wstring& bonDriver
	)
{
	CBlockLock lock(&this->objLock);

	for( auto itr = serviceUtilMap.begin(); itr != serviceUtilMap.end(); itr++ ){
		itr->second->SetBonDriver(bonDriver);
	}
	bonFile = bonDriver;
}

