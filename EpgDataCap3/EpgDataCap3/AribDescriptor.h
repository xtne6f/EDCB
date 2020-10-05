#pragma once

#ifndef NOEXCEPT
#if !defined(_MSC_VER) || _MSC_VER >= 1900
#define NOEXCEPT noexcept
#else
#define NOEXCEPT
#endif
#endif

namespace AribDescriptor
{
	enum {
		D_FIN = -1,
		D_END = 0x1F00,
		D_BEGIN,
		D_BEGIN_SUB,
		D_BEGIN_IF,
		D_BEGIN_IF_NOT,
		D_BEGIN_FOR,
		D_BEGIN_FOR_TO_END,
		D_DESCRIPTOR_LOOP,
		D_ASSERT_CRC_32,
		D_LOCAL,
		D_LOCAL_TO_END,
		D_BINARY,
		D_BINARY_TO_END,
		D_IMMEDIATE_MAX = 0x1FFF,
	};

	enum property_id
#if !defined(_MSC_VER) || _MSC_VER >= 1900
		: short
#endif
	{
		d_invalid = 0,
		reserved = D_IMMEDIATE_MAX + 1,
		descriptor_tag,
		descriptor_length,
		stream_content,
		component_type,
		component_tag,
		stream_type,
		simulcast_group_tag,
		ES_multi_lingual_flag,
		main_component_flag,
		quality_indicator,
		sampling_rate,
		ISO_639_language_code,
		ISO_639_language_code_2,
		text_char,
		hrd_management_valid_flag,
		picture_and_timing_info_present,
		d_90kHz_flag,
		d_N,
		d_K,
		num_units_in_tick,
		fixed_frame_rate_flag,
		temporal_poc_flag,
		picture_to_display_conversion_flag,
		profile_idc,
		constraint_set0_flag,
		constraint_set1_flag,
		constraint_set2_flag,
		AVC_compatible_flags,
		level_idc,
		AVC_still_present,
		AVC_24_hour_picture_flag,
		title_length,
		title_char,
		text_length,
		d_char,
		CA_system_ID,
		CA_PID,
		private_data_byte,
		component_group_type,
		total_bit_rate_flag,
		num_of_group,
		component_group_id,
		num_of_CA_unit,
		CA_unit_id,
		num_of_component,
		total_bit_rate,
		connected_transmission_group_id,
		segment_type,
		modulation_type_A,
		modulation_type_B,
		additional_connected_transmission_info,
		copy_restriction_mode,
		image_constraint_token,
		retention_mode,
		retention_state,
		encryption_mode,
		content_user_nibble,
		country_availability_flag,
		country_code,
		data_component_id,
		additional_data_component_info,
		entry_component,
		selector_length,
		selector_byte,
		num_of_component_ref,
		component_ref,
		digital_recording_control_data,
		maximum_bitrate_flag,
		component_control_flag,
		user_defined,
		maximum_bitrate,
		component_control_length,
		reboot,
		add_on,
		compatibility_flag,
		module_info_flag,
		text_info_flag,
		component_size,
		download_id,
		time_out_value_DII,
		leak_rate,
		compatibility_descriptor_length,
		compatibility_descriptor_byte,
		num_of_modules,
		module_id,
		module_size,
		module_info_length,
		module_info_byte,
		private_data_length,
		service_id,
		start_end_flag,
		signal_level,
		area_code_length,
		area_code,
		group_type,
		event_count,
		event_id,
		original_network_id,
		transport_stream_id,
		broadcaster_type,
		terrestrial_broadcaster_id,
		number_of_affiliation_id_loop,
		number_of_broadcaster_id_loop,
		affiliation_id,
		broadcaster_id,
		terrestrial_sound_broadcaster_id,
		number_of_sound_broadcast_affiliation_id_loop,
		sound_broadcast_affiliation_id,
		descriptor_number,
		last_descriptor_number,
		length_of_items,
		item_description_length,
		item_description_char,
		item_length,
		item_char,
		quality_level,
		reference_PID,
		hyper_linkage_type,
		link_destination_type,
		content_id,
		information_provider_id,
		event_relation_id,
		node_id,
		uri_char,
		private_data,
		original_service_id,
		description_id,
		description_type,
		linkage_type,
		country_region_id,
		local_time_offset_polarity,
		local_time_offset,
		time_of_change,
		next_time_offset,
		logo_transmission_type,
		logo_id,
		logo_version,
		download_data_id,
		logo_char,
		mosaic_entry_point,
		number_of_horizontal_elementary_cells,
		number_of_vertical_elementary_cells,
		logical_cell_id,
		logical_cell_presentation_info,
		elementary_cell_field_length,
		elementary_cell_id,
		cell_linkage_info,
		bouquet_id,
		media_type,
		network_id,
		rating,
		peak_rate,
		minimum_overall_smoothing_rate,
		maximum_overall_smoothing_buffer,
		event_version_number,
		event_start_time,
		d_duration,
		d_offset,
		offset_flag,
		other_descriptor_status,
		jst_time_flag,
		d_frequency,
		orbital_position,
		west_east_flag,
		polarisation,
		modulation,
		symbol_rate,
		FEC_inner,
		series_id,
		repeat_label,
		program_pattern,
		expire_date_valid_flag,
		expire_date,
		episode_number,
		last_episode_number,
		series_name_char,
		service_type,
		service_provider_name_length,
		service_provider_name,
		service_name_length,
		service_name,
		service_group_type,
		primary_service_id,
		secondary_service_id,
		event_name_length,
		event_name_char,
		parameter_version,
		update_time,
		table_id,
		table_description_length,
		table_description_byte,
		SI_prime_ts_network_id,
		SI_prime_transport_stream_id,
		stuffing_byte,
		system_management_id,
		additional_identification_info,
		region_spec_type,
		prefecture_bitmap,
		guard_interval,
		transmission_mode,
		reference_service_id,
		reference_event_id,
		remote_control_key_id,
		length_of_ts_name,
		transmission_type_count,
		ts_name_char,
		transmission_type_info,
		num_of_service,
		still_picture_flag,
		sequence_end_code_flag,
		video_encode_format,
		section_syntax_indicator,
		section_length,
		version_number,
		current_next_indicator,
		section_number,
		last_section_number,
		segment_last_section_number,
		last_table_id,
		start_time_mjd,
		start_time_bcd,
		running_status,
		free_CA_mode,
		descriptors_loop_length,
		CRC_32,
		jst_time_mjd,
		jst_time_bcd,
		transmission_info_loop_length,
		service_loop_length,
		program_number,
		program_map_PID,
		network_descriptors_length,
		transport_stream_loop_length,
		transport_descriptors_length,
		EIT_user_defined_flags,
		EIT_schedule_flag,
		EIT_present_following_flag,
		broadcast_view_propriety,
		first_descriptors_length,
		broadcaster_descriptors_length,
		data_type,
		data_module_byte,
		PCR_PID,
		program_info_length,
		elementary_PID,
		ES_info_length,
	};

	enum {
		//CA_descriptor								= 0x09,
		//AVC_video_descriptor						= 0x28,
		//AVC_timing_and_HRD_descriptor				= 0x2A,
		network_name_descriptor						= 0x40,
		service_list_descriptor						= 0x41,
		//stuffing_descriptor						= 0x42,
		//satellite_delivery_system_descriptor		= 0x43,
		//bouquet_name_descriptor					= 0x47,
		service_descriptor							= 0x48,
		//country_availability_descriptor			= 0x49,
		//linkage_descriptor						= 0x4A,
		//NVOD_reference_descriptor					= 0x4B,
		//time_shifted_service_descriptor			= 0x4C,
		short_event_descriptor						= 0x4D,
		extended_event_descriptor					= 0x4E,
		//time_shifted_event_descriptor				= 0x4F,
		component_descriptor						= 0x50,
		//mosaic_descriptor							= 0x51,
		stream_identifier_descriptor				= 0x52,
		//CA_identifier_descriptor					= 0x53,
		content_descriptor							= 0x54,
		//parental_rating_descriptor				= 0x55,
		//local_time_offset_descriptor				= 0x58,
		//partial_transport_stream_descriptor		= 0x63,
		//hierarchical_transmission_descriptor		= 0xC0,
		//digital_copy_control_descriptor			= 0xC1,
		network_identification_descriptor			= 0xC2,
		partialTS_time_descriptor					= 0xC3,
		audio_component_descriptor					= 0xC4,
		//hyperlink_descriptor						= 0xC5,
		//target_region_descriptor					= 0xC6,
		data_content_descriptor						= 0xC7,
		//video_decode_control_descriptor			= 0xC8,
		//Download_content_descriptor				= 0xC9,
		ts_information_descriptor					= 0xCD,
		//extended_broadcaster_descriptor			= 0xCE,
		logo_transmission_descriptor				= 0xCF,
		//series_descriptor							= 0xD5,
		event_group_descriptor						= 0xD6,
		//SI_parameter_descriptor					= 0xD7,
		//broadcaster_name_descriptor				= 0xD8,
		//component_group_descriptor				= 0xD9,
		//SI_prime_ts_descriptor					= 0xDA,
		//board_information_descriptor				= 0xDB,
		//LDT_linkage_descriptor					= 0xDC,
		//connected_transmission_descriptor			= 0xDD,
		//content_availability_descriptor			= 0xDE,
		//service_group_descriptor					= 0xE0,
		//terrestrial_delivery_system_descriptor	= 0xFA,
		partial_reception_descriptor				= 0xFB,
		//emergency_information_descriptor			= 0xFC,
		//data_component_descriptor					= 0xFD,
		//system_management_descriptor				= 0xFE,
	};

	enum si_type {
		TYPE_PAT,
		TYPE_PMT,
		TYPE_DSMCC_HEAD,
		TYPE_NIT,
		TYPE_SDT,
		TYPE_EIT,
		TYPE_TDT,
		TYPE_TOT,
		TYPE_SIT,
		TYPE_BIT,
		TYPE_CDT,
	};

	struct PARSER_PAIR {
		BYTE tag;
		const short* parser;
	};
	extern const PARSER_PAIR parserMap[17];

	class CDescriptor
	{
	private:
		struct DESCRIPTOR_PROPERTY;
	public:
		class CLoopPointer {
		public:
			CLoopPointer() : pl(NULL) {}
		private:
			friend CDescriptor;
			vector<vector<DESCRIPTOR_PROPERTY>>* pl;
			DWORD index;
		};
		void Clear();
		bool DecodeSI(const BYTE* data, DWORD dataSize, DWORD* decodeReadSize, si_type type, const PARSER_PAIR* customParserList = NULL);
		static DWORD GetDecodeReadSize(const BYTE* data, DWORD dataSize);
		bool Decode(const BYTE* data, DWORD dataSize, DWORD* decodeReadSize, const PARSER_PAIR* customParserList = NULL);
		bool EnterLoop(CLoopPointer& lp, DWORD offset = 0) const;
		bool SetLoopIndex(CLoopPointer& lp, DWORD index) const;
		bool NextLoopIndex(CLoopPointer& lp) const { return SetLoopIndex(lp, lp.index + 1); }
		DWORD GetLoopSize(CLoopPointer lp = CLoopPointer()) const { return lp.pl != NULL ? (DWORD)lp.pl->size() : 1; }
		bool Has(property_id id, CLoopPointer lp = CLoopPointer()) const { return FindProperty(id, lp) != NULL; }
		DWORD GetNumber(property_id id, CLoopPointer lp = CLoopPointer()) const;
		bool SetNumber(property_id id, DWORD n, CLoopPointer lp = CLoopPointer());
		const BYTE* GetBinary(property_id id, DWORD* size = NULL, CLoopPointer lp = CLoopPointer()) const;
	private:
		struct DESCRIPTOR_PROPERTY {
			property_id id;
			short type;
			union {
				DWORD n;
				vector<vector<DESCRIPTOR_PROPERTY>>* pl;
				BYTE b[sizeof(BYTE*)];
				BYTE* pb;
			};
			enum {
				TYPE_N = -2,
				TYPE_P = -1,
			};
			DESCRIPTOR_PROPERTY() : id(d_invalid), type(TYPE_N) {}
			~DESCRIPTOR_PROPERTY();
			DESCRIPTOR_PROPERTY(const DESCRIPTOR_PROPERTY& o);
			DESCRIPTOR_PROPERTY& operator=(DESCRIPTOR_PROPERTY&& o) NOEXCEPT;
			DESCRIPTOR_PROPERTY(DESCRIPTOR_PROPERTY&& o) NOEXCEPT : type(TYPE_N) { *this = std::move(o); }
			DESCRIPTOR_PROPERTY& operator=(const DESCRIPTOR_PROPERTY& o) { return *this = DESCRIPTOR_PROPERTY(o); }
		};
		struct LOCAL_PROPERTY {
			property_id id;
			DWORD n;
		};
		static int DecodeProperty(const BYTE* data, DWORD dataSize, const short** parser, vector<DESCRIPTOR_PROPERTY>* pp, LOCAL_PROPERTY* ppLocal, const PARSER_PAIR* customParserList);
		static DWORD GetOperand(short id, const LOCAL_PROPERTY* ppLocal);
		static DWORD DecodeNumber(const BYTE* data, DWORD bitSize, DWORD* readSize, DWORD* bitOffset);
		const DESCRIPTOR_PROPERTY* FindProperty(property_id id, CLoopPointer lp) const;

		vector<DESCRIPTOR_PROPERTY> rootProperty;
	};
}
