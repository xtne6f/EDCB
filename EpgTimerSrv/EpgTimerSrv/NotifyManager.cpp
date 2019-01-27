#include "stdafx.h"
#include "NotifyManager.h"

#include "../../Common/CtrlCmdDef.h"
#include "../../Common/SendCtrlCmd.h"
#include "../../Common/EpgTimerUtil.h"
#include "../../Common/StringUtil.h"
#include "../../Common/TimeUtil.h"

CNotifyManager::CNotifyManager()
{
	this->srvStatus = 0;
	this->notifyCount = 1;
	this->notifyRemovePos = 0;
	this->hwndNotify = NULL;
	this->guiFlag = false;
}

CNotifyManager::~CNotifyManager()
{
	if( this->notifyThread.joinable() ){
		this->notifyStopFlag = true;
		this->notifyEvent.Set();
		this->notifyThread.join();
	}
	while( this->registGUIList.empty() == false ){
		UnRegistGUI(this->registGUIList.back().first);
	}
}

void CNotifyManager::RegistGUI(DWORD processID)
{
	CBlockLock lock(&this->managerLock);

	for( size_t i = 0; i < this->registGUIList.size(); i++ ){
		if( this->registGUIList[i].first == processID ){
			if( this->registGUIList[i].second &&
			    WaitForSingleObject(this->registGUIList[i].second, 0) == WAIT_TIMEOUT ){
				return;
			}
			UnRegistGUI(this->registGUIList[i].first);
			break;
		}
	}
	//権限によってはNULLになるがプロセスID再利用の曖昧さを避けるためのもので必須ではない
	HANDLE hProcess = OpenProcess(SYNCHRONIZE, FALSE, processID);
	this->registGUIList.push_back(std::make_pair(processID, hProcess));
	SetNotifySrvStatus(0xFFFFFFFF);
}

void CNotifyManager::RegistTCP(const REGIST_TCP_INFO& info)
{
	CBlockLock lock(&this->managerLock);

	UnRegistTCP(info);
	this->registTCPList.push_back(info);
	SetNotifySrvStatus(0xFFFFFFFF);
}

void CNotifyManager::UnRegistGUI(DWORD processID)
{
	CBlockLock lock(&this->managerLock);

	for( size_t i = 0; i < this->registGUIList.size(); i++ ){
		if( this->registGUIList[i].first == processID ){
			if( this->registGUIList[i].second ){
				CloseHandle(this->registGUIList[i].second);
			}
			this->registGUIList.erase(this->registGUIList.begin() + i);
			break;
		}
	}
}

void CNotifyManager::UnRegistTCP(const REGIST_TCP_INFO& info)
{
	CBlockLock lock(&this->managerLock);

	for( size_t i = 0; i < this->registTCPList.size(); i++ ){
		if( this->registTCPList[i].ip == info.ip && this->registTCPList[i].port == info.port ){
			this->registTCPList.erase(this->registTCPList.begin() + i);
			break;
		}
	}
}

void CNotifyManager::SetNotifyWindow(HWND hwnd, UINT msgID)
{
	CBlockLock lock(&this->managerLock);

	this->hwndNotify = hwnd;
	this->msgIDNotify = msgID;
	SetNotifySrvStatus(0xFFFFFFFF);
}

vector<NOTIFY_SRV_INFO> CNotifyManager::RemoveSentList()
{
	CBlockLock lock(&this->managerLock);

	vector<NOTIFY_SRV_INFO> list(this->notifySentList.begin() + this->notifyRemovePos, this->notifySentList.end());
	this->notifyRemovePos = this->notifySentList.size();
	return list;
}

bool CNotifyManager::GetNotify(NOTIFY_SRV_INFO* info, DWORD targetCount) const
{
	CBlockLock lock(&this->managerLock);

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

vector<DWORD> CNotifyManager::GetRegistGUI() const
{
	CBlockLock lock(&this->managerLock);

	vector<DWORD> list;
	for( size_t i = 0; i < this->registGUIList.size(); i++ ){
		if( this->registGUIList[i].second == NULL ||
		    WaitForSingleObject(this->registGUIList[i].second, 0) == WAIT_TIMEOUT ){
			list.push_back(this->registGUIList[i].first);
		}
	}
	return list;
}

vector<REGIST_TCP_INFO> CNotifyManager::GetRegistTCP() const
{
	CBlockLock lock(&this->managerLock);

	return this->registTCPList;
}

void CNotifyManager::AddNotify(DWORD status)
{
	CBlockLock lock(&this->managerLock);

	NOTIFY_SRV_INFO info = {};
	ConvertSystemTime(GetNowI64Time(), &info.time);
	info.notifyID = status;
	//同じものがあるときは追加しない
	if( std::find_if(this->notifyList.begin(), this->notifyList.end(),
	                 [=](const NOTIFY_SRV_INFO& a) { return a.notifyID == status; }) == this->notifyList.end() ){
		this->notifyList.push_back(info);
		SendNotify();
	}
}

void CNotifyManager::SetNotifySrvStatus(DWORD status)
{
	CBlockLock lock(&this->managerLock);

	if( status != this->srvStatus ){
		NOTIFY_SRV_INFO info = {};
		info.notifyID = NOTIFY_UPDATE_SRV_STATUS;
		ConvertSystemTime(GetNowI64Time(), &info.time);
		info.param1 = this->srvStatus = (status == 0xFFFFFFFF ? this->srvStatus : status);

		this->notifyList.push_back(info);
		SendNotify();
	}
}

void CNotifyManager::AddNotifyMsg(DWORD notifyID, wstring msg)
{
	CBlockLock lock(&this->managerLock);

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
		vector<REGIST_TCP_INFO> registTCP;
		NOTIFY_SRV_INFO notifyInfo;

		DWORD wait = INFINITE;
		if( waitNotify ){
			wait = GetTickCount() - waitNotifyTick;
			wait = (wait < 5000 ? 5000 - wait : 0);
		}
		WaitForSingleObject(sys->notifyEvent.Handle(), wait);
		if( sys->notifyStopFlag ){
			//キャンセルされた
			break;
		}
		//現在の情報取得
		{
			CBlockLock lock(&sys->managerLock);
			registGUI = sys->GetRegistGUI();
			for( size_t i = 0; i < sys->registGUIList.size(); ){
				if( std::find(registGUI.begin(), registGUI.end(), sys->registGUIList[i].first) == registGUI.end() ){
					//終了したGUIを削除
					sys->UnRegistGUI(sys->registGUIList[i].first);
				}else{
					i++;
				}
			}
			registTCP = sys->GetRegistTCP();
			if( waitNotify && GetTickCount() - waitNotifyTick < 5000 ){
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
					waitNotifyTick = GetTickCount();
				}
			}
			if( sys->notifyList.empty() == false ){
				//次の通知がある
				sys->notifyEvent.Set();
			}
			//巡回カウンタをつける(0を避けるため奇数)
			sys->notifyCount += 2;
			notifyInfo.param3 = sys->notifyCount;
			//送信済みリストに追加してウィンドウメッセージで知らせる
			sys->notifySentList.push_back(notifyInfo);
			if( sys->notifySentList.size() > 100 ){
				sys->notifySentList.erase(sys->notifySentList.begin());
				if( sys->notifyRemovePos != 0 ){
					sys->notifyRemovePos--;
				}
			}
			if( sys->hwndNotify != NULL ){
				PostMessage(sys->hwndNotify, sys->msgIDNotify, 0, 0);
			}
		}

		for( int tcp = 0; tcp < 2; tcp++ ){
			for( size_t i = 0; sys->notifyStopFlag == false && i < (tcp ? registTCP.size() : registGUI.size()); i++ ){
				CSendCtrlCmd sendCtrl;
				if( tcp ){
					sendCtrl.SetSendMode(TRUE);
					sendCtrl.SetNWSetting(registTCP[i].ip, registTCP[i].port);
				}else{
					sendCtrl.SetPipeSetting(CMD2_GUI_CTRL_WAIT_CONNECT, CMD2_GUI_CTRL_PIPE, registGUI[i]);
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
					_OutputDebugString(L"notifyErr %s:%d\r\n", registTCP[i].ip.c_str(), registTCP[i].port);
					sys->UnRegistTCP(registTCP[i]);
				}
			}
		}
	}
}
