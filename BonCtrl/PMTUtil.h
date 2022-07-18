#pragma once

#include "../Common/TSBuffUtil.h"
#include "../Common/TSPacketUtil.h"

class CPMTUtil
{
public:
	CPMTUtil(void);
	//Get系メソッドの返値が変化した可能性があるとき真が返る
	BOOL AddPacket(const CTSPacketUtil& packet);
	//未解析のとき0が返る
	WORD GetProgramNumber() const { return program_number; }
	BYTE GetVersion() const { return version_number; }
	const vector<BYTE>& GetSectionData() const { return lastSection; }
	WORD GetPcrPID() const { return PCR_PID; }
	const vector<pair<WORD, BYTE>>& GetPIDTypeList() const { return PIDList; }

protected:
	CTSBuffUtil buffUtil;
	vector<BYTE> lastSection;
	WORD program_number;
	BYTE version_number;
	WORD PCR_PID;
	vector<pair<WORD, BYTE>> PIDList;

protected:
	BOOL DecodePMT(BYTE* data, DWORD dataSize);

};

