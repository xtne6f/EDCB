#include "StdAfx.h"
#include "resource.h"
#include "StreamCtrlDlg.h"
#include <Commctrl.h>

#include <Iphlpapi.h>
#pragma comment(lib, "IPHLPAPI.lib")


CStreamCtrlDlg::CStreamCtrlDlg(void)
{
	this->hwnd = NULL;
	this->cmd = NULL;
	this->ctrlID = 0;
	this->iniTCP = FALSE;
	this->iniUDP = FALSE;
	this->timeShiftMode = FALSE;
	this->callbackFunc = NULL;
	this->callbackParam = NULL;
}

CStreamCtrlDlg::~CStreamCtrlDlg(void)
{
}

void CStreamCtrlDlg::SetCtrlCmd(CSendCtrlCmd* ctrlCmd, DWORD ctrlID_, BOOL chkUdp, BOOL chkTcp, BOOL play, BOOL timeShiftMode_)
{
	this->cmd = ctrlCmd;
	this->ctrlID = ctrlID_;
	this->iniTCP = chkTcp;
	this->iniUDP = chkUdp;
	this->timeShiftMode = timeShiftMode_;
	if( this->hwnd != NULL ){
		PostMessage(this->hwnd, WM_RESET_GUI, play, 0);
	}
}

void CStreamCtrlDlg::SetMessageCallback(MessageCallbackFunc func, void* param)
{
	this->callbackFunc = func;
	this->callbackParam = param;
}

DWORD CStreamCtrlDlg::CreateStreamCtrlDialog(HINSTANCE hInstance, HWND parentHWND)
{
	this->hwnd = CreateDialogParam(hInstance,MAKEINTRESOURCE(IDD_DIALOG_NW_CTRL), parentHWND, (DLGPROC)DlgProc, (LPARAM)this );
	if( this->hwnd == NULL ){
		return FALSE;
	}
	this->parentHwnd = parentHWND;
	return TRUE;
}

void CStreamCtrlDlg::CloseStreamCtrlDialog()
{
	if( this->hwnd != NULL ){
		KillTimer(this->hwnd, 1010);
		KillTimer(this->hwnd, 1000);
	}
	if( this->hwnd != NULL ){
		DestroyWindow(this->hwnd);
	}
}

void CStreamCtrlDlg::StopTimer()
{
	if( this->hwnd != NULL ){
		KillTimer(this->hwnd, 1010);
		KillTimer(this->hwnd, 1000);
	}
}

BOOL CStreamCtrlDlg::ShowCtrlDlg(DWORD cmdShow)
{
	if( this->hwnd == NULL ){
		return FALSE;
	}
	return ShowWindow(this->hwnd, cmdShow);
}

void CStreamCtrlDlg::StartFullScreenMouseChk()
{
	if( this->hwnd != NULL ){
		SetTimer(this->hwnd, 1010, 300, NULL);
	}
}

void CStreamCtrlDlg::StopFullScreenMouseChk()
{
	if( this->hwnd != NULL ){
		KillTimer(this->hwnd, 1010);
	}
}

LRESULT CALLBACK CStreamCtrlDlg::DlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static CStreamCtrlDlg* sys = NULL;
	switch (msg) {
		case WM_KEYDOWN:
			break;
		case WM_INITDIALOG:
			sys = (CStreamCtrlDlg*)lp;
			sys->hwnd = hDlgWnd;
			sys->EnumIP();
			SetWindowText(GetDlgItem(hDlgWnd, IDC_EDIT_LOG), L"停止");
			break;
        case WM_COMMAND:
			if(sys != NULL ){
				if( sys->cmd != NULL ){
					switch (LOWORD(wp)) {
					case IDC_BUTTON_PLAY:
						sys->SetNWModeSend();
						sys->cmd->SendNwPlayStart(sys->ctrlID);
						SetTimer(hDlgWnd, 1000, 500, NULL);
						sys->UpdateLog();
						break;
					case IDC_BUTTON_STOP:
						KillTimer(hDlgWnd, 1000);
						sys->cmd->SendNwPlayStop(sys->ctrlID);
						SetWindowText(GetDlgItem(hDlgWnd, IDC_EDIT_LOG), L"停止");
						break;
					case IDC_CHECK_UDP:
					case IDC_CHECK_TCP:
						sys->SetNWModeSend();
						sys->UpdateLog();
						break;
					case IDC_BUTTON_CLOSE:
						KillTimer(hDlgWnd, 1000);
						KillTimer(hDlgWnd, 1010);
						sys->cmd->SendNwPlayStop(sys->ctrlID);
						SetWindowText(GetDlgItem(hDlgWnd, IDC_EDIT_LOG), L"停止");
						SendMessage(GetDlgItem(sys->hwnd, IDC_CHECK_UDP), BM_SETCHECK, BST_UNCHECKED, 0);
						SendMessage(GetDlgItem(sys->hwnd, IDC_CHECK_TCP), BM_SETCHECK, BST_UNCHECKED, 0);
						sys->SetNWModeSend();
						ShowWindow(hDlgWnd, SW_HIDE);
						PostMessage(hDlgWnd, WM_PLAY_CLOSE, 0, 0);
						break;
					default:
						break;
					}
				}
			}
			return FALSE;
		case WM_SIZE:
			{
				RECT rc;
				GetWindowRect(hDlgWnd, &rc);

				if( wp == SIZE_RESTORED || wp == SIZE_MAXIMIZED ){
					int x = rc.right-rc.left - 140;
					int y = 3;
					SetWindowPos(GetDlgItem(hDlgWnd, IDC_EDIT_LOG), NULL, x, y, 0, 0, SWP_NOZORDER|SWP_NOSIZE|SWP_SHOWWINDOW);
					int cx = rc.right-rc.left - 140 - 20;
					int cy = 30;
					SetWindowPos(GetDlgItem(hDlgWnd, IDC_SLIDER_SEEK), NULL, 0, 0, cx, cy, SWP_NOZORDER|SWP_NOMOVE|SWP_SHOWWINDOW);
				}
			}
			break;
		case WM_TIMER:
			{
				if( wp == 1010 ){
					WINDOWPLACEMENT info;
					info.length = sizeof(WINDOWPLACEMENT);

					GetWindowPlacement(sys->parentHwnd, &info);
					RECT rcWnd = info.rcNormalPosition;

					HMONITOR hMonitor = MonitorFromRect(&rcWnd, MONITOR_DEFAULTTONEAREST);
					MONITORINFO mi;
					mi.cbSize = sizeof(MONITORINFO);
					GetMonitorInfo(hMonitor, &mi);

					RECT rc = mi.rcMonitor;

					POINT pos;
					GetCursorPos(&pos);

					if( pos.y > rc.bottom - 65 && rc.left < pos.x && pos.x < rc.right){
						int x = rc.left;
						int y = rc.bottom-65;
						int cx = rc.right - rc.left;
						int cy = 65;

						ShowWindow(hDlgWnd, SW_SHOW);
						SetWindowPos(hDlgWnd, HWND_TOPMOST, x, y, cx, cy, SWP_SHOWWINDOW);
					}else{
						ShowWindow(hDlgWnd, SW_HIDE);
					}
				}else if(wp == 1000){
					KillTimer(hDlgWnd, 1000);
					__int64 totalPos = 0;
					__int64 filePos = 0;

					NWPLAY_POS_CMD item;
					item.ctrlID = sys->ctrlID;
					sys->cmd->SendNwPlayGetPos(&item);
					totalPos = item.totalPos;
					filePos = item.currentPos;

					if( totalPos<10000 ){
						SendDlgItemMessage( hDlgWnd, IDC_SLIDER_SEEK, TBM_SETRANGEMAX, FALSE, (DWORD)totalPos );
						SendDlgItemMessage( hDlgWnd, IDC_SLIDER_SEEK, TBM_SETPOS, TRUE, (DWORD)filePos );
					}else{
						SendDlgItemMessage( hDlgWnd, IDC_SLIDER_SEEK, TBM_SETRANGEMAX, FALSE, 10000 );
						double pos1 = ((double)totalPos)/10000;
						int setPos = (int)(filePos/pos1);
						SendDlgItemMessage( hDlgWnd, IDC_SLIDER_SEEK, TBM_SETPOS, TRUE, (DWORD)setPos );
					}
					SetTimer(hDlgWnd, 1000, 500, NULL);
				}
			}
			return FALSE;
		case WM_HSCROLL:
			if(sys != NULL ){
				if( sys->cmd != NULL ){
					switch(LOWORD(wp)){
					case SB_THUMBTRACK:
						KillTimer(hDlgWnd, 1000);

						sys->cmd->SendNwPlayStop(sys->ctrlID);
						break;
					case SB_LINELEFT:
					case SB_LINERIGHT:
					case SB_PAGELEFT:
					case SB_PAGERIGHT:
					case SB_THUMBPOSITION:
						{
							KillTimer(hDlgWnd, 1000);

							sys->cmd->SendNwPlayStop(sys->ctrlID);

							__int64 filePos = SendDlgItemMessage( hDlgWnd, IDC_SLIDER_SEEK, TBM_GETPOS, 0, 0);

							NWPLAY_POS_CMD item;
							item.ctrlID = sys->ctrlID;
							sys->cmd->SendNwPlayGetPos(&item);

							if( item.totalPos<10000 ){
								SendDlgItemMessage( hDlgWnd, IDC_SLIDER_SEEK, TBM_SETRANGEMAX, FALSE, (DWORD)item.totalPos );
								item.currentPos = filePos;
								//SendDlgItemMessage( hDlgWnd, IDC_SLIDER_SEEK, TBM_SETPOS, TRUE, (DWORD)item.currentPos );
							}else{
								SendDlgItemMessage( hDlgWnd, IDC_SLIDER_SEEK, TBM_SETRANGEMAX, FALSE, 10000 );
								double pos1 = ((double)item.totalPos)/10000;
								filePos = (__int64)(filePos*pos1);
								item.currentPos = filePos;
								//SendDlgItemMessage( hDlgWnd, IDC_SLIDER_SEEK, TBM_SETPOS, TRUE, (DWORD)filePos );
							}
							sys->cmd->SendNwPlaySetPos(&item);
							sys->cmd->SendNwPlayStart(sys->ctrlID);
							SetTimer(hDlgWnd, 1000, 500, NULL);
						}
						break;
					default:
						break;
					}
					return FALSE;
				}
			}
			break;
		case WM_RESET_GUI:
			{
				SendDlgItemMessage( hDlgWnd, IDC_SLIDER_SEEK, TBM_SETPOS, TRUE, 0 );
				SetWindowText(GetDlgItem(hDlgWnd, IDC_EDIT_LOG), L"停止");
				if( sys->iniUDP == TRUE ){
					SendMessage(GetDlgItem(sys->hwnd, IDC_CHECK_UDP), BM_SETCHECK, BST_CHECKED, 0);
				}else{
					SendMessage(GetDlgItem(sys->hwnd, IDC_CHECK_UDP), BM_SETCHECK, BST_UNCHECKED, 0);
				}
				if( sys->iniTCP == TRUE ){
					SendMessage(GetDlgItem(sys->hwnd, IDC_CHECK_TCP), BM_SETCHECK, BST_CHECKED, 0);
				}else{
					SendMessage(GetDlgItem(sys->hwnd, IDC_CHECK_TCP), BM_SETCHECK, BST_UNCHECKED, 0);
				}
				if( wp == 1 ){
					sys->SetNWModeSend();
					sys->cmd->SendNwPlayStart(sys->ctrlID);
					SetTimer(hDlgWnd, 1000, 500, NULL);
					sys->UpdateLog();
				}
			}
			break;
		default:
			if( (msg == WM_CHG_PORT || msg == WM_PLAY_CLOSE || msg >= WM_CUSTOM) && sys->callbackFunc != NULL ){
				return sys->callbackFunc(hDlgWnd, msg, wp, lp, sys->callbackParam);
			}
			return FALSE;
	}

	return TRUE;
}

void CStreamCtrlDlg::SetNWModeSend()
{
	WCHAR textBuff[512] = L"";
	GetWindowText(GetDlgItem(this->hwnd, IDC_COMBO_IP), textBuff, 512);
	
	wstring strIP = textBuff;
	wstring ip1;
	wstring ip2;
	wstring ip3;
	wstring ip4;
	Separate(strIP, L".", ip1, strIP);
	Separate(strIP, L".", ip2, strIP);
	Separate(strIP, L".", ip3, ip4);

	nwPlayInfo.ctrlID = this->ctrlID;
	nwPlayInfo.udp = (BYTE)SendMessage(GetDlgItem(this->hwnd, IDC_CHECK_UDP), BM_GETCHECK, 0, 0);
	nwPlayInfo.tcp = (BYTE)SendMessage(GetDlgItem(this->hwnd, IDC_CHECK_TCP), BM_GETCHECK, 0, 0);
	nwPlayInfo.ip = _wtoi(ip1.c_str())<<24 | _wtoi(ip2.c_str())<<16 | _wtoi(ip3.c_str())<<8 | _wtoi(ip4.c_str());
	this->cmd->SendNwPlaySetIP(&nwPlayInfo);

	DWORD udpPort = 0;
	if( nwPlayInfo.udp == 1 ){
		udpPort = nwPlayInfo.udpPort;
	}
	DWORD tcpPort = 0;
	if( nwPlayInfo.tcp == 1 ){
		tcpPort = nwPlayInfo.tcpPort;
	}
	PostMessage(this->hwnd, WM_CHG_PORT, udpPort, tcpPort);
}

void CStreamCtrlDlg::EnumIP()
{
	ULONG len = 0;   // 列挙に必要なバイト数です。
	DWORD ret = GetAdaptersAddresses(AF_UNSPEC, 0, 0, 0, &len);
	if(ret != ERROR_BUFFER_OVERFLOW) return;   // メモリ不足以外のエラーなら制御を返します。

	// 要求されたバイト数 len 分のメモリを adpts に用意します。
	PIP_ADAPTER_ADDRESSES adpts = (PIP_ADAPTER_ADDRESSES)new BYTE[len];
	if(adpts == 0) return;      // メモリを用意できなければ制御を返します。

	ret = GetAdaptersAddresses(AF_INET, 0, 0, adpts, &len);   // adpts に情報を列挙します。
	if(ret != ERROR_SUCCESS){   // アダプタを列挙できなかったら制御を返します。
		delete [] adpts;
		return;
	}

	WSADATA data;
	ZeroMemory(&data, sizeof(data));
	WSAStartup(MAKEWORD(2, 2), &data);
	// adpts 配列内の各アダプタ情報を一つずつ adpt に入れてみていきます。
	for(PIP_ADAPTER_ADDRESSES adpt = adpts; adpt; adpt = adpt->Next){
		if(adpt->PhysicalAddressLength == 0) continue;            // データリンク層を持たないなら
		if(adpt->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;   // ループバック アドレスなら
		// adpt アダプタ情報の IP アドレスを一つずつ uni に入れてみていきます。
		for(PIP_ADAPTER_UNICAST_ADDRESS uni = adpt->FirstUnicastAddress; uni; uni = uni->Next){
			// DNS に対して適切なアドレスでない（プライベートなど）ならば
			if(~(uni->Flags) & IP_ADAPTER_ADDRESS_DNS_ELIGIBLE) continue;   // 次のアドレスへ
			// アプリケーションが使うべきでないアドレスなら
			if(uni->Flags & IP_ADAPTER_ADDRESS_TRANSIENT) continue;         // 次のアドレスへ
			char host[NI_MAXHOST + 1] = {'\0'};   // host に「0.0.0.0」形式の文字列を取得します。
			if(getnameinfo(uni->Address.lpSockaddr, uni->Address.iSockaddrLength
			, host, sizeof(host), 0, 0, NI_NUMERICHOST/*NI_NAMEREQD*/) == 0){
				wstring strW;
				AtoW(host, strW);
				SendDlgItemMessage(this->hwnd , IDC_COMBO_IP, CB_ADDSTRING , 0 , (LPARAM)strW.c_str());
			}
		}
	}
	WSACleanup();
	delete [] adpts;

	if(SendDlgItemMessage(this->hwnd , IDC_COMBO_IP, CB_GETCOUNT , 0 , 0) > 0){
		SendDlgItemMessage(this->hwnd , IDC_COMBO_IP, CB_SETCURSEL , 0 , 0);
	}
}

void CStreamCtrlDlg::UpdateLog()
{
	wstring editLog = L"";

	WCHAR textBuff[512] = L"";
	GetWindowText(GetDlgItem(this->hwnd, IDC_COMBO_IP), textBuff, 512);
	
	wstring IP = textBuff;

	if( this->nwPlayInfo.udp == 1 ){
		wstring udp = L"";
		Format(udp, L"UDP送信：%s:%d\r\n", IP.c_str(), nwPlayInfo.udpPort);
		editLog += udp;
	}
	if( this->nwPlayInfo.tcp == 1 ){
		wstring tcp = L"";
		Format(tcp, L"TCP送信：%s:%d\r\n", IP.c_str(), nwPlayInfo.tcpPort);
		editLog += tcp;
	}
	
	if( editLog.size() == 0 ){
		editLog = L"送信先なし";
	}

	SetWindowText(GetDlgItem(this->hwnd, IDC_EDIT_LOG), editLog.c_str());
}
