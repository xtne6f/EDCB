#include "StdAfx.h"
#include "SDTTTable.h"

#include "../../../Common/EpgTimerUtil.h"

CSDTTTable::CSDTTTable(void)
{
}

CSDTTTable::~CSDTTTable(void)
{
	Clear();
}

void CSDTTTable::Clear()
{
	for( size_t i=0 ;i<contentInfoList.size(); i++ ){
		SAFE_DELETE(contentInfoList[i]);
	}
	contentInfoList.clear();
}

BOOL CSDTTTable::Decode( BYTE* data, DWORD dataSize, DWORD* decodeReadSize )
{
	if( InitDecode(data, dataSize, decodeReadSize, TRUE) == FALSE ){
		return FALSE;
	}
	Clear();

	if( section_syntax_indicator != 1 ){
		//ŒÅ’è’l‚ª‚¨‚©‚µ‚¢
		_OutputDebugString( L"++CSDTTTable:: section_syntax err" );
		return FALSE;
	}
	if( table_id != 0xC3 ){
		//table_id‚ª‚¨‚©‚µ‚¢
		_OutputDebugString( L"++CSDTTTable:: table_id err 0xC3 != 0x%02X", table_id );
		return FALSE;
	}

	if( section_length - 4 > 11 ){
		maker_id = data[readSize];
		model_id = data[readSize+1];
		version_number = (data[readSize+2]&0x3E)>>1;
		current_next_indicator = data[readSize+2]&0x01;
		section_number = data[readSize+3];
		last_section_number = data[readSize+4];
		transport_stream_id = ((WORD)data[readSize+5])<<8 | data[readSize+6];
		original_network_id = ((WORD)data[readSize+7])<<8 | data[readSize+8];
		service_id = ((WORD)data[readSize+9])<<8 | data[readSize+10];
		num_of_contents = data[readSize+11];
		readSize += 12;

		for( BYTE i=0; readSize+7 < (DWORD)section_length+3-4 && i<num_of_contents; i++ ){
			CONTENT_INFO_DATA* item = new CONTENT_INFO_DATA;
			item->group = (data[readSize]&0xF0)>>4;
			item->target_version = ((WORD)data[readSize]&0x0F)<<8 | data[readSize+1];
			item->new_version = ((WORD)data[readSize+2])<<4 | (data[readSize+3]&0xF0)>>4;
			item->download_level = (data[readSize+3]&0x0C)>>2;
			item->version_indicator = (data[readSize+3]&0x03);
			item->content_description_length = ((WORD)data[readSize+4])<<4 | (data[readSize+5]&0xF0)>>4;
			item->schedule_description_length = ((WORD)data[readSize+6])<<4 | (data[readSize+7]&0xF0)>>4;
			item->schedule_time_shift_information = data[readSize+7]&0x0F;
			readSize += 8;

			for( WORD j=0; readSize+7 < (DWORD)section_length+3-4 && j<item->schedule_description_length; j+=8){
				SCHEDULE_INFO_DATA time;

				DWORD mjd = ((DWORD)data[readSize])<<8 | data[readSize+1];
				_MJDtoSYSTEMTIME(mjd, &time.start_time);
				time.start_time.wHour = (WORD)_BCDtoDWORD(data+readSize+2, 1, 2);
				time.start_time.wMinute = (WORD)_BCDtoDWORD(data+readSize+3, 1, 2);
				time.start_time.wSecond = (WORD)_BCDtoDWORD(data+readSize+4, 1, 2);
				readSize += 5;

				time.duration = (_BCDtoDWORD(data+readSize, 1, 2)*60*60)
					+ (_BCDtoDWORD(data+readSize+1, 1, 2)*60)
					+ _BCDtoDWORD(data+readSize+2, 1, 2);
				readSize += 3;

				item->scheduleList.push_back(time);
			}

			WORD descLength = item->content_description_length - item->schedule_description_length;
			if( readSize+descLength <= (DWORD)section_length+3-4 && descLength > 0 ){
				if( AribDescriptor::CreateDescriptors( data+readSize, descLength, &item->descriptorList, NULL ) == FALSE ){
					_OutputDebugString( L"++CSDTTTable:: descriptor err" );
					SAFE_DELETE(item);
					return FALSE;
				}
				readSize+=descLength;
			}

			contentInfoList.push_back(item);
		}

	}else{
		return FALSE;
	}

	return TRUE;
}
