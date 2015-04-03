
// EpgDataCap_BonDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "EpgDataCap_Bon.h"
#include "EpgDataCap_BonDlg.h"
#include "afxdialogex.h"

#include "../../Common/TimeUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CEpgDataCap_BonDlg ダイアログ


UINT CEpgDataCap_BonDlg::taskbarCreated = 0;

CEpgDataCap_BonDlg::CEpgDataCap_BonDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CEpgDataCap_BonDlg::IDD, pParent)
	, log(_T(""))
	, statusLog(_T(""))
	, pgInfo(_T(""))
{
	m_hIcon = (HICON)LoadImage( AfxGetInstanceHandle(), MAKEINTRESOURCE( IDI_ICON_BLUE ), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	m_hIcon2 = (HICON)LoadImage( AfxGetInstanceHandle(), MAKEINTRESOURCE( IDI_ICON_BLUE ), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
	iconRed = (HICON)LoadImage( AfxGetInstanceHandle(), MAKEINTRESOURCE( IDI_ICON_RED ), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	iconBlue = (HICON)LoadImage( AfxGetInstanceHandle(), MAKEINTRESOURCE( IDI_ICON_BLUE ), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	iconGreen = (HICON)LoadImage( AfxGetInstanceHandle(), MAKEINTRESOURCE( IDI_ICON_GREEN ), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	iconGray = (HICON)LoadImage( AfxGetInstanceHandle(), MAKEINTRESOURCE( IDI_ICON_GRAY ), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

	wstring strPath = L"";
	GetModuleIniPath(strPath);
	this->moduleIniPath = strPath.c_str();
	GetCommonIniPath(strPath);
	this->commonIniPath = strPath.c_str();
	GetEpgTimerSrvIniPath(strPath);
	this->timerSrvIniPath = strPath.c_str();

	this->initONID = GetPrivateProfileInt( L"Set", L"LastONID", -1, this->moduleIniPath );
	this->initTSID = GetPrivateProfileInt( L"Set", L"LastTSID", -1, this->moduleIniPath );
	this->initSID = GetPrivateProfileInt( L"Set", L"LastSID", -1, this->moduleIniPath );
	WCHAR buff[512]=L"";
	GetPrivateProfileString( L"Set", L"LastBon", L"", buff, 512, this->moduleIniPath );
	this->iniBonDriver = buff;

	iniView = FALSE;
	iniNetwork = TRUE;
	iniMin = FALSE;
	this->iniUDP = FALSE;
	this->iniTCP = FALSE;
	
	this->minTask = GetPrivateProfileInt( L"Set", L"MinTask", 0, this->moduleIniPath );
	this->openLastCh = GetPrivateProfileInt( L"Set", L"OpenLast", 1, this->moduleIniPath );
	if( this->openLastCh == 0 ){
		if( GetPrivateProfileInt( L"Set", L"OpenFix", 0, this->moduleIniPath ) == 1){
			this->initONID = GetPrivateProfileInt( L"Set", L"FixONID", -1, this->moduleIniPath );
			this->initTSID = GetPrivateProfileInt( L"Set", L"FixTSID", -1, this->moduleIniPath );
			this->initSID = GetPrivateProfileInt( L"Set", L"FixSID", -1, this->moduleIniPath );
			GetPrivateProfileString( L"Set", L"FixBon", L"", buff, 512, this->moduleIniPath );
			this->iniBonDriver = buff;
		}else{
			this->initONID = -1;
			this->initTSID = -1;
			this->initSID = -1;
			this->iniBonDriver = L"";
		}
	}
	this->initOpenWait = 0;
	this->initChgWait = 0;
}

void CEpgDataCap_BonDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_TUNER, combTuner);
	DDX_Control(pDX, IDC_COMBO_SERVICE, combService);
	DDX_Control(pDX, IDC_BUTTON_CHSCAN, btnChScan);
	DDX_Control(pDX, IDC_BUTTON_EPG, btnEpg);
	DDX_Control(pDX, IDC_BUTTON_SET, btnSet);
	DDX_Control(pDX, IDC_BUTTON_REC, btnRec);
	DDX_Control(pDX, IDC_COMBO_REC_H, combRecH);
	DDX_Control(pDX, IDC_COMBO_REC_M, combRecM);
	DDX_Control(pDX, IDC_CHECK_REC_SET, chkRecSet);
	DDX_Control(pDX, IDC_BUTTON_CANCEL, btnCancel);
	DDX_Control(pDX, IDC_BUTTON_VIEW, btnView);
	DDX_Control(pDX, IDC_CHECK_UDP, chkUDP);
	DDX_Control(pDX, IDC_CHECK_TCP, chkTCP);
	DDX_Text(pDX, IDC_EDIT_LOG, log);
	DDX_Text(pDX, IDC_EDIT_STATUS, statusLog);
	DDX_Text(pDX, IDC_EDIT_PG_INFO, pgInfo);
	DDX_Control(pDX, IDC_EDIT_STATUS, editStatus);
	DDX_Control(pDX, IDC_CHECK_NEXTPG, btnPgNext);
	DDX_Control(pDX, IDC_BUTTON_TIMESHIFT, btnTimeShift);
}

BEGIN_MESSAGE_MAP(CEpgDataCap_BonDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_CREATE()
	ON_WM_SYSCOMMAND()
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_WM_SIZE()
	ON_REGISTERED_MESSAGE(CEpgDataCap_BonDlg::taskbarCreated, OnTaskbarCreated)
	ON_CBN_SELCHANGE(IDC_COMBO_TUNER, &CEpgDataCap_BonDlg::OnCbnSelchangeComboTuner)
	ON_CBN_SELCHANGE(IDC_COMBO_SERVICE, &CEpgDataCap_BonDlg::OnCbnSelchangeComboService)
	ON_BN_CLICKED(IDC_BUTTON_SET, &CEpgDataCap_BonDlg::OnBnClickedButtonSet)
	ON_BN_CLICKED(IDC_BUTTON_CHSCAN, &CEpgDataCap_BonDlg::OnBnClickedButtonChscan)
	ON_BN_CLICKED(IDC_BUTTON_EPG, &CEpgDataCap_BonDlg::OnBnClickedButtonEpg)
	ON_BN_CLICKED(IDC_BUTTON_REC, &CEpgDataCap_BonDlg::OnBnClickedButtonRec)
	ON_BN_CLICKED(IDC_BUTTON_CANCEL, &CEpgDataCap_BonDlg::OnBnClickedButtonCancel)
	ON_BN_CLICKED(IDC_BUTTON_VIEW, &CEpgDataCap_BonDlg::OnBnClickedButtonView)
	ON_BN_CLICKED(IDC_CHECK_UDP, &CEpgDataCap_BonDlg::OnBnClickedCheckUdp)
	ON_BN_CLICKED(IDC_CHECK_TCP, &CEpgDataCap_BonDlg::OnBnClickedCheckTcp)
	ON_BN_CLICKED(IDC_CHECK_REC_SET, &CEpgDataCap_BonDlg::OnBnClickedCheckRecSet)
	ON_BN_CLICKED(IDC_CHECK_NEXTPG, &CEpgDataCap_BonDlg::OnBnClickedCheckNextpg)
	ON_BN_CLICKED(IDC_BUTTON_TIMESHIFT, &CEpgDataCap_BonDlg::OnBnClickedButtonTimeshift)
	ON_WM_QUERYENDSESSION()
	ON_WM_ENDSESSION()
END_MESSAGE_MAP()


// CEpgDataCap_BonDlg メッセージ ハンドラー
void CEpgDataCap_BonDlg::SetInitBon(CString bonFile)
{
	iniBonDriver = bonFile;
	if( GetPrivateProfileInt( iniBonDriver.GetBuffer(0), L"OpenFix", 0, this->moduleIniPath ) == 1){
		OutputDebugString(L"強制サービス指定 設定値ロード");
		this->initONID = GetPrivateProfileInt( iniBonDriver.GetBuffer(0), L"FixONID", -1, this->moduleIniPath );
		this->initTSID = GetPrivateProfileInt( iniBonDriver.GetBuffer(0), L"FixTSID", -1, this->moduleIniPath );
		this->initSID = GetPrivateProfileInt( iniBonDriver.GetBuffer(0), L"FixSID", -1, this->moduleIniPath );
		this->initOpenWait = GetPrivateProfileInt( iniBonDriver.GetBuffer(0), L"OpenWait", 0, this->moduleIniPath );
		this->initChgWait = GetPrivateProfileInt( iniBonDriver.GetBuffer(0), L"ChgWait", 0, this->moduleIniPath );
		_OutputDebugString(L"%d,%d,%d,%d,%d",initONID,initTSID,initSID,initOpenWait,initChgWait );
	}
}

BOOL CEpgDataCap_BonDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// このダイアログのアイコンを設定します。アプリケーションのメイン ウィンドウがダイアログでない場合、
	//  Framework は、この設定を自動的に行います。
	SetIcon(m_hIcon2, TRUE);			// 大きいアイコンの設定
	SetIcon(m_hIcon, FALSE);		// 小さいアイコンの設定

	// TODO: 初期化をここに追加します。
	this->main.ReloadSetting();

	for( int i=0; i<25; i++ ){
		CString buff;
		buff.Format(L"%d",i);
		int index = combRecH.AddString(buff);
		combRecH.SetItemData(index, i);
	}
	combRecH.SetCurSel(0);

	for( int i=0; i<59; i++ ){
		CString buff;
		buff.Format(L"%d",i);
		int index = combRecM.AddString(buff);
		combRecM.SetItemData(index, i);
	}
	combRecM.SetCurSel(0);

	//BonDriverの一覧取得
	ReloadBonDriver();

	//BonDriverのオープン
	DWORD err = NO_ERR;
	if( this->iniBonDriver.IsEmpty() == false ){
		err = SelectBonDriver(this->iniBonDriver.GetBuffer(0), TRUE);
		Sleep(this->initOpenWait);
	}else{
		map<int, wstring>::iterator itr;
		itr = this->bonList.begin();
		if( itr != this->bonList.end() ){
			err = SelectBonDriver(itr->second.c_str());
		}else{
			err = ERR_FALSE;
			this->log += L"BonDriverが見つかりませんでした\r\n";
		}
	}

	if( err == NO_ERR ){
		//チャンネル変更
		if( this->initONID != -1 && this->initTSID != -1 && this->initSID != -1 ){
			SelectService(this->initONID, this->initTSID, this->initSID);
			this->initONID = -1;
			this->initTSID = -1;
			this->initSID = -1;
			Sleep(this->initChgWait);
		}else{
			int sel = this->combService.GetCurSel();
			if( sel != CB_ERR ){
				DWORD index = (DWORD)this->combService.GetItemData(sel);
				SelectService(this->serviceList[index].originalNetworkID, this->serviceList[index].transportStreamID, this->serviceList[index].serviceID, this->serviceList[index].space, this->serviceList[index].ch );
			}
		}
	}

	//ウインドウの復元
	WINDOWPLACEMENT Pos;
	Pos.length = sizeof(WINDOWPLACEMENT);
	Pos.flags = NULL;
	if( this->iniMin == FALSE ){
		Pos.showCmd = SW_SHOW;
	}else{
		Pos.showCmd = SW_SHOWMINNOACTIVE;
	}
	Pos.rcNormalPosition.left = GetPrivateProfileInt(L"SET_WINDOW", L"left", 0, this->moduleIniPath);
	Pos.rcNormalPosition.right = GetPrivateProfileInt(L"SET_WINDOW", L"right", 0, this->moduleIniPath);
	Pos.rcNormalPosition.top = GetPrivateProfileInt(L"SET_WINDOW", L"top", 0, this->moduleIniPath);
	Pos.rcNormalPosition.bottom = GetPrivateProfileInt(L"SET_WINDOW", L"bottom", 0, this->moduleIniPath);
	if( Pos.rcNormalPosition.left != 0 &&
		Pos.rcNormalPosition.right != 0 &&
		Pos.rcNormalPosition.top != 0 &&
		Pos.rcNormalPosition.bottom != 0 ){
		SetWindowPlacement(&Pos);
	}
	SetTimer(TIMER_STATUS_UPDATE, 1000, NULL);
	SetTimer(TIMER_INIT_DLG, 1, NULL);
	this->main.SetHwnd(GetSafeHwnd());

	if( this->iniNetwork == TRUE ){
		if( this->iniUDP == TRUE || this->iniTCP == TRUE ){
			if( this->iniUDP == TRUE ){
				this->chkUDP.SetCheck(TRUE);
			}
			if( this->iniTCP == TRUE ){
				this->chkTCP.SetCheck(TRUE);
			}
		}else{
			this->chkUDP.SetCheck(GetPrivateProfileInt(L"SET", L"ChkUDP", 0, this->moduleIniPath));
			this->chkTCP.SetCheck(GetPrivateProfileInt(L"SET", L"ChkTCP", 0, this->moduleIniPath));
		}
	}

	ReloadNWSet();
	UpdateData(FALSE);

	this->main.StartServer();

	return TRUE;  // フォーカスをコントロールに設定した場合を除き、TRUE を返します。
}

// ダイアログに最小化ボタンを追加する場合、アイコンを描画するための
//  下のコードが必要です。ドキュメント/ビュー モデルを使う MFC アプリケーションの場合、
//  これは、Framework によって自動的に設定されます。

void CEpgDataCap_BonDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 描画のデバイス コンテキスト

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// クライアントの四角形領域内の中央
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// アイコンの描画
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// ユーザーが最小化したウィンドウをドラッグしているときに表示するカーソルを取得するために、
//  システムがこの関数を呼び出します。
HCURSOR CEpgDataCap_BonDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



BOOL CEpgDataCap_BonDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: ここに特定なコードを追加するか、もしくは基本クラスを呼び出してください。
	if( pMsg->message == WM_KEYDOWN ){
		if( pMsg->wParam  == VK_RETURN || pMsg->wParam  == VK_ESCAPE ){
			return FALSE;
		}
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}


int CEpgDataCap_BonDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDialogEx::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  ここに特定な作成コードを追加してください。

	return 0;
}


void CEpgDataCap_BonDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	// TODO: ここにメッセージ ハンドラー コードを追加するか、既定の処理を呼び出します。
	if( nID == SC_CLOSE ){
		if( this->main.IsRec() == TRUE ){
			if( AfxMessageBox( L"録画中ですが終了しますか？", MB_YESNO ) == IDNO ){
				return ;
			}
			this->main.StopReserveRec();
			this->main.StopRec();
		}
	}

	CDialogEx::OnSysCommand(nID, lParam);
}


void CEpgDataCap_BonDlg::OnClose()
{
	// TODO: ここにメッセージ ハンドラー コードを追加するか、既定の処理を呼び出します。
	KillTimer(TIMER_STATUS_UPDATE);
	this->main.StopServer();
	this->main.CloseBonDriver();

	CDialogEx::OnClose();
}


void CEpgDataCap_BonDlg::OnDestroy()
{
	UpdateData(TRUE);
	this->main.StopServer();
	this->main.CloseBonDriver();
	KillTimer(TIMER_STATUS_UPDATE);

	KillTimer(RETRY_ADD_TRAY);
	DeleteTaskBar(GetSafeHwnd(), TRAYICON_ID);

	WINDOWPLACEMENT Pos;
	GetWindowPlacement(&Pos);
	CString strAdd;

	strAdd.Format(L"%d", Pos.rcNormalPosition.top );
	WritePrivateProfileString(L"SET_WINDOW", L"top", strAdd.GetBuffer(0), this->moduleIniPath);
	strAdd.Format(L"%d", Pos.rcNormalPosition.left );
	WritePrivateProfileString(L"SET_WINDOW", L"left", strAdd.GetBuffer(0), this->moduleIniPath);
	strAdd.Format(L"%d", Pos.rcNormalPosition.bottom );
	WritePrivateProfileString(L"SET_WINDOW", L"bottom", strAdd.GetBuffer(0), this->moduleIniPath);
	strAdd.Format(L"%d", Pos.rcNormalPosition.right );
	WritePrivateProfileString(L"SET_WINDOW", L"right", strAdd.GetBuffer(0), this->moduleIniPath);

	int selONID = -1;
	int selTSID = -1;
	int selSID = -1;
	CString bon = L"";

	this->combTuner.GetWindowText(bon);
	int sel = this->combService.GetCurSel();
	if( sel != CB_ERR ){
		DWORD index = (DWORD)this->combService.GetItemData(sel);
		selONID = this->serviceList[index].originalNetworkID;
		selTSID = this->serviceList[index].transportStreamID;
		selSID = this->serviceList[index].serviceID;
	}

	strAdd.Format(L"%d", selONID );
	WritePrivateProfileString(L"SET", L"LastONID", strAdd.GetBuffer(0), this->moduleIniPath);
	strAdd.Format(L"%d", selTSID );
	WritePrivateProfileString(L"SET", L"LastTSID", strAdd.GetBuffer(0), this->moduleIniPath);
	strAdd.Format(L"%d", selSID );
	WritePrivateProfileString(L"SET", L"LastSID", strAdd.GetBuffer(0), this->moduleIniPath);

	WritePrivateProfileString(L"SET", L"LastBon", bon.GetBuffer(0), this->moduleIniPath);

	strAdd.Format(L"%d", this->chkUDP.GetCheck() );
	WritePrivateProfileString(L"SET", L"ChkUDP", strAdd.GetBuffer(0), this->moduleIniPath);
	strAdd.Format(L"%d", this->chkTCP.GetCheck() );
	WritePrivateProfileString(L"SET", L"ChkTCP", strAdd.GetBuffer(0), this->moduleIniPath);


	CDialogEx::OnDestroy();

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
				    ShowWindow(SW_HIDE);
				}
			}
			break;
		case TIMER_STATUS_UPDATE:
			{
				KillTimer( TIMER_STATUS_UPDATE );
				SetThreadExecutionState(ES_SYSTEM_REQUIRED);

				int iLine = this->editStatus.GetFirstVisibleLine();
				float signal = 0;
				DWORD space = 0;
				DWORD ch = 0;
				ULONGLONG drop = 0;
				ULONGLONG scramble = 0;
				vector<NW_SEND_INFO> udpSendList;
				vector<NW_SEND_INFO> tcpSendList;

				BOOL ret = this->main.GetViewStatusInfo(&signal, &space, &ch, &drop, &scramble, &udpSendList, &tcpSendList);

				if(ret==TRUE){
					this->statusLog.Format(L"Signal: %.02f Drop: %I64d Scramble: %I64d  space: %d ch: %d",signal, drop, scramble, space, ch);
				}else{
					this->statusLog.Format(L"Signal: %.02f Drop: %I64d Scramble: %I64d",signal, drop, scramble);
				}
				this->statusLog += L"\r\n";

				CString udp = L"";
				if( udpSendList.size() > 0 ){
					udp = "UDP送信：";
					for( size_t i=0; i<udpSendList.size(); i++ ){
						CString buff;
						if( udpSendList[i].broadcastFlag == FALSE ){
							buff.Format(L"%s:%d ",udpSendList[i].ipString.c_str(), udpSendList[i].port);
						}else{
							buff.Format(L"%s:%d(Broadcast) ",udpSendList[i].ipString.c_str(), udpSendList[i].port);
						}
						udp += buff;
					}
					udp += L"\r\n";
				}
				this->statusLog += udp;

				CString tcp = L"";
				if( tcpSendList.size() > 0 ){
					tcp = "TCP送信：";
					for( size_t i=0; i<tcpSendList.size(); i++ ){
						CString buff;
						buff.Format(L"%s:%d ",tcpSendList[i].ipString.c_str(), tcpSendList[i].port);
						tcp += buff;
					}
					tcp += L"\r\n";
				}
				this->statusLog += tcp;

				SetDlgItemText(IDC_EDIT_STATUS, this->statusLog);
				editStatus.LineScroll(iLine);

				CString info = L"";
				this->main.GetEpgInfo(this->btnPgNext.GetCheck(), &info);
				if( this->pgInfo.Compare(info) != 0 ){
					this->pgInfo = info;
					SetDlgItemText(IDC_EDIT_PG_INFO, this->pgInfo);
				}
				SetTimer(TIMER_STATUS_UPDATE, 1000, NULL);
			}
			break;
		case TIMER_CHSCAN_STATSU:
			{
				KillTimer( TIMER_CHSCAN_STATSU );
				DWORD space = 0;
				DWORD ch = 0;
				wstring chName = L"";
				DWORD chkNum = 0;
				DWORD totalNum = 0;
				DWORD status = this->main.GetChScanStatus(&space, &ch, &chName, &chkNum, &totalNum);
				if( status == ST_WORKING ){
					this->log.Format(L"%s (%d/%d 残り約 %d 秒)\r\n", chName.c_str(), chkNum, totalNum, (totalNum - chkNum)*10);
					SetDlgItemText(IDC_EDIT_LOG, this->log);
					SetTimer(TIMER_CHSCAN_STATSU, 1000, NULL);
				}else if( status == ST_CANCEL ){
					KillTimer(TIMER_CHSCAN_STATSU);
					this->log = L"キャンセルされました\r\n";
					SetDlgItemText(IDC_EDIT_LOG, this->log);
				}else if( status == ST_COMPLETE ){
					KillTimer(TIMER_CHSCAN_STATSU);
					this->log = L"終了しました\r\n";
					SetDlgItemText(IDC_EDIT_LOG, this->log);
					ReloadServiceList();
					int sel = this->combService.GetCurSel();
					if( sel != CB_ERR ){
						DWORD index = (DWORD)this->combService.GetItemData(sel);
						SelectService(this->serviceList[index].originalNetworkID, this->serviceList[index].transportStreamID, this->serviceList[index].serviceID, this->serviceList[index].space, this->serviceList[index].ch );
					}
					BtnUpdate(GUI_NORMAL);
					ChgIconStatus();

					//同じサービスが別の物理チャンネルにあるかチェック
					wstring msg = L"";
					for( size_t i=0; i<this->serviceList.size(); i++ ){
						for( size_t j=i+1; j<this->serviceList.size(); j++ ){
							if( this->serviceList[i].originalNetworkID == this->serviceList[j].originalNetworkID &&
								this->serviceList[i].transportStreamID == this->serviceList[j].transportStreamID &&
								this->serviceList[i].serviceID == this->serviceList[j].serviceID ){
									wstring log = L"";
									Format(log, L"%s space:%d ch:%d <=> %s space:%d ch:%d\r\n",
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
						MessageBox(log.c_str());
					}
				}
			}
			break;
		case TIMER_EPGCAP_STATSU:
			{
				KillTimer( TIMER_EPGCAP_STATSU );
				EPGCAP_SERVICE_INFO info;
				DWORD status = this->main.GetEpgCapStatus(&info);
				if( status == ST_WORKING ){
					int sel = this->combService.GetCurSel();
					if( sel != CB_ERR ){
						DWORD index = (DWORD)this->combService.GetItemData(sel);
						if( info.ONID != this->serviceList[index].originalNetworkID ||
							info.TSID != this->serviceList[index].transportStreamID ||
							info.SID != this->serviceList[index].serviceID ){
						}
						this->initONID = info.ONID;
						this->initTSID = info.TSID;
						this->initSID = info.SID;
						ReloadServiceList();
						this->main.SetSID(info.SID);
					}

					this->log = L"EPG取得中\r\n";
					SetDlgItemText(IDC_EDIT_LOG, this->log);
					SetTimer(TIMER_EPGCAP_STATSU, 1000, NULL);
				}else if( status == ST_CANCEL ){
					KillTimer(TIMER_EPGCAP_STATSU);
					this->log = L"キャンセルされました\r\n";
					SetDlgItemText(IDC_EDIT_LOG, this->log);
				}else if( status == ST_COMPLETE ){
					KillTimer(TIMER_EPGCAP_STATSU);
					this->log = L"終了しました\r\n";
					SetDlgItemText(IDC_EDIT_LOG, this->log);
					BtnUpdate(GUI_NORMAL);
					ChgIconStatus();
				}
			}
			break;
		case TIMER_REC_END:
			{
				this->main.StopRec();
				KillTimer(TIMER_REC_END);
				this->log = L"録画停止しました\r\n";
				SetDlgItemText(IDC_EDIT_LOG, this->log);
				BtnUpdate(GUI_NORMAL);
				chkRecSet.SetCheck(FALSE);
				ChgIconStatus();
			}
			break;
		case RETRY_ADD_TRAY:
			{
				KillTimer(RETRY_ADD_TRAY);
				CString buff=L"";
				wstring bonFile = L"";
				this->main.GetOpenBonDriver(&bonFile);
				CString strBuff2=L"";
				this->combService.GetWindowText(strBuff2);
				buff.Format(L"%s ： %s", bonFile.c_str(), strBuff2.GetBuffer(0));

				HICON setIcon = this->iconBlue;
				if( this->main.IsRec() == TRUE ){
					setIcon = this->iconRed;
				}else if( this->main.GetEpgCapStatus(NULL) == ST_WORKING ){
					setIcon = this->iconGreen;
				}else if( this->main.GetOpenBonDriver(NULL) == FALSE ){
					setIcon = this->iconGray;
				}
		
				if( AddTaskBar( GetSafeHwnd(),
						WM_TRAY_PUSHICON,
						TRAYICON_ID,
						setIcon,
						buff ) == FALSE ){
							SetTimer(RETRY_ADD_TRAY, 5000, NULL);
				}
			}
			break;
		default:
			break;
	}
	CDialogEx::OnTimer(nIDEvent);
}


void CEpgDataCap_BonDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	// TODO: ここにメッセージ ハンドラー コードを追加します。
	if( nType == SIZE_MINIMIZED && this->minTask == TRUE){
		CString buff=L"";
		wstring bonFile = L"";
		this->main.GetOpenBonDriver(&bonFile);
		CString strBuff2=L"";
		this->combService.GetWindowText(strBuff2);
		buff.Format(L"%s ： %s", bonFile.c_str(), strBuff2.GetBuffer(0));

		HICON setIcon = this->iconBlue;
		if( this->main.IsRec() == TRUE ){
			setIcon = this->iconRed;
		}else if( this->main.GetEpgCapStatus(NULL) == ST_WORKING ){
			setIcon = this->iconGreen;
		}else if( this->main.GetOpenBonDriver(NULL) == FALSE ){
			setIcon = this->iconGray;
		}
		
		if( AddTaskBar( GetSafeHwnd(),
				WM_TRAY_PUSHICON,
				TRAYICON_ID,
				setIcon,
				buff ) == FALSE ){
					SetTimer(RETRY_ADD_TRAY, 5000, NULL);
		}
		if(!this->iniMin) ShowWindow(SW_HIDE);
	}
}


LRESULT CEpgDataCap_BonDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	// TODO: ここに特定なコードを追加するか、もしくは基本クラスを呼び出してください。
	switch(message){
	case WM_RESERVE_REC_START:
		{
			BtnUpdate(GUI_OTHER_CTRL);
			if( this->log.Find(L"予約録画中") < 0 ){
				this->log = L"予約録画中\r\n";
				SetDlgItemText(IDC_EDIT_LOG, this->log);
			}
			ChgIconStatus();
		}
		break;
	case WM_RESERVE_REC_STOP:
		{
			BtnUpdate(GUI_NORMAL);
			this->log = L"予約録画終了しました\r\n";
			SetDlgItemText(IDC_EDIT_LOG, this->log);
			ChgIconStatus();
		}
		break;
	case WM_RESERVE_EPGCAP_START:
		{
			SetTimer(TIMER_EPGCAP_STATSU, 1000, NULL);
			BtnUpdate(GUI_CANCEL_ONLY);
			ChgIconStatus();
		}
		break;
	case WM_RESERVE_EPGCAP_STOP:
		{
			ChgIconStatus();
		}
		break;
	case WM_CHG_TUNER:
		{
			wstring bonDriver = L"";
			this->main.GetOpenBonDriver(&bonDriver);
			this->iniBonDriver = bonDriver.c_str();
			ReloadBonDriver();
			ChgIconStatus();
		}
		break;
	case WM_CHG_CH:
		{
			WORD ONID;
			WORD TSID;
			WORD SID;
			this->main.GetCh(&ONID, &TSID, &SID);
			this->initONID = ONID;
			this->initTSID = TSID;
			this->initSID = SID;
			ReloadServiceList();
			ChgIconStatus();
		}
		break;
	case WM_RESERVE_REC_STANDBY:
		{
			if( wParam == 1 ){
				BtnUpdate(GUI_REC_STANDBY);
				this->log = L"予約録画待機中\r\n";
				SetDlgItemText(IDC_EDIT_LOG, this->log);
			}else if( wParam == 2 ){
				BtnUpdate(GUI_NORMAL);
				this->log = L"視聴モード\r\n";
				SetDlgItemText(IDC_EDIT_LOG, this->log);
			}else{
				BtnUpdate(GUI_NORMAL);
				this->log = L"";
				SetDlgItemText(IDC_EDIT_LOG, this->log);
			}
		}
		break;
	case WM_TRAY_PUSHICON:
		{
			//タスクトレイ関係
			switch(LOWORD(lParam)){
				case WM_LBUTTONDOWN:
					{
						this->iniMin = FALSE;
						ShowWindow(SW_RESTORE);
						SetForegroundWindow();
						KillTimer(RETRY_ADD_TRAY);
						DeleteTaskBar(GetSafeHwnd(), TRAYICON_ID);
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

	return CDialogEx::WindowProc(message, wParam, lParam);
}


BOOL CEpgDataCap_BonDlg::AddTaskBar(HWND wnd, UINT msg, UINT id, HICON icon, CString tips)
{ 
	BOOL ret=TRUE;
	NOTIFYICONDATA data;
	ZeroMemory(&data, sizeof(NOTIFYICONDATA));

	data.cbSize = sizeof(NOTIFYICONDATA); 
	data.hWnd = wnd; 
	data.uID = id; 
	data.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP; 
	data.uCallbackMessage = msg; 
	data.hIcon = icon; 

	if( tips.IsEmpty() == false ){
		wcsncpy_s(data.szTip, sizeof(data.szTip), tips.GetBuffer(0), sizeof(data.szTip) );
	}
 
	ret = Shell_NotifyIcon(NIM_ADD, &data);
  
	return ret; 
}

BOOL CEpgDataCap_BonDlg::ChgTipsTaskBar(HWND wnd, UINT id, HICON icon, CString tips)
{ 
	BOOL ret=TRUE;
	NOTIFYICONDATA data;
	ZeroMemory(&data, sizeof(NOTIFYICONDATA));

	data.cbSize = sizeof(NOTIFYICONDATA); 
	data.hWnd = wnd; 
	data.uID = id; 
	data.hIcon = icon; 
	data.uFlags = NIF_ICON | NIF_TIP; 

	if( tips.IsEmpty() == false ){
		wcsncpy_s(data.szTip, sizeof(data.szTip), tips.GetBuffer(0), sizeof(data.szTip) );
	}
 
	ret = Shell_NotifyIcon(NIM_MODIFY, &data); 
 
	return ret; 
}

BOOL CEpgDataCap_BonDlg::DeleteTaskBar(HWND wnd, UINT id)
{ 
	BOOL ret=TRUE; 
	NOTIFYICONDATA data; 
	ZeroMemory(&data, sizeof(NOTIFYICONDATA));
 
	data.cbSize = sizeof(NOTIFYICONDATA); 
	data.hWnd = wnd; 
	data.uID = id; 
         
	ret = Shell_NotifyIcon(NIM_DELETE, &data); 

	return ret; 
}

void CEpgDataCap_BonDlg::ChgIconStatus(){
	if( this->minTask == TRUE){
		CString buff=L"";
		wstring bonFile = L"";
		this->main.GetOpenBonDriver(&bonFile);
		CString strBuff2=L"";
		this->combService.GetWindowText(strBuff2);
		buff.Format(L"%s ： %s", bonFile.c_str(), strBuff2.GetBuffer(0));

		HICON setIcon = this->iconBlue;
		if( this->main.IsRec() == TRUE ){
			setIcon = this->iconRed;
		}else if( this->main.GetEpgCapStatus(NULL) == ST_WORKING ){
			setIcon = this->iconGreen;
		}else if( this->main.GetOpenBonDriver(NULL) == FALSE ){
			setIcon = this->iconGray;
		}

		ChgTipsTaskBar( GetSafeHwnd(),
				TRAYICON_ID,
				setIcon,
				buff );
	}
}

LRESULT CEpgDataCap_BonDlg::OnTaskbarCreated(WPARAM, LPARAM)
{
	if( IsWindowVisible() == FALSE && this->minTask == TRUE){
		CString buff=L"";
		wstring bonFile = L"";
		this->main.GetOpenBonDriver(&bonFile);
		CString strBuff2=L"";
		this->combService.GetWindowText(strBuff2);
		buff.Format(L"%s ： %s", bonFile.c_str(), strBuff2.GetBuffer(0));

		HICON setIcon = this->iconBlue;
		if( this->main.IsRec() == TRUE ){
			setIcon = this->iconRed;
		}else if( this->main.GetEpgCapStatus(NULL) == ST_WORKING ){
			setIcon = this->iconGreen;
		}else if( this->main.GetOpenBonDriver(NULL) == FALSE ){
			setIcon = this->iconGray;
		}
		
		if( AddTaskBar( GetSafeHwnd(),
				WM_TRAY_PUSHICON,
				TRAYICON_ID,
				setIcon,
				buff ) == FALSE ){
					SetTimer(RETRY_ADD_TRAY, 5000, NULL);
		}
	}

	return 0;
}

void CEpgDataCap_BonDlg::BtnUpdate(DWORD guiMode)
{
	switch(guiMode){
		case GUI_NORMAL:
			this->combTuner.EnableWindow(TRUE);
			this->combService.EnableWindow(TRUE);
			this->btnChScan.EnableWindow(TRUE);
			this->btnEpg.EnableWindow(TRUE);
			this->btnSet.EnableWindow(TRUE);
			this->btnRec.EnableWindow(TRUE);
			this->combRecH.EnableWindow(FALSE);
			this->combRecM.EnableWindow(FALSE);
			this->chkRecSet.EnableWindow(FALSE);
			this->chkRecSet.SetCheck(FALSE);
			this->btnCancel.EnableWindow(FALSE);
			this->btnView.EnableWindow(TRUE);
			this->chkUDP.EnableWindow(TRUE);
			this->chkTCP.EnableWindow(TRUE);
			this->btnTimeShift.EnableWindow(FALSE);
			break;
		case GUI_CANCEL_ONLY:
			this->combTuner.EnableWindow(FALSE);
			this->combService.EnableWindow(FALSE);
			this->btnChScan.EnableWindow(FALSE);
			this->btnEpg.EnableWindow(FALSE);
			this->btnSet.EnableWindow(FALSE);
			this->btnRec.EnableWindow(FALSE);
			this->combRecH.EnableWindow(FALSE);
			this->combRecM.EnableWindow(FALSE);
			this->chkRecSet.EnableWindow(FALSE);
			this->btnCancel.EnableWindow(TRUE);
			this->btnView.EnableWindow(TRUE);
			this->chkUDP.EnableWindow(TRUE);
			this->chkTCP.EnableWindow(TRUE);
			this->btnTimeShift.EnableWindow(FALSE);
			break;
		case GUI_OPEN_FAIL:
			this->combTuner.EnableWindow(TRUE);
			this->combService.EnableWindow(FALSE);
			this->btnChScan.EnableWindow(FALSE);
			this->btnEpg.EnableWindow(FALSE);
			this->btnSet.EnableWindow(TRUE);
			this->btnRec.EnableWindow(FALSE);
			this->combRecH.EnableWindow(FALSE);
			this->combRecM.EnableWindow(FALSE);
			this->chkRecSet.EnableWindow(FALSE);
			this->btnCancel.EnableWindow(FALSE);
			this->btnView.EnableWindow(TRUE);
			this->chkUDP.EnableWindow(FALSE);
			this->chkTCP.EnableWindow(FALSE);
			this->btnTimeShift.EnableWindow(FALSE);
			break;
		case GUI_REC:
			this->combTuner.EnableWindow(FALSE);
			this->combService.EnableWindow(FALSE);
			this->btnChScan.EnableWindow(FALSE);
			this->btnEpg.EnableWindow(FALSE);
			this->btnSet.EnableWindow(FALSE);
			this->btnRec.EnableWindow(FALSE);
			this->combRecH.EnableWindow(TRUE);
			this->combRecM.EnableWindow(TRUE);
			this->chkRecSet.EnableWindow(TRUE);
			this->chkRecSet.SetCheck(FALSE);
			this->btnCancel.EnableWindow(TRUE);
			this->btnView.EnableWindow(TRUE);
			this->chkUDP.EnableWindow(TRUE);
			this->chkTCP.EnableWindow(TRUE);
			this->btnTimeShift.EnableWindow(TRUE);
			break;
		case GUI_REC_SET_TIME:
			this->combTuner.EnableWindow(FALSE);
			this->combService.EnableWindow(FALSE);
			this->btnChScan.EnableWindow(FALSE);
			this->btnEpg.EnableWindow(FALSE);
			this->btnSet.EnableWindow(FALSE);
			this->btnRec.EnableWindow(FALSE);
			this->combRecH.EnableWindow(FALSE);
			this->combRecM.EnableWindow(FALSE);
			this->chkRecSet.EnableWindow(TRUE);
			this->btnCancel.EnableWindow(TRUE);
			this->btnView.EnableWindow(TRUE);
			this->chkUDP.EnableWindow(TRUE);
			this->chkTCP.EnableWindow(TRUE);
			this->btnTimeShift.EnableWindow(TRUE);
			break;
		case GUI_OTHER_CTRL:
			this->combTuner.EnableWindow(FALSE);
			this->combService.EnableWindow(FALSE);
			this->btnChScan.EnableWindow(FALSE);
			this->btnEpg.EnableWindow(FALSE);
			this->btnSet.EnableWindow(FALSE);
			this->btnRec.EnableWindow(FALSE);
			this->combRecH.EnableWindow(FALSE);
			this->combRecM.EnableWindow(FALSE);
			this->chkRecSet.EnableWindow(FALSE);
			this->btnCancel.EnableWindow(TRUE);
			this->btnView.EnableWindow(TRUE);
			this->chkUDP.EnableWindow(TRUE);
			this->chkTCP.EnableWindow(TRUE);
			this->btnTimeShift.EnableWindow(TRUE);
			break;
		case GUI_REC_STANDBY:
			this->combTuner.EnableWindow(FALSE);
			this->combService.EnableWindow(FALSE);
			this->btnChScan.EnableWindow(FALSE);
			this->btnEpg.EnableWindow(FALSE);
			this->btnSet.EnableWindow(FALSE);
			this->btnRec.EnableWindow(FALSE);
			this->combRecH.EnableWindow(FALSE);
			this->combRecM.EnableWindow(FALSE);
			this->chkRecSet.EnableWindow(FALSE);
			this->btnCancel.EnableWindow(FALSE);
			this->btnView.EnableWindow(TRUE);
			this->chkUDP.EnableWindow(TRUE);
			this->chkTCP.EnableWindow(TRUE);
			this->btnTimeShift.EnableWindow(FALSE);
			break;
		default:
			break;
	}
}



void CEpgDataCap_BonDlg::OnCbnSelchangeComboTuner()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	KillTimer(TIMER_STATUS_UPDATE);
	CString buff=L"";
	this->combTuner.GetWindowText(buff);

	if( buff.IsEmpty() == false ){
		SelectBonDriver(buff.GetBuffer(0));

		int sel = this->combService.GetCurSel();
		if( sel != CB_ERR ){
			DWORD index = (DWORD)this->combService.GetItemData(sel);
			SelectService(this->serviceList[index].originalNetworkID, this->serviceList[index].transportStreamID, this->serviceList[index].serviceID, this->serviceList[index].space, this->serviceList[index].ch );
		}
	}
	ChgIconStatus();
	SetTimer(TIMER_STATUS_UPDATE, 1000, NULL);
}


void CEpgDataCap_BonDlg::OnCbnSelchangeComboService()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	int sel = this->combService.GetCurSel();
	if( sel != CB_ERR ){
		DWORD index = (DWORD)this->combService.GetItemData(sel);
		SelectService(this->serviceList[index].originalNetworkID, this->serviceList[index].transportStreamID, this->serviceList[index].serviceID, this->serviceList[index].space, this->serviceList[index].ch );
	}
	ChgIconStatus();
}


void CEpgDataCap_BonDlg::OnBnClickedButtonSet()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	CSettingDlg setDlg;
	if( setDlg.DoModal() == IDOK ){

		this->main.ReloadSetting();

		ReloadNWSet();

		WORD ONID;
		WORD TSID;
		WORD SID;
		this->main.GetCh(&ONID, &TSID, &SID);
		this->initONID = ONID;
		this->initTSID = TSID;
		this->initSID = SID;
		ReloadServiceList();
		
		this->minTask = GetPrivateProfileInt( L"Set", L"MinTask", 0, this->moduleIniPath );
	}
}

void CEpgDataCap_BonDlg::ReloadNWSet()
{
	this->main.SendUDP(FALSE);
	this->main.SendTCP(FALSE);
	if( this->main.GetCountUDP() > 0 ){
		this->chkUDP.EnableWindow(TRUE);
	}else{
		this->chkUDP.EnableWindow(FALSE);
		this->chkUDP.SetCheck(FALSE);
	}
	if( this->main.GetCountTCP() > 0 ){
		this->chkTCP.EnableWindow(TRUE);
	}else{
		this->chkTCP.EnableWindow(FALSE);
		this->chkTCP.SetCheck(FALSE);
	}
	this->main.SendUDP(this->chkUDP.GetCheck());
	this->main.SendTCP(this->chkTCP.GetCheck());
}

void CEpgDataCap_BonDlg::ReloadBonDriver()
{
	this->bonList.clear();
	this->combTuner.ResetContent();

	this->main.EnumBonDriver(&bonList);

	int selectIndex = 0;
	map<int, wstring>::iterator itr;
	for( itr = this->bonList.begin(); itr != this->bonList.end(); itr++ ){
		int index = this->combTuner.AddString(itr->second.c_str());
		if( this->iniBonDriver.IsEmpty() == false ){
			if( this->iniBonDriver.Compare(itr->second.c_str()) == 0 ){
				selectIndex = index;
			}
		}
	}
	if( this->bonList.size() > 0){
		this->combTuner.SetCurSel(selectIndex);
	}
}

void CEpgDataCap_BonDlg::ReloadServiceList(BOOL ini)
{
	this->serviceList.clear();
	this->combService.ResetContent();

	DWORD ret = this->main.GetServiceList(&this->serviceList);
	if( ret != NO_ERR || this->serviceList.size() == 0 ){
		this->log += L"チャンネル情報の読み込みに失敗しました\r\n";
		SetDlgItemText(IDC_EDIT_LOG, this->log);
	}else{
		int selectSel = 0;
		for( size_t i=0; i<this->serviceList.size(); i++ ){
			if( this->serviceList[i].useViewFlag == TRUE ){
				int index = this->combService.AddString(this->serviceList[i].serviceName.c_str());
				this->combService.SetItemData(index, (DWORD)i);
				if( this->serviceList[i].originalNetworkID == this->initONID &&
					this->serviceList[i].transportStreamID == this->initTSID &&
					this->serviceList[i].serviceID == this->initSID ){
						if( ini == FALSE ){
							this->initONID = -1;
							this->initTSID = -1;
							this->initSID = -1;
						}
						selectSel = index;
				}
			}
		}
		if( this->combService.GetCount() > 0 ){
			this->combService.SetCurSel(selectSel);
		}

	}

}

DWORD CEpgDataCap_BonDlg::SelectBonDriver(LPCWSTR fileName, BOOL ini)
{
	this->main.CloseBonDriver();
	DWORD err = this->main.OpenBonDriver(fileName);
	if( err != NO_ERR ){
		this->log.Format(L"BonDriverのオープンができませんでした\r\n%s\r\n", fileName);
		SetDlgItemText(IDC_EDIT_LOG, this->log);
		BtnUpdate(GUI_OPEN_FAIL);
	}else{
		this->log = L"";
		SetDlgItemText(IDC_EDIT_LOG, this->log);
		BtnUpdate(GUI_NORMAL);
	}
	ReloadServiceList(ini);
	return err;
}

DWORD CEpgDataCap_BonDlg::SelectService(WORD ONID, WORD TSID, WORD SID)
{
	DWORD err = this->main.SetCh(ONID, TSID, SID);
	return err;
}

DWORD CEpgDataCap_BonDlg::SelectService(WORD ONID, WORD TSID, WORD SID,	DWORD space, DWORD ch)
{
	DWORD err = this->main.SetCh(ONID, TSID, SID, space, ch);
	return err;
}

void CEpgDataCap_BonDlg::OnBnClickedButtonChscan()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	if( this->main.StartChScan() != NO_ERR ){
		this->log.Format(L"チャンネルスキャンを開始できませんでした\r\n");
		SetDlgItemText(IDC_EDIT_LOG, this->log);
		return;
	}
	SetTimer(TIMER_CHSCAN_STATSU, 1000, NULL);
	BtnUpdate(GUI_CANCEL_ONLY);
}


void CEpgDataCap_BonDlg::OnBnClickedButtonEpg()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	if( this->main.StartEpgCap() != NO_ERR ){
		this->log.Format(L"EPG取得を開始できませんでした\r\n");
		SetDlgItemText(IDC_EDIT_LOG, this->log);
		return;
	}
	SetTimer(TIMER_EPGCAP_STATSU, 1000, NULL);
	BtnUpdate(GUI_CANCEL_ONLY);
	ChgIconStatus();
}


void CEpgDataCap_BonDlg::OnBnClickedButtonRec()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	if( this->main.StartRec() != NO_ERR ){
		this->log.Format(L"録画を開始できませんでした\r\n");
		SetDlgItemText(IDC_EDIT_LOG, this->log);
		return;
	}
	SYSTEMTIME now;
	GetLocalTime(&now);
	SYSTEMTIME end;
	GetSumTime(now, 30*60, &end);

	this->combRecH.SetCurSel(end.wHour);
	this->combRecM.SetCurSel(end.wMinute);

	this->log = L"録画中\r\n";
	SetDlgItemText(IDC_EDIT_LOG, this->log);

	BtnUpdate(GUI_REC);
	ChgIconStatus();
}


void CEpgDataCap_BonDlg::OnBnClickedButtonCancel()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	if( this->main.IsRec() == TRUE ){
		if( AfxMessageBox( L"録画を停止しますか？", MB_YESNO ) == IDNO ){
			return ;
		}
	}
	this->log = L"キャンセルされました\r\n";
	SetDlgItemText(IDC_EDIT_LOG, this->log);

	this->main.StopChScan();
	KillTimer(TIMER_CHSCAN_STATSU);
	this->main.StopEpgCap();
	KillTimer(TIMER_EPGCAP_STATSU);
	this->main.StopRec();
	KillTimer(TIMER_REC_END);
	this->main.StopReserveRec();


	BtnUpdate(GUI_NORMAL);
	ChgIconStatus();
}


void CEpgDataCap_BonDlg::OnBnClickedButtonView()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	this->main.ViewAppOpen();
}


void CEpgDataCap_BonDlg::OnBnClickedCheckUdp()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	this->main.SendUDP(this->chkUDP.GetCheck());
}


void CEpgDataCap_BonDlg::OnBnClickedCheckTcp()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	this->main.SendTCP(this->chkTCP.GetCheck());
}


void CEpgDataCap_BonDlg::OnBnClickedCheckRecSet()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	if( chkRecSet.GetCheck() == TRUE ){
		BtnUpdate(GUI_REC_SET_TIME);
		SYSTEMTIME now;
		GetLocalTime(&now);

		int selH = this->combRecH.GetCurSel();
		int selM = this->combRecM.GetCurSel();

		DWORD nowTime = now.wHour*60*60 + now.wMinute*60 + now.wSecond;
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
	CString info = L"";
	this->main.GetEpgInfo(this->btnPgNext.GetCheck(), &info);
	if( this->pgInfo.Compare(info) != 0 ){
		this->pgInfo = info;
		SetDlgItemText(IDC_EDIT_PG_INFO, this->pgInfo);
	}
}



void CEpgDataCap_BonDlg::OnBnClickedButtonTimeshift()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
	this->main.StartTimeShift();
}


BOOL CEpgDataCap_BonDlg::OnQueryEndSession()
{
	if (!CDialogEx::OnQueryEndSession())
		return FALSE;

	// TODO:  ここに特定なクエリの終了セッション コードを追加してください。
	if( this->main.IsRec() == TRUE ){
		ShowWindow(SW_SHOW);
		return FALSE;
	}
	return TRUE;
}


void CEpgDataCap_BonDlg::OnEndSession(BOOL bEnding)
{
	CDialogEx::OnEndSession(bEnding);

	// TODO: ここにメッセージ ハンドラー コードを追加します。
	if( bEnding == TRUE ){
		if( this->main.IsRec() == TRUE ){
			this->main.StopReserveRec();
			this->main.StopRec();
		}
	}
}
