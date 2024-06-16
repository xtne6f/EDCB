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

BOOL CSetDlgNetwork::Create(LPCWSTR lpszTemplateName, HWND hWndParent, const APP_SETTING& setting)
{
	udpSendList = setting.udpSendList;
	tcpSendList = setting.tcpSendList;
	return CreateDialogParam(GetModuleHandle(NULL), lpszTemplateName, hWndParent, DlgProc, (LPARAM)this) != NULL;
}


// CSetDlgNetwork メッセージ ハンドラー


BOOL CSetDlgNetwork::OnInitDialog()
{
	// TODO:  ここに初期化を追加してください
	fs_path appIniPath = GetModuleIniPath();
	SetDlgItemInt(m_hWnd, IDC_EDIT_WAIT_SEC, GetPrivateProfileInt(L"SET", L"UDPWait", 4, appIniPath.c_str()), FALSE);
	SetDlgItemInt(m_hWnd, IDC_EDIT_WAIT_PACKET, GetPrivateProfileInt(L"SET", L"UDPPacket", 128, appIniPath.c_str()), FALSE);

	for( int tcp = 0; tcp < 2; tcp++ ){
		vector<NW_SEND_INFO>& sendList = tcp ? tcpSendList : udpSendList;
		for( size_t i = 0; i < sendList.size(); i++ ){
			wstring add;
			Format(add, L"%ls:%d-%d%ls",
			       sendList[i].ipString.c_str(),
			       sendList[i].port, sendList[i].port + BON_NW_PORT_RANGE - 1,
			       sendList[i].broadcastFlag ? L" (Broadcast)" :
			       tcp == 0 ? L"" :
			       sendList[i].ipString == BON_NW_SRV_PIPE_IP ? L" (SrvPipe)" :
			       sendList[i].ipString == BON_NW_PIPE_IP ? L" (Pipe)" : L"");
			ListBox_AddString(GetDlgItem(tcp ? IDC_LIST_IP_TCP : IDC_LIST_IP_UDP), add.c_str());
		}
	}
	SetDlgItemText(m_hWnd, IDC_IPADDRESS_UDP, L"127.0.0.1");
	SetDlgItemInt(m_hWnd, IDC_EDIT_PORT_UDP, BON_UDP_PORT_BEGIN, FALSE);

	CheckRadioButton(m_hWnd, IDC_RADIO_TCP, IDC_RADIO_PIPE, IDC_RADIO_TCP);
	SetDlgItemText(m_hWnd, IDC_IPADDRESS_TCP, L"127.0.0.1");
	SetDlgItemInt(m_hWnd, IDC_EDIT_PORT_TCP, BON_TCP_PORT_BEGIN, FALSE);

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
		int n;
		if( ParseIPv4Address(udpSendList[i].ipString.c_str(), n) ){
			WritePrivateProfileInt(L"SET_UDP", key, n, appIniPath.c_str());
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
		int n;
		if( ParseIPv4Address(tcpSendList[i].ipString.c_str(), n) ){
			WritePrivateProfileInt(L"SET_TCP", key, n, appIniPath.c_str());
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
	// IPアドレスっぽくないものを弾く
	if( std::find_if(item.ipString.begin(), item.ipString.end(), [](WCHAR c) {
	        return (c < L'0' || L'9' < c) && (c < L'A' || L'Z' < c) && (c < L'a' || L'z' < c) && c != L'%' && c != L'.' && c != L':'; }) != item.ipString.end() ||
	    item.ipString.find_first_not_of(L"%.:") == wstring::npos ||
	    item.ipString.find_first_of(L".:") == wstring::npos ){
		return;
	}

	item.broadcastFlag = Button_GetCheck(GetDlgItem(IDC_CHECK_BROADCAST)) != BST_UNCHECKED;
	wstring add;
	Format(add, L"%ls:%d-%d%ls",
	       item.ipString.c_str(),
	       item.port, item.port + BON_NW_PORT_RANGE - 1,
	       item.broadcastFlag ? L" (Broadcast)" : L"");

	for( size_t i = 0; i < udpSendList.size(); i++ ){
		if( CompareNoCase(udpSendList[i].ipString, item.ipString) == 0 &&
		    udpSendList[i].port == item.port &&
		    udpSendList[i].broadcastFlag == item.broadcastFlag ){
			return;
		}
	}
	ListBox_AddString(GetDlgItem(IDC_LIST_IP_UDP), add.c_str());
	udpSendList.push_back(item);
}


void CSetDlgNetwork::OnBnClickedButtonDelUdp()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	HWND hItem = GetDlgItem(IDC_LIST_IP_UDP);
	int sel = ListBox_GetCurSel(hItem);
	if( sel != LB_ERR ){
		ListBox_DeleteString(hItem, sel);
		udpSendList.erase(udpSendList.begin() + sel);
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
	// IPアドレスっぽくないものを弾く
	if( std::find_if(item.ipString.begin(), item.ipString.end(), [](WCHAR c) {
	        return (c < L'0' || L'9' < c) && (c < L'A' || L'Z' < c) && (c < L'a' || L'z' < c) && c != L'%' && c != L'.' && c != L':'; }) != item.ipString.end() ||
	    item.ipString.find_first_not_of(L"%.:") == wstring::npos ||
	    item.ipString.find_first_of(L".:") == wstring::npos ){
		return;
	}

	wstring add;
	Format(add, L"%ls:%d-%d%ls",
	       item.ipString.c_str(),
	       item.port, item.port + BON_NW_PORT_RANGE - 1,
	       item.ipString == BON_NW_SRV_PIPE_IP ? L" (SrvPipe)" :
	       item.ipString == BON_NW_PIPE_IP ? L" (Pipe)" : L"");
	item.broadcastFlag = FALSE;

	for( size_t i = 0; i < tcpSendList.size(); i++ ){
		if( CompareNoCase(tcpSendList[i].ipString, item.ipString) == 0 &&
		    tcpSendList[i].port == item.port ){
			return;
		}
	}
	ListBox_AddString(GetDlgItem(IDC_LIST_IP_TCP), add.c_str());
	tcpSendList.push_back(item);
}


void CSetDlgNetwork::OnBnClickedButtonDelTcp()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	HWND hItem = GetDlgItem(IDC_LIST_IP_TCP);
	int sel = ListBox_GetCurSel(hItem);
	if( sel != LB_ERR ){
		ListBox_DeleteString(hItem, sel);
		tcpSendList.erase(tcpSendList.begin() + sel);
	}
}


void CSetDlgNetwork::OnBnClickedRadioTcp()
{
	if( Button_GetCheck(GetDlgItem(IDC_RADIO_SRV_PIPE)) == BST_CHECKED ){
		EnableWindow(GetDlgItem(IDC_IPADDRESS_TCP), FALSE);
		EnableWindow(GetDlgItem(IDC_EDIT_PORT_TCP), FALSE);
		SetDlgItemText(m_hWnd, IDC_IPADDRESS_TCP, BON_NW_SRV_PIPE_IP);
		SetDlgItemInt(m_hWnd, IDC_EDIT_PORT_TCP, 0, FALSE);
	}else if( Button_GetCheck(GetDlgItem(IDC_RADIO_PIPE)) == BST_CHECKED ){
		EnableWindow(GetDlgItem(IDC_IPADDRESS_TCP), FALSE);
		EnableWindow(GetDlgItem(IDC_EDIT_PORT_TCP), TRUE);
		SetDlgItemText(m_hWnd, IDC_IPADDRESS_TCP, BON_NW_PIPE_IP);
		SetDlgItemInt(m_hWnd, IDC_EDIT_PORT_TCP, 0, FALSE);
	}else{
		EnableWindow(GetDlgItem(IDC_IPADDRESS_TCP), TRUE);
		EnableWindow(GetDlgItem(IDC_EDIT_PORT_TCP), TRUE);
		SetDlgItemText(m_hWnd, IDC_IPADDRESS_TCP, L"127.0.0.1");
		SetDlgItemInt(m_hWnd, IDC_EDIT_PORT_TCP, BON_TCP_PORT_BEGIN, FALSE);
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
		case IDC_RADIO_TCP:
		case IDC_RADIO_SRV_PIPE:
		case IDC_RADIO_PIPE:
			pSys->OnBnClickedRadioTcp();
			break;
		}
		break;
	}
	return FALSE;
}
