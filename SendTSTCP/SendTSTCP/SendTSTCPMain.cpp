#include "stdafx.h"
#include "SendTSTCPMain.h"
#include "../../Common/StringUtil.h"
#include "../../Common/TimeUtil.h"

namespace
{
//SendTSTCPプロトコルのヘッダの送信を抑制する既定のポート範囲
const DWORD SEND_TS_TCP_NOHEAD_PORT_MIN = 22000;
const DWORD SEND_TS_TCP_NOHEAD_PORT_MAX = 22999;

//送信先が0.0.0.1のとき待ち受ける名前付きパイプ名
const WCHAR SEND_TS_TCP_0001_PIPE_NAME[] = L"\\\\.\\pipe\\SendTSTCP_%d_%u";

//送信先が0.0.0.2のとき開く名前付きパイプ名
const WCHAR SEND_TS_TCP_0002_PIPE_NAME[] = L"\\\\.\\pipe\\BonDriver_Pipe%02d";

//送信バッファの最大数(サイズはAddSendData()の入力に依存)
const DWORD SEND_TS_TCP_BUFF_MAX = 500;

//送信バッファのうち先行書き込み可能な最大数
//(複数送信先の送信タイミングの同期によりパフォーマンスが低下するのを避ける)
const DWORD SEND_TS_TCP_WRITE_AHEAD_MAX = SEND_TS_TCP_BUFF_MAX / 4;

//送信先(サーバ)接続のためのポーリング間隔
const DWORD SEND_TS_TCP_CONNECT_INTERVAL_MSEC = 2000;

//UDP送信バッファのサイズ
const int UDP_SNDBUF_SIZE = 3 * 1024 * 1024;
}

CSendTSTCPMain::CSendTSTCPMain(void)
{
	m_wsaStartupResult = -1;
}

CSendTSTCPMain::~CSendTSTCPMain(void)
{
	StopSend();
	ClearSendAddr();

	if( m_wsaStartupResult == 0 ){
		WSACleanup();
	}
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
	SEND_INFO item;
	WtoUTF8(lpcwszIP, item.strIP);
	item.port = (WORD)dwPort;
	item.bSuppressHeader = (dwPort & 0x10000) != 0 || (SEND_TS_TCP_NOHEAD_PORT_MIN <= dwPort && dwPort <= SEND_TS_TCP_NOHEAD_PORT_MAX);
	item.sock = INVALID_SOCKET;
	for( size_t i = 0; i < array_size(item.pipe); i++ ){
		item.pipe[i] = INVALID_HANDLE_VALUE;
		item.olEvent[i] = NULL;
		item.bConnect[i] = false;
		item.bPipeWriting[i] = false;
		item.writeAheadCount[i] = 0;
	}

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
	if( m_wsaStartupResult == -1 ){
		WSAData wsaData;
		m_wsaStartupResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	}
	if( m_wsaStartupResult != 0 ){
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

		lock_recursive_mutex lock(m_sendLock);
		if( m_SendList.empty() == false ){
			m_TSBuff.push_back(vector<BYTE>());
			m_TSBuff.back().reserve(sizeof(DWORD) * 2 + dwSize);
			m_TSBuff.back().resize(sizeof(DWORD) * 2);
			m_TSBuff.back().insert(m_TSBuff.back().end(), pbData, pbData + dwSize);
			if( m_TSBuff.size() > SEND_TS_TCP_BUFF_MAX ){
				ClearSendBuff();
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
	//ヘッダのdwCount情報を3バイト目が0でない値で始める。原作は0で始めていたが仕様的に始点に意味はなく
	//また他のTCPインタフェースのヘッダと区別しにくいため設定を誤った場合に想定外のことが起きるのを防ぐため
	DWORD dwCount = 0x01000000;
	DWORD dwCheckConnectTick = GetU32Tick();
	vector<HANDLE> olEventList;
	for(;;){
		DWORD tick = GetU32Tick();
		bool bCheckConnect = tick - dwCheckConnectTick > SEND_TS_TCP_CONNECT_INTERVAL_MSEC;
		if( bCheckConnect ){
			dwCheckConnectTick = tick;
		}
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
				}
			}else if( itr->strIP == "0.0.0.2" ){
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
							itr->sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
							if( itr->sock != INVALID_SOCKET ){
								//ノンブロッキングモードへ
								unsigned long x = 1;
								if( ioctlsocket(itr->sock, FIONBIO, &x) == SOCKET_ERROR ){
									closesocket(itr->sock);
									itr->sock = INVALID_SOCKET;
								}else if( connect(itr->sock, result->ai_addr, (int)result->ai_addrlen) != SOCKET_ERROR ){
									itr->bConnect[0] = true;
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
		size_t sendListSize;
		{
			lock_recursive_mutex lock(pSys->m_sendLock);

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

		if( pSys->m_stopSendEvent.WaitOne(item.empty() ? 100 : 0) ){
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
			olEventList.assign(1, pSys->m_stopSendEvent.Handle());
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
				}
			}
			if( bItemWriting == false ){
				//すべてのパイプにitemを書き込んだ
				break;
			}
			DWORD dwRet;
			if( olEventList.size() <= MAXIMUM_WAIT_OBJECTS ){
				dwRet = WaitForMultipleObjects((DWORD)olEventList.size(), olEventList.data(), FALSE, INFINITE);
			}else{
				dwRet = WaitForMultipleObjects(MAXIMUM_WAIT_OBJECTS, olEventList.data(), FALSE, 10);
			}
			bStop = dwRet != WAIT_TIMEOUT && (dwRet <= WAIT_OBJECT_0 || dwRet >= WAIT_OBJECT_0 + MAXIMUM_WAIT_OBJECTS);
		}while( bStop == false );

		//TCP送信
		for( size_t itrIndex = 0; bStop == false && itrIndex < sendListSize; itrIndex++ ){
			{
				lock_recursive_mutex lock(pSys->m_sendLock);
				if( itrIndex == 0 ){
					itr = pSys->m_SendList.begin();
				}else{
					itr++;
				}
			}
			if( itr->sock != INVALID_SOCKET && itr->bConnect[0] ){
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
						int ret = send(itr->sock, (char*)(item.back().data() + item.back().size() - adjust), (int)adjust, 0);
						if( ret == 0 || (ret == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) ){
							closesocket(itr->sock);
							itr->sock = INVALID_SOCKET;
							itr->bConnect[0] = false;
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
		}
		if( bStop ){
			break;
		}
	}

	lock_recursive_mutex lock(pSys->m_sendLock);
	for( auto itr = pSys->m_SendList.begin(); itr != pSys->m_SendList.end(); itr++ ){
		if( itr->sock != INVALID_SOCKET ){
			//未送信データが捨てられても問題ないのでshutdown()は省略
			closesocket(itr->sock);
			itr->sock = INVALID_SOCKET;
		}
		for( size_t i = 0; i < array_size(itr->pipe); i++ ){
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
			itr->bConnect[i] = false;
			itr->writeAheadCount[i] = 0;
		}
	}
}
