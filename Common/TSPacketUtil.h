#pragma once

/*
transport_packet(){
	sync_byte						8 bslbf
	transport_error_indicator		1 bslbf
	payload_unit_start_indicator	1 bslbf
	transport_priority				1 bslbf
	PID								13 uimsbf
	transport_scrambling_control	2 bslbf
	adaptation_field_control		2 bslbf
	continuity_counter				4 uimsbf
	if(adaptation_field_control = = '10' || adaptation_field_control = = '11'){
		adaptation_field()
	}
	if(adaptation_field_control = = '01' || adaptation_field_control = = '11') {
		for (i = 0; i < N; i++){
			data_byte					8 bslbf
		}
	}
}

adaptation_field() {
	adaptation_field_length					8 uimsbf
	if (adaptation_field_length > 0) {
		discontinuity_indicator						1 bslbf
		random_access_indicator						1 bslbf
		elementary_stream_priority_indicator		1 bslbf
		PCR_flag									1 bslbf
		OPCR_flag									1 bslbf
		splicing_point_flag							1 bslbf
		transport_private_data_flag					1 bslbf
		adaptation_field_extension_flag				1 bslbf
		if (PCR_flag = = '1') {
			program_clock_reference_base				33 uimsbf
			reserved									6 bslbf
			program_clock_reference_extension			9 uimsbf
		}
		if (OPCR_flag = = '1') {
			original_program_clock_reference_base		33 uimsbf
			reserved									6 bslbf
			original_program_clock_reference_extension	9 uimsbf
		}
		if (splicing_point_flag = = '1') {
			splice_countdown							8 tcimsbf
		}
		if (transport_private_data_flag = = '1') {
			transport_private_data_length				8 uimsbf
			for (i = 0; i < transport_private_data_length; i++) {
				private_data_byte							8 bslbf
			}
		}
		if (adaptation_field_extension_flag = = '1') {
			adaptation_field_extension_length			8 uimsbf
			ltw_flag									1 bslbf
			piecewise_rate_flag							1 bslbf
			seamless_splice_flag						1 bslbf
			reserved									5 bslbf
			if (ltw_flag = = '1') {
				ltw_valid_flag								1 bslbf
				ltw_offset									15 uimsbf
			}
			if (piecewise_rate_flag = = '1') {
				reserved									2 bslbf
				piecewise_rate								22 uimsbf
			}
			if (seamless_splice_flag = = '1') {
				splice_type									4 bslbf
				DTS_next_AU[32..30]							3 bslbf
				marker_bit									1 bslbf
				DTS_next_AU[29..15]							15 bslbf
				marker_bit									1 bslbf
				DTS_next_AU[14..0]							15 bslbf
				marker_bit									1 bslbf
			}
			for (i = 0; i < N; i++) {
				reserved									8 bslbf
			}
		}
		for (i = 0; i < N; i++) {
			stuffing_byte								8 bslbf
		}
	}
}

*/

class CTSPacketUtil
{
public:
	BYTE sync_byte;
	BYTE transport_error_indicator;
	BYTE payload_unit_start_indicator;
	BYTE transport_priority;
	WORD PID;
	BYTE transport_scrambling_control;
	BYTE adaptation_field_control;
	BYTE continuity_counter;
	BYTE data_byteSize;
	BYTE* data_byte;

	BYTE adaptation_field_length;
	BYTE discontinuity_indicator;
	BYTE random_access_indicator;
	BYTE elementary_stream_priority_indicator;
	BYTE PCR_flag;
	BYTE OPCR_flag;
	BYTE splicing_point_flag;
	BYTE transport_private_data_flag;
	BYTE adaptation_field_extension_flag;
	ULONGLONG program_clock_reference_base;
	WORD program_clock_reference_extension;
	ULONGLONG original_program_clock_reference_base;
	WORD original_program_clock_reference_extension;
	BYTE splice_countdown;
	BYTE transport_private_data_length;
//	BYTE* private_data_byte;

	BYTE adaptation_field_extension_length;
	BYTE ltw_flag;
	BYTE piecewise_rate_flag;
	BYTE seamless_splice_flag;
	BYTE ltw_valid_flag;
	WORD ltw_offset;
	DWORD piecewise_rate;
	BYTE splice_type;
	ULONGLONG DTS_next_AU;

public:
	CTSPacketUtil(void);

	BOOL Set188TS(BYTE* data, DWORD dataSize);
};
