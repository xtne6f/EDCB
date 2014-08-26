#include "StdAfx.h"
#include "ReserveManager.h"
#include <process.h>
#include "../../Common/CtrlCmdDef.h"
#include "../../Common/SendCtrlCmd.h"
#include "../../Common/ReNamePlugInUtil.h"
#include "../../Common/EpgTimerUtil.h"

#include <tlhelp32.h> 
#include <shlwapi.h>
#include <algorithm>
#include <locale>

#include <LM.h>
#pragma comment (lib, "netapi32.lib")

CReserveManager::CReserveManager(void)
{
	this->lockEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
//	this->lockNotify = _CreateEvent(FALSE, TRUE, NULL);

	this->bankCheckThread = NULL;
	this->bankCheckStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	this->notifyStatus = 0;
/*
	this->notifyThread = NULL;
	this->notifyStopEvent = _CreateEvent(FALSE, FALSE, NULL);
	this->notifyStatusThread = NULL;
	this->notifyStatusStopEvent = _CreateEvent(FALSE, FALSE, NULL);
	this->notifyEpgReloadThread = NULL;
	this->notifyEpgReloadStopEvent = _CreateEvent(FALSE, FALSE, NULL);
	*/
	this->notifyManager = NULL;

	this->tunerManager.ReloadTuner();
	vector<DWORD> idList;
	this->tunerManager.GetEnumID( &idList );
	for( size_t i=0; i<idList.size(); i++){
		BANK_INFO* item = new BANK_INFO;
		item->tunerID = idList[i];
		this->bankMap.insert(pair<DWORD, BANK_INFO*>(item->tunerID, item));
	}

	this->tunerManager.GetEnumTunerBank(&this->tunerBankMap);

	this->backPriorityFlag = FALSE;
	this->sameChPriorityFlag = FALSE;

	this->defStartMargine = 5;
	this->defEndMargine = 2;

	this->enableSetSuspendMode = 0xFF;
	this->enableSetRebootFlag = 0xFF;

	this->epgCapCheckFlag = FALSE;

	this->autoDel = FALSE;

	this->eventRelay = FALSE;

	this->duraChgMarginMin = 0;
	this->notFindTuijyuHour = 6;
	this->noEpgTuijyuMin = 30;

	this->autoDelRecInfo = FALSE;
	this->autoDelRecInfoNum = 100;
	this->timeSync = FALSE;
	this->setTimeSync = FALSE;

	this->NWTVPID = 0;
	this->NWTVUDP = FALSE;
	this->NWTVTCP = FALSE;

	this->twitterManager = NULL;
	this->useTweet = FALSE;
	this->useProxy = FALSE;

	this->reloadBankMapAlgo = 0;

	this->ngAddResSrvCoop = FALSE;
	this->errEndBatRun = FALSE;

	this->ngShareFile = FALSE;
	this->epgDBManager = NULL;

	this->chgRecInfo = FALSE;

	ReloadSetting();
}


CReserveManager::~CReserveManager(void)
{
	if( this->bankCheckThread != NULL ){
		::SetEvent(this->bankCheckStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->bankCheckThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->bankCheckThread, 0xffffffff);
		}
		CloseHandle(this->bankCheckThread);
		this->bankCheckThread = NULL;
	}
	if( this->bankCheckStopEvent != NULL ){
		CloseHandle(this->bankCheckStopEvent);
		this->bankCheckStopEvent = NULL;
	}
	/*
	if( this->notifyEpgReloadThread != NULL ){
		::SetEvent(this->notifyEpgReloadStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->notifyEpgReloadThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->notifyEpgReloadThread, 0xffffffff);
		}
		CloseHandle(this->notifyEpgReloadThread);
		this->notifyEpgReloadThread = NULL;
	}
	if( this->notifyEpgReloadStopEvent != NULL ){
		CloseHandle(this->notifyEpgReloadStopEvent);
		this->notifyEpgReloadStopEvent = NULL;
	}

	if( this->notifyStatusThread != NULL ){
		::SetEvent(this->notifyStatusStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->notifyStatusThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->notifyStatusThread, 0xffffffff);
		}
		CloseHandle(this->notifyStatusThread);
		this->notifyStatusThread = NULL;
	}
	if( this->notifyStatusStopEvent != NULL ){
		CloseHandle(this->notifyStatusStopEvent);
		this->notifyStatusStopEvent = NULL;
	}

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
	*/
	map<DWORD, CTunerBankCtrl*>::iterator itrCtrl;
	for( itrCtrl = this->tunerBankMap.begin(); itrCtrl != this->tunerBankMap.end(); itrCtrl++ ){
		SAFE_DELETE(itrCtrl->second);
	}

	map<DWORD, BANK_INFO*>::iterator itrBank;
	for( itrBank = this->bankMap.begin(); itrBank != this->bankMap.end(); itrBank++){
		map<DWORD, BANK_WORK_INFO*>::iterator itrWork;
		for( itrWork = itrBank->second->reserveList.begin(); itrWork != itrBank->second->reserveList.end(); itrWork++ ){
			SAFE_DELETE(itrWork->second);
		}
		SAFE_DELETE(itrBank->second);
	}

	map<DWORD, BANK_WORK_INFO*>::iterator itrNG;
	for( itrNG = this->NGReserveMap.begin(); itrNG != this->NGReserveMap.end(); itrNG++){
		SAFE_DELETE(itrNG->second);
	}

	map<DWORD, CReserveInfo*>::iterator itr;
	for( itr = this->reserveInfoMap.begin(); itr != this->reserveInfoMap.end(); itr++ ){
		SAFE_DELETE(itr->second);
	}

	SAFE_DELETE(this->twitterManager);
	/*
	if( this->lockNotify != NULL ){
		NotifyUnLock();
		CloseHandle(this->lockNotify);
		this->lockNotify = NULL;
	}*/

	if( this->lockEvent != NULL ){
		UnLock();
		CloseHandle(this->lockEvent);
		this->lockEvent = NULL;
	}

}

BOOL CReserveManager::Lock(LPCWSTR log, DWORD timeOut)
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
			OutputDebugString(L"◆CReserveManager::Lock FALSE");
			if( log != NULL ){
				OutputDebugString(log);
			}
		return FALSE;
	}
	return TRUE;
}

void CReserveManager::UnLock(LPCWSTR log)
{
	if( this->lockEvent != NULL ){
		SetEvent(this->lockEvent);
	}
	if( log != NULL ){
		OutputDebugString(log);
	}
}
/*
BOOL CReserveManager::NotifyLock(LPCWSTR log, DWORD timeOut)
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
			OutputDebugString(L"◆CReserveManager::NotifyLock FALSE");
		return FALSE;
	}
	return TRUE;
}

void CReserveManager::NotifyUnLock(LPCWSTR log)
{
	if( this->lockNotify != NULL ){
		SetEvent(this->lockNotify);
	}
	if( log != NULL ){
		OutputDebugString(log);
	}
}
*/
void CReserveManager::SetNotifyManager(CNotifyManager* manager)
{
	if( Lock(L"CReserveManager::SetNotifyManager") == FALSE ) return;
	this->notifyManager = manager;

	map<DWORD, CTunerBankCtrl*>::iterator itrCtrl;
	for( itrCtrl = this->tunerBankMap.begin(); itrCtrl != this->tunerBankMap.end(); itrCtrl++ ){
		itrCtrl->second->SetNotifyManager(manager);
	}

	this->batManager.SetNotifyManager(manager);

	UnLock();
}

void CReserveManager::SetEpgDBManager(CEpgDBManager* epgDBManager)
{
	if( Lock(L"CReserveManager::SetEpgDBManager") == FALSE ) return;

	this->epgDBManager = epgDBManager;
	map<DWORD, CTunerBankCtrl*>::iterator itrCtrl;
	for( itrCtrl = this->tunerBankMap.begin(); itrCtrl != this->tunerBankMap.end(); itrCtrl++ ){
		itrCtrl->second->SetEpgDBManager(epgDBManager);
	}

	UnLock();
}

void CReserveManager::ChangeRegist()
{
	if( Lock(L"CReserveManager::ChangeRegist") == FALSE ) return;

	if( this->notifyManager != NULL ){
		this->notifyManager->AddNotifySrvStatus(this->notifyStatus);
	}

	UnLock();
}
/*
void CReserveManager::SetRegistGUI(map<DWORD, DWORD> registGUIMap)
{
	if( Lock(L"SetRegistGUI") == FALSE ) return;

	if( this->notifyThread != NULL ){
		::SetEvent(this->notifyStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->notifyThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->notifyThread, 0xffffffff);
		}
		CloseHandle(this->notifyThread);
		this->notifyThread = NULL;
	}

	this->registGUIMap = registGUIMap;

	map<DWORD, CTunerBankCtrl*>::iterator itrCtrl;
	for( itrCtrl = this->tunerBankMap.begin(); itrCtrl != this->tunerBankMap.end(); itrCtrl++ ){
		itrCtrl->second->SetRegistGUI(registGUIMap);
	}

	this->batManager.SetRegistGUI(registGUIMap);

	SendNotifyStatus(this->notifyStatus);

	UnLock();
}

void CReserveManager::SetRegistTCP(map<wstring, REGIST_TCP_INFO> registTCPMap)
{
	if( Lock(L"SetRegistTCP") == FALSE ) return;

	if( this->notifyThread != NULL ){
		::SetEvent(this->notifyStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->notifyThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->notifyThread, 0xffffffff);
		}
		CloseHandle(this->notifyThread);
		this->notifyThread = NULL;
	}

	this->registTCPMap = registTCPMap;

	SendNotifyStatus(this->notifyStatus);

	UnLock();
}
*/
void CReserveManager::ReloadSetting()
{
	if( Lock(L"ReloadSetting") == FALSE ) return;

	wstring iniAppPath = L"";
	GetModuleIniPath(iniAppPath);

	wstring iniCommonPath = L"";
	GetCommonIniPath(iniCommonPath);

	this->BSOnly = GetPrivateProfileInt(L"SET", L"BSBasicOnly", 1, iniCommonPath.c_str());
	this->CS1Only = GetPrivateProfileInt(L"SET", L"CS1BasicOnly", 1, iniCommonPath.c_str());
	this->CS2Only = GetPrivateProfileInt(L"SET", L"CS2BasicOnly", 1, iniCommonPath.c_str());

	this->ngCapMin = GetPrivateProfileInt(L"SET", L"NGEpgCapTime", 20, iniAppPath.c_str());
	this->ngCapTunerMin = GetPrivateProfileInt(L"SET", L"NGEpgCapTunerTime", 20, iniAppPath.c_str());

	this->epgCapTimeList.clear();
	int count = GetPrivateProfileInt(L"EPG_CAP", L"Count", 0, iniAppPath.c_str());
	for( int i=0; i<count; i++ ){
		wstring selectKey;
		Format(selectKey, L"%dSelect", i);
		if( GetPrivateProfileInt(L"EPG_CAP", selectKey.c_str(), 0, iniAppPath.c_str()) == 1 ){
			wstring timeKey;
			Format(timeKey, L"%d", i);
			WCHAR buff[256] = L"";
			GetPrivateProfileString(L"EPG_CAP", timeKey.c_str(), L"", buff, 256, iniAppPath.c_str());
			wstring time = buff;
			if( time.size() > 0 ){
				wstring left = L"";
				wstring right = L"";
				Separate(time, L":", left, right);

				DWORD second = _wtoi(left.c_str()) * 60 * 60 + _wtoi(right.c_str()) * 60;
				EPGTIME_INFO ei;
				ei.time = second;
				//曜日指定接尾辞(w1=Mon,...,w7=Sun)
				ei.wday = 0;
				if( Separate(right, L"w", left, right) != FALSE ){
					ei.wday = _wtoi(right.c_str());
				}
				//取得種別(bit0(LSB)=BS,bit1=CS1,bit2=CS2)。負値のときは共通設定に従う
				wstring flagsKey;
				Format(flagsKey, L"%dBasicOnlyFlags", i);
				ei.basicOnlyFlags = GetPrivateProfileInt(L"EPG_CAP", flagsKey.c_str(), -1, iniAppPath.c_str());
				this->epgCapTimeList.push_back(ei);
			}
		}
	}

	this->wakeTime = GetPrivateProfileInt(L"SET", L"WakeTime", 5, iniAppPath.c_str());

	this->defSuspendMode = GetPrivateProfileInt(L"SET", L"RecEndMode", 0, iniAppPath.c_str());
	this->defRebootFlag = GetPrivateProfileInt(L"SET", L"Reboot", 0, iniAppPath.c_str());

	this->batMargin = GetPrivateProfileInt(L"SET", L"BatMargin", 10, iniAppPath.c_str());
	
	this->noStandbyExeList.clear();
	count = GetPrivateProfileInt(L"NO_SUSPEND", L"Count", 0, iniAppPath.c_str());
	if( count == 0 ){
		this->noStandbyExeList.push_back(L"EpgDataCap_Bon.exe");
	}else{
		for( int i=0; i<count; i++ ){
			wstring key;
			Format(key, L"%d", i);
			WCHAR buff[256] = L"";
			GetPrivateProfileString(L"NO_SUSPEND", key.c_str(), L"", buff, 256, iniAppPath.c_str());
			wstring exe = buff;
			if( exe.size() > 0 ){
				std::transform(exe.begin(), exe.end(), exe.begin(), tolower);
				this->noStandbyExeList.push_back(exe);
			}
		}
	}

	this->noStandbyTime = GetPrivateProfileInt(L"NO_SUSPEND", L"NoStandbyTime", 10, iniAppPath.c_str());
	this->ngShareFile = (BOOL)GetPrivateProfileInt(L"NO_SUSPEND", L"NoShareFile", 0, iniAppPath.c_str());
	this->autoDel = (BOOL)GetPrivateProfileInt(L"SET", L"AutoDel", 0, iniAppPath.c_str());

	this->delExtList.clear();
	count = GetPrivateProfileInt(L"DEL_EXT", L"Count", 0, iniAppPath.c_str());
	for( int i=0; i<count; i++ ){
		wstring key;
		Format(key, L"%d", i);
		WCHAR buff[512] = L"";
		GetPrivateProfileString(L"DEL_EXT", key.c_str(), L"", buff, 512, iniAppPath.c_str());
		wstring ext = buff;
		this->delExtList.push_back(ext);
	}
	this->delFolderList.clear();
	count = GetPrivateProfileInt(L"DEL_CHK", L"Count", 0, iniAppPath.c_str());
	for( int i=0; i<count; i++ ){
		wstring key;
		Format(key, L"%d", i);
		WCHAR buff[512] = L"";
		GetPrivateProfileString(L"DEL_CHK", key.c_str(), L"", buff, 512, iniAppPath.c_str());
		wstring folder = buff;
		this->delFolderList.push_back(folder);
	}

	this->backPriorityFlag = (BOOL)GetPrivateProfileInt(L"SET", L"BackPriority", 1, iniAppPath.c_str());
	this->sameChPriorityFlag = (BOOL)GetPrivateProfileInt(L"SET", L"SameChPriority", 0, iniAppPath.c_str());

	this->eventRelay = (BOOL)GetPrivateProfileInt(L"SET", L"EventRelay", 0, iniAppPath.c_str());

	wstring ch5Path = L"";
	GetSettingPath(ch5Path);
	ch5Path += L"\\ChSet5.txt";

	chUtil.ParseText(ch5Path.c_str());

	this->tvtestUseBon.clear();
	count = GetPrivateProfileInt(L"TVTEST", L"Num", 0, iniAppPath.c_str());
	for( int i=0; i<count; i++ ){
		wstring key;
		Format(key, L"%d", i);
		WCHAR buff[256] = L"";
		GetPrivateProfileString(L"TVTEST", key.c_str(), L"", buff, 256, iniAppPath.c_str());
		wstring bon = buff;
		if( bon.size() > 0 ){
			this->tvtestUseBon.push_back(bon);
		}
	}

	this->defStartMargine = GetPrivateProfileInt(L"SET", L"StartMargin", 5, iniAppPath.c_str());
	this->defEndMargine = GetPrivateProfileInt(L"SET", L"EndMargin", 2, iniAppPath.c_str());
	this->notFindTuijyuHour = GetPrivateProfileInt(L"SET", L"TuijyuHour", 6, iniAppPath.c_str());
	this->duraChgMarginMin = GetPrivateProfileInt(L"SET", L"DuraChgMarginMin", 0, iniAppPath.c_str());
	this->noEpgTuijyuMin = GetPrivateProfileInt(L"SET", L"NoEpgTuijyuMin", 30, iniAppPath.c_str());

	map<DWORD, CTunerBankCtrl*>::iterator itrCtrl;
	for( itrCtrl = this->tunerBankMap.begin(); itrCtrl != this->tunerBankMap.end(); itrCtrl++ ){
		itrCtrl->second->ReloadSetting();
		itrCtrl->second->SetAutoDel(this->autoDel, &this->delExtList, &this->delFolderList); 
	}

	WCHAR buff[512] = L"";
	GetPrivateProfileString( L"SET", L"RecExePath", L"", buff, 512, iniCommonPath.c_str() );
	this->recExePath = buff;
	if( this->recExePath.size() == 0 ){
		GetModuleFolderPath(this->recExePath);
		this->recExePath += L"\\EpgDataCap_Bon.exe";
	}

	this->autoDelRecInfo = GetPrivateProfileInt(L"SET", L"AutoDelRecInfo", 0, iniAppPath.c_str());
	this->autoDelRecInfoNum = GetPrivateProfileInt(L"SET", L"AutoDelRecInfoNum", 100, iniAppPath.c_str());

	this->timeSync = GetPrivateProfileInt(L"SET", L"TimeSync", 0, iniAppPath.c_str());

	recInfoText.SetKeepCount(this->autoDelRecInfo == FALSE ? UINT_MAX : this->autoDelRecInfoNum);
	recInfoText.SetRecInfoDelFile(GetPrivateProfileInt(L"SET", L"RecInfoDelFile", 0, iniCommonPath.c_str()) != 0);
	ZeroMemory(buff, sizeof(WCHAR)*512);
	GetPrivateProfileString(L"SET", L"RecInfoFolder", L"", buff, 512, iniCommonPath.c_str());
	recInfoText.SetRecInfoFolder(buff);

	this->recInfo2Text.SetKeepCount(GetPrivateProfileInt(L"SET", L"RecInfo2Max", 1000, iniAppPath.c_str()));
	this->recInfo2DropChk = GetPrivateProfileInt(L"SET", L"RecInfo2DropChk", 15, iniAppPath.c_str());
	WCHAR buffRecInfo2RegExp[1024];
	GetPrivateProfileString(L"SET", L"RecInfo2RegExp", L"", buffRecInfo2RegExp, 1024, iniAppPath.c_str());
	this->recInfo2RegExp = buffRecInfo2RegExp;

	this->useTweet = GetPrivateProfileInt(L"TWITTER", L"use", 0, iniAppPath.c_str());
	this->useProxy = GetPrivateProfileInt(L"TWITTER", L"useProxy", 0, iniAppPath.c_str());
	ZeroMemory(buff, sizeof(WCHAR)*512);
	GetPrivateProfileStringW(L"TWITTER", L"ProxyServer", L"", buff, 512, iniAppPath.c_str());
	this->proxySrv = buff;
	ZeroMemory(buff, sizeof(WCHAR)*512);
	GetPrivateProfileStringW(L"TWITTER", L"ProxyID", L"", buff, 512, iniAppPath.c_str());
	this->proxyID = buff;
	ZeroMemory(buff, sizeof(WCHAR)*512);
	GetPrivateProfileStringW(L"TWITTER", L"ProxyPWD", L"", buff, 512, iniAppPath.c_str());
	this->proxyPWD = buff;

	USE_PROXY_INFO proxyInfo;
	if( this->useProxy == TRUE ){
		if( this->proxySrv.size() > 0 ){
			proxyInfo.serverName = new WCHAR[this->proxySrv.size()+1];
			wcscpy_s(proxyInfo.serverName, this->proxySrv.size()+1, this->proxySrv.c_str());
		}
		if( this->proxyID.size() > 0 ){
			proxyInfo.userName = new WCHAR[this->proxyID.size()+1];
			wcscpy_s(proxyInfo.userName, this->proxyID.size()+1, this->proxyID.c_str());
		}
		if( this->proxyPWD.size() > 0 ){
			proxyInfo.serverName = new WCHAR[this->proxyPWD.size()+1];
			wcscpy_s(proxyInfo.password, this->proxyPWD.size()+1, this->proxyPWD.c_str());
		}
	}
	if( this->useTweet == TRUE ){
		if( this->twitterManager == NULL ){
			this->twitterManager = new CTwitterManager;
		}
		this->twitterManager->SetProxy(this->useProxy, &proxyInfo);
	}

	map<DWORD, CTunerBankCtrl*>::iterator itr;
	for(itr = this->tunerBankMap.begin(); itr != this->tunerBankMap.end(); itr++ ){
		if(this->useTweet == 1 ){
			itr->second->SetTwitterCtrl(this->twitterManager);
		}else{
			itr->second->SetTwitterCtrl(NULL);
		}
	}

	this->reloadBankMapAlgo = GetPrivateProfileInt(L"SET", L"ReloadBankMapAlgo", 0, iniAppPath.c_str());

	this->ngAddResSrvCoop = GetPrivateProfileInt(L"SET", L"NgAddResSrvCoop", 0, iniAppPath.c_str());
	if( GetPrivateProfileInt(L"SET", L"UseSrvCoop", 0, iniAppPath.c_str()) != TRUE ){
		this->ngAddResSrvCoop = TRUE;
	}

	this->errEndBatRun = GetPrivateProfileInt(L"SET", L"ErrEndBatRun", 0, iniAppPath.c_str());

	this->recEndTweetErr = GetPrivateProfileInt(L"SET", L"RecEndTweetErr", 0, iniAppPath.c_str());
	this->recEndTweetDrop = GetPrivateProfileInt(L"SET", L"RecEndTweetDrop", 0, iniAppPath.c_str());

	this->useRecNamePlugIn = GetPrivateProfileInt(L"SET", L"RecNamePlugIn", 0, iniAppPath.c_str());
	GetPrivateProfileString(L"SET", L"RecNamePlugInFile", L"RecName_Macro.dll", buff, 512, iniAppPath.c_str());

	GetModuleFolderPath(this->recNamePlugInFilePath);
	this->recNamePlugInFilePath += L"\\RecName\\";
	this->recNamePlugInFilePath += buff;

	IsFindShareTSFile();
	UnLock();
}

void CReserveManager::SendNotifyUpdate(DWORD notifyId)
{
	if( Lock(L"SendNotifyUpdate") == FALSE ) return;

	_SendNotifyUpdate(notifyId);

	UnLock();
}

void CReserveManager::SendNotifyChgReserveAutoAdd(RESERVE_DATA* oldInfo, RESERVE_DATA* newInfo)
{
	if( Lock(L"SendNotifyUpdate") == FALSE ) return;

	_SendNotifyChgReserve(NOTIFY_UPDATE_CHG_TUIJYU, oldInfo, newInfo);

	UnLock();
}

void CReserveManager::_SendNotifyUpdate(DWORD notifyId)
{
	if( this->notifyManager != NULL ){
		this->notifyManager->AddNotify(notifyId);
	}
}


void CReserveManager::_SendNotifyStatus(WORD status)
{
	this->notifyStatus = status;
	if( this->notifyManager != NULL ){
		this->notifyManager->AddNotifySrvStatus(this->notifyStatus);
	}
}

void CReserveManager::_SendNotifyRecEnd(REC_FILE_INFO* item)
{
	if( this->notifyManager != NULL ){
		SYSTEMTIME endTime;
		GetSumTime(item->startTime, item->durationSecond, &endTime);
		wstring msg;
		Format(msg, L"%s %04d/%02d/%02d %02d:%02d～%02d:%02d\r\n%s\r\n%s",
			item->serviceName.c_str(),
			item->startTime.wYear,
			item->startTime.wMonth,
			item->startTime.wDay,
			item->startTime.wHour,
			item->startTime.wMinute,
			endTime.wHour,
			endTime.wMinute,
			item->title.c_str(),
			item->comment.c_str()
			);
		this->notifyManager->AddNotifyMsg(NOTIFY_UPDATE_REC_END, msg);
	}
}

void CReserveManager::_SendNotifyChgReserve(DWORD notifyId, RESERVE_DATA* oldInfo, RESERVE_DATA* newInfo)
{
	if( this->notifyManager != NULL ){
		SYSTEMTIME endTimeOld;
		GetSumTime(oldInfo->startTime, oldInfo->durationSecond, &endTimeOld);
		SYSTEMTIME endTimeNew;
		GetSumTime(newInfo->startTime, newInfo->durationSecond, &endTimeNew);
		wstring msg;
		Format(msg, L"%s %04d/%02d/%02d %02d:%02d～%02d:%02d\r\n%s\r\nEventID:0x%04X\r\n↓\r\n%s %04d/%02d/%02d %02d:%02d～%02d:%02d\r\n%s\r\nEventID:0x%04X\r\n",
			oldInfo->stationName.c_str(),
			oldInfo->startTime.wYear,
			oldInfo->startTime.wMonth,
			oldInfo->startTime.wDay,
			oldInfo->startTime.wHour,
			oldInfo->startTime.wMinute,
			endTimeOld.wHour,
			endTimeOld.wMinute,
			oldInfo->title.c_str(),
			oldInfo->eventID,
			newInfo->stationName.c_str(),
			newInfo->startTime.wYear,
			newInfo->startTime.wMonth,
			newInfo->startTime.wDay,
			newInfo->startTime.wHour,
			newInfo->startTime.wMinute,
			endTimeNew.wHour,
			endTimeNew.wMinute,
			newInfo->title.c_str(),
			newInfo->eventID
			);
		this->notifyManager->AddNotifyMsg(notifyId, msg);
	}
}

/*
UINT WINAPI CReserveManager::SendNotifyThread(LPVOID param)
{
	CReserveManager* sys = (CReserveManager*)param;
	CSendCtrlCmd sendCtrl;
	map<DWORD,DWORD>::iterator itr;

	if( sys->NotifyLock() == FALSE ) return 0;

	vector<DWORD> errID;
	for( itr = sys->registGUIMap.begin(); itr != sys->registGUIMap.end(); itr++){
		if( ::WaitForSingleObject(sys->notifyStopEvent, 0) != WAIT_TIMEOUT ){
			//キャンセルされた
			break;
		}
		if( _FindOpenExeProcess(itr->first) == TRUE ){
			wstring pipe;
			wstring waitEvent;
			Format(pipe, L"%s%d", CMD2_GUI_CTRL_PIPE, itr->first);
			Format(waitEvent, L"%s%d", CMD2_GUI_CTRL_WAIT_CONNECT, itr->first);

			sendCtrl.SetPipeSetting(waitEvent, pipe);
			sendCtrl.SetConnectTimeOut(5*1000);
			if( sendCtrl.SendGUIUpdateReserve() != CMD_SUCCESS ){
				errID.push_back(itr->first);
			}
		}else{
			errID.push_back(itr->first);
		}
	}
	for( size_t i=0; i<errID.size(); i++ ){
		itr = sys->registGUIMap.find(errID[i]);
		if( itr != sys->registGUIMap.end() ){
			sys->registGUIMap.erase(itr);
		}
	}

	map<wstring, REGIST_TCP_INFO>::iterator itrTCP;
	vector<wstring> errIP;
	for( itrTCP = sys->registTCPMap.begin(); itrTCP != sys->registTCPMap.end(); itrTCP++){
		if( ::WaitForSingleObject(sys->notifyStopEvent, 0) != WAIT_TIMEOUT ){
			//キャンセルされた
			break;
		}

		sendCtrl.SetSendMode(TRUE);
		sendCtrl.SetNWSetting(itrTCP->second.ip, itrTCP->second.port);
		sendCtrl.SetConnectTimeOut(5*1000);
		if( sendCtrl.SendGUIUpdateReserve() != CMD_SUCCESS ){
			errIP.push_back(itrTCP->first);
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

	return 0;
}

void CReserveManager::SendNotifyEpgReload()
{
	if( Lock(L"SendNotifyEpgReload") == FALSE ) return;

	_SendNotifyEpgReload();

	UnLock();
}

void CReserveManager::_SendNotifyEpgReload()
{
	if( this->notifyEpgReloadThread != NULL ){
		if( ::WaitForSingleObject(this->notifyEpgReloadThread, 0) == WAIT_OBJECT_0 ){
			CloseHandle(this->notifyEpgReloadThread);
			this->notifyEpgReloadThread = NULL;
		}
	}
	if( this->notifyEpgReloadThread == NULL ){
		ResetEvent(this->notifyEpgReloadStopEvent);
		this->notifyEpgReloadThread = (HANDLE)_beginthreadex(NULL, 0, SendNotifyEpgReloadThread, (LPVOID)this, CREATE_SUSPENDED, NULL);
		SetThreadPriority( this->notifyEpgReloadThread, THREAD_PRIORITY_NORMAL );
		ResumeThread(this->notifyEpgReloadThread);
	}
}


UINT WINAPI CReserveManager::SendNotifyEpgReloadThread(LPVOID param)
{
	CReserveManager* sys = (CReserveManager*)param;
	CSendCtrlCmd sendCtrl;
	map<DWORD,DWORD>::iterator itr;

	if( sys->NotifyLock() == FALSE ) return 0;

	vector<DWORD> errID;
	for( itr = sys->registGUIMap.begin(); itr != sys->registGUIMap.end(); itr++){
		if( ::WaitForSingleObject(sys->notifyStopEvent, 0) != WAIT_TIMEOUT ){
			//キャンセルされた
			break;
		}
		if( _FindOpenExeProcess(itr->first) == TRUE ){
			wstring pipe;
			wstring waitEvent;
			Format(pipe, L"%s%d", CMD2_GUI_CTRL_PIPE, itr->first);
			Format(waitEvent, L"%s%d", CMD2_GUI_CTRL_WAIT_CONNECT, itr->first);

			sendCtrl.SetPipeSetting(waitEvent, pipe);
			sendCtrl.SetConnectTimeOut(5*1000);
			if( sendCtrl.SendGUIUpdateEpgData() != CMD_SUCCESS ){
				errID.push_back(itr->first);
			}
		}else{
			errID.push_back(itr->first);
		}
	}
	for( size_t i=0; i<errID.size(); i++ ){
		itr = sys->registGUIMap.find(errID[i]);
		if( itr != sys->registGUIMap.end() ){
			sys->registGUIMap.erase(itr);
		}
	}

	map<wstring, REGIST_TCP_INFO>::iterator itrTCP;
	vector<wstring> errIP;
	for( itrTCP = sys->registTCPMap.begin(); itrTCP != sys->registTCPMap.end(); itrTCP++){
		if( ::WaitForSingleObject(sys->notifyStopEvent, 0) != WAIT_TIMEOUT ){
			//キャンセルされた
			break;
		}

		sendCtrl.SetSendMode(TRUE);
		sendCtrl.SetNWSetting(itrTCP->second.ip, itrTCP->second.port);
		sendCtrl.SetConnectTimeOut(5*1000);
		if( sendCtrl.SendGUIUpdateEpgData() != CMD_SUCCESS ){
			errIP.push_back(itrTCP->first);
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

	return 0;
}

void CReserveManager::SendNotifyStatus(WORD status)
{
	if( this->notifyStatusThread != NULL ){
		if( ::WaitForSingleObject(this->notifyStatusThread, 0) == WAIT_OBJECT_0 ){
			CloseHandle(this->notifyStatusThread);
			this->notifyStatusThread = NULL;
		}
	}
	if( this->notifyStatusThread == NULL ){
		this->notifyStatus = status;
		ResetEvent(this->notifyStatusStopEvent);
		this->notifyStatusThread = (HANDLE)_beginthreadex(NULL, 0, SendNotifyStatusThread, (LPVOID)this, CREATE_SUSPENDED, NULL);
		SetThreadPriority( this->notifyStatusThread, THREAD_PRIORITY_NORMAL );
		ResumeThread(this->notifyStatusThread);
	}
}

UINT WINAPI CReserveManager::SendNotifyStatusThread(LPVOID param)
{
	CReserveManager* sys = (CReserveManager*)param;
	CSendCtrlCmd sendCtrl;
	map<DWORD,DWORD>::iterator itr;

	if( sys->NotifyLock() == FALSE ) return 0;

	vector<DWORD> errID;
	for( itr = sys->registGUIMap.begin(); itr != sys->registGUIMap.end(); itr++){
		if( ::WaitForSingleObject(sys->notifyStatusStopEvent, 0) != WAIT_TIMEOUT ){
			//キャンセルされた
			break;
		}
		if( _FindOpenExeProcess(itr->first) == TRUE ){
			wstring pipe;
			wstring waitEvent;
			Format(pipe, L"%s%d", CMD2_GUI_CTRL_PIPE, itr->first);
			Format(waitEvent, L"%s%d", CMD2_GUI_CTRL_WAIT_CONNECT, itr->first);

			sendCtrl.SetPipeSetting(waitEvent, pipe);
			sendCtrl.SetConnectTimeOut(5*1000);
			if( sendCtrl.SendGUIStatusChg(sys->notifyStatus) != CMD_SUCCESS ){
				errID.push_back(itr->first);
			}
		}else{
			errID.push_back(itr->first);
		}
	}
	for( size_t i=0; i<errID.size(); i++ ){
		itr = sys->registGUIMap.find(errID[i]);
		if( itr != sys->registGUIMap.end() ){
			sys->registGUIMap.erase(itr);
		}
	}
	
	map<wstring, REGIST_TCP_INFO>::iterator itrTCP;
	vector<wstring> errIP;
	for( itrTCP = sys->registTCPMap.begin(); itrTCP != sys->registTCPMap.end(); itrTCP++){
		if( ::WaitForSingleObject(sys->notifyStopEvent, 0) != WAIT_TIMEOUT ){
			//キャンセルされた
			break;
		}

		sendCtrl.SetSendMode(TRUE);
		sendCtrl.SetNWSetting(itrTCP->second.ip, itrTCP->second.port);
		sendCtrl.SetConnectTimeOut(5*1000);
		if( sendCtrl.SendGUIStatusChg(sys->notifyStatus) != CMD_SUCCESS ){
			errIP.push_back(itrTCP->first);
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

	return 0;
}
*/

BOOL CReserveManager::ReloadReserveData()
{
	if( Lock(L"ReloadReserveData") == FALSE ) return FALSE;
	BOOL ret = TRUE;

	wstring reserveFilePath = L"";
	GetSettingPath(reserveFilePath);
	reserveFilePath += L"\\";
	reserveFilePath += RESERVE_TEXT_NAME;

	wstring recInfoFilePath = L"";
	GetSettingPath(recInfoFilePath);
	recInfoFilePath += L"\\";
	recInfoFilePath += REC_INFO_TEXT_NAME;

	wstring recInfo2FilePath = L"";
	GetSettingPath(recInfo2FilePath);
	recInfo2FilePath += L"\\";
	recInfo2FilePath += REC_INFO2_TEXT_NAME;

	map<DWORD, CReserveInfo*>::iterator itr;
	for( itr = this->reserveInfoMap.begin(); itr != this->reserveInfoMap.end(); itr++ ){
		SAFE_DELETE(itr->second);
	}
	this->reserveInfoMap.clear();
	this->reserveInfoIDMap.clear();

	this->recInfoText.ParseText(recInfoFilePath.c_str());
	this->chgRecInfo = TRUE;

	this->recInfo2Text.ParseText(recInfo2FilePath.c_str());

	vector<DWORD> deleteList;
	LONGLONG nowTime = GetNowI64Time();

	ret = this->reserveText.ParseText(reserveFilePath.c_str());
	if( ret == TRUE ){
		map<DWORD, RESERVE_DATA>::const_iterator itrData;
		for( itrData = this->reserveText.GetMap().begin(); itrData != this->reserveText.GetMap().end(); itrData++){
			LONGLONG chkEndTime;
			CalcEntireReserveTime(NULL, &chkEndTime, itrData->second);
			chkEndTime -= 60*I64_1SEC;

			if( nowTime < chkEndTime ){
				CReserveInfo* item = new CReserveInfo;
				item->SetData(const_cast<RESERVE_DATA*>(&itrData->second));

				//サービスサポートしてないチューナー検索
				if( itrData->second.recSetting.tunerID == 0 ){
					vector<DWORD> idList;
					if( this->tunerManager.GetNotSupportServiceTuner(
						itrData->second.originalNetworkID,
						itrData->second.transportStreamID,
						itrData->second.serviceID,
						&idList ) == TRUE ){
							item->SetNGChTunerID(&idList);
					}
				}else{
					//チューナー固定
					vector<DWORD> idList;
					if( this->tunerManager.GetEnumID( &idList ) == TRUE ){
						for( size_t i=0; i<idList.size(); i++ ){
							if( idList[i] != itrData->second.recSetting.tunerID ){
								item->AddNGTunerID(idList[i]);
							}
						}
					}
				}
				
				this->reserveInfoMap.insert(pair<DWORD, CReserveInfo*>(itrData->second.reserveID, item));
				LONGLONG keyID = _Create64Key2(
					itrData->second.originalNetworkID,
					itrData->second.transportStreamID,
					itrData->second.serviceID,
					itrData->second.eventID);
				this->reserveInfoIDMap.insert(pair<LONGLONG, DWORD>(keyID, itrData->second.reserveID));

			}else{
				//時間過ぎているので失敗
				deleteList.push_back(itrData->second.reserveID);
				REC_FILE_INFO item;
				item = itrData->second;
				if( itrData->second.recSetting.recMode != RECMODE_NO ){
					item.recStatus = REC_END_STATUS_START_ERR;
					item.comment = L"録画時間に起動していなかった可能性があります";
					this->recInfoText.AddRecInfo(item);
				}
			}
		}
	}

	if( deleteList.size() > 0 ){
		for( size_t i=0; i<deleteList.size(); i++ ){
			this->reserveText.DelReserve(deleteList[i]);
		}
		this->reserveText.SaveText();
		this->recInfoText.SaveText();
		this->chgRecInfo = TRUE;
	}

	if( this->bankCheckThread != NULL ){
		if( ::WaitForSingleObject(this->bankCheckThread, 0) == WAIT_OBJECT_0 ){
			CloseHandle(this->bankCheckThread);
			this->bankCheckThread = NULL;
		}
	}
	if( this->bankCheckThread == NULL ){
		ResetEvent(this->bankCheckStopEvent);
		this->bankCheckThread = (HANDLE)_beginthreadex(NULL, 0, BankCheckThread, (LPVOID)this, CREATE_SUSPENDED, NULL);
		SetThreadPriority( this->bankCheckThread, THREAD_PRIORITY_NORMAL );
		ResumeThread(this->bankCheckThread);
	}

	UnLock();
	return ret;
}

//予約情報を取得する
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// reserveList		[OUT]予約情報一覧（呼び出し元で解放する必要あり）
BOOL CReserveManager::GetReserveDataAll(
	vector<RESERVE_DATA*>* reserveList
	)
{
	if( Lock(L"GetReserveDataAll") == FALSE ) return FALSE;
	BOOL ret = TRUE;

	map<DWORD, TUNER_RESERVE_INFO> tunerResMap;
	map<DWORD, BANK_INFO*>::iterator itrBank;
	for( itrBank = this->bankMap.begin(); itrBank != this->bankMap.end(); itrBank++ ){
		TUNER_RESERVE_INFO item;
		item.tunerID = itrBank->second->tunerID;
		this->tunerManager.GetBonFileName(item.tunerID, item.tunerName);

		map<DWORD, BANK_WORK_INFO*>::iterator itrInfo;
		for( itrInfo = itrBank->second->reserveList.begin(); itrInfo != itrBank->second->reserveList.end(); itrInfo++){
			tunerResMap.insert(pair<DWORD, TUNER_RESERVE_INFO>(itrInfo->second->reserveID, item));
		}
	}

	TUNER_RESERVE_INFO itemNg;
	itemNg.tunerID = 0xFFFFFFFF;
	itemNg.tunerName = L"チューナー不足";
	map<DWORD, BANK_WORK_INFO*>::iterator itrNg;
	for( itrNg = this->NGReserveMap.begin(); itrNg != this->NGReserveMap.end(); itrNg++ ){
		tunerResMap.insert(pair<DWORD, TUNER_RESERVE_INFO>(itrNg->second->reserveID, itemNg));
	}


	map<DWORD, CReserveInfo*>::iterator itr;
	for( itr = this->reserveInfoMap.begin(); itr != this->reserveInfoMap.end(); itr++ ){
		RESERVE_DATA* item = new RESERVE_DATA;
		itr->second->GetData(item);

		map<DWORD, TUNER_RESERVE_INFO>::iterator itrTune;
		itrTune = tunerResMap.find(item->reserveID);
		if( itrTune != tunerResMap.end() ){
			wstring defName = L"";

			PLUGIN_RESERVE_INFO info;

			info.startTime = item->startTime;
			info.durationSec = item->durationSecond;
			wcscpy_s(info.eventName, 512, item->title.c_str());
			info.ONID = item->originalNetworkID;
			info.TSID = item->transportStreamID;
			info.SID = item->serviceID;
			info.EventID = item->eventID;
			wcscpy_s(info.serviceName, 256, item->stationName.c_str());
			wcscpy_s(info.bonDriverName, 256, itrTune->second.tunerName.c_str());
			info.bonDriverID = (itrTune->second.tunerID & 0xFFFF0000)>>16;
			info.tunerID = itrTune->second.tunerID & 0x0000FFFF;

			EPG_EVENT_INFO* epgInfo = NULL;
			EPGDB_EVENT_INFO epgDBInfo;
			if( this->epgDBManager != NULL && info.EventID != 0xFFFF ){
				if( this->epgDBManager->SearchEpg(info.ONID, info.TSID, info.SID, info.EventID, &epgDBInfo) == TRUE ){
					epgInfo = new EPG_EVENT_INFO;
					CopyEpgInfo(epgInfo, &epgDBInfo);
				}
			}

			if( this->useRecNamePlugIn == TRUE ){
				CReNamePlugInUtil plugIn;
				if( plugIn.Initialize(this->recNamePlugInFilePath.c_str()) == TRUE ){
					WCHAR name[512] = L"";
					DWORD size = 512;

					if( epgInfo != NULL ){
						if( plugIn.ConvertRecName2(&info, epgInfo, name, &size) == TRUE ){
							defName = name;
						}
					}else{
						if( plugIn.ConvertRecName(&info, name, &size) == TRUE ){
							defName = name;
						}
					}
				}
			}
			if( defName.size() ==0 ){
				Format(defName, L"%04d%02d%02d%02d%02d%02X%02X%02d-%s.ts",
					item->startTime.wYear,
					item->startTime.wMonth,
					item->startTime.wDay,
					item->startTime.wHour,
					item->startTime.wMinute,
					((itrTune->second.tunerID & 0xFFFF0000)>>16),
					(itrTune->second.tunerID & 0x0000FFFF),
					0,
					item->title.c_str()
					);
			}
			if( item->recSetting.recFolderList.size() == 0 ){
				item->recFileNameList.push_back(defName);
			}else{
				for( size_t i=0; i<item->recSetting.recFolderList.size(); i++ ){
					if( item->recSetting.recFolderList[i].recNamePlugIn.size() > 0){
						CReNamePlugInUtil plugIn;
						wstring plugInPath = L"";
						GetModuleFolderPath(plugInPath);
						plugInPath += L"\\RecName\\";
						plugInPath += item->recSetting.recFolderList[i].recNamePlugIn;
						if( plugIn.Initialize(plugInPath.c_str()) == TRUE ){
							WCHAR name[512] = L"";
							DWORD size = 512;

							if( epgInfo != NULL ){
								if( plugIn.ConvertRecName2(&info, epgInfo, name, &size) == TRUE ){
									item->recFileNameList.push_back(name);
								}
							}else{
								if( plugIn.ConvertRecName(&info, name, &size) == TRUE ){
									item->recFileNameList.push_back(name);
								}
							}
						}
					}else{
						item->recFileNameList.push_back(defName);
					}
				}
			}
			SAFE_DELETE(epgInfo);
		}
		reserveList->push_back(item);
	}

	UnLock();
	return ret;
}

//チューナー毎の予約情報を取得する
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// reserveList		[OUT]予約情報一覧
BOOL CReserveManager::GetTunerReserveAll(
	vector<TUNER_RESERVE_INFO>* list
	)
{
	if( Lock(L"GetTunerReserveAll") == FALSE ) return FALSE;
	BOOL ret = TRUE;

	map<DWORD, BANK_INFO*>::iterator itr;
	for( itr = this->bankMap.begin(); itr != this->bankMap.end(); itr++ ){
		TUNER_RESERVE_INFO item;
		item.tunerID = itr->second->tunerID;
		this->tunerManager.GetBonFileName(item.tunerID, item.tunerName);

		map<DWORD, BANK_WORK_INFO*>::iterator itrInfo;
		for( itrInfo = itr->second->reserveList.begin(); itrInfo != itr->second->reserveList.end(); itrInfo++){
			item.reserveList.push_back(itrInfo->second->reserveID);
		}
		list->push_back(item);
	}

	TUNER_RESERVE_INFO item;
	item.tunerID = 0xFFFFFFFF;
	item.tunerName = L"チューナー不足";
	map<DWORD, BANK_WORK_INFO*>::iterator itrNg;
	for( itrNg = this->NGReserveMap.begin(); itrNg != this->NGReserveMap.end(); itrNg++ ){
		item.reserveList.push_back(itrNg->second->reserveID);
	}
	list->push_back(item);

	UnLock();
	return ret;
}

//予約情報を取得する
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// id				[IN]予約ID
// reserveData		[OUT]予約情報
BOOL CReserveManager::GetReserveData(
	DWORD id,
	RESERVE_DATA* reserveData
	)
{
	if( Lock(L"GetReserveData") == FALSE ) return FALSE;
	BOOL ret = TRUE;


	map<DWORD, CReserveInfo*>::iterator itr;
	itr = this->reserveInfoMap.find(id);
	if( itr == this->reserveInfoMap.end() ){
		ret = FALSE;
	}else{
		itr->second->GetData(reserveData);
	}

	UnLock();
	return ret;
}

//予約情報を追加する
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// reserveList		[IN]予約情報
BOOL CReserveManager::AddReserveData(
	vector<RESERVE_DATA>* reserveList,
	BOOL tweet
	)
{
	if( Lock(L"AddReserveData") == FALSE ) return FALSE;
	BOOL ret = TRUE;

	//予約追加
	BOOL add = FALSE;
	for( size_t i=0; i<reserveList->size(); i++ ){
		if( _AddReserveData(&(*reserveList)[i], tweet) == TRUE ){
			add = TRUE;
		}
	}
	if( add == FALSE ){
		//追加成功したものがない
		UnLock();
		return FALSE;
	}

	this->reserveText.SaveText();

	_ReloadBankMap();


	_SendNotifyUpdate(NOTIFY_UPDATE_RESERVE_INFO);

	UnLock();
	return ret;
}

BOOL CReserveManager::_AddReserveData(RESERVE_DATA* reserve, BOOL tweet)
{
	BOOL ret = TRUE;

	DWORD reserveID = 0;
	if( reserve->eventID == 0xFFFF ){
		reserve->recSetting.pittariFlag = 0;
		reserve->recSetting.tuijyuuFlag = 0;
	}
	SYSTEMTIME now, endTime;
	GetLocalTime(&now);
	GetI64Time(reserve->startTime, reserve->durationSecond, NULL, NULL, &endTime);
	if( IsBigTime(now, endTime) != FALSE ){
		//すでに終了している
		return FALSE;
	}
	reserveID = this->reserveText.AddReserve(*reserve);

	map<DWORD, RESERVE_DATA>::const_iterator itrData;
	itrData = this->reserveText.GetMap().find(reserveID);
	if( itrData != this->reserveText.GetMap().end() ){
		if( this->reserveInfoMap.find(itrData->second.reserveID) == this->reserveInfoMap.end() ){
			CReserveInfo* item = new CReserveInfo;
			item->SetData(const_cast<RESERVE_DATA*>(&itrData->second));

			if( itrData->second.recSetting.tunerID == 0 ){
				//サービスサポートしてないチューナー検索
				vector<DWORD> idList;
				if( this->tunerManager.GetNotSupportServiceTuner(
					itrData->second.originalNetworkID,
					itrData->second.transportStreamID,
					itrData->second.serviceID,
					&idList ) == TRUE ){
						item->SetNGChTunerID(&idList);
				}
			}else{
				//チューナー固定
				vector<DWORD> idList;
				if( this->tunerManager.GetEnumID( &idList ) == TRUE ){
					for( size_t i=0; i<idList.size(); i++ ){
						if( idList[i] != itrData->second.recSetting.tunerID ){
							item->AddNGTunerID(idList[i]);
						}
					}
				}
			}

			this->reserveInfoMap.insert(pair<DWORD, CReserveInfo*>(itrData->second.reserveID, item));
			LONGLONG keyID = _Create64Key2(
				itrData->second.originalNetworkID,
				itrData->second.transportStreamID,
				itrData->second.serviceID,
				itrData->second.eventID);
			this->reserveInfoIDMap.insert(pair<LONGLONG, DWORD>(keyID, itrData->second.reserveID));

			if( tweet == TRUE && itrData->second.recSetting.recMode != RECMODE_NO){
				RESERVE_DATA resTweet = itrData->second;
				_SendTweet(TW_ADD_RESERVE, &resTweet, NULL, NULL);
			}
		}
	}

	return ret;
}

//予約情報を変更する
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// reserveList		[IN]予約情報
BOOL CReserveManager::ChgReserveData(
	vector<RESERVE_DATA>* reserveList,
	BOOL timeChg
	)
{
	if( Lock(L"ChgReserveData") == FALSE ) return FALSE;
	BOOL ret = TRUE;

	//予約変更
	BOOL chg = FALSE;
	for( size_t i=0; i<reserveList->size(); i++ ){
		if( _ChgReserveData(&(*reserveList)[i], timeChg) == TRUE ){
			chg = TRUE;
		}
	}
	if( chg == FALSE ){
		//変更成功したものがない
		UnLock();
		return FALSE;
	}

	this->reserveText.SaveText();

	_ReloadBankMap();

	_SendNotifyUpdate(NOTIFY_UPDATE_RESERVE_INFO);

	UnLock();
	return ret;
}

BOOL CReserveManager::_ChgReserveData(RESERVE_DATA* reserve, BOOL chgTime)
{
	if( reserve == NULL ){
		return FALSE;
	}
	if( reserve->eventID == 0xFFFF ){
		reserve->recSetting.pittariFlag = 0;
		reserve->recSetting.tuijyuuFlag = 0;
	}

	map<DWORD, CReserveInfo*>::iterator itrInfo;
	itrInfo = this->reserveInfoMap.find(reserve->reserveID);
	if( itrInfo == this->reserveInfoMap.end() ){
		return FALSE;
	}
	RESERVE_DATA setData;
	itrInfo->second->GetData(&setData);

	BOOL recWaitFlag = FALSE;
	DWORD tunerID = 0;
	itrInfo->second->GetRecWaitMode(&recWaitFlag, &tunerID );

	BOOL chgCtrl = FALSE;
	if( recWaitFlag == TRUE ){
		//すでに録画中
		setData.recSetting.tuijyuuFlag = reserve->recSetting.tuijyuuFlag;
		setData.recSetting.batFilePath = reserve->recSetting.batFilePath;
		setData.recSetting.suspendMode = reserve->recSetting.suspendMode;
		setData.recSetting.rebootFlag = reserve->recSetting.rebootFlag;
		setData.recSetting.useMargineFlag = reserve->recSetting.useMargineFlag;
		setData.recSetting.endMargine = reserve->recSetting.endMargine;
		if( chgTime == TRUE ){
			//追従による時間変更
			setData.startTime = reserve->startTime;
			setData.durationSecond = reserve->durationSecond;
			setData.reserveStatus = reserve->reserveStatus;
			setData.eventID = reserve->eventID;

			//コントロール経由で変更
			map<DWORD, CTunerBankCtrl*>::iterator itrCtrl;
			itrCtrl = this->tunerBankMap.find(tunerID);
			if( itrCtrl != this->tunerBankMap.end() ){
				itrCtrl->second->ChgReserve(&setData);
			}

			chgCtrl = TRUE;
		}else{
			if( reserve->eventID == 0xFFFF ){
				setData.durationSecond = reserve->durationSecond;
			}
		}
		
	}else{
		if( setData.eventID == 0xFFFF ){
			//プログラム予約の場合チャンネルが変わっている可能性あるためチェック
			if( setData.originalNetworkID != reserve->originalNetworkID ||
				setData.transportStreamID != reserve->transportStreamID ||
				setData.serviceID != reserve->serviceID ){
					//チャンネル変わってる
					itrInfo->second->ClearAddNGTuner();
					if( reserve->recSetting.tunerID == 0 ){
						//サービスサポートしてないチューナー検索
						vector<DWORD> idList;
						if( this->tunerManager.GetNotSupportServiceTuner(
							reserve->originalNetworkID,
							reserve->transportStreamID,
							reserve->serviceID,
							&idList ) == TRUE ){
								itrInfo->second->SetNGChTunerID(&idList);
						}
					}else{
						//チューナー固定
						vector<DWORD> idList;
						if( this->tunerManager.GetEnumID( &idList ) == TRUE ){
							for( size_t i=0; i<idList.size(); i++ ){
								if( idList[i] != reserve->recSetting.tunerID ){
									itrInfo->second->AddNGTunerID(idList[i]);
								}
							}
						}
					}
			}
		}
		if( setData.recSetting.tunerID != reserve->recSetting.tunerID ){
			//使用チューナーリセット
			itrInfo->second->ClearAddNGTuner();
			if( reserve->recSetting.tunerID == 0 ){
				//サービスサポートしてないチューナー検索
				vector<DWORD> idList;
				if( this->tunerManager.GetNotSupportServiceTuner(
					reserve->originalNetworkID,
					reserve->transportStreamID,
					reserve->serviceID,
					&idList ) == TRUE ){
						itrInfo->second->SetNGChTunerID(&idList);
				}
			}else{
				//チューナー固定
				vector<DWORD> idList;
				if( this->tunerManager.GetEnumID( &idList ) == TRUE ){
					for( size_t i=0; i<idList.size(); i++ ){
						if( idList[i] != reserve->recSetting.tunerID ){
							itrInfo->second->AddNGTunerID(idList[i]);
						}
					}
				}
			}
		}
		setData = *reserve;
	}

	this->reserveText.ChgReserve(setData);
	if( chgCtrl == FALSE ){
		itrInfo->second->SetData(&setData);
	}

	//EventID変わってる可能性あるのでリスト再構築
	this->reserveInfoIDMap.clear();
	map<DWORD, RESERVE_DATA>::const_iterator itrData;
	for( itrData = this->reserveText.GetMap().begin(); itrData != this->reserveText.GetMap().end(); itrData++){
		LONGLONG keyID = _Create64Key2(
			itrData->second.originalNetworkID,
			itrData->second.transportStreamID,
			itrData->second.serviceID,
			itrData->second.eventID);
		this->reserveInfoIDMap.insert(pair<LONGLONG, DWORD>(keyID, itrData->second.reserveID));
	}


	return TRUE;
}

//予約情報を削除する
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// reserveList		[IN]予約IDリスト
BOOL CReserveManager::DelReserveData(
	vector<DWORD>* reserveList
	)
{
	if( Lock(L"DelReserveData") == FALSE ) return FALSE;
	BOOL ret = TRUE;

	//予約削除
	_DelReserveData(reserveList);

	this->reserveText.SaveText();

	_ReloadBankMap();


	_SendNotifyUpdate(NOTIFY_UPDATE_RESERVE_INFO);

	UnLock();
	return ret;
}

BOOL CReserveManager::_DelReserveData(
	vector<DWORD>* reserveList
)
{
	if( reserveList == NULL ){
		return FALSE;
	}
	for( size_t i=0; i<reserveList->size(); i++ ){
		map<DWORD, BANK_INFO*>::iterator itrBank;
		map<DWORD, BANK_WORK_INFO*>::iterator itrWork;
		for( itrBank = this->bankMap.begin(); itrBank != this->bankMap.end(); itrBank++ ){
			itrWork = itrBank->second->reserveList.find((*reserveList)[i]);
			if( itrWork != itrBank->second->reserveList.end() ){

				map<DWORD, CTunerBankCtrl*>::iterator itrCtrl;
				itrCtrl = this->tunerBankMap.find(itrBank->second->tunerID);
				if( itrCtrl != this->tunerBankMap.end() ){
					itrCtrl->second->DeleteReserve((*reserveList)[i]);
				}
				SAFE_DELETE(itrWork->second);
				itrBank->second->reserveList.erase(itrWork);
			}
		}

		itrWork = this->NGReserveMap.find((*reserveList)[i]);
		if( itrWork != this->NGReserveMap.end() ){
			SAFE_DELETE(itrWork->second);
			this->NGReserveMap.erase(itrWork);
		}

		this->reserveText.DelReserve((*reserveList)[i]);

		map<DWORD, CReserveInfo*>::iterator itr;
		itr = this->reserveInfoMap.find((*reserveList)[i]);
		if( itr != this->reserveInfoMap.end() ){
			//IDリスト削除
			RESERVE_DATA data;
			itr->second->GetData(&data);
			map<LONGLONG, DWORD>::iterator itrID;
			LONGLONG keyID = _Create64Key2(data.originalNetworkID, data.transportStreamID, data.serviceID, data.eventID);
			itrID = this->reserveInfoIDMap.find(keyID);
			if( itrID != this->reserveInfoIDMap.end() ){
				this->reserveInfoIDMap.erase(itrID);
			}
			SAFE_DELETE(itr->second);
			this->reserveInfoMap.erase(itr);
		}
	}
	map<DWORD, CReserveInfo*>(this->reserveInfoMap).swap(this->reserveInfoMap);
	return TRUE;
}

void CReserveManager::ReloadBankMap(BOOL notify)
{
	if( Lock(L"ReloadBankMap") == FALSE ) return ;

	_ReloadBankMap();
	if( notify == TRUE ){
		_SendNotifyUpdate(NOTIFY_UPDATE_RESERVE_INFO);
		_SendNotifyUpdate(NOTIFY_UPDATE_REC_INFO);
	}
	
	UnLock();
}

void CReserveManager::_ReloadBankMap()
{
	OutputDebugString(L"Start _ReloadBankMap\r\n");

	DWORD time = GetTickCount();
	//まずバンクをクリア
	map<DWORD, BANK_INFO*>::iterator itrBank;
	for( itrBank = this->bankMap.begin(); itrBank != this->bankMap.end(); itrBank++){
		map<DWORD, BANK_WORK_INFO*>::iterator itrWork;
		for( itrWork = itrBank->second->reserveList.begin(); itrWork != itrBank->second->reserveList.end(); itrWork++ ){
			SAFE_DELETE(itrWork->second);
		}
		itrBank->second->reserveList.clear();
	}
	map<DWORD, BANK_WORK_INFO*>::iterator itrNG;
	for( itrNG = this->NGReserveMap.begin(); itrNG != this->NGReserveMap.end(); itrNG++){
		SAFE_DELETE(itrNG->second);
	}
	this->NGReserveMap.clear();

	map<DWORD, BANK_INFO*>(this->bankMap).swap(this->bankMap);
	map<DWORD, BANK_WORK_INFO*>(this->NGReserveMap).swap(this->NGReserveMap);

	//待機状態に入っているもの以外クリア
	map<DWORD, CTunerBankCtrl*>::iterator itrCtrl;
	for( itrCtrl = this->tunerBankMap.begin(); itrCtrl != this->tunerBankMap.end(); itrCtrl++){
		itrCtrl->second->ClearNoCtrl();
	}

	//録画時間過ぎているものないかチェック
	CheckOverTimeReserve();

	if( this->autoDel == TRUE ){
		vector<RESERVE_DATA> chkReserveMap;
		map<DWORD, CReserveInfo*>::iterator itrRes;
		for( itrRes = this->reserveInfoMap.begin(); itrRes != this->reserveInfoMap.end(); itrRes++ ){
			chkReserveMap.resize(chkReserveMap.size() + 1);
			itrRes->second->GetData(&chkReserveMap.back());
		}
		wstring defRecPath = L"";
		GetRecFolderPath(defRecPath);
		map<wstring, wstring> protectFile;
		recInfoText.GetProtectFiles(&protectFile);
		CreateDiskFreeSpace(chkReserveMap, defRecPath, protectFile, this->delFolderList, this->delExtList);
	}

	switch(this->reloadBankMapAlgo){
	case 0:
		_ReloadBankMapAlgo(FALSE, FALSE, this->backPriorityFlag, FALSE);
		break;
	case 1:
		_ReloadBankMapAlgo(TRUE, FALSE, this->backPriorityFlag, FALSE);
		break;
	case 2:
		_ReloadBankMapAlgo(TRUE, TRUE, this->backPriorityFlag, TRUE);
		break;
	case 3:
		_ReloadBankMapAlgo(TRUE, TRUE, FALSE, TRUE);
		break;
	default:
		_ReloadBankMapAlgo(FALSE, FALSE, this->backPriorityFlag, FALSE);
		break;
	}


	Sleep(0);

	//予約情報を追加
	for( itrBank = this->bankMap.begin(); itrBank != this->bankMap.end(); itrBank++){
		itrCtrl = this->tunerBankMap.find(itrBank->second->tunerID);
		if( itrCtrl != this->tunerBankMap.end() ){
			vector<CReserveInfo*> reserveInfo;
			map<DWORD, BANK_WORK_INFO*>::iterator itrItem;
			for( itrItem = itrBank->second->reserveList.begin(); itrItem != itrBank->second->reserveList.end(); itrItem++ ){
				reserveInfo.push_back(itrItem->second->reserveInfo);
			}
			if( reserveInfo.size() > 0 ){
				itrCtrl->second->AddReserve(&reserveInfo);
			}
		}
	}

	//TODO: 余裕が出来たらサーバー間連携を復活させる

	_OutputDebugString(L"End _ReloadBankMap %dmsec\r\n", GetTickCount()-time);
}

void CReserveManager::_ReloadBankMapAlgo(BOOL do2Pass, BOOL ignoreUseTunerID, BOOL backPriority, BOOL noTuner)
{
	map<DWORD, BANK_INFO*>::iterator itrBank;
	map<DWORD, BANK_WORK_INFO*>::iterator itrNG;

	//録画待機中のものをバンクに登録＆優先度と時間でソート
	map<DWORD, CReserveInfo*>::iterator itrInfo;
	multimap<wstring, BANK_WORK_INFO*> sortReserveMap;
	for( itrInfo = this->reserveInfoMap.begin(); itrInfo != this->reserveInfoMap.end(); itrInfo++ ){
		itrInfo->second->SetOverlapMode(0);
		BYTE recMode = 0;
		itrInfo->second->GetRecMode(&recMode);
		if( recMode == RECMODE_NO ){
			continue;
		}
		BOOL recWaitFlag = FALSE;
		DWORD tunerID = 0;
		itrInfo->second->GetRecWaitMode(&recWaitFlag, &tunerID);
		if( recWaitFlag == TRUE ){
			//録画処理中なのでバンクに登録
			itrBank = this->bankMap.find(tunerID);
			if( itrBank != this->bankMap.end() ){
				BANK_WORK_INFO* item = new BANK_WORK_INFO;
				CreateWorkData(itrInfo->second, item, backPriority, 0, 0, noTuner);
				itrBank->second->reserveList.insert(pair<DWORD, BANK_WORK_INFO*>(item->reserveID,item));
			}
		}else{
			//まだ録画処理されていないのでソートに追加
			BANK_WORK_INFO* item = new BANK_WORK_INFO;
			CreateWorkData(itrInfo->second, item, backPriority, 0, 0, noTuner);
			sortReserveMap.insert(pair<wstring, BANK_WORK_INFO*>(item->sortKey, item));
		}
	}

	Sleep(0);

	//予約の割り振り
	multimap<wstring, BANK_WORK_INFO*> tempNGMap1Pass;
	multimap<wstring, BANK_WORK_INFO*> tempMap2Pass;
	multimap<wstring, BANK_WORK_INFO*> tempNGMap2Pass;
	multimap<wstring, BANK_WORK_INFO*>::iterator itrSort;
	for( itrSort = do2Pass ? sortReserveMap.begin() : sortReserveMap.end(); itrSort !=  sortReserveMap.end(); itrSort++ ){
		BOOL insert = FALSE;
		if( ignoreUseTunerID || itrSort->second->useTunerID == 0 ){
			//チューナー優先度より同一物理チャンネルで連続となるチューナーの使用を優先する
			if( this->sameChPriorityFlag == TRUE ){
				for( itrBank = this->bankMap.begin(); itrBank != this->bankMap.end(); itrBank++){
					DWORD status = ChkInsertSameChStatus(itrBank->second, itrSort->second);
					if( status == 1 ){
						//問題なく追加可能
						itrBank->second->reserveList.insert(pair<DWORD, BANK_WORK_INFO*>(itrSort->second->reserveID,itrSort->second));
						insert = TRUE;
						break;
					}
				}
			}
			if( insert == FALSE ){
				for( itrBank = this->bankMap.begin(); itrBank != this->bankMap.end(); itrBank++){
					DWORD status = ChkInsertStatus(itrBank->second, itrSort->second);
					if( status == 1 ){
						//問題なく追加可能
						itrBank->second->reserveList.insert(pair<DWORD, BANK_WORK_INFO*>(itrSort->second->reserveID,itrSort->second));
						insert = TRUE;
						break;
					}
				}
			}
		}else{
			//チューナー固定
			if( this->tunerManager.IsSupportService(itrSort->second->useTunerID, itrSort->second->ONID, itrSort->second->TSID, itrSort->second->SID) == TRUE ){
				if( itrSort->second->reserveInfo->IsNGTuner(itrSort->second->useTunerID) == FALSE ){
					map<DWORD, BANK_INFO*>::iterator itrManual;
					itrManual = this->bankMap.find(itrSort->second->useTunerID);
					if( itrManual != this->bankMap.end() ){
						itrManual->second->reserveList.insert(pair<DWORD, BANK_WORK_INFO*>(itrSort->second->reserveID,itrSort->second));
						insert = TRUE;
					}
				}
			}
		}
		if( insert == FALSE ){
			//追加できなかった
			tempNGMap1Pass.insert(pair<wstring, BANK_WORK_INFO*>(itrSort->first, itrSort->second));
		}
	}

	//2Pass
	multimap<wstring, BANK_WORK_INFO*>& targetMap = do2Pass ? tempNGMap1Pass : sortReserveMap;
	for( itrSort = targetMap.begin(); itrSort !=  targetMap.end(); itrSort++ ){
		BOOL insert = FALSE;
		if( ignoreUseTunerID || itrSort->second->useTunerID == 0 ){
			//チューナー優先度より同一物理チャンネルで連続となるチューナーの使用を優先する
			if( this->sameChPriorityFlag == TRUE ){
				for( itrBank = this->bankMap.begin(); itrBank != this->bankMap.end(); itrBank++){
					DWORD status = ChkInsertSameChStatus(itrBank->second, itrSort->second);
					if( status == 1 ){
						//問題なく追加可能
						itrBank->second->reserveList.insert(pair<DWORD, BANK_WORK_INFO*>(itrSort->second->reserveID,itrSort->second));
						insert = TRUE;
						break;
					}
				}
			}
			if( insert == FALSE ){
				for( itrBank = this->bankMap.begin(); itrBank != this->bankMap.end(); itrBank++){
					DWORD status = ChkInsertStatus(itrBank->second, itrSort->second);
					if( status == 1 ){
						//問題なく追加可能
						itrBank->second->reserveList.insert(pair<DWORD, BANK_WORK_INFO*>(itrSort->second->reserveID,itrSort->second));
						insert = TRUE;
						break;
					}else if( status == 2 ){
						//追加可能だが終了時間と開始時間の重なった予約あり
						//仮追加
						itrBank->second->reserveList.insert(pair<DWORD, BANK_WORK_INFO*>(itrSort->second->reserveID,itrSort->second));
						itrSort->second->preTunerID = itrBank->first;
						tempMap2Pass.insert(pair<wstring, BANK_WORK_INFO*>(itrSort->first, itrSort->second));
						insert = TRUE;
						break;
					}
				}
			}
		}else{
			//チューナー固定
			if( this->tunerManager.IsSupportService(itrSort->second->useTunerID, itrSort->second->ONID, itrSort->second->TSID, itrSort->second->SID) == TRUE ){
				if( do2Pass || itrSort->second->reserveInfo->IsNGTuner(itrSort->second->useTunerID) == FALSE ){
					map<DWORD, BANK_INFO*>::iterator itrManual;
					itrManual = this->bankMap.find(itrSort->second->useTunerID);
					if( itrManual != this->bankMap.end() ){
						itrManual->second->reserveList.insert(pair<DWORD, BANK_WORK_INFO*>(itrSort->second->reserveID,itrSort->second));
						insert = TRUE;
					}
				}
			}
		}
		if( insert == FALSE ){
			//追加できなかった
			itrSort->second->reserveInfo->SetOverlapMode(2);
			this->NGReserveMap.insert(pair<DWORD, BANK_WORK_INFO*>(itrSort->second->reserveID, itrSort->second));

			tempNGMap2Pass.insert(pair<wstring, BANK_WORK_INFO*>(itrSort->first, itrSort->second));
		}
	}

	Sleep(0);

	//開始終了重なっている予約で、他のチューナーに回せるやつあるかチェック
	for( itrSort = tempMap2Pass.begin(); itrSort !=  tempMap2Pass.end(); itrSort++ ){
		BOOL insert = FALSE;
		for( itrBank = this->bankMap.begin(); itrBank != this->bankMap.end(); itrBank++){
			if( itrBank->second->tunerID == itrSort->second->preTunerID ){
				if(ReChkInsertStatus(itrBank->second, itrSort->second) == 1 ){
					//前の予約移動した？このままでもOK
					break;
				}else{
					continue;
				}
			}
			DWORD status = ChkInsertStatus(itrBank->second, itrSort->second);
			if( status == 1 ){
				//問題なく追加可能
				itrBank->second->reserveList.insert(pair<DWORD, BANK_WORK_INFO*>(itrSort->second->reserveID,itrSort->second));
				insert = TRUE;
				break;
			}
		}
		if( insert == TRUE ){
			//仮追加を削除
			itrBank = this->bankMap.find(itrSort->second->preTunerID);
			if( itrBank != this->bankMap.end() ){
				map<DWORD, BANK_WORK_INFO*>::iterator itrDel;
				itrDel = itrBank->second->reserveList.find(itrSort->second->reserveID);
				if( itrDel != itrBank->second->reserveList.end() ){
					itrBank->second->reserveList.erase(itrDel);
				}
			}
		}
	}

	Sleep(0);

	multimap<wstring, BANK_WORK_INFO*>::iterator itrSortNG;
	//NGでチューナー入れ替えで録画できるものあるかチェック
	itrSortNG = tempNGMap2Pass.begin();
	while(itrSortNG != tempNGMap2Pass.end() ){
		if( itrSortNG->second->useTunerID != 0 ){
			//チューナー固定でNGになっているのは無視
			itrSortNG++;
			continue;
		}
		if( ChangeNGReserve(itrSortNG->second) == TRUE ){
			//登録できたのでNGから削除
			itrSortNG->second->reserveInfo->SetOverlapMode(0);
			itrNG = this->NGReserveMap.find(itrSortNG->second->reserveID);
			if( itrNG != this->NGReserveMap.end() ){
				this->NGReserveMap.erase(itrNG);
			}
			tempNGMap2Pass.erase(itrSortNG++);
		}else{
			itrSortNG++;
		}
	}

	//NGで少しでも録画できるかチェック
	for( itrSortNG = tempNGMap2Pass.begin(); itrSortNG != tempNGMap2Pass.end(); itrSortNG++){
		if( itrSortNG->second->useTunerID != 0 ){
			//チューナー固定でNGになっているのは無視
			continue;
		}
		DWORD maxDuration = 0;
		DWORD maxID = 0xFFFFFFFF;
		for( itrBank = this->bankMap.begin(); itrBank != this->bankMap.end(); itrBank++){
			DWORD duration = ChkInsertNGStatus(itrBank->second, itrSortNG->second);
			if( maxDuration < duration && duration > 0){
				maxDuration = duration;
				maxID = itrBank->second->tunerID;
			}
		}
		if( maxDuration > 0 && maxID != 0xFFFFFFFF ){
			//少しでも録画できる場所あった
			itrBank = this->bankMap.find(maxID);
			if( itrBank != this->bankMap.end() ){
				itrSortNG->second->reserveInfo->SetOverlapMode(1);
				itrBank->second->reserveList.insert(pair<DWORD, BANK_WORK_INFO*>(itrSortNG->second->reserveID,itrSortNG->second));

				//登録できたのでNGから削除
				itrNG = this->NGReserveMap.find(itrSortNG->second->reserveID);
				if( itrNG != this->NGReserveMap.end() ){
					this->NGReserveMap.erase(itrNG);
				}
			}
		}
	}
}

BOOL CReserveManager::ChangeNGReserve(BANK_WORK_INFO* inItem)
{
	BOOL ret = FALSE;

	if( inItem == NULL ){
		return FALSE;
	}

	map<DWORD, BANK_INFO*>::iterator itrBank;
	for( itrBank = this->bankMap.begin(); itrBank != this->bankMap.end(); itrBank++ ){
		if( inItem->reserveInfo->IsNGTuner(itrBank->second->tunerID) == FALSE ){
			//NGじゃないバンクあり

			//時間のかぶる予約一覧取得
			vector<BANK_WORK_INFO*> chkReserve;
			map<DWORD, BANK_WORK_INFO*>::iterator itrWork;
			for( itrWork = itrBank->second->reserveList.begin(); itrWork != itrBank->second->reserveList.end(); itrWork++ ){
				if( itrWork->second->chID == inItem->chID ){
					//同一チャンネルなのでかぶりではない
					continue;
				}

				//時間かぶっている予約かチェック
				if( itrWork->second->startTime <= inItem->startTime && 
					inItem->startTime < itrWork->second->endTime){
					//開始時間が含まれている
						chkReserve.push_back(itrWork->second);
				}else
				if( itrWork->second->startTime < inItem->endTime &&
					inItem->endTime <= itrWork->second->endTime ){
					//終了時間が含まれている
						chkReserve.push_back(itrWork->second);
				}else
				if( inItem->startTime <= itrWork->second->startTime &&
					itrWork->second->startTime < inItem->endTime ){
					//開始から終了の間に含んでしまう
						chkReserve.push_back(itrWork->second);
				}else
				if( inItem->startTime < itrWork->second->endTime &&
					itrWork->second->endTime <= inItem->endTime ){
					//開始から終了の間に含んでしまう
						chkReserve.push_back(itrWork->second);
				}
			}

			//かぶった予約が別バンクで行えるかチェック
			BOOL moveOK = TRUE;
			vector<BANK_WORK_INFO*> tempIn;
			for( size_t i=0; i<chkReserve.size(); i++ ){
				BOOL inFlag = FALSE;

				//録画中の予約とかぶるとこのバンクは無理
				BOOL recWaitFlag = FALSE;
				DWORD tunerID = 0;
				chkReserve[i]->reserveInfo->GetRecWaitMode(&recWaitFlag, &tunerID);
				if( recWaitFlag == TRUE ){
					moveOK = FALSE;
					break;
				}

				map<DWORD, BANK_INFO*>::iterator itrBank2;
				//まず問題なく入る場所を探す
				for( itrBank2 = this->bankMap.begin(); itrBank2 != this->bankMap.end(); itrBank2++ ){
					if( itrBank2->first == itrBank->first ){
						continue;
					}
					if( ReChkInsertStatus(itrBank2->second, chkReserve[i]) == 1 ){
						inFlag = TRUE;
						//行えるのでバンク移動
						chkReserve[i]->preTunerID = itrBank->second->tunerID;

						itrBank2->second->reserveList.insert(pair<DWORD, BANK_WORK_INFO*>(chkReserve[i]->reserveID, chkReserve[i]));

						tempIn.push_back(chkReserve[i]);
						break;
					}
				}
				if(inFlag == FALSE ){
					//開始時間とか重なるものを探す
					for( itrBank2 = this->bankMap.begin(); itrBank2 != this->bankMap.end(); itrBank2++ ){
						if( itrBank2->first == itrBank->first ){
							continue;
						}
						if( ReChkInsertStatus(itrBank2->second, chkReserve[i]) != 0 ){
							inFlag = TRUE;
							//行えるのでバンク移動
							chkReserve[i]->preTunerID = itrBank->second->tunerID;

							itrBank2->second->reserveList.insert(pair<DWORD, BANK_WORK_INFO*>(chkReserve[i]->reserveID, chkReserve[i]));

							tempIn.push_back(chkReserve[i]);
							break;
						}
					}
				}
				if(inFlag == FALSE ){
					moveOK = FALSE;
					break;
				}
			}
			if(moveOK == FALSE ){
				//移動できなかったので復帰
				for( size_t i=0; i<tempIn.size(); i++ ){
					map<DWORD, BANK_INFO*>::iterator itrBank2;
					itrBank2 = this->bankMap.find(tempIn[i]->preTunerID);
					if( itrBank2 != this->bankMap.end() ){
						map<DWORD, BANK_WORK_INFO*>::iterator itrRes;
						itrRes = itrBank2->second->reserveList.find(tempIn[i]->reserveID);
						if( itrRes != itrBank2->second->reserveList.end() ){
							itrBank2->second->reserveList.erase(itrRes);
						}
					}
				}
			}else{
				//このバンクから移動したものを削除
				for( size_t i=0; i<tempIn.size(); i++ ){
					map<DWORD, BANK_WORK_INFO*>::iterator itrRes;
					itrRes = itrBank->second->reserveList.find(tempIn[i]->reserveID);
					if( itrRes != itrBank->second->reserveList.end() ){
						itrBank->second->reserveList.erase(itrRes);
					}
				}
				//このバンクにNG追加
				itrBank->second->reserveList.insert(pair<DWORD, BANK_WORK_INFO*>(inItem->reserveID, inItem));

				ret = TRUE;
				break;
			}
		}
	}

	return ret;
}

//マージンを考慮した予約時刻を計算する(常にendTime>=startTime)
void CReserveManager::CalcEntireReserveTime(LONGLONG* startTime, LONGLONG* endTime, const RESERVE_DATA& data)
{
	LONGLONG startTime_ = ConvertI64Time(data.startTime);
	LONGLONG endTime_ = startTime_ + data.durationSecond * I64_1SEC;
	LONGLONG startMargin = this->defStartMargine * I64_1SEC;
	LONGLONG endMargin = this->defEndMargine * I64_1SEC;
	if( data.recSetting.useMargineFlag == TRUE ){
		startMargin = data.recSetting.startMargine * I64_1SEC;
		endMargin = data.recSetting.endMargine * I64_1SEC;
	}
	//開始マージンは元の予約終了時刻を超えて負であってはならない
	startMargin = max(startMargin, startTime_ - endTime_);
	//終了マージンは元の予約開始時刻を超えて負であってはならない
	endMargin = max(endMargin, startTime_ - min(startMargin, 0) - endTime_);
	if( startTime != NULL ){
		*startTime = startTime_ - startMargin;
	}
	if( endTime != NULL ){
		*endTime = endTime_ + endMargin;
	}
}

void CReserveManager::CheckOverTimeReserve()
{
	LONGLONG nowTime = GetNowI64Time();

	vector<DWORD> deleteList;
	map<DWORD, CReserveInfo*>::iterator itrInfo;
	for( itrInfo = this->reserveInfoMap.begin(); itrInfo != this->reserveInfoMap.end(); itrInfo++ ){
		BOOL recWaitFlag = FALSE;
		DWORD tunerID = 0;
		itrInfo->second->GetRecWaitMode(&recWaitFlag, &tunerID);
		if( recWaitFlag == FALSE ){
			//録画状態ではない
			RESERVE_DATA data;
			itrInfo->second->GetData(&data);

			LONGLONG endTime;
			CalcEntireReserveTime(NULL, &endTime, data);

			if( endTime < nowTime ){
				//終了時間過ぎてしまっている
				deleteList.push_back(itrInfo->first);
				if( data.recSetting.recMode != RECMODE_NO ){
					//無効ものは結果に残さない
					REC_FILE_INFO item;
					item = data;
					if( itrInfo->second->IsOpenErred() == FALSE ){
						if( data.overlapMode == 0 ){
							item.recStatus = REC_END_STATUS_START_ERR;
							item.comment = L"録画時間に起動していなかった可能性があります";
						}else{
							item.recStatus = REC_END_STATUS_NO_TUNER;
							item.comment = L"チューナー不足のため失敗しました";
						}
					}else{
						item.recStatus = REC_END_STATUS_OPEN_ERR;
						item.comment = L"チューナーのオープンに失敗しました";
					}
					this->recInfoText.AddRecInfo(item);
				}
			}
		}
	}
	if( deleteList.size() > 0 ){
		_DelReserveData(&deleteList);
		this->reserveText.SaveText();
		this->recInfoText.SaveText();
		this->chgRecInfo = TRUE;
	}
}

void CReserveManager::CreateWorkData(CReserveInfo* reserveInfo, BANK_WORK_INFO* workInfo, BOOL backPriority, DWORD reserveCount, DWORD reserveNum, BOOL noTuner)
{
	workInfo->reserveInfo = reserveInfo;

	RESERVE_DATA data;
	reserveInfo->GetData(&data);

	DWORD tunerID = 0;
	reserveInfo->GetRecWaitMode(&workInfo->recWaitFlag, &tunerID);

	CalcEntireReserveTime(&workInfo->startTime, &workInfo->endTime, data);

	workInfo->priority = data.recSetting.priority;

	workInfo->reserveID = data.reserveID;

	workInfo->chID = ((DWORD)data.originalNetworkID)<<16 | data.transportStreamID;

	workInfo->useTunerID = data.recSetting.tunerID;

	workInfo->ONID = data.originalNetworkID;
	workInfo->TSID = data.transportStreamID;
	workInfo->SID = data.serviceID;

	BYTE tunerManual = 1;
	if( noTuner == FALSE ){
		if( workInfo->useTunerID != 0 ){
			tunerManual = 0;
		}
	}

	if( backPriority == TRUE ){
		//後の番組優先
		Format(workInfo->sortKey, L"%01d%02x%016I64x%08x", tunerManual, 255-workInfo->priority, workInfo->startTime*(-1), UINT_MAX-reserveCount);
	}else{
		//前の番組優先
		Format(workInfo->sortKey, L"%01d%02x%016I64x%08x", tunerManual, 255-workInfo->priority, workInfo->startTime, reserveCount);
	}
}

DWORD CReserveManager::ChkInsertStatus(BANK_INFO* bank, BANK_WORK_INFO* inItem, BOOL reCheck, BOOL mustOverlap)
{
	if( bank == NULL || inItem == NULL ){
		return 0;
	}
	if( inItem->reserveInfo->IsNGTuner(bank->tunerID) == TRUE ){
		return 0;
	}
	DWORD status = mustOverlap ? 0 : 1;
	map<DWORD, BANK_WORK_INFO*>::iterator itrBank;
	for( itrBank = bank->reserveList.begin(); itrBank != bank->reserveList.end(); itrBank++ ){
		if( !reCheck || itrBank->second->reserveID != inItem->reserveID ){
			if( itrBank->second->chID == inItem->chID ){
				//同一チャンネル
				if( mustOverlap ){
					if(( itrBank->second->startTime <= inItem->startTime && inItem->startTime <= itrBank->second->endTime ) ||
						( itrBank->second->startTime <= inItem->endTime && inItem->endTime <= itrBank->second->endTime ) ||
						( inItem->startTime <= itrBank->second->startTime && itrBank->second->startTime <= inItem->endTime ) ||
						( inItem->startTime <= itrBank->second->endTime && itrBank->second->endTime <= inItem->endTime ) 
						){
							//開始時間か終了時間が重なっている
							status = 1;
					}
				}
				continue;
			}

			//別チャンネルで開始時間と終了時間が重なっていないかチェック
			if( itrBank->second->startTime == inItem->endTime || itrBank->second->endTime == inItem->startTime ){
				//連続予約の可能性あり
				status = 2;
				if( mustOverlap ){
					break;
				}
			}else if(( itrBank->second->startTime <= inItem->startTime && inItem->startTime <= itrBank->second->endTime ) ||
				( itrBank->second->startTime <= inItem->endTime && inItem->endTime <= itrBank->second->endTime ) ||
				( inItem->startTime <= itrBank->second->startTime && itrBank->second->startTime <= inItem->endTime ) ||
				( inItem->startTime <= itrBank->second->endTime && itrBank->second->endTime <= inItem->endTime ) 
				){
					//開始時間か終了時間が重なっている
					status = 0;
					break;
			}
		}
	}

	return status;
}

DWORD CReserveManager::ReChkInsertStatus(BANK_INFO* bank, BANK_WORK_INFO* inItem)
{
	return ChkInsertStatus(bank, inItem, TRUE, FALSE);
}

DWORD CReserveManager::ChkInsertSameChStatus(BANK_INFO* bank, BANK_WORK_INFO* inItem)
{
	return ChkInsertStatus(bank, inItem, FALSE, TRUE);
}

DWORD CReserveManager::ChkInsertNGStatus(BANK_INFO* bank, BANK_WORK_INFO* inItem)
{
	if( bank == NULL || inItem == NULL ){
		return 0;
	}
	if( inItem->reserveInfo->IsNGTuner(bank->tunerID) == TRUE ){
		return 0;
	}

	LONGLONG chkStartTime = inItem->startTime;
	LONGLONG chkEndTime = inItem->endTime;
	map<DWORD, BANK_WORK_INFO*>::iterator itrBank;
	for( itrBank = bank->reserveList.begin(); itrBank != bank->reserveList.end(); itrBank++ ){
		if( itrBank->second->startTime == inItem->startTime && itrBank->second->endTime == inItem->endTime ){
			//時間完全に一緒ならこれによってNGになったはず
			return 0;
		}
		if( itrBank->second->startTime <= chkStartTime && chkStartTime <= itrBank->second->endTime && chkEndTime <= itrBank->second->endTime ){
			//開始時間が含まれ、終了時間まで含まれている
			if( itrBank->second->priority >= inItem->priority ){
				//優先度も高いのでNG
				return 0;
			}
		}else
		if( itrBank->second->startTime <= chkStartTime && chkStartTime <= itrBank->second->endTime && chkEndTime > itrBank->second->endTime ){
			//開始時間が含まれ、終了時間まで含まれない
			if( itrBank->second->priority >= inItem->priority ){
				//優先度も高いので開始時間削る
				chkStartTime = itrBank->second->endTime;
			}
		}else
		if( itrBank->second->startTime <= chkEndTime && chkEndTime <= itrBank->second->endTime && chkStartTime < itrBank->second->startTime ){
			//終了時間が含まれ、開始時間まで含まれない
			if( itrBank->second->priority >= inItem->priority ){
				//優先度も高いので終了時間削る
				chkEndTime = itrBank->second->startTime;
			}
		}else
		if( chkStartTime <= itrBank->second->startTime && itrBank->second->startTime <= chkEndTime && itrBank->second->endTime <= chkEndTime ){
			//開始から終了の間に含んでしまう
			if( itrBank->second->priority >= inItem->priority ){
				//優先度も高いので終了時間削る
				chkEndTime = itrBank->second->startTime;
			}
		}
		if( chkEndTime < chkStartTime ){
			//終了時間の方が早いとかおかしい
			return 0;
		}
	}

	DWORD duration = (DWORD)((chkEndTime - chkStartTime)/I64_1SEC);

	return duration;
}

UINT WINAPI CReserveManager::BankCheckThread(LPVOID param)
{
	CReserveManager* sys = (CReserveManager*)param;
	CSendCtrlCmd sendCtrl;
	DWORD wait = 1000;
	DWORD countTuijyuChk = 11;
	DWORD countAutoDelChk = 11;
	BOOL sendPreEpgCap = FALSE;

	//EPG取得開始時の設定の一時保存
	BOOL Tmp_BSOnly;
	BOOL Tmp_CS1Only;
	BOOL Tmp_CS2Only;
	BOOL Tmp_EnOnlyFlags = FALSE;

	while(1){
		if( ::WaitForSingleObject(sys->bankCheckStopEvent, wait) != WAIT_TIMEOUT ){
			//キャンセルされた
			break;
		}

		//終了している予約の確認
		if( sys->Lock(L"BankCheckThread1") == TRUE){
			sys->CheckEndReserve();
			sys->UnLock();
		}

		//エラーの発生しているチューナーの確認
		if( sys->Lock(L"BankCheckThread2") == TRUE){
			sys->CheckErrReserve();
			sys->UnLock();
		}

		//追従の確認
		countTuijyuChk++;
		if( countTuijyuChk > 10 ){
			if( sys->Lock(L"BankCheckThread3") == TRUE){
				sys->CheckTuijyu();
				sys->UnLock();
			}
			countTuijyuChk = 0;
		}

		//バッチ処理の確認
		if( sys->Lock(L"BankCheckThread4") == TRUE){
			sys->CheckBatWork();
			sys->UnLock();
		}

		//自動削除の確認
		if( sys->autoDel == TRUE ){
			countAutoDelChk++;
			if( countAutoDelChk > 10 ){
				if( sys->Lock(L"BankCheckThread5") == TRUE){
					vector<RESERVE_DATA> chkReserveMap;
					map<DWORD, CReserveInfo*>::iterator itrRes;
					for( itrRes = sys->reserveInfoMap.begin(); itrRes != sys->reserveInfoMap.end(); itrRes++ ){
						chkReserveMap.resize(chkReserveMap.size() + 1);
						itrRes->second->GetData(&chkReserveMap.back());
					}
					wstring defRecPath = L"";
					GetRecFolderPath(defRecPath);
					map<wstring, wstring> protectFile;
					sys->recInfoText.GetProtectFiles(&protectFile);
					CreateDiskFreeSpace(chkReserveMap, defRecPath, protectFile, sys->delFolderList, sys->delExtList);
					sys->UnLock();
					countAutoDelChk = 0;
				}
			}
		}

		//EPG取得時間の確認
		if( sys->Lock(L"BankCheckThread6") == TRUE){
			LONGLONG capTime = 0;
			int basicOnlyFlags = 0;
			if( sys->GetNextEpgcapTime(&capTime, -1, &basicOnlyFlags) == TRUE ){
				if( (sendPreEpgCap == FALSE) && ((GetNowI64Time()+60*I64_1SEC) > capTime) ){
					BOOL bUseTuner = FALSE;
					vector<pair<vector<DWORD>, WORD>> tunerIDList;
					sys->tunerManager.GetEnumEpgCapTuner(&tunerIDList);
					map<DWORD, CTunerBankCtrl*>::iterator itrCtrl;
					for( size_t i=0; i<tunerIDList.size(); i++ ){
						WORD epgCapMax = tunerIDList[i].second;
						WORD ngCapCount = 0;
						for( size_t j=0; j<tunerIDList[i].first.size() && epgCapMax > 0; j++ ){
							itrCtrl = sys->tunerBankMap.find(tunerIDList[i].first[j]);
							if( itrCtrl != sys->tunerBankMap.end() && itrCtrl->second->IsEpgCapWorking() == FALSE ){
								itrCtrl->second->ClearEpgCapItem();
								if( sys->ngCapMin != 0 ){
									if( itrCtrl->second->IsEpgCapOK(sys->ngCapMin) == FALSE ){
										//実行しちゃいけない
										ngCapCount++;
										continue;
									}
								}
								if( itrCtrl->second->IsEpgCapOK(sys->ngCapTunerMin) == TRUE ){
									//使えるチューナー
									bUseTuner = TRUE;
									epgCapMax--;
								}
							}
						}
						if( tunerIDList[i].second > tunerIDList[i].first.size() - ngCapCount ){
							bUseTuner = FALSE;
							break;
						}
					}

					if( sys->notifyManager != NULL && bUseTuner == TRUE ){
						sys->notifyManager->AddNotifyMsg(NOTIFY_UPDATE_PRE_EPGCAP_START, L"取得開始１分前");
					}
					sendPreEpgCap = TRUE;
				}
				if( GetNowI64Time() > capTime && sys->_IsEpgCap() == FALSE){
					//開始時間過ぎたので開始
					if( basicOnlyFlags >= 0 ){
						//一時的に設定を変更してEPG取得チューナ側の挙動を変える
						wstring iniCommonPath = L"";
						GetCommonIniPath(iniCommonPath);
						if( Tmp_EnOnlyFlags == FALSE ){
							Tmp_BSOnly = sys->BSOnly;
							Tmp_CS1Only = sys->CS1Only;
							Tmp_CS2Only = sys->CS2Only;
							Tmp_EnOnlyFlags = TRUE;
						}
						sys->BSOnly = (basicOnlyFlags & 1) != 0 ? TRUE : FALSE;
						sys->CS1Only = (basicOnlyFlags & 2) != 0 ? TRUE : FALSE;
						sys->CS2Only = (basicOnlyFlags & 4) != 0 ? TRUE : FALSE;
						WritePrivateProfileString(L"SET", L"BSBasicOnly", sys->BSOnly ? L"1" : L"0", iniCommonPath.c_str());
						WritePrivateProfileString(L"SET", L"CS1BasicOnly", sys->CS1Only ? L"1" : L"0", iniCommonPath.c_str());
						WritePrivateProfileString(L"SET", L"CS2BasicOnly", sys->CS2Only ? L"1" : L"0", iniCommonPath.c_str());
					}
					sys->_StartEpgCap();
				}
			}else{
				//EPGの取得予定なし
			}
			sys->UnLock();
		}

		//録画状態の通知
		if( sys->notifyStatus == 0 ){
			if( sys->Lock(L"BankCheckThread7") == TRUE){
				map<DWORD, CTunerBankCtrl*>::iterator itrCtrl;
				for( itrCtrl = sys->tunerBankMap.begin(); itrCtrl != sys->tunerBankMap.end(); itrCtrl++ ){
					if( itrCtrl->second->IsRecWork() == TRUE ){
						sys->_SendNotifyStatus(1);
						break;
					}
				}
				sys->UnLock();
			}
		}else if( sys->notifyStatus == 1 ){
			if( sys->Lock(L"BankCheckThread8") == TRUE){
				map<DWORD, CTunerBankCtrl*>::iterator itrCtrl;
				BOOL noRec = TRUE;
				for( itrCtrl = sys->tunerBankMap.begin(); itrCtrl != sys->tunerBankMap.end(); itrCtrl++ ){
					if( itrCtrl->second->IsRecWork() == TRUE ){
						noRec = FALSE;
						break;
					}
				}
				if( noRec == TRUE ){
					sys->_SendNotifyStatus(0);
				}
				sys->UnLock();
			}
		}

		//EPG取得状態のチェック
		if( sys->epgCapCheckFlag == TRUE ){
			if( sys->Lock(L"BankCheckThread9") == TRUE){
				if( sys->_IsEpgCap() == FALSE ){
					//取得完了
					if( Tmp_EnOnlyFlags != FALSE ){
						//EPG取得開始時の設定を書き戻し
						wstring iniCommonPath = L"";
						GetCommonIniPath(iniCommonPath);
						sys->BSOnly = Tmp_BSOnly;
						sys->CS1Only = Tmp_CS1Only;
						sys->CS2Only = Tmp_CS2Only;
						Tmp_EnOnlyFlags = FALSE;
						WritePrivateProfileString(L"SET", L"BSBasicOnly", sys->BSOnly ? L"1" : L"0", iniCommonPath.c_str());
						WritePrivateProfileString(L"SET", L"CS1BasicOnly", sys->CS1Only ? L"1" : L"0", iniCommonPath.c_str());
						WritePrivateProfileString(L"SET", L"CS2BasicOnly", sys->CS2Only ? L"1" : L"0", iniCommonPath.c_str());
					}
					sys->_SendNotifyStatus(0);
					sys->_SendNotifyUpdate(NOTIFY_UPDATE_EPGCAP_END);
					sys->epgCapCheckFlag = FALSE;
					sys->EnableSuspendWork(0, 0, 1);
					sendPreEpgCap = FALSE;
				}
				sys->UnLock();
			}
		}
	}
	return 0;
}

void CReserveManager::CheckEndReserve()
{
	BOOL needSave = FALSE;
	BYTE suspendMode = 0xFF;
	BYTE rebootFlag = 0xFF;
	map<DWORD, CTunerBankCtrl*>::iterator itrCtrl;
	for( itrCtrl = this->tunerBankMap.begin(); itrCtrl != this->tunerBankMap.end(); itrCtrl++ ){
		map<DWORD, END_RESERVE_INFO*> reserveMap;
		itrCtrl->second->GetEndReserve(&reserveMap);

		if( reserveMap.size() > 0 ){
			needSave = TRUE;
			vector<DWORD> deleteList;

			map<DWORD, END_RESERVE_INFO*>::iterator itrEnd;
			for( itrEnd = reserveMap.begin(); itrEnd != reserveMap.end(); itrEnd++){
				//録画済みとして登録
				if( itrEnd->second->endType == REC_END_STATUS_NORMAL &&
				    (__int64)itrEnd->second->drop < this->recInfo2DropChk &&
				    itrEnd->second->epgEventName.empty() == false ){
					PARSE_REC_INFO2_ITEM item;
					item.originalNetworkID = itrEnd->second->epgOriginalNetworkID;
					item.transportStreamID = itrEnd->second->epgTransportStreamID;
					item.serviceID = itrEnd->second->epgServiceID;
					item.startTime = itrEnd->second->epgStartTime;
					item.eventName = itrEnd->second->epgEventName;
					this->recInfo2Text.Add(item);
				}
				RESERVE_DATA data;
				itrEnd->second->reserveInfo->GetData(&data);
				REC_FILE_INFO item;
				item = data;
				if( item.startTime.wYear == 0 ||item.startTime.wMonth == 0 || item.startTime.wDay == 0 ){
					deleteList.push_back(itrEnd->second->reserveID);
					SAFE_DELETE(itrEnd->second);
					continue;
				}
				item.recFilePath = itrEnd->second->recFilePath;
				item.drops = itrEnd->second->drop;
				item.scrambles = itrEnd->second->scramble;
				if( itrEnd->second->endType == REC_END_STATUS_NORMAL ){
					if( ConvertI64Time(data.startTime) != ConvertI64Time(data.startTimeEpg) ){
						item.recStatus = REC_END_STATUS_CHG_TIME;
						item.comment = L"開始時間が変更されました";
					}else{
						item.recStatus = REC_END_STATUS_NORMAL;
						if( data.recSetting.recMode == RECMODE_VIEW ){
							item.comment = L"終了";
						}else{
							item.comment = L"録画終了";
						}
					}
				}else if( itrEnd->second->endType == REC_END_STATUS_NOT_FIND_PF ){
					item.recStatus = REC_END_STATUS_NOT_FIND_PF;
					item.comment = L"録画中に番組情報を確認できませんでした";
				}else if( itrEnd->second->endType == REC_END_STATUS_NEXT_START_END ){
					item.recStatus = REC_END_STATUS_NEXT_START_END;
					item.comment = L"次の予約開始のためにキャンセルされました";
				}else if( itrEnd->second->endType == REC_END_STATUS_END_SUBREC ){
					item.recStatus = REC_END_STATUS_END_SUBREC;
					item.comment = L"録画終了（空き容量不足で別フォルダへの保存が発生）";
				}else if( itrEnd->second->endType == REC_END_STATUS_ERR_RECSTART ){
					item.recStatus = REC_END_STATUS_ERR_RECSTART;
					item.comment = L"録画開始処理に失敗しました（空き容量不足の可能性あり）";
				}else if( itrEnd->second->endType == REC_END_STATUS_NOT_START_HEAD ){
					item.recStatus = REC_END_STATUS_NOT_START_HEAD;
					item.comment = L"一部のみ録画が実行された可能性があります";
				}else if( itrEnd->second->endType == REC_END_STATUS_ERR_CH_CHG ){
					item.recStatus = REC_END_STATUS_ERR_CH_CHG;
					item.comment = L"指定チャンネルのデータがBonDriverから出力されなかった可能性があります";
				}else if( itrEnd->second->endType == REC_END_STATUS_ERR_END2 ){
					item.recStatus = itrEnd->second->endType;
					item.comment = L"ファイル保存で致命的なエラーが発生した可能性があります";
				}else{
					item.recStatus = itrEnd->second->endType;
					item.comment = L"録画中にキャンセルされた可能性があります";
				}
				this->recInfoText.AddRecInfo(item);
				BOOL tweet = TRUE;
				if( recEndTweetErr == TRUE ){
					if(item.recStatus == REC_END_STATUS_NORMAL ){
						tweet = FALSE;
						if( item.drops > recEndTweetDrop ){
							tweet = TRUE;
						}
					}
				}else{
					if( item.drops < recEndTweetDrop ){
						tweet = FALSE;
					}
				}
				if( tweet == TRUE ){
					_SendTweet(TW_REC_END, &item, NULL, NULL);
				}
				_SendNotifyRecEnd(&item);

				//バッチ処理追加
				if(itrEnd->second->endType == REC_END_STATUS_NORMAL || itrEnd->second->endType == REC_END_STATUS_NEXT_START_END || this->errEndBatRun == TRUE){
					if( data.recSetting.batFilePath.size() > 0 && itrEnd->second->reserveInfo->IsContinueRec() == FALSE && data.recSetting.recMode != RECMODE_VIEW ){
						BAT_WORK_INFO batInfo;
						batInfo.tunerID = itrEnd->second->tunerID;
						batInfo.reserveInfo = data;
						batInfo.recFileInfo = item;

						this->batManager.AddBatWork(batInfo);
					}else{
						suspendMode = data.recSetting.suspendMode;
						rebootFlag = data.recSetting.rebootFlag;
						OutputDebugString(L"★Suspend　add");
					}
				}else{
					suspendMode = data.recSetting.suspendMode;
					rebootFlag = data.recSetting.rebootFlag;
						OutputDebugString(L"★Suspend　add2");
				}

				deleteList.push_back(itrEnd->second->reserveID);
				SAFE_DELETE(itrEnd->second);
			}
			//予約一覧から削除
			_DelReserveData(&deleteList);
		}
	}

	if( needSave == TRUE ){
		//情報ファイルの更新
		this->reserveText.SaveText();
		this->recInfoText.SaveText();
		this->recInfo2Text.SaveText();
		this->chgRecInfo = TRUE;

		_SendNotifyUpdate(NOTIFY_UPDATE_RESERVE_INFO);
		_SendNotifyUpdate(NOTIFY_UPDATE_REC_INFO);
	}
	if( suspendMode != 0xFF && rebootFlag != 0xFF ){
		EnableSuspendWork(suspendMode, rebootFlag, 0);
	}
}

void CReserveManager::CheckErrReserve()
{
	BOOL needNotify = FALSE;

	map<DWORD, CTunerBankCtrl*>::iterator itrCtrl;
	for( itrCtrl = this->tunerBankMap.begin(); itrCtrl != this->tunerBankMap.end(); itrCtrl++ ){
		if( itrCtrl->second->IsOpenErr() == TRUE ){
			vector<CReserveInfo*> reserveInfo;
			itrCtrl->second->GetOpenErrReserve(&reserveInfo);

			for( size_t i=0 ;i<reserveInfo.size(); i++ ){
				//バンクから削除
				RESERVE_DATA data;
				reserveInfo[i]->GetData(&data);

				reserveInfo[i]->AddNGTunerID(itrCtrl->first);
				reserveInfo[i]->SetRecWaitMode(FALSE, 0);
				reserveInfo[i]->SetOpenErred();

				itrCtrl->second->DeleteReserve(data.reserveID);
			}
			
			itrCtrl->second->ResetOpenErr();
			needNotify = TRUE;
		}
	}
	/*
	//移動できないものエラーとして削除
	if( NGAddReserve.size() > 0 ){
		BYTE suspendMode = 0xFF;
		BYTE rebootFlag = 0xFF;

		vector<DWORD> deleteList;
		for( size_t i=0; i<NGAddReserve.size(); i++ ){
			deleteList.push_back(NGAddReserve[i]->reserveID);

			RESERVE_DATA data;
			NGAddReserve[i]->reserveInfo->GetData(&data);
			REC_FILE_INFO item;
			item = data;
			item.recStatus = REC_END_STATUS_OPEN_ERR;
			item.comment = L"チューナーのオープンに失敗しました";
			this->recInfoText.AddRecInfo(&item);
			_SendTweet(TW_REC_END, &item, NULL, NULL);
			_SendNotifyRecEnd(&item);


			SAFE_DELETE(NGAddReserve[i]);

			suspendMode = data.recSetting.suspendMode;
			rebootFlag = data.recSetting.rebootFlag;
		}
		_DelReserveData(&deleteList);

		//情報ファイルの更新
		wstring filePath = L"";
		GetSettingPath(filePath);
		filePath += L"\\";
		filePath += RESERVE_TEXT_NAME;

		this->reserveText.SaveReserveText(filePath.c_str());

		wstring recFilePath = L"";
		GetSettingPath(recFilePath);
		recFilePath += L"\\";
		recFilePath += REC_INFO_TEXT_NAME;

		this->recInfoText.SaveRecInfoText(recFilePath.c_str());

		needNotify = TRUE;

		if( suspendMode != 0xFF && rebootFlag != 0xFF ){
			EnableSuspendWork(suspendMode, rebootFlag, 0);
		}
	}
	*/
	if( needNotify == TRUE ){
		_ReloadBankMap();

		BYTE suspendMode = 0xFF;
		BYTE rebootFlag = 0xFF;
		map<DWORD, BANK_WORK_INFO*>::iterator itrNG;
		for( itrNG = NGReserveMap.begin(); itrNG != NGReserveMap.end(); itrNG++ ){
			if( itrNG->second->reserveInfo->IsOpenErred() == TRUE ){
				RESERVE_DATA data;
				itrNG->second->reserveInfo->GetData(&data);
				data.comment = L"チューナーのオープンに失敗しました";

				this->reserveText.ChgReserve(data);
				itrNG->second->reserveInfo->SetData(&data);

				suspendMode = data.recSetting.suspendMode;
				rebootFlag = data.recSetting.rebootFlag;
			}
		}

		_SendNotifyUpdate(NOTIFY_UPDATE_RESERVE_INFO);
		_SendNotifyUpdate(NOTIFY_UPDATE_REC_INFO);

		if( suspendMode != 0xFF && rebootFlag != 0xFF ){
			EnableSuspendWork(suspendMode, rebootFlag, 0);
		}
	}
}

void CReserveManager::CheckBatWork()
{
	if( this->batManager.GetWorkCount() > 0 ){
		if( this->batMargin != 0 ){
			map<DWORD, CTunerBankCtrl*>::iterator itrCtrl;
			for( itrCtrl = this->tunerBankMap.begin(); itrCtrl != this->tunerBankMap.end(); itrCtrl++ ){
				if( itrCtrl->second->IsOpenTuner() == TRUE ){
					//起動中なので予約処理中
					this->batManager.PauseWork();
					return ;
				}
			}

			LONGLONG chkTime = GetNowI64Time() + this->batMargin*60*I64_1SEC;
			vector<pair<LONGLONG, const RESERVE_DATA*> > resList = this->reserveText.GetReserveList(TRUE, this->defStartMargine);
			vector<pair<LONGLONG, const RESERVE_DATA*> >::iterator itr;
			for( itr = resList.begin(); itr != resList.end(); itr++ ){
				if( itr->second->recSetting.recMode != RECMODE_VIEW && itr->second->recSetting.recMode != RECMODE_NO ){
					LONGLONG startTime;
					LONGLONG endTime;
					CalcEntireReserveTime(&startTime, &endTime, *itr->second);

					if( startTime <= chkTime && chkTime < endTime ){
						//次の予約時間にかぶる
						this->batManager.PauseWork();
						return;
					}
					break;
				}
			}

		}

		this->batManager.StartWork();
	}else{
		if( this->batManager.IsWorking() == FALSE ){
			BYTE suspendMode = 0;
			BYTE rebootFlag = 0;
			if( this->batManager.GetLastWorkSuspend(&suspendMode, &rebootFlag) == TRUE ){
				//バッチ処理終わったのでサスペンド処理に挑戦
				EnableSuspendWork(suspendMode, rebootFlag, 0);
			}
		}
	}
}

void CReserveManager::CheckTuijyu()
{
	//録画処理中のチューナー一覧
	map<DWORD, CTunerBankCtrl*> chkTuner;
	map<DWORD, CTunerBankCtrl*>::iterator itrCtrl;
	for( itrCtrl = this->tunerBankMap.begin(); itrCtrl != this->tunerBankMap.end(); itrCtrl++ ){
		DWORD chID = 0;
		if( itrCtrl->second->GetCurrentChID(&chID) == TRUE ){
			chkTuner.insert(pair<DWORD, CTunerBankCtrl*>(chID, itrCtrl->second));
		}
	}

	//予約の状態を確認する
	vector<DWORD> deleteList;

	BOOL chgReserve = FALSE;
	map<DWORD, CReserveInfo*>::iterator itrRes;
	BOOL chk6h = FALSE;
	if( this->reserveInfoMap.size() > 50 ){
		chk6h = TRUE;
	}
	for( itrRes = this->reserveInfoMap.begin(); itrRes != this->reserveInfoMap.end(); itrRes++ ){
		Sleep(0);
		RESERVE_DATA data;
		itrRes->second->GetData(&data);

		if( data.recSetting.recMode == RECMODE_NO ){
			continue;
		}
		if( data.eventID == 0xFFFF ){
			continue;
		}
		if( data.recSetting.tuijyuuFlag == 0 ){
			continue;
		}
		if( chk6h == TRUE ){
			if( ConvertI64Time(data.startTime) > GetNowI64Time() + 3*60*60*I64_1SEC ){
				continue;
			}
		}
	
		DWORD chkChID = ((DWORD)data.originalNetworkID)<<16 | data.transportStreamID;

		itrCtrl = chkTuner.find(chkChID);
		if( itrCtrl != chkTuner.end() ){
			BOOL chgRes = FALSE;
			BOOL recWaitFlag = FALSE;
			DWORD tunerID = 0;
			itrRes->second->GetRecWaitMode(&recWaitFlag, &tunerID);
			if( recWaitFlag == 0 ){
				//通常チェック
				SEARCH_EPG_INFO_PARAM val;
				val.ONID = data.originalNetworkID;
				val.TSID = data.transportStreamID;
				val.SID = data.serviceID;
				val.eventID = data.eventID;
				val.pfOnlyFlag = 0;
				EPGDB_EVENT_INFO resVal;
				if( itrCtrl->second->SearchEpgInfo(
					&val,
					&resVal
					) == TRUE ){
						chgRes = CheckChgEvent(&resVal, &data);
						if( chgRes == TRUE ){
							//開始時間6時間以内ならEPG再読み込みで変更されないようにする
							if( GetNowI64Time() + 6*60*60*I64_1SEC > ConvertI64Time(data.startTime) ){
								if( data.reserveStatus == ADD_RESERVE_NORMAL ){
									data.reserveStatus = ADD_RESERVE_CHG_PF2;
								}
							}
						}
				}else{
					OutputDebugString( L"番組情報みつからず ");
				}
				if( chgRes == TRUE ){
					_ChgReserveData( &data, TRUE );
					chgReserve = TRUE;
				}
			}else{
				//録画中
				GET_EPG_PF_INFO_PARAM valPF;
				valPF.ONID = data.originalNetworkID;
				valPF.TSID = data.transportStreamID;
				valPF.SID = data.serviceID;
				valPF.pfNextFlag = 0;

				EPGDB_EVENT_INFO resNowVal;
				EPGDB_EVENT_INFO resNextVal;
				BOOL nowSuccess = itrCtrl->second->GetEventPF( &valPF, &resNowVal );
				valPF.pfNextFlag = 1;
				BOOL nextSuccess = itrCtrl->second->GetEventPF( &valPF, &resNextVal );

				BOOL findPF = FALSE;
				BOOL endRec = FALSE;
				if( nowSuccess == TRUE ){
					if( resNowVal.event_id == data.eventID ){
						findPF = TRUE;
						if( itrRes->second->IsChkPfInfo() == FALSE ){
							itrRes->second->SetChkPfInfo(TRUE);
						}
						if( resNowVal.StartTimeFlag == 1 && resNowVal.DurationFlag == 1 ){
							//通常チェック
							BYTE chgMode = 0;
							chgRes = CheckChgEvent(&resNowVal, &data, &chgMode);
							if( chgRes == TRUE && (chgMode&0x02) == 0x02 ){
								//総時間変更された
								data.durationSecond += (DWORD)this->duraChgMarginMin*60;
								//同一サービスで録画中になっているやつの開始時間変更してやる
								ChgDurationChk(&resNowVal);
							}
						}else{
							//時間未定
							if( resNowVal.StartTimeFlag == 1 ){
								data.startTime = resNowVal.start_time;
							}
							LONGLONG dureSec = GetNowI64Time() - ConvertI64Time(data.startTime) + 10*60*I64_1SEC;
							if( (DWORD)(dureSec/I64_1SEC) > data.durationSecond ){
								data.durationSecond = (DWORD)(dureSec/I64_1SEC);
								data.reserveStatus = ADD_RESERVE_UNKNOWN_END;
								chgRes = TRUE;
							}
							_OutputDebugString(L"●p/f 時間未定現在情報に存在 %d/%d/%d %d:%d:%d %dsec %s %s\r\n",
								data.startTime.wYear,
								data.startTime.wMonth,
								data.startTime.wDay,
								data.startTime.wHour,
								data.startTime.wMinute,
								data.startTime.wSecond,
								data.durationSecond,
								data.title.c_str(),
								data.stationName.c_str()
								);
						}
					}
				}
				if( nextSuccess == TRUE ){
					if( resNextVal.event_id == data.eventID ){
						findPF = TRUE;
						if( resNextVal.StartTimeFlag == 1 && resNextVal.DurationFlag == 1 ){
							//通常チェック
							chgRes = CheckChgEvent(&resNextVal, &data);
						}else{
							//総時間未定
							if( resNextVal.StartTimeFlag == 1 && resNowVal.StartTimeFlag == 1 ){
								if( resNowVal.DurationFlag == 1 ){
									if( ConvertI64Time(resNextVal.start_time) < GetSumTime(resNowVal.start_time, resNowVal.durationSec) ){
										//次の方が開始時間が現在の間にあるとかおかしい
										endRec = TRUE;
										_OutputDebugString(L"●p/f 不正状態（現在終了＞次開始 %d/%d/%d %d:%d:%d %dsec %s %s\r\n",
											data.startTime.wYear,
											data.startTime.wMonth,
											data.startTime.wDay,
											data.startTime.wHour,
											data.startTime.wMinute,
											data.startTime.wSecond,
											data.durationSecond,
											data.title.c_str(),
											data.stationName.c_str()
											);
										if( this->eventRelay == TRUE ){
											if( CheckEventRelay(&resNextVal, &data, TRUE) == TRUE ){
												chgReserve = TRUE;
											}
										}
									}
								}else{
									if( ConvertI64Time(resNextVal.start_time) < ConvertI64Time(resNowVal.start_time) ){
										//次の方が開始時間早いとかおかしい
										endRec = TRUE;
										_OutputDebugString(L"●p/f 不正状態（現在開始＞次開始 %d/%d/%d %d:%d:%d %dsec %s %s\r\n",
											data.startTime.wYear,
											data.startTime.wMonth,
											data.startTime.wDay,
											data.startTime.wHour,
											data.startTime.wMinute,
											data.startTime.wSecond,
											data.durationSecond,
											data.title.c_str(),
											data.stationName.c_str()
											);
										if( this->eventRelay == TRUE ){
											if( CheckEventRelay(&resNextVal, &data, TRUE) == TRUE ){
												chgReserve = TRUE;
											}
										}
									}
								}
							}
							if( endRec == FALSE ){
								if( resNextVal.StartTimeFlag == 1 ){
									data.startTime = resNextVal.start_time;
								}
								LONGLONG dureSec = GetNowI64Time() - ConvertI64Time(data.startTime) + 10*60*I64_1SEC;
								if( (DWORD)(dureSec/I64_1SEC) > data.durationSecond ){
									data.durationSecond = (DWORD)(dureSec/I64_1SEC);
									data.reserveStatus = ADD_RESERVE_UNKNOWN_END;
									chgRes = TRUE;
								}
								_OutputDebugString(L"●p/f 次情報に存在 %d/%d/%d %d:%d:%d %dsec %s %s\r\n",
									data.startTime.wYear,
									data.startTime.wMonth,
									data.startTime.wDay,
									data.startTime.wHour,
									data.startTime.wMinute,
									data.startTime.wSecond,
									data.durationSecond,
									data.title.c_str(),
									data.stationName.c_str()
									);
							}
						}
					}
				}
				if( findPF == FALSE && (itrRes->second->IsChkPfInfo() == FALSE )){
					//現在or次ではない
					BOOL chkNormal = TRUE;
					if( nowSuccess == TRUE){
						if(resNowVal.StartTimeFlag != 1 || resNowVal.DurationFlag != 1){
							//時間未定なので6時間追従モードへ
							chkNormal = FALSE;
						}
					}
					if( nextSuccess == TRUE){
						if( resNextVal.StartTimeFlag != 1 || resNextVal.DurationFlag != 1 ){
							//時間未定なので6時間追従モードへ
							chkNormal = FALSE;
						}
					}
					if( chkNormal == FALSE ){
						//時間未定なので6時間追従モードへ
						if( CheckNotFindChgEvent(&data, itrCtrl->second, &deleteList) == TRUE ){
							chgReserve = TRUE;
						}
					}else{
						//イベントID直前変更対応
						BOOL chkChgID = FALSE;
						if( nowSuccess == TRUE ){
							if( CheckChgEventID(&resNowVal, &data) == TRUE ){
								chgRes = TRUE;
								chkChgID = TRUE;
							}
						}
						if(chkChgID == FALSE){
							//p/f正常なので通常検索
							SEARCH_EPG_INFO_PARAM val;
							val.ONID = data.originalNetworkID;
							val.TSID = data.transportStreamID;
							val.SID = data.serviceID;
							val.eventID = data.eventID;
							val.pfOnlyFlag = 0;
							EPGDB_EVENT_INFO resVal;
							if( itrCtrl->second->SearchEpgInfo(
								&val,
								&resVal
								) == TRUE ){
									if( data.reserveStatus == ADD_RESERVE_NO_FIND ){
										if( resVal.StartTimeFlag == 1 ){
											if( ConvertI64Time(resVal.start_time) > ConvertI64Time(data.startTime) ){
												//開始時間後なので更新されたEPGのはず
												chgRes = CheckChgEvent(&resVal, &data);
											}else{
												//古いEPGなので何もしない
											}
										}
									}else{
										chgRes = CheckChgEvent(&resVal, &data);
									}
							}else{
								_OutputDebugString(L"●番組情報みつからず %d/%d/%d %d:%d:%d %dsec %s %s\r\n",
									data.startTime.wYear,
									data.startTime.wMonth,
									data.startTime.wDay,
									data.startTime.wHour,
									data.startTime.wMinute,
									data.startTime.wSecond,
									data.durationSecond,
									data.title.c_str(),
									data.stationName.c_str()
									);
								if( nowSuccess == FALSE && nextSuccess == FALSE ){
									if( data.reserveStatus == ADD_RESERVE_NORMAL ){
										LONGLONG delay = itrCtrl->second->DelayTime();
										LONGLONG nowTime = GetNowI64Time() + delay;
										LONGLONG endTime = GetSumTime(data.startTime, data.durationSecond);
										LONGLONG endTimeE;
										CalcEntireReserveTime(NULL, &endTimeE, data);
										if( endTime > endTimeE ){
											endTime = endTimeE;
										}
										if( nowTime + 2*60*I64_1SEC > endTime ){
											//終了2分前だけどEPGなし
											data.reserveStatus = ADD_RESERVE_NO_EPG;
											data.durationSecond += this->noEpgTuijyuMin * 60;
											chgRes = TRUE;
										}
									}
								}
							}
						}
					}
				}
				if( endRec == TRUE ){
					//誤差も加味
					LONGLONG delay = itrCtrl->second->DelayTime();
					chgRes = TRUE;
					LONGLONG dureSec = GetNowI64Time()+delay - ConvertI64Time(data.startTime);
					data.durationSecond = (DWORD)(dureSec/I64_1SEC);
					//録画時間過ぎている状態作るために終了マージンを-1分にしてやる
					data.recSetting.useMargineFlag = 1;
					data.recSetting.startMargine = 0;
					data.recSetting.endMargine = -60;
					_OutputDebugString(L"●情報確認できず終了 %d/%d/%d %d:%d:%d %dsec %s %s\r\n",
						data.startTime.wYear,
						data.startTime.wMonth,
						data.startTime.wDay,
						data.startTime.wHour,
						data.startTime.wMinute,
						data.startTime.wSecond,
						data.durationSecond,
						data.title.c_str(),
						data.stationName.c_str()
						);
				}
				if( chgRes == TRUE ){
					if( data.reserveStatus == ADD_RESERVE_NORMAL || data.reserveStatus == ADD_RESERVE_CHG_PF2 ){
						data.reserveStatus = ADD_RESERVE_CHG_PF;
					}
					_ChgReserveData( &data, TRUE );
					chgReserve = TRUE;
				}

				if( this->eventRelay == TRUE ){
					//イベントリレーのチェック
					if( nowSuccess == TRUE){
						if( CheckEventRelay(&resNowVal, &data) == TRUE ){
							chgReserve = TRUE;
						}
					}
				}
			}
		}
	}

	if( deleteList.size() > 0 ){
		//予約一覧から削除
		_DelReserveData(&deleteList);
		chgReserve = TRUE;
	}
	if( chgReserve == TRUE ){
		this->reserveText.SaveText();

		_ReloadBankMap();

		_SendNotifyUpdate(NOTIFY_UPDATE_RESERVE_INFO);
		_SendNotifyUpdate(NOTIFY_UPDATE_REC_INFO);
	}
}

BOOL CReserveManager::CheckChgEvent(EPGDB_EVENT_INFO* info, RESERVE_DATA* data, BYTE* chgMode)
{
	BOOL chgRes = FALSE;

	RESERVE_DATA oldData = *data;

	wstring log = L"";
	wstring timeLog1 = L"";
	wstring timeLog2 = L"";

	SYSTEMTIME oldEndTime;
	GetSumTime(data->startTime, data->durationSecond, &oldEndTime);
	Format(timeLog1, L"%d/%d/%d %d:%d:%d～%d:%d:%d",
		data->startTime.wYear,
		data->startTime.wMonth,
		data->startTime.wDay,
		data->startTime.wHour,
		data->startTime.wMinute,
		data->startTime.wSecond,
		oldEndTime.wHour,
		oldEndTime.wMinute,
		oldEndTime.wSecond);

	SYSTEMTIME endTime;
	if( info->StartTimeFlag == 1 && info->DurationFlag == 1){
		GetSumTime(info->start_time, info->durationSec, &endTime);
		Format(timeLog2, L"%d/%d/%d %d:%d:%d～%d:%d:%d",
			info->start_time.wYear,
			info->start_time.wMonth,
			info->start_time.wDay,
			info->start_time.wHour,
			info->start_time.wMinute,
			info->start_time.wSecond,
			endTime.wHour,
			endTime.wMinute,
			endTime.wSecond);
	}else if( info->StartTimeFlag == 1 && info->DurationFlag == 0){
		Format(timeLog2, L"%d/%d/%d %d:%d:%d～未定",
			info->start_time.wYear,
			info->start_time.wMonth,
			info->start_time.wDay,
			info->start_time.wHour,
			info->start_time.wMinute,
			info->start_time.wSecond);
	}else{
		timeLog2 = L"時間未定";
	}

	if( info->StartTimeFlag == 1 && info->DurationFlag == 1){
		if(GetNowI64Time() > GetSumTime(info->start_time, info->durationSec)){
			//終了時間を過ぎているので追従はしない
			return FALSE;
		}
	}
	if( info->StartTimeFlag == 1 ){
		if( ConvertI64Time(data->startTime) != ConvertI64Time(info->start_time) ){
			//開始時間変わっている
			chgRes = TRUE;
			data->startTime = info->start_time;

			log += L"●追従：開始変更 ";
			if( chgMode != NULL ){
				*chgMode |= 0x01;
			}
		}
	}
	if( info->DurationFlag == 1 ){
		if( data->reserveStatus == ADD_RESERVE_CHG_PF ){
			//一度変わってるのでマージン追加されてるはず
			if( data->durationSecond - ((DWORD)this->duraChgMarginMin*60) != info->durationSec ){
				//総時間が変更されている
				chgRes = TRUE;
				data->durationSecond = info->durationSec;
				log += L"●追従：総時間変更 ";
				if( chgMode != NULL ){
					*chgMode |= 0x02;
				}
			}
		}else{
			if( data->durationSecond != info->durationSec ){
				//総時間が変更されている
				chgRes = TRUE;
				data->durationSecond = info->durationSec;
				log += L"●追従：総時間変更 ";
				if( chgMode != NULL ){
					*chgMode |= 0x02;
				}
			}
		}
	}

	if( chgRes == TRUE ){
		log += data->stationName;
		log += L" ";
		log += timeLog1;
		log += L" → ";
		log += timeLog2;
		log += L" ";
		log += data->title;
		log += L"\r\n";
		_SendTweet(TW_CHG_RESERVE_CHK_REC, &oldData, data, info);
		_SendNotifyChgReserve(NOTIFY_UPDATE_REC_TUIJYU, &oldData, data);
	}else{
		log += data->stationName;
		log += L" ";
		log += timeLog2;
		log += L" ";
		log += data->title;
		log += L"\r\n";
	}

	OutputDebugString(log.c_str());
	return chgRes;
}

BOOL CReserveManager::CheckChgEventID(EPGDB_EVENT_INFO* info, RESERVE_DATA* data)
{
	if( info == NULL || data == NULL ){
		return FALSE;
	}
	if( data->reserveStatus == ADD_RESERVE_RELAY ){
		//イベントリレーで追加された物はチェックしない
		return FALSE;
	}
	BOOL chgEventID = FALSE;
	if( info->shortInfo != NULL && (info->event_id != data->eventID)){
		//似たタイトルになっているかチェック
		wstring title1 = data->title;
		wstring title2 = info->shortInfo->event_name;
		if( title1.size() > 0 && title2.size() > 0){
			//記号を除去
			while( (title1.find(L"[") != string::npos) && (title1.find(L"]") != string::npos) ){
				wstring strSep1=L"";
				wstring strSep2=L"";
				Separate(title1, L"[", strSep1, title1);
				Separate(title1, L"]", strSep2, title1);
				strSep1 += title1;
				title1 = strSep1;
			}
			while( (title2.find(L"[") != string::npos) && (title2.find(L"]") != string::npos) ){
				wstring strSep1=L"";
				wstring strSep2=L"";
				Separate(title2, L"[", strSep1, title2);
				Separate(title2, L"]", strSep2, title2);
				strSep1 += title2;
				title2 = strSep1;
			}
			//空白除去
			Replace(title1, L" ", L"");
			Replace(title2, L" ", L"");
			//比較用に文字調整
			this->epgDBManager->ConvertSearchText(title1);
			this->epgDBManager->ConvertSearchText(title2);

			//あいまい検索
			DWORD hitCount = 0;
			DWORD missCount = 0;
			wstring key= L"";
			for( size_t j=0; j<title1.size(); j++ ){
				key += title1.at(j);
				if( title2.find(key) == string::npos ){
					missCount+=1;
					key = title1.at(j);
					if( title2.find(key) == string::npos ){
						missCount+=1;
						key = L"";
					}
				}else{
					hitCount+=(DWORD)key.size();
				}
			}
			DWORD samePer1 = 0;
			if( hitCount+missCount > 0 ){
				samePer1 = (hitCount*100) / (hitCount+missCount);
			}

			hitCount = 0;
			missCount = 0;
			key= L"";
			for( size_t j=0; j<title2.size(); j++ ){
				key += title2.at(j);
				if( title1.find(key) == string::npos ){
					missCount+=1;
					key = title2.at(j);
					if( title1.find(key) == string::npos ){
						missCount+=1;
						key = L"";
					}
				}else{
					hitCount+=(DWORD)key.size();
				}
			}
			DWORD samePer2 = 0;
			if( hitCount+missCount > 0 ){
				samePer2 = (hitCount*100) / (hitCount+missCount);
			}

			if( samePer1 > 80 || samePer2 > 80 ){
				//80%以上の一致で一緒とする
				chgEventID = TRUE;
			}
			/*
			if( data->title.find(nowTitle) != string::npos){
				chgEventID = TRUE;
			}else if( nowTitle.find(data->title) != string::npos ){
				chgEventID = TRUE;
			}*/
		}
	}
	if( chgEventID == FALSE ){
		return FALSE;
	}


	BOOL chgRes = TRUE;

	RESERVE_DATA oldData = *data;

	wstring log = L"";
	wstring timeLog1 = L"";
	wstring timeLog2 = L"";

	SYSTEMTIME oldEndTime;
	GetSumTime(data->startTime, data->durationSecond, &oldEndTime);
	Format(timeLog1, L"%d/%d/%d %d:%d:%d～%d:%d:%d",
		data->startTime.wYear,
		data->startTime.wMonth,
		data->startTime.wDay,
		data->startTime.wHour,
		data->startTime.wMinute,
		data->startTime.wSecond,
		oldEndTime.wHour,
		oldEndTime.wMinute,
		oldEndTime.wSecond);

	SYSTEMTIME endTime;
	if( info->StartTimeFlag == 1 && info->DurationFlag == 1){
		GetSumTime(info->start_time, info->durationSec, &endTime);
		Format(timeLog2, L"%d/%d/%d %d:%d:%d～%d:%d:%d",
			info->start_time.wYear,
			info->start_time.wMonth,
			info->start_time.wDay,
			info->start_time.wHour,
			info->start_time.wMinute,
			info->start_time.wSecond,
			endTime.wHour,
			endTime.wMinute,
			endTime.wSecond);
	}else if( info->StartTimeFlag == 1 && info->DurationFlag == 0){
		Format(timeLog2, L"%d/%d/%d %d:%d:%d～未定",
			info->start_time.wYear,
			info->start_time.wMonth,
			info->start_time.wDay,
			info->start_time.wHour,
			info->start_time.wMinute,
			info->start_time.wSecond);
	}else{
		timeLog2 = L"時間未定";
	}

	if( info->StartTimeFlag == 1 && info->DurationFlag == 1){
		if(GetNowI64Time() > GetSumTime(info->start_time, info->durationSec)){
			//終了時間を過ぎているので追従はしない
			return FALSE;
		}
	}
	if( info->StartTimeFlag == 1 ){
		if( ConvertI64Time(data->startTime) != ConvertI64Time(info->start_time) ){
			//開始時間変わっている
			chgRes = TRUE;
			data->startTime = info->start_time;

			Format(log ,L"●追従 EventID変更 0x%04X -> 0x%04X：開始変更 ", oldData.eventID, info->event_id);
		}
	}
	if( info->DurationFlag == 1 ){
		if( data->reserveStatus == ADD_RESERVE_CHG_PF ){
			//一度変わってるのでマージン追加されてるはず
			if( data->durationSecond - ((DWORD)this->duraChgMarginMin*60) != info->durationSec ){
				//総時間が変更されている
				chgRes = TRUE;
				data->durationSecond = info->durationSec;
				Format(log ,L"●追従 EventID変更 0x%04X -> 0x%04X：総時間変更 ", oldData.eventID, info->event_id);
			}
		}else{
			if( data->durationSecond != info->durationSec ){
				//総時間が変更されている
				chgRes = TRUE;
				data->durationSecond = info->durationSec;
				Format(log ,L"●追従 EventID変更 0x%04X -> 0x%04X：総時間変更 ", oldData.eventID, info->event_id);
			}
		}
	}
	data->eventID = info->event_id;

	if( chgRes == TRUE ){
		log += data->stationName;
		log += L" ";
		log += timeLog1;
		log += L" → ";
		log += timeLog2;
		log += L" ";
		log += data->title;
		log += L"\r\n";
		_SendTweet(TW_CHG_RESERVE_CHK_REC, &oldData, data, info);
		_SendNotifyChgReserve(NOTIFY_UPDATE_REC_TUIJYU, &oldData, data);
	}else{
		log += data->stationName;
		log += L" ";
		log += timeLog2;
		log += L" ";
		log += data->title;
		log += L"\r\n";
	}

	OutputDebugString(log.c_str());
	return chgRes;
}

BOOL CReserveManager::CheckNotFindChgEvent(RESERVE_DATA* data, CTunerBankCtrl* ctrl, vector<DWORD>* deleteList)
{
	BOOL chgRes = FALSE;
	wstring log = L"";

	LONGLONG delay = ctrl->DelayTime();
	LONGLONG nowTime = GetNowI64Time()+delay;

	if( data->reserveStatus == ADD_RESERVE_RELAY ){
		//イベントリレーは追従の必要なし
	}else if(data->reserveStatus == ADD_RESERVE_NORMAL || data->reserveStatus == ADD_RESERVE_CHG_PF){
		//6時間追従用予約へ移行
		LONGLONG delay = ctrl->DelayTime();
		LONGLONG nowTime = GetNowI64Time()+delay;
		//if( nowTime + 60*I64_1SEC > chkEndTime ){
			//終了1分前で番組情報みつからず
			OutputDebugString(L"●6時間追従用予約へ移行");

			//開始時間を延ばす
			LONGLONG chgStart = nowTime - 30*I64_1SEC;
			LONGLONG chgStartE;
			ConvertSystemTime( chgStart, &data->startTime);
			CalcEntireReserveTime(&chgStartE, NULL, *data);
			if( chgStart < chgStartE ){
				chgStart -= chgStartE - chgStart;
			}
			ConvertSystemTime( chgStart, &data->startTime);
			data->reserveStatus = ADD_RESERVE_NO_FIND;
			_ChgReserveData( data, TRUE );
			chgRes = TRUE;

			ctrl->ReRec(data->reserveID, FALSE);
		//}
	}else if(data->reserveStatus == ADD_RESERVE_NO_FIND){
		if( ConvertI64Time(data->startTimeEpg) + ((LONGLONG)this->notFindTuijyuHour)*60*60*I64_1SEC < nowTime ){
			OutputDebugString(L"●指定時間番組情報みつからず");
			REC_FILE_INFO item;
			item = *data;
			item.recFilePath = L"";
			item.drops = 0;
			item.scrambles = 0;
			item.recStatus = REC_END_STATUS_NOT_FIND_6H;
			item.comment = L"指定時間番組情報が見つかりませんでした";
			this->recInfoText.AddRecInfo(item);
			_SendTweet(TW_REC_END, &item, NULL, NULL);
			_SendNotifyRecEnd(&item);

			deleteList->push_back(data->reserveID);

		}else{
			//開始時間を延ばす
			LONGLONG chgStart = nowTime - 30*I64_1SEC;
			LONGLONG chgStartE;
			ConvertSystemTime( chgStart, &data->startTime);
			CalcEntireReserveTime(&chgStartE, NULL, *data);
			if( chgStart < chgStartE ){
				chgStart -= chgStartE - chgStart;
			}
			ConvertSystemTime( chgStart, &data->startTime);
			_ChgReserveData( data, TRUE );

			ctrl->ReRec(data->reserveID, TRUE);
		}
		chgRes = TRUE;
	}

	return chgRes;
}

BOOL CReserveManager::CheckEventRelay(EPGDB_EVENT_INFO* info, RESERVE_DATA* data, BOOL errEnd)
{
	BOOL add = FALSE;
	if( info == NULL ){
		return add;
	}
	if( info->original_network_id != data->originalNetworkID ||
		info->transport_stream_id != data->transportStreamID ||
		info->service_id != data->serviceID ||
		info->event_id != data->eventID ){
			//イベントIDの確認
			return add;
	}
	if( info->eventRelayInfo != NULL ){
		if( errEnd == TRUE ){
			OutputDebugString(L"イベントリレーチェック　総時間異常終了");
		}else{
			if( info->StartTimeFlag == 0 || info->DurationFlag == 0 ){
				OutputDebugString(L"イベントリレーチェック　開始 or 総時間未定");
				return add;
			}
		}
		//イベントリレーあり
		wstring title;
		if( info->shortInfo != NULL ){
			title = info->shortInfo->event_name;
		}
		_OutputDebugString(L"EventRelayCheck");
		_OutputDebugString(L"OriginalEvent : ONID 0x%08x TSID 0x%08x SID 0x%08x EventID 0x%08x %s", 
			info->original_network_id,
			info->transport_stream_id,
			info->service_id,
			info->event_id,
			title.c_str()
			);
		for( size_t i=0; i<info->eventRelayInfo->eventDataList.size(); i++ ){
			LONGLONG chKey = _Create64Key(
				info->eventRelayInfo->eventDataList[i].original_network_id,
				info->eventRelayInfo->eventDataList[i].transport_stream_id,
				info->eventRelayInfo->eventDataList[i].service_id);

			_OutputDebugString(L"RelayEvent : ONID 0x%08x TSID 0x%08x SID 0x%08x EventID 0x%08x", 
				info->eventRelayInfo->eventDataList[i].original_network_id,
				info->eventRelayInfo->eventDataList[i].transport_stream_id,
				info->eventRelayInfo->eventDataList[i].service_id,
				info->eventRelayInfo->eventDataList[i].event_id
				);
			map<LONGLONG, CH_DATA5>::const_iterator itrCh;
			itrCh = this->chUtil.GetMap().find(chKey);
			if( itrCh != this->chUtil.GetMap().end() ){
				//使用できるチャンネル発見
				_OutputDebugString(L"Service find : %s", 
					itrCh->second.serviceName.c_str()
					);

				//同一イベント予約済みかチェック
				BOOL find = FALSE;
				vector<pair<LONGLONG, const RESERVE_DATA*> > resList = this->reserveText.GetReserveList();
				vector<pair<LONGLONG, const RESERVE_DATA*> >::iterator itrRes;
				for( itrRes = resList.begin(); itrRes != resList.end(); itrRes++ ){
					if( itrRes->second->originalNetworkID == info->eventRelayInfo->eventDataList[i].original_network_id &&
						itrRes->second->transportStreamID == info->eventRelayInfo->eventDataList[i].transport_stream_id &&
						itrRes->second->serviceID == info->eventRelayInfo->eventDataList[i].service_id &&
						itrRes->second->eventID == info->eventRelayInfo->eventDataList[i].event_id ){
							//予約済み
							find = TRUE;
							//追加済みなら異常終了のために開始時間変更する必要なし
							if( errEnd == FALSE ){
								//時間変更必要かチェック
								SYSTEMTIME chkStart;
								GetSumTime(info->start_time, info->durationSec, &chkStart);
								if( ConvertI64Time(itrRes->second->startTime) != ConvertI64Time(chkStart) ){
									RESERVE_DATA chgData = *(itrRes->second);
									//開始時間変わっている
									add = TRUE;
									chgData.startTime = chkStart;
									chgData.startTimeEpg = chgData.startTime;
									_ChgReserveData( &chgData, TRUE );
									OutputDebugString(L"★イベントリレー開始変更");
								}
							}
							break;
					}
				}
				if( errEnd == FALSE ){
					map<DWORD, REC_FILE_INFO>::const_iterator itrRecInfo;
					for( itrRecInfo = this->recInfoText.GetMap().begin(); itrRecInfo != this->recInfoText.GetMap().end(); itrRecInfo++ ){
						if( itrRecInfo->second.originalNetworkID == info->eventRelayInfo->eventDataList[i].original_network_id &&
							itrRecInfo->second.transportStreamID == info->eventRelayInfo->eventDataList[i].transport_stream_id &&
							itrRecInfo->second.serviceID == info->eventRelayInfo->eventDataList[i].service_id &&
							itrRecInfo->second.eventID == info->eventRelayInfo->eventDataList[i].event_id 
							){
								if( ConvertI64Time(itrRecInfo->second.startTime) == GetSumTime(info->start_time, info->durationSec)){
									//エラーで録画終了になった？
									find = TRUE;
									break;
								}
						}
					}
				}
				if( find == FALSE ){
					RESERVE_DATA addItem;
					if( data->title.find(L"(イベントリレー)") == string::npos ){
						addItem.title = L"(イベントリレー)";
					}
					addItem.title += data->title;
					if( errEnd == TRUE ){
						GetLocalTime(&addItem.startTime);
					}else{
						GetSumTime(info->start_time, info->durationSec, &addItem.startTime);
					}
					addItem.startTimeEpg = addItem.startTime;
					addItem.durationSecond = 10*60;
					addItem.stationName = itrCh->second.serviceName;
					addItem.originalNetworkID = info->eventRelayInfo->eventDataList[i].original_network_id;
					addItem.transportStreamID = info->eventRelayInfo->eventDataList[i].transport_stream_id;
					addItem.serviceID = info->eventRelayInfo->eventDataList[i].service_id;
					addItem.eventID = info->eventRelayInfo->eventDataList[i].event_id;

					addItem.recSetting = data->recSetting;
					addItem.reserveStatus = ADD_RESERVE_RELAY;
					addItem.recSetting.tunerID = 0;
					_AddReserveData(&addItem);
					add = TRUE;
					OutputDebugString(L"★イベントリレー追加");
				}
				break;
			}else{
					OutputDebugString(L"Service Not find");
			}
		}
	}
	return add;
}

BOOL CReserveManager::ChgDurationChk(EPGDB_EVENT_INFO* info)
{
	if( info == NULL ){
		return FALSE;
	}
	if( info->StartTimeFlag == 0 || info->DurationFlag == 0 ){
		return FALSE;
	}
	BOOL ret = FALSE;
	map<DWORD, CReserveInfo*>::iterator itrRes;
	for( itrRes = this->reserveInfoMap.begin(); itrRes != this->reserveInfoMap.end(); itrRes++ ){
		RESERVE_DATA data;
		itrRes->second->GetData(&data);

		if( data.recSetting.recMode == RECMODE_NO ){
			continue;
		}
		if( data.eventID == 0xFFFF ){
			continue;
		}
		if( data.recSetting.tuijyuuFlag == 0 ){
			continue;
		}
		BOOL recWaitFlag = FALSE;
		DWORD tunerID = 0;
		itrRes->second->GetRecWaitMode(&recWaitFlag, &tunerID);
		if( recWaitFlag == 0 ){
			continue;
		}
		//p/f確認できてるのに変更はおかしい
		if(itrRes->second->IsChkPfInfo() == TRUE){
			continue;
		}
		//未定で終わりかけのやつ除外
		if( data.reserveStatus == ADD_RESERVE_UNKNOWN_END ){
			continue;
		}

		if( data.originalNetworkID == info->original_network_id &&
			data.transportStreamID == info->transport_stream_id &&
			data.serviceID == info->service_id &&
			data.eventID != info->event_id ){
				if( ConvertI64Time(data.startTime) != GetSumTime(info->start_time, info->durationSec) ){
					//開始時間が違うので変更してやる
					GetSumTime(info->start_time, info->durationSec, &data.startTime);
					if( data.reserveStatus == ADD_RESERVE_NORMAL ){
						data.reserveStatus = ADD_RESERVE_CHG_PF2;
					}

					_ChgReserveData( &data, TRUE );
					ret = TRUE;
				}
		}
	}
	return ret;
}

void CReserveManager::EnableSuspendWork(BYTE suspendMode, BYTE rebootFlag, BYTE epgReload)
{
	BYTE setSuspendMode = suspendMode;
	BYTE setRebootFlag = rebootFlag;

	if( suspendMode == 0 ){
		setSuspendMode = this->defSuspendMode;
		setRebootFlag = this->defRebootFlag;
	}

	if( _IsSuspendOK(setRebootFlag) == TRUE ){
		this->enableSetSuspendMode = setSuspendMode;
		this->enableSetRebootFlag = setRebootFlag;
	}else{
		this->enableSetSuspendMode = 0xFF;
		this->enableSetRebootFlag = 0xFF;
		this->enableEpgReload = epgReload;
	}
}

BOOL CReserveManager::IsEnableSuspend(
	BYTE* suspendMode,
	BYTE* rebootFlag
	)
{
	if( Lock(L"IsEnableSuspend") == FALSE ) return FALSE;
	BOOL ret = TRUE;

	if( this->enableSetSuspendMode == 0xFF && this->enableSetRebootFlag == 0xFF ){
		ret = FALSE;
	}else{
		if( this->useTweet != TRUE || this->twitterManager->GetTweetQue() == 0){
			*suspendMode = this->enableSetSuspendMode;
			*rebootFlag = this->enableSetRebootFlag;

			this->enableSetSuspendMode = 0xFF;
			this->enableSetRebootFlag = 0xFF;
		}else{
			ret = FALSE;
		}
	}

	UnLock();
	return ret;
}

BOOL CReserveManager::IsEnableReloadEPG(
	)
{
	if( Lock(L"IsEnableReloadEPG") == FALSE ) return FALSE;
	BOOL ret = FALSE;
	if( this->enableEpgReload == 1 ){
		ret = TRUE;
		this->enableEpgReload = 0;
	}
	UnLock();
	return ret;
}

BOOL CReserveManager::IsSuspendOK()
{
	if( Lock() == FALSE ) return FALSE;
	BOOL ret = _IsSuspendOK(FALSE);
	UnLock();
	return ret;
}

BOOL CReserveManager::_IsSuspendOK(BOOL rebootFlag)
{
	BOOL ret = TRUE;

	map<DWORD, CTunerBankCtrl*>::iterator itrCtrl;
	for( itrCtrl = this->tunerBankMap.begin(); itrCtrl != this->tunerBankMap.end(); itrCtrl++ ){
		if( itrCtrl->second->IsSuspendOK() == FALSE ){
			//起動中なので予約処理中
			OutputDebugString(L"_IsSuspendOK tunerUse");
			return FALSE;
		}
	}

	LONGLONG wakeMargin = this->wakeTime;
	if( rebootFlag == TRUE ){
		wakeMargin += 5;
	}
	LONGLONG chkNoStandbyTime = GetNowI64Time() + ((LONGLONG)this->noStandbyTime)*60*I64_1SEC;

	LONGLONG chkWakeTime = GetNowI64Time() + wakeMargin*60*I64_1SEC;
	/*
	multimap<wstring, RESERVE_DATA*>::iterator itr;
	for( itr = this->reserveText.reserveMap.begin(); itr != this->reserveText.reserveMap.end(); itr++ ){
		if( itr->second->recSetting.recMode != RECMODE_VIEW && itr->second->recSetting.recMode != RECMODE_NO ){
			LONGLONG startTime = ConvertI64Time(itr->second->startTime);
			DWORD sec = itr->second->durationSecond;
			if( sec < (DWORD)wakeMargin*60 ){
				sec = (DWORD)wakeMargin*60;
			}
			LONGLONG endTime = GetSumTime(itr->second->startTime, sec);
			if( itr->second->recSetting.useMargineFlag == 1 ){
				startTime += ((LONGLONG)itr->second->recSetting.startMargine)*I64_1SEC;
				endTime += ((LONGLONG)itr->second->recSetting.endMargine)*I64_1SEC;
			}else{
				startTime += ((LONGLONG)this->defStartMargine)*I64_1SEC;
				endTime += ((LONGLONG)this->defEndMargine)*I64_1SEC;
			}

			if( startTime <= chkWakeTime && chkWakeTime < endTime ){
				OutputDebugString(L"_IsSuspendOK chkWakeTime");
				//次の予約時間にかぶる
				return FALSE;
			}
			break;
		}
	}
	*/
	//ソートされてないので全部回す必要あり
	map<DWORD, CReserveInfo*>::iterator itr;
	for( itr = this->reserveInfoMap.begin(); itr != this->reserveInfoMap.end(); itr++ ){
		RESERVE_DATA data;
		itr->second->GetData(&data);
		if( data.overlapMode == 0 ){
			if( data.recSetting.recMode != RECMODE_NO ){
				LONGLONG startTime;
				LONGLONG endTime;
				CalcEntireReserveTime(&startTime, &endTime, data);
				//短い予約のチェック漏れを防ぐ
				if( endTime < startTime + wakeMargin*60*I64_1SEC ){
					endTime = startTime + wakeMargin*60*I64_1SEC;
				}

				if( startTime <= chkWakeTime && chkWakeTime < endTime ){
					OutputDebugString(L"_IsSuspendOK chkWakeTime");
					//次の予約時間にかぶる
					return FALSE;
				}
				if( startTime <= chkNoStandbyTime ){
					OutputDebugString(L"_IsSuspendOK chkNoStandbyTime");
					//次の予約時間にかぶる
					return FALSE;
				}

			}
		}
	}

	LONGLONG epgcapTime = 0;
	if( GetNextEpgcapTime(&epgcapTime, 0) == TRUE ){
		if( epgcapTime < chkWakeTime ){
			//EPG取得
			OutputDebugString(L"_IsSuspendOK EpgCapTime");
			return FALSE;
		}
	}

	if( this->batManager.IsWorking() == TRUE ){
		//バッチ処理中
		OutputDebugString(L"_IsSuspendOK IsBatWorking");
		return FALSE;
	}

	if( IsFindNoSuspendExe() == TRUE ){
		//Exe起動中
		OutputDebugString(L"IsFindNoSuspendExe");
		return FALSE;
	}

	if( IsFindShareTSFile() == TRUE ){
		//TSファイルアクセス中
		OutputDebugString(L"IsFindShareTSFile");
		return FALSE;
	}

	return ret;
}

BOOL CReserveManager::IsFindNoSuspendExe()
{
	OSVERSIONINFO VerInfo;
	VerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx( &VerInfo );

	BOOL b2000 = FALSE;
	if( VerInfo.dwMajorVersion == 5 && VerInfo.dwMinorVersion == 0 ){
		b2000 = TRUE;
	}else{
		b2000 = FALSE;
	}

	HANDLE hSnapshot;
	PROCESSENTRY32 procent;

	BOOL bFind = FALSE;
	/* Toolhelpスナップショットを作成する */
	hSnapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS,0 );
	if ( hSnapshot != (HANDLE)-1 ) {
		procent.dwSize = sizeof(PROCESSENTRY32);
		if ( Process32First( hSnapshot,&procent ) != FALSE )            {
			do {
				/* procent.szExeFileにプロセス名 */
				wstring strExe = procent.szExeFile;
				std::transform(strExe.begin(), strExe.end(), strExe.begin(), tolower);
				for( size_t i=0; i<this->noStandbyExeList.size(); i++ ){
					if( b2000 == TRUE ){
						if( strExe.find( this->noStandbyExeList[i].substr(0, 15).c_str()) == 0 ){
							_OutputDebugString(L"起動exe:%s", strExe.c_str());
							bFind = TRUE;
							break;
						}
					}else{
						if( strExe.find( this->noStandbyExeList[i].c_str()) == 0 ){
							_OutputDebugString(L"起動exe:%s", strExe.c_str());
							bFind = TRUE;
							break;
						}
					}
				}
				if( bFind == TRUE ){
					break;
				}
			} while ( Process32Next( hSnapshot,&procent ) != FALSE );
		}
		CloseHandle( hSnapshot );
	}
	return bFind;
}

BOOL CReserveManager::IsFindShareTSFile()
{
	BOOL ret = FALSE;
	if( this->ngShareFile == TRUE ){
		LPBYTE bufptr = NULL;
		DWORD entriesread = 0;
		DWORD totalentries = 0;
		NetFileEnum(NULL, NULL, NULL, 3, &bufptr, MAX_PREFERRED_LENGTH, &entriesread, &totalentries, NULL);
		if( entriesread > 0 ){
			_OutputDebugString(L"共有フォルダアクセス");
			for(DWORD i=0; i<entriesread; i++){
				FILE_INFO_3* info = (FILE_INFO_3*)(bufptr+(sizeof(FILE_INFO_3)*i));

				wstring filePath = info->fi3_pathname;
				OutputDebugString(filePath.c_str());
				if( IsExt(filePath, L".ts") == TRUE ){
					ret = TRUE;
				}
			}
		}
		NetApiBufferFree(bufptr);
	}
	return ret;
}

BOOL CReserveManager::GetSleepReturnTime(
	LONGLONG* returnTime
	)
{
	if( returnTime == NULL || Lock(L"GetSleepReturnTime") == FALSE ){
		return FALSE;
	}
	//最も近い予約開始時刻を得る
	LONGLONG nextRec = 0;
	map<DWORD, RESERVE_DATA>::const_iterator itr;
	for( itr = this->reserveText.GetMap().begin(); itr != this->reserveText.GetMap().end(); itr++ ){
		if( itr->second.recSetting.recMode != RECMODE_NO ){
			LONGLONG startTime = ConvertI64Time(itr->second.startTime);
			LONGLONG endTime = startTime + itr->second.durationSecond * I64_1SEC;
			LONGLONG startMargin = this->defStartMargine * I64_1SEC;
			if( itr->second.recSetting.useMargineFlag == TRUE ){
				startMargin = itr->second.recSetting.startMargine * I64_1SEC;
			}
			//開始マージンは元の予約終了時刻を超えて負であってはならない
			startTime -= max(startMargin, startTime - endTime);
			if( nextRec == 0 || nextRec > startTime ){
				nextRec = startTime;
			}
		}
	}

	LONGLONG epgcapTime = 0;
	GetNextEpgcapTime(&epgcapTime, 0);

	UnLock();

	if( nextRec == 0 && epgcapTime == 0 ){
		return FALSE;
	}else if( nextRec == 0 && epgcapTime != 0 ){
		*returnTime = epgcapTime;
	}else if( nextRec != 0 && epgcapTime == 0 ){
		*returnTime = nextRec;
	}else{
		if(nextRec < epgcapTime){
			*returnTime = nextRec;
		}else{
			*returnTime = epgcapTime;
		}
	}
	return TRUE;
}

BOOL CReserveManager::GetNextEpgcapTime(LONGLONG* capTime, LONGLONG chkMargineMin, int* basicOnlyFlags)
{
	if( capTime == NULL ){
		return FALSE;
	}

	wstring iniPath = L"";
	GetModuleIniPath(iniPath);

	SYSTEMTIME srcTime;
	GetLocalTime(&srcTime);
	srcTime.wHour = 0;
	srcTime.wMinute = 0;
	srcTime.wSecond = 0;
	srcTime.wHour = 0;
	srcTime.wMilliseconds = 0;

	map<LONGLONG,EPGTIME_INFO*> timeList;

	vector<EPGTIME_INFO>::iterator itr;
	for( itr = this->epgCapTimeList.begin(); itr != this->epgCapTimeList.end(); itr++ ){
		if( itr->wday <= 0 || itr->wday % 7 == srcTime.wDayOfWeek ){
			//曜日未指定、または曜日指定が今日と一致
			LONGLONG chkTime = GetSumTime(srcTime, itr->time);
			timeList.insert(pair<LONGLONG,EPGTIME_INFO*>(chkTime, &(*itr)));
		}
		if( itr->wday <= 0 || itr->wday % 7 == (srcTime.wDayOfWeek + 1) % 7 ){
			//曜日未指定、または曜日指定が次の日と一致
			LONGLONG chkTime = GetSumTime(srcTime, itr->time + 24*60*60);
			timeList.insert(pair<LONGLONG,EPGTIME_INFO*>(chkTime, &(*itr)));
		}
	}

	//そのまま判定したら直前で次の日になってしまうのでマージン分現在の時刻を調整
	LONGLONG nowTime = GetNowI64Time() + (chkMargineMin*60*I64_1SEC);
	map<LONGLONG,EPGTIME_INFO*>::iterator itr2 = timeList.upper_bound(nowTime);
	if( itr2 == timeList.end() ){
		return FALSE;
	}
	*capTime = itr2->first;
	if( basicOnlyFlags != NULL ){
		*basicOnlyFlags = itr2->second->basicOnlyFlags;
	}

	return TRUE;
}

//録画済み情報一覧を取得する
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// infoList			[OUT]録画済み情報一覧
BOOL CReserveManager::GetRecFileInfoAll(
	vector<REC_FILE_INFO>* infoList
	)
{
	if( Lock(L"GetRecFileInfoAll") == FALSE ) return FALSE;
	BOOL ret = TRUE;

	map<DWORD, REC_FILE_INFO>::const_iterator itr;
	for( itr = this->recInfoText.GetMap().begin(); itr != this->recInfoText.GetMap().end(); itr++ ){
		infoList->push_back(itr->second);
	}

	UnLock();
	return ret;
}

//録画済み情報を削除する
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// idList			[IN]IDリスト
BOOL CReserveManager::DelRecFileInfo(
	vector<DWORD>* idList
	)
{
	if( Lock(L"DelRecFileInfo") == FALSE ) return FALSE;
	BOOL ret = TRUE;

	for( size_t i=0 ;i<idList->size(); i++){
		this->recInfoText.DelRecInfo((*idList)[i]);
	}

	this->recInfoText.SaveText();
	this->chgRecInfo = TRUE;

	_SendNotifyUpdate(NOTIFY_UPDATE_REC_INFO);

	UnLock();
	return ret;
}

//録画済み情報のプロテクトを変更する
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// idList			[IN]IDリスト
BOOL CReserveManager::ChgProtectRecFileInfo(
	vector<REC_FILE_INFO>* infoList
	)
{
	if( Lock(L"DelRecFileInfo") == FALSE ) return FALSE;
	BOOL ret = TRUE;

	for( size_t i=0 ;i<infoList->size(); i++){
		this->recInfoText.ChgProtectRecInfo((*infoList)[i].id, (*infoList)[i].protectFlag);
	}

	this->recInfoText.SaveText();
	this->chgRecInfo = TRUE;

	_SendNotifyUpdate(NOTIFY_UPDATE_REC_INFO);

	UnLock();
	return ret;
}

BOOL CReserveManager::StartEpgCap()
{
	if( Lock(L"StartEpgCap") == FALSE ) return FALSE;

	BOOL ret = _StartEpgCap();

	UnLock();
	return ret;
}

BOOL CReserveManager::_StartEpgCap()
{
	BOOL ret = TRUE;

	wstring chSet5Path = L"";
	GetSettingPath(chSet5Path);
	chSet5Path += L"\\ChSet5.txt";

	CParseChText5 chSet5;
	chSet5.ParseText(chSet5Path.c_str());

	//まず取得対象となるサービスの一覧を抽出
	map<DWORD, CH_DATA5> serviceList;
	map<LONGLONG, CH_DATA5>::const_iterator itrCh;
	for( itrCh = chSet5.GetMap().begin(); itrCh != chSet5.GetMap().end(); itrCh++ ){
		if( itrCh->second.epgCapFlag == TRUE ){
			DWORD key = ((DWORD)itrCh->second.originalNetworkID)<<16 | itrCh->second.transportStreamID;
			map<DWORD, CH_DATA5>::iterator itrIn;
			itrIn = serviceList.find(key);
			if( itrIn == serviceList.end() ){
				serviceList.insert(pair<DWORD, CH_DATA5>(key, itrCh->second));
			}
		}
	}

	//利用可能なチューナーの抽出
	vector<pair<vector<DWORD>, WORD>> tunerIDList;
	this->tunerManager.GetEnumEpgCapTuner(&tunerIDList);

	map<DWORD, CTunerBankCtrl*> epgCapCtrl;
	map<DWORD, CTunerBankCtrl*>::iterator itrCtrl;
	for( size_t i=0; i<tunerIDList.size(); i++ ){
		WORD epgCapMax = tunerIDList[i].second;
		WORD ngCapCount = 0;
		for( size_t j=0; j<tunerIDList[i].first.size() && epgCapMax > 0; j++ ){
			itrCtrl = this->tunerBankMap.find(tunerIDList[i].first[j]);
			if( itrCtrl != this->tunerBankMap.end() && itrCtrl->second->IsEpgCapWorking() == FALSE ){
				itrCtrl->second->ClearEpgCapItem();
				if( ngCapMin != 0 ){
					if( itrCtrl->second->IsEpgCapOK(this->ngCapMin) == FALSE ){
						//実行しちゃいけない
						ngCapCount++;
						continue;
					}
				}
				if( itrCtrl->second->IsEpgCapOK(this->ngCapTunerMin) == TRUE ){
					//使えるチューナー
					epgCapCtrl.insert(pair<DWORD, CTunerBankCtrl*>(itrCtrl->first, itrCtrl->second));
					epgCapMax--;
				}
			}
		}
		if( tunerIDList[i].second > tunerIDList[i].first.size() - ngCapCount ){
			return FALSE;
		}
	}
	if( epgCapCtrl.size() == 0 ){
		//実行できるものない
		return FALSE;
	}

	BOOL inBS = FALSE;
	BOOL inCS1 = FALSE;
	BOOL inCS2 = FALSE;
	//各チューナーに振り分け
	itrCtrl = epgCapCtrl.begin();
	map<DWORD, CH_DATA5>::iterator itrAdd;
	for( itrAdd = serviceList.begin(); itrAdd != serviceList.end(); itrAdd++ ){
		if( itrAdd->second.originalNetworkID == 4 && this->BSOnly == TRUE ){
			if( inBS == TRUE ){
				continue;
			}
		}
		if( itrAdd->second.originalNetworkID == 6 && this->CS1Only == TRUE ){
			if( inCS1 == TRUE ){
				continue;
			}
		}
		if( itrAdd->second.originalNetworkID == 7 && this->CS2Only == TRUE ){
			if( inCS2 == TRUE ){
				continue;
			}
		}
		DWORD startID = itrCtrl->first;
		BOOL add = FALSE;
		do{
			if( this->tunerManager.IsSupportService(
				itrCtrl->first,
				itrAdd->second.originalNetworkID,
				itrAdd->second.transportStreamID,
				itrAdd->second.serviceID
				) == TRUE ){
					SET_CH_INFO addItem;
					addItem.ONID = itrAdd->second.originalNetworkID;
					addItem.TSID = itrAdd->second.transportStreamID;
					addItem.SID = itrAdd->second.serviceID;
					addItem.useSID = TRUE;
					addItem.useBonCh = FALSE;
					itrCtrl->second->AddEpgCapItem(addItem);

					add = TRUE;

					if(itrAdd->second.originalNetworkID == 4){
						inBS = TRUE;
					}
					if(itrAdd->second.originalNetworkID == 6){
						inCS1 = TRUE;
					}
					if(itrAdd->second.originalNetworkID == 7){
						inCS2 = TRUE;
					}
			}

			itrCtrl++;
			if( itrCtrl == epgCapCtrl.end() ){
				itrCtrl = epgCapCtrl.begin();
			}
		}while(startID != itrCtrl->first && add == FALSE);
	}

	for( itrCtrl = epgCapCtrl.begin(); itrCtrl != epgCapCtrl.end(); itrCtrl++ ){
		itrCtrl->second->StartEpgCap();
	}
	this->epgCapCheckFlag = TRUE;

	_SendNotifyStatus(2);
	_SendNotifyUpdate(NOTIFY_UPDATE_EPGCAP_START);
	this->setTimeSync = FALSE;

	return ret;
}

void CReserveManager::StopEpgCap()
{
	if( Lock(L"StopEpgCap") == FALSE ) return ;

	map<DWORD, CTunerBankCtrl*>::iterator itr;
	for( itr = this->tunerBankMap.begin(); itr != this->tunerBankMap.end(); itr++ ){
		itr->second->StopEpgCap();
	}
		 
	UnLock();
}

BOOL CReserveManager::IsEpgCap()
{
	if( Lock(L"StopEpgCap") == FALSE ) return FALSE;

	BOOL ret = _IsEpgCap();
		 
	UnLock();
	return ret;
}

BOOL CReserveManager::_IsEpgCap()
{
	BOOL ret = FALSE;
	map<DWORD, CTunerBankCtrl*>::iterator itr;
	for( itr = this->tunerBankMap.begin(); itr != this->tunerBankMap.end(); itr++ ){
		if(itr->second->IsEpgCapWorking() == TRUE){
			ret = TRUE;
			if( this->timeSync == TRUE && this->setTimeSync == FALSE){
				LONGLONG delay = itr->second->DelayTime();
				if( abs(delay) > 10*I64_1SEC ){
					LONGLONG local = GetNowI64Time();
					local += delay;
					SYSTEMTIME setTime;
					ConvertSystemTime(local, &setTime);
					if( SetLocalTime(&setTime) == FALSE ){
						_OutputDebugString(L"★SetLocalTime err %I64d", delay/I64_1SEC);
					}else{
						_OutputDebugString(L"★SetLocalTime %I64d",delay/I64_1SEC);
						this->setTimeSync = TRUE;
					}
				}
			}
			break;
		}
	}
		 
	return ret;
}

BOOL CReserveManager::IsFindReserve(
	WORD ONID,
	WORD TSID,
	WORD SID,
	WORD eventID
	)
{
	if( Lock(L"IsFindReserve") == FALSE ) return FALSE;

	BOOL ret = FALSE;
/*
	map<DWORD, CReserveInfo*>::iterator itr;
	for(itr = this->reserveInfoMap.begin(); itr != this->reserveInfoMap.end(); itr++ ){
		RESERVE_DATA info;
		itr->second->GetData(&info);
		if( info.originalNetworkID == ONID &&
			info.transportStreamID == TSID &&
			info.serviceID == SID &&
			info.eventID == eventID ){
				ret = TRUE;
				break;
		}
	}
*/
	map<LONGLONG, DWORD>::iterator itr;
	LONGLONG keyID = _Create64Key2(ONID, TSID, SID, eventID);
	itr = this->reserveInfoIDMap.find(keyID);
	if( itr != this->reserveInfoIDMap.end()){
		ret = TRUE;
	}

	UnLock();

	return ret;
}

BOOL CReserveManager::IsFindReserve(
	WORD ONID,
	WORD TSID,
	WORD SID,
	LONGLONG startTime,
	DWORD durationSec
	)
{
	if( Lock(L"IsFindReserve") == FALSE ) return FALSE;

	BOOL ret = FALSE;

	map<DWORD, CReserveInfo*>::iterator itr;
	for(itr = this->reserveInfoMap.begin(); itr != this->reserveInfoMap.end(); itr++ ){
		RESERVE_DATA info;
		itr->second->GetData(&info);
		if( info.originalNetworkID == ONID &&
			info.transportStreamID == TSID &&
			info.serviceID == SID &&
			ConvertI64Time(info.startTime) == startTime &&
			info.durationSecond == durationSec ){
				ret = TRUE;
				break;
		}
	}

	UnLock();

	return ret;
}

BOOL CReserveManager::GetTVTestChgCh(
	LONGLONG chID,
	TVTEST_CH_CHG_INFO* chInfo
	)
{
	if( Lock(L"GetTVTestChgCh") == FALSE ) return FALSE;

	BOOL ret = FALSE;
	
	WORD ONID = (WORD)((chID&0x0000FFFF00000000)>>32);
	WORD TSID = (WORD)((chID&0x00000000FFFF0000)>>16);
	WORD SID = (WORD)((chID&0x000000000000FFFF));

	vector<DWORD> idList;
	this->tunerManager.GetSupportServiceTuner(ONID, TSID, SID, &idList);

	for( int i=(int)idList.size() -1; i>=0; i-- ){
		wstring bonDriver = L"";
		this->tunerManager.GetBonFileName(idList[i], bonDriver);
		BOOL find = FALSE;
		for( size_t j=0; j<this->tvtestUseBon.size(); j++ ){
			if( CompareNoCase(this->tvtestUseBon[j], bonDriver) == 0 ){
				find = TRUE;
				break;
			}
		}
		if( find == TRUE ){
			map<DWORD, CTunerBankCtrl*>::iterator itr;
			itr = this->tunerBankMap.find(idList[i]);
			if( itr != this->tunerBankMap.end() ){
				if( itr->second->IsOpenTuner() == FALSE ){
					chInfo->bonDriver = bonDriver;
					chInfo->chInfo.useSID = TRUE;
					chInfo->chInfo.ONID = ONID;
					chInfo->chInfo.TSID = TSID;
					chInfo->chInfo.SID = SID;
					chInfo->chInfo.useBonCh = TRUE;
					this->tunerManager.GetCh(idList[i], ONID, TSID, SID, &chInfo->chInfo.space, &chInfo->chInfo.ch);
					ret = TRUE;
					break;
				}
			}
		}
	}

	UnLock();

	return ret;
}

BOOL CReserveManager::SetNWTVCh(
	SET_CH_INFO* chInfo
	)
{
	if( Lock(L"SetNWTVCh") == FALSE ) return FALSE;

	BOOL ret = FALSE;

	BOOL findCh = FALSE;
	SET_CH_INFO initCh;
	wstring bonDriver = L"";

	vector<DWORD> idList;
	this->tunerManager.GetSupportServiceTuner(chInfo->ONID, chInfo->TSID, chInfo->SID, &idList);

	for( int i=(int)idList.size() -1; i>=0; i-- ){
		this->tunerManager.GetBonFileName(idList[i], bonDriver);
		BOOL find = FALSE;
		for( size_t j=0; j<this->tvtestUseBon.size(); j++ ){
			if( CompareNoCase(this->tvtestUseBon[j], bonDriver) == 0 ){
				find = TRUE;
				break;
			}
		}
		if( find == TRUE ){
			map<DWORD, CTunerBankCtrl*>::iterator itr;
			itr = this->tunerBankMap.find(idList[i]);
			if( itr != this->tunerBankMap.end() ){
				if( itr->second->IsOpenTuner() == FALSE ){
					initCh.useSID = TRUE;
					initCh.ONID = chInfo->ONID;
					initCh.TSID = chInfo->TSID;
					initCh.SID = chInfo->SID;

					initCh.useBonCh = TRUE;
					this->tunerManager.GetCh(idList[i], chInfo->ONID, chInfo->TSID, chInfo->SID, &initCh.space, &initCh.ch);
					findCh = TRUE;
					break;
				}
			}
		}
	}

	if( findCh == FALSE ){
		UnLock();
		return FALSE;
	}

	BOOL findPID = FALSE;
	vector<DWORD> pidList = _FindPidListByExeName(L"EpgDataCap_Bon.exe");
	for( size_t i=0; i<pidList.size(); i++ ){
		if( pidList[i] == this->NWTVPID ){
			findPID = TRUE;
		}
	}
	if( findPID == FALSE ){
		this->NWTVPID = 0;
	}

	if( this->NWTVPID != 0 ){
		int id = -1;
		this->sendCtrlNWTV.SendViewGetID(&id);
		if( this->sendCtrlNWTV.SendViewGetID(&id) == CMD_SUCCESS ){
			if( id == -1 ){
				DWORD status = 0;
				if(this->sendCtrlNWTV.SendViewGetStatus(&status) == CMD_SUCCESS ){
					if( status == VIEW_APP_ST_NORMAL ){
						this->sendCtrlNWTV.SendViewSetBonDrivere(bonDriver);
						this->sendCtrlNWTV.SendViewSetCh(&initCh);
						ret = TRUE;
					}else{
						//録画とかしてそう
						this->NWTVPID = 0;
					}
				}else{
					//終了された？
					this->NWTVPID = 0;
				}
			}else{
				//録画用に奪われた？
				this->NWTVPID = 0;
			}
		}else{
			//終了された？
			this->NWTVPID = 0;
		}
	}
	if( this->NWTVPID == 0 ){

		DWORD PID = 0;
		BOOL noNW = FALSE;
		if( this->NWTVUDP == FALSE && this->NWTVTCP == FALSE ){
			noNW = TRUE;
		}
		map<DWORD, DWORD> registGUIMap;
		if( this->notifyManager != NULL ){
			this->notifyManager->GetRegistGUI(&registGUIMap);
		}
		if( CTunerBankCtrl::OpenTunerExe(this->recExePath.c_str(), bonDriver.c_str(), -1, TRUE, TRUE, noNW, this->NWTVUDP, this->NWTVTCP, 3, registGUIMap, &PID) == TRUE ){
			this->NWTVPID = PID;
			ret = TRUE;

			this->sendCtrlNWTV.SetPipeSetting(CMD2_VIEW_CTRL_WAIT_CONNECT, CMD2_VIEW_CTRL_PIPE, this->NWTVPID);
			this->sendCtrlNWTV.SendViewSetCh(&initCh);
		}
	}
	if( ret == FALSE ){
		this->NWTVPID = 0;
	}

	UnLock();

	return ret;
}

BOOL CReserveManager::CloseNWTV(
	)
{
	if( Lock(L"CloseNWTV") == FALSE ) return FALSE;

	BOOL ret = FALSE;

	if( this->NWTVPID != 0 ){
		int id = -1;
		if( this->sendCtrlNWTV.SendViewGetID(&id) == CMD_SUCCESS ){
			if( id == -1 ){
				this->sendCtrlNWTV.SendViewAppClose();
				ret = TRUE;
			}
		}
	}
	this->NWTVPID = 0;

	UnLock();

	return ret;
}

void CReserveManager::SetNWTVMode(
	DWORD mode
	)
{
	if( Lock(L"SetNWTVMode") == FALSE ) return ;

	if( mode == 1 ){
		this->NWTVUDP = TRUE;
		this->NWTVTCP = FALSE;
	}else if( mode == 2 ){
		this->NWTVUDP = FALSE;
		this->NWTVTCP = TRUE;
	}else if( mode == 3 ){
		this->NWTVUDP = TRUE;
		this->NWTVTCP = TRUE;
	}else{
		this->NWTVUDP = FALSE;
		this->NWTVTCP = FALSE;
	}
	UnLock();
}

void CReserveManager::SendTweet(
		SEND_TWEET_MODE mode,
		void* param1,
		void* param2,
		void* param3
	)
{
	if( Lock(L"SendTweet") == FALSE ) return ;

	_SendTweet(mode, param1, param2, param3);

	UnLock();
}

void CReserveManager::_SendTweet(
		SEND_TWEET_MODE mode,
		void* param1,
		void* param2,
		void* param3
	)
{
	if( this->useTweet == TRUE ){
		this->twitterManager->SendTweet(mode, param1, param2, param3);
	}
}

BOOL CReserveManager::GetRecFilePath(
	DWORD reserveID,
	wstring& filePath,
	DWORD* ctrlID,
	DWORD* processID
	)
{
	if( Lock(L"GetRecFilePath") == FALSE ) return FALSE;

	BOOL ret = FALSE;

	map<DWORD, CTunerBankCtrl*>::iterator itrBank;
	for( itrBank = this->tunerBankMap.begin(); itrBank != this->tunerBankMap.end(); itrBank++ ){
		if( itrBank->second->GetRecFilePath(reserveID, filePath, ctrlID, processID) == TRUE ){
			ret = TRUE;
			break;
		}
	}
	UnLock();
	return ret;
}

BOOL CReserveManager::ChkAddReserve(RESERVE_DATA* chkData, WORD* chkStatus)
{
	if( Lock(L"ChkAddReserve") == FALSE ) return FALSE;

	if( chkData == NULL || chkStatus == NULL ){
		UnLock();
		return FALSE;
	}
	if( ngAddResSrvCoop == TRUE ){
		UnLock();
		return FALSE;
	}
	BOOL ret = TRUE;

	//同じ物あるかチェック
	map<DWORD, CReserveInfo*>::iterator itrRes;
	for( itrRes = this->reserveInfoMap.begin(); itrRes != this->reserveInfoMap.end(); itrRes++ ){
		RESERVE_DATA data;
		itrRes->second->GetData(&data);
		if( data.originalNetworkID == chkData->originalNetworkID &&
			data.transportStreamID == chkData->transportStreamID &&
			data.serviceID == chkData->serviceID ){
			if( chkData->eventID == 0xFFFF ){
				__int64 chk1 = ConvertI64Time(data.startTime);
				__int64 chk2 = ConvertI64Time(chkData->startTime);
				if( chk1 == chk2 && data.durationSecond == chkData->durationSecond ){
					if( data.recSetting.recMode == 5 ){
						//無効のものあるので不可
						*chkStatus = 0;
					}else{
						//同じ物あり
						if(data.overlapMode == 0 ){
							*chkStatus = 3;
						}else{
							*chkStatus = 0;
						}
					}
					UnLock();
					return TRUE;
				}
			}else{
				if( chkData->eventID == data.eventID ){
					if( data.recSetting.recMode == 5 ){
						//無効のものあるので不可
						*chkStatus = 0;
					}else{
						//同じ物あり
						if(data.overlapMode == 0 ){
							*chkStatus = 3;
						}else{
							*chkStatus = 0;
						}
					}
					UnLock();
					return TRUE;
				}
			}
		}
	}


	CReserveInfo chkItem;
	chkItem.SetData(chkData);

	if( chkData->recSetting.tunerID == 0 ){
		//サービスサポートしてないチューナー検索
		vector<DWORD> idList;
		if( this->tunerManager.GetNotSupportServiceTuner(
			chkData->originalNetworkID,
			chkData->transportStreamID,
			chkData->serviceID,
			&idList ) == TRUE ){
				chkItem.SetNGChTunerID(&idList);
		}
	}else{
		//チューナー固定
		vector<DWORD> idList;
		if( this->tunerManager.GetEnumID( &idList ) == TRUE ){
			for( size_t i=0; i<idList.size(); i++ ){
				if( idList[i] != chkData->recSetting.tunerID ){
					chkItem.AddNGTunerID(idList[i]);
				}
			}
		}
	}

	BANK_WORK_INFO item;
	if( this->reloadBankMapAlgo == 3 ){
		CreateWorkData(&chkItem, &item, FALSE, 0, 0);
	}else{
		CreateWorkData(&chkItem, &item, this->backPriorityFlag, 0, 0);
	}


	BOOL insert = FALSE;
	map<DWORD, BANK_INFO*>::iterator itrBank;
	//チューナー優先度より同一物理チャンネルで連続となるチューナーの使用を優先する
	if( this->sameChPriorityFlag == TRUE ){
		for( itrBank = this->bankMap.begin(); itrBank != this->bankMap.end(); itrBank++){
			DWORD status = ChkInsertSameChStatus(itrBank->second, &item);
			if( status == 1 ){
				//問題なく追加可能
				insert = TRUE;
				*chkStatus = 1;
				break;
			}
		}
	}
	if( insert == FALSE ){
		for( itrBank = this->bankMap.begin(); itrBank != this->bankMap.end(); itrBank++){
			DWORD status = ChkInsertStatus(itrBank->second, &item);
			if( status == 1 ){
				//問題なく追加可能
				insert = TRUE;
				*chkStatus = 1;
				break;
			}else if( status == 2 ){
				//追加可能だが終了時間と開始時間の重なった予約あり
				//仮追加
				*chkStatus = 2;
				insert = TRUE;
				break;
			}
		}
	}

	UnLock();
	return ret;
}

BOOL CReserveManager::IsFindRecEventInfo(EPGDB_EVENT_INFO* info, WORD chkDay)
{
	if( Lock(L"IsFindRecEventInfo") == FALSE ) return FALSE;
	BOOL ret = FALSE;

	CoInitialize(NULL);
	{
		IRegExpPtr regExp;
		regExp.CreateInstance(CLSID_RegExp);
		if( regExp != NULL && info->shortInfo != NULL ){
			wstring infoEventName = info->shortInfo->event_name;
			if( this->recInfo2RegExp.empty() == false ){
				regExp->PutGlobal(VARIANT_TRUE);
				regExp->PutPattern(_bstr_t(this->recInfo2RegExp.c_str()));
				_bstr_t rpl = regExp->Replace(_bstr_t(infoEventName.c_str()), _bstr_t());
				infoEventName = (LPCWSTR)rpl == NULL ? L"" : (LPCWSTR)rpl;
			}
			if( infoEventName.empty() == false && info->StartTimeFlag != 0 ){
				map<DWORD, PARSE_REC_INFO2_ITEM>::const_iterator itr;
				for( itr = this->recInfo2Text.GetMap().begin(); itr != this->recInfo2Text.GetMap().end(); itr++ ){
					if( itr->second.originalNetworkID == info->original_network_id &&
					    itr->second.transportStreamID == info->transport_stream_id &&
					    itr->second.serviceID == info->service_id &&
					    ConvertI64Time(itr->second.startTime) + chkDay*24*60*60*I64_1SEC > ConvertI64Time(info->start_time) ){
						wstring eventName = itr->second.eventName;
						if( this->recInfo2RegExp.empty() == false ){
							_bstr_t rpl = regExp->Replace(_bstr_t(eventName.c_str()), _bstr_t());
							eventName = (LPCWSTR)rpl == NULL ? L"" : (LPCWSTR)rpl;
						}
						if( infoEventName == eventName ){
							ret = TRUE;
							break;
						}
					}
				}
			}
		}
	}
	CoUninitialize();

	UnLock();
	return ret;
}

void CReserveManager::ChgAutoAddNoRec(EPGDB_EVENT_INFO* info)
{
	if( Lock(L"ChgAutoAddNoRec") == FALSE ) return ;

	map<DWORD, CReserveInfo*>::iterator itr;
	for( itr = this->reserveInfoMap.begin(); itr != this->reserveInfoMap.end(); itr++ ){
		RESERVE_DATA item;
		itr->second->GetData(&item);
		if( item.originalNetworkID == info->original_network_id &&
			item.transportStreamID == info->transport_stream_id &&
			item.serviceID == info->service_id &&
			item.eventID == info->event_id
			){
				if( item.comment.find(L"EPG自動予約") != string::npos ){
					item.recSetting.recMode = RECMODE_NO;
					_ChgReserveData(&item, FALSE);
				}
		}
	}

	UnLock();
	return ;
}

BOOL CReserveManager::IsRecInfoChg()
{
	if( Lock(L"IsFindRecEventInfo") == FALSE ) return FALSE;
	BOOL ret = TRUE;

	ret = this->chgRecInfo;

	this->chgRecInfo = FALSE;

	UnLock();
	return ret;
}

void CReserveManager::CreateDiskFreeSpace(
	const vector<RESERVE_DATA>& chkReserve,
	const wstring& defRecFolder,
	const map<wstring, wstring>& protectFile,
	const vector<wstring>& delFolderList,
	const vector<wstring>& delExtList
	)
{
	//ファイル削除可能なフォルダをドライブごとに仕分け
	map<wstring, pair<ULONGLONG, vector<wstring>>> mountMap;
	for( size_t i = 0; i < delFolderList.size(); i++ ){
		wstring mountPath;
		GetChkDrivePath(delFolderList[i], mountPath);
		std::transform(mountPath.begin(), mountPath.end(), mountPath.begin(), toupper);
		map<wstring, pair<ULONGLONG, vector<wstring>>>::iterator itr = mountMap.find(mountPath);
		if( itr == mountMap.end() ){
			pair<wstring, pair<ULONGLONG, vector<wstring>>> item;
			item.first = mountPath;
			item.second.first = 0;
			itr = mountMap.insert(item).first;
		}
		itr->second.second.push_back(delFolderList[i]);
	}

	//直近で必要になりそうな空き領域を概算する
	LONGLONG now = GetNowI64Time();
	for( size_t i = 0; i < chkReserve.size(); i++ ){
		if( chkReserve[i].recSetting.recMode != RECMODE_NO &&
		    chkReserve[i].recSetting.recMode != RECMODE_VIEW &&
		    now < ConvertI64Time(chkReserve[i].startTime) && ConvertI64Time(chkReserve[i].startTime) < now + 2*60*60*I64_1SEC ){
			//録画開始2時間前までの予約
			vector<wstring> recFolderList;
			if( chkReserve[i].recSetting.recFolderList.empty() ){
				//デフォルト
				recFolderList.push_back(defRecFolder);
			}else{
				//複数指定あり
				for( size_t j = 0; j < chkReserve[i].recSetting.recFolderList.size(); j++ ){
					recFolderList.push_back(chkReserve[i].recSetting.recFolderList[j].recFolder);
				}
			}
			for( size_t j = 0; j < recFolderList.size(); j++ ){
				wstring mountPath;
				GetChkDrivePath(recFolderList[j], mountPath);
				std::transform(mountPath.begin(), mountPath.end(), mountPath.begin(), toupper);
				map<wstring, pair<ULONGLONG, vector<wstring>>>::iterator itr = mountMap.find(mountPath);
				if( itr != mountMap.end() ){
					DWORD bitrate = 0;
					_GetBitrate(chkReserve[i].originalNetworkID, chkReserve[i].transportStreamID, chkReserve[i].serviceID, &bitrate);
					itr->second.first += (ULONGLONG)(bitrate / 8 * 1000) * chkReserve[i].durationSecond;
				}
			}
		}
	}

	//ドライブレベルでのチェック
	map<wstring, pair<ULONGLONG, vector<wstring>>>::const_iterator itr;
	for( itr = mountMap.begin(); itr != mountMap.end(); itr++ ){
		ULARGE_INTEGER freeBytes;
		if( itr->second.first > 0 && GetDiskFreeSpaceEx(itr->first.c_str(), &freeBytes, NULL, NULL) != FALSE && freeBytes.QuadPart < itr->second.first ){
			//ドライブにある古いTS順に必要なだけ消す
			LONGLONG needFreeSize = itr->second.first - freeBytes.QuadPart;
			multimap<LONGLONG, pair<ULONGLONG, wstring>> tsFileMap;
			for( size_t i = 0; i < itr->second.second.size(); i++ ){
				wstring delFolder = itr->second.second[i];
				ChkFolderPath(delFolder);
				WIN32_FIND_DATA findData;
				HANDLE hFind = FindFirstFile((delFolder + L"\\*.ts").c_str(), &findData);
				if( hFind != INVALID_HANDLE_VALUE ){
					do{
						if( (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 && IsExt(findData.cFileName, L".ts") != FALSE ){
							pair<LONGLONG, pair<ULONGLONG, wstring>> item;
							item.first = (LONGLONG)findData.ftCreationTime.dwHighDateTime << 32 | findData.ftCreationTime.dwLowDateTime;
							item.second.first = (ULONGLONG)findData.nFileSizeHigh << 32 | findData.nFileSizeLow;
							item.second.second = delFolder + L"\\" + findData.cFileName;
							tsFileMap.insert(item);
						}
					}while( FindNextFile(hFind, &findData) );
					FindClose(hFind);
				}
			}
			while( needFreeSize > 0 && tsFileMap.empty() == false ){
				wstring delPath = tsFileMap.begin()->second.second;
				wstring delPathUpper = delPath;
				std::transform(delPathUpper.begin(), delPathUpper.end(), delPathUpper.begin(), toupper);
				map<wstring, wstring>::const_iterator itrP = protectFile.find(delPathUpper);
				if( itrP != protectFile.end() ){
					_OutputDebugString(L"★No Delete(Protected) : %s", delPath.c_str());
				}else{
					DeleteFile(delPath.c_str());
					needFreeSize -= tsFileMap.begin()->second.first;
					_OutputDebugString(L"★Auto Delete2 : %s", delPath.c_str());
					for( size_t i = 0 ; i < delExtList.size(); i++ ){
						wstring delFolder;
						wstring delTitle;
						GetFileFolder(delPath, delFolder);
						GetFileTitle(delPath, delTitle);
						DeleteFile((delFolder + L"\\" + delTitle + delExtList[i]).c_str());
						_OutputDebugString(L"★Auto Delete2 : %s", (delFolder + L"\\" + delTitle + delExtList[i]).c_str());
					}
				}
				tsFileMap.erase(tsFileMap.begin());
			}
		}
	}
}
