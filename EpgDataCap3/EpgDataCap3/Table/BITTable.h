#pragma once

/*
5.2.13 ブロードキャスタ情報テーブル（BIT）（Broadcaster Information Table）
ブロードキャスタ情報テーブル（表 5－13 参照）は、ネットワーク上に存在するブロー
ドキャスタの情報を提示するために用いる。

broadcaster_information _section(){
	table_id					8 uimsbf
	section_syntax_indicator	1 bslbf
	reserved_future_use			1 bslbf
	reserved					2 bslbf
	section_length				12 uimsbf
	original_network_id			16 uimsbf
	reserved					2 bslbf
	version_number				5 uimsbf
	current_next_indicator		1 bslbf
	section_number				8 uimsbf
	last_section_number			8 uimsbf
	reserved_future_use			3 bslbf
	broadcast_view_propriety	1 bslbf
	first_descriptors_length	12 uimsbf
	for (i = 0;i< N1;i++) {
		descriptor()
	}
	for (j = 0;j< N2;j++) {
		broadcaster_id					8 uimsbf
		reserved_future_use				4 bslbf
		broadcaster_descriptors_length	12 uimsbf
		for(k=0;k<N3;k++){
			descriptor()
		}
	}
	CRC_32						32 rpchof
}
*/

#include "../../../Common/Util.h"
#include "../AribDescriptor.h"
#include "PSITable.h"

class CBITTable : public CPSITable
{
public:
	typedef struct _BROADCASTER_DATA{
		BYTE broadcaster_id;
		WORD broadcaster_descriptors_length;
		vector<AribDescriptor::CDescriptor*> descriptorList;
		~_BROADCASTER_DATA(void){
			for( size_t i=0; i<descriptorList.size(); i++ ){
				SAFE_DELETE(descriptorList[i]);
			}
			descriptorList.clear();
		};
	}BROADCASTER_DATA;
	WORD original_network_id;
	BYTE version_number;
	BYTE current_next_indicator;
	BYTE section_number;
	BYTE last_section_number;
	BYTE broadcast_view_propriety;
	WORD first_descriptors_length;
	vector<AribDescriptor::CDescriptor*> descriptorList;
	vector<BROADCASTER_DATA*> broadcasterDataList;

public:
	CBITTable(void);
	~CBITTable(void);

	BOOL Decode( BYTE* data, DWORD dataSize, DWORD* decodeReadSize );
protected:
	void Clear();
};
