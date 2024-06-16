#include "stdafx.h"
#include "SendTSTCPMain.h"
#include "../../Common/StringUtil.h"
#include "../../Common/TimeUtil.h"
#ifndef _WIN32
#include "../../Common/PathUtil.h"
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

namespace
{
//SendTSTCPプロトコルのヘッダの送信を抑制する既定のポート範囲
const DWORD SEND_TS_TCP_NOHEAD_PORT_MIN = 22000;
const DWORD SEND_TS_TCP_NOHEAD_PORT_MAX = 22999;

#ifdef _WIN32
//送信先が0.0.0.1のとき待ち受ける名前付きパイプ名
const WCHAR SEND_TS_TCP_0001_PIPE_NAME[] = L"\\\\.\\pipe\\SendTSTCP_%d_%u";

//送信先が0.0.0.2のとき開く名前付きパイプ名
const WCHAR SEND_TS_TCP_0002_PIPE_NAME[] = L"\\\\.\\pipe\\BonDriver_Pipe%02d";
#else
//送信先が0.0.0.1のとき待ち受けるFIFOファイル名
const WCHAR SEND_TS_TCP_0001_FIFO_NAME[] = L"SendTSTCP_%d_%d_%d.fifo";
const WCHAR SEND_TS_TCP_0001_FIFO_BASE[] = L"SendTSTCP_";
const WCHAR SEND_TS_TCP_0001_FIFO_SUFFIX_PATTERN[] = L"*_*_?.fifo";

//送信先(FIFOファイル)を開くためのポーリング間隔
const DWORD SEND_TS_FIFO_OPEN_INTERVAL_MSEC = 200;
#endif

//送信バッファの最大数(サイズはAddSendData()の入力に依存)
const DWORD SEND_TS_TCP_BUFF_MAX = 500;

//送信バッファの要素1つあたりの最大サイズ
const DWORD SEND_TS_TCP_BUFF_ITEM_SIZE_MAX = 48128;

//送信バッファのうち先行書き込み可能な最大数
//(複数送信先の送信タイミングの同期によりパフォーマンスが低下するのを避ける)
const DWORD SEND_TS_TCP_WRITE_AHEAD_MAX = SEND_TS_TCP_BUFF_MAX / 4;

//送信先(サーバ)接続のためのポーリング間隔
const DWORD SEND_TS_TCP_CONNECT_INTERVAL_MSEC = 2000;

//送信バッファ数がこの範囲内にあるとき、UDP送信に待ちバッファ数に反比例するウェイトを置く
const DWORD SEND_TS_UDP_RATE_CONTROL_MAX = SEND_TS_TCP_BUFF_MAX / 4;

SOCKET CreateNonBlockingSocket(int af, int type, int protocol)
{
#ifdef _WIN32
	SOCKET sock = socket(af, type, protocol);
	if( sock != INVALID_SOCKET ){
		//ノンブロッキングモードへ
		unsigned long x = 1;
		if( ioctlsocket(sock, FIONBIO, &x) == 0 ){
#else
	SOCKET sock = socket(af, type | SOCK_CLOEXEC | SOCK_NONBLOCK, protocol);
	if( sock != INVALID_SOCKET ){
		//select()を使うので一応検査しておく
		if( sock < FD_SETSIZE ){
#endif
			return sock;
		}
		closesocket(sock);
	}
	return INVALID_SOCKET;
}
}

CSendTSTCPMain::CSendTSTCPMain(void)
{
#ifdef _WIN32
	m_wsaStartupResult = -1;
#endif
	m_bSendingToSomeone = false;
}

CSendTSTCPMain::~CSendTSTCPMain(void)
{
	StopSend();
	ClearSendAddr();

#ifdef _WIN32
	if( m_wsaStartupResult == 0 ){
		WSACleanup();
	}
#endif
}

//送信先を追加
//戻り値：エラーコード
DWORD CSendTSTCPMain::AddSendAddr(
	LPCWSTR lpcwszIP,
	DWORD dwPort
	)
{
	if( lpcwszIP == NULL || !lpcwszIP[0] ){
		return FALSE;
	}
	SEND_INFO item = {};
	WtoUTF8(lpcwszIP, item.strIP);
	item.port = (WORD)dwPort;
	item.bSuppressHeader = (dwPort & 0x10000) != 0 || (SEND_TS_TCP_NOHEAD_PORT_MIN <= dwPort && dwPort <= SEND_TS_TCP_NOHEAD_PORT_MAX);
	item.sock = INVALID_SOCKET;
	for( size_t i = 0; i < array_size(item.pipe); i++ ){
#ifdef _WIN32
		item.pipe[i] = INVALID_HANDLE_VALUE;
#else
		item.pipe[i] = -1;
#endif
	}

#ifdef _WIN32
	//名前付きパイプでなければ
	if( item.strIP != "0.0.0.1" && item.strIP != "0.0.0.2" ){
		if( m_wsaStartupResult == -1 ){
			WSAData wsaData;
			m_wsaStartupResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		}
		if( m_wsaStartupResult != 0 ){
			return FALSE;
		}
	}
#endif

	lock_recursive_mutex lock(m_sendLock);
	if( std::find_if(m_SendList.begin(), m_SendList.end(), [&item](const SEND_INFO& a) {
	        return a.strIP == item.strIP && a.port == item.port; }) == m_SendList.end() ){
		m_SendList.push_back(item);
	}

	return TRUE;
}

//送信先を追加(UDP)
//戻り値：エラーコード
DWORD CSendTSTCPMain::AddSendAddrUdp(
	LPCWSTR lpcwszIP,
	DWORD dwPort,
	BOOL broadcastFlag,
	int maxSendSize
	)
{
	if( lpcwszIP == NULL ){
		return FALSE;
	}
#ifdef _WIN32
	if( m_wsaStartupResult == -1 ){
		WSAData wsaData;
		m_wsaStartupResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	}
	if( m_wsaStartupResult != 0 ){
		return FALSE;
	}
#endif

	SEND_INFO item = {};
	item.bSuppressHeader = true;
	item.sock = INVALID_SOCKET;
	for( size_t i = 0; i < array_size(item.pipe); i++ ){
#ifdef _WIN32
		item.pipe[i] = INVALID_HANDLE_VALUE;
#else
		item.pipe[i] = -1;
#endif
	}
	string ipA;
	WtoUTF8(lpcwszIP, ipA);
	char szPort[16];
	sprintf_s(szPort, "%d", (WORD)dwPort);
	struct addrinfo hints = {};
	hints.ai_flags = AI_NUMERICHOST;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	struct addrinfo* result;
	if( getaddrinfo(ipA.c_str(), szPort, &hints, &result) != 0 ){
		return FALSE;
	}
	item.udpAddrlen = result->ai_addrlen;
	if( item.udpAddrlen <= sizeof(item.udpAddr) ){
		std::copy((const BYTE*)result->ai_addr, (const BYTE*)result->ai_addr + item.udpAddrlen, (BYTE*)&item.udpAddr);
		item.sock = CreateNonBlockingSocket(result->ai_family, result->ai_socktype, result->ai_protocol);
	}
	freeaddrinfo(result);
	if( item.sock == INVALID_SOCKET ){
		return FALSE;
	}

	if( broadcastFlag ){
		BOOL b = TRUE;
		setsockopt(item.sock, SOL_SOCKET, SO_BROADCAST, (const char*)&b, sizeof(b));
	}
	item.udpMaxSendSize = maxSendSize;

	lock_recursive_mutex lock(m_sendLock);
	m_SendList.push_back(item);

	return TRUE;
}

//送信先クリア
//戻り値：エラーコード
DWORD CSendTSTCPMain::ClearSendAddr(
	)
{
	bool bStarted = m_sendThread.joinable();
	if( bStarted ){
		StopSend();
	}
	while( m_SendList.empty() == false ){
		//TCP系は切断済み。UDPも閉じる
		if( m_SendList.back().sock != INVALID_SOCKET ){
			closesocket(m_SendList.back().sock);
		}
		m_SendList.pop_back();
	}
	if( bStarted ){
		StartSend();
	}
	return TRUE;
}

//データ送信を開始
//戻り値：エラーコード
DWORD CSendTSTCPMain::StartSend(
	)
{
	if( m_sendThread.joinable() ){
		return FALSE;
	}

	m_stopSendEvent.Reset();
	m_sendThread = thread_(SendThread, this);

	return TRUE;
}

//データ送信を停止
//戻り値：エラーコード
DWORD CSendTSTCPMain::StopSend(
	)
{
	if( m_sendThread.joinable() ){
		m_stopSendEvent.Set();
		m_sendThread.join();
	}

	return TRUE;
}

//データ送信を開始
//戻り値：エラーコード
DWORD CSendTSTCPMain::AddSendData(
	BYTE* pbData,
	DWORD dwSize
	)
{
	if( m_sendThread.joinable() ){
		lock_recursive_mutex lock(m_sendLock);
		if( m_bSendingToSomeone ){
			while( dwSize ){
				//大きすぎるときは均等に分ける
				DWORD divCount = (dwSize + SEND_TS_TCP_BUFF_ITEM_SIZE_MAX - 1) / SEND_TS_TCP_BUFF_ITEM_SIZE_MAX;
				DWORD itemSize = min((dwSize + 187) / divCount * 188, dwSize);
				m_TSBuff.push_back(vector<BYTE>());
				m_TSBuff.back().reserve(sizeof(DWORD) * 2 + itemSize);
				m_TSBuff.back().resize(sizeof(DWORD) * 2);
				m_TSBuff.back().insert(m_TSBuff.back().end(), pbData, pbData + itemSize);
				pbData += itemSize;
				dwSize -= itemSize;
				if( m_TSBuff.size() > SEND_TS_TCP_BUFF_MAX ){
					ClearSendBuff();
				}
			}
		}
	}
	return TRUE;
}

//送信バッファをクリア
//戻り値：エラーコード
DWORD CSendTSTCPMain::ClearSendBuff(
	)
{
	lock_recursive_mutex lock(m_sendLock);

	DWORD unerasableCount = 0;
	for( auto itr = m_SendList.begin(); itr != m_SendList.end(); itr++ ){
		for( size_t i = 0; i < array_size(itr->pipe); i++ ){
			unerasableCount = std::max(itr->writeAheadCount[i], unerasableCount);
		}
	}
	if( m_TSBuff.size() > unerasableCount ){
		m_TSBuff.resize(unerasableCount);
	}
	return TRUE;
}

void CSendTSTCPMain::SendThread(CSendTSTCPMain* pSys)
{
#ifndef _WIN32
	//異常終了などで残ったFIFOファイルがあれば削除する
	EnumFindFile(fs_path(EDCB_INI_ROOT).append(SEND_TS_TCP_0001_FIFO_BASE).concat(SEND_TS_TCP_0001_FIFO_SUFFIX_PATTERN),
	             [](UTIL_FIND_DATA& findData) -> bool {
		if( findData.fileName.size() > wcslen(SEND_TS_TCP_0001_FIFO_BASE) ){
			WCHAR* endp;
			wcstol(findData.fileName.c_str() + wcslen(SEND_TS_TCP_0001_FIFO_BASE), &endp, 10);
			if( *endp == L'_' ){
				int pid = wcstol(endp + 1, NULL, 10);
				if( pid > 0 && kill(pid, 0) == -1 && errno == ESRCH ){
					AddDebugLogFormat(L"Delete remaining %ls", findData.fileName.c_str());
					DeleteFile(fs_path(EDCB_INI_ROOT).append(findData.fileName).c_str());
				}
			}
		}
		return true;
	});
#endif

	//ヘッダのdwCount情報を3バイト目が0でない値で始める。原作は0で始めていたが仕様的に始点に意味はなく
	//また他のTCPインタフェースのヘッダと区別しにくいため設定を誤った場合に想定外のことが起きるのを防ぐため
	DWORD dwCount = 0x01000000;
	DWORD dwCheckConnectTick = GetU32Tick();
#ifdef _WIN32
	vector<HANDLE> olEventList;
#else
	DWORD dwCheckFifoOpenTick = dwCheckConnectTick;
	vector<pollfd> pfdList;
#endif
	int udpWaitState = 0;

	for(;;){
		DWORD tick = GetU32Tick();
		bool bCheckConnect = tick - dwCheckConnectTick > SEND_TS_TCP_CONNECT_INTERVAL_MSEC;
		if( bCheckConnect ){
			dwCheckConnectTick = tick;
		}
#ifndef _WIN32
		bool bCheckFifoOpen = tick - dwCheckFifoOpenTick > SEND_TS_FIFO_OPEN_INTERVAL_MSEC;
		if( bCheckFifoOpen ){
			dwCheckFifoOpenTick = tick;
		}
#endif
		bool bSendingToSomeone = false;
		std::list<SEND_INFO>::iterator itr;
		for( size_t itrIndex = 0;; itrIndex++ ){
			{
				lock_recursive_mutex lock(pSys->m_sendLock);
				if( itrIndex == 0 ){
					itr = pSys->m_SendList.begin();
				}else{
					itr++;
				}
				if( itr == pSys->m_SendList.end() ){
					break;
				}
			}
			if( itr->strIP == "0.0.0.1" ){
				//サーバとして名前付きパイプで待ち受け
				//クライアントが短時間で切断→接続する場合のために複数インスタンス作る
				for( size_t i = 0; i < array_size(itr->pipe); i++ ){
#ifdef _WIN32
					if( itr->olEvent[i] == NULL ){
						itr->olEvent[i] = CreateEvent(NULL, TRUE, FALSE, NULL);
						if( itr->olEvent[i] ){
							wstring strPipe;
							Format(strPipe, SEND_TS_TCP_0001_PIPE_NAME, itr->port, GetCurrentProcessId());
							itr->pipe[i] = CreateNamedPipe(strPipe.c_str(), PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED,
							                               0, (DWORD)array_size(itr->pipe), 48128, 0, 0, NULL);
							if( itr->pipe[i] != INVALID_HANDLE_VALUE ){
								OVERLAPPED olZero = {};
								itr->ol[i] = olZero;
								itr->ol[i].hEvent = itr->olEvent[i];
								if( ConnectNamedPipe(itr->pipe[i], itr->ol + i) == FALSE ){
									DWORD err = GetLastError();
									if( err == ERROR_PIPE_CONNECTED ){
										itr->bConnect[i] = true;
										itr->bPipeWriting[i] = false;
									}else if( err != ERROR_IO_PENDING ){
										CloseHandle(itr->pipe[i]);
										itr->pipe[i] = INVALID_HANDLE_VALUE;
									}
								}
							}
						}
					}
					if( itr->pipe[i] != INVALID_HANDLE_VALUE ){
						if( itr->bConnect[i] == false ){
							if( WaitForSingleObject(itr->olEvent[i], 0) == WAIT_OBJECT_0 ){
								itr->bConnect[i] = true;
								itr->bPipeWriting[i] = false;
							}
						}
					}
#else
					if( bCheckFifoOpen && itr->bConnect[i] == false ){
						if( itr->strPipe[i].empty() ){
							WCHAR name[array_size(SEND_TS_TCP_0001_FIFO_NAME) + 32];
							swprintf_s(name, SEND_TS_TCP_0001_FIFO_NAME, itr->port, (int)getpid(), (int)i);
							WtoUTF8(fs_path(EDCB_INI_ROOT).append(name).native(), itr->strPipe[i]);
						}
						itr->pipe[i] = open(itr->strPipe[i].c_str(), O_WRONLY | O_NONBLOCK | O_CLOEXEC);
						if( itr->pipe[i] >= 0 ){
							itr->bConnect[i] = true;
							itr->wroteBytes[i] = 0;
						}else if( errno == ENOENT ){
							mkfifo(itr->strPipe[i].c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
						}
					}
#endif
					bSendingToSomeone = bSendingToSomeone || itr->bConnect[i];
				}
				udpWaitState = -1;
			}
#ifdef _WIN32
			else if( itr->strIP == "0.0.0.2" ){
				if( bCheckConnect ){
					//クライアントとして名前付きパイプを開く
					if( itr->olEvent[0] == NULL ){
						itr->olEvent[0] = CreateEvent(NULL, TRUE, FALSE, NULL);
					}
					if( itr->olEvent[0] && itr->pipe[0] == INVALID_HANDLE_VALUE ){
						wstring strPipe;
						Format(strPipe, SEND_TS_TCP_0002_PIPE_NAME, itr->port);
						itr->pipe[0] = CreateFile(strPipe.c_str(), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
						if( itr->pipe[0] != INVALID_HANDLE_VALUE ){
							itr->bConnect[0] = true;
							itr->bPipeWriting[0] = false;
						}
					}
				}
				bSendingToSomeone = bSendingToSomeone || itr->bConnect[0];
				udpWaitState = -1;
			}
#endif
			else if( itr->strIP.empty() ){
				//UDP送信
				bSendingToSomeone = true;
			}else{
				if( bCheckConnect ){
					//クライアントとしてTCPで接続
					if( itr->sock != INVALID_SOCKET && itr->bConnect[0] == false ){
						fd_set wmask;
						FD_ZERO(&wmask);
						FD_SET(itr->sock, &wmask);
						struct timeval tv = {0, 0};
						if( select((int)itr->sock + 1, NULL, &wmask, NULL, &tv) == 1 ){
							itr->bConnect[0] = true;
						}else{
							closesocket(itr->sock);
							itr->sock = INVALID_SOCKET;
						}
					}
					if( itr->sock == INVALID_SOCKET ){
						char szPort[16];
						sprintf_s(szPort, "%d", itr->port);
						struct addrinfo hints = {};
						hints.ai_flags = AI_NUMERICHOST;
						hints.ai_socktype = SOCK_STREAM;
						hints.ai_protocol = IPPROTO_TCP;
						struct addrinfo* result;
						if( getaddrinfo(itr->strIP.c_str(), szPort, &hints, &result) == 0 ){
							itr->sock = CreateNonBlockingSocket(result->ai_family, result->ai_socktype , result->ai_protocol);
							if( itr->sock != INVALID_SOCKET ){
								if( connect(itr->sock, result->ai_addr, (int)result->ai_addrlen) == 0 ){
									itr->bConnect[0] = true;
#ifdef _WIN32
								}else if( WSAGetLastError() != WSAEWOULDBLOCK ){
#else
								}else if( errno != EINPROGRESS ){
#endif
									closesocket(itr->sock);
									itr->sock = INVALID_SOCKET;
								}
							}
							freeaddrinfo(result);
						}
					}
				}
				bSendingToSomeone = bSendingToSomeone || itr->bConnect[0];
				udpWaitState = -1;
			}
		}

		bool bShortWait = false;
		std::list<vector<BYTE>> item;
		size_t sendListSize;
		{
			lock_recursive_mutex lock(pSys->m_sendLock);

			if( udpWaitState < 0 || pSys->m_TSBuff.empty() || pSys->m_TSBuff.size() >= SEND_TS_UDP_RATE_CONTROL_MAX ){
				//TCP系の送信先があるかバッファが空か溜まりすぎているので制御しない
				udpWaitState = 0;
			}else if( pSys->m_TSBuff.size() <= SEND_TS_UDP_RATE_CONTROL_MAX / 2 ){
				bShortWait = true;
				if( ++udpWaitState >= (int)(SEND_TS_UDP_RATE_CONTROL_MAX / pSys->m_TSBuff.size()) ){
					udpWaitState = 0;
					bShortWait = false;
				}
			}else{
				if( ++udpWaitState >= (int)(SEND_TS_UDP_RATE_CONTROL_MAX / (SEND_TS_UDP_RATE_CONTROL_MAX - pSys->m_TSBuff.size())) ){
					udpWaitState = 0;
					bShortWait = true;
				}
			}
			//送信中でないときバッファの追加を省略するため
			pSys->m_bSendingToSomeone = bSendingToSomeone;

			if( pSys->m_TSBuff.empty() == false ){
				//バッファは先行書き込み中かもしれないのでコピーしてはいけない
				item.splice(item.end(), pSys->m_TSBuff, pSys->m_TSBuff.begin());
				DWORD dwCmd[2] = { dwCount, (DWORD)(item.back().size() - sizeof(DWORD) * 2) };
				//ヘッダは2つのリトルエンディアンのDWORD値(dwCount情報はたいてい無視される)
				for( int i = 0; i < 2; i++ ){
					item.back()[i * 4] = (BYTE)dwCmd[i];
					item.back()[i * 4 + 1] = (BYTE)(dwCmd[i] >> 8);
					item.back()[i * 4 + 2] = (BYTE)(dwCmd[i] >> 16);
					item.back()[i * 4 + 3] = (BYTE)(dwCmd[i] >> 24);
				}
				//バッファを1つ消費したので先行書き込み数を1つ減らす
				for( itr = pSys->m_SendList.begin(); itr != pSys->m_SendList.end(); itr++ ){
					for( size_t i = 0; i < array_size(itr->pipe); i++ ){
						if( itr->writeAheadCount[i] ){
							itr->writeAheadCount[i]--;
						}
					}
				}
			}
			//途中で減ることはない
			sendListSize = pSys->m_SendList.size();
		}

		if( pSys->m_stopSendEvent.WaitOne(item.empty() ? 100 : bShortWait ? 10 : 0) ){
			//キャンセルされた
			break;
		}
		if( item.empty() ){
			//送るデータがない
			continue;
		}

		//パイプ書き込み
		bool bStop = false;
		do{
#ifdef _WIN32
			olEventList.assign(1, pSys->m_stopSendEvent.Handle());
#else
			pfdList.resize(1);
			pfdList.back().fd = pSys->m_stopSendEvent.Handle();
			pfdList.back().events = POLLIN;
#endif
			bool bItemWriting = false;
			for( size_t itrIndex = 0; itrIndex < sendListSize; itrIndex++ ){
				{
					lock_recursive_mutex lock(pSys->m_sendLock);
					if( itrIndex == 0 ){
						itr = pSys->m_SendList.begin();
					}else{
						itr++;
					}
				}
				for( size_t i = 0; i < array_size(itr->pipe); i++ ){
#ifdef _WIN32
					if( itr->pipe[i] == INVALID_HANDLE_VALUE ||
					    itr->bConnect[i] == false ||
					    itr->writeAheadCount[i] > SEND_TS_TCP_WRITE_AHEAD_MAX ){
						continue;
					}
					if( itr->bPipeWriting[i] ){
						//書き込み状況を確認
						const vector<BYTE>* pWritingItem = &item.back();
						if( itr->writeAheadCount[i] ){
							lock_recursive_mutex lock(pSys->m_sendLock);
							auto itrAhead = pSys->m_TSBuff.cbegin();
							std::advance(itrAhead, itr->writeAheadCount[i] - 1);
							pWritingItem = &*itrAhead;
						}
						DWORD xferred;
						BOOL bSuccess = GetOverlappedResult(itr->pipe[i], itr->ol + i, &xferred, FALSE);
						if( bSuccess && xferred == pWritingItem->size() - 8 ){
							//成功
							itr->bPipeWriting[i] = false;
							lock_recursive_mutex lock(pSys->m_sendLock);
							itr->writeAheadCount[i]++;
						}else if( bSuccess || GetLastError() != ERROR_IO_INCOMPLETE ){
							//失敗
							itr->bConnect[i] = false;
						}else{
							//さらに待機
							olEventList.push_back(itr->olEvent[i]);
							if( itr->writeAheadCount[i] == 0 ){
								bItemWriting = true;
							}
						}
					}
					OVERLAPPED olReset = {};
					olReset.hEvent = itr->olEvent[i];
					if( itr->bConnect[i] && itr->bPipeWriting[i] == false ){
						const vector<BYTE>* pWriteItem = &item.back();
						if( itr->writeAheadCount[i] ){
							lock_recursive_mutex lock(pSys->m_sendLock);
							pWriteItem = NULL;
							if( pSys->m_TSBuff.size() >= itr->writeAheadCount[i] ){
								auto itrAhead = pSys->m_TSBuff.cbegin();
								std::advance(itrAhead, itr->writeAheadCount[i] - 1);
								pWriteItem = &*itrAhead;
							}
						}
						if( pWriteItem ){
							if( pWriteItem->size() > 8 ){
								//書き込む
								itr->ol[i] = olReset;
								if( WriteFile(itr->pipe[i], pWriteItem->data() + 8, (DWORD)(pWriteItem->size() - 8), NULL, itr->ol + i) ||
								    GetLastError() == ERROR_IO_PENDING ){
									//待機
									itr->bPipeWriting[i] = true;
									olEventList.push_back(itr->olEvent[i]);
									if( itr->writeAheadCount[i] == 0 ){
										bItemWriting = true;
									}
								}else{
									//失敗
									itr->bConnect[i] = false;
								}
							}else if( itr->writeAheadCount[i] <= SEND_TS_TCP_WRITE_AHEAD_MAX ){
								lock_recursive_mutex lock(pSys->m_sendLock);
								itr->writeAheadCount[i]++;
							}
						}
					}
					if( itr->bConnect[i] == false ){
						if( itr->strIP == "0.0.0.1" ){
							//再び待ち受け
							DisconnectNamedPipe(itr->pipe[i]);
							itr->ol[i] = olReset;
							if( ConnectNamedPipe(itr->pipe[i], itr->ol + i) == FALSE ){
								DWORD err = GetLastError();
								if( err == ERROR_PIPE_CONNECTED ){
									itr->bConnect[i] = true;
									itr->bPipeWriting[i] = false;
								}else if( err != ERROR_IO_PENDING ){
									CloseHandle(itr->pipe[i]);
									itr->pipe[i] = INVALID_HANDLE_VALUE;
								}
							}
						}else{
							CloseHandle(itr->pipe[i]);
							itr->pipe[i] = INVALID_HANDLE_VALUE;
						}
						lock_recursive_mutex lock(pSys->m_sendLock);
						itr->writeAheadCount[i] = 0;
					}
#else
					if( itr->pipe[i] < 0 ||
					    itr->bConnect[i] == false ||
					    itr->writeAheadCount[i] > SEND_TS_TCP_WRITE_AHEAD_MAX ){
						continue;
					}
					const vector<BYTE>* pWriteItem = &item.back();
					if( itr->writeAheadCount[i] ){
						lock_recursive_mutex lock(pSys->m_sendLock);
						pWriteItem = NULL;
						if( pSys->m_TSBuff.size() >= itr->writeAheadCount[i] ){
							auto itrAhead = pSys->m_TSBuff.cbegin();
							std::advance(itrAhead, itr->writeAheadCount[i] - 1);
							pWriteItem = &*itrAhead;
						}
					}
					if( pWriteItem ){
						if( pWriteItem->size() > 8 ){
							//書き込む
							int n = (int)write(itr->pipe[i], pWriteItem->data() + 8 + itr->wroteBytes[i], pWriteItem->size() - 8 - itr->wroteBytes[i]);
							if( n == 0 || (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) ){
								//失敗
								close(itr->pipe[i]);
								itr->pipe[i] = -1;
								itr->bConnect[i] = false;
								lock_recursive_mutex lock(pSys->m_sendLock);
								itr->writeAheadCount[i]++;
							}else{
								if( n > 0 ){
									itr->wroteBytes[i] += n;
									if( itr->wroteBytes[i] >= pWriteItem->size() - 8 ){
										itr->wroteBytes[i] = 0;
										lock_recursive_mutex lock(pSys->m_sendLock);
										itr->writeAheadCount[i]++;
									}
								}
								//待機
								pfdList.resize(pfdList.size() + 1);
								pfdList.back().fd = itr->pipe[i];
								pfdList.back().events = POLLOUT;
								if( itr->writeAheadCount[i] == 0 ){
									bItemWriting = true;
								}
							}
						}else{
							lock_recursive_mutex lock(pSys->m_sendLock);
							itr->writeAheadCount[i]++;
						}
					}
#endif
				}
			}
			if( bItemWriting == false ){
				//すべてのパイプにitemを書き込んだ
				break;
			}
#ifdef _WIN32
			DWORD dwRet;
			if( olEventList.size() <= MAXIMUM_WAIT_OBJECTS ){
				dwRet = WaitForMultipleObjects((DWORD)olEventList.size(), olEventList.data(), FALSE, INFINITE);
			}else{
				dwRet = WaitForMultipleObjects(MAXIMUM_WAIT_OBJECTS, olEventList.data(), FALSE, 10);
			}
			bStop = dwRet != WAIT_TIMEOUT && (dwRet <= WAIT_OBJECT_0 || dwRet >= WAIT_OBJECT_0 + MAXIMUM_WAIT_OBJECTS);
#else
			bStop = (poll(pfdList.data(), pfdList.size(), -1) < 0 && errno != EINTR) || pSys->m_stopSendEvent.WaitOne(0);
#endif
		}while( bStop == false );

		//ネットワーク送信
		for( size_t itrIndex = 0; bStop == false && itrIndex < sendListSize; itrIndex++ ){
			{
				lock_recursive_mutex lock(pSys->m_sendLock);
				if( itrIndex == 0 ){
					itr = pSys->m_SendList.begin();
				}else{
					itr++;
				}
			}
			if( itr->sock != INVALID_SOCKET && (itr->strIP.empty() || itr->bConnect[0]) ){
				size_t adjust = item.back().size();
				if( itr->bSuppressHeader ){
					//ヘッダの送信を抑制
					adjust -= sizeof(DWORD) * 2;
				}
				for(;;){
					if( pSys->m_stopSendEvent.WaitOne(0) ){
						//キャンセルされた
						bStop = true;
						break;
					}
					if( adjust != 0 ){
						if( itr->strIP.empty() ){
							//ペイロード分割。BonDriver_UDPに送る場合は受信サイズ48128以下でなければならない
							int len = min(max(itr->udpMaxSendSize, 1), (int)adjust);
							if( sendto(itr->sock, (char*)(item.back().data() + item.back().size() - adjust), len, 0,
							           (const sockaddr*)&itr->udpAddr, (int)itr->udpAddrlen) >= 0 ||
#ifdef _WIN32
							    WSAGetLastError() != WSAEWOULDBLOCK
#else
							    (errno != EAGAIN && errno != EWOULDBLOCK)
#endif
							    ){
								adjust -= len;
							}
						}else{
							//TCP
							int ret = (int)send(itr->sock, (char*)(item.back().data() + item.back().size() - adjust), (int)adjust, 0);

							if( ret == 0 || (ret < 0 &&
#ifdef _WIN32
							    WSAGetLastError() != WSAEWOULDBLOCK
#else
							    errno != EAGAIN && errno != EWOULDBLOCK
#endif
							    ) ){
								closesocket(itr->sock);
								itr->sock = INVALID_SOCKET;
								itr->bConnect[0] = false;
								break;
							}else if( ret > 0 ){
								adjust -= ret;
							}
						}
					}
					if( adjust == 0 ){
						dwCount++;
						break;
					}
					//すこし待つ
					fd_set wmask;
					FD_ZERO(&wmask);
					FD_SET(itr->sock, &wmask);
					struct timeval tv10msec = {0, 10000};
					select((int)itr->sock + 1, NULL, &wmask, NULL, &tv10msec);
				}
			}
		}
		if( bStop ){
			break;
		}
	}

	lock_recursive_mutex lock(pSys->m_sendLock);
	for( auto itr = pSys->m_SendList.begin(); itr != pSys->m_SendList.end(); itr++ ){
		//TCP系を切断
		if( itr->sock != INVALID_SOCKET && itr->strIP.empty() == false ){
			//未送信データが捨てられても問題ないのでshutdown()は省略
			closesocket(itr->sock);
			itr->sock = INVALID_SOCKET;
		}
		for( size_t i = 0; i < array_size(itr->pipe); i++ ){
#ifdef _WIN32
			if( itr->pipe[i] != INVALID_HANDLE_VALUE ){
				if( itr->bConnect[i] && itr->bPipeWriting[i] ){
					//書き込みをキャンセル
					CancelIo(itr->pipe[i]);
					WaitForSingleObject(itr->olEvent[i], INFINITE);
				}
				if( itr->strIP == "0.0.0.1" ){
					if( itr->bConnect[i] ){
						DisconnectNamedPipe(itr->pipe[i]);
					}else{
						//待ち受けをキャンセル
						CancelIo(itr->pipe[i]);
						WaitForSingleObject(itr->olEvent[i], INFINITE);
					}
				}
				CloseHandle(itr->pipe[i]);
				itr->pipe[i] = INVALID_HANDLE_VALUE;
			}
			if( itr->olEvent[i] ){
				CloseHandle(itr->olEvent[i]);
				itr->olEvent[i] = NULL;
			}
#else
			if( itr->strPipe[i].empty() == false ){
				remove(itr->strPipe[i].c_str());
				itr->strPipe[i].clear();
			}
			if( itr->pipe[i] >= 0 ){
				close(itr->pipe[i]);
				itr->pipe[i] = -1;
			}
#endif
			itr->bConnect[i] = false;
			itr->writeAheadCount[i] = 0;
		}
	}
}
