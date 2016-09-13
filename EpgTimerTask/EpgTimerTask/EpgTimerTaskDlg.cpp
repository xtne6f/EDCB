
// EpgTimerTaskDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "EpgTimerTask.h"
#include "EpgTimerTaskDlg.h"
#include "../../Common/CommonDef.h"


// CEpgTimerTaskDlg ダイアログ




CEpgTimerTaskDlg::CEpgTimerTaskDlg()
	: m_hDlg(NULL)
{
	HMODULE hModule = GetModuleHandle(NULL);
	HRESULT (WINAPI* pfnLoadIconMetric)(HINSTANCE,PCWSTR,int,HICON*) =
		(HRESULT (WINAPI*)(HINSTANCE,PCWSTR,int,HICON*))GetProcAddress(GetModuleHandle(L"comctl32.dll"), "LoadIconMetric");
	if( pfnLoadIconMetric == NULL ||
	    pfnLoadIconMetric(hModule, MAKEINTRESOURCE(IDR_MAINFRAME), LIM_SMALL, &m_hIcon) != S_OK ||
	    pfnLoadIconMetric(hModule, MAKEINTRESOURCE(IDR_MAINFRAME), LIM_LARGE, &m_hIcon2) != S_OK ||
	    pfnLoadIconMetric(hModule, MAKEINTRESOURCE(IDI_ICON_RED), LIM_SMALL, &m_hIconRed) != S_OK ||
	    pfnLoadIconMetric(hModule, MAKEINTRESOURCE(IDI_ICON_GREEN), LIM_SMALL, &m_hIconGreen) != S_OK ){
		m_hIcon = (HICON)LoadImage(hModule, MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
		m_hIcon2 = (HICON)LoadImage(hModule, MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
		m_hIconRed = (HICON)LoadImage(hModule, MAKEINTRESOURCE(IDI_ICON_RED), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
		m_hIconGreen = (HICON)LoadImage(hModule, MAKEINTRESOURCE(IDI_ICON_GREEN), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	}

	m_uMsgTaskbarCreated = RegisterWindowMessage(L"TaskbarCreated");
}

INT_PTR CEpgTimerTaskDlg::DoModal()
{
	//return DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD), NULL, DlgProc, (LPARAM)this);
	//最小化起動のため↑とだいたい同じ挙動になるようにメッセージを回す
	HWND hDlg = CreateDialogParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD), NULL, DlgProc, (LPARAM)this);
	if( hDlg != NULL ){
		MSG msg;
		while( m_hDlg != NULL && GetMessage(&msg, NULL, 0, 0) > 0 ){
			if( msg.hwnd == hDlg && msg.message == WM_END_DIALOG ){
				DestroyWindow(hDlg);
				return (INT_PTR)msg.wParam;
			}else if( !IsDialogMessage(hDlg, &msg) ){
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}
	return IDCANCEL;
}


// CEpgTimerTaskDlg メッセージ ハンドラー

BOOL CEpgTimerTaskDlg::OnInitDialog()
{
	// このダイアログのアイコンを設定します。アプリケーションのメイン ウィンドウがダイアログでない場合、
	//  Framework は、この設定を自動的に行います。
	SendMessage(m_hDlg, WM_SETICON, ICON_BIG, (LPARAM)m_hIcon);		// 大きいアイコンの設定
	SendMessage(m_hDlg, WM_SETICON, ICON_SMALL, (LPARAM)m_hIcon);	// 小さいアイコンの設定

	// TODO: 初期化をここに追加します。
	wstring pipeName = L"";
	wstring pipeEventName = L"";
	Format(pipeName, L"\\\\.\\pipe\\EpgTimerGUI_Ctrl_BonPipe_%d", GetCurrentProcessId());
	Format(pipeEventName, L"Global\\EpgTimerGUI_Ctrl_BonConnect_%d", GetCurrentProcessId());

	m_cPipe.StartServer(pipeEventName.c_str(), pipeName.c_str(), OutsideCmdCallback, this);

	//ウインドウの復元
	WINDOWPLACEMENT Pos;
	Pos.length = sizeof(WINDOWPLACEMENT);
	Pos.flags = 0;
	Pos.showCmd = SW_HIDE;
	Pos.rcNormalPosition.left = 0;
	Pos.rcNormalPosition.right = 0;
	Pos.rcNormalPosition.top = 0;
	Pos.rcNormalPosition.bottom = 0;
	SetWindowPlacement(m_hDlg, &Pos);

	m_dwSrvStatus = 0;
	SendMessage(m_hDlg, WM_TIMER, RETRY_ADD_TRAY, 0);

	CSendCtrlCmd cmd;
	for( int timeout = 0; cmd.SendRegistGUI(GetCurrentProcessId()) != CMD_SUCCESS; timeout += 100 ){
		Sleep(100);
		if( timeout > CONNECT_TIMEOUT ){
			TCHAR szCaption[64];
			GetWindowText(m_hDlg, szCaption, _countof(szCaption));
			MessageBox(m_hDlg, L"EpgTimerSrv.exeの起動を確認できませんでした。\r\n終了してください。", szCaption, MB_OK);
			break;
		}
	}

	return TRUE;  // フォーカスをコントロールに設定した場合を除き、TRUE を返します。
}


BOOL CEpgTimerTaskDlg::AddTaskBar(HWND hWnd, UINT uiMsg, UINT uiID, HICON hIcon, wstring strTips)
{ 
	BOOL bRet;
	NOTIFYICONDATA stData;
	ZeroMemory(&stData, sizeof(NOTIFYICONDATA));

	stData.cbSize = sizeof(NOTIFYICONDATA); 
	stData.hWnd = hWnd; 
	stData.uID = uiID; 
	stData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP; 
	stData.uCallbackMessage = uiMsg; 
	stData.hIcon = hIcon; 

	wcsncpy_s(stData.szTip, strTips.c_str(), _countof(stData.szTip));
 
	bRet = Shell_NotifyIcon(NIM_ADD, &stData);
  
	return bRet; 
}

BOOL CEpgTimerTaskDlg::ChgTipsTaskBar(HWND hWnd, UINT uiID, HICON hIcon, wstring strTips)
{ 
	BOOL bRet;
	NOTIFYICONDATA stData;
	ZeroMemory(&stData, sizeof(NOTIFYICONDATA));

	stData.cbSize = sizeof(NOTIFYICONDATA); 
	stData.hWnd = hWnd; 
	stData.uID = uiID; 
	stData.hIcon = hIcon; 
	stData.uFlags = NIF_ICON | NIF_TIP; 

	wcsncpy_s(stData.szTip, strTips.c_str(), _countof(stData.szTip));
 
	bRet = Shell_NotifyIcon(NIM_MODIFY, &stData); 
 
	return bRet; 
}

BOOL CEpgTimerTaskDlg::DeleteTaskBar(HWND hWnd, UINT uiID)
{ 
	BOOL bRet=TRUE; 
	NOTIFYICONDATA stData; 
	ZeroMemory(&stData, sizeof(NOTIFYICONDATA));
 
	stData.cbSize = sizeof(NOTIFYICONDATA); 
	stData.hWnd = hWnd; 
	stData.uID = uiID; 
         
	bRet = Shell_NotifyIcon(NIM_DELETE, &stData); 
	return bRet; 
}


LRESULT CEpgTimerTaskDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	//ここではWM_USER以上のメッセージのみ取得できる
	switch(message){
		case WM_TRAY_PUSHICON:
			{
				//タスクトレイ関係
				switch(LOWORD(lParam)){
					case WM_LBUTTONUP:
						{
							//EpgTimer.exeがあれば起動
							wstring strPath;
							GetModuleFolderPath(strPath);
							strPath += L"\\EpgTimer.exe";
							PROCESS_INFORMATION pi;
							STARTUPINFO si = {};
							si.cb = sizeof(si);
							if( CreateProcess(strPath.c_str(), NULL, NULL, NULL, FALSE, GetPriorityClass(GetCurrentProcess()), NULL, NULL, &si, &pi) != FALSE ){
								CloseHandle(pi.hThread);
								CloseHandle(pi.hProcess);
							}
						}
						break;
					case WM_RBUTTONUP:
						{
							HMENU hMenu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_MENU_TRAY));
							if( hMenu != NULL ){
								HMENU hPopup = GetSubMenu(hMenu, 0);
								if( hPopup != NULL ){
									POINT point;
									GetCursorPos(&point);
									SetForegroundWindow(m_hDlg);
									TrackPopupMenu(hPopup, TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, 0, m_hDlg, NULL);
								}
								DestroyMenu(hMenu);
							}
						}
						break;
					default :
						break ;
					}
			}
			break;
		case WM_QUERY_SUSPEND:
			{
				CQueryWaitDlg cDlg;
				if( cDlg.DoModal() != IDCANCEL ){
					CSendCtrlCmd cmd;
					cmd.SendSuspend((WORD)wParam);
				}
			}
			break;
		case WM_QUERY_REBOOT:
			{
				CQueryWaitDlg cDlg;
				cDlg.SetRebootMode();
				if( cDlg.DoModal() != IDCANCEL ){
					CSendCtrlCmd cmd;
					cmd.SendReboot();
				}
			}
			break;
		default:
			if( message == m_uMsgTaskbarCreated ){
				//シェルの再起動時
				SetTimer(m_hDlg, RETRY_ADD_TRAY, 0, NULL);
			}
			break;
	}

	return FALSE;
}


void CEpgTimerTaskDlg::OnBnClickedButtonEnd()
{
	// TODO: ここにコントロール通知ハンドラ コードを追加します。
	PostMessage(m_hDlg, WM_END_DIALOG, 0, 0);
}

void CEpgTimerTaskDlg::OnBnClickedButtonS3()
{
	// TODO: ここにコントロール通知ハンドラ コードを追加します。
	CSendCtrlCmd cmd;
	if(cmd.SendChkSuspend() != 1 ){
		TCHAR szCaption[64];
		GetWindowText(m_hDlg, szCaption, _countof(szCaption));
		MessageBox(m_hDlg, L"スタンバイに移行できる状態ではありません。\r\n（もうすぐ予約が始まる。または抑制条件のexeが起動している。など）", szCaption, MB_OK);
	}else{
		cmd.SendSuspend(0xFF01);
	}
}

void CEpgTimerTaskDlg::OnBnClickedButtonS4()
{
	// TODO: ここにコントロール通知ハンドラ コードを追加します。
	CSendCtrlCmd cmd;
	if(cmd.SendChkSuspend() != 1 ){
		TCHAR szCaption[64];
		GetWindowText(m_hDlg, szCaption, _countof(szCaption));
		MessageBox(m_hDlg, L"休止に移行できる状態ではありません。\r\n（もうすぐ予約が始まる。または抑制条件のexeが起動している。など）", szCaption, MB_OK);
	}else{
		cmd.SendSuspend(0xFF02);
	}
}


void CEpgTimerTaskDlg::OnDestroy()
{
	KillTimer(m_hDlg, RETRY_ADD_TRAY);
	KillTimer(m_hDlg, RETRY_CHG_TRAY);
	DeleteTaskBar(m_hDlg, TRAYICON_ID);

	CSendCtrlCmd cmd;
    cmd.SendUnRegistGUI(GetCurrentProcessId());

	// TODO: ここにメッセージ ハンドラー コードを追加します。
}


void CEpgTimerTaskDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: ここにメッセージ ハンドラー コードを追加するか、既定の処理を呼び出します。
	if( nIDEvent == RETRY_ADD_TRAY ){
		KillTimer(m_hDlg, RETRY_ADD_TRAY);
		wstring strBuff=L"";
/*		RESERVE_DATA Item;
		if( GetNextReserve(&Item) == TRUE ){
			wstring strTime;
			GetTimeString3(Item.StartTime, Item.dwDurationSec, strTime);

			strBuff += L"次の予約 ";
			strBuff += strTime.c_str();
			strBuff += L" ";
			strBuff += Item.strTitle.c_str();
		}else{
			strBuff += L"次の予約 なし";
		}*/
		HICON hSetIcon = m_hIcon;
		switch(m_dwSrvStatus){
			case 1:
				hSetIcon = m_hIconRed;
				break;
			case 2:
				hSetIcon = m_hIconGreen;
				break;
			default:
				break;
		}
		if( AddTaskBar( m_hDlg,
				WM_TRAY_PUSHICON,
				TRAYICON_ID,
				hSetIcon,
				strBuff ) == FALSE ){
					SetTimer(m_hDlg, RETRY_ADD_TRAY, 5000, NULL);
		}
	}else if( nIDEvent == RETRY_CHG_TRAY ){
		KillTimer(m_hDlg, RETRY_CHG_TRAY);
		wstring strBuff=L"";
/*		RESERVE_DATA Item;
		if( GetNextReserve(&Item) == TRUE ){
			wstring strTime;
			GetTimeString3(Item.StartTime, Item.dwDurationSec, strTime);

			strBuff += L"次の予約 ";
			strBuff += strTime.c_str();
			strBuff += L" ";
			strBuff += Item.strTitle.c_str();
		}else{
			strBuff += L"次の予約 なし";
		}
*/
		HICON hSetIcon = m_hIcon;
		switch(m_dwSrvStatus){
			case 1:
				hSetIcon = m_hIconRed;
				break;
			case 2:
				hSetIcon = m_hIconGreen;
				break;
			default:
				break;
		}
		if( ChgTipsTaskBar( m_hDlg,
				TRAYICON_ID,
				hSetIcon,
				strBuff ) == FALSE ){
					SetTimer(m_hDlg, RETRY_CHG_TRAY, 5000, NULL);
		}
	}
}

int CALLBACK CEpgTimerTaskDlg::OutsideCmdCallback(void* pParam, CMD_STREAM* pCmdParam, CMD_STREAM* pResParam)
{
	CEpgTimerTaskDlg* pSys = (CEpgTimerTaskDlg*)pParam;
	pResParam->param = CMD_NON_SUPPORT;
	switch( pCmdParam->param ){
		case CMD2_TIMER_GUI_VIEW_EXECUTE:
			pSys->CmdViewExecute(pCmdParam, pResParam);
			break;
		case CMD2_TIMER_GUI_QUERY_SUSPEND:
			pSys->CmdViewQuerySuspend(pCmdParam, pResParam);
			break;
		case CMD2_TIMER_GUI_QUERY_REBOOT:
			pSys->CmdViewQueryReboot(pCmdParam, pResParam);
			break;
		case CMD2_TIMER_GUI_SRV_STATUS_NOTIFY2:
			pSys->CmdSrvStatusChg(pCmdParam, pResParam);
			break;
		default:
			pResParam->param = CMD_NON_SUPPORT;
			break;
	}

	return 0;
}

//CMD_TIMER_GUI_VIEW_EXECUTE Viewアプリ（EpgDataCap_Bon.exe）を起動
void CEpgTimerTaskDlg::CmdViewExecute(CMD_STREAM* pCmdParam, CMD_STREAM* pResParam)
{
	OutputDebugString(L"CEpgTimerTaskDlg::CmdViewExecute");
	wstring exeCmd = L"";

	ReadVALUE(&exeCmd, pCmdParam->data, pCmdParam->dataSize, NULL);

	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	ZeroMemory(&si,sizeof(si));
	si.cb=sizeof(si);
	if( exeCmd.find(L".bat") != string::npos ){
		si.wShowWindow |= SW_SHOWMINNOACTIVE;
		si.dwFlags |= STARTF_USESHOWWINDOW;
	}

	BOOL bRet = CreateProcess( NULL, (WCHAR*)exeCmd.c_str(), NULL, NULL, FALSE, GetPriorityClass(GetCurrentProcess()), NULL, NULL, &si, &pi );
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

	if( bRet == TRUE ){
		pResParam->data = NewWriteVALUE(pi.dwProcessId, pResParam->dataSize);
		pResParam->param = CMD_SUCCESS;
	}else{
		pResParam->param = CMD_ERR;
	}
}

//CMD_TIMER_GUI_QUERY_SUSPEND スタンバイ、休止、シャットダウンに入っていいかの確認をユーザーに行う
void CEpgTimerTaskDlg::CmdViewQuerySuspend(CMD_STREAM* pCmdParam, CMD_STREAM* pResParam)
{
	OutputDebugString(L"CEpgTimerTaskDlg::CmdViewQuerySuspend");
	WORD val = 0;
	ReadVALUE(&val, pCmdParam->data, pCmdParam->dataSize, NULL);

	PostMessage(m_hDlg, WM_QUERY_SUSPEND, val, 0);

	pResParam->param = CMD_SUCCESS;
}

//CMD_TIMER_GUI_QUERY_REBOOT PC再起動に入っていいかの確認をユーザーに行う
void CEpgTimerTaskDlg::CmdViewQueryReboot(CMD_STREAM* pCmdParam, CMD_STREAM* pResParam)
{
	OutputDebugString(L"CEpgTimerTaskDlg::CmdViewQueryReboot");
	WORD val = 0;
	ReadVALUE(&val, pCmdParam->data, pCmdParam->dataSize, NULL);

	PostMessage(m_hDlg, WM_QUERY_REBOOT, val, 0);

	pResParam->param = CMD_SUCCESS;
}

//CMD2_TIMER_GUI_SRV_STATUS_NOTIFY2 サーバーのステータス変更通知
void CEpgTimerTaskDlg::CmdSrvStatusChg(CMD_STREAM* pCmdParam, CMD_STREAM* pResParam)
{
	OutputDebugString(L"CEpgTimerTaskDlg::CmdSrvStatusChg");
	WORD ver = 0;
	DWORD readSize = 0;
	NOTIFY_SRV_INFO status;
	if( ReadVALUE(&ver, pCmdParam->data, pCmdParam->dataSize, &readSize) &&
	    ReadVALUE2(ver, &status, pCmdParam->data.get()+readSize, pCmdParam->dataSize-readSize, NULL) ){
		switch(status.notifyID){
		case NOTIFY_UPDATE_SRV_STATUS:
			m_dwSrvStatus = status.param1;
			SetTimer(m_hDlg, RETRY_CHG_TRAY, 0, NULL);
			break;
		default:
			break;
		}
		pResParam->param = CMD_SUCCESS;
	}else{
		pResParam->param = CMD_ERR;
	}
}

INT_PTR CALLBACK CEpgTimerTaskDlg::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CEpgTimerTaskDlg* pSys = (CEpgTimerTaskDlg*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
	switch( uMsg ){
	case WM_INITDIALOG:
		SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
		pSys = (CEpgTimerTaskDlg*)lParam;
		pSys->m_hDlg = hDlg;
		return pSys->OnInitDialog();
	case WM_NCDESTROY:
		pSys->m_hDlg = NULL;
		break;
	case WM_DESTROY:
		pSys->OnDestroy();
		break;
	case WM_TIMER:
		pSys->OnTimer(wParam);
		break;
	case WM_COMMAND:
		switch( LOWORD(wParam) ){
		case IDOK:
		case IDCANCEL:
			PostMessage(hDlg, WM_END_DIALOG, LOWORD(wParam), 0);
			return TRUE;
		case IDC_BUTTON_S3:
			pSys->OnBnClickedButtonS3();
			break;
		case IDC_BUTTON_S4:
			pSys->OnBnClickedButtonS4();
			break;
		case IDC_BUTTON_END:
			pSys->OnBnClickedButtonEnd();
			break;
		}
		break;
	default:
		if( uMsg >= WM_USER ){
			pSys->WindowProc(uMsg, wParam, lParam);
		}
		break;
	}
	return FALSE;
}
