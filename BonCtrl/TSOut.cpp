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

	this->epgUtil.Initialize();

	this->enableDecodeFlag = false;
	this->emmEnableFlag = false;

	this->nextCtrlID = 1;
	this->logoAdditionalNeededPids = nullptr;
	this->noLogScramble = false;
	this->parseEpgPostProcess = false;
}


CTSOut::~CTSOut(void)
{
	StopSaveEPG(false);
}

//チャンネル変更状態に遷移する
void CTSOut::SetChChangeEvent(WORD presumedONID, bool resetEpgUtil)
{
	lock_recursive_mutex lock(this->objLock);

	this->chChangeState = CH_ST_WAIT_PAT;
	this->chChangeTime = GetU32Tick();
	this->chChangePresumedONID = presumedONID;

	this->decodeUtil.UnLoadDll();

	if( resetEpgUtil ){
		lock_recursive_mutex lock2(this->epgUtilLock);
		//EpgDataCap3は内部メソッド単位でアトミック。初期化以外はobjLockかepgUtilLockのどちらかを獲得すればよい
		this->epgUtil.UnInitialize();
		this->epgUtil.Initialize();
		this->logoAdditionalNeededPids = nullptr;
	}
}

//GetStreamID()が失敗する状態かどうか
bool CTSOut::IsChUnknown(DWORD* elapsedTime)
{
	lock_recursive_mutex lock(this->objLock);

	if( this->chChangeState != CH_ST_DONE ){
		if( elapsedTime != nullptr ){
			*elapsedTime = this->chChangeState == CH_ST_INIT ? MAXDWORD : GetU32Tick() - this->chChangeTime;
		}
		return true;
	}
	return false;
}

//現在のストリームのIDを取得する
//戻り値：
// true（成功）、false（失敗）
//引数：
// ONID		[OUT]originalNetworkID
// TSID		[OUT]transportStreamID
bool CTSOut::GetStreamID(WORD* ONID, WORD* TSID)
{
	lock_recursive_mutex lock(this->objLock);

	if( this->chChangeState == CH_ST_DONE ){
		*ONID = this->lastONID;
		*TSID = this->lastTSID;
		return true;
	}
	return false;
}

void CTSOut::OnChChanged(WORD onid, WORD tsid)
{
	this->chChangeState = CH_ST_DONE;
	this->lastONID = onid;
	this->lastTSID = tsid;
	this->epgUtil.ClearSectionStatus();

	if( this->enableDecodeFlag || this->emmEnableFlag ){
		//スクランブル解除かEMM処理が設定されている場合だけ実行
		if( this->decodeUtil.SetNetwork(onid, tsid) == false ){
			AddDebugLog(L"★★Decode DLL load err [CTSOut::OnChChanged()]");
			//再試行は意味がなさそうなので廃止
		}
		this->decodeUtil.SetEmm(this->emmEnableFlag);
	}
	for( const auto& item : this->serviceUtilMap ){
		item.second->Clear(tsid);
	}
	this->serviceFilter.Clear(tsid);
}

void CTSOut::AddTSBuff(BYTE* data, DWORD dataSize)
{
	//dataは同期済みかつそのサイズは188の整数倍であること

	lock_recursive_mutex lock(this->objLock);
	if( dataSize == 0 || data == nullptr ){
		return;
	}
	DWORD tick = GetU32Tick();
	if( this->chChangeState == CH_ST_WAIT_PAT && tick - this->chChangeTime < 1000 ){
		//1秒間はチャンネル切り替え前のパケット来る可能性を考慮して無視する
		return;
	}
	this->decodeBuff.clear();

	BYTE* decodeData = nullptr;
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
						bool ret = this->epgUtil.GetTSID(&onid, &tsid);
						if( ret == false && this->chChangePresumedONID != 0xFFFF && this->lastTSID != 0xFFFF ){
							//ONIDの取得は遅いことがあるので、チャンネル変更などでONIDが予測できる場合はそれを使う
							//切り替え前との区別を確実にするためTSIDが変化する場合に限る
							onid = this->chChangePresumedONID;
							ret = this->epgUtil.GetTSID(nullptr, &tsid) && tsid != this->lastTSID;
						}
						if( ret ){
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
					if( this->parseEpgPostProcess == false ){
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
			if( this->epgUtil.GetTSID(&onid, &tsid) ){
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

				if( decodeUtil.Decode(&this->decodeBuff.front(), (DWORD)this->decodeBuff.size(), &decodeData, &decodeSize) == false ){
					//デコード失敗
					decodeData = &this->decodeBuff.front();
					decodeSize = (DWORD)this->decodeBuff.size();
				}else{
					if( decodeData == nullptr || decodeSize == 0 ){
						decodeData = nullptr;
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
	for( const auto& item : this->serviceUtilMap ){
		item.second->AddTSBuff(decodeData, decodeSize, [this](WORD onid, WORD tsid, WORD sid) -> int {
			lock_recursive_mutex lock2(this->epgUtilLock);
			EPG_EVENT_INFO* epgInfo;
			return this->epgUtil.GetEpgInfo(onid, tsid, sid, false, &epgInfo) ? epgInfo->event_id : -1;
		});
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

	for( const auto& item : this->serviceUtilMap ){
		filterSIDList.push_back(item.second->GetSID());
		if( filterSIDList.back() == 0xFFFF ){
			//下流の処理対象が全サービスなのでフィルタしない
			this->serviceFilter.SetServiceID(true, vector<WORD>());
			return;
		}
	}
	//下流の処理対象サービスを合計したものでフィルタ
	this->serviceFilter.SetServiceID(false, filterSIDList);
}

//ロゴの更新を調べる
//引数：
// logoTypeFlags	[IN]フラグ(ロゴタイプnを取得するとき下位からnビット目をセットする)
// result			[OUT]結果
void CTSOut::CheckLogo(DWORD logoTypeFlags, CHECK_LOGO_RESULT& result)
{
	lock_recursive_mutex lock(this->epgUtilLock);

	this->epgUtil.SetLogoTypeFlags(logoTypeFlags, &this->logoAdditionalNeededPids);
	result.dataUpdated = false;
	result.serviceListUpdated = false;
	if( logoTypeFlags ){
		this->epgUtil.EnumLogoList([this, &result](DWORD logoListSize, const LOGO_INFO* logoList) {
			return EnumLogoListProc(logoListSize, logoList, this->logoServiceListSizeMap, result);
		});
	}
}

bool CTSOut::EnumLogoListProc(DWORD logoListSize, const LOGO_INFO* logoList, vector<pair<LONGLONG, DWORD>>& serviceListSizeMap, CHECK_LOGO_RESULT& result)
{
	if( logoList == nullptr ){
		return true;
	}
	for( ; logoListSize > 0; logoListSize--, logoList++ ){
		if( logoList->serviceListSize > 0 ){
			LONGLONG key = (LONGLONG)logoList->onid << 32 | (LONGLONG)logoList->id << 16 | logoList->type;
			vector<pair<LONGLONG, DWORD>>::iterator itr = lower_bound_first(serviceListSizeMap.begin(), serviceListSizeMap.end(), key);
			try{
				if( itr == serviceListSizeMap.end() || itr->first != key ){
					serviceListSizeMap.emplace(itr, key, 0);
					result.onid = logoList->onid;
					result.id = logoList->id;
					result.type = logoList->type;
					result.data.assign(logoList->data, logoList->data + logoList->dataSize);
					result.dataUpdated = true;
					return false;
				}
				if( itr->second < logoList->serviceListSize ){
					result.onid = logoList->onid;
					result.id = logoList->id;
					result.type = logoList->type;
					result.serviceList.assign(logoList->serviceList, logoList->serviceList + logoList->serviceListSize);
					result.serviceListUpdated = true;
					itr->second = logoList->serviceListSize;
					return false;
				}
			}catch( std::bad_alloc& ){
				return false;
			}
		}
	}
	return true;
}

//EPGデータの保存を開始する
//戻り値：
// true（成功）、false（失敗）
bool CTSOut::StartSaveEPG(
	const wstring& epgFilePath_
	)
{
	lock_recursive_mutex lock(this->objLock);
	if( this->epgFile != nullptr ){
		return false;
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
	if( this->epgFile == nullptr ){
		AddDebugLog(L"err");
		return false;
	}

	return true;
}

//EPGデータの保存を終了する
//引数：
// copy			[IN]tmpからコピー処理行うかどうか
void CTSOut::StopSaveEPG(
	bool copy
	)
{
	lock_recursive_mutex lock(this->objLock);
	if( this->epgFile == nullptr ){
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
	if( copy == false || rename(strTempPath.c_str(), strPath.c_str()) != 0 ){
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
	bool l_eitFlag
	)
{
	lock_recursive_mutex lock(this->epgUtilLock);

	return this->epgUtil.GetSectionStatus(l_eitFlag);
}

//指定サービスのEPGデータの蓄積状態を取得する
//戻り値：
// ステータス,取得できたかどうか
//引数：
// originalNetworkID		[IN]取得対象のOriginalNetworkID
// transportStreamID		[IN]取得対象のTransportStreamID
// serviceID				[IN]取得対象のServiceID
// l_eitFlag				[IN]L-EITのステータスを取得
pair<EPG_SECTION_STATUS, bool> CTSOut::GetSectionStatusService(
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	bool l_eitFlag
	)
{
	lock_recursive_mutex lock(this->epgUtilLock);

	return this->epgUtil.GetSectionStatusService(originalNetworkID, transportStreamID, serviceID, l_eitFlag);
}

//EMM処理の動作設定
//戻り値：
// true（成功）、false（失敗）
//引数：
// enable		[IN] true（処理する）、false（処理しない）
bool CTSOut::SetEmm(
	bool enable
	)
{
	lock_recursive_mutex lock(this->objLock);

	try{
		if( this->chChangeState == CH_ST_DONE ){
			//チューニング済みで
			if( enable && this->enableDecodeFlag == false && this->emmEnableFlag == false ){
				//最初に EMM 処理が設定される場合は DLL を読み込む
				//スクランブル解除が設定されている場合は読み込み済みなので除外
				if( this->decodeUtil.SetNetwork(this->lastONID, this->lastTSID) == false ){
					AddDebugLog(L"★★Decode DLL load err [CTSOut::SetEmm()]");
				}
			}
		}
	}catch(...){
		return false;
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
// true（ロードに成功している）、false（ロードに失敗している）
//引数：
// loadErrDll		[OUT]ロードに失敗したDLLファイル名
bool CTSOut::GetLoadStatus(
	wstring& loadErrDll
	)
{
	lock_recursive_mutex lock(this->objLock);

	return this->decodeUtil.GetLoadStatus(loadErrDll);
}

//自ストリームのサービス一覧を取得する
//戻り値：
// true（成功）、false（失敗）
//引数：
// funcGetList		[IN]戻り値がtrueのときサービス情報の個数とそのリストを引数として呼び出される関数
bool CTSOut::GetServiceListActual(
	const std::function<void(DWORD, SERVICE_INFO*)>& funcGetList
	)
{
	lock_recursive_mutex lock(this->epgUtilLock);

	DWORD serviceListSize;
	SERVICE_INFO* serviceList;
	if( this->epgUtil.GetServiceListActual(&serviceListSize, &serviceList) ){
		if( funcGetList ){
			funcGetList(serviceListSize, serviceList);
		}
		return true;
	}
	return false;
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
	bool sendUdpTcp
	)
{
	lock_recursive_mutex lock(this->objLock);

	auto itr = this->serviceUtilMap.emplace(GetNextID(), std::unique_ptr<COneServiceUtil>(new COneServiceUtil(sendUdpTcp))).first;
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
// true（成功）、false（失敗）
//引数：
// id			[IN]制御識別ID
bool CTSOut::DeleteServiceCtrl(
	DWORD id
	)
{
	lock_recursive_mutex lock(this->objLock);

	if( serviceUtilMap.erase(id) == 0 ){
		return false;
	}

	UpdateEnableDecodeFlag();
	UpdateFilterServiceID();

	return true;
}

//制御対象のサービスを設定する
//戻り値：
// true（成功）、false（失敗）
//引数：
// id			[IN]制御識別ID
// serviceID	[IN]対象サービスID、0xFFFFで全サービス対象
bool CTSOut::SetServiceID(
	DWORD id,
	WORD serviceID
	)
{
	lock_recursive_mutex lock(this->objLock);

	auto itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		return false;
	}

	itr->second->SetSID(serviceID);
	UpdateFilterServiceID();

	return true;
}

bool CTSOut::GetServiceID(
	DWORD id,
	WORD* serviceID
	)
{
	lock_recursive_mutex lock(this->objLock);

	auto itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		return false;
	}
	if( serviceID != nullptr ){
		*serviceID = itr->second->GetSID();
	}

	return true;
}

//UDPで送信を行う
//戻り値：
// true（成功）、false（失敗）
//引数：
// id			[IN]制御識別ID
// sendList		[IN/OUT]送信先リスト。nullptrで停止。送信に使用したポート(失敗のものは0x10000)がportにセットされる。
bool CTSOut::SendUdp(
	DWORD id,
	vector<NW_SEND_INFO>* sendList
	)
{
	lock_recursive_mutex lock(this->objLock);

	auto itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		return false;
	}

	itr->second->SendUdp(sendList);

	return true;
}

//TCPで送信を行う
//戻り値：
// true（成功）、false（失敗）
//引数：
// id			[IN]制御識別ID
// sendList		[IN/OUT]送信先リスト。nullptrで停止。送信に使用したポート(失敗のものは0x10000)がportにセットされる。
bool CTSOut::SendTcp(
	DWORD id,
	vector<NW_SEND_INFO>* sendList
	)
{
	lock_recursive_mutex lock(this->objLock);

	auto itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		return false;
	}

	itr->second->SendTcp(sendList);

	return true;
}

//指定サービスの現在or次のEPG情報を取得する
//戻り値：
// true（成功）、false（失敗）
//引数：
// originalNetworkID		[IN]取得対象のoriginalNetworkID
// transportStreamID		[IN]取得対象のtransportStreamID
// serviceID				[IN]取得対象のServiceID
// nextFlag					[IN]true（次の番組）、false（現在の番組）
// epgInfo					[OUT]EPG情報
bool CTSOut::GetEpgInfo(
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	bool nextFlag,
	EPGDB_EVENT_INFO* epgInfo
	)
{
	lock_recursive_mutex lock(this->epgUtilLock);

	EPG_EVENT_INFO* info;
	if( this->epgUtil.GetEpgInfo(originalNetworkID, transportStreamID, serviceID, nextFlag, &info) ){
		ConvertEpgInfo(originalNetworkID, transportStreamID, serviceID, info, epgInfo);
		return true;
	}
	return false;
}

//指定イベントのEPG情報を取得する
//戻り値：
// true（成功）、false（失敗）
//引数：
// originalNetworkID		[IN]取得対象のoriginalNetworkID
// transportStreamID		[IN]取得対象のtransportStreamID
// serviceID				[IN]取得対象のServiceID
// eventID					[IN]取得対象のEventID
// pfOnlyFlag				[IN]p/fからのみ検索するかどうか
// epgInfo					[OUT]EPG情報
bool CTSOut::SearchEpgInfo(
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	WORD eventID,
	bool pfOnlyFlag,
	EPGDB_EVENT_INFO* epgInfo
	)
{
	lock_recursive_mutex lock(this->epgUtilLock);

	EPG_EVENT_INFO* info;
	if( this->epgUtil.SearchEpgInfo(originalNetworkID, transportStreamID, serviceID, eventID, pfOnlyFlag, &info) ){
		ConvertEpgInfo(originalNetworkID, transportStreamID, serviceID, info, epgInfo);
		return true;
	}
	return false;
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
// true（録画中）、false（していない）
bool CTSOut::IsRec()
{
	lock_recursive_mutex lock(this->objLock);

	for( const auto& item : this->serviceUtilMap ){
		if( item.second->IsRec() ){
			return true;
		}
	}

	return false;
}

//ファイル保存を開始する
//戻り値：
// true（成功）、false（失敗）
//引数：
// recParam				[IN]保存パラメータ
// saveFolderSub		[IN]HDDの空きがなくなった場合に一時的に使用するフォルダ
// maxBuffCount			[IN]出力バッファ上限
bool CTSOut::StartSave(
	const SET_CTRL_REC_PARAM& recParam,
	const vector<wstring>& saveFolderSub,
	int maxBuffCount
)
{
	lock_recursive_mutex lock(this->objLock);

	auto itr = serviceUtilMap.find(recParam.ctrlID);
	if( itr == serviceUtilMap.end() ){
		return false;
	}

	return itr->second->StartSave(recParam, saveFolderSub, maxBuffCount);
}

//ファイル保存を終了する
//戻り値：
// true（成功）、false（失敗）
//引数：
// id			[IN]制御識別ID
// subRecFlag	[OUT]成功のとき、サブ録画が発生したかどうか
bool CTSOut::EndSave(
	DWORD id,
	bool* subRecFlag
	)
{
	lock_recursive_mutex lock(this->objLock);
	auto itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		return false;
	}

	return itr->second->EndSave(subRecFlag);
}

//スクランブル解除処理の動作設定
//戻り値：
// true（成功）、false（失敗）
//引数：
// enable		[IN] true（処理する）、false（処理しない）
bool CTSOut::SetScramble(
	DWORD id,
	bool enable
	)
{
	lock_recursive_mutex lock(this->objLock);

	auto itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		return false;
	}

	itr->second->SetScramble(enable);
	UpdateEnableDecodeFlag();
	return true;
}

bool CTSOut::UpdateEnableDecodeFlag()
{
	bool sendUdpTcpOnly = true;
	bool enable = false;
	for( const auto& item : this->serviceUtilMap ){
		if( item.second->GetScramble() >= 0 ){
			if( item.second->GetSendUdpTcp() ){
				//UDP/TCP送信用だけのときはその設定値に従う
				if( sendUdpTcpOnly && item.second->GetScramble() ){
					enable = true;
				}
			}else{
				//録画用のものがあるときはその設定値に従う
				sendUdpTcpOnly = false;
				enable = false;
				if( item.second->GetScramble() ){
					enable = true;
					break;
				}
			}
		}
	}

	try{
		if( this->chChangeState == CH_ST_DONE ){
			//チューニング済みで
			if( enable && this->enableDecodeFlag == false && this->emmEnableFlag == false ){
				//最初にスクランブル解除が設定される場合は DLL を再読み込みする
				//EMM 処理が設定されている場合は読み込み済みなので除外
				if( this->decodeUtil.SetNetwork(this->lastONID, this->lastTSID) == false ){
					AddDebugLog(L"★★Decode DLL load err [CTSOut::SetScramble()]");
				}
			}
		}
	}catch(...){
		return false;
	}

	this->enableDecodeFlag = enable;
	return true;
}

//字幕とデータ放送含めるかどうか
//引数：
// id					[IN]制御識別ID
// enableCaption		[IN]字幕を true（含める）、false（含めない）
// enableData			[IN]データ放送を true（含める）、false（含めない）
void CTSOut::SetServiceMode(
	DWORD id,
	bool enableCaption,
	bool enableData
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

//録画中のファイルのファイルパスを取得する
//戻り値：
// ファイルパス
//引数：
// id					[IN]制御識別ID
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

//ドロップとスクランブルのカウントを保存する
//引数：
// id					[IN]制御識別ID
// filePath				[IN]保存ファイル名
// asUtf8				[IN]UTF-8で保存するか
// dropSaveThresh		[IN]ドロップ数がこれ以上なら保存する
// drop					[OUT]ドロップ数
void CTSOut::SaveErrCount(
	DWORD id,
	const wstring& filePath,
	bool asUtf8,
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

	for( const auto& item : this->serviceUtilMap ){
		item.second->SetSignalLevel(signalLv);
	}
}


void CTSOut::SetBonDriver(
	const wstring& bonDriver
	)
{
	lock_recursive_mutex lock(this->objLock);

	for( const auto& item : this->serviceUtilMap ){
		item.second->SetBonDriver(bonDriver);
	}
	bonFile = bonDriver;
}

void CTSOut::SetNoLogScramble(
	bool noLog
	)
{
	lock_recursive_mutex lock(this->objLock);

	for( const auto& item : this->serviceUtilMap ){
		item.second->SetNoLogScramble(noLog);
	}
	noLogScramble = noLog;
}

void CTSOut::SetParseEpgPostProcess(
	bool parsePost
	)
{
	lock_recursive_mutex lock(this->objLock);

	parseEpgPostProcess = parsePost;
}
