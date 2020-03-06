
// EpgDataCap_BonDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "EpgDataCap_Bon.h"
#include "EpgDataCap_BonDlg.h"
#include "../../Common/CommonDef.h"
#include "../../Common/CtrlCmdDef.h"
#include "../../Common/CtrlCmdUtil.h"
#include "../../Common/TimeUtil.h"
#include <shellapi.h>
#include <objbase.h>
#include "TaskbarList.h"


// CEpgDataCap_BonDlg ダイアログ


UINT CEpgDataCap_BonDlg::taskbarCreated = 0;
BOOL CEpgDataCap_BonDlg::disableKeyboardHook = FALSE;

CEpgDataCap_BonDlg::CEpgDataCap_BonDlg()
	: m_hWnd(NULL)
	, m_hKeyboardHook(NULL)
{
	HMODULE hModule = GetModuleHandle(NULL);
	HRESULT (WINAPI* pfnLoadIconMetric)(HINSTANCE,PCWSTR,int,HICON*) =
		(HRESULT (WINAPI*)(HINSTANCE,PCWSTR,int,HICON*))GetProcAddress(GetModuleHandle(L"comctl32.dll"), "LoadIconMetric");
	if( pfnLoadIconMetric == NULL ||
	    pfnLoadIconMetric(hModule, MAKEINTRESOURCE(IDI_ICON_BLUE), LIM_SMALL, &m_hIcon) != S_OK ||
	    pfnLoadIconMetric(hModule, MAKEINTRESOURCE(IDI_ICON_BLUE), LIM_LARGE, &m_hIcon2) != S_OK ||
	    pfnLoadIconMetric(hModule, MAKEINTRESOURCE(IDI_ICON_RED), LIM_SMALL, &iconRed) != S_OK ||
	    pfnLoadIconMetric(hModule, MAKEINTRESOURCE(IDI_ICON_GREEN), LIM_SMALL, &iconGreen) != S_OK ||
	    pfnLoadIconMetric(hModule, MAKEINTRESOURCE(IDI_ICON_GRAY), LIM_SMALL, &iconGray) != S_OK ||
	    pfnLoadIconMetric(hModule, MAKEINTRESOURCE(IDI_ICON_OVERLAY_REC), LIM_SMALL, &iconOlRec) != S_OK ||
	    pfnLoadIconMetric(hModule, MAKEINTRESOURCE(IDI_ICON_OVERLAY_EPG), LIM_SMALL, &iconOlEpg) != S_OK ){
		m_hIcon = (HICON)LoadImage(hModule, MAKEINTRESOURCE(IDI_ICON_BLUE), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
		m_hIcon2 = (HICON)LoadImage(hModule, MAKEINTRESOURCE(IDI_ICON_BLUE), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
		iconRed = (HICON)LoadImage(hModule, MAKEINTRESOURCE(IDI_ICON_RED), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
		iconGreen = (HICON)LoadImage(hModule, MAKEINTRESOURCE(IDI_ICON_GREEN), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
		iconGray = (HICON)LoadImage(hModule, MAKEINTRESOURCE(IDI_ICON_GRAY), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
		iconOlRec = (HICON)LoadImage(hModule, MAKEINTRESOURCE(IDI_ICON_OVERLAY_REC), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
		iconOlEpg = (HICON)LoadImage(hModule, MAKEINTRESOURCE(IDI_ICON_OVERLAY_EPG), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	}
	iconBlue = m_hIcon;

	taskbarCreated = RegisterWindowMessage(L"TaskbarCreated");

	iniView = FALSE;
	iniNetwork = TRUE;
	iniMin = FALSE;
	this->iniUDP = FALSE;
	this->iniTCP = FALSE;
	this->outCtrlID = -1;
	this->cmdCapture = NULL;
	this->resCapture = NULL;
	this->lastONID = 0xFFFF;
	this->lastTSID = 0xFFFF;
	this->recCtrlID = 0;
	this->chScanWorking = FALSE;
	this->epgCapWorking = FALSE;

	if( CPipeServer::GrantServerAccessToKernelObject(GetCurrentProcess(), SYNCHRONIZE | PROCESS_TERMINATE | PROCESS_SET_INFORMATION) ){
		OutputDebugString(L"Granted SYNCHRONIZE|PROCESS_TERMINATE|PROCESS_SET_INFORMATION to " SERVICE_NAME L"\r\n");
	}
}

INT_PTR CEpgDataCap_BonDlg::DoModal()
{
	int index = GetPrivateProfileInt(L"SET", L"DialogTemplate", 0, GetModuleIniPath().c_str());
	return DialogBoxParam(GetModuleHandle(NULL),
	                      MAKEINTRESOURCE(index == 1 ? IDD_EPGDATACAP_BON_DIALOG_1 :
	                                      index == 2 ? IDD_EPGDATACAP_BON_DIALOG_2 : IDD),
	                      NULL, DlgProc, (LPARAM)this);
}

void CEpgDataCap_BonDlg::ReloadSetting()
{
	fs_path appIniPath = GetModuleIniPath();

	SetSaveDebugLog(GetPrivateProfileInt(L"SET", L"SaveDebugLog", 0, appIniPath.c_str()) != 0);
	this->modifyTitleBarText = GetPrivateProfileInt(L"SET", L"ModifyTitleBarText", 0, appIniPath.c_str()) != 0;
	this->overlayTaskIcon = GetPrivateProfileInt(L"SET", L"OverlayTaskIcon", 1, appIniPath.c_str()) != 0;
	this->minTask = GetPrivateProfileInt(L"SET", L"MinTask", 0, appIniPath.c_str()) != 0;
	this->recFileName = GetPrivateProfileToString(L"SET", L"RecFileName", L"$DYYYY$$DMM$$DDD$-$THH$$TMM$$TSS$-$ServiceName$.ts", appIniPath.c_str());
	this->overWriteFlag = GetPrivateProfileInt(L"SET", L"OverWrite", 0, appIniPath.c_str()) != 0;
	this->viewPath = GetPrivateProfileToString(L"SET", L"ViewPath", L"", appIniPath.c_str());
	this->viewOpt = GetPrivateProfileToString(L"SET", L"ViewOption", L"", appIniPath.c_str());
	this->dropSaveThresh = GetPrivateProfileInt(L"SET", L"DropSaveThresh", 0, appIniPath.c_str());
	this->scrambleSaveThresh = GetPrivateProfileInt(L"SET", L"ScrambleSaveThresh", -1, appIniPath.c_str());
	this->dropLogAsUtf8 = GetPrivateProfileInt(L"SET", L"DropLogAsUtf8", 0, appIniPath.c_str()) != 0;
	this->tsBuffMaxCount = (DWORD)GetPrivateProfileInt(L"SET", L"TsBuffMaxCount", 5000, appIniPath.c_str());
	this->writeBuffMaxCount = GetPrivateProfileInt(L"SET", L"WriteBuffMaxCount", -1, appIniPath.c_str());
	this->openWait = GetPrivateProfileInt(L"SET", L"OpenWait", 200, appIniPath.c_str());

	this->recFolderList.clear();
	for( int i = 0; ; i++ ){
		this->recFolderList.push_back(GetRecFolderPath(i).native());
		if( this->recFolderList.back().empty() ){
			this->recFolderList.pop_back();
			break;
		}
	}

	this->setUdpSendList.clear();
	this->setTcpSendList.clear();
	for( int tcp = 0; tcp < 2; tcp++ ){
		int count = GetPrivateProfileInt(tcp ? L"SET_TCP" : L"SET_UDP", L"Count", 0, appIniPath.c_str());
		for( int i = 0; i < count; i++ ){
			NW_SEND_INFO item;
			WCHAR key[64];
			swprintf_s(key, L"IP%d", i);
			item.ipString = GetPrivateProfileToString(tcp ? L"SET_TCP" : L"SET_UDP", key, L"2130706433", appIniPath.c_str());
			if( item.ipString.size() >= 2 && item.ipString[0] == L'[' ){
				item.ipString.erase(0, 1).pop_back();
			}else{
				UINT ip = _wtoi(item.ipString.c_str());
				Format(item.ipString, L"%d.%d.%d.%d", ip >> 24, ip >> 16 & 0xFF, ip >> 8 & 0xFF, ip & 0xFF);
			}
			swprintf_s(key, L"Port%d", i);
			item.port = GetPrivateProfileInt(tcp ? L"SET_TCP" : L"SET_UDP", key, tcp ? BON_TCP_PORT_BEGIN : BON_UDP_PORT_BEGIN, appIniPath.c_str());
			swprintf_s(key, L"BroadCast%d", i);
			item.broadcastFlag = tcp ? 0 : GetPrivateProfileInt(L"SET_UDP", key, 0, appIniPath.c_str());
			(tcp ? this->setTcpSendList : this->setUdpSendList).push_back(item);
		}
	}

	this->bonCtrl.SetBackGroundEpgCap(GetPrivateProfileInt(L"SET", L"EpgCapLive", 1, appIniPath.c_str()) != 0,
	                                  GetPrivateProfileInt(L"SET", L"EpgCapRec", 1, appIniPath.c_str()) != 0,
	                                  GetPrivateProfileInt(L"SET", L"EpgCapBackBSBasicOnly", 1, appIniPath.c_str()) != 0,
	                                  GetPrivateProfileInt(L"SET", L"EpgCapBackCS1BasicOnly", 1, appIniPath.c_str()) != 0,
	                                  GetPrivateProfileInt(L"SET", L"EpgCapBackCS2BasicOnly", 1, appIniPath.c_str()) != 0,
	                                  GetPrivateProfileInt(L"SET", L"EpgCapBackCS3BasicOnly", 0, appIniPath.c_str()) != 0,
	                                  (DWORD)GetPrivateProfileInt(L"SET", L"EpgCapBackStartWaitSec", 30, appIniPath.c_str()));

	this->bonCtrl.ReloadSetting(GetPrivateProfileInt(L"SET", L"EMM", 0, appIniPath.c_str()) != 0,
	                            GetPrivateProfileInt(L"SET", L"NoLogScramble", 0, appIniPath.c_str()) != 0,
	                            GetPrivateProfileInt(L"SET", L"ParseEpgPostProcess", 0, appIniPath.c_str()) != 0,
	                            GetPrivateProfileInt(L"SET", L"Scramble", 1, appIniPath.c_str()) != 0,
	                            GetPrivateProfileInt(L"SET", L"Caption", 1, appIniPath.c_str()) != 0,
	                            GetPrivateProfileInt(L"SET", L"Data", 0, appIniPath.c_str()) != 0,
	                            GetPrivateProfileInt(L"SET", L"AllService", 0, appIniPath.c_str()) != 0);

	EnableWindow(GetDlgItem(IDC_BUTTON_VIEW), this->viewPath.empty() == false);
}

// CEpgDataCap_BonDlg メッセージ ハンドラー
BOOL CEpgDataCap_BonDlg::OnInitDialog()
{
	// このダイアログのアイコンを設定します。アプリケーションのメイン ウィンドウがダイアログでない場合、
	//  Framework は、この設定を自動的に行います。
	SendMessage(m_hWnd, WM_SETICON, ICON_BIG, (LPARAM)m_hIcon2);	// 大きいアイコンの設定
	SendMessage(m_hWnd, WM_SETICON, ICON_SMALL, (LPARAM)m_hIcon);	// 小さいアイコンの設定

	// TODO: 初期化をここに追加します。
	ReloadSetting();

	for( int i=0; i<24; i++ ){
		WCHAR buff[32];
		swprintf_s(buff, L"%d", i);
		ComboBox_AddString(GetDlgItem(IDC_COMBO_REC_H), buff);
	}
	ComboBox_SetCurSel(GetDlgItem(IDC_COMBO_REC_H), 0);

	for( int i=0; i<60; i++ ){
		WCHAR buff[32];
		swprintf_s(buff, L"%d", i);
		ComboBox_AddString(GetDlgItem(IDC_COMBO_REC_M), buff);
	}
	ComboBox_SetCurSel(GetDlgItem(IDC_COMBO_REC_M), 0);

	fs_path appIniPath = GetModuleIniPath();

	int initONID = -1;
	int initTSID = -1;
	int initSID = -1;
	int initOpenWait = 0;
	int initChgWait = 0;
	if( this->iniBonDriver.empty() == false &&
	    GetPrivateProfileInt(this->iniBonDriver.c_str(), L"OpenFix", 0, appIniPath.c_str()) ){
		OutputDebugString(L"強制サービス指定 設定値ロード");
		initONID = GetPrivateProfileInt(this->iniBonDriver.c_str(), L"FixONID", -1, appIniPath.c_str());
		initTSID = GetPrivateProfileInt(this->iniBonDriver.c_str(), L"FixTSID", -1, appIniPath.c_str());
		initSID = GetPrivateProfileInt(this->iniBonDriver.c_str(), L"FixSID", -1, appIniPath.c_str());
		initOpenWait = GetPrivateProfileInt(this->iniBonDriver.c_str(), L"OpenWait", 0, appIniPath.c_str());
		initChgWait = GetPrivateProfileInt(this->iniBonDriver.c_str(), L"ChgWait", 0, appIniPath.c_str());
		_OutputDebugString(L"%d,%d,%d,%d,%d", initONID, initTSID, initSID, initOpenWait, initChgWait);
	}else if( GetPrivateProfileInt(L"SET", L"OpenLast", 1, appIniPath.c_str()) ){
		initONID = GetPrivateProfileInt(L"SET", L"LastONID", -1, appIniPath.c_str());
		initTSID = GetPrivateProfileInt(L"SET", L"LastTSID", -1, appIniPath.c_str());
		initSID = GetPrivateProfileInt(L"SET", L"LastSID", -1, appIniPath.c_str());
		if( this->iniBonDriver.empty() ){
			this->iniBonDriver = GetPrivateProfileToString(L"SET", L"LastBon", L"", appIniPath.c_str());
		}
	}else if( GetPrivateProfileInt(L"SET", L"OpenFix", 0, appIniPath.c_str()) ){
		initONID = GetPrivateProfileInt(L"SET", L"FixONID", -1, appIniPath.c_str());
		initTSID = GetPrivateProfileInt(L"SET", L"FixTSID", -1, appIniPath.c_str());
		initSID = GetPrivateProfileInt(L"SET", L"FixSID", -1, appIniPath.c_str());
		if( this->iniBonDriver.empty() ){
			this->iniBonDriver = GetPrivateProfileToString(L"SET", L"FixBon", L"", appIniPath.c_str());
		}
	}

	//BonDriverの一覧取得
	int bonIndex = -1;
	wstring bon;
	EnumFindFile(GetModulePath().replace_filename(BON_DLL_FOLDER).append(L"BonDriver*.dll"), [&](UTIL_FIND_DATA& findData) -> bool {
		if( findData.isDir == false ){
			int index = ComboBox_AddString(this->GetDlgItem(IDC_COMBO_TUNER), findData.fileName.c_str());
			if( bonIndex < 0 || UtilComparePath(findData.fileName.c_str(), this->iniBonDriver.c_str()) == 0 ){
				bonIndex = index;
				bon = std::move(findData.fileName);
			}
		}
		return true;
	});
	if( bonIndex >= 0 ){
		ComboBox_SetCurSel(GetDlgItem(IDC_COMBO_TUNER), bonIndex);
	}

	//BonDriverのオープン
	int serviceIndex = -1;
	if( this->iniBonDriver.empty() == false ){
		//BonDriver指定時は一覧になくてもよい
		if( SelectBonDriver(this->iniBonDriver.c_str()) ){
			if( initOpenWait > 0 ){
				Sleep(initOpenWait);
			}
			serviceIndex = ReloadServiceList(initONID, initTSID, initSID);
		}
	}else{
		if( bonIndex >= 0 ){
			//一覧で選択されたものをオープン
			if( SelectBonDriver(bon.c_str()) ){
				serviceIndex = ReloadServiceList();
			}
		}else{
			SetDlgItemText(m_hWnd, IDC_EDIT_LOG, L"BonDriverが見つかりませんでした\r\n");
			BtnUpdate(GUI_OPEN_FAIL);
		}
	}

	if( serviceIndex >= 0 ){
		//チャンネル変更
		if( SelectService(this->serviceList[serviceIndex]) ){
			if( initONID >= 0 && initTSID >= 0 && initSID >= 0 && initChgWait > 0 ){
				Sleep(initChgWait);
			}
		}
	}

	//ウインドウの復元
	WINDOWPLACEMENT Pos;
	Pos.length = sizeof(WINDOWPLACEMENT);
	int left = GetPrivateProfileInt(L"SET_WINDOW", L"left", INT_MAX, appIniPath.c_str());
	int top = GetPrivateProfileInt(L"SET_WINDOW", L"top", INT_MAX, appIniPath.c_str());
	if( left != INT_MAX && top != INT_MAX && GetWindowPlacement(m_hWnd, &Pos) ){
		Pos.flags = 0;
		Pos.showCmd = this->iniMin ? SW_SHOWMINNOACTIVE : SW_SHOW;
		int width = GetPrivateProfileInt(L"SET_WINDOW", L"width", 0, appIniPath.c_str());
		int height = GetPrivateProfileInt(L"SET_WINDOW", L"height", 0, appIniPath.c_str());
		if( width > 0 && height > 0 ){
			Pos.rcNormalPosition.right = left + width;
			Pos.rcNormalPosition.bottom = top + height;
		}else{
			Pos.rcNormalPosition.right += left - Pos.rcNormalPosition.left;
			Pos.rcNormalPosition.bottom += top - Pos.rcNormalPosition.top;
		}
		Pos.rcNormalPosition.left = left;
		Pos.rcNormalPosition.top = top;
		SetWindowPlacement(m_hWnd, &Pos);
	}
	SetTimer(TIMER_STATUS_UPDATE, 1000, NULL);
	SetTimer(TIMER_INIT_DLG, 1, NULL);

	if( this->iniNetwork == TRUE ){
		if( this->iniUDP == TRUE || this->iniTCP == TRUE ){
			if( this->iniUDP == TRUE ){
				Button_SetCheck(GetDlgItem(IDC_CHECK_UDP), BST_CHECKED);
			}
			if( this->iniTCP == TRUE ){
				Button_SetCheck(GetDlgItem(IDC_CHECK_TCP), BST_CHECKED);
			}
		}else{
			Button_SetCheck(GetDlgItem(IDC_CHECK_UDP), GetPrivateProfileInt(L"SET", L"ChkUDP", 0, appIniPath.c_str()));
			Button_SetCheck(GetDlgItem(IDC_CHECK_TCP), GetPrivateProfileInt(L"SET", L"ChkTCP", 0, appIniPath.c_str()));
		}
	}

	ReloadNWSet();

	StartPipeServer();

	return TRUE;  // フォーカスをコントロールに設定した場合を除き、TRUE を返します。
}


void CEpgDataCap_BonDlg::OnSysCommand(UINT nID, LPARAM lParam, BOOL* pbProcessed)
{
	// TODO: ここにメッセージ ハンドラー コードを追加するか、既定の処理を呼び出します。
	if( nID == SC_CLOSE ){
		if( this->bonCtrl.IsRec() ){
			WCHAR caption[128] = L"";
			GetWindowText(m_hWnd, caption, 128);
			disableKeyboardHook = TRUE;
			int result = MessageBox( m_hWnd, L"録画中ですが終了しますか？", caption, MB_YESNO | MB_ICONQUESTION );
			disableKeyboardHook = FALSE;
			if( result == IDNO ){
				*pbProcessed = TRUE;
			}
		}
	}
}


void CEpgDataCap_BonDlg::OnDestroy()
{
	this->pipeServer.StopServer();
	this->bonCtrl.CloseBonDriver();
	KillTimer(TIMER_STATUS_UPDATE);

	KillTimer(RETRY_ADD_TRAY);
	KillTimer(TIMER_CHG_TRAY);
	DeleteTaskBar(m_hWnd, TRAYICON_ID);
	if( this->overlayTaskIcon ){
		SetOverlayIcon(NULL);
	}

	WINDOWPLACEMENT Pos;
	Pos.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(m_hWnd, &Pos);

	fs_path appIniPath = GetModuleIniPath();

	WritePrivateProfileInt(L"SET_WINDOW", L"top", Pos.rcNormalPosition.top, appIniPath.c_str());
	WritePrivateProfileInt(L"SET_WINDOW", L"left", Pos.rcNormalPosition.left, appIniPath.c_str());
	WritePrivateProfileInt(L"SET_WINDOW", L"bottom", Pos.rcNormalPosition.bottom, appIniPath.c_str());
	WritePrivateProfileInt(L"SET_WINDOW", L"right", Pos.rcNormalPosition.right, appIniPath.c_str());

	int selONID = -1;
	int selTSID = -1;
	int selSID = -1;
	WCHAR bon[512] = L"";

	GetWindowText(GetDlgItem(IDC_COMBO_TUNER), bon, 512);
	int sel = ComboBox_GetCurSel(GetDlgItem(IDC_COMBO_SERVICE));
	if( sel != CB_ERR ){
		DWORD index = (DWORD)ComboBox_GetItemData(GetDlgItem(IDC_COMBO_SERVICE), sel);
		selONID = this->serviceList[index].originalNetworkID;
		selTSID = this->serviceList[index].transportStreamID;
		selSID = this->serviceList[index].serviceID;
	}

	WritePrivateProfileInt(L"SET", L"LastONID", selONID, appIniPath.c_str());
	WritePrivateProfileInt(L"SET", L"LastTSID", selTSID, appIniPath.c_str());
	WritePrivateProfileInt(L"SET", L"LastSID", selSID, appIniPath.c_str());
	WritePrivateProfileString(L"SET", L"LastBon", bon, appIniPath.c_str());
	WritePrivateProfileInt(L"SET", L"ChkUDP", Button_GetCheck(GetDlgItem(IDC_CHECK_UDP)), appIniPath.c_str());
	WritePrivateProfileInt(L"SET", L"ChkTCP", Button_GetCheck(GetDlgItem(IDC_CHECK_TCP)), appIniPath.c_str());

	// TODO: ここにメッセージ ハンドラー コードを追加します。
}


void CEpgDataCap_BonDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: ここにメッセージ ハンドラー コードを追加するか、既定の処理を呼び出します。
	switch(nIDEvent){
		case TIMER_INIT_DLG:
			{
				KillTimer( TIMER_INIT_DLG );
				if( this->iniMin == TRUE && this->minTask == TRUE){
					SetTimer(RETRY_ADD_TRAY, 0, NULL);
				    ShowWindow(m_hWnd, SW_HIDE);
				}
			}
			break;
		case TIMER_STATUS_UPDATE:
			{
				SetThreadExecutionState(ES_SYSTEM_REQUIRED);
				this->bonCtrl.Check();

				int iLine = Edit_GetFirstVisibleLine(GetDlgItem(IDC_EDIT_STATUS));
				float signal;
				int space;
				int ch;
				ULONGLONG drop = 0;
				ULONGLONG scramble = 0;
				this->bonCtrl.GetViewStatusInfo(&signal, &space, &ch, &drop, &scramble);

				wstring statusLog = L"";
				if( space >= 0 && ch >= 0 ){
					Format(statusLog, L"Signal: %.02f Drop: %lld Scramble: %lld  space: %d ch: %d", signal, drop, scramble, space, ch);
				}else{
					Format(statusLog, L"Signal: %.02f Drop: %lld Scramble: %lld", signal, drop, scramble);
				}
				statusLog += L"\r\n";

				wstring udp = L"";
				if( udpSendList.size() > 0 ){
					udp = L"UDP送信：";
					for( size_t i=0; i<udpSendList.size(); i++ ){
						wstring buff;
						Format(buff, L":%d%ls ", udpSendList[i].port, udpSendList[i].broadcastFlag ? L"(Broadcast)" : L"");
						udp += udpSendList[i].ipString.find(L':') == wstring::npos ? udpSendList[i].ipString : L'[' + udpSendList[i].ipString + L']';
						udp += buff;
					}
					udp += L"\r\n";
				}
				statusLog += udp;

				wstring tcp = L"";
				if( tcpSendList.size() > 0 ){
					tcp = L"TCP送信：";
					for( size_t i=0; i<tcpSendList.size(); i++ ){
						wstring buff;
						Format(buff, L":%d ", tcpSendList[i].port);
						tcp += tcpSendList[i].ipString.find(L':') == wstring::npos ? tcpSendList[i].ipString : L'[' + tcpSendList[i].ipString + L']';
						tcp += buff;
					}
					tcp += L"\r\n";
				}
				statusLog += tcp;

				SetDlgItemText(m_hWnd, IDC_EDIT_STATUS, statusLog.c_str());
				Edit_Scroll(GetDlgItem(IDC_EDIT_STATUS), iLine, 0);

				wstring info = L"";
				WORD onid;
				WORD tsid;
				//チャンネルスキャン中はサービス一覧などが安定しないため
				if( this->chScanWorking == FALSE && this->bonCtrl.GetStreamID(&onid, &tsid) ){
					//EPG取得中は別の検出ロジックがある
					if( this->epgCapWorking == FALSE && (this->lastONID != onid || this->lastTSID != tsid) ){
						//チャンネルが変化した
						for( size_t i = 0; i < this->serviceList.size(); i++ ){
							if( this->serviceList[i].originalNetworkID == onid &&
							    this->serviceList[i].transportStreamID == tsid ){
								int index = ReloadServiceList(onid, tsid, this->serviceList[i].serviceID);
								if( index >= 0 ){
									this->lastONID = onid;
									this->lastTSID = tsid;
									this->bonCtrl.SetNWCtrlServiceID(this->serviceList[index].serviceID);
								}
								break;
							}
						}
					}
					EPGDB_EVENT_INFO eventInfo;
					if( this->bonCtrl.GetEpgInfo(onid, tsid, this->bonCtrl.GetNWCtrlServiceID(),
					                             Button_GetCheck(GetDlgItem(IDC_CHECK_NEXTPG)), &eventInfo) == NO_ERR ){
						info = ConvertEpgInfoText(&eventInfo);
					}
				}
				vector<WCHAR> pgInfo(info.size() + 2);
				GetDlgItemText(m_hWnd, IDC_EDIT_PG_INFO, pgInfo.data(), (int)pgInfo.size());
				if( info != pgInfo.data() ){
					SetDlgItemText(m_hWnd, IDC_EDIT_PG_INFO, info.c_str());
				}
			}

			if( this->chScanWorking ){
				DWORD space = 0;
				DWORD ch = 0;
				wstring chName = L"";
				DWORD chkNum = 0;
				DWORD totalNum = 0;
				CBonCtrl::JOB_STATUS status = this->bonCtrl.GetChScanStatus(&space, &ch, &chName, &chkNum, &totalNum);
				if( status == CBonCtrl::ST_WORKING ){
					wstring log;
					Format(log, L"%ls (%d/%d 残り約 %d 秒)\r\n", chName.c_str(), chkNum, totalNum, (totalNum - chkNum)*10);
					SetDlgItemText(m_hWnd, IDC_EDIT_LOG, log.c_str());
				}else if( status == CBonCtrl::ST_CANCEL ){
					this->chScanWorking = FALSE;
					SetDlgItemText(m_hWnd, IDC_EDIT_LOG, L"キャンセルされました\r\n");
				}else if( status == CBonCtrl::ST_COMPLETE ){
					this->chScanWorking = FALSE;
					int index = ReloadServiceList();
					if( index >= 0 ){
						SelectService(this->serviceList[index]);
					}
					SetDlgItemText(m_hWnd, IDC_EDIT_LOG, L"終了しました\r\n");
					BtnUpdate(GUI_NORMAL);
					ChgIconStatus();

					//同じサービスが別の物理チャンネルにあるかチェック
					wstring msg = L"";
					for( size_t i=0; i<this->serviceList.size(); i++ ){
						for( size_t j=i+1; j<this->serviceList.size(); j++ ){
							if( this->serviceList[i].originalNetworkID == this->serviceList[j].originalNetworkID &&
								this->serviceList[i].transportStreamID == this->serviceList[j].transportStreamID &&
								this->serviceList[i].serviceID == this->serviceList[j].serviceID ){
									wstring log;
									Format(log, L"%ls space:%d ch:%d <=> %ls space:%d ch:%d\r\n",
										this->serviceList[i].serviceName.c_str(),
										this->serviceList[i].space,
										this->serviceList[i].ch,
										this->serviceList[j].serviceName.c_str(),
										this->serviceList[j].space,
										this->serviceList[j].ch);
									msg += log;
									break;
							}
						}
					}
					if( msg.size() > 0){
						wstring log = L"同一サービスが複数の物理チャンネルで検出されました。\r\n受信環境のよい物理チャンネルのサービスのみ残すように設定を行ってください。\r\n正常に録画できない可能性が出てきます。\r\n\r\n";
						log += msg;
						MessageBox(m_hWnd, log.c_str(), NULL, MB_OK);
					}
				}else{
					this->chScanWorking = FALSE;
				}
			}

			if( this->epgCapWorking ){
				SET_CH_INFO info;
				CBonCtrl::JOB_STATUS status = this->bonCtrl.GetEpgCapStatus(&info);
				if( status == CBonCtrl::ST_WORKING ){
					ReloadServiceList(info.ONID, info.TSID, info.SID);
					this->lastONID = info.ONID;
					this->lastTSID = info.TSID;
					this->bonCtrl.SetNWCtrlServiceID(info.SID);
					SetDlgItemText(m_hWnd, IDC_EDIT_LOG, L"EPG取得中\r\n");
				}else if( status == CBonCtrl::ST_CANCEL ){
					this->epgCapWorking = FALSE;
					SetDlgItemText(m_hWnd, IDC_EDIT_LOG, L"キャンセルされました\r\n");
				}else if( status == CBonCtrl::ST_COMPLETE ){
					this->epgCapWorking = FALSE;
					SetDlgItemText(m_hWnd, IDC_EDIT_LOG, L"終了しました\r\n");
					BtnUpdate(GUI_NORMAL);
					ChgIconStatus();
				}else{
					this->epgCapWorking = FALSE;
				}
			}
			break;
		case TIMER_REC_END:
			{
				if( this->recCtrlID != 0 ){
					this->bonCtrl.DeleteServiceCtrl(this->recCtrlID);
					this->recCtrlID = 0;
				}
				KillTimer(TIMER_REC_END);
				SetDlgItemText(m_hWnd, IDC_EDIT_LOG, L"録画停止しました\r\n");
				BtnUpdate(GUI_NORMAL);
				Button_SetCheck(GetDlgItem(IDC_CHECK_REC_SET), BST_UNCHECKED);
				ChgIconStatus();
			}
			break;
		case TIMER_CHG_TRAY:
		case RETRY_ADD_TRAY:
			{
				KillTimer(nIDEvent);

				HICON setIcon = this->iconBlue;
				if( this->bonCtrl.IsRec() ){
					setIcon = this->iconRed;
				}else if( this->bonCtrl.GetEpgCapStatus(NULL) == CBonCtrl::ST_WORKING ){
					setIcon = this->iconGreen;
				}else if( this->bonCtrl.GetOpenBonDriver(NULL) == FALSE ){
					setIcon = this->iconGray;
				}
		
				if( this->modifyTitleBarText ){
					WCHAR szTitle[256];
					if( GetWindowText(m_hWnd, szTitle, 256) > 0 ){
						wstring title = szTitle;
						size_t sep = title.rfind(L" - ");
						if( sep == wstring::npos ){
							sep = title.rfind(L" ● ");
							if( sep == wstring::npos ){
								sep = title.rfind(L" ○ ");
							}
						}
						if( sep == wstring::npos ){
							title.insert(0, L" - ");
							sep = 0;
						}
						title[sep + 1] = (setIcon == this->iconRed ? L'●' : setIcon == this->iconGreen ? L'○' : L'-');
						if( title != szTitle ){
							SetWindowText(m_hWnd, title.c_str());
						}
					}
				}
				if( this->overlayTaskIcon ){
					SetOverlayIcon(setIcon == this->iconRed ? this->iconOlRec : setIcon == this->iconGreen ? this->iconOlEpg : NULL);
				}
				if( this->minTask && IsWindowVisible(m_hWnd) == FALSE ){
					wstring bonFile;
					this->bonCtrl.GetOpenBonDriver(&bonFile);
					WCHAR szBuff[256] = L"";
					GetWindowText(GetDlgItem(IDC_COMBO_SERVICE), szBuff, 256);
					wstring buff = bonFile + L" ： " + szBuff;
					if( nIDEvent == RETRY_ADD_TRAY ){
						if( AddTaskBar(m_hWnd, WM_TRAY_PUSHICON, TRAYICON_ID, setIcon, buff) == FALSE ){
							SetTimer(RETRY_ADD_TRAY, 5000, NULL);
						}
					}else{
						ChgTipsTaskBar(m_hWnd, TRAYICON_ID, setIcon, buff);
					}
				}
			}
			break;
		case TIMER_TRY_STOP_SERVER:
			if( this->pipeServer.StopServer(true) ){
				KillTimer(TIMER_TRY_STOP_SERVER);
				OutputDebugString(L"CmdServer stopped\r\n");
				EndDialog(m_hWnd, IDCANCEL);
			}
			break;
		default:
			break;
	}
}


void CEpgDataCap_BonDlg::OnSize(UINT nType, int cx, int cy)
{
	// TODO: ここにメッセージ ハンドラー コードを追加します。
	if( nType == SIZE_MINIMIZED && this->iniMin == FALSE && this->minTask ){
		SetTimer(RETRY_ADD_TRAY, 0, NULL);
		ShowWindow(m_hWnd, SW_HIDE);
	}
}


LRESULT CEpgDataCap_BonDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	// TODO: ここに特定なコードを追加するか、もしくは基本クラスを呼び出してください。
	switch(message){
	case WM_INVOKE_CTRL_CMD:
		CtrlCmdCallbackInvoked();
		break;
	case WM_VIEW_APP_OPEN:
		if( this->viewPath.empty() == false ){
			ShellExecute(NULL, NULL, this->viewPath.c_str(), this->viewOpt.c_str(), NULL, SW_SHOWNORMAL);
		}
		break;
	case WM_TRAY_PUSHICON:
		{
			//タスクトレイ関係
			switch(LOWORD(lParam)){
				case WM_LBUTTONDOWN:
					{
						this->iniMin = FALSE;
						ShowWindow(m_hWnd, SW_RESTORE);
						SetForegroundWindow(m_hWnd);
						DeleteTaskBar(m_hWnd, TRAYICON_ID);
						ChgIconStatus();
					}
					break ;
				default :
					break ;
				}
		}
		break;
	default:
		break;
	}

	return 0;
}


BOOL CEpgDataCap_BonDlg::AddTaskBar(HWND wnd, UINT msg, UINT id, HICON icon, wstring tips)
{ 
	BOOL ret=TRUE;
	NOTIFYICONDATA data = {};

	data.cbSize = sizeof(NOTIFYICONDATA); 
	data.hWnd = wnd; 
	data.uID = id; 
	data.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP; 
	data.uCallbackMessage = msg; 
	data.hIcon = icon; 

	wcsncpy_s(data.szTip, tips.c_str(), _TRUNCATE);
 
	ret = Shell_NotifyIcon(NIM_ADD, &data);
  
	return ret; 
}

BOOL CEpgDataCap_BonDlg::ChgTipsTaskBar(HWND wnd, UINT id, HICON icon, wstring tips)
{ 
	BOOL ret=TRUE;
	NOTIFYICONDATA data = {};

	data.cbSize = sizeof(NOTIFYICONDATA); 
	data.hWnd = wnd; 
	data.uID = id; 
	data.hIcon = icon; 
	data.uFlags = NIF_ICON | NIF_TIP; 

	wcsncpy_s(data.szTip, tips.c_str(), _TRUNCATE);
 
	ret = Shell_NotifyIcon(NIM_MODIFY, &data); 
 
	return ret; 
}

BOOL CEpgDataCap_BonDlg::DeleteTaskBar(HWND wnd, UINT id)
{ 
	BOOL ret=TRUE; 
	NOTIFYICONDATA data = {};
 
	data.cbSize = sizeof(NOTIFYICONDATA); 
	data.hWnd = wnd; 
	data.uID = id; 
         
	ret = Shell_NotifyIcon(NIM_DELETE, &data); 

	return ret; 
}

void CEpgDataCap_BonDlg::ChgIconStatus()
{
	SetTimer(TIMER_CHG_TRAY, 0, NULL);
}

LRESULT CEpgDataCap_BonDlg::OnTaskbarCreated(WPARAM, LPARAM)
{
	SetTimer(RETRY_ADD_TRAY, 0, NULL);

	return 0;
}

#define ENABLE_ITEM(nItem,bEnable) EnableWindow(GetDlgItem(nItem),(bEnable))

void CEpgDataCap_BonDlg::BtnUpdate(DWORD guiMode)
{
	switch(guiMode){
		case GUI_NORMAL:
			ENABLE_ITEM(IDC_COMBO_TUNER, TRUE);
			ENABLE_ITEM(IDC_COMBO_SERVICE, TRUE);
			ENABLE_ITEM(IDC_BUTTON_CHSCAN, TRUE);
			ENABLE_ITEM(IDC_BUTTON_EPG, TRUE);
			ENABLE_ITEM(IDC_BUTTON_SET, TRUE);
			ENABLE_ITEM(IDC_BUTTON_REC, TRUE);
			ENABLE_ITEM(IDC_COMBO_REC_H, FALSE);
			ENABLE_ITEM(IDC_COMBO_REC_M, FALSE);
			ENABLE_ITEM(IDC_CHECK_REC_SET, FALSE);
			Button_SetCheck(GetDlgItem(IDC_CHECK_REC_SET), BST_UNCHECKED);
			ENABLE_ITEM(IDC_BUTTON_CANCEL, FALSE);
			break;
		case GUI_CANCEL_ONLY:
			ENABLE_ITEM(IDC_COMBO_TUNER, FALSE);
			ENABLE_ITEM(IDC_COMBO_SERVICE, FALSE);
			ENABLE_ITEM(IDC_BUTTON_CHSCAN, FALSE);
			ENABLE_ITEM(IDC_BUTTON_EPG, FALSE);
			ENABLE_ITEM(IDC_BUTTON_SET, FALSE);
			ENABLE_ITEM(IDC_BUTTON_REC, FALSE);
			ENABLE_ITEM(IDC_COMBO_REC_H, FALSE);
			ENABLE_ITEM(IDC_COMBO_REC_M, FALSE);
			ENABLE_ITEM(IDC_CHECK_REC_SET, FALSE);
			ENABLE_ITEM(IDC_BUTTON_CANCEL, TRUE);
			break;
		case GUI_OPEN_FAIL:
			ENABLE_ITEM(IDC_COMBO_TUNER, TRUE);
			ENABLE_ITEM(IDC_COMBO_SERVICE, FALSE);
			ENABLE_ITEM(IDC_BUTTON_CHSCAN, FALSE);
			ENABLE_ITEM(IDC_BUTTON_EPG, FALSE);
			ENABLE_ITEM(IDC_BUTTON_SET, TRUE);
			ENABLE_ITEM(IDC_BUTTON_REC, FALSE);
			ENABLE_ITEM(IDC_COMBO_REC_H, FALSE);
			ENABLE_ITEM(IDC_COMBO_REC_M, FALSE);
			ENABLE_ITEM(IDC_CHECK_REC_SET, FALSE);
			ENABLE_ITEM(IDC_BUTTON_CANCEL, FALSE);
			break;
		case GUI_REC:
			ENABLE_ITEM(IDC_COMBO_TUNER, FALSE);
			ENABLE_ITEM(IDC_COMBO_SERVICE, FALSE);
			ENABLE_ITEM(IDC_BUTTON_CHSCAN, FALSE);
			ENABLE_ITEM(IDC_BUTTON_EPG, FALSE);
			ENABLE_ITEM(IDC_BUTTON_SET, FALSE);
			ENABLE_ITEM(IDC_BUTTON_REC, FALSE);
			ENABLE_ITEM(IDC_COMBO_REC_H, TRUE);
			ENABLE_ITEM(IDC_COMBO_REC_M, TRUE);
			ENABLE_ITEM(IDC_CHECK_REC_SET, TRUE);
			Button_SetCheck(GetDlgItem(IDC_CHECK_REC_SET), BST_UNCHECKED);
			ENABLE_ITEM(IDC_BUTTON_CANCEL, TRUE);
			break;
		case GUI_REC_SET_TIME:
			ENABLE_ITEM(IDC_COMBO_TUNER, FALSE);
			ENABLE_ITEM(IDC_COMBO_SERVICE, FALSE);
			ENABLE_ITEM(IDC_BUTTON_CHSCAN, FALSE);
			ENABLE_ITEM(IDC_BUTTON_EPG, FALSE);
			ENABLE_ITEM(IDC_BUTTON_SET, FALSE);
			ENABLE_ITEM(IDC_BUTTON_REC, FALSE);
			ENABLE_ITEM(IDC_COMBO_REC_H, FALSE);
			ENABLE_ITEM(IDC_COMBO_REC_M, FALSE);
			ENABLE_ITEM(IDC_CHECK_REC_SET, TRUE);
			ENABLE_ITEM(IDC_BUTTON_CANCEL, TRUE);
			break;
		case GUI_OTHER_CTRL:
			ENABLE_ITEM(IDC_COMBO_TUNER, FALSE);
			ENABLE_ITEM(IDC_COMBO_SERVICE, FALSE);
			ENABLE_ITEM(IDC_BUTTON_CHSCAN, FALSE);
			ENABLE_ITEM(IDC_BUTTON_EPG, FALSE);
			ENABLE_ITEM(IDC_BUTTON_SET, FALSE);
			ENABLE_ITEM(IDC_BUTTON_REC, FALSE);
			ENABLE_ITEM(IDC_COMBO_REC_H, FALSE);
			ENABLE_ITEM(IDC_COMBO_REC_M, FALSE);
			ENABLE_ITEM(IDC_CHECK_REC_SET, FALSE);
			ENABLE_ITEM(IDC_BUTTON_CANCEL, TRUE);
			break;
		case GUI_REC_STANDBY:
			ENABLE_ITEM(IDC_COMBO_TUNER, FALSE);
			ENABLE_ITEM(IDC_COMBO_SERVICE, FALSE);
			ENABLE_ITEM(IDC_BUTTON_CHSCAN, FALSE);
			ENABLE_ITEM(IDC_BUTTON_EPG, FALSE);
			ENABLE_ITEM(IDC_BUTTON_SET, FALSE);
			ENABLE_ITEM(IDC_BUTTON_REC, FALSE);
			ENABLE_ITEM(IDC_COMBO_REC_H, FALSE);
			ENABLE_ITEM(IDC_COMBO_REC_M, FALSE);
			ENABLE_ITEM(IDC_CHECK_REC_SET, FALSE);
			ENABLE_ITEM(IDC_BUTTON_CANCEL, FALSE);
			break;
		default:
			break;
	}
}



void CEpgDataCap_BonDlg::OnCbnSelchangeComboTuner()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	WCHAR buff[512];
	if( GetWindowText(GetDlgItem(IDC_COMBO_TUNER), buff, 512) > 0 ){
		if( SelectBonDriver(buff) ){
			int index = ReloadServiceList();
			if( index >= 0 ){
				SelectService(this->serviceList[index]);
			}
		}else{
			this->serviceList.clear();
			ComboBox_ResetContent(GetDlgItem(IDC_COMBO_SERVICE));
			UpdateTitleBarText();
		}
	}
	ChgIconStatus();
}


void CEpgDataCap_BonDlg::OnCbnSelchangeComboService()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	int sel = ComboBox_GetCurSel(GetDlgItem(IDC_COMBO_SERVICE));
	if( sel != CB_ERR ){
		DWORD index = (DWORD)ComboBox_GetItemData(GetDlgItem(IDC_COMBO_SERVICE), sel);
		SelectService(this->serviceList[index]);
	}
	UpdateTitleBarText();
	ChgIconStatus();
}


void CEpgDataCap_BonDlg::OnBnClickedButtonSet()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	CSettingDlg setDlg(m_hWnd);
	disableKeyboardHook = TRUE;
	INT_PTR result = setDlg.DoModal();
	disableKeyboardHook = FALSE;
	if( result == IDOK ){
		ReloadSetting();
		ReloadNWSet();
		ReloadServiceList(this->lastONID, this->lastTSID, this->bonCtrl.GetNWCtrlServiceID());
	}
}

void CEpgDataCap_BonDlg::ReloadNWSet()
{
	this->udpSendList.clear();
	this->tcpSendList.clear();
	this->bonCtrl.SendUdp(NULL);
	this->bonCtrl.SendTcp(NULL);
	if( this->setUdpSendList.empty() == false ){
		EnableWindow(GetDlgItem(IDC_CHECK_UDP), TRUE);
		if( Button_GetCheck(GetDlgItem(IDC_CHECK_UDP)) ){
			this->udpSendList = this->setUdpSendList;
			this->bonCtrl.SendUdp(&this->udpSendList);
		}
	}else{
		EnableWindow(GetDlgItem(IDC_CHECK_UDP), FALSE);
		Button_SetCheck(GetDlgItem(IDC_CHECK_UDP), BST_UNCHECKED);
	}
	if( this->setTcpSendList.empty() == false ){
		EnableWindow(GetDlgItem(IDC_CHECK_TCP), TRUE);
		if( Button_GetCheck(GetDlgItem(IDC_CHECK_TCP)) ){
			this->tcpSendList = this->setTcpSendList;
			this->bonCtrl.SendTcp(&this->tcpSendList);
		}
	}else{
		EnableWindow(GetDlgItem(IDC_CHECK_TCP), FALSE);
		Button_SetCheck(GetDlgItem(IDC_CHECK_TCP), BST_UNCHECKED);
	}
}

void CEpgDataCap_BonDlg::SetOverlayIcon(HICON icon)
{
	void* pv;
	if( SUCCEEDED(CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_ITaskbarList3, &pv)) ){
		((ITaskbarList3*)pv)->SetOverlayIcon(m_hWnd, icon, L"");
		((ITaskbarList3*)pv)->Release();
	}
}

void CEpgDataCap_BonDlg::UpdateTitleBarText()
{
	WCHAR szTitle[256];
	if( GetWindowText(m_hWnd, szTitle, 256) > 0 ){
		wstring title = szTitle;
		size_t sep = title.rfind(L" - ");
		if( sep == wstring::npos ){
			sep = title.rfind(L" ● ");
			if( sep == wstring::npos ){
				sep = title.rfind(L" ○ ");
			}
		}
		if( this->modifyTitleBarText ){
			if( sep == wstring::npos ){
				title.insert(0, L" - ");
				sep = 0;
			}
			WCHAR szBuff[128];
			if( GetWindowText(GetDlgItem(IDC_COMBO_SERVICE), szBuff, 128) > 0 ){
				title.replace(0, sep, szBuff);
			}else{
				title.erase(0, sep);
			}
		}else if( sep != wstring::npos ){
			title.erase(0, sep + 3);
		}
		if( title != szTitle ){
			SetWindowText(m_hWnd, title.c_str());
		}
	}
}

int CEpgDataCap_BonDlg::ReloadServiceList(int selONID, int selTSID, int selSID)
{
	this->serviceList.clear();
	ComboBox_ResetContent(GetDlgItem(IDC_COMBO_SERVICE));

	DWORD ret = this->bonCtrl.GetServiceList(&this->serviceList);
	if( ret != NO_ERR || this->serviceList.size() == 0 ){
		WCHAR log[512 + 64] = L"";
		GetDlgItemText(m_hWnd, IDC_EDIT_LOG, log, 512);
		if( wcsstr(log, L"チャンネル情報の読み込みに失敗しました\r\n") == NULL ){
			wcscat_s(log, L"チャンネル情報の読み込みに失敗しました\r\n");
			SetDlgItemText(m_hWnd, IDC_EDIT_LOG, log);
		}
	}else{
		int selectIndex = -1;
		int selectSel = -1;
		for( size_t i=0; i<this->serviceList.size(); i++ ){
			if( selectIndex < 0 ||
			    (this->serviceList[i].originalNetworkID == selONID &&
			     this->serviceList[i].transportStreamID == selTSID &&
			     this->serviceList[i].serviceID == selSID) ){
				//一覧には表示しないがリストには存在する場合もある
				selectIndex = (int)i;
			}
			if( this->serviceList[i].useViewFlag == TRUE ){
				int index = ComboBox_AddString(GetDlgItem(IDC_COMBO_SERVICE), this->serviceList[i].serviceName.c_str());
				ComboBox_SetItemData(GetDlgItem(IDC_COMBO_SERVICE), index, i);
				if( selectSel < 0 || selectIndex == (int)i ){
					selectSel = index;
				}
			}
		}
		if( selectSel >= 0 ){
			ComboBox_SetCurSel(GetDlgItem(IDC_COMBO_SERVICE), selectSel);
		}
		UpdateTitleBarText();
		return selectIndex;
	}
	UpdateTitleBarText();
	return -1;
}

BOOL CEpgDataCap_BonDlg::SelectBonDriver(LPCWSTR fileName)
{
	this->lastONID = 0xFFFF;
	this->lastTSID = 0xFFFF;
	BOOL ret = this->bonCtrl.OpenBonDriver(fileName, this->openWait, this->tsBuffMaxCount);
	if( ret == FALSE ){
		wstring log;
		Format(log, L"BonDriverのオープンができませんでした\r\n%ls\r\n", fileName);
		SetDlgItemText(m_hWnd, IDC_EDIT_LOG, log.c_str());
		BtnUpdate(GUI_OPEN_FAIL);
	}else{
		SetDlgItemText(m_hWnd, IDC_EDIT_LOG, L"");
		BtnUpdate(GUI_NORMAL);
	}
	return ret;
}

BOOL CEpgDataCap_BonDlg::SelectService(const CH_DATA4& chData)
{
	if( this->bonCtrl.SetCh(chData.space, chData.ch, chData.serviceID) ){
		this->lastONID = chData.originalNetworkID;
		this->lastTSID = chData.transportStreamID;
		SetDlgItemText(m_hWnd, IDC_EDIT_LOG, L"");
		return TRUE;
	}
	SetDlgItemText(m_hWnd, IDC_EDIT_LOG, L"チャンネル変更できませんでした\r\n");
	return FALSE;
}

void CEpgDataCap_BonDlg::OnBnClickedButtonChscan()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	if( this->bonCtrl.StartChScan() == FALSE ){
		SetDlgItemText(m_hWnd, IDC_EDIT_LOG, L"チャンネルスキャンを開始できませんでした\r\n");
		return;
	}
	this->chScanWorking = TRUE;
	BtnUpdate(GUI_CANCEL_ONLY);
}


void CEpgDataCap_BonDlg::OnBnClickedButtonEpg()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	if( this->bonCtrl.StartEpgCap(NULL) == FALSE ){
		SetDlgItemText(m_hWnd, IDC_EDIT_LOG, L"EPG取得を開始できませんでした\r\n");
		return;
	}
	this->epgCapWorking = TRUE;
	BtnUpdate(GUI_CANCEL_ONLY);
	ChgIconStatus();
}


void CEpgDataCap_BonDlg::OnBnClickedButtonRec()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	if( this->bonCtrl.IsRec() || this->recCtrlID != 0 ){
		return;
	}

	//即時録画
	this->recCtrlID = this->bonCtrl.CreateServiceCtrl(TRUE);
	wstring serviceName;
	Format(serviceName, L"%04X", this->bonCtrl.GetNWCtrlServiceID());
	for( size_t i = 0; i < this->serviceList.size(); i++ ){
		if( this->serviceList[i].originalNetworkID == this->lastONID &&
		    this->serviceList[i].transportStreamID == this->lastTSID &&
		    this->serviceList[i].serviceID == this->bonCtrl.GetNWCtrlServiceID() ){
			serviceName = this->serviceList[i].serviceName;
			break;
		}
	}
	wstring fileName = this->recFileName;
	SYSTEMTIME now;
	ConvertSystemTime(GetNowI64Time(), &now);
	for( int i = 0; GetTimeMacroName(i); i++ ){
		wstring name;
		UTF8toW(GetTimeMacroName(i), name);
		Replace(fileName, L'$' + name + L'$', GetTimeMacroValue(i, now));
	}
	Replace(fileName, L"$ServiceName$", serviceName);
	CheckFileName(fileName);

	SET_CTRL_REC_PARAM recParam;
	recParam.ctrlID = this->recCtrlID;
	recParam.fileName = L"padding.ts";
	recParam.overWriteFlag = this->overWriteFlag != FALSE;
	recParam.createSize = 0;
	recParam.saveFolder.resize(1);
	recParam.saveFolder.back().recFolder = this->recFolderList[0];
	recParam.saveFolder.back().recFileName = fileName;
	recParam.pittariFlag = FALSE;
	if( this->bonCtrl.StartSave(recParam, this->recFolderList, this->writeBuffMaxCount) == FALSE ){
		this->bonCtrl.DeleteServiceCtrl(this->recCtrlID);
		this->recCtrlID = 0;
		SetDlgItemText(m_hWnd, IDC_EDIT_LOG, L"録画を開始できませんでした\r\n");
		return;
	}
	SYSTEMTIME end;
	ConvertSystemTime(GetNowI64Time() + 30 * 60 * I64_1SEC, &end);

	ComboBox_SetCurSel(GetDlgItem(IDC_COMBO_REC_H), end.wHour);
	ComboBox_SetCurSel(GetDlgItem(IDC_COMBO_REC_M), end.wMinute);

	SetDlgItemText(m_hWnd, IDC_EDIT_LOG, L"録画中\r\n");

	BtnUpdate(GUI_REC);
	ChgIconStatus();
}


void CEpgDataCap_BonDlg::OnBnClickedButtonCancel()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	if( this->bonCtrl.IsRec() ){
		WCHAR caption[128] = L"";
		GetWindowText(m_hWnd, caption, 128);
		disableKeyboardHook = TRUE;
		int result = MessageBox( m_hWnd, L"録画を停止しますか？", caption, MB_YESNO | MB_ICONQUESTION );
		disableKeyboardHook = FALSE;
		if( result == IDNO ){
			return ;
		}
	}
	SetDlgItemText(m_hWnd, IDC_EDIT_LOG, L"キャンセルされました\r\n");

	this->bonCtrl.StopChScan();
	this->chScanWorking = FALSE;
	this->bonCtrl.StopEpgCap();
	this->epgCapWorking = FALSE;
	if( this->recCtrlID != 0 ){
		this->bonCtrl.DeleteServiceCtrl(this->recCtrlID);
		this->recCtrlID = 0;
	}
	KillTimer(TIMER_REC_END);
	while( this->cmdCtrlList.empty() == false ){
		this->bonCtrl.DeleteServiceCtrl(this->cmdCtrlList.back());
		this->cmdCtrlList.pop_back();
	}

	BtnUpdate(GUI_NORMAL);
	ChgIconStatus();
}


void CEpgDataCap_BonDlg::OnBnClickedButtonView()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	SendMessage(m_hWnd, WM_VIEW_APP_OPEN, 0, 0);
}


void CEpgDataCap_BonDlg::OnBnClickedCheckUdp()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	if( Button_GetCheck(GetDlgItem(IDC_CHECK_UDP)) ){
		this->udpSendList = this->setUdpSendList;
	}else{
		this->udpSendList.clear();
	}
	this->bonCtrl.SendUdp(this->udpSendList.empty() ? NULL : &this->udpSendList);
}


void CEpgDataCap_BonDlg::OnBnClickedCheckTcp()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	if( Button_GetCheck(GetDlgItem(IDC_CHECK_TCP)) ){
		this->tcpSendList = this->setTcpSendList;
	}else{
		this->tcpSendList.clear();
	}
	this->bonCtrl.SendTcp(this->tcpSendList.empty() ? NULL : &this->tcpSendList);
}


void CEpgDataCap_BonDlg::OnBnClickedCheckRecSet()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	if( Button_GetCheck(GetDlgItem(IDC_CHECK_REC_SET)) != BST_UNCHECKED ){
		BtnUpdate(GUI_REC_SET_TIME);

		int selH = ComboBox_GetCurSel(GetDlgItem(IDC_COMBO_REC_H));
		int selM = ComboBox_GetCurSel(GetDlgItem(IDC_COMBO_REC_M));

		DWORD nowTime = (DWORD)(GetNowI64Time() / I64_1SEC % (24*60*60));
		DWORD endTime = selH*60*60 + selM*60;

		if( nowTime > endTime ){
			endTime += 24*60*60;
		}
		SetTimer(TIMER_REC_END, (endTime-nowTime)*1000, NULL );
	}else{
		BtnUpdate(GUI_REC);
		KillTimer(TIMER_REC_END);
	}
}


void CEpgDataCap_BonDlg::OnBnClickedCheckNextpg()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	wstring info = L"";
	WORD onid;
	WORD tsid;
	EPGDB_EVENT_INFO eventInfo;
	if( this->bonCtrl.GetStreamID(&onid, &tsid) &&
	    this->bonCtrl.GetEpgInfo(onid, tsid, this->bonCtrl.GetNWCtrlServiceID(),
	                             Button_GetCheck(GetDlgItem(IDC_CHECK_NEXTPG)), &eventInfo) == NO_ERR ){
		info = ConvertEpgInfoText(&eventInfo);
	}
	SetDlgItemText(m_hWnd, IDC_EDIT_PG_INFO, info.c_str());
}


BOOL CEpgDataCap_BonDlg::OnQueryEndSession()
{
	// TODO:  ここに特定なクエリの終了セッション コードを追加してください。
	if( this->bonCtrl.IsRec() ){
		ShowWindow(m_hWnd, SW_SHOW);
		return FALSE;
	}
	return TRUE;
}


void CEpgDataCap_BonDlg::OnEndSession(BOOL bEnding)
{
	// TODO: ここにメッセージ ハンドラー コードを追加します。
	if( bEnding == TRUE ){
		while( this->cmdCtrlList.empty() == false ){
			this->bonCtrl.DeleteServiceCtrl(this->cmdCtrlList.back());
			this->cmdCtrlList.pop_back();
		}
		if( this->recCtrlID != 0 ){
			this->bonCtrl.DeleteServiceCtrl(this->recCtrlID);
			this->recCtrlID = 0;
		}
	}
}


LRESULT CALLBACK CEpgDataCap_BonDlg::KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	//Enter,Escを無視する
	if( disableKeyboardHook == FALSE && nCode == HC_ACTION && (wParam == VK_RETURN || wParam == VK_ESCAPE) && (lParam & (1 << 30)) == 0 ){
		return TRUE;
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}


INT_PTR CALLBACK CEpgDataCap_BonDlg::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CEpgDataCap_BonDlg* pSys = (CEpgDataCap_BonDlg*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
	if( pSys == NULL && uMsg != WM_INITDIALOG ){
		return FALSE;
	}
	switch( uMsg ){
	case WM_INITDIALOG:
		SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
		pSys = (CEpgDataCap_BonDlg*)lParam;
		pSys->m_hWnd = hDlg;
		pSys->m_hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD, KeyboardProc, NULL, GetCurrentThreadId());
		return pSys->OnInitDialog();
	case WM_NCDESTROY:
		UnhookWindowsHookEx(pSys->m_hKeyboardHook);
		pSys->m_hWnd = NULL;
		break;
	case WM_DESTROY:
		pSys->OnDestroy();
		break;
	case WM_TIMER:
		pSys->OnTimer(wParam);
		break;
	case WM_SIZE:
		pSys->OnSize((UINT)wParam, LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_QUERYENDSESSION:
		SetWindowLongPtr(hDlg, DWLP_MSGRESULT, pSys->OnQueryEndSession());
		return TRUE;
	case WM_ENDSESSION:
		pSys->OnEndSession((BOOL)wParam);
		break;
	case WM_COMMAND:
		switch( LOWORD(wParam) ){
		case IDC_COMBO_TUNER:
			if( HIWORD(wParam) == CBN_SELCHANGE ){
				pSys->OnCbnSelchangeComboTuner();
			}
			break;
		case IDC_COMBO_SERVICE:
			if( HIWORD(wParam) == CBN_SELCHANGE ){
				pSys->OnCbnSelchangeComboService();
			}
			break;
		case IDC_BUTTON_SET:
			pSys->OnBnClickedButtonSet();
			break;
		case IDC_BUTTON_CHSCAN:
			pSys->OnBnClickedButtonChscan();
			break;
		case IDC_BUTTON_EPG:
			pSys->OnBnClickedButtonEpg();
			break;
		case IDC_BUTTON_REC:
			pSys->OnBnClickedButtonRec();
			break;
		case IDC_CHECK_REC_SET:
			pSys->OnBnClickedCheckRecSet();
			break;
		case IDC_BUTTON_CANCEL:
			pSys->OnBnClickedButtonCancel();
			break;
		case IDC_CHECK_TCP:
			pSys->OnBnClickedCheckTcp();
			break;
		case IDC_CHECK_UDP:
			pSys->OnBnClickedCheckUdp();
			break;
		case IDC_BUTTON_VIEW:
			pSys->OnBnClickedButtonView();
			break;
		case IDC_CHECK_NEXTPG:
			pSys->OnBnClickedCheckNextpg();
			break;
		case IDOK:
		case IDCANCEL:
			while( pSys->cmdCtrlList.empty() == false ){
				pSys->bonCtrl.DeleteServiceCtrl(pSys->cmdCtrlList.back());
				pSys->cmdCtrlList.pop_back();
			}
			if( pSys->recCtrlID != 0 ){
				pSys->bonCtrl.DeleteServiceCtrl(pSys->recCtrlID);
				pSys->recCtrlID = 0;
			}
			//デッドロック回避のためメッセージポンプを維持しつつサーバを終わらせる
			pSys->pipeServer.StopServer(true);
			pSys->SetTimer(TIMER_TRY_STOP_SERVER, 20, NULL);
			SetWindowLongPtr(hDlg, DWLP_MSGRESULT, 0);
			return TRUE;
		}
		break;
	case WM_SYSCOMMAND:
		{
			BOOL bProcessed = FALSE;
			pSys->OnSysCommand((UINT)(wParam & 0xFFF0), lParam, &bProcessed);
			if( bProcessed != FALSE ){
				SetWindowLongPtr(hDlg, DWLP_MSGRESULT, 0);
				return TRUE;
			}
		}
		break;
	default:
		if( uMsg == taskbarCreated ){
			pSys->OnTaskbarCreated(wParam, lParam);
		}else if( uMsg >= WM_USER ){
			pSys->WindowProc(uMsg, wParam, lParam);
		}
		break;
	}
	return FALSE;
}


void CEpgDataCap_BonDlg::StartPipeServer()
{
	wstring pipeName;
	Format(pipeName, L"%ls%d", CMD2_VIEW_CTRL_PIPE, GetCurrentProcessId());
	OutputDebugString(pipeName.c_str());
	this->pipeServer.StartServer(pipeName, [this](CMD_STREAM* cmdParam, CMD_STREAM* resParam) {
		resParam->param = CMD_ERR;
		//同期呼び出しが不要なコマンドはここで処理する
		switch( cmdParam->param ){
		case CMD2_VIEW_APP_GET_BONDRIVER:
			{
				wstring bonFile;
				if( this->bonCtrl.GetOpenBonDriver(&bonFile) ){
					resParam->data = NewWriteVALUE(bonFile, resParam->dataSize);
					resParam->param = CMD_SUCCESS;
				}
			}
			return;
		case CMD2_VIEW_APP_GET_DELAY:
			resParam->data = NewWriteVALUE(this->bonCtrl.GetTimeDelay(), resParam->dataSize);
			resParam->param = CMD_SUCCESS;
			return;
		case CMD2_VIEW_APP_GET_STATUS:
			{
				DWORD val = VIEW_APP_ST_NORMAL;
				BOOL chChgErr;
				if( this->bonCtrl.GetOpenBonDriver(NULL) == FALSE ){
					val = VIEW_APP_ST_ERR_BON;
				}else if( this->bonCtrl.IsRec() ){
					val = VIEW_APP_ST_REC;
				}else if( this->bonCtrl.GetEpgCapStatus(NULL) == CBonCtrl::ST_WORKING ){
					val = VIEW_APP_ST_GET_EPG;
				}else if( this->bonCtrl.IsChChanging(&chChgErr) == FALSE && chChgErr ){
					val = VIEW_APP_ST_ERR_CH_CHG;
				}
				resParam->data = NewWriteVALUE(val, resParam->dataSize);
				resParam->param = CMD_SUCCESS;
			}
			return;
		case CMD2_VIEW_APP_CLOSE:
			OutputDebugString(L"CMD2_VIEW_APP_CLOSE");
			PostMessage(m_hWnd, WM_CLOSE, 0, 0);
			resParam->param = CMD_SUCCESS;
			return;
		case CMD2_VIEW_APP_SET_ID:
			OutputDebugString(L"CMD2_VIEW_APP_SET_ID");
			if( ReadVALUE(&this->outCtrlID, cmdParam->data, cmdParam->dataSize, NULL) ){
				resParam->param = CMD_SUCCESS;
			}
			return;
		case CMD2_VIEW_APP_GET_ID:
			OutputDebugString(L"CMD2_VIEW_APP_GET_ID");
			resParam->data = NewWriteVALUE(this->outCtrlID, resParam->dataSize);
			resParam->param = CMD_SUCCESS;
			return;
		case CMD2_VIEW_APP_REC_FILE_PATH:
			OutputDebugString(L"CMD2_VIEW_APP_REC_FILE_PATH");
			{
				DWORD id;
				if( ReadVALUE(&id, cmdParam->data, cmdParam->dataSize, NULL) ){
					wstring saveFile = this->bonCtrl.GetSaveFilePath(id);
					if( saveFile.size() > 0 ){
						resParam->data = NewWriteVALUE(saveFile, resParam->dataSize);
						resParam->param = CMD_SUCCESS;
					}
				}
			}
			return;
		case CMD2_VIEW_APP_SEARCH_EVENT:
			{
				SEARCH_EPG_INFO_PARAM key;
				EPGDB_EVENT_INFO epgInfo;
				if( ReadVALUE(&key, cmdParam->data, cmdParam->dataSize, NULL) &&
				    this->bonCtrl.SearchEpgInfo(key.ONID, key.TSID, key.SID, key.eventID, key.pfOnlyFlag, &epgInfo) == NO_ERR ){
					resParam->data = NewWriteVALUE(epgInfo, resParam->dataSize);
					resParam->param = CMD_SUCCESS;
				}
			}
			return;
		case CMD2_VIEW_APP_GET_EVENT_PF:
			{
				GET_EPG_PF_INFO_PARAM key;
				EPGDB_EVENT_INFO epgInfo;
				if( ReadVALUE(&key, cmdParam->data, cmdParam->dataSize, NULL) &&
				    this->bonCtrl.GetEpgInfo(key.ONID, key.TSID, key.SID, key.pfNextFlag, &epgInfo) == NO_ERR ){
					resParam->data = NewWriteVALUE(epgInfo, resParam->dataSize);
					resParam->param = CMD_SUCCESS;
				}
			}
			return;
		case CMD2_VIEW_APP_EXEC_VIEW_APP:
			//原作は同期的
			PostMessage(m_hWnd, WM_VIEW_APP_OPEN, 0, 0);
			resParam->param = CMD_SUCCESS;
			return;
		}
		//CtrlCmdCallbackInvoked()をメインスレッドで呼ぶ
		//注意: CPipeServerがアクティブな間、ウィンドウは確実に存在しなければならない
		this->cmdCapture = cmdParam;
		this->resCapture = resParam;
		SendMessage(m_hWnd, WM_INVOKE_CTRL_CMD, 0, 0);
		this->cmdCapture = NULL;
		this->resCapture = NULL;
	});
}


void CEpgDataCap_BonDlg::CtrlCmdCallbackInvoked()
{
	CMD_STREAM* cmdParam = this->cmdCapture;
	CMD_STREAM* resParam = this->resCapture;

	switch( cmdParam->param ){
	case CMD2_VIEW_APP_SET_BONDRIVER:
		OutputDebugString(L"CMD2_VIEW_APP_SET_BONDRIVER");
		{
			wstring val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) ){
				if( SelectBonDriver(val.c_str()) ){
					ReloadServiceList();
					//可能なら一覧の表示を同期しておく
					for( int i = 0; i < ComboBox_GetCount(GetDlgItem(IDC_COMBO_TUNER)); i++ ){
						WCHAR buff[512];
						if( ComboBox_GetLBTextLen(GetDlgItem(IDC_COMBO_TUNER), i) < 512 &&
						    ComboBox_GetLBText(GetDlgItem(IDC_COMBO_TUNER), i, buff) > 0 &&
						    UtilComparePath(buff, val.c_str()) == 0 ){
							ComboBox_SetCurSel(GetDlgItem(IDC_COMBO_TUNER), i);
							break;
						}
					}
					ChgIconStatus();
					resParam->param = CMD_SUCCESS;
				}else{
					this->serviceList.clear();
					ComboBox_ResetContent(GetDlgItem(IDC_COMBO_SERVICE));
					UpdateTitleBarText();
				}
			}
		}
		break;
	case CMD2_VIEW_APP_SET_CH:
		OutputDebugString(L"CMD2_VIEW_APP_SET_CH");
		{
			SET_CH_INFO val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) ){
				if( val.useSID ){
					int index = ReloadServiceList(val.ONID, val.TSID, val.SID);
					if( index >= 0 && SelectService(this->serviceList[index]) ){
						ChgIconStatus();
						resParam->param = CMD_SUCCESS;
					}
				}else if( val.useBonCh ){
					for( size_t i = 0; i < this->serviceList.size(); i++ ){
						if( (DWORD)this->serviceList[i].space == val.space &&
						    (DWORD)this->serviceList[i].ch == val.ch ){
							int index = ReloadServiceList(this->serviceList[i].originalNetworkID,
							                              this->serviceList[i].transportStreamID,
							                              this->serviceList[i].serviceID);
							if( index >= 0 && SelectService(this->serviceList[index]) ){
								ChgIconStatus();
								resParam->param = CMD_SUCCESS;
							}
							break;
						}
					}
				}
			}
		}
		break;
	case CMD2_VIEW_APP_SET_STANDBY_REC:
		OutputDebugString(L"CMD2_VIEW_APP_SET_STANDBY_REC");
		{
			DWORD val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) ){
				if( val == 1 ){
					BtnUpdate(GUI_REC_STANDBY);
					SetDlgItemText(m_hWnd, IDC_EDIT_LOG, L"予約録画待機中\r\n");
				}else if( val == 2 ){
					BtnUpdate(GUI_NORMAL);
					SetDlgItemText(m_hWnd, IDC_EDIT_LOG, L"視聴モード\r\n");
				}else{
					BtnUpdate(GUI_NORMAL);
					SetDlgItemText(m_hWnd, IDC_EDIT_LOG, L"");
				}
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_VIEW_APP_CREATE_CTRL:
		OutputDebugString(L"CMD2_VIEW_APP_CREATE_CTRL");
		{
			DWORD val = this->bonCtrl.CreateServiceCtrl(FALSE);
			this->cmdCtrlList.push_back(val);
			resParam->data = NewWriteVALUE(val, resParam->dataSize);
			resParam->param = CMD_SUCCESS;
		}
		break;
	case CMD2_VIEW_APP_DELETE_CTRL:
		OutputDebugString(L"CMD2_VIEW_APP_DELETE_CTRL");
		{
			DWORD val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) ){
				auto itr = std::find(this->cmdCtrlList.begin(), this->cmdCtrlList.end(), val);
				if( itr != this->cmdCtrlList.end() ){
					this->cmdCtrlList.erase(itr);
					if( this->bonCtrl.DeleteServiceCtrl(val) ){
						WORD sid;
						if( this->cmdCtrlList.empty() == false &&
						    this->bonCtrl.GetServiceID(this->cmdCtrlList.front(), &sid) && sid != 0xFFFF ){
							this->bonCtrl.SetNWCtrlServiceID(sid);
							ReloadServiceList(this->lastONID, this->lastTSID, sid);
						}
						resParam->param = CMD_SUCCESS;
					}
				}
			}
		}
		break;
	case CMD2_VIEW_APP_SET_CTRLMODE:
		OutputDebugString(L"CMD2_VIEW_APP_SET_CTRLMODE");
		{
			SET_CTRL_MODE val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) ){
				this->bonCtrl.SetScramble(val.ctrlID, val.enableScramble);
				this->bonCtrl.SetServiceMode(val.ctrlID, val.enableCaption, val.enableData);
				this->bonCtrl.SetServiceID(val.ctrlID, val.SID);
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_VIEW_APP_REC_START_CTRL:
		OutputDebugString(L"CMD2_VIEW_APP_REC_START_CTRL");
		{
			SET_CTRL_REC_PARAM val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) ){
				if( val.overWriteFlag == 2 ){
					val.overWriteFlag = this->overWriteFlag != FALSE;
				}
				this->bonCtrl.ClearErrCount(val.ctrlID);
				if( this->bonCtrl.StartSave(val, this->recFolderList, this->writeBuffMaxCount) ){
					BtnUpdate(GUI_OTHER_CTRL);
					WCHAR log[512 + 64] = L"";
					GetDlgItemText(m_hWnd, IDC_EDIT_LOG, log, 512);
					if( wcsstr(log, L"予約録画中\r\n") == NULL ){
						wcscat_s(log, L"予約録画中\r\n");
						SetDlgItemText(m_hWnd, IDC_EDIT_LOG, log);
					}
					ChgIconStatus();
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD2_VIEW_APP_REC_STOP_CTRL:
		OutputDebugString(L"CMD2_VIEW_APP_REC_STOP_CTRL");
		{
			SET_CTRL_REC_STOP_PARAM val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) ){
				SET_CTRL_REC_STOP_RES_PARAM resVal;
				resVal.recFilePath = this->bonCtrl.GetSaveFilePath(val.ctrlID);
				resVal.drop = 0;
				resVal.scramble = 0;
				if( resVal.recFilePath.empty() == false && val.saveErrLog ){
					fs_path infoPath = GetPrivateProfileToString(L"SET", L"RecInfoFolder", L"", GetCommonIniPath().c_str());
					if( infoPath.empty() ){
						infoPath = resVal.recFilePath + L".err";
					}else{
						infoPath.append(fs_path(resVal.recFilePath).filename().concat(L".err").native());
					}
					this->bonCtrl.SaveErrCount(val.ctrlID, infoPath.native(), this->dropLogAsUtf8, this->dropSaveThresh,
					                           this->scrambleSaveThresh, resVal.drop, resVal.scramble);
				}else{
					this->bonCtrl.GetErrCount(val.ctrlID, &resVal.drop, &resVal.scramble);
				}
				BOOL subRec;
				if( this->bonCtrl.EndSave(val.ctrlID, &subRec) ){
					resVal.subRecFlag = subRec != FALSE;
					resParam->data = NewWriteVALUE(resVal, resParam->dataSize);
					resParam->param = CMD_SUCCESS;
					if( this->cmdCtrlList.size() == 1 ){
						BtnUpdate(GUI_NORMAL);
						SetDlgItemText(m_hWnd, IDC_EDIT_LOG, L"予約録画終了しました\r\n");
					}
					ChgIconStatus();
				}
			}
		}
		break;
	case CMD2_VIEW_APP_EPGCAP_START:
		OutputDebugString(L"CMD2_VIEW_APP_EPGCAP_START");
		{
			vector<SET_CH_INFO> val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) ){
				if( this->bonCtrl.StartEpgCap(&val) ){
					this->epgCapWorking = TRUE;
					BtnUpdate(GUI_CANCEL_ONLY);
					ChgIconStatus();
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD2_VIEW_APP_EPGCAP_STOP:
		OutputDebugString(L"CMD2_VIEW_APP_EPGCAP_STOP");
		this->bonCtrl.StopEpgCap();
		ChgIconStatus();
		resParam->param = CMD_SUCCESS;
		break;
	case CMD2_VIEW_APP_REC_STOP_ALL:
		OutputDebugString(L"CMD2_VIEW_APP_REC_STOP_ALL");
		while( this->cmdCtrlList.empty() == false ){
			this->bonCtrl.DeleteServiceCtrl(this->cmdCtrlList.back());
			this->cmdCtrlList.pop_back();
		}
		if( this->recCtrlID != 0 ){
			this->bonCtrl.DeleteServiceCtrl(this->recCtrlID);
			this->recCtrlID = 0;
		}
		BtnUpdate(GUI_NORMAL);
		SetDlgItemText(m_hWnd, IDC_EDIT_LOG, L"予約録画終了しました\r\n");
		ChgIconStatus();
		resParam->param = CMD_SUCCESS;
		break;
	case CMD2_VIEW_APP_REC_WRITE_SIZE:
		{
			DWORD val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL) ){
				__int64 writeSize = -1;
				this->bonCtrl.GetRecWriteSize(val, &writeSize);
				resParam->data = NewWriteVALUE(writeSize, resParam->dataSize);
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	default:
		_OutputDebugString(L"err default cmd %d\r\n", cmdParam->param);
		resParam->param = CMD_NON_SUPPORT;
		break;
	}
}
