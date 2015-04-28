#include "StdAfx.h"
#include "EpgDBUtil.h"

#include "../../Common/EpgTimerUtil.h"
#include "../../Common/StringUtil.h"
#include "../../Common/TimeUtil.h"
#include "../../Common/BlockLock.h"
#include "ARIB8CharDecode.h"

CEpgDBUtil::CEpgDBUtil(void)
{
	InitializeCriticalSection(&this->dbLock);

	this->sectionNowFlag = 0;

	this->epgInfoList = NULL;

	this->epgInfo = NULL;
	this->searchEpgInfo = NULL;

	this->serviceDBList = NULL;
}

CEpgDBUtil::~CEpgDBUtil(void)
{
	Clear();
	ClearSectionStatus();

	SAFE_DELETE_ARRAY(this->epgInfoList);

	SAFE_DELETE(this->epgInfo);

	SAFE_DELETE(this->searchEpgInfo);

	DeleteCriticalSection(&this->dbLock);

	SAFE_DELETE_ARRAY(this->serviceDBList);
}

void CEpgDBUtil::Clear()
{
	map<ULONGLONG, SERVICE_EVENT_INFO>::iterator itr;
	for( itr = this->serviceEventMap.begin(); itr != this->serviceEventMap.end(); itr++ ){
		for( map<WORD, EVENT_INFO*>::iterator jtr = itr->second.eventMap.begin(); jtr != itr->second.eventMap.end(); jtr++ ){
			SAFE_DELETE(jtr->second);
		}
	}
	this->serviceEventMap.clear();

	for( itr = this->serviceEventMapSD.begin(); itr != this->serviceEventMapSD.end(); itr++ ){
		for( map<WORD, EVENT_INFO*>::iterator jtr = itr->second.eventMap.begin(); jtr != itr->second.eventMap.end(); jtr++ ){
			SAFE_DELETE(jtr->second);
		}
	}
	this->serviceEventMapSD.clear();
}

void CEpgDBUtil::SetStreamChangeEvent()
{
	CBlockLock lock(&this->dbLock);
	//ストリーム変わったのでp/fをリセット
	map<ULONGLONG, SERVICE_EVENT_INFO>::iterator itr;
	for( itr = this->serviceEventMap.begin(); itr != this->serviceEventMap.end(); itr++ ){
		itr->second.nowEvent = NULL;
		itr->second.nextEvent = NULL;
	}
}

BOOL CEpgDBUtil::AddEIT(WORD PID, CEITTable* eit)
{
	if( eit == NULL ){
		return FALSE;
	}
	CBlockLock lock(&this->dbLock);

	ULONGLONG key = _Create64Key(eit->original_network_id, eit->transport_stream_id, eit->service_id);

	//サービスのmapを取得
	map<ULONGLONG, SERVICE_EVENT_INFO>::iterator itr;
	SERVICE_EVENT_INFO* serviceInfo = NULL;

	itr = serviceEventMap.find(key);
	if( itr == serviceEventMap.end() ){
		serviceInfo = &serviceEventMap.insert(std::make_pair(key, SERVICE_EVENT_INFO())).first->second;
	}else{
		serviceInfo = &itr->second;
	}

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
			DWORD tag = eitEventInfo->descriptorList[j]->GetNumber(AribDescriptor::descriptor_tag);
			if( tag == AribDescriptor::short_event_descriptor ){
				AddShortEvent( eit->table_id, eit->version_number, eventInfo, eitEventInfo->descriptorList[j], FALSE );
			}else if( tag == AribDescriptor::extended_event_descriptor && checkExtFlag == FALSE){
				AddExtEvent(eit->table_id, eit->version_number, eventInfo, &eitEventInfo->descriptorList, FALSE );
				checkExtFlag = TRUE;
			}else if( tag == AribDescriptor::content_descriptor ){
				AddContent( eit->table_id, eit->version_number, eventInfo, eitEventInfo->descriptorList[j], FALSE );
			}else if( tag == AribDescriptor::component_descriptor ){
				AddComponent( eit->table_id, eit->version_number, eventInfo, eitEventInfo->descriptorList[j], FALSE );
			}else if( tag == AribDescriptor::audio_component_descriptor && checkAudioFlag == FALSE ){
				AddAudioComponent( eit->table_id, eit->version_number, eventInfo, &eitEventInfo->descriptorList, FALSE );
				checkAudioFlag = TRUE;
			}else if( tag == AribDescriptor::event_group_descriptor ){
				if( eitEventInfo->descriptorList[j]->GetNumber(AribDescriptor::group_type) == 0x01 ){
					AddEventGroup( eit, eventInfo, eitEventInfo->descriptorList[j] );
				}else if( eitEventInfo->descriptorList[j]->GetNumber(AribDescriptor::group_type) == 0x02 ||
					eitEventInfo->descriptorList[j]->GetNumber(AribDescriptor::group_type) == 0x04){
					AddEventRelay( eit, eventInfo, eitEventInfo->descriptorList[j] );
				}
			}
		}
	}

	if( eit->original_network_id == 0x0003 ){
		return TRUE;
	}
	
	//セクションステータス
	map<ULONGLONG, SECTION_STATUS_INFO>::iterator itrSec;
	SECTION_STATUS_INFO* sectionInfo = NULL;
	itrSec = this->sectionMap.find(key);
	if( itrSec == this->sectionMap.end() ){
		sectionInfo = &this->sectionMap.insert(std::make_pair(key, SECTION_STATUS_INFO())).first->second;
	}else{
		sectionInfo = &itrSec->second;
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

	return TRUE;
}

BOOL CEpgDBUtil::AddEIT_SD(WORD PID, CEITTable_SD* eit)
{
	if( eit == NULL ){
		return FALSE;
	}
	CBlockLock lock(&this->dbLock);

	if( eit->original_network_id == 0x0003 && eit->table_id != 0x4E && eit->table_id != 0x4F){
		BOOL ret = AddSDEventMap(eit);
		return ret;
	}

	ULONGLONG key = _Create64Key(eit->original_network_id, eit->transport_stream_id, eit->service_id);

	//サービスのmapを取得
	map<ULONGLONG, SERVICE_EVENT_INFO>::iterator itr;
	SERVICE_EVENT_INFO* serviceInfo = NULL;

	itr = serviceEventMap.find(key);
	if( itr == serviceEventMap.end() ){
		serviceInfo = &serviceEventMap.insert(std::make_pair(key, SERVICE_EVENT_INFO())).first->second;
	}else{
		serviceInfo = &itr->second;
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
			DWORD tag = eitEventInfo->descriptorList[j]->GetNumber(AribDescriptor::descriptor_tag);
			if( tag == AribDescriptor::short_event_descriptor ){
				AddShortEvent( eit->table_id, eit->version_number, eventInfo, eitEventInfo->descriptorList[j], TRUE );
			}else if( tag == AribDescriptor::extended_event_descriptor && checkExtFlag == FALSE){
				AddExtEvent(eit->table_id, eit->version_number, eventInfo, &eitEventInfo->descriptorList, TRUE );
				checkExtFlag = TRUE;
			}else if( tag == AribDescriptor::content_descriptor ){
				AddContent( eit->table_id, eit->version_number, eventInfo, eitEventInfo->descriptorList[j], TRUE );
			}else if( tag == AribDescriptor::component_descriptor ){
				AddComponent( eit->table_id, eit->version_number, eventInfo, eitEventInfo->descriptorList[j], TRUE );
			}else if( tag == AribDescriptor::audio_component_descriptor && checkAudioFlag == FALSE ){
				AddAudioComponent( eit->table_id, eit->version_number, eventInfo, &eitEventInfo->descriptorList, TRUE );
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

	return TRUE;
}

static WORD UpdateInfoText(LPWSTR& strOut, LPCSTR strIn)
{
	delete[] strOut;
	int len = MultiByteToWideChar(932, 0, strIn, -1, NULL, 0);
	if( 1 < len && len <= MAXWORD + 1 ){
		strOut = new WCHAR[len];
		if( MultiByteToWideChar(932, 0, strIn, -1, strOut, len) != 0 ){
			return (WORD)(len - 1);
		}
		delete[] strOut;
	}
	//仕様が明確でなく利用側でNULLチェックが省略されているため
	strOut = new WCHAR[1];
	strOut[0] = L'\0';
	return 0;
}

BOOL CEpgDBUtil::AddShortEvent(BYTE table_id, BYTE version_number, EVENT_INFO* eventInfo, AribDescriptor::CDescriptor* shortEvent, BOOL skySDFlag)
{
	BOOL updateFlag = FALSE;
	if( eventInfo->shortInfo == NULL ){
		eventInfo->shortInfo = new SHORT_EVENT_INFO;
		updateFlag = TRUE;
	}else{
		updateFlag = CheckUpdate(table_id, version_number, eventInfo->shortInfo->tableID, eventInfo->shortInfo->version, skySDFlag);
	}
	if( updateFlag == TRUE ){
		//更新必要
		eventInfo->shortInfo->tableID = table_id;
		eventInfo->shortInfo->version = version_number;

		CARIB8CharDecode arib;
		string event_name = "";
		string text_char = "";
		const char* src;
		DWORD srcSize;
		src = shortEvent->GetStringOrEmpty(AribDescriptor::event_name_char, &srcSize);
		arib.PSISI((const BYTE*)src, srcSize, &event_name);
		src = shortEvent->GetStringOrEmpty(AribDescriptor::text_char, &srcSize);
		arib.PSISI((const BYTE*)src, srcSize, &text_char);

		eventInfo->shortInfo->event_nameLength = UpdateInfoText(eventInfo->shortInfo->event_name, event_name.c_str());
		eventInfo->shortInfo->text_charLength = UpdateInfoText(eventInfo->shortInfo->text_char, text_char.c_str());
	}

	return updateFlag;
}

BOOL CEpgDBUtil::AddExtEvent(BYTE table_id, BYTE version_number, EVENT_INFO* eventInfo, vector<AribDescriptor::CDescriptor*>* descriptorList, BOOL skySDFlag)
{
	BOOL updateFlag = FALSE;
	if( eventInfo->extInfo == NULL ){
		eventInfo->extInfo = new EXTENDED_EVENT_INFO;

		updateFlag = TRUE;
	}else{
		updateFlag = CheckUpdate(table_id, version_number, eventInfo->extInfo->tableID, eventInfo->extInfo->version, skySDFlag);
	}

	if( updateFlag == TRUE ){
		//更新必要
		eventInfo->extInfo->tableID = table_id;
		eventInfo->extInfo->version = version_number;

		CARIB8CharDecode arib;
		string extendText = "";
		string itemDescBuff = "";
		string itemBuff = "";
		//text_lengthは0で運用される
//		string textBuff = "";

		for( size_t i=0; i<descriptorList->size(); i++ ){
			if( (*descriptorList)[i]->GetNumber(AribDescriptor::descriptor_tag) == AribDescriptor::extended_event_descriptor ){
				AribDescriptor::CDescriptor* extEvent = (*descriptorList)[i];
				AribDescriptor::CDescriptor::CLoopPointer lp;
				if( extEvent->EnterLoop(lp) ){
					for( DWORD j=0; extEvent->SetLoopIndex(lp, j); j++ ){
						const char* src;
						DWORD srcSize;
						src = extEvent->GetStringOrEmpty(AribDescriptor::item_description_char, &srcSize, lp);
						if( srcSize > 0 ){
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

							itemDescBuff += src;
						}
						src = extEvent->GetStringOrEmpty(AribDescriptor::item_char, &srcSize, lp);
						if( srcSize > 0 ){
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

							itemBuff += src;
						}
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

		eventInfo->extInfo->text_charLength = UpdateInfoText(eventInfo->extInfo->text_char, extendText.c_str());
	}

	return updateFlag;
}

BOOL CEpgDBUtil::AddContent(BYTE table_id, BYTE version_number, EVENT_INFO* eventInfo, AribDescriptor::CDescriptor* content, BOOL skySDFlag)
{
	BOOL updateFlag = FALSE;
	if( eventInfo->contentInfo == NULL ){
		eventInfo->contentInfo = new CONTEN_INFO;
		updateFlag = TRUE;
	}else{
		updateFlag = CheckUpdate(table_id, version_number, eventInfo->contentInfo->tableID, eventInfo->contentInfo->version, skySDFlag);
	}
	if( updateFlag == TRUE ){
		//更新必要
		eventInfo->contentInfo->tableID = table_id;
		eventInfo->contentInfo->version = version_number;

		eventInfo->contentInfo->listSize = 0;
		SAFE_DELETE_ARRAY(eventInfo->contentInfo->nibbleList);
		AribDescriptor::CDescriptor::CLoopPointer lp;
		if( content->EnterLoop(lp) ){
			eventInfo->contentInfo->listSize = (WORD)content->GetLoopSize(lp);
			eventInfo->contentInfo->nibbleList = new EPG_CONTENT[eventInfo->contentInfo->listSize];
			for( DWORD i=0; content->SetLoopIndex(lp, i); i++ ){
				EPG_CONTENT nibble;
				nibble.content_nibble_level_1 = (BYTE)content->GetNumber(AribDescriptor::content_nibble_level_1, lp);
				nibble.content_nibble_level_2 = (BYTE)content->GetNumber(AribDescriptor::content_nibble_level_2, lp);
				nibble.user_nibble_1 = (BYTE)content->GetNumber(AribDescriptor::user_nibble_1, lp);
				nibble.user_nibble_2 = (BYTE)content->GetNumber(AribDescriptor::user_nibble_2, lp);
				eventInfo->contentInfo->nibbleList[i] = nibble;
			}
		}
	}

	return updateFlag;
}

BOOL CEpgDBUtil::AddComponent(BYTE table_id, BYTE version_number, EVENT_INFO* eventInfo, AribDescriptor::CDescriptor* component, BOOL skySDFlag)
{
	BOOL updateFlag = FALSE;
	if( eventInfo->componentInfo == NULL ){
		eventInfo->componentInfo = new COMPONENT_INFO;
		updateFlag = TRUE;
	}else{
		updateFlag = CheckUpdate(table_id, version_number, eventInfo->componentInfo->tableID, eventInfo->componentInfo->version, skySDFlag);
	}
	if( updateFlag == TRUE ){
		//更新必要
		eventInfo->componentInfo->tableID = table_id;
		eventInfo->componentInfo->version = version_number;

		eventInfo->componentInfo->stream_content = (BYTE)component->GetNumber(AribDescriptor::stream_content);
		eventInfo->componentInfo->component_type = (BYTE)component->GetNumber(AribDescriptor::component_type);
		eventInfo->componentInfo->component_tag = (BYTE)component->GetNumber(AribDescriptor::component_tag);

		CARIB8CharDecode arib;
		string text_char = "";
		DWORD srcSize;
		const char* src = component->GetStringOrEmpty(AribDescriptor::text_char, &srcSize);
		arib.PSISI((const BYTE*)src, srcSize, &text_char);
		eventInfo->componentInfo->text_charLength = UpdateInfoText(eventInfo->componentInfo->text_char, text_char.c_str());

	}

	return updateFlag;
}

BOOL CEpgDBUtil::AddAudioComponent(BYTE table_id, BYTE version_number, EVENT_INFO* eventInfo, vector<AribDescriptor::CDescriptor*>* descriptorList, BOOL skySDFlag)
{
	BOOL updateFlag = FALSE;
	if( eventInfo->audioInfo == NULL ){
		eventInfo->audioInfo = new AUDIO_COMPONENT_INFO;
		updateFlag = TRUE;
	}else{
		updateFlag = CheckUpdate(table_id, version_number, eventInfo->audioInfo->tableID, eventInfo->audioInfo->version, skySDFlag);
	}
	if( updateFlag == TRUE ){
		//更新必要
		eventInfo->audioInfo->tableID = table_id;
		eventInfo->audioInfo->version = version_number;
		eventInfo->audioInfo->listSize = 0;
		SAFE_DELETE_ARRAY(eventInfo->audioInfo->audioList);

		for( size_t i=0; i<descriptorList->size(); i++ ){
			if( (*descriptorList)[i]->GetNumber(AribDescriptor::descriptor_tag) == AribDescriptor::audio_component_descriptor ){
				eventInfo->audioInfo->listSize++;
			}
		}
		if( eventInfo->audioInfo->listSize > 0 ){
			eventInfo->audioInfo->audioList = new EPG_AUDIO_COMPONENT_INFO_DATA[eventInfo->audioInfo->listSize];
		}
		for( size_t i=0, j=0; j<eventInfo->audioInfo->listSize; i++ ){
			if( (*descriptorList)[i]->GetNumber(AribDescriptor::descriptor_tag) == AribDescriptor::audio_component_descriptor ){
				AribDescriptor::CDescriptor* audioComponent = (*descriptorList)[i];
				EPG_AUDIO_COMPONENT_INFO_DATA& item = eventInfo->audioInfo->audioList[j++];

				item.stream_content = (BYTE)audioComponent->GetNumber(AribDescriptor::stream_content);
				item.component_type = (BYTE)audioComponent->GetNumber(AribDescriptor::component_type);
				item.component_tag = (BYTE)audioComponent->GetNumber(AribDescriptor::component_tag);

				item.stream_type = (BYTE)audioComponent->GetNumber(AribDescriptor::stream_type);
				item.simulcast_group_tag = (BYTE)audioComponent->GetNumber(AribDescriptor::simulcast_group_tag);
				item.ES_multi_lingual_flag = (BYTE)audioComponent->GetNumber(AribDescriptor::ES_multi_lingual_flag);
				item.main_component_flag = (BYTE)audioComponent->GetNumber(AribDescriptor::main_component_flag);
				item.quality_indicator = (BYTE)audioComponent->GetNumber(AribDescriptor::quality_indicator);
				item.sampling_rate = (BYTE)audioComponent->GetNumber(AribDescriptor::sampling_rate);


				CARIB8CharDecode arib;
				string text_char = "";
				DWORD srcSize;
				const char* src = audioComponent->GetStringOrEmpty(AribDescriptor::text_char, &srcSize);
				arib.PSISI((const BYTE*)src, srcSize, &text_char);
				item.text_charLength = UpdateInfoText(item.text_char, text_char.c_str());

			}
		}
	}

	return updateFlag;
}

BOOL CEpgDBUtil::AddEventGroup(CEITTable* eit, EVENT_INFO* eventInfo, AribDescriptor::CDescriptor* eventGroup)
{
	BOOL updateFlag = FALSE;
	if( eventInfo->eventGroupInfo == NULL ){
		eventInfo->eventGroupInfo = new EVENTGROUP_INFO;
		updateFlag = TRUE;
	}else{
		updateFlag = CheckUpdate(eit->table_id, eit->version_number, eventInfo->eventGroupInfo->tableID, eventInfo->eventGroupInfo->version, FALSE);
	}
	if( updateFlag == TRUE ){
		//更新必要
		eventInfo->eventGroupInfo->tableID = eit->table_id;
		eventInfo->eventGroupInfo->version = eit->version_number;
		SAFE_DELETE_ARRAY(eventInfo->eventGroupInfo->eventDataList);

		eventInfo->eventGroupInfo->group_type = (BYTE)eventGroup->GetNumber(AribDescriptor::group_type);
		eventInfo->eventGroupInfo->event_count = 0;
		AribDescriptor::CDescriptor::CLoopPointer lp;
		if( eventGroup->EnterLoop(lp) ){
			eventInfo->eventGroupInfo->event_count = (BYTE)eventGroup->GetLoopSize(lp);
			eventInfo->eventGroupInfo->eventDataList = new EPG_EVENT_DATA[eventInfo->eventGroupInfo->event_count];
			for( DWORD i=0; eventGroup->SetLoopIndex(lp, i); i++ ){
				EPG_EVENT_DATA item;
				item.event_id = (WORD)eventGroup->GetNumber(AribDescriptor::event_id, lp);
				item.service_id = (WORD)eventGroup->GetNumber(AribDescriptor::service_id, lp);
				item.original_network_id = eit->original_network_id;
				item.transport_stream_id = eit->transport_stream_id;

				eventInfo->eventGroupInfo->eventDataList[i] = item;
			}
		}
	}

	return updateFlag;
}

BOOL CEpgDBUtil::AddEventRelay(CEITTable* eit, EVENT_INFO* eventInfo, AribDescriptor::CDescriptor* eventGroup)
{
	BOOL updateFlag = FALSE;
	if( eventInfo->eventRelayInfo == NULL ){
		eventInfo->eventRelayInfo = new EVENTGROUP_INFO;
		updateFlag = TRUE;
	}else{
		updateFlag = CheckUpdate(eit->table_id, eit->version_number, eventInfo->eventRelayInfo->tableID, eventInfo->eventRelayInfo->version, FALSE);
	}
	if( updateFlag == TRUE ){
		//更新必要
		eventInfo->eventRelayInfo->tableID = eit->table_id;
		eventInfo->eventRelayInfo->version = eit->version_number;
		SAFE_DELETE_ARRAY(eventInfo->eventRelayInfo->eventDataList);

		eventInfo->eventRelayInfo->group_type = (BYTE)eventGroup->GetNumber(AribDescriptor::group_type);
		eventInfo->eventRelayInfo->event_count = 0;
		if( eventInfo->eventRelayInfo->group_type == 0x02 ){
			AribDescriptor::CDescriptor::CLoopPointer lp;
			if( eventGroup->EnterLoop(lp) ){
				eventInfo->eventRelayInfo->event_count = (BYTE)eventGroup->GetLoopSize(lp);
				eventInfo->eventRelayInfo->eventDataList = new EPG_EVENT_DATA[eventInfo->eventRelayInfo->event_count];
				for( DWORD i=0; eventGroup->SetLoopIndex(lp, i); i++ ){
					EPG_EVENT_DATA item;
					item.event_id = (WORD)eventGroup->GetNumber(AribDescriptor::event_id, lp);
					item.service_id = (WORD)eventGroup->GetNumber(AribDescriptor::service_id, lp);
					item.original_network_id = eit->original_network_id;
					item.transport_stream_id = eit->transport_stream_id;

					eventInfo->eventRelayInfo->eventDataList[i] = item;
				}
			}
		}else{
			AribDescriptor::CDescriptor::CLoopPointer lp;
			if( eventGroup->EnterLoop(lp, 1) ){
				//他ネットワークへのリレー情報は第2ループにあるので、これは記述子のevent_countの値とは異なる
				eventInfo->eventRelayInfo->event_count = (BYTE)eventGroup->GetLoopSize(lp);
				eventInfo->eventRelayInfo->eventDataList = new EPG_EVENT_DATA[eventInfo->eventRelayInfo->event_count];
				for( DWORD i=0; eventGroup->SetLoopIndex(lp, i); i++ ){
					EPG_EVENT_DATA item;
					item.event_id = (WORD)eventGroup->GetNumber(AribDescriptor::event_id, lp);
					item.service_id = (WORD)eventGroup->GetNumber(AribDescriptor::service_id, lp);
					item.original_network_id = (WORD)eventGroup->GetNumber(AribDescriptor::original_network_id, lp);
					item.transport_stream_id = (WORD)eventGroup->GetNumber(AribDescriptor::transport_stream_id, lp);

					eventInfo->eventRelayInfo->eventDataList[i] = item;
				}
			}
		}

	}

	return updateFlag;
}

BOOL CEpgDBUtil::CheckUpdate(BYTE eit_table_id, BYTE eit_version_number, BYTE tableID, BYTE version, BOOL skySDFlag)
{
	if( skySDFlag != FALSE ){
		return CheckUpdate_SD(eit_table_id, eit_version_number, tableID, version);
	}
	BOOL changeFlag = FALSE;
	if( eit_table_id == 0x4E ){
		if( tableID == 0x4E ){
			if( version != eit_version_number ){
				//バージョン変わったので更新
				changeFlag = TRUE;
			}else{
				//バージョン変わっていないので更新の必要なし
			}
		}else{
			//[p/f]が最新のはずなので更新
			changeFlag = TRUE;
		}
	}else if( eit_table_id == 0x4F ){
		if( tableID == 0x4F ){
			if( version != eit_version_number ){
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
	}else if( 0x50 <= eit_table_id && eit_table_id <= 0x5F ){
		if( 0x50 <= tableID && tableID <= 0x5F ){
			if( tableID == eit_table_id ){
				if( version != eit_version_number ){
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
	}else if( 0x60 <= eit_table_id && eit_table_id <= 0x6F ){
		if( 0x60 <= tableID && tableID <= 0x6F ){
			if( tableID == eit_table_id ){
				if( version != eit_version_number ){
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

BOOL CEpgDBUtil::CheckUpdate_SD(BYTE eit_table_id, BYTE eit_version_number, BYTE tableID, BYTE version)
{
	BOOL changeFlag = FALSE;
	if( 0xA0 <= eit_table_id && eit_table_id <= 0xAF ){
		if( 0xA0 <= tableID && tableID <= 0xAF ){
			if( tableID == eit_table_id ){
				if( version != eit_version_number ){
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
	CBlockLock lock(&this->dbLock);

	this->sectionMap.clear();
	this->sectionNowFlag = 0;
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
	CBlockLock lock(&this->dbLock);

	EPG_SECTION_STATUS status = EpgNoData;
	if( this->sectionMap.size() == 0 ){
		return status;
	}

	BOOL basicFlag = TRUE;
	BOOL extFlag = TRUE;
	BOOL leitFlag = TRUE;

	map<ULONGLONG, SECTION_STATUS_INFO>::iterator itr;
	for( itr = this->sectionMap.begin(); itr != this->sectionMap.end(); itr++ ){
		if( l_eitFlag == TRUE ){
			//L-EITの状況
			if( itr->second.HEITFlag == FALSE ){
				if( itr->second.last_section_numberBasic > 0 ){
					if( CheckSectionAll( &itr->second.sectionBasicMap, TRUE ) == FALSE ){
						leitFlag = FALSE;
						break;
					}
				}
			}
		}else{
			//H-EITの状況
			if( itr->second.HEITFlag == TRUE ){
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
				if( itr->second.last_section_numberBasic > 0 ){
					if( CheckSectionAll( &itr->second.sectionBasicMap ) == FALSE ){
						basicFlag = FALSE;
					}
				}
				//Ext
				if( itr->second.last_section_numberExt > 0 ){
					if( CheckSectionAll( &itr->second.sectionExtMap ) == FALSE ){
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
	return status;
}

BOOL CEpgDBUtil::AddServiceList(CNITTable* nit)
{
	if( nit == NULL ){
		return FALSE;
	}
	CBlockLock lock(&this->dbLock);

	wstring network_nameW = L"";

	for( size_t i=0; i<nit->descriptorList.size(); i++ ){
		if( nit->descriptorList[i]->GetNumber(AribDescriptor::descriptor_tag) == AribDescriptor::network_name_descriptor ){
			AribDescriptor::CDescriptor* networkName = nit->descriptorList[i];
			DWORD srcSize;
			const char* src = networkName->GetStringOrEmpty(AribDescriptor::d_char, &srcSize);
			if( srcSize > 0 ){
				CARIB8CharDecode arib;
				string network_name = "";
				arib.PSISI((const BYTE*)src, srcSize, &network_name);
				AtoW(network_name, network_nameW);
			}
		}
	}

	for( size_t i=0; i<nit->TSInfoList.size(); i++ ){
		CNITTable::TS_INFO_DATA* tsInfo = nit->TSInfoList[i];
		//サービス情報更新用
		DWORD key = ((DWORD)tsInfo->original_network_id) <<16 | tsInfo->transport_stream_id;
		map<DWORD, DB_TS_INFO>::iterator itrFind;
		itrFind = this->serviceInfoList.find(key);
		if( itrFind != this->serviceInfoList.end() ){
			itrFind->second.network_name = network_nameW;
		}

		for( size_t j=0; j<tsInfo->descriptorList.size(); j++ ){
			AribDescriptor::CDescriptor* desc = tsInfo->descriptorList[j];
			if( desc->GetNumber(AribDescriptor::descriptor_tag) == AribDescriptor::service_list_descriptor ){
				AribDescriptor::CDescriptor::CLoopPointer lp;
				if( desc->EnterLoop(lp) ){
					for( DWORD k=0; desc->SetLoopIndex(lp, k); k++ ){
						ULONGLONG key = _Create64Key(tsInfo->original_network_id, tsInfo->transport_stream_id, (WORD)desc->GetNumber(AribDescriptor::service_id, lp));
						map<ULONGLONG, BYTE>::iterator itrService;
						itrService = this->serviceList.find(key);
						if( itrService == this->serviceList.end() ){
							this->serviceList.insert(pair<ULONGLONG, BYTE>(key, (BYTE)desc->GetNumber(AribDescriptor::service_type, lp)));
						}
					}
				}
			}
			if( desc->GetNumber(AribDescriptor::descriptor_tag) == AribDescriptor::ts_information_descriptor && itrFind != this->serviceInfoList.end()){
				//ts_nameとremote_control_key_id
				DWORD srcSize;
				const char* src = desc->GetStringOrEmpty(AribDescriptor::ts_name_char, &srcSize);
				if( srcSize > 0 ){
					CARIB8CharDecode arib;
					string ts_name = "";
					arib.PSISI((const BYTE*)src, srcSize, &ts_name);
					AtoW(ts_name, itrFind->second.ts_name);
				}
				itrFind->second.remote_control_key_id = (BYTE)desc->GetNumber(AribDescriptor::remote_control_key_id);
			}
			if( desc->GetNumber(AribDescriptor::descriptor_tag) == AribDescriptor::partial_reception_descriptor && itrFind != this->serviceInfoList.end()){
				//部分受信フラグ
				AribDescriptor::CDescriptor::CLoopPointer lp;
				if( desc->EnterLoop(lp) ){
					map<WORD,DB_SERVICE_INFO>::iterator itrService;
					for( DWORD k=0; desc->SetLoopIndex(lp, k); k++ ){
						itrService = itrFind->second.serviceList.find((WORD)desc->GetNumber(AribDescriptor::service_id, lp));
						if( itrService != itrFind->second.serviceList.end() ){
							itrService->second.partialReceptionFlag = 1;
						}
					}
				}
			}
		}
	}

	return TRUE;
}

BOOL CEpgDBUtil::AddServiceList(WORD TSID, CSITTable* sit)
{
	CBlockLock lock(&this->dbLock);

	WORD ONID = 0xFFFF;
	for( size_t i=0; i<sit->descriptorList.size(); i++ ){
		if( sit->descriptorList[i]->GetNumber(AribDescriptor::descriptor_tag) == AribDescriptor::network_identification_descriptor ){
			ONID = (WORD)sit->descriptorList[i]->GetNumber(AribDescriptor::network_id);
		}
	}
	if(ONID == 0xFFFF){
		return FALSE;
	}

	DWORD key = ((DWORD)ONID)<<16 | TSID;
	map<DWORD, DB_TS_INFO>::iterator itrTS;
	itrTS = this->serviceInfoList.find(key);
	if( itrTS == this->serviceInfoList.end() ){
		DB_TS_INFO info;
		info.original_network_id = ONID;
		info.transport_stream_id = TSID;

		for(size_t i=0; i<sit->serviceLoopList.size(); i++ ){
			DB_SERVICE_INFO item;
			item.original_network_id = ONID;
			item.transport_stream_id = TSID;
			item.service_id = sit->serviceLoopList[i]->service_id;

			for( size_t j=0; j<sit->serviceLoopList[i]->descriptorList.size(); j++ ){
				if( sit->serviceLoopList[i]->descriptorList[j]->GetNumber(AribDescriptor::descriptor_tag) == AribDescriptor::service_descriptor ){
					AribDescriptor::CDescriptor* service = sit->serviceLoopList[i]->descriptorList[j];
					CARIB8CharDecode arib;
					string service_provider_name = "";
					string service_name = "";
					const char* src;
					DWORD srcSize;
					src = service->GetStringOrEmpty(AribDescriptor::service_provider_name, &srcSize);
					if( srcSize > 0 ){
						arib.PSISI((const BYTE*)src, srcSize, &service_provider_name);
					}
					src = service->GetStringOrEmpty(AribDescriptor::service_name, &srcSize);
					if( srcSize > 0 ){
						arib.PSISI((const BYTE*)src, srcSize, &service_name);
					}
					AtoW(service_provider_name, item.service_provider_name);
					AtoW(service_name, item.service_name);

					item.service_type = (BYTE)service->GetNumber(AribDescriptor::service_type);
				}
			}
			info.serviceList.insert(std::make_pair(item.service_id, item));
		}
		this->serviceInfoList.insert(std::make_pair(key, info));
	}


	return TRUE;
}

BOOL CEpgDBUtil::AddSDT(CSDTTable* sdt)
{
	CBlockLock lock(&this->dbLock);

	DWORD key = ((DWORD)sdt->original_network_id)<<16 | sdt->transport_stream_id;
	map<DWORD, DB_TS_INFO>::iterator itrTS;
	itrTS = this->serviceInfoList.find(key);
	if( itrTS == this->serviceInfoList.end() ){
		DB_TS_INFO info;
		info.original_network_id = sdt->original_network_id;
		info.transport_stream_id = sdt->transport_stream_id;

		for(size_t i=0; i<sdt->serviceInfoList.size(); i++ ){
			DB_SERVICE_INFO item;
			item.original_network_id = sdt->original_network_id;
			item.transport_stream_id = sdt->transport_stream_id;
			item.service_id = sdt->serviceInfoList[i]->service_id;

			for( size_t j=0; j<sdt->serviceInfoList[i]->descriptorList.size(); j++ ){
				if( sdt->serviceInfoList[i]->descriptorList[j]->GetNumber(AribDescriptor::descriptor_tag) == AribDescriptor::service_descriptor ){
					AribDescriptor::CDescriptor* service = sdt->serviceInfoList[i]->descriptorList[j];
					CARIB8CharDecode arib;
					string service_provider_name = "";
					string service_name = "";
					const char* src;
					DWORD srcSize;
					src = service->GetStringOrEmpty(AribDescriptor::service_provider_name, &srcSize);
					if( srcSize > 0 ){
						arib.PSISI((const BYTE*)src, srcSize, &service_provider_name);
					}
					src = service->GetStringOrEmpty(AribDescriptor::service_name, &srcSize);
					if( srcSize > 0 ){
						arib.PSISI((const BYTE*)src, srcSize, &service_name);
					}
					AtoW(service_provider_name, item.service_provider_name);
					AtoW(service_name, item.service_name);

					item.service_type = (BYTE)service->GetNumber(AribDescriptor::service_type);
				}
			}
			info.serviceList.insert(std::make_pair(item.service_id, item));
		}
		this->serviceInfoList.insert(std::make_pair(key, info));
	}else{
		for(size_t i=0; i<sdt->serviceInfoList.size(); i++ ){
			map<WORD,DB_SERVICE_INFO>::iterator itrS;
			itrS = itrTS->second.serviceList.find(sdt->serviceInfoList[i]->service_id);
			if( itrS == itrTS->second.serviceList.end()){
				DB_SERVICE_INFO item;
				item.original_network_id = sdt->original_network_id;
				item.transport_stream_id = sdt->transport_stream_id;
				item.service_id = sdt->serviceInfoList[i]->service_id;

				for( size_t j=0; j<sdt->serviceInfoList[i]->descriptorList.size(); j++ ){
					if( sdt->serviceInfoList[i]->descriptorList[j]->GetNumber(AribDescriptor::descriptor_tag) == AribDescriptor::service_descriptor ){
						AribDescriptor::CDescriptor* service = sdt->serviceInfoList[i]->descriptorList[j];
						CARIB8CharDecode arib;
						string service_provider_name = "";
						string service_name = "";
						const char* src;
						DWORD srcSize;
						src = service->GetStringOrEmpty(AribDescriptor::service_provider_name, &srcSize);
						if( srcSize > 0 ){
							arib.PSISI((const BYTE*)src, srcSize, &service_provider_name);
						}
						src = service->GetStringOrEmpty(AribDescriptor::service_name, &srcSize);
						if( srcSize > 0 ){
							arib.PSISI((const BYTE*)src, srcSize, &service_name);
						}
						AtoW(service_provider_name, item.service_provider_name);
						AtoW(service_name, item.service_name);

						item.service_type = (BYTE)service->GetNumber(AribDescriptor::service_type);
					}
				}
				itrTS->second.serviceList.insert(std::make_pair(item.service_id, item));
			}
		}
	}

	return TRUE;
}

//指定サービスの全EPG情報を取得する
// originalNetworkID		[IN]取得対象のoriginalNetworkID
// transportStreamID		[IN]取得対象のtransportStreamID
// serviceID				[IN]取得対象のServiceID
// epgInfoListSize			[OUT]epgInfoListの個数
// epgInfoList				[OUT]EPG情報のリスト（DLL内で自動的にdeleteする。次に取得を行うまで有効）
BOOL CEpgDBUtil::GetEpgInfoList(
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	DWORD* epgInfoListSize,
	EPG_EVENT_INFO** epgInfoList
	)
{
	CBlockLock lock(&this->dbLock);

	SAFE_DELETE_ARRAY(this->epgInfoList);

	ULONGLONG key = _Create64Key(originalNetworkID, transportStreamID, serviceID);

	map<ULONGLONG, SERVICE_EVENT_INFO>::iterator itr;
	itr = serviceEventMap.find(key);
	if( itr == serviceEventMap.end() ){
		return FALSE;
	}

	if( itr->second.eventMap.size() == 0 ){
		return FALSE;
	}
	*epgInfoListSize = (DWORD)itr->second.eventMap.size();
	this->epgInfoList = new EPG_EVENT_INFO[*epgInfoListSize];

	map<WORD, EVENT_INFO*>::iterator itrEvt;
	DWORD count = 0;
	for( itrEvt = itr->second.eventMap.begin(); itrEvt != itr->second.eventMap.end(); itrEvt++ ){
		CopyEpgInfo(this->epgInfoList+count, itrEvt->second);
		count++;
	}

	*epgInfoList = this->epgInfoList;

	return TRUE;
}

//アドレスxをTのアラインメントで切り上げて返す
template<class T> static inline T* AlignCeil(void* x)
{
	return (T*)(((size_t)x + (__alignof(T) - 1)) & ~(__alignof(T) - 1));
}

//指定サービスの全EPG情報を列挙する
BOOL CEpgDBUtil::EnumEpgInfoList(
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	BOOL (CALLBACK *enumEpgInfoListProc)(DWORD, EPG_EVENT_INFO*, LPVOID),
	LPVOID param
	)
{
	CBlockLock lock(&this->dbLock);

	map<ULONGLONG, SERVICE_EVENT_INFO>::iterator itr =
		this->serviceEventMap.find(_Create64Key(originalNetworkID, transportStreamID, serviceID));
	if( itr == this->serviceEventMap.end() || itr->second.eventMap.empty() ){
		return FALSE;
	}
	if( enumEpgInfoListProc((DWORD)itr->second.eventMap.size(), NULL, param) == FALSE ){
		return TRUE;
	}

	BYTE info[__alignof(EPG_EVENT_INFO) + sizeof(EPG_EVENT_INFO) * 32];

	map<WORD, EVENT_INFO*>::iterator itrEvt;
	DWORD count = 0;
	for( itrEvt = itr->second.eventMap.begin(); itrEvt != itr->second.eventMap.end(); itrEvt++ ){
		//デストラクタを呼ばないよう領域だけ割り当て(POD構造体だけなので無問題)、マスターを直接参照して構築する
		EPG_EVENT_INFO* item = AlignCeil<EPG_EVENT_INFO>(info) + count;
		const EVENT_INFO* evt = itrEvt->second;
		item->event_id = evt->event_id;
		item->StartTimeFlag = evt->StartTimeFlag;
		item->start_time = evt->start_time;
		item->DurationFlag = evt->DurationFlag;
		item->durationSec = evt->durationSec;
		item->freeCAFlag = evt->freeCAFlag;
		item->shortInfo = evt->shortInfo;
		item->extInfo = evt->extInfo;
		item->contentInfo = evt->contentInfo;
		item->componentInfo = evt->componentInfo;
		item->audioInfo = evt->audioInfo;
		item->eventGroupInfo = evt->eventGroupInfo;
		item->eventRelayInfo = evt->eventRelayInfo;
		if( ++count >= 32 ){
			if( enumEpgInfoListProc(count, AlignCeil<EPG_EVENT_INFO>(info), param) == FALSE ){
				return TRUE;
			}
			count = 0;
		}
	}
	if( count > 0 ){
		enumEpgInfoListProc(count, AlignCeil<EPG_EVENT_INFO>(info), param);
	}
	return TRUE;
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
		destInfo->shortInfo = new EPG_SHORT_EVENT_INFO;
		destInfo->shortInfo->DeepCopy(*srcInfo->shortInfo);
	}

	if( srcInfo->extInfo != NULL ){
		destInfo->extInfo = new EPG_EXTENDED_EVENT_INFO;
		destInfo->extInfo->DeepCopy(*srcInfo->extInfo);
	}

	if( srcInfo->contentInfo != NULL ){
		destInfo->contentInfo = new EPG_CONTEN_INFO;
		destInfo->contentInfo->DeepCopy(*srcInfo->contentInfo);
	}

	if( srcInfo->componentInfo != NULL ){
		destInfo->componentInfo = new EPG_COMPONENT_INFO;
		destInfo->componentInfo->DeepCopy(*srcInfo->componentInfo);
	}

	if( srcInfo->audioInfo != NULL ){
		destInfo->audioInfo = new EPG_AUDIO_COMPONENT_INFO;
		destInfo->audioInfo->DeepCopy(*srcInfo->audioInfo);
	}

	if( srcInfo->eventGroupInfo != NULL ){
		destInfo->eventGroupInfo = new EPG_EVENTGROUP_INFO;
		destInfo->eventGroupInfo->DeepCopy(*srcInfo->eventGroupInfo);
	}

	if( srcInfo->eventRelayInfo != NULL ){
		destInfo->eventRelayInfo = new EPG_EVENTGROUP_INFO;
		destInfo->eventRelayInfo->DeepCopy(*srcInfo->eventRelayInfo);
	}

}

//蓄積されたEPG情報のあるサービス一覧を取得する
//SERVICE_EXT_INFOの情報はない場合がある
//引数：
// serviceListSize			[OUT]serviceListの個数
// serviceList				[OUT]サービス情報のリスト（DLL内で自動的にdeleteする。次に取得を行うまで有効）
void CEpgDBUtil::GetServiceListEpgDB(
	DWORD* serviceListSize,
	SERVICE_INFO** serviceList
	)
{
	CBlockLock lock(&this->dbLock);

	SAFE_DELETE_ARRAY(this->serviceDBList);

	*serviceListSize = (DWORD)this->serviceEventMap.size();
	this->serviceDBList = new SERVICE_INFO[*serviceListSize];

	DWORD count = 0;
	map<ULONGLONG, SERVICE_EVENT_INFO>::iterator itr;
	for(itr = this->serviceEventMap.begin(); itr != this->serviceEventMap.end(); itr++ ){
		this->serviceDBList[count].original_network_id = (WORD)(itr->first>>32);
		this->serviceDBList[count].transport_stream_id = (WORD)((itr->first&0xFFFF0000)>>16);
		this->serviceDBList[count].service_id = (WORD)(itr->first&0xFFFF);

		DWORD infoKey = ((DWORD)this->serviceDBList[count].original_network_id) << 16 | this->serviceDBList[count].transport_stream_id;
		map<DWORD, DB_TS_INFO>::iterator itrInfo;
		itrInfo = this->serviceInfoList.find(infoKey);
		if( itrInfo != this->serviceInfoList.end() ){
			map<WORD,DB_SERVICE_INFO>::iterator itrService;
			itrService = itrInfo->second.serviceList.find(this->serviceDBList[count].service_id);
			if( itrService != itrInfo->second.serviceList.end() ){
				DB_TS_INFO* info = &itrInfo->second;
				DB_SERVICE_INFO* item = &itrService->second;
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

	*serviceList = this->serviceDBList;
}

//指定サービスの現在or次のEPG情報を取得する
//引数：
// originalNetworkID		[IN]取得対象のoriginalNetworkID
// transportStreamID		[IN]取得対象のtransportStreamID
// serviceID				[IN]取得対象のServiceID
// nextFlag					[IN]TRUE（次の番組）、FALSE（現在の番組）
// epgInfo					[OUT]EPG情報（DLL内で自動的にdeleteする。次に取得を行うまで有効）
BOOL CEpgDBUtil::GetEpgInfo(
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	BOOL nextFlag,
	EPG_EVENT_INFO** epgInfo
	)
{
	CBlockLock lock(&this->dbLock);

	SAFE_DELETE(this->epgInfo);

	ULONGLONG key = _Create64Key(originalNetworkID, transportStreamID, serviceID);

	map<ULONGLONG, SERVICE_EVENT_INFO>::iterator itr;
	itr = serviceEventMap.find(key);
	if( itr == serviceEventMap.end() ){
		return FALSE;
	}

	if( itr->second.nowEvent != NULL && nextFlag == FALSE ){
		this->epgInfo = new EPG_EVENT_INFO;
		CopyEpgInfo(this->epgInfo, itr->second.nowEvent);
		*epgInfo = this->epgInfo;
		return TRUE;
	}else if( itr->second.nextEvent != NULL && nextFlag == TRUE ){
		this->epgInfo = new EPG_EVENT_INFO;
		CopyEpgInfo(this->epgInfo, itr->second.nextEvent);
		*epgInfo = this->epgInfo;
		return TRUE;
	}

	return FALSE;
}

//指定イベントのEPG情報を取得する
//引数：
// originalNetworkID		[IN]取得対象のoriginalNetworkID
// transportStreamID		[IN]取得対象のtransportStreamID
// serviceID				[IN]取得対象のServiceID
// EventID					[IN]取得対象のEventID
// pfOnlyFlag				[IN]p/fからのみ検索するかどうか
// epgInfo					[OUT]EPG情報（DLL内で自動的にdeleteする。次に取得を行うまで有効）
BOOL CEpgDBUtil::SearchEpgInfo(
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	WORD eventID,
	BYTE pfOnlyFlag,
	EPG_EVENT_INFO** epgInfo
	)
{
	CBlockLock lock(&this->dbLock);

	SAFE_DELETE(this->searchEpgInfo);

	ULONGLONG key = _Create64Key(originalNetworkID, transportStreamID, serviceID);

	map<ULONGLONG, SERVICE_EVENT_INFO>::iterator itr;
	itr = serviceEventMap.find(key);
	if( itr == serviceEventMap.end() ){
		return FALSE;
	}

	if( pfOnlyFlag == 0 ){
		map<WORD, EVENT_INFO*>::iterator itrEvent;
		itrEvent = itr->second.eventMap.find(eventID);
		if( itrEvent != itr->second.eventMap.end() ){
			this->searchEpgInfo = new EPG_EVENT_INFO;
			CopyEpgInfo(this->searchEpgInfo, itrEvent->second);
			goto Err_End;
		}
	}else{
		if( itr->second.nowEvent != NULL ){
			if( itr->second.nowEvent->event_id == eventID ){
				this->searchEpgInfo = new EPG_EVENT_INFO;
				CopyEpgInfo(this->searchEpgInfo, itr->second.nowEvent);
				goto Err_End;
			}
		}
		if( itr->second.nextEvent != NULL ){
			if( itr->second.nextEvent->event_id == eventID ){
				this->searchEpgInfo = new EPG_EVENT_INFO;
				CopyEpgInfo(this->searchEpgInfo, itr->second.nextEvent);
				goto Err_End;
			}
		}
	}

Err_End:
	if( this->searchEpgInfo == NULL ){
		return FALSE;
	}
	*epgInfo = this->searchEpgInfo;

	return TRUE;
}

BOOL CEpgDBUtil::AddSDEventMap(CEITTable_SD* eit)
{
	if( eit == NULL ){
		return FALSE;
	}

	ULONGLONG key = _Create64Key(eit->original_network_id, 0, eit->service_id);

	//サービスのmapを取得
	map<ULONGLONG, SERVICE_EVENT_INFO>::iterator itr;
	SERVICE_EVENT_INFO* serviceInfo = NULL;

	itr = serviceEventMapSD.find(key);
	if( itr == serviceEventMapSD.end() ){
		serviceInfo = &serviceEventMapSD.insert(std::make_pair(key, SERVICE_EVENT_INFO())).first->second;
	}else{
		serviceInfo = &itr->second;
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
			DWORD tag = eitEventInfo->descriptorList[j]->GetNumber(AribDescriptor::descriptor_tag);
			if( tag == AribDescriptor::short_event_descriptor ){
				AddShortEvent( eit->table_id, eit->version_number, eventInfo, eitEventInfo->descriptorList[j], TRUE );
			}else if( tag == AribDescriptor::extended_event_descriptor && checkExtFlag == FALSE){
				AddExtEvent(eit->table_id, eit->version_number, eventInfo, &eitEventInfo->descriptorList, TRUE );
				checkExtFlag = TRUE;
			}else if( tag == AribDescriptor::content_descriptor ){
				AddContent( eit->table_id, eit->version_number, eventInfo, eitEventInfo->descriptorList[j], TRUE );
			}else if( tag == AribDescriptor::component_descriptor ){
				AddComponent( eit->table_id, eit->version_number, eventInfo, eitEventInfo->descriptorList[j], TRUE );
			}else if( tag == AribDescriptor::audio_component_descriptor && checkAudioFlag == FALSE ){
				AddAudioComponent( eit->table_id, eit->version_number, eventInfo, &eitEventInfo->descriptorList, TRUE );
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
	CBlockLock lock(&this->dbLock);

	ULONGLONG key = _Create64Key(eit->original_network_id, 0, eit->service_id2);

	//サービスのmapを取得
	map<ULONGLONG, SERVICE_EVENT_INFO>::iterator itr;
	SERVICE_EVENT_INFO* serviceInfo = NULL;

	itr = serviceEventMapSD.find(key);
	if( itr != serviceEventMapSD.end() ){
		serviceInfo = &itr->second;
		for( size_t i=0; i<eit->eventMapList.size(); i++ ){
			for( size_t j=0; j<eit->eventMapList[i]->eventList.size(); j++ ){
				map<WORD, EVENT_INFO*>::iterator itrEvent;
				itrEvent = serviceInfo->eventMap.find(eit->eventMapList[i]->eventList[j].a4table_eventID);
				if( itrEvent != serviceInfo->eventMap.end() ){
					ULONGLONG key2 = _Create64Key(itrEvent->second->ONID, itrEvent->second->TSID, eit->service_id);
					map<ULONGLONG, SERVICE_EVENT_INFO>::iterator itrMainDB;
					SERVICE_EVENT_INFO* mainServiceInfo = NULL;
					itrMainDB = serviceEventMap.find(key2);
					if( itrMainDB == serviceEventMap.end() ){
						mainServiceInfo = &serviceEventMap.insert(std::make_pair(key2, SERVICE_EVENT_INFO())).first->second;
					}else{
						mainServiceInfo = &itrMainDB->second;
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
						eventInfo->shortInfo->DeepCopy(*itrEvent->second->shortInfo);
					}
					if(itrEvent->second->extInfo != NULL && eventInfo->extInfo == NULL ){
						eventInfo->extInfo = new EXTENDED_EVENT_INFO;
						eventInfo->extInfo->DeepCopy(*itrEvent->second->extInfo);
					}
					if(itrEvent->second->contentInfo != NULL && eventInfo->contentInfo == NULL ){
						eventInfo->contentInfo = new CONTEN_INFO;
						eventInfo->contentInfo->DeepCopy(*itrEvent->second->contentInfo);
					}
					if(itrEvent->second->componentInfo != NULL && eventInfo->componentInfo == NULL ){
						eventInfo->componentInfo = new COMPONENT_INFO;
						eventInfo->componentInfo->DeepCopy(*itrEvent->second->componentInfo);
					}
					if(itrEvent->second->audioInfo != NULL && eventInfo->audioInfo == NULL ){
						eventInfo->audioInfo = new AUDIO_COMPONENT_INFO;
						eventInfo->audioInfo->DeepCopy(*itrEvent->second->audioInfo);
					}

				}
			}
		}
	}

	return TRUE;
}
