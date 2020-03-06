#include "stdafx.h"
#include "TSPacketUtil.h"

CTSPacketUtil::CTSPacketUtil(void)
{
	data_byteSize = 0;
	data_byte = NULL;
}

BOOL CTSPacketUtil::Set188TS(BYTE* data, DWORD dataSize)
{
	if( data == NULL || dataSize != 188){
		return FALSE;
	}
	data_byteSize = 0;

	DWORD readSize = 0;
	sync_byte = data[0];
	if( sync_byte != 0x47 ){
		return FALSE;
	}
	transport_error_indicator = (data[1]&0x80)>>7;
	if( transport_error_indicator == 1 ){
		return FALSE;
	}
	payload_unit_start_indicator = (data[1]&0x40)>>6;
	transport_priority = (data[1]&0x20)>>5;
	PID = ((WORD)data[1]&0x1F)<<8 | data[2];
	transport_scrambling_control = (data[3]&0xC0)>>6;
	adaptation_field_control = (data[3]&0x30)>>4;
	continuity_counter = data[3]&0x0F;
	readSize+=4;

	if( adaptation_field_control == 0x02 || adaptation_field_control == 0x03 ){
		adaptation_field_length = data[4];
		readSize++;
		if( adaptation_field_length > 0 ){
			discontinuity_indicator = (data[readSize]&0x80)>>7;
			random_access_indicator = (data[readSize]&0x40)>>6;
			elementary_stream_priority_indicator = (data[readSize]&0x20)>>5;
			PCR_flag = (data[readSize]&0x10)>>4;
			OPCR_flag = (data[readSize]&0x08)>>3;
			splicing_point_flag = (data[readSize]&0x04)>>2;
			transport_private_data_flag = (data[readSize]&0x02)>>1;
			adaptation_field_extension_flag = (data[readSize]&0x01);
			readSize++;
			if( PCR_flag == 1 ){
				program_clock_reference_base = ((ULONGLONG)data[readSize])<<25 |
					((ULONGLONG)data[readSize+1])<<17 |
					((ULONGLONG)data[readSize+2])<<9 |
					((ULONGLONG)data[readSize+3])<<1 |
					((ULONGLONG)data[readSize+4]&0x80)>>7;
				program_clock_reference_extension = ((ULONGLONG)data[readSize+4]&0x01)<<8 | data[readSize+5];
				readSize+=6;
			}
			if( OPCR_flag == 1 ){
				original_program_clock_reference_base = ((ULONGLONG)data[readSize])<<25 |
					((ULONGLONG)data[readSize+1])<<17 |
					((ULONGLONG)data[readSize+2])<<9 |
					((ULONGLONG)data[readSize+3])<<1 |
					((ULONGLONG)data[readSize+4]&0x80)>>7;
				original_program_clock_reference_extension = ((ULONGLONG)data[readSize+4]&0x01)<<8 | data[readSize+5];
				readSize+=6;
			}
			if( splicing_point_flag == 1 ){
				splice_countdown = data[readSize];
				readSize++;
			}
			if( transport_private_data_flag == 1 ){
				transport_private_data_length = data[readSize];
				readSize+=1+transport_private_data_length;
			}
			if( adaptation_field_extension_flag == 1 ){
				if( readSize + 1 >= 188 ){
					return FALSE;
				}
				adaptation_field_extension_length = data[readSize];
				ltw_flag = (data[readSize+1]&0x80)>>7;
				piecewise_rate_flag = (data[readSize+1]&0x40)>>6;
				seamless_splice_flag = (data[readSize+1]&0x20)>>5;
				readSize+=2;
				if( ltw_flag == 1 ){
					if( readSize + 1 >= 188 ){
						return FALSE;
					}
					ltw_valid_flag = (data[readSize]&0x80)>>7;
					ltw_offset = ((WORD)data[readSize]&0x7F)<<8 | data[readSize+1];
					readSize+=2;
				}
				if( piecewise_rate_flag == 1 ){
					if( readSize + 2 >= 188 ){
						return FALSE;
					}
					piecewise_rate = ((DWORD)data[readSize]&0x3F)<<16 |
						((DWORD)data[readSize+1])<<8 |
						data[readSize+2];
					readSize += 3;
				}
				if( seamless_splice_flag == 1 ){
					if( readSize + 4 >= 188 ){
						return FALSE;
					}
					splice_type = (data[readSize]&0xF0)>>4;
					DTS_next_AU = ((ULONGLONG)data[readSize]&0x0E)<< 29|
						((ULONGLONG)data[readSize+1])<< 22|
						((ULONGLONG)data[readSize+2]&0xFE)<<14 |
						((ULONGLONG)data[readSize+3])<<7 |
						((ULONGLONG)data[readSize+4]&0xFE)>>1;
					readSize+=5;
				}
			}
		}else{
			discontinuity_indicator = 0;
			random_access_indicator = 0;
			elementary_stream_priority_indicator = 0;
			PCR_flag = 0;
			OPCR_flag = 0;
			splicing_point_flag = 0;
			transport_private_data_flag = 0;
			adaptation_field_extension_flag = 0;
		}
		readSize = 5+adaptation_field_length;
	}else{
		adaptation_field_length = 0;
		discontinuity_indicator = 0;
		random_access_indicator = 0;
		elementary_stream_priority_indicator = 0;
		PCR_flag = 0;
		OPCR_flag = 0;
		splicing_point_flag = 0;
		transport_private_data_flag = 0;
		adaptation_field_extension_flag = 0;
	}
	if( adaptation_field_control == 0x01 || adaptation_field_control == 0x03 ){
		if( 188 < readSize ){
			return FALSE;
		}
		data_byteSize = (BYTE)(188-readSize);
		if( data_byteSize > 0 ){
			data_byte = data+readSize;
		}
	}
	return TRUE;
}
