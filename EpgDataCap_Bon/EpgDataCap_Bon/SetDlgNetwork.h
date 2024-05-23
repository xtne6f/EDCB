#pragma once

#include "AppSetting.h"

// CSetDlgNetwork ダイアログ

class CSetDlgNetwork
{
public:
	CSetDlgNetwork();   // 標準コンストラクター
	~CSetDlgNetwork();
	BOOL Create(LPCWSTR lpszTemplateName, HWND hWndParent, const APP_SETTING& setting);
	HWND GetSafeHwnd() const{ return m_hWnd; }
	void SaveIni(void);

// ダイアログ データ
	enum { IDD = IDD_DIALOG_SET_NW };

protected:
	HWND m_hWnd;
	vector<NW_SEND_INFO> udpSendList;
	vector<NW_SEND_INFO> tcpSendList;

	BOOL OnInitDialog();
	void OnBnClickedButtonAddUdp();
	void OnBnClickedButtonDelUdp();
	void OnBnClickedButtonAddTcp();
	void OnBnClickedButtonDelTcp();
	void OnBnClickedRadioTcp();
	static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	HWND GetDlgItem(int nID) const{ return ::GetDlgItem(m_hWnd, nID); }
};
