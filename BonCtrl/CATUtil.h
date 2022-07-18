#pragma once

#include "../Common/TSBuffUtil.h"
#include "../Common/TSPacketUtil.h"

class CCATUtil
{
public:
	//Get系メソッドの返値が変化した可能性があるとき真が返る
	BOOL AddPacket(const CTSPacketUtil& packet);
	const vector<WORD>& GetPIDList() const { return PIDList; }

protected:
	CTSBuffUtil buffUtil;
	vector<BYTE> lastSection;
	vector<WORD> PIDList;

protected:
	BOOL DecodeCAT(BYTE* data, DWORD dataSize);
};

