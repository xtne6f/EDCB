#pragma once

#include "../../BonCtrl/BonCtrl.h"
#include "../../Common/MessageManager.h"
#include "../../Common/PipeServer.h"
#include "AppSetting.h"

class CEpgDataCap_BonMin
{
public:
	CEpgDataCap_BonMin();
	void Main();
	void ParseCommandLine(char** argv, int argc);
	static bool ValidateCommandLine(char** argv, int argc);

private:
	int ReloadServiceList(int selONID = -1, int selTSID = -1, int selSID = -1);
	void PrintStatusLog(LPCWSTR log);
	void OnInit();
	void OnDestroy();
	void OnStartRec();
	static bool OnMessage(CMessageManager::PARAMS& pa);
	void StartPipeServer();
	void OnTimerStatusUpdate();
	void CtrlCmdCallbackInvoked();

	APP_SETTING setting;
	vector<wstring> recFolderList;

	wstring iniBonDriver;
	bool iniNetwork;
	bool iniUDP;
	bool iniTCP;
	int iniONID;
	int iniTSID;
	int iniSID;
	bool iniChScan;
	bool iniEpgCap;
	bool iniRec;

	CMessageManager msgManager;
	CBonCtrl bonCtrl;
	CPipeServer pipeServer;
	vector<DWORD> cmdCtrlList;
	const CCmdStream* cmdCapture;
	CCmdStream* resCapture;

	recursive_mutex_ statusInfoLock;
	VIEW_APP_STATUS_INFO statusInfo;

	vector<CH_DATA4> serviceList;
	DWORD recCtrlID;
	bool chScanWorking;
	bool epgCapWorking;
	wstring lastChScanLog;
	size_t printStatusSpace;
};
