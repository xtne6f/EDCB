#include "StdAfx.h"
#include "PSITable.h"

#include "../../../Common/EpgTimerUtil.h"

CPSITable::CPSITable(void)
{
}

CPSITable::~CPSITable(void)
{
}

BOOL CPSITable::Decode( BYTE* data, DWORD dataSize, DWORD* decodeReadSize )
{
	return InitDecode( data, dataSize, decodeReadSize, FALSE );
}

BOOL CPSITable::InitDecode( BYTE* data, DWORD dataSize, DWORD* decodeReadSize, BOOL hasCrc32 )
{
	if( data == NULL || dataSize < 3 ){
		return FALSE;
	}
	//解析処理
	table_id = data[0];
	section_syntax_indicator = (data[1]&0x80)>>7;
	section_length = ((WORD)data[1]&0x0F)<<8 | data[2];

	if( 3 + (DWORD)section_length > dataSize || hasCrc32 != FALSE && section_length < 4 ){
		//サイズ異常
		_OutputDebugString( L"++CPSITable<0x%02x>:: size err %d > %d", table_id, 3 + section_length, dataSize );
		return FALSE;
	}
	//CRCチェック
	if( hasCrc32 != FALSE && _Crc32(3 + section_length, data) != 0 ){
		_OutputDebugString( L"++CPSITable<0x%02x>:: CRC err", table_id );
		return FALSE;
	}
	if( decodeReadSize != NULL ){
		*decodeReadSize = 3 + section_length;
	}
	readSize = 3;
	return TRUE;
}
