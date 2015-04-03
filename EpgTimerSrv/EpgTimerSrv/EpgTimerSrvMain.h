#pragma once

#include "EpgDBManager.h"
#include "ReserveManager.h"
#include "SleepUtil.h"
#include "FileStreamingManager.h"
#include "NotifyManager.h"

#include "../../Common/ParseEpgAutoAddText.h"
#include "../../Common/ParseManualAutoAddText.h"
#include "../../Common/PipeServer.h"
#include "../../Common/TCPServer.h"
#include "../../Common/HttpServer.h"
#include "../../Common/TCPServerUtil.h"

#include "DLNAManager.h"

class CEpgTimerSrvMain
{
public:
	CEpgTimerSrvMain(void);
	~CEpgTimerSrvMain(void);

	//メインループ処理
	//引数：
	// serviceFlag			[IN]サービスとしての起動かどうか
	void StartMain(
		BOOL serviceFlag
		);

	//メイン処理停止
	void StopMain();

	//休止／スタンバイ移行処理中かどうか
	//戻り値：
	// TRUE（移行中）、FALSE
	BOOL IsSuspending();

	//休止／スタンバイに移行して構わない状況かどうか
	//戻り値：
	// TRUE（構わない）、FALSE（移行しては駄目）
	BOOL ChkSuspend();

	//ユーザーがPCを使用中かどうか
	//戻り値：
	// TRUE（使用中）、FALSE（使用していない）
	BOOL IsUserWorking();

protected:
	HANDLE lockEvent;

	CEpgDBManager epgDB;
	CReserveManager reserveManager;
	CSleepUtil sleepUtil;
	CFileStreamingManager streamingManager;
	CNotifyManager notifyManager;

	CParseEpgAutoAddText epgAutoAdd;
	CParseManualAutoAddText manualAutoAdd;

	CPipeServer* pipeServer;
	CTCPServer* tcpServer;
	CHttpServer* httpServer;
	CTCPServerUtil* tcpSrvUtil;

	CDLNAManager* dlnaManager;

	HANDLE stopEvent;

	BOOL reloadEpgChkFlag;

	//map<DWORD,DWORD> registGUI; //PID,PID
	//map<wstring,REGIST_TCP_INFO> registTCP; //IP:Port

	BYTE suspendMode;
	BYTE rebootFlag;
	HANDLE sleepThread;
	BYTE suspendModeWork;
	BYTE rebootFlagWork;

	BOOL suspending;

	int wakeMargin;
	BOOL enableTCPSrv;
	DWORD tcpPort;
	int autoAddDays;
	int autoAddHour;
	BOOL chkGroupEvent;
	BYTE rebootDef;
	BOOL ngUsePC;
	DWORD ngUsePCTime;
	BOOL ngFileStreaming;
	BOOL ngEpgFileSrvCoop;
	BOOL enableHttpSrv;
	DWORD httpPort;
	BOOL enableDMS;

	BOOL enableHttpPublic;
	wstring httpPublicFolder;

	BOOL awayMode;

	vector<OLD_EVENT_INFO_DATA3> oldSearchList;
protected:
	//コマンド関係排他制御用
	BOOL Lock(LPCWSTR log = NULL, DWORD timeOut = 60*1000);
	void UnLock(LPCWSTR log = NULL);

	BOOL CheckTuijyu();

	BOOL AutoAddReserveEPG();
	BOOL AutoAddReserveProgram();

	void StartSleep(BYTE rebootFlag, BYTE suspendMode);
	void StartReboot();
	static UINT WINAPI SleepThread(void* param);
	BOOL QuerySleep(BYTE rebootFlag, BYTE suspendMode);
	BOOL QueryReboot(BYTE rebootFlag);

	void ReloadSetting();

	void AddRecFileDMS();

	//外部制御コマンド関係
	static int CALLBACK CtrlCmdCallback(void* param, CMD_STREAM* cmdParam, CMD_STREAM* resParam);
	static int CALLBACK HttpCallback(void* param, HTTP_STREAM* recvParam, HTTP_STREAM* sendParam);
	static int CALLBACK TcpAcceptCallback(void* param, SOCKET clientSock, struct sockaddr_in* client, HANDLE stopEvent);


};

