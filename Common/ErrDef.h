#ifndef __ERR_DEF_H__
#define __ERR_DEF_H__

#define ERR_FALSE		FALSE		//汎用エラー
#define NO_ERR			TRUE		//成功
#define ERR_INIT		10			//初期化失敗
#define ERR_NOT_INIT	11			//未初期化
#define ERR_SIZE		12			//入力サイズが不正
#define ERR_LOAD_MODULE	13			//モジュールのロードに失敗
#define ERR_INVALID_ARG	14			//引数が無効
#define ERR_NOT_FIND	15			//情報が見つからなかった
#define ERR_NEED_NEXT_PACKET	20	//次のTSパケット入れないと解析できない
#define ERR_CAN_NOT_ANALYZ		21	//本当にTSパケット？解析不可能
#define ERR_NOT_FIRST 			22	//最初のTSパケット未入力
#define ERR_INVALID_PACKET		23	//本当にTSパケット？パケット飛んで壊れてるかも
#define ERR_NO_CHAGE			30	//バージョンの変更ないため解析不要

#define ERR_LOAD_B25	40			//B25Decorder.dllのロードに失敗
#define ERR_OPEN_TUNER	41			//指定BonDriverのOpenに失敗
#define ERR_FIND_TUNER	42			//指定BonDriverの検索に失敗
#define ERR_LOAD_EPG	43			//EpgDataCap.dllのロードに失敗

#define NO_ERR_EPG_ALL		100		//EPG情報貯まった BasicとExtend両方
#define NO_ERR_EPG_BASIC	101		//EPG情報貯まった Basicのみ
#define NO_ERR_EPG_EXTENDED	102		//EPG情報貯まった Extendのみ

#define CMD_SUCCESS			NO_ERR		//成功
#define CMD_ERR				ERR_FALSE	//汎用エラー
#define CMD_NEXT			202			//Enumコマンド用、続きあり
#define CMD_NON_SUPPORT		203			//未サポートのコマンド
#define CMD_ERR_INVALID_ARG	204			//引数エラー
#define CMD_ERR_CONNECT		205			//サーバーにコネクトできなかった
#define CMD_ERR_DISCONNECT	206			//サーバーから切断された
#define CMD_ERR_TIMEOUT		207			//タイムアウト発生
#define CMD_ERR_BUSY		208			//ビジー状態で現在処理できない（EPGデータ読み込み中、録画中など）
#define CMD_NO_RES			250			//Post用でレスポンスの必要なし

//チャンネルスキャン、EPG取得のステータス用
#define	ST_STOP				300			//停止中
#define ST_WORKING			301			//実行中
#define ST_COMPLETE			302			//完了
#define ST_CANCEL			303			//キャンセルされた

#endif
