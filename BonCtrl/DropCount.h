#pragma once

#include "../Common/Util.h"

#include "../Common/TSPacketUtil.h"
#include "../Common/TimeUtil.h"

class CDropCount
{
public:
	CDropCount(void);
	~CDropCount(void);

	void AddData(BYTE* data, DWORD size);

	void Clear();

	void GetCount(ULONGLONG* drop, ULONGLONG* scramble);
	ULONGLONG GetDropCount();
	ULONGLONG GetScrambleCount();

	void SaveLog(wstring filePath);

	void SetSignal(float level);
	void SetBonDriver(wstring bonDriver);

	void SetPIDName(
		map<WORD, string>* pidName
		);
protected:
	typedef struct _DROP_INFO{
		WORD PID;
		BYTE lastCounter;
		BOOL duplicateFlag;
		ULONGLONG total;
		ULONGLONG drop;
		ULONGLONG scramble;
		_DROP_INFO(){
			PID = 0xFFFF;
			lastCounter = 0;
			duplicateFlag = FALSE;
			total = 0;
			drop = 0;
			scramble = 0;
		};
	}DROP_INFO;

	map<WORD, DROP_INFO> infoMap;
	ULONGLONG drop;
	ULONGLONG scramble;
	vector<wstring> log;
	DWORD lastLogTime;
	ULONGLONG lastLogDrop;
	ULONGLONG lastLogScramble;
	float signalLv;
	wstring bonFile;

	map<WORD, string> pidName;
protected:
	void CheckCounter(CTSPacketUtil* tsPacket, DROP_INFO* info);

};

