#include "StdAfx.h"
#include "EpgDataCap3Main.h"

#include "../../Common/BlockLock.h"

CEpgDataCap3Main::CEpgDataCap3Main(void)
{
	InitializeCriticalSection(&this->utilLock);

	decodeUtilClass.SetEpgDB(&(this->epgDBUtilClass));
}

CEpgDataCap3Main::~CEpgDataCap3Main(void)
{
	decodeUtilClass.SetEpgDB(NULL);

	DeleteCriticalSection(&this->utilLock);
}

//解析対象のTSパケット１つを読み込ませる
// data		[IN]TSパケット１つ
void CEpgDataCap3Main::AddTSPacket(
	BYTE* data
	)
{
	CBlockLock lock(&this->utilLock);

	this->decodeUtilClass.AddTSData(data);
}

//解析データの現在のストリームＩＤを取得する
//引数：
// originalNetworkID		[OUT]現在のoriginalNetworkID
// transportStreamID		[OUT]現在のtransportStreamID
BOOL CEpgDataCap3Main::GetTSID(
	WORD* originalNetworkID,
	WORD* transportStreamID
	)
{
	CBlockLock lock(&this->utilLock);
	return this->decodeUtilClass.GetTSID(originalNetworkID, transportStreamID);
}

//自ストリームのサービス一覧を取得する
//引数：
// serviceListSize			[OUT]serviceListの個数
// serviceList				[OUT]サービス情報のリスト（DLL内で自動的にdeleteする。次に取得を行うまで有効）
BOOL CEpgDataCap3Main::GetServiceListActual(
	DWORD* serviceListSize,
	SERVICE_INFO** serviceList
	)
{
	CBlockLock lock(&this->utilLock);
	return this->decodeUtilClass.GetServiceListActual(serviceListSize, serviceList);
}

//蓄積されたEPG情報のあるサービス一覧を取得する
//SERVICE_EXT_INFOの情報はない場合がある
//引数：
// serviceListSize			[OUT]serviceListの個数
// serviceList				[OUT]サービス情報のリスト（DLL内で自動的にdeleteする。次に取得を行うまで有効）
void CEpgDataCap3Main::GetServiceListEpgDB(
	DWORD* serviceListSize,
	SERVICE_INFO** serviceList
	)
{
	CBlockLock lock(&this->utilLock);
	this->epgDBUtilClass.GetServiceListEpgDB(serviceListSize, serviceList);
}

//指定サービスの全EPG情報を取得する
//引数：
// originalNetworkID		[IN]取得対象のoriginalNetworkID
// transportStreamID		[IN]取得対象のtransportStreamID
// serviceID				[IN]取得対象のServiceID
// epgInfoListSize			[OUT]epgInfoListの個数
// epgInfoList				[OUT]EPG情報のリスト（DLL内で自動的にdeleteする。次に取得を行うまで有効）
BOOL CEpgDataCap3Main::GetEpgInfoList(
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	DWORD* epgInfoListSize,
	EPG_EVENT_INFO** epgInfoList
	)
{
	CBlockLock lock(&this->utilLock);
	return this->epgDBUtilClass.GetEpgInfoList(originalNetworkID, transportStreamID, serviceID, epgInfoListSize, epgInfoList);
}

//指定サービスの現在or次のEPG情報を取得する
//引数：
// originalNetworkID		[IN]取得対象のoriginalNetworkID
// transportStreamID		[IN]取得対象のtransportStreamID
// serviceID				[IN]取得対象のServiceID
// nextFlag					[IN]TRUE（次の番組）、FALSE（現在の番組）
// epgInfo					[OUT]EPG情報（DLL内で自動的にdeleteする。次に取得を行うまで有効）
BOOL CEpgDataCap3Main::GetEpgInfo(
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	BOOL nextFlag,
	EPG_EVENT_INFO** epgInfo
	)
{
	CBlockLock lock(&this->utilLock);
	SYSTEMTIME nowTime;
	if( this->decodeUtilClass.GetNowTime(&nowTime) == FALSE ){
		GetLocalTime(&nowTime);
	}
	return this->epgDBUtilClass.GetEpgInfo(originalNetworkID, transportStreamID, serviceID, nextFlag, nowTime, epgInfo);
}

//指定イベントのEPG情報を取得する
//引数：
// originalNetworkID		[IN]取得対象のoriginalNetworkID
// transportStreamID		[IN]取得対象のtransportStreamID
// serviceID				[IN]取得対象のServiceID
// EventID					[IN]取得対象のEventID
// pfOnlyFlag				[IN]p/fからのみ検索するかどうか
// epgInfo					[OUT]EPG情報（DLL内で自動的にdeleteする。次に取得を行うまで有効）
BOOL CEpgDataCap3Main::SearchEpgInfo(
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	WORD eventID,
	BYTE pfOnlyFlag,
	EPG_EVENT_INFO** epgInfo
	)
{
	CBlockLock lock(&this->utilLock);

	return this->epgDBUtilClass.SearchEpgInfo(originalNetworkID, transportStreamID, serviceID, eventID, pfOnlyFlag, epgInfo);
}

//EPGデータの蓄積状態をリセットする
void CEpgDataCap3Main::ClearSectionStatus()
{
	CBlockLock lock(&this->utilLock);
	this->epgDBUtilClass.ClearSectionStatus();
	return ;
}

//EPGデータの蓄積状態を取得する
//戻り値：
// ステータス
//引数：
// l_eitFlag		[IN]L-EITのステータスを取得
EPG_SECTION_STATUS CEpgDataCap3Main::GetSectionStatus(BOOL l_eitFlag)
{
	CBlockLock lock(&this->utilLock);
	EPG_SECTION_STATUS status = this->epgDBUtilClass.GetSectionStatus(l_eitFlag);
	return status;
}

//PC時計を元としたストリーム時間との差を取得する
//戻り値：
// 差の秒数
int CEpgDataCap3Main::GetTimeDelay(
	)
{
	CBlockLock lock(&this->utilLock);
	int delay = this->decodeUtilClass.GetTimeDelay();
	return delay;
}
