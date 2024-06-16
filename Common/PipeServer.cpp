#include "stdafx.h"
#include "PipeServer.h"
#include "StringUtil.h"
#include "CtrlCmdDef.h"
#include "ErrDef.h"
#include "CommonDef.h"
#ifdef _WIN32
#include <aclapi.h>
#else
#include "PathUtil.h"
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif

#define PIPE_TIMEOUT 500
#define PIPE_CONNECT_OPEN_TIMEOUT 20000

CPipeServer::CPipeServer(void)
{
#ifdef _WIN32
	for( int i = 0; i < 2; i++ ){
		this->hEventOls[i] = NULL;
		this->hEventConnects[i] = NULL;
		this->hPipes[i] = INVALID_HANDLE_VALUE;
	}
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
	const std::function<void(CCmdStream&, CCmdStream&)>& cmdProc_,
	bool insecureFlag,
	bool doNotCreateNoWaitPipe
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
	for( int i = 0; i < 2; i++ ){
		wstring eventName = (L"Global\\" + pipeName).replace(7 + pipeName.find(L"Pipe"), 4, i == 0 ? L"Connect" : L"NoWaitConnect");
		this->hEventConnects[i] = CreateEvent(NULL, FALSE, FALSE, eventName.c_str());
		if( this->hEventConnects[i] && GetLastError() != ERROR_ALREADY_EXISTS ){

			//NT AUTHORITY\\Authenticated Users
			//ユーザー名はOSの言語設定で変化するので、SIDを使用する
			WCHAR trusteeName[] = L"S-1-5-11";
			SID trusteeSid;
			//SubAuthorityCountが明らかに1なのでバッファの拡張は不要
			DWORD cbTrusteeSid = sizeof(trusteeSid);
			if( !CreateWellKnownSid(WinAuthenticatedUserSid, NULL, &trusteeSid, &cbTrusteeSid) ){
				cbTrusteeSid = 0;
			}
			DWORD writeDac = 0;

			if( insecureFlag ){
				//現在はSYNCHRONIZEでよいが以前のクライアントはCreateEvent()で開いていたのでGENERIC_ALLが必要
				if( cbTrusteeSid && GrantAccessToKernelObject(this->hEventConnects[i], (WCHAR*)&trusteeSid, true, GENERIC_ALL) ){
					AddDebugLogFormat(L"Granted GENERIC_ALL on %ls to %ls", eventName.c_str(), trusteeName);
					writeDac = WRITE_DAC;
				}
			}else if( GrantServerAccessToKernelObject(this->hEventConnects[i], SYNCHRONIZE) ){
				AddDebugLogFormat(L"Granted SYNCHRONIZE on %ls to %ls", eventName.c_str(), SERVICE_NAME);
				writeDac = WRITE_DAC;
			}
			wstring pipePath = (L"\\\\.\\pipe\\" + pipeName).replace(9 + pipeName.find(L"Pipe"), 0, i == 0 ? L"" : L"NoWait");
			this->hPipes[i] = CreateNamedPipe(pipePath.c_str(), PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | writeDac, 0, 1, 8192, 8192, PIPE_TIMEOUT, NULL);
			if( this->hPipes[i] != INVALID_HANDLE_VALUE ){
				if( insecureFlag ){
					if( writeDac && cbTrusteeSid && GrantAccessToKernelObject(this->hPipes[i], (WCHAR*)&trusteeSid, true, GENERIC_READ | GENERIC_WRITE) ){
						AddDebugLogFormat(L"Granted GENERIC_READ|GENERIC_WRITE on %ls to %ls", pipePath.c_str(), trusteeName);
					}
				}else if( writeDac && GrantServerAccessToKernelObject(this->hPipes[i], GENERIC_READ | GENERIC_WRITE) ){
					AddDebugLogFormat(L"Granted GENERIC_READ|GENERIC_WRITE on %ls to %ls", pipePath.c_str(), SERVICE_NAME);
				}
				this->hEventOls[i] = CreateEvent(NULL, TRUE, FALSE, NULL);
				if( this->hEventOls[i] ){
					if( i == (doNotCreateNoWaitPipe ? 0 : 1) ){
						this->exitingFlag = false;
						this->stopEvent.Reset();
						this->workThread = thread_(ServerThread, this);
						return true;
					}
					continue;
				}
			}
		}
		//失敗
		break;
	}
#else
	WtoUTF8(fs_path(EDCB_INI_ROOT).append(pipeName).native(), this->sockPath);
	sockaddr_un addr;
	if( this->sockPath.size() < sizeof(addr.sun_path) ){
		this->srvSock = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
		if( this->srvSock >= 0 ){
			remove(this->sockPath.c_str());
			addr.sun_family = AF_UNIX;
			strcpy(addr.sun_path, this->sockPath.c_str());
			if( bind(this->srvSock, (sockaddr*)&addr, sizeof(addr)) == 0 &&
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
	for( int i = 1; i >= 0; i-- ){
		if( this->hPipes[i] != INVALID_HANDLE_VALUE ){
			CloseHandle(this->hPipes[i]);
			this->hPipes[i] = INVALID_HANDLE_VALUE;
		}
		if( this->hEventConnects[i] ){
			CloseHandle(this->hEventConnects[i]);
			this->hEventConnects[i] = NULL;
		}
		if( this->hEventOls[i] ){
			CloseHandle(this->hEventOls[i]);
			this->hEventOls[i] = NULL;
		}
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
	return GrantAccessToKernelObject(handle, trusteeName, false, permissions);
}

BOOL CPipeServer::GrantAccessToKernelObject(HANDLE handle, WCHAR* trusteeName, bool trusteeIsSid, DWORD permissions)
{
	BOOL ret = FALSE;
	PACL pDacl;
	PSECURITY_DESCRIPTOR pSecurityDesc;
	if( GetSecurityInfo(handle, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, &pDacl, NULL, &pSecurityDesc) == ERROR_SUCCESS ){
		EXPLICIT_ACCESS explicitAccess = {};
		explicitAccess.grfAccessPermissions = permissions;
		explicitAccess.grfAccessMode = GRANT_ACCESS;
		explicitAccess.grfInheritance = NO_INHERITANCE;
		explicitAccess.Trustee.TrusteeForm = trusteeIsSid ? TRUSTEE_IS_SID : TRUSTEE_IS_NAME;
		//入力方向のAPIでは検証されないのでUNKNOWNでよい
		explicitAccess.Trustee.TrusteeType = TRUSTEE_IS_UNKNOWN;
		explicitAccess.Trustee.ptstrName = trusteeName;
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
#else
void CPipeServer::DeleteRemainingFiles(LPCWSTR pipeName)
{
	EnumFindFile(fs_path(EDCB_INI_ROOT).append(pipeName).concat(L"*"), [pipeName](UTIL_FIND_DATA& findData) -> bool {
		if( findData.fileName.size() > wcslen(pipeName) ){
			int pid = (int)wcstol(findData.fileName.c_str() + wcslen(pipeName), NULL, 10);
			if( pid > 0 && kill(pid, 0) == -1 && errno == ESRCH ){
				AddDebugLogFormat(L"Delete remaining %ls", findData.fileName.c_str());
				DeleteFile(fs_path(EDCB_INI_ROOT).append(findData.fileName).c_str());
			}
		}
		return true;
	});
}
#endif

void CPipeServer::ServerThread(CPipeServer* pSys)
{
#ifdef _WIN32
	OVERLAPPED ols[2] = {};
#endif

	for(;;){
#ifdef _WIN32
		int olNum = 0;
		int olIndexes[2] = {};
		HANDLE hEventArray[3] = { pSys->stopEvent.Handle() };
		for( int i = 0; i < (pSys->hEventOls[1] ? 2 : 1); i++ ){
			if( ols[i].hEvent == NULL ){
				OVERLAPPED olZero = {};
				ols[i] = olZero;
				ols[i].hEvent = pSys->hEventOls[i];
				if( ConnectNamedPipe(pSys->hPipes[i], ols + i) == FALSE ){
					DWORD err = GetLastError();
					if( err == ERROR_PIPE_CONNECTED ){
						SetEvent(pSys->hEventOls[i]);
					}else if( err != ERROR_IO_PENDING ){
						//エラー
						ols[i].hEvent = NULL;
						continue;
					}
				}
				SetEvent(pSys->hEventConnects[i]);
			}
			olIndexes[olNum] = i;
			hEventArray[++olNum] = pSys->hEventOls[i];
		}
		if( olNum == 0 ){
			//すべてエラーになった
			break;
		}

		DWORD dwRes;
		int olTimes[2] = {};
		while( (dwRes = WaitForMultipleObjects(olNum + 1, hEventArray, FALSE, 10000)) == WAIT_TIMEOUT ){
			for( int i = 0; i < olNum; i++ ){
				//クライアントが接続待ちイベントを獲得したままパイプに接続しなかった場合に接続不能になるのを防ぐ
				if( WaitForSingleObject(pSys->hEventConnects[olIndexes[i]], 0) == WAIT_OBJECT_0 ||
				    olTimes[i] >= PIPE_CONNECT_OPEN_TIMEOUT ){
					SetEvent(pSys->hEventConnects[olIndexes[i]]);
					olTimes[i] = 0;
				}else{
					olTimes[i] += 10000;
				}
			}
		}
		if( dwRes == WAIT_OBJECT_0 ){
			//STOP
			for( int i = 0; i < olNum; i++ ){
				CancelIo(pSys->hPipes[olIndexes[i]]);
				WaitForSingleObject(pSys->hEventOls[olIndexes[i]], INFINITE);
			}
			break;
		}else if( dwRes == WAIT_OBJECT_0 + 1 || dwRes == WAIT_OBJECT_0 + 2 ){
			//コマンド受信
			ols[olIndexes[dwRes - WAIT_OBJECT_0 - 1]].hEvent = NULL;
			HANDLE hPipe = pSys->hPipes[olIndexes[dwRes - WAIT_OBJECT_0 - 1]];

			for(;;){
				DWORD dwWrite = 0;
				BYTE head[8];
				DWORD n = 0;
				for( DWORD m; n < sizeof(head) && ReadFile(hPipe, head + n, sizeof(head) - n, &m, NULL); n += m );
				if( n != sizeof(head) ){
					break;
				}
				CCmdStream cmd(head[0] | head[1] << 8 | head[2] << 16 | (DWORD)head[3] << 24);
				cmd.Resize(head[4] | head[5] << 8 | head[6] << 16 | (DWORD)head[7] << 24);
				n = 0;
				for( DWORD m; n < cmd.GetDataSize() && ReadFile(hPipe, cmd.GetData() + n, cmd.GetDataSize() - n, &m, NULL); n += m );
				if( n != cmd.GetDataSize() ){
					break;
				}

				CCmdStream res;
				pSys->cmdProc(cmd, res);
				if( WriteFile(hPipe, res.GetStream(), res.GetStreamSize(), &dwWrite, NULL) == FALSE ){
					break;
				}
				FlushFileBuffers(hPipe);
				if( res.GetParam() != OLD_CMD_NEXT ){
					//Enum用の繰り返しではない
					break;
				}
			}
			DisconnectNamedPipe(hPipe);
		}
#else
		pollfd pfds[2];
		pfds[0].fd = pSys->stopEvent.Handle();
		pfds[0].events = POLLIN;
		pfds[1].fd = pSys->srvSock;
		pfds[1].events = POLLIN;
		if( poll(pfds, 2, -1) < 0 ){
			if( errno != EINTR ){
				//エラー
				break;
			}
		}else if( pfds[0].revents & POLLIN ){
			if( pSys->stopEvent.WaitOne(0) ){
				//STOP
				break;
			}
		}else if( pfds[1].revents & POLLIN ){
			//コマンド受信
			int sock = accept4(pSys->srvSock, NULL, NULL, SOCK_CLOEXEC);
			if( sock >= 0 ){
				for(;;){
					BYTE head[8];
					DWORD n = 0;
					for( int m; n < sizeof(head) && (m = (int)recv(sock, head + n, sizeof(head) - n, 0)) > 0; n += m );
					if( n != sizeof(head) ){
						break;
					}
					CCmdStream cmd(head[0] | head[1] << 8 | head[2] << 16 | (DWORD)head[3] << 24);
					cmd.Resize(head[4] | head[5] << 8 | head[6] << 16 | (DWORD)head[7] << 24);
					n = 0;
					for( int m; n < cmd.GetDataSize() && (m = (int)recv(sock, cmd.GetData() + n, cmd.GetDataSize() - n, 0)) > 0; n += m );
					if( n != cmd.GetDataSize() ){
						break;
					}

					CCmdStream res;
					pSys->cmdProc(cmd, res);
					if( send(sock, res.GetStream(), res.GetStreamSize(), 0) != (int)res.GetStreamSize() ){
						break;
					}
					if( res.GetParam() != OLD_CMD_NEXT ){
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
