// EdcbPlugIn.cpp : DLL アプリケーション用にエクスポートされる関数を定義します。
//

#include "stdafx.h"
#include "EdcbPlugIn.h"
#include "../../Common/StringUtil.h"
#include "../../Common/TimeUtil.h"
#include "../../Common/CommonDef.h"
#include "../../Common/EpgTimerUtil.h"
#include "../../Common/IniUtil.h"
#include "../../Common/SendCtrlCmd.h"
#include "../../Common/TSPacketUtil.h"
#include "../../Common/ParseTextInstances.h"

namespace
{

BOOL DuplicateSave(LPCWSTR originalPath, DWORD *targetID, wstring *targetPath)
{
	vector<WCHAR> buf;
	if (targetPath) {
		buf.assign(targetPath->begin(), targetPath->end());
		buf.resize(buf.size() + 16, L'\0');
	}
	BOOL ret = FALSE;
	std::unique_ptr<void, decltype(&UtilFreeLibrary)> module(UtilLoadLibrary(GetModulePath().replace_filename(L"Write_Multi.dll")), UtilFreeLibrary);
	if (module) {
		BOOL (WINAPI* pfnDuplicateSave)(LPCWSTR, DWORD*, WCHAR*, DWORD, int, ULONGLONG);
		if (UtilGetProcAddress(module.get(), "DuplicateSave", pfnDuplicateSave)) {
			ret = pfnDuplicateSave(originalPath, targetID, targetPath ? &buf.front() : nullptr, static_cast<DWORD>(buf.size()), -1, 0);
		}
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
		lock_recursive_mutex lock(m_outer.m_streamLock);
		// EpgDataCap3は内部メソッド単位でアトミック。UnInitialize()中にワーカースレッドにアクセスさせないよう排他制御が必要
		m_outer.m_epgUtil.UnInitialize();
		m_outer.m_epgUtil.Initialize(FALSE, m_outer.m_epgUtilPath.c_str());
		m_outer.m_logoAdditionalNeededPids = nullptr;
		m_outer.m_chChangeID = CH_CHANGE_ERR;
		if (ret) {
			m_outer.m_chChangeID = static_cast<DWORD>(ci.NetworkID) << 16 | ci.TransportStreamID;
			m_outer.m_chChangeTick = GetU32Tick();
		}
		lock_recursive_mutex lock2(m_outer.m_statusLock);
		m_outer.m_statusInfo.originalNetworkID = -1;
		m_outer.m_statusInfo.transportStreamID = -1;
	}
	m_outer.m_chChangedAfterSetCh = true;
	SendMessage(m_outer.m_hwnd, WM_UPDATE_STATUS_CODE, 0, 0);
	SendMessage(m_outer.m_hwnd, WM_EPGCAP_BACK_START, 0, 0);
	OnServiceChange();
	return false;
}

bool CEdcbPlugIn::CMyEventHandler::OnServiceChange()
{
#ifdef SEND_PIPE_TEST
	int index = m_outer.m_pApp->GetService();
	TVTest::ServiceInfo si;
	if (index >= 0 && m_outer.m_pApp->GetServiceInfo(index, &si)) {
		lock_recursive_mutex lock(m_outer.m_streamLock);
		m_outer.m_serviceFilter.SetServiceID(false, vector<WORD>(1, si.ServiceID));
	}
#endif
	return false;
}

bool CEdcbPlugIn::CMyEventHandler::OnServiceUpdate()
{
	OnServiceChange();
	return false;
}

bool CEdcbPlugIn::CMyEventHandler::OnDriverChange()
{
	WCHAR name[MAX_PATH];
	if (m_outer.m_pApp->GetDriverName(name, array_size(name)) <= 0) {
		name[0] = L'\0';
	}
	{
		lock_recursive_mutex lock(m_outer.m_statusLock);
		m_outer.m_statusInfo.bonDriver = name;
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
			lock_recursive_mutex lock(m_outer.m_streamLock);
			for (map<DWORD, REC_CTRL>::iterator it = m_outer.m_recCtrlMap.begin(); it != m_outer.m_recCtrlMap.end(); ++it) {
				if (!it->second.filePath.empty()) {
					wstring().swap(it->second.filePath);
					CDropCount sw;
					std::swap(sw, it->second.dropCount);
				}
			}
		}
	}
	SendMessage(m_outer.m_hwnd, WM_UPDATE_STATUS_CODE, 0, 0);
	return false;
}

void CEdcbPlugIn::CMyEventHandler::OnStartupDone()
{
	if (GetModuleHandle(GetModulePath(g_hinstDLL).replace_filename(L"EpgTimerPlugIn.tvtp").c_str())) {
		// 1プロセスに複数サーバは想定外なので開始しない(この判定方法は確実ではない)
		m_outer.m_pApp->AddLog(L"EpgTimerPlugInが読み込まれているため正常動作しません。", TVTest::LOG_TYPE_ERROR);
	}
	else {
		if (CPipeServer::GrantServerAccessToKernelObject(GetCurrentProcess(), SYNCHRONIZE | PROCESS_TERMINATE | PROCESS_SET_INFORMATION)) {
			m_outer.m_pApp->AddLog(L"Granted SYNCHRONIZE|PROCESS_TERMINATE|PROCESS_SET_INFORMATION to " SERVICE_NAME);
		}
		wstring pipeName;
		Format(pipeName, L"%ls%d", CMD2_VIEW_CTRL_PIPE, GetCurrentProcessId());
		CEdcbPlugIn *outer = &m_outer;
		m_outer.m_pipeServer.StartServer(pipeName,
		                                 [outer](CCmdStream &cmd, CCmdStream &res) { outer->CtrlCmdCallback(cmd, res); });
	}
}

CEdcbPlugIn::CEdcbPlugIn()
	: m_handler(*this)
	, m_hwnd(nullptr)
	, m_chChangeID(CH_CHANGE_OK)
	, m_epgCapBack(false)
	, m_recCtrlCount(0)
	, m_logoAdditionalNeededPids(nullptr)
	, m_logoTick(0)
	, m_logoTypeFlags(0)
#ifdef SEND_PIPE_TEST
	, m_sendPipeMutex(UtilCreateGlobalMutex())
#endif
{
	VIEW_APP_STATUS_INFO info = {};
	info.status = VIEW_APP_ST_ERR_BON;
	info.originalNetworkID = -1;
	info.transportStreamID = -1;
	info.appID = -1;
	m_statusInfo = info;
	m_lastSetCh.useSID = FALSE;
	std::fill_n(m_epgCapBasicOnlyONIDs, array_size(m_epgCapBasicOnlyONIDs), false);
	std::fill_n(m_epgCapBackBasicOnlyONIDs, array_size(m_epgCapBackBasicOnlyONIDs), false);
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
	fs_path iniPath = GetModulePath(g_hinstDLL).replace_extension(L".ini");
	m_edcbDir = GetPrivateProfileToString(L"SET", L"EdcbFolderPath", L"", iniPath.c_str());
	// 未指定のときはTVTestと同階層のEDCBフォルダ
	if (m_edcbDir.empty()) {
		fs_path altPath = GetModulePath().parent_path().parent_path();
		if (altPath.is_absolute()) {
			m_edcbDir = altPath.append(L"EDCB").native();
		}
	}
	m_epgUtilPath = GetModulePath().replace_filename(L"EpgDataCap3.dll").native();
	if (!m_edcbDir.empty() && m_epgUtil.Initialize(FALSE, m_epgUtilPath.c_str()) != NO_ERR) {
		m_epgUtilPath = fs_path(m_edcbDir).append(L"EpgDataCap3.dll").native();
		if (m_epgUtil.Initialize(FALSE, m_epgUtilPath.c_str()) != NO_ERR) {
			m_epgUtilPath.clear();
		}
	}
	if (m_edcbDir.empty() || m_epgUtilPath.empty()) {
		m_pApp->AddLog(L"EpgDataCap3.dllが見つかりません。", TVTest::LOG_TYPE_ERROR);
		return false;
	}
	CParseChText5 chText5;
	if (!chText5.ParseText(GetEdcbSettingPath().append(L"ChSet5.txt").c_str())) {
		m_pApp->AddLog(L"ChSet5.txtが見つかりません。", TVTest::LOG_TYPE_ERROR);
		return false;
	}
	m_chSet5.clear();
	for (map<LONGLONG, CH_DATA5>::const_iterator it = chText5.GetMap().begin(); it != chText5.GetMap().end(); ++it) {
		m_chSet5.push_back(it->second);
	}
#ifdef SEND_PIPE_TEST
	if (GetPrivateProfileInt(L"SET", L"SendPipeTest", 0, iniPath.c_str()) && m_sendPipe.Initialize()) {
		int port = 0;
		for (; port < BON_NW_PORT_RANGE; ++port) {
			wstring name;
			Format(name, L"%ls1_%d", MUTEX_TCP_PORT_NAME, port);
			m_sendPipeMutex = UtilCreateGlobalMutex(name.c_str());
			if (m_sendPipeMutex) {
				m_pApp->AddLog(name.c_str());
				break;
			}
		}
		m_sendPipe.AddSendAddr(L"0.0.0.1", port);
		m_sendPipe.StartSend();
	}
#endif
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
#ifdef SEND_PIPE_TEST
	if (m_sendPipe.IsInitialized()) {
		m_sendPipe.StopSend();
		m_sendPipe.UnInitialize();
		m_sendPipeMutex.reset();
	}
#endif
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
			fs_path bonCtrlIniPath = fs_path(m_edcbDir).append(L"BonCtrl.ini");
			m_epgCapTimeout = GetPrivateProfileInt(L"EPGCAP", L"EpgCapTimeOut", 10, bonCtrlIniPath.c_str());
			m_epgCapSaveTimeout = GetPrivateProfileInt(L"EPGCAP", L"EpgCapSaveTimeOut", 0, bonCtrlIniPath.c_str()) != 0;

			fs_path iniPath = GetModulePath(g_hinstDLL).replace_extension(L".ini");
			vector<WCHAR> bufSet = GetPrivateProfileSectionBuffer(L"SET", iniPath.c_str());
			m_nonTunerDrivers = L"::" + GetBufferedProfileToString(bufSet.data(), L"NonTunerDrivers",
				L"BonDriver_UDP.dll:BonDriver_TCP.dll:BonDriver_File.dll:BonDriver_RecTask.dll:BonDriver_TsTask.dll:"
				L"BonDriver_NetworkPipe.dll:BonDriver_Pipe.dll:BonDriver_Pipe2.dll") + L':';
			m_recNamePrefix = GetBufferedProfileToString(bufSet.data(), L"RecNamePrefix", L"");
			m_dropSaveThresh = GetBufferedProfileInt(bufSet.data(), L"DropSaveThresh", 0);
			m_scrambleSaveThresh = GetBufferedProfileInt(bufSet.data(), L"ScrambleSaveThresh", -1);
			m_noLogScramble = GetBufferedProfileInt(bufSet.data(), L"NoLogScramble", 0) != 0;
			m_dropLogAsUtf8 = GetBufferedProfileInt(bufSet.data(), L"DropLogAsUtf8", 0) != 0;
			m_epgCapBackStartWaitSec = GetBufferedProfileInt(bufSet.data(), L"EpgCapLive", 1) == 0 ? MAXDWORD :
				GetBufferedProfileInt(bufSet.data(), L"EpgCapBackStartWaitSec", 30);
			m_epgCapBackBasicOnlyONIDs[4] = GetBufferedProfileInt(bufSet.data(), L"EpgCapBackBSBasicOnly", 1) != 0;
			m_epgCapBackBasicOnlyONIDs[6] = GetBufferedProfileInt(bufSet.data(), L"EpgCapBackCS1BasicOnly", 1) != 0;
			m_epgCapBackBasicOnlyONIDs[7] = GetBufferedProfileInt(bufSet.data(), L"EpgCapBackCS2BasicOnly", 1) != 0;
			m_epgCapBackBasicOnlyONIDs[10] = GetBufferedProfileInt(bufSet.data(), L"EpgCapBackCS3BasicOnly", 0) != 0;
			m_logoTypeFlags = GetBufferedProfileInt(bufSet.data(), L"SaveLogo", 0) == 0 ? 0 :
				GetBufferedProfileInt(bufSet.data(), L"SaveLogoTypeFlags", 32);
		}
		return 0;
	case WM_DESTROY:
		m_pipeServer.StopServer();
		SendMessage(hwnd, WM_EPGCAP_STOP, 0, 0);
		m_pApp->SetStreamCallback(TVTest::STREAM_CALLBACK_REMOVE, StreamCallback);
		if (m_epgReloadThread.joinable()) {
			if (WaitForSingleObject(m_epgReloadThread.native_handle(), 5000) == WAIT_TIMEOUT) {
				TerminateThread(m_epgReloadThread.native_handle(), 0xFFFFFFFF);
			}
			m_epgReloadThread.join();
		}
		m_hwnd = nullptr;
		return 0;
	case WM_CLOSE:
		// デッドロック回避のためメッセージポンプを維持しつつサーバを終わらせる
		m_pipeServer.StopServer(true);
		SetTimer(hwnd, TIMER_TRY_STOP_SERVER, 10, nullptr);
		return 0;
	case WM_TIMER:
		switch (wParam) {
		case TIMER_TRY_STOP_SERVER:
			if (m_pipeServer.StopServer(true)) {
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
				ULONGLONG drop = 0;
				ULONGLONG scramble = 0;
				lock_recursive_mutex lock(m_streamLock);
				for (map<DWORD, REC_CTRL>::iterator it = m_recCtrlMap.begin(); it != m_recCtrlMap.end(); ++it) {
					if (!it->second.filePath.empty()) {
						it->second.dropCount.SetSignal(si.SignalLevel);
						drop = it->second.dropCount.GetDropCount();
						scramble = it->second.dropCount.GetScrambleCount();
					}
				}
				lock_recursive_mutex lock2(m_statusLock);
				m_statusInfo.drop = drop;
				m_statusInfo.scramble = scramble;
				m_statusInfo.signalLv = si.SignalLevel;
			}
			else {
				// 録画停止中は統計情報を取得しない
				KillTimer(hwnd, TIMER_SIGNAL_UPDATE);
				lock_recursive_mutex lock(m_statusLock);
				m_statusInfo.drop = 0;
				m_statusInfo.scramble = 0;
				m_statusInfo.signalLv = 0;
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
						if (!m_epgCapChkONIDs[min<size_t>(chInfo.ONID, array_size(m_epgCapChkONIDs) - 1)]) {
							TVTest::ChannelSelectInfo si = {};
							si.Size = sizeof(si);
							si.Space = -1;
							si.Channel = -1;
							si.NetworkID = chInfo.ONID;
							si.TransportStreamID = chInfo.TSID;
							if (m_pApp->SelectChannel(&si)) {
								m_epgCapStartTick = GetU32Tick();
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
						lock_recursive_mutex lock(m_streamLock);
						chChangeID = m_chChangeID;
					}
					bool saveEpgFile = false;
					if (chChangeID != CH_CHANGE_OK) {
						if (chChangeID == CH_CHANGE_ERR || GetU32Tick() - m_epgCapStartTick > 15000) {
							// チャンネル切り替えエラーか切り替えに15秒以上かかってるので無信号と判断
							m_epgCapChList.erase(m_epgCapChList.begin());
							m_epgCapChkNext = true;
						}
					}
					else if (GetU32Tick() - m_epgCapStartTick > m_epgCapTimeout * 60000) {
						// m_epgCapTimeout分以上かかっているなら停止
						m_epgCapChList.erase(m_epgCapChList.begin());
						m_epgCapChkNext = true;
						saveEpgFile = m_epgCapSaveTimeout;
					}
					else {
						SET_CH_INFO &chInfo = m_epgCapChList.front();
						bool basicFlag = m_epgCapBasicOnlyONIDs[min<size_t>(chInfo.ONID, array_size(m_epgCapBasicOnlyONIDs) - 1)];
						vector<CH_DATA5> chkList = GetEpgCheckList(chInfo.ONID, chInfo.TSID, chInfo.SID, basicFlag);
						if (chkList.empty()) {
							m_epgCapChList.erase(m_epgCapChList.begin());
							m_epgCapChkNext = true;
						}
						else if (!m_epgFile) {
							// 保存開始
							wstring name;
							Format(name, L"%04X%04X_epg.dat", chInfo.ONID, basicFlag ? 0xFFFF : chInfo.TSID);
							m_epgFilePath = GetEdcbSettingPath().append(EPG_SAVE_FOLDER).append(name).native();
							UtilCreateDirectories(fs_path(m_epgFilePath).parent_path());
							FILE* epgFile = UtilOpenFile(m_epgFilePath + L".tmp", UTIL_SECURE_WRITE);
							if (epgFile) {
								m_pApp->AddLog((L'★' + name).c_str());
								lock_recursive_mutex lock(m_streamLock);
								m_epgFile.reset(epgFile);
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
								m_epgCapChkONIDs[min<size_t>(chInfo.ONID, array_size(m_epgCapChkONIDs) - 1)] = basicFlag;
								m_epgCapChList.erase(m_epgCapChList.begin());
								saveEpgFile = true;
							}
						}
					}
					if (m_epgCapChkNext && m_epgFile) {
						// 保存終了
						{
							std::unique_ptr<FILE, fclose_deleter> epgFile;
							lock_recursive_mutex lock(m_streamLock);
							epgFile.swap(m_epgFile);
						}
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
				if (GetU32Tick() - m_epgCapBackStartTick > max<DWORD>(m_epgCapBackStartWaitSec, 15) * 1000) {
					WORD onid;
					WORD tsid;
					if (m_chChangeID != CH_CHANGE_OK || m_epgUtil.GetTSID(&onid, &tsid) != NO_ERR) {
						m_epgCapBack = false;
					}
					else if (GetU32Tick() - m_epgCapBackStartTick > m_epgCapTimeout * 60000 + max<DWORD>(m_epgCapBackStartWaitSec, 15) * 1000) {
						// m_epgCapTimeout分以上かかっているなら停止
						m_epgCapBack = false;
						saveEpgFile = m_epgCapSaveTimeout;
					}
					else {
						bool basicFlag = m_epgCapBackBasicOnlyONIDs[min<size_t>(onid, array_size(m_epgCapBackBasicOnlyONIDs) - 1)];
						vector<CH_DATA5> chkList = GetEpgCheckList(onid, tsid, -1, basicFlag);
						if (chkList.empty()) {
							m_epgCapBack = false;
						}
						else if (!m_epgFile) {
							// 保存開始
							wstring name;
							Format(name, L"%04X%04X_epg.dat", onid, basicFlag ? 0xFFFF : tsid);
							m_epgFilePath = GetEdcbSettingPath().append(EPG_SAVE_FOLDER).append(name).native();
							UtilCreateDirectories(fs_path(m_epgFilePath).parent_path());
							FILE* epgFile = UtilOpenFile(m_epgFilePath + L".tmp", UTIL_SECURE_WRITE);
							if (epgFile) {
								m_pApp->AddLog((L'★' + name).c_str());
								lock_recursive_mutex lock(m_streamLock);
								m_epgFile.reset(epgFile);
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
				if (!m_epgCapBack && m_epgFile) {
					// 保存終了
					{
						std::unique_ptr<FILE, fclose_deleter> epgFile;
						lock_recursive_mutex lock(m_streamLock);
						epgFile.swap(m_epgFile);
					}
					if (saveEpgFile) {
						CopyFile((m_epgFilePath + L".tmp").c_str(), m_epgFilePath.c_str(), FALSE);
					}
					DeleteFile((m_epgFilePath + L".tmp").c_str());
					if (saveEpgFile) {
						if (m_epgReloadThread.joinable() && WaitForSingleObject(m_epgReloadThread.native_handle(), 0) != WAIT_TIMEOUT) {
							m_epgReloadThread.join();
						}
						if (!m_epgReloadThread.joinable()) {
							m_epgReloadThread = thread_(ReloadEpgThread);
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
		CtrlCmdCallbackInvoked(*reinterpret_cast<const CCmdStream*>(wParam), *reinterpret_cast<CCmdStream*>(lParam));
		return 0;
	case WM_APP_CLOSE:
		m_pApp->Close(TVTest::CLOSE_EXIT);
		return 0;
	case WM_APP_ADD_LOG:
		m_pApp->AddLog(reinterpret_cast<LPCWSTR>(lParam));
		return 0;
	case WM_UPDATE_STATUS_CODE:
		{
			DWORD status = m_statusInfo.bonDriver.empty() ? VIEW_APP_ST_ERR_BON :
			               !IsNotRecording() ? VIEW_APP_ST_REC :
			               !m_epgCapChList.empty() ? VIEW_APP_ST_GET_EPG :
			               m_chChangeID == CH_CHANGE_ERR ? VIEW_APP_ST_ERR_CH_CHG : VIEW_APP_ST_NORMAL;
			lock_recursive_mutex lock(m_statusLock);
			m_statusInfo.status = status;
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
					fs_path commonIniPath = fs_path(m_edcbDir).append(L"Common.ini");
					m_epgCapBasicOnlyONIDs[4] = GetPrivateProfileInt(L"SET", L"BSBasicOnly", 1, commonIniPath.c_str()) != 0;
					m_epgCapBasicOnlyONIDs[6] = GetPrivateProfileInt(L"SET", L"CS1BasicOnly", 1, commonIniPath.c_str()) != 0;
					m_epgCapBasicOnlyONIDs[7] = GetPrivateProfileInt(L"SET", L"CS2BasicOnly", 1, commonIniPath.c_str()) != 0;
					m_epgCapBasicOnlyONIDs[10] = GetPrivateProfileInt(L"SET", L"CS3BasicOnly", 0, commonIniPath.c_str()) != 0;
					std::fill_n(m_epgCapChkONIDs, array_size(m_epgCapChkONIDs), false);
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
				m_epgCapBackStartTick = GetU32Tick();
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
		if (m_epgCapChList.empty() && m_epgFile) {
			// 保存キャンセル
			{
				std::unique_ptr<FILE, fclose_deleter> epgFile;
				lock_recursive_mutex lock(m_streamLock);
				epgFile.swap(m_epgFile);
			}
			DeleteFile((m_epgFilePath + L".tmp").c_str());
		}
		return 0;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void CEdcbPlugIn::CtrlCmdCallback(const CCmdStream &cmd, CCmdStream &res)
{
	switch (cmd.GetParam()) {
	case CMD2_VIEW_APP_GET_BONDRIVER:
		{
			lock_recursive_mutex lock(m_statusLock);
			if (!m_statusInfo.bonDriver.empty()) {
				res.WriteVALUE(m_statusInfo.bonDriver);
				res.SetParam(CMD_SUCCESS);
			}
		}
		break;
	case CMD2_VIEW_APP_GET_DELAY:
		{
			lock_recursive_mutex lock(m_streamLock);
			res.WriteVALUE(m_epgUtil.GetTimeDelay());
			res.SetParam(CMD_SUCCESS);
		}
		break;
	case CMD2_VIEW_APP_GET_STATUS:
		{
			lock_recursive_mutex lock(m_statusLock);
			res.WriteVALUE(m_statusInfo.status);
			res.SetParam(CMD_SUCCESS);
		}
		break;
	case CMD2_VIEW_APP_CLOSE:
		SendNotifyMessage(m_hwnd, WM_APP_ADD_LOG, 0, reinterpret_cast<LPARAM>(L"CMD2_VIEW_APP_CLOSE"));
		SendNotifyMessage(m_hwnd, WM_APP_CLOSE, 0, 0);
		res.SetParam(CMD_SUCCESS);
		break;
	case CMD2_VIEW_APP_SET_ID:
		SendNotifyMessage(m_hwnd, WM_APP_ADD_LOG, 0, reinterpret_cast<LPARAM>(L"CMD2_VIEW_APP_SET_ID"));
		if (cmd.ReadVALUE(&m_statusInfo.appID)) {
			res.SetParam(CMD_SUCCESS);
		}
		break;
	case CMD2_VIEW_APP_GET_ID:
		res.WriteVALUE(m_statusInfo.appID);
		res.SetParam(CMD_SUCCESS);
		break;
	case CMD2_VIEW_APP_GET_STATUS_DETAILS:
		{
			DWORD flags;
			if (cmd.ReadVALUE(&flags)) {
				VIEW_APP_STATUS_INFO info = {};
				{
					lock_recursive_mutex lock(m_statusLock);
					if (flags & VIEW_APP_FLAG_GET_STATUS) {
						info.status = m_statusInfo.status;
					}
					if (flags & VIEW_APP_FLAG_GET_BONDRIVER) {
						info.bonDriver = m_statusInfo.bonDriver;
					}
					info.drop = m_statusInfo.drop;
					info.scramble = m_statusInfo.scramble;
					info.signalLv = m_statusInfo.signalLv;
					info.space = -1;
					info.ch = -1;
					info.originalNetworkID = m_statusInfo.originalNetworkID;
					info.transportStreamID = m_statusInfo.transportStreamID;
					info.appID = m_statusInfo.appID;
				}
				if (flags & VIEW_APP_FLAG_GET_DELAY) {
					lock_recursive_mutex lock(m_streamLock);
					info.delaySec = m_epgUtil.GetTimeDelay();
				}
				res.WriteVALUE(info);
				res.SetParam(CMD_SUCCESS);
			}
		}
		break;
	case CMD2_VIEW_APP_SET_STANDBY_REC:
		SendNotifyMessage(m_hwnd, WM_APP_ADD_LOG, 0, reinterpret_cast<LPARAM>(L"CMD2_VIEW_APP_SET_STANDBY_REC"));
		{
			DWORD val;
			if (cmd.ReadVALUE(&val)) {
				// TODO: とりあえず無視
				res.SetParam(CMD_SUCCESS);
			}
		}
		break;
	case CMD2_VIEW_APP_SET_CTRLMODE:
		SendNotifyMessage(m_hwnd, WM_APP_ADD_LOG, 0, reinterpret_cast<LPARAM>(L"CMD2_VIEW_APP_SET_CTRLMODE"));
		{
			SET_CTRL_MODE val;
			if (cmd.ReadVALUE(&val)) {
				lock_recursive_mutex lock(m_streamLock);
				if (m_recCtrlMap.count(val.ctrlID) != 0) {
					m_recCtrlMap[val.ctrlID].sid = val.SID;
				}
				res.SetParam(CMD_SUCCESS);
			}
		}
		break;
	case CMD2_VIEW_APP_SEARCH_EVENT:
		{
			SEARCH_EPG_INFO_PARAM key;
			if (cmd.ReadVALUE(&key)) {
				lock_recursive_mutex lock(m_streamLock);
				EPG_EVENT_INFO *epgInfo;
				if (m_epgUtil.SearchEpgInfo(key.ONID, key.TSID, key.SID, key.eventID, key.pfOnlyFlag, &epgInfo) == NO_ERR) {
					EPGDB_EVENT_INFO epgDBInfo;
					ConvertEpgInfo(key.ONID, key.TSID, key.SID, epgInfo, &epgDBInfo);
					res.WriteVALUE(epgDBInfo);
					res.SetParam(CMD_SUCCESS);
				}
			}
		}
		break;
	case CMD2_VIEW_APP_GET_EVENT_PF:
		{
			GET_EPG_PF_INFO_PARAM key;
			if (cmd.ReadVALUE(&key)) {
				lock_recursive_mutex lock(m_streamLock);
				EPG_EVENT_INFO *epgInfo;
				if (m_epgUtil.GetEpgInfo(key.ONID, key.TSID, key.SID, key.pfNextFlag, &epgInfo) == NO_ERR) {
					EPGDB_EVENT_INFO epgDBInfo;
					ConvertEpgInfo(key.ONID, key.TSID, key.SID, epgInfo, &epgDBInfo);
					res.WriteVALUE(epgDBInfo);
					res.SetParam(CMD_SUCCESS);
				}
			}
		}
		break;
	case CMD2_VIEW_APP_EXEC_VIEW_APP:
		// 無視
		res.SetParam(CMD_SUCCESS);
		break;
	default:
		// 同期呼び出しが必要なコマンド
		SendMessage(m_hwnd, WM_INVOKE_CTRL_CMD, reinterpret_cast<WPARAM>(&cmd), reinterpret_cast<LPARAM>(&res));
		break;
	}
}

void CEdcbPlugIn::CtrlCmdCallbackInvoked(const CCmdStream &cmd, CCmdStream &res)
{
	switch (cmd.GetParam()) {
	case CMD2_VIEW_APP_SET_BONDRIVER:
		m_pApp->AddLog(L"CMD2_VIEW_APP_SET_BONDRIVER");
		if (IsNotRecording()) {
			wstring val;
			if (cmd.ReadVALUE(&val)) {
				m_pApp->SetDriverName(nullptr);
				if (m_pApp->SetDriverName(val.c_str())) {
					res.SetParam(CMD_SUCCESS);
				}
			}
		}
		break;
	case CMD2_VIEW_APP_SET_CH:
		m_pApp->AddLog(L"CMD2_VIEW_APP_SET_CH");
		if (IsNotRecording()) {
			SET_CH_INFO val;
			if (cmd.ReadVALUE(&val)) {
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
						res.SetParam(CMD_SUCCESS);
					}
				}
				// チューナ番号指定には未対応
			}
		}
		break;
	case CMD2_VIEW_APP_CREATE_CTRL:
		m_pApp->AddLog(L"CMD2_VIEW_APP_CREATE_CTRL");
		res.WriteVALUE(++m_recCtrlCount);
		res.SetParam(CMD_SUCCESS);
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
			lock_recursive_mutex lock(m_streamLock);
			m_recCtrlMap[m_recCtrlCount] = REC_CTRL();
			m_recCtrlMap[m_recCtrlCount].sid = 0xFFFF;
			m_recCtrlMap[m_recCtrlCount].dropCount.SetNoLog(FALSE, this->m_noLogScramble);
		}
		break;
	case CMD2_VIEW_APP_DELETE_CTRL:
		m_pApp->AddLog(L"CMD2_VIEW_APP_DELETE_CTRL");
		{
			DWORD val;
			if (cmd.ReadVALUE(&val) && m_recCtrlMap.count(val) != 0) {
				REC_CTRL recCtrl;
				{
					lock_recursive_mutex lock(m_streamLock);
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
				res.SetParam(CMD_SUCCESS);
			}
		}
		break;
	case CMD2_VIEW_APP_REC_START_CTRL:
		m_pApp->AddLog(L"CMD2_VIEW_APP_REC_START_CTRL");
		{
			SET_CTRL_REC_PARAM val;
			if (cmd.ReadVALUE(&val) && m_recCtrlMap.count(val.ctrlID) != 0) {
				// overWriteFlag,pittariFlag,createSizeは無視
				REC_CTRL &recCtrl = m_recCtrlMap[val.ctrlID];
				if (recCtrl.filePath.empty() && !val.saveFolder.empty()) {
					// saveFolderは最初の要素のみ使う
					fs_path filePath = val.saveFolder[0].recFolder;
					filePath.append(val.saveFolder[0].recFileName);
					if (!m_recNamePrefix.empty()) {
						// 対象サービスIDをファイル名に前置する
						wstring prefix = m_recNamePrefix;
						wstring macro;
						Format(macro, L"%d", recCtrl.sid);
						Replace(prefix, L"$SID10$", macro);
						Format(macro, L"%04X", recCtrl.sid);
						Replace(prefix, L"$SID16$", macro);
						wstring name = prefix + filePath.filename().native();
						filePath.replace_filename(name);
					}
					// Write_OneServiceによるフィルタリングが行われると仮定してドロップカウンターにも同じフィルターをかける
					WORD filterSID = 0;
					fs_path name = filePath.filename();
					if (name.c_str()[0] == L'#') {
						WCHAR *endp;
						filterSID = static_cast<WORD>(wcstol(name.c_str() + 1, &endp, 16));
						if (endp - name.c_str() != 5 || *endp != L'#' || filterSID == 0xFFFF) {
							filterSID = 0;
						}
					}
					if (IsEdcbRecording()) {
						// 重複録画
						UtilCreateDirectories(filePath.parent_path());
						wstring strFilePath = filePath.native();
						if (DuplicateSave(m_duplicateOriginalPath.c_str(), &recCtrl.duplicateTargetID, &strFilePath)) {
							SendMessage(m_hwnd, WM_EPGCAP_BACK_START, 0, 0);
							lock_recursive_mutex lock(m_streamLock);
							recCtrl.filePath = strFilePath;
							recCtrl.filterSID = filterSID;
							recCtrl.filterStarted = false;
							res.SetParam(CMD_SUCCESS);
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
						wstring strFilePath = filePath.native();
						Replace(strFilePath, L"%", L"%%");
						vector<WCHAR> buf(strFilePath.c_str(), strFilePath.c_str() + strFilePath.size() + 1);
						ri.pszFileName = &buf.front();
						m_pApp->StopRecord();
						if (m_pApp->StartRecord(&ri)) {
							TVTest::RecordStatusInfo rsi;
							buf.resize(buf.size() + 64);
							rsi.pszFileName = &buf.front();
							rsi.MaxFileName = static_cast<int>(buf.size());
							if (m_pApp->GetRecordStatus(&rsi) && buf[0]) {
								SendMessage(m_hwnd, WM_EPGCAP_BACK_START, 0, 0);
								lock_recursive_mutex lock(m_streamLock);
								recCtrl.filePath = m_duplicateOriginalPath = &buf.front();
								recCtrl.filterSID = filterSID;
								recCtrl.filterStarted = false;
								recCtrl.duplicateTargetID = 1;
								SendMessage(m_hwnd, WM_SIGNAL_UPDATE_START, 0, 0);
								res.SetParam(CMD_SUCCESS);
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
			if (cmd.ReadVALUE(&val) && m_recCtrlMap.count(val.ctrlID) != 0) {
				SET_CTRL_REC_STOP_RES_PARAM resVal;
				CDropCount dropCount;
				DWORD duplicateTargetID;
				{
					lock_recursive_mutex lock(m_streamLock);
					resVal.recFilePath.swap(m_recCtrlMap[val.ctrlID].filePath);
					std::swap(m_recCtrlMap[val.ctrlID].dropCount, dropCount);
					duplicateTargetID = m_recCtrlMap[val.ctrlID].duplicateTargetID;
				}
				if (!resVal.recFilePath.empty()) {
					resVal.drop = dropCount.GetDropCount();
					resVal.scramble = dropCount.GetScrambleCount();
					resVal.subRecFlag = 0;
					if (val.saveErrLog && ((m_dropSaveThresh >= 0 && resVal.drop >= (ULONGLONG)m_dropSaveThresh) ||
					                       (m_scrambleSaveThresh >= 0 && resVal.scramble >= (ULONGLONG)m_scrambleSaveThresh))) {
						fs_path infoPath = GetPrivateProfileToString(L"SET", L"RecInfoFolder", L"", fs_path(m_edcbDir).append(L"Common.ini").c_str());
						if (infoPath.empty()) {
							infoPath = resVal.recFilePath + L".err";
						}
						else {
							infoPath.append(fs_path(resVal.recFilePath).filename().concat(L".err").native());
						}
						vector<pair<WORD, wstring>> pidNameList;
						TVTest::ServiceInfo si;
						for (int i = 0; m_pApp->GetServiceInfo(i, &si); ++i) {
							pidNameList.push_back(std::make_pair(si.VideoPID, wstring()));
							Format(pidNameList.back().second, L"0x%04X-Video", si.ServiceID);
							for (int j = 0; j < si.NumAudioPIDs; ++j) {
								pidNameList.push_back(std::make_pair(si.AudioPID[j], wstring()));
								Format(pidNameList.back().second, L"0x%04X-Audio(0x%02X)", si.ServiceID, si.AudioComponentType[j]);
							}
							pidNameList.push_back(std::make_pair(si.SubtitlePID, wstring()));
							Format(pidNameList.back().second, L"0x%04X-Subtitle", si.ServiceID);
						}
						for (size_t i = pidNameList.size(); i > 0; --i) {
							dropCount.SetPIDName(pidNameList[i - 1].first, pidNameList[i - 1].second);
						}
						dropCount.SetBonDriver(m_statusInfo.bonDriver);
						dropCount.SaveLog(infoPath.native(), m_dropLogAsUtf8);
					}
					if (IsEdcbRecording()) {
						// 重複録画
						DuplicateSave(m_duplicateOriginalPath.c_str(), &duplicateTargetID, nullptr);
					}
					else {
						// 最後の録画
						m_pApp->StopRecord();
					}
					res.WriteVALUE(resVal);
					res.SetParam(CMD_SUCCESS);
				}
			}
		}
		break;
	case CMD2_VIEW_APP_REC_FILE_PATH:
		{
			DWORD val;
			if (cmd.ReadVALUE(&val) && m_recCtrlMap.count(val) != 0) {
				if (!m_recCtrlMap[val].filePath.empty()) {
					res.WriteVALUE(m_recCtrlMap[val].filePath);
					res.SetParam(CMD_SUCCESS);
				}
			}
		}
		break;
	case CMD2_VIEW_APP_EPGCAP_START:
		m_pApp->AddLog(L"CMD2_VIEW_APP_EPGCAP_START");
		{
			vector<SET_CH_INFO> chList;
			if (m_epgCapChList.empty() && cmd.ReadVALUE(&chList)) {
				SendMessage(m_hwnd, WM_EPGCAP_START, 0, reinterpret_cast<LPARAM>(&chList));
				res.SetParam(CMD_SUCCESS);
			}
		}
		break;
	case CMD2_VIEW_APP_EPGCAP_STOP:
		m_pApp->AddLog(L"CMD2_VIEW_APP_EPGCAP_STOP");
		SendMessage(m_hwnd, WM_EPGCAP_STOP, 0, 0);
		res.SetParam(CMD_SUCCESS);
		break;
	case CMD2_VIEW_APP_REC_STOP_ALL:
		m_pApp->AddLog(L"CMD2_VIEW_APP_REC_STOP_ALL");
		{
			if (IsEdcbRecording()) {
				m_pApp->StopRecord();
			}
			lock_recursive_mutex lock(m_streamLock);
			m_recCtrlMap.clear();
			res.SetParam(CMD_SUCCESS);
		}
		break;
	default:
		res.SetParam(CMD_NON_SUPPORT);
		break;
	}
}

fs_path CEdcbPlugIn::GetEdcbSettingPath() const
{
	fs_path ret = GetPrivateProfileToString(L"SET", L"DataSavePath", L"", fs_path(m_edcbDir).append(L"Common.ini").c_str());
	if (ret.empty()) {
		ret = fs_path(m_edcbDir).append(L"Setting");
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
	wstring driver = L':' + m_statusInfo.bonDriver + L':';
	return std::search(m_nonTunerDrivers.begin(), m_nonTunerDrivers.end(), driver.begin(), driver.end(),
		[](wchar_t a, wchar_t b) { return towupper(a) == towupper(b); }) == m_nonTunerDrivers.end();
}

void CEdcbPlugIn::ReloadEpgThread()
{
	CSendCtrlCmd cmd;
	cmd.SetConnectTimeOut(4000);
	cmd.SendReloadEpg();
}

BOOL CALLBACK CEdcbPlugIn::StreamCallback(BYTE *pData, void *pClientData)
{
	CTSPacketUtil packet;
	if (packet.Set188TS(pData, 188)) {
		CEdcbPlugIn &this_ = *static_cast<CEdcbPlugIn*>(pClientData);
		lock_recursive_mutex lock(this_.m_streamLock);
		if (this_.m_chChangeID > CH_CHANGE_ERR) {
			if (packet.PID < BON_SELECTIVE_PID) {
				// チャンネル切り替え中
				// 1秒間は切り替え前のパケット来る可能性を考慮して無視する
				if (GetU32Tick() - this_.m_chChangeTick > 1000) {
					this_.m_epgUtil.AddTSPacket(pData, 188);
					WORD onid;
					WORD tsid;
					if (this_.m_epgUtil.GetTSID(&onid, &tsid) == NO_ERR && onid == HIWORD(this_.m_chChangeID) && tsid == LOWORD(this_.m_chChangeID)) {
						this_.m_chChangeID = CH_CHANGE_OK;
#ifdef SEND_PIPE_TEST
						this_.m_serviceFilter.Clear(tsid);
#endif
						lock_recursive_mutex lock2(this_.m_statusLock);
						this_.m_statusInfo.originalNetworkID = onid;
						this_.m_statusInfo.transportStreamID = tsid;
					}
					else if (GetU32Tick() - this_.m_chChangeTick > 15000) {
						// 15秒以上たってるなら切り替えエラー
						this_.m_chChangeID = CH_CHANGE_ERR;
						SendNotifyMessage(this_.m_hwnd, WM_UPDATE_STATUS_CODE, 0, 0);
					}
				}
			}
		}
		else {
			if (packet.PID < BON_SELECTIVE_PID) {
				if (this_.m_epgFile) {
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
							std::fill_n(nullData + 4, 184, (BYTE)0xFF);
							this_.m_epgFileTotPos = my_ftell(this_.m_epgFile.get());
							fwrite(nullData, 1, 188, this_.m_epgFile.get());
						}
					}
					// まずPAT、次に(あれば)TOTを書き込む。この処理は必須ではないが番組情報をより確実かつ効率的に読み出せる
					if (packet.PID == 0x14 && this_.m_epgFileState == EPG_FILE_ST_TOT) {
						this_.m_epgFileState = EPG_FILE_ST_ALL;
						if (this_.m_epgFileTotPos >= 0) {
							my_fseek(this_.m_epgFile.get(), this_.m_epgFileTotPos, SEEK_SET);
						}
						fwrite(pData, 1, 188, this_.m_epgFile.get());
						my_fseek(this_.m_epgFile.get(), 0, SEEK_END);
					}
					else if (packet.PID == 0 && this_.m_epgFileState >= EPG_FILE_ST_PAT || this_.m_epgFileState >= EPG_FILE_ST_TOT) {
						fwrite(pData, 1, 188, this_.m_epgFile.get());
					}
				}
				this_.m_epgUtil.AddTSPacket(pData, 188);

				DWORD tick = GetU32Tick();
				if (tick - this_.m_logoTick >= 1000) {
					this_.m_epgUtil.SetLogoTypeFlags(this_.m_logoTypeFlags, &this_.m_logoAdditionalNeededPids);
					if (this_.m_logoTypeFlags) {
						this_.m_epgUtil.EnumLogoList(EnumLogoListProc, &this_);
					}
					this_.m_logoTick = tick;
				}
			}
			else {
				if (this_.m_logoAdditionalNeededPids) {
					for (const WORD *pid = this_.m_logoAdditionalNeededPids; *pid; pid++) {
						if (*pid == packet.PID) {
							// ロゴ取得に必要
							this_.m_epgUtil.AddTSPacket(pData, 188);
							break;
						}
					}
				}
			}
		}
		for (map<DWORD, REC_CTRL>::iterator it = this_.m_recCtrlMap.begin(); it != this_.m_recCtrlMap.end(); ++it) {
			if (!it->second.filePath.empty()) {
				if (it->second.filterSID == 0) {
					// 全サービスなので何も弄らない
					it->second.dropCount.AddData(pData, 188);
				}
				else {
					// 指定サービス
					WORD tsid;
					if (!it->second.filterStarted &&
					    this_.m_chChangeID == CH_CHANGE_OK &&
					    this_.m_epgUtil.GetTSID(nullptr, &tsid) == NO_ERR)
					{
						it->second.filterForDropCount.SetServiceID(false, vector<WORD>(1, it->second.filterSID));
						it->second.filterForDropCount.Clear(tsid);
						it->second.filterStarted = true;
					}
					if (it->second.filterStarted) {
						this_.m_bufForDropCount.clear();
						it->second.filterForDropCount.FilterPacket(this_.m_bufForDropCount, pData, packet);
						it->second.dropCount.AddData(this_.m_bufForDropCount.data(), static_cast<DWORD>(this_.m_bufForDropCount.size()));
					}
				}
			}
		}
#ifdef SEND_PIPE_TEST
		if (this_.m_sendPipe.IsInitialized()) {
			if (this_.m_chChangeID == CH_CHANGE_OK) {
				this_.m_serviceFilter.FilterPacket(this_.m_sendPipeBuf, pData, packet);
				if (this_.m_sendPipeBuf.size() >= 48128) {
					this_.m_sendPipe.AddSendData(this_.m_sendPipeBuf.data(), static_cast<DWORD>(this_.m_sendPipeBuf.size()));
					this_.m_sendPipeBuf.clear();
				}
			}
			else {
				this_.m_sendPipeBuf.clear();
			}
		}
#endif
	}
	return TRUE;
}

BOOL CALLBACK CEdcbPlugIn::EnumLogoListProc(DWORD logoListSize, const LOGO_INFO *logoList, LPVOID param)
{
	CEdcbPlugIn &this_ = *static_cast<CEdcbPlugIn*>(param);

	if (logoList == nullptr) {
		return TRUE;
	}
	for (; logoListSize > 0; logoListSize--, logoList++) {
		if (logoList->serviceListSize > 0) {
			LONGLONG key = static_cast<LONGLONG>(logoList->onid) << 32 | static_cast<LONGLONG>(logoList->id) << 16 | logoList->type;
			vector<pair<LONGLONG, DWORD>>::iterator itr =
				lower_bound_first(this_.m_logoServiceListSizeMap.begin(), this_.m_logoServiceListSizeMap.end(), key);
			if (itr == this_.m_logoServiceListSizeMap.end() || itr->first != key) {
				this_.m_logoServiceListSizeMap.insert(itr, pair<LONGLONG, DWORD>(key, 0));
				// ロゴを保存
				WCHAR name[64];
				swprintf_s(name, L"%04x_%03x_000_%02x.png", logoList->onid, logoList->id, logoList->type);
				fs_path path = this_.GetEdcbSettingPath().append(LOGO_SAVE_FOLDER).append(name);
				bool update = true;
				if (UtilFileExists(path).first) {
					update = false;
					std::unique_ptr<FILE, fclose_deleter> logoFile(UtilOpenFile(path, UTIL_SECURE_READ));
					if (logoFile) {
						// 小さいか中身が違っていれば更新
						for (DWORD i = 0; i < logoList->dataSize; i++) {
							int c = fgetc(logoFile.get());
							if (c == EOF || c != logoList->data[i]) {
								update = true;
								break;
							}
						}
					}
				}
				if (update) {
					UtilCreateDirectory(path.parent_path());
					std::unique_ptr<FILE, fclose_deleter> logoFile(UtilOpenFile(path, UTIL_SECURE_WRITE));
					if (logoFile) {
						fwrite(logoList->data, 1, logoList->dataSize, logoFile.get());
					}
				}
				// 負荷分散のため列挙を中止
				return FALSE;
			}

			if (itr->second < logoList->serviceListSize) {
				// サービスからロゴへのポインティングを保存
				fs_path iniPath = this_.GetEdcbSettingPath().append(LOGO_SAVE_FOLDER L".ini");
				for (DWORD i = 0; i < logoList->serviceListSize; i++) {
					WCHAR name[16];
					swprintf_s(name, L"%04X%04X", logoList->onid, logoList->serviceList[i]);
					if (GetPrivateProfileInt(L"LogoIDMap", name, 0xFFFF, iniPath.c_str()) != logoList->id) {
						WritePrivateProfileInt(L"LogoIDMap", name, logoList->id, iniPath.c_str());
					}
				}
				itr->second = logoList->serviceListSize;
				return FALSE;
			}
		}
	}
	return TRUE;
}

TVTest::CTVTestPlugin *CreatePluginClass()
{
	return new CEdcbPlugIn;
}
