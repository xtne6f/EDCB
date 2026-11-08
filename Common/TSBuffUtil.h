#pragma once

#include "TSPacketUtil.h"

class CTSBuffUtil
{
public:
	enum ADD_RESULT {
		ERR_FALSE,
		NO_ERR,
		ERR_ADD_NEXT,
		ERR_NOT_SUPPORT,
		INTERNAL_ERR_CARRY_OVER,
	};

	//PES(Packetized Elementary Stream)対応は廃止した
	CTSBuffUtil();

	//Add188TS()がNO_ERRを返せばGetSectionBuff()は1回以上成功する。このとき受け取らなかったバッファは次のAdd188TS()で消える
	ADD_RESULT Add188TS(const CTSPacketUtil& tsPacket_);
	bool GetSectionBuff(BYTE** sectionData, DWORD* dataSize);

protected:
	DWORD sectionSize;
	vector<BYTE> sectionBuff;
	vector<BYTE> carryPacket;

	WORD lastPID;
	BYTE lastCounter;
	bool duplicateFlag;
protected:
	void Clear();
	bool CheckCounter(const CTSPacketUtil* tsPacket);
	ADD_RESULT AddSectionBuff(const BYTE* payload, BYTE payloadSize, BYTE unitStartIndicator);
};
