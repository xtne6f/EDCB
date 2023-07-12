#include "stdafx.h"
#include "resource.h"
#include "StreamCtrlDlg.h"
#include <commctrl.h>


CStreamCtrlDlg::CStreamCtrlDlg(void)
{
	this->hwnd = NULL;
	this->ctrlEnabled = FALSE;
	this->callbackFunc = NULL;
	this->callbackParam = NULL;
	this->thumbTracking = FALSE;
	this->getPosState = 0;
}

CStreamCtrlDlg::~CStreamCtrlDlg(void)
{
}

void CStreamCtrlDlg::SetCtrl(const TVTEST_STREAMING_INFO& info)
{
	if( this->ctrlEnabled && (info.enableMode == FALSE || info.ctrlID != this->ctrlID) ){
		this->cmd.SendNwPlayClose(this->ctrlID);
		StopTimer();
		this->ctrlEnabled = FALSE;
	}
	if( this->ctrlEnabled == FALSE && info.enableMode ){
		this->ctrlEnabled = TRUE;
		this->ctrlID = info.ctrlID;
		this->ctrlIsNetwork = info.serverIP != 1;
		this->cmd.SetSendMode(this->ctrlIsNetwork);
		if( this->ctrlIsNetwork ){
			// ネットワーク接続を使う
			// TODO: IPv6対応。ただしBonDriver_UDP/TCPが基本未対応なのでメリットは少ない
			wstring ip;
			Format(ip, L"%d.%d.%d.%d", info.serverIP >> 24, (info.serverIP >> 16) & 0xFF, (info.serverIP >> 8) & 0xFF, info.serverIP & 0xFF);
			this->cmd.SetNWSetting(ip, info.serverPort);
		}
		if( this->hwnd ){
			EnableWindow(GetDlgItem(this->hwnd, IDC_CHECK_UDP), this->ctrlIsNetwork);
			SendDlgItemMessage(this->hwnd, IDC_SLIDER_SEEK, TBM_SETPOS, TRUE, 0);
			SendDlgItemMessage(this->hwnd, IDC_CHECK_UDP, BM_SETCHECK, info.udpSend ? BST_CHECKED : BST_UNCHECKED, 0);
			SendDlgItemMessage(this->hwnd, IDC_CHECK_TCP, BM_SETCHECK, info.tcpSend ? BST_CHECKED : BST_UNCHECKED, 0);
			SetNWModeSend();
			this->cmd.SendNwPlayStart(this->ctrlID);
			SetTimer(this->hwnd, 1000, 500, NULL);
			this->thumbTracking = FALSE;
			this->getPosState = 0;
		}
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
	CStreamCtrlDlg* sys = (CStreamCtrlDlg*)GetWindowLongPtr(hDlgWnd, GWLP_USERDATA);
	if( sys == NULL && msg != WM_INITDIALOG ){
		return FALSE;
	}
	switch (msg) {
		case WM_KEYDOWN:
			break;
		case WM_INITDIALOG:
			SetWindowLongPtr(hDlgWnd, GWLP_USERDATA, lp);
			sys = (CStreamCtrlDlg*)lp;
			sys->hwnd = hDlgWnd;
			SetWindowText(GetDlgItem(hDlgWnd, IDC_EDIT_LOG), L"停止");
			break;
        case WM_COMMAND:
			{
				if( sys->ctrlEnabled ){
					switch (LOWORD(wp)) {
					case IDC_BUTTON_PLAY:
						sys->SetNWModeSend();
						sys->cmd.SendNwPlayStart(sys->ctrlID);
						SetTimer(hDlgWnd, 1000, 500, NULL);
						sys->thumbTracking = FALSE;
						sys->getPosState = 0;
						break;
					case IDC_BUTTON_STOP:
						KillTimer(hDlgWnd, 1000);
						sys->cmd.SendNwPlayStop(sys->ctrlID);
						SetWindowText(GetDlgItem(hDlgWnd, IDC_EDIT_LOG), L"停止");
						break;
					case IDC_CHECK_UDP:
					case IDC_CHECK_TCP:
						sys->SetNWModeSend();
						break;
					case IDC_BUTTON_CLOSE:
						KillTimer(hDlgWnd, 1000);
						KillTimer(hDlgWnd, 1010);
						sys->cmd.SendNwPlayStop(sys->ctrlID);
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
				if( wp == SIZE_RESTORED || wp == SIZE_MAXIMIZED ){
					RECT rc;
					GetClientRect(hDlgWnd, &rc);
					int cx = rc.right;

					GetWindowRect(GetDlgItem(hDlgWnd, IDC_EDIT_LOG), &rc);
					POINT pt;
					pt.x = rc.left;
					pt.y = rc.top;
					ScreenToClient(hDlgWnd, &pt);
					int x = cx - (rc.right - rc.left) - 5;
					int y = pt.y;
					GetWindowRect(GetDlgItem(hDlgWnd, IDC_BUTTON_CLOSE), &rc);
					pt.x = rc.right;
					pt.y = rc.top;
					ScreenToClient(hDlgWnd, &pt);
					if( x < pt.x + 5 ){
						x = pt.x + 5;
					}
					SetWindowPos(GetDlgItem(hDlgWnd, IDC_EDIT_LOG), NULL, x, y, 0, 0, SWP_NOZORDER|SWP_NOSIZE|SWP_SHOWWINDOW);

					GetWindowRect(GetDlgItem(hDlgWnd, IDC_SLIDER_SEEK), &rc);
					int cy = rc.bottom - rc.top;
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
					GetWindowRect(hDlgWnd, &rcWnd);
					int cy = rcWnd.bottom - rcWnd.top;

					if( pos.y > rc.bottom - cy && rc.left < pos.x && pos.x < rc.right ){
						int x = rc.left;
						int y = rc.bottom - cy;
						int cx = rc.right - rc.left;

						ShowWindow(hDlgWnd, SW_SHOW);
						SetWindowPos(hDlgWnd, HWND_TOPMOST, x, y, cx, cy, SWP_SHOWWINDOW);
					}else{
						ShowWindow(hDlgWnd, SW_HIDE);
					}
				}else if( wp == 1000 && sys->ctrlEnabled ){
					LONGLONG totalPos = 0;
					LONGLONG filePos = 0;

					// ファイルサイズと位置の変化を予測して問い合わせを減らす
					const int interval = 8;
					if( sys->getPosState <= interval ){
						// 実測
						NWPLAY_POS_CMD item;
						item.ctrlID = sys->ctrlID;
						if( sys->cmd.SendNwPlayGetPos(&item) != CMD_SUCCESS ){
							sys->getPosState = 0;
						}else{
							if( sys->getPosState == interval ){
								sys->totalPosDelta = item.totalPos - sys->measuredTotalPos;
								sys->filePosDelta = item.currentPos - sys->measuredFilePos;
							}
							if( sys->getPosState == 0 || sys->getPosState == interval ){
								sys->measuredTotalPos = item.totalPos;
								sys->measuredFilePos = item.currentPos;
							}
							totalPos = item.totalPos;
							filePos = item.currentPos;
							++sys->getPosState;
						}
					}else{
						// 予測
						totalPos = sys->measuredTotalPos + sys->totalPosDelta * (sys->getPosState - interval) / interval;
						if( totalPos < 0 ){
							totalPos = 0;
						}
						filePos = sys->measuredFilePos + sys->filePosDelta * (sys->getPosState - interval) / interval;
						if( filePos < 0 ){
							filePos = 0;
						}
						if( filePos > totalPos ){
							filePos = totalPos;
						}
						if( ++sys->getPosState == interval * 2 ){
							sys->getPosState = interval;
						}
					}

					if( totalPos<10000 ){
						SendDlgItemMessage( hDlgWnd, IDC_SLIDER_SEEK, TBM_SETRANGEMAX, FALSE, (DWORD)totalPos );
						SendDlgItemMessage( hDlgWnd, IDC_SLIDER_SEEK, TBM_SETPOS, TRUE, (DWORD)filePos );
					}else{
						SendDlgItemMessage( hDlgWnd, IDC_SLIDER_SEEK, TBM_SETRANGEMAX, FALSE, 10000 );
						double pos1 = ((double)totalPos)/10000;
						int setPos = (int)(filePos/pos1);
						SendDlgItemMessage( hDlgWnd, IDC_SLIDER_SEEK, TBM_SETPOS, TRUE, (DWORD)setPos );
					}
				}
			}
			return FALSE;
		case WM_HSCROLL:
			{
				if( sys->ctrlEnabled ){
					switch(LOWORD(wp)){
					case SB_THUMBTRACK:
						if( sys->thumbTracking == FALSE ){
							KillTimer(hDlgWnd, 1000);
							sys->cmd.SendNwPlayStop(sys->ctrlID);
							sys->thumbTracking = TRUE;
						}
						break;
					case SB_LINELEFT:
					case SB_LINERIGHT:
					case SB_PAGELEFT:
					case SB_PAGERIGHT:
					case SB_THUMBPOSITION:
						{
							KillTimer(hDlgWnd, 1000);

							sys->cmd.SendNwPlayStop(sys->ctrlID);

							LONGLONG filePos = SendDlgItemMessage( hDlgWnd, IDC_SLIDER_SEEK, TBM_GETPOS, 0, 0);

							NWPLAY_POS_CMD item;
							item.ctrlID = sys->ctrlID;
							sys->cmd.SendNwPlayGetPos(&item);

							if( item.totalPos<10000 ){
								SendDlgItemMessage( hDlgWnd, IDC_SLIDER_SEEK, TBM_SETRANGEMAX, FALSE, (DWORD)item.totalPos );
								item.currentPos = filePos;
								//SendDlgItemMessage( hDlgWnd, IDC_SLIDER_SEEK, TBM_SETPOS, TRUE, (DWORD)item.currentPos );
							}else{
								SendDlgItemMessage( hDlgWnd, IDC_SLIDER_SEEK, TBM_SETRANGEMAX, FALSE, 10000 );
								double pos1 = ((double)item.totalPos)/10000;
								filePos = (LONGLONG)(filePos*pos1);
								item.currentPos = filePos;
								//SendDlgItemMessage( hDlgWnd, IDC_SLIDER_SEEK, TBM_SETPOS, TRUE, (DWORD)filePos );
							}
							sys->cmd.SendNwPlaySetPos(&item);
							sys->cmd.SendNwPlayStart(sys->ctrlID);
							SetTimer(hDlgWnd, 1000, 500, NULL);
							sys->thumbTracking = FALSE;
							sys->getPosState = 0;
						}
						break;
					default:
						break;
					}
					return FALSE;
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
	NWPLAY_PLAY_INFO nwPlayInfo;
	nwPlayInfo.ctrlID = this->ctrlID;
	nwPlayInfo.udp = this->ctrlIsNetwork && SendDlgItemMessage(this->hwnd, IDC_CHECK_UDP, BM_GETCHECK, 0, 0) != 0;
	nwPlayInfo.tcp = SendDlgItemMessage(this->hwnd, IDC_CHECK_TCP, BM_GETCHECK, 0, 0) != 0;
	nwPlayInfo.udpPort = 65536;
	nwPlayInfo.tcpPort = 65536;
	// cmdがパイプ接続のときはip=2("0.0.0.2"=Pipe)にストリームが送られる
	// cmdがネットワーク接続のときipは無視され、常にサーバから見たこちら側にストリームが送られる
	// getsockname()で得たIPをセットすれば(ipを無視しない)古いサーバにも対応できるがそこまではしない
	nwPlayInfo.ip = 2;
	this->cmd.SendNwPlaySetIP(&nwPlayInfo);

	wstring editLog;
	DWORD udpPort = 65536;
	if( nwPlayInfo.udp == 1 ){
		udpPort = nwPlayInfo.udpPort;
		wstring udp;
		Format(udp, L"UDP送信：%d", udpPort);
		editLog += udp;
	}
	DWORD tcpPort = 65536;
	if( nwPlayInfo.tcp == 1 ){
		tcpPort = nwPlayInfo.tcpPort;
		wstring tcp;
		Format(tcp, L"TCP送信：%ls%d", this->ctrlIsNetwork ? L"" : L"(Pipe)", tcpPort);
		if( editLog.empty() == false ){
			editLog += L", ";
		}
		editLog += tcp;
	}
	SetDlgItemText(this->hwnd, IDC_EDIT_LOG, editLog.empty() ? L"送信先なし" : editLog.c_str());
	PostMessage(this->hwnd, WM_CHG_PORT, udpPort, tcpPort);
}
