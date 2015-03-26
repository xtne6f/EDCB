#include "StdAfx.h"
#include "EITTable_SD.h"

#include "../../../Common/EpgTimerUtil.h"

CEITTable_SD::CEITTable_SD(void)
{
}

CEITTable_SD::~CEITTable_SD(void)
{
	Clear();
}

void CEITTable_SD::Clear()
{
	for( size_t i=0 ;i<eventInfoList.size(); i++ ){
		SAFE_DELETE(eventInfoList[i]);
	}
	eventInfoList.clear();
}

BOOL CEITTable_SD::Decode( BYTE* data, DWORD dataSize, DWORD* decodeReadSize )
{
	if( InitDecode(data, dataSize, decodeReadSize, TRUE) == FALSE ){
		return FALSE;
	}
	Clear();

	if( section_syntax_indicator != 1 ){
		//固定値がおかしい
		_OutputDebugString( L"++CEITTable_SD:: section_syntax err" );
		return FALSE;
	}
	if( table_id != 0xA4 && table_id != 0xA7 ){
		//table_idがおかしい
		_OutputDebugString( L"++CEITTable_SD:: table_id err 0x%02X", table_id );
		return FALSE;
	}

	if( section_length - 4 > 8 ){
		service_id = ((WORD)data[readSize])<<8 | data[readSize+1];
		version_number = (data[readSize+2]&0x3E)>>1;
		current_next_indicator = data[readSize+2]&0x01;
		section_number = data[readSize+3];
		last_section_number = data[readSize+4];
		transport_stream_id = ((WORD)data[readSize+5])<<8 | data[readSize+6];
		original_network_id = ((WORD)data[readSize+7])<<8 | data[readSize+8];

		readSize += 9;
		while( readSize+11 < (DWORD)section_length+3-4 ){
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
					if( AribDescriptor::CreateDescriptors( data+readSize, item->descriptors_loop_length, &(item->descriptorList), NULL ) == FALSE ){
						_OutputDebugString( L"++CEITTable_SD:: descriptor2 err" );
						SAFE_DELETE(item);
						return FALSE;
					}
				}
			}

			readSize+=item->descriptors_loop_length;

			eventInfoList.push_back(item);
		}
	}else{
		return FALSE;
	}

	return TRUE;
}

BOOL CEITTable_SD::SDDecode( BYTE* data, DWORD dataSize, vector<AribDescriptor::CDescriptor*>* descriptorList, DWORD* decodeReadSize )
{
	BOOL ret = TRUE;
	if( data == NULL || dataSize == 0 || descriptorList == NULL ){
		return FALSE;
	}
	DWORD decodeSize = 0;

	AribDescriptor::CDescriptor* shortItem = NULL;

	static const short parser0x82[] = {
		AribDescriptor::descriptor_tag, 8,
		AribDescriptor::descriptor_length, AribDescriptor::D_LOCAL, 8,
		AribDescriptor::D_BEGIN, AribDescriptor::descriptor_length,
			AribDescriptor::reserved, AribDescriptor::D_LOCAL, 8,
			AribDescriptor::event_name_char, AribDescriptor::D_STRING_TO_END,
		AribDescriptor::D_END,
		AribDescriptor::D_FIN,
	};
	static const short parser0x83[] = {
		AribDescriptor::descriptor_tag, 8,
		AribDescriptor::descriptor_length, AribDescriptor::D_LOCAL, 8,
		AribDescriptor::D_BEGIN, AribDescriptor::descriptor_length,
			AribDescriptor::reserved, AribDescriptor::D_LOCAL, 8,
			AribDescriptor::D_BEGIN_FOR, 8,
				AribDescriptor::item_description_char, AribDescriptor::D_STRING_TO_END,
			AribDescriptor::D_END,
		AribDescriptor::D_END,
		AribDescriptor::D_FIN,
	};
	static const short parser0x85[] = {
		AribDescriptor::descriptor_tag, 8,
		AribDescriptor::descriptor_length, AribDescriptor::D_LOCAL, 8,
		AribDescriptor::D_BEGIN, AribDescriptor::descriptor_length,
			AribDescriptor::reserved, AribDescriptor::D_LOCAL, 4,
			AribDescriptor::stream_content, 4,
			AribDescriptor::component_type, 8,
			AribDescriptor::component_tag, 8,
			AribDescriptor::reserved, AribDescriptor::D_LOCAL, 8,
			AribDescriptor::text_char, AribDescriptor::D_STRING_TO_END,
		AribDescriptor::D_END,
		AribDescriptor::D_FIN,
	};
	AribDescriptor::PARSER_PAIR parserList[] = {{0x82, parser0x82}, {0x83, parser0x83}, {0x85, parser0x85}, {0, NULL}};

	while( decodeSize + 2 < dataSize ){
		BYTE* readPos = data+decodeSize;
		if( readPos[0] == 0x54 ){
			AribDescriptor::CDescriptor* item = new AribDescriptor::CDescriptor;
			if( item->Decode(readPos, dataSize - decodeSize, NULL) == false ){
				delete item;
			}else{
				if( item->EnterLoop() ){
					for( DWORD i = 0; item->SetLoopIndex(i); i++ ){
						switch( item->GetNumber(AribDescriptor::user_nibble_1) ){
						case 0x00:
							//映画？
							item->SetNumber(AribDescriptor::content_nibble_level_1, 0x06);
							item->SetNumber(AribDescriptor::content_nibble_level_2, 0x0F);
							break;
						case 0x02:
							//スポーツ？
							item->SetNumber(AribDescriptor::content_nibble_level_1, 0x01);
							item->SetNumber(AribDescriptor::content_nibble_level_2, 0x0F);
							break;
						case 0x04:
							//音楽？
							item->SetNumber(AribDescriptor::content_nibble_level_1, 0x04);
							item->SetNumber(AribDescriptor::content_nibble_level_2, 0x0F);
							break;
						case 0x05:
							//ドラマ？
							item->SetNumber(AribDescriptor::content_nibble_level_1, 0x03);
							item->SetNumber(AribDescriptor::content_nibble_level_2, 0x0F);
							break;
						case 0x06:
							//ニュース？
							item->SetNumber(AribDescriptor::content_nibble_level_1, 0x00);
							item->SetNumber(AribDescriptor::content_nibble_level_2, 0x0F);
							break;
						case 0x07:
							//バラエティ？
							item->SetNumber(AribDescriptor::content_nibble_level_1, 0x05);
							item->SetNumber(AribDescriptor::content_nibble_level_2, 0x0F);
							break;
						case 0x08:
							//趣味／教育？
							item->SetNumber(AribDescriptor::content_nibble_level_1, 0x0A);
							item->SetNumber(AribDescriptor::content_nibble_level_2, 0x0F);
							break;
						case 0x09:
							//アニメ？
							item->SetNumber(AribDescriptor::content_nibble_level_1, 0x07);
							item->SetNumber(AribDescriptor::content_nibble_level_2, 0x0F);
							break;
						case 0x0A:
							//ドキュメンタリー／教養？
							item->SetNumber(AribDescriptor::content_nibble_level_1, 0x08);
							item->SetNumber(AribDescriptor::content_nibble_level_2, 0x0F);
							break;
						default:
							break;
						}
					}
					item->LeaveLoop();
				}
				descriptorList->push_back(item);
			}

			decodeSize += readPos[1]+2;
		}else
		if( readPos[0] == 0x85 ){
			//コンポーネント
			AribDescriptor::CDescriptor* item = new AribDescriptor::CDescriptor;
			if( item->Decode(readPos, dataSize - decodeSize, NULL, parserList) == false ){
				delete item;
			}else{
				if( item->GetNumber(AribDescriptor::stream_content) == 0x01 ){
					//映像。コンポーネント記述子にキャスト
					item->SetNumber(AribDescriptor::descriptor_tag, AribDescriptor::component_descriptor);
					descriptorList->push_back(item);
				}else
				if( item->GetNumber(AribDescriptor::stream_content) == 0x02 ){
					//音声。音声コンポーネント記述子にキャスト
					item->SetNumber(AribDescriptor::descriptor_tag, AribDescriptor::audio_component_descriptor);
					descriptorList->push_back(item);
				}else{
					delete item;
				}
			}
			decodeSize += readPos[1]+2;
		}else
		if( readPos[0] == 0x82 ){
			//番組名
			if( readPos[2] == 0x01 ){
				if( shortItem == NULL ){
					shortItem = new AribDescriptor::CDescriptor;
				}
				if( shortItem->Decode(readPos, dataSize - decodeSize, NULL, parserList) == false ){
					delete shortItem;
					shortItem = NULL;
				}else{
					//日本語版？
					//短形式イベント記述子にキャスト
					shortItem->SetNumber(AribDescriptor::descriptor_tag, AribDescriptor::short_event_descriptor);
				}
			}else if( readPos[2] == 0x02 && shortItem == NULL){
				shortItem = new AribDescriptor::CDescriptor;
				if( shortItem->Decode(readPos, dataSize - decodeSize, NULL, parserList) == false ){
					delete shortItem;
					shortItem = NULL;
				}else{
					//英語版？
					//短形式イベント記述子にキャスト
					shortItem->SetNumber(AribDescriptor::descriptor_tag, AribDescriptor::short_event_descriptor);
				}
			}
			decodeSize += readPos[1]+2;
		}else
		if( readPos[0] == 0x83 ){
			//詳細情報
			AribDescriptor::CDescriptor* item = new AribDescriptor::CDescriptor;
			if( item->Decode(readPos, dataSize - decodeSize, NULL, parserList) == false ){
				delete item;
			}else{
				//拡張形式イベント記述子にキャスト
				//TODO: 本当はdescriptor_number等調整すべきだが、後続の処理で一切利用されないので省略する
				item->SetNumber(AribDescriptor::descriptor_tag, AribDescriptor::extended_event_descriptor);
				descriptorList->push_back(item);
			}

			decodeSize += readPos[1]+2;
		}else{
			//_OutputDebugString(L"0x%02x\r\n", readPos[0]);
			decodeSize += readPos[1]+2;
		}
	}
	if( shortItem != NULL ){
		descriptorList->push_back(shortItem);
	}

	if( descriptorList->size() == 0 ){
		ret = FALSE;
	}

	if( decodeReadSize != NULL ){
		*decodeReadSize = dataSize;
	}
	return ret;
}

