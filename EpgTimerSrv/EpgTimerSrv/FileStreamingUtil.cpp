#include "StdAfx.h"
#include "FileStreamingUtil.h"
#include <process.h>

CFileStreamingUtil::CFileStreamingUtil(void)
{
	this->tcpIP = 0;
	this->udpIP = 0;

    this->sizeChkThread = NULL;
    this->sizeChkStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
}


CFileStreamingUtil::~CFileStreamingUtil(void)
{
	if( this->sizeChkThread != NULL ){
		::SetEvent(this->sizeChkStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->sizeChkThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->sizeChkThread, 0xffffffff);
		}
		CloseHandle(this->sizeChkThread);
		this->sizeChkThread = NULL;
	}
	if( this->sizeChkStopEvent != NULL ){
		CloseHandle(this->sizeChkStopEvent);
		this->sizeChkStopEvent = NULL;
	}

	this->timeShiftUtil.StopTimeShift();
	this->timeShiftUtil.SendUdp(NULL);
	this->sendUdp.CloseUpload();
		
	if(this->udpPortMutex.size() != 0){
		for( size_t i=0; i<this->udpPortMutex.size(); i++ ){
			::ReleaseMutex(this->udpPortMutex[i]);
			::CloseHandle(this->udpPortMutex[i]);
		}
		this->udpPortMutex.clear();
	}

	this->timeShiftUtil.SendTcp(NULL);
	this->sendTcp.CloseUpload();

	if(this->tcpPortMutex.size() != 0){
		for( int i=0; i<(int)this->tcpPortMutex.size(); i++ ){
			::ReleaseMutex(this->tcpPortMutex[i]);
			::CloseHandle(this->tcpPortMutex[i]);
		}
		this->tcpPortMutex.clear();
	}
}

BOOL CFileStreamingUtil::OpenFile(LPCWSTR filePath)
{
	this->timeShiftUtil.StopTimeShift();
	return this->timeShiftUtil.OpenTimeShift(filePath, -1, TRUE);
}

BOOL CFileStreamingUtil::OpenTimeShift(LPCWSTR filePath, DWORD processID,DWORD exeCtrlID)
{
	if( this->sizeChkThread != NULL ){
		::SetEvent(this->sizeChkStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->sizeChkThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->sizeChkThread, 0xffffffff);
		}
		CloseHandle(this->sizeChkThread);
		this->sizeChkThread = NULL;
	}
	this->timeShiftUtil.StopTimeShift();

	cmd.SetPipeSetting(CMD2_VIEW_CTRL_WAIT_CONNECT, CMD2_VIEW_CTRL_PIPE, processID);
	cmd.SetConnectTimeOut(3*1000);

	BOOL ret = FALSE;
	__int64 fileSize = 0;
	if( this->cmd.SendViewGetWriteSize(exeCtrlID, &fileSize) == NO_ERR ){
		ret = this->timeShiftUtil.OpenTimeShift(filePath, fileSize, FALSE);

		this->timeShiftCtrlID = exeCtrlID;

		if( this->sizeChkThread == NULL ){
			//受信スレッド起動
			ResetEvent(this->sizeChkStopEvent);
			this->sizeChkThread = (HANDLE)_beginthreadex(NULL, 0, SizeChkThread, (LPVOID)this, CREATE_SUSPENDED, NULL);
			SetThreadPriority( this->sizeChkThread, THREAD_PRIORITY_NORMAL );
			ResumeThread(this->sizeChkThread);
		}
	}

	return ret;
}


UINT WINAPI CFileStreamingUtil::SizeChkThread(LPVOID param)
{
	CFileStreamingUtil* sys = (CFileStreamingUtil*)param;

	while(1){
		if( ::WaitForSingleObject(sys->sizeChkStopEvent, 300) != WAIT_TIMEOUT ){
			//キャンセルされた
			break;
		}
		__int64 fileSize = 0;
		if( sys->cmd.SendViewGetWriteSize(sys->timeShiftCtrlID, &fileSize) == NO_ERR ){
			sys->timeShiftUtil.SetAvailableSize(fileSize);
		}else{
			sys->timeShiftUtil.SetAvailableSize(-1);
			break;
		}
	}

	return 0;
}

BOOL CFileStreamingUtil::StartSend()
{
	return this->timeShiftUtil.StartTimeShift();
}

BOOL CFileStreamingUtil::StopSend()
{
	return this->timeShiftUtil.StopTimeShift();
}

//ストリーム配信で現在の送信位置と総ファイルサイズを取得する
//戻り値：
// エラーコード
//引数：
// val				[IN/OUT]サイズ情報
BOOL CFileStreamingUtil::GetPos(
	NWPLAY_POS_CMD* val
	)
{
	this->timeShiftUtil.GetCurrentFilePos(&val->currentPos);
	this->timeShiftUtil.GetTotalFilePos(&val->totalPos);
	return TRUE;
}

//ストリーム配信で送信位置をシークする
//戻り値：
// エラーコード
//引数：
// val				[IN]サイズ情報
BOOL CFileStreamingUtil::SetPos(
	NWPLAY_POS_CMD* val
	)
{
	this->timeShiftUtil.SetFilePos(val->currentPos);
	return TRUE;
}

//ストリーム配信で送信先を設定する
//戻り値：
// エラーコード
//引数：
// val				[IN]サイズ情報
BOOL CFileStreamingUtil::SetIP(
	NWPLAY_PLAY_INFO* val
	)
{
	if( val->udp == 1 ){
		if( this->udpIP != val->ip ){
			this->udpSendList.clear();
			NW_SEND_INFO item;

			item.ip = val->ip;
			item.port = 1234;
			Format(item.ipString, L"%d.%d.%d.%d",
				(item.ip&0xFF000000)>>24,
				(item.ip&0x00FF0000)>>16,
				(item.ip&0x0000FF00)>>8,
				(item.ip&0x000000FF) );
			item.broadcastFlag = 0;

			wstring mutexKey = L"";
			while(1){
				Format(mutexKey, L"%s%d_%d", MUTEX_UDP_PORT_NAME, item.ip, item.port );
				HANDLE mutex = ::OpenMutex(MUTEX_ALL_ACCESS, FALSE, mutexKey.c_str());
		
				if(mutex){
					::CloseHandle(mutex);
					item.port++;
				}else{
					break;
				}
			}

			HANDLE portMutex = _CreateMutex( TRUE, mutexKey.c_str());
			_OutputDebugString(L"%s\r\n", mutexKey.c_str());
			this->udpPortMutex.push_back(portMutex);

			this->udpSendList.push_back(item);

			this->sendUdp.StartUpload(&this->udpSendList);

			this->timeShiftUtil.SendUdp(&this->sendUdp);

			this->udpIP = val->ip;

			val->udpPort = item.port;
		}
	}else{
		this->timeShiftUtil.SendUdp(NULL);
		this->sendUdp.CloseUpload();
		
		if(this->udpPortMutex.size() != 0){
			for( size_t i=0; i<this->udpPortMutex.size(); i++ ){
				::ReleaseMutex(this->udpPortMutex[i]);
				::CloseHandle(this->udpPortMutex[i]);
			}
			this->udpPortMutex.clear();
		}
		this->udpIP = 0;
	}
	if( val->tcp == 1 ){
		if( this->tcpIP != val->ip ){
			this->tcpSendList.clear();
			NW_SEND_INFO item;

			item.ip = val->ip;
			item.port = 2230;
			Format(item.ipString, L"%d.%d.%d.%d",
				(item.ip&0xFF000000)>>24,
				(item.ip&0x00FF0000)>>16,
				(item.ip&0x0000FF00)>>8,
				(item.ip&0x000000FF) );
			item.broadcastFlag = 0;

			wstring mutexKey = L"";

			while(1){
				Format(mutexKey, L"%s%d_%d", MUTEX_TCP_PORT_NAME, item.ip, item.port );
				HANDLE mutex = ::OpenMutex(MUTEX_ALL_ACCESS, FALSE, mutexKey.c_str());
		
				if(mutex){
					::CloseHandle(mutex);
					item.port++;
				}else{
					break;
				}
			}

			HANDLE portMutex = _CreateMutex( TRUE, mutexKey.c_str());
			_OutputDebugString(L"%s\r\n", mutexKey.c_str());
			this->tcpPortMutex.push_back(portMutex);

			this->tcpSendList.push_back(item);

			this->sendTcp.StartUpload(&this->tcpSendList);

			this->timeShiftUtil.SendTcp(&this->sendTcp);

			this->tcpIP = val->ip;

			val->tcpPort = item.port;
		}
	}else{
		this->timeShiftUtil.SendTcp(NULL);
		this->sendTcp.CloseUpload();

		if(this->tcpPortMutex.size() != 0){
			for( int i=0; i<(int)this->tcpPortMutex.size(); i++ ){
				::ReleaseMutex(this->tcpPortMutex[i]);
				::CloseHandle(this->tcpPortMutex[i]);
			}
			this->tcpPortMutex.clear();
		}
		this->tcpIP = 0;
		this->tcpSendList.clear();
	}
	return TRUE;
}