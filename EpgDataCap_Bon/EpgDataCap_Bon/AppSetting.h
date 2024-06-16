#pragma once

#include "../../BonCtrl/BonCtrlDef.h"

struct APP_SETTING
{
#ifdef _WIN32
	bool modifyTitleBarText;
	bool overlayTaskIcon;
	bool minTask;
	int dialogTemplate;
	wstring viewPath;
	wstring viewOption;
	bool viewSingle;
	bool viewCloseOnExit;
#endif
	bool allService;
	bool scramble;
	bool emm;
	bool enableCaption;
	bool enableData;
	bool overWrite;
	int openWait;
	int dropSaveThresh;
	int scrambleSaveThresh;
	bool noLogScramble;
	wstring recFileName;
#ifdef _WIN32
	bool openLast;
	bool dropLogAsUtf8;
#endif
	bool saveDebugLog;
	int traceBonDriverLevel;
	DWORD tsBuffMaxCount;
	int writeBuffMaxCount;
	bool epgCapBackBSBasic;
	bool epgCapBackCS1Basic;
	bool epgCapBackCS2Basic;
	bool epgCapBackCS3Basic;
	bool epgCapLive;
	bool epgCapRec;
	bool parseEpgPostProcess;
	DWORD epgCapBackStartWaitSec;
	bool saveLogo;
	DWORD saveLogoTypeFlags;
	vector<NW_SEND_INFO> udpSendList;
	vector<NW_SEND_INFO> tcpSendList;

	static APP_SETTING Load(LPCWSTR iniPath);
};
