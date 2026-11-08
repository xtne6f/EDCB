#pragma once

#include "../Common/TSBuffUtil.h"
#include "../Common/TSPacketUtil.h"

class CPATUtil
{
public:
	CPATUtil();
	//Get系メソッドの返値が変化した可能性があるとき真が返る
	bool AddPacket(const CTSPacketUtil& packet);
	//未解析のとき0が返る
	WORD GetTSID() const { return this->transport_stream_id; }
	BYTE GetVersion() const { return this->version_number; }
	const vector<BYTE>& GetSectionData() const { return this->lastSection; }
	const vector<pair<WORD, WORD>>& GetPIDProgramNumberList() const { return this->pidProgramNumberList; }

private:
	CTSBuffUtil buffUtil;
	vector<BYTE> lastSection;
	WORD transport_stream_id;
	BYTE version_number;
	vector<pair<WORD, WORD>> pidProgramNumberList;

	bool DecodePAT(BYTE* data, DWORD dataSize);
};
