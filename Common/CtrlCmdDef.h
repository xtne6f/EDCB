#ifndef INCLUDE_CTRL_CMD_DEF_H
#define INCLUDE_CTRL_CMD_DEF_H

//デフォルトコネクトタイムアウト
#define CONNECT_TIMEOUT 15*1000

//パイプ名
//"Pipe"を"Connect"に置換した名前のイベントオブジェクトが接続時の勧告ロックとして使われる
//ただし"NoWaitPipe"の場合、クライアントはこのイベントオブジェクトを無視すること(接続は必ずしも成功しない)
#define CMD2_EPG_SRV_PIPE L"EpgTimerSrvPipe"
#define CMD2_EPG_SRV_NOWAIT_PIPE L"EpgTimerSrvNoWaitPipe"
#define CMD2_GUI_CTRL_PIPE L"EpgTimerGUI_Ctrl_BonPipe_" //+プロセスID
#define CMD2_VIEW_CTRL_PIPE L"View_Ctrl_BonPipe_" //+プロセスID
#define CMD2_TVTEST_CTRL_PIPE L"TvTest_Ctrl_BonPipe_" //+プロセスID

//コマンドバージョン
//#define CMD_VER 2	//バージョン情報追加対応　録画設定への部分受信録画フォルダ指定追加
//#define CMD_VER 3	//検索条件に同一録画チェック追加
//#define CMD_VER 4	//録画済み情報にプロテクト追加
#define CMD_VER 5 //予定ファイル名追加

//コマンド
//コメントはコマンド送信側視点で、送信側の目的または受信側の動作を説明する
#define CMD2_EPG_SRV_ADDLOAD_RESERVE		1 //Program.txtの追加読み込み（廃止）
#define CMD2_EPG_SRV_RELOAD_EPG				2 //EPG再読み込みを開始する
#define CMD2_EPG_SRV_RELOAD_SETTING			3 //設定情報を再読み込みする
#define CMD2_EPG_SRV_CLOSE					4 //サービスや常駐モードでなければアプリケーションを終了する
#define CMD2_EPG_SRV_REGIST_GUI				5 //EpgTimerSrvのパイプ接続GUIとしてプロセスを登録する（通知が受信可能になる）
#define CMD2_EPG_SRV_UNREGIST_GUI			6 //EpgTimerSrvのパイプ接続GUI登録を解除する
#define CMD2_EPG_SRV_REGIST_GUI_TCP			7 //TCP接続のGUIアプリケーションのIPと通知受信用ポートを登録
#define CMD2_EPG_SRV_UNREGIST_GUI_TCP		8 //TCP接続のGUIアプリケーションのIPと通知受信用ポートの登録を解除
#define CMD2_EPG_SRV_ISREGIST_GUI_TCP		9 //TCP接続のGUIアプリケーションのIPと通知受信用ポートの登録状況確認

//特別な応答を返すコマンド
#define CMD2_EPG_SRV_RELAY_VIEW_STREAM		301 //ViewアプリのSrvPipeストリームを転送する

#define CMD2_EPG_SRV_ENUM_RESERVE			1011 //予約一覧取得
#define CMD2_EPG_SRV_GET_RESERVE			1012 //予約情報取得
#define CMD2_EPG_SRV_ADD_RESERVE			1013 //予約追加
#define CMD2_EPG_SRV_DEL_RESERVE			1014 //予約削除
#define CMD2_EPG_SRV_CHG_RESERVE			1015 //予約変更
#define CMD2_EPG_SRV_ENUM_TUNER_RESERVE		1016 //チューナーごとの予約ID一覧取得
#define CMD2_EPG_SRV_ENUM_RECINFO			1017 //録画済み情報一覧取得
#define CMD2_EPG_SRV_DEL_RECINFO			1018 //録画済み情報削除
#define CMD2_EPG_SRV_CHG_PATH_RECINFO		1019 //録画済み情報のファイルパス変更

//バージョン情報追加対応版
#define CMD2_EPG_SRV_ENUM_RESERVE2			2011 //予約一覧取得
#define CMD2_EPG_SRV_GET_RESERVE2			2012 //予約情報取得
#define CMD2_EPG_SRV_ADD_RESERVE2			2013 //予約追加
#define CMD2_EPG_SRV_CHG_RESERVE2			2015 //予約変更
#define CMD2_EPG_SRV_ENUM_RECINFO2			2017 //録画済み情報一覧取得
#define CMD2_EPG_SRV_CHG_PROTECT_RECINFO2	2019 //録画済み情報のプロテクト変更
#define CMD2_EPG_SRV_ENUM_RECINFO_BASIC2	2020 //録画済み情報一覧取得（programInfoとerrInfoを除く）
#define CMD2_EPG_SRV_GET_RECINFO_LIST2		2022 //録画済み情報一覧取得（指定IDリスト）
#define CMD2_EPG_SRV_GET_RECINFO2			2024 //録画済み情報取得
#define CMD2_EPG_SRV_ADDCHK_RESERVE2		2030 //サーバー連携用　予約追加できるかのチェック（戻り値 0:追加不可 1:追加可能 2:追加可能だが開始か終了が重なるものあり 3:すでに同じ物がある）
#define CMD2_EPG_SRV_GET_EPG_FILETIME2		2031 //サーバー連携用　EPGデータファイルのタイムスタンプ取得
#define CMD2_EPG_SRV_GET_EPG_FILE2			2032 //サーバー連携用　EPGデータファイル取得
#define CMD2_EPG_SRV_SEARCH_PG2				2125 //番組検索
#define CMD2_EPG_SRV_SEARCH_PG_BYKEY2		2127 //番組検索、キーごとのイベントを全て返す
#define CMD2_EPG_SRV_SEARCH_PG_ARC2			2129 //過去番組検索
#define CMD2_EPG_SRV_ENUM_AUTO_ADD2			2131 //自動予約登録の条件一覧取得
#define CMD2_EPG_SRV_ADD_AUTO_ADD2			2132 //自動予約登録の条件追加
#define CMD2_EPG_SRV_CHG_AUTO_ADD2			2134 //自動予約登録の条件変更
#define CMD2_EPG_SRV_ENUM_MANU_ADD2			2141 //プログラム予約自動登録の条件一覧取得
#define CMD2_EPG_SRV_ADD_MANU_ADD2			2142 //プログラム予約自動登録の条件追加
#define CMD2_EPG_SRV_CHG_MANU_ADD2			2144 //プログラム予約自動登録の条件変更
#define CMD2_EPG_SRV_GET_STATUS_NOTIFY2		2200 //サーバーの情報変更通知を取得（ロングポーリング）

#define CMD2_EPG_SRV_GET_PG_ARC_MINMAX		1020 //過去番組情報の最小開始時間と最大開始時間を取得する
#define CMD2_EPG_SRV_ENUM_SERVICE			1021 //読み込まれたEPGデータのサービスの一覧取得
#define CMD2_EPG_SRV_ENUM_PG_INFO			1022 //サービス指定で番組情報一覧を取得する
#define CMD2_EPG_SRV_GET_PG_INFO			1023 //番組情報取得
#define CMD2_EPG_SRV_GET_PG_INFO_LIST		1024 //番組情報取得(指定IDリスト)
#define CMD2_EPG_SRV_SEARCH_PG				1025 //番組検索
#define CMD2_EPG_SRV_ENUM_PG_ALL			1026 //番組情報一覧取得
#define CMD2_EPG_SRV_GET_PG_INFO_MINMAX		1028 //番組情報の最小開始時間と最大開始時間を取得する
#define CMD2_EPG_SRV_ENUM_PG_INFO_EX		1029 //サービス指定と時間指定で番組情報一覧を取得する
#define CMD2_EPG_SRV_ENUM_PG_ARC			1030 //サービス指定と時間指定で過去番組情報一覧を取得する

#define CMD2_EPG_SRV_ENUM_AUTO_ADD			1031 //自動予約登録の条件一覧取得
#define CMD2_EPG_SRV_ADD_AUTO_ADD			1032 //自動予約登録の条件追加
#define CMD2_EPG_SRV_DEL_AUTO_ADD			1033 //自動予約登録の条件削除
#define CMD2_EPG_SRV_CHG_AUTO_ADD			1034 //自動予約登録の条件変更

#define CMD2_EPG_SRV_ENUM_MANU_ADD			1041 //プログラム予約自動登録の条件一覧取得
#define CMD2_EPG_SRV_ADD_MANU_ADD			1042 //プログラム予約自動登録の条件追加
#define CMD2_EPG_SRV_DEL_MANU_ADD			1043 //プログラム予約自動登録の条件削除
#define CMD2_EPG_SRV_CHG_MANU_ADD			1044 //プログラム予約自動登録の条件変更

#define CMD2_EPG_SRV_CHK_SUSPEND			1050 //スタンバイ、休止、シャットダウンを行っていいかの確認
#define CMD2_EPG_SRV_SUSPEND				1051 //スタンバイ、休止、シャットダウンに移行する（1:スタンバイ 2:休止 3:シャットダウン | 0x0100:復帰後再起動）
#define CMD2_EPG_SRV_REBOOT					1052 //PC再起動を行う
#define CMD2_EPG_SRV_EPG_CAP_NOW			1053 //10秒後にEPGデータの取得を行う

#define CMD2_EPG_SRV_FILE_COPY				1060 //指定ファイルを転送する
#define CMD2_EPG_SRV_FILE_COPY2				2060 //指定ファイルをまとめて転送する
#define CMD2_EPG_SRV_ENUM_PLUGIN			1061 //PlugInファイルの一覧を取得する（1:ReName、2:Write）
#define CMD2_EPG_SRV_GET_CHG_CH_TVTEST		1062 //TVTestのチャンネル切り替え用の情報を取得する
#define CMD2_EPG_SRV_PROFILE_UPDATE			1063 //設定ファイル(ini)の更新を通知させる
#define CMD2_EPG_SRV_GET_NOTIFY_LOG			1065 //保存された情報通知ログを取得する
#define CMD2_EPG_SRV_ENUM_TUNER_PROCESS		1066 //起動中のチューナーについてサーバーが把握している情報の一覧を取得する

#define CMD2_EPG_SRV_NWTV_SET_CH			1070 //NetworkTVモードのViewアプリのチャンネルを切り替え（ID=0のみ）
#define CMD2_EPG_SRV_NWTV_CLOSE				1071 //NetworkTVモードで起動中のViewアプリを終了（ID=0のみ）
#define CMD2_EPG_SRV_NWTV_MODE				1072 //NetworkTVモードで起動するときの送信モード（1:UDP 2:TCP 3:UDP+TCP）
#define CMD2_EPG_SRV_NWTV_ID_SET_CH			1073 //NetworkTVモードのViewアプリのチャンネルを切り替え、または起動の確認（ID指定）
#define CMD2_EPG_SRV_NWTV_ID_CLOSE			1074 //NetworkTVモードで起動中のViewアプリを終了（ID指定）

#define CMD2_EPG_SRV_NWPLAY_OPEN			1080 //ストリーム配信用ファイルを開く
#define CMD2_EPG_SRV_NWPLAY_CLOSE			1081 //ストリーム配信用ファイルを閉じる
#define CMD2_EPG_SRV_NWPLAY_PLAY			1082 //ストリーム配信開始
#define CMD2_EPG_SRV_NWPLAY_STOP			1083 //ストリーム配信停止
#define CMD2_EPG_SRV_NWPLAY_GET_POS			1084 //ストリーム配信で現在の送信位置と総ファイルサイズを取得する
#define CMD2_EPG_SRV_NWPLAY_SET_POS			1085 //ストリーム配信で送信位置をシークする
#define CMD2_EPG_SRV_NWPLAY_SET_IP			1086 //ストリーム配信で送信先を設定する
#define CMD2_EPG_SRV_NWPLAY_TF_OPEN			1087 //ストリーム配信用ファイルをタイムシフトモードで開く

//外部アプリ再生用
#define CMD2_EPG_SRV_GET_NETWORK_PATH		1299 //録画ファイルのネットワークパスを取得

//タイマーGUI（EpgTimerなど）用
#define CMD2_TIMER_GUI_SHOW_DLG				101 //ダイアログを前面に表示
#define CMD2_TIMER_GUI_UPDATE_RESERVE		102 //予約情報などが更新されたことを通知する（旧仕様との互換用）
#define CMD2_TIMER_GUI_UPDATE_EPGDATA		103 //EPGデータが更新されたことを通知する（旧仕様との互換用）
#define CMD2_TIMER_GUI_VIEW_EXECUTE			110 //Viewアプリ（EpgDataCap_Bonなど）を起動
#define CMD2_TIMER_GUI_QUERY_SUSPEND		120 //スタンバイ、休止、シャットダウンに入っていいかユーザーに確認する（了解ならタイマーGUIはCMD_EPG_SRV_SUSPENDを送る）
#define CMD2_TIMER_GUI_QUERY_REBOOT			121 //PC再起動に入っていいかユーザーに確認する（了解ならタイマーGUIはCMD_EPG_SRV_REBOOTを送る）
#define CMD2_TIMER_GUI_SRV_STATUS_CHG		130 //サーバーの動作状況の変更を通知する（旧仕様との互換用）（1:通常、2:EPGデータ取得開始、3:予約録画開始）

//バージョン情報追加対応版
#define CMD2_TIMER_GUI_SRV_STATUS_NOTIFY2	1130 //サーバーの情報変更を通知する

//Viewアプリ（EpgDataCap_Bonなど）用
#define CMD2_VIEW_APP_SET_BONDRIVER			201 //BonDriverの切り替え
#define CMD2_VIEW_APP_GET_BONDRIVER			202 //使用中のBonDriverのファイル名を取得
#define CMD2_VIEW_APP_SET_CH				205 //チャンネル切り替え（SpaceとCh or OriginalNetworkID、TSID、ServieID）
#define CMD2_VIEW_APP_GET_DELAY				206 //放送波の時間とPC時間の誤差取得
#define CMD2_VIEW_APP_GET_STATUS			207 //現在の状態を取得
#define CMD2_VIEW_APP_CLOSE					208 //アプリケーションの終了
#define CMD2_VIEW_APP_SET_ID				1201 //識別用IDの設定
#define CMD2_VIEW_APP_GET_ID				1202 //識別用IDの取得
#define CMD2_VIEW_APP_SET_STANDBY_REC		1203 //予約録画用にGUIキープ
#define CMD2_VIEW_APP_EXEC_VIEW_APP			1204 //Viewボタン登録アプリの起動
#define CMD2_VIEW_APP_GET_STATUS_DETAILS	1205 //現在の状態を詳細に取得
#define CMD2_VIEW_APP_CREATE_CTRL			1221 //ストリーム制御用コントロール作成
#define CMD2_VIEW_APP_DELETE_CTRL			1222 //ストリーム制御用コントロール削除
#define CMD2_VIEW_APP_SET_CTRLMODE			1223 //コントロールの動作を設定（対象サービス、スクランブル、処理対象データ）
#define CMD2_VIEW_APP_REC_START_CTRL		1224 //録画処理開始
#define CMD2_VIEW_APP_REC_STOP_CTRL			1225 //録画処理停止
#define CMD2_VIEW_APP_REC_FILE_PATH			1226 //録画ファイルパスを取得
#define CMD2_VIEW_APP_REC_STOP_ALL			1227 //即時録画を停止
#define CMD2_VIEW_APP_REC_WRITE_SIZE		1228 //ファイル出力したサイズを取得
#define CMD2_VIEW_APP_EPGCAP_START			1241 //EPG取得開始
#define CMD2_VIEW_APP_EPGCAP_STOP			1242 //EPG取得停止
#define CMD2_VIEW_APP_SEARCH_EVENT			1251 //番組情報の検索
#define CMD2_VIEW_APP_GET_EVENT_PF			1252 //現在or次の番組情報を取得する
//TVTest連携のストリーミング配信専用
#define CMD2_VIEW_APP_TT_SET_CTRL			1261 //ストリーミング配信制御IDの設定


//旧バージョン互換コマンド
#define CMD_EPG_SRV_GET_RESERVE_INFO	12 //予約情報取得
#define CMD_EPG_SRV_ADD_RESERVE			13 //予約追加
#define CMD_EPG_SRV_DEL_RESERVE			14 //予約削除
#define CMD_EPG_SRV_CHG_RESERVE			15 //予約変更
#define CMD_EPG_SRV_SEARCH_PG_FIRST		21 //番組検索（先頭）
#define CMD_EPG_SRV_SEARCH_PG_NEXT		22 //番組検索の続き
#define CMD_EPG_SRV_ADD_AUTO_ADD		32 //自動予約登録の条件追加
#define CMD_EPG_SRV_DEL_AUTO_ADD		33 //自動予約登録の条件削除
#define CMD_EPG_SRV_CHG_AUTO_ADD		34 //自動予約登録の条件変更

#define OLD_CMD_SUCCESS			0 //成功
#define OLD_CMD_ERR				1 //汎用エラー
#define OLD_CMD_NEXT			2 //Enumコマンド用、続きあり

#endif
