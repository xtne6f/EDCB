// SetDlgApp.cpp : 実装ファイル
//

#include "stdafx.h"
#include "EpgDataCap_Bon.h"
#include "SetDlgApp.h"


// CSetDlgApp ダイアログ

CSetDlgApp::CSetDlgApp()
	: m_hWnd(NULL)
{

}

CSetDlgApp::~CSetDlgApp()
{
}

BOOL CSetDlgApp::Create(LPCWSTR lpszTemplateName, HWND hWndParent)
{
	return CreateDialogParam(GetModuleHandle(NULL), lpszTemplateName, hWndParent, DlgProc, (LPARAM)this) != NULL;
}


// CSetDlgApp メッセージ ハンドラー


BOOL CSetDlgApp::OnInitDialog()
{
	// TODO:  ここに初期化を追加してください
	fs_path appIniPath = GetModuleIniPath();

	Button_SetCheck(GetDlgItem(IDC_CHECK_ALL_SERVICE), GetPrivateProfileInt(L"SET", L"AllService", 0, appIniPath.c_str()));
	Button_SetCheck(GetDlgItem(IDC_CHECK_ENABLE_DECODE), GetPrivateProfileInt(L"SET", L"Scramble", 1, appIniPath.c_str()));
	Button_SetCheck(GetDlgItem(IDC_CHECK_EMM), GetPrivateProfileInt(L"SET", L"EMM", 0, appIniPath.c_str()));
	Button_SetCheck(GetDlgItem(IDC_CHECK_NEED_CAPTION), GetPrivateProfileInt(L"SET", L"Caption", 1, appIniPath.c_str()));
	Button_SetCheck(GetDlgItem(IDC_CHECK_NEED_DATA), GetPrivateProfileInt(L"SET", L"Data", 0, appIniPath.c_str()));

	SetDlgItemText(m_hWnd, IDC_EDIT_REC_FILENAME,
		GetPrivateProfileToString(L"SET", L"RecFileName", L"$DYYYY$$DMM$$DDD$-$THH$$TMM$$TSS$-$ServiceName$.ts", appIniPath.c_str()).c_str());
	Button_SetCheck(GetDlgItem(IDC_CHECK_OVER_WRITE), GetPrivateProfileInt(L"SET", L"OverWrite", 0, appIniPath.c_str()));
	Button_SetCheck(GetDlgItem(IDC_CHECK_OPENLAST), GetPrivateProfileInt(L"SET", L"OpenLast", 1, appIniPath.c_str()));
	SetDlgItemInt(m_hWnd, IDC_EDIT_DROP_SAVE_THRESH, GetPrivateProfileInt(L"SET", L"DropSaveThresh", 0, appIniPath.c_str()), TRUE);
	SetDlgItemInt(m_hWnd, IDC_EDIT_SCRAMBLE_SAVE_THRESH, GetPrivateProfileInt(L"SET", L"ScrambleSaveThresh", -1, appIniPath.c_str()), TRUE);
	Button_SetCheck(GetDlgItem(IDC_CHECK_NO_LOG_SCRAMBLE), GetPrivateProfileInt(L"SET", L"NoLogScramble", 0, appIniPath.c_str()));
	Button_SetCheck(GetDlgItem(IDC_CHECK_DROP_LOG_AS_UTF8), GetPrivateProfileInt(L"SET", L"DropLogAsUtf8", 0, appIniPath.c_str()));
	Button_SetCheck(GetDlgItem(IDC_CHECK_SAVE_DEBUG_LOG), GetPrivateProfileInt(L"SET", L"SaveDebugLog", 0, appIniPath.c_str()));
	SetDlgItemInt(m_hWnd, IDC_EDIT_TS_BUFF_MAX, GetPrivateProfileInt(L"SET", L"TsBuffMaxCount", 5000, appIniPath.c_str()), FALSE);
	int buffMax = GetPrivateProfileInt(L"SET", L"WriteBuffMaxCount", -1, appIniPath.c_str());
	SetDlgItemInt(m_hWnd, IDC_EDIT_WRITE_BUFF_MAX, buffMax < 0 ? 0 : buffMax, FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 例外 : OCX プロパティ ページは必ず FALSE を返します。
}

void CSetDlgApp::SaveIni(void)
{
	if( m_hWnd == NULL ){
		return;
	}
	fs_path appIniPath = GetModuleIniPath();

	WritePrivateProfileInt( L"SET", L"AllService", Button_GetCheck(GetDlgItem(IDC_CHECK_ALL_SERVICE)), appIniPath.c_str() );
	WritePrivateProfileInt( L"SET", L"Scramble", Button_GetCheck(GetDlgItem(IDC_CHECK_ENABLE_DECODE)), appIniPath.c_str() );
	WritePrivateProfileInt( L"SET", L"EMM", Button_GetCheck(GetDlgItem(IDC_CHECK_EMM)), appIniPath.c_str() );
	WritePrivateProfileInt( L"SET", L"Caption", Button_GetCheck(GetDlgItem(IDC_CHECK_NEED_CAPTION)), appIniPath.c_str() );
	WritePrivateProfileInt( L"SET", L"Data", Button_GetCheck(GetDlgItem(IDC_CHECK_NEED_DATA)), appIniPath.c_str() );

	WCHAR recFileName[512];
	GetDlgItemText(m_hWnd, IDC_EDIT_REC_FILENAME, recFileName, 512);
	WritePrivateProfileString( L"SET", L"RecFileName", recFileName, appIniPath.c_str() );
	WritePrivateProfileInt( L"SET", L"OverWrite", Button_GetCheck(GetDlgItem(IDC_CHECK_OVER_WRITE)), appIniPath.c_str() );
	WritePrivateProfileInt( L"SET", L"OpenLast", Button_GetCheck(GetDlgItem(IDC_CHECK_OPENLAST)), appIniPath.c_str() );
	WritePrivateProfileInt( L"SET", L"DropSaveThresh", (int)GetDlgItemInt(m_hWnd, IDC_EDIT_DROP_SAVE_THRESH, NULL, TRUE), appIniPath.c_str() );
	WritePrivateProfileInt( L"SET", L"ScrambleSaveThresh", (int)GetDlgItemInt(m_hWnd, IDC_EDIT_SCRAMBLE_SAVE_THRESH, NULL, TRUE), appIniPath.c_str() );
	WritePrivateProfileInt( L"SET", L"NoLogScramble", Button_GetCheck(GetDlgItem(IDC_CHECK_NO_LOG_SCRAMBLE)), appIniPath.c_str() );
	WritePrivateProfileInt( L"SET", L"DropLogAsUtf8", Button_GetCheck(GetDlgItem(IDC_CHECK_DROP_LOG_AS_UTF8)), appIniPath.c_str() );
	WritePrivateProfileInt( L"SET", L"SaveDebugLog", Button_GetCheck(GetDlgItem(IDC_CHECK_SAVE_DEBUG_LOG)), appIniPath.c_str() );
	WritePrivateProfileInt( L"SET", L"TsBuffMaxCount", GetDlgItemInt(m_hWnd, IDC_EDIT_TS_BUFF_MAX, NULL, FALSE), appIniPath.c_str() );
	int buffMax = GetDlgItemInt(m_hWnd, IDC_EDIT_WRITE_BUFF_MAX, NULL, FALSE);
	WritePrivateProfileInt( L"SET", L"WriteBuffMaxCount", buffMax <= 0 ? -1 : buffMax, appIniPath.c_str() );
}


INT_PTR CALLBACK CSetDlgApp::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CSetDlgApp* pSys = (CSetDlgApp*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
	if( pSys == NULL && uMsg != WM_INITDIALOG ){
		return FALSE;
	}
	switch( uMsg ){
	case WM_INITDIALOG:
		SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
		pSys = (CSetDlgApp*)lParam;
		pSys->m_hWnd = hDlg;
		return pSys->OnInitDialog();
	case WM_NCDESTROY:
		pSys->m_hWnd = NULL;
		break;
	}
	return FALSE;
}
