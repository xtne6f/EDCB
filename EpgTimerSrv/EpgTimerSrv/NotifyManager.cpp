#include "stdafx.h"
#include "NotifyManager.h"

#include "../../Common/CtrlCmdDef.h"
#include "../../Common/SendCtrlCmd.h"
#include "../../Common/EpgTimerUtil.h"
#include "../../Common/StringUtil.h"
#include "../../Common/TimeUtil.h"
#include "../../Common/PathUtil.h"

CNotifyManager::CNotifyManager()
{
	this->srvStatus = 0;
	this->notifyCount = 1;
	this->activeOrIdleCount = 0;
	std::fill_n(this->notifyUpdateCount, array_size(this->notifyUpdateCount), 0);
	this->guiFlag = false;
}

CNotifyManager::~CNotifyManager()
{
	if( this->notifyThread.joinable() ){
		this->notifyStopFlag = true;
		this->notifyEvent.Set();
		this->notifyThread.join();
	}
}

void CNotifyManager::RegistGUI(DWORD processID)
{
	lock_recursive_mutex lock(this->managerLock);

	UnRegistGUI(processID);
	this->registGUIList.push_back(processID);
	SetNotifySrvStatus(0xFFFFFFFF);
}

void CNotifyManager::RegistTCP(LPCWSTR ip, DWORD port)
{
	lock_recursive_mutex lock(this->managerLock);

	UnRegistTCP(ip, port);
	this->registTCPList.push_back(std::make_pair(wstring(ip), port));
	SetNotifySrvStatus(0xFFFFFFFF);
}

void CNotifyManager::UnRegistGUI(DWORD processID)
{
	lock_recursive_mutex lock(this->managerLock);

	for( size_t i = 0; i < this->registGUIList.size(); i++ ){
		if( this->registGUIList[i] == processID ){
			this->registGUIList.erase(this->registGUIList.begin() + i);
			break;
		}
	}
}

void CNotifyManager::UnRegistTCP(LPCWSTR ip, DWORD port)
{
	lock_recursive_mutex lock(this->managerLock);

	for( size_t i = 0; i < this->registTCPList.size(); i++ ){
		if( this->registTCPList[i].first == ip && this->registTCPList[i].second == port ){
			this->registTCPList.erase(this->registTCPList.begin() + i);
			break;
		}
	}
}

void CNotifyManager::SetNotifyCallback(const std::function<void()>& proc)
{
	lock_recursive_mutex lock(this->managerLock);

	this->notifyProc = proc;
	if( proc ){
		SetNotifySrvStatus(0xFFFFFFFF);
	}
}

void CNotifyManager::SetLogFilePath(LPCWSTR path)
{
	lock_recursive_mutex lock(this->managerLock);

	this->logFilePath = path;
}

bool CNotifyManager::GetNotify(NOTIFY_SRV_INFO* info, DWORD targetCount) const
{
	lock_recursive_mutex lock(this->managerLock);

	if( targetCount == 0 ){
		//現在のsrvStatusを返す
		NOTIFY_SRV_INFO status = {};
		status.notifyID = NOTIFY_UPDATE_SRV_STATUS;
		ConvertSystemTime(GetNowI64Time(), &status.time);
		status.param1 = this->srvStatus;
		//巡回カウンタは最後の通知と同値
		status.param3 = this->notifyCount;
		*info = status;
		return true;
	}else if( this->notifySentList.empty() || targetCount - this->notifySentList.back().param3 < 0x80000000 ){
		//存在するかどうかは即断できる
		return false;
	}else{
		//巡回カウンタがtargetCountよりも大きくなる最初の通知を返す
		*info = *std::find_if(this->notifySentList.begin(), this->notifySentList.end(),
		                      [=](const NOTIFY_SRV_INFO& a) { return targetCount - a.param3 >= 0x80000000; });
		return true;
	}
}

bool CNotifyManager::WaitForIdle(DWORD timeoutMsec) const
{
	DWORD count;
	{
		lock_recursive_mutex lock(this->managerLock);
		count = this->activeOrIdleCount;
		if( count % 2 == 0 ){
			//Idle
			return true;
		}
	}
	for( DWORD t = 1; t <= timeoutMsec; t += 10 ){
		SleepForMsec(10);
		lock_recursive_mutex lock(this->managerLock);
		if( count != this->activeOrIdleCount ){
			//1回以上Idleになった
			return true;
		}
	}
	return false;
}

int CNotifyManager::GetNotifyUpdateCount(DWORD notifyID) const
{
	if( 1 <= notifyID && notifyID < array_size(this->notifyUpdateCount) ){
		lock_recursive_mutex lock(this->managerLock);
		return this->notifyUpdateCount[notifyID] & 0x7FFFFFFF;
	}
	return -1;
}

pair<LPCWSTR, LPCWSTR> CNotifyManager::ExtractTitleFromInfo(const NOTIFY_SRV_INFO* info)
{
	return std::make_pair(
		info->notifyID == NOTIFY_UPDATE_PRE_REC_START ? L"予約録画開始準備" :
		info->notifyID == NOTIFY_UPDATE_REC_START ? L"録画開始" :
		info->notifyID == NOTIFY_UPDATE_REC_END ? L"録画終了" :
		info->notifyID == NOTIFY_UPDATE_REC_TUIJYU ? L"追従発生" :
		info->notifyID == NOTIFY_UPDATE_CHG_TUIJYU ? L"番組変更" :
		info->notifyID == NOTIFY_UPDATE_PRE_EPGCAP_START ? L"EPG取得" :
		info->notifyID == NOTIFY_UPDATE_EPGCAP_START ? L"EPG取得" :
		info->notifyID == NOTIFY_UPDATE_EPGCAP_END ? L"EPG取得" : L"",
		info->notifyID == NOTIFY_UPDATE_EPGCAP_START ? L"開始" :
		info->notifyID == NOTIFY_UPDATE_EPGCAP_END ? L"終了" : info->param4.c_str());
}

vector<DWORD> CNotifyManager::GetRegistGUI() const
{
	lock_recursive_mutex lock(this->managerLock);

	return this->registGUIList;
}

vector<pair<wstring, DWORD>> CNotifyManager::GetRegistTCP() const
{
	lock_recursive_mutex lock(this->managerLock);

	return this->registTCPList;
}

void CNotifyManager::AddNotify(DWORD notifyID)
{
	lock_recursive_mutex lock(this->managerLock);

	NOTIFY_SRV_INFO info = {};
	ConvertSystemTime(GetNowI64Time(), &info.time);
	info.notifyID = notifyID;
	//同じものがあるときは追加しない
	if( std::find_if(this->notifyList.begin(), this->notifyList.end(),
	                 [=](const NOTIFY_SRV_INFO& a) { return a.notifyID == notifyID; }) == this->notifyList.end() ){
		this->notifyList.push_back(info);
		SendNotify();
	}
}

void CNotifyManager::SetNotifySrvStatus(DWORD status)
{
	lock_recursive_mutex lock(this->managerLock);

	if( status != this->srvStatus ){
		NOTIFY_SRV_INFO info = {};
		info.notifyID = NOTIFY_UPDATE_SRV_STATUS;
		ConvertSystemTime(GetNowI64Time(), &info.time);
		info.param1 = this->srvStatus = (status == 0xFFFFFFFF ? this->srvStatus : status);

		this->notifyList.push_back(info);
		SendNotify();
	}
}

void CNotifyManager::AddNotifyMsg(DWORD notifyID, const wstring& msg)
{
	lock_recursive_mutex lock(this->managerLock);

	NOTIFY_SRV_INFO info = {};
	info.notifyID = notifyID;
	ConvertSystemTime(GetNowI64Time(), &info.time);
	info.param4 = msg;

	this->notifyList.push_back(info);
	SendNotify();
}

void CNotifyManager::SendNotify()
{
	if( this->notifyThread.joinable() == false ){
		this->notifyStopFlag = false;
		this->notifyThread = thread_(SendNotifyThread, this);
	}
	this->notifyEvent.Set();
}

void CNotifyManager::SendNotifyThread(CNotifyManager* sys)
{
	bool waitNotify = false;
	DWORD waitNotifyTick = 0;
	for(;;){
		vector<DWORD> registGUI;
		vector<pair<wstring, DWORD>> registTCP;
		NOTIFY_SRV_INFO notifyInfo;
		wstring path;

		if( waitNotify ){
			DWORD wait = GetU32Tick() - waitNotifyTick;
			sys->notifyEvent.WaitOne(wait < 5000 ? 5000 - wait : 0);
		}else{
			sys->notifyEvent.WaitOne();
		}
		if( sys->notifyStopFlag ){
			//キャンセルされた
			break;
		}
		//現在の情報取得
		{
			lock_recursive_mutex lock(sys->managerLock);
			registGUI = sys->GetRegistGUI();
			registTCP = sys->GetRegistTCP();
			if( waitNotify && GetU32Tick() - waitNotifyTick < 5000 ){
				vector<NOTIFY_SRV_INFO>::const_iterator itrNotify = std::find_if(
					sys->notifyList.begin(), sys->notifyList.end(), [](const NOTIFY_SRV_INFO& info) { return info.notifyID <= 100; });
				if( itrNotify == sys->notifyList.end() ){
					continue;
				}
				//NotifyID<=100の通知は遅延させず先に送る
				notifyInfo = *itrNotify;
				sys->notifyList.erase(itrNotify);
			}else{
				waitNotify = false;
				if( sys->notifyList.empty() ){
					continue;
				}
				notifyInfo = sys->notifyList[0];
				sys->notifyList.erase(sys->notifyList.begin());
				//NotifyID>100の通知は遅延させる
				if( notifyInfo.notifyID > 100 ){
					waitNotify = true;
					waitNotifyTick = GetU32Tick();
				}
			}
			if( sys->notifyList.empty() == false ){
				//次の通知がある
				sys->notifyEvent.Set();
			}
			//巡回カウンタをつける(0を避けるため奇数)
			sys->notifyCount += 2;
			notifyInfo.param3 = sys->notifyCount;
			//送信済みリストに追加
			sys->notifySentList.push_back(notifyInfo);
			if( sys->notifySentList.size() > 100 ){
				sys->notifySentList.erase(sys->notifySentList.begin());
			}
			if( notifyInfo.notifyID < array_size(sys->notifyUpdateCount) ){
				//更新系の通知をカウント
				sys->notifyUpdateCount[notifyInfo.notifyID]++;
			}
			if( sys->notifyProc ){
				//コールバックを呼ぶ(排他制御下なので注意)
				sys->notifyProc();
			}
			path = sys->logFilePath;
			sys->activeOrIdleCount++;
		}

		if( path.empty() == false && ExtractTitleFromInfo(&notifyInfo).first[0] ){
			//ログ保存
			std::unique_ptr<FILE, fclose_deleter> fp;
#if WCHAR_MAX <= 0xFFFF
			fp.reset(UtilOpenFile(path, UTIL_O_EXCL_CREAT_APPEND | UTIL_SH_READ));
			if( fp ){
				fwrite(L"\xFEFF", sizeof(WCHAR), 1, fp.get());
			}else
#endif
			{
				fp.reset(UtilOpenFile(path, UTIL_O_CREAT_APPEND | UTIL_SH_READ));
			}
			if( fp ){
				SYSTEMTIME st = notifyInfo.time;
				wstring log;
				Format(log, L"%d/%02d/%02d %02d:%02d:%02d.%03d [%ls] %ls__",
				       st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
				       ExtractTitleFromInfo(&notifyInfo).first, ExtractTitleFromInfo(&notifyInfo).second);
				Replace(log, L"\r\n", L"  ");
				log.replace(log.size() - 2, 2, UTIL_NEWLINE);
#if WCHAR_MAX > 0xFFFF
				for( size_t i = 0; i < log.size(); i++ ){
					char dest[4];
					fwrite(dest, 1, codepoint_to_utf8(log[i], dest), fp.get());
				}
#else
				fwrite(log.c_str(), sizeof(WCHAR), log.size(), fp.get());
#endif
			}
		}

		for( int tcp = 0; tcp < 2; tcp++ ){
			for( size_t i = 0; sys->notifyStopFlag == false && i < (tcp ? registTCP.size() : registGUI.size()); i++ ){
				CSendCtrlCmd sendCtrl;
				if( tcp ){
#ifdef _WIN32
					sendCtrl.SetSendMode(TRUE);
					sendCtrl.SetNWSetting(registTCP[i].first, registTCP[i].second);
#else
					AddDebugLog(L"CNotifyManager: Notification by TCP/IP registration is not supported.");
					continue;
#endif
				}else{
					sendCtrl.SetPipeSetting(CMD2_GUI_CTRL_PIPE, registGUI[i]);
				}
				sendCtrl.SetConnectTimeOut(10*1000);
				DWORD err = sendCtrl.SendGUINotifyInfo2(notifyInfo);
				if( err == CMD_NON_SUPPORT ){
					switch(notifyInfo.notifyID){
					case NOTIFY_UPDATE_EPGDATA:
						err = sendCtrl.SendGUIUpdateEpgData();
						break;
					case NOTIFY_UPDATE_RESERVE_INFO:
					case NOTIFY_UPDATE_REC_INFO:
					case NOTIFY_UPDATE_AUTOADD_EPG:
					case NOTIFY_UPDATE_AUTOADD_MANUAL:
						err = sendCtrl.SendGUIUpdateReserve();
						break;
					case NOTIFY_UPDATE_SRV_STATUS:
						err = sendCtrl.SendGUIStatusChg((WORD)notifyInfo.param1);
						break;
					default:
						break;
					}
				}
				if( tcp && err != CMD_SUCCESS && err != CMD_NON_SUPPORT ){
					//送信できなかったもの削除
					AddDebugLogFormat(L"notifyErr %ls:%d", registTCP[i].first.c_str(), registTCP[i].second);
					sys->UnRegistTCP(registTCP[i].first.c_str(), registTCP[i].second);
				}
				if( !tcp && err == CMD_ERR_CONNECT && sendCtrl.PipeExists() == false ){
					//存在しないので削除
					AddDebugLogFormat(L"notifyErr %ls:%d", L"PID", registGUI[i]);
					sys->UnRegistGUI(registGUI[i]);
				}
			}
		}

		lock_recursive_mutex lock(sys->managerLock);
		sys->activeOrIdleCount++;
	}
}
