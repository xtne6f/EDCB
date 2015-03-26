#pragma once

#include "EpgDBManager.h"
#include "ReserveManager.h"
#include "FileStreamingManager.h"
#include "NotifyManager.h"

#include "../../Common/ParseTextInstances.h"
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
	CEpgDBManager epgDB;
	CReserveManager reserveManager;
	CFileStreamingManager streamingManager;
	CNotifyManager notifyManager;

	CParseEpgAutoAddText epgAutoAdd;
	CParseManualAutoAddText manualAutoAdd;

	CDLNAManager* dlnaManager;

	HANDLE stopEvent;
	HANDLE sleepEvent;
	HANDLE resetServerEvent;
	HANDLE reloadEpgChkEvent;

	//map<DWORD,DWORD> registGUI; //PID,PID
	//map<wstring,REGIST_TCP_INFO> registTCP; //IP:Port

	BYTE suspendModeWork;
	BYTE rebootFlagWork;

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

	//このオブジェクトが持つ設定変数を保護するロック
	//独自にアトミック性を確保するオブジェクトは対象外
	CRITICAL_SECTION settingLock;
protected:
	BOOL CheckTuijyu();

	BOOL AutoAddReserveEPG(int targetSize = -1, EPG_AUTO_ADD_DATA* target = NULL);
	BOOL AutoAddReserveProgram();

	static void SetShutdown(BYTE shutdownMode);
	BOOL QuerySleep(BYTE rebootFlag, BYTE suspendMode);
	BOOL QueryReboot(BYTE rebootFlag);

	BOOL SetResumeTimer(HANDLE* resumeTimer, LONGLONG* resumeTime, BOOL rebootFlag);
	void ResetServer(CTCPServer& tcpServer, CHttpServer& httpServer, CTCPServerUtil& tcpSrvUtil);
	void ReloadSetting();

	void AddRecFileDMS();

	//外部制御コマンド関係
	static int CALLBACK CtrlCmdCallback(void* param, CMD_STREAM* cmdParam, CMD_STREAM* resParam);
	static int CALLBACK HttpCallback(void* param, HTTP_STREAM* recvParam, HTTP_STREAM* sendParam);
	static int CALLBACK TcpAcceptCallback(void* param, SOCKET clientSock, struct sockaddr_in* client, HANDLE stopEvent);


};

