#include "stdafx.h"
#include "PipeServer.h"
#include "StringUtil.h"
#include "CtrlCmdDef.h"
#include "ErrDef.h"
#include "CommonDef.h"
#ifdef _WIN32
#include <aclapi.h>
#else
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>
#endif

#define PIPE_TIMEOUT 500
#define PIPE_CONNECT_OPEN_TIMEOUT 20000

CPipeServer::CPipeServer(void)
{
#ifdef _WIN32
	this->hEventOl = NULL;
	this->hEventConnect = NULL;
	this->hPipe = INVALID_HANDLE_VALUE;
#else
	this->srvSock = -1;
#endif
}

CPipeServer::~CPipeServer(void)
{
	StopServer();
}

bool CPipeServer::StartServer(
	const wstring& pipeName,
	const std::function<void(CMD_STREAM*, CMD_STREAM*)>& cmdProc_,
	bool insecureFlag
)
{
	if( !cmdProc_ || pipeName.find(L"Pipe") == wstring::npos ){
		return false;
	}
	if( this->workThread.joinable() ){
		return false;
	}
	this->cmdProc = cmdProc_;

	//原作仕様では同期的にパイプを生成しないので注意
#ifdef _WIN32
	wstring eventName = (L"Global\\" + pipeName).replace(7 + pipeName.find(L"Pipe"), 4, L"Connect");
	this->hEventConnect = CreateEvent(NULL, FALSE, FALSE, eventName.c_str());
	if( this->hEventConnect && GetLastError() != ERROR_ALREADY_EXISTS ){
		WCHAR trusteeName[] = L"NT AUTHORITY\\Authenticated Users";
		DWORD writeDac = 0;
		if( insecureFlag ){
			//現在はSYNCHRONIZEでよいが以前のクライアントはCreateEvent()で開いていたのでGENERIC_ALLが必要
			if( GrantAccessToKernelObject(this->hEventConnect, trusteeName, GENERIC_ALL) ){
				AddDebugLogFormat(L"Granted GENERIC_ALL on %ls to %ls", eventName.c_str(), trusteeName);
				writeDac = WRITE_DAC;
			}
		}else if( GrantServerAccessToKernelObject(this->hEventConnect, SYNCHRONIZE) ){
			AddDebugLogFormat(L"Granted SYNCHRONIZE on %ls to %ls", eventName.c_str(), SERVICE_NAME);
			writeDac = WRITE_DAC;
		}
		wstring pipePath = L"\\\\.\\pipe\\" + pipeName;
		this->hPipe = CreateNamedPipe(pipePath.c_str(), PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | writeDac, 0, 1, 8192, 8192, PIPE_TIMEOUT, NULL);
		if( this->hPipe != INVALID_HANDLE_VALUE ){
			if( insecureFlag ){
				if( writeDac && GrantAccessToKernelObject(this->hPipe, trusteeName, GENERIC_READ | GENERIC_WRITE) ){
					AddDebugLogFormat(L"Granted GENERIC_READ|GENERIC_WRITE on %ls to %ls", pipePath.c_str(), trusteeName);
				}
			}else if( writeDac && GrantServerAccessToKernelObject(this->hPipe, GENERIC_READ | GENERIC_WRITE) ){
				AddDebugLogFormat(L"Granted GENERIC_READ|GENERIC_WRITE on %ls to %ls", pipePath.c_str(), SERVICE_NAME);
			}
			this->hEventOl = CreateEvent(NULL, TRUE, FALSE, NULL);
			if( this->hEventOl ){
				this->exitingFlag = false;
				this->stopEvent.Reset();
				this->workThread = thread_(ServerThread, this);
				return true;
			}
		}
	}
#else
	WtoUTF8(EDCB_INI_ROOT + pipeName, this->sockPath);
	sockaddr_un addr;
	if( this->sockPath.size() < sizeof(addr.sun_path) ){
		this->srvSock = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
		if( this->srvSock >= 0 ){
			remove(this->sockPath.c_str());
			addr.sun_family = AF_UNIX;
			strcpy(addr.sun_path, this->sockPath.c_str());
			if( bind(this->srvSock, (sockaddr*)&addr, sizeof(addr)) == 0 &&
			    chmod(this->sockPath.c_str(), S_IRWXU) == 0 &&
			    listen(this->srvSock, SOMAXCONN) == 0 ){
				this->exitingFlag = false;
				this->stopEvent.Reset();
				this->workThread = thread_(ServerThread, this);
				return true;
			}
		}
	}
#endif
	StopServer();
	return false;
}

bool CPipeServer::StopServer(bool checkOnlyFlag)
{
	if( this->workThread.joinable() ){
		this->stopEvent.Set();
		if( checkOnlyFlag ){
			//終了チェックして結果を返すだけ
			if( this->exitingFlag == false ){
				return false;
			}
		}
		this->workThread.join();
	}
#ifdef _WIN32
	if( this->hPipe != INVALID_HANDLE_VALUE ){
		CloseHandle(this->hPipe);
		this->hPipe = INVALID_HANDLE_VALUE;
	}
	if( this->hEventConnect ){
		CloseHandle(this->hEventConnect);
		this->hEventConnect = NULL;
	}
	if( this->hEventOl ){
		CloseHandle(this->hEventOl);
		this->hEventOl = NULL;
	}
#else
	if( this->srvSock >= 0 ){
		int x = 0;
		ioctl(this->srvSock, FIONBIO, &x);
		close(this->srvSock);
		this->srvSock = -1;
		remove(this->sockPath.c_str());
	}
#endif
	return true;
}

#ifdef _WIN32
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
#endif

void CPipeServer::ServerThread(CPipeServer* pSys)
{
	for(;;){
#ifdef _WIN32
		HANDLE hEventArray[] = { pSys->stopEvent.Handle(), pSys->hEventOl };
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
				DWORD n = 0;
				for( DWORD m; n < sizeof(head) && ReadFile(pSys->hPipe, (BYTE*)head + n, sizeof(head) - n, &m, NULL); n += m );
				if( n != sizeof(head) ){
					break;
				}
				stCmd.param = head[0];
				stCmd.dataSize = head[1];
				if( stCmd.dataSize > 0 ){
					stCmd.data.reset(new BYTE[stCmd.dataSize]);
					n = 0;
					for( DWORD m; n < stCmd.dataSize && ReadFile(pSys->hPipe, stCmd.data.get() + n, stCmd.dataSize - n, &m, NULL); n += m );
					if( n != stCmd.dataSize ){
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
#else
		pollfd pfds[2];
		pfds[0].fd = pSys->stopEvent.Handle();
		pfds[0].events = POLLIN;
		pfds[1].fd = pSys->srvSock;
		pfds[1].events = POLLIN;
		if( poll(pfds, 2, -1) < 0 ){
			//エラー
			break;
		}else if( pfds[0].revents & POLLIN ){
			//STOP
			break;
		}else if( pfds[1].revents & POLLIN ){
			//コマンド受信
			int sock = accept4(pSys->srvSock, NULL, NULL, SOCK_CLOEXEC);
			if( sock >= 0 ){
				for(;;){
					DWORD head[2];
					CMD_STREAM stCmd;
					CMD_STREAM stRes;
					DWORD n = 0;
					for( int m; n < sizeof(head) && (m = (int)recv(sock, (BYTE*)head + n, sizeof(head) - n, 0)) > 0; n += m );
					if( n != sizeof(head) ){
						break;
					}
					stCmd.param = head[0];
					stCmd.dataSize = head[1];
					if( stCmd.dataSize > 0 ){
						stCmd.data.reset(new BYTE[stCmd.dataSize]);
						n = 0;
						for( int m; n < stCmd.dataSize && (m = (int)recv(sock, stCmd.data.get() + n, stCmd.dataSize - n, 0)) > 0; n += m );
						if( n != stCmd.dataSize ){
							break;
						}
					}

					pSys->cmdProc(&stCmd, &stRes);
					head[0] = stRes.param;
					head[1] = stRes.dataSize;
					if( send(sock, head, sizeof(head), 0) != (int)sizeof(head) ){
						break;
					}
					if( stRes.dataSize > 0 ){
						if( send(sock, stRes.data.get(), stRes.dataSize, 0) != (int)stRes.dataSize ){
							break;
						}
					}
					if( stRes.param != CMD_NEXT && stRes.param != OLD_CMD_NEXT ){
						//Enum用の繰り返しではない
						break;
					}
				}
				close(sock);
			}
		}
#endif
	}

	pSys->exitingFlag = true;
}
