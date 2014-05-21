#pragma once

/*
5.2.1 プログラムアソシエーションテーブル（PAT）（Program Association Table）
PAT は、放送番組に関連するPMT を伝送するTS パケットのパケット識別子を指定する。
詳細はISO/IEC 13818-1(21)の2.4.4 項で定義する。

program_association_section(){
	table_id					8 uimsbf
	section_syntax_indicator	1 bslbf
	‘0’						1 bslbf
	reserved					2 bslbf
	section_length				12 uimsbf
	transport_stream_id			16 uimsbf
	reserved					2 bslbf
	version_number				5 uimsbf
	current_next_indicator		1 bslbf
	section_number				8 uimsbf
	last_section_number			8 uimsbf
	for(i=0;i<N;i++){
		program_number				16 uimsbf
		reserved					3 bslbf
		if (program_number = = ‘0’){
			network_PID					13 uimsbf
		}else{
			program_map_PID				13 uimsbf
		}
	}
	CRC_32						32 rpchof
}
*/

#include "../../../Common/Util.h"
#include "PSITable.h"

class CPATTable : public CPSITable
{
public:
	typedef struct _PMT_DATA{
		WORD program_number;
		WORD PID;
	} PMT_DATA;
	WORD transport_stream_id;
	BYTE version_number;
	BYTE current_next_indicator;
	BYTE section_number;
	BYTE last_section_number;
	map<WORD, PMT_DATA> pmtMap;	//キーprogram_number(ServiceID)

public:
	CPATTable(void);
	~CPATTable(void);

	BOOL Decode( BYTE* data, DWORD dataSize, DWORD* decodeReadSize );
};
