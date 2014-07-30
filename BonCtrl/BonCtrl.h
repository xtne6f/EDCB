#pragma once

#include <windows.h>

#include "../Common/Util.h"
#include "../Common/StructDef.h"
#include "../Common/EpgTimerUtil.h"
#include "../Common/StringUtil.h"

#include "BonDriverUtil.h"
#include "PacketInit.h"
#include "TSOut.h"
#include "ChSetUtil.h"

class CBonCtrl
{
public:
	CBonCtrl(void);
	~CBonCtrl(void);

	//BonDriverフォルダを指定
	//引数：
	// bonDriverFolderPath		[IN]BonDriverフォルダパス
	void SetBonDriverFolder(
		LPCWSTR bonDriverFolderPath
		);

	void SetEMMMode(BOOL enable);

	void SetTsBuffMaxCount(DWORD tsBuffMaxCount, int writeBuffMaxCount);

	//BonDriverフォルダのBonDriver_*.dllを列挙
	//戻り値：
	// エラーコード
	//引数：
	// bonList			[OUT]検索できたBonDriver一覧
	DWORD EnumBonDriver(
		vector<wstring>* bonList
		);

	//BonDriverをロードしてチャンネル情報などを取得（ファイル名で指定）
	//戻り値：
	// エラーコード
	//引数：
	// bonDriverFile	[IN]EnumBonDriverで取得されたBonDriverのファイル名
	DWORD OpenBonDriver(
		LPCWSTR bonDriverFile,
		int openWait = 200
		);

	//ロードしているBonDriverの開放
	//戻り値：
	// エラーコード
	DWORD CloseBonDriver();

	//ロード中のBonDriverのファイル名を取得する（ロード成功しているかの判定）
	//戻り値：
	// TRUE（成功）：FALSE（Openに失敗している）
	//引数：
	// bonDriverFile		[OUT]BonDriverのファイル名(NULL可)
	BOOL GetOpenBonDriver(
		wstring* bonDriverFile
		);

	//ロードしたBonDriverの情報取得
	//SpaceとChの一覧を取得する
	//戻り値：
	// エラーコード
	//引数：
	// spaceMap			[OUT] SpaceとChの一覧（mapのキー Space）
	DWORD GetOriginalChList(
		map<DWORD, BON_SPACE_INFO>* spaceMap
	);

	//BonDriverのチューナー名を取得
	//戻り値：
	// チューナー名
	wstring GetTunerName();

	//チャンネル変更
	//戻り値：
	// エラーコード
	//引数：
	// space			[IN]変更チャンネルのSpace
	// ch				[IN]変更チャンネルの物理Ch
	DWORD SetCh(
		DWORD space,
		DWORD ch
		);

	//チャンネル変更
	//戻り値：
	// エラーコード
	//引数：
	// space			[IN]変更チャンネルのSpace
	// ch				[IN]変更チャンネルの物理Ch
	// SID			[IN]変更チャンネルの物理service_id
	DWORD SetCh(
		DWORD space,
		DWORD ch,
		WORD SID
		);

	//チャンネル変更
	//戻り値：
	// エラーコード
	//引数：
	// ONID			[IN]変更チャンネルのorignal_network_id
	// TSID			[IN]変更チャンネルの物理transport_stream_id
	// SID			[IN]変更チャンネルの物理service_id
	DWORD SetCh(
		WORD ONID,
		WORD TSID,
		WORD SID
		);

	//チャンネル変更中かどうか
	//戻り値：
	// TRUE（変更中）、FALSE（完了）
	BOOL IsChChanging(BOOL* chChgErr);

	//現在のストリームのIDを取得する
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// ONID		[OUT]originalNetworkID
	// TSID		[OUT]transportStreamID
	BOOL GetStreamID(
		WORD* ONID,
		WORD* TSID
		);

	//シグナルレベルの取得
	//戻り値：
	// シグナルレベル
	float GetSignalLevel();

	//EPGデータの蓄積状態をリセットする
	void ClearSectionStatus();

	//EPGデータの蓄積状態を取得する
	//戻り値：
	// ステータス
	//引数：
	// l_eitFlag		[IN]L-EITのステータスを取得
	EPG_SECTION_STATUS GetSectionStatus(
		BOOL l_eitFlag
		);

	//自ストリームのサービス一覧を取得する
	//戻り値：
	// エラーコード
	//引数：
	// serviceList				[OUT]サービス情報のリスト
	DWORD GetServiceListActual(
		vector<TS_SERVICE_INFO>* serviceList
		);

	//サービス一覧を取得する
	//戻り値：
	// エラーコード
	//引数：
	// serviceList				[OUT]サービス情報のリスト
	DWORD GetServiceList(
		vector<CH_DATA4>* serviceList
		);

	//TSストリーム制御用コントロールを作成する
	//戻り値：
	// エラーコード
	//引数：
	// id			[OUT]制御識別ID
	BOOL CreateServiceCtrl(
		DWORD* id
		);

	//TSストリーム制御用コントロールを作成する
	//戻り値：
	// エラーコード
	//引数：
	// id			[IN]制御識別ID
	BOOL DeleteServiceCtrl(
		DWORD id
		);

	//制御対象のサービスを設定する
	//戻り値：
	// TRUE（成功）、FALSE（失敗
	//引数：
	// id			[IN]制御識別ID
	// serviceID	[IN]対象サービスID、0xFFFFで全サービス対象
	BOOL SetServiceID(
		DWORD id,
		WORD serviceID
		);

	BOOL GetServiceID(
		DWORD id,
		WORD* serviceID
		);

	//UDPで送信を行う
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// id			[IN]制御識別ID
	// sendList		[IN/OUT]送信先リスト。NULLで停止。Portは実際に送信に使用したPortが返る。
	BOOL SendUdp(
		DWORD id,
		vector<NW_SEND_INFO>* sendList
		);

	//TCPで送信を行う
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// id			[IN]制御識別ID
	// sendList		[IN/OUT]送信先リスト。NULLで停止。Portは実際に送信に使用したPortが返る。
	BOOL SendTcp(
		DWORD id,
		vector<NW_SEND_INFO>* sendList
		);

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
	BOOL StartSave(
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
	);

	//ファイル保存を終了する
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// id			[IN]制御識別ID
	BOOL EndSave(
		DWORD id
		);

	//スクランブル解除処理の動作設定
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// id			[IN]制御識別ID
	// enable		[IN] TRUE（処理する）、FALSE（処理しない）
	BOOL SetScramble(
		DWORD id,
		BOOL enable
		);

	//字幕とデータ放送含めるかどうか
	//引数：
	// id					[IN]制御識別ID
	// enableCaption		[IN]字幕を TRUE（含める）、FALSE（含めない）
	// enableData			[IN]データ放送を TRUE（含める）、FALSE（含めない）
	void SetServiceMode(
		DWORD id,
		BOOL enableCaption,
		BOOL enableData
		);

	//エラーカウントをクリアする
	//引数：
	// id					[IN]制御識別ID
	void ClearErrCount(
		DWORD id
		);

	//ドロップとスクランブルのカウントを取得する
	//引数：
	// id					[IN]制御識別ID
	// drop					[OUT]ドロップ数
	// scramble				[OUT]スクランブル数
	void GetErrCount(
		DWORD id,
		ULONGLONG* drop,
		ULONGLONG* scramble
		);

	//録画中のファイルのファイルパスを取得する
	//引数：
	// id					[IN]制御識別ID
	// filePath				[OUT]保存ファイル名
	// subRecFlag			[OUT]サブ録画が発生したかどうか
	void GetSaveFilePath(
		DWORD id,
		wstring* filePath,
		BOOL* subRecFlag
		);

	//ドロップとスクランブルのカウントを保存する
	//引数：
	// id					[IN]制御識別ID
	// filePath				[IN]保存ファイル名
	void SaveErrCount(
		DWORD id,
		wstring filePath
		);

	//録画中のファイルの出力サイズを取得する
	//引数：
	// id					[IN]制御識別ID
	// writeSize			[OUT]保存ファイル名
	void GetRecWriteSize(
		DWORD id,
		__int64* writeSize
		);

	//指定サービスの現在or次のEPG情報を取得する
	//戻り値：
	// エラーコード
	//引数：
	// originalNetworkID		[IN]取得対象のoriginalNetworkID
	// transportStreamID		[IN]取得対象のtransportStreamID
	// serviceID				[IN]取得対象のServiceID
	// nextFlag					[IN]TRUE（次の番組）、FALSE（現在の番組）
	// epgInfo					[OUT]EPG情報（DLL内で自動的にdeleteする。次に取得を行うまで有効）
	DWORD GetEpgInfo(
		WORD originalNetworkID,
		WORD transportStreamID,
		WORD serviceID,
		BOOL nextFlag,
		EPGDB_EVENT_INFO* epgInfo
		);

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
	DWORD SearchEpgInfo(
		WORD originalNetworkID,
		WORD transportStreamID,
		WORD serviceID,
		WORD eventID,
		BYTE pfOnlyFlag,
		EPGDB_EVENT_INFO* epgInfo
		);
	
	//PC時計を元としたストリーム時間との差を取得する
	//戻り値：
	// 差の秒数
	int GetTimeDelay(
		);

	//録画中かどうかを取得する
	// TRUE（録画中）、FALSE（録画していない）
	BOOL IsRec();

	//チャンネルスキャンを開始する
	//戻り値：
	// エラーコード
	DWORD StartChScan();

	//チャンネルスキャンをキャンセルする
	//戻り値：
	// エラーコード
	DWORD StopChScan();

	//チャンネルスキャンの状態を取得する
	//戻り値：
	// エラーコード
	//引数：
	// space		[OUT]スキャン中の物理CHのspace
	// ch			[OUT]スキャン中の物理CHのch
	// chName		[OUT]スキャン中の物理CHの名前
	// chkNum		[OUT]チェック済みの数
	// totalNum		[OUT]チェック対象の総数
	DWORD GetChScanStatus(
		DWORD* space,
		DWORD* ch,
		wstring* chName,
		DWORD* chkNum,
		DWORD* totalNum
		);

	//EPG取得対象のサービス一覧を取得する
	//戻り値：
	// エラーコード
	//引数：
	// chList		[OUT]EPG取得するチャンネル一覧
	DWORD GetEpgCapService(
		vector<EPGCAP_SERVICE_INFO>* chList
		);


	//EPG取得を開始する
	//戻り値：
	// エラーコード
	//引数：
	// chList		[IN]EPG取得するチャンネル一覧
	// BSBasic		[IN]BSで１チャンネルから基本情報のみ取得するかどうか
	// CS1Basic		[IN]CS1で１チャンネルから基本情報のみ取得するかどうか
	// CS2Basic		[IN]CS2で１チャンネルから基本情報のみ取得するかどうか
	DWORD StartEpgCap(
		vector<EPGCAP_SERVICE_INFO>* chList
		);

	//EPG取得を停止する
	//戻り値：
	// エラーコード
	DWORD StopEpgCap(
		);

	//EPG取得のステータスを取得する
	//戻り値：
	// エラーコード
	//引数：
	// info			[OUT]取得中のサービス
	DWORD GetEpgCapStatus(
		EPGCAP_SERVICE_INFO* info
		);

	//バックグラウンドでのEPG取得設定
	//引数：
	// enableLive	[IN]視聴中に取得する
	// enableRec	[IN]録画中に取得する
	// enableRec	[IN]EPG取得するチャンネル一覧
	// BSBasic		[IN]BSで１チャンネルから基本情報のみ取得するかどうか
	// CS1Basic		[IN]CS1で１チャンネルから基本情報のみ取得するかどうか
	// CS2Basic		[IN]CS2で１チャンネルから基本情報のみ取得するかどうか
	// backStartWaitSec	[IN]Ch切り替え、録画開始後、バックグラウンドでのEPG取得を開始するまでの秒数
	void SetBackGroundEpgCap(
		BOOL enableLive,
		BOOL enableRec,
		BOOL BSBasic,
		BOOL CS1Basic,
		BOOL CS2Basic,
		DWORD backStartWaitSec
		);

	BOOL GetViewStatusInfo(
		DWORD id,
		float* signal,
		DWORD* space,
		DWORD* ch,
		ULONGLONG* drop,
		ULONGLONG* scramble
		);

protected:
	CBonDriverUtil bonUtil;
	CPacketInit packetInit;
	CTSOut tsOut;
	CChSetUtil chUtil;

	HANDLE lockEvent;

	CRITICAL_SECTION buffLock;
	vector<TS_DATA*> TSBuff;
	DWORD totalTSBuffSize;

	HANDLE recvThread;
	HANDLE recvStopEvent;
	HANDLE analyzeThread;
	HANDLE analyzeStopEvent;

	//チャンネルスキャン用
	HANDLE chScanThread;
	HANDLE chScanStopEvent;
	DWORD chSt_space;
	DWORD chSt_ch;
	wstring chSt_chName;
	DWORD chSt_chkNum;
	DWORD chSt_totalNum;
	DWORD chSt_err;
	typedef struct _CHK_CH_INFO{
		DWORD space;
		DWORD ch;
		wstring spaceName;
		wstring chName;
	}CHK_CH_INFO;

	//EPG取得用
	HANDLE epgCapThread;
	HANDLE epgCapStopEvent;
	vector<EPGCAP_SERVICE_INFO> epgCapChList;
	EPGCAP_SERVICE_INFO epgSt_ch;
	DWORD epgSt_err;

	HANDLE epgCapBackThread;
	HANDLE epgCapBackStopEvent;
	BOOL enableLiveEpgCap;
	BOOL enableRecEpgCap;
	WORD lastSID;

	BOOL epgCapBackBSBasic;
	BOOL epgCapBackCS1Basic;
	BOOL epgCapBackCS2Basic;
	DWORD epgCapBackStartWaitSec;
	DWORD tsBuffMaxCount;
	int writeBuffMaxCount;
protected:
	//PublicAPI排他制御用
	BOOL Lock(LPCWSTR log = NULL, DWORD timeOut = 15*1000);
	void UnLock(LPCWSTR log = NULL);

	//BonDriverをロード後の初期化処理
	//戻り値：
	// エラーコード
	DWORD _OpenBonDriver();

	//ロードしているBonDriverの開放本体
	//戻り値：
	// エラーコード
	DWORD _CloseBonDriver();

	DWORD _SetCh(
		DWORD space,
		DWORD ch,
		BOOL chScan = FALSE
		);

	static void GetEpgDataFilePath(WORD ONID, WORD TSID, wstring& epgDataFilePath, BOOL BSBasic, BOOL CS1Basic, BOOL CS2Basic);

	static UINT WINAPI RecvThread(LPVOID param);
	static UINT WINAPI AnalyzeThread(LPVOID param);

	static UINT WINAPI ChScanThread(LPVOID param);
	static UINT WINAPI EpgCapThread(LPVOID param);

	void StartBackgroundEpgCap();
	void StopBackgroundEpgCap();
	static UINT WINAPI EpgCapBackThread(LPVOID param);
};

