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
	this->nwModeCurrentCtrlID = 0;
	this->fullScreen = FALSE;
	this->showNormal = TRUE;
	this->cmdPool.param = CMD_ERR;
	this->cmdPool.dataSize = 0;
	this->resPool.param = CMD_ERR;
	this->resPool.dataSize = 0;
	InitializeCriticalSection(&this->cmdLock);
}

CEpgTimerPlugIn::~CEpgTimerPlugIn()
{
	DeleteCriticalSection(&this->cmdLock);
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
		OutputDebugString(L"EnablePlugin");
		if(this->m_pApp->SetWindowMessageCallback(WindowMsgeCallback, this)==false){
			OutputDebugString(L"●TVTest Version Err::SetWindowMessageCallback");
		}

		wstring pipeName = L"";
		wstring eventName = L"";

		Format(pipeName, L"%s%d", CMD2_TVTEST_CTRL_PIPE, GetCurrentProcessId());
		Format(eventName, L"%s%d", CMD2_TVTEST_CTRL_WAIT_CONNECT, GetCurrentProcessId());

		OutputDebugString(pipeName.c_str());
		OutputDebugString(eventName.c_str());
		this->pipeServer.StartServer(eventName.c_str(), pipeName.c_str(), CtrlCmdCallback, this);

		if( this->nwMode == TRUE ){
			this->ctrlDlg.SetCtrlCmd(&this->cmd, this->nwModeInfo.ctrlID, this->nwModeInfo.udpSend, this->nwModeInfo.tcpSend, FALSE, this->nwModeInfo.timeShiftMode);
		}
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
		this->cmd.SendNwPlayClose(this->nwModeCurrentCtrlID);
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

int CALLBACK CEpgTimerPlugIn::CtrlCmdCallback(void* param, CMD_STREAM* cmdParam, CMD_STREAM* resParam)
{
	CEpgTimerPlugIn* sys = (CEpgTimerPlugIn*)param;

	resParam->dataSize = 0;
	resParam->param = CMD_NON_SUPPORT;

	if( cmdParam->dataSize <= sizeof(sys->cmdPool.data) ){
		resParam->param = CMD_ERR;
		// SendMessageTimeout()はメッセージ処理中でも容赦なくタイムアウトするのでコマンドデータを排他処理する
		{
			CAutoLock lock(&sys->cmdLock);
			sys->cmdPool.param = cmdParam->param;
			sys->cmdPool.dataSize = cmdParam->dataSize;
			if( cmdParam->dataSize > 0 ){
				memcpy(sys->cmdPool.data, cmdParam->data, cmdParam->dataSize);
			}
		}
		// CtrlCmdCallbackInvoked()をメインスレッドで呼ぶ(デッドロック防止のためタイムアウトつき)
		DWORD_PTR dwResult;
		if( SendMessageTimeout(sys->ctrlDlg.GetDlgHWND(), WM_INVOKE_CTRL_CMD, 0, 0, SMTO_NORMAL, 10000, &dwResult) ){
			CAutoLock lock(&sys->cmdLock);
			resParam->param = sys->resPool.param;
			resParam->dataSize = sys->resPool.dataSize;
			if( resParam->dataSize > 0 ){
				resParam->data = new BYTE[resParam->dataSize];
				memcpy(resParam->data, sys->resPool.data, resParam->dataSize);
			}
		}
	}
	return 0;
}

void CEpgTimerPlugIn::CtrlCmdCallbackInvoked()
{
	CAutoLock lock(&this->cmdLock);
	CMD_STREAM_POOL* cmdParam = &this->cmdPool;
	CMD_STREAM_POOL* resParam = &this->resPool;
	CEpgTimerPlugIn* sys = this;

	resParam->dataSize = 0;
	resParam->param = CMD_ERR;

	switch( cmdParam->param ){
	case CMD2_VIEW_APP_SET_BONDRIVER:
		OutputDebugString(L"TvTest:CMD2_VIEW_APP_SET_BONDRIVER");
		{
			wstring val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
				if( sys->m_pApp->SetDriverName(val.c_str()) == TRUE ){
					resParam->param = CMD_SUCCESS;
				}
				if( CompareNoCase(val, L"BonDriver_UDP.dll") == 0 || CompareNoCase(val, L"BonDriver_TCP.dll") == 0 ){
					sys->m_pApp->SetChannel(0, 0);
				}
			}
		}
		break;
	case CMD2_VIEW_APP_GET_BONDRIVER:
		OutputDebugString(L"TvTest:CMD2_VIEW_APP_GET_BONDRIVER");
		{
			WCHAR buff[512] = L"";
			sys->m_pApp->GetDriverFullPathName(buff, 512);
			wstring bonName;
			GetFileName(buff, bonName );
			if( bonName.size() > 0 ){
				BYTE* newBuff = NewWriteVALUE(bonName, resParam->dataSize);
				memcpy(resParam->data, newBuff, resParam->dataSize);
				delete[] newBuff;
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_VIEW_APP_SET_CH:
		OutputDebugString(L"TvTest:CMD2_VIEW_APP_SET_CH");
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
								OutputDebugString(L"TvTest:NotFind ChInfo");
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
									OutputDebugString(L"TvTest:m_pApp->SetChannel true");
								}else{
									OutputDebugString(L"TvTest:m_pApp->SetChannel false");
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
		OutputDebugString(L"TvTest:CMD2_VIEW_APP_CLOSE");
		{
			resParam->param = CMD_SUCCESS;
			sys->m_pApp->Close(1);
		}
		break;
	case CMD2_VIEW_APP_TT_SET_CTRL:
		OutputDebugString(L"TvTest:CMD2_VIEW_APP_TT_SET_CTRL");
		{
			if( ReadVALUE(&sys->nwModeInfo, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
				resParam->param = CMD_SUCCESS;
				// 投げるだけ
				PostMessage(sys->ctrlDlg.GetDlgHWND(), WM_TT_SET_CTRL, 0, 0);
			}
		}
		break;
	default:
		_OutputDebugString(L"TvTest:err default cmd %d\r\n", cmdParam->param);
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
		_OutputDebugString(L"GetWindowPlacement err");
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
				wstring bonName;
				GetFileName(buff, bonName );
				if( lParam != 0){
					if( CompareNoCase(bonName, L"BonDriver_TCP.dll") != 0 ){
						sys->m_pApp->SetDriverName(L"BonDriver_TCP.dll");
					}
					DWORD ch = (DWORD)lParam-2230;
					sys->m_pApp->SetChannel(0, ch);
				}else
				if( wParam != 0){
					if( CompareNoCase(bonName, L"BonDriver_UDP.dll") != 0 ){
						sys->m_pApp->SetDriverName(L"BonDriver_UDP.dll");
					}
					DWORD ch = (DWORD)wParam-1234;
					sys->m_pApp->SetChannel(0, ch);
				}
			}
			return TRUE;
		case CStreamCtrlDlg::WM_PLAY_CLOSE:
			if( sys->nwMode ){
				sys->cmd.SendNwPlayClose(sys->nwModeCurrentCtrlID);
				sys->nwMode = FALSE;
				sys->ResetStreamingCtrlView();
			}
			return TRUE;
		case WM_INVOKE_CTRL_CMD:
			sys->CtrlCmdCallbackInvoked();
			return TRUE;
		case WM_TT_SET_CTRL:
			if( sys->nwMode ){
				sys->cmd.SendNwPlayClose(sys->nwModeCurrentCtrlID);
				sys->ctrlDlg.StopTimer();
				sys->nwMode = FALSE;
			}
			if( sys->nwModeInfo.enableMode == TRUE ){
				sys->nwModeCurrentCtrlID = sys->nwModeInfo.ctrlID;
				sys->nwMode = TRUE;

				wstring ip = L"";
				Format(ip, L"%d.%d.%d.%d",
					(sys->nwModeInfo.serverIP&0xFF000000)>>24,
					(sys->nwModeInfo.serverIP&0x00FF0000)>>16,
					(sys->nwModeInfo.serverIP&0x0000FF00)>>8,
					(sys->nwModeInfo.serverIP&0x000000FF));

				sys->cmd.SetSendMode(TRUE);
				sys->cmd.SetNWSetting(ip,sys->nwModeInfo.serverPort);
				sys->cmd.SetConnectTimeOut(15*1000);
				sys->ctrlDlg.SetCtrlCmd(&sys->cmd, sys->nwModeInfo.ctrlID, sys->nwModeInfo.udpSend, sys->nwModeInfo.tcpSend, TRUE, sys->nwModeInfo.timeShiftMode);
			}
			sys->ResetStreamingCtrlView();
			return TRUE;
		}
	}
	return FALSE;
}

