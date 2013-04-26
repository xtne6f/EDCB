#include "StdAfx.h"
#include "SendCtrlCmd.h"
/*
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "wsock32.lib")
#pragma comment(lib, "Ws2_32.lib")
*/
#include "StringUtil.h"

#include <Objbase.h>
#pragma comment(lib, "Ole32.lib")

CSendCtrlCmd::CSendCtrlCmd(void)
{
	CoInitialize(NULL);

	WSAData wsaData;
	WSAStartup(MAKEWORD(2,0), &wsaData);

	this->lockEvent = _CreateEvent(FALSE, TRUE, NULL);

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

	CoUninitialize();
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
	wstring eventName,
	wstring pipeName
	)
{
	if( Lock() == FALSE ) return ;
	this->eventName = eventName;
	this->pipeName = pipeName;
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

DWORD CSendCtrlCmd::SendAddloadReserve()
{
	return SendCmdWithoutData(CMD2_EPG_SRV_ADDLOAD_RESERVE);
}

DWORD CSendCtrlCmd::SendReloadEpg()
{
	return SendCmdWithoutData(CMD2_EPG_SRV_RELOAD_EPG);
}

DWORD CSendCtrlCmd::SendReloadSetting()
{
	return SendCmdWithoutData(CMD2_EPG_SRV_RELOAD_SETTING);
}

DWORD CSendCtrlCmd::SendClose()
{
	return SendCmdWithoutData(CMD2_EPG_SRV_CLOSE);
}

DWORD CSendCtrlCmd::SendRegistGUI(DWORD processID)
{
	return SendCmdData(CMD2_EPG_SRV_REGIST_GUI, processID);
}

DWORD CSendCtrlCmd::SendUnRegistGUI(DWORD processID)
{
	return SendCmdData(CMD2_EPG_SRV_UNREGIST_GUI, processID);
}

DWORD CSendCtrlCmd::SendRegistTCP(DWORD port)
{
	return SendCmdData(CMD2_EPG_SRV_REGIST_GUI_TCP, port);
}

DWORD CSendCtrlCmd::SendUnRegistTCP(DWORD port)
{
	return SendCmdData(CMD2_EPG_SRV_UNREGIST_GUI_TCP, port);
}

DWORD CSendCtrlCmd::SendEnumReserve(vector<RESERVE_DATA>* val)
{
	return ReceiveCmdData(CMD2_EPG_SRV_ENUM_RESERVE, val);
}

DWORD CSendCtrlCmd::SendGetReserve(DWORD reserveID, RESERVE_DATA* val)
{
	return SendAndReceiveCmdData(CMD2_EPG_SRV_GET_RESERVE, reserveID, val);
}

DWORD CSendCtrlCmd::SendAddReserve(vector<RESERVE_DATA>* val)
{
	return SendCmdData(CMD2_EPG_SRV_ADD_RESERVE, val);
}

DWORD CSendCtrlCmd::SendDelReserve(vector<DWORD>* val)
{
	return SendCmdData(CMD2_EPG_SRV_DEL_RESERVE, val);
}

DWORD CSendCtrlCmd::SendChgReserve(vector<RESERVE_DATA>* val)
{
	return SendCmdData(CMD2_EPG_SRV_CHG_RESERVE, val);
}

//チューナーごとの予約一覧を取得する
//戻り値：
// エラーコード
//引数：
// val				[IN]予約一覧
DWORD CSendCtrlCmd::SendEnumTunerReserve(vector<TUNER_RESERVE_INFO>* val)
{
	return ReceiveCmdData(CMD2_EPG_SRV_ENUM_TUNER_RESERVE, val);
}

//録画済み情報一覧取得
//戻り値：
// エラーコード
//引数：
// val			[OUT]録画済み情報一覧
DWORD CSendCtrlCmd::SendEnumRecInfo(
	vector<REC_FILE_INFO>* val
	)
{
	return ReceiveCmdData(CMD2_EPG_SRV_ENUM_RECINFO, val);
}

//録画済み情報を削除する
//戻り値：
// エラーコード
//引数：
// val				[IN]削除するID一覧
DWORD CSendCtrlCmd::SendDelRecInfo(vector<DWORD>* val)
{
	return SendCmdData(CMD2_EPG_SRV_DEL_RECINFO, val);
}

//サービス一覧を取得する
//戻り値：
// エラーコード
//引数：
// val				[OUT]サービス一覧
DWORD CSendCtrlCmd::SendEnumService(
	vector<EPGDB_SERVICE_INFO>* val
	)
{
	return ReceiveCmdData(CMD2_EPG_SRV_ENUM_SERVICE, val);
}

//サービス指定で番組情報を一覧を取得する
//戻り値：
// エラーコード
//引数：
// service			[IN]ONID<<32 | TSID<<16 | SIDとしたサービスID
// val				[OUT]番組情報一覧
DWORD CSendCtrlCmd::SendEnumPgInfo(
	ULONGLONG service,
	vector<EPGDB_EVENT_INFO*>* val
	)
{
	return SendAndReceiveCmdData(CMD2_EPG_SRV_ENUM_PG_INFO, service, val);
}

//指定イベントの番組情報を取得する
//戻り値：
// エラーコード
//引数：
// pgID				[IN]ONID<<48 | TSID<<32 | SID<<16 | EventIDとしたID
// val				[OUT]番組情報
DWORD CSendCtrlCmd::SendGetPgInfo(
	ULONGLONG pgID,
	EPGDB_EVENT_INFO* val
	)
{
	return SendAndReceiveCmdData(CMD2_EPG_SRV_GET_PG_INFO, pgID, val);
}

//指定キーワードで番組情報を検索する
//戻り値：
// エラーコード
//引数：
// key				[IN]検索キー（複数指定時はまとめて検索結果が返る）
// val				[OUT]番組情報一覧
DWORD CSendCtrlCmd::SendSearchPg(
	vector<EPGDB_SEARCH_KEY_INFO>* key,
	vector<EPGDB_EVENT_INFO*>* val
	)
{
	return SendAndReceiveCmdData(CMD2_EPG_SRV_SEARCH_PG, key, val);
}

//番組情報一覧を取得する
//戻り値：
// エラーコード
//引数：
// val				[OUT]番組情報一覧
DWORD CSendCtrlCmd::SendEnumPgAll(
	vector<EPGDB_SERVICE_EVENT_INFO*>* val
	)
{
	return ReceiveCmdData(CMD2_EPG_SRV_ENUM_PG_ALL, val);
}

//自動予約登録条件一覧を取得する
//戻り値：
// エラーコード
//引数：
// val			[OUT]条件一覧
DWORD CSendCtrlCmd::SendEnumEpgAutoAdd(
	vector<EPG_AUTO_ADD_DATA>* val
	)
{
	return ReceiveCmdData(CMD2_EPG_SRV_ENUM_AUTO_ADD, val);
}

//自動予約登録条件を追加する
//戻り値：
// エラーコード
//引数：
// val			[IN]条件一覧
DWORD CSendCtrlCmd::SendAddEpgAutoAdd(
	vector<EPG_AUTO_ADD_DATA>* val
	)
{
	return SendCmdData(CMD2_EPG_SRV_ADD_AUTO_ADD, val);
}

//自動予約登録条件を削除する
//戻り値：
// エラーコード
//引数：
// val			[IN]条件一覧
DWORD CSendCtrlCmd::SendDelEpgAutoAdd(
	vector<DWORD>* val
	)
{
	return SendCmdData(CMD2_EPG_SRV_DEL_AUTO_ADD, val);
}

//自動予約登録条件を変更する
//戻り値：
// エラーコード
//引数：
// val			[IN]条件一覧
DWORD CSendCtrlCmd::SendChgEpgAutoAdd(
	vector<EPG_AUTO_ADD_DATA>* val
	)
{
	return SendCmdData(CMD2_EPG_SRV_CHG_AUTO_ADD, val);
}

//自動予約登録条件一覧を取得する
//戻り値：
// エラーコード
//引数：
// val			[OUT]条件一覧	
DWORD CSendCtrlCmd::SendEnumManualAdd(
	vector<MANUAL_AUTO_ADD_DATA>* val
	)
{
	return ReceiveCmdData(CMD2_EPG_SRV_ENUM_MANU_ADD, val);
}

//自動予約登録条件を追加する
//戻り値：
// エラーコード
//引数：
// val			[IN]条件一覧
DWORD CSendCtrlCmd::SendAddManualAdd(
	vector<MANUAL_AUTO_ADD_DATA>* val
	)
{
	return SendCmdData(CMD2_EPG_SRV_ADD_MANU_ADD, val);
}

//プログラム予約自動登録の条件削除
//戻り値：
// エラーコード
//引数：
// val			[IN]条件一覧
DWORD CSendCtrlCmd::SendDelManualAdd(
	vector<DWORD>* val
	)
{
	return SendCmdData(CMD2_EPG_SRV_DEL_MANU_ADD, val);
}

//プログラム予約自動登録の条件変更
//戻り値：
// エラーコード
//引数：
// val			[IN]条件一覧
DWORD CSendCtrlCmd::SendChgManualAdd(
	vector<MANUAL_AUTO_ADD_DATA>* val
	)
{
	return SendCmdData(CMD2_EPG_SRV_CHG_MANU_ADD, val);
}


DWORD CSendCtrlCmd::SendChkSuspend()
{
	return SendCmdWithoutData(CMD2_EPG_SRV_CHK_SUSPEND);
}

DWORD CSendCtrlCmd::SendSuspend(
	WORD val
	)
{
	return SendCmdData(CMD2_EPG_SRV_SUSPEND, val);
}

DWORD CSendCtrlCmd::SendReboot()
{
	return SendCmdWithoutData(CMD2_EPG_SRV_REBOOT);
}

DWORD CSendCtrlCmd::SendEpgCapNow()
{
	return SendCmdWithoutData(CMD2_EPG_SRV_EPG_CAP_NOW);
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

//PlugInファイルの一覧を取得する
//戻り値：
// エラーコード
//引数：
// val			[IN]1:ReName、2:Write
// resVal		[OUT]ファイル名一覧
DWORD CSendCtrlCmd::SendEnumPlugIn(
	WORD val,
	vector<wstring>* resVal
	)
{
	return SendAndReceiveCmdData(CMD2_EPG_SRV_ENUM_PLUGIN, val, resVal);
}

//TVTestのチャンネル切り替え用の情報を取得する
//戻り値：
// エラーコード
//引数：
// val			[IN]ONID<<32 | TSID<<16 | SIDとしたサービスID
// resVal		[OUT]チャンネル情報
DWORD CSendCtrlCmd::SendGetChgChTVTest(
	ULONGLONG val,
	TVTEST_CH_CHG_INFO* resVal
	)
{
	return SendAndReceiveCmdData(CMD2_EPG_SRV_GET_CHG_CH_TVTEST, val, resVal);
}

//ネットワークモードのEpgDataCap_Bonのチャンネルを切り替え
//戻り値：
// エラーコード
//引数：
// chInfo				[OUT]チャンネル情報
DWORD CSendCtrlCmd::SendNwTVSetCh(
	SET_CH_INFO* val
	)
{
	return SendCmdData(CMD2_EPG_SRV_NWTV_SET_CH, val);
}

//ネットワークモードで起動中のEpgDataCap_Bonを終了
//戻り値：
// エラーコード
//引数：
// chInfo				[OUT]チャンネル情報
DWORD CSendCtrlCmd::SendNwTVClose(
	)
{
	return SendCmdWithoutData(CMD2_EPG_SRV_NWTV_CLOSE);
}

//ネットワークモードで起動するときのモード
//戻り値：
// エラーコード
//引数：
// val				[OUT]モード（1:UDP 2:TCP 3:UDP+TCP）
DWORD CSendCtrlCmd::SendNwTVMode(
	DWORD val
	)
{
	return SendCmdData(CMD2_EPG_SRV_NWTV_MODE, val);
}

//ストリーム配信用ファイルを開く
//戻り値：
// エラーコード
//引数：
// val				[IN]開くファイルのサーバー側ファイルパス
// resVal			[OUT]制御用CtrlID
DWORD CSendCtrlCmd::SendNwPlayOpen(
	wstring val,
	DWORD* resVal
	)
{
	return SendAndReceiveCmdData(CMD2_EPG_SRV_NWPLAY_OPEN, val, resVal);
}

//ストリーム配信用ファイルを閉じる
//戻り値：
// エラーコード
//引数：
// val				[IN]制御用CtrlID
DWORD CSendCtrlCmd::SendNwPlayClose(
	DWORD val
	)
{
	return SendCmdData(CMD2_EPG_SRV_NWPLAY_CLOSE, val);
}

//ストリーム配信開始
//戻り値：
// エラーコード
//引数：
// val				[IN]制御用CtrlID
DWORD CSendCtrlCmd::SendNwPlayStart(
	DWORD val
	)
{
	return SendCmdData(CMD2_EPG_SRV_NWPLAY_PLAY, val);
}

//ストリーム配信停止
//戻り値：
// エラーコード
//引数：
// val				[IN]制御用CtrlID
DWORD CSendCtrlCmd::SendNwPlayStop(
	DWORD val
	)
{
	return SendCmdData(CMD2_EPG_SRV_NWPLAY_STOP, val);
}

//ストリーム配信で現在の送信位置と総ファイルサイズを取得する
//戻り値：
// エラーコード
//引数：
// val				[IN/OUT]サイズ情報
DWORD CSendCtrlCmd::SendNwPlayGetPos(
	NWPLAY_POS_CMD* val
	)
{
	return SendAndReceiveCmdData(CMD2_EPG_SRV_NWPLAY_GET_POS, val, val);
}

//ストリーム配信で送信位置をシークする
//戻り値：
// エラーコード
//引数：
// val				[IN]サイズ情報
DWORD CSendCtrlCmd::SendNwPlaySetPos(
	NWPLAY_POS_CMD* val
	)
{
	return SendCmdData(CMD2_EPG_SRV_NWPLAY_SET_POS, val);
}

//ストリーム配信で送信先を設定する
//戻り値：
// エラーコード
//引数：
// val				[IN]サイズ情報
DWORD CSendCtrlCmd::SendNwPlaySetIP(
	NWPLAY_PLAY_INFO* val
	)
{
	return SendAndReceiveCmdData(CMD2_EPG_SRV_NWPLAY_SET_IP, val, val);
}

//ストリーム配信用ファイルをタイムシフトモードで開く
//戻り値：
// エラーコード
//引数：
// val				[IN]予約ID
// resVal			[OUT]ファイルパスとCtrlID
DWORD CSendCtrlCmd::SendNwTimeShiftOpen(
	DWORD val,
	NWPLAY_TIMESHIFT_INFO* resVal
	)
{
	return SendAndReceiveCmdData(CMD2_EPG_SRV_NWPLAY_TF_OPEN, val, resVal);
}

DWORD CSendCtrlCmd::SendEnumReserve2(vector<RESERVE_DATA>* val)
{
	return ReceiveCmdData2(CMD2_EPG_SRV_ENUM_RESERVE2, val);
}

DWORD CSendCtrlCmd::SendGetReserve2(DWORD reserveID, RESERVE_DATA* val)
{
	return SendAndReceiveCmdData2(CMD2_EPG_SRV_GET_RESERVE2, reserveID, val);
}

DWORD CSendCtrlCmd::SendAddReserve2(vector<RESERVE_DATA>* val)
{
	return SendCmdData2(CMD2_EPG_SRV_ADD_RESERVE2, val);
}

DWORD CSendCtrlCmd::SendChgReserve2(vector<RESERVE_DATA>* val)
{
	return SendCmdData2(CMD2_EPG_SRV_CHG_RESERVE2, val);
}

//予約追加が可能か確認する
//戻り値：
// エラーコード
//引数：
// val				[IN]予約情報
// resVal			[OUT]追加可能かのステータス
DWORD CSendCtrlCmd::SendAddChkReserve2(RESERVE_DATA* val, WORD* resVal)
{
	return SendAndReceiveCmdData2(CMD2_EPG_SRV_ADDCHK_RESERVE2, val, resVal);
}


//EPGデータファイルのタイムスタンプ取得
//戻り値：
// エラーコード
//引数：
// val				[IN]取得ファイル名
// resVal			[OUT]タイムスタンプ
DWORD CSendCtrlCmd::SendGetEpgFileTime2(wstring val, LONGLONG* resVal)
{
	return SendAndReceiveCmdData2(CMD2_EPG_SRV_GET_EPG_FILETIME2, val, resVal);
}

//EPGデータファイル取得
//戻り値：
// エラーコード
//引数：
// val			[IN]ファイル名
// resVal		[OUT]ファイルのバイナリデータ
// resValSize	[OUT]resValのサイズ
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

//自動予約登録条件一覧を取得する
//戻り値：
// エラーコード
//引数：
// val			[OUT]条件一覧
DWORD CSendCtrlCmd::SendEnumEpgAutoAdd2(
	vector<EPG_AUTO_ADD_DATA>* val
	)
{
	return ReceiveCmdData2(CMD2_EPG_SRV_ENUM_AUTO_ADD2, val);
}

//自動予約登録条件を追加する
//戻り値：
// エラーコード
//引数：
// val			[IN]条件一覧
DWORD CSendCtrlCmd::SendAddEpgAutoAdd2(
	vector<EPG_AUTO_ADD_DATA>* val
	)
{
	return SendCmdData2(CMD2_EPG_SRV_ADD_AUTO_ADD2, val);
}

//自動予約登録条件を変更する
//戻り値：
// エラーコード
//引数：
// val			[IN]条件一覧
DWORD CSendCtrlCmd::SendChgEpgAutoAdd2(
	vector<EPG_AUTO_ADD_DATA>* val
	)
{
	return SendCmdData2(CMD2_EPG_SRV_CHG_AUTO_ADD2, val);
}

//自動予約登録条件一覧を取得する
//戻り値：
// エラーコード
//引数：
// val			[OUT]条件一覧	
DWORD CSendCtrlCmd::SendEnumManualAdd2(
	vector<MANUAL_AUTO_ADD_DATA>* val
	)
{
	return ReceiveCmdData2(CMD2_EPG_SRV_ENUM_MANU_ADD2, val);
}

//自動予約登録条件を追加する
//戻り値：
// エラーコード
//引数：
// val			[IN]条件一覧
DWORD CSendCtrlCmd::SendAddManualAdd2(
	vector<MANUAL_AUTO_ADD_DATA>* val
	)
{
	return SendCmdData2(CMD2_EPG_SRV_ADD_MANU_ADD2, val);
}

//プログラム予約自動登録の条件変更
//戻り値：
// エラーコード
//引数：
// val			[IN]条件一覧
DWORD CSendCtrlCmd::SendChgManualAdd2(
	vector<MANUAL_AUTO_ADD_DATA>* val
	)
{
	return SendCmdData2(CMD2_EPG_SRV_CHG_MANU_ADD2, val);
}

//自動予約登録条件一覧を取得する
//戻り値：
// エラーコード
//引数：
// val			[OUT]条件一覧	
DWORD CSendCtrlCmd::SendEnumRecInfo2(
	vector<REC_FILE_INFO>* val
	)
{
	return ReceiveCmdData2(CMD2_EPG_SRV_ENUM_RECINFO2, val);
}

DWORD CSendCtrlCmd::SendChgProtectRecInfo2(vector<REC_FILE_INFO>* val)
{
	return SendCmdData2(CMD2_EPG_SRV_CHG_PROTECT_RECINFO2, val);
}

//ダイアログを前面に表示
//戻り値：
// エラーコード
DWORD CSendCtrlCmd::SendGUIShowDlg(
	)
{
	return SendCmdWithoutData(CMD2_TIMER_GUI_SHOW_DLG);
}

//予約一覧の情報が更新された
//戻り値：
// エラーコード
DWORD CSendCtrlCmd::SendGUIUpdateReserve(
	)
{
	return SendCmdWithoutData(CMD2_TIMER_GUI_UPDATE_RESERVE);
}

//EPGデータの再読み込みが完了した
//戻り値：
// エラーコード
DWORD CSendCtrlCmd::SendGUIUpdateEpgData(
	)
{
	return SendCmdWithoutData(CMD2_TIMER_GUI_UPDATE_EPGDATA);
}

DWORD CSendCtrlCmd::SendGUINotifyInfo2(NOTIFY_SRV_INFO* val)
{
	return SendCmdData2(CMD2_TIMER_GUI_SRV_STATUS_NOTIFY2, val);
}

//Viewアプリ（EpgDataCap_Bon.exe）を起動
//戻り値：
// エラーコード
//引数：
// exeCmd			[IN]コマンドライン
// PID				[OUT]起動したexeのPID
DWORD CSendCtrlCmd::SendGUIExecute(
	wstring exeCmd,
	DWORD* PID
	)
{
	return SendAndReceiveCmdData(CMD2_TIMER_GUI_VIEW_EXECUTE, exeCmd, PID);
}

//スタンバイ、休止、シャットダウンに入っていいかの確認をユーザーに行う
//戻り値：
// エラーコード
DWORD CSendCtrlCmd::SendGUIQuerySuspend(
	BYTE rebootFlag,
	BYTE suspendMode
	)
{
	return SendCmdData(CMD2_TIMER_GUI_QUERY_SUSPEND, (WORD)(rebootFlag<<8|suspendMode));
}

//PC再起動に入っていいかの確認をユーザーに行う
//戻り値：
// エラーコード
DWORD CSendCtrlCmd::SendGUIQueryReboot(
	BYTE rebootFlag
	)
{
	return SendCmdData(CMD2_TIMER_GUI_QUERY_REBOOT, (WORD)(rebootFlag<<8));
}

//サーバーのステータス変更通知
//戻り値：
// エラーコード
DWORD CSendCtrlCmd::SendGUIStatusChg(
	WORD status
	)
{
	return SendCmdData(CMD2_TIMER_GUI_SRV_STATUS_CHG, status);
}

//BonDriverの切り替え
//戻り値：
// エラーコード
//引数：
// bonDriver			[IN]BonDriverファイル名
DWORD CSendCtrlCmd::SendViewSetBonDrivere(
	wstring bonDriver
	)
{
	return SendCmdData(CMD2_VIEW_APP_SET_BONDRIVER, bonDriver);
}

//使用中のBonDriverのファイル名を取得
//戻り値：
// エラーコード
//引数：
// bonDriver			[OUT]BonDriverファイル名
DWORD CSendCtrlCmd::SendViewGetBonDrivere(
	wstring* bonDriver
	)
{
	return ReceiveCmdData(CMD2_VIEW_APP_GET_BONDRIVER, bonDriver);
}

//チャンネル切り替え
//戻り値：
// エラーコード
//引数：
// chInfo				[OUT]チャンネル情報
DWORD CSendCtrlCmd::SendViewSetCh(
	SET_CH_INFO* chInfo
	)
{
	return SendCmdData(CMD2_VIEW_APP_SET_CH, chInfo);
}

//放送波の時間とPC時間の誤差取得
//戻り値：
// エラーコード
//引数：
// delaySec				[OUT]誤差（秒）
DWORD CSendCtrlCmd::SendViewGetDelay(
	int* delaySec
	)
{
	return ReceiveCmdData(CMD2_VIEW_APP_GET_DELAY, delaySec);
}

//現在の状態を取得
//戻り値：
// エラーコード
//引数：
// status				[OUT]状態
DWORD CSendCtrlCmd::SendViewGetStatus(
	DWORD* status
	)
{
	return ReceiveCmdData(CMD2_VIEW_APP_GET_STATUS, status);
}

//現在の状態を取得
//戻り値：
// エラーコード
DWORD CSendCtrlCmd::SendViewAppClose(
	)
{
	return SendCmdWithoutData(CMD2_VIEW_APP_CLOSE);
}

//識別用IDの設定
//戻り値：
// エラーコード
//引数：
// id				[IN]ID
DWORD CSendCtrlCmd::SendViewSetID(
	int id
	)
{
	return SendCmdData(CMD2_VIEW_APP_SET_ID, id);
}

//識別用IDの取得
//戻り値：
// エラーコード
//引数：
// id				[OUT]ID
DWORD CSendCtrlCmd::SendViewGetID(
	int* id
	)
{
	return ReceiveCmdData(CMD2_VIEW_APP_GET_ID, id);
}

//予約録画用にGUIキープ
//戻り値：
// エラーコード
DWORD CSendCtrlCmd::SendViewSetStandbyRec(
	DWORD keepFlag
	)
{
	return SendCmdData(CMD2_VIEW_APP_SET_STANDBY_REC, keepFlag);
}

//ストリーム制御用コントロール作成
//戻り値：
// エラーコード
//引数：
// ctrlID				[OUT]制御ID
DWORD CSendCtrlCmd::SendViewCreateCtrl(
	DWORD* ctrlID
	)
{
	return ReceiveCmdData(CMD2_VIEW_APP_CREATE_CTRL, ctrlID);
}

//ストリーム制御用コントロール作成
//戻り値：
// エラーコード
//引数：
// ctrlID				[IN]制御ID
DWORD CSendCtrlCmd::SendViewDeleteCtrl(
	DWORD ctrlID
	)
{
	return SendCmdData(CMD2_VIEW_APP_DELETE_CTRL, ctrlID);
}

//制御コントロールの設定
//戻り値：
// エラーコード
//引数：
// val					[IN]設定値
DWORD CSendCtrlCmd::SendViewSetCtrlMode(
	SET_CTRL_MODE val
	)
{
	return SendCmdData(CMD2_VIEW_APP_SET_CTRLMODE, &val);
}

//録画処理開始
//戻り値：
// エラーコード
//引数：
// val					[IN]設定値
DWORD CSendCtrlCmd::SendViewStartRec(
	SET_CTRL_REC_PARAM val
	)
{
	return SendCmdData(CMD2_VIEW_APP_REC_START_CTRL, &val);
}

//録画処理開始
//戻り値：
// エラーコード
//引数：
// val					[IN]設定値
DWORD CSendCtrlCmd::SendViewStopRec(
	SET_CTRL_REC_STOP_PARAM val,
	SET_CTRL_REC_STOP_RES_PARAM* resVal
	)
{
	return SendAndReceiveCmdData(CMD2_VIEW_APP_REC_STOP_CTRL, &val, resVal);
}

//録画中のファイルパスを取得
//戻り値：
// エラーコード
//引数：
// val					[OUT]ファイルパス
DWORD CSendCtrlCmd::SendViewGetRecFilePath(
	DWORD ctrlID,
	wstring* resVal
	)
{
	return SendAndReceiveCmdData(CMD2_VIEW_APP_REC_FILE_PATH, ctrlID, resVal);
}

//録画処理開始
//戻り値：
// エラーコード
DWORD CSendCtrlCmd::SendViewStopRecAll(
	)
{
	return SendCmdWithoutData(CMD2_VIEW_APP_REC_STOP_ALL);
}

//ファイル出力したサイズを取得
//戻り値：
// エラーコード
//引数：
// resVal					[OUT]ファイル出力したサイズ
DWORD CSendCtrlCmd::SendViewGetWriteSize(
	DWORD ctrlID,
	__int64* resVal
	)
{
	return SendAndReceiveCmdData(CMD2_VIEW_APP_REC_WRITE_SIZE, ctrlID, resVal);
}

//EPG取得開始
//戻り値：
// エラーコード
//引数：
// val					[IN]取得チャンネルリスト
DWORD CSendCtrlCmd::SendViewEpgCapStart(
	vector<SET_CH_INFO>* val
	)
{
	return SendCmdData(CMD2_VIEW_APP_EPGCAP_START, val);
}

//EPG取得キャンセル
//戻り値：
// エラーコード
DWORD CSendCtrlCmd::SendViewEpgCapStop(
	)
{
	return SendCmdWithoutData(CMD2_VIEW_APP_EPGCAP_STOP);
}

//EPGデータの検索
//戻り値：
// エラーコード
// val					[IN]取得番組
// resVal				[OUT]番組情報
DWORD CSendCtrlCmd::SendViewSearchEvent(
	SEARCH_EPG_INFO_PARAM* val,
	EPGDB_EVENT_INFO* resVal
	)
{
	return SendAndReceiveCmdData(CMD2_VIEW_APP_SEARCH_EVENT, val, resVal);
}

//現在or次の番組情報を取得する
//戻り値：
// エラーコード
// val					[IN]取得番組
// resVal				[OUT]番組情報
DWORD CSendCtrlCmd::SendViewGetEventPF(
	GET_EPG_PF_INFO_PARAM* val,
	EPGDB_EVENT_INFO* resVal
	)
{
	return SendAndReceiveCmdData(CMD2_VIEW_APP_GET_EVENT_PF, val, resVal);
}

//Viewボタン登録アプリ起動
//戻り値：
// エラーコード
DWORD CSendCtrlCmd::SendViewExecViewApp(
	)
{
	return SendCmdWithoutData(CMD2_VIEW_APP_EXEC_VIEW_APP);
}

//ストリーミング配信制御IDの設定
//戻り値：
// エラーコード
DWORD CSendCtrlCmd::SendViewSetStreamingInfo(
	TVTEST_STREAMING_INFO* val
	)
{
	return SendCmdData(CMD2_VIEW_APP_TT_SET_CTRL, val);
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

