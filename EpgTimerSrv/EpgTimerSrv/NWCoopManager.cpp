#include "StdAfx.h"
#include "NWCoopManager.h"
#include <process.h>

#include "../../Common/StringUtil.h"
#include "../../Common/PathUtil.h"
#include "../../Common/ErrDef.h"

CNWCoopManager::CNWCoopManager(void)
{
	this->lockEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
	this->lockQueue = CreateEvent(NULL, FALSE, TRUE, NULL);

	this->chkThread = NULL;
	this->chkStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	this->chkEpgThread = NULL;
	this->chkEpgStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	this->updateEpgData = FALSE;

	WSAData wsaData;
	WSAStartup(MAKEWORD(2,0), &wsaData);

}

CNWCoopManager::~CNWCoopManager(void)
{
	if( this->chkThread != NULL ){
		::SetEvent(this->chkStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->chkThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->chkThread, 0xffffffff);
		}
		CloseHandle(this->chkThread);
		this->chkThread = NULL;
	}
	if( this->chkStopEvent != NULL ){
		CloseHandle(this->chkStopEvent);
		this->chkStopEvent = NULL;
	}

	if( this->chkEpgThread != NULL ){
		::SetEvent(this->chkEpgStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->chkEpgThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->chkEpgThread, 0xffffffff);
		}
		CloseHandle(this->chkEpgThread);
		this->chkEpgThread = NULL;
	}
	if( this->chkEpgStopEvent != NULL ){
		CloseHandle(this->chkEpgStopEvent);
		this->chkEpgStopEvent = NULL;
	}

	WSACleanup();

	if( this->lockQueue != NULL ){
		QueueUnLock();
		CloseHandle(this->lockQueue);
		this->lockQueue = NULL;
	}

	if( this->lockEvent != NULL ){
		UnLock();
		CloseHandle(this->lockEvent);
		this->lockEvent = NULL;
	}
}

BOOL CNWCoopManager::Lock(LPCWSTR log, DWORD timeOut)
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
			OutputDebugString(L"◆CNWCoopManager::Lock FALSE");
			if( log != NULL ){
				OutputDebugString(log);
			}
		return FALSE;
	}
	return TRUE;
}

void CNWCoopManager::UnLock(LPCWSTR log)
{
	if( this->lockEvent != NULL ){
		SetEvent(this->lockEvent);
	}
	if( log != NULL ){
		OutputDebugString(log);
	}
}

BOOL CNWCoopManager::QueueLock(LPCWSTR log, DWORD timeOut)
{
	if( this->lockQueue == NULL ){
		return FALSE;
	}
	if( log != NULL ){
		OutputDebugString(log);
	}
	DWORD dwRet = WaitForSingleObject(this->lockQueue, timeOut);
	if( dwRet == WAIT_ABANDONED || 
		dwRet == WAIT_FAILED ||
		dwRet == WAIT_TIMEOUT){
			OutputDebugString(L"◆CNotifyManager::NotifyLock FALSE");
		return FALSE;
	}
	return TRUE;
}

void CNWCoopManager::QueueUnLock(LPCWSTR log)
{
	if( this->lockQueue != NULL ){
		SetEvent(this->lockQueue);
	}
	if( log != NULL ){
		OutputDebugString(log);
	}
}

void CNWCoopManager::SetCoopServer(vector<COOP_SERVER_INFO>* infoList)
{
	if( Lock(L"CNWCoopManager::SetCoopServer") == FALSE ) return;

	if( this->chkThread != NULL ){
		::SetEvent(this->chkStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->chkThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->chkThread, 0xffffffff);
		}
		CloseHandle(this->chkThread);
		this->chkThread = NULL;
	}

	srvList.clear();
	for( size_t i=0; i<infoList->size(); i++ ){
		srvList.insert(pair<DWORD,COOP_SERVER_INFO>((DWORD)i, (*infoList)[i]));
	}

	//キューのチェックステータスをリセット
	if( QueueLock() == TRUE ){
		for( size_t i=0; i<chkQueue.size(); i++){
			chkQueue[i]->chkStatus.clear();
			wstring srv = L"";
			WORD status = 0xFFFF;
			chkQueue[i]->reserve->GetCoopAddStatus(srv, &status);
			if( status == 0 || status == 2){
				//追加に失敗してる物は再チェック
				srv = L"";
				status = 0xFFFF;
				chkQueue[i]->reserve->SetCoopAdd(srv, status);
			}
			for(size_t j=0; j<srvList.size(); j++){
				chkQueue[i]->chkStatus.insert(pair<DWORD, WORD>((DWORD)i, 0xFFFF));
			}
		}

		QueueUnLock();
	}

	UnLock();
}

BOOL CNWCoopManager::AddChkReserve(CReserveInfo* data)
{
	if( Lock(L"CNWCoopManager::AddChkReserve") == FALSE ) return FALSE;

	BOOL ret = TRUE;

	if( QueueLock() == TRUE ){
		CHK_RESERVE_INFO* item = new CHK_RESERVE_INFO;
		item->reserve = data;
		for( size_t i=0; i<srvList.size(); i++){
			item->chkStatus.insert(pair<DWORD, WORD>((DWORD)i, 0xFFFF));
		}

		this->chkQueue.push_back(item);
		QueueUnLock();
	}

	UnLock();
	return ret;
}

void CNWCoopManager::StartChkReserve()
{
	if( Lock(L"CNWCoopManager::AddChkReserve") == FALSE ) return ;

	StartChkThread();

	UnLock();
	return ;
}

void CNWCoopManager::StopChkReserve()
{
	if( Lock(L"CNWCoopManager::ResetCheck") == FALSE ) return;

	if( this->chkThread != NULL ){
		::SetEvent(this->chkStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->chkThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->chkThread, 0xffffffff);
		}
		CloseHandle(this->chkThread);
		this->chkThread = NULL;
	}

	UnLock();
}

void CNWCoopManager::ResetResCheck()
{
	if( Lock(L"CNWCoopManager::ResetCheck") == FALSE ) return;

	if( this->chkThread != NULL ){
		::SetEvent(this->chkStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->chkThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->chkThread, 0xffffffff);
		}
		CloseHandle(this->chkThread);
		this->chkThread = NULL;
	}

	if( QueueLock() == TRUE ){
		this->chkQueue.clear();
		QueueUnLock();
	}

	UnLock();
}

void CNWCoopManager::StartChkThread()
{
	if( this->chkThread != NULL ){
		if( ::WaitForSingleObject(this->chkThread, 0) != WAIT_TIMEOUT ){
			//終わっている
			CloseHandle(this->chkThread);
			this->chkThread = NULL;
		}
	}
	if( this->chkThread == NULL ){
		ResetEvent(this->chkStopEvent);
		this->chkThread = (HANDLE)_beginthreadex(NULL, 0, ChkThread, (LPVOID)this, CREATE_SUSPENDED, NULL);
		SetThreadPriority( this->chkThread, THREAD_PRIORITY_NORMAL );
		ResumeThread(this->chkThread);
	}
}

UINT WINAPI CNWCoopManager::ChkThread(LPVOID param)
{
	CNWCoopManager* sys = (CNWCoopManager*)param;
	CSendCtrlCmd sendCtrl;
	map<DWORD,DWORD>::iterator itr;
	DWORD wait = 0;
	while(1){
		if( ::WaitForSingleObject(sys->chkStopEvent, wait) != WAIT_TIMEOUT ){
			//キャンセルされた
			break;
		}
		vector<CHK_RESERVE_INFO*> chkingList;
		map<DWORD, COOP_SERVER_INFO> srvList;

		//現在の情報取得
		if( sys->QueueLock() == FALSE ) return 0;
		srvList = sys->srvList;
		for(size_t i=0; i<sys->chkQueue.size(); i++){
			chkingList.push_back(sys->chkQueue[i]);
		}
		sys->QueueUnLock();
		if( chkingList.size() == 0 || srvList.size() == 0){
			//リストないので終了
			return 0;
		}

		//追加できるかチェック
		for(size_t i=0; i<chkingList.size(); i++){
			wstring srv = L"";
			WORD status = 0xFFFF;
			chkingList[i]->reserve->GetCoopAddStatus(srv, &status);
			if( status != 0xFFFF ){
				continue;
			}
			RESERVE_DATA data;
			chkingList[i]->reserve->GetData(&data);

			map<DWORD, WORD>::iterator itr;
			BOOL ngAdd = TRUE;
			for( itr = chkingList[i]->chkStatus.begin(); itr != chkingList[i]->chkStatus.end(); itr++){
				if( ::WaitForSingleObject(sys->chkStopEvent, 0) != WAIT_TIMEOUT ){
					//キャンセルされた
					return 0;
				}
				if( itr->second == 0xFFFF ){
					//まだ未チェックなのでチェック
					map<DWORD, COOP_SERVER_INFO>::iterator itrSrv;
					itrSrv = srvList.find(itr->first);
					if( itrSrv != srvList.end() ){
						sendCtrl.SetSendMode(TRUE);
						wstring ip;
						if( sys->GetIP2Host(itrSrv->second.hostName, ip) == FALSE){
							continue;
						}
						_OutputDebugString(L"++CNWCoopManager ReserveChk ip %s\r\n", ip.c_str());
						sendCtrl.SetNWSetting(ip, itrSrv->second.srvPort);
						sendCtrl.SetConnectTimeOut(15*1000);

						WORD status = 0;
						DWORD err = sendCtrl.SendAddChkReserve2(&data, &status);
						if( err == CMD_SUCCESS ){
							_OutputDebugString(L"++CNWCoopManager addChk %d", status);
							if( status == 1 || status == 3){
								//問題なく追加可能orすでに追加済み
								itr->second = status;
								chkingList[i]->reserve->SetCoopAdd(itrSrv->second.hostName, status);
								if( status == 1 ){
									vector<RESERVE_DATA> addList;
									data.reserveID = 0;
									data.overlapMode = 0;
									data.reserveStatus = 0;
									data.recSetting.batFilePath = L"";
									data.recSetting.partialRecFolder.clear();
									data.recSetting.priority = 1;
									data.recSetting.recFolderList.clear();
									data.recSetting.rebootFlag = 0;
									data.recSetting.suspendMode = 0;
									data.recSetting.continueRecFlag = 0;
									data.recSetting.tunerID = 0;
									data.comment = L"別サーバーからの予約登録";
									addList.push_back(data);
									sendCtrl.SendAddReserve(&addList);
								}
								ngAdd = FALSE;
								break;
							}else{
								itr->second = status;
							}
						}
					}
				}
				if( itr->second != 0 ){
					ngAdd = FALSE;
				}
			}
			if( ngAdd == TRUE ){
				chkingList[i]->reserve->SetCoopAdd(L"", 0);
			}
		}
		wait = 10*60*1000;
	}
	return 0;
}

BOOL CNWCoopManager::GetIP2Host(wstring hostName, wstring& ip)
{
	LPHOSTENT host;
	string strA = "";
	WtoA(hostName, strA);

	host = gethostbyname(strA.c_str());
	if (host == NULL) {
		return FALSE;
	}

	if( host->h_addr_list[0] == NULL ){
		return FALSE;
	}
	Format(ip, L"%d.%d.%d.%d", 
			(BYTE)*((host->h_addr_list[0])) ,
			(BYTE)*((host->h_addr_list[0]) + 1) ,
			(BYTE)*((host->h_addr_list[0]) + 2) ,
			(BYTE)*((host->h_addr_list[0]) + 3)
			);

	return TRUE;
}

BOOL CNWCoopManager::GetIP2Host(wstring hostName, string& ip)
{
	LPHOSTENT host;
	string strA = "";
	WtoA(hostName, strA);

	host = gethostbyname(strA.c_str());
	if (host == NULL) {
		return FALSE;
	}

	if( host->h_addr_list[0] == NULL ){
		return FALSE;
	}
	Format(ip, "%d.%d.%d.%d", 
			(BYTE)*((host->h_addr_list[0])) ,
			(BYTE)*((host->h_addr_list[0]) + 1) ,
			(BYTE)*((host->h_addr_list[0]) + 2) ,
			(BYTE)*((host->h_addr_list[0]) + 3)
			);

	return TRUE;
}

BOOL CNWCoopManager::SendMagicPacket(BYTE* mac, wstring host, WORD port)
{
	string ip = "127.0.0.1";

	if( GetIP2Host(host, ip) == FALSE ){
		return FALSE;
	}

	SOCKET sock;
	struct sockaddr_in addr;
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if( sock == INVALID_SOCKET ){
		return FALSE;
	}
	//ノンブロッキングモードへ
	ULONG x = 1;
	ioctlsocket(sock, FIONBIO, &x);

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.S_un.S_addr = inet_addr(ip.c_str());

	DWORD dwHost = inet_addr("127.0.0.1");
	setsockopt(sock, IPPROTO_IP,IP_MULTICAST_IF,(char *)&dwHost, sizeof(DWORD));

	BYTE packet[102];
	memset(packet, 0xFF, 6);
	for( int i=6; i<102; i+=6 ){
		memcpy(packet+i, mac, 6);
	}

	if(sendto(sock, (char*)packet, 102, 0, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR ){
		closesocket(sock);
		return FALSE;
	}

	closesocket(sock);

	return TRUE;
}

BOOL CNWCoopManager::SendMagicPacket(BYTE* mac, WORD port)
{
	string ip = "255.255.255.255";

	SOCKET sock;
	struct sockaddr_in addr;
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if( sock == INVALID_SOCKET ){
		return FALSE;
	}
	//ノンブロッキングモードへ
	ULONG x = 1;
	ioctlsocket(sock, FIONBIO, &x);

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.S_un.S_addr = inet_addr(ip.c_str());

	DWORD dwHost = 1;
	setsockopt(sock,SOL_SOCKET, SO_BROADCAST, (char *)&dwHost, sizeof(DWORD));

	BYTE packet[102];
	memset(packet, 0xFF, 6);
	for( int i=6; i<102; i+=6 ){
		memcpy(packet+i, mac, 6);
	}

	if(sendto(sock, (char*)packet, 102, 0, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR ){
		closesocket(sock);
		return FALSE;
	}

	closesocket(sock);

	return TRUE;
}

BOOL CNWCoopManager::SetChkEpgFile(vector<wstring>* chkFileList)
{
	if( Lock(L"CNWCoopManager::SetChkEpgFile") == FALSE ) return FALSE;

	BOOL ret = TRUE;

	if( QueueLock() == TRUE ){
		this->updateEpgData = FALSE;
		this->chkEpgFileList = *chkFileList;
		QueueUnLock();
	}

	UnLock();
	return ret;
}

void CNWCoopManager::StartChkEpgFile()
{
	if( Lock(L"CNWCoopManager::StartChkEpgFile") == FALSE ) return ;

	if( this->chkEpgThread != NULL ){
		if( ::WaitForSingleObject(this->chkEpgThread, 0) != WAIT_TIMEOUT ){
			//終わっている
			CloseHandle(this->chkEpgThread);
			this->chkEpgThread = NULL;
		}
	}

	if( this->chkEpgThread == NULL ){
		ResetEvent(this->chkEpgStopEvent);
		this->chkEpgThread = (HANDLE)_beginthreadex(NULL, 0, ChkEpgThread, (LPVOID)this, CREATE_SUSPENDED, NULL);
		SetThreadPriority( this->chkEpgThread, THREAD_PRIORITY_NORMAL );
		ResumeThread(this->chkEpgThread);
	}

	UnLock();
	return ;
}

void CNWCoopManager::StopChkEpgFile()
{
	if( Lock(L"CNWCoopManager::StopChkEpgFile") == FALSE ) return ;

	if( this->chkEpgThread != NULL ){
		::SetEvent(this->chkEpgStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->chkEpgThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->chkEpgThread, 0xffffffff);
		}
		CloseHandle(this->chkEpgThread);
		this->chkEpgThread = NULL;
	}

	UnLock();
	return ;
}

BOOL CNWCoopManager::IsUpdateEpgData()
{
	if( Lock(L"CNWCoopManager::IsUpdateEpgData") == FALSE ) return FALSE;

	BOOL ret = this->updateEpgData;
	this->updateEpgData = FALSE;

	UnLock();
	return ret;
}

UINT WINAPI CNWCoopManager::ChkEpgThread(LPVOID param)
{
	CNWCoopManager* sys = (CNWCoopManager*)param;
	CSendCtrlCmd sendCtrl;
	map<DWORD,DWORD>::iterator itr;
	DWORD wait = 0;
	while(1){
		if( ::WaitForSingleObject(sys->chkEpgStopEvent, wait) != WAIT_TIMEOUT ){
			//キャンセルされた
			break;
		}
		vector<wstring> chkingList;
		map<DWORD, COOP_SERVER_INFO> srvList;

		//現在の情報取得
		if( sys->QueueLock() == FALSE ) return 0;
		srvList = sys->srvList;
		chkingList = sys->chkEpgFileList;
		sys->QueueUnLock();

		if( chkingList.size() == 0 || srvList.size() == 0){
			//リストないので終了
			return 0;
		}

		wstring folderPath = L"";
		GetSettingPath(folderPath);
		folderPath += EPG_SAVE_FOLDER;
		folderPath += L"\\";

		BOOL chgFile = FALSE;
		for( size_t i=0; i<chkingList.size(); i++ ){
			if( ::WaitForSingleObject(sys->chkEpgStopEvent, 0) != WAIT_TIMEOUT ){
				//キャンセルされた
				return 0;
			}
			//ローカルファイルのタイムスタンプ確認
			LONGLONG localFileTime = 0;
			wstring filePath = folderPath;
			filePath += chkingList[i];

			HANDLE file = CreateFile(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
			if( file != INVALID_HANDLE_VALUE){
				FILETIME CreationTime;
				FILETIME LastAccessTime;
				FILETIME LastWriteTime;
				GetFileTime(file, &CreationTime, &LastAccessTime, &LastWriteTime);

				localFileTime = ((LONGLONG)LastWriteTime.dwHighDateTime)<<32 | (LONGLONG)LastWriteTime.dwLowDateTime;

				CloseHandle(file);
			}else{
				if( GetLastError() != ERROR_FILE_NOT_FOUND ){
					continue;
				}
			}

			//サーバーのタイムスタンプ確認
			wstring srvIP = L"";
			WORD srvPort = 0;
			LONGLONG maxTime = 0;
			map<DWORD, COOP_SERVER_INFO>::iterator itr;
			for( itr = srvList.begin(); itr != srvList.end(); itr++){
				if( ::WaitForSingleObject(sys->chkEpgStopEvent, 0) != WAIT_TIMEOUT ){
					//キャンセルされた
					return 0;
				}
				sendCtrl.SetSendMode(TRUE);
				wstring ip;
				if( sys->GetIP2Host(itr->second.hostName, ip) == FALSE){
					continue;
				}
				_OutputDebugString(L"++CNWCoopManager EPGChk ip %s\r\n", ip.c_str());
				sendCtrl.SetNWSetting(ip, itr->second.srvPort);
				sendCtrl.SetConnectTimeOut(15*1000);

				LONGLONG fileTime = 0;
				DWORD err = sendCtrl.SendGetEpgFileTime2(chkingList[i], &fileTime);
				if( err == CMD_SUCCESS ){
					if( maxTime < fileTime ){
						srvIP = ip;
						srvPort = itr->second.srvPort;
						maxTime = fileTime;
					}
				}
			}
			if( maxTime > 0 && maxTime > (localFileTime + 60*60*I64_1SEC)){
				//1時間以上新しいファイルなので取得する
				sendCtrl.SetNWSetting(srvIP, srvPort);
				sendCtrl.SetConnectTimeOut(15*1000);

				BYTE* data = NULL;
				DWORD dataSize = 0;
				DWORD err = sendCtrl.SendGetEpgFile2(chkingList[i], &data, &dataSize);
				if( err == CMD_SUCCESS ){
					file = _CreateDirectoryAndFile(filePath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
					if( file != INVALID_HANDLE_VALUE){
						DWORD writeSize = 0;
						WriteFile(file, data, dataSize, &writeSize, NULL);
						CloseHandle(file);

						chgFile = TRUE;
						_OutputDebugString(L"ip:%s port: %d のファイルでEPG更新：%s\r\n", srvIP.c_str(), srvPort, filePath.c_str());
					}
					SAFE_DELETE_ARRAY(data);
				}
			}
		}
		if( chgFile == TRUE ){
			sys->updateEpgData = TRUE;
		}
		wait = 10*60*1000;
	}
	return 0;
}
