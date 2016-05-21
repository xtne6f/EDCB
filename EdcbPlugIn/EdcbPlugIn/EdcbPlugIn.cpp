// EdcbPlugIn.cpp : DLL アプリケーション用にエクスポートされる関数を定義します。
//

#include "stdafx.h"
#include "EdcbPlugIn.h"
#include "../../Common/StringUtil.h"
#include "../../Common/PathUtil.h"
#include "../../Common/CommonDef.h"
#include "../../Common/EpgTimerUtil.h"
#include "../../Common/SendCtrlCmd.h"
#include "../../Common/TsPacketUtil.h"
#include "../../Common/BlockLock.h"
#include "../../Common/ParseTextInstances.h"
#include <process.h>

namespace
{

wstring GetDllIniPath()
{
	WCHAR szDllPath[MAX_PATH];
	DWORD len = GetModuleFileName(g_hinstDLL, szDllPath, MAX_PATH);
	if (len != 0 && len < MAX_PATH) {
		WCHAR szPath[_MAX_DRIVE + _MAX_DIR + _MAX_FNAME + 4 + 8];
		WCHAR szDrive[_MAX_DRIVE];
		WCHAR szDir[_MAX_DIR];
		WCHAR szFname[_MAX_FNAME];
		_wsplitpath_s(szDllPath, szDrive, _MAX_DRIVE, szDir, _MAX_DIR, szFname, _MAX_FNAME, nullptr, 0);
		_wmakepath_s(szPath, szDrive, szDir, szFname, L".ini");
		return szPath;
	}
	return L"";
}

BOOL DuplicateSave(LPCWSTR originalPath, DWORD *targetID, wstring *targetPath)
{
	vector<WCHAR> buf;
	if (targetPath) {
		buf.assign(targetPath->begin(), targetPath->end());
		buf.resize(buf.size() + 16, L'\0');
	}
	BOOL ret = FALSE;
	wstring dir;
	GetModuleFolderPath(dir);
	HMODULE hDll = LoadLibrary((dir + L"\\Write_Multi.dll").c_str());
	if (hDll) {
		BOOL (WINAPI*pfnDuplicateSave)(LPCWSTR,DWORD*,WCHAR*,DWORD,int,ULONGLONG) =
			reinterpret_cast<BOOL (WINAPI*)(LPCWSTR,DWORD*,WCHAR*,DWORD,int,ULONGLONG)>(GetProcAddress(hDll, "DuplicateSave"));
		if (pfnDuplicateSave) {
			ret = pfnDuplicateSave(originalPath, targetID, targetPath ? &buf.front() : nullptr, static_cast<DWORD>(buf.size()), -1, 0);
		}
		FreeLibrary(hDll);
	}
	if (ret && targetPath) {
		*targetPath = &buf.front();
	}
	return ret;
}

}

enum {
	CH_CHANGE_OK = 0,
	CH_CHANGE_ERR,
};

enum {
	WM_INVOKE_CTRL_CMD = WM_APP,
	WM_APP_CLOSE,
	WM_APP_ADD_LOG,
	WM_UPDATE_STATUS_CODE,
	WM_SIGNAL_UPDATE_START,
	WM_EPGCAP_START,
	WM_EPGCAP_BACK_START,
	WM_EPGCAP_STOP,
	WM_EPGCAP_BACK_STOP,
};

bool CEdcbPlugIn::CMyEventHandler::OnChannelChange()
{
	TVTest::ChannelInfo ci;
	bool ret = m_outer.m_pApp->GetCurrentChannelInfo(&ci);
	{
		CBlockLock lock(&m_outer.m_streamLock);
		// EpgDataCap3は内部メソッド単位でアトミック。UnInitialize()中にワーカースレッドにアクセスさせないよう排他制御が必要
		m_outer.m_epgUtil.UnInitialize();
		m_outer.m_epgUtil.Initialize(FALSE, m_outer.m_epgUtilPath.c_str());
		m_outer.m_chChangeID = CH_CHANGE_ERR;
		if (ret) {
			m_outer.m_chChangeID = static_cast<DWORD>(ci.NetworkID) << 16 | ci.TransportStreamID;
			m_outer.m_chChangeTick = GetTickCount();
		}
	}
	m_outer.m_chChangedAfterSetCh = true;
	SendMessage(m_outer.m_hwnd, WM_UPDATE_STATUS_CODE, 0, 0);
	SendMessage(m_outer.m_hwnd, WM_EPGCAP_BACK_START, 0, 0);
	return false;
}

bool CEdcbPlugIn::CMyEventHandler::OnDriverChange()
{
	{
		CBlockLock lock(&m_outer.m_statusLock);
		WCHAR name[MAX_PATH];
		m_outer.m_currentBonDriver = (m_outer.m_pApp->GetDriverName(name, _countof(name)) > 0 ? name : L"");
	}
	// ストリームコールバックはチューナ使用時だけ
	m_outer.m_pApp->SetStreamCallback(m_outer.IsTunerBonDriver() ? 0 : TVTest::STREAM_CALLBACK_REMOVE, StreamCallback, &m_outer);
	m_outer.m_lastSetCh.useSID = FALSE;
	SendMessage(m_outer.m_hwnd, WM_UPDATE_STATUS_CODE, 0, 0);
	SendMessage(m_outer.m_hwnd, WM_EPGCAP_STOP, 0, 0);
	return false;
}

bool CEdcbPlugIn::CMyEventHandler::OnRecordStatusChange(int Status)
{
	if (Status == TVTest::RECORD_STATUS_NOTRECORDING) {
		if (m_outer.IsEdcbRecording()) {
			// キャンセル動作とみなす
			CBlockLock lock(&m_outer.m_streamLock);
			for (map<DWORD, REC_CTRL>::iterator it = m_outer.m_recCtrlMap.begin(); it != m_outer.m_recCtrlMap.end(); ++it) {
				if (!it->second.filePath.empty()) {
					std::swap(it->second.filePath, wstring());
					std::swap(it->second.dropCount, CDropCount());
				}
			}
		}
	}
	SendMessage(m_outer.m_hwnd, WM_UPDATE_STATUS_CODE, 0, 0);
	return false;
}

void CEdcbPlugIn::CMyEventHandler::OnStartupDone()
{
	wstring dir;
	GetFileFolder(GetDllIniPath(), dir);
	if (GetModuleHandle((dir + L"\\EpgTimerPlugIn.tvtp").c_str())) {
		// 1プロセスに複数サーバは想定外なので開始しない(この判定方法は確実ではない)
		m_outer.m_pApp->AddLog(L"EpgTimerPlugInが読み込まれているため正常動作しません。", TVTest::LOG_TYPE_ERROR);
	}
	else {
		wstring pid;
		Format(pid, L"%d", GetCurrentProcessId());
		m_outer.m_pipeServer.StartServer((CMD2_VIEW_CTRL_WAIT_CONNECT + pid).c_str(), (CMD2_VIEW_CTRL_PIPE + pid).c_str(), CtrlCmdCallback, &m_outer);
	}
}

CEdcbPlugIn::CEdcbPlugIn()
	: m_handler(*this)
	, m_hwnd(nullptr)
	, m_outCtrlID(-1)
	, m_statusCode(VIEW_APP_ST_ERR_BON)
	, m_chChangeID(CH_CHANGE_OK)
	, m_epgFile(INVALID_HANDLE_VALUE)
	, m_epgReloadThread(nullptr)
	, m_epgCapBack(false)
	, m_recCtrlCount(0)
{
	m_lastSetCh.useSID = FALSE;
	InitializeCriticalSection(&m_streamLock);
	InitializeCriticalSection(&m_statusLock);
}

CEdcbPlugIn::~CEdcbPlugIn()
{
	DeleteCriticalSection(&m_statusLock);
	DeleteCriticalSection(&m_streamLock);
}

bool CEdcbPlugIn::GetPluginInfo(TVTest::PluginInfo *pInfo)
{
	pInfo->Type = TVTest::PLUGIN_TYPE_NORMAL;
	pInfo->Flags = TVTest::PLUGIN_FLAG_NOENABLEDDISABLED;
	pInfo->pszPluginName = L"EDCB PlugIn";
	pInfo->pszCopyright = L"りょうちん Copyright (C) 2010; Git-fork(xtne6f)";
	pInfo->pszDescription = L"EpgDataCap_Bonのように振舞う";
	return true;
}

bool CEdcbPlugIn::Initialize()
{
	if (m_pApp->GetVersion() < TVTest::MakeVersion(0, 9, 0)) {
		m_pApp->AddLog(L"TVTestのバージョンが古いため初期化できません。");
		return false;
	}
	m_edcbDir = GetPrivateProfileToString(L"SET", L"EdcbFolderPath", L"", GetDllIniPath().c_str());
	ChkFolderPath(m_edcbDir);
	// 未指定のときはTVTestと同階層のEDCBフォルダ
	GetModuleFolderPath(m_epgUtilPath);
	if (m_edcbDir.empty()) {
		GetFileFolder(m_epgUtilPath, m_edcbDir);
		if (!m_edcbDir.empty()) {
			m_edcbDir += L"\\EDCB";
		}
	}
	m_epgUtilPath += L"\\EpgDataCap3.dll";
	if (m_epgUtil.Initialize(FALSE, m_epgUtilPath.c_str()) != NO_ERR) {
		m_epgUtilPath = m_edcbDir + L"\\EpgDataCap3.dll";
		if (m_epgUtil.Initialize(FALSE, m_epgUtilPath.c_str()) != NO_ERR) {
			m_epgUtilPath.clear();
		}
	}
	if (m_edcbDir.empty() || m_epgUtilPath.empty()) {
		m_pApp->AddLog(L"EpgDataCap3.dllが見つかりません。", TVTest::LOG_TYPE_ERROR);
		return false;
	}
	CParseChText5 chText5;
	if (!chText5.ParseText((GetEdcbSettingPath() + L"\\ChSet5.txt").c_str())) {
		m_pApp->AddLog(L"ChSet5.txtが見つかりません。", TVTest::LOG_TYPE_ERROR);
		return false;
	}
	m_chSet5.clear();
	for (map<LONGLONG, CH_DATA5>::const_iterator it = chText5.GetMap().begin(); it != chText5.GetMap().end(); ++it) {
		m_chSet5.push_back(it->second);
	}
	// イベントコールバック関数を登録
	m_pApp->SetEventCallback(CMyEventHandler::EventCallback, &m_handler);

	WNDCLASS wc = {};
	wc.lpfnWndProc = WndProc;
	wc.hInstance = g_hinstDLL;
	wc.lpszClassName = L"EDCB PlugIn";
	if (!RegisterClass(&wc)) {
		return false;
	}
	CreateWindow(L"EDCB PlugIn", nullptr, 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, g_hinstDLL, this);
	return m_hwnd != nullptr;
}

bool CEdcbPlugIn::Finalize()
{
	PostMessage(m_hwnd, WM_CLOSE, 0, 0);
	MSG msg;
	while (m_hwnd && GetMessage(&msg, m_hwnd, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	m_epgUtil.UnInitialize();
	return true;
}

vector<CH_DATA5> CEdcbPlugIn::GetEpgCheckList(WORD onid, WORD tsid, int sid, bool basicFlag) const
{
	vector<CH_DATA5> chkList;
	vector<CH_DATA5>::const_iterator it;
	for (it = m_chSet5.begin(); it != m_chSet5.end(); ++it) {
		if (it->originalNetworkID == onid && it->transportStreamID == tsid && (it->serviceID == sid || it->epgCapFlag)) {
			chkList.push_back(*it);
		}
	}
	if (!chkList.empty() && basicFlag) {
		chkList.clear();
		for (it = m_chSet5.begin(); it != m_chSet5.end(); ++it) {
			if (it->originalNetworkID == onid && (it->transportStreamID == tsid && it->serviceID == sid || it->epgCapFlag)) {
				chkList.push_back(*it);
			}
		}
	}
	return chkList;
}

LRESULT CALLBACK CEdcbPlugIn::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_CREATE) {
		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(reinterpret_cast<LPCREATESTRUCT>(lParam)->lpCreateParams));
	}
	CEdcbPlugIn *pThis = reinterpret_cast<CEdcbPlugIn*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
	if (pThis) {
		LRESULT ret = pThis->WndProc_(hwnd, uMsg, wParam, lParam);
		if (uMsg == WM_DESTROY) {
			SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
		}
		return ret;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CEdcbPlugIn::WndProc_(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	enum {
		TIMER_TRY_STOP_SERVER = 1,
		TIMER_SIGNAL_UPDATE,
		TIMER_EPGCAP,
		TIMER_EPGCAP_BACK,
	};

	switch (uMsg) {
	case WM_CREATE:
		{
			m_hwnd = hwnd;
			wstring bonCtrlIniPath = m_edcbDir + L"\\BonCtrl.ini";
			m_epgCapTimeout = GetPrivateProfileInt(L"EPGCAP", L"EpgCapTimeOut", 15, bonCtrlIniPath.c_str());
			m_epgCapSaveTimeout = GetPrivateProfileInt(L"EPGCAP", L"EpgCapSaveTimeOut", 0, bonCtrlIniPath.c_str()) != 0;
			wstring iniPath = GetDllIniPath();
			m_nonTunerDrivers = L"::" + GetPrivateProfileToString(L"SET", L"NonTunerDrivers",
				L"BonDriver_UDP.dll:BonDriver_TCP.dll:BonDriver_File.dll:BonDriver_RecTask.dll:BonDriver_Pipe.dll", iniPath.c_str()) + L':';
			m_recNamePrefix = GetPrivateProfileToString(L"SET", L"RecNamePrefix", L"", iniPath.c_str());
			m_noLogScramble = GetPrivateProfileInt(L"SET", L"NoLogScramble", 0, iniPath.c_str()) != 0;
			m_epgCapBackStartWaitSec = GetPrivateProfileInt(L"SET", L"EpgCapLive", 1, iniPath.c_str()) == 0 ? MAXDWORD :
				GetPrivateProfileInt(L"SET", L"EpgCapBackStartWaitSec", 30, iniPath.c_str());
			m_epgCapBackBSBasic = GetPrivateProfileInt(L"SET", L"EpgCapBackBSBasicOnly", 1, iniPath.c_str()) != 0;
			m_epgCapBackCS1Basic = GetPrivateProfileInt(L"SET", L"EpgCapBackCS1BasicOnly", 1, iniPath.c_str()) != 0;
			m_epgCapBackCS2Basic = GetPrivateProfileInt(L"SET", L"EpgCapBackCS2BasicOnly", 1, iniPath.c_str()) != 0;
		}
		return 0;
	case WM_DESTROY:
		m_pipeServer.StopServer();
		SendMessage(hwnd, WM_EPGCAP_STOP, 0, 0);
		m_pApp->SetStreamCallback(TVTest::STREAM_CALLBACK_REMOVE, StreamCallback);
		if (m_epgReloadThread) {
			if (WaitForSingleObject(m_epgReloadThread, 5000) == WAIT_TIMEOUT) {
				TerminateThread(m_epgReloadThread, 0xFFFFFFFF);
			}
			CloseHandle(m_epgReloadThread);
			m_epgReloadThread = nullptr;
		}
		m_hwnd = nullptr;
		return 0;
	case WM_CLOSE:
		// デッドロック回避のためメッセージポンプを維持しつつサーバを終わらせる
		m_pipeServer.StopServer(TRUE);
		SetTimer(hwnd, TIMER_TRY_STOP_SERVER, 10, nullptr);
		return 0;
	case WM_TIMER:
		switch (wParam) {
		case TIMER_TRY_STOP_SERVER:
			if (m_pipeServer.StopServer(TRUE)) {
				KillTimer(hwnd, TIMER_TRY_STOP_SERVER);
				DestroyWindow(hwnd);
			}
			return 0;
		case TIMER_SIGNAL_UPDATE:
			if (IsEdcbRecording()) {
				TVTest::StatusInfo si;
				if (!m_pApp->GetStatus(&si)) {
					si.SignalLevel = 0;
				}
				CBlockLock lock(&m_streamLock);
				for (map<DWORD, REC_CTRL>::iterator it = m_recCtrlMap.begin(); it != m_recCtrlMap.end(); ++it) {
					if (!it->second.filePath.empty()) {
						it->second.dropCount.SetSignal(si.SignalLevel);
					}
				}
			}
			else {
				KillTimer(hwnd, TIMER_SIGNAL_UPDATE);
			}
			return 0;
		case TIMER_EPGCAP:
			if (!IsNotRecording()) {
				SendMessage(hwnd, WM_EPGCAP_STOP, 0, 0);
			}
			else if (!m_epgCapChList.empty()) {
				if (m_epgCapChkNext) {
					while (!m_epgCapChList.empty()) {
						SET_CH_INFO &chInfo = m_epgCapChList.front();
						if (!(chInfo.ONID == 4 && m_epgCapChkBS || chInfo.ONID == 6 && m_epgCapChkCS1 || chInfo.ONID == 7 && m_epgCapChkCS2)) {
							TVTest::ChannelSelectInfo si = {};
							si.Size = sizeof(si);
							si.Space = -1;
							si.Channel = -1;
							si.NetworkID = chInfo.ONID;
							si.TransportStreamID = chInfo.TSID;
							if (m_pApp->SelectChannel(&si)) {
								m_epgCapStartTick = GetTickCount();
								m_epgCapChkNext = false;
								break;
							}
						}
						m_epgCapChList.erase(m_epgCapChList.begin());
					}
				}
				else {
					DWORD chChangeID;
					{
						CBlockLock lock(&m_streamLock);
						chChangeID = m_chChangeID;
					}
					bool saveEpgFile = false;
					if (chChangeID != CH_CHANGE_OK) {
						if (chChangeID == CH_CHANGE_ERR || GetTickCount() - m_epgCapStartTick > 15000) {
							// チャンネル切り替えエラーか切り替えに15秒以上かかってるので無信号と判断
							m_epgCapChList.erase(m_epgCapChList.begin());
							m_epgCapChkNext = true;
						}
					}
					else if (GetTickCount() - m_epgCapStartTick > m_epgCapTimeout * 60000) {
						// m_epgCapTimeout分以上かかっているなら停止
						m_epgCapChList.erase(m_epgCapChList.begin());
						m_epgCapChkNext = true;
						saveEpgFile = m_epgCapSaveTimeout;
					}
					else {
						SET_CH_INFO &chInfo = m_epgCapChList.front();
						bool basicFlag = (chInfo.ONID == 4 && m_epgCapBSBasic || chInfo.ONID == 6 && m_epgCapCS1Basic || chInfo.ONID == 7 && m_epgCapCS2Basic);
						vector<CH_DATA5> chkList = GetEpgCheckList(chInfo.ONID, chInfo.TSID, chInfo.SID, basicFlag);
						if (chkList.empty()) {
							m_epgCapChList.erase(m_epgCapChList.begin());
							m_epgCapChkNext = true;
						}
						else if (m_epgFile == INVALID_HANDLE_VALUE) {
							// 保存開始
							wstring name;
							Format(name, L"%04X%04X_epg.dat", chInfo.ONID, basicFlag ? 0xFFFF : chInfo.TSID);
							m_epgFilePath = GetEdcbSettingPath() + L"\\EpgData\\" + name;
							HANDLE epgFile = _CreateDirectoryAndFile((m_epgFilePath + L".tmp").c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
							if (epgFile != INVALID_HANDLE_VALUE) {
								m_pApp->AddLog((L'★' + name).c_str());
								CBlockLock lock(&m_streamLock);
								m_epgFile = epgFile;
								m_epgFileState = EPG_FILE_ST_NONE;
							}
							m_epgUtil.ClearSectionStatus();
						}
						else {
							// 蓄積状態チェック
							for (vector<CH_DATA5>::iterator it = chkList.begin(); it != chkList.end(); ++it) {
								pair<EPG_SECTION_STATUS, BOOL> status = m_epgUtil.GetSectionStatusService(it->originalNetworkID, it->transportStreamID, it->serviceID, it->partialFlag);
								if (!status.second) {
									status.first = m_epgUtil.GetSectionStatus(it->partialFlag);
								}
								if (status.first != EpgNoData) {
									m_epgCapChkNext = true;
									if (status.first != EpgHEITAll && status.first != EpgLEITAll && (status.first != EpgBasicAll || !basicFlag)) {
										m_epgCapChkNext = false;
										break;
									}
								}
							}
							if (m_epgCapChkNext) {
								if (chInfo.ONID == 4) {
									m_epgCapChkBS = m_epgCapBSBasic;
								}
								else if (chInfo.ONID == 6) {
									m_epgCapChkCS1 = m_epgCapCS1Basic;
								}
								else if (chInfo.ONID == 7) {
									m_epgCapChkCS2 = m_epgCapCS2Basic;
								}
								m_epgCapChList.erase(m_epgCapChList.begin());
								saveEpgFile = true;
							}
						}
					}
					if (m_epgCapChkNext && m_epgFile != INVALID_HANDLE_VALUE) {
						// 保存終了
						HANDLE epgFile = m_epgFile;
						{
							CBlockLock lock(&m_streamLock);
							m_epgFile = INVALID_HANDLE_VALUE;
						}
						CloseHandle(epgFile);
						if (saveEpgFile) {
							CopyFile((m_epgFilePath + L".tmp").c_str(), m_epgFilePath.c_str(), FALSE);
						}
						DeleteFile((m_epgFilePath + L".tmp").c_str());
					}
				}
			}
			if (m_epgCapChList.empty()) {
				// 全部チェック終わったので終了
				KillTimer(hwnd, TIMER_EPGCAP);
				SendMessage(hwnd, WM_UPDATE_STATUS_CODE, 0, 0);
			}
			return 0;
		case TIMER_EPGCAP_BACK:
			if (m_epgCapBack) {
				bool saveEpgFile = false;
				if (GetTickCount() - m_epgCapBackStartTick > max(m_epgCapBackStartWaitSec, 15) * 1000) {
					WORD onid;
					WORD tsid;
					if (m_chChangeID != CH_CHANGE_OK || m_epgUtil.GetTSID(&onid, &tsid) != NO_ERR) {
						m_epgCapBack = false;
					}
					else if (GetTickCount() - m_epgCapBackStartTick > m_epgCapTimeout * 60000 + max(m_epgCapBackStartWaitSec, 15) * 1000) {
						// m_epgCapTimeout分以上かかっているなら停止
						m_epgCapBack = false;
						saveEpgFile = m_epgCapSaveTimeout;
					}
					else {
						bool basicFlag = (onid == 4 && m_epgCapBackBSBasic || onid == 6 && m_epgCapBackCS1Basic || onid == 7 && m_epgCapBackCS2Basic);
						vector<CH_DATA5> chkList = GetEpgCheckList(onid, tsid, -1, basicFlag);
						if (chkList.empty()) {
							m_epgCapBack = false;
						}
						else if (m_epgFile == INVALID_HANDLE_VALUE) {
							// 保存開始
							wstring name;
							Format(name, L"%04X%04X_epg.dat", onid, basicFlag ? 0xFFFF : tsid);
							m_epgFilePath = GetEdcbSettingPath() + L"\\EpgData\\" + name;
							HANDLE epgFile = _CreateDirectoryAndFile((m_epgFilePath + L".tmp").c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
							if (epgFile != INVALID_HANDLE_VALUE) {
								m_pApp->AddLog((L'★' + name).c_str());
								CBlockLock lock(&m_streamLock);
								m_epgFile = epgFile;
								m_epgFileState = EPG_FILE_ST_NONE;
							}
							m_epgUtil.ClearSectionStatus();
						}
						else {
							// 蓄積状態チェック
							bool chkNext = false;
							for (vector<CH_DATA5>::iterator it = chkList.begin(); it != chkList.end(); ++it) {
								pair<EPG_SECTION_STATUS, BOOL> status = m_epgUtil.GetSectionStatusService(it->originalNetworkID, it->transportStreamID, it->serviceID, it->partialFlag);
								if (!status.second) {
									status.first = m_epgUtil.GetSectionStatus(it->partialFlag);
								}
								if (status.first != EpgNoData) {
									chkNext = true;
									if (status.first != EpgHEITAll && status.first != EpgLEITAll && (status.first != EpgBasicAll || !basicFlag)) {
										chkNext = false;
										break;
									}
								}
							}
							if (chkNext) {
								m_epgCapBack = false;
								saveEpgFile = true;
							}
						}
					}
				}
				if (!m_epgCapBack && m_epgFile != INVALID_HANDLE_VALUE) {
					// 保存終了
					HANDLE epgFile = m_epgFile;
					{
						CBlockLock lock(&m_streamLock);
						m_epgFile = INVALID_HANDLE_VALUE;
					}
					CloseHandle(epgFile);
					if (saveEpgFile) {
						CopyFile((m_epgFilePath + L".tmp").c_str(), m_epgFilePath.c_str(), FALSE);
					}
					DeleteFile((m_epgFilePath + L".tmp").c_str());
					if (saveEpgFile) {
						if (m_epgReloadThread && WaitForSingleObject(m_epgReloadThread, 0) != WAIT_TIMEOUT) {
							CloseHandle(m_epgReloadThread);
							m_epgReloadThread = nullptr;
						}
						if (!m_epgReloadThread) {
							m_epgReloadThread = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, ReloadEpgThread, nullptr, 0, nullptr));
						}
					}
				}
			}
			if (!m_epgCapBack) {
				KillTimer(hwnd, TIMER_EPGCAP_BACK);
			}
			return 0;
		}
		break;
	case WM_INVOKE_CTRL_CMD:
		CtrlCmdCallbackInvoked(reinterpret_cast<CMD_STREAM*>(wParam), reinterpret_cast<CMD_STREAM*>(lParam));
		return 0;
	case WM_APP_CLOSE:
		m_pApp->Close(TVTest::CLOSE_EXIT);
		return 0;
	case WM_APP_ADD_LOG:
		m_pApp->AddLog(reinterpret_cast<LPCWSTR>(lParam));
		return 0;
	case WM_UPDATE_STATUS_CODE:
		{
			CBlockLock lock(&m_statusLock);
			m_statusCode = m_currentBonDriver.empty() ? VIEW_APP_ST_ERR_BON :
			               !IsNotRecording() ? VIEW_APP_ST_REC :
			               !m_epgCapChList.empty() ? VIEW_APP_ST_GET_EPG :
			               m_chChangeID == CH_CHANGE_ERR ? VIEW_APP_ST_ERR_CH_CHG : VIEW_APP_ST_NORMAL;
		}
		return 0;
	case WM_SIGNAL_UPDATE_START:
		SetTimer(hwnd, TIMER_SIGNAL_UPDATE, 2000, nullptr);
		return 0;
	case WM_EPGCAP_START:
		{
			SendMessage(hwnd, WM_EPGCAP_STOP, 0, 0);
			if (IsTunerBonDriver()) {
				m_epgCapChList = *reinterpret_cast<vector<SET_CH_INFO>*>(lParam);
				if (!m_epgCapChList.empty()) {
					SetTimer(hwnd, TIMER_EPGCAP, 2000, nullptr);
					wstring commonIniPath = m_edcbDir + L"\\Common.ini";
					m_epgCapBSBasic = GetPrivateProfileInt(L"SET", L"BSBasicOnly", 1, commonIniPath.c_str()) != 0;
					m_epgCapCS1Basic = GetPrivateProfileInt(L"SET", L"CS1BasicOnly", 1, commonIniPath.c_str()) != 0;
					m_epgCapCS2Basic = GetPrivateProfileInt(L"SET", L"CS2BasicOnly", 1, commonIniPath.c_str()) != 0;
					m_epgCapChkBS = false;
					m_epgCapChkCS1 = false;
					m_epgCapChkCS2 = false;
					m_epgCapChkNext = true;
					SendMessage(hwnd, WM_UPDATE_STATUS_CODE, 0, 0);
				}
			}
		}
		return 0;
	case WM_EPGCAP_BACK_START:
		{
			SendMessage(hwnd, WM_EPGCAP_BACK_STOP, 0, 0);
			if (IsTunerBonDriver() && m_epgCapChList.empty()) {
				m_epgCapBack = m_epgCapBackStartWaitSec != MAXDWORD;
				m_epgCapBackStartTick = GetTickCount();
				SetTimer(hwnd, TIMER_EPGCAP_BACK, 2000, nullptr);
			}
		}
		return 0;
	case WM_EPGCAP_STOP:
		m_epgCapChList.clear();
		SendMessage(hwnd, WM_UPDATE_STATUS_CODE, 0, 0);
		// FALL THROUGH!
	case WM_EPGCAP_BACK_STOP:
		m_epgCapBack = false;
		if (m_epgCapChList.empty() && m_epgFile != INVALID_HANDLE_VALUE) {
			// 保存キャンセル
			HANDLE epgFile = m_epgFile;
			{
				CBlockLock lock(&m_streamLock);
				m_epgFile = INVALID_HANDLE_VALUE;
			}
			CloseHandle(epgFile);
			DeleteFile((m_epgFilePath + L".tmp").c_str());
		}
		return 0;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int CALLBACK CEdcbPlugIn::CtrlCmdCallback(void *param, CMD_STREAM *cmdParam, CMD_STREAM *resParam)
{
	CEdcbPlugIn &this_ = *static_cast<CEdcbPlugIn*>(param);
	switch (cmdParam->param) {
	case CMD2_VIEW_APP_GET_BONDRIVER:
		{
			CBlockLock lock(&this_.m_statusLock);
			if (!this_.m_currentBonDriver.empty()) {
				resParam->data = NewWriteVALUE(this_.m_currentBonDriver, resParam->dataSize);
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_VIEW_APP_GET_DELAY:
		{
			CBlockLock lock(&this_.m_streamLock);
			resParam->data = NewWriteVALUE(this_.m_epgUtil.GetTimeDelay(), resParam->dataSize);
			resParam->param = CMD_SUCCESS;
		}
		break;
	case CMD2_VIEW_APP_GET_STATUS:
		{
			CBlockLock lock(&this_.m_statusLock);
			resParam->data = NewWriteVALUE(this_.m_statusCode, resParam->dataSize);
			resParam->param = CMD_SUCCESS;
		}
		break;
	case CMD2_VIEW_APP_CLOSE:
		SendNotifyMessage(this_.m_hwnd, WM_APP_ADD_LOG, 0, reinterpret_cast<LPARAM>(L"CMD2_VIEW_APP_CLOSE"));
		SendNotifyMessage(this_.m_hwnd, WM_APP_CLOSE, 0, 0);
		resParam->param = CMD_SUCCESS;
		break;
	case CMD2_VIEW_APP_SET_ID:
		SendNotifyMessage(this_.m_hwnd, WM_APP_ADD_LOG, 0, reinterpret_cast<LPARAM>(L"CMD2_VIEW_APP_SET_ID"));
		if (ReadVALUE(&this_.m_outCtrlID, cmdParam->data, cmdParam->dataSize, nullptr)) {
			resParam->param = CMD_SUCCESS;
		}
		break;
	case CMD2_VIEW_APP_GET_ID:
		resParam->data = NewWriteVALUE(this_.m_outCtrlID, resParam->dataSize);
		resParam->param = CMD_SUCCESS;
		break;
	case CMD2_VIEW_APP_SET_STANDBY_REC:
		SendNotifyMessage(this_.m_hwnd, WM_APP_ADD_LOG, 0, reinterpret_cast<LPARAM>(L"CMD2_VIEW_APP_SET_STANDBY_REC"));
		{
			DWORD val;
			if (ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, nullptr)) {
				// TODO: とりあえず無視
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_VIEW_APP_SET_CTRLMODE:
		SendNotifyMessage(this_.m_hwnd, WM_APP_ADD_LOG, 0, reinterpret_cast<LPARAM>(L"CMD2_VIEW_APP_SET_CTRLMODE"));
		{
			SET_CTRL_MODE val;
			if (ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, nullptr)) {
				CBlockLock lock(&this_.m_streamLock);
				if (this_.m_recCtrlMap.count(val.ctrlID) != 0) {
					this_.m_recCtrlMap[val.ctrlID].sid = val.SID;
				}
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_VIEW_APP_SEARCH_EVENT:
		{
			SEARCH_EPG_INFO_PARAM key;
			if (ReadVALUE(&key, cmdParam->data, cmdParam->dataSize, nullptr)) {
				CBlockLock lock(&this_.m_streamLock);
				EPG_EVENT_INFO *epgInfo;
				if (this_.m_epgUtil.SearchEpgInfo(key.ONID, key.TSID, key.SID, key.eventID, key.pfOnlyFlag, &epgInfo) == NO_ERR) {
					EPGDB_EVENT_INFO epgDBInfo;
					ConvertEpgInfo(key.ONID, key.TSID, key.SID, epgInfo, &epgDBInfo);
					resParam->data = NewWriteVALUE(epgDBInfo, resParam->dataSize);
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD2_VIEW_APP_GET_EVENT_PF:
		{
			GET_EPG_PF_INFO_PARAM key;
			if (ReadVALUE(&key, cmdParam->data, cmdParam->dataSize, nullptr)) {
				CBlockLock lock(&this_.m_streamLock);
				EPG_EVENT_INFO *epgInfo;
				if (this_.m_epgUtil.GetEpgInfo(key.ONID, key.TSID, key.SID, key.pfNextFlag, &epgInfo) == NO_ERR) {
					EPGDB_EVENT_INFO epgDBInfo;
					ConvertEpgInfo(key.ONID, key.TSID, key.SID, epgInfo, &epgDBInfo);
					resParam->data = NewWriteVALUE(epgDBInfo, resParam->dataSize);
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD2_VIEW_APP_EXEC_VIEW_APP:
		// 無視
		resParam->param = CMD_SUCCESS;
		break;
	default:
		// 同期呼び出しが必要なコマンド
		SendMessage(this_.m_hwnd, WM_INVOKE_CTRL_CMD, reinterpret_cast<WPARAM>(cmdParam), reinterpret_cast<LPARAM>(resParam));
		break;
	}
	return 0;
}

void CEdcbPlugIn::CtrlCmdCallbackInvoked(CMD_STREAM *cmdParam, CMD_STREAM *resParam)
{
	switch (cmdParam->param) {
	case CMD2_VIEW_APP_SET_BONDRIVER:
		m_pApp->AddLog(L"CMD2_VIEW_APP_SET_BONDRIVER");
		if (IsNotRecording()) {
			wstring val;
			if (ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, nullptr)) {
				m_pApp->SetDriverName(nullptr);
				if (m_pApp->SetDriverName(val.c_str())) {
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD2_VIEW_APP_SET_CH:
		m_pApp->AddLog(L"CMD2_VIEW_APP_SET_CH");
		if (IsNotRecording()) {
			SET_CH_INFO val;
			if (ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, nullptr)) {
				SendMessage(m_hwnd, WM_EPGCAP_STOP, 0, 0);
				if (val.useSID) {
					TVTest::ChannelSelectInfo si = {};
					si.Size = sizeof(si);
					si.Space = -1;
					si.Channel = -1;
					si.NetworkID = val.ONID;
					si.TransportStreamID = val.TSID;
					si.ServiceID = val.SID;
					if (m_pApp->SelectChannel(&si)) {
						m_lastSetCh = val;
						m_chChangedAfterSetCh = false;
						resParam->param = CMD_SUCCESS;
					}
				}
				// チューナ番号指定には未対応
			}
		}
		break;
	case CMD2_VIEW_APP_CREATE_CTRL:
		m_pApp->AddLog(L"CMD2_VIEW_APP_CREATE_CTRL");
		resParam->data = NewWriteVALUE(++m_recCtrlCount, resParam->dataSize);
		resParam->param = CMD_SUCCESS;
		// TVTestはチャンネルをロックできないので、CMD2_VIEW_APP_SET_CH後にユーザによる変更があれば戻しておく
		if (m_lastSetCh.useSID && m_chChangedAfterSetCh && IsNotRecording()) {
			m_pApp->AddLog(L"SetCh", TVTest::LOG_TYPE_WARNING);
			TVTest::ChannelSelectInfo si = {};
			si.Size = sizeof(si);
			si.Space = -1;
			si.Channel = -1;
			si.NetworkID = m_lastSetCh.ONID;
			si.TransportStreamID = m_lastSetCh.TSID;
			si.ServiceID = m_lastSetCh.SID;
			if (m_pApp->SelectChannel(&si)) {
				m_chChangedAfterSetCh = false;
			}
		}
		{
			CBlockLock lock(&m_streamLock);
			m_recCtrlMap[m_recCtrlCount] = REC_CTRL();
			m_recCtrlMap[m_recCtrlCount].sid = 0xFFFF;
			m_recCtrlMap[m_recCtrlCount].dropCount.SetNoLog(FALSE, this->m_noLogScramble);
		}
		break;
	case CMD2_VIEW_APP_DELETE_CTRL:
		m_pApp->AddLog(L"CMD2_VIEW_APP_DELETE_CTRL");
		{
			DWORD val;
			if (ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, nullptr) && m_recCtrlMap.count(val) != 0) {
				REC_CTRL recCtrl;
				{
					CBlockLock lock(&m_streamLock);
					recCtrl = std::move(m_recCtrlMap[val]);
					m_recCtrlMap.erase(val);
				}
				if (!recCtrl.filePath.empty()) {
					if (IsEdcbRecording()) {
						// 重複録画
						DuplicateSave(m_duplicateOriginalPath.c_str(), &recCtrl.duplicateTargetID, nullptr);
					}
					else {
						// 最後の録画
						m_pApp->StopRecord();
					}
				}
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_VIEW_APP_REC_START_CTRL:
		m_pApp->AddLog(L"CMD2_VIEW_APP_REC_START_CTRL");
		{
			SET_CTRL_REC_PARAM val;
			if (ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, nullptr) && m_recCtrlMap.count(val.ctrlID) != 0) {
				// overWriteFlag,pittariFlag,createSizeは無視
				REC_CTRL &recCtrl = m_recCtrlMap[val.ctrlID];
				if (recCtrl.filePath.empty() && !val.saveFolder.empty()) {
					// saveFolderは最初の要素のみ使う
					wstring filePath = val.saveFolder[0].recFolder;
					ChkFolderPath(filePath);
					filePath += L'\\' + val.saveFolder[0].recFileName;
					if (!m_recNamePrefix.empty()) {
						// 対象サービスIDをファイル名に前置する
						wstring prefix = m_recNamePrefix;
						wstring macro;
						Format(macro, L"%d", recCtrl.sid);
						Replace(prefix, L"$SID10$", macro);
						Format(macro, L"%04X", recCtrl.sid);
						Replace(prefix, L"$SID16$", macro);
						wstring name;
						GetFileName(filePath, name);
						GetFileFolder(filePath, filePath);
						filePath += L'\\' + prefix + name;
					}
					if (IsEdcbRecording()) {
						// 重複録画
						wstring dir;
						GetFileFolder(filePath, dir);
						_CreateDirectory(dir.c_str());
						if (DuplicateSave(m_duplicateOriginalPath.c_str(), &recCtrl.duplicateTargetID, &filePath)) {
							SendMessage(m_hwnd, WM_EPGCAP_BACK_START, 0, 0);
							CBlockLock lock(&m_streamLock);
							recCtrl.filePath = filePath;
							resParam->param = CMD_SUCCESS;
						}
						else {
							m_pApp->AddLog(L"重複録画開始に失敗しました。", TVTest::LOG_TYPE_ERROR);
						}
					}
					else {
						// 最初の録画
						TVTest::RecordInfo ri;
						ri.Mask = TVTest::RECORD_MASK_FILENAME;
						// 置換キーワードを展開させないため
						Replace(filePath, L"%", L"%%");
						vector<WCHAR> buf(filePath.c_str(), filePath.c_str() + filePath.size() + 1);
						ri.pszFileName = &buf.front();
						m_pApp->StopRecord();
						if (m_pApp->StartRecord(&ri)) {
							TVTest::RecordStatusInfo rsi;
							WCHAR szFilePath[MAX_PATH];
							rsi.pszFileName = szFilePath;
							rsi.MaxFileName = _countof(szFilePath);
							if (m_pApp->GetRecordStatus(&rsi) && szFilePath[0]) {
								SendMessage(m_hwnd, WM_EPGCAP_BACK_START, 0, 0);
								CBlockLock lock(&m_streamLock);
								recCtrl.filePath = m_duplicateOriginalPath = szFilePath;
								recCtrl.duplicateTargetID = 1;
								SendMessage(m_hwnd, WM_SIGNAL_UPDATE_START, 0, 0);
								resParam->param = CMD_SUCCESS;
							}
							else {
								m_pApp->StopRecord();
							}
						}
					}
				}
			}
		}
		break;
	case CMD2_VIEW_APP_REC_STOP_CTRL:
		m_pApp->AddLog(L"CMD2_VIEW_APP_REC_STOP_CTRL");
		{
			SET_CTRL_REC_STOP_PARAM val;
			if (ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, nullptr) && m_recCtrlMap.count(val.ctrlID) != 0) {
				REC_CTRL recCtrl;
				{
					CBlockLock lock(&m_streamLock);
					std::swap(m_recCtrlMap[val.ctrlID].filePath, recCtrl.filePath);
					std::swap(m_recCtrlMap[val.ctrlID].dropCount, recCtrl.dropCount);
					recCtrl.duplicateTargetID = m_recCtrlMap[val.ctrlID].duplicateTargetID;
				}
				if (!recCtrl.filePath.empty()) {
					if (val.saveErrLog) {
						wstring infoPath = GetPrivateProfileToString(L"SET", L"RecInfoFolder", L"", (m_edcbDir + L"\\Common.ini").c_str());
						ChkFolderPath(infoPath);
						if (infoPath.empty()) {
							infoPath = recCtrl.filePath + L".err";
						}
						else {
							wstring name;
							GetFileName(recCtrl.filePath, name);
							infoPath += L'\\' + name + L".err";
						}
						map<WORD, string> pidNameMap;
						TVTest::ServiceInfo si;
						for (int i = 0; m_pApp->GetServiceInfo(i, &si); ++i) {
							string name;
							Format(name, "0x%04X-Video", si.ServiceID);
							pidNameMap.insert(std::make_pair(si.VideoPID, name));
							for (int j = 0; j < si.NumAudioPIDs; ++j) {
								Format(name, "0x%04X-Audio(0x%02X)", si.ServiceID, si.AudioComponentType[j]);
								pidNameMap.insert(std::make_pair(si.AudioPID[j], name));
							}
							Format(name, "0x%04X-Subtitle", si.ServiceID);
							pidNameMap.insert(std::make_pair(si.SubtitlePID, name));
						}
						recCtrl.dropCount.SetPIDName(&pidNameMap);
						recCtrl.dropCount.SetBonDriver(m_currentBonDriver);
						recCtrl.dropCount.SaveLog(infoPath);
					}
					SET_CTRL_REC_STOP_RES_PARAM resVal;
					resVal.recFilePath = recCtrl.filePath;
					resVal.drop = recCtrl.dropCount.GetDropCount();
					resVal.scramble = recCtrl.dropCount.GetScrambleCount();
					resVal.subRecFlag = 0;
					if (IsEdcbRecording()) {
						// 重複録画
						DuplicateSave(m_duplicateOriginalPath.c_str(), &recCtrl.duplicateTargetID, nullptr);
					}
					else {
						// 最後の録画
						m_pApp->StopRecord();
					}
					resParam->data = NewWriteVALUE(resVal, resParam->dataSize);
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD2_VIEW_APP_REC_FILE_PATH:
		{
			DWORD val;
			if (ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, nullptr) && m_recCtrlMap.count(val) != 0) {
				if (!m_recCtrlMap[val].filePath.empty()) {
					resParam->data = NewWriteVALUE(m_recCtrlMap[val].filePath, resParam->dataSize);
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD2_VIEW_APP_EPGCAP_START:
		m_pApp->AddLog(L"CMD2_VIEW_APP_EPGCAP_START");
		{
			vector<SET_CH_INFO> chList;
			if (m_epgCapChList.empty() && ReadVALUE(&chList, cmdParam->data, cmdParam->dataSize, nullptr)) {
				SendMessage(m_hwnd, WM_EPGCAP_START, 0, reinterpret_cast<LPARAM>(&chList));
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_VIEW_APP_EPGCAP_STOP:
		m_pApp->AddLog(L"CMD2_VIEW_APP_EPGCAP_STOP");
		SendMessage(m_hwnd, WM_EPGCAP_STOP, 0, 0);
		resParam->param = CMD_SUCCESS;
		break;
	case CMD2_VIEW_APP_REC_STOP_ALL:
		m_pApp->AddLog(L"CMD2_VIEW_APP_REC_STOP_ALL");
		{
			if (IsEdcbRecording()) {
				m_pApp->StopRecord();
			}
			CBlockLock lock(&m_streamLock);
			m_recCtrlMap.clear();
			resParam->param = CMD_SUCCESS;
		}
		break;
	case CMD2_VIEW_APP_REC_WRITE_SIZE:
		{
			DWORD val;
			if (ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, nullptr)) {
				__int64 writeSize = -1;
				if (m_recCtrlMap.count(val) != 0 && !m_recCtrlMap[val].filePath.empty()) {
					WIN32_FIND_DATA findData;
					HANDLE hFind = FindFirstFile(m_recCtrlMap[val].filePath.c_str(), &findData);
					if (hFind != INVALID_HANDLE_VALUE) {
						FindClose(hFind);
						writeSize = static_cast<__int64>(findData.nFileSizeHigh) << 32 | findData.nFileSizeLow;
					}
				}
				resParam->data = NewWriteVALUE(writeSize, resParam->dataSize);
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	default:
		resParam->param = CMD_NON_SUPPORT;
		break;
	}
}

wstring CEdcbPlugIn::GetEdcbSettingPath() const
{
	wstring ret = GetPrivateProfileToString(L"SET", L"DataSavePath", L"", (m_edcbDir + L"\\Common.ini").c_str());
	ChkFolderPath(ret);
	if (ret.empty()) {
		ret = m_edcbDir + L"\\Setting";
	}
	return ret;
}

bool CEdcbPlugIn::IsNotRecording() const
{
	TVTest::RecordStatusInfo info;
	return m_pApp->GetRecordStatus(&info) && info.Status == TVTest::RECORD_STATUS_NOTRECORDING;
}

bool CEdcbPlugIn::IsEdcbRecording() const
{
	return std::find_if(m_recCtrlMap.begin(), m_recCtrlMap.end(),
		[](const pair<DWORD, REC_CTRL> &a) { return !a.second.filePath.empty(); }) != m_recCtrlMap.end();
}

bool CEdcbPlugIn:: IsTunerBonDriver() const
{
	wstring driver = L':' + m_currentBonDriver + L':';
	return std::search(m_nonTunerDrivers.begin(), m_nonTunerDrivers.end(), driver.begin(), driver.end(),
		[](wchar_t a, wchar_t b) { return towupper(a) == towupper(b); }) == m_nonTunerDrivers.end();
}

UINT WINAPI CEdcbPlugIn::ReloadEpgThread(void *param)
{
	CSendCtrlCmd cmd;
	cmd.SetConnectTimeOut(4000);
	cmd.SendReloadEpg();
	return 0;
}

BOOL CALLBACK CEdcbPlugIn::StreamCallback(BYTE *pData, void *pClientData)
{
	CTSPacketUtil packet;
	if (packet.Set188TS(pData, 188)) {
		CEdcbPlugIn &this_ = *static_cast<CEdcbPlugIn*>(pClientData);
		CBlockLock lock(&this_.m_streamLock);
		if (packet.PID <= 0x30) {
			if (this_.m_chChangeID > CH_CHANGE_ERR) {
				// チャンネル切り替え中
				// 1秒間は切り替え前のパケット来る可能性を考慮して無視する
				if (GetTickCount() - this_.m_chChangeTick > 1000) {
					this_.m_epgUtil.AddTSPacket(pData, 188);
					WORD onid;
					WORD tsid;
					if (this_.m_epgUtil.GetTSID(&onid, &tsid) == NO_ERR && onid == HIWORD(this_.m_chChangeID) && tsid == LOWORD(this_.m_chChangeID)) {
						this_.m_chChangeID = CH_CHANGE_OK;
					}
					else if (GetTickCount() - this_.m_chChangeTick > 15000) {
						// 15秒以上たってるなら切り替えエラー
						this_.m_chChangeID = CH_CHANGE_ERR;
						SendNotifyMessage(this_.m_hwnd, WM_UPDATE_STATUS_CODE, 0, 0);
					}
				}
			}
			else {
				if (this_.m_epgFile != INVALID_HANDLE_VALUE) {
					DWORD written;
					if (packet.PID == 0 && packet.payload_unit_start_indicator) {
						if (this_.m_epgFileState == EPG_FILE_ST_NONE) {
							this_.m_epgFileState = EPG_FILE_ST_PAT;
						}
						else if (this_.m_epgFileState == EPG_FILE_ST_PAT) {
							this_.m_epgFileState = EPG_FILE_ST_TOT;
							// 番組情報が不足しないよう改めて蓄積状態をリセット
							this_.m_epgUtil.ClearSectionStatus();
							// TOTを前倒しで書き込むための場所を確保
							BYTE nullData[188] = { 0x47, 0x1F, 0xFF, 0x10 };
							memset(nullData + 4, 0xFF, 184);
							this_.m_epgFileTotPos = SetFilePointer(this_.m_epgFile, 0, nullptr, FILE_CURRENT);
							WriteFile(this_.m_epgFile, nullData, 188, &written, nullptr);
						}
					}
					// まずPAT、次に(あれば)TOTを書き込む。この処理は必須ではないが番組情報をより確実かつ効率的に読み出せる
					if (packet.PID == 0x14 && this_.m_epgFileState == EPG_FILE_ST_TOT) {
						this_.m_epgFileState = EPG_FILE_ST_ALL;
						if (this_.m_epgFileTotPos != INVALID_SET_FILE_POINTER) {
							SetFilePointer(this_.m_epgFile, this_.m_epgFileTotPos, nullptr, FILE_BEGIN);
						}
						WriteFile(this_.m_epgFile, pData, 188, &written, nullptr);
						LONG posHigh = 0;
						SetFilePointer(this_.m_epgFile, 0, &posHigh, FILE_END);
					}
					else if (packet.PID == 0 && this_.m_epgFileState >= EPG_FILE_ST_PAT || this_.m_epgFileState >= EPG_FILE_ST_TOT) {
						WriteFile(this_.m_epgFile, pData, 188, &written, nullptr);
					}
				}
				this_.m_epgUtil.AddTSPacket(pData, 188);
			}
		}
		for (map<DWORD, REC_CTRL>::iterator it = this_.m_recCtrlMap.begin(); it != this_.m_recCtrlMap.end(); ++it) {
			if (!it->second.filePath.empty()) {
				it->second.dropCount.AddData(pData, 188);
			}
		}
	}
	return TRUE;
}

TVTest::CTVTestPlugin *CreatePluginClass()
{
	return new CEdcbPlugIn;
}
