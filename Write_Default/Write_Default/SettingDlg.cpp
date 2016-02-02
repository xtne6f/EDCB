#include "StdAfx.h"
#include "SettingDlg.h"
#include "resource.h"

INT_PTR CSettingDlg::CreateSettingDialog(HINSTANCE hInstance, HWND parentWnd, wstring& size_)
{
	this->size = &size_;
	return DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_DIALOG_SET), parentWnd, DlgProc, (LPARAM)this);
}

INT_PTR CALLBACK CSettingDlg::DlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
		case WM_INITDIALOG:
			{
				CSettingDlg* sys = (CSettingDlg*)lp;
				SetWindowLongPtr(hDlgWnd, DWLP_USER, (LONG_PTR)sys);
				SetDlgItemText(hDlgWnd, IDC_EDIT_SIZE, sys->size->c_str());
			}
			return FALSE;
        case WM_COMMAND:
			switch (LOWORD(wp)) {
				case IDOK:
					{
						CSettingDlg* sys = (CSettingDlg*)GetWindowLongPtr(hDlgWnd, DWLP_USER);
						WCHAR buff[1024] = L"";
						GetDlgItemText(hDlgWnd,IDC_EDIT_SIZE, buff, 1024);
						*sys->size = buff;
					}
					EndDialog(hDlgWnd, IDOK);
					break;
				case IDCANCEL:
					EndDialog(hDlgWnd, IDCANCEL);
					break;
			}
	}
	return FALSE;
}

