#include "stdafx.h"
#include "SettingDlg.h"
#include "resource.h"

INT_PTR CSettingDlg::CreateSettingDialog(HINSTANCE hInstance, HWND parentWnd, wstring& size_, wstring& teeCmd_, wstring& teeSize_, wstring& teeDelay_)
{
	this->size = &size_;
	this->teeCmd = &teeCmd_;
	this->teeSize = &teeSize_;
	this->teeDelay = &teeDelay_;
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
				SendDlgItemMessage(hDlgWnd, IDC_CHECK_TEE, BM_SETCHECK, sys->teeCmd->empty() ? BST_UNCHECKED : BST_CHECKED, 0);
				SendMessage(hDlgWnd, WM_COMMAND, IDC_CHECK_TEE, 0);
				SetDlgItemText(hDlgWnd, IDC_EDIT_TEE, sys->teeCmd->c_str());
				SetDlgItemText(hDlgWnd, IDC_EDIT_TEE_SIZE, sys->teeSize->c_str());
				SetDlgItemText(hDlgWnd, IDC_EDIT_TEE_DELAY, sys->teeDelay->c_str());
			}
			return FALSE;
        case WM_COMMAND:
			switch (LOWORD(wp)) {
				case IDC_CHECK_TEE:
					{
						BOOL b = SendDlgItemMessage(hDlgWnd, IDC_CHECK_TEE, BM_GETCHECK, 0, 0) == BST_CHECKED;
						EnableWindow(GetDlgItem(hDlgWnd, IDC_EDIT_TEE), b);
						EnableWindow(GetDlgItem(hDlgWnd, IDC_STATIC_TEE_SIZE), b);
						EnableWindow(GetDlgItem(hDlgWnd, IDC_EDIT_TEE_SIZE), b);
						EnableWindow(GetDlgItem(hDlgWnd, IDC_STATIC_TEE_DELAY), b);
						EnableWindow(GetDlgItem(hDlgWnd, IDC_EDIT_TEE_DELAY), b);
					}
					break;
				case IDOK:
					{
						CSettingDlg* sys = (CSettingDlg*)GetWindowLongPtr(hDlgWnd, DWLP_USER);
						WCHAR buff[1024] = L"";
						GetDlgItemText(hDlgWnd,IDC_EDIT_SIZE, buff, 1024);
						*sys->size = buff;
						GetDlgItemText(hDlgWnd, IDC_EDIT_TEE, buff, 1024);
						*sys->teeCmd = SendDlgItemMessage(hDlgWnd, IDC_CHECK_TEE, BM_GETCHECK, 0, 0) == BST_CHECKED ? buff : L"";
						GetDlgItemText(hDlgWnd, IDC_EDIT_TEE_SIZE, buff, 1024);
						*sys->teeSize = buff;
						GetDlgItemText(hDlgWnd, IDC_EDIT_TEE_DELAY, buff, 1024);
						*sys->teeDelay = buff;
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

