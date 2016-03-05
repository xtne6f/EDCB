#pragma once

#define TVTEST_PLUGIN_CLASS_IMPLEMENT	// クラスとして実装
#define TVTEST_PLUGIN_VERSION TVTEST_PLUGIN_VERSION_(0,0,13)
#include "../../Common/TVTestPlugin.h"

#include "../../Common/PipeServer.h"
#include "../../Common/ErrDef.h"
#include "../../Common/CtrlCmdDef.h"
#include "../../Common/CtrlCmdUtil.h"
#include "../../Common/SendCtrlCmd.h"

#include "StreamCtrlDlg.h"

class CEpgTimerPlugIn : public TVTest::CTVTestPlugin
{
private:
	CPipeServer pipeServer;

	BOOL nwMode;
	DWORD nwModeCurrentCtrlID;
	TVTEST_STREAMING_INFO nwModeInfo;
	CSendCtrlCmd cmd;
	BOOL fullScreen;
	BOOL showNormal;
	CStreamCtrlDlg ctrlDlg;
	struct CMD_STREAM_POOL {
		DWORD param;
		DWORD dataSize;
		BYTE data[2048]; // 扱うパイプコマンドの最大データサイズで十分
	} cmdPool, resPool;
	CRITICAL_SECTION cmdLock;

private:
	static LRESULT CALLBACK EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData);
	static int CALLBACK CtrlCmdCallback(void* param, CMD_STREAM* cmdParam, CMD_STREAM* resParam);
	void CtrlCmdCallbackInvoked();
	static BOOL CALLBACK WindowMsgeCallback(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,LRESULT *pResult,void *pUserData);
	static LRESULT CALLBACK StreamCtrlDlgCallback(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,void *pUserData);
	void ResetStreamingCtrlView();

public:
	CEpgTimerPlugIn();
	~CEpgTimerPlugIn();
	virtual bool GetPluginInfo(TVTest::PluginInfo *pInfo);
	virtual bool Initialize();
	virtual bool Finalize();

	void EnablePlugin(BOOL enable);
};

class CAutoLock
{
public:
	CAutoLock(LPCRITICAL_SECTION lock_) : lock(lock_) { EnterCriticalSection(this->lock); }
	~CAutoLock() { LeaveCriticalSection(this->lock); }
private:
	LPCRITICAL_SECTION lock;
};
