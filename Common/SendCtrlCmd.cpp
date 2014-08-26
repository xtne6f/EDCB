#include "StdAfx.h"
#include "SendCtrlCmd.h"
/*
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "wsock32.lib")
#pragma comment(lib, "Ws2_32.lib")
*/
#include "StringUtil.h"

CSendCtrlCmd::CSendCtrlCmd(void)
{
	WSAData wsaData;
	WSAStartup(MAKEWORD(2,0), &wsaData);

	this->lockEvent = CreateEvent(NULL, FALSE, TRUE, NULL);

	this->tcpFlag = FALSE;
	this->connectTimeOut = CONNECT_TIMEOUT;

	this->pipeName = CMD2_EPG_SRV_PIPE;
	this->eventName = CMD2_EPG_SRV_EVENT_WAIT_CONNECT;

	this->ip = L"127.0.0.1";
	this->port = 5678;

}


CSendCtrlCmd::~CSendCtrlCmd(void)
{
	if( this->lockEvent != NULL ){
		UnLock();
		CloseHandle(this->lockEvent);
		this->lockEvent = NULL;
	}
	WSACleanup();
}

BOOL CSendCtrlCmd::Lock(LPCWSTR log, DWORD timeOut)
{
	if( this->lockEvent == NULL ){
		return FALSE;
	}
	if( log != NULL ){
		OutputDebugString(log);
	}
	DWORD dwRet = WaitForSingleObject(this->lockEvent, timeOut);
	if( dwRet == WAIT_ABANDONED || 
		dwRet == WAIT_FAILED){
		return FALSE;
	}
	return TRUE;
}

void CSendCtrlCmd::UnLock(LPCWSTR log)
{
	if( this->lockEvent != NULL ){
		SetEvent(this->lockEvent);
	}
	if( log != NULL ){
		OutputDebugString(log);
	}
}

//コマンド送信方法の設定
//引数：
// tcpFlag		[IN] TRUE：TCP/IPモード、FALSE：名前付きパイプモード
void CSendCtrlCmd::SetSendMode(
	BOOL tcpFlag
	)
{
	if( Lock() == FALSE ) return ;
	this->tcpFlag = tcpFlag;
	UnLock();
}

//名前付きパイプモード時の接続先を設定
//EpgTimerSrv.exeに対するコマンドは設定しなくても可（デフォルト値になっている）
//引数：
// eventName	[IN]排他制御用Eventの名前
// pipeName		[IN]接続パイプの名前
void CSendCtrlCmd::SetPipeSetting(
	LPCWSTR eventName,
	LPCWSTR pipeName
	)
{
	if( Lock() == FALSE ) return ;
	this->eventName = eventName;
	this->pipeName = pipeName;
	UnLock();
}

//名前付きパイプモード時の接続先を設定（接尾にプロセスIDを伴うタイプ）
//引数：
// pid			[IN]プロセスID
void CSendCtrlCmd::SetPipeSetting(
	LPCWSTR eventName,
	LPCWSTR pipeName,
	DWORD pid
	)
{
	if( Lock() == FALSE ) return ;
	Format(this->eventName, L"%s%d", eventName, pid);
	Format(this->pipeName, L"%s%d", pipeName, pid);
	UnLock();
}

//TCP/IPモード時の接続先を設定
//引数：
// ip			[IN]接続先IP
// port			[IN]接続先ポート
void CSendCtrlCmd::SetNWSetting(
	wstring ip,
	DWORD port
	)
{
	if( Lock() == FALSE ) return ;
	this->ip = ip;
	this->port = port;
	UnLock();
}

//接続処理時のタイムアウト設定
// timeOut		[IN]タイムアウト値（単位：ms）
void CSendCtrlCmd::SetConnectTimeOut(
	DWORD timeOut
	)
{
	if( Lock() == FALSE ) return ;
	this->connectTimeOut = timeOut;
	UnLock();
}

DWORD CSendCtrlCmd::SendPipe(LPCWSTR pipeName, LPCWSTR eventName, DWORD timeOut, CMD_STREAM* send, CMD_STREAM* res)
{
	if( pipeName == NULL || eventName == NULL || send == NULL || res == NULL ){
		return CMD_ERR_INVALID_ARG;
	}

	//接続待ち
	HANDLE waitEvent = _CreateEvent(FALSE, FALSE, eventName);
	if( waitEvent == NULL ){
		return CMD_ERR;
	}
	if(WaitForSingleObject(waitEvent, timeOut) != WAIT_OBJECT_0){
		CloseHandle(waitEvent);
		return CMD_ERR_TIMEOUT;
	}
	CloseHandle(waitEvent);

	//接続
	HANDLE pipe = _CreateFile( pipeName, GENERIC_READ|GENERIC_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if( pipe == INVALID_HANDLE_VALUE ){
		_OutputDebugString(L"*+* ConnectPipe Err:%d\r\n", GetLastError());
		return CMD_ERR_CONNECT;
	}

	DWORD write = 0;
	DWORD read = 0;

	//送信
	DWORD head[2];
	head[0] = send->param;
	head[1] = send->dataSize;
	if( WriteFile(pipe, head, sizeof(DWORD)*2, &write, NULL ) == FALSE ){
		CloseHandle(pipe);
		return CMD_ERR;
	}
	if( send->dataSize > 0 ){
		if( send->data == NULL ){
			CloseHandle(pipe);
			return CMD_ERR_INVALID_ARG;
		}
		DWORD sendNum = 0;
		while(sendNum < send->dataSize ){
			DWORD sendSize = 0;
			if( send->dataSize - sendNum < CMD2_SEND_BUFF_SIZE ){
				sendSize = send->dataSize - sendNum;
			}else{
				sendSize = CMD2_SEND_BUFF_SIZE;
			}
			if( WriteFile(pipe, send->data + sendNum, sendSize, &write, NULL ) == FALSE ){
				CloseHandle(pipe);
				return CMD_ERR;
			}
			sendNum += write;
		}
	}

	//受信
	if( ReadFile(pipe, head, sizeof(DWORD)*2, &read, NULL ) == FALSE ){
		CloseHandle(pipe);
		return CMD_ERR;
	}
	res->param = head[0];
	res->dataSize = head[1];
	if( res->dataSize > 0 ){
		res->data = new BYTE[res->dataSize];
		DWORD readNum = 0;
		while(readNum < res->dataSize ){
			DWORD readSize = 0;
			if( res->dataSize - readNum < CMD2_RES_BUFF_SIZE ){
				readSize = res->dataSize - readNum;
			}else{
				readSize = CMD2_RES_BUFF_SIZE;
			}
			if( ReadFile(pipe, res->data + readNum, readSize, &read, NULL ) == FALSE ){
				CloseHandle(pipe);
				return CMD_ERR;
			}
			readNum += read;
		}
	}
	CloseHandle(pipe);

	return res->param;
}

DWORD CSendCtrlCmd::SendTCP(wstring ip, DWORD port, DWORD timeOut, CMD_STREAM* sendCmd, CMD_STREAM* resCmd)
{
	if( sendCmd == NULL || resCmd == NULL ){
		return CMD_ERR_INVALID_ARG;
	}

	struct sockaddr_in server;
	SOCKET sock;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	server.sin_family = AF_INET;
	server.sin_port = htons((WORD)port);
	string strA = "";
	WtoA(ip, strA);
	server.sin_addr.S_un.S_addr = inet_addr(strA.c_str());
	DWORD socketBuffSize = 1024*1024;
	setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (const char*)&socketBuffSize, sizeof(socketBuffSize));
	setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (const char*)&socketBuffSize, sizeof(socketBuffSize));

	int ret = connect(sock, (struct sockaddr *)&server, sizeof(server));
	if( ret == SOCKET_ERROR ){
		int a= GetLastError();
		wstring aa;
		Format(aa,L"%d",a);
		OutputDebugString(aa.c_str());
		closesocket(sock);
		return CMD_ERR_CONNECT;
	}

	DWORD read = 0;
	//送信
	DWORD head[2];
	head[0] = sendCmd->param;
	head[1] = sendCmd->dataSize;
	send(sock, (char*)head, sizeof(DWORD)*2, 0 );
	if( ret == SOCKET_ERROR ){
		closesocket(sock);
		return CMD_ERR;
	}
	if( sendCmd->dataSize > 0 ){
		if( sendCmd->data == NULL ){
			closesocket(sock);
			return CMD_ERR_INVALID_ARG;
		}
		ret = send(sock, (char*)sendCmd->data, sendCmd->dataSize, 0 );
		if( ret == SOCKET_ERROR ){
			closesocket(sock);
			return CMD_ERR;
		}
	}
	//受信
	ret = recv(sock, (char*)head, sizeof(DWORD)*2, 0 );
	if( ret == SOCKET_ERROR ){
		closesocket(sock);
		return CMD_ERR;
	}
	resCmd->param = head[0];
	resCmd->dataSize = head[1];
	if( resCmd->dataSize > 0 ){
		resCmd->data = new BYTE[resCmd->dataSize];
		read = 0;
		while(ret>0){
			ret = recv(sock, (char*)(resCmd->data + read), resCmd->dataSize - read, 0);
			if( ret == SOCKET_ERROR ){
				closesocket(sock);
				return CMD_ERR;
			}else if( ret == 0 ){
				break;
			}
			read += ret;
			if( read >= resCmd->dataSize ){
				break;
			}
		}
	}
	closesocket(sock);

	return resCmd->param;
}

DWORD CSendCtrlCmd::SendFileCopy(
	wstring val,
	BYTE** resVal,
	DWORD* resValSize
	)
{
	CMD_STREAM res;
	DWORD ret = SendCmdData(CMD2_EPG_SRV_FILE_COPY, val, &res);

	if( ret == CMD_SUCCESS ){
		if( res.dataSize == 0 ){
			return CMD_ERR;
		}
		*resValSize = res.dataSize;
		*resVal = new BYTE[res.dataSize];
		memcpy(*resVal, res.data, res.dataSize);
	}
	return ret;
}

DWORD CSendCtrlCmd::SendGetEpgFile2(
	wstring val,
	BYTE** resVal,
	DWORD* resValSize
	)
{
	CMD_STREAM res;
	DWORD ret = SendCmdData2(CMD2_EPG_SRV_GET_EPG_FILE2, val, &res);

	if( ret == CMD_SUCCESS ){
		WORD ver = 0;
		DWORD readSize = 0;
		if( ReadVALUE(&ver, res.data, res.dataSize, &readSize) == FALSE || res.dataSize <= readSize ){
			return CMD_ERR;
		}
		*resValSize = res.dataSize - readSize;
		*resVal = new BYTE[*resValSize];
		memcpy(*resVal, res.data + readSize, *resValSize);
	}
	return ret;
}

DWORD CSendCtrlCmd::SendCmdStream(CMD_STREAM* send, CMD_STREAM* res)
{
	if( Lock() == FALSE ) return CMD_ERR_TIMEOUT;
	DWORD ret = CMD_ERR;
	CMD_STREAM tmpRes;

	if( res == NULL ){
		res = &tmpRes;
	}
	if( this->tcpFlag == FALSE ){
		ret = SendPipe(this->pipeName.c_str(), this->eventName.c_str(), this->connectTimeOut, send, res);
	}else{
		ret = SendTCP(this->ip, this->port, this->connectTimeOut, send, res);
	}

	UnLock();
	return ret;
}

