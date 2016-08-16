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

void CBonCtrl::SetTsBuffMaxCount(DWORD tsBuffMaxCount, int writeBuffMaxCount)
{
	this->tsBuffMaxCount = tsBuffMaxCount;
	this->writeBuffMaxCount = writeBuffMaxCount;
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
		this->tsOut.ResetChChange();

		this->tsOut.SetBonDriver(bonFile);
		wstring settingPath;
		GetSettingPath(settingPath);
		wstring bonFileTitle;
		GetFileTitle(bonFile, bonFileTitle);
		wstring tunerName = this->bonUtil.GetTunerName();
		CheckFileName(tunerName);
		this->chUtil.LoadChSet( settingPath + L"\\" + bonFileTitle + L"(" + tunerName + L").ChSet4.txt", settingPath + L"\\ChSet5.txt" );
	}

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

	return _SetCh(space, ch);
}

//チャンネル変更
//戻り値：
// エラーコード
//引数：
// ONID			[IN]変更チャンネルのorignal_network_id
// TSID			[IN]変更チャンネルの物理transport_stream_id
DWORD CBonCtrl::SetCh(
	WORD ONID,
	WORD TSID
)
{
	if( this->tsOut.IsRec() == TRUE ){
		return ERR_FALSE;
	}

	DWORD space=0;
	DWORD ch=0;

	DWORD ret = ERR_FALSE;
	if( this->chUtil.GetCh( ONID, TSID, space, ch ) == TRUE ){
		ret = _SetCh(space, ch);
	}

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
		if( this->bonUtil.GetNowCh(&spaceNow, &chNow) == false || space != spaceNow || ch != chNow ){
			this->tsOut.SetChChangeEvent(chScan);
			_OutputDebugString(L"SetCh space %d, ch %d", space, ch);
			ret = this->bonUtil.SetCh(space, ch) ? NO_ERR : ERR_FALSE;

			StartBackgroundEpgCap();
		}else{
			BOOL chChgErr = FALSE;
			if( this->tsOut.IsChChanging(&chChgErr) == TRUE ){
				if( chChgErr == TRUE ){
					//エラーの時は再設定
					this->tsOut.SetChChangeEvent();
					_OutputDebugString(L"SetCh space %d, ch %d", space, ch);
					ret = this->bonUtil.SetCh(space, ch) ? NO_ERR : ERR_FALSE;

					StartBackgroundEpgCap();
				}
			}else{
				if( chChgErr == TRUE ){
					//エラーの時は再設定
					this->tsOut.SetChChangeEvent();
					_OutputDebugString(L"SetCh space %d, ch %d", space, ch);
					ret = this->bonUtil.SetCh(space, ch) ? NO_ERR : ERR_FALSE;

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
	return this->tsOut.IsChChanging(chChgErr);
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
	return this->tsOut.GetEpgInfo(originalNetworkID, transportStreamID, serviceID, nextFlag, epgInfo);
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
	return this->tsOut.SearchEpgInfo(originalNetworkID, transportStreamID, serviceID, eventID, pfOnlyFlag, epgInfo);
}

//PC時計を元としたストリーム時間との差を取得する
//戻り値：
// 差の秒数
int CBonCtrl::GetTimeDelay(
	)
{
	return this->tsOut.GetTimeDelay();
}

//録画中かどうかを取得する
// TRUE（録画中）、FALSE（録画していない）
BOOL CBonCtrl::IsRec()
{
	return this->tsOut.IsRec();
}
//チャンネルスキャンを開始する
//戻り値：
// エラーコード
DWORD CBonCtrl::StartChScan()
{
	if( this->tsOut.IsRec() == TRUE ){
		return ERR_FALSE;
	}
	if( this->epgSt_err == ST_WORKING ){
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
			this->chScanThread = (HANDLE)_beginthreadex(NULL, 0, ChScanThread, this, 0, NULL);
		}
	}

	return ret;
}

//チャンネルスキャンをキャンセルする
//戻り値：
// エラーコード
DWORD CBonCtrl::StopChScan()
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

	return NO_ERR;
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
	return this->chSt_err;
}

UINT WINAPI CBonCtrl::ChScanThread(LPVOID param)
{
	CBonCtrl* sys = (CBonCtrl*)param;

	//TODO: chUtilをconstに保っていないのでスレッド安全性は破綻している。スキャン時だけの問題なので修正はしないが要注意
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
	vector<pair<wstring, vector<wstring>>> spaceList = sys->bonUtil.GetOriginalChList();
	for( size_t i = 0; i < spaceList.size(); i++ ){
		for( size_t j = 0; j < spaceList[i].second.size(); j++ ){
			if( spaceList[i].second[j].empty() == false ){
				CHK_CH_INFO item;
				item.space = (DWORD)i;
				item.spaceName = spaceList[i].first;
				item.ch = (DWORD)j;
				item.chName = spaceList[i].second[j];
				chkList.push_back(item);
			}
		}
	}
	sys->chSt_totalNum = (DWORD)chkList.size();

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
	this->chUtil.GetEpgCapService(chList);
	return NO_ERR;
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
	if( this->tsOut.IsRec() == TRUE ){
		return ERR_FALSE;
	}
	if( this->chSt_err == ST_WORKING ){
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
			this->epgCapThread = (HANDLE)_beginthreadex(NULL, 0, EpgCapThread, this, 0, NULL);
		}
	}

	return ret;
}

//EPG取得を停止する
//戻り値：
// エラーコード
DWORD CBonCtrl::StopEpgCap(
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

	return NO_ERR;
}

//EPG取得のステータスを取得する
//戻り値：
// ステータス
//引数：
// info			[OUT]取得中のサービス
CBonCtrl::JOB_STATUS CBonCtrl::GetEpgCapStatus(
	EPGCAP_SERVICE_INFO* info
	)
{
	if( info != NULL ){
		*info = this->epgSt_ch;
	}
	return this->epgSt_err;
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

	BOOL chkONIDs[16] = {};

	wstring folderPath;
	GetModuleFolderPath( folderPath );
	wstring iniPath = folderPath;
	iniPath += L"\\BonCtrl.ini";

	DWORD timeOut = GetPrivateProfileInt(L"EPGCAP", L"EpgCapTimeOut", 15, iniPath.c_str());
	BOOL saveTimeOut = GetPrivateProfileInt(L"EPGCAP", L"EpgCapSaveTimeOut", 0, iniPath.c_str());

	//Common.iniは一般に外部プロセスが変更する可能性のある(はずの)ものなので、利用の直前にチェックする
	wstring commonIniPath;
	GetCommonIniPath(commonIniPath);
	BOOL basicOnlyONIDs[16] = {};
	basicOnlyONIDs[4] = GetPrivateProfileInt(L"SET", L"BSBasicOnly", 1, commonIniPath.c_str());
	basicOnlyONIDs[6] = GetPrivateProfileInt(L"SET", L"CS1BasicOnly", 1, commonIniPath.c_str());
	basicOnlyONIDs[7] = GetPrivateProfileInt(L"SET", L"CS2BasicOnly", 1, commonIniPath.c_str());
	basicOnlyONIDs[10] = GetPrivateProfileInt(L"SET", L"CS3BasicOnly", 0, commonIniPath.c_str());

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
			chkONIDs[min(sys->epgCapChList[chkCount].ONID, _countof(chkONIDs) - 1)] = TRUE;
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
						GetEpgDataFilePath(sys->epgCapChList[chkCount].ONID,
						                   basicOnlyONIDs[min(sys->epgCapChList[chkCount].ONID, _countof(basicOnlyONIDs) - 1)] ? 0xFFFF : sys->epgCapChList[chkCount].TSID,
						                   epgDataPath);
						sys->tsOut.StartSaveEPG(epgDataPath);
						wait = 60*1000;
					}else{
						vector<EPGCAP_SERVICE_INFO> chkList;
						if( basicOnlyONIDs[min(sys->epgCapChList[chkCount].ONID, _countof(basicOnlyONIDs) - 1)] ){
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
								    (status.first != EpgBasicAll || basicOnlyONIDs[min(itr->ONID, _countof(basicOnlyONIDs) - 1)] == FALSE) ){
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
					sys->epgSt_err = ST_COMPLETE;
					return 0;
				}
				//1チャンネルのみ？
				if( basicOnlyONIDs[min(sys->epgCapChList[chkCount].ONID, _countof(basicOnlyONIDs) - 1)] &&
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
						sys->epgSt_err = ST_COMPLETE;
						return 0;
					}
				}
			}
		}
	}
	return 0;
}

void CBonCtrl::GetEpgDataFilePath(WORD ONID, WORD TSID, wstring& epgDataFilePath)
{
	wstring epgDataFolderPath = L"";
	GetSettingPath(epgDataFolderPath);
	epgDataFolderPath += EPG_SAVE_FOLDER;

	Format(epgDataFilePath, L"%s\\%04X%04X_epg.dat", epgDataFolderPath.c_str(), ONID, TSID);
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
	this->tsOut.GetSaveFilePath(id, filePath, subRecFlag);
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
	if( this->epgCapBackThread == NULL && this->epgCapThread == NULL && this->chScanThread == NULL ){
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
	BOOL ret = FALSE;

	this->tsOut.GetErrCount(id, drop, scramble);

	*signal = this->bonUtil.GetSignalLevel();
	this->tsOut.SetSignalLevel(*signal);

	if( this->bonUtil.GetNowCh(space, ch) ){
		ret = TRUE;
	}

	return ret;
}
