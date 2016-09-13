#include "StdAfx.h"
#include "PMTTable.h"

void CPMTTable::Clear()
{
	descriptorList.clear();
	ESInfoList.clear();
}

BOOL CPMTTable::Decode( BYTE* data, DWORD dataSize, DWORD* decodeReadSize )
{
	if( InitDecode(data, dataSize, decodeReadSize, TRUE) == FALSE ){
		return FALSE;
	}
	Clear();

	if( section_syntax_indicator != 1 || (data[1]&0x40) != 0 ){
		//ŒÅ’è’l‚ª‚¨‚©‚µ‚¢
		_OutputDebugString( L"++CPMTTable:: section_syntax err" );
		return FALSE;
	}
	if( table_id != 0x02 ){
		//table_id‚ª‚¨‚©‚µ‚¢
		_OutputDebugString( L"++CPMTTable:: table_id err 0x01 != 0x%02X", table_id );
		return FALSE;
	}

	if( section_length - 4 > 8 ){
		program_number = ((WORD)data[readSize])<<8 | data[readSize+1];
		version_number = (data[readSize+2]&0x3E)>>1;
		current_next_indicator = data[readSize+2]&0x01;
		section_number = data[readSize+3];
		last_section_number = data[readSize+4];
		PCR_PID = ((WORD)data[readSize+5]&0x1F)<<8 | data[readSize+6];
		program_info_length = ((WORD)data[readSize+7]&0x0F)<<8 | data[readSize+8];
		readSize += 9;
		if( readSize+program_info_length <= (DWORD)section_length+3-4 && program_info_length > 0){
			if( AribDescriptor::CreateDescriptors( data+readSize, program_info_length, &descriptorList, NULL ) == FALSE ){
				_OutputDebugString( L"++CPMTTable:: descriptor err" );
				return FALSE;
			}
			readSize+=program_info_length;
		}
		while( readSize+4 < (DWORD)section_length+3-4 ){
			ESInfoList.push_back(ES_INFO_DATA());
			ES_INFO_DATA* item = &ESInfoList.back();
			item->stream_type = data[readSize];
			item->elementary_PID = ((WORD)data[readSize+1]&0x1F)<<8 | data[readSize+2];
			item->ES_info_length = ((WORD)data[readSize+3]&0x0F)<<8 | data[readSize+4];
			readSize += 5;
			if( readSize+item->ES_info_length <= (DWORD)section_length+3-4 && item->ES_info_length > 0){
				if( AribDescriptor::CreateDescriptors( data+readSize, item->ES_info_length, &(item->descriptorList), NULL ) == FALSE ){
					_OutputDebugString( L"++CPMTTable:: descriptor2 err" );
					return FALSE;
				}
			}
			readSize+=item->ES_info_length;
		}
	}else{
		return FALSE;
	}

	return TRUE;
}
