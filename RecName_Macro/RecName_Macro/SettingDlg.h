#pragma once

class CSettingDlg
{
public:
	INT_PTR CreateSettingDialog(HINSTANCE hInstance, HWND parentWnd, wstring& macro_);

protected:
	static INT_PTR CALLBACK DlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp);
	wstring* macro;
};

