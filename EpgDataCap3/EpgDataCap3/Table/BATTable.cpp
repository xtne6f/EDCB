#include "StdAfx.h"
#include "BATTable.h"

CBATTable::CBATTable(void)
{
}

CBATTable::~CBATTable(void)
{
	Clear();
}

void CBATTable::Clear()
{
	for( size_t i=0 ;i<descriptorList.size(); i++ ){
		SAFE_DELETE(descriptorList[i]);
	}
	descriptorList.clear();
	for( size_t i=0 ;i<TSInfoList.size(); i++ ){
		SAFE_DELETE(TSInfoList[i]);
	}
	TSInfoList.clear();
}

BOOL CBATTable::Decode( BYTE* data, DWORD dataSize, DWORD* decodeReadSize )
{
	if( InitDecode(data, dataSize, decodeReadSize, TRUE) == FALSE ){
		return FALSE;
	}
	Clear();

	if( section_syntax_indicator != 1 ){
		//ŒÅ’è’l‚ª‚¨‚©‚µ‚¢
		_OutputDebugString( L"++CBATTable:: section_syntax err" );
		return FALSE;
	}
	if( table_id != 0x4A ){
		//table_id‚ª‚¨‚©‚µ‚¢
		_OutputDebugString( L"++CBATTable:: table_id err 0x%02X", table_id );
		return FALSE;
	}

	if( section_length - 4 > 8 ){
		bouquet_id = ((WORD)data[readSize])<<8 | data[readSize+1];
		version_number = (data[readSize+2]&0x3E)>>1;
		current_next_indicator = data[readSize+2]&0x01;
		section_number = data[readSize+3];
		last_section_number = data[readSize+4];
		bouquet_descriptors_length = ((WORD)data[readSize+5]&0x0F)<<8 | data[readSize+6];
		readSize += 7;
		if( readSize+bouquet_descriptors_length <= (DWORD)section_length+3-4 && bouquet_descriptors_length > 0){
			if( AribDescriptor::CreateDescriptors( data+readSize, bouquet_descriptors_length, &descriptorList, NULL ) == FALSE ){
				_OutputDebugString( L"++CBATTable:: descriptor err" );
				return FALSE;
			}
			readSize+=bouquet_descriptors_length;
		}
		transport_stream_loop_length = ((WORD)data[readSize]&0x0F)<<8 | data[readSize+1];
		readSize += 2;
		WORD tsLoopReadSize = 0;
		while( readSize+5 < (DWORD)section_length+3-4 && tsLoopReadSize < transport_stream_loop_length){
			TS_INFO_DATA* item = new TS_INFO_DATA;
			item->transport_stream_id = ((WORD)data[readSize])<<8 | data[readSize+1];
			item->original_network_id = ((WORD)data[readSize+2])<<8 | data[readSize+3];
			item->transport_descriptors_length = ((WORD)data[readSize+4]&0x0F)<<8 | data[readSize+5];
			readSize += 6;
			if( readSize+item->transport_descriptors_length <= (DWORD)section_length+3-4 && item->transport_descriptors_length > 0){
				if( AribDescriptor::CreateDescriptors( data+readSize, item->transport_descriptors_length, &(item->descriptorList), NULL ) == FALSE ){
					_OutputDebugString( L"++CBATTable:: descriptor2 err" );
					SAFE_DELETE(item);
					return FALSE;
				}
			}

			readSize+=item->transport_descriptors_length;
			tsLoopReadSize += 6 + item->transport_descriptors_length;

			TSInfoList.push_back(item);
		}
	}else{
		return FALSE;
	}

	return TRUE;
}

