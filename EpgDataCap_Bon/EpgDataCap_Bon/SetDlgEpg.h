#pragma once

#include "AppSetting.h"
#include "../../Common/ParseTextInstances.h"

// CSetDlgEpg ダイアログ

class CSetDlgEpg
{
public:
	CSetDlgEpg();   // 標準コンストラクター
	~CSetDlgEpg();
	BOOL Create(LPCWSTR lpszTemplateName, HWND hWndParent, const APP_SETTING& setting);
	HWND GetSafeHwnd() const{ return m_hWnd; }
	void SaveIni(void);

// ダイアログ データ
	enum { IDD = IDD_DIALOG_SET_EPG };
	
protected:
	HWND m_hWnd;
	const APP_SETTING* m_setting;
	CParseChText5 chSet;

	BOOL OnInitDialog();
	afx_msg void OnBnClickedButtonAllChk();
	afx_msg void OnBnClickedButtonVideoChk();
	afx_msg void OnBnClickedButtonAllClear();
	void OnBnClickedSaveLogo();
	static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	HWND GetDlgItem(int nID) const{ return ::GetDlgItem(m_hWnd, nID); }
};
