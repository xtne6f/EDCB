#include "StdAfx.h"
#include "PipeServer.h"
#include <process.h>

#include "StringUtil.h"
#include "CtrlCmdDef.h"
#include "ErrDef.h"

#define PIPE_TIMEOUT 500

CPipeServer::CPipeServer(void)
{
	this->cmdProc = NULL;
	this->cmdParam = NULL;
	this->eventName = L"";
	this->pipeName = L"";

	this->stopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	this->workThread = NULL;
}

CPipeServer::~CPipeServer(void)
{
	if( this->workThread != NULL ){
		::SetEvent(this->stopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->workThread, 60000) == WAIT_TIMEOUT ){
			::TerminateThread(this->workThread, 0xffffffff);
		}
		CloseHandle(this->workThread);
		this->workThread = NULL;
	}
	::CloseHandle(this->stopEvent);
	this->stopEvent = NULL;
}

BOOL CPipeServer::StartServer(
	LPCWSTR eventName, 
	LPCWSTR pipeName, 
	CMD_CALLBACK_PROC cmdCallback, 
	void* callbackParam, 
	int threadPriority,
	int ctrlCmdEventID
)
{
	if( cmdCallback == NULL || eventName == NULL || pipeName == NULL ){
		return FALSE;
	}
	if( this->workThread != NULL ){
		return FALSE;
	}
	this->cmdProc = cmdCallback;
	this->cmdParam = callbackParam;
	this->eventName = eventName;
	this->pipeName = pipeName;
	this->threadPriority = threadPriority;
	this->ctrlCmdEventID = ctrlCmdEventID;

	ResetEvent(this->stopEvent);
	this->workThread = (HANDLE)_beginthreadex(NULL, 0, ServerThread, (LPVOID)this, CREATE_SUSPENDED, NULL);
	ResumeThread(this->workThread);

	return TRUE;
}

void CPipeServer::StopServer()
{
	if( this->workThread != NULL ){
		::SetEvent(this->stopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->workThread, 60000) == WAIT_TIMEOUT ){
			::TerminateThread(this->workThread, 0xffffffff);
		}
		CloseHandle(this->workThread);
		this->workThread = NULL;
	}
}

UINT WINAPI CPipeServer::ServerThread(LPVOID pParam)
{
	CPipeServer* pSys = (CPipeServer*)pParam;

	HANDLE hCurThread = GetCurrentThread();
	SetThreadPriority(hCurThread, pSys->threadPriority);

	HANDLE hPipe = NULL;
	HANDLE hEventCmdWait = NULL;
	HANDLE hEventConnect = NULL;
	HANDLE hEventArray[2];
	OVERLAPPED stOver;

	if( pSys->ctrlCmdEventID != -1 ){
		wstring strCmdEvent;
		Format(strCmdEvent, L"%s%d", CMD2_CTRL_EVENT_WAIT, pSys->ctrlCmdEventID);
		hEventCmdWait = _CreateEvent(FALSE, TRUE, strCmdEvent.c_str());
	}
	hEventConnect = _CreateEvent(FALSE, FALSE, pSys->eventName.c_str());
	hEventArray[0] = pSys->stopEvent;
	hEventArray[1] = CreateEvent(NULL, FALSE, FALSE, NULL);
	
	if( hPipe == NULL ){
		while(1){
			hPipe = _CreateNamedPipe(pSys->pipeName.c_str(), PIPE_ACCESS_DUPLEX | FILE_FLAG_WRITE_THROUGH | FILE_FLAG_OVERLAPPED,
			PIPE_TYPE_BYTE, 1,
			CMD2_SEND_BUFF_SIZE, CMD2_RES_BUFF_SIZE, PIPE_TIMEOUT);
			if( GetLastError() == ERROR_PIPE_BUSY ){
				Sleep(100);
			}else{
				if(hPipe == INVALID_HANDLE_VALUE){
					hPipe = NULL;
				}
				break;
			}
		}

		ZeroMemory(&stOver, sizeof(OVERLAPPED));
		stOver.hEvent = hEventArray[1];
	}

	while(1){
		ConnectNamedPipe(hPipe, &stOver);
		SetEvent(hEventConnect);

		DWORD dwRes = WaitForMultipleObjects(2, hEventArray, FALSE, INFINITE);
		if( dwRes == WAIT_OBJECT_0 ){
			//STOP
			break;
		}else if( dwRes == WAIT_OBJECT_0+1 ){
			//ほかのサーバーで処理中？
			if( hEventCmdWait != NULL ){
				WaitForSingleObject(hEventCmdWait, INFINITE);
			}
			//コマンド受信
			CMD_STREAM stCmd;
			CMD_STREAM stRes;
			DWORD dwRead = 0;
			DWORD dwWrite = 0;
			DWORD head[2];
			do{
				if( ReadFile(hPipe, head, sizeof(DWORD)*2, &dwRead, NULL ) == FALSE ){
					break;
				}
				stCmd.param = head[0];
				stCmd.dataSize = head[1];
				if( stCmd.dataSize > 0 ){
					stCmd.data = new BYTE[stCmd.dataSize];
					DWORD ReadNum = 0;
					while(ReadNum < stCmd.dataSize ){
						DWORD dwReadSize = 0;
						if( stCmd.dataSize-ReadNum < CMD2_SEND_BUFF_SIZE ){
							dwReadSize = stCmd.dataSize-ReadNum;
						}else{
							dwReadSize = CMD2_SEND_BUFF_SIZE;
						}
						if( ReadFile(hPipe, stCmd.data+ReadNum, dwReadSize, &dwRead, NULL ) == FALSE ){
							break;
						}
						ReadNum+=dwRead;
					}
					if( ReadNum < stCmd.dataSize ){
						break;
					}
				}

				if( pSys->cmdProc != NULL){
					pSys->cmdProc(pSys->cmdParam, &stCmd, &stRes);
					head[0] = stRes.param;
					head[1] = stRes.dataSize;
					if( WriteFile(hPipe, head, sizeof(DWORD)*2, &dwWrite, NULL ) == FALSE ){
						break;
					}
					if( stRes.dataSize > 0 ){
						if( stRes.data == NULL ){
							break;
						}
						DWORD SendNum = 0;
						while(SendNum < stRes.dataSize ){
							DWORD dwSendSize = 0;
							if( stRes.dataSize-SendNum < CMD2_RES_BUFF_SIZE ){
								dwSendSize = stRes.dataSize-SendNum;
							}else{
								dwSendSize = CMD2_RES_BUFF_SIZE;
							}
							if( WriteFile(hPipe, stRes.data+SendNum, dwSendSize, &dwWrite, NULL ) == FALSE ){
								break;
							}
							SendNum+=dwWrite;
						}
						if( SendNum < stRes.dataSize ){
							break;
						}
					}
				}else{
					head[0] = CMD_NON_SUPPORT;
					head[1] = 0;
					if( WriteFile(hPipe, head, sizeof(DWORD)*2, &dwWrite, NULL ) == FALSE ){
						break;
					}
				}
				SAFE_DELETE_ARRAY(stCmd.data);
				SAFE_DELETE_ARRAY(stRes.data);
				stCmd.dataSize = 0;
				stRes.dataSize = 0;

			}while(stRes.param == CMD_NEXT || stRes.param == OLD_CMD_NEXT); //Emun用の繰り返し

			FlushFileBuffers(hPipe);
			DisconnectNamedPipe(hPipe);
			if( hEventCmdWait != NULL ){
				SetEvent(hEventCmdWait);
			}
		}
	}

	if( hPipe != NULL ){
		FlushFileBuffers(hPipe);
		DisconnectNamedPipe(hPipe);
		CloseHandle(hPipe);
	}

	CloseHandle(hEventArray[1]);
	CloseHandle(hEventConnect);
	if( hEventCmdWait != NULL ){
		CloseHandle(hEventCmdWait);
	}
	return 0;
}
