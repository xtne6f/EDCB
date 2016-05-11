#include "StdAfx.h"
#include "BatManager.h"

#include "../../Common/SendCtrlCmd.h"
#include "../../Common/StringUtil.h"
#include "../../Common/PathUtil.h"
#include "../../Common/BlockLock.h"

#include <process.h>

CBatManager::CBatManager(CNotifyManager& notifyManager_, LPCWSTR tmpBatFileName)
	: notifyManager(notifyManager_)
{
	InitializeCriticalSection(&this->managerLock);

	GetModuleFolderPath(this->tmpBatFilePath);
	this->tmpBatFilePath += L'\\';
	this->tmpBatFilePath += tmpBatFileName;
	this->idleMargin = MAXDWORD;
	this->nextBatMargin = 0;
	this->batWorkExitingFlag = FALSE;

	this->batWorkThread = NULL;
	this->batWorkStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
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
				}
			}

			if( work.batFilePath.size() > 0 ){
				wstring batFilePath = sys->tmpBatFilePath;
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
					if( exDirect.empty() && sys->notifyManager.IsGUI() == FALSE ){
						//表示できないのでGUI経由で起動してみる
						CSendCtrlCmd ctrlCmd;
						vector<DWORD> registGUI = sys->notifyManager.GetRegistGUI();
						for( size_t i = 0; i < registGUI.size(); i++ ){
							ctrlCmd.SetPipeSetting(CMD2_GUI_CTRL_WAIT_CONNECT, CMD2_GUI_CTRL_PIPE, registGUI[i]);
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
	for( size_t i = 0; i < info.macroList.size(); i++ ){
		if( var == info.macroList[i].first ){
			strWrite += info.macroList[i].second;
			return TRUE;
		}
	}
	return FALSE;
}

wstring CBatManager::CreateEnvironment(const BAT_WORK_INFO& info)
{
	wstring strEnv;
	LPWCH env = GetEnvironmentStrings();
	if( env ){
		do{
			wstring str(env + strEnv.size());
			string strVar;
			WtoA(str.substr(0, str.find(L'=')), strVar);
			//競合する変数をエスケープ
			for( size_t i = 0; i < info.macroList.size(); i++ ){
				if( CompareNoCase(info.macroList[i].first, strVar) == 0 && strVar.empty() == false ){
					str[0] = L'_';
					break;
				}
			}
			strEnv += str + L'\0';
		}while( env[strEnv.size()] != L'\0' );
		FreeEnvironmentStrings(env);
	}
	for( size_t i = 0; i < info.macroList.size(); i++ ){
		wstring strVar;
		AtoW(info.macroList[i].first, strVar);
		strEnv += strVar + L'=' + info.macroList[i].second;
		strEnv += L'\0';
	}
	return strEnv;
}
