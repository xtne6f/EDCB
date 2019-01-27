#pragma once

#include "../../Common/ParseTextInstances.h"

class CEpgTimerSrvSetting
{
public:
	struct SETTING {
		int epgArchivePeriodHour;
		int residentMode;
		int notifyTipStyle;
		bool blinkPreRec;
		bool noBalloonTip;
		bool saveNotifyLog;
		bool saveDebugLog;
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
		bool recInfoFolderOnly;
		bool applyExtToRecInfoDel;
		bool autoDel;
		vector<wstring> delExtList;
		vector<wstring> delChkList;
		int startMargin;
		int endMargin;
		int tuijyuHour;
		bool backPriority;
		bool fixedTunerPriority;
		bool retryOtherTuners;
		bool separateFixedTuners;
		bool commentAutoAdd;
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
		bool pgInfoLogAsUtf8;
		bool dropLog;
		bool recOverWrite;
		int processPriority;
		bool keepDisk;
		wstring tsExt;
		bool enableCaption; //LoadSetting()Ç≈èâä˙âªÇ≥ÇÍÇ»Ç¢
		bool enableData; //LoadSetting()Ç≈èâä˙âªÇ≥ÇÍÇ»Ç¢
	};
	static SETTING LoadSetting(LPCWSTR iniPath);
	static vector<pair<wstring, wstring>> EnumBonFileName(LPCWSTR settingPath);
	INT_PTR ShowDialog();
private:
	static wstring CheckTSExtension(const wstring& ext);
	static vector<wstring> EnumRecNamePlugInFileName();
	static bool GetDlgButtonCheck(HWND hwnd, int id);
	static void SetDlgButtonCheck(HWND hwnd, int id, bool check);
	static void GetWindowTextBuffer(HWND hwnd, vector<WCHAR>& buff);
	static void GetListBoxTextBuffer(HWND hwnd, int index, vector<WCHAR>& buff);
	static void AddListBoxItem(HWND hList, HWND hItem);
	static void DeleteListBoxItem(HWND hList);
	static void MoveListBoxItem(HWND hList, int step);
	static void OnSelChangeListBoxItem(HWND hList, HWND hItem);
	INT_PTR OnInitDialog();
	void OnTcnSelchangeTab();
	void OnBnClickedOk();
	void OnBnClickedSetRecNamePlugIn();
	void OnBnClickedSetEpgServiceVideo();
	void OnLbnSelchangeListSetEpgService();
	void AddEpgTime(bool check);
	void DeleteEpgTime();
	void BrowseFolder(HWND hTarget);
	void BrowseExeFile(HWND hTarget);
	void ToggleStartup(bool execute, bool add);
	static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK ChildDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

	HWND hwndTop;
	HWND hwndBasic;
	HWND hwndEpg;
	HWND hwndRec;
	HWND hwndReserve;
	HWND hwndOther;
	HWND hwndChild[5];
	CParseChText5 chSet;
};
