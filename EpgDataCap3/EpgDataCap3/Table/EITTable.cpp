#include "StdAfx.h"
#include "EITTable.h"

#include "../../../Common/EpgTimerUtil.h"
#include "../Descriptor/Descriptor.h"

CEITTable::CEITTable(void)
{
	this->failure = FALSE;
}

CEITTable::~CEITTable(void)
{
	Clear();
}

void CEITTable::Clear()
{
	for( size_t i=0 ;i<eventInfoList.size(); i++ ){
		SAFE_DELETE(eventInfoList[i]);
	}
	eventInfoList.clear();
}

BOOL CEITTable::Decode( BYTE* data, DWORD dataSize, DWORD* decodeReadSize )
{
	if( data == NULL ){
		return FALSE;
	}
	Clear();

	//////////////////////////////////////////////////////
	//サイズのチェック
	//最低限table_idとsection_length+CRCのサイズは必須
	if( dataSize < 7 ){
		return FALSE;
	}
	//->サイズのチェック

	DWORD readSize = 0;
	//////////////////////////////////////////////////////
	//解析処理
	table_id = data[0];
	section_syntax_indicator = (data[1]&0x80)>>7;
	section_length = ((WORD)data[1]&0x0F)<<8 | data[2];
	readSize+=3;

	if( section_syntax_indicator != 1 ){
		//固定値がおかしい
		_OutputDebugString( L"++CEITTable:: section_syntax err" );
		return FALSE;
	}
	if( table_id < 0x4E || table_id > 0x6F ){
		//table_idがおかしい
		_OutputDebugString( L"++CEITTable:: table_id err 0x%02X", table_id );
		return FALSE;
	}
	if( readSize+section_length > dataSize && section_length > 3){
		//サイズ異常
		_OutputDebugString( L"++CEITTable:: size err %d > %d", readSize+section_length, dataSize );
		return FALSE;
	}
	//CRCチェック
	crc32 = ((DWORD)data[3+section_length-4])<<24 |
		((DWORD)data[3+section_length-3])<<16 |
		((DWORD)data[3+section_length-2])<<8 |
		data[3+section_length-1];
	if( crc32 != _Crc32(3+section_length-4, data) ){
		_OutputDebugString( L"++CEITTable:: CRC err" );
		return FALSE;
	}

	if( section_length > 4 ){
		service_id = ((WORD)data[readSize])<<8 | data[readSize+1];
		version_number = (data[readSize+2]&0x3E)>>1;
		current_next_indicator = data[readSize+2]&0x01;
		section_number = data[readSize+3];
		last_section_number = data[readSize+4];
		transport_stream_id = ((WORD)data[readSize+5])<<8 | data[readSize+6];
		original_network_id = ((WORD)data[readSize+7])<<8 | data[readSize+8];
		segment_last_section_number = data[readSize+9];
		last_table_id = data[readSize+10];
		readSize += 11;
		while( readSize < (DWORD)section_length+3-4 ){
			EVENT_INFO_DATA* item = new EVENT_INFO_DATA;
			item->event_id = ((WORD)data[readSize])<<8 | data[readSize+1];
			if( data[readSize+2] == 0xFF && data[readSize+3] == 0xFF && data[readSize+4] == 0xFF &&
				data[readSize+5] == 0xFF && data[readSize+6] == 0xFF )
			{
				item->StartTimeFlag = FALSE;
			}else{
				item->StartTimeFlag = TRUE;
				DWORD mjd = ((DWORD)data[readSize+2])<<8 | data[readSize+3];
				_MJDtoSYSTEMTIME(mjd, &(item->start_time));
				item->start_time.wHour = (WORD)_BCDtoDWORD(data+readSize+4, 1, 2);
				item->start_time.wMinute = (WORD)_BCDtoDWORD(data+readSize+5, 1, 2);
				item->start_time.wSecond = (WORD)_BCDtoDWORD(data+readSize+6, 1, 2);
			}
			readSize+=7;
			if( data[readSize] == 0xFF && data[readSize+1] == 0xFF && data[readSize+2] == 0xFF)
			{
				item->DurationFlag = FALSE;
			}else{
				item->DurationFlag = TRUE;
				item->durationHH = (WORD)_BCDtoDWORD(data+readSize, 1, 2);
				item->durationMM = (WORD)_BCDtoDWORD(data+readSize+1, 1, 2);
				item->durationSS = (WORD)_BCDtoDWORD(data+readSize+2, 1, 2);
			}
			readSize+=3;
			item->running_status = (data[readSize]&0xE0)>>5;
			item->free_CA_mode = (data[readSize]&0x10)>>4;
			item->descriptors_loop_length = ((WORD)data[readSize]&0x0F)<<8 | data[readSize+1];
			readSize += 2;
			if( readSize+item->descriptors_loop_length <= (DWORD)section_length+3-4 && item->descriptors_loop_length > 0){
				if( original_network_id == 0x0001 || original_network_id == 0x0003 ){
					SDDecode( data+readSize, item->descriptors_loop_length, &(item->descriptorList), NULL );
				}else{
					CDescriptor descriptor;
					if( descriptor.Decode( data+readSize, item->descriptors_loop_length, &(item->descriptorList), NULL ) == FALSE ){
						_OutputDebugString( L"++CEITTable:: descriptor2 err" );
						this->failure = TRUE;
						return TRUE;
					}
				}
			}

			readSize+=item->descriptors_loop_length;

			eventInfoList.push_back(item);
		}
	}else{
		return FALSE;
	}
	//->解析処理

	if( decodeReadSize != NULL ){
		*decodeReadSize = 3+section_length;
	}

	return TRUE;
}

BOOL CEITTable::SDDecode( BYTE* data, DWORD dataSize, vector<DESCRIPTOR_DATA*>* descriptorList, DWORD* decodeReadSize )
{
	BOOL ret = TRUE;
	if( data == NULL || dataSize == 0 || descriptorList == NULL ){
		return FALSE;
	}
	DWORD decodeSize = 0;

	DESCRIPTOR_DATA* extItem = NULL;
	DESCRIPTOR_DATA* shortItem = NULL;

	while( decodeSize < dataSize ){
		BYTE* readPos = data+decodeSize;

		if( readPos[0] == 0x54 ){
			DESCRIPTOR_DATA* item = new DESCRIPTOR_DATA;
			item->content = new CContentDesc;
			item->content->Decode(readPos, readPos[1]+2, NULL);


			for( size_t i=0; i<item->content->nibbleList.size(); i++ ){
				switch(item->content->nibbleList[i].user_nibble_1){
				case 0x00:
					//映画？
					item->content->nibbleList[i].content_nibble_level_1 = 0x06;
					item->content->nibbleList[i].content_nibble_level_2 = 0x0F;
					break;
				case 0x02:
					//スポーツ？
					item->content->nibbleList[i].content_nibble_level_1 = 0x01;
					item->content->nibbleList[i].content_nibble_level_2 = 0x0F;
					break;
				case 0x04:
					//音楽？
					item->content->nibbleList[i].content_nibble_level_1 = 0x04;
					item->content->nibbleList[i].content_nibble_level_2 = 0x0F;
					break;
				case 0x05:
					//ドラマ？
					item->content->nibbleList[i].content_nibble_level_1 = 0x03;
					item->content->nibbleList[i].content_nibble_level_2 = 0x0F;
					break;
				case 0x06:
					//ニュース？
					item->content->nibbleList[i].content_nibble_level_1 = 0x00;
					item->content->nibbleList[i].content_nibble_level_2 = 0x0F;
					break;
				case 0x07:
					//バラエティ？
					item->content->nibbleList[i].content_nibble_level_1 = 0x05;
					item->content->nibbleList[i].content_nibble_level_2 = 0x0F;
					break;
				case 0x08:
					//趣味／教育？
					item->content->nibbleList[i].content_nibble_level_1 = 0x0A;
					item->content->nibbleList[i].content_nibble_level_2 = 0x0F;
					break;
				case 0x09:
					//アニメ？
					item->content->nibbleList[i].content_nibble_level_1 = 0x07;
					item->content->nibbleList[i].content_nibble_level_2 = 0x0F;
					break;
				case 0x0A:
					//ドキュメンタリー／教養？
					item->content->nibbleList[i].content_nibble_level_1 = 0x08;
					item->content->nibbleList[i].content_nibble_level_2 = 0x0F;
					break;
				default:
					break;
				}
			}

			descriptorList->push_back(item);

			decodeSize += readPos[1]+2;
		}else
		if( readPos[0] == 0x85 ){
			//コンポーネント
			if( (readPos[2]&0x0F) == 0x01 ){
				//映像
				DESCRIPTOR_DATA* item = new DESCRIPTOR_DATA;
				item->component = new CComponentDesc;

				item->component->stream_content = readPos[2]&0x0F;
				item->component->component_type = readPos[3];
				item->component->component_tag = readPos[4];

				item->component->text_charLength = readPos[1]-4;
				item->component->text_char = new BYTE[item->component->text_charLength+1];
				
				memcpy(item->component->text_char, readPos+6, item->component->text_charLength);
				item->component->text_char[item->component->text_charLength] = '\0';
				descriptorList->push_back(item);
			}else
			if( (readPos[2]&0x0F) == 0x02 ){
				//音声
				DESCRIPTOR_DATA* item = new DESCRIPTOR_DATA;
				item->audioComponent = new CAudioComponentDesc;

				item->audioComponent->stream_content = readPos[2]&0x0F;
				item->audioComponent->component_type = readPos[3];
				item->audioComponent->component_tag = readPos[4];

				item->audioComponent->text_charLength = readPos[1]-4;
				item->audioComponent->text_char = new CHAR[item->audioComponent->text_charLength+1];
				
				memcpy(item->audioComponent->text_char, readPos+6, item->audioComponent->text_charLength);
				item->audioComponent->text_char[item->audioComponent->text_charLength] = '\0';
				descriptorList->push_back(item);
			}
			decodeSize += readPos[1]+2;
		}else
		if( readPos[0] == 0x82 ){
			//番組名
			if( readPos[2] == 0x01 ){
				if( shortItem == NULL ){
					shortItem = new DESCRIPTOR_DATA;
				}
				shortItem->shortEvent = new CShortEventDesc;

				//日本語版？
				shortItem->shortEvent->event_name_length = readPos[1]-1;
				shortItem->shortEvent->event_name_char = new CHAR[shortItem->shortEvent->event_name_length+1];
				
				memcpy(shortItem->shortEvent->event_name_char, readPos+3, shortItem->shortEvent->event_name_length);
				shortItem->shortEvent->event_name_char[shortItem->shortEvent->event_name_length] = '\0';
				descriptorList->push_back(shortItem);
			}else if( readPos[2] == 0x02 && shortItem == NULL){
				if( shortItem == NULL ){
					shortItem = new DESCRIPTOR_DATA;
				}
				shortItem->shortEvent = new CShortEventDesc;

				//英語版？
				shortItem->shortEvent->event_name_length = readPos[1]-1;
				shortItem->shortEvent->event_name_char = new CHAR[shortItem->shortEvent->event_name_length+1];
				
				memcpy(shortItem->shortEvent->event_name_char, readPos+3, shortItem->shortEvent->event_name_length);
				shortItem->shortEvent->event_name_char[shortItem->shortEvent->event_name_length] = '\0';
				descriptorList->push_back(shortItem);
			}
			decodeSize += readPos[1]+2;
		}else
		if( readPos[0] == 0x83 ){
			//詳細情報
			if( extItem == NULL ){
				extItem = new DESCRIPTOR_DATA;
				extItem->extendedEvent = new CExtendedEventDesc;

				CExtendedEventDesc::ITEM_DATA* data = new CExtendedEventDesc::ITEM_DATA;
				data->item_description_length = readPos[1]-1;
				data->item_description_char = new CHAR[data->item_description_length+1];
				memcpy(data->item_description_char, readPos+3, data->item_description_length);
				data->item_description_char[data->item_description_length] = '\0';

				extItem->extendedEvent->itemList.push_back(data);
			}else{
				CExtendedEventDesc::ITEM_DATA* data = new CExtendedEventDesc::ITEM_DATA;
				data->item_description_length = readPos[1]-1;
				data->item_description_char = new CHAR[data->item_description_length+1];
				memcpy(data->item_description_char, readPos+3, data->item_description_length);
				data->item_description_char[data->item_description_length] = '\0';

				extItem->extendedEvent->itemList.push_back(data);
			}

			decodeSize += readPos[1]+2;
		}else{
			decodeSize += readPos[1]+2;
		}
	}
	if( extItem != NULL ){
		descriptorList->push_back(extItem);
	}

	if( descriptorList->size() == 0 ){
		ret = FALSE;
	}

	if( decodeReadSize != NULL ){
		*decodeReadSize = dataSize;
	}
	return ret;
}

