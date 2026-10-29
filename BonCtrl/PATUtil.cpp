#include "stdafx.h"
#include "PATUtil.h"
#include "../Common/EpgTimerUtil.h"

CPATUtil::CPATUtil()
{
	this->transport_stream_id = 0;
	this->version_number = 0xFF;
}

BOOL CPATUtil::AddPacket(const CTSPacketUtil& packet)
{
	BOOL updated = FALSE;
	if( this->buffUtil.Add188TS(packet) == TRUE ){
		BYTE* section = nullptr;
		DWORD sectionSize = 0;
		while( this->buffUtil.GetSectionBuff(&section, &sectionSize) ){
			updated = DecodePAT(section, sectionSize) || updated;
		}
	}
	return updated;
}

BOOL CPATUtil::DecodePAT(BYTE* data, DWORD dataSize)
{
	if( data == nullptr || dataSize < 3 ||
	    (dataSize == this->lastSection.size() && std::equal(data, data + dataSize, this->lastSection.begin())) ){
		//解析不要
		return FALSE;
	}

	DWORD readSize = 0;
	//////////////////////////////////////////////////////
	//解析処理
	BYTE table_id = data[0];
	BYTE section_syntax_indicator = data[1] >> 7;
	WORD section_length = (data[1] & 0x0F) << 8 | data[2];
	readSize += 3;

	if( section_syntax_indicator != 1 ){
		//固定値がおかしい
		AddDebugLog(L"CPATUtil::section_syntax_indicator Err");
		return FALSE;
	}
	if( table_id != 0x00 ){
		//table_idがおかしい
		AddDebugLog(L"CPATUtil::table_id Err");
		return FALSE;
	}
	if( readSize + section_length > dataSize || section_length < 5 + 4 ){
		//サイズ異常
		AddDebugLogFormat(L"CPATUtil::section_length %d Err", section_length);
		return FALSE;
	}
	//CRCチェック
	if( CalcCrc32(3 + section_length, data) != 0 ){
		AddDebugLog(L"CPATUtil::crc32 Err");
		return FALSE;
	}
	BYTE current_next_indicator = data[readSize + 2] & 0x01;
	if( current_next_indicator == 0 ){
		//解析不要
		return FALSE;
	}
	this->lastSection.assign(data, data + dataSize);

	this->pidProgramNumberList.clear();
	this->transport_stream_id = data[readSize] << 8 | data[readSize + 1];
	this->version_number = (data[readSize + 2] & 0x3E) >> 1;
	readSize += 5;
	while( readSize + 3 < (DWORD)section_length + 3 - 4 ){
		WORD program_number = data[readSize] << 8 | data[readSize + 1];
		WORD pid = (data[readSize + 2] & 0x0F) << 8 | data[readSize + 3];
		readSize += 4;
		this->pidProgramNumberList.emplace_back(pid, program_number);
	}

	return TRUE;
}
