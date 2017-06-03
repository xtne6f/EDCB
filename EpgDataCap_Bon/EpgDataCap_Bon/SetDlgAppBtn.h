#pragma once


// CSetDlgAppBtn ダイアログ

class CSetDlgAppBtn
{
public:
	CSetDlgAppBtn();   // 標準コンストラクター
	~CSetDlgAppBtn();
	BOOL Create(LPCTSTR lpszTemplateName, HWND hWndParent);
	HWND GetSafeHwnd() const{ return m_hWnd; }
	
	void SetIniPath(wstring commonIniPath, wstring appIniPath){
		this->commonIniPath = commonIniPath;
		this->appIniPath = appIniPath;
	};
	void SaveIni(void);

// ダイアログ データ
	enum { IDD = IDD_DIALOG_SET_APPBTN };

protected:
	HWND m_hWnd;
	wstring commonIniPath;
	wstring appIniPath;

	BOOL OnInitDialog();
	afx_msg void OnBnClickedButtonViewExe();
	static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	HWND GetDlgItem(int nID) const{ return ::GetDlgItem(m_hWnd, nID); }
};
