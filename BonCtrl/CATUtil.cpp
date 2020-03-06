#include "stdafx.h"
#include "CATUtil.h"

#include "../Common/EpgTimerUtil.h"


CCATUtil::CCATUtil(void)
{
}

BOOL CCATUtil::AddPacket(CTSPacketUtil* packet)
{
	if( packet == NULL ){
		return FALSE;
	}
	if( buffUtil.Add188TS(packet) == TRUE ){
		BYTE* section = NULL;
		DWORD sectionSize = 0;
		while( buffUtil.GetSectionBuff( &section, &sectionSize ) == TRUE ){
			if( DecodeCAT(section, sectionSize) == FALSE ){
				return FALSE;
			}
		}
	}else{
		return FALSE;
	}
	return TRUE;
}

BOOL CCATUtil::DecodeCAT(BYTE* data, DWORD dataSize)
{
	if( data == NULL ){
		return FALSE;
	}
	PIDList.clear();

	if( dataSize < 7 ){
		return FALSE;
	}

	DWORD readSize = 0;
	//////////////////////////////////////////////////////
	//解析処理
	table_id = data[0];
	section_syntax_indicator = (data[1]&0x80)>>7;
	section_length = ((WORD)data[1]&0x0F)<<8 | data[2];
	readSize+=3;

	if( section_syntax_indicator != 1 || (data[1]&0x40) != 0 ){
		//固定値がおかしい
		return FALSE;
	}
	if( table_id != 0x01 ){
		//table_idがおかしい
		return FALSE;
	}
	if( readSize+section_length > dataSize || section_length < 4){
		//サイズ異常
		return FALSE;
	}
	//CRCチェック
	if( CalcCrc32(3+section_length, data) != 0 ){
		return FALSE;
	}

	if( section_length > 8 ){
		version_number = (data[readSize+2]&0x3E)>>1;
		current_next_indicator = data[readSize+2]&0x01;
		section_number = data[readSize+3];
		last_section_number = data[readSize+4];
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
						//_OutputDebugString(L"CA_PID:0x%04x\r\n",CA_PID);
					}
				}
				readSize += descriptor_length;

				infoRead+= 2+descriptor_length;
			}
		}

	}

	return TRUE;
}
