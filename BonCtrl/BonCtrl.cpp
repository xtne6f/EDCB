#include "stdafx.h"
#include "BonCtrl.h"
#include <process.h>

#include "../Common/ErrDef.h"
#include "../Common/TimeUtil.h"
#include "../Common/SendCtrlCmd.h"
#include "../Common/BlockLock.h"

CBonCtrl::CBonCtrl(void)
{
	this->lockEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
	InitializeCriticalSection(&this->buffLock);
	this->totalTSBuffSize = 0;

    this->recvThread = NULL;
    this->recvStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    this->analyzeThread = NULL;
    this->analyzeStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    this->chScanThread = NULL;
    this->chScanStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	this->chSt_space = 0;
	this->chSt_ch = 0;
	this->chSt_chName = L"";
	this->chSt_chkNum = 0;
	this->chSt_totalNum = 0;
	this->chSt_err = ST_STOP;

	this->epgCapThread = NULL;
	this->epgCapStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	this->epgSt_err = ST_STOP;

	this->epgCapBackThread = NULL;
	this->epgCapBackStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	this->enableLiveEpgCap = FALSE;
	this->enableRecEpgCap = FALSE;

	this->epgCapBackBSBasic = TRUE;
	this->epgCapBackCS1Basic = TRUE;
	this->epgCapBackCS2Basic = TRUE;
	this->epgCapBackStartWaitSec = 30;
	this->tsBuffMaxCount = 5000;
	this->writeBuffMaxCount = -1;
}


CBonCtrl::~CBonCtrl(void)
{
	_CloseBonDriver();

	if( this->chScanThread != NULL ){
		::SetEvent(this->chScanStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->chScanThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->chScanThread, 0xffffffff);
		}
		CloseHandle(this->chScanThread);
		this->chScanThread = NULL;
	}
	if( this->chScanStopEvent != NULL ){
		CloseHandle(this->chScanStopEvent);
		this->chScanStopEvent = NULL;
	}

	if( this->epgCapThread != NULL ){
		::SetEvent(this->epgCapStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->epgCapThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->epgCapThread, 0xffffffff);
		}
		CloseHandle(this->epgCapThread);
		this->epgCapThread = NULL;
	}
	if( this->epgCapStopEvent != NULL ){
		CloseHandle(this->epgCapStopEvent);
		this->epgCapStopEvent = NULL;
	}

	if( this->epgCapBackThread != NULL ){
		::SetEvent(this->epgCapBackStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->epgCapBackThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->epgCapBackThread, 0xffffffff);
		}
		CloseHandle(this->epgCapBackThread);
		this->epgCapBackThread = NULL;
	}
	if( this->epgCapBackStopEvent != NULL ){
		CloseHandle(this->epgCapBackStopEvent);
		this->epgCapBackStopEvent = NULL;
	}

	if( this->analyzeThread != NULL ){
		::SetEvent(this->analyzeStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->analyzeThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->analyzeThread, 0xffffffff);
		}
		CloseHandle(this->analyzeThread);
		this->analyzeThread = NULL;
	}
	if( this->analyzeStopEvent != NULL ){
		CloseHandle(this->analyzeStopEvent);
		this->analyzeStopEvent = NULL;
	}

	if( this->recvThread != NULL ){
		::SetEvent(this->recvStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->recvThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->recvThread, 0xffffffff);
		}
		CloseHandle(this->recvThread);
		this->recvThread = NULL;
	}
	if( this->recvStopEvent != NULL ){
		CloseHandle(this->recvStopEvent);
		this->recvStopEvent = NULL;
	}
	
	if( this->lockEvent != NULL ){
		UnLock();
		CloseHandle(this->lockEvent);
		this->lockEvent = NULL;
	}
	DeleteCriticalSection(&this->buffLock);
}

BOOL CBonCtrl::Lock(LPCWSTR log, DWORD timeOut)
{
	if( this->lockEvent == NULL ){
		return FALSE;
	}
	//if( log != NULL ){
	//	OutputDebugString(log);
	//}
	DWORD dwRet = WaitForSingleObject(this->lockEvent, timeOut);
	if( dwRet == WAIT_ABANDONED || 
		dwRet == WAIT_FAILED ||
		dwRet == WAIT_TIMEOUT){
			if( log != NULL ){
				_OutputDebugString(L"◆CBonCtrl::Lock FALSE : %s", log);
			}else{
				OutputDebugString(L"◆CBonCtrl::Lock FALSE");
			}
		return FALSE;
	}
	return TRUE;
}

void CBonCtrl::UnLock(LPCWSTR log)
{
	if( this->lockEvent != NULL ){
		SetEvent(this->lockEvent);
	}
	if( log != NULL ){
		OutputDebugString(log);
	}
}

//BonDriverフォルダを指定
//引数：
// bonDriverFolderPath		[IN]BonDriverフォルダパス
void CBonCtrl::SetBonDriverFolder(
	LPCWSTR bonDriverFolderPath
)
{
	if( Lock(L"SetSettingFolder") == FALSE ) return ;
	this->bonUtil.SetBonDriverFolder(bonDriverFolderPath);

	UnLock();
}

void CBonCtrl::SetEMMMode(BOOL enable)
{
	if( Lock(L"SetEMMMode") == FALSE ) return ;

	this->tsOut.SetEmm(enable);

	UnLock();
}

void CBonCtrl::SetTsBuffMaxCount(DWORD tsBuffMaxCount, int writeBuffMaxCount)
{
	if( Lock(L"SetTsBuffMaxCount") == FALSE ) return ;

	this->tsBuffMaxCount = tsBuffMaxCount;
	this->writeBuffMaxCount = writeBuffMaxCount;

	UnLock();
}

//BonDriverフォルダのBonDriver_*.dllを列挙
//戻り値：
// エラーコード
//引数：
// bonList			[OUT]検索できたBonDriver一覧
DWORD CBonCtrl::EnumBonDriver(
	vector<wstring>* bonList
)
{
	if( Lock(L"EnumBonDriver") == FALSE ) return ERR_FALSE;
	DWORD ret = this->bonUtil.EnumBonDriver(bonList);
	UnLock();
	return ret;
}

//BonDriverをロードしてチャンネル情報などを取得（ファイル名で指定）
//戻り値：
// エラーコード
//引数：
// bonDriverFile	[IN]EnumBonDriverで取得されたBonDriverのファイル名
DWORD CBonCtrl::OpenBonDriver(
	LPCWSTR bonDriverFile,
	int openWait
)
{
	if( Lock(L"OpenBonDriver-2") == FALSE ) return ERR_FALSE;
	_CloseBonDriver();
	DWORD ret = this->bonUtil.OpenBonDriver(bonDriverFile, openWait);
	wstring bonFile = this->bonUtil.GetOpenBonDriverFileName();
	if( ret == NO_ERR ){
		ret = _OpenBonDriver();
		this->tsOut.ResetChChange();

		this->tsOut.SetBonDriver(bonFile);
	}
	if( bonFile.empty() == false ){
		wstring settingPath;
		GetSettingPath(settingPath);
		wstring bonFileTitle;
		GetFileTitle(bonFile, bonFileTitle);
		wstring tunerName = this->bonUtil.GetTunerName();
		CheckFileName(tunerName);
		this->chUtil.LoadChSet( settingPath + L"\\" + bonFileTitle + L"(" + tunerName + L").ChSet4.txt", settingPath + L"\\ChSet5.txt" );
	}

	UnLock();
	return ret;
}

//ロードしているBonDriverの開放
//戻り値：
// エラーコード
DWORD CBonCtrl::CloseBonDriver()
{
	if( Lock(L"CloseBonDriver") == FALSE ) return ERR_FALSE;
	StopBackgroundEpgCap();
	DWORD ret = _CloseBonDriver();
	UnLock();
	return ret;
}

//ロード中のBonDriverのファイル名を取得する（ロード成功しているかの判定）
//戻り値：
// TRUE（成功）：FALSE（Openに失敗している）
//引数：
// bonDriverFile		[OUT]BonDriverのファイル名(NULL可)
BOOL CBonCtrl::GetOpenBonDriver(
	wstring* bonDriverFile
	)
{
	if( Lock(L"GetOpenBonDriver") == FALSE ) return ERR_FALSE;

	BOOL ret = FALSE;

	wstring strBonDriverFile = this->bonUtil.GetOpenBonDriverFileName();
	if( strBonDriverFile.empty() == false ){
		ret = TRUE;
		if( bonDriverFile != NULL ){
			*bonDriverFile = strBonDriverFile;
		}
	}

	UnLock();

	return ret;
}
//ロードしたBonDriverの情報取得
//SpaceとChの一覧を取得する
//戻り値：
// エラーコード
//引数：
// spaceMap			[OUT] SpaceとChの一覧（mapのキー Space）
DWORD CBonCtrl::GetOriginalChList(
	map<DWORD, BON_SPACE_INFO>* spaceMap
)
{
	if( Lock(L"GetOriginalChList") == FALSE ) return ERR_FALSE;
	DWORD ret = this->bonUtil.GetOriginalChList(spaceMap);
	UnLock();
	return ret;
}

//BonDriverのチューナー名を取得
//戻り値：
// チューナー名
wstring CBonCtrl::GetTunerName()
{
	if( Lock(L"GetTunerName") == FALSE ) return L"err";
	wstring ret = this->bonUtil.GetTunerName();
	UnLock();
	return ret;
}

//チャンネル変更
//戻り値：
// エラーコード
//引数：
// space			[IN]変更チャンネルのSpace
// ch				[IN]変更チャンネルの物理Ch
DWORD CBonCtrl::SetCh(
	DWORD space,
	DWORD ch
)
{
	if( Lock(L"SetCh") == FALSE ) return ERR_FALSE;

	if( this->tsOut.IsRec() == TRUE ){
		UnLock();
		return ERR_FALSE;
	}

	DWORD ret = _SetCh(space, ch);

	UnLock();
	return ret;
}

//チャンネル変更
//戻り値：
// エラーコード
//引数：
// space			[IN]変更チャンネルのSpace
// ch				[IN]変更チャンネルの物理Ch
// SID			[IN]変更チャンネルの物理service_id
DWORD CBonCtrl::SetCh(
	DWORD space,
	DWORD ch,
	WORD SID
)
{
	if( Lock(L"SetCh-2") == FALSE ) return ERR_FALSE;

	if( this->tsOut.IsRec() == TRUE ){
		UnLock();
		return ERR_FALSE;
	}

	DWORD ret = ERR_FALSE;
	ret = _SetCh(space, ch);
	this->lastSID = SID;

	UnLock();
	return ret;
}

//チャンネル変更
//戻り値：
// エラーコード
//引数：
// ONID			[IN]変更チャンネルのorignal_network_id
// TSID			[IN]変更チャンネルの物理transport_stream_id
// SID			[IN]変更チャンネルの物理service_id
DWORD CBonCtrl::SetCh(
	WORD ONID,
	WORD TSID,
	WORD SID
)
{
	if( Lock(L"SetCh-3") == FALSE ) return ERR_FALSE;

	if( this->tsOut.IsRec() == TRUE ){
		UnLock();
		return ERR_FALSE;
	}

	DWORD space=0;
	DWORD ch=0;

	DWORD ret = ERR_FALSE;
	if( this->chUtil.GetCh( ONID, TSID, space, ch ) == TRUE ){
		ret = _SetCh(space, ch);
		this->lastSID = SID;
	}

	UnLock();
	return ret;
}

DWORD CBonCtrl::_SetCh(
	DWORD space,
	DWORD ch,
	BOOL chScan
	)
{
	DWORD spaceNow=0;
	DWORD chNow=0;

	DWORD ret = ERR_FALSE;
	if( this->bonUtil.GetOpenBonDriverFileName().empty() == false ){
		ret = NO_ERR;
		if( this->bonUtil.GetNowCh(&spaceNow, &chNow) == FALSE || space != spaceNow || ch != chNow ){
			this->tsOut.SetChChangeEvent(chScan);
			_OutputDebugString(L"SetCh space %d, ch %d", space, ch);
			ret = this->bonUtil.SetCh(space, ch);

			StartBackgroundEpgCap();
		}else{
			BOOL chChgErr = FALSE;
			if( this->tsOut.IsChChanging(&chChgErr) == TRUE ){
				if( chChgErr == TRUE ){
					//エラーの時は再設定
					this->tsOut.SetChChangeEvent();
					_OutputDebugString(L"SetCh space %d, ch %d", space, ch);
					ret = this->bonUtil.SetCh(space, ch);

					StartBackgroundEpgCap();
				}
			}else{
				if( chChgErr == TRUE ){
					//エラーの時は再設定
					this->tsOut.SetChChangeEvent();
					_OutputDebugString(L"SetCh space %d, ch %d", space, ch);
					ret = this->bonUtil.SetCh(space, ch);

					StartBackgroundEpgCap();
				}
			}
		}
	}else{
		OutputDebugString(L"Err GetNowCh");
	}
	return ret;
}

//チャンネル変更中かどうか
//戻り値：
// TRUE（変更中）、FALSE（完了）
BOOL CBonCtrl::IsChChanging(BOOL* chChgErr)
{
	if( Lock(L"IsChChanging") == FALSE ) return 0;
	BOOL ret = this->tsOut.IsChChanging(chChgErr);
	UnLock();
	return ret;
}

//現在のストリームのIDを取得する
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// ONID		[OUT]originalNetworkID
// TSID		[OUT]transportStreamID
BOOL CBonCtrl::GetStreamID(
	WORD* ONID,
	WORD* TSID
	)
{
	if( Lock(L"GetStreamID") == FALSE ) return 0;
	BOOL ret = this->tsOut.GetStreamID(ONID, TSID);
	UnLock();
	return ret;
}

//シグナルレベルの取得
//戻り値：
// シグナルレベル
float CBonCtrl::GetSignalLevel()
{
	if( Lock(L"GetSignalLevel") == FALSE ) return 0;
	float ret = this->bonUtil.GetSignalLevel();
	this->tsOut.SetSignalLevel(ret);
	UnLock();
	return ret;
}

//BonDriverをロード後の初期化処理
//戻り値：
// エラーコード
DWORD CBonCtrl::_OpenBonDriver()
{
	if( this->recvThread == NULL ){
		//受信スレッド起動
		ResetEvent(this->recvStopEvent);
		this->recvThread = (HANDLE)_beginthreadex(NULL, 0, RecvThread, (LPVOID)this, CREATE_SUSPENDED, NULL);
		SetThreadPriority( this->recvThread, THREAD_PRIORITY_NORMAL );
		ResumeThread(this->recvThread);
	}
	if( this->analyzeThread == NULL ){
		//解析スレッド起動
		ResetEvent(this->analyzeStopEvent);
		this->analyzeThread = (HANDLE)_beginthreadex(NULL, 0, AnalyzeThread, (LPVOID)this, CREATE_SUSPENDED, NULL);
		SetThreadPriority( this->analyzeThread, THREAD_PRIORITY_NORMAL );
		ResumeThread(this->analyzeThread);
	}

	return NO_ERR;
}

//ロードしているBonDriverの開放本体
//戻り値：
// エラーコード
DWORD CBonCtrl::_CloseBonDriver()
{
	if( this->epgCapBackThread != NULL ){
		::SetEvent(this->epgCapBackStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->epgCapBackThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->epgCapBackThread, 0xffffffff);
		}
		CloseHandle(this->epgCapBackThread);
		this->epgCapBackThread = NULL;
	}

	if( this->epgCapThread != NULL ){
		::SetEvent(this->epgCapStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->epgCapThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->epgCapThread, 0xffffffff);
		}
		CloseHandle(this->epgCapThread);
		this->epgCapThread = NULL;
	}

	if( this->recvThread != NULL ){
		::SetEvent(this->recvStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->recvThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->recvThread, 0xffffffff);
		}
		CloseHandle(this->recvThread);
		this->recvThread = NULL;
	}
	if( this->analyzeThread != NULL ){
		::SetEvent(this->analyzeStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->analyzeThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->analyzeThread, 0xffffffff);
		}
		CloseHandle(this->analyzeThread);
		this->analyzeThread = NULL;
	}

	DWORD ret = this->bonUtil.CloseBonDriver();

	{
		CBlockLock lock(&this->buffLock);
		for( size_t i=0; i<this->TSBuff.size(); i++ ){
			SAFE_DELETE(this->TSBuff[i])
		}
		this->TSBuff.clear();
		this->totalTSBuffSize = 0;
	}
	this->packetInit.ClearBuff();

	return ret;
}

UINT WINAPI CBonCtrl::RecvThread(LPVOID param)
{
	CBonCtrl* sys = (CBonCtrl*)param;
	while(1){
		if( ::WaitForSingleObject(sys->recvStopEvent, 0) != WAIT_TIMEOUT ){
			//キャンセルされた
			break;
		}
		BYTE *data = NULL;
		DWORD size = 0;
		DWORD remain = 0;
		try{
			if( sys->bonUtil.GetTsStream(&data,&size,&remain) == TRUE ){
				if( size != 0 && data != NULL){
					TS_DATA* item = new TS_DATA;
					try{
						if( sys->packetInit.GetTSData(data, size, &item->data, &item->size) == TRUE ){
							CBlockLock lock(&sys->buffLock);
							if( sys->totalTSBuffSize / 48128 > sys->tsBuffMaxCount ){
								while( sys->TSBuff.empty() == false && sys->totalTSBuffSize / 48128 + 1000 > sys->tsBuffMaxCount ){
									sys->totalTSBuffSize -= sys->TSBuff.back()->size;
									SAFE_DELETE(sys->TSBuff.back());
									sys->TSBuff.pop_back();
								}
							}
							sys->TSBuff.push_back(item);
							sys->totalTSBuffSize += sys->TSBuff.back()->size;
						}else{
							delete item;
						}
					}catch(...){
						delete item;
						_OutputDebugString(L"★★RecvThread Exception2");
					}
				}else{
					Sleep(10);
				}
			}else{
				Sleep(10);
			}
		}catch(...){
			_OutputDebugString(L"★★RecvThread Exception1");
		}
	}
	return 0;
}

UINT WINAPI CBonCtrl::AnalyzeThread(LPVOID param)
{
	CBonCtrl* sys = (CBonCtrl*)param;

	while(1){
		if( ::WaitForSingleObject(sys->analyzeStopEvent, 0) != WAIT_TIMEOUT ){
			//キャンセルされた
			break;
		}

		//バッファからデータ取り出し
		TS_DATA* data = NULL;
		{
			CBlockLock lock(&sys->buffLock);
			if( sys->TSBuff.empty() == false ){
				data = sys->TSBuff.front();
				sys->totalTSBuffSize -= data->size;
				sys->TSBuff.erase(sys->TSBuff.begin());
			}
		}
		try{
			if( data != NULL ){
				sys->tsOut.AddTSBuff(data);
				SAFE_DELETE(data);
			}else{
				Sleep(5);
			}
		}catch(...){
			_OutputDebugString(L"★★AnalyzeThread Exception2");
		}
	}
	return 0;
}

//EPGデータの蓄積状態をリセットする
void CBonCtrl::ClearSectionStatus()
{
	if( Lock(L"ClearSectionStatus") == FALSE ) return ;
	this->tsOut.ClearSectionStatus();

	UnLock();
	return ;
}

//EPGデータの蓄積状態を取得する
//戻り値：
// ステータス
//引数：
// l_eitFlag		[IN]L-EITのステータスを取得
EPG_SECTION_STATUS CBonCtrl::GetSectionStatus(
	BOOL l_eitFlag
	)
{
	if( Lock(L"GetSectionStatus") == FALSE ) return EpgNoData;
	EPG_SECTION_STATUS status = this->tsOut.GetSectionStatus(l_eitFlag);

	UnLock();
	return status;
}

//自ストリームのサービス一覧を取得する
//戻り値：
// エラーコード
//引数：
// serviceList				[OUT]サービス情報のリスト
DWORD CBonCtrl::GetServiceListActual(
	vector<TS_SERVICE_INFO>* serviceList
	)
{
	if( Lock(L"GetServiceListActual") == FALSE ) return EpgNoData;

	DWORD _serviceListSize = 0;
	SERVICE_INFO* _serviceList = NULL;
	DWORD err = this->tsOut.GetServiceListActual(&_serviceListSize, &_serviceList);
	if( err == NO_ERR ){
		for( DWORD i=0; i<_serviceListSize; i++ ){
			TS_SERVICE_INFO item;
			item.ONID = _serviceList[i].original_network_id;
			item.TSID = _serviceList[i].transport_stream_id;
			item.SID = _serviceList[i].service_id;
			if( _serviceList[i].extInfo != NULL ){
				item.serviceType = _serviceList[i].extInfo->service_type;
				item.partialFlag = _serviceList[i].extInfo->partialReceptionFlag;
				if( _serviceList[i].extInfo->service_name != NULL ){
					item.serviceName = _serviceList[i].extInfo->service_name;
				}else{
					item.serviceName = L"";
				}
				if( _serviceList[i].extInfo->network_name != NULL ){
					item.networkName = _serviceList[i].extInfo->network_name;
				}else{
					item.networkName = L"";
				}
				item.remoteControlKeyID = _serviceList[i].extInfo->remote_control_key_id;
			}else{
				item.serviceType = 0;
				item.partialFlag = 0;
				item.serviceName = L"";
				item.networkName = L"";
				item.remoteControlKeyID = 0;
			}
			serviceList->push_back(item);
		}
	}

	UnLock();
	return err;
}

//サービス一覧を取得する
//戻り値：
// エラーコード
//引数：
// serviceList				[OUT]サービス情報のリスト
DWORD CBonCtrl::GetServiceList(
	vector<CH_DATA4>* serviceList
	)
{
	if( Lock(L"GetServiceList") == FALSE ) return FALSE;
	BOOL ret = this->chUtil.GetEnumService(serviceList);

	UnLock();
	return ret;
}

//TSストリーム制御用コントロールを作成する
//戻り値：
// エラーコード
//引数：
// id			[OUT]制御識別ID
BOOL CBonCtrl::CreateServiceCtrl(
	DWORD* id
	)
{
	if( Lock(L"CreateServiceCtrl") == FALSE ) return FALSE;
	BOOL ret = this->tsOut.CreateServiceCtrl(id);

	UnLock();
	return ret;
}

//TSストリーム制御用コントロールを作成する
//戻り値：
// エラーコード
//引数：
// id			[IN]制御識別ID
BOOL CBonCtrl::DeleteServiceCtrl(
	DWORD id
	)
{
	if( Lock(L"DeleteServiceCtrl") == FALSE ) return FALSE;
	BOOL ret = this->tsOut.DeleteServiceCtrl(id);

	UnLock();
	return ret;
}

//制御対象のサービスを設定する
BOOL CBonCtrl::SetServiceID(
	DWORD id,
	WORD serviceID
	)
{
	if( Lock(L"SetServiceID") == FALSE ) return FALSE;
	BOOL ret = this->tsOut.SetServiceID(id,serviceID);

	UnLock();
	return ret;
}

BOOL CBonCtrl::GetServiceID(
	DWORD id,
	WORD* serviceID
	)
{
	if( Lock(L"SetServiceID") == FALSE ) return FALSE;
	BOOL ret = this->tsOut.GetServiceID(id,serviceID);

	UnLock();
	return ret;
}

//UDPで送信を行う
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// id			[IN]制御識別ID
// sendList		[IN/OUT]送信先リスト。NULLで停止。Portは実際に送信に使用したPortが返る。。
BOOL CBonCtrl::SendUdp(
	DWORD id,
	vector<NW_SEND_INFO>* sendList
	)
{
	if( Lock(L"SendUdp") == FALSE ) return FALSE;
	BOOL ret = this->tsOut.SendUdp(id,sendList);

	UnLock();
	return ret;
}

//TCPで送信を行う
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// id			[IN]制御識別ID
// sendList		[IN/OUT]送信先リスト。NULLで停止。Portは実際に送信に使用したPortが返る。
BOOL CBonCtrl::SendTcp(
	DWORD id,
	vector<NW_SEND_INFO>* sendList
	)
{
	if( Lock(L"SendTcp") == FALSE ) return FALSE;
	BOOL ret = this->tsOut.SendTcp(id,sendList);

	UnLock();
	return ret;
}

//ファイル保存を開始する
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// id					[IN]制御識別ID
// fileName				[IN]保存ファイルパス
// overWriteFlag		[IN]同一ファイル名存在時に上書きするかどうか（TRUE：する、FALSE：しない）
// pittariFlag			[IN]ぴったりモード（TRUE：する、FALSE：しない）
// pittariONID			[IN]ぴったりモードで録画するONID
// pittariTSID			[IN]ぴったりモードで録画するTSID
// pittariSID			[IN]ぴったりモードで録画するSID
// pittariEventID		[IN]ぴったりモードで録画するイベントID
// createSize			[IN]ファイル作成時にディスクに予約する容量
// saveFolder			[IN]使用するフォルダ一覧
// saveFolderSub		[IN]HDDの空きがなくなった場合に一時的に使用するフォルダ
BOOL CBonCtrl::StartSave(
	DWORD id,
	wstring fileName,
	BOOL overWriteFlag,
	BOOL pittariFlag,
	WORD pittariONID,
	WORD pittariTSID,
	WORD pittariSID,
	WORD pittariEventID,
	ULONGLONG createSize,
	vector<REC_FILE_SET_INFO>* saveFolder,
	vector<wstring>* saveFolderSub
)
{
	if( Lock(L"StartSave") == FALSE ) return FALSE;
	BOOL ret = this->tsOut.StartSave(id, fileName, overWriteFlag, pittariFlag, pittariONID, pittariTSID, pittariSID, pittariEventID, createSize, saveFolder, saveFolderSub, writeBuffMaxCount);

	StartBackgroundEpgCap();

	UnLock();
	return ret;
}

//ファイル保存を終了する
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// id			[IN]制御識別ID
BOOL CBonCtrl::EndSave(
	DWORD id
	)
{
	if( Lock(L"EndSave") == FALSE ) return FALSE;
	BOOL ret = this->tsOut.EndSave(id);

	UnLock();
	return ret;
}

//スクランブル解除処理の動作設定
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// enable		[IN] TRUE（処理する）、FALSE（処理しない）
BOOL CBonCtrl::SetScramble(
	DWORD id,
	BOOL enable
	)
{
	if( Lock(L"SetScramble") == FALSE ) return FALSE;
	BOOL ret = this->tsOut.SetScramble(id, enable);

	UnLock();
	return ret;
}

//字幕とデータ放送含めるかどうか
//引数：
// id					[IN]制御識別ID
// enableCaption		[IN]字幕を TRUE（含める）、FALSE（含めない）
// enableData			[IN]データ放送を TRUE（含める）、FALSE（含めない）
void CBonCtrl::SetServiceMode(
	DWORD id,
	BOOL enableCaption,
	BOOL enableData
	)
{
	if( Lock(L"SetServiceMode") == FALSE ) return ;
	this->tsOut.SetServiceMode(id, enableCaption, enableData);

	UnLock();
}

//エラーカウントをクリアする
void CBonCtrl::ClearErrCount(
	DWORD id
	)
{
	if( Lock(L"ClearErrCount") == FALSE ) return ;
	this->tsOut.ClearErrCount(id);

	UnLock();
}

//ドロップとスクランブルのカウントを取得する
//引数：
// drop				[OUT]ドロップ数
// scramble			[OUT]スクランブル数
void CBonCtrl::GetErrCount(
	DWORD id,
	ULONGLONG* drop,
	ULONGLONG* scramble
	)
{
	if( Lock(L"GetErrCount") == FALSE ) return ;
	this->tsOut.GetErrCount(id, drop, scramble);

	UnLock();
}

//指定サービスの現在or次のEPG情報を取得する
//戻り値：
// エラーコード
//引数：
// originalNetworkID		[IN]取得対象のoriginalNetworkID
// transportStreamID		[IN]取得対象のtransportStreamID
// serviceID				[IN]取得対象のServiceID
// nextFlag					[IN]TRUE（次の番組）、FALSE（現在の番組）
// epgInfo					[OUT]EPG情報（DLL内で自動的にdeleteする。次に取得を行うまで有効）
DWORD CBonCtrl::GetEpgInfo(
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	BOOL nextFlag,
	EPGDB_EVENT_INFO* epgInfo
	)
{
	if( Lock(L"GetEpgInfo") == FALSE ) return FALSE;
	DWORD ret = this->tsOut.GetEpgInfo(originalNetworkID, transportStreamID, serviceID, nextFlag, epgInfo);

	UnLock();
	return ret;
}

//指定イベントのEPG情報を取得する
//戻り値：
// エラーコード
//引数：
// originalNetworkID		[IN]取得対象のoriginalNetworkID
// transportStreamID		[IN]取得対象のtransportStreamID
// serviceID				[IN]取得対象のServiceID
// eventID					[IN]取得対象のEventID
// pfOnlyFlag				[IN]p/fからのみ検索するかどうか
// epgInfo					[OUT]EPG情報（DLL内で自動的にdeleteする。次に取得を行うまで有効）
DWORD CBonCtrl::SearchEpgInfo(
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	WORD eventID,
	BYTE pfOnlyFlag,
	EPGDB_EVENT_INFO* epgInfo
	)
{
	if( Lock(L"SearchEpgInfo") == FALSE ) return FALSE;
	DWORD ret = this->tsOut.SearchEpgInfo(originalNetworkID, transportStreamID, serviceID, eventID, pfOnlyFlag, epgInfo);

	UnLock();
	return ret;
}

//PC時計を元としたストリーム時間との差を取得する
//戻り値：
// 差の秒数
int CBonCtrl::GetTimeDelay(
	)
{
	if( Lock(L"GetTimeDelay") == FALSE ) return 0;
	int delay = this->tsOut.GetTimeDelay();

	UnLock();
	return delay;
}

//録画中かどうかを取得する
// TRUE（録画中）、FALSE（録画していない）
BOOL CBonCtrl::IsRec()
{
	if( Lock(L"IsRec") == FALSE ) return FALSE;

	BOOL ret = this->tsOut.IsRec();

	UnLock();
	return ret;
}
//チャンネルスキャンを開始する
//戻り値：
// エラーコード
DWORD CBonCtrl::StartChScan()
{
	if( Lock(L"StartChScan") == FALSE ) return ERR_FALSE;

	if( this->tsOut.IsRec() == TRUE ){
		UnLock();
		return ERR_FALSE;
	}
	if( this->epgSt_err == ST_WORKING ){
		UnLock();
		return ERR_FALSE;
	}

	if( ::WaitForSingleObject(this->chScanThread, 0) == WAIT_OBJECT_0 ){
		CloseHandle(this->chScanThread);
		this->chScanThread = NULL;
	}

	DWORD ret = ERR_FALSE;
	if( this->chScanThread == NULL ){
		if( this->bonUtil.GetOpenBonDriverFileName().empty() == false ){
			ret = NO_ERR;
			this->chSt_space = 0;
			this->chSt_ch = 0;
			this->chSt_chName = L"";
			this->chSt_chkNum = 0;
			this->chSt_totalNum = 0;
			this->chSt_err = ST_WORKING;

			//受信スレッド起動
			ResetEvent(this->chScanStopEvent);
			this->chScanThread = (HANDLE)_beginthreadex(NULL, 0, ChScanThread, (LPVOID)this, CREATE_SUSPENDED, NULL);
			SetThreadPriority( this->chScanThread, THREAD_PRIORITY_NORMAL );
			ResumeThread(this->chScanThread);
		}
	}

	UnLock();
	return ret;
}

//チャンネルスキャンをキャンセルする
//戻り値：
// エラーコード
DWORD CBonCtrl::StopChScan()
{
	if( Lock(L"StopChScan") == FALSE ) return ERR_FALSE;

	if( this->chScanThread != NULL ){
		::SetEvent(this->chScanStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->chScanThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->chScanThread, 0xffffffff);
		}
		CloseHandle(this->chScanThread);
		this->chScanThread = NULL;
	}

	UnLock();
	return NO_ERR;
}

//チャンネルスキャンの状態を取得する
//戻り値：
// エラーコード
//引数：
// space		[OUT]スキャン中の物理CHのspace
// ch			[OUT]スキャン中の物理CHのch
// chName		[OUT]スキャン中の物理CHの名前
// chkNum		[OUT]チェック済みの数
// totalNum		[OUT]チェック対象の総数
DWORD CBonCtrl::GetChScanStatus(
	DWORD* space,
	DWORD* ch,
	wstring* chName,
	DWORD* chkNum,
	DWORD* totalNum
	)
{
	if( Lock(L"GetChScanStatus") == FALSE ) return ERR_FALSE;

	DWORD ret;
	if( space != NULL ){
		*space = this->chSt_space;
	}
	if( ch != NULL ){
		*ch = this->chSt_ch;
	}
	if( chName != NULL ){
		*chName = this->chSt_chName;
	}
	if( chkNum != NULL ){
		*chkNum = this->chSt_chkNum;
	}
	if( totalNum != NULL ){
		*totalNum = this->chSt_totalNum;
	}
	ret = this->chSt_err;

	UnLock();
	return ret;
}

UINT WINAPI CBonCtrl::ChScanThread(LPVOID param)
{
	CBonCtrl* sys = (CBonCtrl*)param;

	sys->chUtil.Clear();

	wstring settingPath;
	GetSettingPath(settingPath);
	wstring bonFileTitle;
	GetFileTitle(sys->bonUtil.GetOpenBonDriverFileName(), bonFileTitle);
	wstring tunerName = sys->bonUtil.GetTunerName();
	CheckFileName(tunerName);

	wstring chSet4 = settingPath + L"\\" + bonFileTitle + L"(" + tunerName + L").ChSet4.txt";
	wstring chSet5 = settingPath + L"\\ChSet5.txt";

	vector<CHK_CH_INFO> chkList;
	map<DWORD, BON_SPACE_INFO> spaceMap;
	if( sys->bonUtil.GetOriginalChList(&spaceMap) != NO_ERR ){
		sys->chSt_err = ST_COMPLETE;
		sys->chUtil.SaveChSet(chSet4, chSet5);
		return 0;
	}
	map<DWORD, BON_SPACE_INFO>::iterator itrSpace;
	for( itrSpace = spaceMap.begin(); itrSpace != spaceMap.end(); itrSpace++ ){
		sys->chSt_totalNum += (DWORD)itrSpace->second.chMap.size();
		map<DWORD, wstring>::iterator itrCh;
		for( itrCh = itrSpace->second.chMap.begin(); itrCh != itrSpace->second.chMap.end(); itrCh++ ){
			CHK_CH_INFO item;
			item.space = itrSpace->first;
			item.spaceName = itrSpace->second.spaceName;
			item.ch = itrCh->first;
			item.chName = itrCh->second;
			chkList.push_back(item);
		}
	}

	if( sys->chSt_totalNum == 0 ){
		sys->chSt_err = ST_COMPLETE;
		sys->chUtil.SaveChSet(chSet4, chSet5);
		return 0;
	}

	wstring folderPath;
	GetModuleFolderPath( folderPath );
	wstring iniPath = folderPath;
	iniPath += L"\\BonCtrl.ini";

	DWORD chChgTimeOut = GetPrivateProfileInt(L"CHSCAN", L"ChChgTimeOut", 9, iniPath.c_str());
	DWORD serviceChkTimeOut = GetPrivateProfileInt(L"CHSCAN", L"ServiceChkTimeOut", 8, iniPath.c_str());


	DWORD wait = 0;
	BOOL chkNext = TRUE;
	DWORD startTime = 0;
	DWORD chkWait = 0;
	DWORD chkCount = 0;
	BOOL firstChg = FALSE;

	while(1){
		if( ::WaitForSingleObject(sys->chScanStopEvent, wait) != WAIT_TIMEOUT ){
			//キャンセルされた
			sys->chSt_err = ST_CANCEL;
			break;
		}
		if( chkNext == TRUE ){
			sys->chSt_space = chkList[chkCount].space;
			sys->chSt_ch = chkList[chkCount].ch;
			sys->chSt_chName = chkList[chkCount].chName;
			sys->_SetCh(chkList[chkCount].space, chkList[chkCount].ch, TRUE);
			if( firstChg == FALSE ){
				firstChg = TRUE;
				sys->tsOut.ResetChChange();
			}
			startTime = GetTickCount();
			chkNext = FALSE;
			wait = 1000;
			chkWait = chChgTimeOut;
		}else{
			BOOL chChgErr = FALSE;
			if( sys->tsOut.IsChChanging(&chChgErr) == TRUE ){
				if( GetTickCount() - startTime > chkWait * 1000 ){
					//チャンネル切り替えに8秒以上かかってるので無信号と判断
					OutputDebugString(L"★AutoScan Ch Change timeout\r\n");
					chkNext = TRUE;
				}
			}else{
				if( GetTickCount() - startTime > (chkWait + serviceChkTimeOut) * 1000 || chChgErr == TRUE){
					//チャンネル切り替え成功したけどサービス一覧とれないので無信号と判断
					OutputDebugString(L"★AutoScan GetService timeout\r\n");
					chkNext = TRUE;
				}else{
					//サービス一覧の取得を行う
					DWORD serviceListSize;
					SERVICE_INFO* serviceList;
					if( sys->tsOut.GetServiceListActual(&serviceListSize, &serviceList) == NO_ERR ){
						if( serviceListSize > 0 ){
							//一覧の取得ができた
							for( DWORD i=0 ;i<serviceListSize; i++ ){
								if( serviceList[i].extInfo != NULL ){
									if( serviceList[i].extInfo->service_name != NULL ){
										if( wcslen(serviceList[i].extInfo->service_name) > 0 ){
											sys->chUtil.AddServiceInfo(chkList[chkCount].space, chkList[chkCount].ch, chkList[chkCount].chName, &(serviceList[i]));
										}
									}
								}
							}
							chkNext = TRUE;
						}
					}
				}
			}
			if( chkNext == TRUE ){
				//次のチャンネルへ
				chkCount++;
				sys->chSt_chkNum++;
				if( sys->chSt_totalNum <= chkCount ){
					//全部チェック終わったので終了
					sys->chSt_err = ST_COMPLETE;
					sys->chUtil.SaveChSet(chSet4, chSet5);
					break;
				}
			}
		}
	}

	sys->chUtil.LoadChSet(chSet4, chSet5);

	return 0;
}

//EPG取得対象のサービス一覧を取得する
//戻り値：
// エラーコード
//引数：
// chList		[OUT]EPG取得するチャンネル一覧
DWORD CBonCtrl::GetEpgCapService(
	vector<EPGCAP_SERVICE_INFO>* chList
	)
{
	if( Lock(L"GetEpgCapService") == FALSE ) return ERR_FALSE;

	DWORD ret = NO_ERR;
	this->chUtil.GetEpgCapService(chList);
	UnLock();
	return ret;
}

//EPG取得を開始する
//戻り値：
// エラーコード
//引数：
// chList		[IN]EPG取得するチャンネル一覧
DWORD CBonCtrl::StartEpgCap(
	vector<EPGCAP_SERVICE_INFO>* chList
	)
{
	if( Lock(L"StartEpgCap") == FALSE ) return ERR_FALSE;

	if( this->tsOut.IsRec() == TRUE ){
		UnLock();
		return ERR_FALSE;
	}
	if( this->chSt_err == ST_WORKING ){
		UnLock();
		return ERR_FALSE;
	}

	StopBackgroundEpgCap();

	if( ::WaitForSingleObject(this->epgCapThread, 0) == WAIT_OBJECT_0 ){
		CloseHandle(this->epgCapThread);
		this->epgCapThread = NULL;
	}

	DWORD ret = ERR_FALSE;
	if( this->epgCapThread == NULL ){
		if( this->bonUtil.GetOpenBonDriverFileName().empty() == false ){
			ret = NO_ERR;
			this->epgCapChList = *chList;
			this->epgSt_err = ST_WORKING;
			this->epgSt_ch.ONID = 0xFFFF;
			this->epgSt_ch.TSID = 0xFFFF;
			this->epgSt_ch.SID = 0xFFFF;

			//受信スレッド起動
			ResetEvent(this->epgCapStopEvent);
			this->epgCapThread = (HANDLE)_beginthreadex(NULL, 0, EpgCapThread, (LPVOID)this, CREATE_SUSPENDED, NULL);
			SetThreadPriority( this->epgCapThread, THREAD_PRIORITY_NORMAL );
			ResumeThread(this->epgCapThread);
		}
	}

	UnLock();
	return ret;
}

//EPG取得を停止する
//戻り値：
// エラーコード
DWORD CBonCtrl::StopEpgCap(
	)
{
	if( Lock(L"StopEpgCap") == FALSE ) return ERR_FALSE;

	if( this->epgCapThread != NULL ){
		::SetEvent(this->epgCapStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->epgCapThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->epgCapThread, 0xffffffff);
		}
		CloseHandle(this->epgCapThread);
		this->epgCapThread = NULL;
	}

	UnLock();
	return NO_ERR;
}

//EPG取得のステータスを取得する
//戻り値：
// エラーコード
//引数：
// info			[OUT]取得中のサービス
DWORD CBonCtrl::GetEpgCapStatus(
	EPGCAP_SERVICE_INFO* info
	)
{
	if( Lock(L"GetEpgCapStatus") == FALSE ) return ERR_FALSE;

	if( info != NULL ){
		*info = this->epgSt_ch;
	}
	DWORD ret = this->epgSt_err;

	UnLock();
	return ret;
}

UINT WINAPI CBonCtrl::EpgCapThread(LPVOID param)
{
	CBonCtrl* sys = (CBonCtrl*)param;

	BOOL chkNext = TRUE;
	BOOL startCap = FALSE;
	DWORD wait = 0;
	DWORD startTime = 0;
	DWORD chkCount = 0;
	DWORD chkWait = 8;

	BOOL chkBS = FALSE;
	BOOL chkCS1 = FALSE;
	BOOL chkCS2 = FALSE;

	wstring folderPath;
	GetModuleFolderPath( folderPath );
	wstring iniPath = folderPath;
	iniPath += L"\\BonCtrl.ini";

	DWORD timeOut = GetPrivateProfileInt(L"EPGCAP", L"EpgCapTimeOut", 15, iniPath.c_str());
	BOOL saveTimeOut = GetPrivateProfileInt(L"EPGCAP", L"EpgCapSaveTimeOut", 0, iniPath.c_str());

	//Common.iniは一般に外部プロセスが変更する可能性のある(はずの)ものなので、利用の直前にチェックする
	wstring commonIniPath;
	GetCommonIniPath(commonIniPath);
	BOOL BSBasic = GetPrivateProfileInt(L"SET", L"BSBasicOnly", 1, commonIniPath.c_str());
	BOOL CS1Basic = GetPrivateProfileInt(L"SET", L"CS1BasicOnly", 1, commonIniPath.c_str());
	BOOL CS2Basic = GetPrivateProfileInt(L"SET", L"CS2BasicOnly", 1, commonIniPath.c_str());

	while(1){
		if( ::WaitForSingleObject(sys->epgCapStopEvent, wait) != WAIT_TIMEOUT ){
			//キャンセルされた
			sys->epgSt_err = ST_CANCEL;
			sys->tsOut.StopSaveEPG(FALSE);
			break;
		}
		if( chkNext == TRUE ){
			if( sys->tsOut.IsChChanging(NULL) == TRUE ){
				Sleep(200);
				continue;
			}
			DWORD space = 0;
			DWORD ch = 0;
			sys->chUtil.GetCh(sys->epgCapChList[chkCount].ONID, sys->epgCapChList[chkCount].TSID, space, ch);
			sys->_SetCh(space, ch);
			startTime = GetTickCount();
			chkNext = FALSE;
			startCap = FALSE;
			wait = 1000;
			if( sys->epgCapChList[chkCount].ONID == 4 ){
				chkBS = TRUE;
			}else if( sys->epgCapChList[chkCount].ONID == 6 ){
				chkCS1 = TRUE;
			}else if( sys->epgCapChList[chkCount].ONID == 7 ){
				chkCS2 = TRUE;
			}
			sys->epgSt_ch = sys->epgCapChList[chkCount];
		}else{
			BOOL chChgErr = FALSE;
			if( sys->tsOut.IsChChanging(&chChgErr) == TRUE ){
				if( GetTickCount() - startTime > chkWait * 1000 ){
					//チャンネル切り替えに10秒以上かかってるので無信号と判断
					chkNext = TRUE;
				}
			}else{
				if( GetTickCount() - startTime > (chkWait + timeOut * 60) * 1000 || chChgErr == TRUE){
					//15分以上かかっているなら停止
					sys->tsOut.StopSaveEPG(saveTimeOut);
					chkNext = TRUE;
					wait = 0;
					_OutputDebugString(L"++%d分でEPG取得完了せず or Ch変更でエラー", timeOut);
				}else if( GetTickCount() - startTime > chkWait * 1000 ){
					//切り替えから15秒以上過ぎているので取得処理
					if( startCap == FALSE ){
						//取得開始
						startCap = TRUE;
						wstring epgDataPath = L"";
						sys->GetEpgDataFilePath(sys->epgCapChList[chkCount].ONID, sys->epgCapChList[chkCount].TSID, epgDataPath, BSBasic, CS1Basic, CS2Basic);
						sys->tsOut.StartSaveEPG(epgDataPath);
						sys->tsOut.ClearSectionStatus();
						wait = 60*1000;
					}else{
						//蓄積状態チェック
						BOOL leitFlag = sys->chUtil.IsPartial(sys->epgCapChList[chkCount].ONID, sys->epgCapChList[chkCount].TSID, sys->epgCapChList[chkCount].SID);
						EPG_SECTION_STATUS status = sys->tsOut.GetSectionStatus(leitFlag);
						if( sys->epgCapChList[chkCount].ONID == 4 && BSBasic == TRUE ){
							if( status == EpgBasicAll || status == EpgHEITAll ){
								chkNext = TRUE;
							}
						}else if( sys->epgCapChList[chkCount].ONID == 6 && CS1Basic == TRUE ){
							if( status == EpgBasicAll || status == EpgHEITAll ){
								chkNext = TRUE;
							}
						}else if( sys->epgCapChList[chkCount].ONID == 7 && CS2Basic == TRUE ){
							if( status == EpgBasicAll || status == EpgHEITAll ){
								chkNext = TRUE;
							}
						}else{
							if( leitFlag == FALSE && status == EpgHEITAll ){
								chkNext = TRUE;
							}else if( leitFlag == TRUE && status == EpgLEITAll ){
								chkNext = TRUE;
							}
						}
						if( chkNext == TRUE ){
							sys->tsOut.StopSaveEPG(TRUE);
							wait = 0;
						}else{
							wait = 10*1000;
						}
					}
				}
			}
			if( chkNext == TRUE ){
				//次のチャンネルへ
				chkCount++;
				if( sys->epgCapChList.size() <= chkCount ){
					//全部チェック終わったので終了
					sys->epgSt_err = ST_COMPLETE;
					return 0;
				}
				//BS 1チャンネルのみ？
				if( sys->epgCapChList[chkCount].ONID == 4 && BSBasic == TRUE && chkBS == TRUE){
					while(chkCount<(DWORD)sys->epgCapChList.size()){
						if( sys->epgCapChList[chkCount].ONID != 4 ){
							break;
						}
						chkCount++;
						if( sys->epgCapChList.size() <= chkCount ){
							//全部チェック終わったので終了
							sys->epgSt_err = ST_COMPLETE;
							return 0;
						}
					}
				}
				//CS1 1チャンネルのみ？
				if( sys->epgCapChList[chkCount].ONID == 6 && CS1Basic == TRUE && chkCS1 == TRUE ){
					while(chkCount<(DWORD)sys->epgCapChList.size()){
						if( sys->epgCapChList[chkCount].ONID != 6 ){
							break;
						}
						chkCount++;
						if( sys->epgCapChList.size() <= chkCount ){
							//全部チェック終わったので終了
							sys->epgSt_err = ST_COMPLETE;
							return 0;
						}
					}
				}
				//CS2 1チャンネルのみ？
				if( sys->epgCapChList[chkCount].ONID == 7 && CS2Basic == TRUE && chkCS2 == TRUE ){
					while(chkCount<(DWORD)sys->epgCapChList.size()){
						if( sys->epgCapChList[chkCount].ONID != 7 ){
							break;
						}
						chkCount++;
						if( sys->epgCapChList.size() <= chkCount ){
							//全部チェック終わったので終了
							sys->epgSt_err = ST_COMPLETE;
							return 0;
						}
					}
				}
			}
		}
	}
	return 0;
}

void CBonCtrl::GetEpgDataFilePath(WORD ONID, WORD TSID, wstring& epgDataFilePath, BOOL BSBasic, BOOL CS1Basic, BOOL CS2Basic)
{
	wstring epgDataFolderPath = L"";
	GetSettingPath(epgDataFolderPath);
	epgDataFolderPath += EPG_SAVE_FOLDER;

	if( ONID == 4 && BSBasic == TRUE ){
		Format(epgDataFilePath, L"%s\\%04XFFFF_epg.dat", epgDataFolderPath.c_str(), ONID);
	}else if( ONID == 6 && CS1Basic == TRUE ){
		Format(epgDataFilePath, L"%s\\%04XFFFF_epg.dat", epgDataFolderPath.c_str(), ONID);
	}else if( ONID == 7 && CS2Basic == TRUE ){
		Format(epgDataFilePath, L"%s\\%04XFFFF_epg.dat", epgDataFolderPath.c_str(), ONID);
	}else{
		Format(epgDataFilePath, L"%s\\%04X%04X_epg.dat", epgDataFolderPath.c_str(), ONID, TSID);
	}
}

//録画中のファイルのファイルパスを取得する
//引数：
// id					[IN]制御識別ID
// filePath				[OUT]保存ファイル名
// subRecFlag			[OUT]サブ録画が発生したかどうか
void CBonCtrl::GetSaveFilePath(
	DWORD id,
	wstring* filePath,
	BOOL* subRecFlag
	)
{
	if( Lock(L"GetSaveFilePath") == FALSE ) return ;
	this->tsOut.GetSaveFilePath(id, filePath, subRecFlag);

	UnLock();
}

//ドロップとスクランブルのカウントを保存する
//引数：
// id					[IN]制御識別ID
// filePath				[IN]保存ファイル名
void CBonCtrl::SaveErrCount(
	DWORD id,
	wstring filePath
	)
{
	if( Lock(L"SaveErrCount") == FALSE ) return ;
	this->tsOut.SaveErrCount(id, filePath);

	UnLock();
}

//録画中のファイルの出力サイズを取得する
//引数：
// id					[IN]制御識別ID
// writeSize			[OUT]保存ファイル名
void CBonCtrl::GetRecWriteSize(
	DWORD id,
	__int64* writeSize
	)
{
	if( Lock(L"GetRecWriteSize") == FALSE ) return ;
	this->tsOut.GetRecWriteSize(id, writeSize);

	UnLock();
}

//バックグラウンドでのEPG取得設定
//引数：
// enableLive	[IN]視聴中に取得する
// enableRec	[IN]録画中に取得する
// enableRec	[IN]EPG取得するチャンネル一覧
// BSBasic		[IN]BSで１チャンネルから基本情報のみ取得するかどうか
// CS1Basic		[IN]CS1で１チャンネルから基本情報のみ取得するかどうか
// CS2Basic		[IN]CS2で１チャンネルから基本情報のみ取得するかどうか
// backStartWaitSec	[IN]Ch切り替え、録画開始後、バックグラウンドでのEPG取得を開始するまでの秒数
void CBonCtrl::SetBackGroundEpgCap(
	BOOL enableLive,
	BOOL enableRec,
	BOOL BSBasic,
	BOOL CS1Basic,
	BOOL CS2Basic,
	DWORD backStartWaitSec
	)
{
	if( Lock(L"SetBackGroundEpgCap") == FALSE ) return ;

	this->enableLiveEpgCap = enableLive;
	this->enableRecEpgCap = enableRec;
	this->epgCapBackBSBasic = BSBasic;
	this->epgCapBackCS1Basic = CS1Basic;
	this->epgCapBackCS2Basic = CS2Basic;
	this->epgCapBackStartWaitSec = backStartWaitSec;

	StartBackgroundEpgCap();

	UnLock();
}

void CBonCtrl::StartBackgroundEpgCap()
{
	StopBackgroundEpgCap();
	if( this->epgCapBackThread == NULL && this->epgCapThread == NULL ){
		if( this->bonUtil.GetOpenBonDriverFileName().empty() == false ){
			//受信スレッド起動
			ResetEvent(this->epgCapBackStopEvent);
			this->epgCapBackThread = (HANDLE)_beginthreadex(NULL, 0, EpgCapBackThread, (LPVOID)this, CREATE_SUSPENDED, NULL);
			SetThreadPriority( this->epgCapBackThread, THREAD_PRIORITY_NORMAL );
			ResumeThread(this->epgCapBackThread);
		}
	}
}

void CBonCtrl::StopBackgroundEpgCap()
{
	if( this->epgCapBackThread != NULL ){
		::SetEvent(this->epgCapBackStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->epgCapBackThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->epgCapBackThread, 0xffffffff);
		}
		CloseHandle(this->epgCapBackThread);
		this->epgCapBackThread = NULL;
	}
}

UINT WINAPI CBonCtrl::EpgCapBackThread(LPVOID param)
{
	wstring folderPath;
	GetModuleFolderPath( folderPath );
	wstring iniPath = folderPath;
	iniPath += L"\\BonCtrl.ini";

	DWORD timeOut = GetPrivateProfileInt(L"EPGCAP", L"EpgCapTimeOut", 15, iniPath.c_str());
	BOOL saveTimeOut = GetPrivateProfileInt(L"EPGCAP", L"EpgCapSaveTimeOut", 0, iniPath.c_str());

	CBonCtrl* sys = (CBonCtrl*)param;
	if( ::WaitForSingleObject(sys->epgCapBackStopEvent, sys->epgCapBackStartWaitSec*1000) != WAIT_TIMEOUT ){
		//キャンセルされた
		return 0;
	}

	if( sys->tsOut.IsRec() == TRUE ){
		if( sys->enableRecEpgCap == FALSE ){
			return 0;
		}
	}else{
		if( sys->enableLiveEpgCap == FALSE ){
			return 0;
		}
	}

	DWORD startTime = GetTickCount();

	wstring epgDataPath = L"";
	WORD ONID;
	WORD TSID;
	sys->tsOut.GetStreamID(&ONID, &TSID);

	if( sys->chUtil.IsEpgCapService(ONID, TSID) == FALSE ){
		return 0;
	}

	sys->GetEpgDataFilePath(ONID, TSID, epgDataPath, sys->epgCapBackBSBasic, sys->epgCapBackCS1Basic, sys->epgCapBackCS2Basic);
	sys->tsOut.StartSaveEPG(epgDataPath);
	sys->tsOut.ClearSectionStatus();

	if( ::WaitForSingleObject(sys->epgCapBackStopEvent, 60*1000) != WAIT_TIMEOUT ){
		//キャンセルされた
		sys->tsOut.StopSaveEPG(FALSE);
		return 0;
	}
	while(1){
		//蓄積状態チェック
		BOOL chkNext = FALSE;
		BOOL leitFlag = sys->chUtil.IsPartial(ONID, TSID, sys->lastSID);
		EPG_SECTION_STATUS status = sys->tsOut.GetSectionStatus(leitFlag);
		if( ONID == 4 && sys->epgCapBackBSBasic == TRUE ){
			if( status == EpgBasicAll || status == EpgHEITAll ){
				chkNext = TRUE;
			}
		}else if( ONID == 6 && sys->epgCapBackCS1Basic == TRUE ){
			if( status == EpgBasicAll || status == EpgHEITAll ){
				chkNext = TRUE;
			}
		}else if( ONID == 7 && sys->epgCapBackCS2Basic == TRUE ){
			if( status == EpgBasicAll || status == EpgHEITAll ){
				chkNext = TRUE;
			}
		}else{
			if( leitFlag == FALSE && status == EpgHEITAll ){
				chkNext = TRUE;
			}else if( leitFlag == TRUE && status == EpgLEITAll ){
				chkNext = TRUE;
			}
		}

		if( chkNext == TRUE ){
			sys->tsOut.StopSaveEPG(TRUE);
			CSendCtrlCmd cmd;
			cmd.SetConnectTimeOut(1000);
			cmd.SendReloadEpg();
			break;
		}else{
			if( GetTickCount() - startTime > timeOut * 60 * 1000 ){
				//15分以上かかっているなら停止
				sys->tsOut.StopSaveEPG(saveTimeOut);
				CSendCtrlCmd cmd;
				cmd.SetConnectTimeOut(1000);
				cmd.SendReloadEpg();
				_OutputDebugString(L"++%d分でEPG取得完了せず or Ch変更でエラー", timeOut);
				break;
			}
		}

		if( ::WaitForSingleObject(sys->epgCapBackStopEvent, 10*1000) != WAIT_TIMEOUT ){
			//キャンセルされた
			sys->tsOut.StopSaveEPG(FALSE);
			break;
		}
	}

	return 0;
}

BOOL CBonCtrl::GetViewStatusInfo(
	DWORD id,
	float* signal,
	DWORD* space,
	DWORD* ch,
	ULONGLONG* drop,
	ULONGLONG* scramble
	)
{
	if( Lock(L"GetViewStatusInfo") == FALSE ) return FALSE;
	BOOL ret = FALSE;

	this->tsOut.GetErrCount(id, drop, scramble);

	*signal = this->bonUtil.GetSignalLevel();
	this->tsOut.SetSignalLevel(*signal);

	if( this->bonUtil.GetNowCh(space, ch) == TRUE ){
		ret = TRUE;
	}

	UnLock();
	return ret;
}
