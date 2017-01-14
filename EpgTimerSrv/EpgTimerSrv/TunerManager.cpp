#include "StdAfx.h"
#include "TunerManager.h"
#include "../../Common/PathUtil.h"
#include "../../Common/StringUtil.h"


//チューナー一覧の読み込みを行う
//戻り値：
// TRUE（成功）、FALSE（失敗）
BOOL CTunerManager::ReloadTuner()
{
	this->tunerMap.clear();

	wstring path = L"";
	GetSettingPath(path);

	wstring srvIniPath = L"";
	GetModuleIniPath(srvIniPath);

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
				count = 0;
			}
			for( WORD j = 0; j < count; j++ ){
				TUNER_INFO& item = this->tunerMap.insert(std::make_pair((DWORD)priority << 16 | (j + 1), TUNER_INFO())).first->second;
				item.epgCapMaxOfThisBon = min(epgCount, count);
				item.bonFileName = nameList[i].first;
				CParseChText4 chUtil;
				chUtil.ParseText((path + L'\\' + nameList[i].second).c_str());
				item.chList.reserve(chUtil.GetMap().size());
				for( map<DWORD, CH_DATA4>::const_iterator itr = chUtil.GetMap().begin(); itr != chUtil.GetMap().end(); itr++ ){
					item.chList.push_back(itr->second);
				}
			}
		}
	}

	return TRUE;
}


//チューナーのID一覧を取得する。
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// idList			[OUT]チューナーのID一覧
BOOL CTunerManager::GetEnumID(
	vector<DWORD>* idList
	) const
{
	if( idList == NULL ){
		return FALSE;
	}
	map<DWORD, TUNER_INFO>::const_iterator itr;
	for( itr = this->tunerMap.begin(); itr != this->tunerMap.end(); itr++ ){
		idList->push_back(itr->first);
	}
	return TRUE;
}

//チューナー予約制御を取得する
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// ctrlMap			[OUT]チューナー予約制御の一覧
// notifyManager	[IN]CTunerBankCtrlに渡す引数
// epgDBManager		[IN]CTunerBankCtrlに渡す引数
BOOL CTunerManager::GetEnumTunerBank(
	map<DWORD, std::unique_ptr<CTunerBankCtrl>>* ctrlMap,
	CNotifyManager& notifyManager,
	CEpgDBManager& epgDBManager
	) const
{
	if( ctrlMap == NULL ){
		return FALSE;
	}
	map<DWORD, TUNER_INFO>::const_iterator itr;
	for( itr = this->tunerMap.begin(); itr != this->tunerMap.end(); itr++ ){
		ctrlMap->insert(std::make_pair(itr->first,
			std::unique_ptr<CTunerBankCtrl>(new CTunerBankCtrl(itr->first, itr->second.bonFileName.c_str(), itr->second.chList, notifyManager, epgDBManager))));
	}
	return TRUE;
}

//指定サービスをサポートしていないチューナー一覧を取得する
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// ONID				[IN]確認したいサービスのONID
// TSID				[IN]確認したいサービスのTSID
// SID				[IN]確認したいサービスのSID
// idList			[OUT]チューナーのID一覧
BOOL CTunerManager::GetNotSupportServiceTuner(
	WORD ONID,
	WORD TSID,
	WORD SID,
	vector<DWORD>* idList
	) const
{
	if( idList == NULL ){
		return FALSE;
	}
	map<DWORD, TUNER_INFO>::const_iterator itr;
	for( itr = this->tunerMap.begin(); itr != this->tunerMap.end(); itr++ ){
		vector<CH_DATA4>::const_iterator itrCh;
		for( itrCh = itr->second.chList.begin(); itrCh != itr->second.chList.end(); itrCh++ ){
			if( itrCh->originalNetworkID == ONID && itrCh->transportStreamID == TSID && itrCh->serviceID == SID ){
				break;
			}
		}
		if( itrCh == itr->second.chList.end() ){
			idList->push_back(itr->first);
		}

	}
	return TRUE;
}

BOOL CTunerManager::GetSupportServiceTuner(
	WORD ONID,
	WORD TSID,
	WORD SID,
	vector<DWORD>* idList
	) const
{
	if( idList == NULL ){
		return FALSE;
	}
	map<DWORD, TUNER_INFO>::const_iterator itr;
	for( itr = this->tunerMap.begin(); itr != this->tunerMap.end(); itr++ ){
		vector<CH_DATA4>::const_iterator itrCh;
		for( itrCh = itr->second.chList.begin(); itrCh != itr->second.chList.end(); itrCh++ ){
			if( itrCh->originalNetworkID == ONID && itrCh->transportStreamID == TSID && itrCh->serviceID == SID ){
				idList->push_back(itr->first);
				break;
			}
		}

	}
	return TRUE;
}

BOOL CTunerManager::GetCh(
	DWORD tunerID,
	WORD ONID,
	WORD TSID,
	WORD SID,
	DWORD* space,
	DWORD* ch
	) const
{
	map<DWORD, TUNER_INFO>::const_iterator itr;
	for( itr = this->tunerMap.begin(); itr != this->tunerMap.end(); itr++ ){
		vector<CH_DATA4>::const_iterator itrCh;
		for( itrCh = itr->second.chList.begin(); itrCh != itr->second.chList.end(); itrCh++ ){
			if( itrCh->originalNetworkID == ONID && itrCh->transportStreamID == TSID && itrCh->serviceID == SID ){
				if( space != NULL ){
					*space = itrCh->space;
				}
				if( ch != NULL ){
					*ch = itrCh->ch;
				}
				return TRUE;
			}
		}
	}
	return FALSE;
}

//ドライバ毎のチューナー一覧とEPG取得に使用できるチューナー数のペアを取得する
BOOL CTunerManager::GetEnumEpgCapTuner(
	vector<pair<vector<DWORD>, WORD>>* idList
	) const
{
	if( idList == NULL ){
		return FALSE;
	}
	map<DWORD, TUNER_INFO>::const_iterator itr;
	for( itr = this->tunerMap.begin(); itr != this->tunerMap.end(); itr++ ){
		if( idList->empty() || idList->back().first.back() >> 16 != itr->first >> 16 ){
			idList->push_back(pair<vector<DWORD>, WORD>(vector<DWORD>(), itr->second.epgCapMaxOfThisBon));
		}
		idList->back().first.push_back(itr->first);
	}
	return TRUE;
}

BOOL CTunerManager::IsSupportService(
	DWORD tunerID,
	WORD ONID,
	WORD TSID,
	WORD SID
	) const
{
	map<DWORD, TUNER_INFO>::const_iterator itr;
	itr = this->tunerMap.find(tunerID);
	if( itr == this->tunerMap.end() ){
		return FALSE;
	}

	vector<CH_DATA4>::const_iterator itrCh;
	for( itrCh = itr->second.chList.begin(); itrCh != itr->second.chList.end(); itrCh++ ){
		if( itrCh->originalNetworkID == ONID && itrCh->transportStreamID == TSID && itrCh->serviceID == SID ){
			return TRUE;
		}
	}
	return FALSE;
}

BOOL CTunerManager::GetBonFileName(
	DWORD tunerID,
	wstring& bonFileName
	) const
{
	map<DWORD, TUNER_INFO>::const_iterator itr;
	itr = this->tunerMap.find(tunerID);
	if( itr == this->tunerMap.end() ){
		return FALSE;
	}
	bonFileName = itr->second.bonFileName;

	return TRUE;
}
