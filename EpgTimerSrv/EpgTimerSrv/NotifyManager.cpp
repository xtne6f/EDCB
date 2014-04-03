#include "StdAfx.h"
#include "NotifyManager.h"
#include <process.h>

#include "../../Common/CtrlCmdDef.h"
#include "../../Common/SendCtrlCmd.h"

CNotifyManager::CNotifyManager(void)
{
	this->lockEvent = _CreateEvent(FALSE, TRUE, NULL);
	this->lockNotify = _CreateEvent(FALSE, TRUE, NULL);

	this->notifyThread = NULL;
	this->notifyStopEvent = _CreateEvent(FALSE, FALSE, NULL);
}

CNotifyManager::~CNotifyManager(void)
{
	if( this->notifyThread != NULL ){
		::SetEvent(this->notifyStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->notifyThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->notifyThread, 0xffffffff);
		}
		CloseHandle(this->notifyThread);
		this->notifyThread = NULL;
	}
	if( this->notifyStopEvent != NULL ){
		CloseHandle(this->notifyStopEvent);
		this->notifyStopEvent = NULL;
	}

	if( this->lockNotify != NULL ){
		NotifyUnLock();
		CloseHandle(this->lockNotify);
		this->lockNotify = NULL;
	}

	if( this->lockEvent != NULL ){
		UnLock();
		CloseHandle(this->lockEvent);
		this->lockEvent = NULL;
	}
}

BOOL CNotifyManager::Lock(LPCWSTR log, DWORD timeOut)
{
	if( this->lockEvent == NULL ){
		return FALSE;
	}
	//if( log != NULL ){
	//	_OutputDebugString(L"◆%s",log);
	//}
	DWORD dwRet = WaitForSingleObject(this->lockEvent, timeOut);
	if( dwRet == WAIT_ABANDONED || 
		dwRet == WAIT_FAILED ||
		dwRet == WAIT_TIMEOUT){
			OutputDebugString(L"◆CNotifyManager::Lock FALSE");
			if( log != NULL ){
				OutputDebugString(log);
			}
		return FALSE;
	}
	return TRUE;
}

void CNotifyManager::UnLock(LPCWSTR log)
{
	if( this->lockEvent != NULL ){
		SetEvent(this->lockEvent);
	}
	if( log != NULL ){
		OutputDebugString(log);
	}
}

BOOL CNotifyManager::NotifyLock(LPCWSTR log, DWORD timeOut)
{
	if( this->lockNotify == NULL ){
		return FALSE;
	}
	if( log != NULL ){
		OutputDebugString(log);
	}
	DWORD dwRet = WaitForSingleObject(this->lockNotify, timeOut);
	if( dwRet == WAIT_ABANDONED || 
		dwRet == WAIT_FAILED ||
		dwRet == WAIT_TIMEOUT){
			OutputDebugString(L"◆CNotifyManager::NotifyLock FALSE");
		return FALSE;
	}
	return TRUE;
}

void CNotifyManager::NotifyUnLock(LPCWSTR log)
{
	if( this->lockNotify != NULL ){
		SetEvent(this->lockNotify);
	}
	if( log != NULL ){
		OutputDebugString(log);
	}
}

void CNotifyManager::RegistGUI(DWORD processID)
{
	if( Lock(L"RegistGUI") == FALSE ) return;

	if( NotifyLock() == TRUE ){
		this->registGUIMap.insert(pair<DWORD,DWORD>(processID,processID));
		NotifyUnLock();
	}

	UnLock();
}

void CNotifyManager::RegistTCP(REGIST_TCP_INFO* info)
{
	if( Lock(L"RegistTCP") == FALSE ) return;

	if( NotifyLock() == TRUE ){
		wstring key = L"";
		Format(key, L"%s:%d", info->ip.c_str(), info->port);

		this->registTCPMap.insert(pair<wstring,REGIST_TCP_INFO>(key,*info));
		NotifyUnLock();
	}

	UnLock();
}

void CNotifyManager::UnRegistGUI(DWORD processID)
{
	if( Lock(L"UnRegistGUI") == FALSE ) return;

	if( NotifyLock() == TRUE ){
		map<DWORD,DWORD>::iterator itr;
		itr = this->registGUIMap.find(processID);
		if( itr != this->registGUIMap.end() ){
			this->registGUIMap.erase(itr);
		}
		NotifyUnLock();
	}

	UnLock();
}

void CNotifyManager::UnRegistTCP(REGIST_TCP_INFO* info)
{
	if( Lock(L"UnRegistTCP") == FALSE ) return;

	if( NotifyLock() == TRUE ){
		wstring key = L"";
		Format(key, L"%s:%d", info->ip.c_str(), info->port);

		map<wstring,REGIST_TCP_INFO>::iterator itr;
		itr = this->registTCPMap.find(key);
		if( itr != this->registTCPMap.end() ){
			this->registTCPMap.erase(itr);
		}
		NotifyUnLock();
	}

	UnLock();
}

void CNotifyManager::GetRegistGUI(map<DWORD, DWORD>* registGUI)
{
	if( Lock(L"GetRegistGUI") == FALSE ) return;

	if( NotifyLock() == TRUE && registGUI != NULL){
		*registGUI = registGUIMap;
		NotifyUnLock();
	}

	UnLock();
}

void CNotifyManager::GetRegistTCP(map<wstring, REGIST_TCP_INFO>* registTCP)
{
	if( Lock(L"GetRegistGUI") == FALSE ) return;

	if( NotifyLock() == TRUE && registTCP != NULL){
		*registTCP = registTCPMap;
		NotifyUnLock();
	}

	UnLock();
}

void CNotifyManager::AddNotifyLog(NOTIFY_SRV_INFO info)
{
	if( Lock(L"AddNotifyStatus") == FALSE ) return;

	if( NotifyLock() == TRUE ){
		this->notifyList.push_back(info);
		NotifyUnLock();
	}
	_SendNotify();

	UnLock();
}

void CNotifyManager::AddNotify(DWORD status)
{
	if( Lock(L"AddNotify") == FALSE ) return;

	if( NotifyLock() == TRUE ){
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
		}
		NotifyUnLock();
	}
	_SendNotify();

	UnLock();
}

void CNotifyManager::AddNotifySrvStatus(DWORD status)
{
	if( Lock(L"AddNotifySrvStatus") == FALSE ) return;

	if( NotifyLock() == TRUE ){
		NOTIFY_SRV_INFO info;
		info.notifyID = NOTIFY_UPDATE_SRV_STATUS;
		GetLocalTime(&info.time);
		info.param1 = status;

		this->notifyList.push_back(info);

		NotifyUnLock();
	}
	_SendNotify();

	UnLock();
}

void CNotifyManager::AddNotifyMsg(DWORD notifyID, wstring msg)
{
	if( Lock(L"AddNotifyMsg") == FALSE ) return;

	if( NotifyLock() == TRUE ){
		NOTIFY_SRV_INFO info;
		info.notifyID = notifyID;
		GetLocalTime(&info.time);
		info.param4 = msg;

		this->notifyList.push_back(info);

		NotifyUnLock();
	}
	_SendNotify();

	UnLock();
}

void CNotifyManager::_SendNotify()
{
	if( this->notifyThread != NULL ){
		if( ::WaitForSingleObject(this->notifyThread, 0) != WAIT_TIMEOUT ){
			//終わっている
			CloseHandle(this->notifyThread);
			this->notifyThread = NULL;
		}
	}
	if( this->notifyThread == NULL ){
		ResetEvent(this->notifyStopEvent);
		this->notifyThread = (HANDLE)_beginthreadex(NULL, 0, SendNotifyThread, (LPVOID)this, CREATE_SUSPENDED, NULL);
		SetThreadPriority( this->notifyThread, THREAD_PRIORITY_NORMAL );
		ResumeThread(this->notifyThread);
	}
}

UINT WINAPI CNotifyManager::SendNotifyThread(LPVOID param)
{
	CNotifyManager* sys = (CNotifyManager*)param;
	CSendCtrlCmd sendCtrl;
	map<DWORD,DWORD>::iterator itr;
	DWORD wait = 0;
	while(1){
		map<DWORD, DWORD> registGUI;
		map<wstring, REGIST_TCP_INFO> registTCP;
		NOTIFY_SRV_INFO notifyInfo;

		if( ::WaitForSingleObject(sys->notifyStopEvent, wait) != WAIT_TIMEOUT ){
			//キャンセルされた
			break;
		}
		//現在の情報取得
		if( sys->NotifyLock() == FALSE ) return 0;
		registGUI = sys->registGUIMap;
		registTCP = sys->registTCPMap;
		if( sys->notifyList.size() > 0 ){
			notifyInfo = sys->notifyList[0];
			sys->notifyList.erase(sys->notifyList.begin());
		}else{
			//リストないので終了
			sys->NotifyUnLock();
			return 0;
		}
		sys->NotifyUnLock();

		vector<DWORD> errID;
		for( itr = registGUI.begin(); itr != registGUI.end(); itr++){
			if( ::WaitForSingleObject(sys->notifyStopEvent, 0) != WAIT_TIMEOUT ){
				//キャンセルされた
				break;
			}
			if( _FindOpenExeProcess(itr->first) == TRUE ){
				wstring pipe;
				wstring waitEvent;
				Format(pipe, L"%s%d", CMD2_GUI_CTRL_PIPE, itr->first);
				Format(waitEvent, L"%s%d", CMD2_GUI_CTRL_WAIT_CONNECT, itr->first);

				sendCtrl.SetSendMode(FALSE);
				sendCtrl.SetPipeSetting(waitEvent, pipe);
				sendCtrl.SetConnectTimeOut(5*1000);
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
			if( ::WaitForSingleObject(sys->notifyStopEvent, 0) != WAIT_TIMEOUT ){
				//キャンセルされた
				break;
			}

			sendCtrl.SetSendMode(TRUE);
			sendCtrl.SetNWSetting(itrTCP->second.ip, itrTCP->second.port);
			sendCtrl.SetConnectTimeOut(5*1000);

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
		if( sys->NotifyLock() == FALSE ) return 0;

		if( notifyInfo.notifyID <= 100 ){
			wait = 0;
		}else{
			wait = 0;
			if( sys->notifyList.size() > 0 ){
				if( sys->notifyList[0].notifyID > 100 ){
					wait = 5*1000;
				}
			}
		}

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
		sys->NotifyUnLock();
	}

	return 0;
}
