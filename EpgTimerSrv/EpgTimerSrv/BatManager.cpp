#include "StdAfx.h"
#include "BatManager.h"

#include "../../Common/StringUtil.h"
#include "../../Common/PathUtil.h"
#include "../../Common/BlockLock.h"

#include <process.h>

CBatManager::CBatManager(void)
{
	InitializeCriticalSection(&this->managerLock);

	this->pauseFlag = FALSE;
	this->batWorkExitingFlag = FALSE;

	this->batWorkThread = NULL;
	this->batWorkStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	this->lastSuspendMode = 0xFF;
	this->lastRebootFlag = 0xFF;

	this->notifyManager = NULL;
}

CBatManager::~CBatManager(void)
{
	if( this->batWorkThread != NULL ){
		::SetEvent(this->batWorkStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->batWorkThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->batWorkThread, 0xffffffff);
		}
		CloseHandle(this->batWorkThread);
		this->batWorkThread = NULL;
	}
	if( this->batWorkStopEvent != NULL ){
		CloseHandle(this->batWorkStopEvent);
		this->batWorkStopEvent = NULL;
	}

	DeleteCriticalSection(&this->managerLock);
}

void CBatManager::SetNotifyManager(CNotifyManager* manager)
{
	CBlockLock lock(&this->managerLock);

	this->notifyManager = manager;
}

void CBatManager::AddBatWork(const BAT_WORK_INFO& info)
{
	CBlockLock lock(&this->managerLock);

	this->workList.push_back(info);
}

DWORD CBatManager::GetWorkCount()
{
	CBlockLock lock(&this->managerLock);

	return (DWORD)this->workList.size();
}

BOOL CBatManager::IsWorking()
{
	CBlockLock lock(&this->managerLock);

	return this->batWorkThread != NULL && WaitForSingleObject(this->batWorkThread, 0) == WAIT_TIMEOUT ? TRUE : FALSE;
}

void CBatManager::StartWork()
{
	CBlockLock lock(&this->managerLock);

	//ワーカスレッドが終了しようとしているときはその完了を待つ
	if( this->batWorkThread != NULL && this->batWorkExitingFlag != FALSE ){
		WaitForSingleObject(this->batWorkThread, INFINITE);
		CloseHandle(this->batWorkThread);
		this->batWorkThread = NULL;
	}
	this->pauseFlag = FALSE;
	if( this->batWorkThread == NULL ){
		ResetEvent(this->batWorkStopEvent);
		this->batWorkExitingFlag = FALSE;
		this->batWorkThread = (HANDLE)_beginthreadex(NULL, 0, BatWorkThread, this, 0, NULL);
	}
}

void CBatManager::PauseWork()
{
	CBlockLock lock(&this->managerLock);

	this->pauseFlag = TRUE;
}


BOOL CBatManager::GetLastWorkSuspend(BYTE* suspendMode, BYTE* rebootFlag)
{
	CBlockLock lock(&this->managerLock);

	BOOL ret = FALSE;
	if( this->lastSuspendMode != 0xFF && this->lastRebootFlag != 0xFF ){
		ret = TRUE;
		*suspendMode = this->lastSuspendMode;
		*rebootFlag = this->lastRebootFlag;

		this->lastSuspendMode = 0xFF;
		this->lastRebootFlag = 0xFF;
	}

	return ret;
}

UINT WINAPI CBatManager::BatWorkThread(LPVOID param)
{
	CBatManager* sys = (CBatManager*)param;
	CSendCtrlCmd sendCtrl;

	while(1){
		if( ::WaitForSingleObject(sys->batWorkStopEvent, 1000) != WAIT_TIMEOUT ){
			//キャンセルされた
			break;
		}
		{
			BAT_WORK_INFO work;
			{
				CBlockLock lock(&sys->managerLock);
				if( sys->pauseFlag != FALSE || sys->workList.empty() ){
					//このフラグを立てたあとは二度とロックを確保してはいけない
					sys->batWorkExitingFlag = TRUE;
					break;
				}else{
					work = sys->workList[0];
					sys->lastSuspendMode = work.reserveInfo.recSetting.suspendMode;
					sys->lastRebootFlag = work.reserveInfo.recSetting.rebootFlag;
				}
			}

			if( work.reserveInfo.recSetting.batFilePath.size() > 0 ){
				wstring batFilePath = L"";
				GetModuleFolderPath(batFilePath);
				batFilePath += L"\\EpgTimer_Bon_RecEnd.bat";
				if( CreateBatFile(work, work.reserveInfo.recSetting.batFilePath.c_str(), batFilePath.c_str()) != FALSE ){
					wstring strExecute;
					Format(strExecute, L"\"%s\"", batFilePath.c_str());

					BOOL send = FALSE;
					DWORD PID = 0;
					map<DWORD, DWORD>::iterator itr;
					map<DWORD, DWORD> registGUIMap;
					if( sys->notifyManager != NULL ){
						sys->notifyManager->GetRegistGUI(&registGUIMap);
					}
					for( itr = registGUIMap.begin(); itr != registGUIMap.end(); itr++ ){
						sendCtrl.SetPipeSetting(CMD2_GUI_CTRL_WAIT_CONNECT, CMD2_GUI_CTRL_PIPE, itr->first);

						if( sendCtrl.SendGUIExecute(strExecute.c_str(), &PID) == CMD_SUCCESS ){
							send = TRUE;
							break;
						}
					}
					if( send == FALSE ){
						//GUI経由で起動できなかった
						PROCESS_INFORMATION pi;
						STARTUPINFO si;
						ZeroMemory(&si,sizeof(si));
						si.cb=sizeof(si);

						vector<WCHAR> strBuff(strExecute.c_str(), strExecute.c_str() + strExecute.size() + 1);
						send = CreateProcess( NULL, &strBuff.front(), NULL, NULL, FALSE, GetPriorityClass(GetCurrentProcess()), NULL, NULL, &si, &pi );
						if( send != FALSE ){
							CloseHandle(pi.hThread);
							CloseHandle(pi.hProcess);

							PID = pi.dwProcessId;
						}
					}
					if( send != FALSE ){
						//終了監視
						while(1){
							if( WaitForSingleObject( sys->batWorkStopEvent, 2000 ) != WAIT_TIMEOUT ){
								//中止
								break;
							}
							if( _FindOpenExeProcess(PID) == FALSE ){
								//終わった
								break;
							}
						}
					}
				}else{
					_OutputDebugString(L"BATファイル作成エラー：%s", work.reserveInfo.recSetting.batFilePath.c_str());
				}
			}

			CBlockLock lock(&sys->managerLock);
			sys->workList.erase(sys->workList.begin());
		}
	}

	return 0;
}

BOOL CBatManager::CreateBatFile(const BAT_WORK_INFO& info, LPCWSTR batSrcFilePath, LPCWSTR batFilePath )
{
	//バッチの作成
	HANDLE hRead = CreateFile( batSrcFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if( hRead == INVALID_HANDLE_VALUE ){
		return FALSE;
	}
	HANDLE hWrite = CreateFile( batFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if( hWrite == INVALID_HANDLE_VALUE ){
		CloseHandle(hRead);
		return FALSE;
	}

	DWORD dwRead=0;
	DWORD dwL = GetFileSize(hRead, NULL);
	if( dwL == INVALID_FILE_SIZE ){
		dwL = 0;
	}
	vector<char> buff(dwL + 1);
	if( dwL == 0 || ReadFile(hRead, &buff.front(), dwL, &dwRead, NULL) == FALSE || dwRead != dwL ){
		CloseHandle(hRead);
		CloseHandle(hWrite);
		return FALSE;
	}
	buff[dwL] = '\0';
	CloseHandle(hRead);

	string strRead = "";
	strRead = &buff.front();

	string strWrite;
	for( size_t pos = 0;; ){
		size_t next = strRead.find('$', pos);
		if( next == string::npos ){
			strWrite.append(strRead, pos, string::npos);
			break;
		}
		strWrite.append(strRead, pos, next - pos);
		pos = next;

		next = strRead.find('$', pos + 1);
		if( next == string::npos ){
			strWrite.append(strRead, pos, string::npos);
			break;
		}
		if( ExpandMacro(strRead.substr(pos + 1, next - pos - 1), info, strWrite) == FALSE ){
			strWrite += '$';
			pos++;
		}else{
			pos = next + 1;
		}
	}

	DWORD dwWrite=0;
	if( WriteFile(hWrite, strWrite.c_str(), (DWORD)strWrite.size(), &dwWrite, NULL) == FALSE || dwWrite != strWrite.size() ){
		CloseHandle(hWrite);
		return FALSE;
	}
	CloseHandle(hWrite);

	return TRUE;
}

BOOL CBatManager::ExpandMacro(const string& var, const BAT_WORK_INFO& info, string& strWrite)
{
	string strSDW28;
	SYSTEMTIME t28TimeS;
	if( 0 <= info.recFileInfo.startTime.wHour && info.recFileInfo.startTime.wHour < 4 ){
		GetSumTime(info.recFileInfo.startTime, -24*60*60, &t28TimeS);
		GetDayOfWeekString2(t28TimeS, strSDW28);
		t28TimeS.wHour+=24;
	}else{
		t28TimeS = info.recFileInfo.startTime;
		GetDayOfWeekString2(t28TimeS, strSDW28);
	}

	SYSTEMTIME tEnd;
	GetI64Time(info.recFileInfo.startTime, info.recFileInfo.durationSecond, NULL, NULL, &tEnd);

	string strEDW28;
	SYSTEMTIME t28TimeE;
	if( 0 <= tEnd.wHour && tEnd.wHour < 4 ){
		GetSumTime(tEnd, -24*60*60, &t28TimeE);
		GetDayOfWeekString2(t28TimeE, strEDW28);
		t28TimeE.wHour+=24;
	}else{
		t28TimeE = tEnd;
		GetDayOfWeekString2(t28TimeE, strEDW28);
	}

	string ret;
	if( var == "FilePath" )		WtoA(info.recFileInfo.recFilePath, ret);
	else if( var == "Title" )	WtoA(info.recFileInfo.title, ret);
	else if( var == "SDYYYY" )	Format(ret, "%04d", info.recFileInfo.startTime.wYear);
	else if( var == "SDYY" )	Format(ret, "%02d", info.recFileInfo.startTime.wYear%100);
	else if( var == "SDMM" )	Format(ret, "%02d", info.recFileInfo.startTime.wMonth);
	else if( var == "SDM" )		Format(ret, "%d", info.recFileInfo.startTime.wMonth);
	else if( var == "SDDD" )	Format(ret, "%02d", info.recFileInfo.startTime.wDay);
	else if( var == "SDD" )		Format(ret, "%d", info.recFileInfo.startTime.wDay);
	else if( var == "SDW" )		GetDayOfWeekString2(info.recFileInfo.startTime, ret);
	else if( var == "STHH" )	Format(ret, "%02d", info.recFileInfo.startTime.wHour);
	else if( var == "STH" )		Format(ret, "%d", info.recFileInfo.startTime.wHour);
	else if( var == "STMM" )	Format(ret, "%02d", info.recFileInfo.startTime.wMinute);
	else if( var == "STM" )		Format(ret, "%d", info.recFileInfo.startTime.wMinute);
	else if( var == "STSS" )	Format(ret, "%02d", info.recFileInfo.startTime.wSecond);
	else if( var == "STS" )		Format(ret, "%d", info.recFileInfo.startTime.wSecond);
	else if( var == "EDYYYY" )	Format(ret, "%04d", tEnd.wYear);
	else if( var == "EDYY" )	Format(ret, "%02d", tEnd.wYear%100);
	else if( var == "EDMM" )	Format(ret, "%02d", tEnd.wMonth);
	else if( var == "EDM" )		Format(ret, "%d", tEnd.wMonth);
	else if( var == "EDDD" )	Format(ret, "%02d", tEnd.wDay);
	else if( var == "EDD" )		Format(ret, "%d", tEnd.wDay);
	else if( var == "EDW" )		GetDayOfWeekString2(tEnd, ret);
	else if( var == "ETHH" )	Format(ret, "%02d", tEnd.wHour);
	else if( var == "ETH" )		Format(ret, "%d", tEnd.wHour);
	else if( var == "ETMM" )	Format(ret, "%02d", tEnd.wMinute);
	else if( var == "ETM" )		Format(ret, "%d", tEnd.wMinute);
	else if( var == "ETSS" )	Format(ret, "%02d", tEnd.wSecond);
	else if( var == "ETS" )		Format(ret, "%d", tEnd.wSecond);
	else if( var == "ONID10" )	Format(ret, "%d", info.recFileInfo.originalNetworkID);
	else if( var == "TSID10" )	Format(ret, "%d", info.recFileInfo.transportStreamID);
	else if( var == "SID10" )	Format(ret, "%d", info.recFileInfo.serviceID);
	else if( var == "EID10" )	Format(ret, "%d", info.recFileInfo.eventID);
	else if( var == "ONID16" )	Format(ret, "%04X", info.recFileInfo.originalNetworkID);
	else if( var == "TSID16" )	Format(ret, "%04X", info.recFileInfo.transportStreamID);
	else if( var == "SID16" )	Format(ret, "%04X", info.recFileInfo.serviceID);
	else if( var == "EID16" )	Format(ret, "%04X", info.recFileInfo.eventID);
	else if( var == "ServiceName" )	WtoA(info.recFileInfo.serviceName, ret);
	else if( var == "SDYYYY28" )	Format(ret, "%04d", t28TimeS.wYear);
	else if( var == "SDYY28" )	Format(ret, "%02d", t28TimeS.wYear%100);
	else if( var == "SDMM28" )	Format(ret, "%02d", t28TimeS.wMonth);
	else if( var == "SDM28" )	Format(ret, "%d", t28TimeS.wMonth);
	else if( var == "SDDD28" )	Format(ret, "%02d", t28TimeS.wDay);
	else if( var == "SDD28" )	Format(ret, "%d", t28TimeS.wDay);
	else if( var == "SDW28" )	ret = strSDW28;
	else if( var == "STHH28" )	Format(ret, "%02d", t28TimeS.wHour);
	else if( var == "STH28" )	Format(ret, "%d", t28TimeS.wHour);
	else if( var == "EDYYYY28" )	Format(ret, "%04d", t28TimeE.wYear);
	else if( var == "EDYY28" )	Format(ret, "%02d", t28TimeE.wYear%100);
	else if( var == "EDMM28" )	Format(ret, "%02d", t28TimeE.wMonth);
	else if( var == "EDM28" )	Format(ret, "%d", t28TimeE.wMonth);
	else if( var == "EDDD28" )	Format(ret, "%02d", t28TimeE.wDay);
	else if( var == "EDD28" )	Format(ret, "%d", t28TimeE.wDay);
	else if( var == "EDW28" )	ret = strEDW28;
	else if( var == "ETHH28" )	Format(ret, "%02d", t28TimeE.wHour);
	else if( var == "ETH28" )	Format(ret, "%d", t28TimeE.wHour);
	else if( var == "DUHH" )	Format(ret, "%02d", info.recFileInfo.durationSecond/(60*60));
	else if( var == "DUH" )		Format(ret, "%d", info.recFileInfo.durationSecond/(60*60));
	else if( var == "DUMM" )	Format(ret, "%02d", (info.recFileInfo.durationSecond%(60*60))/60);
	else if( var == "DUM" )		Format(ret, "%d", (info.recFileInfo.durationSecond%(60*60))/60);
	else if( var == "DUSS" )	Format(ret, "%02d", info.recFileInfo.durationSecond%60);
	else if( var == "DUS" )		Format(ret, "%d", info.recFileInfo.durationSecond%60);
	else if( var == "Drops" )	Format(ret, "%I64d", info.recFileInfo.drops);
	else if( var == "Scrambles" )	Format(ret, "%I64d", info.recFileInfo.scrambles);
	else if( var == "Result" )	WtoA(info.recFileInfo.comment, ret);
	else if( var == "FolderPath" ){
		wstring strFolder;
		GetFileFolder(info.recFileInfo.recFilePath, strFolder);
		ChkFolderPath(strFolder);
		WtoA(strFolder, ret);
	}else if( var == "FileName" ){
		wstring strTitle;
		GetFileTitle(info.recFileInfo.recFilePath, strTitle);
		WtoA(strTitle, ret);
	}else if( var == "TitleF" ){
		wstring strTemp = info.recFileInfo.title;
		CheckFileName(strTemp);
		WtoA(strTemp, ret);
	}else if( var == "Title2" || var == "Title2F" ){
		wstring strTemp = info.recFileInfo.title;
		while( strTemp.find(L"[") != wstring::npos && strTemp.find(L"]") != wstring::npos ){
			wstring strSep1;
			wstring strSep2;
			Separate(strTemp, L"[", strTemp, strSep1);
			Separate(strSep1, L"]", strSep2, strSep1);
			strTemp += strSep1;
		}
		if( var == "Title2F" ){
			CheckFileName(strTemp);
		}
		WtoA(strTemp, ret);
	}else if( var == "AddKey" ){
		if( info.reserveInfo.comment.compare(0, 8, L"EPG自動予約(") == 0 && info.reserveInfo.comment.size() >= 9 ){
			WtoA(info.reserveInfo.comment.substr(8, info.reserveInfo.comment.size() - 9), ret);
		}
	}else{
		return FALSE;
	}

	strWrite += ret;

	return TRUE;
}

