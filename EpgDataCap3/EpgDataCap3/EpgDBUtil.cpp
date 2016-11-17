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

namespace Desc = AribDescriptor;

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
	//������[p/f]�̃��Z�b�g�͂��Ȃ�
}

BOOL CEpgDBUtil::AddEIT(WORD PID, const Desc::CDescriptor& eit, __int64 streamTime)
{
	CBlockLock lock(&this->dbLock);

	WORD original_network_id = (WORD)eit.GetNumber(Desc::original_network_id);
	WORD transport_stream_id = (WORD)eit.GetNumber(Desc::transport_stream_id);
	WORD service_id = (WORD)eit.GetNumber(Desc::service_id);
	ULONGLONG key = _Create64Key(original_network_id, transport_stream_id, service_id);

	//�T�[�r�X��map���擾
	map<ULONGLONG, SERVICE_EVENT_INFO>::iterator itr;
	SERVICE_EVENT_INFO* serviceInfo = NULL;

	itr = serviceEventMap.find(key);
	if( itr == serviceEventMap.end() ){
		serviceInfo = &serviceEventMap.insert(std::make_pair(key, SERVICE_EVENT_INFO())).first->second;
	}else{
		serviceInfo = &itr->second;
	}

	BYTE table_id = (BYTE)eit.GetNumber(Desc::table_id);
	BYTE version_number = (BYTE)eit.GetNumber(Desc::version_number);
	BYTE section_number = (BYTE)eit.GetNumber(Desc::section_number);
	SI_TAG siTag;
	siTag.tableID = table_id;
	siTag.version = version_number;
	siTag.time = (DWORD)(streamTime / (10 * I64_1SEC));

	DWORD eventLoopSize = 0;
	Desc::CDescriptor::CLoopPointer lp;
	if( eit.EnterLoop(lp) ){
		eventLoopSize = eit.GetLoopSize(lp);
	}
	if( table_id <= 0x4F && section_number <= 1 ){
		//[p/f]
		if( siTag.time == 0 ){
			//�`�����l���ύX���̉������̂��߁A�^�C���X�^���v�s����[p/f]��������DB��̕s���łȂ�[p/f]���N���A����
			//EPG�t�@�C������͂���Ƃ��͌Â�[p/f]�ɂ��㏑������������̂ŁA���p���Ŏ��n��ɂ��邩�^�C���X�^���v���m�肳����H�v���K�v
			if( serviceInfo->nowEvent && serviceInfo->nowEvent->time != 0 ||
			    serviceInfo->nextEvent && serviceInfo->nextEvent->time != 0 ){
				serviceInfo->nowEvent.reset();
				serviceInfo->nextEvent.reset();
			}
		}
		if( eventLoopSize == 0 ){
			//��Z�N�V����
			if( section_number == 0 ){
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
	//�C�x���g���ƂɍX�V�K�v������
	for( DWORD i = 0; i < eventLoopSize; i++ ){
		eit.SetLoopIndex(lp, i);
		WORD event_id = (WORD)eit.GetNumber(Desc::event_id, lp);
		SYSTEMTIME start_time = {};
		DWORD mjd = eit.GetNumber(Desc::start_time_mjd, lp);
		DWORD bcd = eit.GetNumber(Desc::start_time_bcd, lp);
		if( mjd != 0xFFFF || bcd != 0xFFFFFF ){
			_MJDtoSYSTEMTIME(mjd, &start_time);
			start_time.wHour = (bcd >> 20 & 15) * 10 + (bcd >> 16 & 15);
			start_time.wMinute = (bcd >> 12 & 15) * 10 + (bcd >> 8 & 15);
			start_time.wSecond = (bcd >> 4 & 15) * 10 + (bcd & 15);
		}
		DWORD duration = eit.GetNumber(Desc::d_duration, lp);
		if( duration != 0xFFFFFF ){
			duration = (duration >> 20 & 15) * 36000 + (duration >> 16 & 15) * 3600 + (duration >> 12 & 15) * 600 +
			           (duration >> 8 & 15) * 60 + (duration >> 4 & 15) * 10 + (duration & 15);
		}
		map<WORD, std::unique_ptr<EVENT_INFO>>::iterator itrEvent;
		EVENT_INFO* eventInfo = NULL;

		if( eit.GetNumber(Desc::running_status, lp) == 1 || eit.GetNumber(Desc::running_status, lp) == 3 ){
			//����s���܂��͒�~��
			_OutputDebugString(L"������s���܂��͒�~���C�x���g ONID:0x%04x TSID:0x%04x SID:0x%04x EventID:0x%04x %04d/%02d/%02d %02d:%02d",
				original_network_id,  transport_stream_id, service_id, event_id,
				start_time.wYear, start_time.wMonth, start_time.wDay, start_time.wHour, start_time.wMinute
				);
			continue;
		}

#ifdef DEBUG_EIT
		wsprintfA(g_szDebugEIT, "%c%04x.%02x%02x.%02d.%d\r\n", table_id <= 0x4F ? 'P' : 'S',
			eitEventInfo->event_id, table_id, section_number, version_number, siTag.time % 1000000);
#endif
		//[actual]��[other]�͓����Ɉ���
		//[p/f]��[schedule]�͊e�X���S�ɓƗ����ăf�[�^�x�[�X���쐬����
		if( table_id <= 0x4F && section_number <= 1 ){
			//[p/f]
			if( section_number == 0 ){
				if( start_time.wYear == 0 ){
					OutputDebugString(L"Invalid EIT[p/f]\r\n");
				}else if( serviceInfo->nowEvent == NULL || siTag.time >= serviceInfo->nowEvent->time ){
					if( serviceInfo->nowEvent == NULL || serviceInfo->nowEvent->event_id != event_id ){
						//�C�x���g����ւ��
						serviceInfo->nowEvent.reset();
						if( serviceInfo->nextEvent && serviceInfo->nextEvent->event_id == event_id ){
							serviceInfo->nowEvent.swap(serviceInfo->nextEvent);
							serviceInfo->nowEvent->time = 0;
						}
					}
					if( serviceInfo->nowEvent == NULL ){
						eventInfo = new EVENT_INFO;
						serviceInfo->nowEvent.reset(eventInfo);
						eventInfo->event_id = event_id;
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
					if( serviceInfo->nextEvent == NULL || serviceInfo->nextEvent->event_id != event_id ){
						serviceInfo->nextEvent.reset();
						if( serviceInfo->nowEvent && serviceInfo->nowEvent->event_id == event_id ){
							serviceInfo->nextEvent.swap(serviceInfo->nowEvent);
							serviceInfo->nextEvent->time = 0;
						}
					}
					if( serviceInfo->nextEvent == NULL ){
						eventInfo = new EVENT_INFO;
						serviceInfo->nextEvent.reset(eventInfo);
						eventInfo->event_id = event_id;
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
		}else if( PID != 0x0012 || table_id > 0x4F ){
			//[schedule]��������(H-EIT�łȂ��Ƃ�)[p/f after]
			//TODO: �C�x���g���łɂ͑Ή����Ă��Ȃ�(�N���X�݌v�I�ɑΉ��͌�����)�BEDCB�I�ɂ͎��p��̃f�����b�g�͂��܂薳��
			if( start_time.wYear == 0 || duration == 0xFFFFFF ){
				OutputDebugString(L"Invalid EIT[schedule]\r\n");
			}else{
				itrEvent = serviceInfo->eventMap.find(event_id);
				if( itrEvent == serviceInfo->eventMap.end() ){
					eventInfo = new EVENT_INFO;
					eventInfo->event_id = event_id;
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
			//�J�n���ԓ��̓^�C���X�^���v�݂̂���ɍX�V
			if( siTag.time >= eventInfo->time ){
				eventInfo->StartTimeFlag = start_time.wYear != 0;
				eventInfo->start_time = start_time;
				eventInfo->DurationFlag = duration != 0xFFFFFF;
				eventInfo->durationSec = duration != 0xFFFFFF ? duration : 0;
				eventInfo->freeCAFlag = eit.GetNumber(Desc::free_CA_mode, lp) != 0;
				eventInfo->time = siTag.time;
			}
			//�L�q�q�̓e�[�u���o�[�W�������������čX�V(�P�Ɍ����̂���)
			if( siTag.time >= eventInfo->tagExt.time ){
				if( version_number != eventInfo->tagExt.version ||
				    table_id != eventInfo->tagExt.tableID ||
				    siTag.time > eventInfo->tagExt.time + 180 ){
					if( AddExtEvent(eventInfo, eit, lp) != FALSE ){
						eventInfo->tagExt = siTag;
					}
				}else{
					eventInfo->tagExt.time = siTag.time;
				}
			}
			//[schedule extended]�ȊO
			if( (table_id < 0x58 || 0x5F < table_id) && (table_id < 0x68 || 0x6F < table_id) ){
				if( table_id > 0x4F && eventInfo->tagBasic.version != 0xFF && eventInfo->tagBasic.tableID <= 0x4F ){
					//[schedule][p/f after]�Ƃ��^�p����T�[�r�X�������[p/f after]��D�悷��(���̂Ƃ���T�[�r�X�K�w���������Ă���̂ł��蓾�Ȃ��͂�)
					_OutputDebugString(L"Conflicts EIT[schedule][p/f after] SID:0x%04x EventID:0x%04x\r\n", service_id, eventInfo->event_id);
				}else if( siTag.time >= eventInfo->tagBasic.time ){
					if( version_number != eventInfo->tagBasic.version ||
					    table_id != eventInfo->tagBasic.tableID ||
					    siTag.time > eventInfo->tagBasic.time + 180 ){
						AddBasicInfo(eventInfo, eit, lp, original_network_id, transport_stream_id);
					}
					eventInfo->tagBasic = siTag;
				}
			}
		}
	}

	//�Z�N�V�����X�e�[�^�X
	if( PID != 0x0012 ){
		//L-EIT
		if( table_id <= 0x4F ){
			if( serviceInfo->lastTableID != table_id ||
			    serviceInfo->sectionList[0].version != version_number + 1 ){
				serviceInfo->lastTableID = 0;
			}
			if( serviceInfo->lastTableID == 0 ){
				//���Z�b�g
				memset(&serviceInfo->sectionList.front(), 0, sizeof(SECTION_FLAG_INFO) * 8);
				for( int i = 1; i < 8; i++ ){
					//��0�e�[�u���ȊO�̃Z�N�V�����𖳎�
					memset(serviceInfo->sectionList[i].ignoreFlags, 0xFF, sizeof(serviceInfo->sectionList[0].ignoreFlags));
				}
				serviceInfo->lastTableID = table_id;
			}
			//��0�Z�O�����g�ȊO�̃Z�N�V�����𖳎�
			memset(serviceInfo->sectionList[0].ignoreFlags + 1, 0xFF, sizeof(serviceInfo->sectionList[0].ignoreFlags) - 1);
			//��0�Z�O�����g�̑����Ȃ��Z�N�V�����𖳎�
			for( int i = eit.GetNumber(Desc::segment_last_section_number) % 8 + 1; i < 8; i++ ){
				serviceInfo->sectionList[0].ignoreFlags[0] |= 1 << i;
			}
			serviceInfo->sectionList[0].version = version_number + 1;
			serviceInfo->sectionList[0].flags[0] |= 1 << (section_number % 8);
		}

	}else{
		//H-EIT
		if( table_id > 0x4F ){
			BYTE& lastTableID = table_id % 16 >= 8 ? serviceInfo->lastTableIDExt : serviceInfo->lastTableID;
			vector<SECTION_FLAG_INFO>& sectionList = table_id % 16 >= 8 ? serviceInfo->sectionExtList : serviceInfo->sectionList;
			if( sectionList.empty() ){
				//�g�����͂Ȃ����Ƃ������̂Œx�����蓖��
				sectionList.resize(8);
			}
			if( lastTableID != eit.GetNumber(Desc::last_table_id) ){
				lastTableID = 0;
			}else if( sectionList[table_id % 8].version != 0 &&
			          sectionList[table_id % 8].version != version_number + 1 ){
				OutputDebugString(L"EIT[schedule] updated\r\n");
				lastTableID = 0;
			}
			if( lastTableID == 0 ){
				//���Z�b�g
				memset(&sectionList.front(), 0, sizeof(SECTION_FLAG_INFO) * 8);
				for( int i = eit.GetNumber(Desc::last_table_id) % 8 + 1; i < 8; i++ ){
					//�����Ȃ��e�[�u���̃Z�N�V�����𖳎�
					memset(sectionList[i].ignoreFlags, 0xFF, sizeof(sectionList[0].ignoreFlags));
				}
				lastTableID = (BYTE)eit.GetNumber(Desc::last_table_id);
			}
			//�����Ȃ��Z�O�����g�̃Z�N�V�����𖳎�
			memset(sectionList[table_id % 8].ignoreFlags + (BYTE)eit.GetNumber(Desc::last_section_number) / 8 + 1, 0xFF,
				sizeof(sectionList[0].ignoreFlags) - (BYTE)eit.GetNumber(Desc::last_section_number) / 8 - 1);
			if( table_id % 8 == 0 && streamTime > 0 ){
				//�����ς݃Z�O�����g�̃Z�N�V�����𖳎�
				memset(sectionList[0].ignoreFlags, 0xFF, streamTime / (3 * 60 * 60 * I64_1SEC) % 8);
			}
			//���̃Z�O�����g�̑����Ȃ��Z�N�V�����𖳎�
			for( int i = eit.GetNumber(Desc::segment_last_section_number) % 8 + 1; i < 8; i++ ){
				sectionList[table_id % 8].ignoreFlags[section_number / 8] |= 1 << i;
			}
			sectionList[table_id % 8].version = version_number + 1;
			sectionList[table_id % 8].flags[section_number / 8] |= 1 << (section_number % 8);
		}
	}

	return TRUE;
}

void CEpgDBUtil::AddBasicInfo(EVENT_INFO* eventInfo, const Desc::CDescriptor& eit, Desc::CDescriptor::CLoopPointer lpParent, WORD onid, WORD tsid)
{
	BOOL foundShort = FALSE;
	BOOL foundContent = FALSE;
	BOOL foundComponent = FALSE;
	BOOL foundGroup = FALSE;
	BOOL foundRelay = FALSE;
	Desc::CDescriptor::CLoopPointer lp = lpParent;
	if( eit.EnterLoop(lp) ){
		for( DWORD i = 0; eit.SetLoopIndex(lp, i); i++ ){
			switch( eit.GetNumber(Desc::descriptor_tag, lp) ){
			case Desc::short_event_descriptor:
				AddShortEvent(eventInfo, eit, lp);
				foundShort = TRUE;
				break;
			case Desc::content_descriptor:
				AddContent(eventInfo, eit, lp);
				foundContent = TRUE;
				break;
			case Desc::component_descriptor:
				AddComponent(eventInfo, eit, lp);
				foundComponent = TRUE;
				break;
			case Desc::event_group_descriptor:
				if( eit.GetNumber(Desc::group_type, lp) == 1 ){
					AddEventGroup(eventInfo, eit, lp, onid, tsid);
					foundGroup = TRUE;
				}else if( eit.GetNumber(Desc::group_type, lp) == 2 || eit.GetNumber(Desc::group_type, lp) == 4 ){
					AddEventRelay(eventInfo, eit, lp, onid, tsid);
					foundRelay = TRUE;
				}
				break;
			}
		}
	}
	if( AddAudioComponent(eventInfo, eit, lpParent) == FALSE ){
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
	//�d�l�����m�łȂ����p����NULL�`�F�b�N���ȗ�����Ă��邽��
	strOut = new WCHAR[1];
	strOut[0] = L'\0';
	return 0;
}

void CEpgDBUtil::AddShortEvent(EVENT_INFO* eventInfo, const Desc::CDescriptor& eit, Desc::CDescriptor::CLoopPointer lp)
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
		src = eit.GetStringOrEmpty(Desc::event_name_char, &srcSize, lp);
		arib.PSISI((const BYTE*)src, srcSize, &event_name);
		src = eit.GetStringOrEmpty(Desc::text_char, &srcSize, lp);
		arib.PSISI((const BYTE*)src, srcSize, &text_char);
#ifdef DEBUG_EIT
		text_char = g_szDebugEIT + text_char;
#endif

		eventInfo->shortInfo->event_nameLength = UpdateInfoText(eventInfo->shortInfo->event_name, event_name.c_str());
		eventInfo->shortInfo->text_charLength = UpdateInfoText(eventInfo->shortInfo->text_char, text_char.c_str());
	}
}

BOOL CEpgDBUtil::AddExtEvent(EVENT_INFO* eventInfo, const Desc::CDescriptor& eit, Desc::CDescriptor::CLoopPointer lpParent)
{
	{
		BOOL foundFlag = FALSE;
		CARIB8CharDecode arib;
		string extendText = "";
		string itemDescBuff = "";
		string itemBuff = "";
		//text_length��0�ŉ^�p�����
//		string textBuff = "";

		Desc::CDescriptor::CLoopPointer lp = lpParent;
		if( eit.EnterLoop(lp) ){
			for( DWORD i = 0; eit.SetLoopIndex(lp, i); i++ ){
				if( eit.GetNumber(Desc::descriptor_tag, lp) != Desc::extended_event_descriptor ){
					continue;
				}
				foundFlag = TRUE;
				Desc::CDescriptor::CLoopPointer lp2 = lp;
				if( eit.EnterLoop(lp2) ){
					for( DWORD j=0; eit.SetLoopIndex(lp2, j); j++ ){
						const char* src;
						DWORD srcSize;
						src = eit.GetStringOrEmpty(Desc::item_description_char, &srcSize, lp2);
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
						src = eit.GetStringOrEmpty(Desc::item_char, &srcSize, lp2);
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

void CEpgDBUtil::AddContent(EVENT_INFO* eventInfo, const Desc::CDescriptor& eit, Desc::CDescriptor::CLoopPointer lp)
{
	if( eventInfo->contentInfo == NULL ){
		eventInfo->contentInfo = new EPG_CONTEN_INFO;
	}
	{
		eventInfo->contentInfo->listSize = 0;
		SAFE_DELETE_ARRAY(eventInfo->contentInfo->nibbleList);
		if( eit.EnterLoop(lp) ){
			eventInfo->contentInfo->listSize = (WORD)eit.GetLoopSize(lp);
			eventInfo->contentInfo->nibbleList = new EPG_CONTENT[eventInfo->contentInfo->listSize];
			for( DWORD i=0; eit.SetLoopIndex(lp, i); i++ ){
				EPG_CONTENT nibble;
				nibble.content_nibble_level_1 = (BYTE)eit.GetNumber(Desc::content_nibble_level_1, lp);
				nibble.content_nibble_level_2 = (BYTE)eit.GetNumber(Desc::content_nibble_level_2, lp);
				nibble.user_nibble_1 = (BYTE)eit.GetNumber(Desc::user_nibble_1, lp);
				nibble.user_nibble_2 = (BYTE)eit.GetNumber(Desc::user_nibble_2, lp);
				eventInfo->contentInfo->nibbleList[i] = nibble;
			}
		}
	}
}

void CEpgDBUtil::AddComponent(EVENT_INFO* eventInfo, const Desc::CDescriptor& eit, Desc::CDescriptor::CLoopPointer lp)
{
	if( eventInfo->componentInfo == NULL ){
		eventInfo->componentInfo = new EPG_COMPONENT_INFO;
	}
	{
		eventInfo->componentInfo->stream_content = (BYTE)eit.GetNumber(Desc::stream_content, lp);
		eventInfo->componentInfo->component_type = (BYTE)eit.GetNumber(Desc::component_type, lp);
		eventInfo->componentInfo->component_tag = (BYTE)eit.GetNumber(Desc::component_tag, lp);

		CARIB8CharDecode arib;
		string text_char = "";
		DWORD srcSize;
		const char* src = eit.GetStringOrEmpty(Desc::text_char, &srcSize, lp);
		arib.PSISI((const BYTE*)src, srcSize, &text_char);
		eventInfo->componentInfo->text_charLength = UpdateInfoText(eventInfo->componentInfo->text_char, text_char.c_str());

	}
}

BOOL CEpgDBUtil::AddAudioComponent(EVENT_INFO* eventInfo, const Desc::CDescriptor& eit, Desc::CDescriptor::CLoopPointer lpParent)
{
	{
		WORD listSize = 0;
		Desc::CDescriptor::CLoopPointer lp = lpParent;
		if( eit.EnterLoop(lp) ){
			for( DWORD i = 0; eit.SetLoopIndex(lp, i); i++ ){
				if( eit.GetNumber(Desc::descriptor_tag, lp) == Desc::audio_component_descriptor ){
					listSize++;
				}
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

		for( WORD i=0, j=0; j<eventInfo->audioInfo->listSize; i++ ){
			eit.SetLoopIndex(lp, i);
			if( eit.GetNumber(Desc::descriptor_tag, lp) == Desc::audio_component_descriptor ){
				EPG_AUDIO_COMPONENT_INFO_DATA& item = eventInfo->audioInfo->audioList[j++];

				item.stream_content = (BYTE)eit.GetNumber(Desc::stream_content, lp);
				item.component_type = (BYTE)eit.GetNumber(Desc::component_type, lp);
				item.component_tag = (BYTE)eit.GetNumber(Desc::component_tag, lp);

				item.stream_type = (BYTE)eit.GetNumber(Desc::stream_type, lp);
				item.simulcast_group_tag = (BYTE)eit.GetNumber(Desc::simulcast_group_tag, lp);
				item.ES_multi_lingual_flag = (BYTE)eit.GetNumber(Desc::ES_multi_lingual_flag, lp);
				item.main_component_flag = (BYTE)eit.GetNumber(Desc::main_component_flag, lp);
				item.quality_indicator = (BYTE)eit.GetNumber(Desc::quality_indicator, lp);
				item.sampling_rate = (BYTE)eit.GetNumber(Desc::sampling_rate, lp);


				CARIB8CharDecode arib;
				string text_char = "";
				DWORD srcSize;
				const char* src = eit.GetStringOrEmpty(Desc::text_char, &srcSize, lp);
				arib.PSISI((const BYTE*)src, srcSize, &text_char);
				item.text_charLength = UpdateInfoText(item.text_char, text_char.c_str());

			}
		}
	}

	return TRUE;
}

void CEpgDBUtil::AddEventGroup(EVENT_INFO* eventInfo, const Desc::CDescriptor& eit, Desc::CDescriptor::CLoopPointer lp, WORD onid, WORD tsid)
{
	if( eventInfo->eventGroupInfo == NULL ){
		eventInfo->eventGroupInfo = new EPG_EVENTGROUP_INFO;
	}
	{
		SAFE_DELETE_ARRAY(eventInfo->eventGroupInfo->eventDataList);

		eventInfo->eventGroupInfo->group_type = (BYTE)eit.GetNumber(Desc::group_type, lp);
		eventInfo->eventGroupInfo->event_count = 0;
		if( eit.EnterLoop(lp) ){
			eventInfo->eventGroupInfo->event_count = (BYTE)eit.GetLoopSize(lp);
			eventInfo->eventGroupInfo->eventDataList = new EPG_EVENT_DATA[eventInfo->eventGroupInfo->event_count];
			for( DWORD i=0; eit.SetLoopIndex(lp, i); i++ ){
				EPG_EVENT_DATA item;
				item.event_id = (WORD)eit.GetNumber(Desc::event_id, lp);
				item.service_id = (WORD)eit.GetNumber(Desc::service_id, lp);
				item.original_network_id = onid;
				item.transport_stream_id = tsid;

				eventInfo->eventGroupInfo->eventDataList[i] = item;
			}
		}
	}
}

void CEpgDBUtil::AddEventRelay(EVENT_INFO* eventInfo, const Desc::CDescriptor& eit, Desc::CDescriptor::CLoopPointer lp, WORD onid, WORD tsid)
{
	if( eventInfo->eventRelayInfo == NULL ){
		eventInfo->eventRelayInfo = new EPG_EVENTGROUP_INFO;
	}
	{
		SAFE_DELETE_ARRAY(eventInfo->eventRelayInfo->eventDataList);

		eventInfo->eventRelayInfo->group_type = (BYTE)eit.GetNumber(Desc::group_type, lp);
		eventInfo->eventRelayInfo->event_count = 0;
		if( eventInfo->eventRelayInfo->group_type == 0x02 ){
			if( eit.EnterLoop(lp) ){
				eventInfo->eventRelayInfo->event_count = (BYTE)eit.GetLoopSize(lp);
				eventInfo->eventRelayInfo->eventDataList = new EPG_EVENT_DATA[eventInfo->eventRelayInfo->event_count];
				for( DWORD i=0; eit.SetLoopIndex(lp, i); i++ ){
					EPG_EVENT_DATA item;
					item.event_id = (WORD)eit.GetNumber(Desc::event_id, lp);
					item.service_id = (WORD)eit.GetNumber(Desc::service_id, lp);
					item.original_network_id = onid;
					item.transport_stream_id = tsid;

					eventInfo->eventRelayInfo->eventDataList[i] = item;
				}
			}
		}else{
			if( eit.EnterLoop(lp, 1) ){
				//���l�b�g���[�N�ւ̃����[���͑�2���[�v�ɂ���̂ŁA����͋L�q�q��event_count�̒l�Ƃ͈قȂ�
				eventInfo->eventRelayInfo->event_count = (BYTE)eit.GetLoopSize(lp);
				eventInfo->eventRelayInfo->eventDataList = new EPG_EVENT_DATA[eventInfo->eventRelayInfo->event_count];
				for( DWORD i=0; eit.SetLoopIndex(lp, i); i++ ){
					EPG_EVENT_DATA item;
					item.event_id = (WORD)eit.GetNumber(Desc::event_id, lp);
					item.service_id = (WORD)eit.GetNumber(Desc::service_id, lp);
					item.original_network_id = (WORD)eit.GetNumber(Desc::original_network_id, lp);
					item.transport_stream_id = (WORD)eit.GetNumber(Desc::transport_stream_id, lp);

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
			//L-EIT�̏�
			if( itr->second.lastTableID != 0 && itr->second.lastTableID <= 0x4F ){
				return CheckSectionAll(itr->second.sectionList) ? EpgLEITAll : EpgNeedData;
			}
		}else{
			//H-EIT�̏�
			if( itr->second.lastTableID > 0x4F ){
				//�g����񂪂Ȃ��ꍇ���u���܂����v�Ƃ݂Ȃ�
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
				//�T�[�r�X���X�g����Ȃ�f���T�[�r�X�̂ݑΏ�
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

BOOL CEpgDBUtil::AddServiceListNIT(const Desc::CDescriptor& nit)
{
	CBlockLock lock(&this->dbLock);

	wstring network_nameW = L"";

	Desc::CDescriptor::CLoopPointer lp;
	if( nit.EnterLoop(lp) ){
		for( DWORD i = 0; nit.SetLoopIndex(lp, i); i++ ){
			if( nit.GetNumber(Desc::descriptor_tag, lp) == Desc::network_name_descriptor ){
				DWORD srcSize;
				const char* src = nit.GetStringOrEmpty(Desc::d_char, &srcSize, lp);
				if( srcSize > 0 ){
					CARIB8CharDecode arib;
					string network_name = "";
					arib.PSISI((const BYTE*)src, srcSize, &network_name);
					AtoW(network_name, network_nameW);
				}
			}
		}
	}

	lp = Desc::CDescriptor::CLoopPointer();
	if( nit.EnterLoop(lp, 1) ){
		for( DWORD i = 0; nit.SetLoopIndex(lp, i); i++ ){
			//�T�[�r�X���X�V�p
			WORD onid = (WORD)nit.GetNumber(Desc::original_network_id, lp);
			WORD tsid = (WORD)nit.GetNumber(Desc::transport_stream_id, lp);
			map<DWORD, DB_TS_INFO>::iterator itrFind;
			itrFind = this->serviceInfoList.find((DWORD)onid << 16 | tsid);
			if( itrFind != this->serviceInfoList.end() ){
				itrFind->second.network_name = network_nameW;
			}

			Desc::CDescriptor::CLoopPointer lp2 = lp;
			if( nit.EnterLoop(lp2) ){
				for( DWORD j = 0; nit.SetLoopIndex(lp2, j); j++ ){
					if( nit.GetNumber(Desc::descriptor_tag, lp2) == Desc::service_list_descriptor ){
						Desc::CDescriptor::CLoopPointer lp3 = lp2;
						if( nit.EnterLoop(lp3) ){
							for( DWORD k=0; nit.SetLoopIndex(lp3, k); k++ ){
								ULONGLONG key = _Create64Key(onid, tsid, (WORD)nit.GetNumber(Desc::service_id, lp3));
								map<ULONGLONG, BYTE>::iterator itrService;
								itrService = this->serviceList.find(key);
								if( itrService == this->serviceList.end() ){
									this->serviceList.insert(pair<ULONGLONG, BYTE>(key, (BYTE)nit.GetNumber(Desc::service_type, lp3)));
								}
							}
						}
					}
					if( nit.GetNumber(Desc::descriptor_tag, lp2) == Desc::ts_information_descriptor && itrFind != this->serviceInfoList.end()){
						//ts_name��remote_control_key_id
						DWORD srcSize;
						const char* src = nit.GetStringOrEmpty(Desc::ts_name_char, &srcSize, lp2);
						if( srcSize > 0 ){
							CARIB8CharDecode arib;
							string ts_name = "";
							arib.PSISI((const BYTE*)src, srcSize, &ts_name);
							AtoW(ts_name, itrFind->second.ts_name);
						}
						itrFind->second.remote_control_key_id = (BYTE)nit.GetNumber(Desc::remote_control_key_id, lp2);
					}
					if( nit.GetNumber(Desc::descriptor_tag, lp2) == Desc::partial_reception_descriptor && itrFind != this->serviceInfoList.end()){
						//������M�t���O
						Desc::CDescriptor::CLoopPointer lp3 = lp2;
						if( nit.EnterLoop(lp3) ){
							for( DWORD k=0; nit.SetLoopIndex(lp3, k); k++ ){
								map<WORD,DB_SERVICE_INFO>::iterator itrService;
								itrService = itrFind->second.serviceList.find((WORD)nit.GetNumber(Desc::service_id, lp3));
								if( itrService != itrFind->second.serviceList.end() ){
									itrService->second.partialReceptionFlag = 1;
								}
							}
						}
					}
				}
			}
		}
	}

	return TRUE;
}

BOOL CEpgDBUtil::AddServiceListSIT(WORD TSID, const Desc::CDescriptor& sit)
{
	CBlockLock lock(&this->dbLock);

	WORD ONID = 0xFFFF;
	Desc::CDescriptor::CLoopPointer lp;
	if( sit.EnterLoop(lp) ){
		for( DWORD i = 0; sit.SetLoopIndex(lp, i); i++ ){
			if( sit.GetNumber(Desc::descriptor_tag, lp) == Desc::network_identification_descriptor ){
				ONID = (WORD)sit.GetNumber(Desc::network_id, lp);
			}
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

		lp = Desc::CDescriptor::CLoopPointer();
		if( sit.EnterLoop(lp, 1) ){
			for( DWORD i = 0; sit.SetLoopIndex(lp, i); i++ ){
				DB_SERVICE_INFO item;
				item.original_network_id = ONID;
				item.transport_stream_id = TSID;
				item.service_id = (WORD)sit.GetNumber(Desc::service_id, lp);

				Desc::CDescriptor::CLoopPointer lp2 = lp;
				if( sit.EnterLoop(lp2) ){
					for( DWORD j = 0; sit.SetLoopIndex(lp2, j); j++ ){
						if( sit.GetNumber(Desc::descriptor_tag, lp2) == Desc::service_descriptor ){
							CARIB8CharDecode arib;
							string service_provider_name = "";
							string service_name = "";
							const char* src;
							DWORD srcSize;
							src = sit.GetStringOrEmpty(Desc::service_provider_name, &srcSize, lp2);
							if( srcSize > 0 ){
								arib.PSISI((const BYTE*)src, srcSize, &service_provider_name);
							}
							src = sit.GetStringOrEmpty(Desc::service_name, &srcSize, lp2);
							if( srcSize > 0 ){
								arib.PSISI((const BYTE*)src, srcSize, &service_name);
							}
							AtoW(service_provider_name, item.service_provider_name);
							AtoW(service_name, item.service_name);

							item.service_type = (BYTE)sit.GetNumber(Desc::service_type, lp2);
						}
					}
				}
				info.serviceList.insert(std::make_pair(item.service_id, item));
			}
		}
		this->serviceInfoList.insert(std::make_pair(key, info));
	}


	return TRUE;
}

BOOL CEpgDBUtil::AddSDT(const Desc::CDescriptor& sdt)
{
	CBlockLock lock(&this->dbLock);

	DWORD key = sdt.GetNumber(Desc::original_network_id) << 16 | sdt.GetNumber(Desc::transport_stream_id);
	map<DWORD, DB_TS_INFO>::iterator itrTS;
	itrTS = this->serviceInfoList.find(key);
	if( itrTS == this->serviceInfoList.end() ){
		DB_TS_INFO info;
		info.original_network_id = (WORD)sdt.GetNumber(Desc::original_network_id);
		info.transport_stream_id = (WORD)sdt.GetNumber(Desc::transport_stream_id);
		itrTS = this->serviceInfoList.insert(std::make_pair(key, info)).first;
	}

	Desc::CDescriptor::CLoopPointer lp;
	if( sdt.EnterLoop(lp) ){
		for( DWORD i = 0; sdt.SetLoopIndex(lp, i); i++ ){
			map<WORD,DB_SERVICE_INFO>::iterator itrS;
			itrS = itrTS->second.serviceList.find((WORD)sdt.GetNumber(Desc::service_id, lp));
			if( itrS == itrTS->second.serviceList.end()){
				DB_SERVICE_INFO item;
				item.original_network_id = itrTS->second.original_network_id;
				item.transport_stream_id = itrTS->second.transport_stream_id;
				item.service_id = (WORD)sdt.GetNumber(Desc::service_id, lp);

				Desc::CDescriptor::CLoopPointer lp2 = lp;
				if( sdt.EnterLoop(lp2) ){
					for( DWORD j = 0; sdt.SetLoopIndex(lp2, j); j++ ){
						if( sdt.GetNumber(Desc::descriptor_tag, lp2) != Desc::service_descriptor ){
							continue;
						}
						CARIB8CharDecode arib;
						string service_provider_name = "";
						string service_name = "";
						const char* src;
						DWORD srcSize;
						src = sdt.GetStringOrEmpty(Desc::service_provider_name, &srcSize, lp2);
						if( srcSize > 0 ){
							arib.PSISI((const BYTE*)src, srcSize, &service_provider_name);
						}
						src = sdt.GetStringOrEmpty(Desc::service_name, &srcSize, lp2);
						if( srcSize > 0 ){
							arib.PSISI((const BYTE*)src, srcSize, &service_name);
						}
						AtoW(service_provider_name, item.service_provider_name);
						AtoW(service_name, item.service_name);

						item.service_type = (BYTE)sdt.GetNumber(Desc::service_type, lp2);
					}
				}
				itrTS->second.serviceList.insert(std::make_pair(item.service_id, item));
			}
		}
	}

	return TRUE;
}

//�w��T�[�r�X�̑SEPG�����擾����
// originalNetworkID		[IN]�擾�Ώۂ�originalNetworkID
// transportStreamID		[IN]�擾�Ώۂ�transportStreamID
// serviceID				[IN]�擾�Ώۂ�ServiceID
// epgInfoListSize			[OUT]epgInfoList�̌�
// epgInfoList				[OUT]EPG���̃��X�g�iDLL���Ŏ����I��delete����B���Ɏ擾���s���܂ŗL���j
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
			//[p/f]���o��
			evt = evtPF[0];
			evtPF[0] = evtPF[1];
			evtPF[1] = NULL;
		}else{
			if( evtPF[0] && evtPF[0]->event_id == itrEvt->first ){
				//��������Ƃ���[p/f]��D��
				evt = evtPF[0];
				evtPF[0] = evtPF[1];
				evtPF[1] = NULL;
				if( evt->extInfo == NULL && itrEvt->second->extInfo ){
					extInfoSchedule = new EPG_EXTENDED_EVENT_INFO;
					extInfoSchedule->DeepCopy(*itrEvt->second->extInfo);
				}
			}else{
				//[schedule]���o��
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

//�A�h���Xx��T�̃A���C�������g�Ő؂�グ�ĕԂ�
template<class T> static inline T* AlignCeil(void* x)
{
	return (T*)(((size_t)x + (__alignof(T) - 1)) & ~(__alignof(T) - 1));
}

//�w��T�[�r�X�̑SEPG����񋓂���
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
		//�f�X�g���N�^���Ă΂Ȃ��悤�̈悾�����蓖��(POD�\���̂����Ȃ̂Ŗ����)�A�}�X�^�[�𒼐ڎQ�Ƃ��č\�z����
		EPG_EVENT_INFO* item = AlignCeil<EPG_EVENT_INFO>(info) + count;
		EPG_EXTENDED_EVENT_INFO* extInfoSchedule = NULL;
		const EVENT_INFO* evt;
		if( itrEvt == itr->second.eventMap.end() || evtPF[0] && evtPF[0]->event_id < itrEvt->first ){
			//[p/f]���o��
			evt = evtPF[0];
			evtPF[0] = evtPF[1];
			evtPF[1] = NULL;
		}else{
			if( evtPF[0] && evtPF[0]->event_id == itrEvt->first ){
				//��������Ƃ���[p/f]��D��
				evt = evtPF[0];
				evtPF[0] = evtPF[1];
				evtPF[1] = NULL;
				if( evt->extInfo == NULL ){
					extInfoSchedule = itrEvt->second->extInfo;
				}
			}else{
				//[schedule]���o��
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

//�~�ς��ꂽEPG���̂���T�[�r�X�ꗗ���擾����
//SERVICE_EXT_INFO�̏��͂Ȃ��ꍇ������
//�����F
// serviceListSize			[OUT]serviceList�̌�
// serviceList				[OUT]�T�[�r�X���̃��X�g�iDLL���Ŏ����I��delete����B���Ɏ擾���s���܂ŗL���j
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

//�w��T�[�r�X�̌���or����EPG�����擾����
//�����F
// originalNetworkID		[IN]�擾�Ώۂ�originalNetworkID
// transportStreamID		[IN]�擾�Ώۂ�transportStreamID
// serviceID				[IN]�擾�Ώۂ�ServiceID
// nextFlag					[IN]TRUE�i���̔ԑg�j�AFALSE�i���݂̔ԑg�j
// epgInfo					[OUT]EPG���iDLL���Ŏ����I��delete����B���Ɏ擾���s���܂ŗL���j
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

//�w��C�x���g��EPG�����擾����
//�����F
// originalNetworkID		[IN]�擾�Ώۂ�originalNetworkID
// transportStreamID		[IN]�擾�Ώۂ�transportStreamID
// serviceID				[IN]�擾�Ώۂ�ServiceID
// EventID					[IN]�擾�Ώۂ�EventID
// pfOnlyFlag				[IN]p/f����̂݌������邩�ǂ���
// epgInfo					[OUT]EPG���iDLL���Ŏ����I��delete����B���Ɏ擾���s���܂ŗL���j
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
