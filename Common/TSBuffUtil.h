#pragma once

#include "TSPacketUtil.h"

class CTSBuffUtil
{
public:
	static const DWORD ERR_ADD_NEXT = 100;
	static const DWORD ERR_NOT_SUPPORT = 101;

	//PES(Packetized Elementary Stream)対応は廃止した
	CTSBuffUtil();

	//Add188TS()がTRUEを返せばGetSectionBuff()は1回以上成功する。このとき受け取らなかったバッファは次のAdd188TS()で消える
	DWORD Add188TS(CTSPacketUtil* tsPacket);
	BOOL GetSectionBuff(BYTE** sectionData, DWORD* dataSize);

protected:
	DWORD sectionSize;
	vector<BYTE> sectionBuff;
	vector<BYTE> carryPacket;

	WORD lastPID;
	BYTE lastCounter;
	BOOL duplicateFlag;
protected:
	void Clear();
	BOOL CheckCounter(CTSPacketUtil* tsPacket);
	DWORD AddSectionBuff(CTSPacketUtil* tsPacket);
};
