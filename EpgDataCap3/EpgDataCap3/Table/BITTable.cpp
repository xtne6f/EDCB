#include "StdAfx.h"
#include "BITTable.h"

CBITTable::CBITTable(void)
{
}

CBITTable::~CBITTable(void)
{
	Clear();
}

void CBITTable::Clear()
{
	for( size_t i=0 ;i<descriptorList.size(); i++ ){
		SAFE_DELETE(descriptorList[i]);
	}
	descriptorList.clear();
	for( size_t i=0 ;i<broadcasterDataList.size(); i++ ){
		SAFE_DELETE(broadcasterDataList[i]);
	}
	broadcasterDataList.clear();
}

BOOL CBITTable::Decode( BYTE* data, DWORD dataSize, DWORD* decodeReadSize )
{
	if( InitDecode(data, dataSize, decodeReadSize, TRUE) == FALSE ){
		return FALSE;
	}
	Clear();

	if( section_syntax_indicator != 1 ){
		//ŒÅ’è’l‚ª‚¨‚©‚µ‚¢
		_OutputDebugString( L"++CBITTable:: section_syntax err" );
		return FALSE;
	}
	if( table_id != 0xC4 ){
		//table_id‚ª‚¨‚©‚µ‚¢
		_OutputDebugString( L"++CBITTable:: table_id err 0x%02X", table_id );
		return FALSE;
	}

	if( section_length - 4 > 6 ){
		original_network_id = ((WORD)data[readSize])<<8 | data[readSize+1];
		version_number = (data[readSize+2]&0x3E)>>1;
		current_next_indicator = data[readSize+2]&0x01;
		section_number = data[readSize+3];
		last_section_number = data[readSize+4];
		broadcast_view_propriety = (data[readSize+5]&0x10)>>4;
		first_descriptors_length = ((WORD)data[readSize+5]&0x0F)<<8 | data[readSize+6];
		readSize += 7;
		if( readSize+first_descriptors_length <= (DWORD)section_length+3-4 && first_descriptors_length > 0){
			if( AribDescriptor::CreateDescriptors( data+readSize, first_descriptors_length, &descriptorList, NULL ) == FALSE ){
				_OutputDebugString( L"++CBITTable:: descriptor err" );
				return FALSE;
			}
			readSize+=first_descriptors_length;
		}
		while( readSize+2 < (DWORD)section_length+3-4 ){
			BROADCASTER_DATA* item = new BROADCASTER_DATA;
			item->broadcaster_id = data[readSize];
			item->broadcaster_descriptors_length = ((WORD)data[readSize+1]&0x0F)<<8 | data[readSize+2];
			readSize+=3;
			if( readSize+item->broadcaster_descriptors_length <= (DWORD)section_length+3-4 && item->broadcaster_descriptors_length > 0){
				if( AribDescriptor::CreateDescriptors( data+readSize, item->broadcaster_descriptors_length, &(item->descriptorList), NULL ) == FALSE ){
					_OutputDebugString( L"++CBITTable:: descriptor2 err" );
					SAFE_DELETE(item);
					return FALSE;
				}
			}

			readSize+=item->broadcaster_descriptors_length;

			broadcasterDataList.push_back(item);
		}
	}else{
		return FALSE;
	}

	return TRUE;
}

