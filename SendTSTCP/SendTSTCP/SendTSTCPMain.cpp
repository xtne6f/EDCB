#include "stdafx.h"
#include "SendTSTCPMain.h"

//SendTSTCPプロトコルのヘッダの送信を抑制する既定のポート範囲
#define SEND_TS_TCP_NOHEAD_PORT_MIN 22000
#define SEND_TS_TCP_NOHEAD_PORT_MAX 22999
//送信先が0.0.0.1のとき待ち受ける名前付きパイプ名
#define SEND_TS_TCP_0001_PIPE_NAME L"\\\\.\\pipe\\SendTSTCP_%d_%u"
//送信先が0.0.0.2のとき開く名前付きパイプ名
#define SEND_TS_TCP_0002_PIPE_NAME L"\\\\.\\pipe\\BonDriver_Pipe%02d"
//送信バッファの最大数(サイズはAddSendData()の入力に依存)
#define SEND_TS_TCP_BUFF_MAX 500
//送信先(サーバ)接続のためのポーリング間隔
#define SEND_TS_TCP_CONNECT_INTERVAL_MSEC 2000

//UDP送信バッファのサイズ
static const int UDP_SNDBUF_SIZE = 3 * 1024 * 1024;

CSendTSTCPMain::CSendTSTCPMain(void)
{
	WSAData wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
}

CSendTSTCPMain::~CSendTSTCPMain(void)
{
	StopSend();
	ClearSendAddr();

	WSACleanup();
}

//送信先を追加
//戻り値：エラーコード
DWORD CSendTSTCPMain::AddSendAddr(
	LPCWSTR lpcwszIP,
	DWORD dwPort
	)
{
	if( lpcwszIP == NULL ){
		return FALSE;
	}
	SEND_INFO Item;
	WtoUTF8(lpcwszIP, Item.strIP);
	Item.dwPort = dwPort;
	if( SEND_TS_TCP_NOHEAD_PORT_MIN <= dwPort && dwPort <= SEND_TS_TCP_NOHEAD_PORT_MAX ){
		//上位ワードが1のときはヘッダの送信が抑制される
		Item.dwPort |= 0x10000;
	}
	Item.sock = INVALID_SOCKET;
	Item.pipe = INVALID_HANDLE_VALUE;
	Item.olEvent = NULL;
	Item.bConnect = FALSE;

	CBlockLock lock(&m_sendLock);
	if( std::find_if(m_SendList.begin(), m_SendList.end(), [&Item](const SEND_INFO& a) {
	        return a.strIP == Item.strIP && (WORD)a.dwPort == (WORD)Item.dwPort; }) == m_SendList.end() ){
		m_SendList.push_back(Item);
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
	SOCKET_DATA item;
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
	item.addrlen = min((size_t)result->ai_addrlen, sizeof(item.addr));
	memcpy(&item.addr, result->ai_addr, item.addrlen);
	item.sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	freeaddrinfo(result);
	if( item.sock == INVALID_SOCKET ){
		return FALSE;
	}

	//ノンブロッキングモードへ
	unsigned long x = 1;
	if( ioctlsocket(item.sock, FIONBIO, &x) != 0 ||
	    setsockopt(item.sock, SOL_SOCKET, SO_SNDBUF, (const char*)&UDP_SNDBUF_SIZE, sizeof(UDP_SNDBUF_SIZE)) != 0 ){
		closesocket(item.sock);
		return FALSE;
	}
	if( broadcastFlag ){
		BOOL b = TRUE;
		setsockopt(item.sock, SOL_SOCKET, SO_BROADCAST, (const char*)&b, sizeof(b));
	}
	item.maxSendSize = maxSendSize;
	m_udpSockList.push_back(item);

	return TRUE;
}

//送信先クリア
//戻り値：エラーコード
DWORD CSendTSTCPMain::ClearSendAddr(
	)
{
	if( m_sendThread.joinable() ){
		StopSend();
		m_SendList.clear();
		StartSend();
	}else{
		m_SendList.clear();
	}

	while( m_udpSockList.empty() == false ){
		unsigned long x = 0;
		ioctlsocket(m_udpSockList.back().sock, FIONBIO, &x);
		closesocket(m_udpSockList.back().sock);
		m_udpSockList.pop_back();
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
		//UDPは基本的にブロックしない(輻輳制御がない)のでここで送る
		for( auto itr = m_udpSockList.cbegin(); itr != m_udpSockList.end(); itr++ ){
			for( DWORD dwRead = 0; dwRead < dwSize; ){
				//ペイロード分割。BonDriver_UDPに送る場合は受信サイズ48128以下でなければならない
				int len = (int)min((DWORD)max(itr->maxSendSize, 1), dwSize - dwRead);
				if( sendto(itr->sock, (const char*)(pbData + dwRead), len, 0, (const sockaddr*)&itr->addr, (int)itr->addrlen) < 0 ){
					if( WSAGetLastError() == WSAEWOULDBLOCK ){
						//送信処理が追いつかずSO_SNDBUFで指定したバッファも尽きてしまった
						//帯域が足りないときはどう足掻いてもドロップするしかないので、Sleep()によるフロー制御はしない
						AddDebugLog(L"Dropped");
					}
				}
				dwRead += len;
			}
		}

		CBlockLock lock(&m_sendLock);
		if( m_SendList.empty() == false ){
			m_TSBuff.push_back(vector<BYTE>());
			m_TSBuff.back().reserve(sizeof(DWORD) * 2 + dwSize);
			m_TSBuff.back().resize(sizeof(DWORD) * 2);
			m_TSBuff.back().insert(m_TSBuff.back().end(), pbData, pbData + dwSize);
			if( m_TSBuff.size() > SEND_TS_TCP_BUFF_MAX ){
				for( ; m_TSBuff.size() > SEND_TS_TCP_BUFF_MAX / 2; m_TSBuff.pop_front() );
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
	CBlockLock lock(&m_sendLock);
	m_TSBuff.clear();

	return TRUE;
}

void CSendTSTCPMain::SendThread(CSendTSTCPMain* pSys)
{
	DWORD dwCount = 0;
	DWORD dwCheckConnectTick = GetTickCount();
	for(;;){
		DWORD tick = GetTickCount();
		if( tick - dwCheckConnectTick > SEND_TS_TCP_CONNECT_INTERVAL_MSEC ){
			dwCheckConnectTick = tick;
			std::list<SEND_INFO>::iterator itr;
			for( size_t i = 0;; i++ ){
				{
					CBlockLock lock(&pSys->m_sendLock);
					if( i == 0 ){
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
					if( itr->olEvent == NULL ){
						itr->olEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
					}
					if( itr->olEvent ){
						if( itr->pipe == INVALID_HANDLE_VALUE ){
							wstring strPipe;
							Format(strPipe, SEND_TS_TCP_0001_PIPE_NAME, (WORD)itr->dwPort, GetCurrentProcessId());
							itr->pipe = CreateNamedPipe(strPipe.c_str(), PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED, 0, 1, 48128, 0, 0, NULL);
							if( itr->pipe != INVALID_HANDLE_VALUE ){
								OVERLAPPED olZero = {};
								itr->ol = olZero;
								itr->ol.hEvent = itr->olEvent;
								if( ConnectNamedPipe(itr->pipe, &itr->ol) == FALSE ){
									DWORD err = GetLastError();
									if( err == ERROR_PIPE_CONNECTED ){
										itr->bConnect = TRUE;
									}else if( err != ERROR_IO_PENDING ){
										CloseHandle(itr->pipe);
										itr->pipe = INVALID_HANDLE_VALUE;
									}
								}
							}
						}else if( itr->bConnect == FALSE ){
							if( WaitForSingleObject(itr->olEvent, 0) == WAIT_OBJECT_0 ){
								itr->bConnect = TRUE;
							}
						}
					}
				}else if( itr->strIP == "0.0.0.2" ){
					//クライアントとして名前付きパイプを開く
					if( itr->olEvent == NULL ){
						itr->olEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
					}
					if( itr->olEvent && itr->pipe == INVALID_HANDLE_VALUE ){
						wstring strPipe;
						Format(strPipe, SEND_TS_TCP_0002_PIPE_NAME, (WORD)itr->dwPort);
						itr->pipe = CreateFile(strPipe.c_str(), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
						if( itr->pipe != INVALID_HANDLE_VALUE ){
							itr->bConnect = TRUE;
						}
					}
				}else{
					//クライアントとしてTCPで接続
					if( itr->sock != INVALID_SOCKET && itr->bConnect == FALSE ){
						fd_set wmask;
						FD_ZERO(&wmask);
						FD_SET(itr->sock, &wmask);
						struct timeval tv = {0, 0};
						if( select((int)itr->sock + 1, NULL, &wmask, NULL, &tv) == 1 ){
							itr->bConnect = TRUE;
						}else{
							closesocket(itr->sock);
							itr->sock = INVALID_SOCKET;
						}
					}
					if( itr->sock == INVALID_SOCKET ){
						char szPort[16];
						sprintf_s(szPort, "%d", (WORD)itr->dwPort);
						struct addrinfo hints = {};
						hints.ai_flags = AI_NUMERICHOST;
						hints.ai_socktype = SOCK_STREAM;
						hints.ai_protocol = IPPROTO_TCP;
						struct addrinfo* result;
						if( getaddrinfo(itr->strIP.c_str(), szPort, &hints, &result) == 0 ){
							itr->sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
							if( itr->sock != INVALID_SOCKET ){
								//ノンブロッキングモードへ
								unsigned long x = 1;
								if( ioctlsocket(itr->sock, FIONBIO, &x) == SOCKET_ERROR ){
									closesocket(itr->sock);
									itr->sock = INVALID_SOCKET;
								}else if( connect(itr->sock, result->ai_addr, (int)result->ai_addrlen) != SOCKET_ERROR ){
									itr->bConnect = TRUE;
								}else if( WSAGetLastError() != WSAEWOULDBLOCK ){
									closesocket(itr->sock);
									itr->sock = INVALID_SOCKET;
								}
							}
							freeaddrinfo(result);
						}
					}
				}
			}
		}

		std::list<vector<BYTE>> item;
		size_t sendListSizeOrStop;
		{
			CBlockLock lock(&pSys->m_sendLock);

			if( pSys->m_TSBuff.empty() == false ){
				item.splice(item.end(), pSys->m_TSBuff, pSys->m_TSBuff.begin());
				DWORD dwCmd[2] = { dwCount, (DWORD)(item.back().size() - sizeof(DWORD) * 2) };
				memcpy(&item.back().front(), dwCmd, sizeof(dwCmd));
			}
			//途中で減ることはない
			sendListSizeOrStop = pSys->m_SendList.size();
		}

		if( item.empty() || sendListSizeOrStop == 0 ){
			if( pSys->m_stopSendEvent.WaitOne(item.empty() ? 100 : 0) ){
				//キャンセルされた
				break;
			}
		}else{
			std::list<SEND_INFO>::iterator itr;
			for( size_t i = 0; i < sendListSizeOrStop; i++ ){
				{
					CBlockLock lock(&pSys->m_sendLock);
					if( i == 0 ){
						itr = pSys->m_SendList.begin();
					}else{
						itr++;
					}
				}
				size_t adjust = item.back().size();
				if( itr->pipe != INVALID_HANDLE_VALUE || itr->dwPort >> 16 == 1 ){
					adjust -= sizeof(DWORD) * 2;
				}
				if( itr->pipe != INVALID_HANDLE_VALUE && itr->bConnect && adjust != 0 ){
					//名前付きパイプに書き込む
					OVERLAPPED olZero = {};
					itr->ol = olZero;
					itr->ol.hEvent = itr->olEvent;
					HANDLE olEvents[] = { pSys->m_stopSendEvent.Handle(), itr->olEvent };
					BOOL bClose = FALSE;
					DWORD xferred;
					if( WriteFile(itr->pipe, item.back().data() + item.back().size() - adjust, (DWORD)adjust, NULL, &itr->ol) == FALSE &&
					    GetLastError() != ERROR_IO_PENDING ){
						bClose = TRUE;
					}else if( WaitForMultipleObjects(2, olEvents, FALSE, INFINITE) != WAIT_OBJECT_0 + 1 ){
						//キャンセルされた
						CancelIo(itr->pipe);
						WaitForSingleObject(itr->olEvent, INFINITE);
						sendListSizeOrStop = 0;
					}else if( GetOverlappedResult(itr->pipe, &itr->ol, &xferred, FALSE) == FALSE || xferred < adjust ){
						bClose = TRUE;
					}
					if( bClose ){
						if( itr->strIP == "0.0.0.1" ){
							//再び待ち受け
							DisconnectNamedPipe(itr->pipe);
							itr->bConnect = FALSE;
							itr->ol = olZero;
							itr->ol.hEvent = itr->olEvent;
							if( ConnectNamedPipe(itr->pipe, &itr->ol) == FALSE ){
								DWORD err = GetLastError();
								if( err == ERROR_PIPE_CONNECTED ){
									itr->bConnect = TRUE;
								}else if( err != ERROR_IO_PENDING ){
									CloseHandle(itr->pipe);
									itr->pipe = INVALID_HANDLE_VALUE;
								}
							}
						}else{
							CloseHandle(itr->pipe);
							itr->pipe = INVALID_HANDLE_VALUE;
							itr->bConnect = FALSE;
						}
					}
				}
				for(;;){
					if( pSys->m_stopSendEvent.WaitOne(0) ){
						//キャンセルされた
						sendListSizeOrStop = 0;
						break;
					}
					if( itr->sock == INVALID_SOCKET || itr->bConnect == FALSE ){
						break;
					}
					if( adjust != 0 ){
						int ret = send(itr->sock, (char*)(item.back().data() + item.back().size() - adjust), (int)adjust, 0);
						if( ret == 0 || (ret == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) ){
							closesocket(itr->sock);
							itr->sock = INVALID_SOCKET;
							itr->bConnect = FALSE;
							break;
						}else if( ret != SOCKET_ERROR ){
							adjust -= ret;
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
			if( sendListSizeOrStop == 0 ){
				break;
			}
		}
	}

	CBlockLock lock(&pSys->m_sendLock);
	for( auto itr = pSys->m_SendList.begin(); itr != pSys->m_SendList.end(); itr++ ){
		if( itr->sock != INVALID_SOCKET ){
			//未送信データが捨てられても問題ないのでshutdown()は省略
			closesocket(itr->sock);
			itr->sock = INVALID_SOCKET;
		}
		if( itr->pipe != INVALID_HANDLE_VALUE ){
			if( itr->strIP == "0.0.0.1" ){
				if( itr->bConnect ){
					DisconnectNamedPipe(itr->pipe);
				}else{
					//待ち受けをキャンセル
					CancelIo(itr->pipe);
					WaitForSingleObject(itr->olEvent, INFINITE);
				}
			}
			CloseHandle(itr->pipe);
			itr->pipe = INVALID_HANDLE_VALUE;
		}
		if( itr->olEvent ){
			CloseHandle(itr->olEvent);
			itr->olEvent = NULL;
		}
		itr->bConnect = FALSE;
	}
}
