// EpgTimerPlugIn.cpp : DLL アプリケーション用にエクスポートされる関数を定義します。
//

#include "stdafx.h"
#include "EpgTimerPlugIn.h"

#include "../../Common/StringUtil.h"
#include "../../Common/PathUtil.h"

#define WM_INVOKE_CTRL_CMD	(CStreamCtrlDlg::WM_CUSTOM + 0)
#define WM_TT_SET_CTRL		(CStreamCtrlDlg::WM_CUSTOM + 1)

extern HINSTANCE g_hinstDLL;

// プラグインクラスのインスタンスを生成する
TVTest::CTVTestPlugin *CreatePluginClass()
{
	return new CEpgTimerPlugIn;
}


CEpgTimerPlugIn::CEpgTimerPlugIn()
{
	this->nwMode = FALSE;
	this->fullScreen = FALSE;
	this->showNormal = TRUE;
	this->grantServerAccess = FALSE;
}

// プラグインの情報を返す
bool CEpgTimerPlugIn::GetPluginInfo(TVTest::PluginInfo *pInfo)
{
	pInfo->Type           = TVTest::PLUGIN_TYPE_NORMAL;
	pInfo->Flags          = 0;
	pInfo->pszPluginName  = L"EpgTimer PlugIn";
	pInfo->pszCopyright   = L"りょうちん Copyright (C) 2010";
	pInfo->pszDescription = L"EpgTimerSrvからの制御用";
	return true;
}


// 初期化処理
bool CEpgTimerPlugIn::Initialize()
{
	// イベントコールバック関数を登録
	m_pApp->SetEventCallback(EventCallback, this);

	// ダイアログを確実に生成する
	if( !this->ctrlDlg.CreateStreamCtrlDialog(g_hinstDLL, this->m_pApp->GetAppWindow()) ){
		return false;
	}
	this->ctrlDlg.SetMessageCallback(StreamCtrlDlgCallback, this);

	return true;
}

void CEpgTimerPlugIn::EnablePlugin(BOOL enable)
{
	this->showNormal = TRUE;

	if( enable == TRUE ){
		AddDebugLog(L"EnablePlugin");
		if(this->m_pApp->SetWindowMessageCallback(WindowMsgeCallback, this)==false){
			AddDebugLog(L"●TVTest Version Err::SetWindowMessageCallback");
		}
		if( this->grantServerAccess == FALSE ){
			if( CPipeServer::GrantServerAccessToKernelObject(GetCurrentProcess(), SYNCHRONIZE | PROCESS_TERMINATE | PROCESS_SET_INFORMATION) ){
				AddDebugLog(L"Granted SYNCHRONIZE|PROCESS_TERMINATE|PROCESS_SET_INFORMATION");
			}
			this->grantServerAccess = TRUE;
		}

		wstring pipeName;
		Format(pipeName, L"%ls%d", CMD2_TVTEST_CTRL_PIPE, GetCurrentProcessId());
		AddDebugLogFormat(L"%ls", pipeName.c_str());
		this->pipeServer.StartServer(pipeName, [this](CMD_STREAM* cmdParam, CMD_STREAM* resParam) {
			// SendMessageTimeout()はメッセージ処理中でも容赦なくタイムアウトするのでコマンドデータを排他処理する
			{
				CBlockLock lock(&this->cmdLock);
				std::swap(this->cmdCapture.param, cmdParam->param);
				std::swap(this->cmdCapture.dataSize, cmdParam->dataSize);
				this->cmdCapture.data.swap(cmdParam->data);
			}
			// CtrlCmdCallbackInvoked()をメインスレッドで呼ぶ(デッドロック防止のためタイムアウトつき)
			DWORD_PTR dwResult;
			if( SendMessageTimeout(this->ctrlDlg.GetDlgHWND(), WM_INVOKE_CTRL_CMD, 0, 0, SMTO_NORMAL, 10000, &dwResult) ){
				CBlockLock lock(&this->cmdLock);
				std::swap(resParam->param, this->resCapture.param);
				std::swap(resParam->dataSize, this->resCapture.dataSize);
				resParam->data.swap(this->resCapture.data);
			}else{
				resParam->param = CMD_ERR;
			}
		});

		if( this->m_pApp->GetFullscreen() == true ){
			this->fullScreen = TRUE;
			this->ctrlDlg.StartFullScreenMouseChk();
		}else{
			this->fullScreen = FALSE;
		}
		this->ResetStreamingCtrlView();
	}else{
		this->m_pApp->SetWindowMessageCallback(NULL, NULL);
		this->pipeServer.StopServer();

		this->ctrlDlg.ShowCtrlDlg(SW_HIDE);
		this->ctrlDlg.StopTimer();
	}
	return ;
}

// 終了処理
bool CEpgTimerPlugIn::Finalize()
{
	if( this->m_pApp->IsPluginEnabled() ){
		this->EnablePlugin(FALSE);
	}
	this->ctrlDlg.SetMessageCallback(NULL);
	this->ctrlDlg.CloseStreamCtrlDialog();

	if( this->nwMode ){
		this->nwModeInfo.enableMode = FALSE;
		this->ctrlDlg.SetCtrl(this->nwModeInfo);
		this->nwMode = FALSE;
	}
	return true;
}

// イベントコールバック関数
// 何かイベントが起きると呼ばれる
LRESULT CALLBACK CEpgTimerPlugIn::EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData)
{
	CEpgTimerPlugIn *pThis=static_cast<CEpgTimerPlugIn*>(pClientData);
	switch (Event) {
	case TVTest::EVENT_PLUGINENABLE:
		if (lParam1!=0) {
			pThis->EnablePlugin(TRUE);
			return TRUE;
		}else{
			pThis->EnablePlugin(FALSE);
			return TRUE;
		}
		break;
	case TVTest::EVENT_FULLSCREENCHANGE:
		if( pThis->nwMode == TRUE ){
			if (lParam1!=0) {
				pThis->fullScreen = TRUE;
				pThis->ctrlDlg.StartFullScreenMouseChk();
			}else{
				pThis->fullScreen = FALSE;
				pThis->ctrlDlg.StopFullScreenMouseChk();
				pThis->ResetStreamingCtrlView();
			}
		}else{
			if (lParam1!=0) {
				pThis->fullScreen = TRUE;
			}else{
				pThis->fullScreen = FALSE;
			}
			pThis->ctrlDlg.StopFullScreenMouseChk();
			pThis->ResetStreamingCtrlView();
		}
		break;
	default:
		break;
	}

	return 0;
}

void CEpgTimerPlugIn::CtrlCmdCallbackInvoked()
{
	CBlockLock lock(&this->cmdLock);
	CMD_STREAM* cmdParam = &this->cmdCapture;
	CMD_STREAM* resParam = &this->resCapture;
	CEpgTimerPlugIn* sys = this;

	resParam->dataSize = 0;
	resParam->param = CMD_ERR;

	switch( cmdParam->param ){
	case CMD2_VIEW_APP_SET_BONDRIVER:
		AddDebugLog(L"TvTest:CMD2_VIEW_APP_SET_BONDRIVER");
		{
			wstring val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
				if( sys->m_pApp->SetDriverName(val.c_str()) == TRUE ){
					resParam->param = CMD_SUCCESS;
				}
				if( CompareNoCase(val, L"BonDriver_UDP.dll") == 0 ||
				    CompareNoCase(val, L"BonDriver_TCP.dll") == 0 ||
				    CompareNoCase(val, L"BonDriver_NetworkPipe.dll") == 0 ){
					sys->m_pApp->SetChannel(0, 0);
				}
			}
		}
		break;
	case CMD2_VIEW_APP_GET_BONDRIVER:
		AddDebugLog(L"TvTest:CMD2_VIEW_APP_GET_BONDRIVER");
		{
			WCHAR buff[512] = L"";
			sys->m_pApp->GetDriverFullPathName(buff, 512);
			wstring bonName = fs_path(buff).filename().native();
			if( bonName.size() > 0 ){
				resParam->data = NewWriteVALUE(bonName, resParam->dataSize);
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_VIEW_APP_SET_CH:
		AddDebugLog(L"TvTest:CMD2_VIEW_APP_SET_CH");
		{
			SET_CH_INFO val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
				if( val.useSID == TRUE && val.useBonCh == TRUE ){
					int space = 0;
					int ch = 0;
					TVTest::ChannelInfo chInfo;
					while(1){
						if( sys->m_pApp->GetChannelInfo(space, ch, &chInfo) == false ){
							if( ch == 0 ){
								AddDebugLog(L"TvTest:NotFind ChInfo");
								break;
							}else{
								space++;
								ch = 0;
							}
						}else{
							if( chInfo.Space == (int)val.space &&
								chInfo.Channel == (int)val.ch )
							{
								if( sys->m_pApp->SetChannel(space, ch, val.SID) == true ){
									resParam->param = CMD_SUCCESS;
									AddDebugLog(L"TvTest:m_pApp->SetChannel true");
								}else{
									AddDebugLog(L"TvTest:m_pApp->SetChannel false");
								}
								break;
							}
							ch++;
						}
					}
				}
			}
			if( sys->nwMode == TRUE ){
				// コマンド処理中なので直接SendNwPlayClose()を呼ぶとアプリケーション間でロックする恐れがある
				PostMessage(sys->ctrlDlg.GetDlgHWND(), CStreamCtrlDlg::WM_PLAY_CLOSE, 0, 0);
			}
		}
		break;
	case CMD2_VIEW_APP_CLOSE:
		AddDebugLog(L"TvTest:CMD2_VIEW_APP_CLOSE");
		{
			resParam->param = CMD_SUCCESS;
			sys->m_pApp->Close(1);
		}
		break;
	case CMD2_VIEW_APP_TT_SET_CTRL:
		AddDebugLog(L"TvTest:CMD2_VIEW_APP_TT_SET_CTRL");
		{
			if( ReadVALUE(&sys->nwModeInfo, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
				resParam->param = CMD_SUCCESS;
				// 投げるだけ
				PostMessage(sys->ctrlDlg.GetDlgHWND(), WM_TT_SET_CTRL, 0, 0);
			}
		}
		break;
	default:
		AddDebugLogFormat(L"TvTest:err default cmd %d", cmdParam->param);
		resParam->param = CMD_NON_SUPPORT;
		break;
	}
}

void CEpgTimerPlugIn::ResetStreamingCtrlView()
{
	if( this->nwMode == FALSE ){
		this->ctrlDlg.ShowCtrlDlg(SW_HIDE);
		return ;
	}
	WINDOWPLACEMENT info;
	info.length = sizeof(WINDOWPLACEMENT);

	if( GetWindowPlacement(this->m_pApp->GetAppWindow(), &info) == FALSE ){
		AddDebugLog(L"GetWindowPlacement err");
		return;
	}
	if( this->fullScreen == FALSE ){
		if( info.showCmd == SW_SHOWNORMAL ){
			RECT rc;
			GetWindowRect(this->m_pApp->GetAppWindow(), &rc);

			int x = rc.left;
			int y = rc.bottom+3;
			int cx = rc.right - rc.left;
			int cy = 65;

			this->ctrlDlg.ShowCtrlDlg(SW_SHOW);
			SetWindowPos(this->ctrlDlg.GetDlgHWND(), this->m_pApp->GetAppWindow(), x, y, cx, cy, SWP_SHOWWINDOW);
			this->showNormal = TRUE;
		}else if( info.showCmd == SW_SHOWMAXIMIZED ){
			RECT rc;
			GetWindowRect(this->m_pApp->GetAppWindow(), &rc);

			POINT pos;
			GetCursorPos(&pos);

			if( pos.y > rc.bottom - 65 ){
				int x = rc.left;
				int y = rc.bottom-65;
				int cx = rc.right - rc.left;
				int cy = 65;

				this->ctrlDlg.ShowCtrlDlg(SW_SHOW);
				SetWindowPos(this->ctrlDlg.GetDlgHWND(), HWND_TOPMOST, x, y, cx, cy, SWP_SHOWWINDOW);
			}else{
				this->ctrlDlg.ShowCtrlDlg(SW_HIDE);
			}
			this->showNormal = FALSE;
		}
	}
}

BOOL CALLBACK CEpgTimerPlugIn::WindowMsgeCallback(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,LRESULT *pResult,void *pUserData)
{
	CEpgTimerPlugIn* sys = (CEpgTimerPlugIn*)pUserData;
	if( sys->nwMode == TRUE ){
		switch(uMsg){
		case WM_SIZE:
			sys->ResetStreamingCtrlView();
			break;
		case WM_MOVE:
			sys->ResetStreamingCtrlView();
			break;
		case WM_MOUSEMOVE:
			if( sys->showNormal == FALSE ){
				sys->ResetStreamingCtrlView();
			}
			break;
		}
	}
	return FALSE;
}

LRESULT CALLBACK CEpgTimerPlugIn::StreamCtrlDlgCallback(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,void *pUserData)
{
	CEpgTimerPlugIn* sys = (CEpgTimerPlugIn*)pUserData;
	{
		switch(uMsg){
		case CStreamCtrlDlg::WM_CHG_PORT:
			{
				WCHAR buff[512] = L"";
				sys->m_pApp->GetDriverFullPathName(buff, 512);
				wstring bonName = fs_path(buff).filename().native();
				DWORD udpPort = (DWORD)wParam;
				DWORD tcpPort = (DWORD)lParam;
				if( tcpPort < 65536 ){
					if( CompareNoCase(bonName, tcpPort < 2230 ? L"BonDriver_NetworkPipe.dll" : L"BonDriver_TCP.dll") != 0 ){
						sys->m_pApp->SetDriverName(tcpPort < 2230 ? L"BonDriver_NetworkPipe.dll" : L"BonDriver_TCP.dll");
					}
					sys->m_pApp->SetChannel(0, (int)(tcpPort < 2230 ? tcpPort : tcpPort - 2230));
				}else if( 1234 <= udpPort && udpPort < 65536 ){
					if( CompareNoCase(bonName, L"BonDriver_UDP.dll") != 0 ){
						sys->m_pApp->SetDriverName(L"BonDriver_UDP.dll");
					}
					sys->m_pApp->SetChannel(0, (int)(udpPort - 1234));
				}
			}
			return TRUE;
		case CStreamCtrlDlg::WM_PLAY_CLOSE:
			if( sys->nwMode ){
				sys->nwModeInfo.enableMode = FALSE;
				sys->ctrlDlg.SetCtrl(sys->nwModeInfo);
				sys->nwMode = FALSE;
				sys->ResetStreamingCtrlView();
			}
			return TRUE;
		case WM_INVOKE_CTRL_CMD:
			sys->CtrlCmdCallbackInvoked();
			return TRUE;
		case WM_TT_SET_CTRL:
			sys->ctrlDlg.SetCtrl(sys->nwModeInfo);
			sys->nwMode = sys->nwModeInfo.enableMode ? TRUE : FALSE;
			sys->ResetStreamingCtrlView();
			return TRUE;
		}
	}
	return FALSE;
}

