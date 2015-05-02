#include "StdAfx.h"
#include "BatManager.h"

#include "../../Common/SendCtrlCmd.h"
#include "../../Common/StringUtil.h"
#include "../../Common/PathUtil.h"
#include "../../Common/TimeUtil.h"
#include "../../Common/BlockLock.h"

#include <process.h>

CBatManager::CBatManager(CNotifyManager& notifyManager_)
	: notifyManager(notifyManager_)
{
	InitializeCriticalSection(&this->managerLock);

	this->idleMargin = MAXDWORD;
	this->nextBatMargin = 0;
	this->batWorkExitingFlag = FALSE;

	this->batWorkThread = NULL;
	this->batWorkStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	this->lastSuspendMode = 0xFF;
	this->lastRebootFlag = 0xFF;
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

void CBatManager::AddBatWork(const BAT_WORK_INFO& info)
{
	CBlockLock lock(&this->managerLock);

	this->workList.push_back(info);
	StartWork();
}

void CBatManager::SetIdleMargin(DWORD marginSec)
{
	CBlockLock lock(&this->managerLock);

	this->idleMargin = marginSec;
	StartWork();
}

DWORD CBatManager::GetWorkCount() const
{
	CBlockLock lock(&this->managerLock);

	return (DWORD)this->workList.size();
}

BOOL CBatManager::IsWorking() const
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
	if( this->batWorkThread == NULL && this->workList.empty() == false && this->idleMargin >= this->nextBatMargin ){
		ResetEvent(this->batWorkStopEvent);
		this->batWorkExitingFlag = FALSE;
		this->batWorkThread = (HANDLE)_beginthreadex(NULL, 0, BatWorkThread, this, 0, NULL);
	}
}

BOOL CBatManager::PopLastWorkSuspend(BYTE* suspendMode, BYTE* rebootFlag)
{
	CBlockLock lock(&this->managerLock);

	BOOL ret = FALSE;
	if( IsWorking() == FALSE && this->lastSuspendMode != 0xFF ){
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

	while(1){
		{
			BAT_WORK_INFO work;
			{
				CBlockLock lock(&sys->managerLock);
				if( sys->workList.empty() ){
					//このフラグを立てたあとは二度とロックを確保してはいけない
					sys->batWorkExitingFlag = TRUE;
					sys->nextBatMargin = 0;
					break;
				}else{
					work = sys->workList[0];
					sys->lastSuspendMode = work.suspendMode;
					sys->lastRebootFlag = work.rebootFlag;
				}
			}

			if( work.batFilePath.size() > 0 ){
				wstring batFilePath = L"";
				GetModuleFolderPath(batFilePath);
				batFilePath += L"\\EpgTimer_Bon_RecEnd.bat";
				DWORD exBatMargin;
				WORD exSW;
				wstring exDirect;
				if( CreateBatFile(work, work.batFilePath.c_str(), batFilePath.c_str(), exBatMargin, exSW, exDirect) != FALSE ){
					{
						CBlockLock(&sys->managerLock);
						if( sys->idleMargin < exBatMargin ){
							//アイドル時間に余裕がないので中止
							sys->batWorkExitingFlag = TRUE;
							sys->nextBatMargin = exBatMargin;
							break;
						}
					}
					bool executed = false;
					HANDLE hProcess = NULL;
					if( exDirect.empty() && GetShellWindow() == NULL ){
						OutputDebugString(L"GetShellWindow() failed\r\n");
						//表示できない可能性が高いのでGUI経由で起動してみる
						CSendCtrlCmd ctrlCmd;
						map<DWORD, DWORD> registGUIMap;
						sys->notifyManager.GetRegistGUI(&registGUIMap);
						for( map<DWORD, DWORD>::iterator itr = registGUIMap.begin(); itr != registGUIMap.end(); itr++ ){
							ctrlCmd.SetPipeSetting(CMD2_GUI_CTRL_WAIT_CONNECT, CMD2_GUI_CTRL_PIPE, itr->first);
							DWORD pid;
							if( ctrlCmd.SendGUIExecute(L'"' + batFilePath + L'"', &pid) == CMD_SUCCESS ){
								//ハンドル開く前に終了するかもしれない
								executed = true;
								hProcess = OpenProcess(SYNCHRONIZE | PROCESS_SET_INFORMATION, FALSE, pid);
								if( hProcess ){
									SetPriorityClass(hProcess, BELOW_NORMAL_PRIORITY_CLASS);
								}
								break;
							}
						}
					}
					if( executed == false ){
						PROCESS_INFORMATION pi;
						STARTUPINFO si = {};
						si.cb = sizeof(si);
						si.dwFlags = STARTF_USESHOWWINDOW;
						si.wShowWindow = exSW;
						wstring batFolder;
						if( exDirect.empty() == false ){
							batFilePath = work.batFilePath;
							GetFileFolder(batFilePath, batFolder);
						}
						wstring strParam = L" /c \"\"" + batFilePath + L"\" \"";
						vector<WCHAR> strBuff(strParam.c_str(), strParam.c_str() + strParam.size() + 1);
						WCHAR cmdExePath[MAX_PATH];
						DWORD dwRet = GetEnvironmentVariable(L"ComSpec", cmdExePath, MAX_PATH);
						if( dwRet && dwRet < MAX_PATH &&
						    CreateProcess(cmdExePath, &strBuff.front(), NULL, NULL, FALSE,
						                  BELOW_NORMAL_PRIORITY_CLASS | (exDirect.empty() ? 0 : CREATE_UNICODE_ENVIRONMENT),
						                  exDirect.empty() ? NULL : const_cast<LPWSTR>(exDirect.c_str()),
						                  exDirect.empty() ? NULL : batFolder.c_str(), &si, &pi) != FALSE ){
							CloseHandle(pi.hThread);
							hProcess = pi.hProcess;
						}
					}
					if( hProcess ){
						//終了監視
						HANDLE hEvents[2] = { sys->batWorkStopEvent, hProcess };
						DWORD dwRet = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
						CloseHandle(hProcess);
						if( dwRet == WAIT_OBJECT_0 ){
							//中止
							break;
						}
					}
				}else{
					_OutputDebugString(L"BATファイル作成エラー：%s\r\n", work.batFilePath.c_str());
				}
			}

			CBlockLock lock(&sys->managerLock);
			sys->workList.erase(sys->workList.begin());
		}
	}

	return 0;
}

BOOL CBatManager::CreateBatFile(const BAT_WORK_INFO& info, LPCWSTR batSrcFilePath, LPCWSTR batFilePath, DWORD& exBatMargin, WORD& exSW, wstring& exDirect)
{
	//バッチの作成
	HANDLE hRead = CreateFile( batSrcFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if( hRead == INVALID_HANDLE_VALUE ){
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
		return FALSE;
	}
	buff[dwL] = '\0';
	CloseHandle(hRead);

	string strRead = "";
	strRead = &buff.front();

	//拡張命令: BatMargin
	exBatMargin = 0;
	if( strRead.find("_EDCBX_BATMARGIN_=") != string::npos ){
		exBatMargin = atoi(strRead.c_str() + strRead.find("_EDCBX_BATMARGIN_=") + 18) * 60;
	}
	//拡張命令: ウィンドウ表示状態
	exSW = strRead.find("_EDCBX_HIDE_") != string::npos ? SW_HIDE :
	       strRead.find("_EDCBX_NORMAL_") != string::npos ? SW_SHOWNORMAL : SW_SHOWMINNOACTIVE;
	//拡張命令: 環境渡しによる直接実行
	exDirect = L"";
	if( strRead.find("_EDCBX_DIRECT_") != string::npos ){
		exDirect = CreateEnvironment(info);
		return TRUE;
	}

	HANDLE hWrite = CreateFile( batFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if( hWrite == INVALID_HANDLE_VALUE ){
		return FALSE;
	}

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
		wstring strValW;
		if( ExpandMacro(strRead.substr(pos + 1, next - pos - 1), info, strValW) == FALSE ){
			strWrite += '$';
			pos++;
		}else{
			string strValA;
			WtoA(strValW, strValA);
			strWrite += strValA;
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

BOOL CBatManager::ExpandMacro(const string& var, const BAT_WORK_INFO& info, wstring& strWrite)
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
	if( var == "FilePath" )		strWrite += info.recFileInfo.recFilePath;
	else if( var == "Title" )	strWrite += info.recFileInfo.title;
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
	else if( var == "ServiceName" )	strWrite += info.recFileInfo.serviceName;
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
	else if( var == "Result" )	strWrite += info.recFileInfo.comment;
	else if( var == "FolderPath" ){
		wstring strFolder;
		GetFileFolder(info.recFileInfo.recFilePath, strFolder);
		ChkFolderPath(strFolder);
		strWrite += strFolder;
	}else if( var == "FileName" ){
		wstring strTitle;
		GetFileTitle(info.recFileInfo.recFilePath, strTitle);
		strWrite += strTitle;
	}else if( var == "TitleF" ){
		wstring strTemp = info.recFileInfo.title;
		CheckFileName(strTemp);
		strWrite += strTemp;
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
		strWrite += strTemp;
	}else if( var == "AddKey" ){
		strWrite += info.addKey;
	}else{
		return FALSE;
	}

	wstring retW;
	AtoW(ret, retW);
	strWrite += retW;

	return TRUE;
}

wstring CBatManager::CreateEnvironment(const BAT_WORK_INFO& info)
{
	static const LPCSTR VAR_ARRAY[] = {
		"FilePath",
		"Title",
		"SDYYYY", "SDYY", "SDMM", "SDM", "SDDD", "SDD", "SDW", "STHH", "STH", "STMM", "STM", "STSS", "STS",
		"EDYYYY", "EDYY", "EDMM", "EDM", "EDDD", "EDD", "EDW", "ETHH", "ETH", "ETMM", "ETM", "ETSS", "ETS",
		"ONID10", "TSID10", "SID10", "EID10",
		"ONID16", "TSID16", "SID16", "EID16",
		"ServiceName",
		"SDYYYY28", "SDYY28", "SDMM28", "SDM28", "SDDD28", "SDD28", "SDW28", "STHH28", "STH28",
		"EDYYYY28", "EDYY28", "EDMM28", "EDM28", "EDDD28", "EDD28", "EDW28", "ETHH28", "ETH28",
		"DUHH", "DUH", "DUMM", "DUM", "DUSS", "DUS",
		"Drops",
		"Scrambles",
		"Result",
		"FolderPath",
		"FileName",
		"TitleF",
		"Title2",
		"Title2F",
		"AddKey",
		NULL,
	};

	wstring strEnv;
	LPWCH env = GetEnvironmentStrings();
	if( env ){
		do{
			wstring str(env + strEnv.size());
			string strVar;
			WtoA(str.substr(0, str.find(L'=')), strVar);
			//競合する変数をエスケープ
			for( int i = 0; VAR_ARRAY[i]; i++ ){
				if( _stricmp(VAR_ARRAY[i], strVar.c_str()) == 0 ){
					str[0] = L'_';
					break;
				}
			}
			strEnv += str + L'\0';
		}while( env[strEnv.size()] != L'\0' );
		FreeEnvironmentStrings(env);
	}
	for( int i = 0; VAR_ARRAY[i]; i++ ){
		wstring strVar;
		AtoW(VAR_ARRAY[i], strVar);
		strEnv += strVar + L'=';
		ExpandMacro(VAR_ARRAY[i], info, strEnv);
		strEnv += L'\0';
	}
	return strEnv;
}
