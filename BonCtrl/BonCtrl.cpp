#include "stdafx.h"
#include "BonCtrl.h"

#include "../Common/CommonDef.h"
#include "../Common/TimeUtil.h"
#include "../Common/SendCtrlCmd.h"

CBonCtrl::CBonCtrl(void)
{
	this->statusSignalLv = 0.0f;
	this->viewSpace = -1;
	this->viewCh = -1;

	this->nwCtrlID = this->tsOut.CreateServiceCtrl(true);
	this->nwCtrlEnableScramble = true;
	this->nwCtrlNeedCaption = true;
	this->nwCtrlNeedData = false;
	this->nwCtrlAllService = false;
	this->nwCtrlServiceID = 0xFFFF;

	this->chScanIndexOrStatus = ST_STOP;
	this->epgCapIndexOrStatus = ST_STOP;
	this->epgCapBackIndexOrStatus = ST_STOP;
	this->enableLiveEpgCap = false;
	this->enableRecEpgCap = false;

	this->epgCapBackBSBasic = true;
	this->epgCapBackCS1Basic = true;
	this->epgCapBackCS2Basic = true;
	this->epgCapBackCS3Basic = false;
	this->epgCapBackStartWaitSec = 30;

	this->tsOutLogoTypeFlags = 0;
}


CBonCtrl::~CBonCtrl(void)
{
	CloseBonDriver();

	StopChScan();
}

void CBonCtrl::ReloadSetting(
	bool enableEmm,
	bool noLogScramble,
	bool parseEpgPostProcess,
	bool enableScramble,
	bool needCaption,
	bool needData,
	bool allService,
	DWORD logoTypeFlags
	)
{
	this->tsOut.SetEmm(enableEmm);
	this->tsOut.SetNoLogScramble(noLogScramble);
	this->tsOut.SetParseEpgPostProcess(parseEpgPostProcess);
	this->nwCtrlEnableScramble = enableScramble;
	this->nwCtrlNeedCaption = needCaption;
	this->nwCtrlNeedData = needData;
	this->nwCtrlAllService = allService;
	this->tsOut.SetScramble(this->nwCtrlID, this->nwCtrlEnableScramble);
	this->tsOut.SetServiceMode(this->nwCtrlID, this->nwCtrlNeedCaption, this->nwCtrlNeedData);
	this->tsOut.SetServiceID(this->nwCtrlID, this->nwCtrlAllService ? 0xFFFF : this->nwCtrlServiceID);
	this->tsOutLogoTypeFlags = logoTypeFlags;
}

//ネットワーク送信と統計の対象サービスIDを設定する
//※GetStreamID()で受動的なチャンネル変化を検出した時などに使う
void CBonCtrl::SetNWCtrlServiceID(
	WORD serviceID
	)
{
	if( this->nwCtrlServiceID != serviceID ){
		this->nwCtrlServiceID = serviceID;
		this->tsOut.SetServiceID(this->nwCtrlID, this->nwCtrlAllService ? 0xFFFF : this->nwCtrlServiceID);
	}
}

//EPG取得などの状態を更新する
//※概ね1秒ごとに呼ぶ
void CBonCtrl::Check()
{
	CheckChScan();
	CheckEpgCap();
	CheckEpgCapBack();
	CheckLogo();
}

//BonDriverをロードしてチャンネル情報などを取得（ファイル名で指定）
//戻り値：
// true（成功）、false（失敗）
//引数：
// bonDriverFile	[IN]BonDriverのファイル名
bool CBonCtrl::OpenBonDriver(
	LPCWSTR bonDriverFile,
	int traceLevel,
	int openWait,
	DWORD tsBuffMaxCount
)
{
	CloseBonDriver();
	if( this->bonUtil.OpenBonDriver(
#ifdef EDCB_LIB_ROOT
	                                EDCB_LIB_ROOT,
#else
	                                GetModulePath().replace_filename(BON_DLL_FOLDER).c_str(),
#endif
	                                bonDriverFile,
	                                [=](BYTE* data, DWORD size, DWORD remain) { RecvCallback(data, size, remain, tsBuffMaxCount); },
	                                [=](float signalLv, int space, int ch) { StatusCallback(signalLv, space, ch); }, traceLevel) ){
		if( openWait > 0 ){
			SleepForMsec(openWait);
		}
		wstring bonFile = this->bonUtil.GetOpenBonDriverFileName();
		//解析スレッド起動
		this->analyzeStopFlag = false;
		this->analyzeThread = thread_(AnalyzeThread, this);

		this->tsOut.SetBonDriver(bonFile);
		this->chUtil.LoadChSet(GetSettingPath().native(), bonFile, this->bonUtil.GetTunerName());
		this->tsOut.ClearErrCount(this->nwCtrlID);
		return true;
	}

	return false;
}

//ロード中のBonDriverのファイル名を取得する（ロード成功しているかの判定）
//※スレッドセーフ
//戻り値：
// true（成功）、false（Openに失敗している）
//引数：
// bonDriverFile		[OUT]BonDriverのファイル名(nullptr可)
bool CBonCtrl::GetOpenBonDriver(
	wstring* bonDriverFile
	)
{
	if( bonDriverFile == nullptr ){
		return this->bonUtil.IsOpen();
	}
	wstring strBonDriverFile = this->bonUtil.GetOpenBonDriverFileName();
	if( strBonDriverFile.empty() == false ){
		*bonDriverFile = strBonDriverFile;
		return true;
	}

	return false;
}

//チャンネル変更
//戻り値：
// true（成功）、false（失敗）
//引数：
// chData		[IN]変更チャンネルのサービス情報
bool CBonCtrl::SetCh(
	const CH_DATA4& chData
)
{
	if( this->tsOut.IsRec() ){
		return false;
	}
	StopEpgCap();

	if( ProcessSetCh(chData.space, chData.ch, chData.originalNetworkID) ){
		this->nwCtrlServiceID = chData.serviceID;
		this->tsOut.SetServiceID(this->nwCtrlID, this->nwCtrlAllService ? 0xFFFF : this->nwCtrlServiceID);
		return true;
	}

	return false;
}

bool CBonCtrl::ProcessSetCh(
	DWORD space,
	DWORD ch,
	int onidOrChScan
	)
{
	DWORD spaceNow=0;
	DWORD chNow=0;

	bool ret = false;
	if( this->bonUtil.IsOpen() ){
		ret = true;
		DWORD elapsed;
		if( this->bonUtil.GetNowCh(&spaceNow, &chNow) == false || space != spaceNow || ch != chNow || this->tsOut.IsChUnknown(&elapsed) && elapsed > 15000 ){
			StopBackgroundEpgCap();
			this->tsOut.SetChChangeEvent((WORD)(onidOrChScan < 0 ? 0xFFFF : onidOrChScan), onidOrChScan < 0);
			AddDebugLogFormat(L"SetCh space %d, ch %d", space, ch);
			ret = this->bonUtil.SetCh(space, ch);
			StartBackgroundEpgCap();
		}
	}else{
		AddDebugLog(L"Err GetNowCh");
	}
	return ret;
}

//チャンネル変更中かどうか
//※スレッドセーフ
//戻り値：
// true（変更中）、false（完了）
bool CBonCtrl::IsChChanging(bool* chChgErr)
{
	if( chChgErr != nullptr ){
		*chChgErr = false;
	}
	DWORD elapsed;
	if( this->tsOut.IsChUnknown(&elapsed) && elapsed != MAXDWORD ){
		if( elapsed > 15000 ){
			//タイムアウトした
			if( chChgErr != nullptr ){
				*chChgErr = true;
			}
			return false;
		}
		return true;
	}
	return false;
}

//ロードしているBonDriverの開放
void CBonCtrl::CloseBonDriver()
{
	StopBackgroundEpgCap();

	StopEpgCap();

	if( this->analyzeThread.joinable() ){
		this->analyzeStopFlag = true;
		this->analyzeEvent.Set();
		this->analyzeThread.join();
	}

	this->bonUtil.CloseBonDriver();
	this->packetInit.ClearBuff();
	this->tsBuffList.clear();
	this->tsFreeList.clear();
	this->nwCtrlServiceID = 0xFFFF;
}

void CBonCtrl::RecvCallback(BYTE* data, DWORD size, DWORD remain, DWORD tsBuffMaxCount)
{
	BYTE* outData;
	DWORD outSize;
	if( data != nullptr && size != 0 && this->packetInit.GetTSData(data, size, &outData, &outSize) ){
		lock_recursive_mutex lock(this->buffLock);
		while( outSize != 0 ){
			if( this->tsFreeList.empty() ){
				//バッファを増やす
				if( this->tsBuffList.size() > tsBuffMaxCount ){
					for( auto itr = this->tsBuffList.begin(); itr != this->tsBuffList.end(); (itr++)->clear() );
					this->tsFreeList.splice(this->tsFreeList.end(), this->tsBuffList);
				}else{
					this->tsFreeList.emplace_back();
					this->tsFreeList.back().reserve(48128);
				}
			}
			DWORD insertSize = min(48128 - (DWORD)this->tsFreeList.front().size(), outSize);
			this->tsFreeList.front().insert(this->tsFreeList.front().end(), outData, outData + insertSize);
			if( this->tsFreeList.front().size() == 48128 ){
				this->tsBuffList.splice(this->tsBuffList.end(), this->tsFreeList, this->tsFreeList.begin());
			}
			outData += insertSize;
			outSize -= insertSize;
		}
	}
	if( remain == 0 ){
		this->analyzeEvent.Set();
	}
}

void CBonCtrl::StatusCallback(float signalLv, int space, int ch)
{
	lock_recursive_mutex lock(this->buffLock);
	this->statusSignalLv = signalLv;
	this->viewSpace = space;
	this->viewCh = ch;
}

void CBonCtrl::AnalyzeThread(CBonCtrl* sys)
{
	std::list<vector<BYTE>> data;

	while( sys->analyzeStopFlag == false ){
		//バッファからデータ取り出し
		float signalLv;
		{
			lock_recursive_mutex lock(sys->buffLock);
			if( data.empty() == false ){
				//返却
				data.front().clear();
				sys->tsFreeList.splice(sys->tsFreeList.end(), data);
			}
			if( sys->tsBuffList.empty() == false ){
				data.splice(data.end(), sys->tsBuffList, sys->tsBuffList.begin());
			}
			signalLv = sys->statusSignalLv;
		}
		sys->tsOut.SetSignalLevel(signalLv);
		if( data.empty() == false ){
			sys->tsOut.AddTSBuff(&data.front().front(), (DWORD)data.front().size());
		}else{
			sys->analyzeEvent.WaitOne(1000);
		}
	}
}

//TSストリーム制御用コントロールを作成する
//戻り値：
// 制御識別ID
//引数：
// duplicateNWCtrl		[IN]ネットワーク送信と統計用のものと同じ初期値を適用するかどうか
DWORD CBonCtrl::CreateServiceCtrl(
	bool duplicateNWCtrl
	)
{
	DWORD id = this->tsOut.CreateServiceCtrl(false);
	if( duplicateNWCtrl ){
		this->tsOut.SetScramble(id, this->nwCtrlEnableScramble);
		this->tsOut.SetServiceMode(id, this->nwCtrlNeedCaption, this->nwCtrlNeedData);
		this->tsOut.SetServiceID(id, this->nwCtrlAllService ? 0xFFFF : this->nwCtrlServiceID);
	}
	return id;
}

//ファイル保存を開始する
//戻り値：
// true（成功）、false（失敗）
//引数：
// recParam				[IN]保存パラメータ
// saveFolderSub		[IN]HDDの空きがなくなった場合に一時的に使用するフォルダ
// writeBuffMaxCount	[IN]出力バッファ上限
bool CBonCtrl::StartSave(
	const SET_CTRL_REC_PARAM& recParam,
	const vector<wstring>& saveFolderSub,
	int writeBuffMaxCount
)
{
	if( this->tsOut.StartSave(recParam, saveFolderSub, writeBuffMaxCount) ){
		StartBackgroundEpgCap();
		return true;
	}
	return false;
}

//チャンネルスキャンを開始する
//戻り値：
// true（成功）、false（失敗）
bool CBonCtrl::StartChScan()
{
	if( this->tsOut.IsRec() ){
		return false;
	}
	if( this->chScanIndexOrStatus >= ST_WORKING ||
	    this->epgCapIndexOrStatus >= ST_WORKING ){
		return false;
	}

	StopBackgroundEpgCap();

	if( this->bonUtil.IsOpen() ){
		this->chScanChkList.clear();
		const vector<pair<wstring, vector<wstring>>>& spaceList = this->bonUtil.GetOriginalChList();
		for( size_t i = 0; i < spaceList.size(); i++ ){
			for( size_t j = 0; j < spaceList[i].second.size(); j++ ){
				if( spaceList[i].second[j].empty() == false ){
					CHK_CH_INFO item;
					item.space = (DWORD)i;
					item.spaceName = spaceList[i].first;
					item.ch = (DWORD)j;
					item.chName = spaceList[i].second[j];
					this->chScanChkList.push_back(item);
				}
			}
		}
		this->chScanIndexOrStatus = ST_WORKING;
		if( this->chScanChkList.empty() ){
			//スキャンするものがない。ST_COMPLETEに遷移
			CheckChScan();
			return true;
		}
		//スキャン開始
		return true;
	}

	return false;
}

//チャンネルスキャンをキャンセルする
void CBonCtrl::StopChScan()
{
	if( this->chScanIndexOrStatus >= ST_WORKING ){
		this->chUtil.LoadChSet(GetSettingPath().native(), this->bonUtil.GetOpenBonDriverFileName(), this->bonUtil.GetTunerName());
		this->chScanIndexOrStatus = ST_CANCEL;
	}
}

//チャンネルスキャンの状態を取得する
//戻り値：
// ステータス
//引数：
// space		[OUT]スキャン中の物理CHのspace
// ch			[OUT]スキャン中の物理CHのch
// chName		[OUT]スキャン中の物理CHの名前
// chkNum		[OUT]チェック済みの数
// totalNum		[OUT]チェック対象の総数
CBonCtrl::JOB_STATUS CBonCtrl::GetChScanStatus(
	DWORD* space,
	DWORD* ch,
	wstring* chName,
	DWORD* chkNum,
	DWORD* totalNum
	)
{
	int indexOrStatus = this->chScanIndexOrStatus;
	if( indexOrStatus < ST_WORKING ){
		return (JOB_STATUS)indexOrStatus;
	}
	if( indexOrStatus < 0 ){
		indexOrStatus = 0;
	}
	if( space != nullptr ){
		*space = this->chScanChkList[indexOrStatus].space;
	}
	if( ch != nullptr ){
		*ch = this->chScanChkList[indexOrStatus].ch;
	}
	if( chName != nullptr ){
		*chName = this->chScanChkList[indexOrStatus].chName;
	}
	if( chkNum != nullptr ){
		*chkNum = indexOrStatus;
	}
	if( totalNum != nullptr ){
		*totalNum = (DWORD)this->chScanChkList.size();
	}
	return ST_WORKING;
}

void CBonCtrl::CheckChScan()
{
	int chkCount = this->chScanIndexOrStatus;
	if( chkCount >= ST_WORKING ){
		if( chkCount == ST_WORKING ){
			fs_path iniPath = GetCommonIniPath().replace_filename(L"BonCtrl.ini");
			this->chScanChChgTimeOut = GetPrivateProfileInt(L"CHSCAN", L"ChChgTimeOut", 9, iniPath.c_str());
			this->chScanServiceChkTimeOut = GetPrivateProfileInt(L"CHSCAN", L"ServiceChkTimeOut", 8, iniPath.c_str());
			this->chUtil.Clear();
			this->chScanChkNext = true;
		}

		if( this->chScanChkNext == false ){
			DWORD elapsed;
			if( this->tsOut.IsChUnknown(&elapsed) ){
				if( elapsed > this->chScanChChgTimeOut * 1000 ){
					//チャンネル切り替えにchChgTimeOut秒以上かかってるので無信号と判断
					AddDebugLog(L"★AutoScan Ch Change timeout");
					this->chScanChkNext = true;
				}
			}else{
				if( GetU32Tick() - this->chScanTick > (this->chScanChChgTimeOut + this->chScanServiceChkTimeOut) * 1000 ){
					//チャンネル切り替え成功したけどサービス一覧とれないので無信号と判断
					AddDebugLog(L"★AutoScan GetService timeout");
					this->chScanChkNext = true;
				}else{
					//サービス一覧の取得を行う
					this->tsOut.GetServiceListActual([=](DWORD serviceListSize, SERVICE_INFO* serviceList) {
						if( serviceListSize > 0 ){
							//一覧の取得ができた
							for( int currSID = 0; currSID < 0x10000; ){
								//ServiceID順に追加
								int nextSID = 0x10000;
								for( DWORD i = 0; i < serviceListSize; i++ ){
									WORD serviceID = serviceList[i].service_id;
									if( serviceID == currSID && serviceList[i].extInfo && serviceList[i].extInfo->service_name ){
										if( wcslen(serviceList[i].extInfo->service_name) > 0 ){
											this->chUtil.AddServiceInfo(this->chScanChkList[chkCount].space,
											                            this->chScanChkList[chkCount].ch,
											                            this->chScanChkList[chkCount].chName, serviceList[i]);
										}
									}
									if( serviceID > currSID && serviceID < nextSID ){
										nextSID = serviceID;
									}
								}
								currSID = nextSID;
							}
							this->chScanChkNext = true;
						}
					});
				}
			}
		}

		if( this->chScanChkNext ){
			//次のチャンネルへ
			chkCount++;
			if( this->chScanChkList.size() <= (size_t)chkCount ){
				//全部チェック終わったので終了
				this->chUtil.SaveChSet(GetSettingPath().native(), this->bonUtil.GetOpenBonDriverFileName(), this->bonUtil.GetTunerName());
				this->chScanIndexOrStatus = ST_COMPLETE;
				return;
			}
			this->chScanIndexOrStatus = chkCount;
			if( this->ProcessSetCh(this->chScanChkList[chkCount].space, this->chScanChkList[chkCount].ch, -1) ){
				this->chScanTick = GetU32Tick();
				this->chScanChkNext = false;
			}
		}
	}
}

//EPG取得を開始する
//戻り値：
// true（成功）、false（失敗）
//引数：
// chList		[IN]EPG取得するチャンネル一覧(nullptr可)
bool CBonCtrl::StartEpgCap(
	const vector<SET_CH_INFO>* chList
	)
{
	if( this->tsOut.IsRec() ){
		return false;
	}
	if( this->chScanIndexOrStatus >= ST_WORKING ||
	    this->epgCapIndexOrStatus >= ST_WORKING ){
		return false;
	}

	StopBackgroundEpgCap();

	if( this->bonUtil.IsOpen() ){
		if( chList ){
			this->epgCapChList.clear();
			for( size_t i = 0; i < chList->size(); i++ ){
				//SID指定のみ対応
				if( (*chList)[i].useSID ){
					this->epgCapChList.push_back((*chList)[i]);
				}
			}
		}else{
			this->epgCapChList = this->chUtil.GetEpgCapService();
		}
		if( this->epgCapChList.empty() ){
			//取得するものがない
			this->epgCapIndexOrStatus = ST_COMPLETE;
			return true;
		}
		//取得開始
		this->epgCapIndexOrStatus = ST_WORKING;
		return true;
	}

	return false;
}

//EPG取得を停止する
void CBonCtrl::StopEpgCap(
	)
{
	if( this->epgCapIndexOrStatus >= ST_WORKING ){
		this->tsOut.StopSaveEPG(false);
		this->epgCapIndexOrStatus = ST_CANCEL;
	}
}

//EPG取得のステータスを取得する
//※info==nullptrの場合に限りスレッドセーフ
//戻り値：
// ステータス
//引数：
// info			[OUT]取得中のサービス
CBonCtrl::JOB_STATUS CBonCtrl::GetEpgCapStatus(
	SET_CH_INFO* info
	)
{
	int indexOrStatus = this->epgCapIndexOrStatus;
	if( indexOrStatus < ST_WORKING ){
		return (JOB_STATUS)indexOrStatus;
	}
	if( indexOrStatus < 0 ){
		indexOrStatus = 0;
	}
	if( info != nullptr ){
		*info = this->epgCapChList[indexOrStatus];
	}
	return ST_WORKING;
}

void CBonCtrl::CheckEpgCap()
{
	int chkCount = this->epgCapIndexOrStatus;
	if( chkCount >= ST_WORKING ){
		if( chkCount == ST_WORKING ){
			fs_path iniPath = GetCommonIniPath().replace_filename(L"BonCtrl.ini");
			this->epgCapTimeOut = GetPrivateProfileInt(L"EPGCAP", L"EpgCapTimeOut", 10, iniPath.c_str());
			this->epgCapSaveTimeOut = GetPrivateProfileInt(L"EPGCAP", L"EpgCapSaveTimeOut", 0, iniPath.c_str()) != 0;
			//Common.iniは一般に外部プロセスが変更する可能性のある(はずの)ものなので、利用の直前にチェックする
			fs_path commonIniPath = GetCommonIniPath();
			this->epgCapBSBasic = GetPrivateProfileInt(L"SET", L"BSBasicOnly", 1, commonIniPath.c_str()) != 0;
			this->epgCapCS1Basic = GetPrivateProfileInt(L"SET", L"CS1BasicOnly", 1, commonIniPath.c_str()) != 0;
			this->epgCapCS2Basic = GetPrivateProfileInt(L"SET", L"CS2BasicOnly", 1, commonIniPath.c_str()) != 0;
			this->epgCapCS3Basic = GetPrivateProfileInt(L"SET", L"CS3BasicOnly", 0, commonIniPath.c_str()) != 0;
			this->epgCapChkBS = false;
			this->epgCapChkCS1 = false;
			this->epgCapChkCS2 = false;
			this->epgCapChkCS3 = false;
			this->epgCapChkNext = true;
		}

		if( this->epgCapChkNext == false ){
			DWORD elapsed;
			if( this->tsOut.IsChUnknown(&elapsed) ){
				if( this->epgCapSetChState != 0 || elapsed > 15000 ){
					//チャンネル切り替えがタイムアウトしたので無信号と判断
					this->tsOut.StopSaveEPG(false);
					this->epgCapChkNext = true;
				}
			}else{
				DWORD tick = GetU32Tick();
				if( this->epgCapSetChState == 0 ){
					//切り替え完了
					this->epgCapSetChState = 1;
					this->epgCapTick = tick;
				}
				if( tick - this->epgCapTick > this->epgCapTimeOut * 60 * 1000 ){
					//timeOut分以上かかっているなら停止
					this->tsOut.StopSaveEPG(this->epgCapSaveTimeOut);
					this->epgCapChkNext = true;
					AddDebugLogFormat(L"++%d分でEPG取得完了せず or Ch変更でエラー", this->epgCapTimeOut);
				}else if( tick - this->epgCapTick > 5000 ){
					SET_CH_INFO ch = this->epgCapChList[chkCount];
					bool basicOnly = ch.ONID == 4 && this->epgCapBSBasic ||
					                 ch.ONID == 6 && this->epgCapCS1Basic ||
					                 ch.ONID == 7 && this->epgCapCS2Basic ||
					                 ch.ONID == 10 && this->epgCapCS3Basic;
					//切り替え完了から5秒以上過ぎているので取得処理
					if( this->epgCapSetChState == 1 ){
						//取得開始
						wstring epgDataPath = L"";
						GetEpgDataFilePath(ch.ONID, basicOnly ? 0xFFFF : ch.TSID, epgDataPath);
						if( this->tsOut.StartSaveEPG(epgDataPath) ){
							this->epgCapSetChState = 2;
							this->epgCapLastChkTick = tick;
							if( ch.ONID == 4 ){
								this->epgCapChkBS = true;
							}else if( ch.ONID == 6 ){
								this->epgCapChkCS1 = true;
							}else if( ch.ONID == 7 ){
								this->epgCapChkCS2 = true;
							}else if( ch.ONID == 10 ){
								this->epgCapChkCS3 = true;
							}
						}else{
							this->epgCapChkNext = true;
						}
					}else if( tick - this->epgCapLastChkTick > (DWORD)(this->epgCapSetChState == 2 ? 60 : 10) * 1000 ){
						//最低60秒は取得し、以後10秒ごとにチェック
						this->epgCapSetChState = 3;
						this->epgCapLastChkTick = tick;
						vector<SET_CH_INFO> chkList = this->chUtil.GetEpgCapServiceAll(ch.ONID, basicOnly ? -1 : ch.TSID);
						//epgCapChListのサービスはEPG取得対象でなかったとしてもチェックしなければならない
						if( std::find_if(chkList.begin(), chkList.end(), [=](const SET_CH_INFO& a) {
						        return a.ONID == ch.ONID && a.TSID == ch.TSID && a.SID == ch.SID; }) == chkList.end() ){
							chkList.push_back(ch);
						}
						//蓄積状態チェック
						for( vector<SET_CH_INFO>::iterator itr = chkList.begin(); itr != chkList.end(); itr++ ){
							bool leitFlag = this->chUtil.IsPartial(itr->ONID, itr->TSID, itr->SID);
							pair<EPG_SECTION_STATUS, bool> status = this->tsOut.GetSectionStatusService(itr->ONID, itr->TSID, itr->SID, leitFlag);
							if( status.second == false ){
								status.first = this->tsOut.GetSectionStatus(leitFlag);
							}
							if( status.first != EpgNoData ){
								this->epgCapChkNext = true;
								if( status.first != EpgHEITAll &&
								    status.first != EpgLEITAll &&
								    (status.first != EpgBasicAll || basicOnly == false) ){
									this->epgCapChkNext = false;
									break;
								}
							}
						}
						WORD onid;
						WORD tsid;
						if( this->tsOut.GetStreamID(&onid, &tsid) == false || onid != ch.ONID || tsid != ch.TSID ){
							//チャンネルが変化したので停止
							this->tsOut.StopSaveEPG(false);
							this->epgCapChkNext = true;
						}else if( this->epgCapChkNext ){
							this->tsOut.StopSaveEPG(true);
						}
					}
				}
			}
		}

		if( this->epgCapChkNext ){
			//次のチャンネルへ
			for(;;){
				chkCount++;
				if( this->epgCapChList.size() <= (size_t)chkCount ){
					//全部チェック終わったので終了
					this->epgCapIndexOrStatus = ST_COMPLETE;
					return;
				}
				if( this->epgCapChList[chkCount].ONID == 4 && this->epgCapBSBasic && this->epgCapChkBS ||
				    this->epgCapChList[chkCount].ONID == 6 && this->epgCapCS1Basic && this->epgCapChkCS1 ||
				    this->epgCapChList[chkCount].ONID == 7 && this->epgCapCS2Basic && this->epgCapChkCS2 ||
				    this->epgCapChList[chkCount].ONID == 10 && this->epgCapCS3Basic && this->epgCapChkCS3 ){
					continue;
				}
				break;
			}
			this->epgCapIndexOrStatus = chkCount;
			DWORD space;
			DWORD ch;
			if( this->chUtil.GetCh(this->epgCapChList[chkCount].ONID, this->epgCapChList[chkCount].TSID,
			                       this->epgCapChList[chkCount].SID, space, ch) &&
			    this->ProcessSetCh(space, ch, this->epgCapChList[chkCount].ONID) ){
				this->epgCapSetChState = 0;
				this->epgCapChkNext = false;
			}
		}
	}
}

void CBonCtrl::GetEpgDataFilePath(WORD ONID, WORD TSID, wstring& epgDataFilePath)
{
	Format(epgDataFilePath, L"%04X%04X_epg.dat", ONID, TSID);
	epgDataFilePath = GetSettingPath().append(EPG_SAVE_FOLDER).append(epgDataFilePath).native();
}

//バックグラウンドでのEPG取得設定
//引数：
// enableLive	[IN]視聴中に取得する
// enableRec	[IN]録画中に取得する
// *Basic		[IN]１チャンネルから基本情報のみ取得するかどうか
// backStartWaitSec	[IN]Ch切り替え、録画開始後、バックグラウンドでのEPG取得を開始するまでの秒数
void CBonCtrl::SetBackGroundEpgCap(
	bool enableLive,
	bool enableRec,
	bool BSBasic,
	bool CS1Basic,
	bool CS2Basic,
	bool CS3Basic,
	DWORD backStartWaitSec
	)
{
	this->enableLiveEpgCap = enableLive;
	this->enableRecEpgCap = enableRec;
	this->epgCapBackBSBasic = BSBasic;
	this->epgCapBackCS1Basic = CS1Basic;
	this->epgCapBackCS2Basic = CS2Basic;
	this->epgCapBackCS3Basic = CS3Basic;
	this->epgCapBackStartWaitSec = backStartWaitSec;

	if( this->epgCapBackIndexOrStatus >= ST_WORKING ){
		StartBackgroundEpgCap();
	}
}

void CBonCtrl::StartBackgroundEpgCap()
{
	StopBackgroundEpgCap();
	if( this->chScanIndexOrStatus < ST_WORKING &&
	    this->epgCapIndexOrStatus < ST_WORKING ){
		if( this->bonUtil.IsOpen() ){
			this->epgCapTick = GetU32Tick();
			this->epgCapBackIndexOrStatus = ST_WORKING;
		}
	}
}

void CBonCtrl::StopBackgroundEpgCap()
{
	if( this->epgCapBackIndexOrStatus >= 0 ){
		this->tsOut.StopSaveEPG(false);
	}
	this->epgCapBackIndexOrStatus = ST_STOP;
}

void CBonCtrl::CheckEpgCapBack()
{
	DWORD tick = GetU32Tick();
	if( this->epgCapBackIndexOrStatus == ST_WORKING ){
		//取得待機中
		if( tick - this->epgCapTick > this->epgCapBackStartWaitSec * 1000 ){
			WORD onid;
			WORD tsid;
			if( (this->tsOut.IsRec() ? this->enableRecEpgCap : this->enableLiveEpgCap) && this->tsOut.GetStreamID(&onid, &tsid) ){
				bool basicOnly = onid == 4 && this->epgCapBackBSBasic ||
				                 onid == 6 && this->epgCapBackCS1Basic ||
				                 onid == 7 && this->epgCapBackCS2Basic ||
				                 onid == 10 && this->epgCapBackCS3Basic;
				this->epgCapChList = this->chUtil.GetEpgCapServiceAll(onid, basicOnly ? -1 : tsid);
				vector<SET_CH_INFO>::const_iterator itr = std::find_if(this->epgCapChList.begin(), this->epgCapChList.end(),
				                                                       [=](const SET_CH_INFO& a) { return a.TSID == tsid; });
				if( itr != this->epgCapChList.end() ){
					//取得対象チャンネルなので取得開始
					wstring epgDataPath;
					GetEpgDataFilePath(onid, basicOnly ? 0xFFFF : tsid, epgDataPath);
					if( this->tsOut.StartSaveEPG(epgDataPath) ){
						fs_path iniPath = GetCommonIniPath().replace_filename(L"BonCtrl.ini");
						this->epgCapTimeOut = GetPrivateProfileInt(L"EPGCAP", L"EpgCapTimeOut", 10, iniPath.c_str());
						this->epgCapSaveTimeOut = GetPrivateProfileInt(L"EPGCAP", L"EpgCapSaveTimeOut", 0, iniPath.c_str()) != 0;
						this->epgCapSetChState = 2;
						this->epgCapTick = tick;
						this->epgCapLastChkTick = tick;
						this->epgCapBackIndexOrStatus = (int)(itr - this->epgCapChList.begin());
					}
				}
			}
			if( this->epgCapBackIndexOrStatus == ST_WORKING ){
				this->epgCapChList.clear();
				this->epgCapBackIndexOrStatus = ST_STOP;
			}
		}
		return;
	}else if( this->epgCapBackIndexOrStatus < 0 ){
		return;
	}

	//取得中。最低60秒は取得し、以後10秒ごとにチェック
	if( tick - this->epgCapLastChkTick > (DWORD)(this->epgCapSetChState == 2 ? 60 : 10) * 1000 ){
		this->epgCapSetChState = 3;
		this->epgCapLastChkTick = tick;
		bool basicOnly = this->epgCapChList[0].ONID == 4 && this->epgCapBackBSBasic ||
		                 this->epgCapChList[0].ONID == 6 && this->epgCapBackCS1Basic ||
		                 this->epgCapChList[0].ONID == 7 && this->epgCapBackCS2Basic ||
		                 this->epgCapChList[0].ONID == 10 && this->epgCapBackCS3Basic;
		//蓄積状態チェック
		bool chkNext = false;
		for( vector<SET_CH_INFO>::const_iterator itr = this->epgCapChList.begin(); itr != this->epgCapChList.end(); itr++ ){
			bool leitFlag = this->chUtil.IsPartial(itr->ONID, itr->TSID, itr->SID);
			pair<EPG_SECTION_STATUS, bool> status = this->tsOut.GetSectionStatusService(itr->ONID, itr->TSID, itr->SID, leitFlag);
			if( status.second == false ){
				status.first = this->tsOut.GetSectionStatus(leitFlag);
			}
			if( status.first != EpgNoData ){
				chkNext = true;
				if( status.first != EpgHEITAll &&
				    status.first != EpgLEITAll &&
				    (status.first != EpgBasicAll || basicOnly == false) ){
					chkNext = false;
					break;
				}
			}
		}

		WORD onid;
		WORD tsid;
		if( this->tsOut.GetStreamID(&onid, &tsid) == false ||
		    onid != this->epgCapChList[this->epgCapBackIndexOrStatus].ONID ||
		    tsid != this->epgCapChList[this->epgCapBackIndexOrStatus].TSID ){
			//チャンネルが変化したので停止
			this->tsOut.StopSaveEPG(false);
			this->epgCapBackIndexOrStatus = ST_STOP;
		}else if( chkNext ){
			this->tsOut.StopSaveEPG(true);
			CSendCtrlCmd cmd;
			cmd.SetConnectTimeOut(1000);
			cmd.SendReloadEpg();
			this->epgCapBackIndexOrStatus = ST_STOP;
		}else{
			if( tick - this->epgCapTick > this->epgCapTimeOut * 60 * 1000 ){
				//timeOut分以上かかっているなら停止
				this->tsOut.StopSaveEPG(this->epgCapSaveTimeOut);
				CSendCtrlCmd cmd;
				cmd.SetConnectTimeOut(1000);
				cmd.SendReloadEpg();
				AddDebugLogFormat(L"++%d分でEPG取得完了せず or Ch変更でエラー", this->epgCapTimeOut);
				this->epgCapBackIndexOrStatus = ST_STOP;
			}
		}
	}
}

void CBonCtrl::CheckLogo()
{
	CTSOut::CHECK_LOGO_RESULT result;
	this->tsOut.CheckLogo(this->tsOutLogoTypeFlags, result);

	if( result.dataUpdated ){
		//ロゴを保存
		WCHAR name[64];
		swprintf_s(name, L"%04x_%03x_000_%02x.png", result.onid, result.id, result.type);
		fs_path path = GetSettingPath().append(LOGO_SAVE_FOLDER).append(name);
		bool update = true;
		if( UtilFileExists(path).first ){
			update = false;
			std::unique_ptr<FILE, fclose_deleter> logoFile(UtilOpenFile(path, UTIL_SECURE_READ));
			if( logoFile ){
				//小さいか中身が違っていれば更新
				for( size_t i = 0; i < result.data.size(); i++ ){
					int c = fgetc(logoFile.get());
					if( c == EOF || c != result.data[i] ){
						update = true;
						break;
					}
				}
			}
		}
		if( update ){
			UtilCreateDirectory(path.parent_path());
			std::unique_ptr<FILE, fclose_deleter> logoFile(UtilOpenFile(path, UTIL_SECURE_WRITE));
			if( logoFile ){
				fwrite(result.data.data(), 1, result.data.size(), logoFile.get());
			}
		}
	}

	if( result.serviceListUpdated ){
		//サービスからロゴへのポインティングを保存
		fs_path iniPath = GetSettingPath().append(LOGO_SAVE_FOLDER L".ini");
		for( size_t i = 0; i < result.serviceList.size(); i++ ){
			WCHAR name[16];
			swprintf_s(name, L"%04X%04X", result.onid, result.serviceList[i]);
			if( GetPrivateProfileInt(L"LogoIDMap", name, 0xFFFF, iniPath.c_str()) != result.id ){
				WritePrivateProfileInt(L"LogoIDMap", name, result.id, iniPath.c_str());
			}
		}
	}
}

//現在のストリームの表示用のステータスを取得する
//引数：
// signalLv		[OUT]シグナルレベル
// space		[OUT]物理CHのspace(不明のとき負)
// ch			[OUT]物理CHのch(不明のとき負)
// drop			[OUT]ドロップ数
// scramble		[OUT]スクランブル数
void CBonCtrl::GetViewStatusInfo(
	float* signalLv,
	int* space,
	int* ch,
	ULONGLONG* drop,
	ULONGLONG* scramble
	)
{
	this->tsOut.GetErrCount(this->nwCtrlID, drop, scramble);

	lock_recursive_mutex lock(this->buffLock);
	*signalLv = this->statusSignalLv;
	*space = this->viewSpace;
	*ch = this->viewCh;
}
