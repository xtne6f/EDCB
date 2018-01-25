#include "stdafx.h"
#include "PipeServer.h"
#include "StringUtil.h"
#include "CtrlCmdDef.h"
#include "ErrDef.h"
#include "CommonDef.h"
#include <aclapi.h>

#define PIPE_TIMEOUT 500
#define PIPE_CONNECT_OPEN_TIMEOUT 20000

CPipeServer::CPipeServer(void)
{
	this->hEventConnect = NULL;
	this->hPipe = INVALID_HANDLE_VALUE;
}

CPipeServer::~CPipeServer(void)
{
	StopServer();
}

BOOL CPipeServer::StartServer(
	LPCWSTR eventName, 
	LPCWSTR pipeName, 
	const std::function<void(CMD_STREAM*, CMD_STREAM*)>& cmdProc_,
	BOOL insecureFlag
)
{
	if( !cmdProc_ || eventName == NULL || pipeName == NULL ){
		return FALSE;
	}
	if( this->workThread.joinable() ){
		return FALSE;
	}
	this->cmdProc = cmdProc_;

	//原作仕様では同期的にパイプを生成しないので注意
	this->hEventConnect = CreateEvent(NULL, FALSE, FALSE, eventName);
	if( this->hEventConnect && GetLastError() != ERROR_ALREADY_EXISTS ){
		WCHAR trusteeName[] = L"NT AUTHORITY\\Authenticated Users";
		DWORD writeDac = 0;
		if( insecureFlag ){
			//現在はSYNCHRONIZEでよいが以前のクライアントはCreateEvent()で開いていたのでGENERIC_ALLが必要
			if( GrantAccessToKernelObject(this->hEventConnect, trusteeName, GENERIC_ALL) ){
				_OutputDebugString(L"Granted GENERIC_ALL on %s to %s\r\n", eventName, trusteeName);
				writeDac = WRITE_DAC;
			}
		}else if( GrantServerAccessToKernelObject(this->hEventConnect, SYNCHRONIZE) ){
			_OutputDebugString(L"Granted SYNCHRONIZE on %s to %s\r\n", eventName, SERVICE_NAME);
			writeDac = WRITE_DAC;
		}
		this->hPipe = CreateNamedPipe(pipeName, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | writeDac, 0, 1, 8192, 8192, PIPE_TIMEOUT, NULL);
		if( this->hPipe != INVALID_HANDLE_VALUE ){
			if( insecureFlag ){
				if( writeDac && GrantAccessToKernelObject(this->hPipe, trusteeName, GENERIC_READ | GENERIC_WRITE) ){
					_OutputDebugString(L"Granted GENERIC_READ|GENERIC_WRITE on %s to %s\r\n", pipeName, trusteeName);
				}
			}else if( writeDac && GrantServerAccessToKernelObject(this->hPipe, GENERIC_READ | GENERIC_WRITE) ){
				_OutputDebugString(L"Granted GENERIC_READ|GENERIC_WRITE on %s to %s\r\n", pipeName, SERVICE_NAME);
			}
			this->stopEvent.Reset();
			this->workThread = thread_(ServerThread, this);
			return TRUE;
		}
	}
	StopServer();
	return TRUE;
}

BOOL CPipeServer::StopServer(BOOL checkOnlyFlag)
{
	if( this->workThread.joinable() ){
		this->stopEvent.Set();
		if( checkOnlyFlag ){
			//終了チェックして結果を返すだけ
			if( WaitForSingleObject(this->workThread.native_handle(), 0) == WAIT_TIMEOUT ){
				return FALSE;
			}
		}
		this->workThread.join();
	}
	if( this->hPipe != INVALID_HANDLE_VALUE ){
		CloseHandle(this->hPipe);
		this->hPipe = INVALID_HANDLE_VALUE;
	}
	if( this->hEventConnect ){
		CloseHandle(this->hEventConnect);
		this->hEventConnect = NULL;
	}
	return TRUE;
}

BOOL CPipeServer::GrantServerAccessToKernelObject(HANDLE handle, DWORD permissions)
{
	WCHAR trusteeName[] = L"NT Service\\" SERVICE_NAME;
	return GrantAccessToKernelObject(handle, trusteeName, permissions);
}

BOOL CPipeServer::GrantAccessToKernelObject(HANDLE handle, WCHAR* trusteeName, DWORD permissions)
{
	BOOL ret = FALSE;
	PACL pDacl;
	PSECURITY_DESCRIPTOR pSecurityDesc;
	if( GetSecurityInfo(handle, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, &pDacl, NULL, &pSecurityDesc) == ERROR_SUCCESS ){
		EXPLICIT_ACCESS explicitAccess;
		BuildExplicitAccessWithName(&explicitAccess, trusteeName, permissions, GRANT_ACCESS, 0);
		PACL pNewDacl;
		if( SetEntriesInAcl(1, &explicitAccess, pDacl, &pNewDacl) == ERROR_SUCCESS ){
			if( SetSecurityInfo(handle, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, pNewDacl, NULL) == ERROR_SUCCESS ){
				ret = TRUE;
			}
			LocalFree(pNewDacl);
		}
		LocalFree(pSecurityDesc);
	}
	return ret;
}

static DWORD ReadFileAll(HANDLE hFile, BYTE* lpBuffer, DWORD dwToRead)
{
	DWORD dwRet = 0;
	for( DWORD dwRead; dwRet < dwToRead && ReadFile(hFile, lpBuffer + dwRet, dwToRead - dwRet, &dwRead, NULL); dwRet += dwRead );
	return dwRet;
}

void CPipeServer::ServerThread(CPipeServer* pSys)
{
	HANDLE hEventArray[] = { pSys->stopEvent.Handle(), CreateEvent(NULL, TRUE, FALSE, NULL) };
	if( hEventArray[1] == NULL ){
		return;
	}

	for(;;){
		OVERLAPPED stOver = {};
		stOver.hEvent = hEventArray[1];
		if( ConnectNamedPipe(pSys->hPipe, &stOver) == FALSE ){
			DWORD err = GetLastError();
			if( err == ERROR_PIPE_CONNECTED ){
				SetEvent(hEventArray[1]);
			}else if( err != ERROR_IO_PENDING ){
				//エラー
				break;
			}
		}
		SetEvent(pSys->hEventConnect);

		DWORD dwRes;
		for( int t = 0; (dwRes = WaitForMultipleObjects(2, hEventArray, FALSE, 10000)) == WAIT_TIMEOUT; ){
			//クライアントが接続待ちイベントを獲得したままパイプに接続しなかった場合に接続不能になるのを防ぐ
			if( WaitForSingleObject(pSys->hEventConnect, 0) == WAIT_OBJECT_0 || t >= PIPE_CONNECT_OPEN_TIMEOUT ){
				SetEvent(pSys->hEventConnect);
				t = 0;
			}else{
				t += 10000;
			}
		}
		if( dwRes == WAIT_OBJECT_0 ){
			//STOP
			CancelIo(pSys->hPipe);
			WaitForSingleObject(hEventArray[1], INFINITE);
			break;
		}else if( dwRes == WAIT_OBJECT_0+1 ){
			//コマンド受信
			DWORD dwWrite = 0;
			DWORD head[2];
			for(;;){
				CMD_STREAM stCmd;
				CMD_STREAM stRes;
				if( ReadFileAll(pSys->hPipe, (BYTE*)head, sizeof(head)) != sizeof(head) ){
					break;
				}
				stCmd.param = head[0];
				stCmd.dataSize = head[1];
				if( stCmd.dataSize > 0 ){
					stCmd.data.reset(new BYTE[stCmd.dataSize]);
					if( ReadFileAll(pSys->hPipe, stCmd.data.get(), stCmd.dataSize) != stCmd.dataSize ){
						break;
					}
				}

				pSys->cmdProc(&stCmd, &stRes);
				head[0] = stRes.param;
				head[1] = stRes.dataSize;
				if( WriteFile(pSys->hPipe, head, sizeof(head), &dwWrite, NULL) == FALSE ){
					break;
				}
				if( stRes.dataSize > 0 ){
					if( WriteFile(pSys->hPipe, stRes.data.get(), stRes.dataSize, &dwWrite, NULL) == FALSE ){
						break;
					}
				}
				FlushFileBuffers(pSys->hPipe);
				if( stRes.param != CMD_NEXT && stRes.param != OLD_CMD_NEXT ){
					//Enum用の繰り返しではない
					break;
				}
			}
			DisconnectNamedPipe(pSys->hPipe);
		}
	}

	CloseHandle(hEventArray[1]);
}
