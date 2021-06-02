// SettingDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "EpgDataCap_Bon.h"
#include "SettingDlg.h"


// CSettingDlg ダイアログ

CSettingDlg::CSettingDlg(HWND hWndOwner)
	: m_hWnd(NULL)
	, m_hWndOwner(hWndOwner)
{

}

CSettingDlg::~CSettingDlg()
{
}

INT_PTR CSettingDlg::DoModal()
{
	return DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD), m_hWndOwner, DlgProc, (LPARAM)this);
}


// CSettingDlg メッセージ ハンドラー


BOOL CSettingDlg::OnInitDialog()
{
	// TODO:  ここに初期化を追加してください
	basicDlg.Create( MAKEINTRESOURCE(IDD_DIALOG_SET_BASIC), GetSafeHwnd() );
	appDlg.Create( MAKEINTRESOURCE(IDD_DIALOG_SET_APP), GetSafeHwnd() );
	epgDlg.Create( MAKEINTRESOURCE(IDD_DIALOG_SET_EPG), GetSafeHwnd() );
	networkDlg.Create( MAKEINTRESOURCE(IDD_DIALOG_SET_NW), GetSafeHwnd() );
	appBtnDlg.Create( MAKEINTRESOURCE(IDD_DIALOG_SET_APPBTN), GetSafeHwnd() );
	serviceDlg.Create( MAKEINTRESOURCE(IDD_DIALOG_SET_SERVICE), GetSafeHwnd() );

	HWND hwndItems[6] = {
		basicDlg.GetSafeHwnd(),
		appDlg.GetSafeHwnd(),
		epgDlg.GetSafeHwnd(),
		serviceDlg.GetSafeHwnd(),
		networkDlg.GetSafeHwnd(),
		appBtnDlg.GetSafeHwnd()
	};

	for( int i = 0; i < 6; i++ ){
		WCHAR text[32] = {};
		GetWindowText(hwndItems[i], text, 32);
		TCITEM tci;
		tci.mask = TCIF_TEXT;
		tci.pszText = text;
		TabCtrl_InsertItem(GetDlgItem(IDC_TAB), i, &tci);
	}
	RECT rc;
	GetWindowRect(GetDlgItem(IDC_TAB), &rc);
	TabCtrl_AdjustRect(GetDlgItem(IDC_TAB), FALSE, &rc);
	POINT pt;
	pt.x = rc.left;
	pt.y = rc.top;
	ScreenToClient(GetSafeHwnd(), &pt);

	rc.right -= rc.left;
	rc.bottom -= rc.top;
	for( int i = 0; i < 6; i++ ){
		MoveWindow(hwndItems[i], pt.x, pt.y, rc.right, rc.bottom, TRUE);
	}

	TabCtrl_SetCurSel(GetDlgItem(IDC_TAB), 0);
	ShowWindow(basicDlg.GetSafeHwnd(), SW_SHOW);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 例外 : OCX プロパティ ページは必ず FALSE を返します。
}


void CSettingDlg::OnBnClickedOk()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	basicDlg.SaveIni();
	appDlg.SaveIni();
	epgDlg.SaveIni();
	networkDlg.SaveIni();
	appBtnDlg.SaveIni();
	serviceDlg.SaveIni();
}


void CSettingDlg::OnTcnSelchangingTab(NMHDR *pNMHDR, LRESULT *pResult)
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	int index = TabCtrl_GetCurSel(pNMHDR->hwndFrom);
	switch(index){
		case 0:
			ShowWindow(basicDlg.GetSafeHwnd(), SW_HIDE);
			break;
		case 1:
			ShowWindow(appDlg.GetSafeHwnd(), SW_HIDE);
			break;
		case 2:
			ShowWindow(epgDlg.GetSafeHwnd(), SW_HIDE);
			break;
		case 3:
			ShowWindow(serviceDlg.GetSafeHwnd(), SW_HIDE);
			break;
		case 4:
			ShowWindow(networkDlg.GetSafeHwnd(), SW_HIDE);
			break;
		case 5:
			ShowWindow(appBtnDlg.GetSafeHwnd(), SW_HIDE);
			break;
		default:
			break;
	}
	*pResult = 0;
}


void CSettingDlg::OnTcnSelchangeTab(NMHDR *pNMHDR, LRESULT *pResult)
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	int index = TabCtrl_GetCurSel(pNMHDR->hwndFrom);
	switch(index){
		case 0:
			ShowWindow(basicDlg.GetSafeHwnd(), SW_SHOW);
			break;
		case 1:
			ShowWindow(appDlg.GetSafeHwnd(), SW_SHOW);
			break;
		case 2:
			ShowWindow(epgDlg.GetSafeHwnd(), SW_SHOW);
			break;
		case 3:
			ShowWindow(serviceDlg.GetSafeHwnd(), SW_SHOW);
			break;
		case 4:
			ShowWindow(networkDlg.GetSafeHwnd(), SW_SHOW);
			break;
		case 5:
			ShowWindow(appBtnDlg.GetSafeHwnd(), SW_SHOW);
			break;
		default:
			break;
	}

	*pResult = 0;
}


INT_PTR CALLBACK CSettingDlg::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CSettingDlg* pSys = (CSettingDlg*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
	if( pSys == NULL && uMsg != WM_INITDIALOG ){
		return FALSE;
	}
	switch( uMsg ){
	case WM_INITDIALOG:
		SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
		pSys = (CSettingDlg*)lParam;
		pSys->m_hWnd = hDlg;
		return pSys->OnInitDialog();
	case WM_NCDESTROY:
		pSys->m_hWnd = NULL;
		break;
	case WM_NOTIFY:
		{
			LPNMHDR pNMHDR = (LPNMHDR)lParam;
			if( pNMHDR->idFrom	== IDC_TAB ){
				LRESULT result = 0;
				if( pNMHDR->code == TCN_SELCHANGING ){
					pSys->OnTcnSelchangingTab(pNMHDR, &result);
					SetWindowLongPtr(hDlg, DWLP_MSGRESULT, result);
					return TRUE;
				}else if( pNMHDR->code == TCN_SELCHANGE ){
					pSys->OnTcnSelchangeTab(pNMHDR, &result);
					SetWindowLongPtr(hDlg, DWLP_MSGRESULT, result);
					return TRUE;
				}
			}
		}
		break;
	case WM_COMMAND:
		switch( LOWORD(wParam) ){
		case IDOK:
			pSys->OnBnClickedOk();
			//FALL THROUGH!
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			SetWindowLongPtr(hDlg, DWLP_MSGRESULT, 0);
			return TRUE;
		}
		break;
	}
	return FALSE;
}
