#include "stdafx.h"
#include "CATUtil.h"

#include "../Common/EpgTimerUtil.h"


BOOL CCATUtil::AddPacket(const CTSPacketUtil& packet)
{
	BOOL updated = FALSE;
	if( buffUtil.Add188TS(packet) == TRUE ){
		BYTE* section = NULL;
		DWORD sectionSize = 0;
		while( buffUtil.GetSectionBuff(&section, &sectionSize) ){
			updated = DecodeCAT(section, sectionSize) || updated;
		}
	}
	return updated;
}

BOOL CCATUtil::DecodeCAT(BYTE* data, DWORD dataSize)
{
	if( data == NULL || dataSize < 3 ||
	    (dataSize == lastSection.size() && std::equal(data, data + dataSize, lastSection.begin())) ){
		//解析不要
		return FALSE;
	}

	DWORD readSize = 0;
	//////////////////////////////////////////////////////
	//解析処理
	BYTE table_id = data[0];
	BYTE section_syntax_indicator = (data[1]&0x80)>>7;
	WORD section_length = ((WORD)data[1]&0x0F)<<8 | data[2];
	readSize+=3;

	if( section_syntax_indicator != 1 || (data[1]&0x40) != 0 ){
		//固定値がおかしい
		AddDebugLog(L"CCATUtil::section_syntax_indicator Err");
		return FALSE;
	}
	if( table_id != 0x01 ){
		//table_idがおかしい
		AddDebugLog(L"CCATUtil::table_id Err");
		return FALSE;
	}
	if( readSize+section_length > dataSize || section_length < 5 + 4 ){
		//サイズ異常
		AddDebugLogFormat(L"CCATUtil::section_length %d Err", section_length);
		return FALSE;
	}
	//CRCチェック
	if( CalcCrc32(3+section_length, data) != 0 ){
		AddDebugLog(L"CCATUtil::crc32 Err");
		return FALSE;
	}
	BYTE current_next_indicator = data[readSize+2]&0x01;
	if( current_next_indicator == 0 ){
		//解析不要
		return FALSE;
	}
	lastSection.assign(data, data + dataSize);

	{
		PIDList.clear();
		readSize += 5;
		WORD descriptorSize = (WORD)((section_length+3-4) - readSize);
		if( descriptorSize > 0 ){
			WORD infoRead = 0;
			while(infoRead+1 < descriptorSize){
				BYTE descriptor_tag = data[readSize];
				BYTE descriptor_length = data[readSize+1];
				readSize+=2;

				if( descriptor_tag == 0x09 && descriptor_length >= 4 && infoRead+5 < descriptorSize ){
					//CA
					WORD CA_PID = ((WORD)data[readSize+2]&0x1F)<<8 | (WORD)data[readSize+3];
					if (CA_PID != 0x1fff) {
						PIDList.push_back(CA_PID);
						//AddDebugLogFormat(L"CA_PID:0x%04x", CA_PID);
					}
				}
				readSize += descriptor_length;

				infoRead+= 2+descriptor_length;
			}
		}

	}

	return TRUE;
}
