// SetDlgService.cpp : 実装ファイル
//

#include "stdafx.h"
#include "EpgDataCap_Bon.h"
#include "SetDlgService.h"

static LPARAM ListView_GetItemParam(HWND hItem, int iItem, int iSubItem)
{
	LVITEM lvi;
	lvi.mask = LVIF_PARAM;
	lvi.iItem = iItem;
	lvi.iSubItem = iSubItem;
	if( ListView_GetItem(hItem, &lvi) != FALSE ){
		return lvi.lParam;
	}
	return 0;
}

// CSetDlgService ダイアログ

CSetDlgService::CSetDlgService()
	: m_hWnd(NULL)
{

}

CSetDlgService::~CSetDlgService()
{
	map<wstring, CH_SET_INFO*>::iterator itr;
	for( itr = chList.begin(); itr != chList.end(); itr++ ){
		SAFE_DELETE(itr->second);
	}
}

BOOL CSetDlgService::Create(LPCTSTR lpszTemplateName, HWND hWndParent)
{
	return CreateDialogParam(GetModuleHandle(NULL), lpszTemplateName, hWndParent, DlgProc, (LPARAM)this) != NULL;
}


// CSetDlgService メッセージ ハンドラー


void CSetDlgService::OnBnClickedButtonChkAll()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	for( int i=0; i<ListView_GetItemCount(GetDlgItem(IDC_LIST_SERVICE)); i++ ){
		ListView_SetCheckState(GetDlgItem(IDC_LIST_SERVICE), i, TRUE);
	}
}


void CSetDlgService::OnBnClickedButtonChkVideo()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	for( int i=0; i<ListView_GetItemCount(GetDlgItem(IDC_LIST_SERVICE)); i++ ){
		CH_DATA4* chSet = (CH_DATA4*)ListView_GetItemParam(GetDlgItem(IDC_LIST_SERVICE), i, 0);
		if( chSet != NULL ){
			ListView_SetCheckState(GetDlgItem(IDC_LIST_SERVICE), i, chSet->serviceType == 0x01 || chSet->serviceType == 0xA5);
		}
	}
}


void CSetDlgService::OnBnClickedButtonChkClear()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	for( int i=0; i<ListView_GetItemCount(GetDlgItem(IDC_LIST_SERVICE)); i++ ){
		ListView_SetCheckState(GetDlgItem(IDC_LIST_SERVICE), i, FALSE);
	}
}


BOOL CSetDlgService::OnInitDialog()
{
	// TODO:  ここに初期化を追加してください

	//リストビューにチェックボックスと列をつくる
	HWND hItem = GetDlgItem(IDC_LIST_SERVICE);
	ListView_SetExtendedListViewStyleEx(hItem, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
	RECT rc;
	GetClientRect(hItem, &rc);
	LVCOLUMN lvc;
	lvc.mask = LVCF_WIDTH;
	lvc.cx = rc.right - GetSystemMetrics(SM_CXVSCROLL) - 4;
	ListView_InsertColumn(hItem, 0, &lvc);

	wstring path = L"";
	GetSettingPath(path);

	wstring srvIniPath = L"";
	GetEpgTimerSrvIniPath(srvIniPath);

	wstring searchKey = path;
	searchKey += L"\\*.ChSet4.txt";

	WIN32_FIND_DATA findData;
	HANDLE find;

	//指定フォルダのファイル一覧取得
	find = FindFirstFile( searchKey.c_str(), &findData);
	if ( find == INVALID_HANDLE_VALUE ) {
		return FALSE;
	}
	do{
		if( (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 ){
			//本当に拡張子DLL?
			if( IsExt(findData.cFileName, L".txt") == TRUE ){
				wstring chSetPath = L"";
				Format(chSetPath, L"%s\\%s", path.c_str(), findData.cFileName);

				wstring bonFileName = L"";
				wstring buff = findData.cFileName;

				FindBonFileName(buff, bonFileName);

				bonFileName += L".dll";

				CH_SET_INFO* item = new CH_SET_INFO;
				item->bonFile = bonFileName;
				item->chSetPath = chSetPath;
				item->chSet.ParseText(chSetPath.c_str());

				chList.insert(pair<wstring, CH_SET_INFO*>(item->bonFile, item));

				ComboBox_AddString(GetDlgItem(IDC_COMBO_BON), item->bonFile.c_str());
			}
		}
	}while(FindNextFile(find, &findData));

	FindClose(find);
	if( chList.size() > 0 ){
		ComboBox_SetCurSel(GetDlgItem(IDC_COMBO_BON), 0);
		ReloadList();

		WCHAR text[512] = L"";
		GetDlgItemText(m_hWnd, IDC_COMBO_BON, text, 512);
		lastSelect = text;
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// 例外 : OCX プロパティ ページは必ず FALSE を返します。
}

void CSetDlgService::ReloadList()
{
	HWND hItem = GetDlgItem(IDC_LIST_SERVICE);
	ListView_DeleteAllItems(hItem);

	WCHAR text[512] = L"";
	GetDlgItemText(m_hWnd, IDC_COMBO_BON, text, 512);

	wstring key = text;
	map<wstring, CH_SET_INFO*>::iterator itr;
	itr = chList.find(key);
	if( itr != chList.end()){
		vector<CH_DATA4*> chDataList = itr->second->chSet.GetChDataList();
		for( vector<CH_DATA4*>::iterator itrCh = chDataList.begin(); itrCh != chDataList.end(); itrCh++ ){
			LVITEM lvi;
			lvi.mask = LVIF_TEXT | LVIF_PARAM;
			lvi.iItem = ListView_GetItemCount(hItem);
			lvi.iSubItem = 0;
			lvi.pszText = (LPWSTR)(*itrCh)->serviceName.c_str();
			lvi.lParam = (LPARAM)(*itrCh);
			int index = ListView_InsertItem(hItem, &lvi);
			ListView_SetCheckState(hItem, index, (*itrCh)->useViewFlag);
		}
	}
	SetDlgItemText(m_hWnd, IDC_EDIT_CH, L"");
}

void CSetDlgService::SynchronizeCheckState()
{
	for( int i=0; i<ListView_GetItemCount(GetDlgItem(IDC_LIST_SERVICE)); i++ ){
		CH_DATA4* chSet = (CH_DATA4*)ListView_GetItemParam(GetDlgItem(IDC_LIST_SERVICE), i, 0);
		if( chSet != NULL ){
			chSet->useViewFlag = ListView_GetCheckState(GetDlgItem(IDC_LIST_SERVICE), i);
		}
	}
}

BOOL CSetDlgService::FindBonFileName(wstring src, wstring& dllName)
{
	wstring buff = src;
	size_t pos = buff.rfind(L")");
	if( pos == string::npos ){
		dllName = src;
		return FALSE;
	}

	int count = 1;
	for( size_t i=pos-1; i>=0; i-- ){
		if(buff.compare(i,1,L")") == 0 ){
			count++;
		}else if(buff.compare(i,1,L"(") == 0){
			count--;
		}
		if( count == 0 ){
			dllName = buff.substr(0, i);
			break;
		}
	}

	return TRUE;
}

void CSetDlgService::SaveIni()
{
	if( m_hWnd == NULL ){
		return;
	}

	SynchronizeCheckState();

	map<wstring, CH_SET_INFO*>::iterator itr;
	for( itr = chList.begin(); itr != chList.end(); itr++ ){
		itr->second->chSet.SaveText();
	}
}

void CSetDlgService::OnCbnSelchangeComboBon()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	int sel = ComboBox_GetCurSel(GetDlgItem(IDC_COMBO_BON));
	if( sel == CB_ERR ){
		return;
	}
	SynchronizeCheckState();
	ReloadList();
}


void CSetDlgService::OnBnClickedButtonDel()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	int sel = ListView_GetNextItem(GetDlgItem(IDC_LIST_SERVICE), -1, LVNI_SELECTED);
	if( sel < 0 ){
		return ;
	}
	CH_DATA4* chSet = (CH_DATA4*)ListView_GetItemParam(GetDlgItem(IDC_LIST_SERVICE), sel, 0);
	if( chSet == NULL ){
		return ;
	}
	if( MessageBox(m_hWnd, L"削除を行うと、再度チャンネルスキャンを行うまで項目が表示されなくなります。\r\nよろしいですか？",L"", MB_OKCANCEL) == IDOK ){
		WCHAR text[512] = L"";
		GetDlgItemText(m_hWnd, IDC_COMBO_BON, text, 512);

		wstring key = text;
		map<wstring, CH_SET_INFO*>::iterator itr;
		itr = chList.find(key);
		if( itr != chList.end()){
			SynchronizeCheckState();
			itr->second->chSet.DelChService(chSet->space, chSet->ch, chSet->serviceID);
			ReloadList();
		}
	}
}


void CSetDlgService::OnLbnSelchangeListService()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	int sel = ListView_GetNextItem(GetDlgItem(IDC_LIST_SERVICE), -1, LVNI_SELECTED);
	if( sel < 0 ){
		return ;
	}
	CH_DATA4* chSet = (CH_DATA4*)ListView_GetItemParam(GetDlgItem(IDC_LIST_SERVICE), sel, 0);
	if( chSet == NULL ){
		return ;
	}
	wstring info = L"";
	Format(info, L"space: %d ch: %d (%s)\r\nOriginalNetworkID: %d(0x%04X)\r\nTransportStreamID: %d(0x%04X)\r\nServiceID: %d(0x%04X)\r\nServiceType: %d(0x%02X)\r\n",
		chSet->space,
		chSet->ch,
		chSet->chName.c_str(),
		chSet->originalNetworkID,
		chSet->originalNetworkID,
		chSet->transportStreamID,
		chSet->transportStreamID,
		chSet->serviceID,
		chSet->serviceID,
		chSet->serviceType,
		chSet->serviceType
		);
	SetDlgItemText(m_hWnd, IDC_EDIT_CH, info.c_str());

}


INT_PTR CALLBACK CSetDlgService::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CSetDlgService* pSys = (CSetDlgService*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
	if( pSys == NULL && uMsg != WM_INITDIALOG ){
		return FALSE;
	}
	switch( uMsg ){
	case WM_INITDIALOG:
		SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
		pSys = (CSetDlgService*)lParam;
		pSys->m_hWnd = hDlg;
		return pSys->OnInitDialog();
	case WM_NCDESTROY:
		pSys->m_hWnd = NULL;
		break;
	case WM_NOTIFY:
		{
			LPNMHDR pNMHDR = (LPNMHDR)lParam;
			if( pNMHDR->idFrom	== IDC_LIST_SERVICE ){
				if( pNMHDR->code == LVN_ITEMCHANGED ){
					pSys->OnLbnSelchangeListService();
				}
			}
		}
		break;
	case WM_COMMAND:
		switch( LOWORD(wParam) ){
		case IDC_BUTTON_CHK_ALL:
			pSys->OnBnClickedButtonChkAll();
			break;
		case IDC_BUTTON_CHK_VIDEO:
			pSys->OnBnClickedButtonChkVideo();
			break;
		case IDC_BUTTON_CHK_CLEAR:
			pSys->OnBnClickedButtonChkClear();
			break;
		case IDC_COMBO_BON:
			if( HIWORD(wParam) == CBN_SELCHANGE ){
				pSys->OnCbnSelchangeComboBon();
			}
			break;
		case IDC_BUTTON_DEL:
			pSys->OnBnClickedButtonDel();
			break;
		}
		break;
	}
	return FALSE;
}
