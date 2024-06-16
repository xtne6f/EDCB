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
		return UtilFileExists(fs_path(EDCB_INI_ROOT).append(this->pipeName)).first;
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
DWORD SendPipe(const wstring& pipeName, DWORD timeOut, const CCmdStream& cmd, CCmdStream* res)
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
	WtoUTF8(fs_path(EDCB_INI_ROOT).append(pipeName).native(), sockPath);
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
	DWORD n;
#ifdef _WIN32
	if( WriteFile(pipe, cmd.GetStream(), cmd.GetStreamSize(), &n, NULL) == FALSE ){
#else
	if( send(sock, cmd.GetStream(), cmd.GetStreamSize(), 0) != (int)cmd.GetStreamSize() ){
#endif
		closeFile();
		return CMD_ERR;
	}

	//受信
	BYTE head[8];
	n = 0;
#ifdef _WIN32
	for( DWORD m; n < sizeof(head) && ReadFile(pipe, head + n, sizeof(head) - n, &m, NULL); n += m );
#else
	for( int m; n < sizeof(head) && (m = (int)recv(sock, head + n, sizeof(head) - n, 0)) > 0; n += m );
#endif
	if( n != sizeof(head) ){
		closeFile();
		return CMD_ERR;
	}
	res->SetParam(head[0] | head[1] << 8 | head[2] << 16 | (DWORD)head[3] << 24);
	res->Resize(head[4] | head[5] << 8 | head[6] << 16 | (DWORD)head[7] << 24);
	n = 0;
#ifdef _WIN32
	for( DWORD m; n < res->GetDataSize() && ReadFile(pipe, res->GetData() + n, res->GetDataSize() - n, &m, NULL); n += m );
#else
	for( int m; n < res->GetDataSize() && (m = (int)recv(sock, res->GetData() + n, res->GetDataSize() - n, 0)) > 0; n += m );
#endif
	if( n != res->GetDataSize() ){
		closeFile();
		return CMD_ERR;
	}
	closeFile();

	return res->GetParam();
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

DWORD SendTCP(const wstring& ip, DWORD port, DWORD timeOut, const CCmdStream& cmd, CCmdStream* res)
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
	if( send(sock, (const char*)cmd.GetStream(), cmd.GetStreamSize(), 0) != (int)cmd.GetStreamSize() ){
		closesocket(sock);
		return CMD_ERR;
	}
	//受信
	BYTE head[8];
	if( RecvAll(sock, (char*)head, sizeof(head), 0) != (int)sizeof(head) ){
		closesocket(sock);
		return CMD_ERR;
	}
	res->SetParam(head[0] | head[1] << 8 | head[2] << 16 | (DWORD)head[3] << 24);
	res->Resize(head[4] | head[5] << 8 | head[6] << 16 | (DWORD)head[7] << 24);
	if( RecvAll(sock, (char*)res->GetData(), res->GetDataSize(), 0) != (int)res->GetDataSize() ){
		closesocket(sock);
		return CMD_ERR;
	}
	closesocket(sock);

	return res->GetParam();
}

#endif
}

DWORD CSendCtrlCmd::SendCmdStream(const CCmdStream& cmd, CCmdStream* res)
{
	DWORD ret = CMD_ERR;
	CCmdStream tmpRes;

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

