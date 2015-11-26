#pragma once

#include "../../Common/ParseTextInstances.h"

// CSetDlgEpg ダイアログ

class CSetDlgEpg
{
public:
	CSetDlgEpg();   // 標準コンストラクター
	~CSetDlgEpg();
	BOOL Create(LPCTSTR lpszTemplateName, HWND hWndParent);
	HWND GetSafeHwnd() const{ return m_hWnd; }
	
	void SetIniPath(std::wstring commonIniPath, std::wstring appIniPath){
		this->commonIniPath = commonIniPath;
		this->appIniPath = appIniPath;
	};
	void SaveIni(void);

// ダイアログ データ
	enum { IDD = IDD_DIALOG_SET_EPG };
	
protected:
	HWND m_hWnd;
	std::wstring commonIniPath;
	std::wstring appIniPath;
	CParseChText5 chSet;

	BOOL OnInitDialog();
	afx_msg void OnBnClickedButtonAllChk();
	afx_msg void OnBnClickedButtonVideoChk();
	afx_msg void OnBnClickedButtonAllClear();
	static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	HWND GetDlgItem(int nID) const{ return ::GetDlgItem(m_hWnd, nID); }
};
