#pragma once

/*
5.2.2 限定受信テーブル（CAT）（Conditional Access Table）
CAT は、有料放送の関連情報を伝送するTS パケットのパケット識別子を指定する。
詳細はISO/IEC 13818-1(21)の2.4.4 項で定義する。

CA_section(){
	table_id					8 uimsbf
	section_syntax_indicator	1 bslbf
	‘0’						1 bslbf
	reserved					2 bslbf
	section_length				12 uimsbf
	reserved					18 bslbf
	version_number				5 uimsbf
	current_next_indicator		1 bslbf
	section_number				8 uimsbf
	last_section_number			8 uimsbf
	for (i=0;i<N;i++){
		descriptor()
	}
	CRC_32						32 rpchof
}
*/

#include "../../../Common/Util.h"
#include "../AribDescriptor.h"
#include "PSITable.h"

class CCATTable : public CPSITable
{
public:
	BYTE version_number;
	BYTE current_next_indicator;
	BYTE section_number;
	BYTE last_section_number;
	vector<AribDescriptor::CDescriptor*> descriptorList;

public:
	CCATTable(void);
	~CCATTable(void);

	BOOL Decode( BYTE* data, DWORD dataSize, DWORD* decodeReadSize );
protected:
	void Clear();
};
