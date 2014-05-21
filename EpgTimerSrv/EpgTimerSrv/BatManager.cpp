#include "StdAfx.h"
#include "BatManager.h"

#include "../../Common/StringUtil.h"
#include "../../Common/PathUtil.h"

#include <process.h>

CBatManager::CBatManager(void)
{
	this->lockEvent = CreateEvent(NULL, FALSE, TRUE, NULL);

	this->workFlag = FALSE;

	this->batWorkThread = NULL;
	this->batWorkStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	this->lastSuspendMode = 0xFF;
	this->lastRebootFlag = 0xFF;

	this->notifyManager = NULL;

	ReloadSetting();
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

	if( this->lockEvent != NULL ){
		UnLock();
		CloseHandle(this->lockEvent);
		this->lockEvent = NULL;
	}
}

BOOL CBatManager::Lock(LPCWSTR log, DWORD timeOut)
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
			OutputDebugString(L"◆CBatManager::Lock FALSE");
			if( log != NULL ){
				OutputDebugString(log);
			}
		return FALSE;
	}
	return TRUE;
}

void CBatManager::UnLock(LPCWSTR log)
{
	if( this->lockEvent != NULL ){
		SetEvent(this->lockEvent);
	}
	if( log != NULL ){
		OutputDebugString(log);
	}
}

void CBatManager::ReloadSetting()
{
	if( Lock() == FALSE ) return ;

	
	UnLock();
}
/*
void CBatManager::SetRegistGUI(map<DWORD, DWORD> registGUIMap)
{
	if( Lock() == FALSE ) return ;

	this->registGUIMap = registGUIMap;

	UnLock();
}
*/
void CBatManager::SetNotifyManager(CNotifyManager* manager)
{
	if( Lock(L"CBatManager::SetNotifyManager") == FALSE ) return;

	this->notifyManager = manager;

	UnLock();
}

void CBatManager::AddBatWork(BAT_WORK_INFO* info)
{
	if( Lock() == FALSE ) return ;

	workList.push_back(*info);
	
	UnLock();
}

DWORD CBatManager::GetWorkCount()
{
	if( Lock() == FALSE ) return FALSE;

	DWORD ret = (DWORD)workList.size();
	
	UnLock();

	return ret;
}

BOOL CBatManager::IsWorking()
{
	if( Lock() == FALSE ) return FALSE;

	BOOL ret = this->workFlag;
	
	UnLock();

	return ret;
}

void CBatManager::StartWork()
{
	if( Lock() == FALSE ) return ;

	this->workFlag = TRUE;
	if( this->batWorkThread != NULL ){
		if( ::WaitForSingleObject(this->batWorkThread, 0) == WAIT_OBJECT_0 ){
			CloseHandle(this->batWorkThread);
			this->batWorkThread = NULL;
		}
	}
	if( this->batWorkThread == NULL ){
		ResetEvent(this->batWorkStopEvent);
		this->batWorkThread = (HANDLE)_beginthreadex(NULL, 0, BatWorkThread, (LPVOID)this, CREATE_SUSPENDED, NULL);
		SetThreadPriority( this->batWorkThread, THREAD_PRIORITY_NORMAL );
		ResumeThread(this->batWorkThread);
	}

	UnLock();
}

void CBatManager::PauseWork()
{
	if( Lock() == FALSE ) return ;

	this->workFlag = FALSE;

	UnLock();
}


BOOL CBatManager::GetLastWorkSuspend(BYTE* suspendMode, BYTE* rebootFlag)
{
	if( Lock() == FALSE ) return FALSE;

	BOOL ret = FALSE;
	if( this->lastSuspendMode != 0xFF && this->lastRebootFlag != 0xFF ){
		ret = TRUE;
		*suspendMode = this->lastSuspendMode;
		*rebootFlag = this->lastRebootFlag;

		this->lastSuspendMode = 0xFF;
		this->lastRebootFlag = 0xFF;
	}

	UnLock();
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
		if( sys->workFlag == TRUE ){
			BAT_WORK_INFO work;
			if( sys->Lock() == TRUE ){
				if( sys->workList.size() == 0 ){
					sys->UnLock();
					break;
				}else{
					work = sys->workList[0];
				}
			}else{
				continue;
			}

			sys->UnLock();

			if( work.reserveInfo.recSetting.batFilePath.size() > 0 ){
				wstring batFilePath = L"";
				if( sys->CreateBatFile(&work, work.reserveInfo.recSetting.batFilePath, batFilePath) == TRUE ){
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
						wstring pipeName = L"";
						wstring waitEventName = L"";

						Format(pipeName, L"%s%d", CMD2_GUI_CTRL_PIPE, itr->first );
						Format(waitEventName, L"%s%d", CMD2_GUI_CTRL_WAIT_CONNECT, itr->first );

						sendCtrl.SetPipeSetting(waitEventName, pipeName);

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

						send = CreateProcess( NULL, (WCHAR*)strExecute.c_str(), NULL, NULL, FALSE, GetPriorityClass(GetCurrentProcess()), NULL, NULL, &si, &pi );
						if( send == TRUE ){
							CloseHandle(pi.hThread);
							CloseHandle(pi.hProcess);

							PID = pi.dwProcessId;
						}
					}
					if( send == TRUE ){
						//終了監視
						while(1){
							if( WaitForSingleObject( sys->batWorkStopEvent, 2000 ) != WAIT_TIMEOUT ){
								//中止
								break;
							}
							if( _FindOpenExeProcess(PID) == FALSE ){
								//終わった
								sys->lastSuspendMode = work.reserveInfo.recSetting.suspendMode;
								sys->lastRebootFlag = work.reserveInfo.recSetting.rebootFlag;
								break;
							}
						}
					}
				}else{
					_OutputDebugString(L"BATファイル作成エラー：%s", work.reserveInfo.recSetting.batFilePath.c_str());
				}
			}

			if( sys->Lock() == TRUE ){
				sys->workList.erase(sys->workList.begin());
				sys->UnLock();
			}
		}
	}
	sys->workFlag = FALSE;

	return 0;
}

BOOL CBatManager::CreateBatFile(BAT_WORK_INFO* info, wstring batSrcFilePath, wstring& batFilePath )
{
	if( info == NULL ){
		return FALSE;
	}
	GetModuleFolderPath(batFilePath);
	batFilePath+=L"\\EpgTimer_Bon_RecEnd.bat";

	//バッチの作成
	HANDLE hRead = CreateFileW( batSrcFilePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if( hRead == INVALID_HANDLE_VALUE ){
		return FALSE;
	}
	HANDLE hWrite = CreateFile( batFilePath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if( hWrite == INVALID_HANDLE_VALUE ){
		CloseHandle(hRead);
		return FALSE;
	}

	DWORD dwRead=0;
	DWORD dwL = GetFileSize(hRead, NULL);
	char* pBuff = new char[dwL+1];
	ZeroMemory(pBuff, dwL+1);
	ReadFile(hRead, pBuff, dwL, &dwRead, NULL );
	CloseHandle(hRead);

	string strRead = "";
	strRead = pBuff;
	SAFE_DELETE_ARRAY(pBuff);

	string strRecFilePath="";
	string strFolderPath="";
	string strFileName="";
	string strTitle="";
	string strSDYYYY="";
	string strSDYY="";
	string strSDMM="";
	string strSDM="";
	string strSDDD="";
	string strSDD="";
	string strSDW="";
	string strSTHH="";
	string strSTH="";
	string strSTMM="";
	string strSTM="";
	string strSTSS="";
	string strSTS="";
	string strEDYYYY="";
	string strEDYY="";
	string strEDMM="";
	string strEDM="";
	string strEDDD="";
	string strEDD="";
	string strEDW="";
	string strETHH="";
	string strETH="";
	string strETMM="";
	string strETM="";
	string strETSS="";
	string strETS="";
	string strONID10="";
	string strTSID10="";
	string strSID10="";
	string strEID10="";
	string strONID16="";
	string strTSID16="";
	string strSID16="";
	string strEID16="";
	string strServiceName="";
	string strSDYYYY28="";
	string strSDYY28="";
	string strSDMM28="";
	string strSDM28="";
	string strSDDD28="";
	string strSDD28="";
	string strSDW28="";
	string strSTHH28="";
	string strSTH28="";
	string strEDYYYY28="";
	string strEDYY28="";
	string strEDMM28="";
	string strEDM28="";
	string strEDDD28="";
	string strEDD28="";
	string strEDW28="";
	string strETHH28="";
	string strETH28="";
	string strDUH="";
	string strDUHH="";
	string strDUM="";
	string strDUMM="";
	string strDUS="";
	string strDUSS="";
	string strTitle2="";
	string strDrops="";
	string strScrambles="";
	string strResult="";
	string strTitleF="";
	string strTitle2F="";
	string strAddKey="";

	WtoA(info->recFileInfo.recFilePath, strRecFilePath);

	WCHAR szDrive[_MAX_DRIVE];
	WCHAR szDir[_MAX_DIR];
	WCHAR szFname[_MAX_FNAME];
	WCHAR szExt[_MAX_EXT];
	WCHAR szPath[_MAX_PATH] = L"";
	_tsplitpath_s( info->recFileInfo.recFilePath.c_str(), szDrive, _MAX_DRIVE, szDir, _MAX_DIR, szFname, _MAX_FNAME, szExt, _MAX_EXT );
	_tmakepath_s( szPath, _MAX_PATH, szDrive, szDir, NULL, NULL );
	wstring strFolder;
	strFolder = szPath;
	ChkFolderPath(strFolder);

	WtoA(szFname, strFileName);
	WtoA(strFolder, strFolderPath);
	WtoA(info->recFileInfo.title, strTitle);

	Format(strSDYYYY, "%04d", info->recFileInfo.startTime.wYear);
	Format(strSDYY, "%02d", info->recFileInfo.startTime.wYear%100);
	Format(strSDMM, "%02d", info->recFileInfo.startTime.wMonth);
	Format(strSDM, "%d", info->recFileInfo.startTime.wMonth);
	Format(strSDDD, "%02d", info->recFileInfo.startTime.wDay);
	Format(strSDD, "%d", info->recFileInfo.startTime.wDay);
	GetDayOfWeekString2(info->recFileInfo.startTime, strSDW);
	Format(strSTHH, "%02d", info->recFileInfo.startTime.wHour);
	Format(strSTH, "%d", info->recFileInfo.startTime.wHour);
	Format(strSTMM, "%02d", info->recFileInfo.startTime.wMinute);
	Format(strSTM, "%d", info->recFileInfo.startTime.wMinute);
	Format(strSTSS, "%02d", info->recFileInfo.startTime.wSecond);
	Format(strSTS, "%d", info->recFileInfo.startTime.wSecond);

	SYSTEMTIME t28TimeS;
	if( 0 <= info->recFileInfo.startTime.wHour && info->recFileInfo.startTime.wHour < 4 ){
		GetSumTime(info->recFileInfo.startTime, -24*60*60, &t28TimeS);
		GetDayOfWeekString2(t28TimeS, strSDW28);
		t28TimeS.wHour+=24;
	}else{
		t28TimeS = info->recFileInfo.startTime;
		GetDayOfWeekString2(t28TimeS, strSDW28);
	}

	Format(strSDYYYY28, "%04d", t28TimeS.wYear);
	Format(strSDYY28, "%02d", t28TimeS.wYear%100);
	Format(strSDMM28, "%02d", t28TimeS.wMonth);
	Format(strSDM28, "%d", t28TimeS.wMonth);
	Format(strSDDD28, "%02d", t28TimeS.wDay);
	Format(strSDD28, "%d", t28TimeS.wDay);
	Format(strSTHH28, "%02d", t28TimeS.wHour);
	Format(strSTH28, "%d", t28TimeS.wHour);

	SYSTEMTIME tEnd;
	GetI64Time(info->recFileInfo.startTime, info->recFileInfo.durationSecond, NULL, NULL, &tEnd);

	Format(strEDYYYY, "%04d", tEnd.wYear);
	Format(strEDYY, "%02d", tEnd.wYear%100);
	Format(strEDMM, "%02d", tEnd.wMonth);
	Format(strEDM, "%d", tEnd.wMonth);
	Format(strEDDD, "%02d", tEnd.wDay);
	Format(strEDD, "%d", tEnd.wDay);
	GetDayOfWeekString2(tEnd, strEDW);
	Format(strETHH, "%02d", tEnd.wHour);
	Format(strETH, "%d", tEnd.wHour);
	Format(strETMM, "%02d", tEnd.wMinute);
	Format(strETM, "%d", tEnd.wMinute);
	Format(strETSS, "%02d", tEnd.wSecond);
	Format(strETS, "%d", tEnd.wSecond);

	SYSTEMTIME t28TimeE;
	if( 0 <= tEnd.wHour && tEnd.wHour < 4 ){
		GetSumTime(tEnd, -24*60*60, &t28TimeE);
		GetDayOfWeekString2(t28TimeE, strEDW28);
		t28TimeE.wHour+=24;
	}else{
		t28TimeE = tEnd;
		GetDayOfWeekString2(tEnd, strEDW28);
	}

	Format(strEDYYYY28, "%04d", t28TimeE.wYear);
	Format(strEDYY28, "%02d", t28TimeE.wYear%100);
	Format(strEDMM28, "%02d", t28TimeE.wMonth);
	Format(strEDM28, "%d", t28TimeE.wMonth);
	Format(strEDDD28, "%02d", t28TimeE.wDay);
	Format(strEDD28, "%d", t28TimeE.wDay);
	Format(strETHH28, "%02d", t28TimeE.wHour);
	Format(strETH28, "%d", t28TimeE.wHour);

	Format(strONID10, "%d", info->recFileInfo.originalNetworkID);
	Format(strTSID10, "%d", info->recFileInfo.transportStreamID);
	Format(strSID10, "%d", info->recFileInfo.serviceID);
	Format(strEID10, "%d", info->recFileInfo.eventID);
	Format(strONID16, "%04X", info->recFileInfo.originalNetworkID);
	Format(strTSID16, "%04X", info->recFileInfo.transportStreamID);
	Format(strSID16, "%04X", info->recFileInfo.serviceID);
	Format(strEID16, "%04X", info->recFileInfo.eventID);

	WtoA(info->recFileInfo.serviceName, strServiceName);

	Format(strDUHH, "%02d", info->recFileInfo.durationSecond/(60*60));
	Format(strDUH, "%d", info->recFileInfo.durationSecond/(60*60));
	Format(strDUMM, "%02d", (info->recFileInfo.durationSecond%(60*60))/60);
	Format(strDUM, "%d", (info->recFileInfo.durationSecond%(60*60))/60);
	Format(strDUSS, "%02d", info->recFileInfo.durationSecond%60);
	Format(strDUS, "%d", info->recFileInfo.durationSecond%60);

	wstring strTemp = info->recFileInfo.title;
	while( (strTemp.find(L"[") != string::npos) && (strTemp.find(L"]") != string::npos) ){
		wstring strSep1=L"";
		wstring strSep2=L"";
		Separate(strTemp, L"[", strSep1, strTemp);
		Separate(strTemp, L"]", strSep2, strTemp);
		strSep1 += strTemp;
		strTemp = strSep1;
	}
	WtoA(strTemp, strTitle2);

	Format(strDrops, "%I64d", info->recFileInfo.drops);
	Format(strScrambles, "%I64d", info->recFileInfo.scrambles);
	WtoA(info->recFileInfo.comment, strResult);

	CheckFileName(strTitleF);
	CheckFileName(strTemp);
	WtoA(strTemp, strTitle2F);
	strTemp = info->recFileInfo.title;
	CheckFileName(strTemp);
	WtoA(strTemp, strTitleF);

	if( info->reserveInfo.comment.find(L"EPG自動予約(") != string::npos ){
		WtoA(info->reserveInfo.comment, strAddKey);
		Replace(strAddKey, "EPG自動予約(", "");
		strAddKey.erase(strAddKey.length()-1, 1);
	}

	Replace(strRead, "$FilePath$", strRecFilePath);
	Replace(strRead, "$FolderPath$", strFolderPath);
	Replace(strRead, "$FileName$", strFileName);
	Replace(strRead, "$Title$", strTitle);
	Replace(strRead, "$SDYYYY$", strSDYYYY);
	Replace(strRead, "$SDYY$", strSDYY);
	Replace(strRead, "$SDMM$", strSDMM);
	Replace(strRead, "$SDM$", strSDM);
	Replace(strRead, "$SDDD$", strSDDD);
	Replace(strRead, "$SDD$", strSDD);
	Replace(strRead, "$SDW$", strSDW);
	Replace(strRead, "$STHH$", strSTHH);
	Replace(strRead, "$STH$", strSTH);
	Replace(strRead, "$STMM$", strSTMM);
	Replace(strRead, "$STM$", strSTM);
	Replace(strRead, "$STSS$", strSTSS);
	Replace(strRead, "$STS$", strSTS);
	Replace(strRead, "$EDYYYY$", strEDYYYY);
	Replace(strRead, "$EDYY$", strEDYY);
	Replace(strRead, "$EDMM$", strEDMM);
	Replace(strRead, "$EDM$", strEDM);
	Replace(strRead, "$EDDD$", strEDDD);
	Replace(strRead, "$EDD$", strEDD);
	Replace(strRead, "$EDW$", strEDW);
	Replace(strRead, "$ETHH$", strETHH);
	Replace(strRead, "$ETH$", strETH);
	Replace(strRead, "$ETMM$", strETMM);
	Replace(strRead, "$ETM$", strETM);
	Replace(strRead, "$ETSS$", strETSS);
	Replace(strRead, "$ETS$", strETS);
	Replace(strRead, "$ONID10$", strONID10);
	Replace(strRead, "$TSID10$", strTSID10);
	Replace(strRead, "$SID10$", strSID10);
	Replace(strRead, "$EID10$", strEID10);
	Replace(strRead, "$ONID16$", strONID16);
	Replace(strRead, "$TSID16$", strTSID16);
	Replace(strRead, "$SID16$", strSID16);
	Replace(strRead, "$EID16$", strEID16);
	Replace(strRead, "$ServiceName$", strServiceName);
	Replace(strRead, "$SDYYYY28$", strSDYYYY28);
	Replace(strRead, "$SDYY28$", strSDYY28);
	Replace(strRead, "$SDMM28$", strSDMM28);
	Replace(strRead, "$SDM28$", strSDM28);
	Replace(strRead, "$SDDD28$", strSDDD28);
	Replace(strRead, "$SDD28$", strSDD28);
	Replace(strRead, "$SDW28$", strSDW28);
	Replace(strRead, "$STHH28$", strSTHH28);
	Replace(strRead, "$STH28$", strSTH28);
	Replace(strRead, "$EDYYYY28$", strEDYYYY28);
	Replace(strRead, "$EDYY28$", strEDYY28);
	Replace(strRead, "$EDMM28$", strEDMM28);
	Replace(strRead, "$EDM28$", strEDM28);
	Replace(strRead, "$EDDD28$", strEDDD28);
	Replace(strRead, "$EDD28$", strEDD28);
	Replace(strRead, "$EDW28$", strEDW28);
	Replace(strRead, "$ETHH28$", strETHH28);
	Replace(strRead, "$ETH28$", strETH28);
	Replace(strRead, "$DUHH$", strDUHH);
	Replace(strRead, "$DUH$", strDUH);
	Replace(strRead, "$DUMM$", strDUMM);
	Replace(strRead, "$DUM$", strDUM);
	Replace(strRead, "$DUSS$", strDUSS);
	Replace(strRead, "$DUS$", strDUS);
	Replace(strRead, "$Title2$", strTitle2);
	Replace(strRead, "$Drops$", strDrops);
	Replace(strRead, "$Scrambles$", strScrambles);
	Replace(strRead, "$Result$", strResult);
	Replace(strRead, "$TitleF$", strTitleF);
	Replace(strRead, "$Title2F$", strTitle2F);
	Replace(strRead, "$AddKey$", strAddKey);

	DWORD dwWrite=0;
	WriteFile(hWrite, strRead.c_str(), (DWORD)strRead.length(), &dwWrite, NULL );
	CloseHandle(hWrite);

	return TRUE;
}

