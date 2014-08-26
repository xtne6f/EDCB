#include "StdAfx.h"
#include "NotifyManager.h"
#include <process.h>

#include "../../Common/CtrlCmdDef.h"
#include "../../Common/SendCtrlCmd.h"
#include "../../Common/BlockLock.h"

CNotifyManager::CNotifyManager(void)
{
	InitializeCriticalSection(&this->managerLock);

	this->notifyEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	this->notifyThread = NULL;
	this->notifyStopFlag = FALSE;
}

CNotifyManager::~CNotifyManager(void)
{
	if( this->notifyThread != NULL ){
		this->notifyStopFlag = TRUE;
		::SetEvent(this->notifyEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->notifyThread, 20000) == WAIT_TIMEOUT ){
			::TerminateThread(this->notifyThread, 0xffffffff);
		}
		CloseHandle(this->notifyThread);
		this->notifyThread = NULL;
	}
	if( this->notifyEvent != NULL ){
		CloseHandle(this->notifyEvent);
		this->notifyEvent = NULL;
	}

	DeleteCriticalSection(&this->managerLock);
}

void CNotifyManager::RegistGUI(DWORD processID)
{
	CBlockLock lock(&this->managerLock);

	{
		this->registGUIMap.insert(pair<DWORD,DWORD>(processID,processID));
	}
}

void CNotifyManager::RegistTCP(const REGIST_TCP_INFO& info)
{
	CBlockLock lock(&this->managerLock);

	{
		wstring key = L"";
		Format(key, L"%s:%d", info.ip.c_str(), info.port);

		this->registTCPMap.insert(pair<wstring,REGIST_TCP_INFO>(key,info));
	}
}

void CNotifyManager::UnRegistGUI(DWORD processID)
{
	CBlockLock lock(&this->managerLock);

	{
		map<DWORD,DWORD>::iterator itr;
		itr = this->registGUIMap.find(processID);
		if( itr != this->registGUIMap.end() ){
			this->registGUIMap.erase(itr);
		}
	}
}

void CNotifyManager::UnRegistTCP(const REGIST_TCP_INFO& info)
{
	CBlockLock lock(&this->managerLock);

	{
		wstring key = L"";
		Format(key, L"%s:%d", info.ip.c_str(), info.port);

		map<wstring,REGIST_TCP_INFO>::iterator itr;
		itr = this->registTCPMap.find(key);
		if( itr != this->registTCPMap.end() ){
			this->registTCPMap.erase(itr);
		}
	}
}

void CNotifyManager::GetRegistGUI(map<DWORD, DWORD>* registGUI)
{
	CBlockLock lock(&this->managerLock);

	if( registGUI != NULL){
		*registGUI = registGUIMap;
	}
}

void CNotifyManager::GetRegistTCP(map<wstring, REGIST_TCP_INFO>* registTCP)
{
	CBlockLock lock(&this->managerLock);

	if( registTCP != NULL){
		*registTCP = registTCPMap;
	}
}

void CNotifyManager::AddNotify(DWORD status)
{
	CBlockLock lock(&this->managerLock);

	{
		NOTIFY_SRV_INFO info;
		GetLocalTime(&info.time);
		info.notifyID = status;
		BOOL find = FALSE;
		for(size_t i=0; i<this->notifyList.size(); i++ ){
			if( this->notifyList[i].notifyID == status ){
				find = TRUE;
				break;
			}
		}
		//同じものがあるときは追加しない
		if( find == FALSE ){
			this->notifyList.push_back(info);
			_SendNotify();
		}
	}
}

void CNotifyManager::AddNotifySrvStatus(DWORD status)
{
	CBlockLock lock(&this->managerLock);

	{
		NOTIFY_SRV_INFO info;
		info.notifyID = NOTIFY_UPDATE_SRV_STATUS;
		GetLocalTime(&info.time);
		info.param1 = status;

		this->notifyList.push_back(info);
	}
	_SendNotify();
}

void CNotifyManager::AddNotifyMsg(DWORD notifyID, wstring msg)
{
	CBlockLock lock(&this->managerLock);

	{
		NOTIFY_SRV_INFO info;
		info.notifyID = notifyID;
		GetLocalTime(&info.time);
		info.param4 = msg;

		this->notifyList.push_back(info);
	}
	_SendNotify();
}

void CNotifyManager::_SendNotify()
{
	if( this->notifyThread == NULL ){
		this->notifyThread = (HANDLE)_beginthreadex(NULL, 0, SendNotifyThread, this, 0, NULL);
	}
	SetEvent(this->notifyEvent);
}

UINT WINAPI CNotifyManager::SendNotifyThread(LPVOID param)
{
	CNotifyManager* sys = (CNotifyManager*)param;
	CSendCtrlCmd sendCtrl;
	map<DWORD,DWORD>::iterator itr;
	BOOL wait1Sec = FALSE;
	BOOL waitNotify = FALSE;
	DWORD waitNotifyTick;
	while(1){
		map<DWORD, DWORD> registGUI;
		map<wstring, REGIST_TCP_INFO> registTCP;
		NOTIFY_SRV_INFO notifyInfo;

		if( wait1Sec != FALSE ){
			wait1Sec = FALSE;
			Sleep(1000);
		}
		if( ::WaitForSingleObject(sys->notifyEvent, INFINITE) != WAIT_OBJECT_0 || sys->notifyStopFlag != FALSE ){
			//キャンセルされた
			break;
		}
		//現在の情報取得
		{
			CBlockLock lock(&sys->managerLock);
			if( sys->notifyList.empty() ){
				continue;
			}
			registGUI = sys->registGUIMap;
			registTCP = sys->registTCPMap;
			if( waitNotify != FALSE && GetTickCount() - waitNotifyTick < 5000 ){
				vector<NOTIFY_SRV_INFO>::const_iterator itrNotify;
				for( itrNotify = sys->notifyList.begin(); itrNotify != sys->notifyList.end(); itrNotify++ ){
					if( itrNotify->notifyID <= 100 ){
						break;
					}
				}
				if( itrNotify == sys->notifyList.end() ){
					SetEvent(sys->notifyEvent);
					wait1Sec = TRUE;
					continue;
				}
				//NotifyID<=100の通知は遅延させず先に送る
				notifyInfo = *itrNotify;
				sys->notifyList.erase(itrNotify);
			}else{
				waitNotify = FALSE;
				notifyInfo = sys->notifyList[0];
				sys->notifyList.erase(sys->notifyList.begin());
				//NotifyID>100の通知は遅延させる
				if( notifyInfo.notifyID > 100 ){
					waitNotify = TRUE;
					waitNotifyTick = GetTickCount();
				}
			}
			if( sys->notifyList.empty() == false ){
				//次の通知がある
				SetEvent(sys->notifyEvent);
			}
		}

		vector<DWORD> errID;
		for( itr = registGUI.begin(); itr != registGUI.end(); itr++){
			if( sys->notifyStopFlag != FALSE ){
				//キャンセルされた
				break;
			}
			if( _FindOpenExeProcess(itr->first) == TRUE ){
				sendCtrl.SetSendMode(FALSE);
				sendCtrl.SetPipeSetting(CMD2_GUI_CTRL_WAIT_CONNECT, CMD2_GUI_CTRL_PIPE, itr->first);
				sendCtrl.SetConnectTimeOut(10*1000);
				DWORD err = sendCtrl.SendGUINotifyInfo2(&notifyInfo);
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
				if( err != CMD_SUCCESS && err != CMD_NON_SUPPORT){
					errID.push_back(itr->first);
				}
			}else{
				errID.push_back(itr->first);
			}
		}

		map<wstring, REGIST_TCP_INFO>::iterator itrTCP;
		vector<wstring> errIP;
		for( itrTCP = registTCP.begin(); itrTCP != registTCP.end(); itrTCP++){
			if( sys->notifyStopFlag != FALSE ){
				//キャンセルされた
				break;
			}

			sendCtrl.SetSendMode(TRUE);
			sendCtrl.SetNWSetting(itrTCP->second.ip, itrTCP->second.port);
			sendCtrl.SetConnectTimeOut(10*1000);

			DWORD err = sendCtrl.SendGUINotifyInfo2(&notifyInfo);
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
			if( err != CMD_SUCCESS && err != CMD_NON_SUPPORT){
				errIP.push_back(itrTCP->first);
			}
		}

		//送信できなかったもの削除
		CBlockLock lock(&sys->managerLock);

		for( size_t i=0; i<errID.size(); i++ ){
			itr = sys->registGUIMap.find(errID[i]);
			if( itr != sys->registGUIMap.end() ){
				sys->registGUIMap.erase(itr);
			}
		}
		for( size_t i=0; i<errIP.size(); i++ ){
			itrTCP = sys->registTCPMap.find(errIP[i]);
			if( itrTCP != sys->registTCPMap.end() ){
				_OutputDebugString(L"notifyErr %s:%d", itrTCP->second.ip.c_str(), itrTCP->second.port);
				sys->registTCPMap.erase(itrTCP);
			}
		}
	}

	return 0;
}
