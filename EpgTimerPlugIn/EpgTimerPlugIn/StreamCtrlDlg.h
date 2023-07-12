#pragma once

#include "../../Common/SendCtrlCmd.h"
#include "../../Common/StringUtil.h"


class CStreamCtrlDlg
{
public:
	enum {
		WM_CHG_PORT = WM_APP,
		WM_PLAY_CLOSE,
		WM_CUSTOM = WM_APP + 0x100
	};
	typedef LRESULT (CALLBACK *MessageCallbackFunc)(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, void* param);

	CStreamCtrlDlg(void);
	~CStreamCtrlDlg(void);

	void SetCtrl(const TVTEST_STREAMING_INFO& info);
	void SetMessageCallback(MessageCallbackFunc func, void* param = NULL);
	DWORD CreateStreamCtrlDialog(HINSTANCE hInstance, HWND parentHWND);
	void CloseStreamCtrlDialog();

	void StopTimer();

	BOOL ShowCtrlDlg(DWORD cmdShow);
	HWND GetDlgHWND(){ return this->hwnd; }

	void StartFullScreenMouseChk();
	void StopFullScreenMouseChk();
protected:
	void SetNWModeSend();

	static LRESULT CALLBACK DlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp);

protected:
	HWND hwnd;
	HWND parentHwnd;
	CSendCtrlCmd cmd;
	BOOL ctrlEnabled;
	DWORD ctrlID;
	BOOL ctrlIsNetwork;
	MessageCallbackFunc callbackFunc;
	void* callbackParam;
	BOOL thumbTracking;
	int getPosState;
	LONGLONG measuredTotalPos;
	LONGLONG measuredFilePos;
	LONGLONG totalPosDelta;
	LONGLONG filePosDelta;
};

