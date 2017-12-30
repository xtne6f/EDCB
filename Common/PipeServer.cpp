#include "stdafx.h"
#include "PipeServer.h"
#include "StringUtil.h"
#include "CtrlCmdDef.h"
#include "ErrDef.h"

#define PIPE_TIMEOUT 500
#define PIPE_CONNECT_OPEN_TIMEOUT 20000

CPipeServer::~CPipeServer(void)
{
	StopServer();
}

BOOL CPipeServer::StartServer(
	LPCWSTR eventName_, 
	LPCWSTR pipeName_, 
	const std::function<void(CMD_STREAM*, CMD_STREAM*)>& cmdProc_,
	BOOL insecureFlag_
)
{
	if( !cmdProc_ || eventName_ == NULL || pipeName_ == NULL ){
		return FALSE;
	}
	if( this->workThread.joinable() ){
		return FALSE;
	}
	this->cmdProc = cmdProc_;
	this->eventName = eventName_;
	this->pipeName = pipeName_;
	this->insecureFlag = insecureFlag_;

	this->stopEvent.Reset();
	this->workThread = thread_(ServerThread, this);

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
	return TRUE;
}

static DWORD ReadFileAll(HANDLE hFile, BYTE* lpBuffer, DWORD dwToRead)
{
	DWORD dwRet = 0;
	for( DWORD dwRead; dwRet < dwToRead && ReadFile(hFile, lpBuffer + dwRet, dwToRead - dwRet, &dwRead, NULL); dwRet += dwRead );
	return dwRet;
}

void CPipeServer::ServerThread(CPipeServer* pSys)
{
	HANDLE hPipe = INVALID_HANDLE_VALUE;
	HANDLE hEventConnect = NULL;
	HANDLE hEventArray[2];
	OVERLAPPED stOver = {};

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
	hEventArray[0] = pSys->stopEvent.Handle();
	hEventArray[1] = CreateEvent(NULL, FALSE, FALSE, NULL);
	
	if( hEventConnect && hEventArray[1] ){
		hPipe = CreateNamedPipe(pSys->pipeName.c_str(), PIPE_ACCESS_DUPLEX | FILE_FLAG_WRITE_THROUGH | FILE_FLAG_OVERLAPPED,
		                        PIPE_TYPE_BYTE, 1, 8192, 8192, PIPE_TIMEOUT, sa.nLength != 0 ? &sa : NULL);
		stOver.hEvent = hEventArray[1];
	}

	while( hPipe != INVALID_HANDLE_VALUE ){
		ConnectNamedPipe(hPipe, &stOver);
		SetEvent(hEventConnect);

		DWORD dwRes;
		for( int t = 0; (dwRes = WaitForMultipleObjects(2, hEventArray, FALSE, 10000)) == WAIT_TIMEOUT; ){
			//クライアントが接続待ちイベントを獲得したままパイプに接続しなかった場合に接続不能になるのを防ぐ
			if( WaitForSingleObject(hEventConnect, 0) == WAIT_OBJECT_0 || t >= PIPE_CONNECT_OPEN_TIMEOUT ){
				SetEvent(hEventConnect);
				t = 0;
			}else{
				t += 10000;
			}
		}
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

				pSys->cmdProc(&stCmd, &stRes);
				head[0] = stRes.param;
				head[1] = stRes.dataSize;
				if( WriteFile(hPipe, head, sizeof(head), &dwWrite, NULL) == FALSE ){
					break;
				}
				if( stRes.dataSize > 0 ){
					if( WriteFile(hPipe, stRes.data.get(), stRes.dataSize, &dwWrite, NULL) == FALSE ){
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

	if( hPipe != INVALID_HANDLE_VALUE ){
		FlushFileBuffers(hPipe);
		DisconnectNamedPipe(hPipe);
		CloseHandle(hPipe);
	}

	if( hEventArray[1] ){
		CloseHandle(hEventArray[1]);
	}
	if( hEventConnect ){
		CloseHandle(hEventConnect);
	}
}
