// SetDlgApp.cpp : 実装ファイル
//

#include "stdafx.h"
#include "EpgDataCap_Bon.h"
#include "SetDlgApp.h"
#include "../../Common/PathUtil.h"


// CSetDlgApp ダイアログ

CSetDlgApp::CSetDlgApp()
	: m_hWnd(NULL)
	, m_setting(NULL)
{

}

CSetDlgApp::~CSetDlgApp()
{
}

BOOL CSetDlgApp::Create(LPCWSTR lpszTemplateName, HWND hWndParent, const APP_SETTING& setting)
{
	m_setting = &setting;
	return CreateDialogParam(GetModuleHandle(NULL), lpszTemplateName, hWndParent, DlgProc, (LPARAM)this) != NULL;
}


// CSetDlgApp メッセージ ハンドラー


BOOL CSetDlgApp::OnInitDialog()
{
	// TODO:  ここに初期化を追加してください
	Button_SetCheck(GetDlgItem(IDC_CHECK_ALL_SERVICE), m_setting->allService);
	Button_SetCheck(GetDlgItem(IDC_CHECK_ENABLE_DECODE), m_setting->scramble);
	Button_SetCheck(GetDlgItem(IDC_CHECK_EMM), m_setting->emm);
	Button_SetCheck(GetDlgItem(IDC_CHECK_NEED_CAPTION), m_setting->enableCaption);
	Button_SetCheck(GetDlgItem(IDC_CHECK_NEED_DATA), m_setting->enableData);

	SetDlgItemText(m_hWnd, IDC_EDIT_REC_FILENAME, m_setting->recFileName.c_str());
	Button_SetCheck(GetDlgItem(IDC_CHECK_OVER_WRITE), m_setting->overWrite);
	Button_SetCheck(GetDlgItem(IDC_CHECK_OPENLAST), m_setting->openLast);
	SetDlgItemInt(m_hWnd, IDC_EDIT_DROP_SAVE_THRESH, m_setting->dropSaveThresh, TRUE);
	SetDlgItemInt(m_hWnd, IDC_EDIT_SCRAMBLE_SAVE_THRESH, m_setting->scrambleSaveThresh, TRUE);
	Button_SetCheck(GetDlgItem(IDC_CHECK_NO_LOG_SCRAMBLE), m_setting->noLogScramble);
	Button_SetCheck(GetDlgItem(IDC_CHECK_DROP_LOG_AS_UTF8), m_setting->dropLogAsUtf8);
	Button_SetCheck(GetDlgItem(IDC_CHECK_SAVE_DEBUG_LOG), m_setting->saveDebugLog);
	Button_SetCheck(GetDlgItem(IDC_CHECK_TRACE_BON_LEVEL), m_setting->traceBonDriverLevel != 0);
	SetDlgItemInt(m_hWnd, IDC_EDIT_TS_BUFF_MAX, m_setting->tsBuffMaxCount, FALSE);
	SetDlgItemInt(m_hWnd, IDC_EDIT_WRITE_BUFF_MAX, m_setting->writeBuffMaxCount < 0 ? 0 : m_setting->writeBuffMaxCount, FALSE);

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
	int traceLevel = 0;
	if( Button_GetCheck(GetDlgItem(IDC_CHECK_TRACE_BON_LEVEL)) ){
		//チェックボックスだが設定的には多値なので、値を維持する
		traceLevel = GetPrivateProfileInt(L"SET", L"TraceBonDriverLevel", 0, appIniPath.c_str());
		if( traceLevel == 0 ){
			traceLevel = 2;
		}
	}
	WritePrivateProfileInt( L"SET", L"TraceBonDriverLevel", traceLevel, appIniPath.c_str() );
	WritePrivateProfileInt( L"SET", L"TsBuffMaxCount", GetDlgItemInt(m_hWnd, IDC_EDIT_TS_BUFF_MAX, NULL, FALSE), appIniPath.c_str() );
	int buffMax = GetDlgItemInt(m_hWnd, IDC_EDIT_WRITE_BUFF_MAX, NULL, FALSE);
	WritePrivateProfileInt( L"SET", L"WriteBuffMaxCount", buffMax <= 0 ? -1 : buffMax, appIniPath.c_str() );
}


INT_PTR CALLBACK CSetDlgApp::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	(void)wParam;
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
