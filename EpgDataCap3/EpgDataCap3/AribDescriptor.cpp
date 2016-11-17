#include "StdAfx.h"
#include "AribDescriptor.h"
#include "../../Common/EpgTimerUtil.h"

namespace AribDescriptor
{

//�EPSI/SI�̋L�q�q���p�[�X����Ȉ�DSL
//�EARIB STD-B10���̍\���̂������������̂܂܏�����
//
//���l: 0�`0x1EFF�B�P�ʂ͊�{�I��bit
//�Q��: 0x2000�`�B�����Ƃ��ĉ��߂����Ƃ��̒P�ʂ͊�{�I��byte
//D_FIN:                �p�[�T�I�[
//D_END:                D_BEGIN*�����
//D_BEGIN,{���l|�Q��}:  D_END�܂ŉߕs���Ȃ�{���l|�Q��}�T�C�Y�������͂������
//D_BEGIN_SUB,{���l|�Q��},{���l}: D_END�܂ŉߕs���Ȃ�{���l|�Q��}-{���l}�T�C�Y�������͂������
//D_BEGIN_IF,{�Q��},{���lL},{���lR}: {�Q��}��{���lL}�ȏ�{���lR}�ȉ��̏ꍇ��D_END�܂ł̋�Ԃ�L���ɂ���
//D_BEGIN_IF_NOT,{�Q��},{���lL},{���lR}: D_BEGIN_IF�̔ے�
//D_BEGIN_FOR,{�Q��}:   �v���p�e�B�̃��[�v���쐬����{�Q��}�񂾂��J��Ԃ�
//D_BEGIN_FOR_TO_END:   �v���p�e�B�̃��[�v���쐬���Ăł��邾�����͂������悤�ɌJ��Ԃ�
//D_DESCRIPTOR_LOOP:    �v���p�e�B�̃��[�v���쐬���ċL�q�q���[�v�Ƃ��ăp�[�X���A�ł��邾�����͂������
//{�Q��},{���l|�Q��}:          ���̓f�[�^��{���l|�Q��}�T�C�Y����{�Q��}��DWORD�l�Ƃ��Ċi�[
//{�Q��},D_LOCAL,{���l|�Q��}:  DWORD�l�Ƃ��Ċi�[���p�[�X��Ɏ̂Ă�
//{�Q��},D_LOCAL_TO_END:       D_LOCAL�Ɠ��l�����A�ł��邾�����͂������
//{�Q��},D_STRING,{���l|�Q��}: ���̓f�[�^��{���l|�Q��}�T�C�Y����{�Q��}�ɕ�����Ƃ��Ċi�[
//{�Q��},D_STRING_TO_END:      D_STRING�Ɠ��l�����A�ł��邾�����͂������
//{�Q��},D_BINARY,{���l|�Q��}: ���̓f�[�^��{���l|�Q��}�T�C�Y����{�Q��}�Ƀo�C�g��Ƃ��Ċi�[
//{�Q��},D_BINARY_TO_END:      D_BINARY�Ɠ��l�����A�ł��邾�����͂������
//
//��D_BEGIN*,D_DESCRIPTOR_LOOP,D_STRING*,D_BINARY*�̓o�C�g(8bit)���E�ł̂ݎg�p�ł���

const short audio_component_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length,
		reserved, D_LOCAL, 4,
		stream_content, 4,
		component_type, 8,
		component_tag, 8,
		stream_type, 8,
		simulcast_group_tag, 8,
		ES_multi_lingual_flag, 1,
		main_component_flag, 1,
		quality_indicator, 2,
		sampling_rate, 3,
		reserved, D_LOCAL, 1,
		ISO_639_language_code, D_STRING, 24,
		D_BEGIN_IF, ES_multi_lingual_flag, 1, 1,
			ISO_639_language_code_2, D_STRING, 24,
		D_END,
		text_char, D_STRING_TO_END,
	D_END,
	D_FIN,
};
const short AVC_timing_and_HRD_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length,
		hrd_management_valid_flag, 1,
		reserved, D_LOCAL, 6,
		picture_and_timing_info_present, 1,
		D_BEGIN_IF, picture_and_timing_info_present, 1, 1,
			d_90kHz_flag, 1,
			reserved, D_LOCAL, 7,
			D_BEGIN_IF, d_90kHz_flag, 0, 0,
				d_N, 32,
				d_K, 32,
			D_END,
			num_units_in_tick, 32,
		D_END,
		fixed_frame_rate_flag, 1,
		temporal_poc_flag, 1,
		picture_to_display_conversion_flag, 1,
		reserved, D_LOCAL, 5,
	D_END,
	D_FIN,
};
const short AVC_video_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length,
		profile_idc, 8,
		constraint_set0_flag, 1,
		constraint_set1_flag, 1,
		constraint_set2_flag, 1,
		AVC_compatible_flags, 5,
		level_idc, 8,
		AVC_still_present, 1,
		AVC_24_hour_picture_flag, 1,
		reserved, D_LOCAL, 6,
	D_END,
	D_FIN,
};
const short board_information_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length,
		title_length, D_LOCAL, 8,
		title_char, D_STRING, title_length,
		text_length, D_LOCAL, 8,
		text_char, D_STRING, text_length,
	D_END,
	D_FIN,
};
const short bouquet_name_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	d_char, D_STRING, descriptor_length,
	D_FIN,
};
const short broadcaster_name_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	d_char, D_STRING, descriptor_length,
	D_FIN,
};
const short CA_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length,
		CA_system_ID, 16,
		reserved, D_LOCAL, 3,
		CA_PID, 13,
		private_data_byte, D_BINARY_TO_END,
	D_END,
	D_FIN,
};
const short CA_identifier_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length, D_BEGIN_FOR_TO_END,
		CA_system_ID, 16,
	D_END, D_END,
	D_FIN,
};
const short component_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length,
		reserved, D_LOCAL, 4,
		stream_content, 4,
		component_type, 8,
		component_tag, 8,
		ISO_639_language_code, D_STRING, 24,
		text_char, D_STRING_TO_END,
	D_END,
	D_FIN,
};
const short component_group_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length,
		component_group_type, 3,
		total_bit_rate_flag, 1,
		num_of_group, 4,
		D_BEGIN_FOR, num_of_group,
			component_group_id, 4,
			num_of_CA_unit, 4,
			D_BEGIN_FOR, num_of_CA_unit,
				CA_unit_id, 4,
				num_of_component, D_LOCAL, 4,
				component_tag, D_BINARY, num_of_component,
			D_END,
			D_BEGIN_IF, total_bit_rate_flag, 1, 1,
				total_bit_rate, 8,
			D_END,
			text_length, D_LOCAL, 8,
			text_char, D_STRING, text_length,
		D_END,
	D_END,
	D_FIN,
};
const short connected_transmission_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length,
		connected_transmission_group_id, 16,
		segment_type, 2,
		modulation_type_A, 2,
		modulation_type_B, 2,
		reserved, D_LOCAL, 2,
		additional_connected_transmission_info, D_BINARY_TO_END,
	D_END,
	D_FIN,
};
const short content_availability_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length,
		reserved, D_LOCAL, 1,
		copy_restriction_mode, 1,
		image_constraint_token, 1,
		retention_mode, 1,
		retention_state, 3,
		encryption_mode, 1,
		reserved, D_LOCAL_TO_END,
	D_END,
	D_FIN,
};
const short content_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length, D_BEGIN_FOR_TO_END,
		content_nibble_level_1, 4,
		content_nibble_level_2, 4,
		user_nibble_1, 4,
		user_nibble_2, 4,
	D_END, D_END,
	D_FIN,
};
const short country_availability_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length,
		country_availability_flag, 1,
		reserved, D_LOCAL, 7,
		D_BEGIN_FOR_TO_END,
			country_code, D_STRING, 24,
		D_END,
	D_END,
	D_FIN,
};
const short data_component_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length,
		data_component_id, 16,
		additional_data_component_info, D_BINARY_TO_END,
	D_END,
	D_FIN,
};
const short data_content_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length,
		data_component_id, 16,
		entry_component, 8,
		selector_length, D_LOCAL, 8,
		selector_byte, D_BINARY, selector_length,
		num_of_component_ref, D_LOCAL, 8,
		component_ref, D_BINARY, num_of_component_ref,
		ISO_639_language_code, D_STRING, 24,
		text_length, D_LOCAL, 8,
		text_char, D_STRING, text_length,
	D_END,
	D_FIN,
};
const short digital_copy_control_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length,
		digital_recording_control_data, 2,
		maximum_bitrate_flag, 1,
		component_control_flag, 1,
		user_defined, 4,
		D_BEGIN_IF, maximum_bitrate_flag, 1, 1,
			maximum_bitrate, 8,
		D_END,
		D_BEGIN_IF, component_control_flag, 1, 1,
			component_control_length, D_LOCAL, 8,
			D_BEGIN, component_control_length, D_BEGIN_FOR_TO_END,
				component_tag, 8,
				digital_recording_control_data, 2,
				maximum_bitrate_flag, 1,
				reserved, D_LOCAL, 1,
				user_defined, 4,
				D_BEGIN_IF, maximum_bitrate_flag, 1, 1,
					maximum_bitrate, 8,
				D_END,
			D_END, D_END,
		D_END,
	D_END,
	D_FIN,
};
const short Download_content_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length,
		reboot, 1,
		add_on, 1,
		compatibility_flag, 1,
		module_info_flag, 1,
		text_info_flag, 1,
		reserved, D_LOCAL, 3,
		component_size, 32,
		download_id, 32,
		time_out_value_DII, 32,
		leak_rate, 22,
		reserved, D_LOCAL, 2,
		component_tag, 8,
		D_BEGIN_IF, compatibility_flag, 1, 1,
			//compatibilityDescriptor()
			compatibility_descriptor_length, D_LOCAL, 16,
			compatibility_descriptor_byte, compatibility_descriptor_length,
		D_END,
		D_BEGIN_IF, module_info_flag, 1, 1,
			num_of_modules, 16,
			D_BEGIN_FOR, num_of_modules,
				module_id, 16,
				module_size, 32,
				module_info_length, D_LOCAL, 8,
				module_info_byte, D_BINARY, module_info_length,
			D_END,
		D_END,
		private_data_length, D_LOCAL, 8,
		private_data_byte, D_BINARY, private_data_length,
		D_BEGIN_IF, text_info_flag, 1, 1,
			ISO_639_language_code, D_STRING, 24,
			text_length, D_LOCAL, 8,
			text_char, D_STRING, text_length,
		D_END,
	D_END,
	D_FIN,
};
const short emergency_information_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length, D_BEGIN_FOR_TO_END,
		service_id, 16,
		start_end_flag, 1,
		signal_level, 1,
		reserved, D_LOCAL, 6,
		area_code_length, D_LOCAL, 8,
		D_BEGIN, area_code_length, D_BEGIN_FOR_TO_END,
			area_code, 12,
			reserved, D_LOCAL, 4,
		D_END, D_END,
	D_END, D_END,
	D_FIN,
};
const short event_group_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length,
		group_type, 4,
		event_count, 4,
		D_BEGIN_FOR, event_count,
			service_id, 16,
			event_id, 16,
		D_END,
		D_BEGIN_IF, group_type, 4, 5,
			D_BEGIN_FOR_TO_END,
				original_network_id, 16,
				transport_stream_id, 16,
				service_id, 16,
				event_id, 16,
			D_END,
		D_END,
		D_BEGIN_IF_NOT, group_type, 4, 5,
			private_data_byte, D_BINARY_TO_END,
		D_END,
	D_END,
	D_FIN,
};
const short extended_broadcaster_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length,
		broadcaster_type, 4,
		reserved, D_LOCAL, 4,
		D_BEGIN_IF, broadcaster_type, 1, 1,
			terrestrial_broadcaster_id, 16,
			number_of_affiliation_id_loop, 4,
			number_of_broadcaster_id_loop, 4,
			affiliation_id, D_BINARY, number_of_affiliation_id_loop,
			D_BEGIN_FOR, number_of_broadcaster_id_loop,
				original_network_id, 16,
				broadcaster_id, 8,
			D_END,
			private_data_byte, D_BINARY_TO_END,
		D_END,
		D_BEGIN_IF, broadcaster_type, 2, 2,
			terrestrial_sound_broadcaster_id, 16,
			number_of_sound_broadcast_affiliation_id_loop, 4,
			number_of_broadcaster_id_loop, 4,
			sound_broadcast_affiliation_id, D_BINARY, number_of_sound_broadcast_affiliation_id_loop,
			D_BEGIN_FOR, number_of_broadcaster_id_loop,
				original_network_id, 16,
				broadcaster_id, 8,
			D_END,
			private_data_byte, D_BINARY_TO_END,
		D_END,
		D_BEGIN_IF_NOT, broadcaster_type, 1, 2,
			reserved, D_LOCAL_TO_END,
		D_END,
	D_END,
	D_FIN,
};
const short extended_event_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length,
		descriptor_number, 4,
		last_descriptor_number, 4,
		ISO_639_language_code, D_STRING, 24,
		length_of_items, D_LOCAL, 8,
		D_BEGIN, length_of_items, D_BEGIN_FOR_TO_END,
			item_description_length, D_LOCAL, 8,
			item_description_char, D_STRING, item_description_length,
			item_length, D_LOCAL, 8,
			item_char, D_STRING, item_length,
		D_END, D_END,
		text_length, D_LOCAL, 8,
		text_char, D_STRING, text_length,
	D_END,
	D_FIN,
};
const short hierarchical_transmission_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length,
		reserved, D_LOCAL, 7,
		quality_level, 1,
		reserved, D_LOCAL, 3,
		reference_PID, 13,
	D_END,
	D_FIN,
};
const short hyperlink_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length,
		hyper_linkage_type, 8,
		link_destination_type, 8,
		selector_length, D_LOCAL, 8,
		D_BEGIN, selector_length,
			D_BEGIN_IF, link_destination_type, 1, 6,
				D_BEGIN_FOR_TO_END,
					D_BEGIN_IF, link_destination_type, 1, 5,
						original_network_id, 16,
						transport_stream_id, 16,
						service_id, 16,
						D_BEGIN_IF, link_destination_type, 2, 2,
							event_id, 16,
						D_END,
						D_BEGIN_IF, link_destination_type, 3, 3,
							event_id, 16,
							component_tag, 8,
							module_id, 16,
						D_END,
						D_BEGIN_IF, link_destination_type, 4, 4,
							content_id, 32,
						D_END,
						D_BEGIN_IF, link_destination_type, 5, 5,
							content_id, 32,
							component_tag, 8,
							module_id, 16,
						D_END,
					D_END,
					D_BEGIN_IF, link_destination_type, 6, 6,
						information_provider_id, 16,
						event_relation_id, 16,
						node_id, 16,
					D_END,
				D_END,
			D_END,
			D_BEGIN_IF, link_destination_type, 7, 7,
				uri_char, D_STRING_TO_END,
			D_END,
			D_BEGIN_IF_NOT, link_destination_type, 1, 7,
				reserved, D_LOCAL_TO_END,
			D_END,
		D_END,
		private_data, D_BINARY_TO_END,
	D_END,
	D_FIN,
};
const short LDT_linkage_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length,
		original_service_id, 16,
		transport_stream_id, 16,
		original_network_id, 16,
		D_BEGIN_FOR_TO_END,
			description_id, 16,
			reserved, D_LOCAL, 4,
			description_type, 4,
			user_defined, 8,
		D_END,
	D_END,
	D_FIN,
};
const short linkage_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length,
		transport_stream_id, 16,
		original_network_id, 16,
		service_id, 16,
		linkage_type, 8,
		private_data_byte, D_BINARY_TO_END,
	D_END,
	D_FIN,
};
const short local_time_offset_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length, D_BEGIN_FOR_TO_END,
		country_code, D_STRING, 24,
		country_region_id, 6,
		reserved, D_LOCAL, 1,
		local_time_offset_polarity, 1,
		local_time_offset, 16,
		time_of_change, D_BINARY, 40,
		next_time_offset, 16,
	D_END, D_END,
	D_FIN,
};
const short logo_transmission_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length,
		logo_transmission_type, 8,
		D_BEGIN_IF, logo_transmission_type, 1, 1,
			reserved, D_LOCAL, 7,
			logo_id, 9,
			reserved, D_LOCAL, 4,
			logo_version, 12,
			download_data_id, 16,
		D_END,
		D_BEGIN_IF, logo_transmission_type, 2, 2,
			reserved, D_LOCAL, 7,
			logo_id, 9,
		D_END,
		D_BEGIN_IF, logo_transmission_type, 3, 3,
			logo_char, D_BINARY_TO_END,
		D_END,
		D_BEGIN_IF_NOT, logo_transmission_type, 1, 3,
			reserved, D_LOCAL_TO_END,
		D_END,
	D_END,
	D_FIN,
};
const short mosaic_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length,
		mosaic_entry_point, 1,
		number_of_horizontal_elementary_cells, 3,
		reserved, D_LOCAL, 1,
		number_of_vertical_elementary_cells, 3,
		D_BEGIN_FOR_TO_END,
			logical_cell_id, 6,
			reserved, D_LOCAL, 7,
			logical_cell_presentation_info, 3,
			elementary_cell_field_length, 8,
			D_BEGIN_FOR, elementary_cell_field_length,
				reserved, D_LOCAL, 2,
				elementary_cell_id, 6,
			D_END,
			cell_linkage_info, 8,
			D_BEGIN_IF, cell_linkage_info, 1, 1,
				bouquet_id, 16,
			D_END,
			D_BEGIN_IF, cell_linkage_info, 2, 4,
				original_network_id, 16,
				transport_stream_id, 16,
				service_id, 16,
				D_BEGIN_IF, cell_linkage_info, 4, 4,
					event_id, 16,
				D_END,
			D_END,
		D_END,
	D_END,
	D_FIN,
};
const short network_identification_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length,
		country_code, D_STRING, 24,
		media_type, 16,
		network_id, 16,
		private_data, D_BINARY_TO_END,
	D_END,
	D_FIN,
};
const short network_name_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	d_char, D_STRING, descriptor_length,
	D_FIN,
};
const short NVOD_reference_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length, D_BEGIN_FOR_TO_END,
		transport_stream_id, 16,
		original_network_id, 16,
		service_id, 16,
	D_END, D_END,
	D_FIN,
};
const short parental_rating_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length, D_BEGIN_FOR_TO_END,
		country_code, D_STRING, 24,
		rating, 8,
	D_END, D_END,
	D_FIN,
};
const short partial_reception_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length, D_BEGIN_FOR_TO_END,
		service_id, 16,
	D_END, D_END,
	D_FIN,
};
const short partial_transport_stream_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length,
		reserved, D_LOCAL, 2,
		peak_rate, 22,
		reserved, D_LOCAL, 2,
		minimum_overall_smoothing_rate, 22,
		reserved, D_LOCAL, 2,
		maximum_overall_smoothing_buffer, 14,
	D_END,
	D_FIN,
};
const short partialTS_time_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length,
		event_version_number, 8,
		event_start_time, D_BINARY, 40,
		d_duration, 24,
		d_offset, 24,
		reserved, D_LOCAL, 5,
		offset_flag, 1,
		other_descriptor_status, 1,
		jst_time_flag, 1,
		D_BEGIN_IF, jst_time_flag, 1, 1,
			jst_time, D_BINARY, 40,
		D_END,
	D_END,
	D_FIN,
};
const short satellite_delivery_system_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length,
		d_frequency, 32,
		orbital_position, 16,
		west_east_flag, 1,
		polarisation, 2,
		modulation, 5,
		symbol_rate, 28,
		FEC_inner, 4,
	D_END,
	D_FIN,
};
const short series_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length,
		series_id, 16,
		repeat_label, 4,
		program_pattern, 3,
		expire_date_valid_flag, 1,
		expire_date, 16,
		episode_number, 12,
		last_episode_number, 12,
		series_name_char, D_STRING_TO_END,
	D_END,
	D_FIN,
};
const short service_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length,
		service_type, 8,
		service_provider_name_length, D_LOCAL, 8,
		service_provider_name, D_STRING, service_provider_name_length,
		service_name_length, D_LOCAL, 8,
		service_name, D_STRING, service_name_length,
	D_END,
	D_FIN,
};
const short service_group_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length,
		service_group_type, 4,
		reserved, D_LOCAL, 4,
		D_BEGIN_IF, service_group_type, 1, 1,
			D_BEGIN_FOR_TO_END,
				primary_service_id, 16,
				secondary_service_id, 16,
			D_END,
		D_END,
		D_BEGIN_IF_NOT, service_group_type, 1, 1,
			private_data_byte, D_BINARY_TO_END,
		D_END,
	D_END,
	D_FIN,
};
const short service_list_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length, D_BEGIN_FOR_TO_END,
		service_id, 16,
		service_type, 8,
	D_END, D_END,
	D_FIN,
};
const short short_event_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length,
		ISO_639_language_code, D_STRING, 24,
		event_name_length, D_LOCAL, 8,
		event_name_char, D_STRING, event_name_length,
		text_length, D_LOCAL, 8,
		text_char, D_STRING, text_length,
	D_END,
	D_FIN,
};
const short SI_parameter_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length,
		parameter_version, 8,
		update_time, 16,
		D_BEGIN_FOR_TO_END,
			table_id, 8,
			table_description_length, D_LOCAL, 8,
			table_description_byte, D_BINARY, table_description_length,
		D_END,
	D_END,
	D_FIN,
};
const short SI_prime_ts_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length,
		parameter_version, 8,
		update_time, 16,
		SI_prime_ts_network_id, 16,
		SI_prime_transport_stream_id, 16,
		D_BEGIN_FOR_TO_END,
			table_id, 8,
			table_description_length, D_LOCAL, 8,
			table_description_byte, D_BINARY, table_description_length,
		D_END,
	D_END,
	D_FIN,
};
const short stream_identifier_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length,
		component_tag, 8,
	D_END,
	D_FIN,
};
const short stuffing_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	stuffing_byte, D_LOCAL, descriptor_length,
	D_FIN,
};
const short system_management_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length,
		system_management_id, 16,
		additional_identification_info, D_BINARY_TO_END,
	D_END,
	D_FIN,
};
const short target_region_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length,
		region_spec_type, 8,
		D_BEGIN_IF, region_spec_type, 1, 1,
			prefecture_bitmap, D_BINARY, 56,
		D_END,
	D_END,
	D_FIN,
};
const short terrestrial_delivery_system_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length,
		area_code, 12,
		guard_interval, 2,
		transmission_mode, 2,
		D_BEGIN_FOR_TO_END,
			d_frequency, 16,
		D_END,
	D_END,
	D_FIN,
};
const short time_shifted_event_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length,
		reference_service_id, 16,
		reference_event_id, 16,
	D_END,
	D_FIN,
};
const short time_shifted_service_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length,
		reference_service_id, 16,
	D_END,
	D_FIN,
};
const short ts_information_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length,
		remote_control_key_id, 8,
		length_of_ts_name, D_LOCAL, 6,
		transmission_type_count, 2,
		ts_name_char, D_STRING, length_of_ts_name,
		D_BEGIN_FOR, transmission_type_count,
			transmission_type_info, 8,
			num_of_service, 8,
			D_BEGIN_FOR, num_of_service,
				service_id, 16,
			D_END,
		D_END,
		reserved, D_LOCAL_TO_END,
	D_END,
	D_FIN,
};
const short unknown_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	reserved, D_LOCAL, descriptor_length,
	D_FIN,
};
const short video_decode_control_descriptor_p[] = {
	descriptor_tag, 8,
	descriptor_length, D_LOCAL, 8,
	D_BEGIN, descriptor_length,
		still_picture_flag, 1,
		sequence_end_code_flag, 1,
		video_encode_format, 4,
		reserved, D_LOCAL, 2,
	D_END,
	D_FIN,
};

//����: ���p����Ȃ��L�q�q�̓R�����g�A�E�g���Ă���(unknown_descriptor����)
const PARSER_PAIR parserMap[] = {
	//{ CA_descriptor,							CA_descriptor_p },
	//{ AVC_video_descriptor,					AVC_video_descriptor_p },
	//{ AVC_timing_and_HRD_descriptor,			AVC_timing_and_HRD_descriptor_p },
	{ network_name_descriptor,					network_name_descriptor_p },
	{ service_list_descriptor,					service_list_descriptor_p },
	//{ stuffing_descriptor,					stuffing_descriptor_p },
	//{ satellite_delivery_system_descriptor,	satellite_delivery_system_descriptor_p },
	//{ bouquet_name_descriptor,				bouquet_name_descriptor_p },
	{ service_descriptor,						service_descriptor_p },
	//{ country_availability_descriptor,		country_availability_descriptor_p },
	//{ linkage_descriptor,						linkage_descriptor_p },
	//{ NVOD_reference_descriptor,				NVOD_reference_descriptor_p },
	//{ time_shifted_service_descriptor,		time_shifted_service_descriptor_p },
	{ short_event_descriptor,					short_event_descriptor_p },
	{ extended_event_descriptor,				extended_event_descriptor_p },
	//{ time_shifted_event_descriptor,			time_shifted_event_descriptor_p },
	{ component_descriptor,						component_descriptor_p },
	//{ mosaic_descriptor,						mosaic_descriptor_p },
	//{ stream_identifier_descriptor,			stream_identifier_descriptor_p },
	//{ CA_identifier_descriptor,				CA_identifier_descriptor_p },
	{ content_descriptor,						content_descriptor_p },
	//{ parental_rating_descriptor,				parental_rating_descriptor_p },
	//{ local_time_offset_descriptor,			local_time_offset_descriptor_p },
	//{ partial_transport_stream_descriptor,	partial_transport_stream_descriptor_p },
	//{ hierarchical_transmission_descriptor,	hierarchical_transmission_descriptor_p },
	//{ digital_copy_control_descriptor,		digital_copy_control_descriptor_p },
	{ network_identification_descriptor,		network_identification_descriptor_p },
	{ partialTS_time_descriptor,				partialTS_time_descriptor_p },
	{ audio_component_descriptor,				audio_component_descriptor_p },
	//{ hyperlink_descriptor,					hyperlink_descriptor_p },
	//{ target_region_descriptor,				target_region_descriptor_p },
	//{ data_content_descriptor,				data_content_descriptor_p },
	//{ video_decode_control_descriptor,		video_decode_control_descriptor_p },
	//{ Download_content_descriptor,			Download_content_descriptor_p },
	{ ts_information_descriptor,				ts_information_descriptor_p },
	//{ extended_broadcaster_descriptor,		extended_broadcaster_descriptor_p },
	//{ logo_transmission_descriptor,			logo_transmission_descriptor_p },
	//{ series_descriptor,						series_descriptor_p },
	{ event_group_descriptor,					event_group_descriptor_p },
	//{ SI_parameter_descriptor,				SI_parameter_descriptor_p },
	//{ broadcaster_name_descriptor,			broadcaster_name_descriptor_p },
	//{ component_group_descriptor,				component_group_descriptor_p },
	//{ SI_prime_ts_descriptor,					SI_prime_ts_descriptor_p },
	//{ board_information_descriptor,			board_information_descriptor_p },
	//{ LDT_linkage_descriptor,					LDT_linkage_descriptor_p },
	//{ connected_transmission_descriptor,		connected_transmission_descriptor_p },
	//{ content_availability_descriptor,		content_availability_descriptor_p },
	//{ service_group_descriptor,				service_group_descriptor_p },
	//{ terrestrial_delivery_system_descriptor,	terrestrial_delivery_system_descriptor_p },
	{ partial_reception_descriptor,				partial_reception_descriptor_p },
	//{ emergency_information_descriptor,		emergency_information_descriptor_p },
	//{ data_component_descriptor,				data_component_descriptor_p },
	//{ system_management_descriptor,			system_management_descriptor_p },
	{ 0xFF,										unknown_descriptor_p },
};

const short program_association_section_p[] = {
	table_id, 8,
	section_syntax_indicator, D_LOCAL, 1,
	reserved, D_LOCAL, 3,
	section_length, D_LOCAL, 12,
	D_BEGIN_IF, section_syntax_indicator, 1, 1,
		D_BEGIN_SUB, section_length, 32,
			transport_stream_id, 16,
			reserved, D_LOCAL, 2,
			version_number, 5,
			current_next_indicator, 1,
			section_number, 8,
			last_section_number, 8,
			D_BEGIN_FOR_TO_END,
				program_number, 16,
				reserved, D_LOCAL, 3,
				program_map_PID, 13,
			D_END,
		D_END,
		CRC_32, 32,
	D_END,
	D_FIN,
};

const short network_information_section_p[] = {
	table_id, 8,
	section_syntax_indicator, D_LOCAL, 1,
	reserved, D_LOCAL, 3,
	section_length, D_LOCAL, 12,
	D_BEGIN_IF, section_syntax_indicator, 1, 1,
		D_BEGIN_SUB, section_length, 32,
			network_id, 16,
			reserved, D_LOCAL, 2,
			version_number, 5,
			current_next_indicator, 1,
			section_number, 8,
			last_section_number, 8,
			reserved, D_LOCAL, 4,
			network_descriptors_length, D_LOCAL, 12,
			D_BEGIN, network_descriptors_length,
				D_DESCRIPTOR_LOOP,
			D_END,
			reserved, D_LOCAL, 4,
			transport_stream_loop_length, D_LOCAL, 12,
			D_BEGIN, transport_stream_loop_length,
				D_BEGIN_FOR_TO_END,
					transport_stream_id, 16,
					original_network_id, 16,
					reserved, D_LOCAL, 4,
					transport_descriptors_length, D_LOCAL, 12,
					D_BEGIN, transport_descriptors_length,
						D_DESCRIPTOR_LOOP,
					D_END,
				D_END,
			D_END,
		D_END,
		CRC_32, 32,
	D_END,
	D_FIN,
};

const short service_description_section_p[] = {
	table_id, 8,
	section_syntax_indicator, D_LOCAL, 1,
	reserved, D_LOCAL, 3,
	section_length, D_LOCAL, 12,
	D_BEGIN_IF, section_syntax_indicator, 1, 1,
		D_BEGIN_SUB, section_length, 32,
			transport_stream_id, 16,
			reserved, D_LOCAL, 2,
			version_number, 5,
			current_next_indicator, 1,
			section_number, 8,
			last_section_number, 8,
			original_network_id, 16,
			reserved, D_LOCAL, 8,
			D_BEGIN_FOR_TO_END,
				service_id, 16,
				reserved, D_LOCAL, 3,
				EIT_user_defined_flags, 3,
				EIT_schedule_flag, 1,
				EIT_present_following_flag, 1,
				running_status, 3,
				free_CA_mode, 1,
				descriptors_loop_length, D_LOCAL, 12,
				D_BEGIN, descriptors_loop_length,
					D_DESCRIPTOR_LOOP,
				D_END,
			D_END,
		D_END,
		CRC_32, 32,
	D_END,
	D_FIN,
};

const short event_information_section_p[] = {
	table_id, 8,
	section_syntax_indicator, D_LOCAL, 1,
	reserved, D_LOCAL, 3,
	section_length, D_LOCAL, 12,
	D_BEGIN_IF, section_syntax_indicator, 1, 1,
		D_BEGIN_SUB, section_length, 32,
			service_id, 16,
			reserved, D_LOCAL, 2,
			version_number, 5,
			current_next_indicator, 1,
			section_number, 8,
			last_section_number, 8,
			transport_stream_id, 16,
			original_network_id, 16,
			segment_last_section_number, 8,
			last_table_id, 8,
			D_BEGIN_FOR_TO_END,
				event_id, 16,
				start_time_mjd, 16,
				start_time_bcd, 24,
				d_duration,  24,
				running_status, 3,
				free_CA_mode, 1,
				descriptors_loop_length, D_LOCAL, 12,
				D_BEGIN, descriptors_loop_length,
					D_DESCRIPTOR_LOOP,
				D_END,
			D_END,
		D_END,
		CRC_32, 32,
	D_END,
	D_FIN,
};

const short time_date_section_p[] = {
	table_id, 8,
	section_syntax_indicator, D_LOCAL, 1,
	reserved, D_LOCAL, 3,
	section_length, D_LOCAL, 12,
	D_BEGIN_IF, section_syntax_indicator, 0, 0,
		D_BEGIN, section_length,
			jst_time_mjd, 16,
			jst_time_bcd, 24,
		D_END,
	D_END,
	D_FIN,
};

const short time_offset_section_p[] = {
	table_id, 8,
	section_syntax_indicator, D_LOCAL, 1,
	reserved, D_LOCAL, 3,
	section_length, D_LOCAL, 12,
	D_BEGIN_IF, section_syntax_indicator, 0, 0,
		D_BEGIN_SUB, section_length, 32,
			jst_time_mjd, 16,
			jst_time_bcd, 24,
			reserved, D_LOCAL, 4,
			descriptors_loop_length, D_LOCAL, 12,
			D_BEGIN, descriptors_loop_length,
				D_DESCRIPTOR_LOOP,
			D_END,
		D_END,
		CRC_32, 32,
	D_END,
	D_FIN,
};

const short selection_information_section_p[] = {
	table_id, 8,
	section_syntax_indicator, D_LOCAL, 1,
	reserved, D_LOCAL, 3,
	section_length, D_LOCAL, 12,
	D_BEGIN_IF, section_syntax_indicator, 1, 1,
		D_BEGIN_SUB, section_length, 32,
			reserved, D_LOCAL, 16,
			reserved, D_LOCAL, 2,
			version_number, 5,
			current_next_indicator, 1,
			section_number, 8,
			last_section_number, 8,
			reserved, D_LOCAL, 4,
			transmission_info_loop_length, D_LOCAL, 12,
			D_BEGIN, transmission_info_loop_length,
				D_DESCRIPTOR_LOOP,
			D_END,
			D_BEGIN_FOR_TO_END,
				service_id, 16,
				reserved, D_LOCAL, 1,
				running_status, 3,
				service_loop_length, 12, D_LOCAL, 12,
				D_BEGIN, service_loop_length,
					D_DESCRIPTOR_LOOP,
				D_END,
			D_END,
		D_END,
		CRC_32, 32,
	D_END,
	D_FIN,
};

const short broadcaster_information_section_p[] = {
	table_id, 8,
	section_syntax_indicator, D_LOCAL, 1,
	reserved, D_LOCAL, 3,
	section_length, D_LOCAL, 12,
	D_BEGIN_IF, section_syntax_indicator, 1, 1,
		D_BEGIN_SUB, section_length, 32,
			original_network_id, 16,
			reserved, D_LOCAL, 2,
			version_number, 5,
			current_next_indicator, 1,
			section_number, 8,
			last_section_number, 8,
			reserved, D_LOCAL, 3,
			broadcast_view_propriety, 1,
			first_descriptors_length, D_LOCAL, 12,
			D_BEGIN, first_descriptors_length,
				D_DESCRIPTOR_LOOP,
			D_END,
			D_BEGIN_FOR_TO_END,
				broadcaster_id, 8,
				reserved, D_LOCAL, 4,
				broadcaster_descriptors_length, D_LOCAL, 12,
				D_BEGIN, broadcaster_descriptors_length,
					D_DESCRIPTOR_LOOP,
				D_END,
			D_END,
		D_END,
		CRC_32, 32,
	D_END,
	D_FIN,
};

const short* const sectionParserList[] = {
	program_association_section_p,
	network_information_section_p,
	service_description_section_p,
	event_information_section_p,
	time_date_section_p,
	time_offset_section_p,
	selection_information_section_p,
	broadcaster_information_section_p,
};

void CDescriptor::Clear()
{
	this->rootProperty.clear();
}

bool CDescriptor::DecodeSI(const BYTE* data, DWORD dataSize, DWORD* decodeReadSize, si_type type, const PARSER_PAIR* customParserList)
{
	Clear();

	if( data == NULL ){
		return false;
	}

	//SI�^�ɑΉ�����p�[�T��I��
	const short* parser = sectionParserList[type];

	//���[�J���Q�Ɨp�X�^�b�N
	LOCAL_PROPERTY localProperty[128];
	localProperty->id = D_FIN;
	localProperty->type = _countof(localProperty);
	localProperty->n = 1;

	int readSize = DecodeProperty(data, dataSize, &parser, &this->rootProperty, localProperty, customParserList);
	if( readSize < 0 || this->rootProperty.size() < 2 ){
		if( readSize == -3 ){
			//���̏��������������Ƃ��̓p�[�T�Ƀ~�X������
			OutputDebugString(L"CDescriptor::DecodeProperty: Parser syntax error\r\n");
		}else if( readSize == -4 ){
			//�L�q�q���[�v�Ɉُ킪����
			OutputDebugString(L"CDescriptor::DecodeProperty: Invalid descriptor loop error\r\n");
		}
		//���͂��p�[�X�ł��Ȃ�
		Clear();
		return false;
	}
	if( Has(CRC_32) && _Crc32(readSize, data) != 0 ){
		OutputDebugString(L"CDescriptor::DecodeSI: CRC32 error\r\n");
		Clear();
		return false;
	}
	if( decodeReadSize != NULL ){
		*decodeReadSize = readSize;
	}
	return true;
}

DWORD CDescriptor::GetDecodeReadSize(const BYTE* data, DWORD dataSize)
{
	if( data == NULL || dataSize < 2 || data[1] > dataSize - 2 ){
		return 0;
	}
	return 2 + data[1];
}

bool CDescriptor::Decode(const BYTE* data, DWORD dataSize, DWORD* decodeReadSize, const PARSER_PAIR* customParserList)
{
	Clear();

	if( data == NULL || dataSize == 0 ){
		return false;
	}

	//�L�q�q�^�O�ɑΉ�����p�[�T��T��(customParserList��D��)
	const short* parser = NULL;
	if( customParserList != NULL ){
		for( int i = 0; customParserList[i].parser != NULL; ++i ){
			if( customParserList[i].tag == data[0] ){
				parser = customParserList[i].parser;
				break;
			}
		}
	}
	if( parser == NULL ){
		for( int i = 0; parserMap[i].parser != unknown_descriptor_p; ++i ){
			if( parserMap[i].tag == data[0] ){
				parser = parserMap[i].parser;
				break;
			}
		}
		if( parser == NULL ){
			parser = unknown_descriptor_p;
		}
	}

	//���[�J���Q�Ɨp�X�^�b�N
	LOCAL_PROPERTY localProperty[128];
	localProperty->id = D_FIN;
	localProperty->type = _countof(localProperty);
	localProperty->n = 1;

	int readSize = DecodeProperty(data, dataSize, &parser, &this->rootProperty, localProperty, customParserList);
	if( readSize < 0 ){
		if( readSize == -3 ){
			//���̏��������������Ƃ��̓p�[�T�Ƀ~�X������
			OutputDebugString(L"CDescriptor::DecodeProperty: Parser syntax error\r\n");
		}
		Clear();
		return false;
	}
	if( decodeReadSize != NULL ){
		*decodeReadSize = readSize;
	}
	return true;
}

int CDescriptor::DecodeProperty(const BYTE* data, DWORD dataSize, const short** parser, vector<DESCRIPTOR_PROPERTY>* pp, LOCAL_PROPERTY* ppLocal, const PARSER_PAIR* customParserList)
{
	DWORD readSize = 0;
	DWORD bitOffset = 0;
	short dpID;

	while( **parser != D_FIN && **parser != D_END ){
		switch( **parser ){
		case D_BEGIN:
		case D_BEGIN_SUB:
			{
				if( bitOffset != 0 ){
					return -3;
				}
				++*parser;
				DWORD subSize = GetOperand(**parser, ppLocal) / 8;
				++*parser;
				if( *(*parser - 2) == D_BEGIN_SUB ){
					if( subSize < (DWORD)**parser / 8 ){
						//����T�C�Y�����ɂȂ�B���̃G���[�͉񕜂ł��Ȃ�
						return -2;
					}
					subSize -= **parser / 8;
					++*parser;
				}
				if( readSize + subSize > dataSize ){
					return -1;
				}
				int subReadSize = DecodeProperty(data + readSize, subSize, parser, pp, ppLocal, customParserList);
				if( subReadSize < 0 ){
					return subReadSize;
				}
				if( (DWORD)subReadSize < subSize ){
					//�����f�[�^��������Ȃ��B���̃G���[�͉񕜂ł��Ȃ�
					return -2;
				}
				readSize += subReadSize;
			}
			break;
		case D_BEGIN_IF:
		case D_BEGIN_IF_NOT:
			{
				if( bitOffset != 0 ){
					return -3;
				}
				bool bNot = **parser == D_BEGIN_IF_NOT;
				++*parser;
				DWORD val = GetOperand(**parser, ppLocal) / 8;
				++*parser;
				DWORD exprL = **parser;
				++*parser;
				DWORD exprR = **parser;
				++*parser;
				if( bNot && !(exprL <= val && val <= exprR) || !bNot && (exprL <= val && val <= exprR) ){
					int subReadSize = DecodeProperty(data + readSize, dataSize - readSize, parser, pp, ppLocal, customParserList);
					if( subReadSize < 0 ){
						return subReadSize;
					}
					readSize += subReadSize;
				}else{
					//D_END�܂ňړ�
					for( int n = 1; n > 0; ++*parser ){
						if( D_BEGIN <= **parser && **parser <= D_BEGIN_FOR_TO_END ){
							++n;
						}else if( **parser == D_END ){
							--n;
						}
					}
				}
			}
			break;
		case D_BEGIN_FOR:
		case D_BEGIN_FOR_TO_END:
			{
				if( bitOffset != 0 ){
					return -3;
				}
				pp->resize(pp->size() + 1);
				DESCRIPTOR_PROPERTY& dp = pp->back();
				dp.id = 0;
				dp.type = DESCRIPTOR_PROPERTY::TYPE_P;
				dp.pl = new vector<vector<DESCRIPTOR_PROPERTY>>;

				int loopNum = -1;
				if( **parser == D_BEGIN_FOR ){
					loopNum = GetOperand(*(++*parser), ppLocal) / 8;
					dp.pl->reserve(loopNum);
				}
				++*parser;

				for( ; loopNum != 0; --loopNum ){
					dp.pl->resize(dp.pl->size() + 1);
					const short* parserRollback = *parser;
					DWORD localRollback = ppLocal->n;
					int subReadSize = DecodeProperty(data + readSize, dataSize - readSize, parser, &dp.pl->back(), ppLocal, customParserList);
					ppLocal->n = localRollback;
					*parser = parserRollback;

					if( subReadSize < 0 ){
						dp.pl->pop_back();
						if( subReadSize < -1 ){
							return subReadSize;
						}
						break;
					}
					readSize += subReadSize;
				}
				if( loopNum > 0 || loopNum < 0 && readSize < dataSize ){
					//�w��̃��[�v�񐔂ɒB���Ȃ����f�[�^������������B���̃G���[�͉񕜂ł��Ȃ�
					return -2;
				}
				//D_END�܂ňړ�
				for( int n = 1; n > 0; ++*parser ){
					if( D_BEGIN <= **parser && **parser <= D_BEGIN_FOR_TO_END ){
						++n;
					}else if( **parser == D_END ){
						--n;
					}
				}
			}
			break;
		case D_DESCRIPTOR_LOOP:
			{
				if( bitOffset != 0 ){
					return -3;
				}
				pp->resize(pp->size() + 1);
				DESCRIPTOR_PROPERTY& dp = pp->back();
				dp.id = 0;
				dp.type = DESCRIPTOR_PROPERTY::TYPE_P;
				dp.pl = new vector<vector<DESCRIPTOR_PROPERTY>>;
				DWORD reserveCount = 0;
				for( DWORD n, m = readSize; (n = CDescriptor::GetDecodeReadSize(data + m, dataSize - m)) != 0; m += n, ++reserveCount );
				dp.pl->reserve(reserveCount);
				++*parser;

				CDescriptor desc;
				while( readSize < dataSize ){
					DWORD subReadSize;
					if( !desc.Decode(data + readSize, dataSize - readSize, &subReadSize, customParserList) ){
						//�L�q�q���ُ�B���̃G���[�͉񕜂ł��Ȃ�
						return -4;
					}
					dp.pl->resize(dp.pl->size() + 1);
					dp.pl->back().swap(desc.rootProperty);
					readSize += subReadSize;
				}
				if( readSize < dataSize ){
					//�f�[�^������������B���̃G���[�͉񕜂ł��Ȃ�
					return -2;
				}
			}
			break;
		default:
			dpID = **parser;
			++*parser;

			switch( **parser ){
			case D_STRING:
			case D_STRING_TO_END:
				{
					if( bitOffset != 0 ){
						return -3;
					}
					DWORD byteSize = dataSize - readSize;
					if( **parser == D_STRING ){
						byteSize = GetOperand(*(++*parser), ppLocal) / 8;
					}
					++*parser;
					if( readSize + byteSize > dataSize ){
						return -1;
					}
					DWORD copySize = min(byteSize, 0xFFF);
					pp->resize(pp->size() + 1);
					DESCRIPTOR_PROPERTY& dp = pp->back();
					dp.id = dpID;
					dp.type = (short)(DESCRIPTOR_PROPERTY::TYPE_S | copySize);
					if( copySize < sizeof(dp.s) ){
						//���u��
						memcpy(dp.s, data + readSize, copySize);
						dp.s[copySize] = '\0';
					}else{
						dp.ps = new char[copySize + 1];
						memcpy(dp.ps, data + readSize, copySize);
						dp.ps[copySize] = '\0';
					}
					readSize += byteSize;
				}
				break;
			case D_BINARY:
			case D_BINARY_TO_END:
				{
					if( bitOffset != 0 ){
						return -3;
					}
					DWORD byteSize = dataSize - readSize;
					if( **parser == D_BINARY ){
						byteSize = GetOperand(*(++*parser), ppLocal) / 8;
					}
					++*parser;
					if( readSize + byteSize > dataSize ){
						return -1;
					}
					DWORD copySize = min(byteSize, 0xFFF);
					pp->resize(pp->size() + 1);
					DESCRIPTOR_PROPERTY& dp = pp->back();
					dp.id = dpID;
					dp.type = (short)(DESCRIPTOR_PROPERTY::TYPE_B | copySize);
					if( copySize < sizeof(dp.b) + 1 ){
						//���u��
						memcpy(dp.b, data + readSize, copySize);
					}else{
						dp.pb = new BYTE[copySize];
						memcpy(dp.pb, data + readSize, copySize);
					}
					readSize += byteSize;
				}
				break;
			case D_LOCAL:
			case D_LOCAL_TO_END:
				{
					DWORD bitSize = dataSize * 8 - (readSize * 8 + bitOffset);
					if( **parser == D_LOCAL ){
						bitSize = GetOperand(*(++*parser), ppLocal);
					}
					++*parser;
					if( readSize * 8 + bitOffset + bitSize > dataSize * 8 ){
						return -1;
					}
					if( ppLocal->n == (DWORD)ppLocal->type ){
						//�X�^�b�N���s�����B���̃G���[�͉񕜂ł��Ȃ�
						return -3;
					}
					ppLocal[ppLocal->n].id = dpID;
					ppLocal[ppLocal->n++].n = DecodeNumber(data, bitSize, &readSize, &bitOffset);
				}
				break;
			default:
				{
					DWORD bitSize = GetOperand(**parser, ppLocal);
					++*parser;
					if( readSize * 8 + bitOffset + bitSize > dataSize * 8 ){
						return -1;
					}
					if( ppLocal->n == (DWORD)ppLocal->type ){
						return -3;
					}
					DESCRIPTOR_PROPERTY dp;
					ppLocal[ppLocal->n].id = dp.id = dpID;
					ppLocal[ppLocal->n++].n = dp.n = DecodeNumber(data, bitSize, &readSize, &bitOffset);
					pp->push_back(dp);
				}
				break;
			}
			break;
		}
	}
	if( bitOffset != 0 ){
		//�u���b�N�̓o�C�g���E�ɂȂ���΂Ȃ�Ȃ��B���̃G���[�͉񕜂ł��Ȃ�
		return -3;
	}
	++*parser;
	return readSize;
}

DWORD CDescriptor::GetOperand(short id, const LOCAL_PROPERTY* ppLocal)
{
	//���l���ǂ����B���l�̒P�ʂ̓r�b�g
	if( id <= D_IMMEDIATE_MAX ){
		return id;
	}
	for( ppLocal += ppLocal->n; (--ppLocal)->id != D_FIN; ){
		if( ppLocal->id == id ){
			return ppLocal->n * 8;
		}
	}
	//���̏��������������Ƃ��̓p�[�T�Ƀ~�X������
	OutputDebugString(L"CDescriptor::GetOperand: Parser syntax error\r\n");
	return 0;
}

DWORD CDescriptor::DecodeNumber(const BYTE* data, DWORD bitSize, DWORD* readSize, DWORD* bitOffset)
{
	DWORD n = 0;
	if( *bitOffset > 0 ){
		if( 8 - *bitOffset > bitSize ){
			n = n << bitSize | (BYTE)(data[*readSize] << *bitOffset) >> (8 - bitSize);
			*bitOffset += bitSize;
			return n;
		}
		n = n << (8 - *bitOffset) | (BYTE)(data[*readSize] << *bitOffset) >> *bitOffset;
		bitSize -= 8 - *bitOffset;
		*bitOffset = 0;
		++*readSize;
	}
	for( ; bitSize >= 8; bitSize -= 8, ++*readSize ){
		n = n << 8 | data[*readSize];
	}
	if( bitSize > 0 ){
		n = n << bitSize | data[*readSize] >> (8 - bitSize);
		*bitOffset = bitSize;
	}
	return n;
}

bool CDescriptor::EnterLoop(CLoopPointer& lp, DWORD offset) const
{
	const vector<DESCRIPTOR_PROPERTY>* current = lp.pl != NULL ? &(*lp.pl)[lp.index] : &this->rootProperty;

	vector<DESCRIPTOR_PROPERTY>::const_iterator itr;
	for( itr = current->begin(); itr != current->end(); ++itr ){
		if( itr->type == DESCRIPTOR_PROPERTY::TYPE_P && offset-- == 0 ){
			//��̃��[�v�ɂ͓���Ȃ�
			if( !itr->pl->empty() ){
				lp.pl = itr->pl;
				lp.index = 0;
				return true;
			}
			return false;
		}
	}
	return false;
}

bool CDescriptor::SetLoopIndex(CLoopPointer& lp, DWORD index) const
{
	if( index < GetLoopSize(lp) ){
		lp.index = index;
		return true;
	}
	return false;
}

const CDescriptor::DESCRIPTOR_PROPERTY* CDescriptor::FindProperty(short id, CLoopPointer lp) const
{
	const vector<DESCRIPTOR_PROPERTY>* current = lp.pl != NULL ? &(*lp.pl)[lp.index] : &this->rootProperty;

	vector<DESCRIPTOR_PROPERTY>::const_iterator itr;
	for( itr = current->begin(); itr != current->end(); ++itr ){
		if( itr->id == id ){
			return &*itr;
		}
	}
	return NULL;
}

DWORD CDescriptor::GetNumber(short id, CLoopPointer lp) const
{
	const DESCRIPTOR_PROPERTY* pp = FindProperty(id, lp);
	if( pp != NULL && pp->type == DESCRIPTOR_PROPERTY::TYPE_N ){
		return pp->n;
	}
	return 0;
}

bool CDescriptor::SetNumber(short id, DWORD n, CLoopPointer lp)
{
	DESCRIPTOR_PROPERTY* pp = const_cast<DESCRIPTOR_PROPERTY*>(FindProperty(id, lp));
	if( pp != NULL && pp->type == DESCRIPTOR_PROPERTY::TYPE_N ){
		pp->n = n;
		return true;
	}
	return false;
}

const char* CDescriptor::GetString(short id, DWORD* size, CLoopPointer lp) const
{
	const DESCRIPTOR_PROPERTY* pp = FindProperty(id, lp);
	if( pp != NULL && (pp->type & DESCRIPTOR_PROPERTY::TYPE_S) ){
		if( size != NULL ){
			*size = pp->type & DESCRIPTOR_PROPERTY::TYPE_MASK;
		}
		return (pp->type & DESCRIPTOR_PROPERTY::TYPE_MASK) < sizeof(pp->s) ? pp->s : pp->ps;
	}
	return NULL;
}

const char* CDescriptor::GetStringOrEmpty(short id, DWORD* size, CLoopPointer lp) const
{
	const char* ps = GetString(id, size, lp);
	if( ps == NULL ){
		if( size != NULL ){
			*size = 0;
		}
		ps = "";
	}
	return ps;
}

const BYTE* CDescriptor::GetBinary(short id, DWORD* size, CLoopPointer lp) const
{
	const DESCRIPTOR_PROPERTY* pp = FindProperty(id, lp);
	if( pp != NULL && (pp->type & DESCRIPTOR_PROPERTY::TYPE_B) ){
		if( size != NULL ){
			*size = pp->type & DESCRIPTOR_PROPERTY::TYPE_MASK;
		}
		return (pp->type & DESCRIPTOR_PROPERTY::TYPE_MASK) < sizeof(pp->b) + 1 ? pp->b : pp->pb;
	}
	return NULL;
}

CDescriptor::DESCRIPTOR_PROPERTY::~DESCRIPTOR_PROPERTY()
{
	if( type == TYPE_P ){
		delete pl;
	}else if( (type & TYPE_S) && (type & TYPE_MASK) >= sizeof(s) ){
		delete[] ps;
	}else if( (type & TYPE_B) && (type & TYPE_MASK) >= sizeof(b) + 1 ){
		delete[] pb;
	}
}

CDescriptor::DESCRIPTOR_PROPERTY::DESCRIPTOR_PROPERTY(const DESCRIPTOR_PROPERTY& o)
{
	id = o.id;
	type = o.type;
	if( type == TYPE_N ){
		n = o.n;
	}else if( type == TYPE_P ){
		pl = new vector<vector<DESCRIPTOR_PROPERTY>>(*o.pl);
	}else if( type & TYPE_S ){
		if( (type & TYPE_MASK) >= sizeof(s) ){
			ps = new char[(type & TYPE_MASK) + 1];
			memcpy(ps, o.ps, (type & TYPE_MASK) + 1);
		}else{
			memcpy(s, o.s, sizeof(s));
		}
	}else if( type & TYPE_B ){
		if( (type & TYPE_MASK) >= sizeof(b) + 1 ){
			pb = new BYTE[type & TYPE_MASK];
			memcpy(pb, o.pb, type & TYPE_MASK);
		}else{
			memcpy(b, o.b, sizeof(b));
		}
	}
}

CDescriptor::DESCRIPTOR_PROPERTY& CDescriptor::DESCRIPTOR_PROPERTY::operator=(DESCRIPTOR_PROPERTY&& o) NOEXCEPT
{
	id = o.id;
	std::swap(type, o.type);
	if( type == TYPE_N ){
		n = o.n;
	}else if( type == TYPE_P ){
		std::swap(pl, o.pl);
	}else if( type & TYPE_S ){
		if( (type & TYPE_MASK) >= sizeof(s) ){
			std::swap(ps, o.ps);
		}else{
			memmove(s, o.s, sizeof(s));
		}
	}else if( type & TYPE_B ){
		if( (type & TYPE_MASK) >= sizeof(b) + 1 ){
			std::swap(pb, o.pb);
		}else{
			memmove(b, o.b, sizeof(b));
		}
	}
	return *this;
}

BOOL CreateDescriptors(const BYTE* data, DWORD dataSize, vector<CDescriptor>* descriptorList, DWORD* decodeReadSize, const PARSER_PAIR* customParserList)
{
	if( data == NULL || descriptorList == NULL ){
		return FALSE;
	}
	DWORD decodeSize = 0;
	DWORD readSize;
	DWORD reserveCount = 0;
	while( (readSize = CDescriptor::GetDecodeReadSize(data + decodeSize, dataSize - decodeSize)) != 0 ){
		reserveCount++;
		decodeSize += readSize;
	}
	descriptorList->reserve(descriptorList->size() + reserveCount);

	decodeSize = 0;
	while( decodeSize < dataSize ){
		descriptorList->resize(descriptorList->size() + 1);
		if( descriptorList->back().Decode(data + decodeSize, dataSize - decodeSize, &readSize, customParserList) == false ){
			descriptorList->clear();
			return FALSE;
		}
		decodeSize += readSize;
	}
	if( decodeReadSize != NULL ){
		*decodeReadSize = decodeSize;
	}
	return TRUE;
}

}
