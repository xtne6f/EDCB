#include "stdafx.h"
#include "ReserveManager.h"
#include "../../Common/PathUtil.h"
#include "../../Common/TimeUtil.h"

CReserveManager::CReserveManager(CNotifyManager& notifyManager_, CEpgDBManager& epgDBManager_)
	: notifyManager(notifyManager_)
	, epgDBManager(epgDBManager_)
	, batManager(notifyManager_, L"EpgTimer_Bon_RecEnd")
	, batPostManager(notifyManager_, L"EpgTimer_Bon_Post")
	, checkCount(0)
	, epgCapRequested(false)
	, epgCapWork(false)
	, shutdownModePending(-1)
	, reserveModified(false)
{
}

void CReserveManager::Initialize(const CEpgTimerSrvSetting::SETTING& s)
{
	fs_path settingPath = GetSettingPath();
	fs_path iniPath = GetModuleIniPath();

	//チューナ一覧を構築
	vector<pair<wstring, wstring>> nameList = CEpgTimerSrvSetting::EnumBonFileName(settingPath.c_str());
	for( size_t i = 0; i < nameList.size(); i++ ){
		WORD count = (WORD)GetPrivateProfileInt(nameList[i].first.c_str(), L"Count", 0, iniPath.c_str());
		WORD priority = (WORD)GetPrivateProfileInt(nameList[i].first.c_str(), L"Priority", 0, iniPath.c_str());
		if( count != 0 && priority != 0xFFFF ){
			//カウント1以上のものだけ利用
			WORD epgCount = 0;
			if( GetPrivateProfileInt(nameList[i].first.c_str(), L"GetEpg", 1, iniPath.c_str()) != 0 ){
				epgCount = (WORD)GetPrivateProfileInt(nameList[i].first.c_str(), L"EPGCount", 0, iniPath.c_str());
				if( epgCount == 0 ){
					epgCount = count;
				}
			}
			DWORD tunerID = (DWORD)priority << 16 | 1;
			if( this->tunerBankMap.count(tunerID) != 0 ){
				AddDebugLog(L"CReserveManager::Initialize(): Duplicate bonID");
			}else{
				CParseChText4 chText4;
				chText4.ParseText(fs_path(settingPath).append(nameList[i].second).c_str());
				for( WORD j = 0; j < count; j++, tunerID++ ){
					this->tunerBankMap.insert(std::make_pair(tunerID, std::unique_ptr<CTunerBankCtrl>(new CTunerBankCtrl(
						tunerID, nameList[i].first.c_str(), min(epgCount, count), chText4.GetMap(), this->notifyManager, this->epgDBManager))));
				}
			}
		}
	}

	this->lastCheckEpgCap = GetNowI64Time();
	this->reserveText.ParseText(fs_path(settingPath).append(RESERVE_TEXT_NAME).c_str());

	ReloadSetting(s);
	this->recInfoText.ParseText(fs_path(settingPath).append(REC_INFO_TEXT_NAME).c_str());
	this->recInfo2Text.ParseText(fs_path(settingPath).append(REC_INFO2_TEXT_NAME).c_str());

	this->watchdogThread = thread_(WatchdogThread, this);
}

void CReserveManager::Finalize()
{
	//カスタムハンドラを止めるため
	this->batManager.Finalize();
	this->batPostManager.Finalize();
	this->epgDBManager.CancelLoadData();
	if( this->watchdogThread.joinable() ){
		this->watchdogStopEvent.Set();
		this->watchdogThread.join();
	}
	this->tunerBankMap.clear();
}

void CReserveManager::ReloadSetting(const CEpgTimerSrvSetting::SETTING& s)
{
	lock_recursive_mutex lock(this->managerLock);

	fs_path commonIniPath = GetCommonIniPath();

	this->chUtil.ParseText(GetSettingPath().append(L"ChSet5.txt").c_str());

	//チューナがあるものだけ残す
	for( map<LONGLONG, CH_DATA5>::const_iterator itrCh = this->chUtil.GetMap().begin(); itrCh != this->chUtil.GetMap().end(); ){
		auto itr = this->tunerBankMap.cbegin();
		const CH_DATA4* chData = NULL;
		for( ; itr != this->tunerBankMap.end(); itr++ ){
			chData = itr->second->GetCh(itrCh->second.originalNetworkID, itrCh->second.transportStreamID, itrCh->second.serviceID);
			if( chData ){
				break;
			}
		}
		if( itr == this->tunerBankMap.end() ){
			LONGLONG key = itrCh->first;
			this->chUtil.DelCh(key);
			itrCh = this->chUtil.GetMap().lower_bound(key);
		}else{
			this->chUtil.SetRemoconID(chData->originalNetworkID, chData->transportStreamID, chData->serviceID, chData->remoconID);
			itrCh++;
		}
	}

	this->setting = s;

	this->recInfoText.SetKeepCount(s.autoDelRecInfo ? s.autoDelRecInfoNum : UINT_MAX);
	this->recInfoText.SetRecInfoDelFile(GetPrivateProfileInt(L"SET", L"RecInfoDelFile", 0, commonIniPath.c_str()) != 0);
	this->recInfoText.SetRecInfoFolder(GetPrivateProfileToString(L"SET", L"RecInfoFolder", L"", commonIniPath.c_str()).c_str());
	this->recInfoText.CustomizeDelExt(s.applyExtToRecInfoDel);
	this->recInfoText.SetCustomDelExt(s.delExtList);

	this->recInfo2Text.SetKeepCount(s.recInfo2Max);

	for( auto itr = this->tunerBankMap.cbegin(); itr != this->tunerBankMap.end(); itr++ ){
		itr->second->ReloadSetting(s);
	}
	ReloadBankMap();
}

vector<RESERVE_DATA> CReserveManager::GetReserveDataAll(bool getRecFileName) const
{
	lock_recursive_mutex lock(this->managerLock);

	vector<RESERVE_DATA> list;
	list.reserve(this->reserveText.GetMap().size());
	CReNamePlugInUtil utilCache;
	for( map<DWORD, RESERVE_DATA>::const_iterator itr = this->reserveText.GetMap().begin(); itr != this->reserveText.GetMap().end(); itr++ ){
		list.resize(list.size() + 1);
		GetReserveData(itr->first, &list.back(), getRecFileName, &utilCache);
	}
	return list;
}

vector<TUNER_RESERVE_INFO> CReserveManager::GetTunerReserveAll() const
{
	lock_recursive_mutex lock(this->managerLock);

	vector<TUNER_RESERVE_INFO> list;
	list.reserve(this->tunerBankMap.size() + 1);
	for( auto itr = this->tunerBankMap.cbegin(); itr != this->tunerBankMap.end(); itr++ ){
		list.resize(list.size() + 1);
		list.back().tunerID = itr->first;
		list.back().tunerName = itr->second->GetBonFileName();
		list.back().reserveList = itr->second->GetReserveIDList();
	}
	list.resize(list.size() + 1);
	list.back().tunerID = 0xFFFFFFFF;
	list.back().tunerName = L"チューナー不足";
	vector<DWORD> &ngList = list.back().reserveList = GetNoTunerReserveAll();
	for( size_t i = 0; i < ngList.size(); ){
		//無効予約は「チューナ不足」ではない
		if( this->reserveText.GetMap().find(ngList[i])->second.recSetting.IsNoRec() ){
			ngList.erase(ngList.begin() + i);
		}else{
			i++;
		}
	}
	return list;
}

vector<TUNER_PROCESS_STATUS_INFO> CReserveManager::GetTunerProcessStatusAll() const
{
	lock_recursive_mutex lock(this->managerLock);

	vector<TUNER_PROCESS_STATUS_INFO> list;
	for( auto itr = this->tunerBankMap.cbegin(); itr != this->tunerBankMap.end(); itr++ ){
		if( itr->second->GetState() != CTunerBankCtrl::TR_IDLE ){
			list.push_back(itr->second->GetProcessStatusInfo());
		}
	}
	return list;
}

vector<DWORD> CReserveManager::GetNoTunerReserveAll() const
{
	lock_recursive_mutex lock(this->managerLock);

	vector<DWORD> list;
	list.reserve(this->reserveText.GetMap().size());
	for( map<DWORD, RESERVE_DATA>::const_iterator itr = this->reserveText.GetMap().begin(); itr != this->reserveText.GetMap().end(); itr++ ){
		list.push_back(itr->first);
	}
	//全予約からバンクに存在する予約を引く
	for( auto itr = this->tunerBankMap.cbegin(); itr != this->tunerBankMap.end(); itr++ ){
		vector<DWORD> diffList = itr->second->GetReserveIDList();
		size_t k = 0;
		for( size_t i = 0, j = 0; i < list.size(); ){
			if( j >= diffList.size() || list[i] < diffList[j] ){
				list[k++] = list[i++];
			}else if( diffList[j] < list[i] ){
				j++;
			}else{
				i++;
			}
		}
		list.resize(k);
	}
	return list;
}

bool CReserveManager::GetReserveData(DWORD id, RESERVE_DATA* reserveData, bool getRecFileName, CReNamePlugInUtil* util) const
{
	lock_recursive_mutex lock(this->managerLock);

	map<DWORD, RESERVE_DATA>::const_iterator itr = this->reserveText.GetMap().find(id);
	if( itr != this->reserveText.GetMap().end() ){
		*reserveData = itr->second;
		reserveData->recFileNameList.clear();
		if( getRecFileName && reserveData->recSetting.GetRecMode() != RECMODE_VIEW ){
			CReNamePlugInUtil utilCache;
			if( util == NULL ){
				util = &utilCache;
			}
			RESERVE_DATA& r = *reserveData;
			//recNamePlugInを展開して実ファイル名をセット
			for( size_t i = 0; i <= r.recSetting.recFolderList.size(); i++ ){
				if( i < r.recSetting.recFolderList.size() || r.recSetting.recFolderList.empty() ){
					LPCWSTR recNamePlugIn = this->setting.recNamePlugIn ? this->setting.recNamePlugInFile.c_str() : L"";
					if( i < r.recSetting.recFolderList.size() && r.recSetting.recFolderList[i].recNamePlugIn.empty() == false ){
						recNamePlugIn = r.recSetting.recFolderList[i].recNamePlugIn.c_str();
					}
					r.recFileNameList.push_back(CTunerBankCtrl::ConvertRecName(
						recNamePlugIn, r.startTime, r.durationSecond, r.title.c_str(), r.originalNetworkID, r.transportStreamID, r.serviceID, r.eventID,
						r.stationName.c_str(), L"チューナー不明", 0xFFFFFFFF, r.reserveID, this->epgDBManager,
						r.startTime, 0, this->setting.tsExt.c_str(), this->setting.noChkYen, *util));
				}
			}
		}
		return true;
	}
	return false;
}

bool CReserveManager::AddReserveData(const vector<RESERVE_DATA>& reserveList, bool setReserveStatus)
{
	lock_recursive_mutex lock(this->managerLock);

	bool modified = false;
	LONGLONG minStartTime = LLONG_MAX;
	LONGLONG now = GetNowI64Time();
	vector<CBatManager::BAT_WORK_INFO> batWorkList;
	for( size_t i = 0; i < reserveList.size(); i++ ){
		RESERVE_DATA r = reserveList[i];
		//すでに終了していないか
		if( now < ConvertI64Time(r.startTime) + r.durationSecond * I64_1SEC ){
			if( r.recSetting.IsNoRec() && this->setting.fixNoRecToServiceOnly ){
				r.recSetting.recMode = REC_SETTING_DATA::DIV_RECMODE;
			}
			r.presentFlag = FALSE;
			r.overlapMode = RESERVE_EXECUTE;
			if( setReserveStatus == false ){
				r.reserveStatus = ADD_RESERVE_NORMAL;
			}
			r.ngTunerIDList.clear();
			r.recFileNameList.clear();
			r.reserveID = this->reserveText.AddReserve(r);
			this->reserveModified = true;
			modified = true;
			if( r.recSetting.IsNoRec() == false ){
				LONGLONG startTime;
				CalcEntireReserveTime(&startTime, NULL, r);
				minStartTime = min(startTime, minStartTime);
				batWorkList.resize(batWorkList.size() + 1);
				AddReserveDataMacro(batWorkList.back().macroList, r, "");
			}
		}
	}
	if( modified ){
		this->reserveText.SaveText();
		ReloadBankMap(minStartTime);
		CheckAutoDel();
		AddNotifyAndPostBat(NOTIFY_UPDATE_RESERVE_INFO);
		AddPostBatWork(batWorkList, L"PostAddReserve");
		return true;
	}
	return false;
}

bool CReserveManager::ChgReserveData(const vector<RESERVE_DATA>& reserveList, bool setReserveStatus)
{
	lock_recursive_mutex lock(this->managerLock);

	bool modified = false;
	LONGLONG minStartTime = LLONG_MAX;
	vector<CBatManager::BAT_WORK_INFO> batWorkList;
	for( size_t i = 0; i < reserveList.size(); i++ ){
		RESERVE_DATA r = reserveList[i];
		map<DWORD, RESERVE_DATA>::const_iterator itr = this->reserveText.GetMap().find(r.reserveID);
		if( itr != this->reserveText.GetMap().end() ){
			if( r.recSetting.IsNoRec() && this->setting.fixNoRecToServiceOnly ){
				r.recSetting.recMode = REC_SETTING_DATA::DIV_RECMODE;
			}
			//変更できないフィールドを上書き
			r.presentFlag = itr->second.presentFlag;
			r.startTimeEpg = itr->second.startTimeEpg;
			if( setReserveStatus == false ){
				r.reserveStatus = itr->second.reserveStatus;
			}
			r.ngTunerIDList = itr->second.ngTunerIDList;
			r.recFileNameList.clear();

			if( r.recSetting.IsNoRec() ){
				if( itr->second.recSetting.IsNoRec() == false ){
					//バンクから削除
					for( auto jtr = this->tunerBankMap.cbegin(); jtr != this->tunerBankMap.end(); jtr++ ){
						if( jtr->second->DelReserve(r.reserveID) ){
							break;
						}
					}
					r.overlapMode = RESERVE_EXECUTE;
					LONGLONG startTime;
					CalcEntireReserveTime(&startTime, NULL, itr->second);
					minStartTime = min(startTime, minStartTime);
				}
			}else{
				//バンクに渡す予約情報を作成
				CTunerBankCtrl::TUNER_RESERVE tr;
				tr.reserveID = r.reserveID;
				tr.title = r.title;
				tr.stationName = r.stationName;
				tr.onid = r.originalNetworkID;
				tr.tsid = r.transportStreamID;
				tr.sid = r.serviceID;
				tr.eid = r.eventID;
				tr.recMode = r.recSetting.GetRecMode();
				tr.priority = r.recSetting.priority;
				bool enableCaption = tr.enableCaption =
					r.recSetting.serviceMode & RECSERVICEMODE_SET ? (r.recSetting.serviceMode & RECSERVICEMODE_CAP) != 0 : this->setting.enableCaption;
				bool enableData = tr.enableData =
					r.recSetting.serviceMode & RECSERVICEMODE_SET ? (r.recSetting.serviceMode & RECSERVICEMODE_DATA) != 0 : this->setting.enableData;
				tr.pittari = r.recSetting.pittariFlag != 0;
				tr.partialRecMode = r.recSetting.partialRecFlag;
				tr.continueRecFlag = r.recSetting.continueRecFlag != 0;
				LONGLONG startTime, endTime;
				CalcEntireReserveTime(&startTime, &endTime, r);
				tr.startTime = ConvertI64Time(r.startTime);
				tr.durationSecond = r.durationSecond;
				LONGLONG startMargin = tr.startMargin = tr.startTime - startTime;
				LONGLONG endMargin = tr.endMargin = endTime - (tr.startTime + tr.durationSecond * I64_1SEC);
				tr.recFolder = r.recSetting.recFolderList;
				tr.partialRecFolder = r.recSetting.partialRecFolder;

				bool bankDeleted = false;
				auto jtr = this->tunerBankMap.cbegin();
				for( ; jtr != this->tunerBankMap.end(); jtr++ ){
					if( jtr->second->ChgCtrlReserve(&tr) ){
						//この予約はこのバンクに待機状態で存在する
						if( tr.onid != r.originalNetworkID ||
						    tr.tsid != r.transportStreamID ||
						    tr.sid != r.serviceID ||
						    tr.eid != r.eventID ||
						    tr.startTime != ConvertI64Time(r.startTime) ||
						    tr.durationSecond != r.durationSecond ||
						    tr.startMargin != startMargin ||
						    tr.endMargin != endMargin ){
							//必ず変更すべきフィールドを変更できなかったので待機状態を解除する
							jtr->second->DelReserve(r.reserveID);
							bankDeleted = true;
						}else{
							//必ずしも変更する必要のないフィールドは妥協する
							r.title = tr.title;
							r.stationName = tr.stationName;
							r.recSetting.recMode = r.recSetting.recMode / REC_SETTING_DATA::DIV_RECMODE * REC_SETTING_DATA::DIV_RECMODE + tr.recMode;
							r.recSetting.priority = tr.priority;
							if( tr.enableCaption != enableCaption || tr.enableData != enableData ){
								r.recSetting.serviceMode = 0;
								r.recSetting.serviceMode |= tr.enableCaption ? RECSERVICEMODE_SET | RECSERVICEMODE_CAP : 0;
								r.recSetting.serviceMode |= tr.enableData ? RECSERVICEMODE_SET | RECSERVICEMODE_DATA : 0;
							}
							r.recSetting.pittariFlag = tr.pittari;
							r.recSetting.partialRecFlag = tr.partialRecMode;
							r.recSetting.continueRecFlag = tr.continueRecFlag;
							r.recSetting.recFolderList = tr.recFolder;
							r.recSetting.partialRecFolder = tr.partialRecFolder;
							r.recSetting.tunerID = itr->second.recSetting.tunerID;
						}
						break;
					}
				}
				if( jtr == this->tunerBankMap.end() ){
					//この予約は待機状態ではないので単純に削除と追加で更新できる
					for( jtr = this->tunerBankMap.begin(); jtr != this->tunerBankMap.end(); jtr++ ){
						if( jtr->second->DelReserve(tr.reserveID) ){
							jtr->second->AddReserve(tr);
							break;
						}
					}
				}

				//これらのフィールドに変化がなければバンク配置を再構築する必要はない
				if( bankDeleted ||
				    r.originalNetworkID != itr->second.originalNetworkID ||
				    r.transportStreamID != itr->second.transportStreamID ||
				    r.serviceID != itr->second.serviceID ||
				    ConvertI64Time(r.startTime) != ConvertI64Time(itr->second.startTime) ||
				    r.durationSecond != itr->second.durationSecond ||
				    r.recSetting.recMode != itr->second.recSetting.recMode ||
				    r.recSetting.priority != itr->second.recSetting.priority ||
				    r.recSetting.useMargineFlag != itr->second.recSetting.useMargineFlag ||
				    r.recSetting.useMargineFlag && (
				        r.recSetting.startMargine != itr->second.recSetting.startMargine ||
				        r.recSetting.endMargine != itr->second.recSetting.endMargine) ||
				    r.recSetting.tunerID != itr->second.recSetting.tunerID ){
					LONGLONG startTimeNext;
					CalcEntireReserveTime(&startTime, NULL, itr->second);
					CalcEntireReserveTime(&startTimeNext, NULL, r);
					minStartTime = min(min(startTime, startTimeNext), minStartTime);
				}
				batWorkList.resize(batWorkList.size() + 1);
				AddReserveDataMacro(batWorkList.back().macroList, itr->second, "OLD");
				AddReserveDataMacro(batWorkList.back().macroList, r, "");
			}
			this->reserveText.ChgReserve(r);
			this->reserveModified = true;
			modified = true;
		}
	}
	if( modified ){
		this->reserveText.SaveText();
		ReloadBankMap(minStartTime);
		CheckAutoDel();
		AddNotifyAndPostBat(NOTIFY_UPDATE_RESERVE_INFO);
		AddPostBatWork(batWorkList, L"PostChgReserve");
		return true;
	}
	return false;
}

void CReserveManager::DelReserveData(const vector<DWORD>& idList)
{
	lock_recursive_mutex lock(this->managerLock);

	vector<CTunerBankCtrl::CHECK_RESULT> retList;
	LONGLONG minStartTime = LLONG_MAX;
	for( size_t i = 0; i < idList.size(); i++ ){
		map<DWORD, RESERVE_DATA>::const_iterator itr = this->reserveText.GetMap().find(idList[i]);
		if( itr != this->reserveText.GetMap().end() ){
			if( itr->second.recSetting.IsNoRec() == false ){
				//バンクから削除
				for( auto jtr = this->tunerBankMap.cbegin(); jtr != this->tunerBankMap.end(); jtr++ ){
					if( jtr->second->DelReserve(idList[i], this->setting.delReserveMode == 0 ? NULL : &retList) ){
						break;
					}
				}
				LONGLONG startTime;
				CalcEntireReserveTime(&startTime, NULL, itr->second);
				minStartTime = min(startTime, minStartTime);
			}
		}
	}
	for( auto itrRet = retList.begin(); itrRet != retList.end(); itrRet++ ){
		//正常終了をキャンセル中断に差し替える
		if( this->setting.delReserveMode == 2 && itrRet->type == CTunerBankCtrl::CHECK_END ){
			itrRet->type = CTunerBankCtrl::CHECK_END_CANCEL;
		}
	}
	ProcessRecEnd(retList);
	bool modified = false;
	for( size_t i = 0; i < idList.size(); i++ ){
		if( this->reserveText.DelReserve(idList[i]) ){
			this->reserveModified = true;
			modified = true;
		}
	}
	ReloadBankMap(minStartTime);
	if( modified ){
		this->reserveText.SaveText();
		AddNotifyAndPostBat(NOTIFY_UPDATE_RESERVE_INFO);
	}
}

vector<REC_FILE_INFO> CReserveManager::GetRecFileInfoAll(bool getExtraInfo) const
{
	vector<REC_FILE_INFO> infoList;
	wstring folder;
	bool folderOnly;
	{
		lock_recursive_mutex lock(this->managerLock);
		infoList.reserve(this->recInfoText.GetMap().size());
		for( map<DWORD, REC_FILE_INFO>::const_iterator itr = this->recInfoText.GetMap().begin(); itr != this->recInfoText.GetMap().end(); itr++ ){
			infoList.push_back(itr->second);
		}
		if( getExtraInfo ){
			folder = this->recInfoText.GetRecInfoFolder();
		}
		folderOnly = this->setting.recInfoFolderOnly;
	}
	if( getExtraInfo ){
		for( size_t i = 0; i < infoList.size(); i++ ){
			infoList[i].programInfo = CParseRecInfoText::GetExtraInfo(infoList[i].recFilePath.c_str(), L".program.txt", folder, folderOnly);
			infoList[i].errInfo = CParseRecInfoText::GetExtraInfo(infoList[i].recFilePath.c_str(), L".err", folder, folderOnly);
		}
	}
	return infoList;
}

bool CReserveManager::GetRecFileInfo(DWORD id, REC_FILE_INFO* recInfo, bool getExtraInfo) const
{
	wstring folder;
	bool folderOnly;
	{
		lock_recursive_mutex lock(this->managerLock);
		map<DWORD, REC_FILE_INFO>::const_iterator itr = this->recInfoText.GetMap().find(id);
		if( itr == this->recInfoText.GetMap().end() ){
			return false;
		}
		*recInfo = itr->second;
		if( getExtraInfo ){
			folder = this->recInfoText.GetRecInfoFolder();
		}
		folderOnly = this->setting.recInfoFolderOnly;
	}
	if( getExtraInfo ){
		recInfo->programInfo = CParseRecInfoText::GetExtraInfo(recInfo->recFilePath.c_str(), L".program.txt", folder, folderOnly);
		recInfo->errInfo = CParseRecInfoText::GetExtraInfo(recInfo->recFilePath.c_str(), L".err", folder, folderOnly);
	}
	return true;
}

void CReserveManager::DelRecFileInfo(const vector<DWORD>& idList)
{
	lock_recursive_mutex lock(this->managerLock);

	for( size_t i = 0; i < idList.size(); i++ ){
		this->recInfoText.DelRecInfo(idList[i]);
	}
	this->recInfoText.SaveText();
	AddNotifyAndPostBat(NOTIFY_UPDATE_REC_INFO);
}

void CReserveManager::ChgPathRecFileInfo(const vector<REC_FILE_INFO>& infoList)
{
	lock_recursive_mutex lock(this->managerLock);

	for( size_t i = 0; i < infoList.size(); i++ ){
		this->recInfoText.ChgPathRecInfo(infoList[i].id, infoList[i].recFilePath.c_str());
	}
	this->recInfoText.SaveText();
	AddNotifyAndPostBat(NOTIFY_UPDATE_REC_INFO);
}

void CReserveManager::ChgProtectRecFileInfo(const vector<REC_FILE_INFO>& infoList)
{
	lock_recursive_mutex lock(this->managerLock);

	for( size_t i = 0; i < infoList.size(); i++ ){
		this->recInfoText.ChgProtectRecInfo(infoList[i].id, infoList[i].protectFlag);
	}
	this->recInfoText.SaveText();
	AddNotifyAndPostBat(NOTIFY_UPDATE_REC_INFO);
}

void CReserveManager::ReloadBankMap(LONGLONG reloadTime)
{
	lock_recursive_mutex lock(this->managerLock);

	if( reloadTime == LLONG_MAX ){
		return;
	}
	AddDebugLog(L"Start ReloadBankMap");
	DWORD tick = GetU32Tick();

	LONGLONG boundaryReloadTime = 0;

	//reloadTimeより前の予約を開始時間順にソート
	vector<pair<LONGLONG, const RESERVE_DATA*>> sortTimeMap;
	sortTimeMap.push_back(std::make_pair(reloadTime, (RESERVE_DATA*)NULL));
	for( map<DWORD, RESERVE_DATA>::const_iterator itr = this->reserveText.GetMap().begin(); itr != this->reserveText.GetMap().end(); itr++ ){
		if( itr->second.recSetting.IsNoRec() == false ){
			LONGLONG startTime;
			CalcEntireReserveTime(&startTime, NULL, itr->second);
			if( startTime < reloadTime ){
				sortTimeMap.push_back(std::make_pair(startTime, &itr->second));
			}
		}
	}
	std::sort(sortTimeMap.begin(), sortTimeMap.end());

	//READY_MARGIN秒以上の無予約時間帯を探す。無予約時間帯より後ろだけを再割り当てすればOK
	for( auto itrTime = sortTimeMap.crbegin(); itrTime != sortTimeMap.rend(); itrTime++ ){
		auto itrRes = itrTime;
		for( itrRes++; itrRes != sortTimeMap.rend(); itrRes++ ){
			LONGLONG endTime;
			CalcEntireReserveTime(NULL, &endTime, *itrRes->second);
			if( endTime + CTunerBankCtrl::READY_MARGIN * I64_1SEC > itrTime->first ){
				break;
			}
		}
		if( itrRes == sortTimeMap.rend() ){
			boundaryReloadTime = itrTime->first;
			break;
		}
	}

	//開始済み予約リストとバンク決定した予約リストのマップ
	vector<pair<DWORD, CHK_BANK_DATA>> bankResMap;
	for( auto itr = this->tunerBankMap.cbegin(); itr != this->tunerBankMap.end(); itr++ ){
		//待機状態に入っているもの以外クリア
		itr->second->ClearNoCtrl(boundaryReloadTime);
		bankResMap.push_back(std::make_pair(itr->first, CHK_BANK_DATA()));
		bankResMap.back().second.startedResList = itr->second->GetReserveIDList();
	}

	//boundaryReloadTimeより後の予約を開始時間順にソート
	sortTimeMap.clear();
	for( map<DWORD, RESERVE_DATA>::const_iterator itr = this->reserveText.GetMap().begin(); itr != this->reserveText.GetMap().end(); itr++ ){
		if( itr->second.recSetting.IsNoRec() == false ){
			LONGLONG startTime;
			CalcEntireReserveTime(&startTime, NULL, itr->second);
			if( startTime >= boundaryReloadTime ){
				this->reserveText.SetOverlapMode(itr->first, RESERVE_NO_EXECUTE);
				sortTimeMap.push_back(std::make_pair(startTime, &itr->second));
			}
		}
	}
	std::sort(sortTimeMap.begin(), sortTimeMap.end());

	//バンク未決の予約マップ
	vector<pair<LONGLONG, const RESERVE_DATA*>> sortResMap;

	//予約を無予約時間帯ごとに組分けしてバンク配置する(組ごとに独立して処理できるので速度や配置安定性が増す)
	for( auto itrTime = sortTimeMap.crbegin(); itrTime != sortTimeMap.rend(); ){
		auto itrRes = itrTime;
		for( itrRes++; itrRes != sortTimeMap.rend(); itrRes++ ){
			LONGLONG endTime;
			CalcEntireReserveTime(NULL, &endTime, *itrRes->second);
			if( endTime + CTunerBankCtrl::READY_MARGIN * I64_1SEC > itrTime->first ){
				break;
			}
		}
		itrTime++;
		if( itrRes == sortTimeMap.rend() ){
			sortResMap.clear();
			for( itrRes = sortTimeMap.rbegin(); itrRes != itrTime; itrRes++ ){
				//バンク決定順のキーはチューナ固定優先ビットつき実効優先度(予約優先度<<60|チューナ固定優先ビット<<59|開始順)
				LONGLONG startOrder = itrRes->first / I64_1SEC << 16 | (itrRes->second->reserveID & 0xFFFF);
				LONGLONG priority = (this->setting.backPriority ? itrRes->second->recSetting.priority : ~itrRes->second->recSetting.priority) & 7;
				LONGLONG fixedBit = (this->setting.fixedTunerPriority && itrRes->second->recSetting.tunerID != 0) ? this->setting.backPriority : !this->setting.backPriority;
				sortResMap.push_back(std::make_pair((this->setting.backPriority ? -1 : 1) * (priority << 60 | fixedBit << 59 | startOrder), itrRes->second));
			}
			std::sort(sortResMap.begin(), sortResMap.end());
			sortTimeMap.erase(itrTime.base(), sortTimeMap.end());
			itrTime = sortTimeMap.rbegin();

			for( vector<pair<DWORD, CHK_BANK_DATA>>::iterator itrBank = bankResMap.begin(); itrBank != bankResMap.end(); itrBank++ ){
				itrBank->second.assignedResList.clear();
				//開始済み予約はそのままバンク決定
				for( auto itr = sortResMap.cbegin(); itr != sortResMap.end(); ){
					if( std::find(itrBank->second.startedResList.begin(), itrBank->second.startedResList.end(), itr->second->reserveID) != itrBank->second.startedResList.end() ){
						CHK_RESERVE_DATA item;
						CalcEntireReserveTime(&item.cutStartTime, &item.cutEndTime, *itr->second);
						item.cutStartTime -= CTunerBankCtrl::READY_MARGIN * I64_1SEC;
						item.startOrder = llabs(itr->first) & 0x07FFFFFFFFFFFFFF;
						//チューナ固定優先ビットを除去
						item.effectivePriority = (itr->first < 0 ? -1 : 1) * (llabs(itr->first) & 0x77FFFFFFFFFFFFFF);
						item.started = true;
						item.r = itr->second;
						//開始済み予約はすべてバンク内で同一チャンネルなのでChkInsertStatus()は不要
						itrBank->second.assignedResList.push_back(item);
						itr = sortResMap.erase(itr);
					}else{
						itr++;
					}
				}
			}

			for( auto itr = sortResMap.cbegin(); itr != sortResMap.end(); ){
				CHK_RESERVE_DATA item;
				CalcEntireReserveTime(&item.cutStartTime, &item.cutEndTime, *itr->second);
				item.cutStartTime -= CTunerBankCtrl::READY_MARGIN * I64_1SEC;
				item.startOrder = llabs(itr->first) & 0x07FFFFFFFFFFFFFF;
				//チューナ固定優先ビットを除去
				item.effectivePriority = (itr->first < 0 ? -1 : 1) * (llabs(itr->first) & 0x77FFFFFFFFFFFFFF);
				item.started = false;
				item.r = itr->second;
				//NGチューナが追加されているときはチューナIDを固定しない
				if( itr->second->recSetting.tunerID != 0 && itr->second->ngTunerIDList.empty() ){
					//チューナID固定
					vector<pair<DWORD, CHK_BANK_DATA>>::iterator itrBank =
						lower_bound_first(bankResMap.begin(), bankResMap.end(), itr->second->recSetting.tunerID);
					if( itrBank != bankResMap.end() &&
					    itrBank->first == itr->second->recSetting.tunerID &&
					    this->tunerBankMap.find(itrBank->first)->second->GetCh(itr->second->originalNetworkID, itr->second->transportStreamID, itr->second->serviceID) ){
						CHK_RESERVE_DATA testItem = item;
						ChkInsertStatus(itrBank->second.assignedResList, testItem, false);
						if( testItem.cutEndTime - testItem.cutStartTime > CTunerBankCtrl::READY_MARGIN * I64_1SEC ){
							//録画時間がある
							ChkInsertStatus(itrBank->second.assignedResList, item, true);
							itrBank->second.assignedResList.push_back(item);
							itr = sortResMap.erase(itr);
							continue;
						}
					}
				}else{
					//もっとも良いと思われるバンクに割り当てる
					vector<pair<DWORD, CHK_BANK_DATA>>::iterator itrMin = bankResMap.end();
					LONGLONG costMin = LLONG_MAX;
					LONGLONG durationMin = 0;
					for( vector<pair<DWORD, CHK_BANK_DATA>>::iterator jtr = bankResMap.begin(); jtr != bankResMap.end(); jtr++ ){
						//NGチューナを除く
						if( std::find(itr->second->ngTunerIDList.begin(), itr->second->ngTunerIDList.end(), jtr->first) == itr->second->ngTunerIDList.end() &&
						    this->tunerBankMap.find(jtr->first)->second->GetCh(itr->second->originalNetworkID, itr->second->transportStreamID, itr->second->serviceID) ){
							CHK_RESERVE_DATA testItem = item;
							LONGLONG cost = ChkInsertStatus(jtr->second.assignedResList, testItem, false);
							if( cost < costMin ){
								itrMin = jtr;
								costMin = cost;
								durationMin = testItem.cutEndTime - testItem.cutStartTime;
							}
						}
					}
					if( itrMin != bankResMap.end() && durationMin > CTunerBankCtrl::READY_MARGIN * I64_1SEC ){
						//録画時間がある
						ChkInsertStatus(itrMin->second.assignedResList, item, true);
						itrMin->second.assignedResList.push_back(item);
						itr = sortResMap.erase(itr);
						continue;
					}
				}
				itr++;
			}

			//実際にバンクに追加する
			for( vector<pair<DWORD, CHK_BANK_DATA>>::const_iterator itr = bankResMap.begin(); itr != bankResMap.end(); itr++ ){
				for( size_t i = 0; i < itr->second.assignedResList.size(); i++ ){
					const RESERVE_DATA& r = *itr->second.assignedResList[i].r;
					LONGLONG startTime, endTime;
					CalcEntireReserveTime(&startTime, &endTime, r);
					//かぶり状態を記録する(参考程度の情報)
					this->reserveText.SetOverlapMode(r.reserveID,
						itr->second.assignedResList[i].cutStartTime == startTime - CTunerBankCtrl::READY_MARGIN * I64_1SEC &&
						itr->second.assignedResList[i].cutEndTime == endTime ? RESERVE_EXECUTE : RESERVE_PILED_UP);
					//バンクに渡す予約情報を作成
					CTunerBankCtrl::TUNER_RESERVE tr;
					tr.reserveID = r.reserveID;
					tr.title = r.title;
					tr.stationName = r.stationName;
					tr.onid = r.originalNetworkID;
					tr.tsid = r.transportStreamID;
					tr.sid = r.serviceID;
					tr.eid = r.eventID;
					tr.recMode = r.recSetting.GetRecMode();
					tr.priority = r.recSetting.priority;
					tr.enableCaption = r.recSetting.serviceMode & RECSERVICEMODE_SET ? (r.recSetting.serviceMode & RECSERVICEMODE_CAP) != 0 : this->setting.enableCaption;
					tr.enableData = r.recSetting.serviceMode & RECSERVICEMODE_SET ? (r.recSetting.serviceMode & RECSERVICEMODE_DATA) != 0 : this->setting.enableData;
					tr.pittari = r.recSetting.pittariFlag != 0;
					tr.partialRecMode = r.recSetting.partialRecFlag;
					tr.continueRecFlag = r.recSetting.continueRecFlag != 0;
					tr.startTime = ConvertI64Time(r.startTime);
					tr.durationSecond = r.durationSecond;
					tr.startMargin = tr.startTime - startTime;
					tr.endMargin = endTime - (tr.startTime + tr.durationSecond * I64_1SEC);
					tr.recFolder = r.recSetting.recFolderList;
					tr.partialRecFolder = r.recSetting.partialRecFolder;
					this->tunerBankMap.find(itr->first)->second->AddReserve(tr);
				}
			}
		}
	}

	AddDebugLogFormat(L"End ReloadBankMap %dmsec", GetU32Tick() - tick);
}

LONGLONG CReserveManager::ChkInsertStatus(vector<CHK_RESERVE_DATA>& bank, CHK_RESERVE_DATA& inItem, bool modifyBank) const
{
	//lock_recursive_mutex lock(this->managerLock);

	LONGLONG distanceSameCh[] = { LLONG_MAX, LLONG_MAX };
	LONGLONG distanceOtherCh[] = { LLONG_MAX, LLONG_MAX };
	LONGLONG otherCosts[5] = {};

	for( size_t i = 0; i < bank.size(); i++ ){
		bool latter = false;
		LONGLONG dist = -1;
		if( bank[i].cutStartTime >= inItem.cutEndTime ){
			latter = true;
			dist = bank[i].cutStartTime - inItem.cutEndTime;
		}else if( bank[i].cutEndTime <= inItem.cutStartTime ){
			dist = inItem.cutStartTime - bank[i].cutEndTime;
		}

		if( bank[i].r->originalNetworkID == inItem.r->originalNetworkID && bank[i].r->transportStreamID == inItem.r->transportStreamID ){
			//同一チャンネル
			distanceSameCh[latter] = min(dist, distanceSameCh[latter]);
		}else{
			distanceOtherCh[latter] = min(dist, distanceOtherCh[latter]);
			if( bank[i].effectivePriority < inItem.effectivePriority ){
				//相手が高優先度なので自分の予約時間を削る
				if( bank[i].startOrder > inItem.startOrder ){
					//相手が遅れて開始するので自分の後方を削る
					LONGLONG cutEndTime = max(min(inItem.cutEndTime, bank[i].cutStartTime), inItem.cutStartTime);
					otherCosts[min(max<int>(inItem.r->recSetting.priority, 1), 5) - 1] += inItem.cutEndTime - cutEndTime;
					inItem.cutEndTime = cutEndTime;
				}else{
					//前方を削る
					LONGLONG cutStartTime = min(max(inItem.cutStartTime, bank[i].cutEndTime), inItem.cutEndTime);
					otherCosts[min(max<int>(inItem.r->recSetting.priority, 1), 5) - 1] += cutStartTime - inItem.cutStartTime;
					inItem.cutStartTime = cutStartTime;
				}
			}else{
				//相手の予約時間を削る
				if( inItem.startOrder > bank[i].startOrder ){
					//相手の後方を削る
					LONGLONG cutEndTime = max(min(bank[i].cutEndTime, inItem.cutStartTime), bank[i].cutStartTime);
					otherCosts[min(max<int>(bank[i].r->recSetting.priority, 1), 5) - 1] += bank[i].cutEndTime - cutEndTime;
					if( modifyBank ){
						bank[i].cutEndTime = cutEndTime;
					}
				}else{
					//前方を削る
					LONGLONG cutStartTime = bank[i].started ? bank[i].cutEndTime : min(max(bank[i].cutStartTime, inItem.cutEndTime), bank[i].cutEndTime);
					otherCosts[min(max<int>(bank[i].r->recSetting.priority, 1), 5) - 1] += cutStartTime - bank[i].cutStartTime;
					if( modifyBank ){
						bank[i].cutStartTime = cutStartTime;
					}
				}
			}
		}
	}

	//優先度ごとに重みをつけてコストを算出
	LONGLONG cost = 0;
	LONGLONG weight = 1;
	for( int i = 0; i < 5; i++ ){
		cost += min((otherCosts[i] + 10 * I64_1SEC - 1) / (10 * I64_1SEC), 5400LL - 1) * weight;
		weight *= 5400;
	}
	if( cost == 0 ){
		//犠牲なく配置できる
		//TODO: コスト0以下はどれを選んでもよいということなので、より良い配置を投機的に評価するとよいかも
		LONGLONG dist = min(distanceSameCh[0] < distanceOtherCh[0] ? distanceSameCh[0] : LLONG_MAX,
		                   distanceSameCh[1] < distanceOtherCh[1] ? distanceSameCh[1] : LLONG_MAX);
		//同一チャンネルで重なっていれば最低コスト、近ければまとまりが良いので低コストとする
		cost = -5401;
		if( dist >= 0 ){
			cost += 1 + min(dist / (10 * I64_1SEC), 5400LL);
		}
	}
	return cost;
}

void CReserveManager::CalcEntireReserveTime(LONGLONG* startTime, LONGLONG* endTime, const RESERVE_DATA& data) const
{
	//lock_recursive_mutex lock(this->managerLock);

	LONGLONG startTime_ = ConvertI64Time(data.startTime);
	LONGLONG endTime_ = startTime_ + data.durationSecond * I64_1SEC;
	LONGLONG startMargin = this->setting.startMargin * I64_1SEC;
	LONGLONG endMargin = this->setting.endMargin * I64_1SEC;
	if( data.recSetting.useMargineFlag != 0 ){
		startMargin = data.recSetting.startMargine * I64_1SEC;
		endMargin = data.recSetting.endMargine * I64_1SEC;
	}
	//開始マージンは元の予約終了時刻を超えて負であってはならない
	startMargin = max(startMargin, startTime_ - endTime_);
	//終了マージンは元の予約開始時刻を超えて負であってはならない
	endMargin = max(endMargin, startTime_ - min(startMargin, 0LL) - endTime_);
	if( startTime != NULL ){
		*startTime = startTime_ - startMargin;
	}
	if( endTime != NULL ){
		*endTime = endTime_ + endMargin;
	}
}

wstring CReserveManager::GetNotifyChgReserveMessage(const RESERVE_DATA& oldInfo, const RESERVE_DATA& newInfo)
{
	SYSTEMTIME stOld = oldInfo.startTime;
	SYSTEMTIME stOldEnd;
	ConvertSystemTime(ConvertI64Time(stOld) + oldInfo.durationSecond * I64_1SEC, &stOldEnd);
	SYSTEMTIME stNew = newInfo.startTime;
	SYSTEMTIME stNewEnd;
	ConvertSystemTime(ConvertI64Time(stNew) + newInfo.durationSecond * I64_1SEC, &stNewEnd);
	wstring msg;
	Format(msg, L"%ls %04d/%02d/%02d %02d:%02d～%02d:%02d\r\n%ls\r\nEventID:0x%04X\r\n↓\r\n%ls %04d/%02d/%02d %02d:%02d～%02d:%02d\r\n%ls\r\nEventID:0x%04X",
		oldInfo.stationName.c_str(), stOld.wYear, stOld.wMonth, stOld.wDay, stOld.wHour, stOld.wMinute,
		stOldEnd.wHour, stOldEnd.wMinute, oldInfo.title.c_str(), oldInfo.eventID,
		newInfo.stationName.c_str(), stNew.wYear, stNew.wMonth, stNew.wDay, stNew.wHour, stNew.wMinute,
		stNewEnd.wHour, stNewEnd.wMinute, newInfo.title.c_str(), newInfo.eventID);
	return msg;
}

void CReserveManager::CheckTuijyu()
{
	lock_recursive_mutex lock(this->managerLock);

	vector<RESERVE_DATA> chgList;
	for( map<DWORD, RESERVE_DATA>::const_iterator itr = this->reserveText.GetMap().begin(); itr != this->reserveText.GetMap().end(); itr++ ){
		if( itr->second.eventID == 0xFFFF || itr->second.reserveStatus != ADD_RESERVE_NORMAL ){
			//プログラム予約、および最新EPG(チューナからの情報)で変更済みの予約は対象外
			continue;
		}
		//原作と異なりrecMode==RECMODE_NOも扱う。またtuijyuuFlagは意味が変化しているので注意
		EPGDB_EVENT_INFO info;
		if( this->epgDBManager.SearchEpg(itr->second.originalNetworkID, itr->second.transportStreamID, itr->second.serviceID, itr->second.eventID, &info) ){
			//マージの都合でEIT[p/f]由来の未定時刻のイベントが混じるかもしれないがここでは無視する
			//EventIDの再使用と区別するため終了した番組は無視する
			if( info.StartTimeFlag != 0 && info.DurationFlag != 0 &&
			    GetNowI64Time() < ConvertI64Time(info.start_time) + info.durationSec * I64_1SEC ){
				RESERVE_DATA r = itr->second;
				bool chgRes = false;
				if( info.hasShortInfo && r.title != info.shortInfo.event_name ){
					r.title = info.shortInfo.event_name;
					chgRes = true;
				}
				if( ConvertI64Time(r.startTime) != ConvertI64Time(info.start_time) ){
					r.startTime = info.start_time;
					chgRes = true;
				}
				if( r.durationSecond != info.durationSec ){
					r.durationSecond = info.durationSec;
					chgRes = true;
				}
				if( chgRes ){
					chgList.push_back(r);
					wstring msg = GetNotifyChgReserveMessage(itr->second, r);
					this->notifyManager.AddNotifyMsg(NOTIFY_UPDATE_CHG_TUIJYU, msg);
					Replace(msg, L"\r\n", L" ");
					AddDebugLogFormat(L"●予約(ID=%d)を追従 %ls", r.reserveID, msg.c_str());
				}
			}
		}
	}
	if( chgList.empty() == false ){
		ChgReserveData(chgList);
	}
}

void CReserveManager::CheckTuijyuTuner()
{
	vector<DWORD> chkChList;
	//tunerBankMapそのものは排他制御の対象外
	for( auto itrBank = this->tunerBankMap.cbegin(); itrBank != this->tunerBankMap.end(); itrBank++ ){
		lock_recursive_mutex lock(this->managerLock);

		WORD onid, tsid;
		if( itrBank->second->GetCurrentChID(&onid, &tsid) == false ){
			//このチューナは起動していない
			continue;
		}
		if( std::find(chkChList.begin(), chkChList.end(), (DWORD)onid << 16 | tsid) != chkChList.end() ){
			//このチャンネルはチェック済み
			continue;
		}
		chkChList.push_back((DWORD)onid << 16 | tsid);
		vector<RESERVE_DATA> chgList;
		vector<RESERVE_DATA> relayAddList;
		const vector<pair<ULONGLONG, DWORD>>& cacheList = this->reserveText.GetSortByEventList();

		auto itrCache = lower_bound_first(cacheList.begin(), cacheList.end(), Create64PgKey(onid, tsid, 0, 0));
		for( ; itrCache != cacheList.end() && itrCache->first <= Create64PgKey(onid, tsid, 0xFFFF, 0xFFFF); ){
			//起動中のチャンネルに一致する予約をEIT[p/f]と照合する
			WORD sid = itrCache->first >> 16 & 0xFFFF;
			EPGDB_EVENT_INFO resPfVal[2];
			int nowSuccess = itrBank->second->GetEventPF(sid, false, &resPfVal[0]);
			int nextSuccess = itrBank->second->GetEventPF(sid, true, &resPfVal[1]);
			for( ; itrCache != cacheList.end() && itrCache->first <= Create64PgKey(onid, tsid, sid, 0xFFFF); itrCache++ ){
				map<DWORD, RESERVE_DATA>::const_iterator itrRes = this->reserveText.GetMap().find(itrCache->second);
				if( itrRes->second.eventID == 0xFFFF ||
				    itrRes->second.recSetting.IsNoRec() ||
				    ConvertI64Time(itrRes->second.startTime) > GetNowI64Time() + 6 * 3600 * I64_1SEC ){
					//プログラム予約、無効予約、および6時間以上先の予約は対象外
					continue;
				}
				//ADD_RESERVE_NORMAL,RELAY
				//├→CHG_PF2
				//└─┼→CHG_PF←┐
				//    └─┴→UNKNOWN_END
				bool pfFound = false;
				bool pfUnknownEnd = false;
				bool pfExplicitlyUnknownEnd = false;
				for( int i = (nowSuccess == 0 ? 0 : 1); i < (nextSuccess == 0 ? 2 : 1); i++ ){
					const EPGDB_EVENT_INFO& info = resPfVal[i];
					if( i == 1 && (info.StartTimeFlag == 0 || info.DurationFlag == 0) ){
						pfUnknownEnd = true;
						if( info.StartTimeFlag == 0 && info.DurationFlag == 0 ){
							//未定イベントのときは以降の放送未定が明示されているとみなす(主にNHK)
							pfExplicitlyUnknownEnd = true;
						}
					}
					if( info.event_id == itrRes->second.eventID && (info.StartTimeFlag != 0 || info.DurationFlag != 0) ){
						//現在(present)に現れた予約が番組終了後に時間未定追従に移行しないようにするため(旧SetChkPfInfo()に相当)
						if( i == 0 && itrRes->second.presentFlag == FALSE ){
							this->reserveText.SetPresentFlag(itrRes->first, TRUE);
							AddDebugLogFormat(L"●予約(ID=%d)のEIT[present]を確認しました", itrRes->first);
						}
						RESERVE_DATA r = itrRes->second;
						bool chgRes = false;
						if( info.hasShortInfo && r.title != info.shortInfo.event_name ){
							r.title = info.shortInfo.event_name;
							if( r.reserveStatus != ADD_RESERVE_UNKNOWN_END ){
								r.reserveStatus = ADD_RESERVE_CHG_PF;
							}
							chgRes = true;
						}
						if( info.StartTimeFlag != 0 ){
							if( ConvertI64Time(r.startTime) != ConvertI64Time(info.start_time) ){
								r.startTime = info.start_time;
								if( r.reserveStatus != ADD_RESERVE_UNKNOWN_END ){
									r.reserveStatus = ADD_RESERVE_CHG_PF;
								}
								chgRes = true;
							}
							if( info.DurationFlag == 0 ){
								//継続時間未定。終了まで5分を切る予約は5分伸ばす
								LONGLONG endTime;
								CalcEntireReserveTime(NULL, &endTime, r);
								if( endTime < GetNowI64Time() + 300 * I64_1SEC ){
									r.durationSecond += 300;
									r.reserveStatus = ADD_RESERVE_UNKNOWN_END;
									chgRes = true;
									AddDebugLog(L"●p/f 継続時間未定の現在/次イベントの予約を延長します");
								}
							}else if( r.reserveStatus == ADD_RESERVE_UNKNOWN_END || r.durationSecond != info.durationSec ){
								r.durationSecond = info.durationSec;
								r.reserveStatus = ADD_RESERVE_CHG_PF;
								chgRes = true;
							}
						}else{
							//開始時刻未定。次(following)かつ終了まで5分を切る予約は5分伸ばす
							LONGLONG endTime;
							CalcEntireReserveTime(NULL, &endTime, r);
							if( i == 1 && endTime < GetNowI64Time() + 300 * I64_1SEC ){
								r.durationSecond += 300;
								r.reserveStatus = ADD_RESERVE_UNKNOWN_END;
								chgRes = true;
								AddDebugLog(L"●p/f 開始時刻未定の次イベントの予約を延長します");
							}
						}
						if( chgRes ){
							chgList.push_back(r);
							wstring msg = GetNotifyChgReserveMessage(itrRes->second, r);
							this->notifyManager.AddNotifyMsg(NOTIFY_UPDATE_REC_TUIJYU, msg);
							Replace(msg, L"\r\n", L" ");
							AddDebugLogFormat(L"●p/f 予約(ID=%d)を追従 %ls", r.reserveID, msg.c_str());
						}
						//現在(present)についてはイベントリレーもチェック
						if( i == 0 && r.recSetting.tuijyuuFlag && info.StartTimeFlag && info.DurationFlag && info.eventRelayInfoGroupType ){
							//イベントリレーあり
							vector<EPGDB_EVENT_DATA>::const_iterator itrR = info.eventRelayInfo.eventDataList.begin();
							for( ; itrR != info.eventRelayInfo.eventDataList.end(); itrR++ ){
								if( IsFindReserve(itrR->original_network_id, itrR->transport_stream_id,
								                  itrR->service_id, itrR->event_id, r.recSetting.tunerID) ){
									//リレー済み
									break;
								}
							}
							if( itrR == info.eventRelayInfo.eventDataList.end() ){
								AddDebugLog(L"EventRelayCheck");
								for( itrR = info.eventRelayInfo.eventDataList.begin(); itrR != info.eventRelayInfo.eventDataList.end(); itrR++ ){
									map<LONGLONG, CH_DATA5>::const_iterator itrCh = this->chUtil.GetMap().find(
										Create64Key(itrR->original_network_id, itrR->transport_stream_id, itrR->service_id));
									if( itrCh != this->chUtil.GetMap().end() && relayAddList.empty() ){
										//リレーできるチャンネル発見
										RESERVE_DATA rr;
										rr.title = L"(イベントリレー)" + r.title;
										//リレー元の終了時間をリレー先の開始時間とする
										ConvertSystemTime(ConvertI64Time(info.start_time) + info.durationSec * I64_1SEC, &rr.startTime);
										rr.startTimeEpg = rr.startTime;
										rr.durationSecond = 600;
										rr.stationName = itrCh->second.serviceName;
										rr.originalNetworkID = itrR->original_network_id;
										rr.transportStreamID = itrR->transport_stream_id;
										rr.serviceID = itrR->service_id;
										rr.eventID = itrR->event_id;
										//録画設定はリレー元の予約を継承
										rr.recSetting = r.recSetting;
										rr.reserveStatus = ADD_RESERVE_RELAY;
										relayAddList.push_back(rr);
										AddDebugLog(L"★イベントリレー追加");
										break;
									}
								}
							}
						}
						pfFound = true;
						break;
					}
				}
				//EIT[p/f]にあるものや現在(present)に現れたことのある予約は除外する
				if( pfFound == false && itrRes->second.presentFlag == FALSE ){
					RESERVE_DATA r = itrRes->second;
					bool chgRes = false;
					bool chgResStatusOnly = false;
					if( pfUnknownEnd ){
						//EIT[p/f]の継続時間未定。以降の予約も時間未定とみなし、終了まで5分を切る予約は5分伸ばす
						LONGLONG startTime, endTime;
						CalcEntireReserveTime(&startTime, &endTime, r);
						if( endTime - startTime < this->setting.tuijyuHour * 3600 * I64_1SEC && endTime < GetNowI64Time() + 300 * I64_1SEC ){
							r.durationSecond += 300;
							r.reserveStatus = ADD_RESERVE_UNKNOWN_END;
							chgRes = true;
							AddDebugLog(L"●時間未定の通常イベントの予約を延長します");
						}
						if( pfExplicitlyUnknownEnd && r.reserveStatus != ADD_RESERVE_UNKNOWN_END && ConvertI64Time(r.startTime) < GetNowI64Time() + 3600 * I64_1SEC ){
							//明示的な放送未定の場合は開始まで60分を切る時点でUNKNOWN_ENDにする
							//(この閾値は時間未定解消後のEIT[p/f]にイベントID変更で現れる可能性がありそうな範囲として適当に決めたもの)
							r.reserveStatus = ADD_RESERVE_UNKNOWN_END;
							chgRes = true;
							chgResStatusOnly = true;
							AddDebugLog(L"●時間未定の通常イベントの予約をUNKNOWN_ENDにします");
						}
					}else if( r.reserveStatus == ADD_RESERVE_UNKNOWN_END ){
						//イベントID直前変更対応(主にNHK)
						//時間未定解消後にUNKNOWN_ENDになっている予約に限り、イベント名が完全一致するEIT[p/f]が存在すれば、そのイベントの終了時間まで予約を伸ばす
						for( int i = (nowSuccess == 0 ? 0 : 1); i < (nextSuccess == 0 ? 2 : 1); i++ ){
							const EPGDB_EVENT_INFO& info = resPfVal[i];
							if( info.StartTimeFlag != 0 && info.DurationFlag != 0 &&
							    r.title.empty() == false && info.hasShortInfo && r.title == info.shortInfo.event_name ){
								LONGLONG endTime = ConvertI64Time(info.start_time) + info.durationSec * I64_1SEC;
								if( endTime > ConvertI64Time(r.startTime) + r.durationSecond * I64_1SEC ){
									r.durationSecond = (DWORD)((endTime - ConvertI64Time(r.startTime)) / I64_1SEC) + 1;
									chgRes = true;
									AddDebugLog(L"●時間未定の通常イベントの予約と同じイベント名のp/fが見つかりました。予約を延長します");
								}
								break;
							}
						}
					}
					//EIT[p/f]が正しく取得できる状況でEIT[p/f]にないものは通常チェック
					EPGDB_EVENT_INFO info;
					if( nowSuccess != 2 && nextSuccess != 2 &&
					    r.reserveStatus != ADD_RESERVE_CHG_PF &&
					    r.reserveStatus != ADD_RESERVE_UNKNOWN_END &&
					    itrBank->second->SearchEpgInfo(sid, r.eventID, &info) ){
						//EventIDの再使用と区別するため終了した番組は無視する
						if( info.StartTimeFlag != 0 && info.DurationFlag != 0 &&
						    GetNowI64Time() < ConvertI64Time(info.start_time) + info.durationSec * I64_1SEC ){
							if( info.hasShortInfo && r.title != info.shortInfo.event_name ){
								r.title = info.shortInfo.event_name;
								//EPG再読み込みで変更されないようにする
								r.reserveStatus = ADD_RESERVE_CHG_PF2;
								chgRes = true;
							}
							if( ConvertI64Time(r.startTime) != ConvertI64Time(info.start_time) ){
								r.startTime = info.start_time;
								r.reserveStatus = ADD_RESERVE_CHG_PF2;
								chgRes = true;
							}
							if( r.durationSecond != info.durationSec ){
								r.durationSecond = info.durationSec;
								r.reserveStatus = ADD_RESERVE_CHG_PF2;
								chgRes = true;
							}
						}
					}
					if( chgRes ){
						chgList.push_back(r);
						wstring msg = GetNotifyChgReserveMessage(itrRes->second, r);
						if( chgResStatusOnly == false ){
							this->notifyManager.AddNotifyMsg(NOTIFY_UPDATE_REC_TUIJYU, msg);
						}
						Replace(msg, L"\r\n", L" ");
						AddDebugLogFormat(L"●予約(ID=%d)を追従 %ls", r.reserveID, msg.c_str());
					}
				}
			}
		}
		if( chgList.empty() == false ){
			ChgReserveData(chgList, true);
		}
		if( relayAddList.empty() == false ){
			AddReserveData(relayAddList, true);
		}
	}
}

void CReserveManager::CheckAutoDel() const
{
	lock_recursive_mutex lock(this->managerLock);

	if( this->setting.autoDel == false ){
		return;
	}

	//ファイル削除可能なフォルダのドライブを調べる
	vector<wstring> mountList;
	for( size_t i = 0; i < this->setting.delChkList.size(); i++ ){
		mountList.push_back(UtilGetStorageID(this->setting.delChkList[i]));
	}

	//ドライブレベルでのチェック
	LONGLONG now = GetNowI64Time();
	for( size_t checkIndex = 0; checkIndex < mountList.size(); checkIndex++ ){
		if( mountList[checkIndex].empty() ){
			//チェック済み
			continue;
		}
		//直近で必要になりそうな空き領域を概算する
		LONGLONG needSize = 0;
		for( auto jtr = this->reserveText.GetMap().cbegin(); jtr != this->reserveText.GetMap().end(); jtr++ ){
			LONGLONG startTime, endTime;
			CalcEntireReserveTime(&startTime, &endTime, jtr->second);
			if( jtr->second.recSetting.IsNoRec() == false &&
			    jtr->second.recSetting.GetRecMode() != RECMODE_VIEW &&
			    startTime < now + 2 * 60 * 60 * I64_1SEC ){
				//録画開始2時間前までの予約
				const vector<REC_FILE_SET_INFO>& recFolderList = jtr->second.recSetting.recFolderList;
				for( size_t i = 0; (i == 0 && recFolderList.empty()) || i < recFolderList.size(); i++ ){
					wstring mountPath;
					if( recFolderList.empty() || CompareNoCase(recFolderList[i].recFolder, L"!Default") == 0 ){
						//デフォルト(注意: !Defaultの置換は原作にはない)
						mountPath = UtilGetStorageID(GetRecFolderPath());
					}else{
						mountPath = UtilGetStorageID(recFolderList[i].recFolder);
					}
					if( CompareNoCase(mountPath, mountList[checkIndex]) == 0 ){
						if( needSize == 0 ){
							//延長や外部要因による空き領域減少に対処するため最低限の余裕をとる
							needSize = 512 * 1024 * 1024;
						}
						//時計精度の関係で実際に録画が始まった後もしばらくこの条件を満たし、余分に確保されるかもしれない
						//(厳密にやるのは簡単ではないので、従来通りゆるい実装にしておく)
						if( now < startTime ){
							DWORD bitrate = GetBitrateFromIni(jtr->second.originalNetworkID, jtr->second.transportStreamID, jtr->second.serviceID);
							needSize += (LONGLONG)(bitrate / 8 * 1000) * (endTime - startTime) / I64_1SEC;
						}
					}
				}
			}
		}

		LONGLONG freeBytes = UtilGetStorageFreeBytes(this->setting.delChkList[checkIndex]);
		if( freeBytes >= 0 && freeBytes < needSize ){
			//ドライブにある古いTS順に必要なだけ消す
			LONGLONG needFreeSize = needSize - freeBytes;
			vector<pair<UTIL_FIND_DATA, size_t>> findList;
			for( size_t i = checkIndex; i < mountList.size(); i++ ){
				if( CompareNoCase(mountList[i], mountList[checkIndex]) == 0 ){
					EnumFindFile(fs_path(this->setting.delChkList[i]).append(L'*' + this->setting.tsExt),
					             [&](UTIL_FIND_DATA& findData) -> bool {
						if( findData.isDir == false && UtilPathEndsWith(findData.fileName.c_str(), this->setting.tsExt.c_str()) ){
							findList.push_back(std::make_pair(std::move(findData), i));
						}
						return true;
					});
				}
			}
			while( needFreeSize > 0 && findList.empty() == false ){
				//更新日時が古いもの
				auto jtr = std::min_element(findList.begin(), findList.end(),
					[](const pair<UTIL_FIND_DATA, size_t>& a, const pair<UTIL_FIND_DATA, size_t>& b) {
						return a.first.lastWriteTime < b.first.lastWriteTime; });
				fs_path delPath = fs_path(this->setting.delChkList[jtr->second]).append(jtr->first.fileName);
				if( this->recInfoText.GetMap().end() != std::find_if(this->recInfoText.GetMap().begin(), this->recInfoText.GetMap().end(),
				        [&](const pair<DWORD, REC_FILE_INFO>& a) {
				            return a.second.protectFlag && UtilComparePath(a.second.recFilePath.c_str(), delPath.c_str()) == 0; }) ){
					//プロテクトされた録画済みファイルは消さない
					AddDebugLogFormat(L"★No Delete(Protected) : %ls", delPath.c_str());
				}else{
					DeleteFile(delPath.c_str());
					needFreeSize -= jtr->first.fileSize;
					AddDebugLogFormat(L"★Auto Delete2 : %ls", delPath.c_str());
					for( size_t i = 0 ; i < this->setting.delExtList.size(); i++ ){
						DeleteFile(fs_path(delPath).replace_extension(this->setting.delExtList[i]).c_str());
						AddDebugLogFormat(L"★Auto Delete2 : %ls", fs_path(delPath).replace_extension(this->setting.delExtList[i]).c_str());
					}
				}
				findList.erase(jtr);
			}
		}
		//チェック済みにする
		for( size_t i = checkIndex + 1; i < mountList.size(); i++ ){
			if( CompareNoCase(mountList[i], mountList[checkIndex]) == 0 ){
				mountList[i].clear();
			}
		}
	}
}

void CReserveManager::CheckOverTimeReserve()
{
	lock_recursive_mutex lock(this->managerLock);

	bool modified = false;
	LONGLONG now = GetNowI64Time();
	vector<DWORD> noList = GetNoTunerReserveAll();
	for( size_t i = 0; i < noList.size(); i++ ){
		map<DWORD, RESERVE_DATA>::const_iterator itr = this->reserveText.GetMap().find(noList[i]);
		LONGLONG endTime;
		CalcEntireReserveTime(NULL, &endTime, itr->second);
		if( endTime < now ){
			//終了時間過ぎてしまっている
			if( itr->second.recSetting.IsNoRec() == false ){
				//無効のものは結果に残さない
				REC_FILE_INFO item;
				item = itr->second;
				item.recStatus = REC_END_STATUS_NO_TUNER;
				this->recInfoText.AddRecInfo(item);
			}
			this->reserveText.DelReserve(itr->first);
			this->reserveModified = true;
			modified = true;
		}
	}
	if( modified ){
		this->reserveText.SaveText();
		this->recInfoText.SaveText();
		AddNotifyAndPostBat(NOTIFY_UPDATE_RESERVE_INFO);
		AddNotifyAndPostBat(NOTIFY_UPDATE_REC_INFO);
	}
}

void CReserveManager::ProcessRecEnd(const vector<CTunerBankCtrl::CHECK_RESULT>& retList, DWORD tunerID, int* shutdownMode)
{
	vector<CBatManager::BAT_WORK_INFO> batWorkList;
	bool modified = false;
	bool ngTunerAdded = false;
	for( auto itrRet = retList.cbegin(); itrRet != retList.end(); itrRet++ ){
		map<DWORD, RESERVE_DATA>::const_iterator itrRes = this->reserveText.GetMap().find(itrRet->reserveID);
		if( itrRes != this->reserveText.GetMap().end() ){
			if( this->setting.retryOtherTuners && itrRet->type == CTunerBankCtrl::CHECK_ERR_OPEN ){
				AddDebugLogFormat(L"●予約(ID=%d)にNGチューナー(ID=0x%08x)を追加します", itrRes->first, tunerID);
				this->reserveText.AddNGTunerID(itrRes->first, tunerID);
				ngTunerAdded = true;
				continue;
			}
			if( itrRet->type == CTunerBankCtrl::CHECK_END && itrRet->recFilePath.empty() == false &&
			    itrRet->drops < this->setting.recInfo2DropChk && itrRet->epgEventName.empty() == false ){
				//録画済みとして登録
				PARSE_REC_INFO2_ITEM item;
				item.originalNetworkID = itrRes->second.originalNetworkID;
				item.transportStreamID = itrRes->second.transportStreamID;
				item.serviceID = itrRes->second.serviceID;
				item.startTime = itrRet->epgStartTime;
				item.eventName = itrRet->epgEventName;
				this->recInfo2Text.Add(item);
			}

			REC_FILE_INFO item;
			item = itrRes->second;
			if( itrRet->type <= CTunerBankCtrl::CHECK_END_NOT_START_HEAD ){
				item.recFilePath = itrRet->recFilePath;
				item.drops = itrRet->drops;
				item.scrambles = itrRet->scrambles;
			}
			switch( itrRet->type ){
			case CTunerBankCtrl::CHECK_END:
				if( ConvertI64Time(item.startTime) != ConvertI64Time(item.startTimeEpg) ){
					item.recStatus = REC_END_STATUS_CHG_TIME;
				}else{
					item.recStatus = REC_END_STATUS_NORMAL;
				}
				break;
			case CTunerBankCtrl::CHECK_END_NOT_FIND_PF:
				item.recStatus = REC_END_STATUS_NOT_FIND_PF;
				break;
			case CTunerBankCtrl::CHECK_END_NEXT_START_END:
				item.recStatus = REC_END_STATUS_NEXT_START_END;
				break;
			case CTunerBankCtrl::CHECK_END_END_SUBREC:
				item.recStatus = REC_END_STATUS_END_SUBREC;
				break;
			case CTunerBankCtrl::CHECK_END_NOT_START_HEAD:
				item.recStatus = REC_END_STATUS_NOT_START_HEAD;
				break;
			case CTunerBankCtrl::CHECK_ERR_RECEND:
				item.recStatus = REC_END_STATUS_ERR_END2;
				break;
			case CTunerBankCtrl::CHECK_END_CANCEL:
			case CTunerBankCtrl::CHECK_ERR_REC:
				item.recStatus = REC_END_STATUS_ERR_END;
				break;
			case CTunerBankCtrl::CHECK_ERR_RECSTART:
			case CTunerBankCtrl::CHECK_ERR_CTRL:
				item.recStatus = REC_END_STATUS_ERR_RECSTART;
				break;
			case CTunerBankCtrl::CHECK_ERR_OPEN:
				item.recStatus = REC_END_STATUS_OPEN_ERR;
				break;
			case CTunerBankCtrl::CHECK_ERR_PASS:
				item.recStatus = REC_END_STATUS_START_ERR;
				break;
			}
			item.id = this->recInfoText.AddRecInfo(item);

			//バッチ処理追加
			CBatManager::BAT_WORK_INFO batInfo;
			AddRecInfoMacro(batInfo.macroList, item);
			batInfo.macroList.push_back(pair<string, wstring>("AddKey",
				itrRes->second.comment.compare(0, 8, L"EPG自動予約(") == 0 && itrRes->second.comment.find(L')') != wstring::npos ?
				itrRes->second.comment.substr(8, itrRes->second.comment.find(L')') - 8) : wstring()));
			batInfo.macroList.push_back(pair<string, wstring>("BatFileTag",
				itrRes->second.recSetting.batFilePath.find(L'*') != wstring::npos ?
				itrRes->second.recSetting.batFilePath.substr(itrRes->second.recSetting.batFilePath.find(L'*') + 1) : wstring()));
			if( itrRet->type != CTunerBankCtrl::CHECK_ERR_PASS ){
				batWorkList.push_back(batInfo);
				if( shutdownMode ){
					*shutdownMode = itrRes->second.recSetting.rebootFlag << 8 | itrRes->second.recSetting.suspendMode;
				}
			}
			batInfo.batFilePath.assign(itrRes->second.recSetting.batFilePath, 0, itrRes->second.recSetting.batFilePath.find(L'*'));
			if( (itrRet->type == CTunerBankCtrl::CHECK_END || itrRet->type == CTunerBankCtrl::CHECK_END_NEXT_START_END || this->setting.errEndBatRun) &&
			    item.recFilePath.empty() == false && batInfo.batFilePath.empty() == false && itrRet->continueRec == false ){
				this->batManager.AddBatWork(batInfo);
			}

			this->reserveText.DelReserve(itrRes->first);
			this->reserveModified = true;
			modified = true;

			//予約終了を通知
			SYSTEMTIME st = item.startTime;
			SYSTEMTIME stEnd;
			ConvertSystemTime(ConvertI64Time(st) + item.durationSecond * I64_1SEC, &stEnd);
			wstring msg;
			Format(msg, L"%ls %04d/%02d/%02d %02d:%02d～%02d:%02d\r\n%ls\r\n%ls",
			       item.serviceName.c_str(), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute,
			       stEnd.wHour, stEnd.wMinute, item.title.c_str(), item.GetComment());
			this->notifyManager.AddNotifyMsg(NOTIFY_UPDATE_REC_END, msg);
		}
	}
	if( ngTunerAdded ){
		ReloadBankMap();
		if( modified == false ){
			AddNotifyAndPostBat(NOTIFY_UPDATE_RESERVE_INFO);
		}
	}
	if( modified ){
		this->reserveText.SaveText();
		this->recInfoText.SaveText();
		this->recInfo2Text.SaveText();
		AddNotifyAndPostBat(NOTIFY_UPDATE_RESERVE_INFO);
		AddNotifyAndPostBat(NOTIFY_UPDATE_REC_INFO);
		AddPostBatWork(batWorkList, L"PostRecEnd");
	}
}

pair<CReserveManager::CHECK_STATUS, int> CReserveManager::Check()
{
	this->checkCount++;

	bool isRec = false;
	bool isEpgCap = false;
	//tunerBankMapそのものは排他制御の対象外
	for( auto itrBank = this->tunerBankMap.cbegin(); itrBank != this->tunerBankMap.end(); itrBank++ ){
		lock_recursive_mutex lock(this->managerLock);

		// チューナの予約状態遷移を行い、予約終了をチェックする
		vector<DWORD> startedReserveIDList;
		vector<CTunerBankCtrl::CHECK_RESULT> retList = itrBank->second->Check(&startedReserveIDList);
		CTunerBankCtrl::TR_STATE state = itrBank->second->GetState();
		isRec = isRec || state == CTunerBankCtrl::TR_REC;
		isEpgCap = isEpgCap || state == CTunerBankCtrl::TR_EPGCAP;
		vector<CBatManager::BAT_WORK_INFO> batWorkList;
		for( size_t i = 0; i < startedReserveIDList.size(); i++ ){
			map<DWORD, RESERVE_DATA>::const_iterator itrRes = this->reserveText.GetMap().find(startedReserveIDList[i]);
			if( itrRes != this->reserveText.GetMap().end() ){
				batWorkList.resize(batWorkList.size() + 1);
				AddReserveDataMacro(batWorkList.back().macroList, itrRes->second, "");
			}
		}
		AddPostBatWork(batWorkList, L"PostRecStart");
		ProcessRecEnd(retList, itrBank->first, &this->shutdownModePending);
	}
	if( this->checkCount % 30 == 0 ){
		CheckAutoDel();
		CheckOverTimeReserve();
	}
	if( this->checkCount % 3 == 0 ){
		CheckTuijyuTuner();
	}
	LONGLONG idleMargin = GetNearestRecReserveTime() - GetNowI64Time();
	this->batManager.SetIdleMargin((DWORD)min(max(idleMargin / I64_1SEC, 0LL), 0xFFFFFFFFLL));
	this->notifyManager.SetNotifySrvStatus(isRec ? 1 : isEpgCap ? 2 : 0);

	if( CheckEpgCap(isEpgCap) ){
		//EPG取得が完了した
		this->notifyManager.AddNotifyMsg(NOTIFY_UPDATE_EPGCAP_END, L"");
		return std::make_pair(CHECK_EPGCAP_END, 0);
	}else if( this->shutdownModePending >= 0 &&
	          this->batManager.IsWorking() == false &&
	          this->batPostManager.IsWorkingWithoutNotification() == false ){
		//バッチ処理が完了した
		int shutdownMode = this->shutdownModePending;
		this->shutdownModePending = -1;
		return std::make_pair(CHECK_NEED_SHUTDOWN, shutdownMode);
	}else if( this->reserveModified ){
		lock_recursive_mutex lock(this->managerLock);
		if( this->reserveModified ){
			this->reserveModified = false;
			return std::make_pair(CHECK_RESERVE_MODIFIED, 0);
		}
	}
	return std::make_pair(CHECK_NO_ACTION, 0);
}

vector<CTunerBankCtrl*> CReserveManager::GetEpgCapTunerList(LONGLONG now) const
{
	lock_recursive_mutex lock(this->managerLock);

	//利用可能なチューナの抽出
	vector<CTunerBankCtrl*> tunerList;
	for( auto itr = this->tunerBankMap.cbegin(); itr != this->tunerBankMap.end(); ){
		DWORD bonID = itr->first >> 16;
		WORD epgCapMax = itr->second->GetEpgCapMaxOfThisBon();
		WORD epgCapCount = 0;
		WORD ngCapCount = 0;
		WORD tunerCount = 0;
		for( ; itr != this->tunerBankMap.end() && itr->first >> 16 == bonID; itr++, tunerCount++ ){
			if( epgCapCount == epgCapMax ){
				continue;
			}
			CTunerBankCtrl::TR_STATE state = itr->second->GetState();
			LONGLONG minTime = itr->second->GetNearestReserveTime();
			if( this->setting.ngEpgCapTime != 0 && (state != CTunerBankCtrl::TR_IDLE || minTime < now + this->setting.ngEpgCapTime * 60 * I64_1SEC) ){
				//実行しちゃいけない
				ngCapCount++;
			}else if( state == CTunerBankCtrl::TR_IDLE && minTime > now + this->setting.ngEpgCapTunerTime * 60 * I64_1SEC ){
				//使えるチューナ
				tunerList.push_back(itr->second.get());
				epgCapCount++;
			}
		}
		if( epgCapMax > tunerCount - ngCapCount ){
			tunerList.clear();
			break;
		}
	}
	return tunerList;
}

bool CReserveManager::RequestStartEpgCap()
{
	lock_recursive_mutex lock(this->managerLock);

	if( this->epgCapRequested || this->epgCapWork || GetEpgCapTunerList(GetNowI64Time()).empty() ){
		return false;
	}
	this->epgCapRequested = true;
	return true;
}

bool CReserveManager::CheckEpgCap(bool isEpgCap)
{
	lock_recursive_mutex lock(this->managerLock);

	bool doneEpgCap = false;
	LONGLONG now = GetNowI64Time();
	if( this->epgCapWork == false ){
		//毎分0秒を跨ぐタイミングでEPG取得のチェックを行う
		if( this->epgCapRequested || now / (60 * I64_1SEC) > this->lastCheckEpgCap / (60 * I64_1SEC) ){
			int basicOnlyFlags = -1;
			LONGLONG capTime = this->epgCapRequested ? now : GetNextEpgCapTime(now, &basicOnlyFlags);
			if( capTime <= now + 60 * I64_1SEC ){
				vector<CTunerBankCtrl*> tunerList = GetEpgCapTunerList(now);
				if( tunerList.empty() == false ){
					if( capTime > now ){
						//取得開始1分前
						//この通知はあくまで参考。開始しないのに通知する可能性も、その逆もありえる
						this->notifyManager.AddNotifyMsg(NOTIFY_UPDATE_PRE_EPGCAP_START, L"取得開始１分前");
					}else{
						//取得開始
						fs_path iniCommonPath = GetCommonIniPath();
						int lastFlags = (GetPrivateProfileInt(L"SET", L"BSBasicOnly", 1, iniCommonPath.c_str()) != 0 ? 1 : 0) |
						                (GetPrivateProfileInt(L"SET", L"CS1BasicOnly", 1, iniCommonPath.c_str()) != 0 ? 2 : 0) |
						                (GetPrivateProfileInt(L"SET", L"CS2BasicOnly", 1, iniCommonPath.c_str()) != 0 ? 4 : 0) |
						                (GetPrivateProfileInt(L"SET", L"CS3BasicOnly", 0, iniCommonPath.c_str()) != 0 ? 8 : 0);
						if( basicOnlyFlags >= 0 ){
							//一時的に設定を変更してEPG取得チューナ側の挙動を変える
							//TODO: パイプコマンドを拡張すべき
							this->epgCapBasicOnlyFlags = lastFlags;
							WritePrivateProfileInt(L"SET", L"BSBasicOnly", (basicOnlyFlags & 1) != 0, iniCommonPath.c_str());
							WritePrivateProfileInt(L"SET", L"CS1BasicOnly", (basicOnlyFlags & 2) != 0, iniCommonPath.c_str());
							WritePrivateProfileInt(L"SET", L"CS2BasicOnly", (basicOnlyFlags & 4) != 0, iniCommonPath.c_str());
							WritePrivateProfileInt(L"SET", L"CS3BasicOnly", (basicOnlyFlags & 8) != 0, iniCommonPath.c_str());
						}else{
							this->epgCapBasicOnlyFlags = -1;
							basicOnlyFlags = lastFlags;
						}
						//各チューナに振り分け
						LONGLONG lastKey = -1;
						bool inONIDs[16] = {};
						size_t listIndex = 0;
						vector<vector<SET_CH_INFO>> epgCapChList(tunerList.size());
						for( map<LONGLONG, CH_DATA5>::const_iterator itr = this->chUtil.GetMap().begin(); itr != this->chUtil.GetMap().end(); itr++ ){
							if( itr->second.epgCapFlag == FALSE ||
							    lastKey >= 0 && lastKey == itr->first >> 16 ||
							    itr->second.originalNetworkID == 4 && (basicOnlyFlags & 1) && inONIDs[4] ||
							    itr->second.originalNetworkID == 6 && (basicOnlyFlags & 2) && inONIDs[6] ||
							    itr->second.originalNetworkID == 7 && (basicOnlyFlags & 4) && inONIDs[7] ||
							    itr->second.originalNetworkID == 10 && (basicOnlyFlags & 8) && inONIDs[10] ){
								continue;
							}
							lastKey = itr->first >> 16;
							SET_CH_INFO addCh;
							addCh.ONID = itr->second.originalNetworkID;
							addCh.TSID = itr->second.transportStreamID;
							addCh.SID = itr->second.serviceID;
							addCh.useSID = TRUE;
							addCh.useBonCh = FALSE;
							for( size_t i = 0; i < tunerList.size(); i++ ){
								if( tunerList[listIndex]->GetCh(addCh.ONID, addCh.TSID, addCh.SID) ){
									epgCapChList[listIndex].push_back(addCh);
									inONIDs[min<size_t>(addCh.ONID, array_size(inONIDs) - 1)] = true;
									listIndex = (listIndex + 1) % tunerList.size();
									break;
								}
								listIndex = (listIndex + 1) % tunerList.size();
							}
						}
						for( size_t i = 0; i < tunerList.size(); i++ ){
							tunerList[i]->StartEpgCap(epgCapChList[i]);
						}
						this->epgCapWork = true;
						this->epgCapSetTimeSync = false;
						this->epgCapTimeSyncBase = -1;
						this->notifyManager.AddNotifyMsg(NOTIFY_UPDATE_EPGCAP_START, L"");
					}
				}
			}
		}
	}else{
		//EPG取得中
		if( this->setting.timeSync && this->epgCapSetTimeSync == false ){
			DWORD tick = GetU32Tick();
			for( auto itr = this->tunerBankMap.cbegin(); itr != this->tunerBankMap.end(); itr++ ){
				if( itr->second->GetState() == CTunerBankCtrl::TR_EPGCAP ){
					LONGLONG delay = itr->second->DelayTime();
					if( this->epgCapTimeSyncBase < 0 ){
						if( delay < -10 * I64_1SEC || 10 * I64_1SEC < delay ){
							//時計合わせが必要かもしれない。遅延時間の観測開始
							this->epgCapTimeSyncBase = now;
							this->epgCapTimeSyncDelayMin = delay;
							this->epgCapTimeSyncDelayMax = delay;
							this->epgCapTimeSyncTick = tick;
							this->epgCapTimeSyncQuality = 0;
							AddDebugLog(L"★SetSystemTime start");
						}
					}else if( delay != 0 ){
						//遅延時間の揺らぎを記録する(delay==0は未取得と区別できないので除外)
						this->epgCapTimeSyncDelayMin = min(delay, this->epgCapTimeSyncDelayMin);
						this->epgCapTimeSyncDelayMax = max(delay, this->epgCapTimeSyncDelayMax);
						this->epgCapTimeSyncQuality += tick - this->epgCapTimeSyncTick;
					}
				}
			}
			if( this->epgCapTimeSyncBase >= 0 ){
				this->epgCapTimeSyncBase += (tick - this->epgCapTimeSyncTick) * (I64_1SEC / 1000);
				this->epgCapTimeSyncTick = tick;
				if( now - this->epgCapTimeSyncBase < -3 * I64_1SEC || 3 * I64_1SEC < now - this->epgCapTimeSyncBase ||
				    this->epgCapTimeSyncDelayMax - this->epgCapTimeSyncDelayMin > 10 * I64_1SEC ){
					//別のプロセスが時計合わせしたor揺らぎすぎ
					this->epgCapTimeSyncBase = -1;
					AddDebugLog(L"★SetSystemTime cancel");
				}else if( this->epgCapTimeSyncQuality > 150 * 1000 ){
					//概ね2チャンネル以上の遅延時間を観測できたはず
					//時計合わせ(要SE_SYSTEMTIME_NAME特権)
					LONGLONG delay = (this->epgCapTimeSyncDelayMax + this->epgCapTimeSyncDelayMin) / 2;
					SYSTEMTIME setTime;
					ConvertSystemTime(now + delay - I64_UTIL_TIMEZONE, &setTime);
					LPCWSTR debug = L" err ";
#ifdef _WIN32
					if( SetSystemTime(&setTime) ){
						debug = L" ";
					}else{
						//代理プロセス経由で時計合わせを試みる
						HWND hwnd = FindWindowEx(HWND_MESSAGE, NULL, L"EpgTimerAdminProxy", NULL);
						FILETIME ft;
						if( hwnd && SystemTimeToFileTime(&setTime, &ft) ){
							DWORD_PTR result;
							if( SendMessageTimeout(hwnd, WM_APP, ft.dwLowDateTime, ft.dwHighDateTime, SMTO_BLOCK, 5000, &result) && result == TRUE ){
								debug = L"(Proxy) ";
							}
						}
					}
#else
					char cmd[128];
					sprintf_s(cmd, "/usr/bin/sudo -n /bin/date -s %d-%02d-%02dT%02d:%02d:%02dZ",
					          setTime.wYear, setTime.wMonth, setTime.wDay, setTime.wHour, setTime.wMinute, setTime.wSecond);
					if( system(cmd) == 0 ){
						debug = L" ";
					}
#endif
					AddDebugLogFormat(L"★SetSystemTime%ls%d", debug, (int)(delay / I64_1SEC));
					this->epgCapSetTimeSync = true;
				}
			}
		}
		if( isEpgCap == false ){
			//EPG取得中のチューナが無くなったので取得完了
			if( this->epgCapBasicOnlyFlags >= 0 ){
				//EPG取得開始時の設定を書き戻し
				fs_path iniCommonPath = GetCommonIniPath();
				WritePrivateProfileInt(L"SET", L"BSBasicOnly", (this->epgCapBasicOnlyFlags & 1) != 0, iniCommonPath.c_str());
				WritePrivateProfileInt(L"SET", L"CS1BasicOnly", (this->epgCapBasicOnlyFlags & 2) != 0, iniCommonPath.c_str());
				WritePrivateProfileInt(L"SET", L"CS2BasicOnly", (this->epgCapBasicOnlyFlags & 4) != 0, iniCommonPath.c_str());
				WritePrivateProfileInt(L"SET", L"CS3BasicOnly", (this->epgCapBasicOnlyFlags & 8) != 0, iniCommonPath.c_str());
			}
			this->epgCapWork = false;
			doneEpgCap = true;
		}
	}
	this->epgCapRequested = false;
	this->lastCheckEpgCap = now;
	return doneEpgCap;
}

bool CReserveManager::IsActive() const
{
	lock_recursive_mutex lock(this->managerLock);

	if( this->epgCapRequested || this->epgCapWork ||
	    this->batManager.IsWorking() ||
	    this->batPostManager.IsWorkingWithoutNotification() ){
		return true;
	}
	for( auto itr = this->tunerBankMap.cbegin(); itr != this->tunerBankMap.end(); itr++ ){
		if( itr->second->GetState() != CTunerBankCtrl::TR_IDLE ){
			return true;
		}
	}
	return false;
}

LONGLONG CReserveManager::GetSleepReturnTime(LONGLONG baseTime, RESERVE_DATA* reserveData) const
{
	lock_recursive_mutex lock(this->managerLock);

	//最も近い予約開始時刻を得る
	LONGLONG nextRec = LLONG_MAX;
	const RESERVE_DATA* nextReserveData = NULL;
	for( map<DWORD, RESERVE_DATA>::const_iterator itr = this->reserveText.GetMap().begin(); itr != this->reserveText.GetMap().end(); itr++ ){
		if( itr->second.recSetting.IsNoRec() == false ){
			LONGLONG startTime;
			CalcEntireReserveTime(&startTime, NULL, itr->second);
			if( startTime >= baseTime && startTime < nextRec ){
				nextRec = startTime;
				nextReserveData = &itr->second;
			}
		}
	}
	if( reserveData ){
		reserveData->reserveID = 0;
		if( nextReserveData ){
			*reserveData = *nextReserveData;
		}
	}
	LONGLONG capTime = GetNextEpgCapTime(baseTime + 60 * I64_1SEC);
	return min(nextRec, capTime);
}

LONGLONG CReserveManager::GetNearestRecReserveTime() const
{
	lock_recursive_mutex lock(this->managerLock);

	LONGLONG minTime = LLONG_MAX;
	for( map<DWORD, RESERVE_DATA>::const_iterator itr = this->reserveText.GetMap().begin(); itr != this->reserveText.GetMap().end(); itr++ ){
		if( itr->second.recSetting.GetRecMode() != RECMODE_VIEW &&
		    itr->second.recSetting.IsNoRec() == false ){
			LONGLONG startTime;
			CalcEntireReserveTime(&startTime, NULL, itr->second);
			minTime = min(startTime, minTime);
		}
	}
	return minTime;
}

LONGLONG CReserveManager::GetNextEpgCapTime(LONGLONG now, int* basicOnlyFlags) const
{
	lock_recursive_mutex lock(this->managerLock);

	SYSTEMTIME st;
	ConvertSystemTime(now, &st);
	//現在時刻に対する日曜日からのオフセット(分)。秒以下の切り捨てに注意
	int baseTime = st.wDayOfWeek * 1440 + (int)(now / (60 * I64_1SEC) % 1440);
	//baseTimeとの差が最小のEPG取得時刻を探す
	int minDiff = INT_MAX;
	int minVal = 0;
	for( auto itr = this->setting.epgCapTimeList.cbegin(); itr != this->setting.epgCapTimeList.end(); itr++ ){
		if( itr->first ){
			int diff = (itr->second.first + 7 * 1440 - baseTime) % (itr->second.first < 1440 ? 1440 : 7 * 1440);
			if( minDiff > diff ){
				minDiff = diff;
				minVal = itr->second.second;
			}
		}
	}
	if( minDiff == INT_MAX ){
		return LLONG_MAX;
	}
	if( basicOnlyFlags ){
		*basicOnlyFlags = minVal;
	}
	return (now / (60 * I64_1SEC) + minDiff) * (60 * I64_1SEC);
}

bool CReserveManager::IsFindReserve(WORD onid, WORD tsid, WORD sid, WORD eid, DWORD tunerID) const
{
	lock_recursive_mutex lock(this->managerLock);

	const vector<pair<ULONGLONG, DWORD>>& sortList = this->reserveText.GetSortByEventList();

	auto itr = lower_bound_first(sortList.begin(), sortList.end(), Create64PgKey(onid, tsid, sid, eid));
	for( ; itr != sortList.end() && itr->first == Create64PgKey(onid, tsid, sid, eid); itr++ ){
		if( this->setting.separateFixedTuners == false ||
		    this->reserveText.GetMap().find(itr->second)->second.recSetting.tunerID == tunerID ){
			return true;
		}
	}
	return false;
}

vector<DWORD> CReserveManager::GetSupportServiceTuner(WORD onid, WORD tsid, WORD sid) const
{
	//tunerBankMapそのものは排他制御の対象外
	vector<DWORD> idList;
	for( auto itr = this->tunerBankMap.cbegin(); itr != this->tunerBankMap.end(); itr++ ){
		if( itr->second->GetCh(onid, tsid, sid) ){
			idList.push_back(itr->first);
		}
	}
	return idList;
}

bool CReserveManager::GetTunerCh(DWORD tunerID, WORD onid, WORD tsid, WORD sid, int* space, int* ch) const
{
	auto itr = this->tunerBankMap.find(tunerID);
	if( itr != this->tunerBankMap.end() ){
		const CH_DATA4* chData = itr->second->GetCh(onid, tsid, sid);
		if( chData ){
			if( space ){
				*space = chData->space;
			}
			if( ch ){
				*ch = chData->ch;
			}
			return true;
		}
	}
	return false;
}

wstring CReserveManager::GetTunerBonFileName(DWORD tunerID) const
{
	auto itr = this->tunerBankMap.find(tunerID);
	return itr != this->tunerBankMap.end() ? itr->second->GetBonFileName() : wstring();
}

bool CReserveManager::IsOpenTuner(DWORD tunerID) const
{
	lock_recursive_mutex lock(this->managerLock);

	auto itr = this->tunerBankMap.find(tunerID);
	return itr != this->tunerBankMap.end() && itr->second->GetState() != CTunerBankCtrl::TR_IDLE;
}

pair<bool, int> CReserveManager::OpenNWTV(int id, bool nwUdp, bool nwTcp, WORD onid, WORD tsid, WORD sid, const vector<DWORD>& tunerIDList)
{
	lock_recursive_mutex lock(this->managerLock);

	SET_CH_INFO chInfo = {};
	chInfo.useSID = TRUE;
	chInfo.ONID = onid;
	chInfo.TSID = tsid;
	chInfo.SID = sid;
	for( auto itr = this->tunerBankMap.cbegin(); itr != this->tunerBankMap.end(); itr++ ){
		if( itr->second->GetState() == CTunerBankCtrl::TR_NWTV && itr->second->GetNWTVID() == id ){
			//すでに起動しているので使えたら使う
			if( itr->second->GetCh(chInfo.ONID, chInfo.TSID, chInfo.SID) ){
				itr->second->OpenNWTV(id, nwUdp, nwTcp, chInfo);
				return std::make_pair(true, itr->second->GetProcessID());
			}
			itr->second->CloseNWTV();
			break;
		}
	}
	for( size_t i = 0; i < tunerIDList.size(); i++ ){
		auto itr = this->tunerBankMap.find(tunerIDList[i]);
		if( itr != this->tunerBankMap.end() && itr->second->GetCh(chInfo.ONID, chInfo.TSID, chInfo.SID) ){
			//別IDのネットワークモードを邪魔しない
			if( itr->second->GetState() != CTunerBankCtrl::TR_NWTV &&
			    itr->second->OpenNWTV(id, nwUdp, nwTcp, chInfo) ){
				return std::make_pair(true, itr->second->GetProcessID());
			}
		}
	}
	return std::make_pair(false, 0);
}

pair<bool, int> CReserveManager::IsOpenNWTV(int id) const
{
	lock_recursive_mutex lock(this->managerLock);

	for( auto itr = this->tunerBankMap.cbegin(); itr != this->tunerBankMap.end(); itr++ ){
		if( itr->second->GetState() == CTunerBankCtrl::TR_NWTV && itr->second->GetNWTVID() == id ){
			return std::make_pair(true, itr->second->GetProcessID());
		}
	}
	return std::make_pair(false, 0);
}

bool CReserveManager::CloseNWTV(int id)
{
	lock_recursive_mutex lock(this->managerLock);

	for( auto itr = this->tunerBankMap.cbegin(); itr != this->tunerBankMap.end(); itr++ ){
		if( itr->second->GetState() == CTunerBankCtrl::TR_NWTV && itr->second->GetNWTVID() == id ){
			itr->second->CloseNWTV();
			return true;
		}
	}
	return false;
}

vector<pair<DWORD, int>> CReserveManager::GetNWTVIDAll() const
{
	lock_recursive_mutex lock(this->managerLock);

	vector<pair<DWORD, int>> idList;
	for( auto itr = this->tunerBankMap.cbegin(); itr != this->tunerBankMap.end(); itr++ ){
		if( itr->second->GetState() == CTunerBankCtrl::TR_NWTV ){
			idList.push_back(std::make_pair(itr->first, itr->second->GetNWTVID()));
		}
	}
	return idList;
}

bool CReserveManager::GetRecFilePath(DWORD reserveID, wstring& filePath) const
{
	lock_recursive_mutex lock(this->managerLock);

	for( auto itr = this->tunerBankMap.cbegin(); itr != this->tunerBankMap.end(); itr++ ){
		if( itr->second->GetRecFilePath(reserveID, filePath) ){
			return true;
		}
	}
	return false;
}

bool CReserveManager::IsFindRecEventInfo(const EPGDB_EVENT_INFO& info, WORD chkDay) const
{
	lock_recursive_mutex lock(this->managerLock);
	bool ret = false;

#if !defined(EPGDB_STD_WREGEX) && defined(_WIN32)
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	void* pv;
	if( SUCCEEDED(CoCreateInstance(CLSID_RegExp, NULL, CLSCTX_INPROC_SERVER, IID_IRegExp, &pv)) ){
		CEpgDBManager::RegExpPtr regExp((IRegExp*)pv, CEpgDBManager::ComRelease);
#else
	{
		std::wregex re;
#endif
		if( info.hasShortInfo ){
			wstring infoEventName = info.shortInfo.event_name;
			if( this->setting.recInfo2RegExp.empty() == false ){
#if !defined(EPGDB_STD_WREGEX) && defined(_WIN32)
				CEpgDBManager::OleCharPtr pattern(SysAllocString(this->setting.recInfo2RegExp.c_str()), SysFreeString);
				CEpgDBManager::OleCharPtr rplFrom(SysAllocString(infoEventName.c_str()), SysFreeString);
				CEpgDBManager::OleCharPtr rplTo(SysAllocString(L""), SysFreeString);
				BSTR rpl_;
				if( pattern && rplFrom && rplTo &&
				    SUCCEEDED(regExp->put_Global(VARIANT_TRUE)) &&
				    SUCCEEDED(regExp->put_Pattern(pattern.get())) &&
				    SUCCEEDED(regExp->Replace(rplFrom.get(), rplTo.get(), &rpl_)) ){
					CEpgDBManager::OleCharPtr rpl(rpl_, SysFreeString);
					infoEventName = SysStringLen(rpl.get()) ? rpl.get() : L"";
				}else{
#else
				try{
					re.assign(this->setting.recInfo2RegExp);
					infoEventName = std::regex_replace(infoEventName, re, wstring());
				}catch( std::regex_error& ){
#endif
					AddDebugLog(L"RecInfo2RegExp seems ill-formed");
					infoEventName = L"";
				}
			}
			if( infoEventName.empty() == false && info.StartTimeFlag != 0 ){
				int chkDayActual = chkDay >= 20000 ? chkDay % 10000 : chkDay;
				map<DWORD, PARSE_REC_INFO2_ITEM>::const_iterator itr;
				for( itr = this->recInfo2Text.GetMap().begin(); itr != this->recInfo2Text.GetMap().end(); itr++ ){
					if( (chkDay >= 40000 || itr->second.originalNetworkID == info.original_network_id) &&
					    (chkDay >= 30000 || itr->second.transportStreamID == info.transport_stream_id) &&
					    (chkDay >= 20000 || itr->second.serviceID == info.service_id) &&
					    ConvertI64Time(itr->second.startTime) + chkDayActual*24*60*60*I64_1SEC > ConvertI64Time(info.start_time) ){
						wstring eventName = itr->second.eventName;
						if( this->setting.recInfo2RegExp.empty() == false ){
#if !defined(EPGDB_STD_WREGEX) && defined(_WIN32)
							CEpgDBManager::OleCharPtr rplFrom(SysAllocString(eventName.c_str()), SysFreeString);
							CEpgDBManager::OleCharPtr rplTo(SysAllocString(L""), SysFreeString);
							BSTR rpl_;
							if( rplFrom && rplTo && SUCCEEDED(regExp->Replace(rplFrom.get(), rplTo.get(), &rpl_)) ){
								CEpgDBManager::OleCharPtr rpl(rpl_, SysFreeString);
								eventName = SysStringLen(rpl.get()) ? rpl.get() : L"";
							}else{
#else
							try{
								eventName = std::regex_replace(eventName, re, wstring());
							}catch( std::regex_error& ){
#endif
								eventName = L"";
							}
						}
						if( infoEventName == eventName ){
							ret = true;
							break;
						}
					}
				}
			}
		}
	}
#if !defined(EPGDB_STD_WREGEX) && defined(_WIN32)
	CoUninitialize();
#endif

	return ret;
}

bool CReserveManager::ChgAutoAddNoRec(WORD onid, WORD tsid, WORD sid, WORD eid, DWORD tunerID)
{
	lock_recursive_mutex lock(this->managerLock);

	vector<RESERVE_DATA> chgList;
	const vector<pair<ULONGLONG, DWORD>>& sortList = this->reserveText.GetSortByEventList();

	auto itr = lower_bound_first(sortList.begin(), sortList.end(), Create64PgKey(onid, tsid, sid, eid));
	for( ; itr != sortList.end() && itr->first == Create64PgKey(onid, tsid, sid, eid); itr++ ){
		map<DWORD, RESERVE_DATA>::const_iterator itrRes = this->reserveText.GetMap().find(itr->second);
		if( itrRes->second.recSetting.IsNoRec() == false &&
		    itrRes->second.comment.compare(0, 7, L"EPG自動予約") == 0 &&
		    (this->setting.separateFixedTuners == false || itrRes->second.recSetting.tunerID == tunerID) ){
			chgList.push_back(itrRes->second);
			//無効にする
			chgList.back().recSetting.recMode = REC_SETTING_DATA::DIV_RECMODE +
				(chgList.back().recSetting.GetRecMode() + REC_SETTING_DATA::DIV_RECMODE - 1) % REC_SETTING_DATA::DIV_RECMODE;
		}
	}
	return chgList.empty() == false && ChgReserveData(chgList);
}

bool CReserveManager::GetChData(WORD onid, WORD tsid, WORD sid, CH_DATA5* chData) const
{
	lock_recursive_mutex lock(this->managerLock);

	map<LONGLONG, CH_DATA5>::const_iterator itr = this->chUtil.GetMap().find(Create64Key(onid, tsid, sid));
	if( itr != this->chUtil.GetMap().end() ){
		*chData = itr->second;
		return true;
	}
	return false;
}

vector<CH_DATA5> CReserveManager::GetChDataList() const
{
	lock_recursive_mutex lock(this->managerLock);

	vector<CH_DATA5> list;
	list.reserve(this->chUtil.GetMap().size());
	for( map<LONGLONG, CH_DATA5>::const_iterator itr = this->chUtil.GetMap().begin(); itr != this->chUtil.GetMap().end(); itr++ ){
		list.push_back(itr->second);
	}
	return list;
}

bool CReserveManager::GetChDataListAsText(string& text) const
{
	lock_recursive_mutex lock(this->managerLock);

	return this->chUtil.SaveTextWithExtraFields(&text);
}

void CReserveManager::WatchdogThread(CReserveManager* sys)
{
	while( sys->watchdogStopEvent.WaitOne(2000) == false ){
		for( auto itr = sys->tunerBankMap.cbegin(); itr != sys->tunerBankMap.end(); itr++ ){
			itr->second->Watch();
		}
	}
}

void CReserveManager::AddPostBatWork(vector<CBatManager::BAT_WORK_INFO>& workList, LPCWSTR baseName)
{
	if( workList.empty() == false ){
		workList[0].batFilePath = this->batPostManager.FindExistingPath(GetCommonIniPath().replace_filename(baseName).c_str());
		if( workList[0].batFilePath.empty() == false ){
			for( size_t i = 0; i < workList.size(); i++ ){
				workList[i].batFilePath = workList[0].batFilePath;
				this->batPostManager.AddBatWork(workList[i]);
			}
		}
	}
}

void CReserveManager::AddNotifyAndPostBat(DWORD notifyID)
{
	this->notifyManager.AddNotify(notifyID);
	vector<CBatManager::BAT_WORK_INFO> workList(1);
	workList[0].macroList.push_back(pair<string, wstring>("NotifyID", L""));
	Format(workList[0].macroList.back().second, L"%d", notifyID);
	AddPostBatWork(workList, L"PostNotify");
}

void CReserveManager::SetBatCustomHandler(LPCWSTR ext, const std::function<void(CBatManager::BAT_WORK_INFO&, vector<char>&)>& handler)
{
	this->batManager.SetCustomHandler(ext, handler);
	this->batPostManager.SetCustomHandler(ext, handler);
}

void CReserveManager::AddTimeMacro(vector<pair<string, wstring>>& macroList, const SYSTEMTIME& startTime, DWORD durationSecond, LPCSTR suffix)
{
	WCHAR v[64];
	swprintf_s(v, L"%04d-%02d-%02dT%02d:%02d:%02d%lc%02d:%02d",
	           startTime.wYear, startTime.wMonth, startTime.wDay, startTime.wHour, startTime.wMinute, startTime.wSecond,
	           (I64_UTIL_TIMEZONE < 0 ? L'-' : L'+'),
	           (int)((I64_UTIL_TIMEZONE < 0 ? -I64_UTIL_TIMEZONE : I64_UTIL_TIMEZONE) / I64_1SEC) / 3600,
	           (int)((I64_UTIL_TIMEZONE < 0 ? -I64_UTIL_TIMEZONE : I64_UTIL_TIMEZONE) / I64_1SEC) / 60 % 60);
	macroList.push_back(pair<string, wstring>(string("StartTime") + suffix, v));
	for( string p = "S"; p != ""; p = (p == "S" ? "E" : "") ){
		SYSTEMTIME t = startTime;
		if( p == "E" ){
			ConvertSystemTime(ConvertI64Time(t) + durationSecond * I64_1SEC, &t);
		}
		for( int i = 0; GetTimeMacroName(i); i++ ){
			//従来形式は#でコメントアウトしておく
			macroList.push_back(std::make_pair('#' + p + GetTimeMacroName(i) + suffix, GetTimeMacroValue(i, t)));
		}
	}
	swprintf_s(v, L"%u", durationSecond);				macroList.push_back(pair<string, wstring>(string("DurationSecond") + suffix, v));
	swprintf_s(v, L"%02d", durationSecond / 3600);		macroList.push_back(pair<string, wstring>(string("#DUHH") + suffix, v));
	swprintf_s(v, L"%d", durationSecond / 3600);		macroList.push_back(pair<string, wstring>(string("#DUH") + suffix, v));
	swprintf_s(v, L"%02d", durationSecond % 3600 / 60);	macroList.push_back(pair<string, wstring>(string("#DUMM") + suffix, v));
	swprintf_s(v, L"%d", durationSecond % 3600 / 60);	macroList.push_back(pair<string, wstring>(string("#DUM") + suffix, v));
	swprintf_s(v, L"%02d", durationSecond % 60);		macroList.push_back(pair<string, wstring>(string("#DUSS") + suffix, v));
	swprintf_s(v, L"%d", durationSecond % 60);			macroList.push_back(pair<string, wstring>(string("#DUS") + suffix, v));
}

void CReserveManager::AddReserveDataMacro(vector<pair<string, wstring>>& macroList, const RESERVE_DATA& data, LPCSTR suffix)
{
	WCHAR v[64];
	AddTimeMacro(macroList, data.startTime, data.durationSecond, suffix);
	swprintf_s(v, L"%d", data.originalNetworkID);	macroList.push_back(pair<string, wstring>(string("ONID10") + suffix, v));
	swprintf_s(v, L"%d", data.transportStreamID);	macroList.push_back(pair<string, wstring>(string("TSID10") + suffix, v));
	swprintf_s(v, L"%d", data.serviceID);			macroList.push_back(pair<string, wstring>(string("SID10") + suffix, v));
	swprintf_s(v, L"%d", data.eventID);				macroList.push_back(pair<string, wstring>(string("EID10") + suffix, v));
	swprintf_s(v, L"%04X", data.originalNetworkID);	macroList.push_back(pair<string, wstring>(string("ONID16") + suffix, v));
	swprintf_s(v, L"%04X", data.transportStreamID);	macroList.push_back(pair<string, wstring>(string("TSID16") + suffix, v));
	swprintf_s(v, L"%04X", data.serviceID);			macroList.push_back(pair<string, wstring>(string("SID16") + suffix, v));
	swprintf_s(v, L"%04X", data.eventID);			macroList.push_back(pair<string, wstring>(string("EID16") + suffix, v));
	swprintf_s(v, L"%d", data.reserveID);			macroList.push_back(pair<string, wstring>(string("ReserveID") + suffix, v));
	swprintf_s(v, L"%d", data.recSetting.recMode);	macroList.push_back(pair<string, wstring>(string("RecMode") + suffix, v));
	macroList.push_back(std::make_pair(string("Title") + suffix, data.title));
	macroList.push_back(std::make_pair(string("ServiceName") + suffix, data.stationName));
	macroList.push_back(std::make_pair(string("ReserveComment") + suffix, data.comment));
	macroList.push_back(std::make_pair(string("BatFileTag") + suffix,
		data.recSetting.batFilePath.find(L'*') != wstring::npos ?
		data.recSetting.batFilePath.substr(data.recSetting.batFilePath.find(L'*') + 1) : wstring()));
}

void CReserveManager::AddRecInfoMacro(vector<pair<string, wstring>>& macroList, const REC_FILE_INFO& recInfo)
{
	WCHAR v[64];
	AddTimeMacro(macroList, recInfo.startTime, recInfo.durationSecond, "");
	swprintf_s(v, L"%d", recInfo.id);					macroList.push_back(pair<string, wstring>("RecInfoID", v));
	swprintf_s(v, L"%d", recInfo.originalNetworkID);	macroList.push_back(pair<string, wstring>("ONID10", v));
	swprintf_s(v, L"%d", recInfo.transportStreamID);	macroList.push_back(pair<string, wstring>("TSID10", v));
	swprintf_s(v, L"%d", recInfo.serviceID);			macroList.push_back(pair<string, wstring>("SID10", v));
	swprintf_s(v, L"%d", recInfo.eventID);				macroList.push_back(pair<string, wstring>("EID10", v));
	swprintf_s(v, L"%04X", recInfo.originalNetworkID);	macroList.push_back(pair<string, wstring>("ONID16", v));
	swprintf_s(v, L"%04X", recInfo.transportStreamID);	macroList.push_back(pair<string, wstring>("TSID16", v));
	swprintf_s(v, L"%04X", recInfo.serviceID);			macroList.push_back(pair<string, wstring>("SID16", v));
	swprintf_s(v, L"%04X", recInfo.eventID);			macroList.push_back(pair<string, wstring>("EID16", v));
	swprintf_s(v, L"%lld", recInfo.drops);				macroList.push_back(pair<string, wstring>("Drops", v));
	swprintf_s(v, L"%lld", recInfo.scrambles);			macroList.push_back(pair<string, wstring>("Scrambles", v));
	macroList.push_back(pair<string, wstring>("Title", recInfo.title));
	macroList.push_back(pair<string, wstring>("ServiceName", recInfo.serviceName));
	macroList.push_back(pair<string, wstring>("Result", recInfo.GetComment()));
	macroList.push_back(pair<string, wstring>("FilePath", recInfo.recFilePath));
	fs_path path = recInfo.recFilePath;
	macroList.push_back(pair<string, wstring>("FolderPath", path.parent_path().native()));
	macroList.push_back(pair<string, wstring>("FileName", path.stem().native()));
	macroList.push_back(pair<string, wstring>("TitleF", recInfo.title));
	CheckFileName(macroList.back().second);
	macroList.push_back(pair<string, wstring>("Title2", recInfo.title));
	while( macroList.back().second.find(L'[') != wstring::npos && macroList.back().second.find(L']') != wstring::npos ){
		wstring strSep1;
		wstring strSep2;
		Separate(macroList.back().second, L"[", macroList.back().second, strSep1);
		Separate(strSep1, L"]", strSep2, strSep1);
		macroList.back().second += strSep1;
	}
	macroList.push_back(pair<string, wstring>("Title2F", macroList.back().second));
	CheckFileName(macroList.back().second);
}
