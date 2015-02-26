#pragma once

#include "EpgDBManager.h"
#include "ReserveManager.h"
#include "FileStreamingManager.h"
#include "NotifyManager.h"
#include "../../Common/ParseTextInstances.h"

//各種サーバと自動予約の管理をおこなう
//必ずオブジェクト生成→Main()→…→破棄の順番で利用しなければならない
class CEpgTimerSrvMain
{
public:
	CEpgTimerSrvMain();
	~CEpgTimerSrvMain();
	//メインループ処理
	//serviceFlag_: サービスとしての起動かどうか
	bool Main(bool serviceFlag_);
	//メイン処理停止
	void StopMain();
	//休止／スタンバイに移行して構わない状況かどうか
	bool IsSuspendOK(); //const;
private:
	void ReloadSetting();
	//現在の予約状態に応じた復帰タイマをセットする
	bool SetResumeTimer(HANDLE* resumeTimer, __int64* resumeTime, DWORD marginSec);
	//システムをシャットダウンする
	static void SetShutdown(BYTE shutdownMode);
	//GUIにシャットダウン可能かどうかの問い合わせを開始させる
	//suspendMode==0:再起動(常にrebootFlag==1とする)
	//suspendMode!=0:スタンバイ休止または電源断
	bool QueryShutdown(BYTE rebootFlag, BYTE suspendMode);
	//ユーザーがPCを使用中かどうか
	bool IsUserWorking() const;
	//抑制条件のプロセスが起動しているかどうか
	bool IsFindNoSuspendExe() const;
	bool AutoAddReserveEPG(const EPG_AUTO_ADD_DATA& data);
	bool AutoAddReserveProgram(const MANUAL_AUTO_ADD_DATA& data);
	//外部制御コマンド関係
	static int CALLBACK CtrlCmdCallback(void* param, CMD_STREAM* cmdParam, CMD_STREAM* resParam);

	CNotifyManager notifyManager;
	CEpgDBManager epgDB;
	//reserveManagerはnotifyManagerとepgDBに依存するので、順序を入れ替えてはいけない
	CReserveManager reserveManager;
	CFileStreamingManager streamingManager;

	CParseEpgAutoAddText epgAutoAdd;
	CParseManualAutoAddText manualAutoAdd;

	mutable CRITICAL_SECTION settingLock;
	HANDLE requestEvent;
	bool requestStop;
	bool requestResetServer;
	bool requestReloadEpgChk;
	WORD requestShutdownMode;

	bool serviceFlag;
	DWORD wakeMarginSec;
	unsigned short tcpPort;
	int autoAddHour;
	bool chkGroupEvent;
	//LOBYTEにモード(1=スタンバイ,2=休止,3=電源断,4=なにもしない)、HIBYTEに再起動フラグ
	WORD defShutdownMode;
	DWORD ngUsePCTime;
	bool ngFileStreaming;
	vector<wstring> noSuspendExeList;
	vector<wstring> tvtestUseBon;
	bool nwtvUdp;
	bool nwtvTcp;

	vector<OLD_EVENT_INFO_DATA3> oldSearchList;
};
