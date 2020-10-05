#include "stdafx.h"
#include "SendCtrlCmd.h"
#if !defined(SEND_CTRL_CMD_NO_TCP) && defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
#include "StringUtil.h"
#ifndef _WIN32
#include "PathUtil.h"
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif

CSendCtrlCmd::CSendCtrlCmd(void)
{
	this->tcpFlag = FALSE;
	this->connectTimeOut = CONNECT_TIMEOUT;

	this->pipeName = CMD2_EPG_SRV_PIPE;
	this->sendIP = L"127.0.0.1";
	this->sendPort = 5678;

}


CSendCtrlCmd::~CSendCtrlCmd(void)
{
#if !defined(SEND_CTRL_CMD_NO_TCP) && defined(_WIN32)
	SetSendMode(FALSE);
#endif
}

#if !defined(SEND_CTRL_CMD_NO_TCP) && defined(_WIN32)

//コマンド送信方法の設定
//引数：
// tcpFlag		[IN] TRUE：TCP/IPモード、FALSE：名前付きパイプモード
void CSendCtrlCmd::SetSendMode(
	BOOL tcpFlag_
	)
{
	if( this->tcpFlag == FALSE && tcpFlag_ ){
		WSAData wsaData;
		WSAStartup(MAKEWORD(2, 2), &wsaData);
		this->tcpFlag = TRUE;
	}else if( this->tcpFlag && tcpFlag_ == FALSE ){
		WSACleanup();
		this->tcpFlag = FALSE;
	}
}

#endif

//名前付きパイプモード時の接続先を設定
//EpgTimerSrv.exeに対するコマンドは設定しなくても可（デフォルト値になっている）
//引数：
// pipeName		[IN]接続パイプの名前
void CSendCtrlCmd::SetPipeSetting(
	LPCWSTR pipeName_
	)
{
	this->pipeName = pipeName_;
}

//名前付きパイプモード時の接続先を設定（接尾にプロセスIDを伴うタイプ）
//引数：
// pid			[IN]プロセスID
void CSendCtrlCmd::SetPipeSetting(
	LPCWSTR pipeName_,
	DWORD pid
	)
{
	Format(this->pipeName, L"%ls%d", pipeName_, pid);
}

bool CSendCtrlCmd::PipeExists()
{
	if( this->pipeName.find(L"Pipe") != wstring::npos ){
#ifdef _WIN32
		HANDLE waitEvent = OpenEvent(SYNCHRONIZE, FALSE,
		                             (L"Global\\" + this->pipeName).replace(7 + this->pipeName.find(L"Pipe"), 4, L"Connect").c_str());
		if( waitEvent ){
			CloseHandle(waitEvent);
			return true;
		}
#else
		return UtilFileExists(EDCB_INI_ROOT + this->pipeName).first;
#endif
	}
	return false;
}

//TCP/IPモード時の接続先を設定
//引数：
// ip			[IN]接続先IP
// port			[IN]接続先ポート
void CSendCtrlCmd::SetNWSetting(
	const wstring& ip,
	DWORD port
	)
{
	this->sendIP = ip;
	this->sendPort = port;
}

//接続処理時のタイムアウト設定
// timeOut		[IN]タイムアウト値（単位：ms）
void CSendCtrlCmd::SetConnectTimeOut(
	DWORD timeOut
	)
{
	this->connectTimeOut = timeOut;
}

namespace
{
DWORD SendPipe(const wstring& pipeName, DWORD timeOut, const CMD_STREAM* cmd, CMD_STREAM* res)
{
#ifdef _WIN32
	//接続待ち
	//CreateEvent()してはいけない。イベントを作成するのはサーバの仕事のはず
	//CreateEvent()してしまうとサーバが終了した後は常にタイムアウトまで待たされることになる
	HANDLE waitEvent = NULL;
	if( pipeName.find(L"Pipe") != wstring::npos ){
		waitEvent = OpenEvent(SYNCHRONIZE, FALSE, (L"Global\\" + pipeName).replace(7 + pipeName.find(L"Pipe"), 4, L"Connect").c_str());
	}
	if( waitEvent == NULL ){
		return CMD_ERR_CONNECT;
	}
	DWORD dwRet = WaitForSingleObject(waitEvent, timeOut);
	CloseHandle(waitEvent);
	if( dwRet == WAIT_TIMEOUT ){
		return CMD_ERR_TIMEOUT;
	}else if( dwRet != WAIT_OBJECT_0 ){
		return CMD_ERR_CONNECT;
	}

	//接続
	HANDLE pipe = CreateFile((L"\\\\.\\pipe\\" + pipeName).c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if( pipe == INVALID_HANDLE_VALUE ){
		AddDebugLogFormat(L"*+* ConnectPipe Err:%d", GetLastError());
		return CMD_ERR_CONNECT;
	}
	auto closeFile = [=]() { return CloseHandle(pipe); };
#else
	string sockPath;
	WtoUTF8(EDCB_INI_ROOT + pipeName, sockPath);
	sockaddr_un addr;
	if( sockPath.size() >= sizeof(addr.sun_path) ){
		return CMD_ERR_INVALID_ARG;
	}
	int sock = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
	if( sock < 0 ){
		return CMD_ERR_CONNECT;
	}
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, sockPath.c_str());
	if( connect(sock, (sockaddr*)&addr, sizeof(addr)) != 0 ){
		if( errno != EINPROGRESS ){
			close(sock);
			return CMD_ERR_CONNECT;
		}
		pollfd pfd;
		pfd.fd = sock;
		pfd.events = POLLOUT;
		if( poll(&pfd, 1, timeOut) <= 0 || (pfd.revents & POLLOUT) == 0 ){
			close(sock);
			return CMD_ERR_TIMEOUT;
		}
	}
	int x = 0;
	ioctl(sock, FIONBIO, &x);
	auto closeFile = [=]() { return close(sock); };
#endif

	//送信
	DWORD head[2];
	head[0] = cmd->param;
	head[1] = cmd->dataSize;
	DWORD n;
#ifdef _WIN32
	if( WriteFile(pipe, head, sizeof(head), &n, NULL) == FALSE ){
#else
	if( send(sock, head, sizeof(head), 0) != (int)sizeof(head) ){
#endif
		closeFile();
		return CMD_ERR;
	}
	if( cmd->dataSize > 0 ){
#ifdef _WIN32
		if( WriteFile(pipe, cmd->data.get(), cmd->dataSize, &n, NULL) == FALSE ){
#else
		if( send(sock, cmd->data.get(), cmd->dataSize, 0) != (int)cmd->dataSize ){
#endif
			closeFile();
			return CMD_ERR;
		}
	}

	//受信
	n = 0;
#ifdef _WIN32
	for( DWORD m; n < sizeof(head) && ReadFile(pipe, (BYTE*)head + n, sizeof(head) - n, &m, NULL); n += m );
#else
	for( int m; n < sizeof(head) && (m = (int)recv(sock, (BYTE*)head + n, sizeof(head) - n, 0)) > 0; n += m );
#endif
	if( n != sizeof(head) ){
		closeFile();
		return CMD_ERR;
	}
	res->param = head[0];
	res->dataSize = head[1];
	if( res->dataSize > 0 ){
		res->data.reset(new BYTE[res->dataSize]);
		n = 0;
#ifdef _WIN32
		for( DWORD m; n < res->dataSize && ReadFile(pipe, res->data.get() + n, res->dataSize - n, &m, NULL); n += m );
#else
		for( int m; n < res->dataSize && (m = (int)recv(sock, res->data.get() + n, res->dataSize - n, 0)) > 0; n += m );
#endif
		if( n != res->dataSize ){
			closeFile();
			return CMD_ERR;
		}
	}
	closeFile();

	return res->param;
}

#if !defined(SEND_CTRL_CMD_NO_TCP) && defined(_WIN32)

int RecvAll(SOCKET sock, char* buf, int len, int flags)
{
	int n = 0;
	while( n < len ){
		int ret = recv(sock, buf + n, len - n, flags);
		if( ret < 0 ){
			return ret;
		}else if( ret <= 0 ){
			break;
		}
		n += ret;
	}
	return n;
}

DWORD SendTCP(const wstring& ip, DWORD port, DWORD timeOut, const CMD_STREAM* sendCmd, CMD_STREAM* resCmd)
{
	string ipA;
	WtoUTF8(ip, ipA);
	char szPort[16];
	sprintf_s(szPort, "%d", port);

	struct addrinfo hints = {};
	hints.ai_flags = AI_NUMERICHOST;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	struct addrinfo* result;
	if( getaddrinfo(ipA.c_str(), szPort, &hints, &result) != 0 ){
		return CMD_ERR_INVALID_ARG;
	}
	SOCKET sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if( sock != INVALID_SOCKET ){
		fd_set wmask;
		FD_ZERO(&wmask);
		FD_SET(sock, &wmask);
		struct timeval tv = {(long)(timeOut / 1000), 0};
		//ノンブロッキングモードで接続待ち
		unsigned long x = 1;
		if( ioctlsocket(sock, FIONBIO, &x) == SOCKET_ERROR ||
		    (connect(sock, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR &&
		     (WSAGetLastError() != WSAEWOULDBLOCK || select((int)sock + 1, NULL, &wmask, NULL, &tv) != 1)) ){
			closesocket(sock);
			sock = INVALID_SOCKET;
		}else{
			x = 0;
			ioctlsocket(sock, FIONBIO, &x);
			DWORD to = CSendCtrlCmd::SND_RCV_TIMEOUT;
			setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&to, sizeof(to));
			setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&to, sizeof(to));
		}
	}
	freeaddrinfo(result);

	if( sock == INVALID_SOCKET ){
		return CMD_ERR_CONNECT;
	}

	//送信
	DWORD head[256];
	head[0] = sendCmd->param;
	head[1] = sendCmd->dataSize;
	DWORD extSize = 0;
	if( sendCmd->dataSize > 0 ){
		extSize = min(sendCmd->dataSize, (DWORD)(sizeof(head) - sizeof(DWORD)*2));
		memcpy(head + 2, sendCmd->data.get(), extSize);
	}
	if( send(sock, (char*)head, sizeof(DWORD)*2 + extSize, 0) != (int)(sizeof(DWORD)*2 + extSize) ||
	    (sendCmd->dataSize > extSize &&
	     send(sock, (char*)sendCmd->data.get() + extSize, sendCmd->dataSize - extSize, 0) != (int)(sendCmd->dataSize - extSize)) ){
		closesocket(sock);
		return CMD_ERR;
	}
	//受信
	if( RecvAll(sock, (char*)head, sizeof(DWORD)*2, 0) != sizeof(DWORD)*2 ){
		closesocket(sock);
		return CMD_ERR;
	}
	resCmd->param = head[0];
	resCmd->dataSize = head[1];
	if( resCmd->dataSize > 0 ){
		resCmd->data.reset(new BYTE[resCmd->dataSize]);
		if( RecvAll(sock, (char*)resCmd->data.get(), resCmd->dataSize, 0) != (int)resCmd->dataSize ){
			closesocket(sock);
			return CMD_ERR;
		}
	}
	closesocket(sock);

	return resCmd->param;
}

#endif
}

DWORD CSendCtrlCmd::SendCmdStream(const CMD_STREAM* cmd, CMD_STREAM* res)
{
	DWORD ret = CMD_ERR;
	CMD_STREAM tmpRes;

	if( res == NULL ){
		res = &tmpRes;
	}
	if( this->tcpFlag == FALSE ){
		ret = SendPipe(this->pipeName, this->connectTimeOut, cmd, res);
	}
#if !defined(SEND_CTRL_CMD_NO_TCP) && defined(_WIN32)
	else{
		ret = SendTCP(this->sendIP, this->sendPort, this->connectTimeOut, cmd, res);
	}
#endif

	return ret;
}

