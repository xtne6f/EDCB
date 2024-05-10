#pragma once

#include "../../BonCtrl/BonCtrl.h"
#include "../../Common/MessageManager.h"
#include "../../Common/PipeServer.h"

class CEpgDataCap_BonMin
{
public:
	CEpgDataCap_BonMin();
	void Main();
	void ParseCommandLine(char** argv, int argc);

private:
	int ReloadServiceList(int selONID = -1, int selTSID = -1, int selSID = -1);
	void PrintStatusLog(LPCWSTR log);
	void OnInit();
	void OnDestroy();
	static bool OnMessage(CMessageManager::PARAMS& pa);
	void StartPipeServer();
	void OnTimerStatusUpdate();
	void CtrlCmdCallbackInvoked();

	bool overWriteFlag;
	int dropSaveThresh;
	int scrambleSaveThresh;
	DWORD tsBuffMaxCount;
	int writeBuffMaxCount;
	int traceBonDriverLevel;
	int openWait;
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

	CMessageManager msgManager;
	CBonCtrl bonCtrl;
	CPipeServer pipeServer;
	int outCtrlID;
	vector<DWORD> cmdCtrlList;
	const CCmdStream* cmdCapture;
	CCmdStream* resCapture;

	vector<CH_DATA4> serviceList;
	int lastONID;
	int lastTSID;
	DWORD recCtrlID;
	bool chScanWorking;
	bool epgCapWorking;
	wstring lastChScanLog;
	size_t printStatusSpace;
};
