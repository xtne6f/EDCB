#include "stdafx.h"
#include "ChSetUtil.h"
#include "BonCtrlDef.h"
#include "../Common/EpgTimerUtil.h"
#include "../Common/PathUtil.h"

//チャンネル設定ファイルを読み込む
BOOL CChSetUtil::LoadChSet(
	const wstring& settingPath,
	const wstring& driverName,
	wstring tunerName
	)
{
	BOOL ret = TRUE;
	CheckFileName(tunerName);
	bool mightExist = false;
	if( tunerName.empty() == false ){
		//チューナー名付きのファイルがあれば優先する
		fs_path chSet4FilePath = fs_path(settingPath).append(fs_path(driverName).stem().concat(L"(" + tunerName + L").ChSet4.txt").native());
		if( UtilFileExists(chSet4FilePath, &mightExist).first || mightExist ){
			mightExist = true;
			if( this->chText4.ParseText(chSet4FilePath.c_str()) == false ){
				ret = FALSE;
			}
		}
	}
	if( mightExist == false ){
		if( this->chText4.ParseText(fs_path(settingPath).append(fs_path(driverName).stem().concat(L"().ChSet4.txt").native()).c_str()) == false ){
			ret = FALSE;
		}
	}
	if( this->chText5.ParseText(fs_path(settingPath).append(L"ChSet5.txt").c_str()) == false ){
		ret = FALSE;
	}
	return ret;
}

//チャンネル設定ファイルを保存する
BOOL CChSetUtil::SaveChSet(
	const wstring& settingPath,
	const wstring& driverName,
	wstring tunerName
	)
{
	CheckFileName(tunerName);
	fs_path chSet4FilePath = fs_path(settingPath).append(fs_path(driverName).stem().concat(L"(" + tunerName + L").ChSet4.txt").native());
	fs_path chSet5FilePath = fs_path(settingPath).append(L"ChSet5.txt");

	//接続待ち
	HANDLE waitEvent = CreateEvent(NULL, FALSE, TRUE, CHSET_SAVE_EVENT_WAIT);
	if( waitEvent == NULL ){
		return FALSE;
	}
	if(WaitForSingleObject(waitEvent, 10000) == WAIT_TIMEOUT){
		CloseHandle(waitEvent);
		return FALSE;
	}

	BOOL ret = TRUE;
	this->chText4.SetFilePath(chSet4FilePath.c_str());
	if( this->chText4.SaveText() == false ){
		ret = FALSE;
	}else if( tunerName.empty() == false ){
		//チューナー名付きと無しのファイルは同時に存在すべきでない
		DeleteFile(fs_path(settingPath).append(fs_path(driverName).stem().concat(L"().ChSet4.txt").native()).c_str());
	}

	//他で更新されてる可能性あるので再読み込み
	CParseChText5 mergeChText5;
	mergeChText5.ParseText(chSet5FilePath.c_str());
	//現在保持している情報を追加
	map<LONGLONG, CH_DATA5>::const_iterator itr;
	for( itr = this->chText5.GetMap().begin(); itr != this->chText5.GetMap().end(); itr++ ){
		mergeChText5.AddCh(itr->second);
	}
	//保存
	if( mergeChText5.SaveText() == false ){
		ret = FALSE;
	}
	//最新版を再読み込み
	this->chText5.ParseText(chSet5FilePath.c_str());

	SetEvent(waitEvent);
	CloseHandle(waitEvent);

	return ret;
}

//チャンネルスキャン用にクリアする
BOOL CChSetUtil::Clear()
{
	this->chText4.ParseText(L"");
	this->chText5.ParseText(L"");
	return TRUE;
}

//チャンネル情報を追加する
BOOL CChSetUtil::AddServiceInfo(
	DWORD space,
	DWORD ch,
	const wstring& chName,
	SERVICE_INFO* serviceInfo
	)
{
	CH_DATA4 item4;

	item4.space = space;
	item4.ch = ch;
	item4.originalNetworkID = serviceInfo->original_network_id;
	item4.transportStreamID = serviceInfo->transport_stream_id;
	item4.serviceID = serviceInfo->service_id;
	item4.serviceType = 0;
	item4.partialFlag = FALSE;
	item4.useViewFlag = TRUE;
	item4.remoconID = 0;
	if( serviceInfo->extInfo != NULL ){
		item4.serviceType = serviceInfo->extInfo->service_type;
		item4.partialFlag = serviceInfo->extInfo->partialReceptionFlag;
		if( IsVideoServiceType(item4.serviceType) == FALSE ){
			item4.useViewFlag = FALSE;
		}
		item4.serviceName = serviceInfo->extInfo->service_name;
		item4.chName = chName;
		if( serviceInfo->extInfo->ts_name != NULL ){
			item4.networkName = serviceInfo->extInfo->ts_name;
		}else if( serviceInfo->extInfo->network_name != NULL){
			item4.networkName = serviceInfo->extInfo->network_name;
		}
		item4.remoconID = serviceInfo->extInfo->remote_control_key_id;
	}

	map<DWORD, CH_DATA4>::const_iterator itr;
	for( itr = this->chText4.GetMap().begin(); itr != this->chText4.GetMap().end(); itr++ ){
		if( itr->second.originalNetworkID == item4.originalNetworkID &&
		    itr->second.transportStreamID == item4.transportStreamID &&
		    itr->second.serviceID == item4.serviceID &&
		    itr->second.space == item4.space &&
		    itr->second.ch == item4.ch ){
			break;
		}
	}
	if( itr == this->chText4.GetMap().end() ){
		this->chText4.AddCh(item4);
	}

	CH_DATA5 item5;

	item5.originalNetworkID = serviceInfo->original_network_id;
	item5.transportStreamID = serviceInfo->transport_stream_id;
	item5.serviceID = serviceInfo->service_id;
	item5.serviceType = 0;
	item5.partialFlag = FALSE;
	item5.epgCapFlag = TRUE;
	item5.searchFlag = TRUE;
	if( serviceInfo->extInfo != NULL ){
		item5.serviceType = serviceInfo->extInfo->service_type;
		item5.partialFlag = serviceInfo->extInfo->partialReceptionFlag;
		item5.serviceName = serviceInfo->extInfo->service_name;
		if( serviceInfo->extInfo->ts_name != NULL ){
			item5.networkName = serviceInfo->extInfo->ts_name;
		}else if( serviceInfo->extInfo->network_name != NULL){
			item5.networkName = serviceInfo->extInfo->network_name;
		}
		if( IsVideoServiceType(item4.serviceType) == FALSE ){
			item5.epgCapFlag = FALSE;
			item5.searchFlag = FALSE;
		}
	}

	this->chText5.AddCh(item5);

	return TRUE;
}


//サービス一覧を取得する
BOOL CChSetUtil::GetEnumService(
	vector<CH_DATA4>* serviceList
	)
{
	if( this->chText4.GetMap().size() == 0 ){
		return FALSE;
	}
	map<DWORD, CH_DATA4>::const_iterator itr;
	for( itr = this->chText4.GetMap().begin(); itr != this->chText4.GetMap().end(); itr++ ){
		serviceList->push_back(itr->second);
	}
	return TRUE;
}

//IDから物理チャンネルを検索する
BOOL CChSetUtil::GetCh(
	WORD ONID,
	WORD TSID,
	WORD SID,
	DWORD& space,
	DWORD& ch
	)
{
	BOOL ret = FALSE;
	map<DWORD, CH_DATA4>::const_iterator itr;
	for( itr = this->chText4.GetMap().begin(); itr != this->chText4.GetMap().end(); itr++ ){
		if( itr->second.originalNetworkID == ONID && itr->second.transportStreamID == TSID ){
			if( ret == FALSE || itr->second.serviceID == SID ){
				ret = TRUE;
				space = itr->second.space;
				ch = itr->second.ch;
				//SIDが同じものを優先する
				if( itr->second.serviceID == SID ){
					break;
				}
			}
		}
	}
	return ret;
}

vector<SET_CH_INFO> CChSetUtil::GetEpgCapService()
{
	vector<SET_CH_INFO> ret;
	map<DWORD, CH_DATA4>::const_iterator itrCh4;
	for( itrCh4 = this->chText4.GetMap().begin(); itrCh4 != this->chText4.GetMap().end(); itrCh4++ ){
		LONGLONG key = Create64Key(itrCh4->second.originalNetworkID, itrCh4->second.transportStreamID, itrCh4->second.serviceID);
		map<LONGLONG, CH_DATA5>::const_iterator itrCh5;
		itrCh5 = this->chText5.GetMap().find(key);

		if( itrCh5 != this->chText5.GetMap().end() ){
			if( itrCh5->second.epgCapFlag == TRUE ){
				SET_CH_INFO item;
				item.useBonCh = TRUE;
				item.space = itrCh4->second.space;
				item.ch = itrCh4->second.ch;
				if( std::find_if(ret.begin(), ret.end(), [&](const SET_CH_INFO& a) {
				        return a.space == item.space && a.ch == item.ch; }) == ret.end() ){
					item.useSID = TRUE;
					item.ONID = itrCh5->second.originalNetworkID;
					item.TSID = itrCh5->second.transportStreamID;
					item.SID = itrCh5->second.serviceID;
					ret.push_back(item);
				}
			}
		}
	}
	return ret;
}

vector<SET_CH_INFO> CChSetUtil::GetEpgCapServiceAll(
	int ONID,
	int TSID
	)
{
	vector<SET_CH_INFO> ret;
	map<LONGLONG, CH_DATA5>::const_iterator itrCh5;
	for( itrCh5 = this->chText5.GetMap().begin(); itrCh5 != this->chText5.GetMap().end(); itrCh5++ ){
		if( (ONID < 0 || itrCh5->second.originalNetworkID == ONID) &&
			(TSID < 0 || itrCh5->second.transportStreamID == TSID) &&
			itrCh5->second.epgCapFlag == TRUE
			){
			ret.push_back(SET_CH_INFO());
			ret.back().useSID = TRUE;
			ret.back().ONID = itrCh5->second.originalNetworkID;
			ret.back().TSID = itrCh5->second.transportStreamID;
			ret.back().SID = itrCh5->second.serviceID;
			ret.back().useBonCh = FALSE;
		}
	}
	return ret;
}

BOOL CChSetUtil::IsPartial(
	WORD ONID,
	WORD TSID,
	WORD SID
	)
{
	LONGLONG key = Create64Key(ONID, TSID, SID);
	map<LONGLONG, CH_DATA5>::const_iterator itr;
	itr = this->chText5.GetMap().find(key);
	if( itr != this->chText5.GetMap().end() ){
		if( itr->second.partialFlag == 1 ){
			return TRUE;
		}
	}
	return FALSE;
}

