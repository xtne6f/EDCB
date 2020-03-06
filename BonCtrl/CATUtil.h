#pragma once

#include "../Common/ErrDef.h"
#include "../Common/TSBuffUtil.h"
#include "../Common/TSPacketUtil.h"

class CCATUtil
{
public:
	CCATUtil(void);

	BOOL AddPacket(CTSPacketUtil* packet);
	const vector<WORD>& GetPIDList() const { return PIDList; }

protected:
	CTSBuffUtil buffUtil;

	BYTE table_id;
	BYTE section_syntax_indicator;
	WORD section_length;
	BYTE version_number;
	BYTE current_next_indicator;
	BYTE section_number;
	BYTE last_section_number;
	vector<WORD> PIDList;

protected:
	BOOL DecodeCAT(BYTE* data, DWORD dataSize);
};

