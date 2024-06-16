#include "stdafx.h"
#include "EpgTimerTask.h"
#include "../../Common/CommonDef.h"
#include "../../Common/SendCtrlCmd.h"
#include "../../Common/PathUtil.h"
#include "../../Common/StringUtil.h"
#include "../../Common/TimeUtil.h"
#include "resource.h"
#include <shellapi.h>
#include <commctrl.h>

#define EPG_TIMER_SERVICE_EXE L"EpgTimerSrv.exe"

namespace
{
enum {
	WM_APP_REQUEST_SHUTDOWN = WM_APP + 3,
	WM_APP_REQUEST_REBOOT,
	WM_APP_QUERY_SHUTDOWN,
	WM_APP_RECEIVE_NOTIFY,
	WM_APP_TRAY_PUSHICON,
};

enum {
	SD_MODE_INVALID,
	SD_MODE_STANDBY,
	SD_MODE_SUSPEND,
	SD_MODE_SHUTDOWN,
	SD_MODE_NONE,
};
}

CEpgTimerTask::CEpgTimerTask()
	: msgTaskbarCreated(RegisterWindowMessage(L"TaskbarCreated"))
	, queryShutdownContext((HWND)NULL, pair<BYTE, bool>())
	, notifySrvStatus(0)
{
}

bool CEpgTimerTask::Main()
{
	//非表示のメインウィンドウを作成
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpfnWndProc = MainWndProc;
	wc.hInstance = GetModuleHandle(NULL);
	wc.lpszClassName = SERVICE_NAME L" Task";
	wc.hIcon = (HICON)LoadImage(NULL, IDI_INFORMATION, IMAGE_ICON, 0, 0, LR_SHARED);
	if( RegisterClassEx(&wc) == 0 ){
		return false;
	}
	if( CreateWindowEx(0, wc.lpszClassName, wc.lpszClassName, 0, 0, 0, 0, 0, NULL, NULL, wc.hInstance, this) == NULL ){
		return false;
	}
	//メッセージループ
	MSG msg;
	while( GetMessage(&msg, NULL, 0, 0) > 0 ){
		if( this->queryShutdownContext.first == NULL || IsDialogMessage(this->queryShutdownContext.first, &msg) == FALSE ){
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return true;
}

LRESULT CALLBACK CEpgTimerTask::MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	enum {
		TIMER_RETRY_ADD_TRAY = 1,
	};

	CEpgTimerTask* ctx = (CEpgTimerTask*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	if( uMsg != WM_CREATE && ctx == NULL ){
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	switch( uMsg ){
	case WM_CREATE:
		{
			ctx = (CEpgTimerTask*)((LPCREATESTRUCT)lParam)->lpCreateParams;
			SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)ctx);
			wstring pipeName;
			Format(pipeName, L"%ls%d", CMD2_GUI_CTRL_PIPE, GetCurrentProcessId());
			ctx->pipeServer.StartServer(pipeName, [hwnd](CCmdStream& cmd, CCmdStream& res) {
				res.SetParam(CMD_ERR);
				switch( cmd.GetParam() ){
				case CMD2_TIMER_GUI_VIEW_EXECUTE:
					{
						wstring exeCmd;
						if( cmd.ReadVALUE(&exeCmd) && exeCmd.compare(0, 1, L"\"") == 0 ){
							//形式は("FileName")か("FileName" Arguments..)のどちらか。ほかは拒否してよい
							size_t i = exeCmd.find(L'"', 1);
							if( i >= 2 && (exeCmd.size() == i + 1 || exeCmd[i + 1] == L' ') ){
								wstring file(exeCmd, 1, i - 1);
								SHELLEXECUTEINFO sei = {};
								sei.cbSize = sizeof(sei);
								sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
								sei.lpFile = file.c_str();
								if( exeCmd.size() > i + 2 ){
									sei.lpParameters = exeCmd.erase(0, i + 2).c_str();
								}
								sei.nShow = UtilPathEndsWith(file.c_str(), L".bat") ? SW_SHOWMINNOACTIVE : SW_SHOWNORMAL;
								if( ShellExecuteEx(&sei) && sei.hProcess ){
									CPipeServer::GrantServerAccessToKernelObject(sei.hProcess, SYNCHRONIZE | PROCESS_TERMINATE | PROCESS_SET_INFORMATION);
									res.WriteVALUE(GetProcessId(sei.hProcess));
									res.SetParam(CMD_SUCCESS);
									CloseHandle(sei.hProcess);
								}
							}
						}
					}
					break;
				case CMD2_TIMER_GUI_QUERY_SUSPEND:
					{
						WORD val;
						if( cmd.ReadVALUE(&val) ){
							res.SetParam(CMD_SUCCESS);
							if( SD_MODE_STANDBY <= LOBYTE(val) && LOBYTE(val) <= SD_MODE_SHUTDOWN ){
								fs_path srvIniPath = GetCommonIniPath().replace_filename(EPG_TIMER_SERVICE_EXE).replace_extension(L".ini");
								if( GetPrivateProfileInt(L"NO_SUSPEND", L"NoUsePC", 0, srvIniPath.c_str()) != 0 ){
									DWORD noUsePCTime = GetPrivateProfileInt(L"NO_SUSPEND", L"NoUsePCTime", 3, srvIniPath.c_str());
									LASTINPUTINFO lii;
									lii.cbSize = sizeof(lii);
									if( noUsePCTime == 0 || (GetLastInputInfo(&lii) && GetU32Tick() - lii.dwTime < noUsePCTime * 60 * 1000) ){
										break;
									}
								}
								PostMessage(hwnd, WM_APP_QUERY_SHUTDOWN, LOBYTE(val), HIBYTE(val));
							}
						}
					}
					break;
				case CMD2_TIMER_GUI_QUERY_REBOOT:
					PostMessage(hwnd, WM_APP_QUERY_SHUTDOWN, SD_MODE_INVALID, TRUE);
					res.SetParam(CMD_SUCCESS);
					break;
				case CMD2_TIMER_GUI_SRV_STATUS_NOTIFY2:
					{
						WORD ver;
						NOTIFY_SRV_INFO status;
						if( cmd.ReadVALUE2WithVersion(&ver, &status) ){
							if( status.notifyID == NOTIFY_UPDATE_SRV_STATUS ){
								PostMessage(hwnd, WM_APP_RECEIVE_NOTIFY, FALSE, status.param1);
							}
							res.SetParam(CMD_SUCCESS);
						}
					}
					break;
				default:
					res.SetParam(CMD_NON_SUPPORT);
					break;
				}
			});
			CSendCtrlCmd cmd;
			for( int timeout = 0; cmd.SendRegistGUI(GetCurrentProcessId()) != CMD_SUCCESS; timeout += 100 ){
				SleepForMsec(100);
				if( timeout > CONNECT_TIMEOUT ){
					MessageBox(hwnd, L"サービスの起動を確認できませんでした。", NULL, MB_ICONERROR);
					PostMessage(hwnd, WM_CLOSE, 0, 0);
					break;
				}
			}
			PostMessage(hwnd, WM_APP_RECEIVE_NOTIFY, TRUE, 0);
		}
		return 0;
	case WM_DESTROY:
		{
			CSendCtrlCmd cmd;
			cmd.SendUnRegistGUI(GetCurrentProcessId());
			ctx->pipeServer.StopServer();
			//タスクトレイから削除
			NOTIFYICONDATA nid = {};
			nid.cbSize = NOTIFYICONDATA_V2_SIZE;
			nid.hWnd = hwnd;
			nid.uID = 1;
			Shell_NotifyIcon(NIM_DELETE, &nid);
			RemoveProp(hwnd, L"PopupSel");
			RemoveProp(hwnd, L"PopupSelData");
			PostQuitMessage(0);
		}
		return 0;
	case WM_APP_REQUEST_SHUTDOWN:
		{
			CSendCtrlCmd cmd;
			cmd.SendSuspend(MAKEWORD(wParam, lParam));
		}
		break;
	case WM_APP_REQUEST_REBOOT:
		{
			CSendCtrlCmd cmd;
			cmd.SendReboot();
		}
		break;
	case WM_APP_QUERY_SHUTDOWN:
		if( ctx->queryShutdownContext.first == NULL ){
			INITCOMMONCONTROLSEX icce;
			icce.dwSize = sizeof(icce);
			icce.dwICC = ICC_PROGRESS_CLASS;
			InitCommonControlsEx(&icce);
			ctx->queryShutdownContext.second.first = (BYTE)wParam;
			ctx->queryShutdownContext.second.second = lParam != FALSE;
			CreateDialogParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_EPGTIMERSRV_DIALOG), hwnd, QueryShutdownDlgProc, (LPARAM)&ctx->queryShutdownContext);
		}
		return TRUE;
	case WM_APP_RECEIVE_NOTIFY:
		//通知を受け取る
		{
			if( wParam == FALSE ){
				ctx->notifySrvStatus = (DWORD)lParam;
			}
			NOTIFYICONDATA nid = {};
			nid.cbSize = NOTIFYICONDATA_V2_SIZE;
			nid.hWnd = hwnd;
			nid.uID = 1;
			nid.hIcon = LoadSmallIcon(ctx->notifySrvStatus == 1 ? IDI_ICON_RED : ctx->notifySrvStatus == 2 ? IDI_ICON_GREEN : IDI_ICON_BLUE);
			nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
			nid.uCallbackMessage = WM_APP_TRAY_PUSHICON;
			if( Shell_NotifyIcon(NIM_MODIFY, &nid) == FALSE && Shell_NotifyIcon(NIM_ADD, &nid) == FALSE ){
				SetTimer(hwnd, TIMER_RETRY_ADD_TRAY, 5000, NULL);
			}
			if( nid.hIcon ){
				DestroyIcon(nid.hIcon);
			}
		}
		break;
	case WM_APP_TRAY_PUSHICON:
		//タスクトレイ関係
		switch( LOWORD(lParam) ){
		case WM_LBUTTONUP:
			OpenGUI();
			break;
		case WM_RBUTTONUP:
			{
				HMENU hMenu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_MENU_TRAY));
				if( hMenu ){
					POINT point;
					GetCursorPos(&point);
					SetForegroundWindow(hwnd);
					TrackPopupMenu(GetSubMenu(hMenu, 0), 0, point.x, point.y, 0, hwnd, NULL);
					DestroyMenu(hMenu);
				}
			}
			break;
		}
		break;
	case WM_TIMER:
		switch( wParam ){
		case TIMER_RETRY_ADD_TRAY:
			KillTimer(hwnd, TIMER_RETRY_ADD_TRAY);
			SendMessage(hwnd, WM_APP_RECEIVE_NOTIFY, TRUE, 0);
			break;
		}
		break;
	case WM_INITMENUPOPUP:
		if( GetMenuItemID((HMENU)wParam, 0) == IDC_MENU_RESERVE ){
			CSendCtrlCmd cmd;
			vector<RESERVE_DATA> list;
			cmd.SendEnumReserve(&list);
			InitReserveMenuPopup((HMENU)wParam, list);
			return 0;
		}
		break;
	case WM_MENUSELECT:
		if( lParam != 0 && (HIWORD(wParam) & MF_POPUP) == 0 ){
			MENUITEMINFO mii;
			mii.cbSize = sizeof(mii);
			mii.fMask = MIIM_ID | MIIM_DATA;
			if( GetMenuItemInfo((HMENU)lParam, LOWORD(wParam), FALSE, &mii) ){
				//WM_COMMANDでは取得できないので、ここで選択内容を記録する
				SetProp(hwnd, L"PopupSel", (HANDLE)(UINT_PTR)mii.wID);
				SetProp(hwnd, L"PopupSelData", (HANDLE)mii.dwItemData);
			}
		}
		break;
	case WM_COMMAND:
		switch( LOWORD(wParam) ){
		case IDC_BUTTON_SETTING:
			ShellExecute(NULL, NULL, GetModulePath().replace_filename(EPG_TIMER_SERVICE_EXE).c_str(), L"/setting", NULL, SW_SHOWNORMAL);
			break;
		case IDC_BUTTON_S3:
		case IDC_BUTTON_S4:
			{
				CSendCtrlCmd cmd;
				if(cmd.SendChkSuspend() == CMD_SUCCESS ){
					cmd.SendSuspend(LOWORD(wParam) == IDC_BUTTON_S3 ? 0xFF01 : 0xFF02);
				}else{
					MessageBox(hwnd, L"移行できる状態ではありません。\r\n（もうすぐ予約が始まる。または抑制条件のexeが起動している。など）", NULL, MB_ICONERROR);
				}
			}
			break;
		case IDC_BUTTON_END:
			if( MessageBox(hwnd, SERVICE_NAME L" (Task) を終了します（サービスは終了しません）。", L"確認", MB_OKCANCEL | MB_ICONINFORMATION) == IDOK ){
				SendMessage(hwnd, WM_CLOSE, 0, 0);
			}
			break;
		case IDC_BUTTON_GUI:
			OpenGUI();
			break;
		default:
			if( IDC_MENU_RESERVE <= LOWORD(wParam) && LOWORD(wParam) <= IDC_MENU_RESERVE_MAX ){
				//「予約削除」
				if( (UINT_PTR)GetProp(hwnd, L"PopupSel") == LOWORD(wParam) ){
					CSendCtrlCmd cmd;
					cmd.SendDelReserve(vector<DWORD>(1, (DWORD)(UINT_PTR)GetProp(hwnd, L"PopupSelData")));
				}
			}
			break;
		}
		break;
	default:
		if( uMsg == ctx->msgTaskbarCreated ){
			//シェルの再起動時
			SetTimer(hwnd, TIMER_RETRY_ADD_TRAY, 0, NULL);
		}
		break;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

INT_PTR CALLBACK CEpgTimerTask::QueryShutdownDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	pair<HWND, pair<BYTE, bool>>* ctx = (pair<HWND, pair<BYTE, bool>>*)GetWindowLongPtr(hDlg, GWLP_USERDATA);

	switch( uMsg ){
	case WM_INITDIALOG:
		ctx = (pair<HWND, pair<BYTE, bool>>*)lParam;
		SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)ctx);
		ctx->first = hDlg;
		SetDlgItemText(hDlg, IDC_STATIC_SHUTDOWN,
			ctx->second.first == SD_MODE_STANDBY ? L"スタンバイに移行します。" :
			ctx->second.first == SD_MODE_SUSPEND ? L"休止に移行します。" :
			ctx->second.first == SD_MODE_SHUTDOWN ? L"シャットダウンします。" : L"再起動します。");
		SetTimer(hDlg, 1, 1000, NULL);
		SendDlgItemMessage(hDlg, IDC_PROGRESS_SHUTDOWN, PBM_SETRANGE, 0, MAKELONG(0, ctx->second.first == SD_MODE_INVALID ? 30 : 15));
		SendDlgItemMessage(hDlg, IDC_PROGRESS_SHUTDOWN, PBM_SETPOS, ctx->second.first == SD_MODE_INVALID ? 30 : 15, 0);
		return TRUE;
	case WM_DESTROY:
		ctx->first = NULL;
		break;
	case WM_TIMER:
		if( SendDlgItemMessage(hDlg, IDC_PROGRESS_SHUTDOWN, PBM_SETPOS,
		    SendDlgItemMessage(hDlg, IDC_PROGRESS_SHUTDOWN, PBM_GETPOS, 0, 0) - 1, 0) <= 1 ){
			SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDOK, BN_CLICKED), (LPARAM)hDlg);
		}
		break;
	case WM_COMMAND:
		switch( LOWORD(wParam) ){
		case IDOK:
			if( ctx->second.first == SD_MODE_INVALID ){
				//再起動
				PostMessage(GetParent(hDlg), WM_APP_REQUEST_REBOOT, 0, 0);
			}else{
				//スタンバイ休止または電源断
				PostMessage(GetParent(hDlg), WM_APP_REQUEST_SHUTDOWN, ctx->second.first, ctx->second.second);
			}
			//FALL THROUGH!
		case IDCANCEL:
			DestroyWindow(hDlg);
			break;
		}
		break;
	}
	return FALSE;
}

HICON CEpgTimerTask::LoadSmallIcon(int iconID)
{
	HMODULE hModule = GetModuleHandle(L"comctl32.dll");
	if( hModule ){
		HICON hIcon;
		HRESULT (WINAPI* pfnLoadIconMetric)(HINSTANCE, PCWSTR, int, HICON*);
		if( UtilGetProcAddress(hModule, "LoadIconMetric", pfnLoadIconMetric) &&
		    pfnLoadIconMetric(GetModuleHandle(NULL), MAKEINTRESOURCE(iconID), LIM_SMALL, &hIcon) == S_OK ){
			return hIcon;
		}
	}
	return (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(iconID), IMAGE_ICON, 16, 16, 0);
}

void CEpgTimerTask::OpenGUI()
{
	if( UtilFileExists(GetModulePath().replace_filename(L"EpgTimer.lnk")).first ){
		//EpgTimer.lnk(ショートカット)を優先的に開く
		ShellExecute(NULL, NULL, GetModulePath().replace_filename(L"EpgTimer.lnk").c_str(), NULL, NULL, SW_SHOWNORMAL);
	}else{
		//EpgTimer.exeがあれば起動
		ShellExecute(NULL, NULL, GetModulePath().replace_filename(L"EpgTimer.exe").c_str(), NULL, NULL, SW_SHOWNORMAL);
	}
}

void CEpgTimerTask::InitReserveMenuPopup(HMENU hMenu, vector<RESERVE_DATA>& list)
{
	LONGLONG maxTime = GetNowI64Time() + 24 * 3600 * I64_1SEC;
	list.erase(std::remove_if(list.begin(), list.end(), [=](const RESERVE_DATA& a) {
		return a.recSetting.IsNoRec() || ConvertI64Time(a.startTime) > maxTime;
	}), list.end());
	std::sort(list.begin(), list.end(), [](const RESERVE_DATA& a, const RESERVE_DATA& b) {
		return ConvertI64Time(a.startTime) < ConvertI64Time(b.startTime);
	});
	while( GetMenuItemCount(hMenu) > 0 && DeleteMenu(hMenu, 0, MF_BYPOSITION) );
	if( list.empty() ){
		InsertMenu(hMenu, 0, MF_GRAYED | MF_BYPOSITION, IDC_MENU_RESERVE, L"(24時間以内に予約なし)");
	}
	for( UINT i = 0; i < list.size() && i <= IDC_MENU_RESERVE_MAX - IDC_MENU_RESERVE; i++ ){
		MENUITEMINFO mii;
		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_ID | MIIM_DATA | MIIM_STRING;
		mii.wID = IDC_MENU_RESERVE + i;
		mii.dwItemData = list[i].reserveID;
		SYSTEMTIME endTime;
		ConvertSystemTime(ConvertI64Time(list[i].startTime) + list[i].durationSecond * I64_1SEC, &endTime);
		WCHAR text[128];
		swprintf_s(text, L"%02d:%02d-%02d:%02d%ls %.31ls 【%.31ls】",
		           list[i].startTime.wHour, list[i].startTime.wMinute, endTime.wHour, endTime.wMinute,
		           list[i].recSetting.GetRecMode() == RECMODE_VIEW ? L"▲" : L"",
		           list[i].title.c_str(), list[i].stationName.c_str());
		std::replace(text, text + wcslen(text), L'　', L' ');
		std::replace(text, text + wcslen(text), L'&', L'＆');
		mii.dwTypeData = text;
		InsertMenuItem(hMenu, i, TRUE, &mii);
	}
}
