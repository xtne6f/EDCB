#include "StdAfx.h"
#include "EpgDataCap_BonMain.h"

#include "EpgDataCap_BonDef.h"
#include "../../Common/CtrlCmdDef.h"
#include "../../Common/CtrlCmdUtil.h"

CEpgDataCap_BonMain::CEpgDataCap_BonMain(void)
{
	this->msgWnd = NULL;
	this->nwCtrlID = 0;
	this->sendUdpFlag = FALSE;
	this->sendTcpFlag = FALSE;

	this->startMargine = 5;
	this->endMargine = 5;
	this->overWriteFlag = FALSE;
	this->enableScrambleFlag = TRUE;
	this->enableEMMFlag = FALSE;

	this->allService = FALSE;
	this->needCaption = TRUE;
	this->needData = FALSE;

	this->viewPath = L"";
	this->viewOpt = L"";

	this->lastONID = 0xFFFF;
	this->lastTSID = 0xFFFF;
	this->lastSID = 0xFFFF;

	this->recCtrlID = 0;

	this->currentBonDriver = L"";
	this->outCtrlID = -1;

	this->cmdCapture = NULL;
	this->resCapture = NULL;

	this->openWait = 200;
}


CEpgDataCap_BonMain::~CEpgDataCap_BonMain(void)
{
	this->pipeServer.StopServer();
}

void CEpgDataCap_BonMain::SetHwnd(HWND wnd)
{
	this->msgWnd = wnd;
}

//設定を行う
void CEpgDataCap_BonMain::ReloadSetting()
{
	wstring commonIniPath = L"";
	GetCommonIniPath(commonIniPath);

	wstring appIniPath = L"";
	GetModuleIniPath(appIniPath);

	wstring settingPath = L"";
	GetSettingPath(settingPath);

	wstring bonDriverPath = L"";
	GetModuleFolderPath(bonDriverPath);
	bonDriverPath += BON_DLL_FOLDER;

	this->bonCtrl.SetBonDriverFolder( bonDriverPath.c_str() );

	this->recFolderList.clear();
	int iNum = GetPrivateProfileInt( L"SET", L"RecFolderNum", 0, commonIniPath.c_str() );
	if( iNum == 0 ){
		this->recFolderList.push_back( settingPath );
	}else{
		for( int i = 0; i < iNum; i++ ){
			WCHAR key[64];
			wsprintf(key, L"RecFolderPath%d", i );
			WCHAR wBuff[512]=L"";
			GetPrivateProfileString( L"SET", key, L"", wBuff, 512, commonIniPath.c_str() );

			this->recFolderList.push_back(wBuff);
		}
	}

	this->enableScrambleFlag = GetPrivateProfileInt( L"SET", L"Scramble", 1, appIniPath.c_str() );
	this->enableEMMFlag = GetPrivateProfileInt( L"SET", L"EMM", 0, appIniPath.c_str() );
	this->allService = GetPrivateProfileInt( L"SET", L"AllService", 0, appIniPath.c_str() );
	this->needCaption = GetPrivateProfileInt( L"SET", L"Caption", 1, appIniPath.c_str() );
	this->needData = GetPrivateProfileInt( L"SET", L"Data", 0, appIniPath.c_str() );

	this->startMargine = GetPrivateProfileInt( L"SET", L"StartMargine", 5, appIniPath.c_str() );
	this->endMargine = GetPrivateProfileInt( L"SET", L"EndMargine", 5, appIniPath.c_str() );

	this->overWriteFlag = GetPrivateProfileInt( L"SET", L"OverWrite", 0, appIniPath.c_str() );

	WCHAR buff[512]=L"";
	GetPrivateProfileString( L"SET", L"ViewPath", L"", buff, 512, appIniPath.c_str() );
	this->viewPath = buff;
	GetPrivateProfileString( L"SET", L"ViewOption", L"", buff, 512, appIniPath.c_str() );
	this->viewOpt = buff;

	this->udpCount = (DWORD)GetPrivateProfileInt( L"SET_UDP", L"Count", 0, appIniPath.c_str() );
	this->tcpCount = (DWORD)GetPrivateProfileInt( L"SET_TCP", L"Count", 0, appIniPath.c_str() );

	if( this->nwCtrlID != 0 ){
		if( this->allService == TRUE ){
			this->bonCtrl.SetServiceID(this->nwCtrlID, 0xFFFF);
		}else{
			this->bonCtrl.SetServiceID(this->nwCtrlID, this->lastSID);
		}
	}

	BOOL epgCapLive = (BOOL)GetPrivateProfileInt( L"SET", L"EpgCapLive", 1, appIniPath.c_str() );
	BOOL epgCapRec = (BOOL)GetPrivateProfileInt( L"SET", L"EpgCapRec", 1, appIniPath.c_str() );
	BOOL epgCapBackBSBasic = GetPrivateProfileInt( L"SET", L"EpgCapBackBSBasicOnly", 1, appIniPath.c_str() );
	BOOL epgCapBackCS1Basic = GetPrivateProfileInt( L"SET", L"EpgCapBackCS1BasicOnly", 1, appIniPath.c_str() );
	BOOL epgCapBackCS2Basic = GetPrivateProfileInt( L"SET", L"EpgCapBackCS2BasicOnly", 1, appIniPath.c_str() );
	DWORD epgCapBackStartWaitSec = (DWORD)GetPrivateProfileInt( L"SET", L"EpgCapBackStartWaitSec", 30, appIniPath.c_str() );

	this->bonCtrl.SetBackGroundEpgCap(epgCapLive, epgCapRec, epgCapBackBSBasic, epgCapBackCS1Basic, epgCapBackCS2Basic, epgCapBackStartWaitSec);
	if( this->sendTcpFlag == FALSE && this->sendUdpFlag == FALSE ){
		this->bonCtrl.SetScramble(this->nwCtrlID, this->enableScrambleFlag);
	}
	this->bonCtrl.SetEMMMode(this->enableEMMFlag);

	DWORD tsBuffMaxCount = (DWORD)GetPrivateProfileInt( L"SET", L"TsBuffMaxCount", 5000, appIniPath.c_str() );
	int writeBuffMaxCount = GetPrivateProfileInt( L"SET", L"WriteBuffMaxCount", -1, appIniPath.c_str() );
	this->bonCtrl.SetTsBuffMaxCount(tsBuffMaxCount, writeBuffMaxCount);

	this->openWait = (DWORD)GetPrivateProfileInt( L"SET", L"OpenWait", 200, appIniPath.c_str() );
}

//BonDriverフォルダのBonDriver_*.dllを列挙
//戻り値：
// エラーコード
//引数：
// bonList			[OUT]検索できたBonDriver一覧
DWORD CEpgDataCap_BonMain::EnumBonDriver(
	vector<wstring>* bonList
)
{
	return this->bonCtrl.EnumBonDriver(bonList);
}

//BonDriverをロードしてチャンネル情報などを取得（ファイル名で指定）
//戻り値：
// エラーコード
//引数：
// bonDriverFile	[IN]EnumBonDriverで取得されたBonDriverのファイル名
DWORD CEpgDataCap_BonMain::OpenBonDriver(
	LPCWSTR bonDriverFile
)
{
	DWORD ret = this->bonCtrl.OpenBonDriver(bonDriverFile, this->openWait);
	if( ret == NO_ERR ){
		this->lastONID = 0xFFFF;
		this->lastTSID = 0xFFFF;
		this->lastSID = 0xFFFF;
		this->currentBonDriver = bonDriverFile;
		if( this->nwCtrlID == 0 ){
			if( this->bonCtrl.CreateServiceCtrl(&this->nwCtrlID) == TRUE ){
				this->bonCtrl.SetScramble(this->nwCtrlID, this->enableScrambleFlag);
				this->bonCtrl.SetServiceMode(this->nwCtrlID, this->needCaption, this->needData);
			}
		}else{
			this->bonCtrl.ClearErrCount(this->nwCtrlID);
		}
	}else{
		this->currentBonDriver = L"";
	}
	return ret;
}

//ロードしているBonDriverの開放
//戻り値：
// エラーコード
DWORD CEpgDataCap_BonMain::CloseBonDriver()
{
	this->currentBonDriver = L"";
	return this->bonCtrl.CloseBonDriver();
}

//サービス一覧を取得する
//戻り値：
// エラーコード
//引数：
// serviceList				[OUT]サービス情報のリスト
DWORD CEpgDataCap_BonMain::GetServiceList(
	vector<CH_DATA4>* serviceList
	)
{
	return this->bonCtrl.GetServiceList(serviceList);
}

//ロード中のBonDriverのファイル名を取得する（ロード成功しているかの判定）
//戻り値：
// TRUE（成功）：FALSE（Openに失敗している）
//引数：
// bonDriverFile		[OUT]BonDriverのファイル名(NULL可)
BOOL CEpgDataCap_BonMain::GetOpenBonDriver(
	wstring* bonDriverFile
	)
{
	return this->bonCtrl.GetOpenBonDriver(bonDriverFile);
}

//チャンネル変更
//戻り値：
// エラーコード
//引数：
// ONID			[IN]変更チャンネルのorignal_network_id
// TSID			[IN]変更チャンネルの物理transport_stream_id
// SID			[IN]変更チャンネルの物理service_id
DWORD CEpgDataCap_BonMain::SetCh(
	WORD ONID,
	WORD TSID,
	WORD SID
	)
{
	DWORD err = ERR_FALSE;
	if( this->bonCtrl.IsRec() == FALSE ){
		if( this->bonCtrl.GetEpgCapStatus(NULL) == ST_WORKING ){
			this->bonCtrl.StopEpgCap();
		}
		err = this->bonCtrl.SetCh(ONID, TSID, SID);
		if( err == NO_ERR ){
			this->lastONID = ONID;
			this->lastTSID = TSID;
			this->lastSID = SID;

			if( this->nwCtrlID != 0 ){
				if( this->allService == TRUE ){
					this->bonCtrl.SetServiceID(this->nwCtrlID, 0xFFFF);
				}else{
					this->bonCtrl.SetServiceID(this->nwCtrlID, this->lastSID);
				}
			}
		}
	}
	return err;
}

//チャンネル変更
//戻り値：
// エラーコード
//引数：
// SID			[IN]変更チャンネルのservice_id
// SID			[IN]変更チャンネルのspace
// SID			[IN]変更チャンネルのch
DWORD CEpgDataCap_BonMain::SetCh(
	WORD ONID,
	WORD TSID,
	WORD SID,
	DWORD space,
	DWORD ch
	)
{
	DWORD err = ERR_FALSE;
	if( this->bonCtrl.IsRec() == FALSE ){
		if( this->bonCtrl.GetEpgCapStatus(NULL) == ST_WORKING ){
			this->bonCtrl.StopEpgCap();
		}
		err = this->bonCtrl.SetCh(space, ch, SID);
		if( err == NO_ERR ){
			this->lastONID = ONID;
			this->lastTSID = TSID;
			this->lastSID = SID;

			if( this->nwCtrlID != 0 ){
				if( this->allService == TRUE ){
					this->bonCtrl.SetServiceID(this->nwCtrlID, 0xFFFF);
				}else{
					this->bonCtrl.SetServiceID(this->nwCtrlID, this->lastSID);
				}
			}
		}
	}
	return err;
}

//現在のサービス取得
//戻り値：
// エラーコード
//引数：
// ONID			[IN]現在のorignal_network_id
// TSID			[IN]現在のtransport_stream_id
// SID			[IN]現在のservice_id
void CEpgDataCap_BonMain::GetCh(
	WORD* ONID,
	WORD* TSID,
	WORD* SID
	)
{
	*ONID = this->lastONID;
	*TSID = this->lastTSID;
	*SID = this->lastSID;
}

//チャンネル変更中かどうか
//戻り値：
// TRUE（変更中）、FALSE（完了）
BOOL CEpgDataCap_BonMain::IsChChanging(BOOL* chChgErr)
{
	return this->bonCtrl.IsChChanging(chChgErr);
}

void CEpgDataCap_BonMain::SetSID(
	WORD SID
	)
{
	this->lastSID = SID;
}

//UDPで送信を行う
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// enableFlag		[IN]TRUE（開始）、FALSE（停止）
BOOL CEpgDataCap_BonMain::SendUDP(
	BOOL enableFlag
	)
{
	this->udpSendList.clear();
	if( enableFlag == TRUE ){
		wstring appIniPath = L"";
		GetModuleIniPath(appIniPath);

		int udpCount = GetPrivateProfileInt( L"SET_UDP", L"Count", 0, appIniPath.c_str() );
		for( int i = 0; i < udpCount; i++ ){
			NW_SEND_INFO item;

			WCHAR key[64];
			wsprintf(key, L"IP%d",i);
			item.ip = GetPrivateProfileInt( L"SET_UDP", key, 2130706433, appIniPath.c_str() );
			wsprintf(key, L"Port%d",i);
			item.port = GetPrivateProfileInt( L"SET_UDP", key, 1234, appIniPath.c_str() );
			Format(item.ipString, L"%d.%d.%d.%d",
				(item.ip&0xFF000000)>>24,
				(item.ip&0x00FF0000)>>16,
				(item.ip&0x0000FF00)>>8,
				(item.ip&0x000000FF) );
			wsprintf(key, L"BroadCast%d",i);
			item.broadcastFlag = GetPrivateProfileInt( L"SET_UDP", key, 0, appIniPath.c_str() );

			udpSendList.push_back(item);
		}
	}

	BOOL ret = TRUE;
	if( udpSendList.size() == 0 ){
		this->sendUdpFlag = FALSE;
		if( this->nwCtrlID != 0 ){
			this->bonCtrl.SendUdp(this->nwCtrlID,NULL);
/*			if( this->sendUdpFlag == FALSE && this->sendTcpFlag == FALSE ){
				this->bonCtrl.DeleteServiceCtrl(this->nwCtrlID);
				this->nwCtrlID = 0;
			}*/
		}
	}else{
		this->sendUdpFlag = TRUE;
		if( this->nwCtrlID == 0 ){
			if( this->bonCtrl.CreateServiceCtrl(&this->nwCtrlID) == FALSE ){
				this->sendTcpFlag = FALSE;
				return FALSE;
			}
		}
		if( this->nwCtrlID != 0 ){
			if( this->allService == TRUE ){
				this->bonCtrl.SetServiceID(this->nwCtrlID, 0xFFFF);
			}else{
				this->bonCtrl.SetServiceID(this->nwCtrlID, this->lastSID);
			}
		}
		this->bonCtrl.SetScramble(this->nwCtrlID, this->enableScrambleFlag);
		this->bonCtrl.SetServiceMode(this->nwCtrlID, this->needCaption, this->needData);
		ret = this->bonCtrl.SendUdp(this->nwCtrlID,&this->udpSendList);
	}
	return ret;
}

//TCPで送信を行う
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// enableFlag		[IN]TRUE（開始）、FALSE（停止）
BOOL CEpgDataCap_BonMain::SendTCP(
	BOOL enableFlag
	)
{
	this->tcpSendList.clear();
	if( enableFlag == TRUE ){
		wstring appIniPath = L"";
		GetModuleIniPath(appIniPath);

		int tcpCount = GetPrivateProfileInt( L"SET_TCP", L"Count", 0, appIniPath.c_str() );
		for( int i = 0; i < tcpCount; i++ ){
			NW_SEND_INFO item;

			WCHAR key[64];
			wsprintf(key, L"IP%d",i);
			item.ip = GetPrivateProfileInt( L"SET_TCP", key, 2130706433, appIniPath.c_str() );
			wsprintf(key, L"Port%d",i);
			item.port = GetPrivateProfileInt( L"SET_TCP", key, 2230, appIniPath.c_str() );
			Format(item.ipString, L"%d.%d.%d.%d",
				(item.ip&0xFF000000)>>24,
				(item.ip&0x00FF0000)>>16,
				(item.ip&0x0000FF00)>>8,
				(item.ip&0x000000FF) );
			item.broadcastFlag = 0;

			tcpSendList.push_back(item);
		}
	}

	BOOL ret = TRUE;
	if( tcpSendList.size() == 0 ){
		this->sendTcpFlag = FALSE;
		if( this->nwCtrlID != 0 ){
			this->bonCtrl.SendTcp(this->nwCtrlID,NULL);
/*			if( this->sendUdpFlag == FALSE && this->sendTcpFlag == FALSE ){
				this->bonCtrl.DeleteServiceCtrl(this->nwCtrlID);
				this->nwCtrlID = 0;
			}*/
		}
	}else{
		this->sendTcpFlag = TRUE;
		if( this->nwCtrlID == 0 ){
			if( this->bonCtrl.CreateServiceCtrl(&this->nwCtrlID) == FALSE ){
				this->sendTcpFlag = FALSE;
				return FALSE;
			}
		}
		if( this->nwCtrlID != 0 ){
			if( this->allService == TRUE ){
				this->bonCtrl.SetServiceID(this->nwCtrlID, 0xFFFF);
			}else{
				this->bonCtrl.SetServiceID(this->nwCtrlID, this->lastSID);
			}
		}
		this->bonCtrl.SetScramble(this->nwCtrlID, this->enableScrambleFlag);
		this->bonCtrl.SetServiceMode(this->nwCtrlID, this->needCaption, this->needData);
		ret = this->bonCtrl.SendTcp(this->nwCtrlID,&this->tcpSendList);
	}
	return ret;
}

//UDP送信の設定数を取得
DWORD CEpgDataCap_BonMain::GetCountUDP()
{
	return this->udpCount;
}

//TCP送信の設定数を取得
DWORD CEpgDataCap_BonMain::GetCountTCP()
{
	return this->tcpCount;
}

//UDPの送信中先一覧取得
BOOL CEpgDataCap_BonMain::GetSendUDPList(
	vector<NW_SEND_INFO>* sendList
	)
{
	if( this->udpSendList.size() == 0 ){
		return FALSE;
	}
	*sendList = this->udpSendList;

	return TRUE;
}

//TCPの送信中先一覧取得
BOOL CEpgDataCap_BonMain::GetSendTCPList(
	vector<NW_SEND_INFO>* sendList
	)
{
	if( this->tcpSendList.size() == 0 ){
		return FALSE;
	}
	*sendList = this->tcpSendList;

	return TRUE;
}

//指定サービスの現在or次のEPG情報を取得する
//戻り値：
// エラーコード
//引数：
// serviceID				[IN]取得対象のServiceID
// nextFlag					[IN]TRUE（次の番組）、FALSE（現在の番組）
// epgInfo					[OUT]EPG情報（DLL内で自動的にdeleteする。次に取得を行うまで有効）
DWORD CEpgDataCap_BonMain::GetEpgInfo(
	BOOL nextFlag,
	wstring* epgInfo
	)
{
	WORD ONID = 0;
	WORD TSID = 0;
	if( this->bonCtrl.IsChChanging(NULL) == TRUE ){
		return FALSE;
	}
	if( this->bonCtrl.GetStreamID(&ONID, &TSID) == FALSE ){
		return FALSE;
	}
	EPGDB_EVENT_INFO info;

	DWORD ret = this->bonCtrl.GetEpgInfo(ONID, TSID, this->lastSID, nextFlag, &info);

	if(epgInfo != NULL ){
		*epgInfo = L"";
		if( ret == NO_ERR ){
			wstring epgText = L"";
			_ConvertEpgInfoText(&info, epgText);
			*epgInfo = epgText.c_str();
		}
	}

	return ret;
}

//シグナルレベルの取得
//戻り値：
// シグナルレベル
float CEpgDataCap_BonMain::GetSignalLevel()
{
	return this->bonCtrl.GetSignalLevel();
}

//エラーカウントをクリアする
void CEpgDataCap_BonMain::ClearErrCount(
	)
{
	if( this->nwCtrlID != 0 ){
		this->bonCtrl.ClearErrCount(this->nwCtrlID);
	}
}

//ドロップとスクランブルのカウントを取得する
//引数：
// drop				[OUT]ドロップ数
// scramble			[OUT]スクランブル数
void CEpgDataCap_BonMain::GetErrCount(
	ULONGLONG* drop,
	ULONGLONG* scramble
	)
{
	if( this->nwCtrlID != 0 ){
		this->bonCtrl.GetErrCount(this->nwCtrlID, drop, scramble);
	}

}

//録画を開始する
//戻り値：
// TRUE（成功）、FALSE（失敗）
BOOL CEpgDataCap_BonMain::StartRec(
	)
{
	if( this->bonCtrl.IsRec() == TRUE || this->recCtrlID != 0 ){
		return FALSE;
	}

	if( this->bonCtrl.CreateServiceCtrl(&this->recCtrlID) == FALSE ){
		return FALSE;
	}
	if( this->allService == TRUE ){
		this->bonCtrl.SetServiceID(this->recCtrlID, 0xFFFF);
	}else{
		this->bonCtrl.SetServiceID(this->recCtrlID, this->lastSID);
	}
	this->bonCtrl.SetScramble(this->recCtrlID, this->enableScrambleFlag);
	this->bonCtrl.SetServiceMode(this->recCtrlID, this->needCaption, this->needData);

	vector<CH_DATA4> serviceList;
	this->bonCtrl.GetServiceList(&serviceList);
	wstring serviceName = L"";
	Format(serviceName, L"%04X", this->lastSID);
	for( size_t i=0; i<serviceList.size(); i++){
		if( serviceList[i].originalNetworkID == this->lastONID &&
			serviceList[i].transportStreamID == this->lastTSID &&
			serviceList[i].serviceID == this->lastSID ){
				serviceName = serviceList[i].serviceName;
		}
	}

	wstring fileName = L"";
	SYSTEMTIME now;
	GetLocalTime(&now);
	Format(fileName, L"%04d%02d%02d-%02d%02d%02d-%s.ts",
		now.wYear,
		now.wMonth,
		now.wDay,
		now.wHour,
		now.wMinute,
		now.wSecond,
		serviceName.c_str()
		);
	ChkFileName(fileName);

	vector<REC_FILE_SET_INFO> saveFolder;
	REC_FILE_SET_INFO forderItem;
	forderItem.recFolder = this->recFolderList[0];
	saveFolder.push_back(forderItem);

	this->bonCtrl.StartSave(this->recCtrlID, fileName, this->overWriteFlag, FALSE, 0,0,0,0, 0, &saveFolder, &this->recFolderList);

	return TRUE;
}

//録画を停止する
// TRUE（成功）、FALSE（失敗）
BOOL CEpgDataCap_BonMain::StopRec()
{
	if( this->recCtrlID != 0 ){
		this->bonCtrl.EndSave(this->recCtrlID);
		this->bonCtrl.DeleteServiceCtrl(this->recCtrlID);
		this->recCtrlID = 0;
	}
	return TRUE;
}

//録画中かどうかを取得する
// TRUE（録画中）、FALSE（録画していない）
BOOL CEpgDataCap_BonMain::IsRec()
{
	return this->bonCtrl.IsRec();
}

//予約録画を停止する
void CEpgDataCap_BonMain::StopReserveRec()
{
	map<DWORD,DWORD>::iterator itr;
	for( itr = this->ctrlMap.begin(); itr != this->ctrlMap.end(); itr++ ){
		this->bonCtrl.DeleteServiceCtrl(itr->second);
	}
	this->ctrlMap.clear();
}

//チャンネルスキャンを開始する
//戻り値：
// エラーコード
DWORD CEpgDataCap_BonMain::StartChScan()
{
	return this->bonCtrl.StartChScan();
}

//チャンネルスキャンをキャンセルする
//戻り値：
// エラーコード
DWORD CEpgDataCap_BonMain::StopChScan()
{
	return this->bonCtrl.StopChScan();
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
DWORD CEpgDataCap_BonMain::GetChScanStatus(
	DWORD* space,
	DWORD* ch,
	wstring* chName,
	DWORD* chkNum,
	DWORD* totalNum
	)
{
	return this->bonCtrl.GetChScanStatus(space, ch, chName, chkNum, totalNum);
}

//EPG取得を開始する
//戻り値：
// エラーコード
DWORD CEpgDataCap_BonMain::StartEpgCap(
	)
{
	vector<EPGCAP_SERVICE_INFO> chList;
	this->bonCtrl.GetEpgCapService(&chList);
	if( chList.size() == 0 ){
		return ERR_FALSE;
	}
	return this->bonCtrl.StartEpgCap(&chList);
}

//EPG取得を停止する
//戻り値：
// エラーコード
DWORD CEpgDataCap_BonMain::StopEpgCap(
	)
{
	return this->bonCtrl.StopEpgCap();
}

//EPG取得のステータスを取得する
//戻り値：
// エラーコード
//引数：
// info			[OUT]取得中のサービス
DWORD CEpgDataCap_BonMain::GetEpgCapStatus(
	EPGCAP_SERVICE_INFO* info
	)
{
	return this->bonCtrl.GetEpgCapStatus(info);
}

//Viewアプリの起動を行う
void CEpgDataCap_BonMain::ViewAppOpen()
{
	if( this->viewPath.size() > 0 ){
		PROCESS_INFORMATION pi;
		STARTUPINFO si;
		ZeroMemory(&si,sizeof(si));
		si.cb=sizeof(si);

		wstring strOpen;
		Format(strOpen, L"\"%s\" %s", this->viewPath.c_str(), this->viewOpt.c_str());

		WCHAR* pszOpen = new WCHAR[strOpen.size() + 1];
		lstrcpy(pszOpen, strOpen.c_str());
		CreateProcess( NULL, pszOpen, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi );
		delete [] pszOpen;

		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}
}

void CEpgDataCap_BonMain::StartServer()
{
	wstring pipeName = L"";
	wstring eventName = L"";

	Format(pipeName, L"%s%d", CMD2_VIEW_CTRL_PIPE, GetCurrentProcessId());
	Format(eventName, L"%s%d", CMD2_VIEW_CTRL_WAIT_CONNECT, GetCurrentProcessId());

	OutputDebugString(pipeName.c_str());
	OutputDebugString(eventName.c_str());
	this->pipeServer.StartServer(eventName.c_str(), pipeName.c_str(), CtrlCmdCallback, this, 0, GetCurrentProcessId());
}

void CEpgDataCap_BonMain::StopServer()
{
	this->pipeServer.StopServer();
}

BOOL CEpgDataCap_BonMain::GetViewStatusInfo(
	float* signal,
	DWORD* space,
	DWORD* ch,
	ULONGLONG* drop,
	ULONGLONG* scramble,
	vector<NW_SEND_INFO>* sendUdpList,
	vector<NW_SEND_INFO>* sendTcpList
	)
{
	*sendUdpList = this->udpSendList;
	*sendTcpList = this->tcpSendList;
	return this->bonCtrl.GetViewStatusInfo(this->nwCtrlID, signal, space, ch, drop, scramble);
}

int CALLBACK CEpgDataCap_BonMain::CtrlCmdCallback(void* param, CMD_STREAM* cmdParam, CMD_STREAM* resParam)
{
	CEpgDataCap_BonMain* sys = (CEpgDataCap_BonMain*)param;

	resParam->dataSize = 0;
	resParam->param = CMD_ERR;

	//CtrlCmdCallbackInvoked()をメインスレッドで呼ぶ
	//注意: CPipeServerがアクティブな間、ウィンドウは確実に存在しなければならない
	sys->cmdCapture = cmdParam;
	sys->resCapture = resParam;
	SendMessage(sys->msgWnd, WM_INVOKE_CTRL_CMD, 0, 0);
	sys->cmdCapture = NULL;
	sys->resCapture = NULL;
	return 0;
}

void CEpgDataCap_BonMain::CtrlCmdCallbackInvoked()
{
	CMD_STREAM* cmdParam = this->cmdCapture;
	CMD_STREAM* resParam = this->resCapture;
	CEpgDataCap_BonMain* sys = this;

	switch( cmdParam->param ){
	case CMD2_VIEW_APP_SET_BONDRIVER:
		OutputDebugString(L"CMD2_VIEW_APP_SET_BONDRIVER");
		{
			wstring val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
				sys->CloseBonDriver();
				if( sys->OpenBonDriver(val.c_str()) == NO_ERR ){
					resParam->param = CMD_SUCCESS;
					PostMessage(sys->msgWnd, WM_CHG_TUNER, 0, 0);
				}
			}
		}
		break;
	case CMD2_VIEW_APP_GET_BONDRIVER:
		OutputDebugString(L"CMD2_VIEW_APP_GET_BONDRIVER");
		{
			if( sys->currentBonDriver.size() > 0 ){
				resParam->dataSize = GetVALUESize(sys->currentBonDriver);
				resParam->data = new BYTE[resParam->dataSize];
				if( WriteVALUE(sys->currentBonDriver, resParam->data, resParam->dataSize, NULL) == TRUE ){
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD2_VIEW_APP_SET_CH:
		OutputDebugString(L"CMD2_VIEW_APP_SET_CH");
		{
			if( sys->bonCtrl.IsRec() == FALSE ){
				if( sys->bonCtrl.GetEpgCapStatus(NULL) == ST_WORKING ){
					sys->bonCtrl.StopEpgCap();
				}
				SET_CH_INFO val;
				if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
					if( sys->bonCtrl.IsRec() == FALSE ){
						if( val.useSID == TRUE ){
							if(sys->SetCh(val.ONID, val.TSID, val.SID) == TRUE){
								sys->lastONID = val.ONID;
								sys->lastTSID = val.TSID;
								sys->lastSID = val.SID;

								resParam->param = CMD_SUCCESS;
								PostMessage(sys->msgWnd, WM_CHG_CH, 0, 0);
							}
						}else if( val.useBonCh == TRUE ){
							if(sys->bonCtrl.SetCh(val.space, val.ch) == TRUE){

								resParam->param = CMD_SUCCESS;
								PostMessage(sys->msgWnd, WM_CHG_CH, 0, 0);
							}
						}
					}
				}
			}
		}
		break;
	case CMD2_VIEW_APP_GET_DELAY:
		{
			int val = sys->bonCtrl.GetTimeDelay();

			resParam->dataSize = GetVALUESize(val);
			resParam->data = new BYTE[resParam->dataSize];
			if( WriteVALUE(val, resParam->data, resParam->dataSize, NULL) == TRUE ){
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_VIEW_APP_GET_STATUS:
		{
			DWORD val = VIEW_APP_ST_NORMAL;
			if( sys->currentBonDriver.size() == 0 ){
				val = VIEW_APP_ST_ERR_BON;
			}else if( sys->bonCtrl.IsRec() == TRUE ){
				val = VIEW_APP_ST_REC;
			}else if( sys->bonCtrl.GetEpgCapStatus(NULL) == ST_WORKING ){
				val = VIEW_APP_ST_GET_EPG;
			}else{
				//VIEW_APP_ST_NORMAL
				BOOL chChgErr = FALSE;
				if(sys->IsChChanging(&chChgErr) == FALSE ){
					if( chChgErr == TRUE ){
						val = VIEW_APP_ST_ERR_CH_CHG;
					}
				}
			}
			resParam->dataSize = GetVALUESize(val);
			resParam->data = new BYTE[resParam->dataSize];
			if( WriteVALUE(val, resParam->data, resParam->dataSize, NULL) == TRUE ){
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_VIEW_APP_CLOSE:
		OutputDebugString(L"CMD2_VIEW_APP_CLOSE");
		{
			map<DWORD,DWORD>::iterator itr;
			for( itr = sys->ctrlMap.begin(); itr != sys->ctrlMap.end(); itr++ ){
				sys->bonCtrl.DeleteServiceCtrl(itr->second);
			}
			sys->ctrlMap.clear();
			if( sys->recCtrlID != 0 ){
				sys->bonCtrl.DeleteServiceCtrl(sys->recCtrlID);
				sys->recCtrlID = 0;
			}
			if( sys->nwCtrlID != 0 ){
				sys->bonCtrl.DeleteServiceCtrl(sys->nwCtrlID);
				sys->nwCtrlID = 0;
			}
			resParam->param = CMD_SUCCESS;
			PostMessage(sys->msgWnd, WM_CLOSE, 0, 0);
		}
		break;
	case CMD2_VIEW_APP_SET_ID:
		OutputDebugString(L"CMD2_VIEW_APP_SET_ID");
		{
			if( ReadVALUE(&sys->outCtrlID, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_VIEW_APP_GET_ID:
		OutputDebugString(L"CMD2_VIEW_APP_GET_ID");
		{
			resParam->dataSize = GetVALUESize(sys->outCtrlID);
			resParam->data = new BYTE[resParam->dataSize];
			if( WriteVALUE(sys->outCtrlID, resParam->data, resParam->dataSize, NULL) == TRUE ){
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_VIEW_APP_SET_STANDBY_REC:
		OutputDebugString(L"CMD2_VIEW_APP_SET_STANDBY_REC");
		{
			DWORD val = 0;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
				resParam->param = CMD_SUCCESS;
				PostMessage(sys->msgWnd, WM_RESERVE_REC_STANDBY, val, 0);
			}
		}
		break;
	case CMD2_VIEW_APP_CREATE_CTRL:
		OutputDebugString(L"CMD2_VIEW_APP_CREATE_CTRL");
		{
			DWORD val = 0;
			if( sys->bonCtrl.CreateServiceCtrl(&val) == TRUE ){
				sys->ctrlMap.insert(pair<DWORD,DWORD>(val, val));
				resParam->dataSize = GetVALUESize(val);
				resParam->data = new BYTE[resParam->dataSize];
				if( WriteVALUE(val, resParam->data, resParam->dataSize, NULL) == TRUE ){
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD2_VIEW_APP_DELETE_CTRL:
		OutputDebugString(L"CMD2_VIEW_APP_DELETE_CTRL");
		{
			DWORD val = 0;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
				if( sys->bonCtrl.DeleteServiceCtrl(val) == TRUE ){
					map<DWORD,DWORD>::iterator itr;
					itr = sys->ctrlMap.find(val);
					if( itr != sys->ctrlMap.end() ){
						sys->ctrlMap.erase(itr);
					}
					resParam->param = CMD_SUCCESS;

					if( sys->ctrlMap.size() > 0 ){
						WORD sid = 0xFFFF;
						sys->bonCtrl.GetServiceID(sys->ctrlMap.begin()->first, &sid);
						sys->bonCtrl.SetServiceID(sys->nwCtrlID, sid);
						if( sid != 0xFFFF ){
							sys->lastSID = sid;
							PostMessage(sys->msgWnd, WM_CHG_CH, 0, 0);
						}
					}
				}
			}
		}
		break;
	case CMD2_VIEW_APP_SET_CTRLMODE:
		OutputDebugString(L"CMD2_VIEW_APP_SET_CTRLMODE");
		{
			SET_CTRL_MODE val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
				if(val.enableScramble == 0 && sys->nwCtrlID != 0){
					sys->bonCtrl.SetScramble(sys->nwCtrlID, FALSE);
				}
				sys->bonCtrl.SetScramble(val.ctrlID, val.enableScramble);
				sys->bonCtrl.SetServiceMode(val.ctrlID, val.enableCaption, val.enableData);
				sys->bonCtrl.SetServiceID(val.ctrlID, val.SID);
				
				resParam->param = CMD_SUCCESS;
			}
		}
		break;
	case CMD2_VIEW_APP_REC_START_CTRL:
		OutputDebugString(L"CMD2_VIEW_APP_REC_START_CTRL");
		{
			SET_CTRL_REC_PARAM val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
				BOOL overWrite = sys->overWriteFlag;
				if( val.overWriteFlag != 2 ){
					overWrite = val.overWriteFlag;
				}
				sys->bonCtrl.ClearErrCount(val.ctrlID);
				if(sys->bonCtrl.StartSave(val.ctrlID, val.fileName, overWrite, val.pittariFlag, val.pittariONID, val.pittariTSID, val.pittariSID, val.pittariEventID, val.createSize, &val.saveFolder, &sys->recFolderList) == TRUE){
					resParam->param = CMD_SUCCESS;
					PostMessage(sys->msgWnd, WM_RESERVE_REC_START, 0, 0);
				}
			}
		}
		break;
	case CMD2_VIEW_APP_REC_STOP_CTRL:
		OutputDebugString(L"CMD2_VIEW_APP_REC_STOP_CTRL");
		{
			SET_CTRL_REC_STOP_PARAM val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
				wstring saveFile = L"";
				BOOL subRec = FALSE;
				sys->bonCtrl.GetSaveFilePath(val.ctrlID, &saveFile, &subRec);
				if( saveFile.size() > 0 && val.saveErrLog == 1 ){
					wstring iniCommonPath = L"";
					GetCommonIniPath(iniCommonPath);

					WCHAR buff[512] = L"";
					GetPrivateProfileString(L"SET", L"RecInfoFolder", L"", buff, 512, iniCommonPath.c_str());
					wstring infoFolder = buff;
					ChkFolderPath(infoFolder);

					if( infoFolder.size() > 0 ){
						wstring tsFileName = L"";
						GetFileName(saveFile, tsFileName);
						wstring saveFileErr = L"";
						Format(saveFileErr, L"%s\\%s.err", infoFolder.c_str(), tsFileName.c_str());
						sys->bonCtrl.SaveErrCount(val.ctrlID, saveFileErr);
					}else{
						wstring saveFileErr = saveFile;
						saveFileErr += L".err";
						sys->bonCtrl.SaveErrCount(val.ctrlID, saveFileErr);
					}
				}
				SET_CTRL_REC_STOP_RES_PARAM resVal;
				resVal.recFilePath = saveFile;
				resVal.drop = 0;
				resVal.scramble = 0;
				resVal.subRecFlag = (BYTE)subRec;
				sys->bonCtrl.GetErrCount(val.ctrlID, &resVal.drop, &resVal.scramble);
				if(sys->bonCtrl.EndSave(val.ctrlID) == TRUE){
					resParam->dataSize = GetVALUESize(&resVal);
					resParam->data = new BYTE[resParam->dataSize];
					if( WriteVALUE(&resVal, resParam->data, resParam->dataSize, NULL) == TRUE ){
						resParam->param = CMD_SUCCESS;
					}
					if( sys->ctrlMap.size() == 1 ){
						PostMessage(sys->msgWnd, WM_RESERVE_REC_STOP, 0, 0);
					}
				}
			}
		}
		break;
	case CMD2_VIEW_APP_REC_FILE_PATH:
		OutputDebugString(L"CMD2_VIEW_APP_REC_FILE_PATH");
		{
			DWORD val = 0;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
				wstring saveFile = L"";
				BOOL subRec;
				sys->bonCtrl.GetSaveFilePath(val, &saveFile, &subRec);
				if( saveFile.size() > 0 ){
					resParam->dataSize = GetVALUESize(saveFile);
					resParam->data = new BYTE[resParam->dataSize];
					if( WriteVALUE(saveFile, resParam->data, resParam->dataSize, NULL) == TRUE ){
						resParam->param = CMD_SUCCESS;
					}
				}
			}
		}
		break;
	case CMD2_VIEW_APP_EPGCAP_START:
		OutputDebugString(L"CMD2_VIEW_APP_EPGCAP_START");
		{
			vector<SET_CH_INFO> val;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
				vector<EPGCAP_SERVICE_INFO> chList;
				for( size_t i=0; i<val.size(); i++ ){
					EPGCAP_SERVICE_INFO item;
					item.ONID = val[i].ONID;
					item.TSID = val[i].TSID;
					item.SID = val[i].SID;
					chList.push_back(item);
				}
				if( sys->bonCtrl.StartEpgCap(&chList) == NO_ERR ){
					PostMessage(sys->msgWnd, WM_RESERVE_EPGCAP_START, 0, 0);
					
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	case CMD2_VIEW_APP_EPGCAP_STOP:
		OutputDebugString(L"CMD2_VIEW_APP_EPGCAP_STOP");
		{
			sys->bonCtrl.StopEpgCap();
			resParam->param = CMD_SUCCESS;
			PostMessage(sys->msgWnd, WM_RESERVE_EPGCAP_STOP, 0, 0);
		}
		break;
	case CMD2_VIEW_APP_REC_STOP_ALL:
		OutputDebugString(L"CMD2_VIEW_APP_REC_STOP_ALL");
		{
			DWORD ret = CMD_SUCCESS;
			if( sys->recCtrlID != 0 ){
				if(sys->bonCtrl.EndSave(sys->recCtrlID) == FALSE ){
					ret = CMD_ERR;
				}
				sys->bonCtrl.DeleteServiceCtrl(sys->recCtrlID);
				sys->recCtrlID = 0;
			}
			map<DWORD,DWORD>::iterator itr;
			for( itr = sys->ctrlMap.begin(); itr != sys->ctrlMap.end(); itr++ ){
				sys->bonCtrl.DeleteServiceCtrl(itr->second);
			}
			sys->ctrlMap.clear();

			resParam->param = ret;
			PostMessage(sys->msgWnd, WM_RESERVE_REC_STOP, 0, 0);
		}
		break;
	case CMD2_VIEW_APP_SEARCH_EVENT:
		{
			SEARCH_EPG_INFO_PARAM key;
			if( ReadVALUE(&key, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
				EPGDB_EVENT_INFO epgInfo;
				if( sys->bonCtrl.SearchEpgInfo(key.ONID, key.TSID, key.SID, key.eventID, key.pfOnlyFlag, &epgInfo) == TRUE ){
					resParam->dataSize = GetVALUESize(&epgInfo);
					resParam->data = new BYTE[resParam->dataSize];
					if( WriteVALUE(&epgInfo, resParam->data, resParam->dataSize, NULL) == TRUE ){
						resParam->param = CMD_SUCCESS;
					}
				}
			}
		}
		break;
	case CMD2_VIEW_APP_GET_EVENT_PF:
		{
			GET_EPG_PF_INFO_PARAM key;
			if( ReadVALUE(&key, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
				EPGDB_EVENT_INFO epgInfo;
				if( sys->bonCtrl.GetEpgInfo(key.ONID, key.TSID, key.SID, key.pfNextFlag, &epgInfo) == TRUE ){
					resParam->dataSize = GetVALUESize(&epgInfo);
					resParam->data = new BYTE[resParam->dataSize];
					if( WriteVALUE(&epgInfo, resParam->data, resParam->dataSize, NULL) == TRUE ){
						resParam->param = CMD_SUCCESS;
					}
				}
			}
		}
		break;
	case CMD2_VIEW_APP_EXEC_VIEW_APP:
		{
			sys->ViewAppOpen();
			resParam->param = CMD_SUCCESS;
		}
		break;
	case CMD2_VIEW_APP_REC_WRITE_SIZE:
		{
			DWORD val = 0;
			if( ReadVALUE(&val, cmdParam->data, cmdParam->dataSize, NULL ) == TRUE ){
				__int64 writeSize = -1;
				sys->bonCtrl.GetRecWriteSize(val, &writeSize);
				resParam->dataSize = GetVALUESize(writeSize);
				resParam->data = new BYTE[resParam->dataSize];
				if( WriteVALUE(writeSize, resParam->data, resParam->dataSize, NULL) == TRUE ){
					resParam->param = CMD_SUCCESS;
				}
			}
		}
		break;
	default:
		_OutputDebugString(L"err default cmd %d\r\n", cmdParam->param);
		resParam->param = CMD_NON_SUPPORT;
		break;
	}
}
