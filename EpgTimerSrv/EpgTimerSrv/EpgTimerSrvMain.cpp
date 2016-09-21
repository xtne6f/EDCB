#include "StdAfx.h"
#include "EpgTimerSrvMain.h"
#include "SyoboiCalUtil.h"
#include "UpnpSsdpServer.h"
#include "../../Common/PipeServer.h"
#include "../../Common/TCPServer.h"
#include "../../Common/SendCtrlCmd.h"
#include "../../Common/PathUtil.h"
#include "../../Common/TimeUtil.h"
#include "../../Common/BlockLock.h"
#include "resource.h"
#include <shellapi.h>
#include <tlhelp32.h>
#include <wincrypt.h>
#include <LM.h>
#pragma comment (lib, "netapi32.lib")

//互換動作のためのグローバルなフラグ(この手法は綺麗ではないが最もシンプルなので)
DWORD g_compatFlags;

static const char UPNP_URN_DMS_1[] = "urn:schemas-upnp-org:device:MediaServer:1";
static const char UPNP_URN_CDS_1[] = "urn:schemas-upnp-org:service:ContentDirectory:1";
static const char UPNP_URN_CMS_1[] = "urn:schemas-upnp-org:service:ConnectionManager:1";
static const char UPNP_URN_AVT_1[] = "urn:schemas-upnp-org:service:AVTransport:1";

enum {
	WM_RESET_SERVER = WM_APP,
	WM_RELOAD_EPG_CHK,
	WM_REQUEST_SHUTDOWN,
	WM_QUERY_SHUTDOWN,
	WM_RECEIVE_NOTIFY,
	WM_TRAY_PUSHICON,
	WM_SHOW_TRAY,
};

struct MAIN_WINDOW_CONTEXT {
	CEpgTimerSrvMain* const sys;
	const bool serviceFlag;
	const DWORD awayMode;
	const UINT msgTaskbarCreated;
	CPipeServer pipeServer;
	CTCPServer tcpServer;
	CHttpServer httpServer;
	CUpnpSsdpServer upnpServer;
	HANDLE resumeTimer;
	__int64 resumeTime;
	WORD shutdownModePending;
	DWORD shutdownPendingTick;
	HWND hDlgQueryShutdown;
	WORD queryShutdownMode;
	bool taskFlag;
	bool showBalloonTip;
	DWORD notifySrvStatus;
	__int64 notifyActiveTime;
	MAIN_WINDOW_CONTEXT(CEpgTimerSrvMain* sys_, bool serviceFlag_, DWORD awayMode_)
		: sys(sys_)
		, serviceFlag(serviceFlag_)
		, awayMode(awayMode_)
		, msgTaskbarCreated(RegisterWindowMessage(L"TaskbarCreated"))
		, resumeTimer(NULL)
		, shutdownModePending(0)
		, shutdownPendingTick(0)
		, hDlgQueryShutdown(NULL)
		, taskFlag(false)
		, showBalloonTip(false)
		, notifySrvStatus(0)
		, notifyActiveTime(LLONG_MAX) {}
};

CEpgTimerSrvMain::CEpgTimerSrvMain()
	: reserveManager(notifyManager, epgDB)
	, hwndMain(NULL)
	, nwtvUdp(false)
	, nwtvTcp(false)
{
	memset(this->notifyUpdateCount, 0, sizeof(this->notifyUpdateCount));
	InitializeCriticalSection(&this->settingLock);
}

CEpgTimerSrvMain::~CEpgTimerSrvMain()
{
	DeleteCriticalSection(&this->settingLock);
}

bool CEpgTimerSrvMain::Main(bool serviceFlag_)
{
	this->notifyManager.SetGUI(!serviceFlag_);
	this->residentFlag = serviceFlag_;

	wstring iniPath;
	GetModuleIniPath(iniPath);
	g_compatFlags = GetPrivateProfileInt(L"SET", L"CompatFlags", 4095, iniPath.c_str());

	DWORD awayMode;
	OSVERSIONINFOEX osvi;
	osvi.dwOSVersionInfoSize = sizeof(osvi);
	osvi.dwMajorVersion = 6;
	awayMode = VerifyVersionInfo(&osvi, VER_MAJORVERSION, VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL)) ? ES_AWAYMODE_REQUIRED : 0;

	wstring settingPath;
	GetSettingPath(settingPath);
	this->epgAutoAdd.ParseText((settingPath + L"\\" + EPG_AUTO_ADD_TEXT_NAME).c_str());
	this->manualAutoAdd.ParseText((settingPath + L"\\" + MANUAL_AUTO_ADD_TEXT_NAME).c_str());

	//非表示のメインウィンドウを作成
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpfnWndProc = MainWndProc;
	wc.hInstance = GetModuleHandle(NULL);
	wc.lpszClassName = SERVICE_NAME;
	wc.hIcon = (HICON)LoadImage(NULL, IDI_INFORMATION, IMAGE_ICON, 0, 0, LR_SHARED);
	if( RegisterClassEx(&wc) == 0 ){
		return false;
	}
	MAIN_WINDOW_CONTEXT ctx(this, serviceFlag_, awayMode);
	if( CreateWindowEx(0, SERVICE_NAME, SERVICE_NAME, 0, 0, 0, 0, 0, NULL, NULL, GetModuleHandle(NULL), &ctx) == NULL ){
		return false;
	}
	this->notifyManager.SetNotifyWindow(this->hwndMain, WM_RECEIVE_NOTIFY);

	//メッセージループ
	MSG msg;
	while( GetMessage(&msg, NULL, 0, 0) > 0 ){
		if( ctx.hDlgQueryShutdown == NULL || IsDialogMessage(ctx.hDlgQueryShutdown, &msg) == FALSE ){
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return true;
}

LRESULT CALLBACK CEpgTimerSrvMain::MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	enum {
		TIMER_RELOAD_EPG_CHK_PENDING = 1,
		TIMER_QUERY_SHUTDOWN_PENDING,
		TIMER_RETRY_ADD_TRAY,
		TIMER_SET_RESUME,
		TIMER_CHECK,
		TIMER_RESET_HTTP_SERVER,
	};

	MAIN_WINDOW_CONTEXT* ctx = (MAIN_WINDOW_CONTEXT*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	if( uMsg != WM_CREATE && ctx == NULL ){
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	switch( uMsg ){
	case WM_CREATE:
		ctx = (MAIN_WINDOW_CONTEXT*)((LPCREATESTRUCT)lParam)->lpCreateParams;
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)ctx);
		ctx->sys->hwndMain = hwnd;
		ctx->sys->reserveManager.Initialize();
		ctx->sys->ReloadSetting();
		ctx->sys->ReloadNetworkSetting();
		ctx->pipeServer.StartServer(CMD2_EPG_SRV_EVENT_WAIT_CONNECT, CMD2_EPG_SRV_PIPE, CtrlCmdPipeCallback, ctx->sys, ctx->serviceFlag);
		ctx->sys->epgDB.ReloadEpgData();
		SendMessage(hwnd, WM_RELOAD_EPG_CHK, 0, 0);
		SendMessage(hwnd, WM_TIMER, TIMER_SET_RESUME, 0);
		SetTimer(hwnd, TIMER_SET_RESUME, 30000, NULL);
		SetTimer(hwnd, TIMER_CHECK, 1000, NULL);
		OutputDebugString(L"*** Server initialized ***\r\n");
		return 0;
	case WM_DESTROY:
		if( ctx->resumeTimer ){
			CloseHandle(ctx->resumeTimer);
		}
		ctx->upnpServer.Stop();
		ctx->httpServer.StopServer();
		ctx->tcpServer.StopServer();
		ctx->pipeServer.StopServer();
		ctx->sys->reserveManager.Finalize();
		OutputDebugString(L"*** Server finalized ***\r\n");
		//タスクトレイから削除
		SendMessage(hwnd, WM_SHOW_TRAY, FALSE, FALSE);
		ctx->sys->hwndMain = NULL;
		SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
		PostQuitMessage(0);
		return 0;
	case WM_ENDSESSION:
		if( wParam ){
			DestroyWindow(hwnd);
		}
		return 0;
	case WM_RESET_SERVER:
		{
			//サーバリセット処理
			unsigned short tcpPort_;
			DWORD tcpResTo;
			wstring tcpAcl;
			{
				CBlockLock lock(&ctx->sys->settingLock);
				tcpPort_ = ctx->sys->tcpPort;
				tcpResTo = ctx->sys->tcpResponseTimeoutSec * 1000;
				tcpAcl = ctx->sys->tcpAccessControlList;
			}
			if( tcpPort_ == 0 ){
				ctx->tcpServer.StopServer();
			}else{
				ctx->tcpServer.StartServer(tcpPort_, tcpResTo ? tcpResTo : MAXDWORD, tcpAcl.c_str(), CtrlCmdTcpCallback, ctx->sys);
			}
			SetTimer(hwnd, TIMER_RESET_HTTP_SERVER, 200, NULL);
		}
		break;
	case WM_RELOAD_EPG_CHK:
		//EPGリロード完了のチェックを開始
		SetTimer(hwnd, TIMER_RELOAD_EPG_CHK_PENDING, 200, NULL);
		KillTimer(hwnd, TIMER_QUERY_SHUTDOWN_PENDING);
		ctx->shutdownPendingTick = GetTickCount();
		break;
	case WM_REQUEST_SHUTDOWN:
		//シャットダウン処理
		if( wParam == 0x01FF ){
			SetShutdown(4);
		}else if( ctx->sys->IsSuspendOK() ){
			if( LOBYTE(wParam) == 1 || LOBYTE(wParam) == 2 ){
				//ストリーミングを終了する
				ctx->sys->streamingManager.CloseAllFile();
				//スリープ抑止解除
				SetThreadExecutionState(ES_CONTINUOUS);
				//rebootFlag時は(指定+5分前)に復帰
				if( ctx->sys->SetResumeTimer(&ctx->resumeTimer, &ctx->resumeTime, ctx->sys->wakeMarginSec + (HIBYTE(wParam) != 0 ? 300 : 0)) ){
					SetShutdown(LOBYTE(wParam));
					if( HIBYTE(wParam) != 0 ){
						//再起動問い合わせ
						if( SendMessage(hwnd, WM_QUERY_SHUTDOWN, 0x0100, 0) == FALSE ){
							SetShutdown(4);
						}
					}
				}
			}else if( LOBYTE(wParam) == 3 ){
				SetShutdown(3);
			}
		}
		break;
	case WM_QUERY_SHUTDOWN:
		if( ctx->sys->notifyManager.IsGUI() ){
			//直接尋ねる
			if( ctx->hDlgQueryShutdown == NULL ){
				INITCOMMONCONTROLSEX icce;
				icce.dwSize = sizeof(icce);
				icce.dwICC = ICC_PROGRESS_CLASS;
				InitCommonControlsEx(&icce);
				ctx->queryShutdownMode = (WORD)wParam;
				CreateDialogParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_EPGTIMERSRV_DIALOG), hwnd, QueryShutdownDlgProc, (LPARAM)ctx);
			}
		}else if( ctx->sys->QueryShutdown(HIBYTE(wParam), LOBYTE(wParam)) == false ){
			//GUI経由で問い合わせ開始できなかった
			return FALSE;
		}
		return TRUE;
	case WM_RECEIVE_NOTIFY:
		//通知を受け取る
		{
			vector<NOTIFY_SRV_INFO> list(1);
			if( wParam ){
				//更新だけ
				list.back().notifyID = NOTIFY_UPDATE_SRV_STATUS;
				list.back().param1 = ctx->notifySrvStatus;
			}else{
				list = ctx->sys->notifyManager.RemoveSentList();
				ctx->tcpServer.NotifyUpdate();
			}
			for( vector<NOTIFY_SRV_INFO>::const_iterator itr = list.begin(); itr != list.end(); itr++ ){
				if( itr->notifyID == NOTIFY_UPDATE_SRV_STATUS ){
					ctx->notifySrvStatus = itr->param1;
					if( ctx->taskFlag ){
						NOTIFYICONDATA nid = {};
						nid.cbSize = NOTIFYICONDATA_V2_SIZE;
						nid.hWnd = hwnd;
						nid.uID = 1;
						int iconID = ctx->notifySrvStatus == 1 ? IDI_ICON_RED :
						             ctx->notifySrvStatus == 2 ? IDI_ICON_GREEN : IDI_ICON_BLUE;
						HRESULT (WINAPI* pfnLoadIconMetric)(HINSTANCE,PCWSTR,int,HICON*) =
							(HRESULT (WINAPI*)(HINSTANCE,PCWSTR,int,HICON*))GetProcAddress(GetModuleHandle(L"comctl32.dll"), "LoadIconMetric");
						if( pfnLoadIconMetric == NULL ||
						    pfnLoadIconMetric(GetModuleHandle(NULL), MAKEINTRESOURCE(iconID), LIM_SMALL, &nid.hIcon) != S_OK ){
							nid.hIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(iconID), IMAGE_ICON, 16, 16, 0);
						}
						if( ctx->notifyActiveTime != LLONG_MAX ){
							SYSTEMTIME st;
							ConvertSystemTime(ctx->notifyActiveTime + 30 * I64_1SEC, &st);
							swprintf_s(nid.szTip, L"次の予約・取得：%d/%d(%c) %d:%02d",
								st.wMonth, st.wDay, wstring(L"日月火水木金土").at(st.wDayOfWeek % 7), st.wHour, st.wMinute);
						}
						nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
						nid.uCallbackMessage = WM_TRAY_PUSHICON;
						if( Shell_NotifyIcon(NIM_MODIFY, &nid) == FALSE && Shell_NotifyIcon(NIM_ADD, &nid) == FALSE ){
							SetTimer(hwnd, TIMER_RETRY_ADD_TRAY, 5000, NULL);
						}
						if( nid.hIcon ){
							DestroyIcon(nid.hIcon);
						}
					}
				}else if( itr->notifyID < _countof(ctx->sys->notifyUpdateCount) ){
					//更新系の通知をカウント。書き込みがここだけかつDWORDなので排他はしない
					ctx->sys->notifyUpdateCount[itr->notifyID]++;
				}else{
					NOTIFYICONDATA nid = {};
					wcscpy_s(nid.szInfoTitle,
						itr->notifyID == NOTIFY_UPDATE_PRE_REC_START ? L"予約録画開始準備" :
						itr->notifyID == NOTIFY_UPDATE_REC_START ? L"録画開始" :
						itr->notifyID == NOTIFY_UPDATE_REC_END ? L"録画終了" :
						itr->notifyID == NOTIFY_UPDATE_REC_TUIJYU ? L"追従発生" :
						itr->notifyID == NOTIFY_UPDATE_CHG_TUIJYU ? L"番組変更" :
						itr->notifyID == NOTIFY_UPDATE_PRE_EPGCAP_START ? L"EPG取得" :
						itr->notifyID == NOTIFY_UPDATE_EPGCAP_START ? L"EPG取得" :
						itr->notifyID == NOTIFY_UPDATE_EPGCAP_END ? L"EPG取得" : L"");
					if( nid.szInfoTitle[0] ){
						wstring info = itr->notifyID == NOTIFY_UPDATE_EPGCAP_START ? wstring(L"開始") :
						               itr->notifyID == NOTIFY_UPDATE_EPGCAP_END ? wstring(L"終了") : itr->param4;
						if( ctx->sys->saveNotifyLog ){
							//通知情報ログ保存
							wstring logPath;
							GetModuleFolderPath(logPath);
							logPath += L"\\EpgTimerSrvNotifyLog.txt";
							HANDLE hFile = CreateFile(logPath.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
							if( hFile != INVALID_HANDLE_VALUE ){
								SYSTEMTIME st = itr->time;
								wstring log;
								Format(log, L"%d/%02d/%02d %02d:%02d:%02d.%03d [%s] %s",
									st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, nid.szInfoTitle, info.c_str());
								Replace(log, L"\r\n", L"  ");
								string logA;
								WtoA(log + L"\r\n", logA);
								LARGE_INTEGER liPos = {};
								SetFilePointerEx(hFile, liPos, NULL, FILE_END);
								DWORD dwWritten;
								WriteFile(hFile, logA.c_str(), (DWORD)logA.size(), &dwWritten, NULL);
								CloseHandle(hFile);
							}
						}
						if( ctx->showBalloonTip ){
							//バルーンチップ表示
							nid.cbSize = NOTIFYICONDATA_V2_SIZE;
							nid.hWnd = hwnd;
							nid.uID = 1;
							nid.uFlags = NIF_INFO;
							nid.dwInfoFlags = NIIF_INFO;
							nid.uTimeout = 10000; //効果はない
							if( info.size() > 63 ){
								info.resize(62);
								info += L'…';
							}
							Shell_NotifyIcon(NIM_MODIFY, &nid);
							wcscpy_s(nid.szInfo, info.c_str());
							Shell_NotifyIcon(NIM_MODIFY, &nid);
						}
					}
				}
			}
		}
		break;
	case WM_TRAY_PUSHICON:
		//タスクトレイ関係
		switch( LOWORD(lParam) ){
		case WM_LBUTTONUP:
			{
				wstring moduleFolder;
				GetModuleFolderPath(moduleFolder);
				if( GetFileAttributes((moduleFolder + L"\\EpgTimer.lnk").c_str()) != INVALID_FILE_ATTRIBUTES ){
					//EpgTimer.lnk(ショートカット)を優先的に開く
					ShellExecute(NULL, L"open", (moduleFolder + L"\\EpgTimer.lnk").c_str(), NULL, NULL, SW_SHOWNORMAL);
				}else{
					//EpgTimer.exeがあれば起動
					PROCESS_INFORMATION pi;
					STARTUPINFO si = {};
					si.cb = sizeof(si);
					if( CreateProcess((moduleFolder + L"\\EpgTimer.exe").c_str(), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi) != FALSE ){
						CloseHandle(pi.hThread);
						CloseHandle(pi.hProcess);
					}
				}
			}
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
	case WM_SHOW_TRAY:
		//タスクトレイに表示/非表示する
		if( ctx->taskFlag && wParam == FALSE ){
			NOTIFYICONDATA nid = {};
			nid.cbSize = NOTIFYICONDATA_V2_SIZE;
			nid.hWnd = hwnd;
			nid.uID = 1;
			Shell_NotifyIcon(NIM_DELETE, &nid);
		}
		ctx->taskFlag = wParam != FALSE;
		ctx->showBalloonTip = ctx->taskFlag && lParam;
		if( ctx->taskFlag ){
			SetTimer(hwnd, TIMER_RETRY_ADD_TRAY, 0, NULL);
		}
		return TRUE;
	case WM_TIMER:
		switch( wParam ){
		case TIMER_RELOAD_EPG_CHK_PENDING:
			if( GetTickCount() - ctx->shutdownPendingTick > 30000 ){
				//30秒以内にシャットダウン問い合わせできなければキャンセル
				if( ctx->shutdownModePending ){
					ctx->shutdownModePending = 0;
					OutputDebugString(L"Shutdown cancelled\r\n");
				}
			}
			if( ctx->sys->epgDB.IsLoadingData() == FALSE ){
				KillTimer(hwnd, TIMER_RELOAD_EPG_CHK_PENDING);
				if( ctx->shutdownModePending ){
					//このタイマはWM_TIMER以外でもKillTimer()するためメッセージキューに残った場合に対処するためシフト
					ctx->shutdownPendingTick -= 100000;
					SetTimer(hwnd, TIMER_QUERY_SHUTDOWN_PENDING, 200, NULL);
				}
				//リロード終わったので自動予約登録処理を行う
				ctx->sys->reserveManager.CheckTuijyu();
				bool addCountUpdated = false;
				bool addReserve = false;
				{
					CBlockLock lock(&ctx->sys->settingLock);
					for( map<DWORD, EPG_AUTO_ADD_DATA>::const_iterator itr = ctx->sys->epgAutoAdd.GetMap().begin(); itr != ctx->sys->epgAutoAdd.GetMap().end(); itr++ ){
						DWORD addCount = itr->second.addCount;
						addReserve |= ctx->sys->AutoAddReserveEPG(itr->second, true);
						if( addCount != itr->second.addCount ){
							addCountUpdated = true;
						}
					}
					for( map<DWORD, MANUAL_AUTO_ADD_DATA>::const_iterator itr = ctx->sys->manualAutoAdd.GetMap().begin(); itr != ctx->sys->manualAutoAdd.GetMap().end(); itr++ ){
						addReserve |= ctx->sys->AutoAddReserveProgram(itr->second, true);
					}
				}
				if( addCountUpdated ){
					//予約登録数の変化を通知する
					ctx->sys->notifyManager.AddNotify(NOTIFY_UPDATE_AUTOADD_EPG);
				}
				if( addReserve ){
					//予約情報の変化を通知する
					ctx->sys->reserveManager.AddNotifyAndPostBat(NOTIFY_UPDATE_RESERVE_INFO);
				}
				ctx->sys->reserveManager.AddNotifyAndPostBat(NOTIFY_UPDATE_EPGDATA);

				if( ctx->sys->useSyoboi ){
					//しょぼいカレンダー対応
					CSyoboiCalUtil syoboi;
					vector<RESERVE_DATA> reserveList = ctx->sys->reserveManager.GetReserveDataAll();
					vector<TUNER_RESERVE_INFO> tunerList = ctx->sys->reserveManager.GetTunerReserveAll();
					syoboi.SendReserve(&reserveList, &tunerList);
				}
			}
			break;
		case TIMER_QUERY_SHUTDOWN_PENDING:
			if( GetTickCount() - ctx->shutdownPendingTick >= 100000 ){
				if( GetTickCount() - ctx->shutdownPendingTick - 100000 > 30000 ){
					//30秒以内にシャットダウン問い合わせできなければキャンセル
					KillTimer(hwnd, TIMER_QUERY_SHUTDOWN_PENDING);
					if( ctx->shutdownModePending ){
						ctx->shutdownModePending = 0;
						OutputDebugString(L"Shutdown cancelled\r\n");
					}
				}else if( ctx->shutdownModePending && ctx->sys->IsSuspendOK() ){
					KillTimer(hwnd, TIMER_QUERY_SHUTDOWN_PENDING);
					if( ctx->sys->IsUserWorking() == false &&
					    1 <= LOBYTE(ctx->shutdownModePending) && LOBYTE(ctx->shutdownModePending) <= 3 ){
						//シャットダウン問い合わせ
						if( SendMessage(hwnd, WM_QUERY_SHUTDOWN, ctx->shutdownModePending, 0) == FALSE ){
							SendMessage(hwnd, WM_REQUEST_SHUTDOWN, ctx->shutdownModePending, 0);
						}
					}
					ctx->shutdownModePending = 0;
				}
			}
			break;
		case TIMER_RETRY_ADD_TRAY:
			KillTimer(hwnd, TIMER_RETRY_ADD_TRAY);
			SendMessage(hwnd, WM_RECEIVE_NOTIFY, TRUE, 0);
			break;
		case TIMER_SET_RESUME:
			{
				//復帰タイマ更新(powercfg /waketimersでデバッグ可能)
				ctx->sys->SetResumeTimer(&ctx->resumeTimer, &ctx->resumeTime, ctx->sys->wakeMarginSec);
				//スリープ抑止
				EXECUTION_STATE esFlags = ctx->shutdownModePending == 0 && ctx->sys->IsSuspendOK() ? ES_CONTINUOUS : ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ctx->awayMode;
				if( SetThreadExecutionState(esFlags) != esFlags ){
					_OutputDebugString(L"SetThreadExecutionState(0x%08x)\r\n", (DWORD)esFlags);
				}
				//チップヘルプの更新が必要かチェック
				__int64 activeTime = ctx->sys->reserveManager.GetSleepReturnTime(GetNowI64Time());
				if( activeTime != ctx->notifyActiveTime ){
					ctx->notifyActiveTime = activeTime;
					SetTimer(hwnd, TIMER_RETRY_ADD_TRAY, 0, NULL);
				}
			}
			break;
		case TIMER_CHECK:
			{
				DWORD ret = ctx->sys->reserveManager.Check();
				switch( HIWORD(ret) ){
				case CReserveManager::CHECK_EPGCAP_END:
					if( ctx->sys->epgDB.ReloadEpgData() != FALSE ){
						//EPGリロード完了後にデフォルトのシャットダウン動作を試みる
						SendMessage(hwnd, WM_RELOAD_EPG_CHK, 0, 0);
						ctx->shutdownModePending = ctx->sys->defShutdownMode;
					}
					SendMessage(hwnd, WM_TIMER, TIMER_SET_RESUME, 0);
					break;
				case CReserveManager::CHECK_NEED_SHUTDOWN:
					if( ctx->sys->epgDB.ReloadEpgData() != FALSE ){
						//EPGリロード完了後に要求されたシャットダウン動作を試みる
						SendMessage(hwnd, WM_RELOAD_EPG_CHK, 0, 0);
						ctx->shutdownModePending = LOWORD(ret);
						if( LOBYTE(ctx->shutdownModePending) == 0 ){
							ctx->shutdownModePending = ctx->sys->defShutdownMode;
						}
					}
					SendMessage(hwnd, WM_TIMER, TIMER_SET_RESUME, 0);
					break;
				case CReserveManager::CHECK_RESERVE_MODIFIED:
					SendMessage(hwnd, WM_TIMER, TIMER_SET_RESUME, 0);
					break;
				}
			}
			break;
		case TIMER_RESET_HTTP_SERVER:
			ctx->upnpServer.Stop();
			if( ctx->httpServer.StopServer(true) ){
				KillTimer(hwnd, TIMER_RESET_HTTP_SERVER);
				CHttpServer::SERVER_OPTIONS op;
				bool enableSsdpServer_;
				{
					CBlockLock lock(&ctx->sys->settingLock);
					op = ctx->sys->httpOptions;
					enableSsdpServer_ = ctx->sys->enableSsdpServer;
				}
				if( op.ports.empty() == false && ctx->sys->httpServerRandom.empty() ){
					HCRYPTPROV prov;
					if( CryptAcquireContext(&prov, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT) ){
						unsigned __int64 r[4] = {};
						if( CryptGenRandom(prov, sizeof(r), (BYTE*)r) ){
							Format(ctx->sys->httpServerRandom, "%016I64x%016I64x%016I64x%016I64x", r[0], r[1], r[2], r[3]);
						}
						CryptReleaseContext(prov, 0);
					}
				}
				if( op.ports.empty() == false &&
				    ctx->sys->httpServerRandom.empty() == false &&
				    ctx->httpServer.StartServer(op, InitLuaCallback, ctx->sys) &&
				    enableSsdpServer_ ){
					//"ddd.xml"の先頭から2KB以内に"<UDN>uuid:{UUID}</UDN>"が必要
					char dddBuf[2048] = {};
					HANDLE hFile = CreateFile((op.rootPath + L"\\dlna\\dms\\ddd.xml").c_str(),
					                          GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
					if( hFile != INVALID_HANDLE_VALUE ){
						DWORD dwRead;
						ReadFile(hFile, dddBuf, sizeof(dddBuf) - 1, &dwRead, NULL);
						CloseHandle(hFile);
					}
					string dddStr = dddBuf;
					size_t udnFrom = dddStr.find("<UDN>uuid:");
					if( udnFrom != string::npos && dddStr.size() > udnFrom + 10 + 36 && dddStr.compare(udnFrom + 10 + 36, 6, "</UDN>") == 0 ){
						string notifyUuid(dddStr, udnFrom + 5, 41);
						//最後にみつかった':'より後ろか先頭を_wtoiした結果を通知ポートとする
						unsigned short notifyPort = (unsigned short)_wtoi(op.ports.c_str() +
							(op.ports.find_last_of(':') == wstring::npos ? 0 : op.ports.find_last_of(':') + 1));
						//UPnPのUDP(Port1900)部分を担当するサーバ
						LPCSTR targetArray[] = { "upnp:rootdevice", UPNP_URN_DMS_1, UPNP_URN_CDS_1, UPNP_URN_CMS_1, UPNP_URN_AVT_1 };
						vector<CUpnpSsdpServer::SSDP_TARGET_INFO> targetList(2 + _countof(targetArray));
						targetList[0].target = notifyUuid;
						Format(targetList[0].location, "http://$HOST$:%d/dlna/dms/ddd.xml", notifyPort);
						targetList[0].usn = targetList[0].target;
						targetList[0].notifyFlag = true;
						targetList[1].target = "ssdp:all";
						targetList[1].location = targetList[0].location;
						targetList[1].usn = notifyUuid + "::" + "upnp:rootdevice";
						targetList[1].notifyFlag = false;
						for( size_t i = 2; i < targetList.size(); i++ ){
							targetList[i].target = targetArray[i - 2];
							targetList[i].location = targetList[0].location;
							targetList[i].usn = notifyUuid + "::" + targetList[i].target;
							targetList[i].notifyFlag = true;
						}
						ctx->upnpServer.Start(targetList);
					}else{
						OutputDebugString(L"Invalid ddd.xml\r\n");
					}
				}
			}
			break;
		}
		break;
	case WM_COMMAND:
		switch( LOWORD(wParam) ){
		case IDC_BUTTON_S3:
		case IDC_BUTTON_S4:
			if( ctx->sys->IsSuspendOK() ){
				PostMessage(hwnd, WM_REQUEST_SHUTDOWN, MAKEWORD(LOWORD(wParam) == IDC_BUTTON_S3 ? 1 : 2, HIBYTE(ctx->sys->defShutdownMode)), 0);
			}else{
				MessageBox(hwnd, L"移行できる状態ではありません。\r\n（もうすぐ予約が始まる。または抑制条件のexeが起動している。など）", NULL, MB_ICONERROR);
			}
			break;
		case IDC_BUTTON_END:
			if( MessageBox(hwnd, SERVICE_NAME L" を終了します。", L"確認", MB_OKCANCEL | MB_ICONINFORMATION) == IDOK ){
				SendMessage(hwnd, WM_CLOSE, 0, 0);
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

INT_PTR CALLBACK CEpgTimerSrvMain::QueryShutdownDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	MAIN_WINDOW_CONTEXT* ctx = (MAIN_WINDOW_CONTEXT*)GetWindowLongPtr(hDlg, GWLP_USERDATA);

	switch( uMsg ){
	case WM_INITDIALOG:
		ctx = (MAIN_WINDOW_CONTEXT*)lParam;
		SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)ctx);
		ctx->hDlgQueryShutdown = hDlg;
		SetDlgItemText(hDlg, IDC_STATIC_SHUTDOWN,
			LOBYTE(ctx->queryShutdownMode) == 1 ? L"スタンバイに移行します。" :
			LOBYTE(ctx->queryShutdownMode) == 2 ? L"休止に移行します。" :
			LOBYTE(ctx->queryShutdownMode) == 3 ? L"シャットダウンします。" : L"再起動します。");
		SetTimer(hDlg, 1, 1000, NULL);
		SendDlgItemMessage(hDlg, IDC_PROGRESS_SHUTDOWN, PBM_SETRANGE, 0, MAKELONG(0, LOBYTE(ctx->queryShutdownMode) == 0 ? 30 : 15));
		SendDlgItemMessage(hDlg, IDC_PROGRESS_SHUTDOWN, PBM_SETPOS, LOBYTE(ctx->queryShutdownMode) == 0 ? 30 : 15, 0);
		return TRUE;
	case WM_DESTROY:
		ctx->hDlgQueryShutdown = NULL;
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
			if( LOBYTE(ctx->queryShutdownMode) == 0 ){
				//再起動
				PostMessage(ctx->sys->hwndMain, WM_REQUEST_SHUTDOWN, 0x01FF, 0);
			}else if( ctx->sys->IsSuspendOK() ){
				//スタンバイ休止または電源断
				PostMessage(ctx->sys->hwndMain, WM_REQUEST_SHUTDOWN, HIBYTE(ctx->queryShutdownMode) == 0xFF ?
					MAKEWORD(LOBYTE(ctx->queryShutdownMode), HIBYTE(ctx->sys->defShutdownMode)) : ctx->queryShutdownMode, 0);
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

void CEpgTimerSrvMain::StopMain()
{
	volatile HWND hwndMain_ = this->hwndMain;
	if( hwndMain_ ){
		SendNotifyMessage(hwndMain_, WM_CLOSE, 0, 0);
	}
}

bool CEpgTimerSrvMain::IsSuspendOK()
{
	DWORD marginSec;
	bool ngFileStreaming_;
	{
		CBlockLock lock(&this->settingLock);
		//rebootFlag時の復帰マージンを基準に3分余裕を加えたものと抑制条件のどちらか大きいほう
		marginSec = max(this->wakeMarginSec + 300 + 180, this->noStandbySec);
		ngFileStreaming_ = this->ngFileStreaming;
	}
	__int64 now = GetNowI64Time();
	//シャットダウン可能で復帰が間に合うときだけ
	return (ngFileStreaming_ == false || this->streamingManager.IsStreaming() == FALSE) &&
	       this->reserveManager.IsActive() == false &&
	       this->reserveManager.GetSleepReturnTime(now) > now + marginSec * I64_1SEC &&
	       IsFindNoSuspendExe() == false &&
	       IsFindShareTSFile() == false;
}

void CEpgTimerSrvMain::ReloadNetworkSetting()
{
	CBlockLock lock(&this->settingLock);

	wstring iniPath;
	GetModuleIniPath(iniPath);
	this->tcpPort = 0;
	if( GetPrivateProfileInt(L"SET", L"EnableTCPSrv", 0, iniPath.c_str()) != 0 ){
		this->tcpAccessControlList = GetPrivateProfileToString(L"SET", L"TCPAccessControlList", L"+127.0.0.1,+192.168.0.0/16", iniPath.c_str());
		this->tcpResponseTimeoutSec = GetPrivateProfileInt(L"SET", L"TCPResponseTimeoutSec", 120, iniPath.c_str());
		this->tcpPort = (unsigned short)GetPrivateProfileInt(L"SET", L"TCPPort", 4510, iniPath.c_str());
	}
	this->httpOptions.ports.clear();
	int enableHttpSrv = GetPrivateProfileInt(L"SET", L"EnableHttpSrv", 0, iniPath.c_str());
	if( enableHttpSrv != 0 ){
		this->httpOptions.rootPath = GetPrivateProfileToString(L"SET", L"HttpPublicFolder", L"", iniPath.c_str());
		if(this->httpOptions.rootPath.empty() ){
			GetModuleFolderPath(this->httpOptions.rootPath);
			this->httpOptions.rootPath += L"\\HttpPublic";
		}
		ChkFolderPath(this->httpOptions.rootPath);
		if( this->dmsPublicFileList.empty() || CompareNoCase(this->httpOptions.rootPath, this->dmsPublicFileList[0].second) != 0 ){
			//公開フォルダの場所が変わったのでクリア
			this->dmsPublicFileList.clear();
		}
		this->httpOptions.accessControlList = GetPrivateProfileToString(L"SET", L"HttpAccessControlList", L"+127.0.0.1", iniPath.c_str());
		this->httpOptions.authenticationDomain = GetPrivateProfileToString(L"SET", L"HttpAuthenticationDomain", L"", iniPath.c_str());
		this->httpOptions.numThreads = GetPrivateProfileInt(L"SET", L"HttpNumThreads", 5, iniPath.c_str());
		this->httpOptions.requestTimeout = GetPrivateProfileInt(L"SET", L"HttpRequestTimeoutSec", 120, iniPath.c_str()) * 1000;
		this->httpOptions.sslCipherList = GetPrivateProfileToString(L"SET", L"HttpSslCipherList", L"HIGH:!aNULL:!MD5", iniPath.c_str());
		this->httpOptions.sslProtocolVersion = GetPrivateProfileInt(L"SET", L"HttpSslProtocolVersion", 2, iniPath.c_str());
		this->httpOptions.keepAlive = GetPrivateProfileInt(L"SET", L"HttpKeepAlive", 0, iniPath.c_str()) != 0;
		this->httpOptions.ports = GetPrivateProfileToString(L"SET", L"HttpPort", L"5510", iniPath.c_str());
		this->httpOptions.saveLog = enableHttpSrv == 2;
	}
	this->enableSsdpServer = GetPrivateProfileInt(L"SET", L"EnableDMS", 0, iniPath.c_str()) != 0;

	PostMessage(this->hwndMain, WM_RESET_SERVER, 0, 0);
}

void CEpgTimerSrvMain::ReloadSetting()
{
	this->reserveManager.ReloadSetting();

	CBlockLock lock(&this->settingLock);

	wstring iniPath;
	GetModuleIniPath(iniPath);
	if( this->residentFlag == false ){
		int residentMode = GetPrivateProfileInt(L"SET", L"ResidentMode", 0, iniPath.c_str());
		if( residentMode >= 1 ){
			//常駐する(CMD2_EPG_SRV_CLOSEを無視)
			this->residentFlag = true;
			//タスクトレイに表示するかどうか
			PostMessage(this->hwndMain, WM_SHOW_TRAY, residentMode >= 2,
				GetPrivateProfileInt(L"SET", L"NoBalloonTip", 0, iniPath.c_str()) == 0);
		}
	}
	this->saveNotifyLog = GetPrivateProfileInt(L"SET", L"SaveNotifyLog", 0, iniPath.c_str()) != 0;
	this->wakeMarginSec = GetPrivateProfileInt(L"SET", L"WakeTime", 5, iniPath.c_str()) * 60;
	this->autoAddHour = GetPrivateProfileInt(L"SET", L"AutoAddDays", 8, iniPath.c_str()) * 24 +
	                    GetPrivateProfileInt(L"SET", L"AutoAddHour", 0, iniPath.c_str());
	this->chkGroupEvent = GetPrivateProfileInt(L"SET", L"ChkGroupEvent", 1, iniPath.c_str()) != 0;
	this->defShutdownMode = MAKEWORD((GetPrivateProfileInt(L"SET", L"RecEndMode", 0, iniPath.c_str()) + 3) % 4 + 1,
	                                 (GetPrivateProfileInt(L"SET", L"Reboot", 0, iniPath.c_str()) != 0 ? 1 : 0));
	this->ngUsePCTime = 0;
	if( GetPrivateProfileInt(L"NO_SUSPEND", L"NoUsePC", 0, iniPath.c_str()) != 0 ){
		this->ngUsePCTime = GetPrivateProfileInt(L"NO_SUSPEND", L"NoUsePCTime", 3, iniPath.c_str()) * 60 * 1000;
		//閾値が0のときは常に使用中扱い
		if( this->ngUsePCTime == 0 ){
			this->ngUsePCTime = MAXDWORD;
		}
	}
	this->ngFileStreaming = GetPrivateProfileInt(L"NO_SUSPEND", L"NoFileStreaming", 0, iniPath.c_str()) != 0;
	this->ngShareFile = GetPrivateProfileInt(L"NO_SUSPEND", L"NoShareFile", 0, iniPath.c_str()) != 0;
	this->noStandbySec = GetPrivateProfileInt(L"NO_SUSPEND", L"NoStandbyTime", 10, iniPath.c_str()) * 60;
	this->useSyoboi = GetPrivateProfileInt(L"SYOBOI", L"use", 0, iniPath.c_str()) != 0;

	this->noSuspendExeList.clear();
	int count = GetPrivateProfileInt(L"NO_SUSPEND", L"Count", 0, iniPath.c_str());
	if( count == 0 ){
		this->noSuspendExeList.push_back(L"EpgDataCap_Bon.exe");
	}
	for( int i = 0; i < count; i++ ){
		WCHAR key[16];
		wsprintf(key, L"%d", i);
		wstring buff = GetPrivateProfileToString(L"NO_SUSPEND", key, L"", iniPath.c_str());
		if( buff.empty() == false ){
			this->noSuspendExeList.push_back(buff);
		}
	}

	this->tvtestUseBon.clear();
	count = GetPrivateProfileInt(L"TVTEST", L"Num", 0, iniPath.c_str());
	for( int i = 0; i < count; i++ ){
		WCHAR key[16];
		wsprintf(key, L"%d", i);
		wstring buff = GetPrivateProfileToString(L"TVTEST", key, L"", iniPath.c_str());
		if( buff.empty() == false ){
			this->tvtestUseBon.push_back(buff);
		}
	}
}

pair<wstring, REC_SETTING_DATA> CEpgTimerSrvMain::LoadRecSetData(WORD preset) const
{
	wstring iniPath;
	GetModuleIniPath(iniPath);
	WCHAR defIndex[32];
	WCHAR defName[32];
	WCHAR defFolderName[2][32];
	defIndex[preset == 0 ? 0 : wsprintf(defIndex, L"%d", preset)] = L'\0';
	wsprintf(defName, L"REC_DEF%s", defIndex);
	wsprintf(defFolderName[0], L"REC_DEF_FOLDER%s", defIndex);
	wsprintf(defFolderName[1], L"REC_DEF_FOLDER_1SEG%s", defIndex);

	pair<wstring, REC_SETTING_DATA> ret;
	ret.first = preset == 0 ? wstring(L"デフォルト") : GetPrivateProfileToString(defName, L"SetName", L"", iniPath.c_str());
	REC_SETTING_DATA& rs = ret.second;
	rs.recMode = (BYTE)GetPrivateProfileInt(defName, L"RecMode", 1, iniPath.c_str());
	rs.priority = (BYTE)GetPrivateProfileInt(defName, L"Priority", 2, iniPath.c_str());
	rs.tuijyuuFlag = (BYTE)GetPrivateProfileInt(defName, L"TuijyuuFlag", 1, iniPath.c_str());
	rs.serviceMode = (BYTE)GetPrivateProfileInt(defName, L"ServiceMode", 0, iniPath.c_str());
	rs.pittariFlag = (BYTE)GetPrivateProfileInt(defName, L"PittariFlag", 0, iniPath.c_str());
	rs.batFilePath = GetPrivateProfileToString(defName, L"BatFilePath", L"", iniPath.c_str());
	for( int i = 0; i < 2; i++ ){
		vector<REC_FILE_SET_INFO>& recFolderList = i == 0 ? rs.recFolderList : rs.partialRecFolder;
		int count = GetPrivateProfileInt(defFolderName[i], L"Count", 0, iniPath.c_str());
		for( int j = 0; j < count; j++ ){
			recFolderList.resize(j + 1);
			WCHAR key[32];
			wsprintf(key, L"%d", j);
			recFolderList[j].recFolder = GetPrivateProfileToString(defFolderName[i], key, L"", iniPath.c_str());
			wsprintf(key, L"WritePlugIn%d", j);
			recFolderList[j].writePlugIn = GetPrivateProfileToString(defFolderName[i], key, L"Write_Default.dll", iniPath.c_str());
			wsprintf(key, L"RecNamePlugIn%d", j);
			recFolderList[j].recNamePlugIn = GetPrivateProfileToString(defFolderName[i], key, L"", iniPath.c_str());
		}
	}
	rs.suspendMode = (BYTE)GetPrivateProfileInt(defName, L"SuspendMode", 0, iniPath.c_str());
	rs.rebootFlag = (BYTE)GetPrivateProfileInt(defName, L"RebootFlag", 0, iniPath.c_str());
	rs.useMargineFlag = (BYTE)GetPrivateProfileInt(defName, L"UseMargineFlag", 0, iniPath.c_str());
	rs.startMargine = GetPrivateProfileInt(defName, L"StartMargine", 0, iniPath.c_str());
	rs.endMargine = GetPrivateProfileInt(defName, L"EndMargine", 0, iniPath.c_str());
	rs.continueRecFlag = (BYTE)GetPrivateProfileInt(defName, L"ContinueRec", 0, iniPath.c_str());
	rs.partialRecFlag = (BYTE)GetPrivateProfileInt(defName, L"PartialRec", 0, iniPath.c_str());
	rs.tunerID = GetPrivateProfileInt(defName, L"TunerID", 0, iniPath.c_str());
	return ret;
}

bool CEpgTimerSrvMain::SetResumeTimer(HANDLE* resumeTimer, __int64* resumeTime, DWORD marginSec)
{
	__int64 returnTime = this->reserveManager.GetSleepReturnTime(GetNowI64Time() + marginSec * I64_1SEC);
	if( returnTime == LLONG_MAX ){
		if( *resumeTimer != NULL ){
			CloseHandle(*resumeTimer);
			*resumeTimer = NULL;
		}
		return true;
	}
	__int64 setTime = returnTime - marginSec * I64_1SEC;
	if( *resumeTimer != NULL && *resumeTime == setTime ){
		//同時刻でセット済み
		return true;
	}
	if( *resumeTimer == NULL ){
		*resumeTimer = CreateWaitableTimer(NULL, FALSE, NULL);
	}
	if( *resumeTimer != NULL ){
		FILETIME locTime;
		locTime.dwLowDateTime = (DWORD)setTime;
		locTime.dwHighDateTime = (DWORD)(setTime >> 32);
		FILETIME utcTime = {};
		LocalFileTimeToFileTime(&locTime, &utcTime);
		LARGE_INTEGER liTime;
		liTime.QuadPart = (LONGLONG)utcTime.dwHighDateTime << 32 | utcTime.dwLowDateTime;
		if( SetWaitableTimer(*resumeTimer, &liTime, 0, NULL, NULL, TRUE) != FALSE ){
			*resumeTime = setTime;
			return true;
		}
		CloseHandle(*resumeTimer);
		*resumeTimer = NULL;
	}
	return false;
}

void CEpgTimerSrvMain::SetShutdown(BYTE shutdownMode)
{
	HANDLE hToken;
	if ( OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken) ){
		TOKEN_PRIVILEGES tokenPriv;
		LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tokenPriv.Privileges[0].Luid);
		tokenPriv.PrivilegeCount = 1;
		tokenPriv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		AdjustTokenPrivileges(hToken, FALSE, &tokenPriv, 0, NULL, NULL);
		CloseHandle(hToken);
	}
	if( shutdownMode == 1 ){
		//スタンバイ(同期)
		SetSystemPowerState(TRUE, FALSE);
	}else if( shutdownMode == 2 ){
		//休止(同期)
		SetSystemPowerState(FALSE, FALSE);
	}else if( shutdownMode == 3 ){
		//電源断(非同期)
		ExitWindowsEx(EWX_POWEROFF, 0);
	}else if( shutdownMode == 4 ){
		//再起動(非同期)
		ExitWindowsEx(EWX_REBOOT, 0);
	}
}

bool CEpgTimerSrvMain::QueryShutdown(BYTE rebootFlag, BYTE suspendMode)
{
	CSendCtrlCmd ctrlCmd;
	vector<DWORD> registGUI = this->notifyManager.GetRegistGUI();
	for( size_t i = 0; i < registGUI.size(); i++ ){
		ctrlCmd.SetPipeSetting(CMD2_GUI_CTRL_WAIT_CONNECT, CMD2_GUI_CTRL_PIPE, registGUI[i]);
		//通信できる限り常に成功するので、重複問い合わせを考慮する必要はない
		if( suspendMode == 0 && ctrlCmd.SendGUIQueryReboot(rebootFlag) == CMD_SUCCESS ||
		    suspendMode != 0 && ctrlCmd.SendGUIQuerySuspend(rebootFlag, suspendMode) == CMD_SUCCESS ){
			return true;
		}
	}
	return false;
}

bool CEpgTimerSrvMain::IsUserWorking() const
{
	CBlockLock lock(&this->settingLock);

	//最終入力時刻取得
	LASTINPUTINFO lii;
	lii.cbSize = sizeof(LASTINPUTINFO);
	return this->ngUsePCTime == MAXDWORD || this->ngUsePCTime && GetLastInputInfo(&lii) && GetTickCount() - lii.dwTime < this->ngUsePCTime;
}

bool CEpgTimerSrvMain::IsFindShareTSFile() const
{
	bool found = false;
	FILE_INFO_3* fileInfo;
	DWORD entriesread;
	DWORD totalentries;
	if( this->ngShareFile && NetFileEnum(NULL, NULL, NULL, 3, (LPBYTE*)&fileInfo, MAX_PREFERRED_LENGTH, &entriesread, &totalentries, NULL) == NERR_Success ){
		for( DWORD i = 0; i < entriesread; i++ ){
			if( IsExt(fileInfo[i].fi3_pathname, L".ts") != FALSE ){
				OutputDebugString(L"共有フォルダTSアクセス\r\n");
				found = true;
				break;
			}
		}
		NetApiBufferFree(fileInfo);
	}
	return found;
}

bool CEpgTimerSrvMain::IsFindNoSuspendExe() const
{
	CBlockLock lock(&this->settingLock);

	if( this->noSuspendExeList.empty() == false ){
		//Toolhelpスナップショットを作成する
		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if( hSnapshot != INVALID_HANDLE_VALUE ){
			bool found = false;
			PROCESSENTRY32 procent;
			procent.dwSize = sizeof(PROCESSENTRY32);
			if( Process32First(hSnapshot, &procent) != FALSE ){
				do{
					for( size_t i = 0; i < this->noSuspendExeList.size(); i++ ){
						//procent.szExeFileにプロセス名
						wstring strExe = wstring(procent.szExeFile).substr(0, this->noSuspendExeList[i].size());
						if( CompareNoCase(strExe, this->noSuspendExeList[i]) == 0 ){
							_OutputDebugString(L"起動exe:%s\r\n", procent.szExeFile);
							found = true;
							break;
						}
					}
				}while( found == false && Process32Next(hSnapshot, &procent) != FALSE );
			}
			CloseHandle(hSnapshot);
			return found;
		}
	}
	return false;
}

bool CEpgTimerSrvMain::AutoAddReserveEPG(const EPG_AUTO_ADD_DATA& data, bool noReportNotify)
{
	bool modified = false;
	vector<RESERVE_DATA> setList;
	int addCount = 0;
	int autoAddHour_;
	bool chkGroupEvent_;
	{
		CBlockLock lock(&this->settingLock);
		autoAddHour_ = this->autoAddHour;
		chkGroupEvent_ = this->chkGroupEvent;
	}
	__int64 now = GetNowI64Time();

	vector<CEpgDBManager::SEARCH_RESULT_EVENT_DATA> resultList;
	vector<EPGDB_SEARCH_KEY_INFO> key(1, data.searchInfo);
	this->epgDB.SearchEpg(&key, &resultList);
	for( size_t i = 0; i < resultList.size(); i++ ){
		const EPGDB_EVENT_INFO& info = resultList[i].info;
		//時間未定でなく対象期間内かどうか
		if( info.StartTimeFlag != 0 && info.DurationFlag != 0 &&
		    now < ConvertI64Time(info.start_time) && ConvertI64Time(info.start_time) < now + autoAddHour_ * 60 * 60 * I64_1SEC ){
			addCount++;
			if( this->reserveManager.IsFindReserve(info.original_network_id, info.transport_stream_id, info.service_id, info.event_id) == false ){
				bool found = false;
				if( info.eventGroupInfo != NULL && chkGroupEvent_ ){
					//イベントグループのチェックをする
					for( size_t j = 0; found == false && j < info.eventGroupInfo->eventDataList.size(); j++ ){
						//group_typeは必ず1(イベント共有)
						const EPGDB_EVENT_DATA& e = info.eventGroupInfo->eventDataList[j];
						if( this->reserveManager.IsFindReserve(e.original_network_id, e.transport_stream_id, e.service_id, e.event_id) ){
							found = true;
							break;
						}
						//追加前予約のチェックをする
						for( size_t k = 0; k < setList.size(); k++ ){
							if( setList[k].originalNetworkID == e.original_network_id &&
							    setList[k].transportStreamID == e.transport_stream_id &&
							    setList[k].serviceID == e.service_id &&
							    setList[k].eventID == e.event_id ){
								found = true;
								break;
							}
						}
					}
				}
				//追加前予約のチェックをする
				for( size_t j = 0; found == false && j < setList.size(); j++ ){
					if( setList[j].originalNetworkID == info.original_network_id &&
					    setList[j].transportStreamID == info.transport_stream_id &&
					    setList[j].serviceID == info.service_id &&
					    setList[j].eventID == info.event_id ){
						found = true;
					}
				}
				if( found == false ){
					//まだ存在しないので追加対象
					setList.resize(setList.size() + 1);
					RESERVE_DATA& item = setList.back();
					if( info.shortInfo != NULL ){
						item.title = info.shortInfo->event_name;
					}
					item.startTime = info.start_time;
					item.startTimeEpg = item.startTime;
					item.durationSecond = info.durationSec;
					this->epgDB.SearchServiceName(info.original_network_id, info.transport_stream_id, info.service_id, item.stationName);
					item.originalNetworkID = info.original_network_id;
					item.transportStreamID = info.transport_stream_id;
					item.serviceID = info.service_id;
					item.eventID = info.event_id;
					item.recSetting = data.recSetting;
					if( data.searchInfo.chkRecEnd != 0 && this->reserveManager.IsFindRecEventInfo(info, data.searchInfo.chkRecDay) ){
						item.recSetting.recMode = RECMODE_NO;
					}
					item.comment = L"EPG自動予約";
					if( resultList[i].findKey.empty() == false ){
						item.comment += L"(" + resultList[i].findKey + L")";
						Replace(item.comment, L"\r", L"");
						Replace(item.comment, L"\n", L"");
					}
				}
			}else if( data.searchInfo.chkRecEnd != 0 && this->reserveManager.IsFindRecEventInfo(info, data.searchInfo.chkRecDay) ){
				//録画済みなので無効でない予約は無効にする
				if( this->reserveManager.ChgAutoAddNoRec(info.original_network_id, info.transport_stream_id, info.service_id, info.event_id) ){
					modified = true;
				}
			}
		}
	}
	if( setList.empty() == false && this->reserveManager.AddReserveData(setList, false, noReportNotify) ){
		modified = true;
	}
	CBlockLock lock(&this->settingLock);
	//addCountは参考程度の情報。保存もされないので更新を通知する必要はない
	this->epgAutoAdd.SetAddCount(data.dataID, addCount);
	return modified;
}

bool CEpgTimerSrvMain::AutoAddReserveProgram(const MANUAL_AUTO_ADD_DATA& data, bool noReportNotify)
{
	vector<RESERVE_DATA> setList;
	SYSTEMTIME baseTime;
	GetLocalTime(&baseTime);
	__int64 now = ConvertI64Time(baseTime);
	baseTime.wHour = 0;
	baseTime.wMinute = 0;
	baseTime.wSecond = 0;
	baseTime.wMilliseconds = 0;
	__int64 baseStartTime = ConvertI64Time(baseTime);

	for( int i = 0; i < 8; i++ ){
		//今日から8日分を調べる
		if( data.dayOfWeekFlag >> ((i + baseTime.wDayOfWeek) % 7) & 1 ){
			__int64 startTime = baseStartTime + (data.startTime + i * 24 * 60 * 60) * I64_1SEC;
			if( startTime > now ){
				//同一時間の予約がすでにあるかチェック
				if( this->reserveManager.IsFindProgramReserve(
				    data.originalNetworkID, data.transportStreamID, data.serviceID, startTime, data.durationSecond) == false ){
					//見つからなかったので予約追加
					setList.resize(setList.size() + 1);
					RESERVE_DATA& item = setList.back();
					item.title = data.title;
					ConvertSystemTime(startTime, &item.startTime); 
					item.startTimeEpg = item.startTime;
					item.durationSecond = data.durationSecond;
					item.stationName = data.stationName;
					item.originalNetworkID = data.originalNetworkID;
					item.transportStreamID = data.transportStreamID;
					item.serviceID = data.serviceID;
					item.eventID = 0xFFFF;
					item.recSetting = data.recSetting;
					item.comment = L"プログラム自動予約";
				}
			}
		}
	}
	return setList.empty() == false && this->reserveManager.AddReserveData(setList, false, noReportNotify);
}

int CALLBACK CEpgTimerSrvMain::CtrlCmdPipeCallback(void* param, CMD_STREAM* cmdParam, CMD_STREAM* resParam)
{
	return CtrlCmdCallback(param, cmdParam, resParam, false);
}

int CALLBACK CEpgTimerSrvMain::CtrlCmdTcpCallback(void* param, CMD_STREAM* cmdParam, CMD_STREAM* resParam)
{
	return CtrlCmdCallback(param, cmdParam, resParam, true);
}

int CEpgTimerSrvMain::CtrlCmdCallback(void* param, CMD_STREAM* cmdParam, CMD_STREAM* resParam, bool tcpFlag)
{
	//この関数はPipeとTCPとで同時に呼び出されるかもしれない(各々が同時に複数呼び出すことはない)
	CEpgTimerSrvMain* sys = (CEpgTimerSrvMain*)param;

	resParam->dataSize = 0;
	resParam->param = CMD_ERR;

	if( sys->CtrlCmdProcessCompatible(*cmdParam, *resParam) ){
		return 0;
	}

	switch( cmdParam->param ){
	case CMD2_EPG_SRV_RELOAD_EPG:
		if( sys->epgDB.IsLoadingData() != FALSE ){
			resParam->param = CMD_ERR_BUSY;
		}else if( sys->epgDB.ReloadEpgData() != FALSE ){
			PostMessage(sys->hwndMain, WM_RELOAD_EPG_CHK, 0, 0);
			resParam->param = CMD_SUCCESS;
		}
		break;
	case CMD2_EPG_SRV_RELOAD_SETTING:
		sys->ReloadSetting();
		sys->ReloadNetworkSetting();
		resParam->param = CMD_SUCCESS;
		break;
	case CMD2_EPG_SRV_CLOSE:
		if( sys->residentFlag == false ){
			sys->StopMain();
			resParam->param = CMD_SUCCESS;
		}
		break;
	case CMD2_EPG_SRV_REGIST_GUI:
		{
			DWORD processID = 0;
			if( ReadVALUE(&processID, cmdParam->data, cmdParam->dataSize, NULL) ){
				//CPipeServerの仕様的にこの時点で相手と通信できるとは限らない。接続待機用イベントが作成されるまで少し待つ
				wstring eventName;
				Format(eventName, L"%s%d", CMD2_GUI_CTRL_WAIT_CONNECT, processID);
				for( int i = 0; i < 100; i++ ){
					HANDLE waitEvent = OpenEvent(SYNCHRONIZE, FALSE, eventName.c_str());
					if( waitEvent ){
						CloseHandle(waitEvent);
						break;
					}
					Sleep(100);
				}
				resParam->param = CMD_SUCCESS;
				sys->notifyManager.RegistGUI(processID);
			}
		}
		break;
	case CMD2_EPG_SRV_UNREGIST_GUI:
		{
			DWORD processID = 0;
			if( ReadVALUE(&processID, cmdParam->data, cmdParam->dataSize, NULL) ){
				resParam->param = CMD_SUCCESS;
				sys->notifyManager.UnRegistGUI(processID);
			}
		}
		break;
	case CMD2_EPG_SRV_REGIST_GUI_TCP:
		{
			REGIST_TCP_INFO val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) ){
				resParam->param = CMD_SUCCESS;
				sys->notifyManager.RegistTCP(val);
			}
		}
		break;
	case CMD2_EPG_SRV_UNREGIST_GUI_TCP:
		{
			REGIST_TCP_INFO val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) ){
				resParam->param = CMD_SUCCESS;
				sys->notifyManager.UnRegistTCP(val);
			}
		}
		break;
	case CMD2_EPG_SRV_ENUM_RESERVE:
		OutputDebugString(L"CMD2_EPG_SRV_ENUM_RESERVE\r\n");
		resParam->data = NewWriteVALUE(sys->reserveManager.GetReserveDataAll(), resParam->dataSize);
		resParam->param = CMD_SUCCESS;
		break;
	case CMD2_EPG_SRV_GET_RESERVE:
		{
			OutputDebugString(L"CMD2_EPG_SRV_GET_RESERVE\r\n");
			DWORD reserveID;
			if( ReadVALUE(&reserveID, cmdParam->data, cmdParam->dataSize, NULL) ){
				RESERVE_DATA info;
				if( sys->reserveManager.GetReserveData(reserveID, &info) ){
					resParam->data = NewWriteVALUE(info, resParam->dataSize);
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD2_EPG_SRV_ADD_RESERVE:
		{
			vector<RESERVE_DATA> list;
			if( ReadVALUE(&list, cmdParam->data, cmdParam->dataSize, NULL) &&
			    sys->reserveManager.AddReserveData(list) ){
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_DEL_RESERVE:
		{
			vector<DWORD> list;
			if( ReadVALUE(&list, cmdParam->data, cmdParam->dataSize, NULL) ){
				sys->reserveManager.DelReserveData(list);
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_CHG_RESERVE:
		{
			vector<RESERVE_DATA> list;
			if( ReadVALUE(&list, cmdParam->data, cmdParam->dataSize, NULL) &&
			    sys->reserveManager.ChgReserveData(list) ){
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_ENUM_RECINFO:
		OutputDebugString(L"CMD2_EPG_SRV_ENUM_RECINFO\r\n");
		resParam->data = NewWriteVALUE(sys->reserveManager.GetRecFileInfoAll(), resParam->dataSize);
		resParam->param = CMD_SUCCESS;
		break;
	case CMD2_EPG_SRV_DEL_RECINFO:
		{
			vector<DWORD> list;
			if( ReadVALUE(&list, cmdParam->data, cmdParam->dataSize, NULL) ){
				sys->reserveManager.DelRecFileInfo(list);
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_ENUM_SERVICE:
		OutputDebugString(L"CMD2_EPG_SRV_ENUM_SERVICE\r\n");
		if( sys->epgDB.IsInitialLoadingDataDone() == FALSE ){
			resParam->param = CMD_ERR_BUSY;
		}else{
			vector<EPGDB_SERVICE_INFO> list;
			if( sys->epgDB.GetServiceList(&list) != FALSE ){
				resParam->data = NewWriteVALUE(list, resParam->dataSize);
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_ENUM_PG_INFO:
		OutputDebugString(L"CMD2_EPG_SRV_ENUM_PG_INFO\r\n");
		if( sys->epgDB.IsInitialLoadingDataDone() == FALSE ){
			resParam->param = CMD_ERR_BUSY;
		}else{
			LONGLONG serviceKey;
			if( ReadVALUE(&serviceKey, cmdParam->data, cmdParam->dataSize, NULL) ){
				sys->epgDB.EnumEventInfo(serviceKey, [=](const vector<EPGDB_EVENT_INFO>& val) {
					resParam->param = CMD_SUCCESS;
					resParam->data = NewWriteVALUE(val, resParam->dataSize);
				});
			}
		}
		break;
	case CMD2_EPG_SRV_SEARCH_PG:
		OutputDebugString(L"CMD2_EPG_SRV_SEARCH_PG\r\n");
		if( sys->epgDB.IsInitialLoadingDataDone() == FALSE ){
			resParam->param = CMD_ERR_BUSY;
		}else{
			vector<EPGDB_SEARCH_KEY_INFO> key;
			if( ReadVALUE(&key, cmdParam->data, cmdParam->dataSize, NULL) ){
				sys->epgDB.SearchEpg(&key, [=](vector<CEpgDBManager::SEARCH_RESULT_EVENT>& val) {
					vector<const EPGDB_EVENT_INFO*> valp;
					valp.reserve(val.size());
					for( size_t i = 0; i < val.size(); valp.push_back(val[i++].info) );
					resParam->param = CMD_SUCCESS;
					resParam->data = NewWriteVALUE(valp, resParam->dataSize);
				});
			}
		}
		break;
	case CMD2_EPG_SRV_GET_PG_INFO:
		OutputDebugString(L"CMD2_EPG_SRV_GET_PG_INFO\r\n");
		if( sys->epgDB.IsInitialLoadingDataDone() == FALSE ){
			resParam->param = CMD_ERR_BUSY;
		}else{
			ULONGLONG key;
			EPGDB_EVENT_INFO val;
			if( ReadVALUE(&key, cmdParam->data, cmdParam->dataSize, NULL) &&
			    sys->epgDB.SearchEpg(key>>48&0xFFFF, key>>32&0xFFFF, key>>16&0xFFFF, key&0xFFFF, &val) ){
				resParam->data = NewWriteVALUE(val, resParam->dataSize);
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_CHK_SUSPEND:
		if( sys->IsSuspendOK() ){
			resParam->param = CMD_SUCCESS;
		}
		break;
	case CMD2_EPG_SRV_SUSPEND:
		{
			WORD val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) && sys->IsSuspendOK() ){
				if( HIBYTE(val) == 0xFF ){
					val = MAKEWORD(LOBYTE(val), HIBYTE(sys->defShutdownMode));
				}
				PostMessage(sys->hwndMain, WM_REQUEST_SHUTDOWN, val, 0);
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_REBOOT:
		{
			PostMessage(sys->hwndMain, WM_REQUEST_SHUTDOWN, 0x01FF, 0);
			resParam->param = CMD_SUCCESS;
		}
		break;
	case CMD2_EPG_SRV_EPG_CAP_NOW:
		if( sys->epgDB.IsInitialLoadingDataDone() == FALSE ){
			resParam->param = CMD_ERR_BUSY;
		}else if( sys->reserveManager.RequestStartEpgCap() ){
			resParam->param = CMD_SUCCESS;
		}
		break;
	case CMD2_EPG_SRV_ENUM_AUTO_ADD:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ENUM_AUTO_ADD\r\n");
			vector<EPG_AUTO_ADD_DATA> val;
			{
				CBlockLock lock(&sys->settingLock);
				map<DWORD, EPG_AUTO_ADD_DATA>::const_iterator itr;
				for( itr = sys->epgAutoAdd.GetMap().begin(); itr != sys->epgAutoAdd.GetMap().end(); itr++ ){
					val.push_back(itr->second);
				}
			}
			resParam->data = NewWriteVALUE(val, resParam->dataSize);
			resParam->param = CMD_SUCCESS;
		}
		break;
	case CMD2_EPG_SRV_ADD_AUTO_ADD:
		{
			vector<EPG_AUTO_ADD_DATA> val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) ){
				{
					CBlockLock lock(&sys->settingLock);
					for( size_t i = 0; i < val.size(); i++ ){
						val[i].dataID = sys->epgAutoAdd.AddData(val[i]);
					}
					sys->epgAutoAdd.SaveText();
				}
				bool addReserve = false;
				for( size_t i = 0; i < val.size(); i++ ){
					addReserve |= sys->AutoAddReserveEPG(val[i], true);
				}
				sys->notifyManager.AddNotify(NOTIFY_UPDATE_AUTOADD_EPG);
				if( addReserve ){
					sys->reserveManager.AddNotifyAndPostBat(NOTIFY_UPDATE_RESERVE_INFO);
				}
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_DEL_AUTO_ADD:
		{
			vector<DWORD> val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) ){
				CBlockLock lock(&sys->settingLock);
				for( size_t i = 0; i < val.size(); i++ ){
					sys->epgAutoAdd.DelData(val[i]);
				}
				sys->epgAutoAdd.SaveText();
				sys->notifyManager.AddNotify(NOTIFY_UPDATE_AUTOADD_EPG);
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_CHG_AUTO_ADD:
		{
			vector<EPG_AUTO_ADD_DATA> val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL ) ){
				{
					CBlockLock lock(&sys->settingLock);
					for( size_t i = 0; i < val.size(); i++ ){
						if( sys->epgAutoAdd.ChgData(val[i]) == false ){
							val.erase(val.begin() + (i--));
						}
					}
					sys->epgAutoAdd.SaveText();
				}
				bool addReserve = false;
				for( size_t i = 0; i < val.size(); i++ ){
					addReserve |= sys->AutoAddReserveEPG(val[i], true);
				}
				sys->notifyManager.AddNotify(NOTIFY_UPDATE_AUTOADD_EPG);
				if( addReserve ){
					sys->reserveManager.AddNotifyAndPostBat(NOTIFY_UPDATE_RESERVE_INFO);
				}
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_ENUM_MANU_ADD:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ENUM_MANU_ADD\r\n");
			vector<MANUAL_AUTO_ADD_DATA> val;
			{
				CBlockLock lock(&sys->settingLock);
				map<DWORD, MANUAL_AUTO_ADD_DATA>::const_iterator itr;
				for( itr = sys->manualAutoAdd.GetMap().begin(); itr != sys->manualAutoAdd.GetMap().end(); itr++ ){
					val.push_back(itr->second);
				}
			}
			resParam->data = NewWriteVALUE(val, resParam->dataSize);
			resParam->param = CMD_SUCCESS;
		}
		break;
	case CMD2_EPG_SRV_ADD_MANU_ADD:
		{
			vector<MANUAL_AUTO_ADD_DATA> val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) ){
				{
					CBlockLock lock(&sys->settingLock);
					for( size_t i = 0; i < val.size(); i++ ){
						val[i].dataID = sys->manualAutoAdd.AddData(val[i]);
					}
					sys->manualAutoAdd.SaveText();
				}
				bool addReserve = false;
				for( size_t i = 0; i < val.size(); i++ ){
					addReserve |= sys->AutoAddReserveProgram(val[i], true);
				}
				sys->notifyManager.AddNotify(NOTIFY_UPDATE_AUTOADD_MANUAL);
				if( addReserve ){
					sys->reserveManager.AddNotifyAndPostBat(NOTIFY_UPDATE_RESERVE_INFO);
				}
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_DEL_MANU_ADD:
		{
			vector<DWORD> val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) ){
				CBlockLock lock(&sys->settingLock);
				for( size_t i = 0; i < val.size(); i++ ){
					sys->manualAutoAdd.DelData(val[i]);
				}
				sys->manualAutoAdd.SaveText();
				sys->notifyManager.AddNotify(NOTIFY_UPDATE_AUTOADD_MANUAL);
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_CHG_MANU_ADD:
		{
			vector<MANUAL_AUTO_ADD_DATA> val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL ) ){
				{
					CBlockLock lock(&sys->settingLock);
					for( size_t i = 0; i < val.size(); i++ ){
						if( sys->manualAutoAdd.ChgData(val[i]) == false ){
							val.erase(val.begin() + (i--));
						}
					}
					sys->manualAutoAdd.SaveText();
				}
				bool addReserve = false;
				for( size_t i = 0; i < val.size(); i++ ){
					addReserve |= sys->AutoAddReserveProgram(val[i], true);
				}
				sys->notifyManager.AddNotify(NOTIFY_UPDATE_AUTOADD_MANUAL);
				if( addReserve ){
					sys->reserveManager.AddNotifyAndPostBat(NOTIFY_UPDATE_RESERVE_INFO);
				}
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_ENUM_TUNER_RESERVE:
		OutputDebugString(L"CMD2_EPG_SRV_ENUM_TUNER_RESERVE\r\n");
		resParam->data = NewWriteVALUE(sys->reserveManager.GetTunerReserveAll(), resParam->dataSize);
		resParam->param = CMD_SUCCESS;
		break;
	case CMD2_EPG_SRV_FILE_COPY:
		{
			wstring val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) && CompareNoCase(val, L"ChSet5.txt") == 0 ){
				wstring path;
				GetSettingPath(path);
				HANDLE hFile = CreateFile((path + L"\\ChSet5.txt").c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
				if( hFile != INVALID_HANDLE_VALUE ){
					DWORD dwFileSize = GetFileSize(hFile, NULL);
					if( dwFileSize != INVALID_FILE_SIZE && dwFileSize != 0 ){
						resParam->data.reset(new BYTE[dwFileSize]);
						DWORD dwRead;
						if( ReadFile(hFile, resParam->data.get(), dwFileSize, &dwRead, NULL) && dwRead == dwFileSize ){
							resParam->dataSize = dwRead;
						}
					}
					CloseHandle(hFile);
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD2_EPG_SRV_ENUM_PG_ALL:
		OutputDebugString(L"CMD2_EPG_SRV_ENUM_PG_ALL\r\n");
		if( sys->epgDB.IsInitialLoadingDataDone() == FALSE ){
			resParam->param = CMD_ERR_BUSY;
		}else{
			sys->epgDB.EnumEventAll([=](const map<LONGLONG, EPGDB_SERVICE_EVENT_INFO>& val) {
				vector<const EPGDB_SERVICE_EVENT_INFO*> valp;
				valp.reserve(val.size());
				for( auto itr = val.cbegin(); itr != val.end(); valp.push_back(&(itr++)->second) );
				resParam->param = CMD_SUCCESS;
				resParam->data = NewWriteVALUE(valp, resParam->dataSize);
			});
		}
		break;
	case CMD2_EPG_SRV_ENUM_PLUGIN:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ENUM_PLUGIN\r\n");
			WORD mode;
			if( ReadVALUE(&mode, cmdParam->data, cmdParam->dataSize, NULL) && (mode == 1 || mode == 2) ){
				wstring path;
				GetModuleFolderPath(path);
				WIN32_FIND_DATA findData;
				//指定フォルダのファイル一覧取得
				HANDLE hFind = FindFirstFile((path + (mode == 1 ? L"\\RecName\\RecName*.dll" : L"\\Write\\Write*.dll")).c_str(), &findData);
				if( hFind != INVALID_HANDLE_VALUE ){
					vector<wstring> fileList;
					do{
						if( (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 && IsExt(findData.cFileName, L".dll") ){
							fileList.push_back(findData.cFileName);
						}
					}while( FindNextFile(hFind, &findData) );
					FindClose(hFind);
					resParam->data = NewWriteVALUE(fileList, resParam->dataSize);
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD2_EPG_SRV_GET_CHG_CH_TVTEST:
		{
			OutputDebugString(L"CMD2_EPG_SRV_GET_CHG_CH_TVTEST\r\n");
			LONGLONG key;
			if( ReadVALUE(&key, cmdParam->data, cmdParam->dataSize, NULL) ){
				CBlockLock lock(&sys->settingLock);
				TVTEST_CH_CHG_INFO info;
				info.chInfo.useSID = TRUE;
				info.chInfo.ONID = key >> 32 & 0xFFFF;
				info.chInfo.TSID = key >> 16 & 0xFFFF;
				info.chInfo.SID = key & 0xFFFF;
				info.chInfo.useBonCh = FALSE;
				vector<DWORD> idList = sys->reserveManager.GetSupportServiceTuner(info.chInfo.ONID, info.chInfo.TSID, info.chInfo.SID);
				for( int i = (int)idList.size() - 1; i >= 0; i-- ){
					info.bonDriver = sys->reserveManager.GetTunerBonFileName(idList[i]);
					for( size_t j = 0; j < sys->tvtestUseBon.size(); j++ ){
						if( CompareNoCase(sys->tvtestUseBon[j], info.bonDriver) == 0 ){
							if( sys->reserveManager.IsOpenTuner(idList[i]) == false ){
								info.chInfo.useBonCh = TRUE;
								sys->reserveManager.GetTunerCh(idList[i], info.chInfo.ONID, info.chInfo.TSID, info.chInfo.SID, &info.chInfo.space, &info.chInfo.ch);
							}
							break;
						}
					}
					if( info.chInfo.useBonCh ){
						resParam->data = NewWriteVALUE(info, resParam->dataSize);
						resParam->param = CMD_SUCCESS;
						break;
					}
				}
			}
		}
		break;
	case CMD2_EPG_SRV_NWTV_SET_CH:
		{
			OutputDebugString(L"CMD2_EPG_SRV_NWTV_SET_CH\r\n");
			SET_CH_INFO val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) && val.useSID ){
				CBlockLock lock(&sys->settingLock);
				vector<DWORD> idList = sys->reserveManager.GetSupportServiceTuner(val.ONID, val.TSID, val.SID);
				vector<DWORD> idUseList;
				for( int i = (int)idList.size() - 1; i >= 0; i-- ){
					wstring bonDriver = sys->reserveManager.GetTunerBonFileName(idList[i]);
					for( size_t j = 0; j < sys->tvtestUseBon.size(); j++ ){
						if( CompareNoCase(sys->tvtestUseBon[j], bonDriver) == 0 ){
							idUseList.push_back(idList[i]);
							break;
						}
					}
				}
				if( sys->reserveManager.SetNWTVCh(sys->nwtvUdp, sys->nwtvTcp, val, idUseList) ){
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD2_EPG_SRV_NWTV_CLOSE:
		{
			OutputDebugString(L"CMD2_EPG_SRV_NWTV_CLOSE\r\n");
			if( sys->reserveManager.CloseNWTV() ){
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_NWTV_MODE:
		{
			OutputDebugString(L"CMD2_EPG_SRV_NWTV_MODE\r\n");
			DWORD val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) ){
				CBlockLock lock(&sys->settingLock);
				sys->nwtvUdp = val == 1 || val == 3;
				sys->nwtvTcp = val == 2 || val == 3;
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_NWPLAY_OPEN:
		{
			OutputDebugString(L"CMD2_EPG_SRV_NWPLAY_OPEN\r\n");
			wstring val;
			DWORD id;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) && sys->streamingManager.OpenFile(val.c_str(), &id) ){
				resParam->data = NewWriteVALUE(id, resParam->dataSize);
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_NWPLAY_CLOSE:
		{
			OutputDebugString(L"CMD2_EPG_SRV_NWPLAY_CLOSE\r\n");
			DWORD val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) && sys->streamingManager.CloseFile(val) ){
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_NWPLAY_PLAY:
		{
			OutputDebugString(L"CMD2_EPG_SRV_NWPLAY_PLAY\r\n");
			DWORD val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) && sys->streamingManager.StartSend(val) ){
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_NWPLAY_STOP:
		{
			OutputDebugString(L"CMD2_EPG_SRV_NWPLAY_STOP\r\n");
			DWORD val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) && sys->streamingManager.StopSend(val) ){
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_NWPLAY_GET_POS:
		{
			OutputDebugString(L"CMD2_EPG_SRV_NWPLAY_GET_POS\r\n");
			NWPLAY_POS_CMD val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) && sys->streamingManager.GetPos(&val) ){
				resParam->data = NewWriteVALUE(val, resParam->dataSize);
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_NWPLAY_SET_POS:
		{
			OutputDebugString(L"CMD2_EPG_SRV_NWPLAY_SET_POS\r\n");
			NWPLAY_POS_CMD val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) && sys->streamingManager.SetPos(&val) ){
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_NWPLAY_SET_IP:
		{
			OutputDebugString(L"CMD2_EPG_SRV_NWPLAY_SET_IP\r\n");
			NWPLAY_PLAY_INFO val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) && sys->streamingManager.SetIP(&val) ){
				resParam->data = NewWriteVALUE(val, resParam->dataSize);
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_NWPLAY_TF_OPEN:
		{
			OutputDebugString(L"CMD2_EPG_SRV_NWPLAY_TF_OPEN\r\n");
			DWORD val;
			NWPLAY_TIMESHIFT_INFO resVal;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) &&
			    sys->reserveManager.GetRecFilePath(val, resVal.filePath) &&
			    sys->streamingManager.OpenTimeShift(resVal.filePath.c_str(), &resVal.ctrlID) ){
				resParam->data = NewWriteVALUE(resVal, resParam->dataSize);
				resParam->param = CMD_SUCCESS;
			}
		}
		break;

	////////////////////////////////////////////////////////////
	//CMD_VER対応コマンド
	case CMD2_EPG_SRV_ENUM_RESERVE2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ENUM_RESERVE2\r\n");
			WORD ver;
			if( ReadVALUE(&ver, cmdParam->data, cmdParam->dataSize, NULL) ){
				//ver>=5では録画予定ファイル名も返す
				resParam->data = NewWriteVALUE2WithVersion(ver, sys->reserveManager.GetReserveDataAll(ver >= 5), resParam->dataSize);
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_GET_RESERVE2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_GET_RESERVE2\r\n");
			WORD ver;
			DWORD readSize;
			if( ReadVALUE(&ver, cmdParam->data, cmdParam->dataSize, &readSize) ){
				DWORD reserveID;
				if( ReadVALUE2(ver, &reserveID, cmdParam->data.get() + readSize, cmdParam->dataSize - readSize, NULL) ){
					RESERVE_DATA info;
					if( sys->reserveManager.GetReserveData(reserveID, &info) ){
						resParam->data = NewWriteVALUE2WithVersion(ver, info, resParam->dataSize);
						resParam->param = CMD_SUCCESS;
					}
				}
			}
		}
		break;
	case CMD2_EPG_SRV_ADD_RESERVE2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ADD_RESERVE2\r\n");
			WORD ver;
			DWORD readSize;
			if( ReadVALUE(&ver, cmdParam->data, cmdParam->dataSize, &readSize) ){
				vector<RESERVE_DATA> list;
				if( ReadVALUE2(ver, &list, cmdParam->data.get() + readSize, cmdParam->dataSize - readSize, NULL) &&
				    sys->reserveManager.AddReserveData(list) ){
					resParam->data = NewWriteVALUE(ver, resParam->dataSize);
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD2_EPG_SRV_CHG_RESERVE2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_CHG_RESERVE2\r\n");
			WORD ver;
			DWORD readSize;
			if( ReadVALUE(&ver, cmdParam->data, cmdParam->dataSize, &readSize) ){
				vector<RESERVE_DATA> list;
				if( ReadVALUE2(ver, &list, cmdParam->data.get() + readSize, cmdParam->dataSize - readSize, NULL) &&
				    sys->reserveManager.ChgReserveData(list) ){
					resParam->data = NewWriteVALUE(ver, resParam->dataSize);
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD2_EPG_SRV_ADDCHK_RESERVE2:
		OutputDebugString(L"CMD2_EPG_SRV_ADDCHK_RESERVE2\r\n");
		resParam->param = CMD_NON_SUPPORT;
		break;
	case CMD2_EPG_SRV_GET_EPG_FILETIME2:
		OutputDebugString(L"CMD2_EPG_SRV_GET_EPG_FILETIME2\r\n");
		resParam->param = CMD_NON_SUPPORT;
		break;
	case CMD2_EPG_SRV_GET_EPG_FILE2:
		OutputDebugString(L"CMD2_EPG_SRV_GET_EPG_FILE2\r\n");
		resParam->param = CMD_NON_SUPPORT;
		break;
	case CMD2_EPG_SRV_ENUM_AUTO_ADD2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ENUM_AUTO_ADD2\r\n");
			WORD ver;
			if( ReadVALUE(&ver, cmdParam->data, cmdParam->dataSize, NULL) ){
				vector<EPG_AUTO_ADD_DATA> val;
				{
					CBlockLock lock(&sys->settingLock);
					map<DWORD, EPG_AUTO_ADD_DATA>::const_iterator itr;
					for( itr = sys->epgAutoAdd.GetMap().begin(); itr != sys->epgAutoAdd.GetMap().end(); itr++ ){
						val.push_back(itr->second);
					}
				}
				resParam->data = NewWriteVALUE2WithVersion(ver, val, resParam->dataSize);
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_ADD_AUTO_ADD2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ADD_AUTO_ADD2\r\n");
			WORD ver;
			DWORD readSize;
			if( ReadVALUE(&ver, cmdParam->data, cmdParam->dataSize, &readSize) ){
				vector<EPG_AUTO_ADD_DATA> val;
				if( ReadVALUE2(ver, &val, cmdParam->data.get() + readSize, cmdParam->dataSize - readSize, NULL) ){
					{
						CBlockLock lock(&sys->settingLock);
						for( size_t i = 0; i < val.size(); i++ ){
							val[i].dataID = sys->epgAutoAdd.AddData(val[i]);
						}
						sys->epgAutoAdd.SaveText();
					}
					bool addReserve = false;
					for( size_t i = 0; i < val.size(); i++ ){
						addReserve |= sys->AutoAddReserveEPG(val[i], true);
					}
					sys->notifyManager.AddNotify(NOTIFY_UPDATE_AUTOADD_EPG);
					if( addReserve ){
						sys->reserveManager.AddNotifyAndPostBat(NOTIFY_UPDATE_RESERVE_INFO);
					}
					resParam->data = NewWriteVALUE(ver, resParam->dataSize);
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD2_EPG_SRV_CHG_AUTO_ADD2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_CHG_AUTO_ADD2\r\n");
			WORD ver;
			DWORD readSize;
			if( ReadVALUE(&ver, cmdParam->data, cmdParam->dataSize, &readSize) ){
				vector<EPG_AUTO_ADD_DATA> val;
				if( ReadVALUE2(ver, &val, cmdParam->data.get() + readSize, cmdParam->dataSize - readSize, NULL) ){
					{
						CBlockLock lock(&sys->settingLock);
						for( size_t i = 0; i < val.size(); i++ ){
							if( sys->epgAutoAdd.ChgData(val[i]) == false ){
								val.erase(val.begin() + (i--));
							}
						}
						sys->epgAutoAdd.SaveText();
					}
					bool addReserve = false;
					for( size_t i = 0; i < val.size(); i++ ){
						addReserve |= sys->AutoAddReserveEPG(val[i], true);
					}
					sys->notifyManager.AddNotify(NOTIFY_UPDATE_AUTOADD_EPG);
					if( addReserve ){
						sys->reserveManager.AddNotifyAndPostBat(NOTIFY_UPDATE_RESERVE_INFO);
					}
					resParam->data = NewWriteVALUE(ver, resParam->dataSize);
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD2_EPG_SRV_ENUM_MANU_ADD2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ENUM_MANU_ADD2\r\n");
			WORD ver;
			if( ReadVALUE(&ver, cmdParam->data, cmdParam->dataSize, NULL) ){
				vector<MANUAL_AUTO_ADD_DATA> val;
				{
					CBlockLock lock(&sys->settingLock);
					map<DWORD, MANUAL_AUTO_ADD_DATA>::const_iterator itr;
					for( itr = sys->manualAutoAdd.GetMap().begin(); itr != sys->manualAutoAdd.GetMap().end(); itr++ ){
						val.push_back(itr->second);
					}
				}
				resParam->data = NewWriteVALUE2WithVersion(ver, val, resParam->dataSize);
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_ADD_MANU_ADD2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ADD_MANU_ADD2\r\n");
			WORD ver;
			DWORD readSize;
			if( ReadVALUE(&ver, cmdParam->data, cmdParam->dataSize, &readSize) ){
				vector<MANUAL_AUTO_ADD_DATA> val;
				if( ReadVALUE2(ver, &val, cmdParam->data.get() + readSize, cmdParam->dataSize - readSize, NULL) ){
					{
						CBlockLock lock(&sys->settingLock);
						for( size_t i = 0; i < val.size(); i++ ){
							val[i].dataID = sys->manualAutoAdd.AddData(val[i]);
						}
						sys->manualAutoAdd.SaveText();
					}
					bool addReserve = false;
					for( size_t i = 0; i < val.size(); i++ ){
						addReserve |= sys->AutoAddReserveProgram(val[i], true);
					}
					sys->notifyManager.AddNotify(NOTIFY_UPDATE_AUTOADD_MANUAL);
					if( addReserve ){
						sys->reserveManager.AddNotifyAndPostBat(NOTIFY_UPDATE_RESERVE_INFO);
					}
					resParam->data = NewWriteVALUE(ver, resParam->dataSize);
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD2_EPG_SRV_CHG_MANU_ADD2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_CHG_MANU_ADD2\r\n");
			WORD ver;
			DWORD readSize;
			if( ReadVALUE(&ver, cmdParam->data, cmdParam->dataSize, &readSize) ){
				vector<MANUAL_AUTO_ADD_DATA> val;
				if( ReadVALUE2(ver, &val, cmdParam->data.get() + readSize, cmdParam->dataSize - readSize, NULL) ){
					{
						CBlockLock lock(&sys->settingLock);
						for( size_t i = 0; i < val.size(); i++ ){
							if( sys->manualAutoAdd.ChgData(val[i]) == false ){
								val.erase(val.begin() + (i--));
							}
						}
						sys->manualAutoAdd.SaveText();
					}
					bool addReserve = false;
					for( size_t i = 0; i < val.size(); i++ ){
						addReserve |= sys->AutoAddReserveProgram(val[i], true);
					}
					sys->notifyManager.AddNotify(NOTIFY_UPDATE_AUTOADD_MANUAL);
					if( addReserve ){
						sys->reserveManager.AddNotifyAndPostBat(NOTIFY_UPDATE_RESERVE_INFO);
					}
					resParam->data = NewWriteVALUE(ver, resParam->dataSize);
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD2_EPG_SRV_ENUM_RECINFO2:
	case CMD2_EPG_SRV_ENUM_RECINFO_BASIC2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_ENUM_RECINFO2\r\n");
			WORD ver;
			if( ReadVALUE(&ver, cmdParam->data, cmdParam->dataSize, NULL) ){
				resParam->data = NewWriteVALUE2WithVersion(ver,
					sys->reserveManager.GetRecFileInfoAll(cmdParam->param == CMD2_EPG_SRV_ENUM_RECINFO2), resParam->dataSize);
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_EPG_SRV_GET_RECINFO2:
		{
			WORD ver;
			DWORD readSize;
			if( ReadVALUE(&ver, cmdParam->data, cmdParam->dataSize, &readSize) ){
				REC_FILE_INFO info;
				if( ReadVALUE2(ver, &info.id, cmdParam->data.get() + readSize, cmdParam->dataSize - readSize, NULL) &&
				    sys->reserveManager.GetRecFileInfo(info.id, &info) ){
					resParam->data = NewWriteVALUE2WithVersion(ver, info, resParam->dataSize);
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD2_EPG_SRV_CHG_PROTECT_RECINFO2:
		{
			OutputDebugString(L"CMD2_EPG_SRV_CHG_PROTECT_RECINFO2\r\n");
			WORD ver;
			DWORD readSize;
			if( ReadVALUE(&ver, cmdParam->data, cmdParam->dataSize, &readSize) ){
				vector<REC_FILE_INFO> list;
				if( ReadVALUE2(ver, &list, cmdParam->data.get() + readSize, cmdParam->dataSize - readSize, NULL) ){
					sys->reserveManager.ChgProtectRecFileInfo(list);
					resParam->data = NewWriteVALUE(ver, resParam->dataSize);
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD2_EPG_SRV_GET_STATUS_NOTIFY2:
		{
			WORD ver;
			DWORD readSize;
			if( ReadVALUE(&ver, cmdParam->data, cmdParam->dataSize, &readSize) ){
				DWORD count;
				if( ReadVALUE2(ver, &count, cmdParam->data.get() + readSize, cmdParam->dataSize - readSize, NULL) ){
					NOTIFY_SRV_INFO info;
					if( sys->notifyManager.GetNotify(&info, count) ){
						resParam->data = NewWriteVALUE2WithVersion(ver, info, resParam->dataSize);
						resParam->param = CMD_SUCCESS;
					}else{
						resParam->param = CMD_NO_RES;
					}
				}
			}
		}
		break;
#if 1
	////////////////////////////////////////////////////////////
	//旧バージョン互換コマンド
	case CMD_EPG_SRV_GET_RESERVE_INFO:
		{
			resParam->param = OLD_CMD_ERR;
			DWORD reserveID;
			if( ReadVALUE(&reserveID, cmdParam->data, cmdParam->dataSize, NULL) ){
				RESERVE_DATA info;
				if( sys->reserveManager.GetReserveData(reserveID, &info) ){
					resParam->data = DeprecatedNewWriteVALUE(info, resParam->dataSize);
					resParam->param = OLD_CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD_EPG_SRV_ADD_RESERVE:
		{
			resParam->param = OLD_CMD_ERR;
			vector<RESERVE_DATA> list(1);
			if( DeprecatedReadVALUE(&list.back(), cmdParam->data, cmdParam->dataSize) &&
			    sys->reserveManager.AddReserveData(list) ){
				resParam->param = OLD_CMD_SUCCESS;
			}
		}
		break;
	case CMD_EPG_SRV_DEL_RESERVE:
		{
			resParam->param = OLD_CMD_ERR;
			RESERVE_DATA item;
			if( DeprecatedReadVALUE(&item, cmdParam->data, cmdParam->dataSize) ){
				vector<DWORD> list(1, item.reserveID);
				sys->reserveManager.DelReserveData(list);
				resParam->param = OLD_CMD_SUCCESS;
			}
		}
		break;
	case CMD_EPG_SRV_CHG_RESERVE:
		{
			resParam->param = OLD_CMD_ERR;
			vector<RESERVE_DATA> list(1);
			if( DeprecatedReadVALUE(&list.back(), cmdParam->data, cmdParam->dataSize) &&
			    sys->reserveManager.ChgReserveData(list) ){
				resParam->param = OLD_CMD_SUCCESS;
			}
		}

		break;
	case CMD_EPG_SRV_ADD_AUTO_ADD:
		{
			resParam->param = OLD_CMD_ERR;
			EPG_AUTO_ADD_DATA item;
			if( DeprecatedReadVALUE(&item, cmdParam->data, cmdParam->dataSize) ){
				{
					CBlockLock lock(&sys->settingLock);
					item.dataID = sys->epgAutoAdd.AddData(item);
					sys->epgAutoAdd.SaveText();
				}
				resParam->param = OLD_CMD_SUCCESS;
				sys->AutoAddReserveEPG(item);
				sys->notifyManager.AddNotify(NOTIFY_UPDATE_AUTOADD_EPG);
			}
		}
		break;
	case CMD_EPG_SRV_DEL_AUTO_ADD:
		{
			resParam->param = OLD_CMD_ERR;
			EPG_AUTO_ADD_DATA item;
			if( DeprecatedReadVALUE(&item, cmdParam->data, cmdParam->dataSize) ){
				CBlockLock lock(&sys->settingLock);
				sys->epgAutoAdd.DelData(item.dataID);
				sys->epgAutoAdd.SaveText();
				resParam->param = OLD_CMD_SUCCESS;
				sys->notifyManager.AddNotify(NOTIFY_UPDATE_AUTOADD_EPG);
			}
		}
		break;
	case CMD_EPG_SRV_CHG_AUTO_ADD:
		{
			resParam->param = OLD_CMD_ERR;
			EPG_AUTO_ADD_DATA item;
			if( DeprecatedReadVALUE(&item, cmdParam->data, cmdParam->dataSize) ){
				{
					CBlockLock lock(&sys->settingLock);
					sys->epgAutoAdd.ChgData(item);
					sys->epgAutoAdd.SaveText();
				}
				resParam->param = OLD_CMD_SUCCESS;
				sys->AutoAddReserveEPG(item);
				sys->notifyManager.AddNotify(NOTIFY_UPDATE_AUTOADD_EPG);
			}
		}
		break;
	case CMD_EPG_SRV_SEARCH_PG_FIRST:
		{
			resParam->param = OLD_CMD_ERR;
			if( sys->epgDB.IsInitialLoadingDataDone() == FALSE ){
				resParam->param = CMD_ERR_BUSY;
			}else{
				EPG_AUTO_ADD_DATA item;
				if( DeprecatedReadVALUE(&item, cmdParam->data, cmdParam->dataSize) ){
					vector<EPGDB_SEARCH_KEY_INFO> key(1, item.searchInfo);
					vector<CEpgDBManager::SEARCH_RESULT_EVENT_DATA> val;
					if( sys->epgDB.SearchEpg(&key, &val) != FALSE ){
						sys->oldSearchList[tcpFlag].clear();
						sys->oldSearchList[tcpFlag].resize(val.size());
						for( size_t i = 0; i < val.size(); i++ ){
							sys->oldSearchList[tcpFlag][val.size() - i - 1].DeepCopy(val[i].info);
						}
						if( sys->oldSearchList[tcpFlag].empty() == false ){
							resParam->data = DeprecatedNewWriteVALUE(sys->oldSearchList[tcpFlag].back(), resParam->dataSize);
							sys->oldSearchList[tcpFlag].pop_back();
							resParam->param = sys->oldSearchList[tcpFlag].empty() ? OLD_CMD_SUCCESS : OLD_CMD_NEXT;
						}
					}
				}
			}
		}
		break;
	case CMD_EPG_SRV_SEARCH_PG_NEXT:
		{
			resParam->param = OLD_CMD_ERR;
			if( sys->oldSearchList[tcpFlag].empty() == false ){
				resParam->data = DeprecatedNewWriteVALUE(sys->oldSearchList[tcpFlag].back(), resParam->dataSize);
				sys->oldSearchList[tcpFlag].pop_back();
				resParam->param = sys->oldSearchList[tcpFlag].empty() ? OLD_CMD_SUCCESS : OLD_CMD_NEXT;
			}
		}
		break;
	//旧バージョン互換コマンドここまで
#endif
	default:
		_OutputDebugString(L"err default cmd %d\r\n", cmdParam->param);
		resParam->param = CMD_NON_SUPPORT;
		break;
	}

	return 0;
}

bool CEpgTimerSrvMain::CtrlCmdProcessCompatible(CMD_STREAM& cmdParam, CMD_STREAM& resParam)
{
	//※この関数はtkntrec版( https://github.com/tkntrec/EDCB )を参考にした

	switch( cmdParam.param ){
	case CMD2_EPG_SRV_ISREGIST_GUI_TCP:
		if( g_compatFlags & 0x04 ){
			//互換動作: TCP接続の登録状況確認コマンドを実装する
			OutputDebugString(L"CMD2_EPG_SRV_ISREGIST_GUI_TCP\r\n");
			REGIST_TCP_INFO val;
			if( ReadVALUE(&val, cmdParam.data, cmdParam.dataSize, NULL) ){
				vector<REGIST_TCP_INFO> registTCP = this->notifyManager.GetRegistTCP();
				BOOL registered = std::find_if(registTCP.begin(), registTCP.end(), [&val](const REGIST_TCP_INFO& a) {
					return a.ip == val.ip && a.port == val.port;
				}) != registTCP.end();
				resParam.data = NewWriteVALUE(registered, resParam.dataSize);
				resParam.param = CMD_SUCCESS;
			}
			return true;
		}
		break;
	case CMD2_EPG_SRV_PROFILE_UPDATE:
		if( g_compatFlags & 0x08 ){
			//互換動作: 設定更新通知コマンドを実装する
			OutputDebugString(L"CMD2_EPG_SRV_PROFILE_UPDATE\r\n");
			this->notifyManager.AddNotify(NOTIFY_UPDATE_PROFILE);
			resParam.param = CMD_SUCCESS;
			return true;
		}
		break;
	case CMD2_EPG_SRV_GET_NETWORK_PATH:
		if( g_compatFlags & 0x10 ){
			//互換動作: ネットワークパス取得コマンドを実装する
			OutputDebugString(L"CMD2_EPG_SRV_GET_NETWORK_PATH\r\n");
			wstring path;
			if( ReadVALUE(&path, cmdParam.data, cmdParam.dataSize, NULL) ){
				wstring netPath;
				//UNCパスはそのまま返す
				if( path.compare(0, 2, L"\\\\") == 0 ){
					netPath = path;
				}else{
					DWORD resume = 0;
					for( NET_API_STATUS res = ERROR_MORE_DATA; netPath.empty() && res == ERROR_MORE_DATA; ){
						PSHARE_INFO_502 bufPtr;
						DWORD er, tr;
						res = NetShareEnum(NULL, 502, (BYTE**)&bufPtr, MAX_PREFERRED_LENGTH, &er, &tr, &resume);
						if( res != ERROR_SUCCESS && res != ERROR_MORE_DATA ){
							break;
						}
						for( PSHARE_INFO_502 p = bufPtr; p < bufPtr + er; p++ ){
							//共有名が$で終わるのは隠し共有
							if( p->shi502_netname[0] && p->shi502_netname[wcslen(p->shi502_netname) - 1] != L'$' ){
								wstring shiPath = p->shi502_path;
								ChkFolderPath(shiPath);
								if( path.size() >= shiPath.size() &&
								    CompareNoCase(shiPath, path.substr(0, shiPath.size())) == 0 &&
								    (path.size() == shiPath.size() || path[shiPath.size()] == L'\\') ){
									//共有パスそのものか配下にある
									WCHAR computerName[MAX_COMPUTERNAME_LENGTH + 1];
									DWORD len = MAX_COMPUTERNAME_LENGTH + 1;
									if( GetComputerName(computerName, &len) ){
										netPath = wstring(L"\\\\") + computerName + L'\\' + p->shi502_netname + path.substr(shiPath.size());
										break;
									}
								}
							}
						}
						NetApiBufferFree(bufPtr);
					}
				}
				if( netPath.empty() == false ){
					resParam.data = NewWriteVALUE(netPath, resParam.dataSize);
					resParam.param = CMD_SUCCESS;
				}
			}
			return true;
		}
		break;
	case CMD2_EPG_SRV_SEARCH_PG2:
		if( g_compatFlags & 0x20 ){
			//互換動作: 番組検索の追加コマンドを実装する
			OutputDebugString(L"CMD2_EPG_SRV_SEARCH_PG2\r\n");
			if( this->epgDB.IsInitialLoadingDataDone() == FALSE ){
				resParam.param = CMD_ERR_BUSY;
			}else{
				WORD ver;
				DWORD readSize;
				if( ReadVALUE(&ver, cmdParam.data, cmdParam.dataSize, &readSize) ){
					vector<EPGDB_SEARCH_KEY_INFO> key;
					if( ReadVALUE2(ver, &key, cmdParam.data.get() + readSize, cmdParam.dataSize - readSize, NULL) ){
						this->epgDB.SearchEpg(&key, [=, &resParam](vector<CEpgDBManager::SEARCH_RESULT_EVENT>& val) {
							vector<const EPGDB_EVENT_INFO*> valp;
							valp.reserve(val.size());
							for( size_t i = 0; i < val.size(); valp.push_back(val[i++].info) );
							resParam.data = NewWriteVALUE2WithVersion(ver, valp, resParam.dataSize);
							resParam.param = CMD_SUCCESS;
						});
					}
				}
			}
			return true;
		}
		break;
	case CMD2_EPG_SRV_SEARCH_PG_BYKEY2:
		if( g_compatFlags & 0x20 ){
			//互換動作: 番組検索の追加コマンドを実装する
			OutputDebugString(L"CMD2_EPG_SRV_SEARCH_PG_BYKEY2\r\n");
			if( this->epgDB.IsInitialLoadingDataDone() == FALSE ){
				resParam.param = CMD_ERR_BUSY;
			}else{
				WORD ver;
				DWORD readSize;
				if( ReadVALUE(&ver, cmdParam.data, cmdParam.dataSize, &readSize) ){
					vector<EPGDB_SEARCH_KEY_INFO> key;
					if( ReadVALUE2(ver, &key, cmdParam.data.get() + readSize, cmdParam.dataSize - readSize, NULL) ){
						vector<vector<CEpgDBManager::SEARCH_RESULT_EVENT_DATA>> byResult(key.size());
						vector<const EPGDB_EVENT_INFO*> valp;
						EPGDB_EVENT_INFO dummy;
						dummy.original_network_id = 0;
						dummy.transport_stream_id = 0;
						dummy.service_id = 0;
						dummy.event_id = 0;
						GetSystemTime(&dummy.start_time);
						for( size_t i = 0; i < key.size(); i++ ){
							vector<EPGDB_SEARCH_KEY_INFO> byKey(1, key[i]);
							if( this->epgDB.SearchEpg(&byKey, &byResult[i]) ){
								for( size_t j = 0; j < byResult[i].size(); valp.push_back(&byResult[i][j++].info) );
							}
							valp.push_back(&dummy);
						}
						resParam.data = NewWriteVALUE2WithVersion(ver, valp, resParam.dataSize);
						resParam.param = CMD_SUCCESS;
					}
				}
			}
			return true;
		}
		break;
	case CMD2_EPG_SRV_GET_RECINFO_LIST2:
		if( g_compatFlags & 0x40 ){
			//互換動作: リスト指定の録画済み一覧取得コマンドを実装する
			OutputDebugString(L"CMD2_EPG_SRV_GET_RECINFO_LIST2\r\n");
			WORD ver;
			DWORD readSize;
			if( ReadVALUE(&ver, cmdParam.data, cmdParam.dataSize, &readSize) ){
				vector<DWORD> idList;
				if( ReadVALUE2(ver, &idList, cmdParam.data.get() + readSize, cmdParam.dataSize - readSize, NULL) ){
					vector<REC_FILE_INFO> listA = this->reserveManager.GetRecFileInfoAll(false);
					vector<REC_FILE_INFO> list;
					REC_FILE_INFO info;
					for( size_t i = 0; i < idList.size(); i++ ){
						info.id = idList[i];
						vector<REC_FILE_INFO>::const_iterator itr =
							std::lower_bound(listA.begin(), listA.end(), info, [](const REC_FILE_INFO& a, const REC_FILE_INFO& b) { return a.id < b.id; });
						if( itr != listA.end() && itr->id == info.id ){
							list.push_back(*itr);
						}
					}
					for( size_t i = 0; i < list.size(); i++ ){
						if( this->reserveManager.GetRecFileInfo(list[i].id, &info) ){
							list[i].programInfo = info.programInfo;
							list[i].errInfo = info.errInfo;
						}
					}
					resParam.data = NewWriteVALUE2WithVersion(ver, list, resParam.dataSize);
					resParam.param = CMD_SUCCESS;
				}
			}
			return true;
		}
		break;
	case CMD2_EPG_SRV_FILE_COPY2:
		if( g_compatFlags & 0x80 ){
			//互換動作: 指定ファイルをまとめて転送するコマンドを実装する
			OutputDebugString(L"CMD2_EPG_SRV_FILE_COPY2\r\n");
			WORD ver;
			DWORD readSize;
			if( ReadVALUE(&ver, cmdParam.data, cmdParam.dataSize, &readSize) ){
				vector<wstring> list;
				if( ReadVALUE2(ver, &list, cmdParam.data.get() + readSize, cmdParam.dataSize - readSize, NULL) && list.size() < 32 ){
					vector<FILE_DATA> result(list.size());
					for( size_t i = 0; i < list.size(); i++ ){
						result[i].Name = list[i];
						wstring path;
						if( CompareNoCase(list[i], L"ChSet5.txt") == 0 ){
							GetSettingPath(path);
							path += L'\\' + list[i];
						}else if( CompareNoCase(list[i], L"EpgTimerSrv.ini") == 0 ||
						          CompareNoCase(list[i], L"Common.ini") == 0 ||
						          CompareNoCase(list[i], L"EpgDataCap_Bon.ini") == 0 ||
						          CompareNoCase(list[i], L"BonCtrl.ini") == 0 ||
						          CompareNoCase(list[i], L"Bitrate.ini") == 0 ){
							GetModuleFolderPath(path);
							path += L'\\' + list[i];
						}
						if( path.empty() == false ){
							HANDLE hFile = CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
							if( hFile != INVALID_HANDLE_VALUE ){
								DWORD dwFileSize = GetFileSize(hFile, NULL);
								if( dwFileSize != INVALID_FILE_SIZE && dwFileSize != 0 && dwFileSize < 16 * 1024 * 1024 ){
									result[i].Data.resize(dwFileSize);
									DWORD dwRead;
									if( ReadFile(hFile, &result[i].Data.front(), dwFileSize, &dwRead, NULL) == FALSE || dwRead != dwFileSize ){
										result[i].Data.clear();
									}
								}
								CloseHandle(hFile);
							}
						}
					}
					resParam.data = NewWriteVALUE2WithVersion(ver, result, resParam.dataSize);
					resParam.param = CMD_SUCCESS;
				}
			}
			return true;
		}
		break;
	}
	return false;
}

int CEpgTimerSrvMain::InitLuaCallback(lua_State* L)
{
	CEpgTimerSrvMain* sys = (CEpgTimerSrvMain*)lua_touserdata(L, -1);
	lua_newtable(L);
	LuaHelp::reg_function(L, "GetGenreName", LuaGetGenreName, sys);
	LuaHelp::reg_function(L, "GetComponentTypeName", LuaGetComponentTypeName, sys);
	LuaHelp::reg_function(L, "Sleep", LuaSleep, sys);
	LuaHelp::reg_function(L, "Convert", LuaConvert, sys);
	LuaHelp::reg_function(L, "GetPrivateProfile", LuaGetPrivateProfile, sys);
	LuaHelp::reg_function(L, "WritePrivateProfile", LuaWritePrivateProfile, sys);
	LuaHelp::reg_function(L, "ReloadEpg", LuaReloadEpg, sys);
	LuaHelp::reg_function(L, "ReloadSetting", LuaReloadSetting, sys);
	LuaHelp::reg_function(L, "EpgCapNow", LuaEpgCapNow, sys);
	LuaHelp::reg_function(L, "GetChDataList", LuaGetChDataList, sys);
	LuaHelp::reg_function(L, "GetServiceList", LuaGetServiceList, sys);
	LuaHelp::reg_function(L, "GetEventMinMaxTime", LuaGetEventMinMaxTime, sys);
	LuaHelp::reg_function(L, "EnumEventInfo", LuaEnumEventInfo, sys);
	LuaHelp::reg_function(L, "SearchEpg", LuaSearchEpg, sys);
	LuaHelp::reg_function(L, "AddReserveData", LuaAddReserveData, sys);
	LuaHelp::reg_function(L, "ChgReserveData", LuaChgReserveData, sys);
	LuaHelp::reg_function(L, "DelReserveData", LuaDelReserveData, sys);
	LuaHelp::reg_function(L, "GetReserveData", LuaGetReserveData, sys);
	LuaHelp::reg_function(L, "GetRecFilePath", LuaGetRecFilePath, sys);
	LuaHelp::reg_function(L, "GetRecFileInfo", LuaGetRecFileInfo, sys);
	LuaHelp::reg_function(L, "GetRecFileInfoBasic", LuaGetRecFileInfoBasic, sys);
	LuaHelp::reg_function(L, "ChgProtectRecFileInfo", LuaChgProtectRecFileInfo, sys);
	LuaHelp::reg_function(L, "DelRecFileInfo", LuaDelRecFileInfo, sys);
	LuaHelp::reg_function(L, "GetTunerReserveAll", LuaGetTunerReserveAll, sys);
	LuaHelp::reg_function(L, "EnumRecPresetInfo", LuaEnumRecPresetInfo, sys);
	LuaHelp::reg_function(L, "EnumAutoAdd", LuaEnumAutoAdd, sys);
	LuaHelp::reg_function(L, "EnumManuAdd", LuaEnumManuAdd, sys);
	LuaHelp::reg_function(L, "DelAutoAdd", LuaDelAutoAdd, sys);
	LuaHelp::reg_function(L, "DelManuAdd", LuaDelManuAdd, sys);
	LuaHelp::reg_function(L, "AddOrChgAutoAdd", LuaAddOrChgAutoAdd, sys);
	LuaHelp::reg_function(L, "AddOrChgManuAdd", LuaAddOrChgManuAdd, sys);
	LuaHelp::reg_function(L, "GetNotifyUpdateCount", LuaGetNotifyUpdateCount, sys);
	LuaHelp::reg_function(L, "ListDmsPublicFile", LuaListDmsPublicFile, sys);
	LuaHelp::reg_int(L, "htmlEscape", 0);
	LuaHelp::reg_string(L, "serverRandom", sys->httpServerRandom.c_str());
	lua_setglobal(L, "edcb");
	return 0;
}

#if 1
//Lua-edcb空間のコールバック

CEpgTimerSrvMain::CLuaWorkspace::CLuaWorkspace(lua_State* L_)
	: L(L_)
	, sys((CEpgTimerSrvMain*)lua_touserdata(L, lua_upvalueindex(1)))
{
	lua_getglobal(L, "edcb");
	this->htmlEscape = LuaHelp::get_int(L, "htmlEscape");
	lua_pop(L, 1);
}

const char* CEpgTimerSrvMain::CLuaWorkspace::WtoUTF8(const wstring& strIn)
{
	::WtoUTF8(strIn.c_str(), strIn.size(), this->strOut);
	if( this->htmlEscape != 0 ){
		LPCSTR rpl[] = { "&amp;", "<lt;", ">gt;", "\"quot;", "'apos;" };
		for( size_t i = 0; this->strOut[i] != '\0'; i++ ){
			for( int j = 0; j < 5; j++ ){
				if( rpl[j][0] == this->strOut[i] && (this->htmlEscape >> j & 1) ){
					this->strOut[i] = '&';
					this->strOut.insert(this->strOut.begin() + i + 1, rpl[j] + 1, rpl[j] + lstrlenA(rpl[j]));
					break;
				}
			}
		}
	}
	return &this->strOut[0];
}

int CEpgTimerSrvMain::LuaGetGenreName(lua_State* L)
{
	CLuaWorkspace ws(L);
	wstring name;
	if( lua_gettop(L) == 1 ){
		GetGenreName(lua_tointeger(L, -1) >> 8 & 0xFF, lua_tointeger(L, -1) & 0xFF, name);
	}
	lua_pushstring(L, ws.WtoUTF8(name));
	return 1;
}

int CEpgTimerSrvMain::LuaGetComponentTypeName(lua_State* L)
{
	CLuaWorkspace ws(L);
	wstring name;
	if( lua_gettop(L) == 1 ){
		GetComponentTypeName(lua_tointeger(L, -1) >> 8 & 0xFF, lua_tointeger(L, -1) & 0xFF, name);
	}
	lua_pushstring(L, ws.WtoUTF8(name));
	return 1;
}

int CEpgTimerSrvMain::LuaSleep(lua_State* L)
{
	Sleep((DWORD)lua_tointeger(L, 1));
	return 0;
}

int CEpgTimerSrvMain::LuaConvert(lua_State* L)
{
	CLuaWorkspace ws(L);
	if( lua_gettop(L) == 3 ){
		LPCSTR to = lua_tostring(L, 1);
		LPCSTR from = lua_tostring(L, 2);
		LPCSTR src = lua_tostring(L, 3);
		if( to && from && src ){
			wstring wsrc;
			if( _stricmp(from, "utf-8") == 0 ){
				UTF8toW(src, wsrc);
			}else if( _stricmp(from, "cp932") == 0 ){
				AtoW(src, wsrc);
			}else{
				lua_pushnil(L);
				return 1;
			}
			if( _stricmp(to, "utf-8") == 0 ){
				lua_pushstring(L, ws.WtoUTF8(wsrc));
				return 1;
			}else if( _stricmp(to, "cp932") == 0 ){
				UTF8toW(ws.WtoUTF8(wsrc), wsrc);
				string dest;
				WtoA(wsrc, dest);
				lua_pushstring(L, dest.c_str());
				return 1;
			}
		}
	}
	lua_pushnil(L);
	return 1;
}

int CEpgTimerSrvMain::LuaGetPrivateProfile(lua_State* L)
{
	CLuaWorkspace ws(L);
	if( lua_gettop(L) == 4 ){
		LPCSTR app = lua_tostring(L, 1);
		LPCSTR key = lua_tostring(L, 2);
		LPCSTR def = lua_isboolean(L, 3) ? (lua_toboolean(L, 3) ? "1" : "0") : lua_tostring(L, 3);
		LPCSTR file = lua_tostring(L, 4);
		if( app && key && def && file ){
			wstring path;
			if( _stricmp(key, "ModulePath") == 0 && _stricmp(app, "SET") == 0 && _stricmp(file, "Common.ini") == 0 ){
				GetModuleFolderPath(path);
				lua_pushstring(L, ws.WtoUTF8(path));
			}else{
				wstring strApp;
				wstring strKey;
				wstring strDef;
				wstring strFile;
				UTF8toW(app, strApp);
				UTF8toW(key, strKey);
				UTF8toW(def, strDef);
				UTF8toW(file, strFile);
				if( _wcsicmp(strFile.substr(0, 8).c_str(), L"Setting\\") == 0 ){
					GetSettingPath(path);
					strFile = path + strFile.substr(7);
				}else{
					GetModuleFolderPath(path);
					strFile = path + L"\\" + strFile;
				}
				wstring buff = GetPrivateProfileToString(strApp.c_str(), strKey.c_str(), strDef.c_str(), strFile.c_str());
				lua_pushstring(L, ws.WtoUTF8(buff));
			}
			return 1;
		}
	}
	lua_pushstring(L, "");
	return 1;
}

int CEpgTimerSrvMain::LuaWritePrivateProfile(lua_State* L)
{
	if( lua_gettop(L) == 4 ){
		LPCSTR app = lua_tostring(L, 1);
		LPCSTR key = lua_tostring(L, 2);
		LPCSTR val = lua_isboolean(L, 3) ? (lua_toboolean(L, 3) ? "1" : "0") : lua_tostring(L, 3);
		LPCSTR file = lua_tostring(L, 4);
		if( app && file ){
			wstring strApp;
			wstring strKey;
			wstring strVal;
			wstring strFile;
			UTF8toW(app, strApp);
			UTF8toW(key ? key : "", strKey);
			UTF8toW(val ? val : "", strVal);
			UTF8toW(file, strFile);
			wstring path;
			if( _wcsicmp(strFile.substr(0, 8).c_str(), L"Setting\\") == 0 ){
				GetSettingPath(path);
				strFile = path + strFile.substr(7);
			}else{
				GetModuleFolderPath(path);
				strFile = path + L"\\" + strFile;
			}
			lua_pushboolean(L, WritePrivateProfileString(strApp.c_str(), key ? strKey.c_str() : NULL, val ? strVal.c_str() : NULL, strFile.c_str()));
			return 1;
		}
	}
	lua_pushboolean(L, false);
	return 1;
}

int CEpgTimerSrvMain::LuaReloadEpg(lua_State* L)
{
	CLuaWorkspace ws(L);
	if( ws.sys->epgDB.IsLoadingData() == FALSE && ws.sys->epgDB.ReloadEpgData() ){
		PostMessage(ws.sys->hwndMain, WM_RELOAD_EPG_CHK, 0, 0);
		lua_pushboolean(L, true);
		return 1;
	}
	lua_pushboolean(L, false);
	return 1;
}

int CEpgTimerSrvMain::LuaReloadSetting(lua_State* L)
{
	CLuaWorkspace ws(L);
	ws.sys->ReloadSetting();
	if( lua_gettop(L) == 1 && lua_toboolean(L, 1) ){
		ws.sys->ReloadNetworkSetting();
	}
	return 0;
}

int CEpgTimerSrvMain::LuaEpgCapNow(lua_State* L)
{
	CLuaWorkspace ws(L);
	lua_pushboolean(L, ws.sys->epgDB.IsInitialLoadingDataDone() && ws.sys->reserveManager.RequestStartEpgCap());
	return 1;
}

int CEpgTimerSrvMain::LuaGetChDataList(lua_State* L)
{
	CLuaWorkspace ws(L);
	vector<CH_DATA5> list = ws.sys->reserveManager.GetChDataList();
	lua_newtable(L);
	for( size_t i = 0; i < list.size(); i++ ){
		lua_newtable(L);
		LuaHelp::reg_int(L, "onid", list[i].originalNetworkID);
		LuaHelp::reg_int(L, "tsid", list[i].transportStreamID);
		LuaHelp::reg_int(L, "sid", list[i].serviceID);
		LuaHelp::reg_int(L, "serviceType", list[i].serviceType);
		LuaHelp::reg_boolean(L, "partialFlag", list[i].partialFlag != 0);
		LuaHelp::reg_string(L, "serviceName", ws.WtoUTF8(list[i].serviceName));
		LuaHelp::reg_string(L, "networkName", ws.WtoUTF8(list[i].networkName));
		LuaHelp::reg_boolean(L, "epgCapFlag", list[i].epgCapFlag != 0);
		LuaHelp::reg_boolean(L, "searchFlag", list[i].searchFlag != 0);
		lua_rawseti(L, -2, (int)i + 1);
	}
	return 1;
}

int CEpgTimerSrvMain::LuaGetServiceList(lua_State* L)
{
	CLuaWorkspace ws(L);
	vector<EPGDB_SERVICE_INFO> list;
	if( ws.sys->epgDB.GetServiceList(&list) != FALSE ){
		lua_newtable(L);
		for( size_t i = 0; i < list.size(); i++ ){
			lua_newtable(L);
			LuaHelp::reg_int(L, "onid", list[i].ONID);
			LuaHelp::reg_int(L, "tsid", list[i].TSID);
			LuaHelp::reg_int(L, "sid", list[i].SID);
			LuaHelp::reg_int(L, "service_type", list[i].service_type);
			LuaHelp::reg_boolean(L, "partialReceptionFlag", list[i].partialReceptionFlag != 0);
			LuaHelp::reg_string(L, "service_provider_name", ws.WtoUTF8(list[i].service_provider_name));
			LuaHelp::reg_string(L, "service_name", ws.WtoUTF8(list[i].service_name));
			LuaHelp::reg_string(L, "network_name", ws.WtoUTF8(list[i].network_name));
			LuaHelp::reg_string(L, "ts_name", ws.WtoUTF8(list[i].ts_name));
			LuaHelp::reg_int(L, "remote_control_key_id", list[i].remote_control_key_id);
			lua_rawseti(L, -2, (int)i + 1);
		}
		return 1;
	}
	lua_pushnil(L);
	return 1;
}

int CEpgTimerSrvMain::LuaGetEventMinMaxTime(lua_State* L)
{
	CLuaWorkspace ws(L);
	if( lua_gettop(L) == 3 ){
		__int64 minMaxTime[2] = { LLONG_MAX, LLONG_MIN };
		ws.sys->epgDB.EnumEventInfo(_Create64Key(
			(WORD)lua_tointeger(L, 1), (WORD)lua_tointeger(L, 2), (WORD)lua_tointeger(L, 3)),
			[&minMaxTime](const vector<EPGDB_EVENT_INFO>& val) {
			for( size_t i = 0; i < val.size(); i++ ){
				if( val[i].StartTimeFlag ){
					__int64 startTime = ConvertI64Time(val[i].start_time);
					minMaxTime[0] = min(minMaxTime[0], startTime);
					minMaxTime[1] = max(minMaxTime[1], startTime);
				}
			}
		});
		if( minMaxTime[0] != LLONG_MAX ){
			lua_newtable(ws.L);
			SYSTEMTIME st;
			ConvertSystemTime(minMaxTime[0], &st);
			LuaHelp::reg_time(L, "minTime", st);
			ConvertSystemTime(minMaxTime[1], &st);
			LuaHelp::reg_time(L, "maxTime", st);
			return 1;
		}
	}
	lua_pushnil(L);
	return 1;
}

int CEpgTimerSrvMain::LuaEnumEventInfo(lua_State* L)
{
	CLuaWorkspace ws(L);
	if( lua_gettop(L) >= 1 && lua_istable(L, 1) ){
		vector<int> key;
		__int64 enumStart = 0;
		__int64 enumEnd = LLONG_MAX;
		if( lua_gettop(L) == 2 && lua_istable(L, -1) ){
			if( LuaHelp::isnil(L, "startTime") ){
				enumStart = LLONG_MAX;
			}else{
				enumStart = ConvertI64Time(LuaHelp::get_time(L, "startTime"));
				enumEnd = enumStart + LuaHelp::get_int(L, "durationSecond") * I64_1SEC;
			}
			lua_pop(L, 1);
		}
		for( int i = 0;; i++ ){
			lua_rawgeti(L, -1, i + 1);
			if( !lua_istable(L, -1) ){
				lua_pop(L, 1);
				break;
			}
			key.push_back(LuaHelp::isnil(L, "onid") ? -1 : LuaHelp::get_int(L, "onid"));
			key.push_back(LuaHelp::isnil(L, "tsid") ? -1 : LuaHelp::get_int(L, "tsid"));
			key.push_back(LuaHelp::isnil(L, "sid") ? -1 : LuaHelp::get_int(L, "sid"));
			lua_pop(L, 1);
		}
		BOOL ret = ws.sys->epgDB.EnumEventAll([=, &ws, &key](const map<LONGLONG, EPGDB_SERVICE_EVENT_INFO>& val) {
			lua_newtable(ws.L);
			int n = 0;
			for( auto itr = val.cbegin(); itr != val.end(); itr++ ){
				for( size_t i = 0; i + 2 < key.size(); i += 3 ){
					if( (key[i] < 0 || key[i] == itr->second.serviceInfo.ONID) &&
					    (key[i+1] < 0 || key[i+1] == itr->second.serviceInfo.TSID) &&
					    (key[i+2] < 0 || key[i+2] == itr->second.serviceInfo.SID) ){
						for( size_t j = 0; j < itr->second.eventList.size(); j++ ){
							if( (enumStart != 0 || enumEnd != LLONG_MAX) && (enumStart != LLONG_MAX || itr->second.eventList[j].StartTimeFlag) ){
								if( itr->second.eventList[j].StartTimeFlag == 0 ){
									continue;
								}
								__int64 startTime = ConvertI64Time(itr->second.eventList[j].start_time);
								if( startTime < enumStart || enumEnd <= startTime ){
									continue;
								}
							}
							lua_newtable(ws.L);
							PushEpgEventInfo(ws, itr->second.eventList[j]);
							lua_rawseti(ws.L, -2, ++n);
						}
						break;
					}
				}
			}
		});
		if( ret ){
			return 1;
		}
	}
	lua_pushnil(L);
	return 1;
}

int CEpgTimerSrvMain::LuaSearchEpg(lua_State* L)
{
	CLuaWorkspace ws(L);
	if( lua_gettop(L) == 4 ){
		EPGDB_EVENT_INFO info;
		if( ws.sys->epgDB.SearchEpg((WORD)lua_tointeger(L, 1), (WORD)lua_tointeger(L, 2), (WORD)lua_tointeger(L, 3), (WORD)lua_tointeger(L, 4), &info) != FALSE ){
			lua_newtable(ws.L);
			PushEpgEventInfo(ws, info);
			return 1;
		}
	}else if( lua_gettop(L) == 1 && lua_istable(L, -1) ){
		vector<EPGDB_SEARCH_KEY_INFO> keyList(1);
		EPGDB_SEARCH_KEY_INFO& key = keyList.back();
		FetchEpgSearchKeyInfo(ws, key);
		//対象ネットワーク
		vector<EPGDB_SERVICE_INFO> list;
		int network = LuaHelp::get_int(L, "network");
		if( network != 0 && ws.sys->epgDB.GetServiceList(&list) != FALSE ){
			for( size_t i = 0; i < list.size(); i++ ){
				WORD onid = list[i].ONID;
				if( (network & 1) && 0x7880 <= onid && onid <= 0x7FE8 || //地デジ
				    (network & 2) && onid == 4 || //BS
				    (network & 4) && (onid == 6 || onid == 7) || //CS
				    (network & 8) && ((onid < 0x7880 || 0x7FE8 < onid) && onid != 4 && onid != 6 && onid != 7) //その他
				    ){
					LONGLONG id = _Create64Key(onid, list[i].TSID, list[i].SID);
					if( std::find(key.serviceList.begin(), key.serviceList.end(), id) == key.serviceList.end() ){
						key.serviceList.push_back(id);
					}
				}
			}
		}
		BOOL ret = ws.sys->epgDB.SearchEpg(&keyList, [&ws](vector<CEpgDBManager::SEARCH_RESULT_EVENT>& val) {
			SYSTEMTIME now;
			GetLocalTime(&now);
			now.wHour = 0;
			now.wMinute = 0;
			now.wSecond = 0;
			now.wMilliseconds = 0;
			//対象期間
			__int64 chkTime = LuaHelp::get_int(ws.L, "days") * 24 * 60 * 60 * I64_1SEC;
			if( chkTime > 0 ){
				chkTime += ConvertI64Time(now);
			}else{
				//たぶんバグだが互換のため
				chkTime = LuaHelp::get_int(ws.L, "days29") * 29 * 60 * 60 * I64_1SEC;
				if( chkTime > 0 ){
					chkTime += ConvertI64Time(now);
				}
			}
			lua_newtable(ws.L);
			int n = 0;
			for( size_t i = 0; i < val.size(); i++ ){
				if( chkTime <= 0 || val[i].info->StartTimeFlag != 0 && chkTime > ConvertI64Time(val[i].info->start_time) ){
					//イベントグループはチェックしないので注意
					lua_newtable(ws.L);
					PushEpgEventInfo(ws, *val[i].info);
					lua_rawseti(ws.L, -2, ++n);
				}
			}
		});
		if( ret ){
			return 1;
		}
	}
	lua_pushnil(L);
	return 1;
}

int CEpgTimerSrvMain::LuaAddReserveData(lua_State* L)
{
	CLuaWorkspace ws(L);
	if( lua_gettop(L) == 1 && lua_istable(L, -1) ){
		vector<RESERVE_DATA> list(1);
		if( FetchReserveData(ws, list.back()) && ws.sys->reserveManager.AddReserveData(list) ){
			lua_pushboolean(L, true);
			return 1;
		}
	}
	lua_pushboolean(L, false);
	return 1;
}

int CEpgTimerSrvMain::LuaChgReserveData(lua_State* L)
{
	CLuaWorkspace ws(L);
	if( lua_gettop(L) == 1 && lua_istable(L, -1) ){
		vector<RESERVE_DATA> list(1);
		if( FetchReserveData(ws, list.back()) && ws.sys->reserveManager.ChgReserveData(list) ){
			lua_pushboolean(L, true);
			return 1;
		}
	}
	lua_pushboolean(L, false);
	return 1;
}

int CEpgTimerSrvMain::LuaDelReserveData(lua_State* L)
{
	CLuaWorkspace ws(L);
	if( lua_gettop(L) == 1 ){
		vector<DWORD> list(1, (DWORD)lua_tointeger(L, -1));
		ws.sys->reserveManager.DelReserveData(list);
	}
	return 0;
}

int CEpgTimerSrvMain::LuaGetReserveData(lua_State* L)
{
	CLuaWorkspace ws(L);
	if( lua_gettop(L) == 0 ){
		lua_newtable(L);
		vector<RESERVE_DATA> list = ws.sys->reserveManager.GetReserveDataAll();
		for( size_t i = 0; i < list.size(); i++ ){
			lua_newtable(L);
			PushReserveData(ws, list[i]);
			lua_rawseti(L, -2, (int)i + 1);
		}
		return 1;
	}else{
		RESERVE_DATA r;
		if( ws.sys->reserveManager.GetReserveData((DWORD)lua_tointeger(L, 1), &r) ){
			lua_newtable(L);
			PushReserveData(ws, r);
			return 1;
		}
	}
	lua_pushnil(L);
	return 1;
}

int CEpgTimerSrvMain::LuaGetRecFilePath(lua_State* L)
{
	CLuaWorkspace ws(L);
	if( lua_gettop(L) == 1 ){
		wstring filePath;
		if( ws.sys->reserveManager.GetRecFilePath((DWORD)lua_tointeger(L, 1), filePath) ){
			lua_pushstring(L, ws.WtoUTF8(filePath));
			return 1;
		}
	}
	lua_pushnil(L);
	return 1;
}

int CEpgTimerSrvMain::LuaGetRecFileInfo(lua_State* L)
{
	return LuaGetRecFileInfoProc(L, true);
}

int CEpgTimerSrvMain::LuaGetRecFileInfoBasic(lua_State* L)
{
	return LuaGetRecFileInfoProc(L, false);
}

int CEpgTimerSrvMain::LuaGetRecFileInfoProc(lua_State* L, bool getExtraInfo)
{
	CLuaWorkspace ws(L);
	bool getAll = lua_gettop(L) == 0;
	vector<REC_FILE_INFO> list;
	if( getAll ){
		lua_newtable(L);
		list = ws.sys->reserveManager.GetRecFileInfoAll(getExtraInfo);
	}else{
		DWORD id = (DWORD)lua_tointeger(L, 1);
		list.resize(1);
		if( ws.sys->reserveManager.GetRecFileInfo(id, &list.front(), getExtraInfo) == false ){
			lua_pushnil(L);
			return 1;
		}
	}
	for( size_t i = 0; i < list.size(); i++ ){
		const REC_FILE_INFO& r = list[i];
		{
			lua_newtable(L);
			LuaHelp::reg_int(L, "id", (int)r.id);
			LuaHelp::reg_string(L, "recFilePath", ws.WtoUTF8(r.recFilePath));
			LuaHelp::reg_string(L, "title", ws.WtoUTF8(r.title));
			LuaHelp::reg_time(L, "startTime", r.startTime);
			LuaHelp::reg_int(L, "durationSecond", (int)r.durationSecond);
			LuaHelp::reg_string(L, "serviceName", ws.WtoUTF8(r.serviceName));
			LuaHelp::reg_int(L, "onid", r.originalNetworkID);
			LuaHelp::reg_int(L, "tsid", r.transportStreamID);
			LuaHelp::reg_int(L, "sid", r.serviceID);
			LuaHelp::reg_int(L, "eid", r.eventID);
			LuaHelp::reg_int(L, "drops", (int)r.drops);
			LuaHelp::reg_int(L, "scrambles", (int)r.scrambles);
			LuaHelp::reg_int(L, "recStatus", (int)r.recStatus);
			LuaHelp::reg_time(L, "startTimeEpg", r.startTimeEpg);
			LuaHelp::reg_string(L, "comment", ws.WtoUTF8(r.comment));
			LuaHelp::reg_string(L, "programInfo", ws.WtoUTF8(r.programInfo));
			LuaHelp::reg_string(L, "errInfo", ws.WtoUTF8(r.errInfo));
			LuaHelp::reg_boolean(L, "protectFlag", r.protectFlag != 0);
			if( getAll == false ){
				break;
			}
			lua_rawseti(L, -2, (int)i + 1);
		}
	}
	return 1;
}

int CEpgTimerSrvMain::LuaChgProtectRecFileInfo(lua_State* L)
{
	CLuaWorkspace ws(L);
	if( lua_gettop(L) == 2 ){
		vector<REC_FILE_INFO> list(1);
		list.front().id = (DWORD)lua_tointeger(L, 1);
		list.front().protectFlag = lua_toboolean(L, 2) ? 1 : 0;
		ws.sys->reserveManager.ChgProtectRecFileInfo(list);
	}
	return 0;
}

int CEpgTimerSrvMain::LuaDelRecFileInfo(lua_State* L)
{
	CLuaWorkspace ws(L);
	if( lua_gettop(L) == 1 ){
		vector<DWORD> list(1, (DWORD)lua_tointeger(L, -1));
		ws.sys->reserveManager.DelRecFileInfo(list);
	}
	return 0;
}

int CEpgTimerSrvMain::LuaGetTunerReserveAll(lua_State* L)
{
	CLuaWorkspace ws(L);
	lua_newtable(L);
	vector<TUNER_RESERVE_INFO> list = ws.sys->reserveManager.GetTunerReserveAll();
	for( size_t i = 0; i < list.size(); i++ ){
		lua_newtable(L);
		LuaHelp::reg_int(L, "tunerID", (int)list[i].tunerID);
		LuaHelp::reg_string(L, "tunerName", ws.WtoUTF8(list[i].tunerName));
		lua_pushstring(L, "reserveList");
		lua_newtable(L);
		for( size_t j = 0; j < list[i].reserveList.size(); j++ ){
			lua_pushinteger(L, (int)list[i].reserveList[j]);
			lua_rawseti(L, -2, (int)j + 1);
		}
		lua_rawset(L, -3);
		lua_rawseti(L, -2, (int)i + 1);
	}
	return 1;
}

int CEpgTimerSrvMain::LuaEnumRecPresetInfo(lua_State* L)
{
	CLuaWorkspace ws(L);
	lua_newtable(L);
	wstring iniPath;
	GetModuleIniPath(iniPath);
	wstring parseBuff = GetPrivateProfileToString(L"SET", L"PresetID", L"", iniPath.c_str());
	vector<WORD> idList(1, 0);
	while( parseBuff.empty() == false ){
		wstring presetID;
		Separate(parseBuff, L",", presetID, parseBuff);
		idList.push_back((WORD)_wtoi(presetID.c_str()));
	}
	std::sort(idList.begin(), idList.end());
	idList.erase(std::unique(idList.begin(), idList.end()), idList.end());
	for( size_t i = 0; i < idList.size(); i++ ){
		lua_newtable(L);
		pair<wstring, REC_SETTING_DATA> ret = ws.sys->LoadRecSetData(idList[i]);
		LuaHelp::reg_int(L, "id", idList[i]);
		LuaHelp::reg_string(L, "name", ws.WtoUTF8(ret.first));
		lua_pushstring(L, "recSetting");
		lua_newtable(L);
		PushRecSettingData(ws, ret.second);
		lua_rawset(L, -3);
		lua_rawseti(L, -2, (int)i + 1);
	}
	return 1;
}

int CEpgTimerSrvMain::LuaEnumAutoAdd(lua_State* L)
{
	CLuaWorkspace ws(L);
	CBlockLock lock(&ws.sys->settingLock);
	lua_newtable(L);
	int i = 0;
	for( map<DWORD, EPG_AUTO_ADD_DATA>::const_iterator itr = ws.sys->epgAutoAdd.GetMap().begin(); itr != ws.sys->epgAutoAdd.GetMap().end(); itr++, i++ ){
		lua_newtable(L);
		LuaHelp::reg_int(L, "dataID", itr->second.dataID);
		LuaHelp::reg_int(L, "addCount", itr->second.addCount);
		lua_pushstring(L, "searchInfo");
		lua_newtable(L);
		PushEpgSearchKeyInfo(ws, itr->second.searchInfo);
		lua_rawset(L, -3);
		lua_pushstring(L, "recSetting");
		lua_newtable(L);
		PushRecSettingData(ws, itr->second.recSetting);
		lua_rawset(L, -3);
		lua_rawseti(L, -2, (int)i + 1);
	}
	return 1;
}

int CEpgTimerSrvMain::LuaEnumManuAdd(lua_State* L)
{
	CLuaWorkspace ws(L);
	CBlockLock lock(&ws.sys->settingLock);
	lua_newtable(L);
	int i = 0;
	for( map<DWORD, MANUAL_AUTO_ADD_DATA>::const_iterator itr = ws.sys->manualAutoAdd.GetMap().begin(); itr != ws.sys->manualAutoAdd.GetMap().end(); itr++, i++ ){
		lua_newtable(L);
		LuaHelp::reg_int(L, "dataID", itr->second.dataID);
		LuaHelp::reg_int(L, "dayOfWeekFlag", itr->second.dayOfWeekFlag);
		LuaHelp::reg_int(L, "startTime", itr->second.startTime);
		LuaHelp::reg_int(L, "durationSecond", itr->second.durationSecond);
		LuaHelp::reg_string(L, "title", ws.WtoUTF8(itr->second.title));
		LuaHelp::reg_string(L, "stationName", ws.WtoUTF8(itr->second.stationName));
		LuaHelp::reg_int(L, "onid", itr->second.originalNetworkID);
		LuaHelp::reg_int(L, "tsid", itr->second.transportStreamID);
		LuaHelp::reg_int(L, "sid", itr->second.serviceID);
		lua_pushstring(L, "recSetting");
		lua_newtable(L);
		PushRecSettingData(ws, itr->second.recSetting);
		lua_rawset(L, -3);
		lua_rawseti(L, -2, (int)i + 1);
	}
	return 1;
}

int CEpgTimerSrvMain::LuaDelAutoAdd(lua_State* L)
{
	CLuaWorkspace ws(L);
	if( lua_gettop(L) == 1 ){
		CBlockLock lock(&ws.sys->settingLock);
		if( ws.sys->epgAutoAdd.DelData((DWORD)lua_tointeger(L, -1)) ){
			ws.sys->epgAutoAdd.SaveText();
			ws.sys->notifyManager.AddNotify(NOTIFY_UPDATE_AUTOADD_EPG);
		}
	}
	return 0;
}

int CEpgTimerSrvMain::LuaDelManuAdd(lua_State* L)
{
	CLuaWorkspace ws(L);
	if( lua_gettop(L) == 1 ){
		CBlockLock lock(&ws.sys->settingLock);
		if( ws.sys->manualAutoAdd.DelData((DWORD)lua_tointeger(L, -1)) ){
			ws.sys->manualAutoAdd.SaveText();
			ws.sys->notifyManager.AddNotify(NOTIFY_UPDATE_AUTOADD_MANUAL);
		}
	}
	return 0;
}

int CEpgTimerSrvMain::LuaAddOrChgAutoAdd(lua_State* L)
{
	CLuaWorkspace ws(L);
	if( lua_gettop(L) == 1 && lua_istable(L, -1) ){
		EPG_AUTO_ADD_DATA item;
		item.dataID = LuaHelp::get_int(L, "dataID");
		lua_getfield(L, -1, "searchInfo");
		if( lua_istable(L, -1) ){
			FetchEpgSearchKeyInfo(ws, item.searchInfo);
			lua_getfield(L, -2, "recSetting");
			if( lua_istable(L, -1) ){
				FetchRecSettingData(ws, item.recSetting);
				bool modified = true;
				{
					CBlockLock lock(&ws.sys->settingLock);
					if( item.dataID == 0 ){
						item.dataID = ws.sys->epgAutoAdd.AddData(item);
					}else{
						modified = ws.sys->epgAutoAdd.ChgData(item);
					}
					if( modified ){
						ws.sys->epgAutoAdd.SaveText();
					}
				}
				if( modified ){
					ws.sys->AutoAddReserveEPG(item);
					ws.sys->notifyManager.AddNotify(NOTIFY_UPDATE_AUTOADD_EPG);
					lua_pushboolean(L, true);
					return 1;
				}
			}
		}
	}
	lua_pushboolean(L, false);
	return 1;
}

int CEpgTimerSrvMain::LuaAddOrChgManuAdd(lua_State* L)
{
	CLuaWorkspace ws(L);
	if( lua_gettop(L) == 1 && lua_istable(L, -1) ){
		MANUAL_AUTO_ADD_DATA item;
		item.dataID = LuaHelp::get_int(L, "dataID");
		item.dayOfWeekFlag = (BYTE)LuaHelp::get_int(L, "dayOfWeekFlag");
		item.startTime = LuaHelp::get_int(L, "startTime");
		item.durationSecond = LuaHelp::get_int(L, "durationSecond");
		UTF8toW(LuaHelp::get_string(L, "title"), item.title);
		UTF8toW(LuaHelp::get_string(L, "stationName"), item.stationName);
		item.originalNetworkID = (WORD)LuaHelp::get_int(L, "onid");
		item.transportStreamID = (WORD)LuaHelp::get_int(L, "tsid");
		item.serviceID = (WORD)LuaHelp::get_int(L, "sid");
		lua_getfield(L, -1, "recSetting");
		if( lua_istable(L, -1) ){
			FetchRecSettingData(ws, item.recSetting);
			bool modified = true;
			{
				CBlockLock lock(&ws.sys->settingLock);
				if( item.dataID == 0 ){
					item.dataID = ws.sys->manualAutoAdd.AddData(item);
				}else{
					modified = ws.sys->manualAutoAdd.ChgData(item);
				}
				if( modified ){
					ws.sys->manualAutoAdd.SaveText();
				}
			}
			if( modified ){
				ws.sys->AutoAddReserveProgram(item);
				ws.sys->notifyManager.AddNotify(NOTIFY_UPDATE_AUTOADD_MANUAL);
				lua_pushboolean(L, true);
				return 1;
			}
		}
	}
	lua_pushboolean(L, false);
	return 1;
}

int CEpgTimerSrvMain::LuaGetNotifyUpdateCount(lua_State* L)
{
	CLuaWorkspace ws(L);
	int n = -1;
	if( lua_gettop(L) == 1 && 1 <= lua_tointeger(L, 1) && lua_tointeger(L, 1) < _countof(ws.sys->notifyUpdateCount) ){
		n = ws.sys->notifyUpdateCount[lua_tointeger(L, 1)] & 0x7FFFFFFF;
	}
	lua_pushinteger(L, n);
	return 1;
}

int CEpgTimerSrvMain::LuaListDmsPublicFile(lua_State* L)
{
	CLuaWorkspace ws(L);
	CBlockLock lock(&ws.sys->settingLock);
	WIN32_FIND_DATA findData;
	HANDLE hFind = FindFirstFile((ws.sys->httpOptions.rootPath + L"\\dlna\\dms\\PublicFile\\*").c_str(), &findData);
	vector<pair<int, WIN32_FIND_DATA>> newList;
	if( hFind == INVALID_HANDLE_VALUE ){
		ws.sys->dmsPublicFileList.clear();
	}else{
		if( ws.sys->dmsPublicFileList.empty() ){
			//要素0には公開フォルダの場所と次のIDを格納する
			ws.sys->dmsPublicFileList.push_back(std::make_pair(0, ws.sys->httpOptions.rootPath));
		}
		do{
			//TODO: 再帰的にリストしほうがいいがとりあえず…
			if( (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 ){
				bool exist = false;
				for( size_t i = 1; i < ws.sys->dmsPublicFileList.size(); i++ ){ 
					if( lstrcmpi(ws.sys->dmsPublicFileList[i].second.c_str(), findData.cFileName) == 0 ){
						newList.push_back(std::make_pair(ws.sys->dmsPublicFileList[i].first, findData));
						exist = true;
						break;
					}
				}
				if( exist == false ){
					newList.push_back(std::make_pair(ws.sys->dmsPublicFileList[0].first++, findData));
				}
			}
		}while( FindNextFile(hFind, &findData) );
		ws.sys->dmsPublicFileList.resize(1);
	}
	lua_newtable(L);
	for( size_t i = 0; i < newList.size(); i++ ){
		//IDとファイル名の対応を記録しておく
		ws.sys->dmsPublicFileList.push_back(std::make_pair(newList[i].first, wstring(newList[i].second.cFileName)));
		lua_newtable(L);
		LuaHelp::reg_int(L, "id", newList[i].first);
		LuaHelp::reg_string(L, "name", ws.WtoUTF8(newList[i].second.cFileName));
		lua_pushstring(L, "size");
		lua_pushnumber(L, (lua_Number)((__int64)newList[i].second.nFileSizeHigh << 32 | newList[i].second.nFileSizeLow));
		lua_rawset(L, -3);
		lua_rawseti(L, -2, (int)i + 1);
	}
	return 1;
}

void CEpgTimerSrvMain::PushEpgEventInfo(CLuaWorkspace& ws, const EPGDB_EVENT_INFO& e)
{
	lua_State* L = ws.L;
	LuaHelp::reg_int(L, "onid", e.original_network_id);
	LuaHelp::reg_int(L, "tsid", e.transport_stream_id);
	LuaHelp::reg_int(L, "sid", e.service_id);
	LuaHelp::reg_int(L, "eid", e.event_id);
	if( e.StartTimeFlag ){
		LuaHelp::reg_time(L, "startTime", e.start_time);
	}
	if( e.DurationFlag ){
		LuaHelp::reg_int(L, "durationSecond", (int)e.durationSec);
	}
	LuaHelp::reg_boolean(L, "freeCAFlag", e.freeCAFlag != 0);
	if( e.shortInfo ){
		lua_pushstring(L, "shortInfo");
		lua_newtable(L);
		LuaHelp::reg_string(L, "event_name", ws.WtoUTF8(e.shortInfo->event_name));
		LuaHelp::reg_string(L, "text_char", ws.WtoUTF8(e.shortInfo->text_char));
		lua_rawset(L, -3);
	}
	if( e.extInfo ){
		lua_pushstring(L, "extInfo");
		lua_newtable(L);
		LuaHelp::reg_string(L, "text_char", ws.WtoUTF8(e.extInfo->text_char));
		lua_rawset(L, -3);
	}
	if( e.contentInfo ){
		lua_pushstring(L, "contentInfoList");
		lua_newtable(L);
		for( size_t i = 0; i < e.contentInfo->nibbleList.size(); i++ ){
			lua_newtable(L);
			LuaHelp::reg_int(L, "content_nibble", e.contentInfo->nibbleList[i].content_nibble_level_1 << 8 | e.contentInfo->nibbleList[i].content_nibble_level_2);
			LuaHelp::reg_int(L, "user_nibble", e.contentInfo->nibbleList[i].user_nibble_1 << 8 | e.contentInfo->nibbleList[i].user_nibble_2);
			lua_rawseti(L, -2, (int)i + 1);
		}
		lua_rawset(L, -3);
	}
	if( e.componentInfo ){
		lua_pushstring(L, "componentInfo");
		lua_newtable(L);
		LuaHelp::reg_int(L, "stream_content", e.componentInfo->stream_content);
		LuaHelp::reg_int(L, "component_type", e.componentInfo->component_type);
		LuaHelp::reg_int(L, "component_tag", e.componentInfo->component_tag);
		LuaHelp::reg_string(L, "text_char", ws.WtoUTF8(e.componentInfo->text_char));
		lua_rawset(L, -3);
	}
	if( e.audioInfo ){
		lua_pushstring(L, "audioInfoList");
		lua_newtable(L);
		for( size_t i = 0; i < e.audioInfo->componentList.size(); i++ ){
			lua_newtable(L);
			LuaHelp::reg_int(L, "stream_content", e.audioInfo->componentList[i].stream_content);
			LuaHelp::reg_int(L, "component_type", e.audioInfo->componentList[i].component_type);
			LuaHelp::reg_int(L, "component_tag", e.audioInfo->componentList[i].component_tag);
			LuaHelp::reg_int(L, "stream_type", e.audioInfo->componentList[i].stream_type);
			LuaHelp::reg_int(L, "simulcast_group_tag", e.audioInfo->componentList[i].simulcast_group_tag);
			LuaHelp::reg_boolean(L, "ES_multi_lingual_flag", e.audioInfo->componentList[i].ES_multi_lingual_flag != 0);
			LuaHelp::reg_boolean(L, "main_component_flag", e.audioInfo->componentList[i].main_component_flag != 0);
			LuaHelp::reg_int(L, "quality_indicator", e.audioInfo->componentList[i].quality_indicator);
			LuaHelp::reg_int(L, "sampling_rate", e.audioInfo->componentList[i].sampling_rate);
			LuaHelp::reg_string(L, "text_char", ws.WtoUTF8(e.audioInfo->componentList[i].text_char));
			lua_rawseti(L, -2, (int)i + 1);
		}
		lua_rawset(L, -3);
	}
	if( e.eventGroupInfo ){
		lua_pushstring(L, "eventGroupInfo");
		lua_newtable(L);
		LuaHelp::reg_int(L, "group_type", e.eventGroupInfo->group_type);
		lua_pushstring(L, "eventDataList");
		lua_newtable(L);
		for( size_t i = 0; i < e.eventGroupInfo->eventDataList.size(); i++ ){
			lua_newtable(L);
			LuaHelp::reg_int(L, "onid", e.eventGroupInfo->eventDataList[i].original_network_id);
			LuaHelp::reg_int(L, "tsid", e.eventGroupInfo->eventDataList[i].transport_stream_id);
			LuaHelp::reg_int(L, "sid", e.eventGroupInfo->eventDataList[i].service_id);
			LuaHelp::reg_int(L, "eid", e.eventGroupInfo->eventDataList[i].event_id);
			lua_rawseti(L, -2, (int)i + 1);
		}
		lua_rawset(L, -3);
		lua_rawset(L, -3);
	}
	if( e.eventRelayInfo ){
		lua_pushstring(L, "eventRelayInfo");
		lua_newtable(L);
		LuaHelp::reg_int(L, "group_type", e.eventRelayInfo->group_type);
		lua_pushstring(L, "eventDataList");
		lua_newtable(L);
		for( size_t i = 0; i < e.eventRelayInfo->eventDataList.size(); i++ ){
			lua_newtable(L);
			LuaHelp::reg_int(L, "onid", e.eventRelayInfo->eventDataList[i].original_network_id);
			LuaHelp::reg_int(L, "tsid", e.eventRelayInfo->eventDataList[i].transport_stream_id);
			LuaHelp::reg_int(L, "sid", e.eventRelayInfo->eventDataList[i].service_id);
			LuaHelp::reg_int(L, "eid", e.eventRelayInfo->eventDataList[i].event_id);
			lua_rawseti(L, -2, (int)i + 1);
		}
		lua_rawset(L, -3);
		lua_rawset(L, -3);
	}
}

void CEpgTimerSrvMain::PushReserveData(CLuaWorkspace& ws, const RESERVE_DATA& r)
{
	lua_State* L = ws.L;
	LuaHelp::reg_string(L, "title", ws.WtoUTF8(r.title));
	LuaHelp::reg_time(L, "startTime", r.startTime);
	LuaHelp::reg_int(L, "durationSecond", (int)r.durationSecond);
	LuaHelp::reg_string(L, "stationName", ws.WtoUTF8(r.stationName));
	LuaHelp::reg_int(L, "onid", r.originalNetworkID);
	LuaHelp::reg_int(L, "tsid", r.transportStreamID);
	LuaHelp::reg_int(L, "sid", r.serviceID);
	LuaHelp::reg_int(L, "eid", r.eventID);
	LuaHelp::reg_string(L, "comment", ws.WtoUTF8(r.comment));
	LuaHelp::reg_int(L, "reserveID", (int)r.reserveID);
	LuaHelp::reg_int(L, "overlapMode", r.overlapMode);
	LuaHelp::reg_time(L, "startTimeEpg", r.startTimeEpg);
	lua_pushstring(L, "recFileNameList");
	lua_newtable(L);
	for( size_t i = 0; i < r.recFileNameList.size(); i++ ){
		lua_pushstring(L, ws.WtoUTF8(r.recFileNameList[i]));
		lua_rawseti(L, -2, (int)i + 1);
	}
	lua_rawset(L, -3);
	lua_pushstring(L, "recSetting");
	lua_newtable(L);
	PushRecSettingData(ws, r.recSetting);
	lua_rawset(L, -3);
}

void CEpgTimerSrvMain::PushRecSettingData(CLuaWorkspace& ws, const REC_SETTING_DATA& rs)
{
	lua_State* L = ws.L;
	LuaHelp::reg_int(L, "recMode", rs.recMode);
	LuaHelp::reg_int(L, "priority", rs.priority);
	LuaHelp::reg_boolean(L, "tuijyuuFlag", rs.tuijyuuFlag != 0);
	LuaHelp::reg_int(L, "serviceMode", (int)rs.serviceMode);
	LuaHelp::reg_boolean(L, "pittariFlag", rs.pittariFlag != 0);
	LuaHelp::reg_string(L, "batFilePath", ws.WtoUTF8(rs.batFilePath));
	LuaHelp::reg_int(L, "suspendMode", rs.suspendMode);
	LuaHelp::reg_boolean(L, "rebootFlag", rs.rebootFlag != 0);
	if( rs.useMargineFlag ){
		LuaHelp::reg_int(L, "startMargin", rs.startMargine);
		LuaHelp::reg_int(L, "endMargin", rs.endMargine);
	}
	LuaHelp::reg_boolean(L, "continueRecFlag", rs.continueRecFlag != 0);
	LuaHelp::reg_int(L, "partialRecFlag", rs.partialRecFlag);
	LuaHelp::reg_int(L, "tunerID", (int)rs.tunerID);
	for( int i = 0; i < 2; i++ ){
		const vector<REC_FILE_SET_INFO>& recFolderList = i == 0 ? rs.recFolderList : rs.partialRecFolder;
		lua_pushstring(L, i == 0 ? "recFolderList" : "partialRecFolder");
		lua_newtable(L);
		for( size_t j = 0; j < recFolderList.size(); j++ ){
			lua_newtable(L);
			LuaHelp::reg_string(L, "recFolder", ws.WtoUTF8(recFolderList[j].recFolder));
			LuaHelp::reg_string(L, "writePlugIn", ws.WtoUTF8(recFolderList[j].writePlugIn));
			LuaHelp::reg_string(L, "recNamePlugIn", ws.WtoUTF8(recFolderList[j].recNamePlugIn));
			lua_rawseti(L, -2, (int)j + 1);
		}
		lua_rawset(L, -3);
	}
}

void CEpgTimerSrvMain::PushEpgSearchKeyInfo(CLuaWorkspace& ws, const EPGDB_SEARCH_KEY_INFO& k)
{
	lua_State* L = ws.L;
	wstring andKey = k.andKey;
	size_t pos = andKey.compare(0, 7, L"^!{999}") ? 0 : 7;
	pos += andKey.compare(pos, 7, L"C!{999}") ? 0 : 7;
	int durMin = 0;
	int durMax = 0;
	if( andKey.compare(pos, 4, L"D!{1") == 0 ){
		LPWSTR endp;
		DWORD dur = wcstoul(andKey.c_str() + pos + 3, &endp, 10);
		if( endp - (andKey.c_str() + pos + 3) == 9 && endp[0] == L'}' ){
			andKey.erase(pos, 13);
			durMin = dur / 10000 % 10000;
			durMax = dur % 10000;
		}
	}
	LuaHelp::reg_string(L, "andKey", ws.WtoUTF8(andKey));
	LuaHelp::reg_string(L, "notKey", ws.WtoUTF8(k.notKey));
	LuaHelp::reg_boolean(L, "regExpFlag", k.regExpFlag != 0);
	LuaHelp::reg_boolean(L, "titleOnlyFlag", k.titleOnlyFlag != 0);
	LuaHelp::reg_boolean(L, "aimaiFlag", k.aimaiFlag != 0);
	LuaHelp::reg_boolean(L, "notContetFlag", k.notContetFlag != 0);
	LuaHelp::reg_boolean(L, "notDateFlag", k.notDateFlag != 0);
	LuaHelp::reg_int(L, "freeCAFlag", k.freeCAFlag);
	LuaHelp::reg_boolean(L, "chkRecEnd", k.chkRecEnd != 0);
	LuaHelp::reg_int(L, "chkRecDay", k.chkRecDay >= 40000 ? k.chkRecDay % 10000 : k.chkRecDay);
	LuaHelp::reg_boolean(L, "chkRecNoService", k.chkRecDay >= 40000);
	LuaHelp::reg_int(L, "chkDurationMin", durMin);
	LuaHelp::reg_int(L, "chkDurationMax", durMax);
	lua_pushstring(L, "contentList");
	lua_newtable(L);
	for( size_t i = 0; i < k.contentList.size(); i++ ){
		lua_newtable(L);
		LuaHelp::reg_int(L, "content_nibble", k.contentList[i].content_nibble_level_1 << 8 | k.contentList[i].content_nibble_level_2);
		LuaHelp::reg_int(L, "user_nibble", k.contentList[i].user_nibble_1 << 8 | k.contentList[i].user_nibble_2);
		lua_rawseti(L, -2, (int)i + 1);
	}
	lua_rawset(L, -3);
	lua_pushstring(L, "dateList");
	lua_newtable(L);
	for( size_t i = 0; i < k.dateList.size(); i++ ){
		lua_newtable(L);
		LuaHelp::reg_int(L, "startDayOfWeek", k.dateList[i].startDayOfWeek);
		LuaHelp::reg_int(L, "startHour", k.dateList[i].startHour);
		LuaHelp::reg_int(L, "startMin", k.dateList[i].startMin);
		LuaHelp::reg_int(L, "endDayOfWeek", k.dateList[i].endDayOfWeek);
		LuaHelp::reg_int(L, "endHour", k.dateList[i].endHour);
		LuaHelp::reg_int(L, "endMin", k.dateList[i].endMin);
		lua_rawseti(L, -2, (int)i + 1);
	}
	lua_rawset(L, -3);
	lua_pushstring(L, "serviceList");
	lua_newtable(L);
	for( size_t i = 0; i < k.serviceList.size(); i++ ){
		lua_newtable(L);
		LuaHelp::reg_int(L, "onid", k.serviceList[i] >> 32 & 0xFFFF);
		LuaHelp::reg_int(L, "tsid", k.serviceList[i] >> 16 & 0xFFFF);
		LuaHelp::reg_int(L, "sid", k.serviceList[i] & 0xFFFF);
		lua_rawseti(L, -2, (int)i + 1);
	}
	lua_rawset(L, -3);
}

bool CEpgTimerSrvMain::FetchReserveData(CLuaWorkspace& ws, RESERVE_DATA& r)
{
	lua_State* L = ws.L;
	UTF8toW(LuaHelp::get_string(L, "title"), r.title);
	r.startTime = LuaHelp::get_time(L, "startTime");
	r.durationSecond = LuaHelp::get_int(L, "durationSecond");
	UTF8toW(LuaHelp::get_string(L, "stationName"), r.stationName);
	r.originalNetworkID = (WORD)LuaHelp::get_int(L, "onid");
	r.transportStreamID = (WORD)LuaHelp::get_int(L, "tsid");
	r.serviceID = (WORD)LuaHelp::get_int(L, "sid");
	r.eventID = (WORD)LuaHelp::get_int(L, "eid");
	UTF8toW(LuaHelp::get_string(L, "comment"), r.comment);
	r.reserveID = (WORD)LuaHelp::get_int(L, "reserveID");
	r.startTimeEpg = LuaHelp::get_time(L, "startTimeEpg");
	lua_getfield(L, -1, "recSetting");
	if( r.startTime.wYear && r.startTimeEpg.wYear && lua_istable(L, -1) ){
		FetchRecSettingData(ws, r.recSetting);
		lua_pop(L, 1);
		return true;
	}
	lua_pop(L, 1);
	return false;
}

void CEpgTimerSrvMain::FetchRecSettingData(CLuaWorkspace& ws, REC_SETTING_DATA& rs)
{
	lua_State* L = ws.L;
	rs.recMode = (BYTE)LuaHelp::get_int(L, "recMode");
	rs.priority = (BYTE)LuaHelp::get_int(L, "priority");
	rs.tuijyuuFlag = LuaHelp::get_boolean(L, "tuijyuuFlag");
	rs.serviceMode = (BYTE)LuaHelp::get_int(L, "serviceMode");
	rs.pittariFlag = LuaHelp::get_boolean(L, "pittariFlag");
	UTF8toW(LuaHelp::get_string(L, "batFilePath"), rs.batFilePath);
	rs.suspendMode = (BYTE)LuaHelp::get_int(L, "suspendMode");
	rs.rebootFlag = LuaHelp::get_boolean(L, "rebootFlag");
	rs.useMargineFlag = LuaHelp::isnil(L, "startMargin") == false;
	rs.startMargine = LuaHelp::get_int(L, "startMargin");
	rs.endMargine = LuaHelp::get_int(L, "endMargin");
	rs.continueRecFlag = LuaHelp::get_boolean(L, "continueRecFlag");
	rs.partialRecFlag = (BYTE)LuaHelp::get_int(L, "partialRecFlag");
	rs.tunerID = LuaHelp::get_int(L, "tunerID");
	for( int i = 0; i < 2; i++ ){
		lua_getfield(L, -1, i == 0 ? "recFolderList" : "partialRecFolder");
		if( lua_istable(L, -1) ){
			for( int j = 0;; j++ ){
				lua_rawgeti(L, -1, j + 1);
				if( !lua_istable(L, -1) ){
					lua_pop(L, 1);
					break;
				}
				vector<REC_FILE_SET_INFO>& recFolderList = i == 0 ? rs.recFolderList : rs.partialRecFolder;
				recFolderList.resize(j + 1);
				UTF8toW(LuaHelp::get_string(L, "recFolder"), recFolderList[j].recFolder);
				UTF8toW(LuaHelp::get_string(L, "writePlugIn"), recFolderList[j].writePlugIn);
				UTF8toW(LuaHelp::get_string(L, "recNamePlugIn"), recFolderList[j].recNamePlugIn);
				lua_pop(L, 1);
			}
		}
		lua_pop(L, 1);
	}
}

void CEpgTimerSrvMain::FetchEpgSearchKeyInfo(CLuaWorkspace& ws, EPGDB_SEARCH_KEY_INFO& k)
{
	lua_State* L = ws.L;
	UTF8toW(LuaHelp::get_string(L, "andKey"), k.andKey);
	UTF8toW(LuaHelp::get_string(L, "notKey"), k.notKey);
	k.regExpFlag = LuaHelp::get_boolean(L, "regExpFlag");
	k.titleOnlyFlag = LuaHelp::get_boolean(L, "titleOnlyFlag");
	k.aimaiFlag = LuaHelp::get_boolean(L, "aimaiFlag");
	k.notContetFlag = LuaHelp::get_boolean(L, "notContetFlag");
	k.notDateFlag = LuaHelp::get_boolean(L, "notDateFlag");
	k.freeCAFlag = (BYTE)LuaHelp::get_int(L, "freeCAFlag");
	k.chkRecEnd = LuaHelp::get_boolean(L, "chkRecEnd");
	k.chkRecDay = (WORD)LuaHelp::get_int(L, "chkRecDay");
	if( LuaHelp::get_boolean(L, "chkRecNoService") ){
		k.chkRecDay = k.chkRecDay % 10000 + 40000;
	}
	int durMin = LuaHelp::get_int(L, "chkDurationMin");
	int durMax = LuaHelp::get_int(L, "chkDurationMax");
	if( durMin > 0 || durMax > 0 ){
		wstring dur;
		Format(dur, L"D!{%d}", (10000 + min(max(durMin, 0), 9999)) * 10000 + min(max(durMax, 0), 9999));
		size_t pos = k.andKey.compare(0, 7, L"^!{999}") ? 0 : 7;
		pos += k.andKey.compare(pos, 7, L"C!{999}") ? 0 : 7;
		k.andKey.insert(pos, dur);
	}
	lua_getfield(L, -1, "contentList");
	if( lua_istable(L, -1) ){
		for( int i = 0;; i++ ){
			lua_rawgeti(L, -1, i + 1);
			if( !lua_istable(L, -1) ){
				lua_pop(L, 1);
				break;
			}
			k.contentList.resize(i + 1);
			k.contentList[i].content_nibble_level_1 = LuaHelp::get_int(L, "content_nibble") >> 8 & 0xFF;
			k.contentList[i].content_nibble_level_2 = LuaHelp::get_int(L, "content_nibble") & 0xFF;
			k.contentList[i].user_nibble_1 = LuaHelp::get_int(L, "user_nibble") >> 8 & 0xFF;
			k.contentList[i].user_nibble_2 = LuaHelp::get_int(L, "user_nibble") & 0xFF;
			lua_pop(L, 1);
		}
	}
	lua_pop(L, 1);
	lua_getfield(L, -1, "dateList");
	if( lua_istable(L, -1) ){
		for( int i = 0;; i++ ){
			lua_rawgeti(L, -1, i + 1);
			if( !lua_istable(L, -1) ){
				lua_pop(L, 1);
				break;
			}
			k.dateList.resize(i + 1);
			k.dateList[i].startDayOfWeek = (BYTE)LuaHelp::get_int(L, "startDayOfWeek");
			k.dateList[i].startHour = (WORD)LuaHelp::get_int(L, "startHour");
			k.dateList[i].startMin = (WORD)LuaHelp::get_int(L, "startMin");
			k.dateList[i].endDayOfWeek = (BYTE)LuaHelp::get_int(L, "endDayOfWeek");
			k.dateList[i].endHour = (WORD)LuaHelp::get_int(L, "endHour");
			k.dateList[i].endMin = (WORD)LuaHelp::get_int(L, "endMin");
			lua_pop(L, 1);
		}
	}
	lua_pop(L, 1);
	lua_getfield(L, -1, "serviceList");
	if( lua_istable(L, -1) ){
		for( int i = 0;; i++ ){
			lua_rawgeti(L, -1, i + 1);
			if( !lua_istable(L, -1) ){
				lua_pop(L, 1);
				break;
			}
			k.serviceList.push_back(
				(__int64)LuaHelp::get_int(L, "onid") << 32 |
				(__int64)LuaHelp::get_int(L, "tsid") << 16 |
				LuaHelp::get_int(L, "sid"));
			lua_pop(L, 1);
		}
	}
	lua_pop(L, 1);
}

//Lua-edcb空間のコールバックここまで
#endif
