#include "StdAfx.h"
#include "TunerManager.h"


CTunerManager::CTunerManager(void)
{
}


CTunerManager::~CTunerManager(void)
{
	map<DWORD, TUNER_INFO*>::iterator itr;
	for( itr = this->tunerMap.begin(); itr != this->tunerMap.end(); itr++ ){
		SAFE_DELETE(itr->second);
	}
}

BOOL CTunerManager::FindBonFileName(wstring src, wstring& dllName)
{
	wstring buff = src;
	size_t pos = buff.rfind(L")");
	if( pos == string::npos ){
		dllName = src;
		return FALSE;
	}

	int count = 1;
	for( size_t i=pos-1; i>=0; i-- ){
		if(buff.compare(i,1,L")") == 0 ){
			count++;
		}else if(buff.compare(i,1,L"(") == 0){
			count--;
		}
		if( count == 0 ){
			dllName = buff.substr(0, i);
			break;
		}
	}

	return TRUE;
}

//チューナー一覧の読み込みを行う
//戻り値：
// TRUE（成功）、FALSE（失敗）
BOOL CTunerManager::ReloadTuner()
{
	map<DWORD, TUNER_INFO*>::iterator itr;
	for( itr = this->tunerMap.begin(); itr != this->tunerMap.end(); itr++ ){
		SAFE_DELETE(itr->second);
	}
	this->tunerMap.clear();

	wstring path = L"";
	GetSettingPath(path);

	wstring srvIniPath = L"";
	GetEpgTimerSrvIniPath(srvIniPath);

	wstring searchKey = path;
	searchKey += L"\\*.ChSet4.txt";

	WIN32_FIND_DATA findData;
	HANDLE find;

	//指定フォルダのファイル一覧取得
	find = FindFirstFile( searchKey.c_str(), &findData);
	if ( find == INVALID_HANDLE_VALUE ) {
		return FALSE;
	}
	do{
		if( (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 ){
			//本当に拡張子DLL?
			if( IsExt(findData.cFileName, L".txt") == TRUE ){
				wstring chSetPath = L"";
				Format(chSetPath, L"%s\\%s", path.c_str(), findData.cFileName);

				wstring bonFileName = L"";
				wstring buff = findData.cFileName;

				FindBonFileName(buff, bonFileName);

				bonFileName += L".dll";

				WORD count = (WORD)GetPrivateProfileInt(bonFileName.c_str(), L"Count", 0, srvIniPath.c_str());
				if( count != 0 ){
					//カウント0以上のものだけ利用
					WORD priority = (WORD)GetPrivateProfileInt(bonFileName.c_str(), L"Priority", 0, srvIniPath.c_str());
					BOOL epgCapFlag = (BOOL)GetPrivateProfileInt(bonFileName.c_str(), L"GetEpg", 1, srvIniPath.c_str());
					WORD EPGcount = (WORD)GetPrivateProfileInt(bonFileName.c_str(), L"EPGCount", count, srvIniPath.c_str());
					if(EPGcount==0)	EPGcount = count;

					if( this->tunerMap.find((DWORD)priority<<16 | 1) != this->tunerMap.end() ){
						OutputDebugString(L"CTunerManager::ReloadTuner(): Duplicate bonID\r\n");
						count = 0;
					}
					for( WORD i=1; i<=count; i++ ){
						TUNER_INFO* item = new TUNER_INFO;
						item->bonID = priority;
						item->tunerID = i;
						item->epgCapMaxOfThisBon = min(epgCapFlag == FALSE ? 0 : EPGcount, count);
						item->bonFileName = bonFileName;
						item->chUtil.ParseText(chSetPath.c_str());
						item->chSet4FilePath = chSetPath;
						DWORD key = ((DWORD)item->bonID)<<16 | item->tunerID;
						this->tunerMap.insert(pair<DWORD, TUNER_INFO*>(key, item));
					}
				}
			}
		}
	}while(FindNextFile(find, &findData));

	FindClose(find);

	return TRUE;
}


//チューナーのID一覧を取得する。
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// idList			[OUT]チューナーのID一覧
BOOL CTunerManager::GetEnumID(
	vector<DWORD>* idList
	)
{
	if( idList == NULL ){
		return FALSE;
	}
	map<DWORD, TUNER_INFO*>::iterator itr;
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
BOOL CTunerManager::GetEnumTunerBank(
	map<DWORD, CTunerBankCtrl*>* ctrlMap
	)
{
	if( ctrlMap == NULL ){
		return FALSE;
	}
	map<DWORD, TUNER_INFO*>::iterator itr;
	for( itr = this->tunerMap.begin(); itr != this->tunerMap.end(); itr++ ){
		CTunerBankCtrl* ctrl = new CTunerBankCtrl;
		ctrl->SetTunerInfo( itr->second->bonID, itr->second->tunerID, itr->second->bonFileName, itr->second->chSet4FilePath);
		ctrlMap->insert(pair<DWORD, CTunerBankCtrl*>(itr->first, ctrl));
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
	)
{
	if( idList == NULL ){
		return FALSE;
	}
	map<DWORD, TUNER_INFO*>::iterator itr;
	for( itr = this->tunerMap.begin(); itr != this->tunerMap.end(); itr++ ){
		map<DWORD, CH_DATA4>::const_iterator itrCh;
		for( itrCh = itr->second->chUtil.GetMap().begin(); itrCh != itr->second->chUtil.GetMap().end(); itrCh++ ){
			if( itrCh->second.originalNetworkID == ONID && itrCh->second.transportStreamID == TSID && itrCh->second.serviceID == SID ){
				break;
			}
		}
		if( itrCh == itr->second->chUtil.GetMap().end() ){
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
	)
{
	if( idList == NULL ){
		return FALSE;
	}
	map<DWORD, TUNER_INFO*>::iterator itr;
	for( itr = this->tunerMap.begin(); itr != this->tunerMap.end(); itr++ ){
		map<DWORD, CH_DATA4>::const_iterator itrCh;
		for( itrCh = itr->second->chUtil.GetMap().begin(); itrCh != itr->second->chUtil.GetMap().end(); itrCh++ ){
			if( itrCh->second.originalNetworkID == ONID && itrCh->second.transportStreamID == TSID && itrCh->second.serviceID == SID ){
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
	)
{
	map<DWORD, TUNER_INFO*>::iterator itr;
	for( itr = this->tunerMap.begin(); itr != this->tunerMap.end(); itr++ ){
		map<DWORD, CH_DATA4>::const_iterator itrCh;
		for( itrCh = itr->second->chUtil.GetMap().begin(); itrCh != itr->second->chUtil.GetMap().end(); itrCh++ ){
			if( itrCh->second.originalNetworkID == ONID && itrCh->second.transportStreamID == TSID && itrCh->second.serviceID == SID ){
				if( space != NULL ){
					*space = itrCh->second.space;
				}
				if( ch != NULL ){
					*ch = itrCh->second.ch;
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
	)
{
	if( idList == NULL ){
		return FALSE;
	}
	map<DWORD, TUNER_INFO*>::iterator itr;
	for( itr = this->tunerMap.begin(); itr != this->tunerMap.end(); itr++ ){
		if( idList->empty() || idList->back().first.back() >> 16 != itr->first >> 16 ){
			idList->push_back(pair<vector<DWORD>, WORD>(vector<DWORD>(), itr->second->epgCapMaxOfThisBon));
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
	)
{
	map<DWORD, TUNER_INFO*>::iterator itr;
	itr = this->tunerMap.find(tunerID);
	if( itr == this->tunerMap.end() ){
		return FALSE;
	}

	map<DWORD, CH_DATA4>::const_iterator itrCh;
	for( itrCh = itr->second->chUtil.GetMap().begin(); itrCh != itr->second->chUtil.GetMap().end(); itrCh++ ){
		if( itrCh->second.originalNetworkID == ONID && itrCh->second.transportStreamID == TSID && itrCh->second.serviceID == SID ){
			return TRUE;
		}
	}
	return FALSE;
}

BOOL CTunerManager::GetBonFileName(
	DWORD tunerID,
	wstring& bonFileName
	)
{
	map<DWORD, TUNER_INFO*>::iterator itr;
	itr = this->tunerMap.find(tunerID);
	if( itr == this->tunerMap.end() ){
		return FALSE;
	}
	bonFileName = itr->second->bonFileName;

	return TRUE;
}
