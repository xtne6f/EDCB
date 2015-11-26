#pragma once

#include "TSPacketUtil.h"

#define ERR_ADD_NEXT		100
#define ERR_NOT_SUPPORT		101

class CTSBuffUtil
{
public:
	CTSBuffUtil(BOOL supportPES = FALSE);
	~CTSBuffUtil(void);

	//Add188TS()がTRUEを返せばGetSectionBuff()は1回以上成功する。このとき受け取らなかったバッファは次のAdd188TS()で消える
	DWORD Add188TS(CTSPacketUtil* tsPacket);
	BOOL GetSectionBuff(BYTE** sectionData, DWORD* dataSize);
	BOOL IsPES();

protected:
	DWORD sectionSize;
	vector<BYTE> sectionBuff;
	vector<BYTE> carryPacket;

	WORD lastPID;
	BYTE lastCounter;
	BOOL duplicateFlag;

	BOOL supportPES;
	BOOL PESMode;
protected:
	void Clear();
	BOOL CheckCounter(CTSPacketUtil* tsPacket);
	DWORD AddSectionBuff(CTSPacketUtil* tsPacket);
	DWORD AddPESBuff(CTSPacketUtil* tsPacket);
};
