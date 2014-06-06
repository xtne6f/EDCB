#include "StdAfx.h"
#include "CATTable.h"

CCATTable::CCATTable(void)
{
}

CCATTable::~CCATTable(void)
{
	Clear();
}

void CCATTable::Clear()
{
	for( size_t i=0 ;i<descriptorList.size(); i++ ){
		SAFE_DELETE(descriptorList[i]);
	}
	descriptorList.clear();
}

BOOL CCATTable::Decode( BYTE* data, DWORD dataSize, DWORD* decodeReadSize )
{
	if( InitDecode(data, dataSize, decodeReadSize, TRUE) == FALSE ){
		return FALSE;
	}
	Clear();

	if( section_syntax_indicator != 1 || (data[1]&0x40) != 0 ){
		//ŒÅ’è’l‚ª‚¨‚©‚µ‚¢
		_OutputDebugString( L"++CCATTable:: section_syntax err" );
		return FALSE;
	}
	if( table_id != 0x01 ){
		//table_id‚ª‚¨‚©‚µ‚¢
		_OutputDebugString( L"++CCATTable:: table_id err 0x01 != 0x%02X", table_id );
		return FALSE;
	}

	if( section_length - 4 > 4 ){
		version_number = (data[readSize+2]&0x3E)>>1;
		current_next_indicator = data[readSize+2]&0x01;
		section_number = data[readSize+3];
		last_section_number = data[readSize+4];
		readSize += 5;
		if( readSize <= (DWORD)section_length+3-4 ){
			DWORD descriptorSize = (DWORD)(section_length+3-4) - readSize;
			if( descriptorSize > 0 ){
				if( AribDescriptor::CreateDescriptors( data+readSize, descriptorSize, &descriptorList, NULL ) == FALSE ){
					_OutputDebugString( L"++CCATTable:: descriptor err" );
					return FALSE;
				}
				readSize+=descriptorSize;
			}
		}
	}else{
		return FALSE;
	}

	return TRUE;
}
