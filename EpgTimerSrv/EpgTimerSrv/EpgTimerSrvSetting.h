#pragma once

class CEpgTimerSrvSetting
{
public:
	struct SETTING {
		int epgArchivePeriodHour;
		int residentMode;
		bool noBalloonTip;
		bool saveNotifyLog;
		DWORD wakeTime;
		int autoAddHour;
		bool chkGroupEvent;
		BYTE recEndMode;
		bool reboot;
		bool noUsePC;
		DWORD noUsePCTime;
		bool noFileStreaming;
		bool noShareFile;
		DWORD noStandbyTime;
		vector<wstring> noSuspendExeList;
		vector<wstring> viewBonList;
		DWORD ngEpgCapTime;
		DWORD ngEpgCapTunerTime;
		bool timeSync;
		vector<pair<bool, pair<int, int>>> epgCapTimeList;
		bool autoDel;
		vector<wstring> delExtList;
		vector<wstring> delChkList;
		int startMargin;
		int endMargin;
		int tuijyuHour;
		bool backPriority;
		bool fixedTunerPriority;
		bool autoDelRecInfo;
		DWORD autoDelRecInfoNum;
		DWORD recInfo2Max;
		int recInfo2DropChk;
		wstring recInfo2RegExp;
		bool errEndBatRun;
		bool recNamePlugIn;
		wstring recNamePlugInFile;
		bool noChkYen;
		int delReserveMode;
		int recAppWakeTime;
		bool recMinWake;
		bool recView;
		bool recNW;
		bool pgInfoLog;
		bool dropLog;
		bool recOverWrite;
		int processPriority;
		bool keepDisk;
	};
	static SETTING LoadSetting(LPCWSTR iniPath);
	static vector<pair<wstring, wstring>> EnumBonFileName(LPCWSTR settingPath);
};
