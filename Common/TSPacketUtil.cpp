#include "stdafx.h"
#include "TSPacketUtil.h"

CTSPacketUtil::CTSPacketUtil(void)
{
	data_byteSize = 0;
	data_byte = nullptr;
}

bool CTSPacketUtil::Set188TS(const BYTE* data, DWORD dataSize)
{
	data_byteSize = 0;
	data_byte = nullptr;

	if( data == nullptr || dataSize != 188 || data[0] != 0x47 ){
		// Not a TS packet
		transport_error_indicator = 0;
		return false;
	}
	transport_error_indicator = GetTransportErrorIndicatorFrom188TS(data);
	if( transport_error_indicator == 1 ){
		return false;
	}
	payload_unit_start_indicator = GetPayloadUnitStartIndicatorFrom188TS(data);
	transport_priority = GetTransportPriorityFrom188TS(data);
	PID = GetPidFrom188TS(data);
	transport_scrambling_control = GetTransportScramblingControlFrom188TS(data);
	adaptation_field_control = GetAdaptationFieldControlFrom188TS(data);
	continuity_counter = GetContinuityCounterFrom188TS(data);
	has_adaptation_field_flags = 0;
	DWORD readSize = 4;

	if( adaptation_field_control == 0x02 || adaptation_field_control == 0x03 ){
		BYTE adaptation_field_length = data[4];
		readSize++;
		if( adaptation_field_length > 0 ){
			has_adaptation_field_flags = 1;
			discontinuity_indicator = (data[readSize]&0x80)>>7;
			random_access_indicator = (data[readSize]&0x40)>>6;
			elementary_stream_priority_indicator = (data[readSize]&0x20)>>5;
			PCR_flag = (data[readSize]&0x10)>>4;
			OPCR_flag = (data[readSize]&0x08)>>3;
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
		}
		readSize = 5+adaptation_field_length;
	}
	if( adaptation_field_control == 0x01 || adaptation_field_control == 0x03 ){
		if( 188 < readSize ){
			return false;
		}
		data_byteSize = (BYTE)(188-readSize);
		if( data_byteSize > 0 ){
			data_byte = data+readSize;
		}
	}
	return true;
}
