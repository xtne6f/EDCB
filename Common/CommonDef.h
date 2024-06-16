#ifndef INCLUDE_COMMON_DEF_H
#define INCLUDE_COMMON_DEF_H

#include "ErrDef.h"
#include "StructDef.h"

#define EPG_ARCHIVE_FOLDER L"EpgArc2"
#define EPG_SAVE_FOLDER L"EpgData"
#define LOGO_SAVE_FOLDER L"LogoData"
#define BON_DLL_FOLDER L"BonDriver"

#define EPG_ARCHIVE_DATA_NAME L"EpgArc.dat"
#define RESERVE_TEXT_NAME L"Reserve.txt"
#define REC_INFO_TEXT_NAME L"RecInfo.txt"
#define REC_INFO2_TEXT_NAME L"RecInfo2.txt"
#define EPG_AUTO_ADD_TEXT_NAME L"EpgAutoAdd.txt"
#define MANUAL_AUTO_ADD_TEXT_NAME L"ManualAutoAdd.txt"

#define EPG_TIMER_BON_SRV_MUTEX L"EpgTimer_Bon_Service"
#define SERVICE_NAME L"EpgTimer Service"

#define RECMODE_ALL 0 //全サービス
#define RECMODE_SERVICE 1 //指定サービスのみ
#define RECMODE_ALL_NOB25 2 //全サービス（B25処理なし）
#define RECMODE_SERVICE_NOB25 3 //指定サービスのみ（B25処理なし）
#define RECMODE_VIEW 4 //視聴

#define RESERVE_EXECUTE 0 //普通に予約実行
#define RESERVE_PILED_UP 1 //重なって実行できない予約あり
#define RESERVE_NO_EXECUTE 2 //重なって実行できない

#define RECSERVICEMODE_DEF	0x00000000	//デフォルト設定
#define RECSERVICEMODE_SET	0x00000001	//設定値使用
#define RECSERVICEMODE_CAP	0x00000010	//字幕データ含む
#define RECSERVICEMODE_DATA	0x00000020	//データカルーセル含む

//予約追加状態
#define ADD_RESERVE_NORMAL		0x00	//通常
#define ADD_RESERVE_RELAY		0x01	//イベントリレーで追加
#define ADD_RESERVE_NO_FIND		0x02	//6時間追従モード
#define ADD_RESERVE_CHG_PF		0x04	//最新EPGで変更済み(p/fチェック)
#define ADD_RESERVE_CHG_PF2		0x08	//最新EPGで変更済み(通常チェック)
#define ADD_RESERVE_NO_EPG		0x10	//EPGなしで延長済み
#define ADD_RESERVE_UNKNOWN_END	0x20	//終了未定状態

//Viewアプリ（EpgDataCap_Bon）のステータス
#define VIEW_APP_ST_NORMAL				0 //通常状態
#define VIEW_APP_ST_ERR_BON				1 //BonDriverの初期化に失敗
#define VIEW_APP_ST_REC					2 //録画状態
#define VIEW_APP_ST_GET_EPG				3 //EPG取得状態
#define VIEW_APP_ST_ERR_CH_CHG			4 //チャンネル切り替え失敗状態

//Viewアプリの取得すべき情報のフラグ
#define VIEW_APP_FLAG_GET_STATUS	0x01	//statusフィールドを取得する
#define VIEW_APP_FLAG_GET_DELAY		0x02	//delaySecフィールドを取得する
#define VIEW_APP_FLAG_GET_BONDRIVER	0x04	//bonDriverフィールドを取得する

//NotifyID
#define NOTIFY_UPDATE_EPGDATA		1		//EPGデータが更新された
#define NOTIFY_UPDATE_RESERVE_INFO	2		//予約情報が更新された
#define NOTIFY_UPDATE_REC_INFO	3			//録画結果情報が更新された
#define NOTIFY_UPDATE_AUTOADD_EPG	4		//EPG自動予約登録情報が更新された
#define NOTIFY_UPDATE_AUTOADD_MANUAL	5	//プログラム自動予約登録情報が更新された
#define NOTIFY_UPDATE_PROFILE		51		//設定ファイル(ini)が更新された
#define NOTIFY_UPDATE_SRV_STATUS	100		//Srvの動作状況が変更（param1:ステータス 0:通常、1:録画中、2:EPG取得中）
#define NOTIFY_UPDATE_PRE_REC_START	101		//録画準備開始（param4:ログ用メッセージ）
#define NOTIFY_UPDATE_REC_START		102		//録画開始（param4:ログ用メッセージ）
#define NOTIFY_UPDATE_REC_END		103		//録画終了（param4:ログ用メッセージ）
#define NOTIFY_UPDATE_REC_TUIJYU	104		//録画中に追従が発生（param4:ログ用メッセージ）
#define NOTIFY_UPDATE_CHG_TUIJYU	105		//EPG自動予約登録で追従が発生（param4:ログ用メッセージ）
#define NOTIFY_UPDATE_PRE_EPGCAP_START	106	//EPG取得準備開始
#define NOTIFY_UPDATE_EPGCAP_START	107		//EPG取得開始
#define NOTIFY_UPDATE_EPGCAP_END	108		//EPG取得終了

//WM_COPYDATAの型
#define COPYDATA_TYPE_LUAPOST 0x45544C50 //Luaスクリプト実行をプロセスに要求する(完了を待たない)

//Luaスクリプト受け取り用FIFOファイル名
#define LUAPOST_FIFO L"EpgTimerSrvLuaPost.fifo"

#endif
