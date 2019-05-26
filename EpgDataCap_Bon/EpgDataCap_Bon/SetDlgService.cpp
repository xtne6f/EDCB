// SetDlgService.cpp : 実装ファイル
//

#include "stdafx.h"
#include "EpgDataCap_Bon.h"
#include "SetDlgService.h"
#include "../../BonCtrl/ChSetUtil.h"

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
}

BOOL CSetDlgService::Create(LPCWSTR lpszTemplateName, HWND hWndParent)
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
		map<wstring, pair<CParseChText4, bool>>::const_iterator itr = chList.find(currentChListKey);
		if( itr != chList.end() ){
			ListView_SetCheckState(GetDlgItem(IDC_LIST_SERVICE), i, CChSetUtil::IsVideoServiceType(
				itr->second.first.GetMap().find((DWORD)ListView_GetItemParam(GetDlgItem(IDC_LIST_SERVICE), i, 0))->second.serviceType));
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

	const fs_path path = GetSettingPath();

	//指定フォルダのファイル一覧取得
	EnumFindFile(fs_path(path).append(L"*.ChSet4.txt"), [this, &path](UTIL_FIND_DATA& findData) -> bool {
		if( findData.isDir == false ){
			//本当に拡張子TXT?
			if( UtilPathEndsWith(findData.fileName.c_str(), L".txt") ){
				wstring bonFileName;
				FindBonFileName(findData.fileName, bonFileName);
				bonFileName += L".dll";

				if( chList.insert(std::make_pair(bonFileName, std::make_pair(CParseChText4(), false))).second ){
					chList[bonFileName].first.ParseText(fs_path(path).append(findData.fileName).c_str());
					ComboBox_AddString(this->GetDlgItem(IDC_COMBO_BON), bonFileName.c_str());
				}
			}
		}
		return true;
	});

	if( chList.size() > 0 ){
		ComboBox_SetCurSel(GetDlgItem(IDC_COMBO_BON), 0);
		OnCbnSelchangeComboBon();
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// 例外 : OCX プロパティ ページは必ず FALSE を返します。
}

void CSetDlgService::ReloadList()
{
	HWND hItem = GetDlgItem(IDC_LIST_SERVICE);
	ListView_DeleteAllItems(hItem);

	map<wstring, pair<CParseChText4, bool>>::const_iterator itr = chList.find(currentChListKey);
	if( itr != chList.end() ){
		for( map<DWORD, CH_DATA4>::const_iterator itrCh = itr->second.first.GetMap().begin(); itrCh != itr->second.first.GetMap().end(); itrCh++ ){
			LVITEM lvi;
			lvi.mask = LVIF_TEXT | LVIF_PARAM;
			lvi.iItem = ListView_GetItemCount(hItem);
			lvi.iSubItem = 0;
			lvi.pszText = (LPWSTR)itrCh->second.serviceName.c_str();
			lvi.lParam = (LPARAM)itrCh->first;
			int index = ListView_InsertItem(hItem, &lvi);
			ListView_SetCheckState(hItem, index, itrCh->second.useViewFlag);
		}
	}
	SetDlgItemText(m_hWnd, IDC_EDIT_CH, L"");
}

void CSetDlgService::SynchronizeCheckState()
{
	for( int i=0; i<ListView_GetItemCount(GetDlgItem(IDC_LIST_SERVICE)); i++ ){
		map<wstring, pair<CParseChText4, bool>>::iterator itr = chList.find(currentChListKey);
		if( itr != chList.end() ){
			DWORD key = (DWORD)ListView_GetItemParam(GetDlgItem(IDC_LIST_SERVICE), i, 0);
			BOOL useViewFlag = ListView_GetCheckState(GetDlgItem(IDC_LIST_SERVICE), i);
			if( itr->second.first.GetMap().find(key)->second.useViewFlag != useViewFlag ){
				itr->second.first.SetUseViewFlag(key, useViewFlag);
				itr->second.second = true;
			}
		}
	}
}

BOOL CSetDlgService::FindBonFileName(wstring src, wstring& dllName)
{
	size_t i = src.size();
	for( int depth = 0; i > 0; ){
		if( src[--i] == L')' ){
			depth++;
		}else if( src[i] == L'(' && depth > 0 ){
			if( --depth == 0 ){
				break;
			}
		}
	}
	dllName.swap(src);
	if( i == 0 ){
		return FALSE;
	}
	dllName.erase(i);

	return TRUE;
}

void CSetDlgService::SaveIni()
{
	if( m_hWnd == NULL ){
		return;
	}

	SynchronizeCheckState();

	for( map<wstring, pair<CParseChText4, bool>>::const_iterator itr = chList.begin(); itr != chList.end(); itr++ ){
		//変更したときだけ保存する
		if( itr->second.second ){
			itr->second.first.SaveText();
		}
	}
}

void CSetDlgService::OnCbnSelchangeComboBon()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	SynchronizeCheckState();
	WCHAR key[512];
	currentChListKey = (GetDlgItemText(m_hWnd, IDC_COMBO_BON, key, 512) > 0 ? key : L"");
	ReloadList();
}


void CSetDlgService::OnBnClickedButtonDel()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	int sel = ListView_GetNextItem(GetDlgItem(IDC_LIST_SERVICE), -1, LVNI_SELECTED);
	if( sel < 0 ){
		return ;
	}
	if( MessageBox(m_hWnd, L"削除を行うと、再度チャンネルスキャンを行うまで項目が表示されなくなります。\r\nよろしいですか？",L"", MB_OKCANCEL) == IDOK ){
		map<wstring, pair<CParseChText4, bool>>::iterator itr = chList.find(currentChListKey);
		if( itr != chList.end() ){
			SynchronizeCheckState();
			itr->second.first.DelCh((DWORD)ListView_GetItemParam(GetDlgItem(IDC_LIST_SERVICE), sel, 0));
			itr->second.second = true;
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
	map<wstring, pair<CParseChText4, bool>>::const_iterator itr = chList.find(currentChListKey);
	if( itr == chList.end() ){
		return ;
	}
	const CH_DATA4* chSet = &itr->second.first.GetMap().find((DWORD)ListView_GetItemParam(GetDlgItem(IDC_LIST_SERVICE), sel, 0))->second;
	wstring info;
	Format(info, L"space: %d ch: %d (%ls)\r\nOriginalNetworkID: %d(0x%04X)\r\nTransportStreamID: %d(0x%04X)\r\nServiceID: %d(0x%04X)\r\nServiceType: %d(0x%02X)\r\n",
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
