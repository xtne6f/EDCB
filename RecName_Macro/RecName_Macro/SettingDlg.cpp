#include "stdafx.h"
#include "SettingDlg.h"
#include "resource.h"

INT_PTR CSettingDlg::CreateSettingDialog(HINSTANCE hInstance, HWND parentWnd, wstring& macro_)
{
	this->macro = &macro_;
	return DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_DIALOG_MACRO), parentWnd, DlgProc, (LPARAM)this);
}

INT_PTR CALLBACK CSettingDlg::DlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
		case WM_INITDIALOG:
			{
				CSettingDlg* sys = (CSettingDlg*)lp;
				SetWindowLongPtr(hDlgWnd, DWLP_USER, (LONG_PTR)sys);
				SetDlgItemText(hDlgWnd, IDC_EDIT_MACRO, sys->macro->c_str());
			}
			return FALSE;
        case WM_COMMAND:
			switch (LOWORD(wp)) {
				case IDOK:
					{
						CSettingDlg* sys = (CSettingDlg*)GetWindowLongPtr(hDlgWnd, DWLP_USER);
						vector<WCHAR> buff(GetWindowTextLength(GetDlgItem(hDlgWnd, IDC_EDIT_MACRO)) + 1, L'\0');
						GetWindowText(GetDlgItem(hDlgWnd, IDC_EDIT_MACRO), buff.data(), (int)buff.size());
						*sys->macro = buff.data();
					}
					EndDialog(hDlgWnd, IDOK);
					break;
				case IDCANCEL:
					EndDialog(hDlgWnd, IDCANCEL);
					break;
			}
			return FALSE;
	}
	return FALSE;
}

