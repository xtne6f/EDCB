#include "StdAfx.h"
#include "TunerCtrl.h"


#include <tlhelp32.h> 
#include <shlwapi.h>
#include <algorithm>
#include <locale>

CTunerCtrl::CTunerCtrl(void)
{
	OSVERSIONINFO VerInfo;
	VerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx( &VerInfo );

	if( VerInfo.dwMajorVersion == 5 && VerInfo.dwMinorVersion == 0 ){
		win2kFlag = TRUE;
	}else{
		win2kFlag = FALSE;
	}

	this->openWaitEvent = _CreateEvent(FALSE, TRUE, _T("Global\\EpgTimerSrv_OpenTuner_Event") );
}

CTunerCtrl::~CTunerCtrl(void)
{
	if( this->openWaitEvent != NULL ){
		CloseHandle(this->openWaitEvent);
		this->openWaitEvent = NULL;
	}
}

void CTunerCtrl::SetExePath(wstring exePath)
{
	this->exePath = exePath;
	GetFileName(this->exePath, this->exeName);
}

void CTunerCtrl::GetOption()
{
	wstring strIni;
	GetModuleFolderPath(strIni);

	strIni+=L"\\ViewApp.ini";

	WCHAR buff[512]=L"";
	GetPrivateProfileString( L"APP_CMD_OPT", L"Bon", L"-d", buff, 512, strIni.c_str() );
	this->optBon = buff;
	ZeroMemory(buff, sizeof(WCHAR)*512);
	GetPrivateProfileString( L"APP_CMD_OPT", L"Min", L"-min", buff, 512, strIni.c_str() );
	this->optMin = buff;
	ZeroMemory(buff, sizeof(WCHAR)*512);
	GetPrivateProfileString( L"APP_CMD_OPT", L"ViewOff", L"-noview", buff, 512, strIni.c_str() );
	this->optNoView = buff;
	ZeroMemory(buff, sizeof(WCHAR)*512);
	GetPrivateProfileString( L"APP_CMD_OPT", L"NetworkOff", L"-nonw", buff, 512, strIni.c_str() );
	this->optNoNW = buff;
}

BOOL CTunerCtrl::OpenExe(wstring bonDriver, DWORD id, BOOL minWake, BOOL noView, BOOL noNW, map<DWORD, DWORD> registGUIMap, DWORD* PID, BOOL UDP, BOOL TCP, DWORD Priority )
{
	GetOption();
	if( this->exePath.empty() == true ){
		SetEvent(this->openWaitEvent);
		return FALSE;
	}

	WaitForSingleObject(this->openWaitEvent, INFINITE);

	wstring strExecute;
	Format(strExecute, L"\"%s\" %s %s", this->exePath.c_str(), this->optBon.c_str(), bonDriver.c_str());

	if( minWake == TRUE ){
		strExecute += L" ";
		strExecute += this->optMin;
	}
	if( noView == TRUE ){
		strExecute += L" ";
		strExecute += this->optNoView;
	}
	if( noNW == TRUE ){
		strExecute += L" ";
		strExecute += this->optNoNW;
	}else{
		if( UDP == TRUE ){
			strExecute += L" -nwudp";
		}
		if( TCP == TRUE ){
			strExecute += L" -nwtcp";
		}
	}

	BOOL send = FALSE;
	map<DWORD, DWORD>::iterator itr;
	for( itr = registGUIMap.begin(); itr != registGUIMap.end(); itr++ ){
		wstring pipeName = L"";
		wstring waitEventName = L"";

		Format(pipeName, L"%s%d", CMD2_GUI_CTRL_PIPE, itr->first );
		Format(waitEventName, L"%s%d", CMD2_GUI_CTRL_WAIT_CONNECT, itr->first );

		cmdSend.SetPipeSetting(waitEventName, pipeName);
		cmdSend.SetConnectTimeOut(20*1000);

		if( cmdSend.SendGUIExecute(strExecute, PID) == CMD_SUCCESS ){
			send = TRUE;
			break;
		}
	}

	if( send == FALSE ){
		OutputDebugString(L"gui exec err");
		//GUI経由で起動できなかった
		PROCESS_INFORMATION pi;
		STARTUPINFO si;
		ZeroMemory(&si,sizeof(si));
		si.cb=sizeof(si);

		BOOL bRet = CreateProcess( NULL, (WCHAR*)strExecute.c_str(), NULL, NULL, FALSE, GetPriorityClass(GetCurrentProcess()), NULL, NULL, &si, &pi );
		if( bRet == FALSE ){
			SetEvent(this->openWaitEvent);
			return FALSE;
		}
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);

		*PID = pi.dwProcessId;
	}

	//IDのセット
	wstring pipeName = L"";
	wstring waitEventName = L"";

	Format(pipeName, L"%s%d", CMD2_VIEW_CTRL_PIPE, *PID );
	Format(waitEventName, L"%s%d", CMD2_VIEW_CTRL_WAIT_CONNECT, *PID );
	cmdSend.SetPipeSetting(waitEventName, pipeName);

	BOOL bRet = FALSE;
	for( int i=0; i<6; i++ ){
		Sleep(1000);
		if(cmdSend.SendViewSetID(id) == CMD_SUCCESS){
			bRet = TRUE;
			break;
		}
	}
	if( bRet == FALSE ){
		CloseExe(*PID);
	}else{
		//起動ステータスを確認
		DWORD status = 0;
		if(cmdSend.SendViewGetStatus(&status) == CMD_SUCCESS){
			if( status == VIEW_APP_ST_ERR_BON ){
				CloseExe(*PID);
				bRet = FALSE;
			}else{
				HANDLE hProcess = OpenProcess(PROCESS_SET_INFORMATION, FALSE, *PID);
				if( hProcess != NULL ){
					switch(Priority){
					case 0:
						SetPriorityClass(hProcess, REALTIME_PRIORITY_CLASS);
						break;
					case 1:
						SetPriorityClass(hProcess, HIGH_PRIORITY_CLASS);
						break;
					case 2:
						SetPriorityClass(hProcess, ABOVE_NORMAL_PRIORITY_CLASS);
						break;
					case 3:
						SetPriorityClass(hProcess, NORMAL_PRIORITY_CLASS);
						break;
					case 4:
						SetPriorityClass(hProcess, BELOW_NORMAL_PRIORITY_CLASS);
						break;
					case 5:
						SetPriorityClass(hProcess, IDLE_PRIORITY_CLASS);
						break;
					default:
						break;
					}
					CloseHandle(hProcess);
				}
			}
		}
	}

	SetEvent(this->openWaitEvent);
	return bRet;
}

void CTunerCtrl::CloseExe(DWORD PID)
{
	wstring pipeName = L"";
	wstring waitEventName = L"";

	Format(pipeName, L"%s%d", CMD2_VIEW_CTRL_PIPE, PID );
	Format(waitEventName, L"%s%d", CMD2_VIEW_CTRL_WAIT_CONNECT, PID );

	cmdSend.SetPipeSetting(waitEventName, pipeName);

	BOOL bFind = TRUE;
	DWORD dwCount=0;
	bFind = _FindOpenExeProcess(PID);
	if( bFind == TRUE ){
		cmdSend.SendViewAppClose();
	}
	while(bFind == TRUE && dwCount<60){
		bFind = _FindOpenExeProcess(PID);
		Sleep(500);
		dwCount++;
	}
	if( bFind == TRUE ){
		//ぶち殺す
		HANDLE hProcess =  OpenProcess(PROCESS_TERMINATE, FALSE, PID);
		TerminateProcess(hProcess, (UINT)-1);
		CloseHandle(hProcess);
		Sleep(500);
	}
}

BOOL CALLBACK CTunerCtrl::CloseEnumWindowProc(HWND hWnd, LPARAM lParam)
{
	if( GetWindow(hWnd, GW_OWNER)==NULL){
		DWORD dwProcessId;
		//WindowのプロセスIDとスレッドIDを得る
		DWORD dwThreadId = GetWindowThreadProcessId(hWnd, &dwProcessId);
		if( (DWORD)lParam == dwProcessId ){
			::PostMessage(hWnd,WM_CLOSE,0,0);
			return FALSE;
		}
	}
	return TRUE;
}

BOOL CTunerCtrl::GetOpenExe(vector<DWORD>* IDList)
{
	if( IDList == NULL ){
		return FALSE;
	}
	HANDLE hSnapshot;
	PROCESSENTRY32 procent;

	wstring strExeName = this->exeName;
	std::transform(strExeName.begin(), strExeName.end(), strExeName.begin(), tolower);

	BOOL bFind = FALSE;
	/* Toolhelpスナップショットを作成する */
	hSnapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS,0 );
	if ( hSnapshot != (HANDLE)-1 ) {
		procent.dwSize = sizeof(PROCESSENTRY32);
		if ( Process32First( hSnapshot,&procent ) != FALSE ){
			do {
				wstring strExe = procent.szExeFile;
				std::transform(strExe.begin(), strExe.end(), strExe.begin(), tolower);

				/* procent.szExeFileにプロセス名 */
				if( win2kFlag == TRUE ){
					if( strExe.find( strExeName.substr(0, 15).c_str()) == 0 ){
						IDList->push_back(procent.th32ProcessID);
						bFind = TRUE;
					}
				}else{
					if( strExe.find( strExeName.c_str()) == 0 ){
						IDList->push_back(procent.th32ProcessID);
						bFind = TRUE;
					}
				}
			} while ( Process32Next( hSnapshot,&procent ) != FALSE );
		}
		CloseHandle( hSnapshot );
	}
	return bFind;
}

BOOL CTunerCtrl::GetOpenExe(wstring exeName, vector<DWORD>* IDList)
{
	if( IDList == NULL ){
		return FALSE;
	}
	HANDLE hSnapshot;
	PROCESSENTRY32 procent;

	wstring strExeName = exeName;
	std::transform(strExeName.begin(), strExeName.end(), strExeName.begin(), tolower);

	BOOL bFind = FALSE;
	/* Toolhelpスナップショットを作成する */
	hSnapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS,0 );
	if ( hSnapshot != (HANDLE)-1 ) {
		procent.dwSize = sizeof(PROCESSENTRY32);
		if ( Process32First( hSnapshot,&procent ) != FALSE ){
			do {
				wstring strExe = procent.szExeFile;
				std::transform(strExe.begin(), strExe.end(), strExe.begin(), tolower);

				/* procent.szExeFileにプロセス名 */
				if( win2kFlag == TRUE ){
					if( strExe.find( strExeName.substr(0, 15).c_str()) == 0 ){
						IDList->push_back(procent.th32ProcessID);
						bFind = TRUE;
					}
				}else{
					if( strExe.find( strExeName.c_str()) == 0 ){
						IDList->push_back(procent.th32ProcessID);
						bFind = TRUE;
					}
				}
			} while ( Process32Next( hSnapshot,&procent ) != FALSE );
		}
		CloseHandle( hSnapshot );
	}
	return bFind;
}
