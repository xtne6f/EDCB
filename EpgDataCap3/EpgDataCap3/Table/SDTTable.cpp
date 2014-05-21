#include "StdAfx.h"
#include "SDTTable.h"

#include "../Descriptor/Descriptor.h"
#include "../ARIB8CharDecode.h"

CSDTTable::CSDTTable(void)
{
}

CSDTTable::~CSDTTable(void)
{
	Clear();
}

void CSDTTable::Clear()
{
	for( size_t i=0 ;i<serviceInfoList.size(); i++ ){
		SAFE_DELETE(serviceInfoList[i]);
	}
	serviceInfoList.clear();
}

BOOL CSDTTable::Decode( BYTE* data, DWORD dataSize, DWORD* decodeReadSize )
{
	if( InitDecode(data, dataSize, decodeReadSize, TRUE) == FALSE ){
		return FALSE;
	}
	Clear();

	if( section_syntax_indicator != 1 ){
		//固定値がおかしい
		_OutputDebugString( L"++CSDTTable:: section_syntax err" );
		return FALSE;
	}
	if( table_id != 0x42 && table_id != 0x46 ){
		//table_idがおかしい
		_OutputDebugString( L"++CSDTTable:: table_id err 0x%02X", table_id );
		return FALSE;
	}

	if( section_length - 4 > 7 ){
		transport_stream_id = ((WORD)data[readSize])<<8 | data[readSize+1];
		version_number = (data[readSize+2]&0x3E)>>1;
		current_next_indicator = data[readSize+2]&0x01;
		section_number = data[readSize+3];
		last_section_number = data[readSize+4];
		original_network_id = ((WORD)data[readSize+5])<<8 | data[readSize+6];
		readSize += 8;
		while( readSize+4 < (DWORD)section_length+3-4 ){
			SERVICE_INFO_DATA* item = new SERVICE_INFO_DATA;
			//if( original_network_id == 0x0001 || original_network_id == 0x0003 || original_network_id == 0x000A ){
			//	item->service_id = ((WORD)data[readSize]&0x0F)<<8 | data[readSize+1];
			//}else{
				item->service_id = ((WORD)data[readSize])<<8 | data[readSize+1];
			//}
			item->EIT_user_defined_flags = (data[readSize+2]&0x1C)>>2;
			item->EIT_schedule_flag = (data[readSize+2]&0x02)>>1;
			item->EIT_present_following_flag = data[readSize+2]&0x01;
			item->running_status = (data[readSize+3]&0xE0)>>5;
			item->free_CA_mode = (data[readSize+3]&0x10)>>4;
			item->descriptors_loop_length = ((WORD)data[readSize+3]&0x0F)<<8 | data[readSize+4];
			readSize += 5;
			BOOL err = FALSE;
			if( readSize+item->descriptors_loop_length <= (DWORD)section_length+3-4 && item->descriptors_loop_length > 0){
				if( original_network_id == 0x0001 || original_network_id == 0x0003){
					if( SDDecode( data+readSize, item->descriptors_loop_length, &(item->descriptorList), NULL ) == FALSE ){
						err = TRUE;
					}else{
						serviceInfoList.push_back(item);
					}
				}else{
					CDescriptor descriptor;
					if( descriptor.Decode( data+readSize, item->descriptors_loop_length, &(item->descriptorList), NULL ) == FALSE ){
						_OutputDebugString( L"++CSDTTable:: descriptor2 err" );
						SAFE_DELETE(item);
						return FALSE;
					}
					serviceInfoList.push_back(item);
				}
			}

			readSize+=item->descriptors_loop_length;
			if( err == TRUE ){
				SAFE_DELETE(item);
			}

		}
	}else{
		return FALSE;
	}

	return TRUE;
}

BOOL CSDTTable::SDDecode( BYTE* data, DWORD dataSize, vector<DESCRIPTOR_DATA*>* descriptorList, DWORD* decodeReadSize )
{
	BOOL ret = TRUE;
	if( data == NULL || dataSize == 0 || descriptorList == NULL ){
		return FALSE;
	}
	DWORD decodeSize = 0;

	DESCRIPTOR_DATA* item = new DESCRIPTOR_DATA;
	item->service = new CServiceDesc;

	while( decodeSize < dataSize ){
		BYTE* readPos = data+decodeSize;
		if( readPos[0] == 0x8A ){
			//サービスタイプ
			item->service->service_type = readPos[2];
			if( item->service->service_type == 0x81 ){
				item->service->service_type = 0xA1;
			}
			decodeSize += readPos[1]+2;
		}else
		if( readPos[0] == 0x82 ){
			//サービス名
			if( readPos[2] == 0x01 ){
				//日本語版？
				item->service->service_name_length = readPos[1]-1;
				item->service->char_service_name = new CHAR[item->service->service_name_length];
				memcpy(item->service->char_service_name, readPos+3, item->service->service_name_length);
			}
			decodeSize += readPos[1]+2;
		}else{
			decodeSize += readPos[1]+2;
		}
	}
	if( item->service->service_name_length == 0 ){
		SAFE_DELETE(item);
		ret = FALSE;
	}else{
		descriptorList->push_back(item);
	}

	if( decodeReadSize != NULL ){
		*decodeReadSize = dataSize;
	}
	return ret;
}

