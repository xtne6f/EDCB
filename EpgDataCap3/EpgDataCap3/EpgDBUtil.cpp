#include "StdAfx.h"
#include "EpgDBUtil.h"

#include "../../Common/EpgTimerUtil.h"
#include "../../Common/StringUtil.h"
#include "../../Common/TimeUtil.h"
#include "../../Common/BlockLock.h"
#include "ARIB8CharDecode.h"

//#define DEBUG_EIT
#ifdef DEBUG_EIT
static char g_szDebugEIT[128];
#endif

CEpgDBUtil::CEpgDBUtil(void)
{
	InitializeCriticalSection(&this->dbLock);
}

CEpgDBUtil::~CEpgDBUtil(void)
{
	DeleteCriticalSection(&this->dbLock);
}

void CEpgDBUtil::Clear()
{
	this->serviceEventMap.clear();
}

void CEpgDBUtil::SetStreamChangeEvent()
{
	//ここで[p/f]のリセットはしない
}

BOOL CEpgDBUtil::AddEIT(WORD PID, const CEITTable* eit, __int64 streamTime)
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

	SI_TAG siTag;
	siTag.tableID = eit->table_id;
	siTag.version = eit->version_number;
	siTag.time = (DWORD)(streamTime / (10 * I64_1SEC));

	if( eit->table_id <= 0x4F && eit->section_number <= 1 ){
		//[p/f]
		if( siTag.time == 0 ){
			//チャンネル変更時の応答性のため、タイムスタンプ不明の[p/f]が来たらDB上の不明でない[p/f]をクリアする
			//EPGファイルを入力するときは古い[p/f]による上書きが発生するので、利用側で時系列にするかタイムスタンプを確定させる工夫が必要
			if( serviceInfo->nowEvent && serviceInfo->nowEvent->time != 0 ||
			    serviceInfo->nextEvent && serviceInfo->nextEvent->time != 0 ){
				serviceInfo->nowEvent.reset();
				serviceInfo->nextEvent.reset();
			}
		}
		if( eit->eventInfoList.empty() ){
			//空セクション
			if( eit->section_number == 0 ){
				if( serviceInfo->nowEvent && siTag.time >= serviceInfo->nowEvent->time ){
					serviceInfo->nowEvent.reset();
				}
			}else{
				if( serviceInfo->nextEvent && siTag.time >= serviceInfo->nextEvent->time ){
					serviceInfo->nextEvent.reset();
				}
			}
		}
	}
	//イベントごとに更新必要が判定
	for( size_t i=0; i<eit->eventInfoList.size(); i++ ){
		const CEITTable::EVENT_INFO_DATA* eitEventInfo = &eit->eventInfoList[i];
		map<WORD, std::unique_ptr<EVENT_INFO>>::iterator itrEvent;
		EVENT_INFO* eventInfo = NULL;

		if( eitEventInfo->running_status == 1 || eitEventInfo->running_status == 3 ){
			//非実行中または停止中
			_OutputDebugString(L"★非実行中または停止中イベント ONID:0x%04x TSID:0x%04x SID:0x%04x EventID:0x%04x %04d/%02d/%02d %02d:%02d",
				eit->original_network_id,  eit->transport_stream_id, eit->service_id, eitEventInfo->event_id,
				eitEventInfo->start_time.wYear, eitEventInfo->start_time.wMonth, eitEventInfo->start_time.wDay, eitEventInfo->start_time.wHour, eitEventInfo->start_time.wMinute
				);
			continue;
		}

#ifdef DEBUG_EIT
		wsprintfA(g_szDebugEIT, "%c%04x.%02x%02x.%02d.%d\r\n", eit->table_id <= 0x4F ? 'P' : 'S',
			eitEventInfo->event_id, eit->table_id, eit->section_number, eit->version_number, siTag.time % 1000000);
#endif
		//[actual]と[other]は等価に扱う
		//[p/f]と[schedule]は各々完全に独立してデータベースを作成する
		if( eit->table_id <= 0x4F && eit->section_number <= 1 ){
			//[p/f]
			if( eit->section_number == 0 ){
				if( eitEventInfo->StartTimeFlag == FALSE ){
					OutputDebugString(L"Invalid EIT[p/f]\r\n");
				}else if( serviceInfo->nowEvent == NULL || siTag.time >= serviceInfo->nowEvent->time ){
					if( serviceInfo->nowEvent == NULL || serviceInfo->nowEvent->event_id != eitEventInfo->event_id ){
						//イベント入れ替わり
						serviceInfo->nowEvent.reset();
						if( serviceInfo->nextEvent && serviceInfo->nextEvent->event_id == eitEventInfo->event_id ){
							serviceInfo->nowEvent.swap(serviceInfo->nextEvent);
							serviceInfo->nowEvent->time = 0;
						}
					}
					if( serviceInfo->nowEvent == NULL ){
						eventInfo = new EVENT_INFO;
						serviceInfo->nowEvent.reset(eventInfo);
						eventInfo->event_id = eitEventInfo->event_id;
						eventInfo->time = 0;
						eventInfo->tagBasic.version = 0xFF;
						eventInfo->tagBasic.time = 0;
						eventInfo->tagExt.version = 0xFF;
						eventInfo->tagExt.time = 0;
					}else{
						eventInfo = serviceInfo->nowEvent.get();
					}
				}
			}else{
				if( serviceInfo->nextEvent == NULL || siTag.time >= serviceInfo->nextEvent->time ){
					if( serviceInfo->nextEvent == NULL || serviceInfo->nextEvent->event_id != eitEventInfo->event_id ){
						serviceInfo->nextEvent.reset();
						if( serviceInfo->nowEvent && serviceInfo->nowEvent->event_id == eitEventInfo->event_id ){
							serviceInfo->nextEvent.swap(serviceInfo->nowEvent);
							serviceInfo->nextEvent->time = 0;
						}
					}
					if( serviceInfo->nextEvent == NULL ){
						eventInfo = new EVENT_INFO;
						serviceInfo->nextEvent.reset(eventInfo);
						eventInfo->event_id = eitEventInfo->event_id;
						eventInfo->time = 0;
						eventInfo->tagBasic.version = 0xFF;
						eventInfo->tagBasic.time = 0;
						eventInfo->tagExt.version = 0xFF;
						eventInfo->tagExt.time = 0;
					}else{
						eventInfo = serviceInfo->nextEvent.get();
					}
				}
			}
		}else if( PID != 0x0012 || eit->table_id > 0x4F ){
			//[schedule]もしくは(H-EITでないとき)[p/f after]
			//TODO: イベント消滅には対応していない(クラス設計的に対応は厳しい)。EDCB的には実用上のデメリットはあまり無い
			if( eitEventInfo->StartTimeFlag == FALSE || eitEventInfo->DurationFlag == FALSE ){
				OutputDebugString(L"Invalid EIT[schedule]\r\n");
			}else{
				itrEvent = serviceInfo->eventMap.find(eitEventInfo->event_id);
				if( itrEvent == serviceInfo->eventMap.end() ){
					eventInfo = new EVENT_INFO;
					eventInfo->event_id = eitEventInfo->event_id;
					serviceInfo->eventMap[eventInfo->event_id].reset(eventInfo);
					eventInfo->time = 0;
					eventInfo->tagBasic.version = 0xFF;
					eventInfo->tagBasic.time = 0;
					eventInfo->tagExt.version = 0xFF;
					eventInfo->tagExt.time = 0;
				}else{
					eventInfo = itrEvent->second.get();
				}
			}
		}
		if( eventInfo ){
			//開始時間等はタイムスタンプのみを基準に更新
			if( siTag.time >= eventInfo->time ){
				eventInfo->StartTimeFlag = eitEventInfo->StartTimeFlag;
				eventInfo->start_time = eitEventInfo->start_time;
				eventInfo->DurationFlag = eitEventInfo->DurationFlag;
				eventInfo->durationSec = (eitEventInfo->durationHH * 60 + eitEventInfo->durationMM) * 60 + eitEventInfo->durationSS;
				eventInfo->freeCAFlag = eitEventInfo->free_CA_mode;
				eventInfo->time = siTag.time;
			}
			//記述子はテーブルバージョンも加味して更新(単に効率のため)
			if( siTag.time >= eventInfo->tagExt.time ){
				if( eit->version_number != eventInfo->tagExt.version ||
				    eit->table_id != eventInfo->tagExt.tableID ||
				    siTag.time > eventInfo->tagExt.time + 180 ){
					if( AddExtEvent(eventInfo, &eitEventInfo->descriptorList) != FALSE ){
						eventInfo->tagExt = siTag;
					}
				}else{
					eventInfo->tagExt.time = siTag.time;
				}
			}
			//[schedule extended]以外
			if( (eit->table_id < 0x58 || 0x5F < eit->table_id) && (eit->table_id < 0x68 || 0x6F < eit->table_id) ){
				if( eit->table_id > 0x4F && eventInfo->tagBasic.version != 0xFF && eventInfo->tagBasic.tableID <= 0x4F ){
					//[schedule][p/f after]とも運用するサービスがあれば[p/f after]を優先する(今のところサービス階層が分離しているのであり得ないはず)
					_OutputDebugString(L"Conflicts EIT[schedule][p/f after] SID:0x%04x EventID:0x%04x\r\n", eit->service_id, eventInfo->event_id);
				}else if( siTag.time >= eventInfo->tagBasic.time ){
					if( eit->version_number != eventInfo->tagBasic.version ||
					    eit->table_id != eventInfo->tagBasic.tableID ||
					    siTag.time > eventInfo->tagBasic.time + 180 ){
						AddBasicInfo(eventInfo, &eitEventInfo->descriptorList, eit->original_network_id, eit->transport_stream_id);
					}
					eventInfo->tagBasic = siTag;
				}
			}
		}
	}

	if( eit->original_network_id == 0x0003 ){
		return TRUE;
	}
	
	//セクションステータス
	if( PID != 0x0012 ){
		//L-EIT
		if( eit->table_id <= 0x4F ){
			if( serviceInfo->lastTableID != eit->table_id ||
			    serviceInfo->sectionList[0].version != eit->version_number + 1 ){
				serviceInfo->lastTableID = 0;
			}
			if( serviceInfo->lastTableID == 0 ){
				//リセット
				memset(&serviceInfo->sectionList.front(), 0, sizeof(SECTION_FLAG_INFO) * 8);
				for( int i = 1; i < 8; i++ ){
					//第0テーブル以外のセクションを無視
					memset(serviceInfo->sectionList[i].ignoreFlags, 0xFF, sizeof(serviceInfo->sectionList[0].ignoreFlags));
				}
				serviceInfo->lastTableID = eit->table_id;
			}
			//第0セグメント以外のセクションを無視
			memset(serviceInfo->sectionList[0].ignoreFlags + 1, 0xFF, sizeof(serviceInfo->sectionList[0].ignoreFlags) - 1);
			//第0セグメントの送られないセクションを無視
			for( int i = eit->segment_last_section_number % 8 + 1; i < 8; i++ ){
				serviceInfo->sectionList[0].ignoreFlags[0] |= 1 << i;
			}
			serviceInfo->sectionList[0].version = eit->version_number + 1;
			serviceInfo->sectionList[0].flags[0] |= 1 << (eit->section_number % 8);
		}

	}else{
		//H-EIT
		if( eit->table_id > 0x4F ){
			BYTE& lastTableID = eit->table_id % 16 >= 8 ? serviceInfo->lastTableIDExt : serviceInfo->lastTableID;
			vector<SECTION_FLAG_INFO>& sectionList = eit->table_id % 16 >= 8 ? serviceInfo->sectionExtList : serviceInfo->sectionList;
			if( sectionList.empty() ){
				//拡張情報はないことも多いので遅延割り当て
				sectionList.resize(8);
			}
			if( lastTableID != eit->last_table_id ){
				lastTableID = 0;
			}else if( sectionList[eit->table_id % 8].version != 0 &&
			          sectionList[eit->table_id % 8].version != eit->version_number + 1 ){
				OutputDebugString(L"EIT[schedule] updated\r\n");
				lastTableID = 0;
			}
			if( lastTableID == 0 ){
				//リセット
				memset(&sectionList.front(), 0, sizeof(SECTION_FLAG_INFO) * 8);
				for( int i = eit->last_table_id % 8 + 1; i < 8; i++ ){
					//送られないテーブルのセクションを無視
					memset(sectionList[i].ignoreFlags, 0xFF, sizeof(sectionList[0].ignoreFlags));
				}
				lastTableID = eit->last_table_id;
			}
			//送られないセグメントのセクションを無視
			memset(sectionList[eit->table_id % 8].ignoreFlags + eit->last_section_number / 8 + 1, 0xFF,
				sizeof(sectionList[0].ignoreFlags) - eit->last_section_number / 8 - 1);
			if( eit->table_id % 8 == 0 && streamTime > 0 ){
				//放送済みセグメントのセクションを無視
				memset(sectionList[0].ignoreFlags, 0xFF, streamTime / (3 * 60 * 60 * I64_1SEC) % 8);
			}
			//このセグメントの送られないセクションを無視
			for( int i = eit->segment_last_section_number % 8 + 1; i < 8; i++ ){
				sectionList[eit->table_id % 8].ignoreFlags[eit->section_number / 8] |= 1 << i;
			}
			sectionList[eit->table_id % 8].version = eit->version_number + 1;
			sectionList[eit->table_id % 8].flags[eit->section_number / 8] |= 1 << (eit->section_number % 8);
		}
	}

	return TRUE;
}

void CEpgDBUtil::AddBasicInfo(EVENT_INFO* eventInfo, const vector<AribDescriptor::CDescriptor>* descriptorList, WORD onid, WORD tsid)
{
	BOOL foundShort = FALSE;
	BOOL foundContent = FALSE;
	BOOL foundComponent = FALSE;
	BOOL foundGroup = FALSE;
	BOOL foundRelay = FALSE;
	for( size_t i=0; i<descriptorList->size(); i++ ){
		switch( (*descriptorList)[i].GetNumber(AribDescriptor::descriptor_tag) ){
		case AribDescriptor::short_event_descriptor:
			AddShortEvent(eventInfo, &(*descriptorList)[i]);
			foundShort = TRUE;
			break;
		case AribDescriptor::content_descriptor:
			AddContent(eventInfo, &(*descriptorList)[i]);
			foundContent = TRUE;
			break;
		case AribDescriptor::component_descriptor:
			AddComponent(eventInfo, &(*descriptorList)[i]);
			foundComponent = TRUE;
			break;
		case AribDescriptor::event_group_descriptor:
			if( (*descriptorList)[i].GetNumber(AribDescriptor::group_type) == 1 ){
				AddEventGroup(eventInfo, &(*descriptorList)[i], onid, tsid);
				foundGroup = TRUE;
			}else if( (*descriptorList)[i].GetNumber(AribDescriptor::group_type) == 2 ||
			          (*descriptorList)[i].GetNumber(AribDescriptor::group_type) == 4 ){
				AddEventRelay(eventInfo, &(*descriptorList)[i], onid, tsid);
				foundRelay = TRUE;
			}
			break;
		}
	}
	if( AddAudioComponent(eventInfo, descriptorList) == FALSE ){
		SAFE_DELETE(eventInfo->audioInfo);
	}
	if( foundShort == FALSE ){
		SAFE_DELETE(eventInfo->shortInfo);
	}
	if( foundContent == FALSE ){
		SAFE_DELETE(eventInfo->contentInfo);
	}
	if( foundComponent == FALSE ){
		SAFE_DELETE(eventInfo->componentInfo);
	}
	if( foundGroup == FALSE ){
		SAFE_DELETE(eventInfo->eventGroupInfo);
	}
	if( foundRelay == FALSE ){
		SAFE_DELETE(eventInfo->eventRelayInfo);
	}
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

void CEpgDBUtil::AddShortEvent(EVENT_INFO* eventInfo, const AribDescriptor::CDescriptor* shortEvent)
{
	if( eventInfo->shortInfo == NULL ){
		eventInfo->shortInfo = new EPG_SHORT_EVENT_INFO;
	}
	{
		CARIB8CharDecode arib;
		string event_name = "";
		string text_char = "";
		const char* src;
		DWORD srcSize;
		src = shortEvent->GetStringOrEmpty(AribDescriptor::event_name_char, &srcSize);
		arib.PSISI((const BYTE*)src, srcSize, &event_name);
		src = shortEvent->GetStringOrEmpty(AribDescriptor::text_char, &srcSize);
		arib.PSISI((const BYTE*)src, srcSize, &text_char);
#ifdef DEBUG_EIT
		text_char = g_szDebugEIT + text_char;
#endif

		eventInfo->shortInfo->event_nameLength = UpdateInfoText(eventInfo->shortInfo->event_name, event_name.c_str());
		eventInfo->shortInfo->text_charLength = UpdateInfoText(eventInfo->shortInfo->text_char, text_char.c_str());
	}
}

BOOL CEpgDBUtil::AddExtEvent(EVENT_INFO* eventInfo, const vector<AribDescriptor::CDescriptor>* descriptorList)
{
	{
		BOOL foundFlag = FALSE;
		CARIB8CharDecode arib;
		string extendText = "";
		string itemDescBuff = "";
		string itemBuff = "";
		//text_lengthは0で運用される
//		string textBuff = "";

		for( size_t i=0; i<descriptorList->size(); i++ ){
			if( (*descriptorList)[i].GetNumber(AribDescriptor::descriptor_tag) == AribDescriptor::extended_event_descriptor ){
				foundFlag = TRUE;
				const AribDescriptor::CDescriptor* extEvent = &(*descriptorList)[i];
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

		if( foundFlag == FALSE ){
			return FALSE;
		}
		if( eventInfo->extInfo == NULL ){
			eventInfo->extInfo = new EPG_EXTENDED_EVENT_INFO;
		}
#ifdef DEBUG_EIT
		extendText = g_szDebugEIT + extendText;
#endif
		eventInfo->extInfo->text_charLength = UpdateInfoText(eventInfo->extInfo->text_char, extendText.c_str());
	}

	return TRUE;
}

void CEpgDBUtil::AddContent(EVENT_INFO* eventInfo, const AribDescriptor::CDescriptor* content)
{
	if( eventInfo->contentInfo == NULL ){
		eventInfo->contentInfo = new EPG_CONTEN_INFO;
	}
	{
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
}

void CEpgDBUtil::AddComponent(EVENT_INFO* eventInfo, const AribDescriptor::CDescriptor* component)
{
	if( eventInfo->componentInfo == NULL ){
		eventInfo->componentInfo = new EPG_COMPONENT_INFO;
	}
	{
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
}

BOOL CEpgDBUtil::AddAudioComponent(EVENT_INFO* eventInfo, const vector<AribDescriptor::CDescriptor>* descriptorList)
{
	{
		WORD listSize = 0;
		for( size_t i=0; i<descriptorList->size(); i++ ){
			if( (*descriptorList)[i].GetNumber(AribDescriptor::descriptor_tag) == AribDescriptor::audio_component_descriptor ){
				listSize++;
			}
		}
		if( listSize == 0 ){
			return FALSE;
		}
		if( eventInfo->audioInfo == NULL ){
			eventInfo->audioInfo = new EPG_AUDIO_COMPONENT_INFO;
		}
		SAFE_DELETE_ARRAY(eventInfo->audioInfo->audioList);
		eventInfo->audioInfo->listSize = listSize;
		eventInfo->audioInfo->audioList = new EPG_AUDIO_COMPONENT_INFO_DATA[listSize];

		for( size_t i=0, j=0; j<eventInfo->audioInfo->listSize; i++ ){
			if( (*descriptorList)[i].GetNumber(AribDescriptor::descriptor_tag) == AribDescriptor::audio_component_descriptor ){
				const AribDescriptor::CDescriptor* audioComponent = &(*descriptorList)[i];
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

	return TRUE;
}

void CEpgDBUtil::AddEventGroup(EVENT_INFO* eventInfo, const AribDescriptor::CDescriptor* eventGroup, WORD onid, WORD tsid)
{
	if( eventInfo->eventGroupInfo == NULL ){
		eventInfo->eventGroupInfo = new EPG_EVENTGROUP_INFO;
	}
	{
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
				item.original_network_id = onid;
				item.transport_stream_id = tsid;

				eventInfo->eventGroupInfo->eventDataList[i] = item;
			}
		}
	}
}

void CEpgDBUtil::AddEventRelay(EVENT_INFO* eventInfo, const AribDescriptor::CDescriptor* eventGroup, WORD onid, WORD tsid)
{
	if( eventInfo->eventRelayInfo == NULL ){
		eventInfo->eventRelayInfo = new EPG_EVENTGROUP_INFO;
	}
	{
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
					item.original_network_id = onid;
					item.transport_stream_id = tsid;

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
}

void CEpgDBUtil::ClearSectionStatus()
{
	CBlockLock lock(&this->dbLock);

	map<ULONGLONG, SERVICE_EVENT_INFO>::iterator itr;
	for( itr = this->serviceEventMap.begin(); itr != this->serviceEventMap.end(); itr++ ){
		itr->second.lastTableID = 0;
		itr->second.lastTableIDExt = 0;
	}
}

BOOL CEpgDBUtil::CheckSectionAll(const vector<SECTION_FLAG_INFO>& sectionList)
{
	for( size_t i = 0; i < sectionList.size(); i++ ){
		for( int j = 0; j < sizeof(sectionList[0].flags); j++ ){
			if( (sectionList[i].flags[j] | sectionList[i].ignoreFlags[j]) != 0xFF ){
				return FALSE;
			}
		}
	}

	return TRUE;
}

EPG_SECTION_STATUS CEpgDBUtil::GetSectionStatusService(
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	BOOL l_eitFlag
	)
{
	CBlockLock lock(&this->dbLock);

	map<ULONGLONG, SERVICE_EVENT_INFO>::const_iterator itr =
		this->serviceEventMap.find(_Create64Key(originalNetworkID, transportStreamID, serviceID));
	if( itr != this->serviceEventMap.end() ){
		if( l_eitFlag ){
			//L-EITの状況
			if( itr->second.lastTableID != 0 && itr->second.lastTableID <= 0x4F ){
				return CheckSectionAll(itr->second.sectionList) ? EpgLEITAll : EpgNeedData;
			}
		}else{
			//H-EITの状況
			if( itr->second.lastTableID > 0x4F ){
				//拡張情報がない場合も「たまった」とみなす
				BOOL extFlag = itr->second.lastTableIDExt == 0 || CheckSectionAll(itr->second.sectionExtList);
				return CheckSectionAll(itr->second.sectionList) ? (extFlag ? EpgHEITAll : EpgBasicAll) : (extFlag ? EpgExtendAll : EpgNeedData);
			}
		}
	}
	return EpgNoData;
}

EPG_SECTION_STATUS CEpgDBUtil::GetSectionStatus(BOOL l_eitFlag)
{
	CBlockLock lock(&this->dbLock);

	EPG_SECTION_STATUS status = EpgNoData;

	map<ULONGLONG, SERVICE_EVENT_INFO>::iterator itr;
	for( itr = this->serviceEventMap.begin(); itr != this->serviceEventMap.end(); itr++ ){
		EPG_SECTION_STATUS s = GetSectionStatusService((WORD)(itr->first >> 32), (WORD)(itr->first >> 16), (WORD)itr->first, l_eitFlag);
		if( s != EpgNoData ){
			if( status == EpgNoData ){
				status = l_eitFlag ? EpgLEITAll : EpgHEITAll;
			}
			if( l_eitFlag ){
				if( s == EpgNeedData ){
					status = EpgNeedData;
					break;
				}
			}else{
				//サービスリストあるなら映像サービスのみ対象
				map<ULONGLONG, BYTE>::iterator itrType;
				itrType = this->serviceList.find(itr->first);
				if( itrType != this->serviceList.end() ){
					if( itrType->second != 0x01 && itrType->second != 0xA5 ){
						continue;
					}
				}
				if( s == EpgNeedData || s == EpgExtendAll && status == EpgBasicAll || s == EpgBasicAll && status == EpgExtendAll ){
					status = EpgNeedData;
					break;
				}
				if( status == EpgHEITAll ){
					status = s;
				}
			}
		}
	}

	return status;
}

BOOL CEpgDBUtil::AddServiceList(const CNITTable* nit)
{
	if( nit == NULL ){
		return FALSE;
	}
	CBlockLock lock(&this->dbLock);

	wstring network_nameW = L"";

	for( size_t i=0; i<nit->descriptorList.size(); i++ ){
		if( nit->descriptorList[i].GetNumber(AribDescriptor::descriptor_tag) == AribDescriptor::network_name_descriptor ){
			const AribDescriptor::CDescriptor* networkName = &nit->descriptorList[i];
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
		const CNITTable::TS_INFO_DATA* tsInfo = &nit->TSInfoList[i];
		//サービス情報更新用
		map<DWORD, DB_TS_INFO>::iterator itrFind;
		itrFind = this->serviceInfoList.find((DWORD)tsInfo->original_network_id << 16 | tsInfo->transport_stream_id);
		if( itrFind != this->serviceInfoList.end() ){
			itrFind->second.network_name = network_nameW;
		}

		for( size_t j=0; j<tsInfo->descriptorList.size(); j++ ){
			const AribDescriptor::CDescriptor* desc = &tsInfo->descriptorList[j];
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

BOOL CEpgDBUtil::AddServiceList(WORD TSID, const CSITTable* sit)
{
	CBlockLock lock(&this->dbLock);

	WORD ONID = 0xFFFF;
	for( size_t i=0; i<sit->descriptorList.size(); i++ ){
		if( sit->descriptorList[i].GetNumber(AribDescriptor::descriptor_tag) == AribDescriptor::network_identification_descriptor ){
			ONID = (WORD)sit->descriptorList[i].GetNumber(AribDescriptor::network_id);
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
			item.service_id = sit->serviceLoopList[i].service_id;

			for( size_t j=0; j<sit->serviceLoopList[i].descriptorList.size(); j++ ){
				if( sit->serviceLoopList[i].descriptorList[j].GetNumber(AribDescriptor::descriptor_tag) == AribDescriptor::service_descriptor ){
					const AribDescriptor::CDescriptor* service = &sit->serviceLoopList[i].descriptorList[j];
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

BOOL CEpgDBUtil::AddSDT(const CSDTTable* sdt)
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
			item.service_id = sdt->serviceInfoList[i].service_id;

			for( size_t j=0; j<sdt->serviceInfoList[i].descriptorList.size(); j++ ){
				if( sdt->serviceInfoList[i].descriptorList[j].GetNumber(AribDescriptor::descriptor_tag) == AribDescriptor::service_descriptor ){
					const AribDescriptor::CDescriptor* service = &sdt->serviceInfoList[i].descriptorList[j];
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
			itrS = itrTS->second.serviceList.find(sdt->serviceInfoList[i].service_id);
			if( itrS == itrTS->second.serviceList.end()){
				DB_SERVICE_INFO item;
				item.original_network_id = sdt->original_network_id;
				item.transport_stream_id = sdt->transport_stream_id;
				item.service_id = sdt->serviceInfoList[i].service_id;

				for( size_t j=0; j<sdt->serviceInfoList[i].descriptorList.size(); j++ ){
					if( sdt->serviceInfoList[i].descriptorList[j].GetNumber(AribDescriptor::descriptor_tag) == AribDescriptor::service_descriptor ){
						const AribDescriptor::CDescriptor* service = &sdt->serviceInfoList[i].descriptorList[j];
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
	EPG_EVENT_INFO** epgInfoList_
	)
{
	CBlockLock lock(&this->dbLock);

	ULONGLONG key = _Create64Key(originalNetworkID, transportStreamID, serviceID);

	map<ULONGLONG, SERVICE_EVENT_INFO>::iterator itr;
	itr = serviceEventMap.find(key);
	if( itr == serviceEventMap.end() ){
		return FALSE;
	}

	EVENT_INFO* evtPF[2] = {itr->second.nowEvent.get(), itr->second.nextEvent.get()};
	if( evtPF[0] == NULL || evtPF[1] && evtPF[0]->event_id > evtPF[1]->event_id ){
		std::swap(evtPF[0], evtPF[1]);
	}
	size_t listSize = itr->second.eventMap.size() +
		(evtPF[0] && itr->second.eventMap.count(evtPF[0]->event_id) == 0 ? 1 : 0) +
		(evtPF[1] && itr->second.eventMap.count(evtPF[1]->event_id) == 0 ? 1 : 0);
	if( listSize == 0 ){
		return FALSE;
	}
	*epgInfoListSize = (DWORD)listSize;
	this->epgInfoList.reset(new EPG_EVENT_INFO[*epgInfoListSize]);

	map<WORD, std::unique_ptr<EVENT_INFO>>::iterator itrEvt = itr->second.eventMap.begin();
	DWORD count = 0;
	while( evtPF[0] || itrEvt != itr->second.eventMap.end() ){
		EPG_EXTENDED_EVENT_INFO* extInfoSchedule = NULL;
		EVENT_INFO* evt;
		if( itrEvt == itr->second.eventMap.end() || evtPF[0] && evtPF[0]->event_id < itrEvt->first ){
			//[p/f]を出力
			evt = evtPF[0];
			evtPF[0] = evtPF[1];
			evtPF[1] = NULL;
		}else{
			if( evtPF[0] && evtPF[0]->event_id == itrEvt->first ){
				//両方あるときは[p/f]を優先
				evt = evtPF[0];
				evtPF[0] = evtPF[1];
				evtPF[1] = NULL;
				if( evt->extInfo == NULL && itrEvt->second->extInfo ){
					extInfoSchedule = new EPG_EXTENDED_EVENT_INFO;
					extInfoSchedule->DeepCopy(*itrEvt->second->extInfo);
				}
			}else{
				//[schedule]を出力
				evt = itrEvt->second.get();
			}
			itrEvt++;
		}
		CopyEpgInfo(this->epgInfoList.get()+count, evt);
		if( extInfoSchedule ){
			this->epgInfoList[count].extInfo = extInfoSchedule;
		}
		count++;
	}

	*epgInfoList_ = this->epgInfoList.get();

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
	if( itr == this->serviceEventMap.end() ){
		return FALSE;
	}
	const EVENT_INFO* evtPF[2] = {itr->second.nowEvent.get(), itr->second.nextEvent.get()};
	if( evtPF[0] == NULL || evtPF[1] && evtPF[0]->event_id > evtPF[1]->event_id ){
		std::swap(evtPF[0], evtPF[1]);
	}
	size_t listSize = itr->second.eventMap.size() +
		(evtPF[0] && itr->second.eventMap.count(evtPF[0]->event_id) == 0 ? 1 : 0) +
		(evtPF[1] && itr->second.eventMap.count(evtPF[1]->event_id) == 0 ? 1 : 0);
	if( listSize == 0 ){
		return FALSE;
	}
	if( enumEpgInfoListProc((DWORD)listSize, NULL, param) == FALSE ){
		return TRUE;
	}

	BYTE info[__alignof(EPG_EVENT_INFO) + sizeof(EPG_EVENT_INFO) * 32];

	map<WORD, std::unique_ptr<EVENT_INFO>>::iterator itrEvt = itr->second.eventMap.begin();
	DWORD count = 0;
	while( evtPF[0] || itrEvt != itr->second.eventMap.end() ){
		//デストラクタを呼ばないよう領域だけ割り当て(POD構造体だけなので無問題)、マスターを直接参照して構築する
		EPG_EVENT_INFO* item = AlignCeil<EPG_EVENT_INFO>(info) + count;
		EPG_EXTENDED_EVENT_INFO* extInfoSchedule = NULL;
		const EVENT_INFO* evt;
		if( itrEvt == itr->second.eventMap.end() || evtPF[0] && evtPF[0]->event_id < itrEvt->first ){
			//[p/f]を出力
			evt = evtPF[0];
			evtPF[0] = evtPF[1];
			evtPF[1] = NULL;
		}else{
			if( evtPF[0] && evtPF[0]->event_id == itrEvt->first ){
				//両方あるときは[p/f]を優先
				evt = evtPF[0];
				evtPF[0] = evtPF[1];
				evtPF[1] = NULL;
				if( evt->extInfo == NULL ){
					extInfoSchedule = itrEvt->second->extInfo;
				}
			}else{
				//[schedule]を出力
				evt = itrEvt->second.get();
			}
			itrEvt++;
		}
		memcpy(item, static_cast<const EPG_EVENT_INFO*>(evt), sizeof(EPG_EVENT_INFO));
		if( extInfoSchedule ){
			item->extInfo = extInfoSchedule;
		}
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
	SERVICE_INFO** serviceList_
	)
{
	CBlockLock lock(&this->dbLock);

	*serviceListSize = (DWORD)this->serviceEventMap.size();
	this->serviceDBList.reset(new SERVICE_INFO[*serviceListSize]);

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

	*serviceList_ = this->serviceDBList.get();
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
	EPG_EVENT_INFO** epgInfo_
	)
{
	CBlockLock lock(&this->dbLock);

	this->epgInfo.reset();

	ULONGLONG key = _Create64Key(originalNetworkID, transportStreamID, serviceID);

	map<ULONGLONG, SERVICE_EVENT_INFO>::iterator itr;
	itr = serviceEventMap.find(key);
	if( itr == serviceEventMap.end() ){
		return FALSE;
	}

	if( itr->second.nowEvent != NULL && nextFlag == FALSE ){
		this->epgInfo.reset(new EPG_EVENT_INFO);
		CopyEpgInfo(this->epgInfo.get(), itr->second.nowEvent.get());
		*epgInfo_ = this->epgInfo.get();
	}else if( itr->second.nextEvent != NULL && nextFlag == TRUE ){
		this->epgInfo.reset(new EPG_EVENT_INFO);
		CopyEpgInfo(this->epgInfo.get(), itr->second.nextEvent.get());
		*epgInfo_ = this->epgInfo.get();
	}
	if( this->epgInfo != NULL ){
		if( (*epgInfo_)->extInfo == NULL && itr->second.eventMap.count((*epgInfo_)->event_id) && itr->second.eventMap[(*epgInfo_)->event_id]->extInfo ){
			(*epgInfo_)->extInfo = new EPG_EXTENDED_EVENT_INFO;
			(*epgInfo_)->extInfo->DeepCopy(*itr->second.eventMap[(*epgInfo_)->event_id]->extInfo);
		}
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
	EPG_EVENT_INFO** epgInfo_
	)
{
	CBlockLock lock(&this->dbLock);

	this->searchEpgInfo.reset();

	ULONGLONG key = _Create64Key(originalNetworkID, transportStreamID, serviceID);

	map<ULONGLONG, SERVICE_EVENT_INFO>::iterator itr;
	itr = serviceEventMap.find(key);
	if( itr == serviceEventMap.end() ){
		return FALSE;
	}

	if( itr->second.nowEvent != NULL && itr->second.nowEvent->event_id == eventID ){
		this->searchEpgInfo.reset(new EPG_EVENT_INFO);
		CopyEpgInfo(this->searchEpgInfo.get(), itr->second.nowEvent.get());
		*epgInfo_ = this->searchEpgInfo.get();
	}else if( itr->second.nextEvent != NULL && itr->second.nextEvent->event_id == eventID ){
		this->searchEpgInfo.reset(new EPG_EVENT_INFO);
		CopyEpgInfo(this->searchEpgInfo.get(), itr->second.nextEvent.get());
		*epgInfo_ = this->searchEpgInfo.get();
	}
	if( this->searchEpgInfo != NULL ){
		if( (*epgInfo_)->extInfo == NULL && itr->second.eventMap.count(eventID) && itr->second.eventMap[eventID]->extInfo ){
			(*epgInfo_)->extInfo = new EPG_EXTENDED_EVENT_INFO;
			(*epgInfo_)->extInfo->DeepCopy(*itr->second.eventMap[eventID]->extInfo);
		}
		return TRUE;
	}
	if( pfOnlyFlag == 0 ){
		map<WORD, std::unique_ptr<EVENT_INFO>>::iterator itrEvent;
		itrEvent = itr->second.eventMap.find(eventID);
		if( itrEvent != itr->second.eventMap.end() ){
			this->searchEpgInfo.reset(new EPG_EVENT_INFO);
			CopyEpgInfo(this->searchEpgInfo.get(), itrEvent->second.get());
			*epgInfo_ = this->searchEpgInfo.get();
			return TRUE;
		}
	}

	return FALSE;
}
