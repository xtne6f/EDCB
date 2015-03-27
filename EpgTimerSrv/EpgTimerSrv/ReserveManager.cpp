#include "StdAfx.h"
#include "ReserveManager.h"
#include <process.h>
#include "../../Common/PathUtil.h"
#include "../../Common/ReNamePlugInUtil.h"
#include "../../Common/EpgTimerUtil.h"
#include "../../Common/TimeUtil.h"
#include "../../Common/BlockLock.h"

CReserveManager::CReserveManager(CNotifyManager& notifyManager_, CEpgDBManager& epgDBManager_)
	: notifyManager(notifyManager_)
	, epgDBManager(epgDBManager_)
	, batManager(notifyManager_)
	, waitCount(0)
	, epgCapRequested(false)
	, epgCapWork(false)
	, reserveModified(false)
	, watchdogStopEvent(NULL)
	, watchdogThread(NULL)
{
	InitializeCriticalSection(&this->managerLock);
}

CReserveManager::~CReserveManager(void)
{
	DeleteCriticalSection(&this->managerLock);
}

void CReserveManager::Initialize()
{
	this->tunerManager.ReloadTuner();
	this->tunerManager.GetEnumTunerBank(&this->tunerBankMap, this->notifyManager, this->epgDBManager);
	this->waitTick = GetTickCount();
	this->lastCheckEpgCap = GetNowI64Time();

	wstring settingPath;
	GetSettingPath(settingPath);
	this->reserveText.ParseText((settingPath + L"\\" + RESERVE_TEXT_NAME).c_str());
	this->recInfoText.ParseText((settingPath + L"\\" + REC_INFO_TEXT_NAME).c_str());
	this->recInfo2Text.ParseText((settingPath + L"\\" + REC_INFO2_TEXT_NAME).c_str());

	ReloadSetting();

	this->watchdogStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if( this->watchdogStopEvent ){
		this->watchdogThread = (HANDLE)_beginthreadex(NULL, 0, WatchdogThread, this, 0, NULL);
	}
}

void CReserveManager::Finalize()
{
	if( this->watchdogThread ){
		SetEvent(this->watchdogStopEvent);
		if( WaitForSingleObject(this->watchdogThread, 15000) == WAIT_TIMEOUT ){
			TerminateThread(this->watchdogThread, 0xffffffff);
		}
		CloseHandle(this->watchdogThread);
		this->watchdogThread = NULL;
	}
	if( this->watchdogStopEvent ){
		CloseHandle(this->watchdogStopEvent);
		this->watchdogStopEvent = NULL;
	}
	for( map<DWORD, CTunerBankCtrl*>::iterator itr = this->tunerBankMap.begin(); itr != this->tunerBankMap.end(); itr++ ){
		SAFE_DELETE(itr->second);
	}
}

void CReserveManager::ReloadSetting()
{
	CBlockLock lock(&this->managerLock);

	wstring iniPath;
	GetModuleIniPath(iniPath);
	wstring commonIniPath;
	GetCommonIniPath(commonIniPath);
	wstring viewIniPath;
	GetModuleFolderPath(viewIniPath);
	viewIniPath += L"\\EpgDataCap_Bon.ini";
	wstring settingPath;
	GetSettingPath(settingPath);
	WCHAR buff[1024];

	this->chUtil.ParseText((settingPath + L"\\ChSet5.txt").c_str());

	this->ngCapTimeSec = GetPrivateProfileInt(L"SET", L"NGEpgCapTime", 20, iniPath.c_str()) * 60;
	this->ngCapTunerTimeSec = GetPrivateProfileInt(L"SET", L"NGEpgCapTunerTime", 20, iniPath.c_str()) * 60;
	this->epgCapTimeSync = GetPrivateProfileInt(L"SET", L"TimeSync", 0, iniPath.c_str()) != 0;
	this->epgCapTimeList.clear();
	int count = GetPrivateProfileInt(L"EPG_CAP", L"Count", 0, iniPath.c_str());
	for( int i = 0; i < count; i++ ){
		WCHAR key[64];
		wsprintf(key, L"%dSelect", i);
		if( GetPrivateProfileInt(L"EPG_CAP", key, 0, iniPath.c_str()) != 0 ){
			wsprintf(key, L"%d", i);
			GetPrivateProfileString(L"EPG_CAP", key, L"", buff, 256, iniPath.c_str());
			//曜日指定接尾辞(w1=Mon,...,w7=Sun)
			unsigned int hour, minute, wday = 0;
			if( swscanf_s(buff, L"%u:%uw%u", &hour, &minute, &wday) >= 2 ){
				//取得種別(bit0(LSB)=BS,bit1=CS1,bit2=CS2)。負値のときは共通設定に従う
				wsprintf(key, L"%dBasicOnlyFlags", i);
				int basicOnlyFlags = GetPrivateProfileInt(L"EPG_CAP", key, -1, iniPath.c_str());
				basicOnlyFlags = basicOnlyFlags < 0 ? 0xFF : basicOnlyFlags & 7;
				if( wday == 0 ){
					//曜日指定なし
					for( int j = 0; j < 7; j++ ){
						this->epgCapTimeList.push_back(MAKELONG(((j * 24 + hour) * 60 + minute) % 10080, basicOnlyFlags));
					}
				}else{
					this->epgCapTimeList.push_back(MAKELONG(((wday * 24 + hour) * 60 + minute) % 10080, basicOnlyFlags));
				}
			}
		}
	}

	this->defStartMargin = GetPrivateProfileInt(L"SET", L"StartMargin", 5, iniPath.c_str());
	this->defEndMargin = GetPrivateProfileInt(L"SET", L"EndMargin", 2, iniPath.c_str());
	this->backPriority = GetPrivateProfileInt(L"SET", L"BackPriority", 1, iniPath.c_str()) != 0;

	this->recInfoText.SetKeepCount(
		GetPrivateProfileInt(L"SET", L"AutoDelRecInfo", 0, iniPath.c_str()) == 0 ? UINT_MAX :
		GetPrivateProfileInt(L"SET", L"AutoDelRecInfoNum", 100, iniPath.c_str()));
	this->recInfoText.SetRecInfoDelFile(GetPrivateProfileInt(L"SET", L"RecInfoDelFile", 0, commonIniPath.c_str()) != 0);
	GetPrivateProfileString(L"SET", L"RecInfoFolder", L"", buff, 512, commonIniPath.c_str());
	this->recInfoText.SetRecInfoFolder(buff);

	this->recInfo2Text.SetKeepCount(GetPrivateProfileInt(L"SET", L"RecInfo2Max", 1000, iniPath.c_str()));
	this->recInfo2DropChk = GetPrivateProfileInt(L"SET", L"RecInfo2DropChk", 15, iniPath.c_str());
	GetPrivateProfileString(L"SET", L"RecInfo2RegExp", L"", buff, 1024, iniPath.c_str());
	this->recInfo2RegExp = buff;

	this->defEnableCaption = GetPrivateProfileInt(L"SET", L"Caption", 1, viewIniPath.c_str()) != 0;
	this->defEnableData = GetPrivateProfileInt(L"SET", L"Data", 0, viewIniPath.c_str()) != 0;

	this->recNamePlugInFileName.clear();
	if( GetPrivateProfileInt(L"SET", L"RecNamePlugIn", 0, iniPath.c_str()) != 0 ){
		GetPrivateProfileString(L"SET", L"RecNamePlugInFile", L"RecName_Macro.dll", buff, 512, iniPath.c_str());
		this->recNamePlugInFileName = buff;
	}
	this->recNameNoChkYen = GetPrivateProfileInt(L"SET", L"NoChkYen", 0, iniPath.c_str()) != 0;

	for( map<DWORD, CTunerBankCtrl*>::const_iterator itr = this->tunerBankMap.begin(); itr != this->tunerBankMap.end(); itr++ ){
		itr->second->ReloadSetting();
	}
	ReloadBankMap();
}

vector<RESERVE_DATA> CReserveManager::GetReserveDataAll(bool getRecFileName) const
{
	CBlockLock lock(&this->managerLock);

	vector<RESERVE_DATA> list;
	list.reserve(this->reserveText.GetMap().size());
	for( map<DWORD, RESERVE_DATA>::const_iterator itr = this->reserveText.GetMap().begin(); itr != this->reserveText.GetMap().end(); itr++ ){
		list.resize(list.size() + 1);
		GetReserveData(itr->first, &list.back(), getRecFileName);
	}
	return list;
}

vector<TUNER_RESERVE_INFO> CReserveManager::GetTunerReserveAll() const
{
	CBlockLock lock(&this->managerLock);

	vector<TUNER_RESERVE_INFO> list;
	list.reserve(this->tunerBankMap.size() + 1);
	for( map<DWORD, CTunerBankCtrl*>::const_iterator itr = this->tunerBankMap.begin(); itr != this->tunerBankMap.end(); itr++ ){
		list.resize(list.size() + 1);
		list.back().tunerID = itr->first;
		this->tunerManager.GetBonFileName(itr->first, list.back().tunerName);
		list.back().reserveList = itr->second->GetReserveIDList();
	}
	list.resize(list.size() + 1);
	list.back().tunerID = 0xFFFFFFFF;
	list.back().tunerName = L"チューナー不足";
	vector<DWORD> &ngList = list.back().reserveList = GetNoTunerReserveAll();
	for( size_t i = 0; i < ngList.size(); i++ ){
		//無効予約は「チューナ不足」ではない
		if( this->reserveText.GetMap().find(ngList[i])->second.recSetting.recMode == RECMODE_NO ){
			ngList.erase(ngList.begin() + i);
		}
	}
	return list;
}

vector<DWORD> CReserveManager::GetNoTunerReserveAll() const
{
	CBlockLock lock(&this->managerLock);

	vector<DWORD> list;
	list.reserve(this->reserveText.GetMap().size());
	for( map<DWORD, RESERVE_DATA>::const_iterator itr = this->reserveText.GetMap().begin(); itr != this->reserveText.GetMap().end(); itr++ ){
		list.push_back(itr->first);
	}
	//全予約からバンクに存在する予約を引く
	for( map<DWORD, CTunerBankCtrl*>::const_iterator itr = this->tunerBankMap.begin(); itr != this->tunerBankMap.end(); itr++ ){
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

bool CReserveManager::GetReserveData(DWORD id, RESERVE_DATA* reserveData, bool getRecFileName) const
{
	CBlockLock lock(&this->managerLock);

	map<DWORD, RESERVE_DATA>::const_iterator itr = this->reserveText.GetMap().find(id);
	if( itr != this->reserveText.GetMap().end() ){
		*reserveData = itr->second;
		reserveData->recFileNameList.clear();
		if( getRecFileName ){
			const vector<REC_FILE_SET_INFO>& folderList = reserveData->recSetting.recFolderList;
			vector<wstring>& nameList = reserveData->recFileNameList;
			if( folderList.empty() ){
				nameList.push_back(this->recNamePlugInFileName);
			}else{
				for( size_t i = 0; i < folderList.size(); i++ ){
					nameList.push_back(folderList[i].recNamePlugIn.empty() ? this->recNamePlugInFileName : folderList[i].recNamePlugIn);
				}
			}
			//recNamePlugInを展開して実ファイル名をセット
			for( size_t i = 0; i < nameList.size(); i++ ){
				if( nameList[i].empty() == false ){
					CReNamePlugInUtil plugIn;
					wstring plugInPath;
					GetModuleFolderPath(plugInPath);
					plugInPath += L"\\RecName\\" + nameList[i];
					nameList[i].clear();
					if( plugIn.Initialize(plugInPath.c_str()) != FALSE ){
						PLUGIN_RESERVE_INFO info;
						info.startTime = reserveData->startTime;
						info.durationSec = reserveData->durationSecond;
						wcscpy_s(info.eventName, reserveData->title.c_str());
						info.ONID = reserveData->originalNetworkID;
						info.TSID = reserveData->transportStreamID;
						info.SID = reserveData->serviceID;
						info.EventID = reserveData->eventID;
						wcscpy_s(info.serviceName, reserveData->stationName.c_str());
						//TODO: チューナに関する情報をセット
						wcscpy_s(info.bonDriverName, L"チューナー不明");
						info.bonDriverID = 0xFFFF;
						info.tunerID = 0xFFFF;
						EPG_EVENT_INFO* epgInfo = NULL;
						if( info.EventID != 0xFFFF ){
							EPGDB_EVENT_INFO epgDBInfo;
							if( this->epgDBManager.SearchEpg(info.ONID, info.TSID, info.SID, info.EventID, &epgDBInfo) != FALSE ){
								epgInfo = new EPG_EVENT_INFO;
								CopyEpgInfo(epgInfo, &epgDBInfo);
							}
						}
						WCHAR name[512];
						DWORD size = 512;
						//ConvertRecName()は呼ばない(epgInfo==NULLの場合と等価なので)
						if( plugIn.ConvertRecName2(&info, epgInfo, name, &size) != FALSE ){
							nameList[i] = name;
							CheckFileName(nameList[i], this->recNameNoChkYen);
						}
						delete epgInfo;
					}
				}
				//実ファイル名は空にしない
				if( nameList[i].empty() ){
					SYSTEMTIME st = reserveData->startTime;
					Format(nameList[i], L"%04d%02d%02d%02d%02dFFFFFFFF0-%s.ts",
					       st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, reserveData->title.c_str());
					CheckFileName(nameList[i]);
				}
			}
		}
		return true;
	}
	return false;
}

bool CReserveManager::AddReserveData(const vector<RESERVE_DATA>& reserveList, bool setComment)
{
	CBlockLock lock(&this->managerLock);

	bool modified = false;
	__int64 minStartTime = LLONG_MAX;
	__int64 now = GetNowI64Time();
	for( size_t i = 0; i < reserveList.size(); i++ ){
		RESERVE_DATA r = reserveList[i];
		//すでに終了していないか
		if( now < ConvertI64Time(r.startTime) + r.durationSecond * I64_1SEC ){
			if( setComment == false ){
				r.comment.clear();
			}
			r.overlapMode = RESERVE_EXECUTE;
			r.reserveStatus = 0;
			r.recFileNameList.clear();
			r.reserveID = this->reserveText.AddReserve(r);
			this->reserveTextCache.clear();
			this->reserveModified = true;
			modified = true;
			if( r.recSetting.recMode != RECMODE_NO ){
				__int64 startTime;
				CalcEntireReserveTime(&startTime, NULL, r);
				minStartTime = min(startTime, minStartTime);
			}
		}
	}
	if( modified ){
		this->reserveText.SaveText();
		ReloadBankMap(minStartTime);
		this->notifyManager.AddNotify(NOTIFY_UPDATE_RESERVE_INFO);
		return true;
	}
	return false;
}

bool CReserveManager::ChgReserveData(const vector<RESERVE_DATA>& reserveList, bool setReserveStatus)
{
	CBlockLock lock(&this->managerLock);

	bool modified = false;
	__int64 minStartTime = LLONG_MAX;
	for( size_t i = 0; i < reserveList.size(); i++ ){
		RESERVE_DATA r = reserveList[i];
		map<DWORD, RESERVE_DATA>::const_iterator itr = this->reserveText.GetMap().find(r.reserveID);
		if( itr != this->reserveText.GetMap().end() ){
			//変更できないフィールドを上書き
			r.comment = itr->second.comment;
			r.startTimeEpg = itr->second.startTimeEpg;
			if( setReserveStatus == false ){
				r.reserveStatus = itr->second.reserveStatus;
			}
			r.recFileNameList.clear();

			if( r.recSetting.recMode == RECMODE_NO ){
				if( itr->second.recSetting.recMode != RECMODE_NO ){
					//バンクから削除
					for( map<DWORD, CTunerBankCtrl*>::const_iterator jtr = this->tunerBankMap.begin(); jtr != this->tunerBankMap.end(); jtr++ ){
						if( jtr->second->DelReserve(r.reserveID) ){
							break;
						}
					}
					r.overlapMode = RESERVE_EXECUTE;
					__int64 startTime;
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
				tr.recMode = r.recSetting.recMode;
				tr.priority = r.recSetting.priority;
				bool enableCaption = tr.enableCaption =
					r.recSetting.serviceMode & RECSERVICEMODE_SET ? (r.recSetting.serviceMode & RECSERVICEMODE_CAP) != 0 : this->defEnableCaption;
				bool enableData = tr.enableData =
					r.recSetting.serviceMode & RECSERVICEMODE_SET ? (r.recSetting.serviceMode & RECSERVICEMODE_DATA) != 0 : this->defEnableData;
				tr.pittari = r.recSetting.pittariFlag != 0;
				tr.partialRecMode = r.recSetting.partialRecFlag;
				__int64 startTime, endTime;
				CalcEntireReserveTime(&startTime, &endTime, r);
				tr.startTime = ConvertI64Time(r.startTime);
				tr.durationSecond = r.durationSecond;
				__int64 startMargin = tr.startMargin = tr.startTime - startTime;
				__int64 endMargin = tr.endMargin = endTime - (tr.startTime + tr.durationSecond * I64_1SEC);
				tr.recFolder = r.recSetting.recFolderList;
				tr.partialRecFolder = r.recSetting.partialRecFolder;

				bool bankDeleted = false;
				map<DWORD, CTunerBankCtrl*>::const_iterator jtr;
				for( jtr = this->tunerBankMap.begin(); jtr != this->tunerBankMap.end(); jtr++ ){
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
							r.recSetting.recMode = tr.recMode;
							r.recSetting.priority = tr.priority;
							if( tr.enableCaption != enableCaption || tr.enableData != enableData ){
								r.recSetting.serviceMode = 0;
								r.recSetting.serviceMode |= tr.enableCaption ? RECSERVICEMODE_SET | RECSERVICEMODE_CAP : 0;
								r.recSetting.serviceMode |= tr.enableData ? RECSERVICEMODE_SET | RECSERVICEMODE_DATA : 0;
							}
							r.recSetting.pittariFlag = tr.pittari;
							r.recSetting.partialRecFlag = tr.partialRecMode;
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
				    r.recSetting.startMargine != itr->second.recSetting.startMargine ||
				    r.recSetting.endMargine != itr->second.recSetting.endMargine ||
				    r.recSetting.tunerID != itr->second.recSetting.tunerID ){
					__int64 startTime, startTimeNext;
					CalcEntireReserveTime(&startTime, NULL, itr->second);
					CalcEntireReserveTime(&startTimeNext, NULL, r);
					minStartTime = min(min(startTime, startTimeNext), minStartTime);
				}
			}
			this->reserveText.ChgReserve(r);
			this->reserveTextCache.clear();
			this->reserveModified = true;
			modified = true;
		}
	}
	if( modified ){
		this->reserveText.SaveText();
		ReloadBankMap(minStartTime);
		this->notifyManager.AddNotify(NOTIFY_UPDATE_RESERVE_INFO);
		return true;
	}
	return false;
}

void CReserveManager::DelReserveData(const vector<DWORD>& idList)
{
	CBlockLock lock(&this->managerLock);

	bool modified = false;
	__int64 minStartTime = LLONG_MAX;
	for( size_t i = 0; i < idList.size(); i++ ){
		map<DWORD, RESERVE_DATA>::const_iterator itr = this->reserveText.GetMap().find(idList[i]);
		if( itr != this->reserveText.GetMap().end() ){
			if( itr->second.recSetting.recMode != RECMODE_NO ){
				//バンクから削除
				for( map<DWORD, CTunerBankCtrl*>::const_iterator jtr = this->tunerBankMap.begin(); jtr != this->tunerBankMap.end(); jtr++ ){
					if( jtr->second->DelReserve(idList[i]) ){
						break;
					}
				}
				__int64 startTime;
				CalcEntireReserveTime(&startTime, NULL, itr->second);
				minStartTime = min(startTime, minStartTime);
			}
			this->reserveText.DelReserve(idList[i]);
			this->reserveTextCache.clear();
			this->reserveModified = true;
			modified = true;
		}
	}
	if( modified ){
		this->reserveText.SaveText();
		ReloadBankMap(minStartTime);
		this->notifyManager.AddNotify(NOTIFY_UPDATE_RESERVE_INFO);
	}
}

vector<REC_FILE_INFO> CReserveManager::GetRecFileInfoAll() const
{
	CBlockLock lock(&this->managerLock);

	vector<REC_FILE_INFO> infoList;
	infoList.reserve(this->recInfoText.GetMap().size());
	for( map<DWORD, REC_FILE_INFO>::const_iterator itr = this->recInfoText.GetMap().begin(); itr != this->recInfoText.GetMap().end(); itr++ ){
		infoList.push_back(itr->second);
	}
	return infoList;
}

void CReserveManager::DelRecFileInfo(const vector<DWORD>& idList)
{
	CBlockLock lock(&this->managerLock);

	for( size_t i = 0; i < idList.size(); i++ ){
		this->recInfoText.DelRecInfo(idList[i]);
	}
	this->recInfoText.SaveText();
	this->notifyManager.AddNotify(NOTIFY_UPDATE_REC_INFO);
}

void CReserveManager::ChgProtectRecFileInfo(const vector<REC_FILE_INFO>& infoList)
{
	CBlockLock lock(&this->managerLock);

	for( size_t i = 0; i < infoList.size(); i++ ){
		this->recInfoText.ChgProtectRecInfo(infoList[i].id, infoList[i].protectFlag);
	}
	this->recInfoText.SaveText();
	this->notifyManager.AddNotify(NOTIFY_UPDATE_REC_INFO);
}

void CReserveManager::ReloadBankMap(__int64 reloadTime)
{
	CBlockLock lock(&this->managerLock);

	if( reloadTime == LLONG_MAX ){
		return;
	}
	OutputDebugString(L"Start ReloadBankMap\r\n");
	DWORD tick = GetTickCount();

	__int64 boundaryReloadTime = 0;

	//reloadTimeより前の予約を開始時間逆順にソート
	multimap<__int64, const RESERVE_DATA*> sortTimeMap;
	sortTimeMap.insert(std::make_pair(-reloadTime, (RESERVE_DATA*)NULL));
	for( map<DWORD, RESERVE_DATA>::const_iterator itr = this->reserveText.GetMap().begin(); itr != this->reserveText.GetMap().end(); itr++ ){
		if( itr->second.recSetting.recMode != RECMODE_NO ){
			__int64 startTime;
			CalcEntireReserveTime(&startTime, NULL, itr->second);
			if( startTime < reloadTime ){
				sortTimeMap.insert(std::make_pair(-startTime, &itr->second));
			}
		}
	}
	//READY_MARGIN秒以上の無予約時間帯を探す。無予約時間帯より後ろだけを再割り当てすればOK
	for( multimap<__int64, const RESERVE_DATA*>::const_iterator itrRes, itrTime = sortTimeMap.begin(); itrTime != sortTimeMap.end(); itrTime++ ){
		for( (itrRes = itrTime)++; itrRes != sortTimeMap.end(); itrRes++ ){
			__int64 endTime;
			CalcEntireReserveTime(NULL, &endTime, *itrRes->second);
			if( endTime + CTunerBankCtrl::READY_MARGIN * I64_1SEC > -itrTime->first ){
				break;
			}
		}
		if( itrRes == sortTimeMap.end() ){
			boundaryReloadTime = -itrTime->first;
			break;
		}
	}

	//開始済み予約リスト
	vector<pair<DWORD, vector<DWORD>>> startedResList;
	for( map<DWORD, CTunerBankCtrl*>::const_iterator itr = this->tunerBankMap.begin(); itr != this->tunerBankMap.end(); itr++ ){
		//待機状態に入っているもの以外クリア
		itr->second->ClearNoCtrl(boundaryReloadTime);
		startedResList.push_back(std::make_pair(itr->first, itr->second->GetReserveIDList()));
	}

	//boundaryReloadTimeより後の予約を開始時間逆順にソート
	sortTimeMap.clear();
	for( map<DWORD, RESERVE_DATA>::const_iterator itr = this->reserveText.GetMap().begin(); itr != this->reserveText.GetMap().end(); itr++ ){
		if( itr->second.recSetting.recMode != RECMODE_NO ){
			__int64 startTime;
			CalcEntireReserveTime(&startTime, NULL, itr->second);
			if( startTime >= boundaryReloadTime ){
				this->reserveText.SetOverlapMode(itr->first, RESERVE_NO_EXECUTE);
				sortTimeMap.insert(std::make_pair(-startTime, &itr->second));
			}
		}
	}
	//予約を無予約時間帯ごとに組分けしてバンク配置する(組ごとに独立して処理できるので速度や配置安定性が増す)
	for( multimap<__int64, const RESERVE_DATA*>::const_iterator itrRes, itrTime = sortTimeMap.begin(); itrTime != sortTimeMap.end(); ){
		for( (itrRes = itrTime)++; itrRes != sortTimeMap.end(); itrRes++ ){
			__int64 endTime;
			CalcEntireReserveTime(NULL, &endTime, *itrRes->second);
			if( endTime + CTunerBankCtrl::READY_MARGIN * I64_1SEC > -itrTime->first ){
				break;
			}
		}
		itrTime++;
		if( itrRes == sortTimeMap.end() ){
			//バンク未決の予約マップ
			multimap<__int64, const RESERVE_DATA*> sortResMap;
			for( itrRes = sortTimeMap.begin(); itrRes != itrTime; itrRes++ ){
				//キーは実効優先度(予約優先度<<60|開始順)
				__int64 startOrder = -itrRes->first / I64_1SEC << 16 | itrRes->second->reserveID & 0xFFFF;
				__int64 priority = (this->backPriority ? itrRes->second->recSetting.priority : ~itrRes->second->recSetting.priority) & 7;
				sortResMap.insert(std::make_pair((this->backPriority ? -1 : 1) * (priority << 60 | startOrder), itrRes->second));
			}
			itrTime = sortTimeMap.erase(sortTimeMap.begin(), itrTime);

			//バンク決定した予約マップ
			map<DWORD, vector<CHK_RESERVE_DATA>> bankResMap;
			for( size_t i = 0; i < startedResList.size(); i++ ){
				bankResMap.insert(std::make_pair(startedResList[i].first, vector<CHK_RESERVE_DATA>()));
				//開始済み予約はそのままバンク決定
				for( multimap<__int64, const RESERVE_DATA*>::const_iterator itr = sortResMap.begin(); itr != sortResMap.end(); ){
					if( std::find(startedResList[i].second.begin(), startedResList[i].second.end(), itr->second->reserveID) != startedResList[i].second.end() ){
						CHK_RESERVE_DATA item;
						CalcEntireReserveTime(&item.cutStartTime, &item.cutEndTime, *itr->second);
						item.cutStartTime -= CTunerBankCtrl::READY_MARGIN * I64_1SEC;
						item.startOrder = abs(itr->first) & 0x0FFFFFFFFFFFFFFFLL;
						item.effectivePriority = itr->first;
						item.started = true;
						item.r = itr->second;
						//開始済み予約はすべてバンク内で同一チャンネルなのでChkInsertStatus()は不要
						bankResMap[startedResList[i].first].push_back(item);
						sortResMap.erase(itr++);
					}else{
						itr++;
					}
				}
			}

			for( multimap<__int64, const RESERVE_DATA*>::const_iterator itr = sortResMap.begin(); itr != sortResMap.end(); ){
				CHK_RESERVE_DATA item;
				CalcEntireReserveTime(&item.cutStartTime, &item.cutEndTime, *itr->second);
				item.cutStartTime -= CTunerBankCtrl::READY_MARGIN * I64_1SEC;
				item.startOrder = abs(itr->first) & 0x0FFFFFFFFFFFFFFFLL;
				item.effectivePriority = itr->first;
				item.started = false;
				item.r = itr->second;
				if( itr->second->recSetting.tunerID != 0 ){
					//チューナID固定
					map<DWORD, vector<CHK_RESERVE_DATA>>::iterator itrBank = bankResMap.find(itr->second->recSetting.tunerID); 
					if( itrBank != bankResMap.end() &&
					    this->tunerManager.IsSupportService(itrBank->first, itr->second->originalNetworkID, itr->second->transportStreamID, itr->second->serviceID) ){
						CHK_RESERVE_DATA testItem = item;
						ChkInsertStatus(itrBank->second, testItem, false);
						if( testItem.cutEndTime - testItem.cutStartTime > CTunerBankCtrl::READY_MARGIN * I64_1SEC ){
							//録画時間がある
							ChkInsertStatus(itrBank->second, item, true);
							itrBank->second.push_back(item);
							sortResMap.erase(itr++);
							continue;
						}
					}
				}else{
					//もっとも良いと思われるバンクに割り当てる
					map<DWORD, vector<CHK_RESERVE_DATA>>::iterator itrMin = bankResMap.end();
					__int64 costMin = LLONG_MAX;
					__int64 durationMin = 0;
					for( map<DWORD, vector<CHK_RESERVE_DATA>>::iterator jtr = bankResMap.begin(); jtr != bankResMap.end(); jtr++ ){
						if( this->tunerManager.IsSupportService(jtr->first, itr->second->originalNetworkID, itr->second->transportStreamID, itr->second->serviceID) ){
							CHK_RESERVE_DATA testItem = item;
							__int64 cost = ChkInsertStatus(jtr->second, testItem, false);
							if( cost < costMin ){
								itrMin = jtr;
								costMin = cost;
								durationMin = testItem.cutEndTime - testItem.cutStartTime;
							}
						}
					}
					if( itrMin != bankResMap.end() && durationMin > CTunerBankCtrl::READY_MARGIN * I64_1SEC ){
						//録画時間がある
						ChkInsertStatus(itrMin->second, item, true);
						itrMin->second.push_back(item);
						sortResMap.erase(itr++);
						continue;
					}
				}
				itr++;
			}

			//実際にバンクに追加する
			for( map<DWORD, vector<CHK_RESERVE_DATA>>::const_iterator itr = bankResMap.begin(); itr != bankResMap.end(); itr++ ){
				for( size_t i = 0; i < itr->second.size(); i++ ){
					const RESERVE_DATA& r = *itr->second[i].r;
					__int64 startTime, endTime;
					CalcEntireReserveTime(&startTime, &endTime, r);
					//かぶり状態を記録する(参考程度の情報)
					this->reserveText.SetOverlapMode(r.reserveID,
						itr->second[i].cutStartTime == startTime - CTunerBankCtrl::READY_MARGIN * I64_1SEC &&
						itr->second[i].cutEndTime == endTime ? RESERVE_EXECUTE : RESERVE_PILED_UP);
					//バンクに渡す予約情報を作成
					CTunerBankCtrl::TUNER_RESERVE tr;
					tr.reserveID = r.reserveID;
					tr.title = r.title;
					tr.stationName = r.stationName;
					tr.onid = r.originalNetworkID;
					tr.tsid = r.transportStreamID;
					tr.sid = r.serviceID;
					tr.eid = r.eventID;
					tr.recMode = r.recSetting.recMode;
					tr.priority = r.recSetting.priority;
					tr.enableCaption = r.recSetting.serviceMode & RECSERVICEMODE_SET ? (r.recSetting.serviceMode & RECSERVICEMODE_CAP) != 0 : this->defEnableCaption;
					tr.enableData = r.recSetting.serviceMode & RECSERVICEMODE_SET ? (r.recSetting.serviceMode & RECSERVICEMODE_DATA) != 0 : this->defEnableData;
					tr.pittari = r.recSetting.pittariFlag != 0;
					tr.partialRecMode = r.recSetting.partialRecFlag;
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

	_OutputDebugString(L"End ReloadBankMap %dmsec\r\n", GetTickCount() - tick);
}

__int64 CReserveManager::ChkInsertStatus(vector<CHK_RESERVE_DATA>& bank, CHK_RESERVE_DATA& inItem, bool modifyBank) const
{
	//CBlockLock lock(&this->managerLock);

	bool overlapped = false;
	__int64 otherCosts[5] = {};

	for( size_t i = 0; i < bank.size(); i++ ){
		if( bank[i].r->originalNetworkID == inItem.r->originalNetworkID && bank[i].r->transportStreamID == inItem.r->transportStreamID ){
			//同一チャンネル
			if( inItem.cutStartTime < bank[i].cutStartTime && bank[i].cutStartTime < inItem.cutEndTime ||
			    inItem.cutStartTime < bank[i].cutEndTime && bank[i].cutEndTime < inItem.cutEndTime ||
			    inItem.cutStartTime > bank[i].cutStartTime && bank[i].cutEndTime > inItem.cutEndTime ){
				//重なりがある
				overlapped = true;
			}
		}else{
			if( bank[i].effectivePriority < inItem.effectivePriority ){
				//相手が高優先度なので自分の予約時間を削る
				if( bank[i].startOrder > inItem.startOrder ){
					//相手が遅れて開始するので自分の後方を削る
					__int64 cutEndTime = max(min(inItem.cutEndTime, bank[i].cutStartTime), inItem.cutStartTime);
					otherCosts[min(max(inItem.r->recSetting.priority, 1), 5) - 1] += inItem.cutEndTime - cutEndTime;
					inItem.cutEndTime = cutEndTime;
				}else{
					//前方を削る
					__int64 cutStartTime = min(max(inItem.cutStartTime, bank[i].cutEndTime), inItem.cutEndTime);
					otherCosts[min(max(inItem.r->recSetting.priority, 1), 5) - 1] += cutStartTime - inItem.cutStartTime;
					inItem.cutStartTime = cutStartTime;
				}
			}else{
				//相手の予約時間を削る
				if( inItem.startOrder > bank[i].startOrder ){
					//相手の後方を削る
					__int64 cutEndTime = max(min(bank[i].cutEndTime, inItem.cutStartTime), bank[i].cutStartTime);
					otherCosts[min(max(bank[i].r->recSetting.priority, 1), 5) - 1] += bank[i].cutEndTime - cutEndTime;
					if( modifyBank ){
						bank[i].cutEndTime = cutEndTime;
					}
				}else{
					//前方を削る
					_int64 cutStartTime = bank[i].started ? bank[i].cutEndTime : min(max(bank[i].cutStartTime, inItem.cutEndTime), bank[i].cutEndTime);
					otherCosts[min(max(bank[i].r->recSetting.priority, 1), 5) - 1] += cutStartTime - bank[i].cutStartTime;
					if( modifyBank ){
						bank[i].cutStartTime = cutStartTime;
					}
				}
			}
		}
	}

	//優先度ごとに重みをつけてコストを算出
	__int64 cost = 0;
	__int64 weight = 1;
	for( int i = 0; i < 5; i++ ){
		cost += min((otherCosts[i] + 10 * I64_1SEC - 1) / (10 * I64_1SEC), 5400 - 1) * weight;
		weight *= 5400;
	}
	if( cost == 0 && overlapped ){
		//TODO: とりあえず一律に-10秒とするが、重なり度合をコストに反映してもいいかも
		cost = -1;
	}
	return cost;
}

void CReserveManager::CalcEntireReserveTime(__int64* startTime, __int64* endTime, const RESERVE_DATA& data) const
{
	//CBlockLock lock(&this->managerLock);

	__int64 startTime_ = ConvertI64Time(data.startTime);
	__int64 endTime_ = startTime_ + data.durationSecond * I64_1SEC;
	__int64 startMargin = this->defStartMargin * I64_1SEC;
	__int64 endMargin = this->defEndMargin * I64_1SEC;
	if( data.recSetting.useMargineFlag != 0 ){
		startMargin = data.recSetting.startMargine * I64_1SEC;
		endMargin = data.recSetting.endMargine * I64_1SEC;
	}
	//開始マージンは元の予約終了時刻を超えて負であってはならない
	startMargin = max(startMargin, startTime_ - endTime_);
	//終了マージンは元の予約開始時刻を超えて負であってはならない
	endMargin = max(endMargin, startTime_ - min(startMargin, 0) - endTime_);
	if( startTime != NULL ){
		*startTime = startTime_ - startMargin;
	}
	if( endTime != NULL ){
		*endTime = endTime_ + endMargin;
	}
}

void CReserveManager::CheckTuijyu()
{
	CBlockLock lock(&this->managerLock);

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
			if( info.StartTimeFlag != 0 && info.DurationFlag != 0 ){
				__int64 startDiff = ConvertI64Time(info.start_time) - ConvertI64Time(itr->second.startTime);
				if( startDiff < -12 * 3600 * I64_1SEC || 12 * 3600 * I64_1SEC < startDiff ){
					//EventIDの再使用に備えるため12時間以上の移動は対象外
					continue;
				}
				RESERVE_DATA r = itr->second;
				bool chgRes = false;
				if( info.shortInfo != NULL && r.title != info.shortInfo->event_name ){
					r.title = info.shortInfo->event_name;
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
	vector<DWORD> chgIDList;
	//tunerBankMapそのものは排他制御の対象外
	for( map<DWORD, CTunerBankCtrl*>::const_iterator itrBank = this->tunerBankMap.begin(); itrBank != this->tunerBankMap.end(); itrBank++ ){
		CBlockLock lock(&this->managerLock);

		WORD onid, tsid;
		if( itrBank->second->GetCurrentChID(&onid, &tsid) == false ){
			//このチューナは起動していない
			continue;
		}
		vector<RESERVE_DATA> chgList;
		ReCacheReserveText();

		vector<pair<ULONGLONG, DWORD>>::const_iterator itrCache = std::lower_bound(
			this->reserveTextCache.begin(), this->reserveTextCache.end(), pair<ULONGLONG, DWORD>(_Create64Key2(onid, tsid, 0, 0), 0));
		for( ; itrCache != this->reserveTextCache.end() && itrCache->first <= (ULONGLONG)_Create64Key2(onid, tsid, 0xFFFF, 0xFFFF); ){
			//起動中のチャンネルに一致する予約をEIT[p/f]と照合する
			WORD sid = itrCache->first >> 16 & 0xFFFF;
			EPGDB_EVENT_INFO resPfVal[2];
			int nowSuccess = itrBank->second->GetEventPF(sid, false, &resPfVal[0]);
			int nextSuccess = itrBank->second->GetEventPF(sid, true, &resPfVal[1]);
			for( ; itrCache != this->reserveTextCache.end() && itrCache->first <= (ULONGLONG)_Create64Key2(onid, tsid, sid, 0xFFFF); itrCache++ ){
				if( std::find(chgIDList.begin(), chgIDList.end(), itrCache->second) != chgIDList.end() ){
					//この予約はすでに変更済み
					continue;
				}
				map<DWORD, RESERVE_DATA>::const_iterator itrRes = this->reserveText.GetMap().find(itrCache->second);
				if( itrRes->second.eventID == 0xFFFF ||
				    itrRes->second.recSetting.recMode == RECMODE_NO ||
				    ConvertI64Time(itrRes->second.startTime) > GetNowI64Time() + 6 * 3600 * I64_1SEC ){
					//プログラム予約、無効予約、および6時間以上先の予約は対象外
					continue;
				}
				bool pfFound = false;
				for( int i = (nowSuccess == 0 ? 0 : 1); i < (nextSuccess == 0 ? 2 : 1); i++ ){
					const EPGDB_EVENT_INFO& info = resPfVal[i];
					if( info.event_id == itrRes->second.eventID && (info.StartTimeFlag != 0 || info.DurationFlag != 0) ){
						RESERVE_DATA r = itrRes->second;
						bool chgRes = false;
						if( info.shortInfo != NULL && r.title != info.shortInfo->event_name ){
							r.title = info.shortInfo->event_name;
							r.reserveStatus = ADD_RESERVE_CHG_PF;
							chgRes = true;
						}
						if( info.StartTimeFlag != 0 ){
							if( ConvertI64Time(r.startTime) != ConvertI64Time(info.start_time) ){
								r.startTime = info.start_time;
								r.reserveStatus = ADD_RESERVE_CHG_PF;
								chgRes = true;
							}
							if( info.DurationFlag == 0 ){
								//継続時間未定。現在(present)かつ終了まで5分を切る予約は5分伸ばす
								if( i == 0 && ConvertI64Time(r.startTime) + r.durationSecond * I64_1SEC < GetNowI64Time() + 300 * I64_1SEC ){
									r.durationSecond += 300;
									r.reserveStatus = ADD_RESERVE_UNKNOWN_END;
									chgRes = true;
									OutputDebugString(L"●p/f 継続時間未定の現在イベントの予約を変更します\r\n");
								}
							}else if( r.reserveStatus == ADD_RESERVE_UNKNOWN_END || r.durationSecond != info.durationSec ){
								r.durationSecond = info.durationSec;
								r.reserveStatus = ADD_RESERVE_CHG_PF;
								chgRes = true;
							}
						}else{
							//開始時刻未定。次(following)かつ開始まで5分を切る予約は5分移動
							if( i == 1 && ConvertI64Time(r.startTime) < GetNowI64Time() + 300 * I64_1SEC ){
								ConvertSystemTime(ConvertI64Time(r.startTime) + 300 * I64_1SEC, &r.startTime);
								r.reserveStatus = ADD_RESERVE_CHG_PF;
								chgRes = true;
								OutputDebugString(L"●p/f 開始時刻未定の次イベントの予約を変更します\r\n");
							}
							if( r.reserveStatus == ADD_RESERVE_UNKNOWN_END || r.durationSecond != info.durationSec ){
								r.durationSecond = info.durationSec;
								r.reserveStatus = ADD_RESERVE_CHG_PF;
								chgRes = true;
							}
						}
						if( chgRes ){
							chgIDList.push_back(r.reserveID);
							chgList.push_back(r);
						}
						pfFound = true;
						break;
					}
				}
				//EIT[p/f]が正しく取得できる状況でEIT[p/f]にないものは通常チェック
				EPGDB_EVENT_INFO info;
				if( nowSuccess != 2 && nextSuccess != 2 && pfFound == false && itrBank->second->SearchEpgInfo(sid, itrRes->second.eventID, &info) ){
					if( info.StartTimeFlag != 0 && info.DurationFlag != 0 ){
						__int64 startDiff = ConvertI64Time(info.start_time) - ConvertI64Time(itrRes->second.startTime);
						if( startDiff < -12 * 3600 * I64_1SEC || 12 * 3600 * I64_1SEC < startDiff ){
							//EventIDの再使用に備えるため12時間以上の移動は対象外
							continue;
						}
						RESERVE_DATA r = itrRes->second;
						bool chgRes = false;
						if( info.shortInfo != NULL && r.title != info.shortInfo->event_name ){
							r.title = info.shortInfo->event_name;
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
							//EPG再読み込みで変更されないようにする
							r.reserveStatus = ADD_RESERVE_CHG_PF2;
							chgIDList.push_back(r.reserveID);
							chgList.push_back(r);
						}
					}
				}
			}
		}
		if( chgList.empty() == false ){
			ChgReserveData(chgList);
		}
	}
}

void CReserveManager::CheckOverTimeReserve()
{
	CBlockLock lock(&this->managerLock);

	bool modified = false;
	__int64 now = GetNowI64Time();
	vector<DWORD> noList = GetNoTunerReserveAll();
	for( size_t i = 0; i < noList.size(); i++ ){
		map<DWORD, RESERVE_DATA>::const_iterator itr = this->reserveText.GetMap().find(noList[i]);
		__int64 endTime;
		CalcEntireReserveTime(NULL, &endTime, itr->second);
		if( endTime < now ){
			//終了時間過ぎてしまっている
			if( itr->second.recSetting.recMode != RECMODE_NO ){
				//無効のものは結果に残さない
				REC_FILE_INFO item;
				item = itr->second;
				item.recStatus = REC_END_STATUS_NO_TUNER;
				item.comment = L"チューナー不足のため失敗しました";
				this->recInfoText.AddRecInfo(item);
			}
			this->reserveText.DelReserve(itr->first);
			this->reserveTextCache.clear();
			this->reserveModified = true;
			modified = true;
		}
	}
	if( modified ){
		this->reserveText.SaveText();
		this->recInfoText.SaveText();
		this->notifyManager.AddNotify(NOTIFY_UPDATE_RESERVE_INFO);
		this->notifyManager.AddNotify(NOTIFY_UPDATE_REC_INFO);
	}
}

DWORD CReserveManager::Wait(HANDLE hEvent, DWORD timeout, DWORD* extra)
{
	for(;;){
		DWORD tick = GetTickCount();
		DWORD diff = this->waitTick - tick;
		if( (diff & 0x80000000) != 0 ){
			diff = 0;
		}
		if( diff >= timeout ){
			return WaitForSingleObject(hEvent, timeout);
		}
		DWORD dwRet = WaitForSingleObject(hEvent, diff);
		if( dwRet != WAIT_TIMEOUT ){
			return dwRet;
		}
		//1秒ごとにチェックする
		this->waitTick = tick + 1000;
		this->waitCount++;
		if( timeout != INFINITE ){
			timeout -= diff;
		}

		bool isRec = false;
		bool isEpgCap = false;
		int shutdownMode = -1;
		//tunerBankMapそのものは排他制御の対象外
		for( map<DWORD, CTunerBankCtrl*>::const_iterator itrBank = this->tunerBankMap.begin(); itrBank != this->tunerBankMap.end(); itrBank++ ){
			CBlockLock lock(&this->managerLock);

			// チューナの予約状態遷移を行い、予約終了をチェックする
			vector<CTunerBankCtrl::CHECK_RESULT> retList = itrBank->second->Check();
			CTunerBankCtrl::TR_STATE state = itrBank->second->GetState();
			isRec = isRec || state == CTunerBankCtrl::TR_REC;
			isEpgCap = isEpgCap || state == CTunerBankCtrl::TR_EPGCAP;
			bool modified = false;
			for( vector<CTunerBankCtrl::CHECK_RESULT>::const_iterator itrRet = retList.begin(); itrRet != retList.end(); itrRet++ ){
				map<DWORD, RESERVE_DATA>::const_iterator itrRes = this->reserveText.GetMap().find(itrRet->reserveID);
				if( itrRes != this->reserveText.GetMap().end() ){
					if( itrRet->type == CTunerBankCtrl::CHECK_END && itrRet->recFilePath.empty() == false &&
					    itrRet->drops < this->recInfo2DropChk && itrRet->epgEventName.empty() == false ){
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
							item.comment = L"開始時間が変更されました";
						}else{
							item.recStatus = REC_END_STATUS_NORMAL;
							item.comment = item.recFilePath.empty() ? L"終了" : L"録画終了";
						}
						break;
					case CTunerBankCtrl::CHECK_END_NOT_FIND_PF:
						item.recStatus = REC_END_STATUS_NOT_FIND_PF;
						item.comment = L"録画中に番組情報を確認できませんでした";
						break;
					case CTunerBankCtrl::CHECK_END_NEXT_START_END:
						item.recStatus = REC_END_STATUS_NEXT_START_END;
						item.comment = L"次の予約開始のためにキャンセルされました";
						break;
					case CTunerBankCtrl::CHECK_END_END_SUBREC:
						item.recStatus = REC_END_STATUS_END_SUBREC;
						item.comment = L"録画終了（空き容量不足で別フォルダへの保存が発生）";
						break;
					case CTunerBankCtrl::CHECK_END_NOT_START_HEAD:
						item.recStatus = REC_END_STATUS_NOT_START_HEAD;
						item.comment = L"一部のみ録画が実行された可能性があります";
						break;
					case CTunerBankCtrl::CHECK_ERR_RECEND:
						item.recStatus = REC_END_STATUS_ERR_END2;
						item.comment = L"ファイル保存で致命的なエラーが発生した可能性があります";
						break;
					case CTunerBankCtrl::CHECK_ERR_REC:
						item.recStatus = REC_END_STATUS_ERR_END;
						item.comment = L"録画中にキャンセルされた可能性があります";
						break;
					case CTunerBankCtrl::CHECK_ERR_RECSTART:
					case CTunerBankCtrl::CHECK_ERR_CTRL:
						item.recStatus = REC_END_STATUS_ERR_RECSTART;
						item.comment = L"録画開始処理に失敗しました（空き容量不足の可能性あり）";
						break;
					case CTunerBankCtrl::CHECK_ERR_OPEN:
						item.recStatus = REC_END_STATUS_OPEN_ERR;
						item.comment = L"チューナーのオープンに失敗しました";
						break;
					case CTunerBankCtrl::CHECK_ERR_PASS:
						item.recStatus = REC_END_STATUS_START_ERR;
						item.comment = L"録画時間に起動していなかった可能性があります";
						break;
					}
					this->recInfoText.AddRecInfo(item);

					//バッチ処理追加
					if( (itrRet->type == CTunerBankCtrl::CHECK_END || itrRet->type == CTunerBankCtrl::CHECK_END_NEXT_START_END) && item.recFilePath.empty() == false ){
						BAT_WORK_INFO batInfo;
						batInfo.batFilePath = itrRes->second.recSetting.batFilePath;
						batInfo.suspendMode = itrRes->second.recSetting.suspendMode;
						batInfo.rebootFlag = itrRes->second.recSetting.rebootFlag;
						if( itrRes->second.comment.compare(0, 8, L"EPG自動予約(") == 0 && itrRes->second.comment.size() >= 9 ){
							batInfo.addKey.assign(itrRes->second.comment, 8, itrRes->second.comment.size() - 9);
						}
						batInfo.recFileInfo = item;
						this->batManager.AddBatWork(batInfo);
						this->batManager.StartWork();
					}else if( itrRet->type != CTunerBankCtrl::CHECK_ERR_PASS ){
						shutdownMode = MAKEWORD(itrRes->second.recSetting.suspendMode, itrRes->second.recSetting.rebootFlag);
					}

					this->reserveText.DelReserve(itrRes->first);
					this->reserveTextCache.clear();
					this->reserveModified = true;
					modified = true;

					//予約終了を通知
					SYSTEMTIME st = item.startTime;
					SYSTEMTIME stEnd;
					ConvertSystemTime(ConvertI64Time(st) + item.durationSecond * I64_1SEC, &stEnd);
					wstring msg;
					Format(msg, L"%s %04d/%02d/%02d %02d:%02d～%02d:%02d\r\n%s\r\n%s",
					       item.serviceName.c_str(), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute,
					       stEnd.wHour, stEnd.wMinute, item.title.c_str(), item.comment.c_str());
					this->notifyManager.AddNotifyMsg(NOTIFY_UPDATE_REC_END, msg);
				}
			}
			if( modified ){
				CBlockLock lock(&this->managerLock);
				this->reserveText.SaveText();
				this->recInfoText.SaveText();
				this->recInfo2Text.SaveText();
				this->notifyManager.AddNotify(NOTIFY_UPDATE_RESERVE_INFO);
				this->notifyManager.AddNotify(NOTIFY_UPDATE_REC_INFO);
			}
		}
		if( this->waitCount % 30 == 0 ){
			CheckOverTimeReserve();
		}
		if( this->waitCount % 3 == 0 ){
			CheckTuijyuTuner();
		}
		this->notifyManager.SetNotifySrvStatus(isRec ? 1 : isEpgCap ? 2 : 0);

		BYTE suspendMode;
		BYTE rebootFlag;
		if( CheckEpgCap(isEpgCap) ){
			//EPG取得が完了した
			this->notifyManager.AddNotify(NOTIFY_UPDATE_EPGCAP_END);
			*extra = MAKELONG(0, WAIT_EXTRA_EPGCAP_END);
			break;
		}else if( this->batManager.PopLastWorkSuspend(&suspendMode, &rebootFlag) ){
			//バッチ処理が完了した
			*extra = MAKELONG(MAKEWORD(suspendMode, rebootFlag), WAIT_EXTRA_NEED_SHUTDOWN);
			break;
		}else if( shutdownMode >= 0 && this->batManager.IsWorking() == false ){
			*extra = MAKELONG(shutdownMode, WAIT_EXTRA_NEED_SHUTDOWN);
			break;
		}else if( this->reserveModified ){
			CBlockLock lock(&this->managerLock);
			if( this->reserveModified ){
				*extra = MAKELONG(0, WAIT_EXTRA_RESERVE_MODIFIED);
				this->reserveModified = false;
			}
			break;
		}
	}
	return WAIT_OBJECT_0 + 1;
}

vector<DWORD> CReserveManager::GetEpgCapTunerIDList(__int64 now) const
{
	CBlockLock lock(&this->managerLock);

	//利用可能なチューナの抽出
	vector<pair<vector<DWORD>, WORD>> tunerIDList;
	this->tunerManager.GetEnumEpgCapTuner(&tunerIDList);
	vector<DWORD> epgCapIDList;
	for( size_t i = 0; i < tunerIDList.size(); i++ ){
		WORD epgCapMax = tunerIDList[i].second;
		WORD ngCapCount = 0;
		for( size_t j = 0; j < tunerIDList[i].first.size() && epgCapMax > 0; j++ ){
			map<DWORD, CTunerBankCtrl*>::const_iterator itr = this->tunerBankMap.find(tunerIDList[i].first[j]);
			CTunerBankCtrl::TR_STATE state = itr->second->GetState();
			__int64 minTime = itr->second->GetNearestReserveTime();
			if( this->ngCapTimeSec != 0 && (state != CTunerBankCtrl::TR_IDLE || minTime < now + this->ngCapTimeSec * I64_1SEC) ){
				//実行しちゃいけない
				ngCapCount++;
			}else if( state == CTunerBankCtrl::TR_IDLE && minTime > now + this->ngCapTunerTimeSec * I64_1SEC ){
				//使えるチューナ
				epgCapIDList.push_back(itr->first);
				epgCapMax--;
			}
		}
		if( tunerIDList[i].second > tunerIDList[i].first.size() - ngCapCount ){
			epgCapIDList.clear();
			break;
		}
	}
	return epgCapIDList;
}

bool CReserveManager::RequestStartEpgCap()
{
	CBlockLock lock(&this->managerLock);

	if( this->epgCapRequested || this->epgCapWork || GetEpgCapTunerIDList(GetNowI64Time()).empty() ){
		return false;
	}
	this->epgCapRequested = true;
	return true;
}

bool CReserveManager::CheckEpgCap(bool isEpgCap)
{
	CBlockLock lock(&this->managerLock);

	bool doneEpgCap = false;
	__int64 now = GetNowI64Time();
	if( this->epgCapWork == false ){
		//毎分0秒を跨ぐタイミングでEPG取得のチェックを行う
		if( this->epgCapRequested || now / (60 * I64_1SEC) > this->lastCheckEpgCap / (60 * I64_1SEC) ){
			int basicOnlyFlags = -1;
			__int64 capTime = this->epgCapRequested ? now : GetNextEpgCapTime(now, &basicOnlyFlags);
			if( capTime <= now + 60 * I64_1SEC ){
				vector<DWORD> epgCapIDList = GetEpgCapTunerIDList(now);
				if( epgCapIDList.empty() == false ){
					if( capTime > now ){
						//取得開始1分前
						//この通知はあくまで参考。開始しないのに通知する可能性も、その逆もありえる
						this->notifyManager.AddNotifyMsg(NOTIFY_UPDATE_PRE_EPGCAP_START, L"取得開始１分前");
					}else{
						//取得開始
						wstring iniCommonPath;
						GetCommonIniPath(iniCommonPath);
						int lastFlags = (GetPrivateProfileInt(L"SET", L"BSBasicOnly", 1, iniCommonPath.c_str()) != 0 ? 1 : 0) |
						                (GetPrivateProfileInt(L"SET", L"CS1BasicOnly", 1, iniCommonPath.c_str()) != 0 ? 2 : 0) |
						                (GetPrivateProfileInt(L"SET", L"CS2BasicOnly", 1, iniCommonPath.c_str()) != 0 ? 4 : 0);
						if( basicOnlyFlags >= 0 ){
							//一時的に設定を変更してEPG取得チューナ側の挙動を変える
							//TODO: パイプコマンドを拡張すべき
							this->epgCapBasicOnlyFlags = lastFlags;
							WritePrivateProfileString(L"SET", L"BSBasicOnly", basicOnlyFlags & 1 ? L"1" : L"0", iniCommonPath.c_str());
							WritePrivateProfileString(L"SET", L"CS1BasicOnly", basicOnlyFlags & 2 ? L"1" : L"0", iniCommonPath.c_str());
							WritePrivateProfileString(L"SET", L"CS2BasicOnly", basicOnlyFlags & 4 ? L"1" : L"0", iniCommonPath.c_str());
						}else{
							this->epgCapBasicOnlyFlags = -1;
							basicOnlyFlags = lastFlags;
						}
						//各チューナに振り分け
						LONGLONG lastKey = -1;
						bool inBS = false;
						bool inCS1 = false;
						bool inCS2 = false;
						size_t listIndex = 0;
						vector<vector<SET_CH_INFO>> epgCapChList(epgCapIDList.size());
						for( map<LONGLONG, CH_DATA5>::const_iterator itr = this->chUtil.GetMap().begin(); itr != this->chUtil.GetMap().end(); itr++ ){
							if( itr->second.epgCapFlag == FALSE ||
							    lastKey >= 0 && lastKey == itr->first >> 16 ||
							    itr->second.originalNetworkID == 4 && (basicOnlyFlags & 1) && inBS ||
							    itr->second.originalNetworkID == 6 && (basicOnlyFlags & 2) && inCS1 ||
							    itr->second.originalNetworkID == 7 && (basicOnlyFlags & 4) && inCS2 ){
								continue;
							}
							lastKey = itr->first >> 16;
							SET_CH_INFO addCh;
							addCh.ONID = itr->second.originalNetworkID;
							addCh.TSID = itr->second.transportStreamID;
							addCh.SID = itr->second.serviceID;
							addCh.useSID = TRUE;
							addCh.useBonCh = FALSE;
							for( size_t i = 0; i < epgCapIDList.size(); i++ ){
								if( this->tunerManager.IsSupportService(epgCapIDList[listIndex], addCh.ONID, addCh.TSID, addCh.SID) ){
									epgCapChList[listIndex].push_back(addCh);
									inBS = inBS || addCh.ONID == 4;
									inCS1 = inCS1 || addCh.ONID == 6;
									inCS2 = inCS2 || addCh.ONID == 7;
									listIndex = (listIndex + 1) % epgCapIDList.size();
									break;
								}
								listIndex = (listIndex + 1) % epgCapIDList.size();
							}
						}
						for( size_t i = 0; i < epgCapIDList.size(); i++ ){
							this->tunerBankMap[epgCapIDList[i]]->StartEpgCap(epgCapChList[i]);
						}
						this->epgCapWork = true;
						this->epgCapSetTimeSync = false;
						this->notifyManager.AddNotify(NOTIFY_UPDATE_EPGCAP_START);
					}
				}
			}
		}
	}else{
		//EPG取得中
		if( this->epgCapTimeSync && this->epgCapSetTimeSync == false ){
			//時計合わせ(要SE_SYSTEMTIME_NAME特権)
			for( map<DWORD, CTunerBankCtrl*>::const_iterator itr = this->tunerBankMap.begin(); itr != this->tunerBankMap.end(); itr++ ){
				if( itr->second->GetState() == CTunerBankCtrl::TR_EPGCAP ){
					__int64 delay = itr->second->DelayTime();
					if( delay < -10 * I64_1SEC || 10 * I64_1SEC < delay ){
						SYSTEMTIME setTime;
						ConvertSystemTime(now + delay, &setTime);
						_OutputDebugString(L"★SetLocalTime %s%d\r\n", SetLocalTime(&setTime) ? L"" : L"err ", (int)(delay / I64_1SEC));
						this->epgCapSetTimeSync = true;
					}
				}
			}
		}
		if( isEpgCap == false ){
			//EPG取得中のチューナが無くなったので取得完了
			if( this->epgCapBasicOnlyFlags >= 0 ){
				//EPG取得開始時の設定を書き戻し
				wstring iniCommonPath;
				GetCommonIniPath(iniCommonPath);
				WritePrivateProfileString(L"SET", L"BSBasicOnly", this->epgCapBasicOnlyFlags & 1 ? L"1" : L"0", iniCommonPath.c_str());
				WritePrivateProfileString(L"SET", L"CS1BasicOnly", this->epgCapBasicOnlyFlags & 2 ? L"1" : L"0", iniCommonPath.c_str());
				WritePrivateProfileString(L"SET", L"CS2BasicOnly", this->epgCapBasicOnlyFlags & 4 ? L"1" : L"0", iniCommonPath.c_str());
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
	CBlockLock lock(&this->managerLock);

	if( this->epgCapRequested || this->epgCapWork || this->batManager.IsWorking() ){
		return true;
	}
	for( map<DWORD, CTunerBankCtrl*>::const_iterator itr = this->tunerBankMap.begin(); itr != this->tunerBankMap.end(); itr++ ){
		if( itr->second->GetState() != CTunerBankCtrl::TR_IDLE ){
			return true;
		}
	}
	return false;
}

__int64 CReserveManager::GetSleepReturnTime(__int64 baseTime) const
{
	CBlockLock lock(&this->managerLock);

	//最も近い予約開始時刻を得る
	__int64 nextRec = LLONG_MAX;
	for( map<DWORD, RESERVE_DATA>::const_iterator itr = this->reserveText.GetMap().begin(); itr != this->reserveText.GetMap().end(); itr++ ){
		if( itr->second.recSetting.recMode != RECMODE_NO ){
			__int64 startTime;
			CalcEntireReserveTime(&startTime, NULL, itr->second);
			if( startTime >= baseTime ){
				nextRec = min(nextRec, startTime);
			}
		}
	}
	__int64 capTime = GetNextEpgCapTime(baseTime + 60 * I64_1SEC);
	return min(nextRec, capTime);
}

__int64 CReserveManager::GetNextEpgCapTime(__int64 now, int* basicOnlyFlags) const
{
	CBlockLock lock(&this->managerLock);

	SYSTEMTIME st;
	ConvertSystemTime(now, &st);
	//現在時刻に対する日曜日からのオフセット(分)。秒以下の切り捨てに注意
	int baseTime = st.wDayOfWeek * 1440 + (int)(now / (60 * I64_1SEC) % 1440);
	//baseTimeとの差が最小のEPG取得時刻を探す
	int minDiff = INT_MAX;
	WORD minVal = 0;
	for( size_t i = 0; i < this->epgCapTimeList.size(); i++ ){
		int diff = (LOWORD(this->epgCapTimeList[i]) + 7 * 1440 - baseTime) % (7 * 1440);
		if( minDiff > diff ){
			minDiff = diff;
			minVal = HIWORD(this->epgCapTimeList[i]);
		}
	}
	if( minDiff == INT_MAX ){
		return LLONG_MAX;
	}
	if( basicOnlyFlags ){
		*basicOnlyFlags = minVal == 0xFF ? -1 : minVal;
	}
	return (now / (60 * I64_1SEC) + minDiff) * (60 * I64_1SEC);
}

void CReserveManager::ReCacheReserveText() const
{
	CBlockLock lock(&this->managerLock);

	if( this->reserveTextCache.empty() ){
		this->reserveTextCache.reserve(this->reserveText.GetMap().size());
		for( map<DWORD, RESERVE_DATA>::const_iterator itr = this->reserveText.GetMap().begin(); itr != this->reserveText.GetMap().end(); itr++ ){
			this->reserveTextCache.push_back(std::make_pair(_Create64Key2(
				itr->second.originalNetworkID, itr->second.transportStreamID, itr->second.serviceID, itr->second.eventID), itr->first));
		}
	}
	std::sort(this->reserveTextCache.begin(), this->reserveTextCache.end());
}

bool CReserveManager::IsFindReserve(WORD onid, WORD tsid, WORD sid, WORD eid) const
{
	CBlockLock lock(&this->managerLock);

	ReCacheReserveText();

	vector<pair<ULONGLONG, DWORD>>::const_iterator itr = std::lower_bound(
		this->reserveTextCache.begin(), this->reserveTextCache.end(), pair<ULONGLONG, DWORD>(_Create64Key2(onid, tsid, sid, eid), 0));
	return itr != this->reserveTextCache.end() && itr->first == (ULONGLONG)_Create64Key2(onid, tsid, sid, eid);
}

bool CReserveManager::IsFindProgramReserve(WORD onid, WORD tsid, WORD sid, __int64 startTime, DWORD durationSec) const
{
	CBlockLock lock(&this->managerLock);

	ReCacheReserveText();

	vector<pair<ULONGLONG, DWORD>>::const_iterator itr = std::lower_bound(
		this->reserveTextCache.begin(), this->reserveTextCache.end(), pair<ULONGLONG, DWORD>(_Create64Key2(onid, tsid, sid, 0xFFFF), 0));
	for( ; itr != this->reserveTextCache.end() && itr->first == (ULONGLONG)_Create64Key2(onid, tsid, sid, 0xFFFF); itr++ ){
		map<DWORD, RESERVE_DATA>::const_iterator itrRes = this->reserveText.GetMap().find(itr->second);
		if( itrRes->second.durationSecond == durationSec && ConvertI64Time(itrRes->second.startTime) == startTime ){
			return true;
		}
	}
	return false;
}

vector<DWORD> CReserveManager::GetSupportServiceTuner(WORD onid, WORD tsid, WORD sid) const
{
	//tunerManagerは排他制御の対象外
	vector<DWORD> idList;
	this->tunerManager.GetSupportServiceTuner(onid, tsid, sid, &idList);
	return idList;
}

bool CReserveManager::GetTunerCh(DWORD tunerID, WORD onid, WORD tsid, WORD sid, DWORD* space, DWORD* ch) const
{
	return this->tunerManager.GetCh(tunerID, onid, tsid, sid, space, ch) != FALSE;
}

wstring CReserveManager::GetTunerBonFileName(DWORD tunerID) const
{
	wstring bonFileName;
	this->tunerManager.GetBonFileName(tunerID, bonFileName);
	return bonFileName;
}

bool CReserveManager::IsOpenTuner(DWORD tunerID) const
{
	CBlockLock lock(&this->managerLock);

	map<DWORD, CTunerBankCtrl*>::const_iterator itr = this->tunerBankMap.find(tunerID);
	return itr != this->tunerBankMap.end() && itr->second->GetState() != CTunerBankCtrl::TR_IDLE;
}

bool CReserveManager::SetNWTVCh(bool nwUdp, bool nwTcp, const SET_CH_INFO& chInfo, const vector<DWORD>& tunerIDList)
{
	CBlockLock lock(&this->managerLock);

	for( map<DWORD, CTunerBankCtrl*>::const_iterator itr = this->tunerBankMap.begin(); itr != this->tunerBankMap.end(); itr++ ){
		if( itr->second->GetState() == CTunerBankCtrl::TR_NWTV ){
			//すでに起動しているので使えたら使う
			if( this->tunerManager.IsSupportService(itr->first, chInfo.ONID, chInfo.TSID, chInfo.SID) ){
				itr->second->SetNWTVCh(nwUdp, nwTcp, chInfo);
				return true;
			}
			itr->second->CloseNWTV();
			break;
		}
	}
	for( size_t i = 0; i < tunerIDList.size(); i++ ){
		if( this->tunerManager.IsSupportService(tunerIDList[i], chInfo.ONID, chInfo.TSID, chInfo.SID) ){
			map<DWORD, CTunerBankCtrl*>::const_iterator itr = this->tunerBankMap.find(tunerIDList[i]);
			if( itr != this->tunerBankMap.end() && itr->second->SetNWTVCh(nwUdp, nwTcp, chInfo) ){
				return true;
			}
		}
	}
	return false;
}

bool CReserveManager::CloseNWTV()
{
	CBlockLock lock(&this->managerLock);

	for( map<DWORD, CTunerBankCtrl*>::const_iterator itr = this->tunerBankMap.begin(); itr != this->tunerBankMap.end(); itr++ ){
		if( itr->second->GetState() == CTunerBankCtrl::TR_NWTV ){
			itr->second->CloseNWTV();
			return true;
		}
	}
	return false;
}

bool CReserveManager::GetRecFilePath(DWORD reserveID, wstring& filePath, DWORD* ctrlID, DWORD* processID) const
{
	CBlockLock lock(&this->managerLock);

	for( map<DWORD, CTunerBankCtrl*>::const_iterator itr = this->tunerBankMap.begin(); itr != this->tunerBankMap.end(); itr++ ){
		if( itr->second->GetRecFilePath(reserveID, filePath, ctrlID, processID) ){
			return true;
		}
	}
	return false;
}

bool CReserveManager::IsFindRecEventInfo(const EPGDB_EVENT_INFO& info, WORD chkDay) const
{
	CBlockLock lock(&this->managerLock);
	bool ret = false;

	CoInitialize(NULL);
	try{
		IRegExpPtr regExp;
		regExp.CreateInstance(CLSID_RegExp);
		if( regExp != NULL && info.shortInfo != NULL ){
			wstring infoEventName = info.shortInfo->event_name;
			if( this->recInfo2RegExp.empty() == false ){
				regExp->PutGlobal(VARIANT_TRUE);
				regExp->PutPattern(_bstr_t(this->recInfo2RegExp.c_str()));
				_bstr_t rpl = regExp->Replace(_bstr_t(infoEventName.c_str()), _bstr_t());
				infoEventName = (LPCWSTR)rpl == NULL ? L"" : (LPCWSTR)rpl;
			}
			if( infoEventName.empty() == false && info.StartTimeFlag != 0 ){
				map<DWORD, PARSE_REC_INFO2_ITEM>::const_iterator itr;
				for( itr = this->recInfo2Text.GetMap().begin(); itr != this->recInfo2Text.GetMap().end(); itr++ ){
					if( itr->second.originalNetworkID == info.original_network_id &&
					    itr->second.transportStreamID == info.transport_stream_id &&
					    itr->second.serviceID == info.service_id &&
					    ConvertI64Time(itr->second.startTime) + chkDay*24*60*60*I64_1SEC > ConvertI64Time(info.start_time) ){
						wstring eventName = itr->second.eventName;
						if( this->recInfo2RegExp.empty() == false ){
							_bstr_t rpl = regExp->Replace(_bstr_t(eventName.c_str()), _bstr_t());
							eventName = (LPCWSTR)rpl == NULL ? L"" : (LPCWSTR)rpl;
						}
						if( infoEventName == eventName ){
							ret = true;
							break;
						}
					}
				}
			}
		}
	}catch( _com_error& e ){
		_OutputDebugString(L"%s\r\n", e.ErrorMessage());
	}
	CoUninitialize();

	return ret;
}

bool CReserveManager::ChgAutoAddNoRec(WORD onid, WORD tsid, WORD sid, WORD eid)
{
	CBlockLock lock(&this->managerLock);

	vector<RESERVE_DATA> chgList;
	ReCacheReserveText();

	vector<pair<ULONGLONG, DWORD>>::const_iterator itr = std::lower_bound(
		this->reserveTextCache.begin(), this->reserveTextCache.end(), pair<ULONGLONG, DWORD>(_Create64Key2(onid, tsid, sid, eid), 0));
	for( ; itr != this->reserveTextCache.end() && itr->first == (ULONGLONG)_Create64Key2(onid, tsid, sid, eid); itr++ ){
		map<DWORD, RESERVE_DATA>::const_iterator itrRes = this->reserveText.GetMap().find(itr->second);
		if( itrRes->second.recSetting.recMode != RECMODE_NO && itrRes->second.comment.compare(0, 7, L"EPG自動予約") == 0 ){
			chgList.push_back(itrRes->second);
			chgList.back().recSetting.recMode = RECMODE_NO;
		}
	}
	return chgList.empty() == false && ChgReserveData(chgList);
}

vector<CH_DATA5> CReserveManager::GetChDataList() const
{
	CBlockLock lock(&this->managerLock);

	vector<CH_DATA5> list;
	list.reserve(this->chUtil.GetMap().size());
	for( map<LONGLONG, CH_DATA5>::const_iterator itr = this->chUtil.GetMap().begin(); itr != this->chUtil.GetMap().end(); itr++ ){
		list.push_back(itr->second);
	}
	return list;
}

UINT WINAPI CReserveManager::WatchdogThread(LPVOID param)
{
	CReserveManager* sys = (CReserveManager*)param;
	while( WaitForSingleObject(sys->watchdogStopEvent, 2000) == WAIT_TIMEOUT ){
		for( map<DWORD, CTunerBankCtrl*>::const_iterator itr = sys->tunerBankMap.begin(); itr != sys->tunerBankMap.end(); itr++ ){
			itr->second->Watch();
		}
	}
	return 0;
}
