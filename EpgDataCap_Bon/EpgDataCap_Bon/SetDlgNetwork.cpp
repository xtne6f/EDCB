// SetDlgNetwork.cpp : 実装ファイル
//

#include "stdafx.h"
#include "EpgDataCap_Bon.h"
#include "SetDlgNetwork.h"

#include "../../Common/PathUtil.h"
#include "../../Common/StringUtil.h"

// CSetDlgNetwork ダイアログ

CSetDlgNetwork::CSetDlgNetwork()
	: m_hWnd(NULL)
{

}

CSetDlgNetwork::~CSetDlgNetwork()
{
}

BOOL CSetDlgNetwork::Create(LPCWSTR lpszTemplateName, HWND hWndParent)
{
	return CreateDialogParam(GetModuleHandle(NULL), lpszTemplateName, hWndParent, DlgProc, (LPARAM)this) != NULL;
}


// CSetDlgNetwork メッセージ ハンドラー


BOOL CSetDlgNetwork::OnInitDialog()
{
	// TODO:  ここに初期化を追加してください
	fs_path appIniPath = GetModuleIniPath();
	SetDlgItemInt(m_hWnd, IDC_EDIT_WAIT_SEC, GetPrivateProfileInt(L"SET", L"UDPWait", 4, appIniPath.c_str()), FALSE);
	SetDlgItemInt(m_hWnd, IDC_EDIT_WAIT_PACKET, GetPrivateProfileInt(L"SET", L"UDPPacket", 128, appIniPath.c_str()), FALSE);

	int udpCount = GetPrivateProfileInt( L"SET_UDP", L"Count", 0, appIniPath.c_str() );
	for( int i = 0; i < udpCount; i++ ){
		NW_SEND_INFO item;

		WCHAR key[64];
		swprintf_s(key, L"IP%d", i);
		item.ipString = GetPrivateProfileToString(L"SET_UDP", key, L"2130706433", appIniPath.c_str());
		if( item.ipString.size() >= 2 && item.ipString[0] == L'[' ){
			item.ipString.erase(0, 1).pop_back();
		}else{
			UINT ip = _wtoi(item.ipString.c_str());
			Format(item.ipString, L"%d.%d.%d.%d", ip >> 24, ip >> 16 & 0xFF, ip >> 8 & 0xFF, ip & 0xFF);
		}
		swprintf_s(key, L"Port%d", i);
		item.port = GetPrivateProfileInt( L"SET_UDP", key, 1234, appIniPath.c_str() );
		swprintf_s(key, L"BroadCast%d", i);
		item.broadcastFlag = GetPrivateProfileInt( L"SET_UDP", key, 0, appIniPath.c_str() );

		udpSendList.push_back(item);

		wstring add = L"";
		Format(add, L"%s:%d",item.ipString.c_str(), item.port);
		if( item.broadcastFlag == TRUE ){
			add+= L" ブロードキャスト";
		}
		int index = ListBox_AddString(GetDlgItem(IDC_LIST_IP_UDP), add.c_str());
		ListBox_SetItemData(GetDlgItem(IDC_LIST_IP_UDP), index, i);
	}
	SetDlgItemText(m_hWnd, IDC_IPADDRESS_UDP, udpSendList.empty() ? L"127.0.0.1" : udpSendList.back().ipString.c_str());
	SetDlgItemInt(m_hWnd, IDC_EDIT_PORT_UDP, udpSendList.empty() ? 1234 : udpSendList.back().port, FALSE);

	int tcpCount = GetPrivateProfileInt( L"SET_TCP", L"Count", 0, appIniPath.c_str() );
	for( int i = 0; i < tcpCount; i++ ){
		NW_SEND_INFO item;

		WCHAR key[64];
		swprintf_s(key, L"IP%d", i);
		item.ipString = GetPrivateProfileToString(L"SET_TCP", key, L"2130706433", appIniPath.c_str());
		if( item.ipString.size() >= 2 && item.ipString[0] == L'[' ){
			item.ipString.erase(0, 1).pop_back();
		}else{
			UINT ip = _wtoi(item.ipString.c_str());
			Format(item.ipString, L"%d.%d.%d.%d", ip >> 24, ip >> 16 & 0xFF, ip >> 8 & 0xFF, ip & 0xFF);
		}
		swprintf_s(key, L"Port%d", i);
		item.port = GetPrivateProfileInt( L"SET_TCP", key, 2230, appIniPath.c_str() );
		item.broadcastFlag = 0;

		tcpSendList.push_back(item);

		wstring add = L"";
		Format(add, L"%s:%d",item.ipString.c_str(), item.port);
		int index = ListBox_AddString(GetDlgItem(IDC_LIST_IP_TCP), add.c_str());
		ListBox_SetItemData(GetDlgItem(IDC_LIST_IP_TCP), index, i);
	}
	SetDlgItemText(m_hWnd, IDC_IPADDRESS_TCP, tcpSendList.empty() ? L"127.0.0.1" : tcpSendList.back().ipString.c_str());
	SetDlgItemInt(m_hWnd, IDC_EDIT_PORT_TCP, tcpSendList.empty() ? 2230 : tcpSendList.back().port, FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 例外 : OCX プロパティ ページは必ず FALSE を返します。
}

void CSetDlgNetwork::SaveIni(void)
{
	if( m_hWnd == NULL ){
		return;
	}
	fs_path appIniPath = GetModuleIniPath();

	WritePrivateProfileInt( L"SET", L"UDPWait", GetDlgItemInt(m_hWnd, IDC_EDIT_WAIT_SEC, NULL, FALSE), appIniPath.c_str() );
	WritePrivateProfileInt( L"SET", L"UDPPacket", GetDlgItemInt(m_hWnd, IDC_EDIT_WAIT_PACKET, NULL, FALSE), appIniPath.c_str() );

	WritePrivateProfileInt( L"SET_UDP", L"Count", (int)udpSendList.size(), appIniPath.c_str() );
	for( int i = 0; i < (int)udpSendList.size(); i++ ){
		WCHAR key[64];
		swprintf_s(key, L"IP%d", i);
		UINT u[4];
		if( swscanf_s(udpSendList[i].ipString.c_str(), L"%u.%u.%u.%u", &u[0], &u[1], &u[2], &u[3]) == 4 ){
			WritePrivateProfileInt(L"SET_UDP", key, u[0] << 24 | u[1] << 16 | u[2] << 8 | u[3], appIniPath.c_str());
		}else{
			WritePrivateProfileString(L"SET_UDP", key, (L'[' + udpSendList[i].ipString + L']').c_str(), appIniPath.c_str());
		}
		swprintf_s(key, L"Port%d", i);
		WritePrivateProfileInt( L"SET_UDP", key, udpSendList[i].port, appIniPath.c_str() );
		swprintf_s(key, L"BroadCast%d", i);
		WritePrivateProfileInt( L"SET_UDP", key, udpSendList[i].broadcastFlag, appIniPath.c_str() );
	}

	WritePrivateProfileInt( L"SET_TCP", L"Count", (int)tcpSendList.size(), appIniPath.c_str() );
	for( int i = 0; i < (int)tcpSendList.size(); i++ ){
		WCHAR key[64];
		swprintf_s(key, L"IP%d", i);
		UINT u[4];
		if( swscanf_s(tcpSendList[i].ipString.c_str(), L"%u.%u.%u.%u", &u[0], &u[1], &u[2], &u[3]) == 4 ){
			WritePrivateProfileInt(L"SET_TCP", key, u[0] << 24 | u[1] << 16 | u[2] << 8 | u[3], appIniPath.c_str());
		}else{
			WritePrivateProfileString(L"SET_TCP", key, (L'[' + tcpSendList[i].ipString + L']').c_str(), appIniPath.c_str());
		}
		swprintf_s(key, L"Port%d", i);
		WritePrivateProfileInt( L"SET_TCP", key, tcpSendList[i].port, appIniPath.c_str() );
	}
}


void CSetDlgNetwork::OnBnClickedButtonAddUdp()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	WCHAR szIP[64] = {};
	GetDlgItemText(m_hWnd, IDC_IPADDRESS_UDP, szIP, 63);
	UINT udpPort = GetDlgItemInt(m_hWnd, IDC_EDIT_PORT_UDP, NULL, FALSE);

	NW_SEND_INFO item;
	item.port = udpPort;
	item.ipString = szIP;
	if( item.ipString.find_first_not_of(L".0123456789:abcdef") != wstring::npos ||
	    item.ipString.find_first_not_of(L".:") == wstring::npos ||
	    item.ipString.find_first_of(L".:") == wstring::npos ){
		return;
	}

	wstring add = L"";
	Format(add, L"%s:%d",item.ipString.c_str(), item.port);
	if( Button_GetCheck(GetDlgItem(IDC_CHECK_BROADCAST)) != BST_UNCHECKED ){
		add+= L" ブロードキャスト";
		item.broadcastFlag = TRUE;
	}else{
		item.broadcastFlag = FALSE;
	}

	HWND hItem = GetDlgItem(IDC_LIST_IP_UDP);
	for( int i=0; i<ListBox_GetCount(hItem); i++ ){
		WCHAR buff[256]=L"";
		int len = ListBox_GetTextLen(hItem, i);
		if( 0 <= len && len < 256 ){
			ListBox_GetText(hItem, i, buff);
			if( CompareNoCase(add, buff) == 0 ){
				return ;
			}
		}
	}
	int index = ListBox_AddString(hItem, add.c_str());
	ListBox_SetItemData(hItem, index, (int)udpSendList.size());
	udpSendList.push_back(item);
}


void CSetDlgNetwork::OnBnClickedButtonDelUdp()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	HWND hItem = GetDlgItem(IDC_LIST_IP_UDP);
	int sel = ListBox_GetCurSel(hItem);
	if( sel != LB_ERR ){
		int index = (int)ListBox_GetItemData(hItem, sel);

		vector<NW_SEND_INFO>::iterator itr;
		itr = udpSendList.begin();
		advance(itr, index);
		udpSendList.erase(itr);

		ListBox_ResetContent(hItem);

		for( int i=0; i<(int)udpSendList.size(); i++ ){
			wstring add = L"";
			Format(add, L"%s:%d",udpSendList[i].ipString.c_str(), udpSendList[i].port);
			if( udpSendList[i].broadcastFlag == TRUE ){
				add+= L" ブロードキャスト";
			}
			index = ListBox_AddString(hItem, add.c_str());
			ListBox_SetItemData(hItem, index, i);
		}
	}
}


void CSetDlgNetwork::OnBnClickedButtonAddTcp()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	WCHAR szIP[64] = {};
	GetDlgItemText(m_hWnd, IDC_IPADDRESS_TCP, szIP, 63);
	UINT tcpPort = GetDlgItemInt(m_hWnd, IDC_EDIT_PORT_TCP, NULL, FALSE);

	NW_SEND_INFO item;
	item.port = tcpPort;
	item.ipString = szIP;
	if( item.ipString.find_first_not_of(L".0123456789:abcdef") != wstring::npos ||
	    item.ipString.find_first_not_of(L".:") == wstring::npos ||
	    item.ipString.find_first_of(L".:") == wstring::npos ){
		return;
	}

	wstring add = L"";
	Format(add, L"%s:%d",item.ipString.c_str(), item.port);
	item.broadcastFlag = FALSE;

	HWND hItem = GetDlgItem(IDC_LIST_IP_TCP);
	for( int i=0; i<ListBox_GetCount(hItem); i++ ){
		WCHAR buff[256]=L"";
		int len = ListBox_GetTextLen(hItem, i);
		if( 0 <= len && len < 256 ){
			ListBox_GetText(hItem, i, buff);
			if( CompareNoCase(add, buff) == 0 ){
				return ;
			}
		}
	}
	int index = ListBox_AddString(hItem, add.c_str());
	ListBox_SetItemData(hItem, index, (int)tcpSendList.size());
	tcpSendList.push_back(item);
}


void CSetDlgNetwork::OnBnClickedButtonDelTcp()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	HWND hItem = GetDlgItem(IDC_LIST_IP_TCP);
	int sel = ListBox_GetCurSel(hItem);
	if( sel != LB_ERR ){
		int index = (int)ListBox_GetItemData(hItem, sel);

		vector<NW_SEND_INFO>::iterator itr;
		itr = tcpSendList.begin();
		advance(itr, index);
		tcpSendList.erase(itr);

		ListBox_ResetContent(hItem);

		for( int i=0; i<(int)tcpSendList.size(); i++ ){
			wstring add = L"";
			Format(add, L"%s:%d",tcpSendList[i].ipString.c_str(), tcpSendList[i].port);
			index = ListBox_AddString(hItem, add.c_str());
			ListBox_SetItemData(hItem, index, i);
		}
	}
}


INT_PTR CALLBACK CSetDlgNetwork::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CSetDlgNetwork* pSys = (CSetDlgNetwork*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
	if( pSys == NULL && uMsg != WM_INITDIALOG ){
		return FALSE;
	}
	switch( uMsg ){
	case WM_INITDIALOG:
		SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
		pSys = (CSetDlgNetwork*)lParam;
		pSys->m_hWnd = hDlg;
		return pSys->OnInitDialog();
	case WM_NCDESTROY:
		pSys->m_hWnd = NULL;
		break;
	case WM_COMMAND:
		switch( LOWORD(wParam) ){
		case IDC_BUTTON_ADD_UDP:
			pSys->OnBnClickedButtonAddUdp();
			break;
		case IDC_BUTTON_DEL_UDP:
			pSys->OnBnClickedButtonDelUdp();
			break;
		case IDC_BUTTON_ADD_TCP:
			pSys->OnBnClickedButtonAddTcp();
			break;
		case IDC_BUTTON_DEL_TCP:
			pSys->OnBnClickedButtonDelTcp();
			break;
		}
		break;
	}
	return FALSE;
}
