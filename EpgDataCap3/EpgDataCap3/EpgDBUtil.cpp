#include "StdAfx.h"
#include "EpgDBUtil.h"

#include "../../Common/EpgTimerUtil.h"
#include "../../Common/StringUtil.h"
#include "../../Common/TimeUtil.h"
#include "../../Common/ErrDef.h"
#include "ARIB8CharDecode.h"

CEpgDBUtil::CEpgDBUtil(void)
{
	this->lockEvent = _CreateEvent(FALSE, TRUE, NULL);

	this->sectionNowFlag = 0;

	this->epgInfoList = NULL;
	this->epgInfoListSize = 0;

	this->epgInfo = NULL;
	this->searchEpgInfo = NULL;

	this->epgSearchList = NULL;
	this->epgSearchListSize = 0;

	this->serviceDBList = NULL;
	this->serviceDBListSize = 0;
}

CEpgDBUtil::~CEpgDBUtil(void)
{
	Clear();
	ClearSectionStatus();

	map<DWORD, DB_TS_INFO*>::iterator itrInfo;
	for( itrInfo = this->serviceInfoList.begin(); itrInfo != this->serviceInfoList.end(); itrInfo++ ){
		SAFE_DELETE(itrInfo->second);
	}

	SAFE_DELETE_ARRAY(this->epgInfoList);
	this->epgInfoListSize = 0;

	SAFE_DELETE(this->epgInfo);

	SAFE_DELETE(this->searchEpgInfo);

	SAFE_DELETE_ARRAY(this->epgSearchList);
	this->epgSearchListSize = 0;

	if( this->lockEvent != NULL ){
		UnLock();
		CloseHandle(this->lockEvent);
		this->lockEvent = NULL;
	}

	SAFE_DELETE_ARRAY(this->serviceDBList);
	this->serviceDBListSize = 0;
}

BOOL CEpgDBUtil::Lock(LPCWSTR log, DWORD timeOut)
{
	if( this->lockEvent == NULL ){
		return FALSE;
	}
	if( log != NULL ){
		OutputDebugString(log);
	}
	DWORD dwRet = WaitForSingleObject(this->lockEvent, timeOut);
	if( dwRet == WAIT_ABANDONED || 
		dwRet == WAIT_FAILED){
		return FALSE;
	}
	return TRUE;
}

void CEpgDBUtil::UnLock(LPCWSTR log)
{
	if( this->lockEvent != NULL ){
		SetEvent(this->lockEvent);
	}
	if( log != NULL ){
		OutputDebugString(log);
	}
}

void CEpgDBUtil::Clear()
{
	map<ULONGLONG, SERVICE_EVENT_INFO*>::iterator itr;
	for( itr = this->serviceEventMap.begin(); itr != this->serviceEventMap.end(); itr++ ){
		SAFE_DELETE(itr->second);
	}
	this->serviceEventMap.clear();

	for( itr = this->serviceEventMapSD.begin(); itr != this->serviceEventMapSD.end(); itr++ ){
		SAFE_DELETE(itr->second);
	}
	this->serviceEventMapSD.clear();
}

void CEpgDBUtil::SetStreamChangeEvent()
{
	if( Lock() == FALSE ) return ;
	//ストリーム変わったのでp/fをリセット
	map<ULONGLONG, SERVICE_EVENT_INFO*>::iterator itr;
	for( itr = this->serviceEventMap.begin(); itr != this->serviceEventMap.end(); itr++ ){
		itr->second->nowEvent = NULL;
		itr->second->nextEvent = NULL;
	}
	UnLock();
}

BOOL CEpgDBUtil::AddEIT(WORD PID, CEITTable* eit)
{
	if( eit == NULL ){
		return FALSE;
	}
	if( Lock() == FALSE ) return FALSE;

	ULONGLONG key = _Create64Key(eit->original_network_id, eit->transport_stream_id, eit->service_id);

	//サービスのmapを取得
	map<ULONGLONG, SERVICE_EVENT_INFO*>::iterator itr;
	SERVICE_EVENT_INFO* serviceInfo = NULL;

	itr = serviceEventMap.find(key);
	if( itr == serviceEventMap.end() ){
		serviceInfo = new SERVICE_EVENT_INFO;
		serviceEventMap.insert(pair<ULONGLONG, SERVICE_EVENT_INFO*>(key, serviceInfo));
	}else{
		serviceInfo = itr->second;
	}

	if( !eit->failure ){
	//イベントごとに更新必要が判定
	for( size_t i=0; i<eit->eventInfoList.size(); i++ ){
		CEITTable::EVENT_INFO_DATA* eitEventInfo = eit->eventInfoList[i];
		map<WORD, EVENT_INFO*>::iterator itrEvent;
		EVENT_INFO* eventInfo = NULL;

		if( eitEventInfo->running_status == 1 || eitEventInfo->running_status == 3 ){
			//非実行中または停止中
			_OutputDebugString(L"★非実行中または停止中イベント ONID:0x%04x TSID:0x%04x SID:0x%04x EventID:0x%04x %04d/%02d/%02d %02d:%02d",
				eit->original_network_id,  eit->transport_stream_id, eit->service_id, eitEventInfo->event_id,
				eitEventInfo->start_time.wYear, eitEventInfo->start_time.wMonth, eitEventInfo->start_time.wDay, eitEventInfo->start_time.wHour, eitEventInfo->start_time.wMinute
				);
			continue;
		}

		itrEvent = serviceInfo->eventMap.find(eitEventInfo->event_id);
		if( itrEvent == serviceInfo->eventMap.end() ){
			eventInfo = new EVENT_INFO;
			eventInfo->ONID = eit->original_network_id;
			eventInfo->TSID = eit->transport_stream_id;
			eventInfo->SID = eit->service_id;
			eventInfo->event_id = eitEventInfo->event_id;
			eventInfo->StartTimeFlag = eitEventInfo->StartTimeFlag;
			eventInfo->start_time = eitEventInfo->start_time;
			eventInfo->DurationFlag = eitEventInfo->DurationFlag;
			eventInfo->durationSec = eitEventInfo->durationHH*60*60 +
				eitEventInfo->durationMM*60 +
				eitEventInfo->durationSS;
			eventInfo->freeCAFlag = eitEventInfo->free_CA_mode;
			serviceInfo->eventMap.insert(pair<WORD, EVENT_INFO*>(eventInfo->event_id, eventInfo));
		}else{
			eventInfo = itrEvent->second;
		}
		if( eit->table_id == 0x4E || eit->table_id == 0x4F ){
			//p/fなので時間更新
			eventInfo->StartTimeFlag = eitEventInfo->StartTimeFlag;
			eventInfo->start_time = eitEventInfo->start_time;
			eventInfo->DurationFlag = eitEventInfo->DurationFlag;
			eventInfo->durationSec = eitEventInfo->durationHH*60*60 +
				eitEventInfo->durationMM*60 +
				eitEventInfo->durationSS;

			if( eit->section_number == 0 ){
				serviceInfo->nowEvent = eventInfo;
				eventInfo->pfFlag = TRUE;
			}else if( eit->section_number == 1 ){
				serviceInfo->nextEvent = eventInfo;
				eventInfo->pfFlag = TRUE;
			}
		}else if( 0x50 <= eit->table_id && eit->table_id <= 0x5F ){
			if( serviceInfo->nowEvent != NULL && serviceInfo->nextEvent != NULL ){
				if( serviceInfo->nowEvent->event_id != eitEventInfo->event_id &&
					serviceInfo->nextEvent->event_id != eitEventInfo->event_id &&
					eventInfo->pfFlag == FALSE){
					//自ストリームでp/fじゃないなら時間更新
					eventInfo->StartTimeFlag = eitEventInfo->StartTimeFlag;
					eventInfo->start_time = eitEventInfo->start_time;
					eventInfo->DurationFlag = eitEventInfo->DurationFlag;
					eventInfo->durationSec = eitEventInfo->durationHH*60*60 +
						eitEventInfo->durationMM*60 +
						eitEventInfo->durationSS;
				}
			}
		}

		//ExtendedEventは複数あるので1度だけチェックする
		BOOL checkExtFlag = FALSE;
		BOOL checkAudioFlag = FALSE;
		for( size_t j=0; j<eitEventInfo->descriptorList.size(); j++ ){
			if( eitEventInfo->descriptorList[j]->shortEvent != NULL ){
				AddShortEvent( eit, eventInfo, eitEventInfo->descriptorList[j]->shortEvent );
			}else if( eitEventInfo->descriptorList[j]->extendedEvent != NULL && checkExtFlag == FALSE){
				AddExtEvent(eit, eventInfo, &eitEventInfo->descriptorList );
				checkExtFlag = TRUE;
			}else if( eitEventInfo->descriptorList[j]->content != NULL ){
				AddContent( eit, eventInfo, eitEventInfo->descriptorList[j]->content );
			}else if( eitEventInfo->descriptorList[j]->component != NULL ){
				AddComponent( eit, eventInfo, eitEventInfo->descriptorList[j]->component );
			}else if( eitEventInfo->descriptorList[j]->audioComponent != NULL && checkAudioFlag == FALSE ){
				AddAudioComponent( eit, eventInfo, &eitEventInfo->descriptorList );
				checkAudioFlag = TRUE;
			}else if( eitEventInfo->descriptorList[j]->eventGroup != NULL ){
				if( eitEventInfo->descriptorList[j]->eventGroup->group_type == 0x01 ){
					AddEventGroup( eit, eventInfo, eitEventInfo->descriptorList[j]->eventGroup );
				}else if( eitEventInfo->descriptorList[j]->eventGroup->group_type == 0x02 ||
					eitEventInfo->descriptorList[j]->eventGroup->group_type == 0x04){
					AddEventRelay( eit, eventInfo, eitEventInfo->descriptorList[j]->eventGroup );
				}
			}
		}
	}
	}

	if( eit->original_network_id == 0x0003 ){
		UnLock();

		return !eit->failure;
	}
	
	//セクションステータス
	map<ULONGLONG, SECTION_STATUS_INFO*>::iterator itrSec;
	SECTION_STATUS_INFO* sectionInfo = NULL;
	itrSec = this->sectionMap.find(key);
	if( itrSec == this->sectionMap.end() ){
		sectionInfo = new SECTION_STATUS_INFO;
		this->sectionMap.insert(pair<ULONGLONG, SECTION_STATUS_INFO*>(key, sectionInfo));
	}else{
		sectionInfo = itrSec->second;
	}

	if( PID == 0x0027 ){
		//L-EIT
		sectionInfo->HEITFlag = FALSE;
		sectionInfo->last_table_idBasic = eit->last_table_id;
		sectionInfo->last_section_numberBasic = eit->last_section_number;

		DWORD sectionNo = eit->section_number;
		map<WORD, SECTION_FLAG_INFO>::iterator itrFlag;
		itrFlag = sectionInfo->sectionBasicMap.find(eit->table_id);
		if( itrFlag == sectionInfo->sectionBasicMap.end() ){
			DWORD maxFlag = 0;
			for( DWORD i=0; i<=eit->last_section_number; i++ ){
				maxFlag |= 1<<i;
			}
			SECTION_FLAG_INFO item;
			item.maxFlag = maxFlag;
			item.sectionFlag = 1<<sectionNo;
			sectionInfo->sectionBasicMap.insert(pair<WORD, SECTION_FLAG_INFO>(eit->table_id, item));
		}else{
			itrFlag->second.sectionFlag |= 1<<sectionNo;
		}
		if (eit->failure){
			UnLock();
			return FALSE;
		}
	}else{
		//H-EIT
		sectionInfo->HEITFlag = TRUE;
		if( eit->section_number == eit->segment_last_section_number ){
			if( 0x50 <= eit->table_id && eit->table_id <= 0x57 ||
				0x60 <= eit->table_id && eit->table_id <= 0x67){
				sectionInfo->last_table_idBasic = eit->last_table_id;
				sectionInfo->last_section_numberBasic = eit->last_section_number;

				DWORD sectionNo = eit->section_number >> 3;
				map<WORD, SECTION_FLAG_INFO>::iterator itrFlag;
				itrFlag = sectionInfo->sectionBasicMap.find(eit->table_id);
				if( itrFlag == sectionInfo->sectionBasicMap.end() ){
					DWORD maxFlag = 0;
					for( DWORD i=0; i<=((DWORD)eit->last_section_number)>>3; i++ ){
						maxFlag |= 1<<i;
					}
					SECTION_FLAG_INFO item;
					item.maxFlag = maxFlag;
					item.sectionFlag = 1<<sectionNo;
					sectionInfo->sectionBasicMap.insert(pair<WORD, SECTION_FLAG_INFO>(eit->table_id, item));
				}else{
					itrFlag->second.sectionFlag |= (DWORD)1<<sectionNo;
				}
			}
			if( 0x58 <= eit->table_id && eit->table_id <= 0x5F ||
				0x68 <= eit->table_id && eit->table_id <= 0x6F){
				sectionInfo->last_table_idExt = eit->last_table_id;
				sectionInfo->last_section_numberExt = eit->last_section_number;

				DWORD sectionNo = eit->section_number >> 3;
				map<WORD, SECTION_FLAG_INFO>::iterator itrFlag;
				itrFlag = sectionInfo->sectionExtMap.find(eit->table_id);
				if( itrFlag == sectionInfo->sectionExtMap.end() ){
					DWORD maxFlag = 0;
					for( DWORD i=0; i<=((DWORD)eit->last_section_number)>>3; i++ ){
						maxFlag |= 1<<i;
					}
					SECTION_FLAG_INFO item;
					item.maxFlag = maxFlag;
					item.sectionFlag = 1<<sectionNo;
					sectionInfo->sectionExtMap.insert(pair<WORD, SECTION_FLAG_INFO>(eit->table_id, item));
				}else{
					itrFlag->second.sectionFlag |= (DWORD)1<<sectionNo;
				}
			}
		}
		if (eit->failure){
			UnLock();
			return FALSE;
		}
		if( eit->table_id == 0x4E && eit->section_number == 0){
			//現在の番組のはずなので、そこまでのセクションはすでに放送済み
			if(eit->eventInfoList.size() > 0){
				if( eit->eventInfoList[0]->StartTimeFlag == TRUE ){
					WORD sectionNo = 0;
					if( eit->eventInfoList[0]->DurationFlag == FALSE ){
						sectionNo = eit->eventInfoList[0]->start_time.wHour / 3;
					}else{
						SYSTEMTIME endTime;
						int DureSec = eit->eventInfoList[0]->durationHH*60*60 + eit->eventInfoList[0]->durationMM*60 + eit->eventInfoList[0]->durationSS;
						GetSumTime(eit->eventInfoList[0]->start_time, DureSec, &endTime);
						if( eit->eventInfoList[0]->start_time.wDay != endTime.wDay ){
							//日付変わってるので今日の分は全部終わってるはず
							sectionNo = 7;
						}else{
							sectionNo = endTime.wHour / 3;
						}
					}
					DWORD flag = 0;
					for( WORD i=0; i<=sectionNo; i++ ){
						flag |= 1<<i;
					}
					if(	this->sectionNowFlag != flag ){
						this->sectionNowFlag = flag;
					}
				}
			}
		}
	}
	UnLock();

	return TRUE;
}

BOOL CEpgDBUtil::AddEIT_SD(WORD PID, CEITTable_SD* eit)
{
	if( eit == NULL ){
		return FALSE;
	}
	if( Lock() == FALSE ) return FALSE;

	if( eit->original_network_id == 0x0003 && eit->table_id != 0x4E && eit->table_id != 0x4F){
		BOOL ret = AddSDEventMap(eit);
		UnLock();
		return ret;
	}

	ULONGLONG key = _Create64Key(eit->original_network_id, eit->transport_stream_id, eit->service_id);

	//サービスのmapを取得
	map<ULONGLONG, SERVICE_EVENT_INFO*>::iterator itr;
	SERVICE_EVENT_INFO* serviceInfo = NULL;

	itr = serviceEventMap.find(key);
	if( itr == serviceEventMap.end() ){
		serviceInfo = new SERVICE_EVENT_INFO;
		serviceEventMap.insert(pair<ULONGLONG, SERVICE_EVENT_INFO*>(key, serviceInfo));
	}else{
		serviceInfo = itr->second;
	}

	//イベントごとに更新必要が判定
	for( size_t i=0; i<eit->eventInfoList.size(); i++ ){
		CEITTable_SD::EVENT_INFO_DATA* eitEventInfo = eit->eventInfoList[i];
		map<WORD, EVENT_INFO*>::iterator itrEvent;
		EVENT_INFO* eventInfo = NULL;

		if( eitEventInfo->running_status != 0 && eitEventInfo->running_status != 2 && eitEventInfo->running_status != 4 ){
			//非実行中または停止中
			_OutputDebugString(L"★非実行中または停止中イベント ONID:0x%04x TSID:0x%04x SID:0x%04x EventID:0x%04x %04d/%02d/%02d %02d:%02d\r\n",
				eit->original_network_id,  eit->transport_stream_id, eit->service_id, eitEventInfo->event_id,
				eitEventInfo->start_time.wYear, eitEventInfo->start_time.wMonth, eitEventInfo->start_time.wDay, eitEventInfo->start_time.wHour, eitEventInfo->start_time.wMinute
				);
			continue;
		}

		itrEvent = serviceInfo->eventMap.find(eitEventInfo->event_id);
		if( itrEvent == serviceInfo->eventMap.end() ){
			eventInfo = new EVENT_INFO;
			eventInfo->ONID = eit->original_network_id;
			eventInfo->TSID = eit->transport_stream_id;
			eventInfo->SID = eit->service_id;
			eventInfo->event_id = eitEventInfo->event_id;
			eventInfo->StartTimeFlag = eitEventInfo->StartTimeFlag;
			eventInfo->start_time = eitEventInfo->start_time;
			eventInfo->DurationFlag = eitEventInfo->DurationFlag;
			eventInfo->durationSec = eitEventInfo->durationHH*60*60 +
				eitEventInfo->durationMM*60 +
				eitEventInfo->durationSS;
			eventInfo->freeCAFlag = eitEventInfo->free_CA_mode;
			serviceInfo->eventMap.insert(pair<WORD, EVENT_INFO*>(eventInfo->event_id, eventInfo));
		}else{
			eventInfo = itrEvent->second;
		}
		if( 0xA0 <= eit->table_id && eit->table_id <= 0xAF ){
			if( serviceInfo->nowEvent != NULL && serviceInfo->nextEvent != NULL ){
				if( serviceInfo->nowEvent->event_id != eitEventInfo->event_id &&
					serviceInfo->nextEvent->event_id != eitEventInfo->event_id &&
					eventInfo->pfFlag == FALSE){
					//自ストリームでp/fじゃないなら時間更新
					eventInfo->StartTimeFlag = eitEventInfo->StartTimeFlag;
					eventInfo->start_time = eitEventInfo->start_time;
					eventInfo->DurationFlag = eitEventInfo->DurationFlag;
					eventInfo->durationSec = eitEventInfo->durationHH*60*60 +
						eitEventInfo->durationMM*60 +
						eitEventInfo->durationSS;
				}
			}
		}

		//ExtendedEventは複数あるので1度だけチェックする
		BOOL checkExtFlag = FALSE;
		BOOL checkAudioFlag = FALSE;
		for( size_t j=0; j<eitEventInfo->descriptorList.size(); j++ ){
			if( eitEventInfo->descriptorList[j]->shortEvent != NULL ){
				AddShortEvent_SD( eit, eventInfo, eitEventInfo->descriptorList[j]->shortEvent );
			}else if( eitEventInfo->descriptorList[j]->extendedEvent != NULL && checkExtFlag == FALSE){
				AddExtEvent_SD(eit, eventInfo, &eitEventInfo->descriptorList );
				checkExtFlag = TRUE;
			}else if( eitEventInfo->descriptorList[j]->content != NULL ){
				AddContent_SD( eit, eventInfo, eitEventInfo->descriptorList[j]->content );
			}else if( eitEventInfo->descriptorList[j]->component != NULL ){
				AddComponent_SD( eit, eventInfo, eitEventInfo->descriptorList[j]->component );
			}else if( eitEventInfo->descriptorList[j]->audioComponent != NULL && checkAudioFlag == FALSE ){
				AddAudioComponent_SD( eit, eventInfo, &eitEventInfo->descriptorList );
				checkAudioFlag = TRUE;
			//}else if( eitEventInfo->descriptorList[j]->eventGroup != NULL ){
			//	if( eitEventInfo->descriptorList[j]->eventGroup->group_type == 0x01 ){
			//		AddEventGroup( eit, eventInfo, eitEventInfo->descriptorList[j]->eventGroup );
			//	}else if( eitEventInfo->descriptorList[j]->eventGroup->group_type == 0x02 ||
			//		eitEventInfo->descriptorList[j]->eventGroup->group_type == 0x04){
			//		AddEventRelay( eit, eventInfo, eitEventInfo->descriptorList[j]->eventGroup );
			//	}
			}
		}
	}
	
	//セクションステータス
	//map<ULONGLONG, SECTION_STATUS_INFO*>::iterator itrSec;
	//SECTION_STATUS_INFO* sectionInfo = NULL;
	//itrSec = this->sectionMap.find(key);
	//if( itrSec == this->sectionMap.end() ){
	//	sectionInfo = new SECTION_STATUS_INFO;
	//	this->sectionMap.insert(pair<ULONGLONG, SECTION_STATUS_INFO*>(key, sectionInfo));
	//}else{
	//	sectionInfo = itrSec->second;
	//}

	//if( PID == 0x0027 ){
	//	//L-EIT
	//	sectionInfo->HEITFlag = FALSE;
	//	sectionInfo->last_table_idBasic = eit->last_table_id;
	//	sectionInfo->last_section_numberBasic = eit->last_section_number;

	//	DWORD sectionNo = eit->section_number;
	//	map<WORD, SECTION_FLAG_INFO>::iterator itrFlag;
	//	itrFlag = sectionInfo->sectionBasicMap.find(eit->table_id);
	//	if( itrFlag == sectionInfo->sectionBasicMap.end() ){
	//		DWORD maxFlag = 0;
	//		for( DWORD i=0; i<=eit->last_section_number; i++ ){
	//			maxFlag |= 1<<i;
	//		}
	//		SECTION_FLAG_INFO item;
	//		item.maxFlag = maxFlag;
	//		item.sectionFlag = 1<<sectionNo;
	//		sectionInfo->sectionBasicMap.insert(pair<WORD, SECTION_FLAG_INFO>(eit->table_id, item));
	//	}else{
	//		itrFlag->second.sectionFlag |= 1<<sectionNo;
	//	}

	//}else{
	//	//H-EIT
	//	sectionInfo->HEITFlag = TRUE;
	//	if( eit->section_number == eit->segment_last_section_number ){
	//		if( 0x50 <= eit->table_id && eit->table_id <= 0x57 ||
	//			0x60 <= eit->table_id && eit->table_id <= 0x67){
	//			sectionInfo->last_table_idBasic = eit->last_table_id;
	//			sectionInfo->last_section_numberBasic = eit->last_section_number;

	//			DWORD sectionNo = eit->section_number >> 3;
	//			map<WORD, SECTION_FLAG_INFO>::iterator itrFlag;
	//			itrFlag = sectionInfo->sectionBasicMap.find(eit->table_id);
	//			if( itrFlag == sectionInfo->sectionBasicMap.end() ){
	//				DWORD maxFlag = 0;
	//				for( DWORD i=0; i<=((DWORD)eit->last_section_number)>>3; i++ ){
	//					maxFlag |= 1<<i;
	//				}
	//				SECTION_FLAG_INFO item;
	//				item.maxFlag = maxFlag;
	//				item.sectionFlag = 1<<sectionNo;
	//				sectionInfo->sectionBasicMap.insert(pair<WORD, SECTION_FLAG_INFO>(eit->table_id, item));
	//			}else{
	//				itrFlag->second.sectionFlag |= (DWORD)1<<sectionNo;
	//			}
	//		}
	//		if( 0x58 <= eit->table_id && eit->table_id <= 0x5F ||
	//			0x68 <= eit->table_id && eit->table_id <= 0x6F){
	//			sectionInfo->last_table_idExt = eit->last_table_id;
	//			sectionInfo->last_section_numberExt = eit->last_section_number;

	//			DWORD sectionNo = eit->section_number >> 3;
	//			map<WORD, SECTION_FLAG_INFO>::iterator itrFlag;
	//			itrFlag = sectionInfo->sectionExtMap.find(eit->table_id);
	//			if( itrFlag == sectionInfo->sectionExtMap.end() ){
	//				DWORD maxFlag = 0;
	//				for( DWORD i=0; i<=((DWORD)eit->last_section_number)>>3; i++ ){
	//					maxFlag |= 1<<i;
	//				}
	//				SECTION_FLAG_INFO item;
	//				item.maxFlag = maxFlag;
	//				item.sectionFlag = 1<<sectionNo;
	//				sectionInfo->sectionExtMap.insert(pair<WORD, SECTION_FLAG_INFO>(eit->table_id, item));
	//			}else{
	//				itrFlag->second.sectionFlag |= (DWORD)1<<sectionNo;
	//			}
	//		}
	//	}
	//	if( eit->table_id == 0x4E && eit->section_number == 0){
	//		//現在の番組のはずなので、そこまでのセクションはすでに放送済み
	//		if(eit->eventInfoList.size() > 0){
	//			if( eit->eventInfoList[0]->StartTimeFlag == TRUE ){
	//				WORD sectionNo = 0;
	//				if( eit->eventInfoList[0]->DurationFlag == FALSE ){
	//					sectionNo = eit->eventInfoList[0]->start_time.wHour / 3;
	//				}else{
	//					SYSTEMTIME endTime;
	//					int DureSec = eit->eventInfoList[0]->durationHH*60*60 + eit->eventInfoList[0]->durationMM*60 + eit->eventInfoList[0]->durationSS;
	//					GetSumTime(eit->eventInfoList[0]->start_time, DureSec, &endTime);
	//					if( eit->eventInfoList[0]->start_time.wDay != endTime.wDay ){
	//						//日付変わってるので今日の分は全部終わってるはず
	//						sectionNo = 7;
	//					}else{
	//						sectionNo = endTime.wHour / 3;
	//					}
	//				}
	//				DWORD flag = 0;
	//				for( WORD i=0; i<=sectionNo; i++ ){
	//					flag |= 1<<i;
	//				}
	//				if(	this->sectionNowFlag != flag ){
	//					this->sectionNowFlag = flag;
	//				}
	//			}
	//		}
	//	}
	//}
	UnLock();

	return TRUE;
}

BOOL CEpgDBUtil::AddShortEvent(CEITTable* eit, EVENT_INFO* eventInfo, CShortEventDesc* shortEvent)
{
	BOOL updateFlag = FALSE;
	if( eventInfo->shortInfo == NULL ){
		eventInfo->shortInfo = new SHORT_EVENT_INFO;
		updateFlag = TRUE;
	}else{
		updateFlag = CheckUpdate(eit, eventInfo->shortInfo->tableID, eventInfo->shortInfo->version);
	}
	if( updateFlag == TRUE ){
		//更新必要
		eventInfo->shortInfo->tableID = eit->table_id;
		eventInfo->shortInfo->version = eit->version_number;

		CARIB8CharDecode arib;
		string event_name = "";
		string text_char = "";
		arib.PSISI((const BYTE*)shortEvent->event_name_char, shortEvent->event_name_length, &event_name);
		arib.PSISI((const BYTE*)shortEvent->text_char, shortEvent->text_length, &text_char);

		AtoW(event_name, eventInfo->shortInfo->event_name);
		AtoW(text_char, eventInfo->shortInfo->text_char);
	}

	return updateFlag;
}

BOOL CEpgDBUtil::AddShortEvent_SD(CEITTable_SD* eit, EVENT_INFO* eventInfo, CShortEventDesc* shortEvent)
{
	BOOL updateFlag = FALSE;
	if( eventInfo->shortInfo == NULL ){
		eventInfo->shortInfo = new SHORT_EVENT_INFO;
		updateFlag = TRUE;
	}else{
		updateFlag = CheckUpdate_SD(eit, eventInfo->shortInfo->tableID, eventInfo->shortInfo->version);
	}
	if( updateFlag == TRUE ){
		//更新必要
		eventInfo->shortInfo->tableID = eit->table_id;
		eventInfo->shortInfo->version = eit->version_number;

		CARIB8CharDecode arib;
		string event_name = "";
		string text_char = "";
		arib.PSISI((const BYTE*)shortEvent->event_name_char, shortEvent->event_name_length, &event_name);
		arib.PSISI((const BYTE*)shortEvent->text_char, shortEvent->text_length, &text_char);

		AtoW(event_name, eventInfo->shortInfo->event_name);
		AtoW(text_char, eventInfo->shortInfo->text_char);
	}

	return updateFlag;
}

BOOL CEpgDBUtil::AddExtEvent(CEITTable* eit, EVENT_INFO* eventInfo, vector<DESCRIPTOR_DATA*>* descriptorList)
{
	BOOL updateFlag = FALSE;
	if( eventInfo->extInfo == NULL ){
		eventInfo->extInfo = new EXTENDED_EVENT_INFO;

		updateFlag = TRUE;
	}else{
		updateFlag = CheckUpdate(eit, eventInfo->extInfo->tableID, eventInfo->extInfo->version);
	}

	if( updateFlag == TRUE ){
		//更新必要
		eventInfo->extInfo->tableID = eit->table_id;
		eventInfo->extInfo->version = eit->version_number;

		CARIB8CharDecode arib;
		string extendText = "";
		string itemDescBuff = "";
		string itemBuff = "";
		//text_lengthは0で運用される
//		string textBuff = "";

		for( size_t i=0; i<descriptorList->size(); i++ ){
			if( (*descriptorList)[i]->extendedEvent != NULL ){
				CExtendedEventDesc* extEvent = (*descriptorList)[i]->extendedEvent;
				for( size_t j=0; j<extEvent->itemList.size(); j++ ){
					CExtendedEventDesc::ITEM_DATA* item = extEvent->itemList[j];
					if( item->item_description_length > 0 ){
						//if( textBuff.size() > 0 ){
						//	string buff = "";
						//	arib.PSISI((const BYTE*)textBuff.c_str(), textBuff.length(), &buff);
						//	buff += "\r\n";
						//	extendText += buff;
						//	textBuff = "";
						//}
						if( itemBuff.size() > 0 ){
							string buff = "";
							arib.PSISI((const BYTE*)itemBuff.c_str(), (DWORD)itemBuff.length(), &buff);
							buff += "\r\n";
							extendText += buff;
							itemBuff = "";
						}

						itemDescBuff += item->item_description_char;
					}
					if( item->item_length > 0 ){
						//if( textBuff.size() > 0 ){
						//	string buff = "";
						//	arib.PSISI((const BYTE*)textBuff.c_str(), textBuff.length(), &buff);
						//	buff += "\r\n";
						//	extendText += buff;
						//	textBuff = "";
						//}
						if( itemDescBuff.size() > 0 ){
							string buff = "";
							arib.PSISI((const BYTE*)itemDescBuff.c_str(), (DWORD)itemDescBuff.length(), &buff);
							buff += "\r\n";
							extendText += buff;
							itemDescBuff = "";
						}

						itemBuff += item->item_char;
					}
				}
				//if( extEvent->text_length > 0 ){
				//	if( itemDescBuff.size() > 0 ){
				//		string buff = "";
				//		arib.PSISI((const BYTE*)itemDescBuff.c_str(), itemDescBuff.length(), &buff);
				//		buff += "\r\n";
				//		extendText += buff;
				//		itemDescBuff = "";
				//	}
				//	if( itemBuff.size() > 0 ){
				//		string buff = "";
				//		arib.PSISI((const BYTE*)itemBuff.c_str(), itemBuff.length(), &buff);
				//		buff += "\r\n";
				//		extendText += buff;
				//		itemBuff = "";
				//	}

				//	textBuff += extEvent->text_char;
				//}
			}
		}

		if( itemDescBuff.size() > 0 ){
			string buff = "";
			arib.PSISI((const BYTE*)itemDescBuff.c_str(), (DWORD)itemDescBuff.length(), &buff);
			buff += "\r\n";
			extendText += buff;
			itemDescBuff = "";
		}
		if( itemBuff.size() > 0 ){
			string buff = "";
			arib.PSISI((const BYTE*)itemBuff.c_str(), (DWORD)itemBuff.length(), &buff);
			buff += "\r\n";
			extendText += buff;
			itemBuff = "";
		}
		//if( textBuff.size() > 0 ){
		//	string buff = "";
		//	arib.PSISI((const BYTE*)textBuff.c_str(), textBuff.length(), &buff);
		//	buff += "\r\n";
		//	extendText += buff;
		//	textBuff = "";
		//}

		AtoW(extendText, eventInfo->extInfo->text_char);
	}

	return updateFlag;
}

BOOL CEpgDBUtil::AddExtEvent_SD(CEITTable_SD* eit, EVENT_INFO* eventInfo, vector<DESCRIPTOR_DATA*>* descriptorList)
{
	BOOL updateFlag = FALSE;
	if( eventInfo->extInfo == NULL ){
		eventInfo->extInfo = new EXTENDED_EVENT_INFO;

		updateFlag = TRUE;
	}else{
		updateFlag = CheckUpdate_SD(eit, eventInfo->extInfo->tableID, eventInfo->extInfo->version);
	}

	if( updateFlag == TRUE ){
		//更新必要
		eventInfo->extInfo->tableID = eit->table_id;
		eventInfo->extInfo->version = eit->version_number;

		CARIB8CharDecode arib;
		string extendText = "";
		string itemDescBuff = "";
		string itemBuff = "";
		//text_lengthは0で運用される
//		string textBuff = "";

		for( size_t i=0; i<descriptorList->size(); i++ ){
			if( (*descriptorList)[i]->extendedEvent != NULL ){
				CExtendedEventDesc* extEvent = (*descriptorList)[i]->extendedEvent;
				for( size_t j=0; j<extEvent->itemList.size(); j++ ){
					CExtendedEventDesc::ITEM_DATA* item = extEvent->itemList[j];
					if( item->item_description_length > 0 ){
						//if( textBuff.size() > 0 ){
						//	string buff = "";
						//	arib.PSISI((const BYTE*)textBuff.c_str(), textBuff.length(), &buff);
						//	buff += "\r\n";
						//	extendText += buff;
						//	textBuff = "";
						//}
						if( itemBuff.size() > 0 ){
							string buff = "";
							arib.PSISI((const BYTE*)itemBuff.c_str(), (DWORD)itemBuff.length(), &buff);
							buff += "\r\n";
							extendText += buff;
							itemBuff = "";
						}

						itemDescBuff += item->item_description_char;
					}
					if( item->item_length > 0 ){
						//if( textBuff.size() > 0 ){
						//	string buff = "";
						//	arib.PSISI((const BYTE*)textBuff.c_str(), textBuff.length(), &buff);
						//	buff += "\r\n";
						//	extendText += buff;
						//	textBuff = "";
						//}
						if( itemDescBuff.size() > 0 ){
							string buff = "";
							arib.PSISI((const BYTE*)itemDescBuff.c_str(), (DWORD)itemDescBuff.length(), &buff);
							buff += "\r\n";
							extendText += buff;
							itemDescBuff = "";
						}

						itemBuff += item->item_char;
					}
				}
				//if( extEvent->text_length > 0 ){
				//	if( itemDescBuff.size() > 0 ){
				//		string buff = "";
				//		arib.PSISI((const BYTE*)itemDescBuff.c_str(), itemDescBuff.length(), &buff);
				//		buff += "\r\n";
				//		extendText += buff;
				//		itemDescBuff = "";
				//	}
				//	if( itemBuff.size() > 0 ){
				//		string buff = "";
				//		arib.PSISI((const BYTE*)itemBuff.c_str(), itemBuff.length(), &buff);
				//		buff += "\r\n";
				//		extendText += buff;
				//		itemBuff = "";
				//	}

				//	textBuff += extEvent->text_char;
				//}
			}
		}

		if( itemDescBuff.size() > 0 ){
			string buff = "";
			arib.PSISI((const BYTE*)itemDescBuff.c_str(), (DWORD)itemDescBuff.length(), &buff);
			buff += "\r\n";
			extendText += buff;
			itemDescBuff = "";
		}
		if( itemBuff.size() > 0 ){
			string buff = "";
			arib.PSISI((const BYTE*)itemBuff.c_str(), (DWORD)itemBuff.length(), &buff);
			buff += "\r\n";
			extendText += buff;
			itemBuff = "";
		}
		//if( textBuff.size() > 0 ){
		//	string buff = "";
		//	arib.PSISI((const BYTE*)textBuff.c_str(), textBuff.length(), &buff);
		//	buff += "\r\n";
		//	extendText += buff;
		//	textBuff = "";
		//}

		AtoW(extendText, eventInfo->extInfo->text_char);
	}

	return updateFlag;
}

BOOL CEpgDBUtil::AddContent(CEITTable* eit, EVENT_INFO* eventInfo, CContentDesc* content)
{
	BOOL updateFlag = FALSE;
	if( eventInfo->contentInfo == NULL ){
		eventInfo->contentInfo = new CONTEN_INFO;
		updateFlag = TRUE;
	}else{
		updateFlag = CheckUpdate(eit, eventInfo->contentInfo->tableID, eventInfo->contentInfo->version);
	}
	if( updateFlag == TRUE ){
		//更新必要
		eventInfo->contentInfo->tableID = eit->table_id;
		eventInfo->contentInfo->version = eit->version_number;

		eventInfo->contentInfo->nibbleList = content->nibbleList;
	}

	return updateFlag;
}

BOOL CEpgDBUtil::AddContent_SD(CEITTable_SD* eit, EVENT_INFO* eventInfo, CContentDesc* content)
{
	BOOL updateFlag = FALSE;
	if( eventInfo->contentInfo == NULL ){
		eventInfo->contentInfo = new CONTEN_INFO;
		updateFlag = TRUE;
	}else{
		updateFlag = CheckUpdate_SD(eit, eventInfo->contentInfo->tableID, eventInfo->contentInfo->version);
	}
	if( updateFlag == TRUE ){
		//更新必要
		eventInfo->contentInfo->tableID = eit->table_id;
		eventInfo->contentInfo->version = eit->version_number;

		eventInfo->contentInfo->nibbleList = content->nibbleList;
	}

	return updateFlag;
}

BOOL CEpgDBUtil::AddComponent(CEITTable* eit, EVENT_INFO* eventInfo, CComponentDesc* component)
{
	BOOL updateFlag = FALSE;
	if( eventInfo->componentInfo == NULL ){
		eventInfo->componentInfo = new COMPONENT_INFO;
		updateFlag = TRUE;
	}else{
		updateFlag = CheckUpdate(eit, eventInfo->componentInfo->tableID, eventInfo->componentInfo->version);
	}
	if( updateFlag == TRUE ){
		//更新必要
		eventInfo->componentInfo->tableID = eit->table_id;
		eventInfo->componentInfo->version = eit->version_number;

		eventInfo->componentInfo->stream_content = component->stream_content;
		eventInfo->componentInfo->component_type = component->component_type;
		eventInfo->componentInfo->component_tag = component->component_tag;

		CARIB8CharDecode arib;
		string text_char = "";
		if( component->text_charLength > 0 ){
			arib.PSISI((const BYTE*)component->text_char, component->text_charLength, &text_char);

			AtoW(text_char, eventInfo->componentInfo->text_char);
		}

	}

	return updateFlag;
}

BOOL CEpgDBUtil::AddComponent_SD(CEITTable_SD* eit, EVENT_INFO* eventInfo, CComponentDesc* component)
{
	BOOL updateFlag = FALSE;
	if( eventInfo->componentInfo == NULL ){
		eventInfo->componentInfo = new COMPONENT_INFO;
		updateFlag = TRUE;
	}else{
		updateFlag = CheckUpdate_SD(eit, eventInfo->componentInfo->tableID, eventInfo->componentInfo->version);
	}
	if( updateFlag == TRUE ){
		//更新必要
		eventInfo->componentInfo->tableID = eit->table_id;
		eventInfo->componentInfo->version = eit->version_number;

		eventInfo->componentInfo->stream_content = component->stream_content;
		eventInfo->componentInfo->component_type = component->component_type;
		eventInfo->componentInfo->component_tag = component->component_tag;

		CARIB8CharDecode arib;
		string text_char = "";
		if( component->text_charLength > 0 ){
			arib.PSISI((const BYTE*)component->text_char, component->text_charLength, &text_char);

			AtoW(text_char, eventInfo->componentInfo->text_char);
		}

	}

	return updateFlag;
}

BOOL CEpgDBUtil::AddAudioComponent(CEITTable* eit, EVENT_INFO* eventInfo, vector<DESCRIPTOR_DATA*>* descriptorList)
{
	BOOL updateFlag = FALSE;
	if( eventInfo->audioInfo == NULL ){
		eventInfo->audioInfo = new AUDIO_COMPONENT_INFO;
		updateFlag = TRUE;
	}else{
		updateFlag = CheckUpdate(eit, eventInfo->audioInfo->tableID, eventInfo->audioInfo->version);
	}
	if( updateFlag == TRUE ){
		//更新必要
		eventInfo->audioInfo->tableID = eit->table_id;
		eventInfo->audioInfo->version = eit->version_number;
		eventInfo->audioInfo->componentList.clear();

		for( size_t i=0; i<descriptorList->size(); i++ ){
			if( (*descriptorList)[i]->audioComponent != NULL ){
				CAudioComponentDesc* audioComponent = (*descriptorList)[i]->audioComponent;
				AUDIO_COMPONENT_INFO_DATA item;

				item.stream_content = audioComponent->stream_content;
				item.component_type = audioComponent->component_type;
				item.component_tag = audioComponent->component_tag;

				item.stream_type = audioComponent->stream_type;
				item.simulcast_group_tag = audioComponent->simulcast_group_tag;
				item.ES_multi_lingual_flag = audioComponent->ES_multi_lingual_flag;
				item.main_component_flag = audioComponent->main_component_flag;
				item.quality_indicator = audioComponent->quality_indicator;
				item.sampling_rate = audioComponent->sampling_rate;


				CARIB8CharDecode arib;
				string text_char = "";
				if( audioComponent->text_charLength > 0 ){
					arib.PSISI((const BYTE*)audioComponent->text_char, audioComponent->text_charLength, &text_char);

					AtoW(text_char, item.text_char);
				}

				eventInfo->audioInfo->componentList.push_back(item);

			}
		}
	}

	return updateFlag;
}

BOOL CEpgDBUtil::AddAudioComponent_SD(CEITTable_SD* eit, EVENT_INFO* eventInfo, vector<DESCRIPTOR_DATA*>* descriptorList)
{
	BOOL updateFlag = FALSE;
	if( eventInfo->audioInfo == NULL ){
		eventInfo->audioInfo = new AUDIO_COMPONENT_INFO;
		updateFlag = TRUE;
	}else{
		updateFlag = CheckUpdate_SD(eit, eventInfo->audioInfo->tableID, eventInfo->audioInfo->version);
	}
	if( updateFlag == TRUE ){
		//更新必要
		eventInfo->audioInfo->tableID = eit->table_id;
		eventInfo->audioInfo->version = eit->version_number;
		eventInfo->audioInfo->componentList.clear();

		for( size_t i=0; i<descriptorList->size(); i++ ){
			if( (*descriptorList)[i]->audioComponent != NULL ){
				CAudioComponentDesc* audioComponent = (*descriptorList)[i]->audioComponent;
				AUDIO_COMPONENT_INFO_DATA item;

				item.stream_content = audioComponent->stream_content;
				item.component_type = audioComponent->component_type;
				item.component_tag = audioComponent->component_tag;

				item.stream_type = audioComponent->stream_type;
				item.simulcast_group_tag = audioComponent->simulcast_group_tag;
				item.ES_multi_lingual_flag = audioComponent->ES_multi_lingual_flag;
				item.main_component_flag = audioComponent->main_component_flag;
				item.quality_indicator = audioComponent->quality_indicator;
				item.sampling_rate = audioComponent->sampling_rate;


				CARIB8CharDecode arib;
				string text_char = "";
				if( audioComponent->text_charLength > 0 ){
					arib.PSISI((const BYTE*)audioComponent->text_char, audioComponent->text_charLength, &text_char);

					AtoW(text_char, item.text_char);
				}

				eventInfo->audioInfo->componentList.push_back(item);

			}
		}
	}

	return updateFlag;
}

BOOL CEpgDBUtil::AddEventGroup(CEITTable* eit, EVENT_INFO* eventInfo, CEventGroupDesc* eventGroup)
{
	BOOL updateFlag = FALSE;
	if( eventInfo->eventGroupInfo == NULL ){
		eventInfo->eventGroupInfo = new EVENTGROUP_INFO;
		updateFlag = TRUE;
	}else{
		updateFlag = CheckUpdate(eit, eventInfo->eventGroupInfo->tableID, eventInfo->eventGroupInfo->version);
	}
	if( updateFlag == TRUE ){
		//更新必要
		eventInfo->eventGroupInfo->tableID = eit->table_id;
		eventInfo->eventGroupInfo->version = eit->version_number;
		eventInfo->eventGroupInfo->eventData2List.clear();

		eventInfo->eventGroupInfo->group_type = eventGroup->group_type;
		eventInfo->eventGroupInfo->event_count = eventGroup->event_count;
		for( size_t i=0; i<eventGroup->eventDataList.size(); i++ ){
			CEventGroupDesc::EVENT_DATA2 item;
			item.event_id = eventGroup->eventDataList[i].event_id;
			item.service_id = eventGroup->eventDataList[i].service_id;
			item.original_network_id = eit->original_network_id;
			item.transport_stream_id = eit->transport_stream_id;

			eventInfo->eventGroupInfo->eventData2List.push_back(item);
		}
	}

	return updateFlag;
}

BOOL CEpgDBUtil::AddEventRelay(CEITTable* eit, EVENT_INFO* eventInfo, CEventGroupDesc* eventGroup)
{
	BOOL updateFlag = FALSE;
	if( eventInfo->eventRelayInfo == NULL ){
		eventInfo->eventRelayInfo = new EVENTGROUP_INFO;
		updateFlag = TRUE;
	}else{
		updateFlag = CheckUpdate(eit, eventInfo->eventRelayInfo->tableID, eventInfo->eventRelayInfo->version);
	}
	if( updateFlag == TRUE ){
		//更新必要
		eventInfo->eventRelayInfo->tableID = eit->table_id;
		eventInfo->eventRelayInfo->version = eit->version_number;
		eventInfo->eventRelayInfo->eventData2List.clear();

		eventInfo->eventRelayInfo->group_type = eventGroup->group_type;
		eventInfo->eventRelayInfo->event_count = eventGroup->event_count;
		if( eventGroup->group_type == 0x02 ){
			for( size_t i=0; i<eventGroup->eventDataList.size(); i++ ){
				CEventGroupDesc::EVENT_DATA2 item;
				item.event_id = eventGroup->eventDataList[i].event_id;
				item.service_id = eventGroup->eventDataList[i].service_id;
				item.original_network_id = eit->original_network_id;
				item.transport_stream_id = eit->transport_stream_id;

				eventInfo->eventRelayInfo->eventData2List.push_back(item);
			}
		}else{
			eventInfo->eventRelayInfo->eventData2List = eventGroup->eventData2List;
		}

	}

	return updateFlag;
}

BOOL CEpgDBUtil::CheckUpdate(CEITTable* eit, BYTE tableID, BYTE version)
{
	BOOL changeFlag = FALSE;
	if( eit->table_id == 0x4E ){
		if( tableID == 0x4E ){
			if( version != eit->version_number ){
				//バージョン変わったので更新
				changeFlag = TRUE;
			}else{
				//バージョン変わっていないので更新の必要なし
			}
		}else{
			//[p/f]が最新のはずなので更新
			changeFlag = TRUE;
		}
	}else if( eit->table_id == 0x4F ){
		if( tableID == 0x4F ){
			if( version != eit->version_number ){
				//バージョン変わったので更新
				changeFlag = TRUE;
			}else{
				//バージョン変わっていないので更新の必要なし
			}
		}else if( 0x60 <= tableID && tableID <= 0x6F ){
			//[p/f]の方が新しいはずなので更新
			changeFlag = TRUE;
		}else{
			//自ストリーム情報なので更新しない
		}
	}else if( 0x50 <= eit->table_id && eit->table_id <= 0x5F ){
		if( 0x50 <= tableID && tableID <= 0x5F ){
			if( tableID == eit->table_id ){
				if( version != eit->version_number ){
					//バージョン変わったので更新
					changeFlag = TRUE;
				}else{
					//バージョン変わっていないので更新の必要なし
				}
			}else{
				//テーブルが変わったので更新
				changeFlag = TRUE;
			}
		}else if( 0x60 <= tableID && tableID <= 0x6F ||
			tableID == 0x4F ){
				//他ストリーム情報なので更新しておく
				changeFlag = TRUE;
		}else{
			//[p/f]が最新のはずなので更新しない
		}
	}else if( 0x60 <= eit->table_id && eit->table_id <= 0x6F ){
		if( 0x60 <= tableID && tableID <= 0x6F ){
			if( tableID == eit->table_id ){
				if( version != eit->version_number ){
					//バージョン変わったので更新
					changeFlag = TRUE;
				}else{
					//バージョン変わっていないので更新の必要なし
				}
			}else{
				//テーブルが変わったので更新
				changeFlag = TRUE;
			}
		}else{
			//[p/f]か自ストリームなので更新の必要なし
		}
	}
	return changeFlag;
}

BOOL CEpgDBUtil::CheckUpdate_SD(CEITTable_SD* eit, BYTE tableID, BYTE version)
{
	BOOL changeFlag = FALSE;
	if( 0xA0 <= eit->table_id && eit->table_id <= 0xAF ){
		if( 0xA0 <= tableID && tableID <= 0xAF ){
			if( tableID == eit->table_id ){
				if( version != eit->version_number ){
					//バージョン変わったので更新
					changeFlag = TRUE;
				}else{
					//バージョン変わっていないので更新の必要なし
				}
			}else{
				//テーブルが変わったので更新
				changeFlag = TRUE;
			}
		}else{
			//[p/f]が最新のはずなので更新しない
		}
	}
	return changeFlag;
}

void CEpgDBUtil::ClearSectionStatus()
{
	if( Lock() == FALSE ) return ;

	map<ULONGLONG, SECTION_STATUS_INFO*>::iterator itr;
	for( itr = this->sectionMap.begin(); itr != this->sectionMap.end(); itr++ ){
		SAFE_DELETE(itr->second);
	}
	this->sectionMap.clear();
	this->sectionNowFlag = 0;

	UnLock();
}

BOOL CEpgDBUtil::CheckSectionAll(map<WORD, SECTION_FLAG_INFO>* sectionMap, BOOL leitFlag)
{
	if( sectionMap == NULL ){
		return FALSE;
	}
	if( sectionMap->size() == 0 ){
		return FALSE;
	}

	BOOL allChk = TRUE;
	map<WORD, SECTION_FLAG_INFO>::iterator itr;
	for( itr = sectionMap->begin(); itr != sectionMap->end(); itr++ ){
//		_OutputDebugString(L"0x%016X, 0x%016X\r\n",itr->second.maxFlag, (itr->second.sectionFlag | this->sectionNowFlag));
		if( leitFlag == FALSE ){
			if( itr->second.maxFlag != (itr->second.sectionFlag | this->sectionNowFlag) ){
				allChk = FALSE;
				break;
			}
		}else{
			if( itr->second.maxFlag != itr->second.sectionFlag ){
				allChk = FALSE;
				break;
			}
		}
	}

	return allChk;
}

EPG_SECTION_STATUS CEpgDBUtil::GetSectionStatus(BOOL l_eitFlag)
{
	if( Lock() == FALSE ) return EpgNoData;

	EPG_SECTION_STATUS status = EpgNoData;
	if( this->sectionMap.size() == 0 ){
		UnLock();
		return status;
	}

	BOOL basicFlag = TRUE;
	BOOL extFlag = TRUE;
	BOOL leitFlag = TRUE;

	map<ULONGLONG, SECTION_STATUS_INFO*>::iterator itr;
	for( itr = this->sectionMap.begin(); itr != this->sectionMap.end(); itr++ ){
		if( l_eitFlag == TRUE ){
			//L-EITの状況
			if( itr->second->HEITFlag == FALSE ){
				if( itr->second->last_section_numberBasic > 0 ){
					if( CheckSectionAll( &itr->second->sectionBasicMap, TRUE ) == FALSE ){
						leitFlag = FALSE;
						break;
					}
				}
			}
		}else{
			//H-EITの状況
			if( itr->second->HEITFlag == TRUE ){
				//サービスリストあるなら映像サービスのみ対象
				map<ULONGLONG, BYTE>::iterator itrType;
				itrType = this->serviceList.find(itr->first);
				if( itrType != this->serviceList.end() ){
					if( itrType->second != 0x01 && itrType->second != 0xA5 ){
						continue;
					}
				}
//				_OutputDebugString(L"0x%I64X, %x,%x, %x,%x, \r\n",itr->first, itr->second->last_section_numberBasic, itr->second->last_table_idBasic, itr->second->last_section_numberExt, itr->second->last_table_idExt);
				//Basic
				if( itr->second->last_section_numberBasic > 0 ){
					if( CheckSectionAll( &itr->second->sectionBasicMap ) == FALSE ){
						basicFlag = FALSE;
					}
				}
				//Ext
				if( itr->second->last_section_numberExt > 0 ){
					if( CheckSectionAll( &itr->second->sectionExtMap ) == FALSE ){
						extFlag = FALSE;
					}
				}
				if( basicFlag == FALSE && extFlag == FALSE ){
					break;
				}
			}
		}
	}

	if( l_eitFlag == TRUE ){
		if( leitFlag == TRUE ){
//			OutputDebugString(L"EpgLEITAll\r\n");
			status = EpgLEITAll;
		}else{
//			OutputDebugString(L"EpgNeedData\r\n");
			status = EpgNeedData;
		}
	}else{
		if( basicFlag == TRUE && extFlag == TRUE ){
//			OutputDebugString(L"EpgHEITAll\r\n");
			status = EpgHEITAll;
		}else if( basicFlag == TRUE ){
//			OutputDebugString(L"EpgBasicAll\r\n");
			status = EpgBasicAll;
		}else if( extFlag == TRUE ){
//			OutputDebugString(L"EpgExtendAll\r\n");
			status = EpgExtendAll;
		}else{
//			OutputDebugString(L"EpgNeedData\r\n");
			status = EpgNeedData;
		}
	}
	UnLock();
	return status;
}

BOOL CEpgDBUtil::AddServiceList(CNITTable* nit)
{
	if( nit == NULL ){
		return FALSE;
	}
	if( Lock() == FALSE ) return FALSE;

	wstring network_nameW = L"";

	for( size_t i=0; i<nit->descriptorList.size(); i++ ){
		if( nit->descriptorList[i]->networkName != NULL ){
			CNetworkNameDesc* networkName = nit->descriptorList[i]->networkName;
			if( networkName->char_nameLength > 0 ){
				CARIB8CharDecode arib;
				string network_name = "";
				arib.PSISI((const BYTE*)networkName->char_name, networkName->char_nameLength, &network_name);
				AtoW(network_name, network_nameW);
			}
		}
	}

	for( size_t i=0; i<nit->TSInfoList.size(); i++ ){
		CNITTable::TS_INFO_DATA* tsInfo = nit->TSInfoList[i];
		//サービス情報更新用
		DWORD key = ((DWORD)tsInfo->original_network_id) <<16 | tsInfo->transport_stream_id;
		map<DWORD, DB_TS_INFO*>::iterator itrFind;
		itrFind = this->serviceInfoList.find(key);
		if( itrFind != this->serviceInfoList.end() ){
			itrFind->second->network_name = network_nameW;
		}

		for( size_t j=0; j<tsInfo->descriptorList.size(); j++ ){
			DESCRIPTOR_DATA* desc = tsInfo->descriptorList[j];
			if( desc->serviceList != NULL ){
				for( size_t k=0; k<desc->serviceList->serviceList.size(); k++ ){
					CServiceListDesc::SERVICE_LIST_DATA serviceData = desc->serviceList->serviceList[k];
					ULONGLONG key = _Create64Key(tsInfo->original_network_id, tsInfo->transport_stream_id, serviceData.service_id);
					map<ULONGLONG, BYTE>::iterator itrService;
					itrService = this->serviceList.find(key);
					if( itrService == this->serviceList.end() ){
						this->serviceList.insert(pair<ULONGLONG, BYTE>(key, serviceData.service_type));
					}
				}
			}
			if( desc->TSInfo != NULL && itrFind != this->serviceInfoList.end()){
				//ts_nameとremote_control_key_id
				if( desc->TSInfo->length_of_ts_name > 0 ){
					CARIB8CharDecode arib;
					string ts_name = "";
					arib.PSISI((const BYTE*)desc->TSInfo->ts_name_char, desc->TSInfo->length_of_ts_name, &ts_name);
					AtoW(ts_name, itrFind->second->ts_name);
				}
				itrFind->second->remote_control_key_id = desc->TSInfo->remote_control_key_id;
			}
			if( desc->partialReception != NULL && itrFind != this->serviceInfoList.end()){
				//部分受信フラグ
				map<WORD,DB_SERVICE_INFO*>::iterator itrService;
				for( size_t k=0; k<desc->partialReception->service_idList.size(); k++ ){
					itrService = itrFind->second->serviceList.find(desc->partialReception->service_idList[k]);
					if( itrService != itrFind->second->serviceList.end() ){
						itrService->second->partialReceptionFlag = 1;
					}
				}
			}
		}
	}

	UnLock();
	return TRUE;
}

BOOL CEpgDBUtil::AddServiceList(WORD TSID, CSITTable* sit)
{
	if( Lock() == FALSE ) return FALSE;

	WORD ONID = 0xFFFF;
	for( size_t i=0; i<sit->descriptorList.size(); i++ ){
		if( sit->descriptorList[i]->networkIdentification != NULL ){
			ONID = sit->descriptorList[i]->networkIdentification->network_id;
		}
	}
	if(ONID == 0xFFFF){
		UnLock();
		return FALSE;
	}

	DWORD key = ((DWORD)ONID)<<16 | TSID;
	map<DWORD, DB_TS_INFO*>::iterator itrTS;
	itrTS = this->serviceInfoList.find(key);
	if( itrTS == this->serviceInfoList.end() ){
		DB_TS_INFO* info = new DB_TS_INFO;
		info->original_network_id = ONID;
		info->transport_stream_id = TSID;

		for(size_t i=0; i<sit->serviceLoopList.size(); i++ ){
			DB_SERVICE_INFO* item = new DB_SERVICE_INFO;
			item->original_network_id = ONID;
			item->transport_stream_id = TSID;
			item->service_id = sit->serviceLoopList[i]->service_id;

			for( size_t j=0; j<sit->serviceLoopList[i]->descriptorList.size(); j++ ){
				if( sit->serviceLoopList[i]->descriptorList[j]->service != NULL ){
					CServiceDesc* service = sit->serviceLoopList[i]->descriptorList[j]->service;
					CARIB8CharDecode arib;
					string service_provider_name = "";
					string service_name = "";
					if( service->service_provider_name_length > 0 ){
						arib.PSISI((const BYTE*)service->char_service_provider_name, service->service_provider_name_length, &service_provider_name);
					}
					if( service->service_name_length > 0 ){
						arib.PSISI((const BYTE*)service->char_service_name, service->service_name_length, &service_name);
					}
					AtoW(service_provider_name, item->service_provider_name);
					AtoW(service_name, item->service_name);

					item->service_type = service->service_type;
				}
			}
			info->serviceList.insert(pair<WORD,DB_SERVICE_INFO*>(item->service_id, item));
		}
		this->serviceInfoList.insert(pair<DWORD, DB_TS_INFO*>(key, info));
	}


	UnLock();
	return TRUE;
}

BOOL CEpgDBUtil::AddSDT(CSDTTable* sdt)
{
	if( Lock() == FALSE ) return FALSE;

	DWORD key = ((DWORD)sdt->original_network_id)<<16 | sdt->transport_stream_id;
	map<DWORD, DB_TS_INFO*>::iterator itrTS;
	itrTS = this->serviceInfoList.find(key);
	if( itrTS == this->serviceInfoList.end() ){
		DB_TS_INFO* info = new DB_TS_INFO;
		info->original_network_id = sdt->original_network_id;
		info->transport_stream_id = sdt->transport_stream_id;

		for(size_t i=0; i<sdt->serviceInfoList.size(); i++ ){
			DB_SERVICE_INFO* item = new DB_SERVICE_INFO;
			item->original_network_id = sdt->original_network_id;
			item->transport_stream_id = sdt->transport_stream_id;
			item->service_id = sdt->serviceInfoList[i]->service_id;

			for( size_t j=0; j<sdt->serviceInfoList[i]->descriptorList.size(); j++ ){
				if( sdt->serviceInfoList[i]->descriptorList[j]->service != NULL ){
					CServiceDesc* service = sdt->serviceInfoList[i]->descriptorList[j]->service;
					CARIB8CharDecode arib;
					string service_provider_name = "";
					string service_name = "";
					if( service->service_provider_name_length > 0 ){
						arib.PSISI((const BYTE*)service->char_service_provider_name, service->service_provider_name_length, &service_provider_name);
					}
					if( service->service_name_length > 0 ){
						arib.PSISI((const BYTE*)service->char_service_name, service->service_name_length, &service_name);
					}
					AtoW(service_provider_name, item->service_provider_name);
					AtoW(service_name, item->service_name);

					item->service_type = service->service_type;
				}
			}
			info->serviceList.insert(pair<WORD,DB_SERVICE_INFO*>(item->service_id, item));
		}
		this->serviceInfoList.insert(pair<DWORD, DB_TS_INFO*>(key, info));
	}else{
		for(size_t i=0; i<sdt->serviceInfoList.size(); i++ ){
			map<WORD,DB_SERVICE_INFO*>::iterator itrS;
			itrS = itrTS->second->serviceList.find(sdt->serviceInfoList[i]->service_id);
			if( itrS == itrTS->second->serviceList.end()){
				DB_SERVICE_INFO* item = new DB_SERVICE_INFO;
				item->original_network_id = sdt->original_network_id;
				item->transport_stream_id = sdt->transport_stream_id;
				item->service_id = sdt->serviceInfoList[i]->service_id;

				for( size_t j=0; j<sdt->serviceInfoList[i]->descriptorList.size(); j++ ){
					if( sdt->serviceInfoList[i]->descriptorList[j]->service != NULL ){
						CServiceDesc* service = sdt->serviceInfoList[i]->descriptorList[j]->service;
						CARIB8CharDecode arib;
						string service_provider_name = "";
						string service_name = "";
						if( service->service_provider_name_length > 0 ){
							arib.PSISI((const BYTE*)service->char_service_provider_name, service->service_provider_name_length, &service_provider_name);
						}
						if( service->service_name_length > 0 ){
							arib.PSISI((const BYTE*)service->char_service_name, service->service_name_length, &service_name);
						}
						AtoW(service_provider_name, item->service_provider_name);
						AtoW(service_name, item->service_name);

						item->service_type = service->service_type;
					}
				}
				itrTS->second->serviceList.insert(pair<WORD,DB_SERVICE_INFO*>(item->service_id, item));
			}
		}
	}

	UnLock();
	return TRUE;
}

//指定サービスの全EPG情報を取得する
//戻り値：
// エラーコード
// originalNetworkID		[IN]取得対象のoriginalNetworkID
// transportStreamID		[IN]取得対象のtransportStreamID
// serviceID				[IN]取得対象のServiceID
// epgInfoListSize			[OUT]epgInfoListの個数
// epgInfoList				[OUT]EPG情報のリスト（DLL内で自動的にdeleteする。次に取得を行うまで有効）
DWORD CEpgDBUtil::GetEpgInfoList(
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	DWORD* epgInfoListSize,
	EPG_EVENT_INFO** epgInfoList
	)
{
	if( epgInfoListSize == NULL || epgInfoList == NULL ){
		return ERR_INVALID_ARG;
	}
	if( Lock() == FALSE ) return ERR_FALSE;

	SAFE_DELETE_ARRAY(this->epgInfoList);
	this->epgInfoListSize = 0;

	ULONGLONG key = _Create64Key(originalNetworkID, transportStreamID, serviceID);

	map<ULONGLONG, SERVICE_EVENT_INFO*>::iterator itr;
	itr = serviceEventMap.find(key);
	if( itr == serviceEventMap.end() ){
		UnLock();
		return ERR_NOT_FIND;
	}

	this->epgInfoListSize = (DWORD)itr->second->eventMap.size();
	if( this->epgInfoListSize == 0 ){
		UnLock();
		return ERR_NOT_FIND;
	}
	this->epgInfoList = new EPG_EVENT_INFO[this->epgInfoListSize];

	map<WORD, EVENT_INFO*>::iterator itrEvt;
	DWORD count = 0;
	for( itrEvt = itr->second->eventMap.begin(); itrEvt != itr->second->eventMap.end(); itrEvt++ ){
		CopyEpgInfo(this->epgInfoList+count, itrEvt->second);
		count++;
	}

	*epgInfoListSize = this->epgInfoListSize;
	*epgInfoList = this->epgInfoList;

	UnLock();
	return NO_ERR;
}

void CEpgDBUtil::CopyEpgInfo(EPG_EVENT_INFO* destInfo, EVENT_INFO* srcInfo)
{
	destInfo->event_id = srcInfo->event_id;
	destInfo->StartTimeFlag = srcInfo->StartTimeFlag;
	destInfo->start_time = srcInfo->start_time;
	destInfo->DurationFlag = srcInfo->DurationFlag;
	destInfo->durationSec = srcInfo->durationSec;
	destInfo->freeCAFlag = srcInfo->freeCAFlag;

	if( srcInfo->shortInfo != NULL ){
		EPG_SHORT_EVENT_INFO* item = new EPG_SHORT_EVENT_INFO;
		destInfo->shortInfo = item;

		item->event_nameLength = (WORD)srcInfo->shortInfo->event_name.size();
		item->event_name = new WCHAR[item->event_nameLength+1];
		ZeroMemory(item->event_name, sizeof(WCHAR)*(item->event_nameLength+1));
		if( item->event_nameLength > 0 ){
			wcscpy_s(item->event_name, item->event_nameLength+1, srcInfo->shortInfo->event_name.c_str());
		}

		item->text_charLength = (WORD)srcInfo->shortInfo->text_char.size();
		item->text_char = new WCHAR[item->text_charLength+1];
		ZeroMemory(item->text_char, sizeof(WCHAR)*(item->text_charLength+1));
		if( item->text_charLength > 0 ){
			wcscpy_s(item->text_char, item->text_charLength+1, srcInfo->shortInfo->text_char.c_str());
		}
	}

	if( srcInfo->extInfo != NULL ){
		EPG_EXTENDED_EVENT_INFO* item = new EPG_EXTENDED_EVENT_INFO;
		destInfo->extInfo = item;

		item->text_charLength = (WORD)srcInfo->extInfo->text_char.size();
		item->text_char = new WCHAR[item->text_charLength+1];
		ZeroMemory(item->text_char, sizeof(WCHAR)*(item->text_charLength+1));
		if( item->text_charLength > 0 ){
			wcscpy_s(item->text_char, item->text_charLength+1, srcInfo->extInfo->text_char.c_str());
		}
	}

	if( srcInfo->contentInfo != NULL ){
		EPG_CONTEN_INFO* item = new EPG_CONTEN_INFO;
		destInfo->contentInfo = item;

		item->listSize = (WORD)srcInfo->contentInfo->nibbleList.size();
		if( item->listSize > 0 ){
			item->nibbleList = new EPG_CONTENT[item->listSize];
			for( WORD i=0; i<item->listSize; i++ ){
				item->nibbleList[i].content_nibble_level_1 = srcInfo->contentInfo->nibbleList[i].content_nibble_level_1;
				item->nibbleList[i].content_nibble_level_2 = srcInfo->contentInfo->nibbleList[i].content_nibble_level_2;
				item->nibbleList[i].user_nibble_1 = srcInfo->contentInfo->nibbleList[i].user_nibble_1;
				item->nibbleList[i].user_nibble_2 = srcInfo->contentInfo->nibbleList[i].user_nibble_2;
			}
		}
	}

	if( srcInfo->componentInfo != NULL ){
		EPG_COMPONENT_INFO* item = new EPG_COMPONENT_INFO;
		destInfo->componentInfo = item;

		item->stream_content = srcInfo->componentInfo->stream_content;
		item->component_type = srcInfo->componentInfo->component_type;
		item->component_tag = srcInfo->componentInfo->component_tag;

		item->text_charLength = (WORD)srcInfo->componentInfo->text_char.size();
		item->text_char = new WCHAR[item->text_charLength+1];
		ZeroMemory(item->text_char, sizeof(WCHAR)*(item->text_charLength+1));
		if( item->text_charLength > 0 ){
			wcscpy_s(item->text_char, item->text_charLength+1, srcInfo->componentInfo->text_char.c_str());
		}
	}

	if( srcInfo->audioInfo != NULL ){
		EPG_AUDIO_COMPONENT_INFO* item = new EPG_AUDIO_COMPONENT_INFO;
		destInfo->audioInfo = item;
		item->listSize = (WORD)srcInfo->audioInfo->componentList.size();
		if( item->listSize > 0 ){
			item->audioList = new EPG_AUDIO_COMPONENT_INFO_DATA[item->listSize];
			for( WORD i=0; i<item->listSize; i++ ){
				item->audioList[i].stream_content = srcInfo->audioInfo->componentList[i].stream_content;
				item->audioList[i].component_type = srcInfo->audioInfo->componentList[i].component_type;
				item->audioList[i].component_tag = srcInfo->audioInfo->componentList[i].component_tag;
				item->audioList[i].stream_type = srcInfo->audioInfo->componentList[i].stream_type;
				item->audioList[i].simulcast_group_tag = srcInfo->audioInfo->componentList[i].simulcast_group_tag;
				item->audioList[i].ES_multi_lingual_flag = srcInfo->audioInfo->componentList[i].ES_multi_lingual_flag;
				item->audioList[i].main_component_flag = srcInfo->audioInfo->componentList[i].main_component_flag;
				item->audioList[i].quality_indicator = srcInfo->audioInfo->componentList[i].quality_indicator;
				item->audioList[i].sampling_rate = srcInfo->audioInfo->componentList[i].sampling_rate;

				item->audioList[i].text_charLength = (WORD)srcInfo->audioInfo->componentList[i].text_char.size();
				item->audioList[i].text_char = new WCHAR[item->audioList[i].text_charLength+1];
				ZeroMemory(item->audioList[i].text_char, sizeof(WCHAR)*(item->audioList[i].text_charLength+1));
				if( item->audioList[i].text_charLength > 0 ){
					wcscpy_s(item->audioList[i].text_char, item->audioList[i].text_charLength+1, srcInfo->audioInfo->componentList[i].text_char.c_str());
				}
			}
		}
	}

	if( srcInfo->eventGroupInfo != NULL ){
		EPG_EVENTGROUP_INFO* item = new EPG_EVENTGROUP_INFO;
		destInfo->eventGroupInfo = item;

		item->group_type = srcInfo->eventGroupInfo->group_type;
		item->event_count = srcInfo->eventGroupInfo->event_count;

		if( item->event_count > 0 ){
			item->eventDataList = new EPG_EVENT_DATA[item->event_count];
			for( BYTE i=0; i<item->event_count; i++ ){
				item->eventDataList[i].original_network_id = srcInfo->eventGroupInfo->eventData2List[i].original_network_id;
				item->eventDataList[i].transport_stream_id = srcInfo->eventGroupInfo->eventData2List[i].transport_stream_id;
				item->eventDataList[i].service_id = srcInfo->eventGroupInfo->eventData2List[i].service_id;
				item->eventDataList[i].event_id = srcInfo->eventGroupInfo->eventData2List[i].event_id;
			}
		}
	}

	if( srcInfo->eventRelayInfo != NULL ){
		EPG_EVENTGROUP_INFO* item = new EPG_EVENTGROUP_INFO;
		destInfo->eventRelayInfo = item;

		item->group_type = srcInfo->eventRelayInfo->group_type;
		//他チャンネルのときevent_countは０になっている
		//item->event_count = srcInfo->eventGroupInfo->event_count;
		item->event_count = (BYTE)srcInfo->eventRelayInfo->eventData2List.size();

		if( item->event_count > 0 ){
			item->eventDataList = new EPG_EVENT_DATA[item->event_count];
			for( BYTE i=0; i<item->event_count; i++ ){
				item->eventDataList[i].original_network_id = srcInfo->eventRelayInfo->eventData2List[i].original_network_id;
				item->eventDataList[i].transport_stream_id = srcInfo->eventRelayInfo->eventData2List[i].transport_stream_id;
				item->eventDataList[i].service_id = srcInfo->eventRelayInfo->eventData2List[i].service_id;
				item->eventDataList[i].event_id = srcInfo->eventRelayInfo->eventData2List[i].event_id;
			}
		}
	}

}

//蓄積されたEPG情報のあるサービス一覧を取得する
//SERVICE_EXT_INFOの情報はない場合がある
//戻り値：
// エラーコード
//引数：
// serviceListSize			[OUT]serviceListの個数
// serviceList				[OUT]サービス情報のリスト（DLL内で自動的にdeleteする。次に取得を行うまで有効）
DWORD CEpgDBUtil::GetServiceListEpgDB(
	DWORD* serviceListSize,
	SERVICE_INFO** serviceList
	)
{
	SAFE_DELETE_ARRAY(this->serviceDBList);
	this->serviceDBListSize = 0;

	if( serviceListSize == NULL || serviceList == NULL ){
		return ERR_INVALID_ARG;
	}

	this->serviceDBListSize = (DWORD)this->serviceEventMap.size();
	this->serviceDBList = new SERVICE_INFO[this->serviceDBListSize];

	DWORD count = 0;
	map<ULONGLONG, SERVICE_EVENT_INFO*>::iterator itr;
	for(itr = this->serviceEventMap.begin(); itr != this->serviceEventMap.end(); itr++ ){
		this->serviceDBList[count].original_network_id = (WORD)(itr->first>>32);
		this->serviceDBList[count].transport_stream_id = (WORD)((itr->first&0xFFFF0000)>>16);
		this->serviceDBList[count].service_id = (WORD)(itr->first&0xFFFF);

		DWORD infoKey = ((DWORD)this->serviceDBList[count].original_network_id) << 16 | this->serviceDBList[count].transport_stream_id;
		map<DWORD, DB_TS_INFO*>::iterator itrInfo;
		itrInfo = this->serviceInfoList.find(infoKey);
		if( itrInfo != this->serviceInfoList.end() ){
			map<WORD,DB_SERVICE_INFO*>::iterator itrService;
			itrService = itrInfo->second->serviceList.find(this->serviceDBList[count].service_id);
			if( itrService != itrInfo->second->serviceList.end() ){
				DB_TS_INFO* info = itrInfo->second;
				DB_SERVICE_INFO* item = itrService->second;
				this->serviceDBList[count].extInfo = new SERVICE_EXT_INFO;

				this->serviceDBList[count].extInfo->service_type = item->service_type;
				this->serviceDBList[count].extInfo->partialReceptionFlag = item->partialReceptionFlag;
				this->serviceDBList[count].extInfo->remote_control_key_id = info->remote_control_key_id;

				if( item->service_provider_name.size() > 0 ){
					this->serviceDBList[count].extInfo->service_provider_name = new WCHAR[item->service_provider_name.size()+1];
					wcscpy_s(this->serviceDBList[count].extInfo->service_provider_name, item->service_provider_name.size()+1, item->service_provider_name.c_str());
				}
				if( item->service_name.size() > 0 ){
					this->serviceDBList[count].extInfo->service_name = new WCHAR[item->service_name.size()+1];
					wcscpy_s(this->serviceDBList[count].extInfo->service_name, item->service_name.size()+1, item->service_name.c_str());
				}
				if( info->network_name.size() > 0 ){
					this->serviceDBList[count].extInfo->network_name = new WCHAR[info->network_name.size()+1];
					wcscpy_s(this->serviceDBList[count].extInfo->network_name, info->network_name.size()+1, info->network_name.c_str());
				}
				if( info->ts_name.size() > 0 ){
					this->serviceDBList[count].extInfo->ts_name = new WCHAR[info->ts_name.size()+1];
					wcscpy_s(this->serviceDBList[count].extInfo->ts_name, info->ts_name.size()+1, info->ts_name.c_str());
				}
			}
		}

		count++;
	}

	*serviceListSize = this->serviceDBListSize;
	*serviceList = this->serviceDBList;

	return NO_ERR;
}

//指定サービスの現在or次のEPG情報を取得する
//戻り値：
// エラーコード
//引数：
// originalNetworkID		[IN]取得対象のoriginalNetworkID
// transportStreamID		[IN]取得対象のtransportStreamID
// serviceID				[IN]取得対象のServiceID
// nextFlag					[IN]TRUE（次の番組）、FALSE（現在の番組）
// nowTime					[IN]現在の時間
// epgInfo					[OUT]EPG情報（DLL内で自動的にdeleteする。次に取得を行うまで有効）
DWORD CEpgDBUtil::GetEpgInfo(
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	BOOL nextFlag,
	SYSTEMTIME nowTime,
	EPG_EVENT_INFO** epgInfo
	)
{
	if( epgInfo == NULL ){
		return ERR_INVALID_ARG;
	}
	if( Lock() == FALSE ) return ERR_FALSE;

	SAFE_DELETE(this->epgInfo);

	ULONGLONG key = _Create64Key(originalNetworkID, transportStreamID, serviceID);

	map<ULONGLONG, SERVICE_EVENT_INFO*>::iterator itr;
	itr = serviceEventMap.find(key);
	if( itr == serviceEventMap.end() ){
		UnLock();
		return ERR_NOT_FIND;
	}

	__int64 nowTime64 = ConvertI64Time(nowTime);

	//まずp/fの確認
	if( itr->second->nowEvent != NULL && nextFlag == FALSE ){
		if( itr->second->nowEvent->StartTimeFlag == TRUE && itr->second->nowEvent->DurationFlag == TRUE){
			__int64 startTime = ConvertI64Time(itr->second->nowEvent->start_time);
			__int64 endTime = GetSumTime(itr->second->nowEvent->start_time, itr->second->nowEvent->durationSec);
			if( startTime <= nowTime64 && nowTime64 < endTime ){
				//時間内にあるので正しいはず
				this->epgInfo = new EPG_EVENT_INFO;
				CopyEpgInfo(this->epgInfo, itr->second->nowEvent);
				goto Err_End;
			}
		}else{
			//どっちか未定なので信じる
			this->epgInfo = new EPG_EVENT_INFO;
			CopyEpgInfo(this->epgInfo, itr->second->nowEvent);
			goto Err_End;
		}
	}else if( itr->second->nextEvent != NULL && nextFlag == TRUE ){
		if( itr->second->nextEvent->StartTimeFlag == TRUE && itr->second->nextEvent->DurationFlag == TRUE){
			__int64 startTime = ConvertI64Time(itr->second->nextEvent->start_time);
			//__int64 endTime = GetSumTime(itr->second->nextEvent->start_time, itr->second->nextEvent->durationSec);
			if( nowTime64 <= startTime ){
				//開始時間先にあるので正しいはず
				this->epgInfo = new EPG_EVENT_INFO;
				CopyEpgInfo(this->epgInfo, itr->second->nextEvent);
				goto Err_End;
			}
		}else{
			//どっちか未定なので信じる
			this->epgInfo = new EPG_EVENT_INFO;
			CopyEpgInfo(this->epgInfo, itr->second->nextEvent);
			goto Err_End;
		}
	}
	/*
	//p/fで確認できなかったのでDBの時間的にあうもの探す
	if( nextFlag == FALSE ){
		//現在
		map<WORD, EVENT_INFO*>::iterator itrEvt;
		for( itrEvt = itr->second->eventMap.begin(); itrEvt != itr->second->eventMap.end(); itrEvt++ ){
			if( itrEvt->second->StartTimeFlag == TRUE && itrEvt->second->DurationFlag == TRUE ){
				__int64 startTime = ConvertI64Time(itrEvt->second->start_time);
				__int64 endTime = GetSumTime(itrEvt->second->start_time, itrEvt->second->durationSec);
				if( startTime <= nowTime64 && nowTime64 < endTime ){
					//時間内にあるので正しいはず
					this->epgInfo = new EPG_EVENT_INFO;
					CopyEpgInfo(this->epgInfo, itrEvt->second);
					goto Err_End;
				}
			}
		}
	}else{
		//p/fにないので時間的に次になる番組探す

		map<__int64, EVENT_INFO*> timeSort;
		map<WORD, EVENT_INFO*>::iterator itrEvt;
		for( itrEvt = itr->second->eventMap.begin(); itrEvt != itr->second->eventMap.end(); itrEvt++ ){
			if( itrEvt->second->StartTimeFlag == TRUE ){
				__int64 startTime = ConvertI64Time(itrEvt->second->start_time);
				//次の見つける必要あるので時間でソート
				timeSort.insert(pair<__int64, EVENT_INFO*>(startTime, itrEvt->second));
			}
		}
		map<__int64, EVENT_INFO*>::iterator itrSort;
		for( itrSort = timeSort.begin(); itrSort != timeSort.end(); itrSort++ ){
			if( nowTime64 < itrSort->first ){
				//現在より開始時間早いので次になるはず
				this->epgInfo = new EPG_EVENT_INFO;
				CopyEpgInfo(this->epgInfo, itrSort->second);
				goto Err_End;
			}
		}
	}
	*/
Err_End:
	if( this->epgInfo == NULL ){
		UnLock();
		return ERR_NOT_FIND;
	}
	*epgInfo = this->epgInfo;

	UnLock();
	return NO_ERR;
}

//指定イベントのEPG情報を取得する
//戻り値：
// エラーコード
//引数：
// originalNetworkID		[IN]取得対象のoriginalNetworkID
// transportStreamID		[IN]取得対象のtransportStreamID
// serviceID				[IN]取得対象のServiceID
// EventID					[IN]取得対象のEventID
// pfOnlyFlag				[IN]p/fからのみ検索するかどうか
// epgInfo					[OUT]EPG情報（DLL内で自動的にdeleteする。次に取得を行うまで有効）
DWORD CEpgDBUtil::SearchEpgInfo(
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	WORD eventID,
	BYTE pfOnlyFlag,
	EPG_EVENT_INFO** epgInfo
	)
{
	if( epgInfo == NULL ){
		return ERR_INVALID_ARG;
	}
	if( Lock() == FALSE ) return ERR_FALSE;

	SAFE_DELETE(this->searchEpgInfo);

	ULONGLONG key = _Create64Key(originalNetworkID, transportStreamID, serviceID);

	map<ULONGLONG, SERVICE_EVENT_INFO*>::iterator itr;
	itr = serviceEventMap.find(key);
	if( itr == serviceEventMap.end() ){
		UnLock();
		return ERR_NOT_FIND;
	}

	if( pfOnlyFlag == 0 ){
		map<WORD, EVENT_INFO*>::iterator itrEvent;
		itrEvent = itr->second->eventMap.find(eventID);
		if( itrEvent != itr->second->eventMap.end() ){
			this->searchEpgInfo = new EPG_EVENT_INFO;
			CopyEpgInfo(this->searchEpgInfo, itrEvent->second);
			goto Err_End;
		}
	}else{
		if( itr->second->nowEvent != NULL ){
			if( itr->second->nowEvent->event_id == eventID ){
				this->searchEpgInfo = new EPG_EVENT_INFO;
				CopyEpgInfo(this->searchEpgInfo, itr->second->nowEvent);
				goto Err_End;
			}
		}
		if( itr->second->nextEvent != NULL ){
			if( itr->second->nextEvent->event_id == eventID ){
				this->searchEpgInfo = new EPG_EVENT_INFO;
				CopyEpgInfo(this->searchEpgInfo, itr->second->nextEvent);
				goto Err_End;
			}
		}
	}

Err_End:
	if( this->searchEpgInfo == NULL ){
		UnLock();
		return ERR_NOT_FIND;
	}
	*epgInfo = this->searchEpgInfo;

	UnLock();
	return NO_ERR;
}

BOOL CEpgDBUtil::AddSDEventMap(CEITTable_SD* eit)
{
	if( eit == NULL ){
		return FALSE;
	}

	ULONGLONG key = _Create64Key(eit->original_network_id, 0, eit->service_id);

	//サービスのmapを取得
	map<ULONGLONG, SERVICE_EVENT_INFO*>::iterator itr;
	SERVICE_EVENT_INFO* serviceInfo = NULL;

	itr = serviceEventMapSD.find(key);
	if( itr == serviceEventMapSD.end() ){
		serviceInfo = new SERVICE_EVENT_INFO;
		serviceEventMapSD.insert(pair<ULONGLONG, SERVICE_EVENT_INFO*>(key, serviceInfo));
	}else{
		serviceInfo = itr->second;
	}

	//イベントごとに更新必要が判定
	for( size_t i=0; i<eit->eventInfoList.size(); i++ ){
		CEITTable_SD::EVENT_INFO_DATA* eitEventInfo = eit->eventInfoList[i];
		map<WORD, EVENT_INFO*>::iterator itrEvent;
		EVENT_INFO* eventInfo = NULL;

		if( eitEventInfo->running_status != 0 && eitEventInfo->running_status != 2 && eitEventInfo->running_status != 4 ){
			//非実行中または停止中
			_OutputDebugString(L"★非実行中または停止中イベント ONID:0x%04x TSID:0x%04x SID:0x%04x EventID:0x%04x %04d/%02d/%02d %02d:%02d\r\n",
				eit->original_network_id,  eit->transport_stream_id, eit->service_id, eitEventInfo->event_id,
				eitEventInfo->start_time.wYear, eitEventInfo->start_time.wMonth, eitEventInfo->start_time.wDay, eitEventInfo->start_time.wHour, eitEventInfo->start_time.wMinute
				);
			continue;
		}

		itrEvent = serviceInfo->eventMap.find(eitEventInfo->event_id);
		if( itrEvent == serviceInfo->eventMap.end() ){
			eventInfo = new EVENT_INFO;
			eventInfo->ONID = eit->original_network_id;
			eventInfo->TSID = eit->transport_stream_id;
			eventInfo->SID = eit->service_id;
			eventInfo->event_id = eitEventInfo->event_id;
			eventInfo->StartTimeFlag = eitEventInfo->StartTimeFlag;
			eventInfo->start_time = eitEventInfo->start_time;
			eventInfo->DurationFlag = eitEventInfo->DurationFlag;
			eventInfo->durationSec = eitEventInfo->durationHH*60*60 +
				eitEventInfo->durationMM*60 +
				eitEventInfo->durationSS;
			eventInfo->freeCAFlag = eitEventInfo->free_CA_mode;
			serviceInfo->eventMap.insert(pair<WORD, EVENT_INFO*>(eventInfo->event_id, eventInfo));
		}else{
			eventInfo = itrEvent->second;

			eventInfo->StartTimeFlag = eitEventInfo->StartTimeFlag;
			eventInfo->start_time = eitEventInfo->start_time;
			eventInfo->DurationFlag = eitEventInfo->DurationFlag;
			eventInfo->durationSec = eitEventInfo->durationHH*60*60 +
				eitEventInfo->durationMM*60 +
				eitEventInfo->durationSS;
		}

		//ExtendedEventは複数あるので1度だけチェックする
		BOOL checkExtFlag = FALSE;
		BOOL checkAudioFlag = FALSE;
		for( size_t j=0; j<eitEventInfo->descriptorList.size(); j++ ){
			if( eitEventInfo->descriptorList[j]->shortEvent != NULL ){
				AddShortEvent_SD( eit, eventInfo, eitEventInfo->descriptorList[j]->shortEvent );
			}else if( eitEventInfo->descriptorList[j]->extendedEvent != NULL && checkExtFlag == FALSE){
				AddExtEvent_SD(eit, eventInfo, &eitEventInfo->descriptorList );
				checkExtFlag = TRUE;
			}else if( eitEventInfo->descriptorList[j]->content != NULL ){
				AddContent_SD( eit, eventInfo, eitEventInfo->descriptorList[j]->content );
			}else if( eitEventInfo->descriptorList[j]->component != NULL ){
				AddComponent_SD( eit, eventInfo, eitEventInfo->descriptorList[j]->component );
			}else if( eitEventInfo->descriptorList[j]->audioComponent != NULL && checkAudioFlag == FALSE ){
				AddAudioComponent_SD( eit, eventInfo, &eitEventInfo->descriptorList );
				checkAudioFlag = TRUE;
			}
		}
	}

	return TRUE;
}

BOOL CEpgDBUtil::AddEIT_SD2(WORD PID, CEITTable_SD2* eit)
{
	if( eit == NULL ){
		return FALSE;
	}
	if( Lock() == FALSE ) return FALSE;

	ULONGLONG key = _Create64Key(eit->original_network_id, 0, eit->service_id2);

	//サービスのmapを取得
	map<ULONGLONG, SERVICE_EVENT_INFO*>::iterator itr;
	SERVICE_EVENT_INFO* serviceInfo = NULL;

	itr = serviceEventMapSD.find(key);
	if( itr != serviceEventMapSD.end() ){
		serviceInfo = itr->second;
		for( size_t i=0; i<eit->eventMapList.size(); i++ ){
			for( size_t j=0; j<eit->eventMapList[i]->eventList.size(); j++ ){
				map<WORD, EVENT_INFO*>::iterator itrEvent;
				itrEvent = serviceInfo->eventMap.find(eit->eventMapList[i]->eventList[j].a4table_eventID);
				if( itrEvent != serviceInfo->eventMap.end() ){
					ULONGLONG key2 = _Create64Key(itrEvent->second->ONID, itrEvent->second->TSID, eit->service_id);
					map<ULONGLONG, SERVICE_EVENT_INFO*>::iterator itrMainDB;
					SERVICE_EVENT_INFO* mainServiceInfo = NULL;
					itrMainDB = serviceEventMap.find(key2);
					if( itrMainDB == serviceEventMap.end() ){
						mainServiceInfo = new SERVICE_EVENT_INFO;
						serviceEventMap.insert(pair<ULONGLONG, SERVICE_EVENT_INFO*>(key2, mainServiceInfo));
					}else{
						mainServiceInfo = itrMainDB->second;
					}
					EVENT_INFO* eventInfo = NULL;
					map<WORD, EVENT_INFO*>::iterator itrMainEvent;
					itrMainEvent = mainServiceInfo->eventMap.find(eit->eventMapList[i]->eventList[j].event_id);
					if( itrMainEvent == mainServiceInfo->eventMap.end()){
						eventInfo = new EVENT_INFO;
						mainServiceInfo->eventMap.insert(pair<WORD, EVENT_INFO*>(eit->eventMapList[i]->eventList[j].event_id, eventInfo));
					}else{
						eventInfo = itrMainEvent->second;
					}

					eventInfo->ONID = itrEvent->second->ONID;
					eventInfo->TSID = itrEvent->second->TSID;
					eventInfo->SID = eit->service_id;
					eventInfo->event_id = eit->eventMapList[i]->eventList[j].event_id;
					eventInfo->StartTimeFlag = itrEvent->second->StartTimeFlag;
					eventInfo->start_time = eit->eventMapList[i]->start_day;
					eventInfo->start_time.wHour = eit->eventMapList[i]->eventList[j].hour;
					eventInfo->start_time.wMinute = eit->eventMapList[i]->eventList[j].minute;
					eventInfo->DurationFlag = itrEvent->second->DurationFlag;
					if( eit->eventMapList[i]->eventList[j].duration == 0 ){
						eventInfo->durationSec = itrEvent->second->durationSec;
					}else{
						eventInfo->durationSec = eit->eventMapList[i]->eventList[j].duration;
					}
					eventInfo->freeCAFlag = itrEvent->second->freeCAFlag;

					if(itrEvent->second->shortInfo != NULL && eventInfo->shortInfo == NULL){
						eventInfo->shortInfo = new SHORT_EVENT_INFO;
						*eventInfo->shortInfo = *itrEvent->second->shortInfo;
					}
					if(itrEvent->second->extInfo != NULL && eventInfo->extInfo == NULL ){
						eventInfo->extInfo = new EXTENDED_EVENT_INFO;
						*eventInfo->extInfo = *itrEvent->second->extInfo;
					}
					if(itrEvent->second->contentInfo != NULL && eventInfo->contentInfo == NULL ){
						eventInfo->contentInfo = new CONTEN_INFO;
						*eventInfo->contentInfo = *itrEvent->second->contentInfo;
					}
					if(itrEvent->second->componentInfo != NULL && eventInfo->componentInfo == NULL ){
						eventInfo->componentInfo = new COMPONENT_INFO;
						*eventInfo->componentInfo = *itrEvent->second->componentInfo;
					}
					if(itrEvent->second->audioInfo != NULL && eventInfo->audioInfo == NULL ){
						eventInfo->audioInfo = new AUDIO_COMPONENT_INFO;
						*eventInfo->audioInfo = *itrEvent->second->audioInfo;
					}

				}
			}
		}
	}

	UnLock();
	return NO_ERR;
}
