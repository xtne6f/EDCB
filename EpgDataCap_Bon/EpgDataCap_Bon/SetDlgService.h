#pragma once

#include "../../Common/PathUtil.h"
#include "../../Common/StringUtil.h"
#include "../../Common/ParseTextInstances.h"


// CSetDlgService ダイアログ

class CSetDlgService
{
public:
	CSetDlgService();   // 標準コンストラクター
	~CSetDlgService();
	BOOL Create(LPCTSTR lpszTemplateName, HWND hWndParent);
	HWND GetSafeHwnd() const{ return m_hWnd; }

	void SetIniPath(wstring commonIniPath, wstring appIniPath){
		this->commonIniPath = commonIniPath;
		this->appIniPath = appIniPath;
	};
	void SaveIni(void);

// ダイアログ データ
	enum { IDD = IDD_DIALOG_SET_SERVICE };


protected:
	HWND m_hWnd;
	wstring commonIniPath;
	wstring appIniPath;

	typedef struct _CH_SET_INFO{
		wstring bonFile;
		wstring chSetPath;
		CParseChText4 chSet;
	}CH_SET_INFO;
	map<wstring, CH_SET_INFO*> chList;
	wstring lastSelect;

	BOOL FindBonFileName(wstring src, wstring& dllName);
	void ReloadList();
	void SynchronizeCheckState();

	afx_msg void OnBnClickedButtonChkAll();
	afx_msg void OnBnClickedButtonChkVideo();
	afx_msg void OnBnClickedButtonChkClear();
	BOOL OnInitDialog();
	afx_msg void OnCbnSelchangeComboBon();
	afx_msg void OnBnClickedButtonDel();
	afx_msg void OnLbnSelchangeListService();
	static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	HWND GetDlgItem(int nID) const{ return ::GetDlgItem(m_hWnd, nID); }
};
