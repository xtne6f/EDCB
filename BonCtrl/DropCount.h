#pragma once

#include "../Common/Util.h"

class CDropCount
{
public:
	CDropCount(void);

	void AddData(const BYTE* data, DWORD size);

	void Clear();

	void GetCount(ULONGLONG* drop, ULONGLONG* scramble);
	ULONGLONG GetDropCount();
	ULONGLONG GetScrambleCount();

	void SaveLog(wstring filePath);

	void SetSignal(float level);
	void SetBonDriver(wstring bonDriver);
	void SetNoLog(BOOL noLogDrop, BOOL noLogScramble);

	void SetPIDName(
		const map<WORD, string>* pidName
		);
protected:
	struct DROP_INFO {
		BYTE lastCounter;
		BOOL duplicateFlag;
		ULONGLONG total;
		ULONGLONG drop;
		ULONGLONG scramble;
	};

	map<WORD, DROP_INFO> infoMap;
	ULONGLONG drop;
	ULONGLONG scramble;
	string log;
	DWORD lastLogTime;
	ULONGLONG lastLogDrop;
	ULONGLONG lastLogScramble;
	float signalLv;
	wstring bonFile;

	map<WORD, string> pidName;
protected:
	void CheckCounter(const BYTE* packet, DROP_INFO* info);

};

