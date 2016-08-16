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
	LPCWSTR eventName_, 
	LPCWSTR pipeName_, 
	CMD_CALLBACK_PROC cmdCallback, 
	void* callbackParam, 
	BOOL insecureFlag_
)
{
	if( cmdCallback == NULL || eventName_ == NULL || pipeName_ == NULL ){
		return FALSE;
	}
	if( this->workThread != NULL ){
		return FALSE;
	}
	this->cmdProc = cmdCallback;
	this->cmdParam = callbackParam;
	this->eventName = eventName_;
	this->pipeName = pipeName_;
	this->insecureFlag = insecureFlag_;

	ResetEvent(this->stopEvent);
	this->workThread = (HANDLE)_beginthreadex(NULL, 0, ServerThread, (LPVOID)this, CREATE_SUSPENDED, NULL);
	ResumeThread(this->workThread);

	return TRUE;
}

BOOL CPipeServer::StopServer(BOOL checkOnlyFlag)
{
	if( this->workThread != NULL ){
		::SetEvent(this->stopEvent);
		if( checkOnlyFlag ){
			//終了チェックして結果を返すだけ
			if( WaitForSingleObject(this->workThread, 0) == WAIT_TIMEOUT ){
				return FALSE;
			}
		}else{
			//スレッド終了待ち
			if( WaitForSingleObject(this->workThread, 60000) == WAIT_TIMEOUT ){
				TerminateThread(this->workThread, 0xffffffff);
			}
		}
		CloseHandle(this->workThread);
		this->workThread = NULL;
	}
	return TRUE;
}

static DWORD ReadFileAll(HANDLE hFile, BYTE* lpBuffer, DWORD dwToRead)
{
	DWORD dwRet = 0;
	for( DWORD dwRead; dwRet < dwToRead && ReadFile(hFile, lpBuffer + dwRet, dwToRead - dwRet, &dwRead, NULL); dwRet += dwRead );
	return dwRet;
}

UINT WINAPI CPipeServer::ServerThread(LPVOID pParam)
{
	CPipeServer* pSys = (CPipeServer*)pParam;

	HANDLE hPipe = NULL;
	HANDLE hEventConnect = NULL;
	HANDLE hEventArray[2];
	OVERLAPPED stOver;

	SECURITY_DESCRIPTOR sd = {};
	SECURITY_ATTRIBUTES sa = {};
	if( pSys->insecureFlag != FALSE &&
	    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION) != FALSE &&
	    SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE) != FALSE ){
		//安全でないセキュリティ記述子(NULL DACL)をセットする
		sa.nLength = sizeof(sa);
		sa.lpSecurityDescriptor = &sd;
	}
	hEventConnect = CreateEvent(sa.nLength != 0 ? &sa : NULL, FALSE, FALSE, pSys->eventName.c_str());
	hEventArray[0] = pSys->stopEvent;
	hEventArray[1] = CreateEvent(NULL, FALSE, FALSE, NULL);
	
	if( hPipe == NULL ){
		hPipe = CreateNamedPipe(pSys->pipeName.c_str(), PIPE_ACCESS_DUPLEX | FILE_FLAG_WRITE_THROUGH | FILE_FLAG_OVERLAPPED,
		                        PIPE_TYPE_BYTE, 1, 8192, 8192, PIPE_TIMEOUT, sa.nLength != 0 ? &sa : NULL);
		if( hPipe == INVALID_HANDLE_VALUE ){
			hPipe = NULL;
		}

		ZeroMemory(&stOver, sizeof(OVERLAPPED));
		stOver.hEvent = hEventArray[1];
	}

	while( hPipe != NULL ){
		ConnectNamedPipe(hPipe, &stOver);
		SetEvent(hEventConnect);

		DWORD dwRes = WaitForMultipleObjects(2, hEventArray, FALSE, INFINITE);
		if( dwRes == WAIT_OBJECT_0 ){
			//STOP
			break;
		}else if( dwRes == WAIT_OBJECT_0+1 ){
			//コマンド受信
			DWORD dwWrite = 0;
			DWORD head[2];
			for(;;){
				CMD_STREAM stCmd;
				CMD_STREAM stRes;
				if( ReadFileAll(hPipe, (BYTE*)head, sizeof(head)) != sizeof(head) ){
					break;
				}
				stCmd.param = head[0];
				stCmd.dataSize = head[1];
				if( stCmd.dataSize > 0 ){
					stCmd.data.reset(new BYTE[stCmd.dataSize]);
					if( ReadFileAll(hPipe, stCmd.data.get(), stCmd.dataSize) != stCmd.dataSize ){
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
						if( WriteFile(hPipe, stRes.data.get(), stRes.dataSize, &dwWrite, NULL ) == FALSE ){
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
				if( stRes.param != CMD_NEXT && stRes.param != OLD_CMD_NEXT ){
					//Enum用の繰り返しではない
					break;
				}
			}

			FlushFileBuffers(hPipe);
			DisconnectNamedPipe(hPipe);
		}
	}

	if( hPipe != NULL ){
		FlushFileBuffers(hPipe);
		DisconnectNamedPipe(hPipe);
		CloseHandle(hPipe);
	}

	CloseHandle(hEventArray[1]);
	CloseHandle(hEventConnect);
	return 0;
}
