#ifndef INCLUDE_ERR_DEF_H
#define INCLUDE_ERR_DEF_H

#define ERR_FALSE		FALSE		//汎用エラー
#define NO_ERR			TRUE		//成功
#define ERR_NOT_INIT	11			//未初期化
#define ERR_INVALID_ARG	14			//引数が無効
#define ERR_NOT_FIND	15			//情報が見つからなかった

#define CMD_SUCCESS			NO_ERR		//成功
#define CMD_ERR				ERR_FALSE	//汎用エラー
#define CMD_NON_SUPPORT		203			//未サポートのコマンド
#define CMD_ERR_INVALID_ARG	204			//引数エラー
#define CMD_ERR_CONNECT		205			//サーバーにコネクトできなかった
#define CMD_ERR_DISCONNECT	206			//サーバーから切断された
#define CMD_ERR_TIMEOUT		207			//タイムアウト発生
#define CMD_ERR_BUSY		208			//ビジー状態で現在処理できない（EPGデータ読み込み中、録画中など）
#define CMD_NO_RES			250			//コマンドの応答を保留したい（サーバー内部で使用）
#define CMD_NO_RES_THREAD	251			//コマンドの応答にスレッドを使いたい（サーバー内部で使用）

#endif
