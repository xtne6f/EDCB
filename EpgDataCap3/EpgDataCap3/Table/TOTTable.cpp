#include "StdAfx.h"
#include "TOTTable.h"

#include "../../../Common/EpgTimerUtil.h"

CTOTTable::CTOTTable(void)
{
}

CTOTTable::~CTOTTable(void)
{
}

void CTOTTable::Clear()
{
	for( size_t i=0 ;i<descriptorList.size(); i++ ){
		SAFE_DELETE(descriptorList[i]);
	}
	descriptorList.clear();
}

BOOL CTOTTable::Decode( BYTE* data, DWORD dataSize, DWORD* decodeReadSize )
{
	if( InitDecode(data, dataSize, decodeReadSize, TRUE) == FALSE ){
		return FALSE;
	}
	Clear();

	if( section_syntax_indicator != 0 ){
		//ŒÅ’è’l‚ª‚¨‚©‚µ‚¢
		_OutputDebugString( L"++CTOTTable:: section_syntax err" );
		return FALSE;
	}
	if( table_id != 0x73 ){
		//table_id‚ª‚¨‚©‚µ‚¢
		_OutputDebugString( L"++CTOTTable:: table_id err 0x%02X", table_id );
		return FALSE;
	}

	if( section_length - 4 > 6 ){
		DWORD mjd = ((DWORD)data[readSize])<<8 | data[readSize+1];
		_MJDtoSYSTEMTIME(mjd, &jst_time);
		jst_time.wHour = (WORD)_BCDtoDWORD(data+readSize+2, 1, 2);
		jst_time.wMinute = (WORD)_BCDtoDWORD(data+readSize+3, 1, 2);
		jst_time.wSecond = (WORD)_BCDtoDWORD(data+readSize+4, 1, 2);
		readSize += 5;

		descriptors_loop_length = ((WORD)data[readSize]&0x0F)<<8 | data[readSize+1];
		readSize += 2;

		if( readSize+descriptors_loop_length <= (DWORD)section_length+3-4 && descriptors_loop_length > 0){
			if( AribDescriptor::CreateDescriptors( data+readSize, descriptors_loop_length, &descriptorList, NULL ) == FALSE ){
				_OutputDebugString( L"++CTOTTable:: descriptor2 err" );
				return FALSE;
			}
		}

		readSize+=descriptors_loop_length;
	}else{
		return FALSE;
	}

	return TRUE;
}
