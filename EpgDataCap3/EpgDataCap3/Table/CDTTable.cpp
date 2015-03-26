#include "StdAfx.h"
#include "CDTTable.h"

CCDTTable::CCDTTable(void)
{
	data_module_byteSize = 0;
	data_module_byte = NULL;
}

CCDTTable::~CCDTTable(void)
{
}

void CCDTTable::Clear()
{
	for( size_t i=0 ;i<descriptorList.size(); i++ ){
		SAFE_DELETE(descriptorList[i]);
	}
	descriptorList.clear();

	SAFE_DELETE_ARRAY(data_module_byte);
}

BOOL CCDTTable::Decode( BYTE* data, DWORD dataSize, DWORD* decodeReadSize )
{
	if( InitDecode(data, dataSize, decodeReadSize, TRUE) == FALSE ){
		return FALSE;
	}
	Clear();

	if( section_syntax_indicator != 1 ){
		//ŒÅ’è’l‚ª‚¨‚©‚µ‚¢
		_OutputDebugString( L"++CCDTTable:: section_syntax err" );
		return FALSE;
	}
	if( table_id != 0xC8 ){
		//table_id‚ª‚¨‚©‚µ‚¢
		_OutputDebugString( L"++CCDTTable:: table_id err 0xC8 != 0x%02X", table_id );
		return FALSE;
	}

	if( section_length - 4 > 9 ){
		download_data_id = ((WORD)data[readSize])<<8 | data[readSize+1];
		version_number = (data[readSize+2]&0x3E)>>1;
		current_next_indicator = data[readSize+2]&0x01;
		section_number = data[readSize+3];
		last_section_number = data[readSize+4];
		original_network_id = ((WORD)data[readSize+5])<<8 | data[readSize+6];
		data_type = data[readSize+7];
		descriptors_loop_length = ((WORD)data[readSize+8]&0x0F)<<8 | data[readSize+9];
		readSize += 10;

		if( readSize+descriptors_loop_length <= (DWORD)section_length+3-4 && descriptors_loop_length > 0 ){
			if( AribDescriptor::CreateDescriptors( data+readSize, descriptors_loop_length, &descriptorList, NULL ) == FALSE ){
				_OutputDebugString( L"++CCDTTable:: descriptor err" );
				return FALSE;
			}
			readSize+=descriptors_loop_length;
		}

		data_module_byteSize = (3+section_length-4) - (WORD)readSize;
		if( data_module_byteSize > 0 ){
			data_module_byte = new BYTE[data_module_byteSize];
			memcpy(data_module_byte, data+readSize, data_module_byteSize);
		}
	}else{
		return FALSE;
	}

	return TRUE;
}