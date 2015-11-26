#pragma once

/*
5.2.3 プログラムマップテーブル（PMT）（Program Map Table）
PMT は、放送番組を構成する各符号化信号を伝送するTS パケットのパケット識別子を
指定する。
詳細はISO/IEC 13818-1(21)の2.4.4 項で定義する。

TS_program_map_section(){
	table_id						8 uimsbf
	section_syntax_indicator		1 bslbf
	‘0’							1 bslbf
	reserved						2 bslbf
	section_length					12 uimsbf
	program_number					16 uimsbf
	reserved						2 bslbf
	version_number					5 uimsbf
	current_next_indicator			1 bslbf
	section_number					8 uimsbf
	last_section_number				8 uimsbf
	reserved						3 bslbf
	PCR_PID							13 uimsbf
	reserved						4 bslbf
	program_info_length				12 uimsbf
	for (i=0;i<N;i++){
		descriptor()
	}
	for (i=0;i<N1;i++){
		stream_type						8 uimsbf
		reserved						3 bslbf
		elementary_PID					13 uimsnf
		reserved						4 bslbf
		ES_info_length					12 uimsbf
		for (i=0;i<N2;i++){
			descriptor()
		}
	}
	CRC_32 32 rpchof
}
*/

#include "../../../Common/Util.h"
#include "../AribDescriptor.h"
#include "PSITable.h"

class CPMTTable : public CPSITable
{
public:
	typedef struct _ES_INFO_DATA{
		BYTE stream_type;
		WORD elementary_PID;
		WORD ES_info_length;
		vector<AribDescriptor::CDescriptor*> descriptorList;
		~_ES_INFO_DATA(void){
			for( size_t i=0; i<descriptorList.size(); i++ ){
				SAFE_DELETE(descriptorList[i]);
			}
			descriptorList.clear();
		};
	} ES_INFO_DATA;
	WORD program_number;
	BYTE version_number;
	BYTE current_next_indicator;
	BYTE section_number;
	BYTE last_section_number;
	WORD PCR_PID;
	WORD program_info_length;
	vector<AribDescriptor::CDescriptor*> descriptorList;
	vector<ES_INFO_DATA*> ESInfoList;

public:
	CPMTTable(void);
	~CPMTTable(void);

	BOOL Decode( BYTE* data, DWORD dataSize, DWORD* decodeReadSize );
protected:
	void Clear();
};
