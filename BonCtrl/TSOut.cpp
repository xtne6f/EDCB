#include "stdafx.h"
#include "TSOut.h"

#include "../Common/TimeUtil.h"
#include "../Common/EpgTimerUtil.h"

CTSOut::CTSOut(void)
{
	this->chChangeState = CH_ST_INIT;
	this->chChangeTime = 0;
	this->chChangePresumedONID = 0xFFFF;
	this->lastONID = 0xFFFF;
	this->lastTSID = 0xFFFF;

	this->epgUtil.Initialize(FALSE);

	this->enableDecodeFlag = FALSE;
	this->emmEnableFlag = FALSE;

	this->nextCtrlID = 1;
	this->logoAdditionalNeededPids = NULL;
	this->noLogScramble = FALSE;
	this->parseEpgPostProcess = FALSE;
}


CTSOut::~CTSOut(void)
{
	StopSaveEPG(FALSE);
}

void CTSOut::SetChChangeEvent(WORD presumedONID, BOOL resetEpgUtil)
{
	lock_recursive_mutex lock(this->objLock);

	this->chChangeState = CH_ST_WAIT_PAT;
	this->chChangeTime = GetU32Tick();
	this->chChangePresumedONID = presumedONID;

	this->decodeUtil.UnLoadDll();

	if( resetEpgUtil == TRUE ){
		lock_recursive_mutex lock2(this->epgUtilLock);
		//EpgDataCap3は内部メソッド単位でアトミック。初期化以外はobjLockかepgUtilLockのどちらかを獲得すればよい
		this->epgUtil.UnInitialize();
		this->epgUtil.Initialize(FALSE);
		this->logoAdditionalNeededPids = NULL;
	}
}

BOOL CTSOut::IsChUnknown(DWORD* elapsedTime)
{
	lock_recursive_mutex lock(this->objLock);

	if( this->chChangeState != CH_ST_DONE ){
		if( elapsedTime != NULL ){
			*elapsedTime = this->chChangeState == CH_ST_INIT ? MAXDWORD : GetU32Tick() - this->chChangeTime;
		}
		return TRUE;
	}
	return FALSE;
}

BOOL CTSOut::GetStreamID(WORD* ONID, WORD* TSID)
{
	lock_recursive_mutex lock(this->objLock);

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
			AddDebugLog(L"★★Decode DLL load err [CTSOut::OnChChanged()]");
			//再試行は意味がなさそうなので廃止
		}
		this->decodeUtil.SetEmm(this->emmEnableFlag);
	}
	for( auto itr = this->serviceUtilMap.begin(); itr != this->serviceUtilMap.end(); itr++ ){
		itr->second->Clear(tsid);
	}
	this->serviceFilter.Clear(tsid);
}

void CTSOut::AddTSBuff(BYTE* data, DWORD dataSize)
{
	//dataは同期済みかつそのサイズは188の整数倍であること

	lock_recursive_mutex lock(this->objLock);
	if( dataSize == 0 || data == NULL ){
		return;
	}
	DWORD tick = GetU32Tick();
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
			if( packet.Set188TS(data + i, 188) ){
				if( this->chChangeState != CH_ST_DONE ){
					//チャンネル切り替え中
					if( packet.transport_scrambling_control != 0 ){
						//スクランブルパケットなので解析できない
						continue;
					}
					if( packet.PID >= BON_SELECTIVE_PID ){
						//間接指定のパケットは不要
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
						DWORD ret = this->epgUtil.GetTSID(&onid, &tsid);
						if( ret != NO_ERR && this->chChangePresumedONID != 0xFFFF && this->lastTSID != 0xFFFF ){
							//ONIDの取得は遅いことがあるので、チャンネル変更などでONIDが予測できる場合はそれを使う
							//切り替え前との区別を確実にするためTSIDが変化する場合に限る
							onid = this->chChangePresumedONID;
							if( this->epgUtil.GetTSID(NULL, &tsid) == NO_ERR && tsid != this->lastTSID ){
								ret = NO_ERR;
							}
						}
						if( ret == NO_ERR ){
							if( this->chChangeState == CH_ST_INIT ){
								AddDebugLogFormat(L"★Ch Init 0x%04X 0x%04X", onid, tsid);
								OnChChanged(onid, tsid);
							}else if( onid != this->lastONID || tsid != this->lastTSID ){
								AddDebugLogFormat(L"★Ch Change 0x%04X 0x%04X => 0x%04X 0x%04X", this->lastONID, this->lastTSID, onid, tsid);
								OnChChanged(onid, tsid);
							}else if( tick - this->chChangeTime > 7000 ){
								AddDebugLog(L"★Ch NoChange");
								OnChChanged(onid, tsid);
							}
						}
					}
				}else{
					this->serviceFilter.FilterPacket(this->decodeBuff, data + i, packet);
					if( this->parseEpgPostProcess == FALSE ){
						if( packet.PID < BON_SELECTIVE_PID ){
							ParseEpgPacket(data + i, packet);
						}else{
							lock_recursive_mutex lock2(this->epgUtilLock);
							if( this->logoAdditionalNeededPids ){
								for( const WORD* pid = this->logoAdditionalNeededPids; *pid; pid++ ){
									if( *pid == packet.PID ){
										//ロゴ取得に必要
										this->epgUtil.AddTSPacket(data + i, 188);
										break;
									}
								}
							}
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
					AddDebugLogFormat(L"★Ch Unexpected Change 0x%04X 0x%04X => 0x%04X 0x%04X", this->lastONID, this->lastTSID, onid, tsid);
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
		AddDebugLog(L"★★CTSOut::AddTSBuff Exception2");
		//デコード失敗
		decodeData = &this->decodeBuff.front();
		decodeSize = (DWORD)this->decodeBuff.size();
	}
	
	//デコード済みのデータを解析させる
	if( this->parseEpgPostProcess ){
		for( DWORD i=0; i<decodeSize; i+=188 ){
			CTSPacketUtil packet;
			if( packet.Set188TS(decodeData + i, 188) && packet.PID < BON_SELECTIVE_PID ){
				ParseEpgPacket(decodeData + i, packet);
			}
		}
	}

	//各サービス処理にデータ渡す
	{
		for( auto itrService = serviceUtilMap.begin(); itrService != serviceUtilMap.end(); itrService++ ){
			itrService->second->AddTSBuff(decodeData, decodeSize, [this](WORD onid, WORD tsid, WORD sid) -> int {
				lock_recursive_mutex lock2(this->epgUtilLock);
				EPG_EVENT_INFO* epgInfo;
				return this->epgUtil.GetEpgInfo(onid, tsid, sid, FALSE, &epgInfo) == NO_ERR ? epgInfo->event_id : -1;
			});
		}
	}
}

void CTSOut::ParseEpgPacket(BYTE* data, const CTSPacketUtil& packet)
{
	if( this->epgFile ){
		if( packet.PID == 0 && packet.payload_unit_start_indicator ){
			if( this->epgFileState == EPG_FILE_ST_NONE ){
				this->epgFileState = EPG_FILE_ST_PAT;
			}else if( this->epgFileState == EPG_FILE_ST_PAT ){
				this->epgFileState = EPG_FILE_ST_TOT;
				//番組情報が不足しないよう改めて蓄積状態をリセット
				this->epgUtil.ClearSectionStatus();
				//TOTを前倒しで書き込むための場所を確保
				BYTE nullData[188] = { 0x47, 0x1F, 0xFF, 0x10 };
				std::fill_n(nullData + 4, 184, (BYTE)0xFF);
				this->epgFileTotPos = my_ftell(this->epgFile.get());
				fwrite(nullData, 1, 188, this->epgFile.get());
			}
		}
		//まずPAT、次に(あれば)TOTを書き込む。この処理は必須ではないが番組情報をより確実かつ効率的に読み出せる
		if( packet.PID == 0x14 && this->epgFileState == EPG_FILE_ST_TOT ){
			this->epgFileState = EPG_FILE_ST_ALL;
			if( this->epgFileTotPos >= 0 ){
				my_fseek(this->epgFile.get(), this->epgFileTotPos, SEEK_SET);
			}
			fwrite(data, 1, 188, this->epgFile.get());
			my_fseek(this->epgFile.get(), 0, SEEK_END);
		}else if( (packet.PID == 0 && this->epgFileState >= EPG_FILE_ST_PAT) || this->epgFileState >= EPG_FILE_ST_TOT ){
			fwrite(data, 1, 188, this->epgFile.get());
		}
	}
	this->epgUtil.AddTSPacket(data, 188);
}

void CTSOut::UpdateFilterServiceID()
{
	vector<WORD> filterSIDList;

	for( auto itr = this->serviceUtilMap.begin(); itr != this->serviceUtilMap.end(); itr++ ){
		filterSIDList.push_back(itr->second->GetSID());
		if( filterSIDList.back() == 0xFFFF ){
			//下流の処理対象が全サービスなのでフィルタしない
			this->serviceFilter.SetServiceID(true, vector<WORD>());
			return;
		}
	}
	//下流の処理対象サービスを合計したものでフィルタ
	this->serviceFilter.SetServiceID(false, filterSIDList);
}

void CTSOut::CheckLogo(DWORD logoTypeFlags, CHECK_LOGO_RESULT& result)
{
	lock_recursive_mutex lock(this->epgUtilLock);

	this->epgUtil.SetLogoTypeFlags(logoTypeFlags, &this->logoAdditionalNeededPids);
	result.dataUpdated = false;
	result.serviceListUpdated = false;
	if( logoTypeFlags ){
		pair<CHECK_LOGO_RESULT*, vector<pair<LONGLONG, DWORD>>*> context(&result, &this->logoServiceListSizeMap);
		this->epgUtil.EnumLogoList(EnumLogoListProc, &context);
	}
}

BOOL CALLBACK CTSOut::EnumLogoListProc(DWORD logoListSize, const LOGO_INFO* logoList, void* param)
{
	CHECK_LOGO_RESULT& result = *((pair<CHECK_LOGO_RESULT*, vector<pair<LONGLONG, DWORD>>*>*)param)->first;
	vector<pair<LONGLONG, DWORD>>& serviceListSizeMap = *((pair<CHECK_LOGO_RESULT*, vector<pair<LONGLONG, DWORD>>*>*)param)->second;

	if( logoList == NULL ){
		return TRUE;
	}
	for( ; logoListSize > 0; logoListSize--, logoList++ ){
		if( logoList->serviceListSize > 0 ){
			LONGLONG key = (LONGLONG)logoList->onid << 32 | (LONGLONG)logoList->id << 16 | logoList->type;
			vector<pair<LONGLONG, DWORD>>::iterator itr = lower_bound_first(serviceListSizeMap.begin(), serviceListSizeMap.end(), key);
			try{
				if( itr == serviceListSizeMap.end() || itr->first != key ){
					serviceListSizeMap.insert(itr, pair<LONGLONG, DWORD>(key, 0));
					result.onid = logoList->onid;
					result.id = logoList->id;
					result.type = logoList->type;
					result.data.assign(logoList->data, logoList->data + logoList->dataSize);
					result.dataUpdated = true;
					return FALSE;
				}
				if( itr->second < logoList->serviceListSize ){
					result.onid = logoList->onid;
					result.id = logoList->id;
					result.type = logoList->type;
					result.serviceList.assign(logoList->serviceList, logoList->serviceList + logoList->serviceListSize);
					result.serviceListUpdated = true;
					itr->second = logoList->serviceListSize;
					return FALSE;
				}
			}catch( std::bad_alloc& ){
				return FALSE;
			}
		}
	}
	return TRUE;
}

//EPGデータの保存を開始する
//戻り値：
// TRUE（成功）、FALSE（失敗）
BOOL CTSOut::StartSaveEPG(
	const wstring& epgFilePath_
	)
{
	lock_recursive_mutex lock(this->objLock);
	if( this->epgFile != NULL ){
		return FALSE;
	}
	this->epgFilePath = epgFilePath_;
	this->epgTempFilePath = epgFilePath_;
	this->epgTempFilePath += L".tmp";

	AddDebugLogFormat(L"★%ls", this->epgFilePath.c_str());
	AddDebugLogFormat(L"★%ls", this->epgTempFilePath.c_str());

	this->epgUtil.ClearSectionStatus();
	this->epgFileState = EPG_FILE_ST_NONE;

	UtilCreateDirectories(fs_path(this->epgTempFilePath).parent_path());
	this->epgFile.reset(UtilOpenFile(this->epgTempFilePath, UTIL_SECURE_WRITE));
	if( this->epgFile == NULL ){
		AddDebugLog(L"err");
		return FALSE;
	}

	return TRUE;
}

//EPGデータの保存を終了する
void CTSOut::StopSaveEPG(
	BOOL copy
	)
{
	lock_recursive_mutex lock(this->objLock);
	if( this->epgFile == NULL ){
		return;
	}

	this->epgFile.reset();

#ifdef _WIN32
	if( copy ){
		CopyFile(this->epgTempFilePath.c_str(), this->epgFilePath.c_str(), FALSE );
	}
	DeleteFile(this->epgTempFilePath.c_str());
#else
	string strTempPath;
	string strPath;
	WtoUTF8(this->epgTempFilePath, strTempPath);
	WtoUTF8(this->epgFilePath, strPath);
	if( copy == FALSE || rename(strTempPath.c_str(), strPath.c_str()) != 0 ){
		remove(strTempPath.c_str());
	}
#endif
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
	lock_recursive_mutex lock(this->epgUtilLock);

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
	lock_recursive_mutex lock(this->epgUtilLock);

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
	lock_recursive_mutex lock(this->objLock);

	try{
		if( this->chChangeState == CH_ST_DONE ){
			//チューニング済みで
			if( enable != FALSE && this->enableDecodeFlag == FALSE && this->emmEnableFlag == FALSE ){
				//最初に EMM 処理が設定される場合は DLL を読み込む
				//スクランブル解除が設定されている場合は読み込み済みなので除外
				if( this->decodeUtil.SetNetwork(this->lastONID, this->lastTSID) == FALSE ){
					AddDebugLog(L"★★Decode DLL load err [CTSOut::SetEmm()]");
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
	lock_recursive_mutex lock(this->objLock);

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
	lock_recursive_mutex lock(this->objLock);

	return this->decodeUtil.GetLoadStatus(loadErrDll);
}

//自ストリームのサービス一覧を取得する
//戻り値：
// エラーコード
//引数：
// funcGetList		[IN]戻り値がNO_ERRのときサービス情報の個数とそのリストを引数として呼び出される関数
DWORD CTSOut::GetServiceListActual(
	const std::function<void(DWORD, SERVICE_INFO*)>& funcGetList
	)
{
	lock_recursive_mutex lock(this->epgUtilLock);

	DWORD serviceListSize;
	SERVICE_INFO* serviceList;
	DWORD ret = this->epgUtil.GetServiceListActual(&serviceListSize, &serviceList);
	if( ret == NO_ERR && funcGetList ){
		funcGetList(serviceListSize, serviceList);
	}
	return ret;
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
// 制御識別ID
//引数：
// sendUdpTcp	[IN]UDP/TCP送信用にする
DWORD CTSOut::CreateServiceCtrl(
	BOOL sendUdpTcp
	)
{
	lock_recursive_mutex lock(this->objLock);

	auto itr = this->serviceUtilMap.insert(
		std::make_pair(GetNextID(), std::unique_ptr<COneServiceUtil>(new COneServiceUtil(sendUdpTcp)))).first;
	itr->second->SetBonDriver(bonFile);
	itr->second->SetNoLogScramble(noLogScramble);
	if( this->chChangeState == CH_ST_DONE ){
		itr->second->Clear(this->lastTSID);
	}
	UpdateFilterServiceID();

	return itr->first;
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
	lock_recursive_mutex lock(this->objLock);

	if( serviceUtilMap.erase(id) == 0 ){
		return FALSE;
	}

	UpdateEnableDecodeFlag();
	UpdateFilterServiceID();

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
	lock_recursive_mutex lock(this->objLock);

	auto itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		return FALSE;
	}

	itr->second->SetSID(serviceID);
	UpdateFilterServiceID();

	return TRUE;
}

BOOL CTSOut::GetServiceID(
	DWORD id,
	WORD* serviceID
	)
{
	lock_recursive_mutex lock(this->objLock);

	auto itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		return FALSE;
	}
	if( serviceID != NULL ){
		*serviceID = itr->second->GetSID();
	}

	return TRUE;
}

BOOL CTSOut::SendUdp(
	DWORD id,
	vector<NW_SEND_INFO>* sendList
	)
{
	lock_recursive_mutex lock(this->objLock);

	auto itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		return FALSE;
	}

	itr->second->SendUdp(sendList);

	return TRUE;
}

BOOL CTSOut::SendTcp(
	DWORD id,
	vector<NW_SEND_INFO>* sendList
	)
{
	lock_recursive_mutex lock(this->objLock);

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
	lock_recursive_mutex lock(this->epgUtilLock);

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
	lock_recursive_mutex lock(this->epgUtilLock);

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
	lock_recursive_mutex lock(this->epgUtilLock);

	return this->epgUtil.GetTimeDelay();
}

//録画中かどうか
//戻り値：
// TRUE（録画中）、FALSE（していない）
BOOL CTSOut::IsRec()
{
	lock_recursive_mutex lock(this->objLock);

	for( auto itr = this->serviceUtilMap.begin(); itr != this->serviceUtilMap.end(); itr++ ){
		if( itr->second->IsRec() == TRUE ){
			return TRUE;
		}
	}

	return FALSE;
}

BOOL CTSOut::StartSave(
	const SET_CTRL_REC_PARAM& recParam,
	const vector<wstring>& saveFolderSub,
	int maxBuffCount
)
{
	lock_recursive_mutex lock(this->objLock);

	auto itr = serviceUtilMap.find(recParam.ctrlID);
	if( itr == serviceUtilMap.end() ){
		return FALSE;
	}

	return itr->second->StartSave(recParam, saveFolderSub, maxBuffCount);
}

BOOL CTSOut::EndSave(
	DWORD id,
	BOOL* subRecFlag
	)
{
	lock_recursive_mutex lock(this->objLock);
	auto itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		return FALSE;
	}

	return itr->second->EndSave(subRecFlag);
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
	lock_recursive_mutex lock(this->objLock);

	auto itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		return FALSE;
	}

	itr->second->SetScramble(enable);
	UpdateEnableDecodeFlag();
	return TRUE;
}

BOOL CTSOut::UpdateEnableDecodeFlag()
{
	BOOL sendUdpTcpOnly = TRUE;
	BOOL enable = FALSE;
	for( auto itr = this->serviceUtilMap.begin(); itr != this->serviceUtilMap.end(); itr++ ){
		if( itr->second->GetScramble() >= 0 ){
			if( itr->second->GetSendUdpTcp() ){
				//UDP/TCP送信用だけのときはその設定値に従う
				if( sendUdpTcpOnly && itr->second->GetScramble() ){
					enable = TRUE;
				}
			}else{
				//録画用のものがあるときはその設定値に従う
				sendUdpTcpOnly = FALSE;
				enable = FALSE;
				if( itr->second->GetScramble() ){
					enable = TRUE;
					break;
				}
			}
		}
	}

	try{
		if( this->chChangeState == CH_ST_DONE ){
			//チューニング済みで
			if( enable != FALSE && this->enableDecodeFlag == FALSE && this->emmEnableFlag == FALSE ){
				//最初にスクランブル解除が設定される場合は DLL を再読み込みする
				//EMM 処理が設定されている場合は読み込み済みなので除外
				if( this->decodeUtil.SetNetwork(this->lastONID, this->lastTSID) == FALSE ){
					AddDebugLog(L"★★Decode DLL load err [CTSOut::SetScramble()]");
				}
			}
		}
	}catch(...){
		return FALSE;
	}

	this->enableDecodeFlag = enable;
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
	lock_recursive_mutex lock(this->objLock);

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
	lock_recursive_mutex lock(this->objLock);

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
	lock_recursive_mutex lock(this->objLock);

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
	LONGLONG* writeSize
	)
{
	lock_recursive_mutex lock(this->objLock);

	auto itr = serviceUtilMap.find(id);
	if( itr != serviceUtilMap.end() ){
		itr->second->GetRecWriteSize(writeSize);
	}
}

wstring CTSOut::GetSaveFilePath(
	DWORD id
	)
{
	lock_recursive_mutex lock(this->objLock);

	auto itr = serviceUtilMap.find(id);
	if( itr != serviceUtilMap.end() ){
		return itr->second->GetSaveFilePath();
	}
	return wstring();
}

void CTSOut::SaveErrCount(
	DWORD id,
	const wstring& filePath,
	BOOL asUtf8,
	int dropSaveThresh,
	int scrambleSaveThresh,
	ULONGLONG& drop,
	ULONGLONG& scramble
	)
{
	lock_recursive_mutex lock(this->objLock);

	auto itr = serviceUtilMap.find(id);
	if( itr != serviceUtilMap.end() ){
		itr->second->SaveErrCount(filePath, asUtf8, dropSaveThresh, scrambleSaveThresh, drop, scramble);
	}
}

void CTSOut::SetSignalLevel(
	float signalLv
	)
{
	lock_recursive_mutex lock(this->objLock);

	for( auto itr = serviceUtilMap.begin(); itr != serviceUtilMap.end(); itr++ ){
		itr->second->SetSignalLevel(signalLv);
	}
}


void CTSOut::SetBonDriver(
	const wstring& bonDriver
	)
{
	lock_recursive_mutex lock(this->objLock);

	for( auto itr = serviceUtilMap.begin(); itr != serviceUtilMap.end(); itr++ ){
		itr->second->SetBonDriver(bonDriver);
	}
	bonFile = bonDriver;
}

void CTSOut::SetNoLogScramble(
	BOOL noLog
	)
{
	lock_recursive_mutex lock(this->objLock);

	for( auto itr = serviceUtilMap.begin(); itr != serviceUtilMap.end(); itr++ ){
		itr->second->SetNoLogScramble(noLog);
	}
	noLogScramble = noLog;
}

void CTSOut::SetParseEpgPostProcess(
	BOOL parsePost
	)
{
	lock_recursive_mutex lock(this->objLock);

	parseEpgPostProcess = parsePost;
}
