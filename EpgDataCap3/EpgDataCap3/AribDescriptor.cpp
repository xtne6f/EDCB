#include "StdAfx.h"
#include "AribDescriptor.h"

namespace AribDescriptor
{

//・PSI/SIの記述子をパースする簡易DSL
//・ARIB STD-B10等の構造体をだいたいそのまま書ける
//
//即値: 0～0x1EFF。単位は基本的にbit
//参照: 0x2000～。引数として解釈されるときの単位は基本的にbyte
//D_FIN:                パーサ終端
//D_END:                D_BEGIN*を閉じる
//D_BEGIN,{即値|参照}:  D_ENDまで過不足なく{即値|参照}サイズだけ入力を消費する
//D_BEGIN_IF,{参照},{即値L},{即値R}: {参照}が{即値L}以上{即値R}以下の場合にD_ENDまでの区間を有効にする
//D_BEGIN_IF_NOT,{参照},{即値L},{即値R}: D_BEGIN_IFの否定
//D_BEGIN_FOR,{参照}:   プロパティのループを作成して{参照}回だけ繰り返す
//D_BEGIN_FOR_TO_END:   プロパティのループを作成してできるだけ入力を消費するように繰り返す
//{参照},{即値|参照}:          入力データを{即値|参照}サイズだけ{参照}にDWORD値として格納
//{参照},D_LOCAL,{即値|参照}:  DWORD値として格納しパース後に捨てる
//{参照},D_LOCAL_TO_END:       D_LOCALと同様だが、できるだけ入力を消費する
//{参照},D_STRING,{即値|参照}: 入力データを{即値|参照}サイズだけ{参照}に文字列として格納
//{参照},D_STRING_TO_END:      D_STRINGと同様だが、できるだけ入力を消費する
//{参照},D_BINARY,{即値|参照}: 入力データを{即値|参照}サイズだけ{参照}にバイト列として格納
//{参照},D_BINARY_TO_END:      D_BINARYと同様だが、できるだけ入力を消費する
//
//※D_BEGIN*,D_STRING*,D_BINARY*はバイト(8bit)境界でのみ使用できる

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

//注意: 利用されない記述子はコメントアウトしている(unknown_descriptor扱い)
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

CDescriptor::CDescriptor()
{
	this->currentLoop = NULL;
}

CDescriptor::~CDescriptor()
{
	Clear();
}

void CDescriptor::Clear()
{
	ClearProperty(&this->rootProperty);
	delete this->currentLoop;
	this->currentLoop = NULL;
}

void CDescriptor::ClearProperty(std::vector<DESCRIPTOR_PROPERTY>* pp)
{
	while( !pp->empty() ){
		switch( pp->back().type >> 12 ){
		case 0: //Number
			break;
		case 1: //PropertyLoop
			while( !pp->back().pl->empty() ){
				ClearProperty(&pp->back().pl->back());
				pp->back().pl->pop_back();
			}
			delete pp->back().pl;
			break;
		case 2: //String
			if( (pp->back().type & 0xFFF) >= 4 ){
				delete[] pp->back().ps;
			}
			break;
		case 3: //Binary
			if( (pp->back().type & 0xFFF) >= 5 ){
				delete[] pp->back().pb;
			}
			break;
		}
		pp->pop_back();
	}
}

bool CDescriptor::Decode(const BYTE* data, DWORD dataSize, DWORD* decodeReadSize, const PARSER_PAIR* customParserList)
{
	Clear();

	if( data == NULL || dataSize == 0 ){
		return false;
	}

	//記述子タグに対応するパーサを探す(customParserListを優先)
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

	//ローカル参照用スタック
	std::vector<DESCRIPTOR_PROPERTY> localProperty;

	int readSize = DecodeProperty(data, dataSize, &parser, &this->rootProperty, &localProperty);
	if( readSize < 0 ){
		if( readSize == -3 ){
			//この条件が満たされるときはパーサにミスがある
			OutputDebugString(L"CDescriptor::DecodeProperty: Parser syntax error\n");
		}
		Clear();
		return false;
	}
	if( decodeReadSize != NULL ){
		*decodeReadSize = readSize;
	}
	return true;
}

int CDescriptor::DecodeProperty(const BYTE* data, DWORD dataSize, const short** parser, std::vector<DESCRIPTOR_PROPERTY>* pp, std::vector<DESCRIPTOR_PROPERTY>* ppLocal)
{
	DWORD readSize = 0;
	DWORD bitOffset = 0;
	DESCRIPTOR_PROPERTY dp;

	while( **parser != D_FIN && **parser != D_END ){
		switch( **parser ){
		case D_BEGIN:
			{
				if( bitOffset != 0 ){
					return -3;
				}
				++*parser;
				DWORD subSize = GetOperand(**parser, *ppLocal) / 8;
				++*parser;
				if( readSize + subSize > dataSize ){
					return -1;
				}
				int subReadSize = DecodeProperty(data + readSize, subSize, parser, pp, ppLocal);
				if( subReadSize < 0 ){
					return subReadSize;
				}
				if( (DWORD)subReadSize < subSize ){
					//内包するデータ長が足りない。このエラーは回復できない
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
				DWORD val = GetOperand(**parser, *ppLocal) / 8;
				++*parser;
				DWORD exprL = **parser;
				++*parser;
				DWORD exprR = **parser;
				++*parser;
				if( bNot && !(exprL <= val && val <= exprR) || !bNot && (exprL <= val && val <= exprR) ){
					int subReadSize = DecodeProperty(data + readSize, dataSize - readSize, parser, pp, ppLocal);
					if( subReadSize < 0 ){
						return subReadSize;
					}
					readSize += subReadSize;
				}else{
					//D_ENDまで移動
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
				dp.id = 0;
				dp.type = 1 << 12;
				dp.pl = new std::vector<std::vector<DESCRIPTOR_PROPERTY>>;
				pp->push_back(dp);

				int loopNum = -1;
				if( **parser == D_BEGIN_FOR ){
					loopNum = GetOperand(*(++*parser), *ppLocal) / 8;
					dp.pl->reserve(loopNum);
				}
				++*parser;

				for( ; loopNum != 0; --loopNum ){
					dp.pl->resize(dp.pl->size() + 1);
					const short* parserRollback = *parser;
					size_t localRollback = ppLocal->size();
					int subReadSize = DecodeProperty(data + readSize, dataSize - readSize, parser, &dp.pl->back(), ppLocal);
					ppLocal->resize(localRollback);
					*parser = parserRollback;

					if( subReadSize < 0 ){
						ClearProperty(&dp.pl->back());
						dp.pl->pop_back();
						if( subReadSize < -1 ){
							return subReadSize;
						}
						break;
					}
					readSize += subReadSize;
				}
				if( loopNum > 0 || loopNum < 0 && readSize < dataSize ){
					//指定のループ回数に達しないかデータ長が矛盾する。このエラーは回復できない
					return -2;
				}
				//D_ENDまで移動
				for( int n = 1; n > 0; ++*parser ){
					if( D_BEGIN <= **parser && **parser <= D_BEGIN_FOR_TO_END ){
						++n;
					}else if( **parser == D_END ){
						--n;
					}
				}
			}
			break;
		default:
			dp.id = **parser;
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
						byteSize = GetOperand(*(++*parser), *ppLocal) / 8;
					}
					++*parser;
					if( readSize + byteSize > dataSize ){
						return -1;
					}
					DWORD copySize = min(byteSize, 0xFFF);
					dp.type = (short)(2 << 12 | copySize);
					//4文字未満は直置き
					if( copySize < 4 ){
						memcpy(dp.s, data + readSize, copySize);
						dp.s[copySize] = '\0';
					}else{
						dp.ps = new char[copySize + 1];
						memcpy(dp.ps, data + readSize, copySize);
						dp.ps[copySize] = '\0';
					}
					pp->push_back(dp);
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
						byteSize = GetOperand(*(++*parser), *ppLocal) / 8;
					}
					++*parser;
					if( readSize + byteSize > dataSize ){
						return -1;
					}
					DWORD copySize = min(byteSize, 0xFFF);
					dp.type = (short)(3 << 12 | copySize);
					//5バイト未満は直置き
					if( copySize < 5 ){
						memcpy(dp.b, data + readSize, copySize);
					}else{
						dp.pb = new BYTE[copySize];
						memcpy(dp.pb, data + readSize, copySize);
					}
					pp->push_back(dp);
					readSize += byteSize;
				}
				break;
			case D_LOCAL:
			case D_LOCAL_TO_END:
				{
					DWORD bitSize = dataSize * 8 - (readSize * 8 + bitOffset);
					if( **parser == D_LOCAL ){
						bitSize = GetOperand(*(++*parser), *ppLocal);
					}
					++*parser;
					if( readSize * 8 + bitOffset + bitSize > dataSize * 8 ){
						return -1;
					}
					dp.type = 0;
					dp.n = DecodeNumber(data, bitSize, &readSize, &bitOffset);
					ppLocal->push_back(dp);
				}
				break;
			default:
				{
					DWORD bitSize = GetOperand(**parser, *ppLocal);
					++*parser;
					if( readSize * 8 + bitOffset + bitSize > dataSize * 8 ){
						return -1;
					}
					dp.type = 0;
					dp.n = DecodeNumber(data, bitSize, &readSize, &bitOffset);
					pp->push_back(dp);
					ppLocal->push_back(dp);
				}
				break;
			}
			break;
		}
	}
	if( bitOffset != 0 ){
		//ブロックはバイト境界になければならない。このエラーは回復できない
		return -3;
	}
	++*parser;
	return readSize;
}

DWORD CDescriptor::GetOperand(short id, const std::vector<DESCRIPTOR_PROPERTY>& pLocal)
{
	//即値かどうか。即値の単位はビット
	if( id <= D_IMMEDIATE_MAX ){
		return id;
	}
	std::vector<DESCRIPTOR_PROPERTY>::const_reverse_iterator itr;
	for( itr = pLocal.rbegin(); itr != pLocal.rend(); ++itr ){
		if( itr->id == id ){
			return itr->n * 8;
		}
	}
	//この条件が満たされるときはパーサにミスがある
	OutputDebugString(L"CDescriptor::GetOperand: Parser syntax error\n");
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

bool CDescriptor::EnterLoop(DWORD offset)
{
	const std::vector<DESCRIPTOR_PROPERTY>* current = this->currentLoop != NULL ?
		&(*this->currentLoop->back().first)[this->currentLoop->back().second] : &this->rootProperty;

	std::vector<DESCRIPTOR_PROPERTY>::const_iterator itr;
	for( itr = current->begin(); itr != current->end(); ++itr ){
		if( itr->type >> 12 == 1 && offset-- == 0 ){
			//空のループには入らない
			if( !itr->pl->empty() ){
				if( this->currentLoop == NULL ){
					this->currentLoop = new std::vector<std::pair<std::vector<std::vector<DESCRIPTOR_PROPERTY>>*,DWORD>>;
				}
				this->currentLoop->push_back(std::make_pair(itr->pl, 0));
				return true;
			}
			return false;
		}
	}
	return false;
}

void CDescriptor::LeaveLoop()
{
	if( this->currentLoop != NULL ){
		this->currentLoop->pop_back();
		if( this->currentLoop->empty() ){
			delete this->currentLoop;
			this->currentLoop = NULL;
		}
	}
}

bool CDescriptor::SetLoopIndex(DWORD index)
{
	if( index < GetLoopSize() ){
		if( this->currentLoop != NULL ){
			this->currentLoop->back().second = index;
		}
		return true;
	}
	return false;
}

DWORD CDescriptor::GetLoopSize() const
{
	return this->currentLoop != NULL ? (DWORD)(this->currentLoop->back().first->size()) : 1;
}

const CDescriptor::DESCRIPTOR_PROPERTY* CDescriptor::FindProperty(short id) const
{
	const std::vector<DESCRIPTOR_PROPERTY>* current = this->currentLoop != NULL ?
		&(*this->currentLoop->back().first)[this->currentLoop->back().second] : &this->rootProperty;

	std::vector<DESCRIPTOR_PROPERTY>::const_iterator itr;
	for( itr = current->begin(); itr != current->end(); ++itr ){
		if( itr->id == id ){
			return &*itr;
		}
	}
	return NULL;
}

bool CDescriptor::Has(short id) const
{
	return FindProperty(id) != NULL;
}

DWORD CDescriptor::GetNumber(short id) const
{
	const DESCRIPTOR_PROPERTY* pp = FindProperty(id);
	if( pp != NULL && pp->type >> 12 == 0 ){
		return pp->n;
	}
	return 0;
}

bool CDescriptor::SetNumber(short id, DWORD n)
{
	DESCRIPTOR_PROPERTY* pp = const_cast<DESCRIPTOR_PROPERTY*>(FindProperty(id));
	if( pp != NULL && pp->type >> 12 == 0 ){
		pp->n = n;
		return true;
	}
	return false;
}

const char* CDescriptor::GetString(short id, DWORD* size) const
{
	const DESCRIPTOR_PROPERTY* pp = FindProperty(id);
	if( pp != NULL && pp->type >> 12 == 2 ){
		if( size != NULL ){
			*size = pp->type & 0xFFF;
		}
		return (pp->type & 0xFFF) < 4 ? pp->s : pp->ps;
	}
	return NULL;
}

const char* CDescriptor::GetStringOrEmpty(short id, DWORD* size) const
{
	const char* ps = GetString(id, size);
	if( ps == NULL ){
		if( size != NULL ){
			*size = 0;
		}
		ps = "";
	}
	return ps;
}

const BYTE* CDescriptor::GetBinary(short id, DWORD* size) const
{
	const DESCRIPTOR_PROPERTY* pp = FindProperty(id);
	if( pp != NULL && pp->type >> 12 == 3 ){
		if( size != NULL ){
			*size = pp->type & 0xFFF;
		}
		return (pp->type & 0xFFF) < 5 ? pp->b : pp->pb;
	}
	return NULL;
}

BOOL CreateDescriptors(BYTE* data, DWORD dataSize, std::vector<CDescriptor*>* descriptorList, DWORD* decodeReadSize, const PARSER_PAIR* customParserList)
{
	if( data == NULL || descriptorList == NULL ){
		return FALSE;
	}
	DWORD decodeSize = 0;
	while( decodeSize < dataSize ){
		descriptorList->push_back(new CDescriptor);
		DWORD readSize;
		if( descriptorList->back()->Decode(data + decodeSize, dataSize - decodeSize, &readSize, customParserList) == false ){
			while( descriptorList->empty() == false ){
				delete descriptorList->back();
				descriptorList->pop_back();
			}
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
