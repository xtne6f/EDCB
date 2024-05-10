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
		int noBalloonTip;
		bool saveNotifyLog;
		bool saveDebugLog;
		DWORD wakeTime;
		int autoAddHour;
		bool chkGroupEvent;
		BYTE recEndMode;
		bool reboot;
		bool noFileStreaming;
		DWORD noStandbyTime;
#ifdef _WIN32
		bool noUsePC;
		DWORD noUsePCTime;
		bool noShareFile;
		vector<wstring> noSuspendExeList;
#endif
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
		bool fixNoRecToServiceOnly;
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
		bool openViewForViewing;
		bool openViewForRec;
		bool openViewAlways;
		bool recNW;
		bool pgInfoLog;
		bool pgInfoLogAsUtf8;
		bool dropLog;
		bool recOverWrite;
		int processPriority;
		bool keepDisk;
		wstring tsExt;
		bool enableCaption;
		bool enableData;
	};
	static SETTING LoadSetting(LPCWSTR iniPath);
	static vector<pair<wstring, wstring>> EnumBonFileName(LPCWSTR settingPath);
#ifdef _WIN32
	INT_PTR ShowDialog();
#endif
private:
	static wstring CheckTSExtension(const wstring& ext);
#ifdef _WIN32
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
#endif
};
