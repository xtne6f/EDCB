#pragma once

class CPESHeadInfo
{
public:
	CPESHeadInfo(void);
	~CPESHeadInfo(void);

	void SetData(BYTE* bData, DWORD dwSize);

public:
	BYTE stream_id;
	WORD PES_packet_length;

	BYTE PES_scrambling_control;
	BYTE PES_priority;
	BYTE data_alignment_indicator;
	BYTE copyright;
	BYTE original_or_copy;
	BYTE PTS_DTS_flags;
	BYTE ESCR_flag;
	BYTE ES_rate_flag;
	BYTE DSM_trick_mode_flag;
	BYTE additional_copy_info_flag;
	BYTE PES_CRC_flag;
	BYTE PES_extension_flag;
	BYTE PES_header_data_length;

	LONGLONG PTS;
	LONGLONG DTS;

	LONGLONG ESCR_base;
	WORD ESCR_extension;
	DWORD ES_rate;

	BYTE trick_mode_control;

	BYTE field_id;
	BYTE intra_slice_refresh;
	BYTE frequency_truncation;
	BYTE rep_cntrl;

	BYTE additional_copy_info;

	WORD previous_PES_packet_CRC;

	BYTE PES_private_data_flag;
	BYTE pack_header_field_flag;
	BYTE program_packet_sequence_counter_flag;
	BYTE P_STD_buffer_flag;
	BYTE PES_extension_flag_2;
	BYTE PES_private_data[16];

	BYTE pack_field_length;

	BYTE program_packet_sequence_counter;
	BYTE MPEG1_MPEG2_identifier;
	BYTE original_stuff_length;

	BYTE P_STD_buffer_scale;
	WORD P_STD_buffer_size;
	BYTE PES_extension_field_length;

	BYTE* PES_packet_data_byte;
	DWORD PES_packet_data_byte_size;
};
