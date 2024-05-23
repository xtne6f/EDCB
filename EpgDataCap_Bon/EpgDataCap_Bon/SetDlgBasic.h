#pragma once

#include "AppSetting.h"

// CSetDlgBasic ダイアログ

class CSetDlgBasic
{
public:
	CSetDlgBasic();   // 標準コンストラクター
	~CSetDlgBasic();
	BOOL Create(LPCWSTR lpszTemplateName, HWND hWndParent, const APP_SETTING& setting);
	HWND GetSafeHwnd() const{ return m_hWnd; }
	void SaveIni(void);


// ダイアログ データ
	enum { IDD = IDD_DIALOG_SET_BASIC };

protected:
	HWND m_hWnd;
	const APP_SETTING* m_setting;

	afx_msg void OnBnClickedButtonRecPath();
	afx_msg void OnBnClickedButtonRecAdd();
	afx_msg void OnBnClickedButtonRecDel();
	afx_msg void OnBnClickedButtonRecUp();
	afx_msg void OnBnClickedButtonRecDown();
	afx_msg void OnBnClickedButtonSetPath();
	BOOL OnInitDialog();
	static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	HWND GetDlgItem(int nID) const{ return ::GetDlgItem(m_hWnd, nID); }
};
