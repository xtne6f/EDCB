#pragma once

#include "../../Common/SendCtrlCmd.h"
#include "../../Common/StringUtil.h"


class CStreamCtrlDlg
{
public:
	enum {
		WM_RESET_GUI = WM_APP,
		WM_CHG_PORT,
		WM_PLAY_CLOSE,
		WM_CUSTOM = WM_APP + 0x100
	};
	typedef LRESULT (CALLBACK *MessageCallbackFunc)(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, void* param);

	CStreamCtrlDlg(void);
	~CStreamCtrlDlg(void);

	void SetCtrlCmd(CSendCtrlCmd* ctrlCmd, DWORD ctrlID_, BOOL chkUdp, BOOL chkTcp, BOOL play, BOOL timeShiftMode_);
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
	void EnumIP();
	void UpdateLog();

	static LRESULT CALLBACK DlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp);

protected:
	HWND hwnd;
	HWND parentHwnd;
	CSendCtrlCmd* cmd;
	DWORD ctrlID;
	BOOL iniTCP;
	BOOL iniUDP;
	BOOL timeShiftMode;
	MessageCallbackFunc callbackFunc;
	void* callbackParam;

	NWPLAY_PLAY_INFO nwPlayInfo;
};

