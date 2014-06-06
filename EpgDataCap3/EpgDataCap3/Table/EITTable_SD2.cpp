#include "StdAfx.h"
#include "EITTable_SD2.h"

#include "../../../Common/EpgTimerUtil.h"

CEITTable_SD2::CEITTable_SD2(void)
{
}

CEITTable_SD2::~CEITTable_SD2(void)
{
	Clear();
}

void CEITTable_SD2::Clear()
{
	for( size_t i=0 ;i<eventMapList.size(); i++ ){
		SAFE_DELETE(eventMapList[i]);
	}
	eventMapList.clear();
}

BOOL CEITTable_SD2::Decode( BYTE* data, DWORD dataSize, DWORD* decodeReadSize )
{
	if( InitDecode(data, dataSize, decodeReadSize, TRUE) == FALSE ){
		return FALSE;
	}
	Clear();

	if( section_syntax_indicator != 1 ){
		//ŒÅ’è’l‚ª‚¨‚©‚µ‚¢
		_OutputDebugString( L"++CEITTable_SD2:: section_syntax err" );
		return FALSE;
	}
	if( table_id != 0xA3 && table_id != 0xA2 ){
		//table_id‚ª‚¨‚©‚µ‚¢
		_OutputDebugString( L"++CEITTable_SD2:: table_id err 0x%02X", table_id );
		return FALSE;
	}

	if( section_length - 4 > 18 ){
		service_id = ((WORD)data[readSize])<<8 | data[readSize+1];
		version_number = (data[readSize+2]&0x3E)>>1;
		current_next_indicator = data[readSize+2]&0x01;
		section_number = data[readSize+3];
		last_section_number = data[readSize+4];
		service_id2 = ((WORD)data[readSize+5])<<8 | data[readSize+6];
		original_network_id = ((WORD)data[readSize+7])<<8 | data[readSize+8];

		readSize += 9;

		DWORD mjd = ((DWORD)data[readSize])<<8 | data[readSize+1];
		_MJDtoSYSTEMTIME(mjd, &start_time);
		start_time.wHour = (WORD)_BCDtoDWORD(data+readSize+2, 1, 2);
		start_time.wMinute = (WORD)_BCDtoDWORD(data+readSize+3, 1, 2);
		start_time.wSecond = (WORD)_BCDtoDWORD(data+readSize+4, 1, 2);
		readSize += 5;

		mjd = ((DWORD)data[readSize])<<8 | data[readSize+1];
		_MJDtoSYSTEMTIME(mjd, &end_time);
		end_time.wHour = (WORD)_BCDtoDWORD(data+readSize+2, 1, 2);
		end_time.wMinute = (WORD)_BCDtoDWORD(data+readSize+3, 1, 2);
		end_time.wSecond = (WORD)_BCDtoDWORD(data+readSize+4, 1, 2);
		readSize += 5;

		while( readSize+3 < (DWORD)section_length+3-4 ){
			EVENT_MAP_INFO* item = new EVENT_MAP_INFO;
			item->descriptor_length = ((WORD)data[readSize]&0x03)<<8 | data[readSize+1];

			mjd = ((DWORD)data[readSize+2])<<8 | data[readSize+3];
			_MJDtoSYSTEMTIME(mjd, &item->start_day);

			DWORD readDesc = 4;
			DWORD max = item->descriptor_length;
			max+=2;
			while( readSize+readDesc+6 < (DWORD)section_length+3-4 && readDesc < max ){
				EVENT_MAP_DATA dataInfo;
				dataInfo.event_id = ((WORD)data[readSize+readDesc])<<8 | data[readSize+readDesc+1];
				dataInfo.hour = data[readSize+readDesc+2]>>3;
				dataInfo.minute = (data[readSize+readDesc+2]&0x07)*10;
				dataInfo.minute += data[readSize+readDesc+3]>>4;
				BYTE length = data[readSize+readDesc+3]&0x0F;
				dataInfo.a4table_eventID = ((WORD)data[readSize+readDesc+5])<<8 | data[readSize+readDesc+6];
				if( readSize+readDesc+10 < (DWORD)section_length+3-4 && length == 7 ){
					dataInfo.duration = _BCDtoDWORD(data+readSize+readDesc+8, 1, 2)*60*60;
					dataInfo.duration += _BCDtoDWORD(data+readSize+readDesc+9, 1, 2)*60;
					dataInfo.duration += _BCDtoDWORD(data+readSize+readDesc+10, 1, 2);
				}else{
					dataInfo.duration = 0;
				}
				readDesc+=4+length;
				item->eventList.push_back(dataInfo);
			}

			eventMapList.push_back(item);
			readSize+=item->descriptor_length+2;
		}
	}else{
		return FALSE;
	}

	return TRUE;
}
