#include "StdAfx.h"
#include "TDTTable.h"

#include "../../../Common/EpgTimerUtil.h"

CTDTTable::CTDTTable(void)
{
}

CTDTTable::~CTDTTable(void)
{
}

BOOL CTDTTable::Decode( BYTE* data, DWORD dataSize, DWORD* decodeReadSize )
{
	if( InitDecode(data, dataSize, decodeReadSize, FALSE) == FALSE ){
		return FALSE;
	}

	if( section_syntax_indicator != 0 ){
		//ŒÅ’è’l‚ª‚¨‚©‚µ‚¢
		_OutputDebugString( L"++CTDTTable:: section_syntax err" );
		return FALSE;
	}
	if( table_id != 0x70 ){
		//table_id‚ª‚¨‚©‚µ‚¢
		_OutputDebugString( L"++CTDTTable:: table_id err 0x%02X", table_id );
		return FALSE;
	}

	if( section_length == 5 ){
		DWORD mjd = ((DWORD)data[readSize])<<8 | data[readSize+1];
		_MJDtoSYSTEMTIME(mjd, &jst_time);
		jst_time.wHour = (WORD)_BCDtoDWORD(data+readSize+2, 1, 2);
		jst_time.wMinute = (WORD)_BCDtoDWORD(data+readSize+3, 1, 2);
		jst_time.wSecond = (WORD)_BCDtoDWORD(data+readSize+4, 1, 2);

		readSize += 5;
	}else{
		return FALSE;
	}

	return TRUE;
}

