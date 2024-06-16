// SetDlgAppBtn.cpp : 実装ファイル
//

#include "stdafx.h"
#include "EpgDataCap_Bon.h"
#include "SetDlgAppBtn.h"
#include "../../Common/PathUtil.h"
#include <commdlg.h>


// CSetDlgAppBtn ダイアログ

CSetDlgAppBtn::CSetDlgAppBtn()
	: m_hWnd(NULL)
	, m_setting(NULL)
{

}

CSetDlgAppBtn::~CSetDlgAppBtn()
{
}

BOOL CSetDlgAppBtn::Create(LPCWSTR lpszTemplateName, HWND hWndParent, const APP_SETTING& setting)
{
	m_setting = &setting;
	return CreateDialogParam(GetModuleHandle(NULL), lpszTemplateName, hWndParent, DlgProc, (LPARAM)this) != NULL;
}


// CSetDlgAppBtn メッセージ ハンドラー


BOOL CSetDlgAppBtn::OnInitDialog()
{
	// TODO:  ここに初期化を追加してください
	SetDlgItemText(m_hWnd, IDC_EDIT_VIEW_EXE, m_setting->viewPath.c_str());
	SetDlgItemText(m_hWnd, IDC_EDIT_VIEW_OPT, m_setting->viewOption.c_str());
	Button_SetCheck(GetDlgItem(IDC_CHECK_VIEW_SINGLE), m_setting->viewSingle);
	Button_SetCheck(GetDlgItem(IDC_CHECK_VIEW_CLOSE_ON_EXIT), m_setting->viewCloseOnExit);
	OnBnClickedCheckViewSingle();

	return TRUE;  // return TRUE unless you set the focus to a control
	// 例外 : OCX プロパティ ページは必ず FALSE を返します。
}

void CSetDlgAppBtn::SaveIni(void)
{
	if( m_hWnd == NULL ){
		return;
	}
	fs_path appIniPath = GetModuleIniPath();

	WCHAR buff[512]=L"";
	GetDlgItemText(m_hWnd, IDC_EDIT_VIEW_EXE, buff, 512);
	WritePrivateProfileString( L"SET", L"ViewPath", buff, appIniPath.c_str() );
	GetDlgItemText(m_hWnd, IDC_EDIT_VIEW_OPT, buff, 512);
	WritePrivateProfileString( L"SET", L"ViewOption", buff, appIniPath.c_str() );
	WritePrivateProfileInt( L"SET", L"ViewSingle", Button_GetCheck(GetDlgItem(IDC_CHECK_VIEW_SINGLE)), appIniPath.c_str() );
	WritePrivateProfileInt( L"SET", L"ViewCloseOnExit", Button_GetCheck(GetDlgItem(IDC_CHECK_VIEW_CLOSE_ON_EXIT)), appIniPath.c_str() );
}

void CSetDlgAppBtn::OnBnClickedButtonViewExe()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	WCHAR strFile[MAX_PATH]=L"";
	OPENFILENAME ofn = {};
	ofn.lStructSize = sizeof (OPENFILENAME);
	ofn.hwndOwner = m_hWnd;
	ofn.lpstrFilter = L"exe files (*.exe)\0*.exe\0All files (*.*)\0*.*\0";
	ofn.lpstrFile = strFile;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_FILEMUSTEXIST;
	if (GetOpenFileName(&ofn) == 0) {
		return ;
	}
	SetDlgItemText(m_hWnd, IDC_EDIT_VIEW_EXE, strFile);
}

void CSetDlgAppBtn::OnBnClickedCheckViewSingle()
{
	EnableWindow(GetDlgItem(IDC_CHECK_VIEW_CLOSE_ON_EXIT),
	             Button_GetCheck(GetDlgItem(IDC_CHECK_VIEW_SINGLE)) == BST_CHECKED);
}

INT_PTR CALLBACK CSetDlgAppBtn::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CSetDlgAppBtn* pSys = (CSetDlgAppBtn*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
	if( pSys == NULL && uMsg != WM_INITDIALOG ){
		return FALSE;
	}
	switch( uMsg ){
	case WM_INITDIALOG:
		SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
		pSys = (CSetDlgAppBtn*)lParam;
		pSys->m_hWnd = hDlg;
		return pSys->OnInitDialog();
	case WM_NCDESTROY:
		pSys->m_hWnd = NULL;
		break;
	case WM_COMMAND:
		switch( LOWORD(wParam) ){
		case IDC_BUTTON_VIEW_EXE:
			pSys->OnBnClickedButtonViewExe();
			break;
		case IDC_CHECK_VIEW_SINGLE:
			pSys->OnBnClickedCheckViewSingle();
			break;
		}
		break;
	}
	return FALSE;
}
