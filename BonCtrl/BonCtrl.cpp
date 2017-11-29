#include "stdafx.h"
#include "BonCtrl.h"
#include <process.h>

#include "../Common/CommonDef.h"
#include "../Common/TimeUtil.h"
#include "../Common/SendCtrlCmd.h"
#include "../Common/BlockLock.h"

CBonCtrl::CBonCtrl(void)
{
	InitializeCriticalSection(&this->buffLock);

    this->analyzeThread = NULL;
    this->analyzeEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    this->analyzeStopFlag = FALSE;

    this->chScanThread = NULL;
    this->chScanStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	this->chScanIndexOrStatus = ST_STOP;

	this->epgCapThread = NULL;
	this->epgCapStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	this->epgCapIndexOrStatus = ST_STOP;

	this->epgCapBackThread = NULL;
	this->epgCapBackStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	this->enableLiveEpgCap = FALSE;
	this->enableRecEpgCap = FALSE;

	this->epgCapBackBSBasic = TRUE;
	this->epgCapBackCS1Basic = TRUE;
	this->epgCapBackCS2Basic = TRUE;
	this->epgCapBackCS3Basic = FALSE;
	this->epgCapBackStartWaitSec = 30;
	this->tsBuffMaxCount = 5000;
	this->writeBuffMaxCount = -1;
}


CBonCtrl::~CBonCtrl(void)
{
	CloseBonDriver();

	StopChScan();
	if( this->chScanStopEvent != NULL ){
		CloseHandle(this->chScanStopEvent);
		this->chScanStopEvent = NULL;
	}
	if( this->epgCapStopEvent != NULL ){
		CloseHandle(this->epgCapStopEvent);
		this->epgCapStopEvent = NULL;
	}
	if( this->epgCapBackStopEvent != NULL ){
		CloseHandle(this->epgCapBackStopEvent);
		this->epgCapBackStopEvent = NULL;
	}
	if( this->analyzeEvent != NULL ){
		CloseHandle(this->analyzeEvent);
		this->analyzeEvent = NULL;
	}
	DeleteCriticalSection(&this->buffLock);
}

//BonDriverフォルダを指定
//引数：
// bonDriverFolderPath		[IN]BonDriverフォルダパス
void CBonCtrl::SetBonDriverFolder(
	LPCWSTR bonDriverFolderPath
)
{
	this->bonUtil.SetBonDriverFolder(bonDriverFolderPath);
}

void CBonCtrl::SetEMMMode(BOOL enable)
{
	this->tsOut.SetEmm(enable);
}

void CBonCtrl::SetNoLogScramble(BOOL noLog)
{
	this->tsOut.SetNoLogScramble(noLog);
}

void CBonCtrl::SetTsBuffMaxCount(DWORD tsBuffMaxCount_, int writeBuffMaxCount_)
{
	this->tsBuffMaxCount = tsBuffMaxCount_;
	this->writeBuffMaxCount = writeBuffMaxCount_;
}

//BonDriverフォルダのBonDriver_*.dllを列挙
//戻り値：
// 検索できたBonDriver一覧
vector<wstring> CBonCtrl::EnumBonDriver()
{
	return this->bonUtil.EnumBonDriver();
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
	CloseBonDriver();
	DWORD ret = ERR_FALSE;
	if( this->bonUtil.OpenBonDriver(bonDriverFile, RecvCallback, this, openWait) ){
		wstring bonFile = this->bonUtil.GetOpenBonDriverFileName();
		ret = NO_ERR;
		//解析スレッド起動
		this->analyzeStopFlag = FALSE;
		this->analyzeThread = (HANDLE)_beginthreadex(NULL, 0, AnalyzeThread, this, 0, NULL);

		this->tsOut.SetBonDriver(bonFile);
		fs_path settingPath = GetSettingPath();
		wstring tunerName = this->bonUtil.GetTunerName();
		CheckFileName(tunerName);
		this->chUtil.LoadChSet(fs_path(settingPath).append(fs_path(bonFile).stem().concat(L"(" + tunerName + L").ChSet4.txt").native()).native(),
		                       fs_path(settingPath).append(L"ChSet5.txt").native());
	}

	return ret;
}

//ロード中のBonDriverのファイル名を取得する（ロード成功しているかの判定）
//※スレッドセーフ
//戻り値：
// TRUE（成功）：FALSE（Openに失敗している）
//引数：
// bonDriverFile		[OUT]BonDriverのファイル名(NULL可)
BOOL CBonCtrl::GetOpenBonDriver(
	wstring* bonDriverFile
	)
{
	BOOL ret = FALSE;

	wstring strBonDriverFile = this->bonUtil.GetOpenBonDriverFileName();
	if( strBonDriverFile.empty() == false ){
		ret = TRUE;
		if( bonDriverFile != NULL ){
			*bonDriverFile = strBonDriverFile;
		}
	}

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
	if( this->tsOut.IsRec() == TRUE ){
		return ERR_FALSE;
	}

	return ProcessSetCh(space, ch);
}

//チャンネル変更
//戻り値：
// エラーコード
//引数：
// ONID			[IN]変更チャンネルのorignal_network_id
// TSID			[IN]変更チャンネルのtransport_stream_id
// SID			[IN]変更チャンネルのservice_id
DWORD CBonCtrl::SetCh(
	WORD ONID,
	WORD TSID,
	WORD SID
)
{
	if( this->tsOut.IsRec() == TRUE ){
		return ERR_FALSE;
	}

	DWORD space=0;
	DWORD ch=0;

	DWORD ret = ERR_FALSE;
	if( this->chUtil.GetCh( ONID, TSID, SID, space, ch ) == TRUE ){
		ret = ProcessSetCh(space, ch);
	}

	return ret;
}

DWORD CBonCtrl::ProcessSetCh(
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
		DWORD elapsed;
		if( this->bonUtil.GetNowCh(&spaceNow, &chNow) == false || space != spaceNow || ch != chNow || this->tsOut.IsChUnknown(&elapsed) && elapsed > 15000 ){
			this->tsOut.SetChChangeEvent(chScan);
			_OutputDebugString(L"SetCh space %d, ch %d", space, ch);
			ret = this->bonUtil.SetCh(space, ch) ? NO_ERR : ERR_FALSE;

			StartBackgroundEpgCap();
		}
	}else{
		OutputDebugString(L"Err GetNowCh");
	}
	return ret;
}

//チャンネル変更中かどうか
//※スレッドセーフ
//戻り値：
// TRUE（変更中）、FALSE（完了）
BOOL CBonCtrl::IsChChanging(BOOL* chChgErr)
{
	if( chChgErr != NULL ){
		*chChgErr = FALSE;
	}
	DWORD elapsed;
	if( this->tsOut.IsChUnknown(&elapsed) && elapsed != MAXDWORD ){
		if( elapsed > 15000 ){
			//タイムアウトした
			if( chChgErr != NULL ){
				*chChgErr = TRUE;
			}
			return FALSE;
		}
		return TRUE;
	}
	return FALSE;
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
	return this->tsOut.GetStreamID(ONID, TSID);
}

//シグナルレベルの取得
//戻り値：
// シグナルレベル
float CBonCtrl::GetSignalLevel()
{
	float ret = this->bonUtil.GetSignalLevel();
	this->tsOut.SetSignalLevel(ret);
	return ret;
}

//ロードしているBonDriverの開放
void CBonCtrl::CloseBonDriver()
{
	StopBackgroundEpgCap();

	StopEpgCap();

	if( this->analyzeThread != NULL ){
		this->analyzeStopFlag = TRUE;
		::SetEvent(this->analyzeEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->analyzeThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->analyzeThread, 0xffffffff);
		}
		CloseHandle(this->analyzeThread);
		this->analyzeThread = NULL;
	}

	this->bonUtil.CloseBonDriver();
	this->packetInit.ClearBuff();
	this->tsBuffList.clear();
}

void CBonCtrl::RecvCallback(void* param, BYTE* data, DWORD size, DWORD remain)
{
	CBonCtrl* sys = (CBonCtrl*)param;
	BYTE* outData;
	DWORD outSize;
	if( data != NULL && size != 0 && sys->packetInit.GetTSData(data, size, &outData, &outSize) ){
		CBlockLock lock(&sys->buffLock);
		for( std::list<vector<BYTE>>::iterator itr = sys->tsBuffList.begin(); outSize != 0; itr++ ){
			if( itr == sys->tsBuffList.end() ){
				//バッファを増やす
				if( sys->tsBuffList.size() > sys->tsBuffMaxCount ){
					for( itr = sys->tsBuffList.begin(); itr != sys->tsBuffList.end(); (itr++)->clear() );
					itr = sys->tsBuffList.begin();
				}else{
					sys->tsBuffList.push_back(vector<BYTE>());
					itr = sys->tsBuffList.end();
					(--itr)->reserve(48128);
				}
			}
			DWORD insertSize = min(48128 - (DWORD)itr->size(), outSize);
			itr->insert(itr->end(), outData, outData + insertSize);
			outData += insertSize;
			outSize -= insertSize;
		}
	}
	if( remain == 0 ){
		SetEvent(sys->analyzeEvent);
	}
}

UINT WINAPI CBonCtrl::AnalyzeThread(LPVOID param)
{
	CBonCtrl* sys = (CBonCtrl*)param;
	std::list<vector<BYTE>> data;

	while( sys->analyzeStopFlag == FALSE ){
		//バッファからデータ取り出し
		{
			CBlockLock lock(&sys->buffLock);
			if( data.empty() == false ){
				//返却
				data.front().clear();
				std::list<vector<BYTE>>::iterator itr;
				for( itr = sys->tsBuffList.begin(); itr != sys->tsBuffList.end() && itr->empty() == false; itr++ );
				sys->tsBuffList.splice(itr, data);
			}
			if( sys->tsBuffList.empty() == false && sys->tsBuffList.front().size() == 48128 ){
				data.splice(data.end(), sys->tsBuffList, sys->tsBuffList.begin());
			}
		}
		if( data.empty() == false ){
			sys->tsOut.AddTSBuff(&data.front().front(), (DWORD)data.front().size());
		}else{
			WaitForSingleObject(sys->analyzeEvent, 1000);
		}
	}
	return 0;
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
	return this->chUtil.GetEnumService(serviceList);
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
	return this->tsOut.CreateServiceCtrl(id);
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
	return this->tsOut.DeleteServiceCtrl(id);
}

//制御対象のサービスを設定する
BOOL CBonCtrl::SetServiceID(
	DWORD id,
	WORD serviceID
	)
{
	return this->tsOut.SetServiceID(id,serviceID);
}

BOOL CBonCtrl::GetServiceID(
	DWORD id,
	WORD* serviceID
	)
{
	return this->tsOut.GetServiceID(id,serviceID);
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
	return this->tsOut.SendUdp(id,sendList);
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
	return this->tsOut.SendTcp(id,sendList);
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
	const wstring& fileName,
	BOOL overWriteFlag,
	BOOL pittariFlag,
	WORD pittariONID,
	WORD pittariTSID,
	WORD pittariSID,
	WORD pittariEventID,
	ULONGLONG createSize,
	const vector<REC_FILE_SET_INFO>& saveFolder,
	const vector<wstring>& saveFolderSub
)
{
	BOOL ret = this->tsOut.StartSave(id, fileName, overWriteFlag, pittariFlag, pittariONID, pittariTSID, pittariSID, pittariEventID, createSize, saveFolder, saveFolderSub, writeBuffMaxCount);

	StartBackgroundEpgCap();

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
	return this->tsOut.EndSave(id);
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
	return this->tsOut.SetScramble(id, enable);
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
	this->tsOut.SetServiceMode(id, enableCaption, enableData);
}

//エラーカウントをクリアする
void CBonCtrl::ClearErrCount(
	DWORD id
	)
{
	this->tsOut.ClearErrCount(id);
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
	this->tsOut.GetErrCount(id, drop, scramble);
}

//チャンネルスキャンを開始する
//戻り値：
// TRUE（成功）、FALSE（失敗）
BOOL CBonCtrl::StartChScan()
{
	if( this->tsOut.IsRec() == TRUE ){
		return FALSE;
	}
	if( this->chScanThread && WaitForSingleObject(this->chScanThread, 0) == WAIT_TIMEOUT ){
		return FALSE;
	}

	StopBackgroundEpgCap();
	StopChScan();

	if( this->bonUtil.GetOpenBonDriverFileName().empty() == false ){
		this->chScanIndexOrStatus = ST_COMPLETE;
		this->chScanChkList.clear();
		vector<pair<wstring, vector<wstring>>> spaceList = this->bonUtil.GetOriginalChList();
		for( size_t i = 0; i < spaceList.size(); i++ ){
			for( size_t j = 0; j < spaceList[i].second.size(); j++ ){
				if( spaceList[i].second[j].empty() == false ){
					CHK_CH_INFO item;
					item.space = (DWORD)i;
					item.spaceName = spaceList[i].first;
					item.ch = (DWORD)j;
					item.chName = spaceList[i].second[j];
					this->chScanChkList.push_back(item);
					this->chScanIndexOrStatus = 0;
				}
			}
		}
		//受信スレッド起動
		ResetEvent(this->chScanStopEvent);
		this->chScanThread = (HANDLE)_beginthreadex(NULL, 0, ChScanThread, this, 0, NULL);
		if( this->chScanThread ){
			return TRUE;
		}
		this->chScanIndexOrStatus = ST_STOP;
	}

	return FALSE;
}

//チャンネルスキャンをキャンセルする
void CBonCtrl::StopChScan()
{
	if( this->chScanThread != NULL ){
		::SetEvent(this->chScanStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->chScanThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->chScanThread, 0xffffffff);
		}
		CloseHandle(this->chScanThread);
		this->chScanThread = NULL;
	}
}

//チャンネルスキャンの状態を取得する
//戻り値：
// ステータス
//引数：
// space		[OUT]スキャン中の物理CHのspace
// ch			[OUT]スキャン中の物理CHのch
// chName		[OUT]スキャン中の物理CHの名前
// chkNum		[OUT]チェック済みの数
// totalNum		[OUT]チェック対象の総数
CBonCtrl::JOB_STATUS CBonCtrl::GetChScanStatus(
	DWORD* space,
	DWORD* ch,
	wstring* chName,
	DWORD* chkNum,
	DWORD* totalNum
	)
{
#if defined(_MSC_VER) && _MSC_VER < 1900
	LONG indexOrStatus = InterlockedExchangeAdd(&this->chScanIndexOrStatus, 0);
#else
	int indexOrStatus = this->chScanIndexOrStatus;
#endif
	if( indexOrStatus < 0 ){
		return (JOB_STATUS)indexOrStatus;
	}
	if( space != NULL ){
		*space = this->chScanChkList[indexOrStatus].space;
	}
	if( ch != NULL ){
		*ch = this->chScanChkList[indexOrStatus].ch;
	}
	if( chName != NULL ){
		*chName = this->chScanChkList[indexOrStatus].chName;
	}
	if( chkNum != NULL ){
		*chkNum = indexOrStatus;
	}
	if( totalNum != NULL ){
		*totalNum = (DWORD)this->chScanChkList.size();
	}
	return ST_WORKING;
}

UINT WINAPI CBonCtrl::ChScanThread(LPVOID param)
{
	CBonCtrl* sys = (CBonCtrl*)param;

	//TODO: chUtilをconstに保っていないのでスレッド安全性は破綻している。スキャン時だけの問題なので修正はしないが要注意
	sys->chUtil.Clear();

	fs_path settingPath = GetSettingPath();
	fs_path bonFile = sys->bonUtil.GetOpenBonDriverFileName();
	wstring tunerName = sys->bonUtil.GetTunerName();
	CheckFileName(tunerName);

	wstring chSet4 = fs_path(settingPath).append(bonFile.stem().concat(L"(" + tunerName + L").ChSet4.txt").native()).native();
	wstring chSet5 = fs_path(settingPath).append(L"ChSet5.txt").native();

	if( sys->chScanIndexOrStatus < 0 ){
		sys->chUtil.SaveChSet(chSet4, chSet5);
		return 0;
	}

	fs_path iniPath = GetModulePath().replace_filename(L"BonCtrl.ini");

	DWORD chChgTimeOut = GetPrivateProfileInt(L"CHSCAN", L"ChChgTimeOut", 9, iniPath.c_str());
	DWORD serviceChkTimeOut = GetPrivateProfileInt(L"CHSCAN", L"ServiceChkTimeOut", 8, iniPath.c_str());


	BOOL chkNext = TRUE;
	DWORD startTime = 0;

	while(1){
		if( ::WaitForSingleObject(sys->chScanStopEvent, chkNext ? 0 : 1000) != WAIT_TIMEOUT ){
			//キャンセルされた
			sys->chScanIndexOrStatus = ST_CANCEL;
			break;
		}
		if( chkNext == TRUE ){
			sys->ProcessSetCh(sys->chScanChkList[sys->chScanIndexOrStatus].space,
			                  sys->chScanChkList[sys->chScanIndexOrStatus].ch, TRUE);
			startTime = GetTickCount();
			chkNext = FALSE;
		}else{
			DWORD elapsed;
			if( sys->tsOut.IsChUnknown(&elapsed) ){
				if( elapsed > chChgTimeOut * 1000 ){
					//チャンネル切り替えにchChgTimeOut秒以上かかってるので無信号と判断
					OutputDebugString(L"★AutoScan Ch Change timeout\r\n");
					chkNext = TRUE;
				}
			}else{
				if( GetTickCount() - startTime > (chChgTimeOut + serviceChkTimeOut) * 1000 ){
					//チャンネル切り替え成功したけどサービス一覧とれないので無信号と判断
					OutputDebugString(L"★AutoScan GetService timeout\r\n");
					chkNext = TRUE;
				}else{
					//サービス一覧の取得を行う
					sys->tsOut.GetServiceListActual([sys, &chkNext](DWORD serviceListSize, SERVICE_INFO* serviceList) {
						if( serviceListSize > 0 ){
							//一覧の取得ができた
							for( int currSID = 0; currSID < 0x10000; ){
								//ServiceID順に追加
								int nextSID = 0x10000;
								for( DWORD i = 0; i < serviceListSize; i++ ){
									WORD serviceID = serviceList[i].service_id;
									if( serviceID == currSID && serviceList[i].extInfo && serviceList[i].extInfo->service_name ){
										if( wcslen(serviceList[i].extInfo->service_name) > 0 ){
											sys->chUtil.AddServiceInfo(sys->chScanChkList[sys->chScanIndexOrStatus].space,
											                           sys->chScanChkList[sys->chScanIndexOrStatus].ch,
											                           sys->chScanChkList[sys->chScanIndexOrStatus].chName, &(serviceList[i]));
										}
									}
									if( serviceID > currSID && serviceID < nextSID ){
										nextSID = serviceID;
									}
								}
								currSID = nextSID;
							}
							chkNext = TRUE;
						}
					});
				}
			}
			if( chkNext == TRUE ){
				//次のチャンネルへ
				if( sys->chScanChkList.size() <= (size_t)sys->chScanIndexOrStatus + 1 ){
					//全部チェック終わったので終了
					sys->chScanIndexOrStatus = ST_COMPLETE;
					sys->chUtil.SaveChSet(chSet4, chSet5);
					break;
				}
				sys->chScanIndexOrStatus++;
			}
		}
	}

	sys->chUtil.LoadChSet(chSet4, chSet5);

	return 0;
}

//EPG取得を開始する
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// chList		[IN]EPG取得するチャンネル一覧(NULL可)
BOOL CBonCtrl::StartEpgCap(
	vector<EPGCAP_SERVICE_INFO>* chList
	)
{
	if( this->tsOut.IsRec() == TRUE ){
		return FALSE;
	}
	if( this->epgCapThread && WaitForSingleObject(this->epgCapThread, 0) == WAIT_TIMEOUT ){
		return FALSE;
	}

	StopBackgroundEpgCap();
	StopEpgCap();

	if( this->bonUtil.GetOpenBonDriverFileName().empty() == false ){
		this->epgCapIndexOrStatus = ST_COMPLETE;
		if( chList ){
			this->epgCapChList = *chList;
		}else{
			this->epgCapChList.clear();
			this->chUtil.GetEpgCapService(&this->epgCapChList);
		}
		if( this->epgCapChList.empty() ){
			//取得するものがない
			return TRUE;
		}
		this->epgCapIndexOrStatus = 0;
		//受信スレッド起動
		ResetEvent(this->epgCapStopEvent);
		this->epgCapThread = (HANDLE)_beginthreadex(NULL, 0, EpgCapThread, this, 0, NULL);
		if( this->epgCapThread ){
			return TRUE;
		}
		this->epgCapIndexOrStatus = ST_STOP;
	}

	return FALSE;
}

//EPG取得を停止する
void CBonCtrl::StopEpgCap(
	)
{
	if( this->epgCapThread != NULL ){
		::SetEvent(this->epgCapStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->epgCapThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->epgCapThread, 0xffffffff);
		}
		CloseHandle(this->epgCapThread);
		this->epgCapThread = NULL;
	}
}

//EPG取得のステータスを取得する
//※info==NULLの場合に限りスレッドセーフ
//戻り値：
// ステータス
//引数：
// info			[OUT]取得中のサービス
CBonCtrl::JOB_STATUS CBonCtrl::GetEpgCapStatus(
	EPGCAP_SERVICE_INFO* info
	)
{
#if defined(_MSC_VER) && _MSC_VER < 1900
	LONG indexOrStatus = InterlockedExchangeAdd(&this->epgCapIndexOrStatus, 0);
#else
	int indexOrStatus = this->epgCapIndexOrStatus;
#endif
	if( indexOrStatus < 0 ){
		return (JOB_STATUS)indexOrStatus;
	}
	if( info != NULL ){
		*info = this->epgCapChList[indexOrStatus];
	}
	return ST_WORKING;
}

UINT WINAPI CBonCtrl::EpgCapThread(LPVOID param)
{
	CBonCtrl* sys = (CBonCtrl*)param;

	BOOL chkNext = TRUE;
	BOOL startCap = FALSE;
	DWORD wait = 0;
	DWORD startTime = 0;

	BOOL chkONIDs[16] = {};

	fs_path iniPath = GetModulePath().replace_filename(L"BonCtrl.ini");

	DWORD timeOut = GetPrivateProfileInt(L"EPGCAP", L"EpgCapTimeOut", 10, iniPath.c_str());
	BOOL saveTimeOut = GetPrivateProfileInt(L"EPGCAP", L"EpgCapSaveTimeOut", 0, iniPath.c_str());

	//Common.iniは一般に外部プロセスが変更する可能性のある(はずの)ものなので、利用の直前にチェックする
	fs_path commonIniPath = GetCommonIniPath();
	BOOL basicOnlyONIDs[16] = {};
	basicOnlyONIDs[4] = GetPrivateProfileInt(L"SET", L"BSBasicOnly", 1, commonIniPath.c_str());
	basicOnlyONIDs[6] = GetPrivateProfileInt(L"SET", L"CS1BasicOnly", 1, commonIniPath.c_str());
	basicOnlyONIDs[7] = GetPrivateProfileInt(L"SET", L"CS2BasicOnly", 1, commonIniPath.c_str());
	basicOnlyONIDs[10] = GetPrivateProfileInt(L"SET", L"CS3BasicOnly", 0, commonIniPath.c_str());

	while(1){
		if( ::WaitForSingleObject(sys->epgCapStopEvent, wait) != WAIT_TIMEOUT ){
			//キャンセルされた
			sys->epgCapIndexOrStatus = ST_CANCEL;
			sys->tsOut.StopSaveEPG(FALSE);
			break;
		}
		DWORD chkCount = sys->epgCapIndexOrStatus;
		if( chkNext == TRUE ){
			DWORD space = 0;
			DWORD ch = 0;
			sys->chUtil.GetCh(sys->epgCapChList[chkCount].ONID, sys->epgCapChList[chkCount].TSID,
			                  sys->epgCapChList[chkCount].SID, space, ch);
			sys->ProcessSetCh(space, ch);
			startTime = GetTickCount();
			chkNext = FALSE;
			startCap = FALSE;
			wait = 1000;
			chkONIDs[min<size_t>(sys->epgCapChList[chkCount].ONID, _countof(chkONIDs) - 1)] = TRUE;
		}else{
			DWORD tick = GetTickCount();
			DWORD elapsed;
			if( sys->tsOut.IsChUnknown(&elapsed) ){
				startTime += min<DWORD>(tick - startTime, 1000);
				if( elapsed > 15000 ){
					//チャンネル切り替えがタイムアウトしたので無信号と判断
					chkNext = TRUE;
				}
			}else{
				if( tick - startTime > timeOut * 60 * 1000 ){
					//timeOut分以上かかっているなら停止
					sys->tsOut.StopSaveEPG(saveTimeOut);
					chkNext = TRUE;
					wait = 0;
					_OutputDebugString(L"++%d分でEPG取得完了せず or Ch変更でエラー", timeOut);
				}else if( tick - startTime > 5000 ){
					//切り替え完了から5秒以上過ぎているので取得処理
					if( startCap == FALSE ){
						//取得開始
						startCap = TRUE;
						wstring epgDataPath = L"";
						GetEpgDataFilePath(sys->epgCapChList[chkCount].ONID,
						                   basicOnlyONIDs[min<size_t>(sys->epgCapChList[chkCount].ONID, _countof(basicOnlyONIDs) - 1)] ? 0xFFFF : sys->epgCapChList[chkCount].TSID,
						                   epgDataPath);
						sys->tsOut.StartSaveEPG(epgDataPath);
						wait = 60*1000;
					}else{
						vector<EPGCAP_SERVICE_INFO> chkList;
						if( basicOnlyONIDs[min<size_t>(sys->epgCapChList[chkCount].ONID, _countof(basicOnlyONIDs) - 1)] ){
							chkList = sys->chUtil.GetEpgCapServiceAll(sys->epgCapChList[chkCount].ONID);
						}else{
							chkList = sys->chUtil.GetEpgCapServiceAll(sys->epgCapChList[chkCount].ONID, sys->epgCapChList[chkCount].TSID);
						}
						//epgCapChListのサービスはEPG取得対象でなかったとしてもチェックしなければならない
						chkList.push_back(sys->epgCapChList[chkCount]);
						for( vector<EPGCAP_SERVICE_INFO>::iterator itr = chkList.begin(); itr != chkList.end(); itr++ ){
							if( itr->ONID == chkList.back().ONID && itr->TSID == chkList.back().TSID && itr->SID == chkList.back().SID ){
								chkList.pop_back();
								break;
							}
						}
						//蓄積状態チェック
						for( vector<EPGCAP_SERVICE_INFO>::iterator itr = chkList.begin(); itr != chkList.end(); itr++ ){
							BOOL leitFlag = sys->chUtil.IsPartial(itr->ONID, itr->TSID, itr->SID);
							pair<EPG_SECTION_STATUS, BOOL> status = sys->tsOut.GetSectionStatusService(itr->ONID, itr->TSID, itr->SID, leitFlag);
							if( status.second == FALSE ){
								status.first = sys->tsOut.GetSectionStatus(leitFlag);
							}
							if( status.first != EpgNoData ){
								chkNext = TRUE;
								if( status.first != EpgHEITAll &&
								    status.first != EpgLEITAll &&
								    (status.first != EpgBasicAll || basicOnlyONIDs[min<size_t>(itr->ONID, _countof(basicOnlyONIDs) - 1)] == FALSE) ){
									chkNext = FALSE;
									break;
								}
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
					sys->epgCapIndexOrStatus = ST_COMPLETE;
					break;
				}
				//1チャンネルのみ？
				if( basicOnlyONIDs[min<size_t>(sys->epgCapChList[chkCount].ONID, _countof(basicOnlyONIDs) - 1)] &&
				    chkONIDs[sys->epgCapChList[chkCount].ONID] ){
					chkCount++;
					while( chkCount < sys->epgCapChList.size() ){
						if( sys->epgCapChList[chkCount].ONID != sys->epgCapChList[chkCount - 1].ONID ){
							break;
						}
						chkCount++;
					}
					if( sys->epgCapChList.size() <= chkCount ){
						//全部チェック終わったので終了
						sys->epgCapIndexOrStatus = ST_COMPLETE;
						break;
					}
				}
				sys->epgCapIndexOrStatus = chkCount;
			}
		}
	}
	return 0;
}

void CBonCtrl::GetEpgDataFilePath(WORD ONID, WORD TSID, wstring& epgDataFilePath)
{
	Format(epgDataFilePath, L"%04X%04X_epg.dat", ONID, TSID);
	epgDataFilePath = GetSettingPath().append(EPG_SAVE_FOLDER).append(epgDataFilePath).native();
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
	this->tsOut.SaveErrCount(id, filePath);
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
	this->tsOut.GetRecWriteSize(id, writeSize);
}

//バックグラウンドでのEPG取得設定
//引数：
// enableLive	[IN]視聴中に取得する
// enableRec	[IN]録画中に取得する
// enableRec	[IN]EPG取得するチャンネル一覧
// *Basic		[IN]１チャンネルから基本情報のみ取得するかどうか
// backStartWaitSec	[IN]Ch切り替え、録画開始後、バックグラウンドでのEPG取得を開始するまでの秒数
void CBonCtrl::SetBackGroundEpgCap(
	BOOL enableLive,
	BOOL enableRec,
	BOOL BSBasic,
	BOOL CS1Basic,
	BOOL CS2Basic,
	BOOL CS3Basic,
	DWORD backStartWaitSec
	)
{
	this->enableLiveEpgCap = enableLive;
	this->enableRecEpgCap = enableRec;
	this->epgCapBackBSBasic = BSBasic;
	this->epgCapBackCS1Basic = CS1Basic;
	this->epgCapBackCS2Basic = CS2Basic;
	this->epgCapBackCS3Basic = CS3Basic;
	this->epgCapBackStartWaitSec = backStartWaitSec;

	StartBackgroundEpgCap();
}

void CBonCtrl::StartBackgroundEpgCap()
{
	StopBackgroundEpgCap();
	if( (this->chScanThread == NULL || WaitForSingleObject(this->chScanThread, 0) != WAIT_TIMEOUT) &&
	    (this->epgCapThread == NULL || WaitForSingleObject(this->epgCapThread, 0) != WAIT_TIMEOUT) ){
		if( this->bonUtil.GetOpenBonDriverFileName().empty() == false ){
			//受信スレッド起動
			ResetEvent(this->epgCapBackStopEvent);
			this->epgCapBackThread = (HANDLE)_beginthreadex(NULL, 0, EpgCapBackThread, this, 0, NULL);
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
	fs_path iniPath = GetModulePath().replace_filename(L"BonCtrl.ini");

	DWORD timeOut = GetPrivateProfileInt(L"EPGCAP", L"EpgCapTimeOut", 10, iniPath.c_str());
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

	BOOL basicOnly = ONID == 4 && sys->epgCapBackBSBasic ||
	                 ONID == 6 && sys->epgCapBackCS1Basic ||
	                 ONID == 7 && sys->epgCapBackCS2Basic ||
	                 ONID == 10 && sys->epgCapBackCS3Basic;
	vector<EPGCAP_SERVICE_INFO> chkList = sys->chUtil.GetEpgCapServiceAll(ONID, TSID);
	if( chkList.empty() == false && basicOnly ){
		chkList = sys->chUtil.GetEpgCapServiceAll(ONID);
	}
	if( chkList.empty() ){
		return 0;
	}

	GetEpgDataFilePath(ONID, basicOnly ? 0xFFFF : TSID, epgDataPath);
	sys->tsOut.StartSaveEPG(epgDataPath);

	if( ::WaitForSingleObject(sys->epgCapBackStopEvent, 60*1000) != WAIT_TIMEOUT ){
		//キャンセルされた
		sys->tsOut.StopSaveEPG(FALSE);
		return 0;
	}
	while(1){
		//蓄積状態チェック
		BOOL chkNext = FALSE;
		for( vector<EPGCAP_SERVICE_INFO>::iterator itr = chkList.begin(); itr != chkList.end(); itr++ ){
			BOOL leitFlag = sys->chUtil.IsPartial(itr->ONID, itr->TSID, itr->SID);
			pair<EPG_SECTION_STATUS, BOOL> status = sys->tsOut.GetSectionStatusService(itr->ONID, itr->TSID, itr->SID, leitFlag);
			if( status.second == FALSE ){
				status.first = sys->tsOut.GetSectionStatus(leitFlag);
			}
			if( status.first != EpgNoData ){
				chkNext = TRUE;
				if( status.first != EpgHEITAll &&
				    status.first != EpgLEITAll &&
				    (status.first != EpgBasicAll || basicOnly == FALSE) ){
					chkNext = FALSE;
					break;
				}
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
				//timeOut分以上かかっているなら停止
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
	BOOL ret = FALSE;

	this->tsOut.GetErrCount(id, drop, scramble);

	*signal = this->bonUtil.GetSignalLevel();
	this->tsOut.SetSignalLevel(*signal);

	if( this->bonUtil.GetNowCh(space, ch) ){
		ret = TRUE;
	}

	return ret;
}
