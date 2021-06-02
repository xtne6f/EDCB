#pragma once

#define TVTEST_PLUGIN_CLASS_IMPLEMENT	// クラスとして実装
#define TVTEST_PLUGIN_VERSION TVTEST_PLUGIN_VERSION_(0,0,13)
#include "../../Common/TVTestPlugin.h"

#include "../../Common/PipeServer.h"
#include "../../Common/ErrDef.h"
#include "../../Common/CtrlCmdDef.h"
#include "../../Common/CtrlCmdUtil.h"
#include "../../Common/ThreadUtil.h"

#include "StreamCtrlDlg.h"

class CEpgTimerPlugIn : public TVTest::CTVTestPlugin
{
private:
	CPipeServer pipeServer;

	BOOL nwMode;
	TVTEST_STREAMING_INFO nwModeInfo;
	BOOL fullScreen;
	BOOL showNormal;
	BOOL grantServerAccess;
	CStreamCtrlDlg ctrlDlg;
	CCmdStream cmdCapture;
	CCmdStream resCapture;
	recursive_mutex_ cmdLock;

private:
	static LRESULT CALLBACK EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData);
	void CtrlCmdCallbackInvoked();
	static BOOL CALLBACK WindowMsgeCallback(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,LRESULT *pResult,void *pUserData);
	static LRESULT CALLBACK StreamCtrlDlgCallback(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,void *pUserData);
	void ResetStreamingCtrlView();

public:
	CEpgTimerPlugIn();
	virtual bool GetPluginInfo(TVTest::PluginInfo *pInfo);
	virtual bool Initialize();
	virtual bool Finalize();

	void EnablePlugin(BOOL enable);
};
