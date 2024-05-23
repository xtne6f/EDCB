#pragma once

#include "AppSetting.h"

// CSetDlgApp ダイアログ

class CSetDlgApp
{
public:
	CSetDlgApp();   // 標準コンストラクター
	~CSetDlgApp();
	BOOL Create(LPCWSTR lpszTemplateName, HWND hWndParent, const APP_SETTING& setting);
	HWND GetSafeHwnd() const{ return m_hWnd; }
	void SaveIni(void);

// ダイアログ データ
	enum { IDD = IDD_DIALOG_SET_APP };

protected:
	HWND m_hWnd;
	const APP_SETTING* m_setting;

	BOOL OnInitDialog();
	static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	HWND GetDlgItem(int nID) const{ return ::GetDlgItem(m_hWnd, nID); }
};
