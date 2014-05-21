#include "StdAfx.h"
#include "EpgDataCap3Main.h"


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

class CBlockLock
{
public:
	CBlockLock(CRITICAL_SECTION* lock_) : lock(lock_) { EnterCriticalSection(lock); }
	~CBlockLock() { LeaveCriticalSection(lock); }
private:
	CRITICAL_SECTION* lock;
};

//DLLの初期化
//戻り値：
// エラーコード
//引数：
// asyncMode		[IN]TRUE:非同期モード、FALSE:同期モード
DWORD CEpgDataCap3Main::Initialize(
	BOOL asyncFlag
	)
{
	return NO_ERR;
}

//DLLの開放
//戻り値：
// エラーコード
DWORD CEpgDataCap3Main::UnInitialize(
	)
{
	return NO_ERR;
}

//解析対象のTSパケット１つを読み込ませる
//戻り値：
// エラーコード
// data		[IN]TSパケット１つ
// size		[IN]dataのサイズ（188、192あたりになるはず）
DWORD CEpgDataCap3Main::AddTSPacket(
	BYTE* data,
	DWORD size
	)
{
	if( size < 188 ){
		return ERR_INVALID_ARG;
	}

	CBlockLock lock(&this->utilLock);

	DWORD stratPos = 0;
	if( size > 188 ){
		if( data[0] != 0x47 ){
			if( data[size-188] == 0x47 ){
				stratPos = size-188;
			}else{
				for( DWORD i=0; i<size-188; i++ ){
					if( data[i] == 0x47 ){
						break;
					}else{
						stratPos++;
					}
				}
			}
		}
	}
	DWORD err = this->decodeUtilClass.AddTSData(data+stratPos, 188);
	return err;
}

//解析データの現在のストリームＩＤを取得する
//戻り値：
// エラーコード
//引数：
// originalNetworkID		[OUT]現在のoriginalNetworkID
// transportStreamID		[OUT]現在のtransportStreamID
DWORD CEpgDataCap3Main::GetTSID(
	WORD* originalNetworkID,
	WORD* transportStreamID
	)
{
	CBlockLock lock(&this->utilLock);
	DWORD err = this->decodeUtilClass.GetTSID(originalNetworkID, transportStreamID);
	return err;
}

//自ストリームのサービス一覧を取得する
//戻り値：
// エラーコード
//引数：
// serviceListSize			[OUT]serviceListの個数
// serviceList				[OUT]サービス情報のリスト（DLL内で自動的にdeleteする。次に取得を行うまで有効）
DWORD CEpgDataCap3Main::GetServiceListActual(
	DWORD* serviceListSize,
	SERVICE_INFO** serviceList
	)
{
	CBlockLock lock(&this->utilLock);
	DWORD err = this->decodeUtilClass.GetServiceListActual(serviceListSize, serviceList);
	return err;
}

//蓄積されたEPG情報のあるサービス一覧を取得する
//SERVICE_EXT_INFOの情報はない場合がある
//戻り値：
// エラーコード
//引数：
// serviceListSize			[OUT]serviceListの個数
// serviceList				[OUT]サービス情報のリスト（DLL内で自動的にdeleteする。次に取得を行うまで有効）
DWORD CEpgDataCap3Main::GetServiceListEpgDB(
	DWORD* serviceListSize,
	SERVICE_INFO** serviceList
	)
{
	CBlockLock lock(&this->utilLock);
	DWORD err = this->epgDBUtilClass.GetServiceListEpgDB(serviceListSize, serviceList);
	return err;
}

//指定サービスの全EPG情報を取得する
//戻り値：
// エラーコード
//引数：
// originalNetworkID		[IN]取得対象のoriginalNetworkID
// transportStreamID		[IN]取得対象のtransportStreamID
// serviceID				[IN]取得対象のServiceID
// epgInfoListSize			[OUT]epgInfoListの個数
// epgInfoList				[OUT]EPG情報のリスト（DLL内で自動的にdeleteする。次に取得を行うまで有効）
DWORD CEpgDataCap3Main::GetEpgInfoList(
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	DWORD* epgInfoListSize,
	EPG_EVENT_INFO** epgInfoList
	)
{
	CBlockLock lock(&this->utilLock);
	DWORD err = this->epgDBUtilClass.GetEpgInfoList(originalNetworkID, transportStreamID, serviceID, epgInfoListSize, epgInfoList);
	return err;
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
DWORD CEpgDataCap3Main::GetEpgInfo(
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	BOOL nextFlag,
	EPG_EVENT_INFO** epgInfo
	)
{
	CBlockLock lock(&this->utilLock);
	SYSTEMTIME nowTime;
	if( this->decodeUtilClass.GetNowTime(&nowTime) != NO_ERR ){
		GetLocalTime(&nowTime);
	}
	DWORD err = this->epgDBUtilClass.GetEpgInfo(originalNetworkID, transportStreamID, serviceID, nextFlag, nowTime, epgInfo);
	return err;
}

//指定イベントのEPG情報を取得する
//戻り値：
// エラーコード
//引数：
// originalNetworkID		[IN]取得対象のoriginalNetworkID
// transportStreamID		[IN]取得対象のtransportStreamID
// serviceID				[IN]取得対象のServiceID
// EventID					[IN]取得対象のEventID
// pfOnlyFlag				[IN]p/fからのみ検索するかどうか
// epgInfo					[OUT]EPG情報（DLL内で自動的にdeleteする。次に取得を行うまで有効）
DWORD CEpgDataCap3Main::SearchEpgInfo(
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	WORD eventID,
	BYTE pfOnlyFlag,
	EPG_EVENT_INFO** epgInfo
	)
{
	CBlockLock lock(&this->utilLock);

	DWORD err = this->epgDBUtilClass.SearchEpgInfo(originalNetworkID, transportStreamID, serviceID, eventID, pfOnlyFlag, epgInfo);
	return err;
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
