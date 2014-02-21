// QueryWaitDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "EpgTimerTask.h"
#include "QueryWaitDlg.h"


// CQueryWaitDlg ダイアログ

CQueryWaitDlg::CQueryWaitDlg()
	: m_hDlg(NULL)
{
	m_bReboot = FALSE;
}

CQueryWaitDlg::~CQueryWaitDlg()
{
}

INT_PTR CQueryWaitDlg::DoModal()
{
	return DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD), NULL, DlgProc, (LPARAM)this);
}


// CQueryWaitDlg メッセージ ハンドラー


BOOL CQueryWaitDlg::OnInitDialog()
{
	// TODO:  ここに初期化を追加してください
	SetWindowLong(m_hDlg, GWL_EXSTYLE, GetWindowLong(m_hDlg, GWL_EXSTYLE) | WS_EX_DLGMODALFRAME);

	SetWindowPos(m_hDlg, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOSIZE);
	SetForegroundWindow(m_hDlg);
	if( m_bReboot == FALSE ){
		dwCount = 15;
		SetDlgItemText(m_hDlg, IDC_STATIC_MSG, L"スタンバイ、休止、またはシャットダウンへ移行を行います");
	}else{
		SetDlgItemText(m_hDlg, IDC_STATIC_MSG, L"PCを再起動します");
		SetWindowText(m_hDlg, L"PC再起動");
		dwCount = 30;
	}
	SetTimer(m_hDlg, TIMER_WAIT_SLEEP, 1000, NULL );
	SendDlgItemMessage(m_hDlg, IDC_PROGRESS1, PBM_SETRANGE, 0, MAKELONG(0, dwCount));
	SendDlgItemMessage(m_hDlg, IDC_PROGRESS1, PBM_SETPOS, dwCount, 0);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 例外 : OCX プロパティ ページは必ず FALSE を返します。
}


void CQueryWaitDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: ここにメッセージ ハンドラー コードを追加するか、既定の処理を呼び出します。
	if( nIDEvent == TIMER_WAIT_SLEEP ){
		if( dwCount == 0 ){
			KillTimer(m_hDlg, TIMER_WAIT_SLEEP);
			EndDialog(m_hDlg, IDOK);
		}else{
			SendDlgItemMessage(m_hDlg, IDC_PROGRESS1, PBM_SETPOS, --dwCount, 0);
		}
	}
}


INT_PTR CALLBACK CQueryWaitDlg::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CQueryWaitDlg* pSys = (CQueryWaitDlg*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
	switch( uMsg ){
	case WM_INITDIALOG:
		SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
		pSys = (CQueryWaitDlg*)lParam;
		pSys->m_hDlg = hDlg;
		return pSys->OnInitDialog();
	case WM_NCDESTROY:
		pSys->m_hDlg = NULL;
		break;
	case WM_TIMER:
		pSys->OnTimer(wParam);
		break;
	case WM_COMMAND:
		switch( LOWORD(wParam) ){
		case IDOK:
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		break;
	}
	return FALSE;
}
