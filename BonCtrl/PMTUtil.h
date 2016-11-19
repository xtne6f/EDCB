#pragma once

#include "../Common/ErrDef.h"
#include "../Common/TSBuffUtil.h"
#include "../Common/TSPacketUtil.h"

class CPMTUtil
{
public:
	map<WORD,WORD> PIDList;
	WORD program_number;
	WORD PCR_PID;

public:
	CPMTUtil(void);

	BOOL AddPacket(CTSPacketUtil* packet);

protected:
	CTSBuffUtil buffUtil;

	BYTE table_id;
	BYTE section_syntax_indicator;
	WORD section_length;
//	WORD program_number;
	BYTE version_number;
	BYTE current_next_indicator;
	BYTE section_number;
	BYTE last_section_number;
//	WORD PCR_PID;
	WORD program_info_length;

protected:
	void Clear();
	BOOL DecodePMT(BYTE* data, DWORD dataSize);

};

