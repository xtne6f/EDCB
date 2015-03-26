#include "StdAfx.h"
#include "PATTable.h"

CPATTable::CPATTable(void)
{
}

CPATTable::~CPATTable(void)
{
}

BOOL CPATTable::Decode( BYTE* data, DWORD dataSize, DWORD* decodeReadSize )
{
	if( InitDecode(data, dataSize, decodeReadSize, TRUE) == FALSE ){
		return FALSE;
	}
	pmtMap.clear();

	if( section_syntax_indicator != 1 || (data[1]&0x40) != 0 ){
		//ŒÅ’è’l‚ª‚¨‚©‚µ‚¢
		_OutputDebugString( L"++CPATTable:: section_syntax err" );
		return FALSE;
	}
	if( table_id != 0x00 ){
		//table_id‚ª‚¨‚©‚µ‚¢
		_OutputDebugString( L"++CPATTable:: table_id err 0x00 != 0x%02X", table_id );
		return FALSE;
	}

	if( section_length - 4 > 4 ){
		transport_stream_id = ((WORD)data[readSize])<<8 | data[readSize+1];
		version_number = (data[readSize+2]&0x3E)>>1;
		current_next_indicator = data[readSize+2]&0x01;
		section_number = data[readSize+3];
		last_section_number = data[readSize+4];
		readSize += 5;
		while( readSize+3 < (DWORD)section_length+3-4 ){
			PMT_DATA item;
			item.program_number = ((WORD)data[readSize])<<8 | data[readSize+1];
			item.PID = ((WORD)data[readSize+2]&0x1F)<<8 | data[readSize+3];

			pmtMap.insert(pair<WORD, PMT_DATA>(item.program_number, item));
			readSize+=4;
		}
	}else{
		return FALSE;
	}

	return TRUE;
}
