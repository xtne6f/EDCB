#include "StdAfx.h"
#include "SITTable.h"

CSITTable::CSITTable(void)
{
}


CSITTable::~CSITTable(void)
{
	Clear();
}

void CSITTable::Clear()
{
	for( size_t i=0 ;i<descriptorList.size(); i++ ){
		SAFE_DELETE(descriptorList[i]);
	}
	descriptorList.clear();
	for( size_t i=0 ;i<serviceLoopList.size(); i++ ){
		SAFE_DELETE(serviceLoopList[i]);
	}
	serviceLoopList.clear();
}

BOOL CSITTable::Decode( BYTE* data, DWORD dataSize, DWORD* decodeReadSize )
{
	if( InitDecode(data, dataSize, decodeReadSize, TRUE) == FALSE ){
		return FALSE;
	}
	Clear();

	if( section_syntax_indicator != 1 ){
		//ŒÅ’è’l‚ª‚¨‚©‚µ‚¢
		_OutputDebugString( L"++CSITTable:: section_syntax err" );
		return FALSE;
	}
	if( table_id != 0x7F ){
		//table_id‚ª‚¨‚©‚µ‚¢
		_OutputDebugString( L"++CSITTable:: table_id err 0x7F != 0x%02X", table_id );
		return FALSE;
	}

	if( section_length - 4 > 6 ){
		version_number = (data[readSize+2]&0x3E)>>1;
		current_next_indicator = data[readSize+2]&0x01;
		section_number = data[readSize+3];
		last_section_number = data[readSize+4];
		transmission_info_loop_length = ((WORD)data[readSize+5]&0x0F)<<8 | data[readSize+6];
		readSize += 7;
		if( readSize+transmission_info_loop_length <= (DWORD)section_length+3-4 && transmission_info_loop_length > 0){
			if( AribDescriptor::CreateDescriptors( data+readSize, transmission_info_loop_length, &descriptorList, NULL ) == FALSE ){
				_OutputDebugString( L"++CSITTable:: descriptor err" );
				return FALSE;
			}
			readSize+=transmission_info_loop_length;
		}
		while( readSize+3 < (DWORD)section_length+3-4 ){
			SERVICE_LOOP_DATA* item = new SERVICE_LOOP_DATA;
			item->service_id = ((WORD)data[readSize])<<8 | data[readSize+1];
			item->running_status = (data[readSize+2]&0x70)>>4;
			item->service_loop_length = ((WORD)data[readSize+2]&0x0F)<<8 | data[readSize+3];
			readSize += 4;
			if( readSize+item->service_loop_length <= (DWORD)section_length+3-4 && item->service_loop_length > 0){
				if( AribDescriptor::CreateDescriptors( data+readSize, item->service_loop_length, &(item->descriptorList), NULL ) == FALSE ){
					_OutputDebugString( L"++CSITTable:: descriptor2 err" );
					SAFE_DELETE(item);
					return FALSE;
				}
			}
			readSize+=item->service_loop_length;
			serviceLoopList.push_back(item);
		}
	}else{
		return FALSE;
	}

	return TRUE;
}

