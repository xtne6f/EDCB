#pragma once


// CSetDlgApp ダイアログ

class CSetDlgApp
{
public:
	CSetDlgApp();   // 標準コンストラクター
	~CSetDlgApp();
	BOOL Create(LPCTSTR lpszTemplateName, HWND hWndParent);
	HWND GetSafeHwnd() const{ return m_hWnd; }
	
	void SetIniPath(wstring commonIniPath, wstring appIniPath){
		this->commonIniPath = commonIniPath;
		this->appIniPath = appIniPath;
	};
	void SaveIni(void);

// ダイアログ データ
	enum { IDD = IDD_DIALOG_SET_APP };

protected:
	HWND m_hWnd;
	wstring commonIniPath;
	wstring appIniPath;

	BOOL OnInitDialog();
	static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	HWND GetDlgItem(int nID) const{ return ::GetDlgItem(m_hWnd, nID); }
};
