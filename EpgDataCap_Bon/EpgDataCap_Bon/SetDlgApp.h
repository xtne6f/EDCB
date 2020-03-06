#pragma once


// CSetDlgApp ダイアログ

class CSetDlgApp
{
public:
	CSetDlgApp();   // 標準コンストラクター
	~CSetDlgApp();
	BOOL Create(LPCWSTR lpszTemplateName, HWND hWndParent);
	HWND GetSafeHwnd() const{ return m_hWnd; }
	void SaveIni(void);

// ダイアログ データ
	enum { IDD = IDD_DIALOG_SET_APP };

protected:
	HWND m_hWnd;

	BOOL OnInitDialog();
	static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	HWND GetDlgItem(int nID) const{ return ::GetDlgItem(m_hWnd, nID); }
};
