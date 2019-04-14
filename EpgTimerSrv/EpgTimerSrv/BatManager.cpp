#include "stdafx.h"
#include "BatManager.h"

#include "../../Common/SendCtrlCmd.h"
#include "../../Common/StringUtil.h"
#include "../../Common/PathUtil.h"

CBatManager::CBatManager(CNotifyManager& notifyManager_, LPCWSTR tmpBatFileName)
	: notifyManager(notifyManager_)
{
	this->tmpBatFilePath = GetModulePath().replace_filename(tmpBatFileName).native();
	this->idleMargin = MAXDWORD;
	this->nextBatMargin = 0;
}

CBatManager::~CBatManager()
{
	Finalize();
}

void CBatManager::Finalize()
{
	if( this->batWorkThread.joinable() ){
		this->batWorkStopFlag = true;
		this->batWorkEvent.Set();
		this->batWorkThread.join();
	}
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

void CBatManager::SetCustomHandler(LPCWSTR ext, const std::function<void(BAT_WORK_INFO&, vector<char>&)>& handler)
{
	CBlockLock lock(&this->managerLock);

	this->customHandler = handler;
	this->customExt = ext;
}

bool CBatManager::IsWorking() const
{
	CBlockLock lock(&this->managerLock);

	return this->workList.empty() == false;
}

bool CBatManager::IsWorkingWithoutNotification() const
{
	CBlockLock lock(&this->managerLock);

	return std::find_if(this->workList.begin(), this->workList.end(),
	                    [](const BAT_WORK_INFO& a) { return a.macroList.empty() || a.macroList[0].first != "NotifyID"; }) != this->workList.end();
}

void CBatManager::StartWork()
{
	CBlockLock lock(&this->managerLock);

	//ワーカスレッドが終了しようとしているときはその完了を待つ
	if( this->batWorkThread.joinable() && this->batWorkExitingFlag ){
		this->batWorkThread.join();
	}
	if( this->workList.empty() == false && this->idleMargin >= this->nextBatMargin ){
		if( this->batWorkThread.joinable() ){
			this->batWorkEvent.Set();
		}else{
			this->batWorkStopFlag = false;
			this->batWorkExitingFlag = false;
			this->batWorkThread = thread_(BatWorkThread, this);
		}
	}
}

void CBatManager::BatWorkThread(CBatManager* sys)
{
	DWORD notifyWorkWait = 0;
	BAT_WORK_INFO notifyWork;
	for(;;){
		while( notifyWorkWait && sys->batWorkStopFlag == false && sys->IsWorking() == false ){
			DWORD tick = GetTickCount();
			WaitForSingleObject(sys->batWorkEvent.Handle(), notifyWorkWait);
			DWORD diff = GetTickCount() - tick;
			notifyWorkWait -= min(diff, notifyWorkWait);
			if( notifyWorkWait == 0 ){
				CBlockLock lock(&sys->managerLock);
				sys->workList.push_back(std::move(notifyWork));
			}
		}
		if( sys->batWorkStopFlag ){
			//中止
			break;
		}

		BAT_WORK_INFO work;
		fs_path batFilePath;
		std::function<void(BAT_WORK_INFO&, vector<char>&)> customHandler_;
		{
			CBlockLock lock(&sys->managerLock);
			if( sys->workList.empty() ){
				//このフラグを立てたあとは二度とロックを確保してはいけない
				sys->batWorkExitingFlag = true;
				sys->nextBatMargin = 0;
				break;
			}
			work = sys->workList[0];
			batFilePath = work.batFilePath;
			if( !IsExt(batFilePath, L".bat") && !IsExt(batFilePath, L".ps1") ){
				if( sys->customHandler && IsExt(batFilePath, sys->customExt.c_str()) ){
					customHandler_ = sys->customHandler;
				}else{
					batFilePath.clear();
				}
			}
		}

		if( batFilePath.empty() == false ){
			DWORD exBatMargin;
			DWORD exNotifyInterval;
			WORD exSW;
			wstring exDirect;
			vector<char> buff;
			if( sys->CreateBatFile(work, exBatMargin, exNotifyInterval, exSW, exDirect, buff) ){
				{
					CBlockLock lock(&sys->managerLock);
					if( sys->idleMargin < exBatMargin ){
						//アイドル時間に余裕がないので中止
						sys->batWorkExitingFlag = true;
						sys->nextBatMargin = exBatMargin;
						break;
					}
					//NotifyInterval拡張: macroList[0]が"NotifyID"のとき、この値を"0"にして指定時間後に再発行する。idleMarginと併用不可
					if( exNotifyInterval && notifyWorkWait == 0 &&
					    sys->workList[0].macroList.empty() == false && sys->workList[0].macroList[0].first == "NotifyID" ){
						notifyWorkWait = exNotifyInterval;
						notifyWork = sys->workList[0];
						notifyWork.macroList[0].second = L"0";
					}
				}
				if( buff.empty() == false ){
					if( customHandler_ ){
						customHandler_(work, buff);
					}
				}else{
					bool executed = false;
					HANDLE hProcess = NULL;
					if( exDirect.empty() && sys->notifyManager.IsGUI() == false ){
						//表示できないのでGUI経由で起動してみる
						CSendCtrlCmd ctrlCmd;
						vector<DWORD> registGUI = sys->notifyManager.GetRegistGUI();
						for( size_t i = 0; i < registGUI.size(); i++ ){
							ctrlCmd.SetPipeSetting(CMD2_GUI_CTRL_WAIT_CONNECT, CMD2_GUI_CTRL_PIPE, registGUI[i]);
							DWORD pid;
							if( ctrlCmd.SendGUIExecute(L'"' + sys->tmpBatFilePath + L'"', &pid) == CMD_SUCCESS ){
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
						fs_path batFolder;
						if( exDirect.empty() ){
							batFilePath = sys->tmpBatFilePath;
						}else{
							batFolder = batFilePath.parent_path();
						}
						fs_path exePath;
						wstring strParam;
						if( IsExt(batFilePath, L".ps1") ){
							//PowerShell
							strParam = L" -NoProfile -ExecutionPolicy RemoteSigned -File \"" + batFilePath.native() + L"\"";
							WCHAR szSystemRoot[MAX_PATH];
							DWORD dwRet = GetEnvironmentVariable(L"SystemRoot", szSystemRoot, MAX_PATH);
							if( dwRet && dwRet < MAX_PATH ){
								exePath = szSystemRoot;
								exePath.append(L"System32\\WindowsPowerShell\\v1.0\\powershell.exe");
							}
						}else{
							//コマンドプロンプト
							strParam = L" /c \"\"" + batFilePath.native() + L"\" \"";
							WCHAR szComSpec[MAX_PATH];
							DWORD dwRet = GetEnvironmentVariable(L"ComSpec", szComSpec, MAX_PATH);
							if( dwRet && dwRet < MAX_PATH ){
								exePath = szComSpec;
							}
						}
						vector<WCHAR> strBuff(strParam.c_str(), strParam.c_str() + strParam.size() + 1);
						if( exePath.empty() == false &&
						    CreateProcess(exePath.c_str(), strBuff.data(), NULL, NULL, FALSE,
						                  BELOW_NORMAL_PRIORITY_CLASS | (exDirect.empty() ? 0 : CREATE_UNICODE_ENVIRONMENT),
						                  exDirect.empty() ? NULL : const_cast<LPWSTR>(exDirect.c_str()),
						                  exDirect.empty() ? NULL : batFolder.c_str(), &si, &pi) != FALSE ){
							CloseHandle(pi.hThread);
							hProcess = pi.hProcess;
						}else{
							_OutputDebugString(L"BAT起動エラー：%s\r\n", batFilePath.c_str());
						}
					}
					if( hProcess ){
						//終了監視
						HANDLE hEvents[2] = { sys->batWorkEvent.Handle(), hProcess };
						while( WaitForMultipleObjects(2, hEvents, FALSE, INFINITE) == WAIT_OBJECT_0 && sys->batWorkStopFlag == false );
						CloseHandle(hProcess);
					}
				}
			}else{
				_OutputDebugString(L"BATファイル作成エラー：%s\r\n", work.batFilePath.c_str());
			}
		}else{
			_OutputDebugString(L"BAT拡張子エラー：%s\r\n", work.batFilePath.c_str());
		}

		CBlockLock lock(&sys->managerLock);
		sys->workList.erase(sys->workList.begin());
	}
}

bool CBatManager::CreateBatFile(BAT_WORK_INFO& info, DWORD& exBatMargin, DWORD& exNotifyInterval, WORD& exSW, wstring& exDirect, vector<char>& buff) const
{
	//バッチの作成
	std::unique_ptr<FILE, decltype(&fclose)> fp(secure_wfopen(info.batFilePath.c_str(), L"rbN"), fclose);
	if( !fp ){
		return false;
	}

	//拡張命令: BatMargin
	exBatMargin = 0;
	//拡張命令: NotifyInterval
	exNotifyInterval = 0;
	//拡張命令: ウィンドウ表示状態
	exSW = SW_SHOWMINNOACTIVE;
	//拡張命令: 環境渡しによる直接実行
	exDirect = L"";
	bool exDirectFlag = false;
	//拡張命令: 日時についての変数をISO形式にする
	bool exFormatTime = false;
	//カスタムハンドラ用ファイルの中身
	buff.clear();

	fs_path batFilePath = info.batFilePath;
	if( !IsExt(batFilePath, L".bat") ){
		exDirectFlag = true;
		exFormatTime = true;
	}
	__int64 fileSize = 0;
	char olbuff[257];
	for( size_t n = fread(olbuff, 1, 256, fp.get()); ; n = fread(olbuff + 64, 1, 192, fp.get()) + 64 ){
		olbuff[n] = '\0';
		if( strstr(olbuff, "_EDCBX_BATMARGIN_=") ){
			//一時的に断片を格納するかもしれないが最終的に正しければよい
			exBatMargin = strtoul(strstr(olbuff, "_EDCBX_BATMARGIN_=") + 18, NULL, 10) * 60;
		}
		if( strstr(olbuff, "_EDCBX_NOTIFY_INTERVAL_=") ){
			exNotifyInterval = strtoul(strstr(olbuff, "_EDCBX_NOTIFY_INTERVAL_=") + 24, NULL, 10) * 1000;
		}
		if( strstr(olbuff, "_EDCBX_HIDE_") ){
			exSW = SW_HIDE;
		}
		if( strstr(olbuff, "_EDCBX_NORMAL_") ){
			exSW = SW_SHOWNORMAL;
		}
		exDirectFlag = exDirectFlag || strstr(olbuff, "_EDCBX_DIRECT_");
		exFormatTime = exFormatTime || strstr(olbuff, "_EDCBX_FORMATTIME_");
		fileSize += (fileSize == 0 ? n : n - 64);
		if( n < 256 ){
			break;
		}
		std::copy(olbuff + 192, olbuff + 256, olbuff);
	}
	if( exFormatTime ){
		//#でコメントアウトされているものを消す
		info.macroList.erase(std::remove_if(info.macroList.begin(), info.macroList.end(),
			[](const pair<string, wstring>& a) { return a.first.compare(0, 1, "#") == 0; }), info.macroList.end());
	}else{
		info.macroList.erase(std::remove_if(info.macroList.begin(), info.macroList.end(),
			[](const pair<string, wstring>& a) { return a.first.compare(0, 9, "StartTime") == 0 || a.first.compare(0, 14, "DurationSecond") == 0; }),
			info.macroList.end());
		//コメントアウトを解除する
		for( size_t i = 0; i < info.macroList.size(); i++ ){
			if( info.macroList[i].first.compare(0, 1, "#") == 0 ){
				info.macroList[i].first.erase(0, 1);
			}
		}
	}
	for( size_t i = 0; i < info.macroList.size(); i++ ){
		for( size_t j = 0; j < info.macroList[i].second.size(); j++ ){
			//制御文字とダブルクォートは置き換える
			if( (L'\x1' <= info.macroList[i].second[j] && info.macroList[i].second[j] <= L'\x1f') || info.macroList[i].second[j] == L'\x7f' ){
				info.macroList[i].second[j] = L'〓';
			}else if( info.macroList[i].second[j] == L'"' ){
				info.macroList[i].second[j] = L'”';
			}
		}
	}
	if( exDirectFlag && (IsExt(batFilePath, L".bat") || IsExt(batFilePath, L".ps1")) ){
		exDirect = CreateEnvironment(info);
		return exDirect.empty() == false;
	}

	if( fileSize >= 64 * 1024 * 1024 ){
		return false;
	}
	buff.resize((size_t)fileSize + 1, '\0');
	rewind(fp.get());
	if( fread(buff.data(), 1, buff.size() - 1, fp.get()) != buff.size() - 1 ){
		return false;
	}
	if( exDirectFlag ){
		//カスタムハンドラ
		return true;
	}
	string strRead = buff.data();
	buff.clear();

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
		if( ExpandMacro(strRead.substr(pos + 1, next - pos - 1), info, strValW) == false ){
			strWrite += '$';
			pos++;
		}else{
			string strValA;
			WtoA(strValW, strValA);
			strWrite += strValA;
			pos = next + 1;
		}
	}

	fp.reset(secure_wfopen(this->tmpBatFilePath.c_str(), L"wbN"));
	if( !fp || fputs(strWrite.c_str(), fp.get()) < 0 || fflush(fp.get()) != 0 ){
		return false;
	}

	return true;
}

bool CBatManager::ExpandMacro(const string& var, const BAT_WORK_INFO& info, wstring& strWrite)
{
	for( size_t i = 0; i < info.macroList.size(); i++ ){
		if( var == info.macroList[i].first ){
			strWrite += info.macroList[i].second;
			return true;
		}
	}
	return false;
}

wstring CBatManager::CreateEnvironment(const BAT_WORK_INFO& info)
{
	wstring strEnv;
	LPWCH env = GetEnvironmentStrings();
	if( env ){
		do{
			wstring str(env + strEnv.size());
			wstring strVar(str, 0, str.find(L'='));
			wstring strMacroVar;
			//競合する変数をエスケープ
			for( size_t i = 0; i < info.macroList.size(); i++ ){
				UTF8toW(info.macroList[i].first, strMacroVar);
				if( CompareNoCase(strMacroVar, strVar) == 0 && strVar.empty() == false ){
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
		UTF8toW(info.macroList[i].first, strVar);
		strEnv += strVar + L'=' + info.macroList[i].second;
		strEnv += L'\0';
	}
	return strEnv;
}
