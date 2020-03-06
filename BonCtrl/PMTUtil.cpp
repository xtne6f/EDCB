#include "stdafx.h"
#include "PMTUtil.h"

#include "../Common/EpgTimerUtil.h"


CPMTUtil::CPMTUtil(void)
{
	this->program_number = 0xFFFF;
	this->PCR_PID = 0xFFFF;
}

BOOL CPMTUtil::AddPacket(CTSPacketUtil* packet)
{
	if( packet == NULL ){
		return FALSE;
	}
	if( buffUtil.Add188TS(packet) == TRUE ){
		BYTE* section = NULL;
		DWORD sectionSize = 0;
		while( buffUtil.GetSectionBuff( &section, &sectionSize ) == TRUE ){
			if( DecodePMT(section, sectionSize) == FALSE ){
				return FALSE;
			}
		}
	}else{
		return FALSE;
	}
	return TRUE;
}

void CPMTUtil::Clear()
{
	PIDList.clear();
}

BOOL CPMTUtil::DecodePMT(BYTE* data, DWORD dataSize)
{
	Clear();

	if( data == NULL ){
		return FALSE;
	}

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

	if( section_syntax_indicator != 1 ){
		//固定値がおかしい
		_OutputDebugString(L"CPMTUtil::section_syntax_indicator Err");
		return FALSE;
	}
	if( table_id != 0x02 ){
		//table_idがおかしい
		_OutputDebugString(L"CPMTUtil::table_id Err");
		return FALSE;
	}
	if( readSize+section_length > dataSize || section_length < 4){
		//サイズ異常
		_OutputDebugString(L"CPMTUtil::section_length %d Err", section_length);
		return FALSE;
	}
	//CRCチェック
	if( CalcCrc32(3+section_length, data) != 0 ){
		_OutputDebugString(L"CPMTUtil::crc32 Err");
		return FALSE;
	}

	if( section_length > 12 ){
		program_number = ((WORD)data[readSize])<<8 | data[readSize+1];
		version_number = (data[readSize+2]&0x3E)>>1;
		current_next_indicator = data[readSize+2]&0x01;
		section_number = data[readSize+3];
		last_section_number = data[readSize+4];
		PCR_PID = ((WORD)data[readSize+5]&0x1F)<<8 | data[readSize+6];
		program_info_length = ((WORD)data[readSize+7]&0x0F)<<8 | data[readSize+8];
		readSize += 9;

		//descriptor
		WORD infoRead = 0;
		while(readSize+1 < (DWORD)section_length+3-4 && infoRead < program_info_length){
			BYTE descriptor_tag = data[readSize];
			BYTE descriptor_length = data[readSize+1];
			readSize+=2;

			if( descriptor_tag == 0x09 && descriptor_length >= 4 && readSize+3 < (DWORD)section_length+3-4 ){
				//CA
				WORD CA_PID = ((WORD)data[readSize+2]&0x1F)<<8 | (WORD)data[readSize+3];
				if (CA_PID != 0x1fff) {
					PIDList.push_back(std::make_pair(CA_PID, (BYTE)0));
				}
			}
			readSize += descriptor_length;

			infoRead+= 2+descriptor_length;
		}

		while( readSize+4 < (DWORD)section_length+3-4 ){
			BYTE stream_type = data[readSize];
			WORD elementary_PID = ((WORD)data[readSize+1]&0x1F)<<8 | data[readSize+2];
			WORD ES_info_length = ((WORD)data[readSize+3]&0x0F)<<8 | data[readSize+4];
			readSize += 5;

			PIDList.push_back(std::make_pair(elementary_PID, stream_type));

			//descriptor
			infoRead = 0;
			while(readSize+1 < (DWORD)section_length+3-4 && infoRead < ES_info_length){
				BYTE descriptor_tag = data[readSize];
				BYTE descriptor_length = data[readSize+1];
				readSize+=2;

				if( descriptor_tag == 0x09 && descriptor_length >= 4 && readSize+3 < (DWORD)section_length+3-4 ){
					//CA
					WORD CA_PID = ((WORD)data[readSize+2]&0x1F)<<8 | (WORD)data[readSize+3];
					if (CA_PID != 0x1fff) {
						PIDList.push_back(std::make_pair(CA_PID, (BYTE)0));
					}
				}
				readSize += descriptor_length;

				infoRead+= 2+descriptor_length;
			}

//			readSize+=item->ES_info_length;
		}
	}

	return TRUE;
}
