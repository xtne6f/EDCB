#ifndef __COMMON_DEF_H__
#define __COMMON_DEF_H__

#include "ErrDef.h"
#include "Util.h"
#include "StructDef.h"

#define SAVE_FOLDER L"\\EpgTimerBon"
#define EPG_SAVE_FOLDER L"\\EpgData"
#define LOGO_SAVE_FOLDER L"\\LogoData"
#define BON_DLL_FOLDER L"\\BonDriver"

#define RESERVE_TEXT_NAME L"Reserve.txt"
#define REC_INFO_TEXT_NAME L"RecInfo.txt"
#define REC_INFO2_TEXT_NAME L"RecInfo2.txt"
#define EPG_AUTO_ADD_TEXT_NAME L"EpgAutoAdd.txt"
#define MANUAL_AUTO_ADD_TEXT_NAME L"ManualAutoAdd.txt"

#define EPG_TIMER_SERVICE_EXE L"EpgTimerSrv.exe"

#define EPG_TIMER_BON_MUTEX L"Global\\EpgTimer_Bon2"
#define EPG_TIMER_BON_LITE_MUTEX L"Global\\EpgTimer_Bon2_Lite"
#define EPG_TIMER_BON_SRV_MUTEX L"Global\\EpgTimer_Bon_Service"
#define SERVICE_NAME L"EpgTimer Service"

#define RECMODE_ALL 0 //全サービス
#define RECMODE_SERVICE 1 //指定サービスのみ
#define RECMODE_ALL_NOB25 2 //全サービス（B25処理なし）
#define RECMODE_SERVICE_NOB25 3 //指定サービスのみ（B25処理なし）
#define RECMODE_VIEW 4 //視聴
#define RECMODE_NO 5 //無効
#define RECMODE_EPG 0xFF //EPG取得

#define RESERVE_EXECUTE 0 //普通に予約実行
#define RESERVE_PILED_UP 1 //重なって実行できない予約あり
#define RESERVE_NO_EXECUTE 2 //重なって実行できない
#define RESERVE_NO 3 //無効

#define RECSERVICEMODE_DEF	0x00000000	//デフォルト設定
#define RECSERVICEMODE_SET	0x00000001	//設定値使用
#define RECSERVICEMODE_CAP	0x00000010	//字幕データ含む
#define RECSERVICEMODE_DATA	0x00000020	//データカルーセル含む

#define REC_STATUS_WAIT		0x00000000	//録画待ち
#define REC_STATUS_END		0x00000001	//正常終了
#define REC_STATUS_ERR_WAKE	0x00000002	//録画時間過ぎてエラー
#define	REC_STATUS_ERR_OPEN	0x00000004	//EXE起動できなくてエラー
#define	REC_STATUS_ERR_FIND	0x00000008	//EPG情報見つからなくてエラー
#define	REC_STATUS_END_PG	0x00000010	//プログラム予約に変更して録画終了（EPG情報確認できなかった）
#define REC_STATUS_CHG_TIME	0x00000100	//予約時の開始時間と変わって録画
#define REC_STATUS_RELAY	0x00000200	//イベントリレーで録画

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

#define REC_END_STATUS_NORMAL		1		//正常終了
#define REC_END_STATUS_OPEN_ERR		2		//チューナーのオープンができなかった
#define REC_END_STATUS_ERR_END		3		//録画中にエラーが発生した
#define REC_END_STATUS_NEXT_START_END	4	//次の予約開始のため終了
#define REC_END_STATUS_START_ERR	5		//開始時間が過ぎていた
#define REC_END_STATUS_CHG_TIME		6		//開始時間が変更された
#define REC_END_STATUS_NO_TUNER		7		//チューナーが足りなかった
#define REC_END_STATUS_NO_RECMODE	8		//無効扱いだった
#define REC_END_STATUS_NOT_FIND_PF	9		//p/fに番組情報確認できなかった
#define REC_END_STATUS_NOT_FIND_6H	10		//6時間番組情報確認できなかった
#define REC_END_STATUS_END_SUBREC	11		//サブフォルダへの録画が発生した
#define REC_END_STATUS_ERR_RECSTART 12		//録画開始に失敗した
#define REC_END_STATUS_NOT_START_HEAD 13	//一部のみ録画された
#define REC_END_STATUS_ERR_CH_CHG	14		//チャンネル切り替えに失敗した
#define REC_END_STATUS_ERR_END2		15		//録画中にエラーが発生した(Writeでexception)

//NotifyID
#define NOTIFY_UPDATE_EPGDATA		1		//EPGデータが更新された
#define NOTIFY_UPDATE_RESERVE_INFO	2		//予約情報が更新された
#define NOTIFY_UPDATE_REC_INFO	3			//録画結果情報が更新された
#define NOTIFY_UPDATE_AUTOADD_EPG	4		//EPG自動予約登録情報が更新された
#define NOTIFY_UPDATE_AUTOADD_MANUAL	5	//プログラム自動予約登録情報が更新された
#define NOTIFY_UPDATE_SRV_STATUS	100		//Srvの動作状況が変更（param1:ステータス 0:通常、1:録画中、2:EPG取得中）
#define NOTIFY_UPDATE_PRE_REC_START	101		//録画準備開始（param4:ログ用メッセージ）
#define NOTIFY_UPDATE_REC_START		102		//録画開始（param4:ログ用メッセージ）
#define NOTIFY_UPDATE_REC_END		103		//録画終了（param4:ログ用メッセージ）
#define NOTIFY_UPDATE_REC_TUIJYU	104		//録画中に追従が発生（param4:ログ用メッセージ）
#define NOTIFY_UPDATE_CHG_TUIJYU	105		//EPG自動予約登録で追従が発生（param4:ログ用メッセージ）
#define NOTIFY_UPDATE_PRE_EPGCAP_START	106	//EPG取得準備開始
#define NOTIFY_UPDATE_EPGCAP_START	107		//EPG取得開始
#define NOTIFY_UPDATE_EPGCAP_END	108		//EPG取得終了

#endif