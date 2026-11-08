#pragma once

class CDropCount
{
public:
	CDropCount(void);

	void AddData(const BYTE* data, DWORD size);

	void Clear();
	ULONGLONG GetDropCount();
	ULONGLONG GetScrambleCount();

	void SaveLog(const wstring& filePath, bool asUtf8);

	void SetSignal(float level);
	void SetBonDriver(const wstring& bonDriver);
	void SetNoLog(bool noLogDrop, bool noLogScramble);

	void SetPIDName(WORD pid, const wstring& name);
protected:
	struct DROP_INFO {
		WORD first; //PID
		BYTE lastCounter;
		bool duplicateFlag;
		ULONGLONG total;
		ULONGLONG drop;
		ULONGLONG scramble;
	};

	vector<DROP_INFO> infoList;
	ULONGLONG drop;
	ULONGLONG scramble;
	string log;
	DWORD lastLogTime;
	ULONGLONG lastLogDrop;
	ULONGLONG lastLogScramble;
	float signalLv;
	wstring bonFile;

	vector<pair<WORD, wstring>> pidName;
protected:
	void CheckCounter(const BYTE* packet, DROP_INFO* info);

};

