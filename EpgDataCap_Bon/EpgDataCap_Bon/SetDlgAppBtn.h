#pragma once

#include "AppSetting.h"

// CSetDlgAppBtn ダイアログ

class CSetDlgAppBtn
{
public:
	CSetDlgAppBtn();   // 標準コンストラクター
	~CSetDlgAppBtn();
	BOOL Create(LPCWSTR lpszTemplateName, HWND hWndParent, const APP_SETTING& setting);
	HWND GetSafeHwnd() const{ return m_hWnd; }
	void SaveIni(void);

// ダイアログ データ
	enum { IDD = IDD_DIALOG_SET_APPBTN };

protected:
	HWND m_hWnd;
	const APP_SETTING* m_setting;

	BOOL OnInitDialog();
	afx_msg void OnBnClickedButtonViewExe();
	void OnBnClickedCheckViewSingle();
	static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	HWND GetDlgItem(int nID) const{ return ::GetDlgItem(m_hWnd, nID); }
};
