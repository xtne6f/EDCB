#pragma once

class CSettingDlg
{
public:
	CSettingDlg(void);
	~CSettingDlg(void);

	DWORD CreateSettingDialog(HINSTANCE hInstance, HWND parentWnd);

public:
	wstring macro;

protected:
	static LRESULT CALLBACK DlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp);
};

