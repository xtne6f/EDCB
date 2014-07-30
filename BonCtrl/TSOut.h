#pragma once

#include <Windows.h>

#include "../Common/Util.h"
#include "../Common/StructDef.h"
#include "../Common/PathUtil.h"
#include "../Common/StringUtil.h"
#include "../Common/ErrDef.h"
#include "../Common/EpgDataCap3Util.h"
#include "../Common/TSPacketUtil.h"

#include "BonCtrlDef.h"
#include "ScrambleDecoderUtil.h"
#include "CreatePATPacket.h"
#include "OneServiceUtil.h"
#include "PMTUtil.h"
#include "CATUtil.h"

class CTSOut
{
public:
	CTSOut(void);
	~CTSOut(void);

	DWORD SetChChangeEvent(BOOL resetEpgUtil = FALSE);
	BOOL IsChChanging(BOOL* chChgErr);
	void ResetChChange();

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

	DWORD AddTSBuff(TS_DATA* data);

	//EMM処理の動作設定
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// enable		[IN] TRUE（処理する）、FALSE（処理しない）
	BOOL SetEmm(
		BOOL enable
		);

	//EMM処理を行った数
	//戻り値：
	// 処理数
	DWORD GetEmmCount();

	//DLLのロード状態を取得
	//戻り値：
	// TRUE（ロードに成功している）、FALSE（ロードに失敗している）
	//引数：
	// loadErrDll		[OUT]ロードに失敗したDLLファイル名
	BOOL GetLoadStatus(
		wstring& loadErrDll
		);

	//EPGデータの保存を開始する
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	BOOL StartSaveEPG(
		wstring epgFilePath
		);

	//EPGデータの保存を終了する
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// copy			[IN]tmpからコピー処理行うかどうか
	BOOL StopSaveEPG(
		BOOL copy
		);

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
	// serviceListSize			[OUT]serviceListの個数
	// serviceList				[OUT]サービス情報のリスト（DLL内で自動的にdeleteする。次に取得を行うまで有効）
	DWORD GetServiceListActual(
		DWORD* serviceListSize,
		SERVICE_INFO** serviceList
		);

	//TSストリーム制御用コントロールを作成する
	//戻り値：
	// TRUE（成功）、FALSE（失敗
	//引数：
	// id			[OUT]制御識別ID
	BOOL CreateServiceCtrl(
		DWORD* id
		);

	//TSストリーム制御用コントロールを作成する
	//戻り値：
	// TRUE（成功）、FALSE（失敗
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
		vector<wstring>* saveFolderSub,
		int maxBuffCount
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
	void ClearErrCount(
		DWORD id
		);

	//ドロップとスクランブルのカウントを取得する
	//引数：
	// drop				[OUT]ドロップ数
	// scramble			[OUT]スクランブル数
	void GetErrCount(
		DWORD id,
		ULONGLONG* drop,
		ULONGLONG* scramble
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
	
	//録画中かどうか
	//戻り値：
	// TRUE（録画中）、FALSE（していない）
	BOOL IsRec();

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

	void SetSignalLevel(
		float signalLv
		);

	void SetBonDriver(
		wstring bonDriver
		);

protected:
	HANDLE lockEvent;

	CEpgDataCap3Util epgUtil;
	CScrambleDecoderUtil decodeUtil;
	CCreatePATPacket patUtil;

	BOOL chChangeFlag;
	BOOL chChangeErr;
	DWORD chChangeTime;
	WORD lastONID;
	WORD lastTSID;

	vector<BYTE> decodeBuff;

	BOOL enableDecodeFlag;
	BOOL emmEnableFlag;
	BOOL serviceOnlyFlag;

	map<DWORD, COneServiceUtil*> serviceUtilMap; //キー識別ID
	map<WORD, CPMTUtil*> pmtUtilMap; //キーPMTのPID
	CCATUtil* catUtil;

	map<WORD,WORD> needPIDMap; //キーPID

	DWORD nextCtrlID;

	HANDLE epgFile;
	wstring epgFilePath;
	wstring epgTempFilePath;

	wstring bonFile;
protected:
	//PublicAPI排他制御用
	BOOL Lock(LPCWSTR log = NULL, DWORD timeOut = 30*1000);
	void UnLock(LPCWSTR log = NULL);

	void CheckNeedPID();
	BOOL IsNeedPID(CTSPacketUtil* packet);

	DWORD GetNextID();

	//EPGデータをコピーする
	BOOL ConvertEpgInfo(EPG_EVENT_INFO* src, EPGDB_EVENT_INFO* dest);

	void ResetErrCount();

	void OnChChanged(WORD onid, WORD tsid);
};

