#include "stdafx.h"
#include "TunerManager.h"
#include "../../Common/PathUtil.h"
#include "../../Common/StringUtil.h"


void CTunerManager::ReloadTuner()
{
	this->tunerMap.clear();

	const fs_path path = GetSettingPath();

	const fs_path srvIniPath = GetModuleIniPath();

	vector<pair<wstring, wstring>> nameList = CEpgTimerSrvSetting::EnumBonFileName(path.c_str());

	for( size_t i = 0; i < nameList.size(); i++ ){
		WORD count = (WORD)GetPrivateProfileInt(nameList[i].first.c_str(), L"Count", 0, srvIniPath.c_str());
		if( count != 0 ){
			//カウント1以上のものだけ利用
			WORD priority = (WORD)GetPrivateProfileInt(nameList[i].first.c_str(), L"Priority", 0, srvIniPath.c_str());
			WORD epgCount = 0;
			if( GetPrivateProfileInt(nameList[i].first.c_str(), L"GetEpg", 1, srvIniPath.c_str()) != 0 ){
				epgCount = (WORD)GetPrivateProfileInt(nameList[i].first.c_str(), L"EPGCount", 0, srvIniPath.c_str());
				if( epgCount == 0 ){
					epgCount = count;
				}
			}
			if( this->tunerMap.find((DWORD)priority << 16 | 1) != this->tunerMap.end() ){
				OutputDebugString(L"CTunerManager::ReloadTuner(): Duplicate bonID\r\n");
			}else{
				TUNER_INFO item;
				item.epgCapMaxOfThisBon = min(epgCount, count);
				item.bonFileName = nameList[i].first;
				CParseChText4 chUtil;
				chUtil.ParseText(fs_path(path).append(nameList[i].second).c_str());
				item.chList.reserve(chUtil.GetMap().size());
				for( map<DWORD, CH_DATA4>::const_iterator itr = chUtil.GetMap().begin(); itr != chUtil.GetMap().end(); itr++ ){
					item.chList.push_back(itr->second);
				}
				for( WORD j = 0; j < count; j++ ){
					this->tunerMap.insert(std::make_pair((DWORD)priority << 16 | (j + 1), item));
				}
			}
		}
	}
}

void CTunerManager::GetEnumTunerBank(
	map<DWORD, std::unique_ptr<CTunerBankCtrl>>* ctrlMap,
	CNotifyManager& notifyManager,
	CEpgDBManager& epgDBManager
	) const
{
	map<DWORD, TUNER_INFO>::const_iterator itr;
	for( itr = this->tunerMap.begin(); itr != this->tunerMap.end(); itr++ ){
		ctrlMap->insert(std::make_pair(itr->first,
			std::unique_ptr<CTunerBankCtrl>(new CTunerBankCtrl(itr->first, itr->second.bonFileName.c_str(), itr->second.chList, notifyManager, epgDBManager))));
	}
}

vector<DWORD> CTunerManager::GetSupportServiceTuner(
	WORD ONID,
	WORD TSID,
	WORD SID
	) const
{
	vector<DWORD> idList;
	map<DWORD, TUNER_INFO>::const_iterator itr;
	for( itr = this->tunerMap.begin(); itr != this->tunerMap.end(); itr++ ){
		vector<CH_DATA4>::const_iterator itrCh;
		for( itrCh = itr->second.chList.begin(); itrCh != itr->second.chList.end(); itrCh++ ){
			if( itrCh->originalNetworkID == ONID && itrCh->transportStreamID == TSID && itrCh->serviceID == SID ){
				idList.push_back(itr->first);
				break;
			}
		}

	}
	return idList;
}

bool CTunerManager::GetCh(
	DWORD tunerID,
	WORD ONID,
	WORD TSID,
	WORD SID,
	DWORD* space,
	DWORD* ch
	) const
{
	map<DWORD, TUNER_INFO>::const_iterator itr = this->tunerMap.find(tunerID);
	if( itr != this->tunerMap.end() ){
		vector<CH_DATA4>::const_iterator itrCh;
		for( itrCh = itr->second.chList.begin(); itrCh != itr->second.chList.end(); itrCh++ ){
			if( itrCh->originalNetworkID == ONID && itrCh->transportStreamID == TSID && itrCh->serviceID == SID ){
				if( space != NULL ){
					*space = itrCh->space;
				}
				if( ch != NULL ){
					*ch = itrCh->ch;
				}
				return true;
			}
		}
	}
	return false;
}

vector<pair<vector<DWORD>, WORD>> CTunerManager::GetEnumEpgCapTuner(
	) const
{
	vector<pair<vector<DWORD>, WORD>> idList;
	map<DWORD, TUNER_INFO>::const_iterator itr;
	for( itr = this->tunerMap.begin(); itr != this->tunerMap.end(); itr++ ){
		if( idList.empty() || idList.back().first.back() >> 16 != itr->first >> 16 ){
			idList.push_back(pair<vector<DWORD>, WORD>(vector<DWORD>(), itr->second.epgCapMaxOfThisBon));
		}
		idList.back().first.push_back(itr->first);
	}
	return idList;
}

bool CTunerManager::IsSupportService(
	DWORD tunerID,
	WORD ONID,
	WORD TSID,
	WORD SID
	) const
{
	map<DWORD, TUNER_INFO>::const_iterator itr;
	itr = this->tunerMap.find(tunerID);
	if( itr == this->tunerMap.end() ){
		return false;
	}

	vector<CH_DATA4>::const_iterator itrCh;
	for( itrCh = itr->second.chList.begin(); itrCh != itr->second.chList.end(); itrCh++ ){
		if( itrCh->originalNetworkID == ONID && itrCh->transportStreamID == TSID && itrCh->serviceID == SID ){
			return true;
		}
	}
	return false;
}

bool CTunerManager::GetBonFileName(
	DWORD tunerID,
	wstring& bonFileName
	) const
{
	map<DWORD, TUNER_INFO>::const_iterator itr;
	itr = this->tunerMap.find(tunerID);
	if( itr == this->tunerMap.end() ){
		return false;
	}
	bonFileName = itr->second.bonFileName;

	return true;
}
