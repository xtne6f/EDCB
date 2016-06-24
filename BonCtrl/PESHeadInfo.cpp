#include "StdAfx.h"
#include "PESHeadInfo.h"

CPESHeadInfo::CPESHeadInfo(void)
{
	stream_id = 0;
	PES_packet_length = 0;

	PES_scrambling_control = 0;
	PES_priority = 0;
	data_alignment_indicator = 0;
	copyright = 0;
	original_or_copy = 0;
	PTS_DTS_flags = 0;
	ESCR_flag = 0;
	ES_rate_flag = 0;
	DSM_trick_mode_flag = 0;
	additional_copy_info_flag = 0;
	PES_CRC_flag = 0;
	PES_extension_flag = 0;
	PES_header_data_length = 0;

	PTS = 0;
	DTS = 0;

	ESCR_base = 0;
	ESCR_extension = 0;
	ES_rate = 0;

	trick_mode_control = 0;

	field_id = 0;
	intra_slice_refresh = 0;
	frequency_truncation = 0;
	rep_cntrl = 0;

	additional_copy_info = 0;

	previous_PES_packet_CRC = 0;

	PES_private_data_flag = 0;
	pack_header_field_flag = 0;
	program_packet_sequence_counter_flag = 0;
	P_STD_buffer_flag = 0;
	PES_extension_flag_2 = 0;

	pack_field_length = 0;

	program_packet_sequence_counter = 0;
	MPEG1_MPEG2_identifier = 0;
	original_stuff_length = 0;

	P_STD_buffer_scale = 0;
	P_STD_buffer_size = 0;
	PES_extension_field_length = 0;

	PES_packet_data_byte = NULL;
	PES_packet_data_byte_size = 0;
}

CPESHeadInfo::~CPESHeadInfo(void)
{
}

void CPESHeadInfo::SetData(BYTE* bData, DWORD dwSize)
{
	if( bData == NULL ){
		return ;
	}

	DWORD dwRead=0;

	//packet_start_code_prefix
	dwRead+=3;

	stream_id = bData[dwRead];
	dwRead++;

	PES_packet_length = ((WORD)bData[dwRead])<<8 | bData[dwRead+1];
	dwRead+=2;
	if( stream_id != 0xBC //program_stream_map
		&& stream_id != 0xBE //padding_stream
		&& stream_id != 0xBF //private_stream_2
		&& stream_id != 0xF0 //ECM
		&& stream_id != 0xF1 //EMM
		&& stream_id != 0xFF //program_stream_directory
		&& stream_id != 0xF2 //DSMCC_stream
		&& stream_id != 0xF8 // ITU-T Rec. H.222.1 type E_stream
		){
			PES_scrambling_control = (bData[dwRead]&0x30)>>4;
			PES_priority = (bData[dwRead]&0x08)>>3;
			data_alignment_indicator = (bData[dwRead]&0x04)>>2;
			copyright = (bData[dwRead]&0x02)>>1;
			original_or_copy = bData[dwRead]&0x01;
			dwRead++;

			PTS_DTS_flags = (bData[dwRead]&0xC0)>>6;
			ESCR_flag = (bData[dwRead]&0x20)>>5;
			ES_rate_flag = (bData[dwRead]&0x10)>>4;
			DSM_trick_mode_flag = (bData[dwRead]&0x08)>>3;
			additional_copy_info_flag = (bData[dwRead]&0x04)>>2;
			PES_CRC_flag = (bData[dwRead]&0x02)>>1;
			PES_extension_flag = bData[dwRead]&0x01;
			dwRead++;

			PES_header_data_length = bData[dwRead];
			dwRead++;

			//DWORD dwHeadEnd = dwRead+PES_header_data_length;

			if( PTS_DTS_flags == 0x02 ){
				PTS = ((__int64)bData[dwRead]&0x0E) << 29 |
					((__int64)bData[dwRead+1]) << 22 |
					((__int64)bData[dwRead+2]&0xFE) << 14 |
					((__int64)bData[dwRead+3]) << 7 |
					((__int64)bData[dwRead+4]&0xFE) >>1;
				dwRead+=5;
			}
			if( PTS_DTS_flags == 0x03 ){
				PTS = ((__int64)bData[dwRead]&0x0E) << 29 |
					((__int64)bData[dwRead+1]) << 22 |
					((__int64)bData[dwRead+2]&0xFE) << 14 |
					((__int64)bData[dwRead+3]) << 7 |
					((__int64)bData[dwRead+4]&0xFE) >>1;
				dwRead+=5;
				DTS = ((__int64)bData[dwRead]&0x0E) << 29 |
					((__int64)bData[dwRead+1]) << 22 |
					((__int64)bData[dwRead+2]&0xFE) << 14 |
					((__int64)bData[dwRead+3]) << 7 |
					((__int64)bData[dwRead+4]&0xFE) >>1;
				dwRead+=5;
			}
			if( ESCR_flag == 0x01 ){
				ESCR_base = ((__int64)bData[dwRead]&0x38) << 26 |
					((__int64)bData[dwRead]&0x03) << 27 |
					((__int64)bData[dwRead+1]) << 19 |
					((__int64)bData[dwRead+2]&0xF8) << 14 |
					((__int64)bData[dwRead+2]&0x03) << 12 |
					((__int64)bData[dwRead+3]) << 4 |
					((__int64)bData[dwRead+4]&0xF8) >>4;
				ESCR_extension = ((WORD)bData[dwRead+4]&0x03) << 7 |
					((WORD)bData[dwRead+5]&0xFE) >> 1;
				dwRead+=6;
			}
			if( ES_rate_flag == 0x01 ){
				ES_rate = ((DWORD)bData[dwRead]&0x7F) << 15|
					(DWORD)bData[dwRead+1] << 7|
					((DWORD)bData[dwRead+2]&0xFE) >> 1;
				dwRead+=3;
			}
			if( DSM_trick_mode_flag == 1 ){
				trick_mode_control = (bData[dwRead]&0xE0)>>5;
				if( trick_mode_control == 0x00 ){//fast forward
					field_id = (bData[dwRead]&0x18)>>3;
					intra_slice_refresh = (bData[dwRead]&0x04)>>2;
					frequency_truncation = bData[dwRead]&0x03;
				}else if(trick_mode_control == 0x01 ){//slow motion
					rep_cntrl = bData[dwRead]&0x1F;
				}else if(trick_mode_control == 0x02 ){//freeze frame
					field_id = (bData[dwRead]&0x18)>>3;
				}else if(trick_mode_control == 0x03 ){//fast reverse
					field_id = (bData[dwRead]&0x18)>>3;
					intra_slice_refresh = (bData[dwRead]&0x04)>>2;
					frequency_truncation = bData[dwRead]&0x03;
				}else if(trick_mode_control == 0x04 ){//slow reverse
					rep_cntrl = bData[dwRead]&0x1F;
				}
				dwRead++;
			}
			if ( additional_copy_info_flag == 1 ) {
				additional_copy_info = bData[dwRead]&0x7F;
				dwRead++;
			}
			if ( PES_CRC_flag == 1 ) {
				previous_PES_packet_CRC = ((WORD)bData[dwRead])<<8 | bData[dwRead+1];
				dwRead+=2;
			}
			if ( PES_extension_flag == 1 ) {
				PES_private_data_flag = (bData[dwRead]&0x80)>>7;
				pack_header_field_flag = (bData[dwRead]&0x40)>>6;
				program_packet_sequence_counter_flag = (bData[dwRead]&0x20)>>5;
				P_STD_buffer_flag = (bData[dwRead]&0x10)>>4;
				PES_extension_flag_2 = bData[dwRead]&0x01;
				dwRead++;
				if( PES_private_data_flag == 1 ){
					memcpy(PES_private_data, bData+dwRead, 16);
					dwRead+=16;
				}
				if( pack_header_field_flag == 1 ){
					pack_field_length = bData[dwRead];
					dwRead++;
					dwRead+= pack_field_length;
				}
				if( program_packet_sequence_counter_flag == 1 ){
					program_packet_sequence_counter = bData[dwRead]&0x7F;
					dwRead++;
					MPEG1_MPEG2_identifier = (bData[dwRead]&0x40)>>6;
					original_stuff_length = bData[dwRead]&0x3F;
				}
				if( P_STD_buffer_flag == 1 ){
					P_STD_buffer_scale = (bData[dwRead]&0x20)>>5;
					P_STD_buffer_size = ((WORD)bData[dwRead]&0x1F)<<8 | bData[dwRead+1];
					dwRead+=2;
				}
				if( PES_extension_flag_2 == 1 ){
					PES_extension_field_length = bData[dwRead]&0x7F;
					dwRead++;
					dwRead+=PES_extension_field_length;
				}
			}
			//stuffing_byte
//			dwRead = dwHeadEnd;
			if( PES_packet_length == 0 ){
				PES_packet_data_byte_size = dwSize-dwRead;
			}else{
				PES_packet_data_byte_size = PES_packet_length-(dwRead-6);
			}
			PES_packet_data_byte = bData + dwRead;
	}else if(
		stream_id == 0xBC || //program_stream_map
		stream_id == 0xBF || //private_stream_2
		stream_id == 0xF0 || //ECM
		stream_id == 0xF1 || //EMM
		stream_id == 0xFF || //program_stream_directory
		stream_id == 0xF2 || //DSMCC_stream
		stream_id == 0xF8 //ITU-T Rec. H.222.1 type E stream
		){
			if( PES_packet_length == 0 ){
				PES_packet_data_byte_size = dwSize-dwRead;
			}else{
				PES_packet_data_byte_size = PES_packet_length;
			}
			PES_packet_data_byte = bData + dwRead;
	}else if( 
		stream_id == 0xBE //padding_stream
		){
			dwRead = dwSize;
	}
}
