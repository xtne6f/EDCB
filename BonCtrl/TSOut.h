#pragma once

#include "../Common/StructDef.h"
#include "../Common/PathUtil.h"
#include "../Common/StringUtil.h"
#include "../Common/EpgDataCap3Util.h"
#include "../Common/TSPacketUtil.h"
#include "../Common/ThreadUtil.h"

#include "BonCtrlDef.h"
#include "ScrambleDecoderUtil.h"
#include "ServiceFilter.h"
#include "OneServiceUtil.h"
#include <functional>

class CTSOut
{
public:
	struct CHECK_LOGO_RESULT {
		bool dataUpdated;
		bool serviceListUpdated;
		WORD onid;
		WORD id;
		BYTE type;
		vector<BYTE> data;
		vector<WORD> serviceList;
	};

	CTSOut(void);
	~CTSOut(void);

	//チャンネル変更状態に遷移する
	void SetChChangeEvent(WORD presumedONID = 0xFFFF, bool resetEpgUtil = false);

	//GetStreamID()が失敗する状態かどうか
	bool IsChUnknown(DWORD* elapsedTime = nullptr);

	//現在のストリームのIDを取得する
	//戻り値：
	// true（成功）、false（失敗）
	//引数：
	// ONID		[OUT]originalNetworkID
	// TSID		[OUT]transportStreamID
	bool GetStreamID(
		WORD* ONID,
		WORD* TSID
		);

	void AddTSBuff(BYTE* data, DWORD dataSize);

	//ロゴの更新を調べる
	//引数：
	// logoTypeFlags	[IN]フラグ(ロゴタイプnを取得するとき下位からnビット目をセットする)
	// result			[OUT]結果
	void CheckLogo(
		DWORD logoTypeFlags,
		CHECK_LOGO_RESULT& result
		);

	//EMM処理の動作設定
	//戻り値：
	// true（成功）、false（失敗）
	//引数：
	// enable		[IN] true（処理する）、false（処理しない）
	bool SetEmm(
		bool enable
		);

	//EMM処理を行った数
	//戻り値：
	// 処理数
	DWORD GetEmmCount();

	//DLLのロード状態を取得
	//戻り値：
	// true（ロードに成功している）、false（ロードに失敗している）
	//引数：
	// loadErrDll		[OUT]ロードに失敗したDLLファイル名
	bool GetLoadStatus(
		wstring& loadErrDll
		);

	//EPGデータの保存を開始する
	//戻り値：
	// true（成功）、false（失敗）
	bool StartSaveEPG(
		const wstring& epgFilePath_
		);

	//EPGデータの保存を終了する
	//引数：
	// copy			[IN]tmpからコピー処理行うかどうか
	void StopSaveEPG(
		bool copy
		);

	//EPGデータの蓄積状態を取得する
	//戻り値：
	// ステータス
	//引数：
	// l_eitFlag		[IN]L-EITのステータスを取得
	EPG_SECTION_STATUS GetSectionStatus(
		bool l_eitFlag
		);

	//指定サービスのEPGデータの蓄積状態を取得する
	//戻り値：
	// ステータス,取得できたかどうか
	//引数：
	// originalNetworkID		[IN]取得対象のOriginalNetworkID
	// transportStreamID		[IN]取得対象のTransportStreamID
	// serviceID				[IN]取得対象のServiceID
	// l_eitFlag				[IN]L-EITのステータスを取得
	pair<EPG_SECTION_STATUS, bool> GetSectionStatusService(
		WORD originalNetworkID,
		WORD transportStreamID,
		WORD serviceID,
		bool l_eitFlag
		);

	//自ストリームのサービス一覧を取得する
	//戻り値：
	// true（成功）、false（失敗）
	//引数：
	// funcGetList		[IN]戻り値がtrueのときサービス情報の個数とそのリストを引数として呼び出される関数
	bool GetServiceListActual(
		const std::function<void(DWORD, SERVICE_INFO*)>& funcGetList
		);

	//TSストリーム制御用コントロールを作成する
	//戻り値：
	// 制御識別ID
	//引数：
	// sendUdpTcp	[IN]UDP/TCP送信用にする
	DWORD CreateServiceCtrl(
		bool sendUdpTcp
		);

	//TSストリーム制御用コントロールを削除する
	//戻り値：
	// true（成功）、false（失敗）
	//引数：
	// id			[IN]制御識別ID
	bool DeleteServiceCtrl(
		DWORD id
		);

	//制御対象のサービスを設定する
	//戻り値：
	// true（成功）、false（失敗）
	//引数：
	// id			[IN]制御識別ID
	// serviceID	[IN]対象サービスID、0xFFFFで全サービス対象
	bool SetServiceID(
		DWORD id,
		WORD serviceID
		);

	bool GetServiceID(
		DWORD id,
		WORD* serviceID
		);

	//UDPで送信を行う
	//戻り値：
	// true（成功）、false（失敗）
	//引数：
	// id			[IN]制御識別ID
	// sendList		[IN/OUT]送信先リスト。nullptrで停止。送信に使用したポート(失敗のものは0x10000)がportにセットされる。
	bool SendUdp(
		DWORD id,
		vector<NW_SEND_INFO>* sendList
		);

	//TCPで送信を行う
	//戻り値：
	// true（成功）、false（失敗）
	//引数：
	// id			[IN]制御識別ID
	// sendList		[IN/OUT]送信先リスト。nullptrで停止。送信に使用したポート(失敗のものは0x10000)がportにセットされる。
	bool SendTcp(
		DWORD id,
		vector<NW_SEND_INFO>* sendList
		);

	//ファイル保存を開始する
	//戻り値：
	// true（成功）、false（失敗）
	//引数：
	// recParam				[IN]保存パラメータ
	// saveFolderSub		[IN]HDDの空きがなくなった場合に一時的に使用するフォルダ
	// maxBuffCount			[IN]出力バッファ上限
	bool StartSave(
		const SET_CTRL_REC_PARAM& recParam,
		const vector<wstring>& saveFolderSub,
		int maxBuffCount
	);

	//ファイル保存を終了する
	//戻り値：
	// true（成功）、false（失敗）
	//引数：
	// id			[IN]制御識別ID
	// subRecFlag	[OUT]成功のとき、サブ録画が発生したかどうか
	bool EndSave(
		DWORD id,
		bool* subRecFlag = nullptr
		);

	//スクランブル解除処理の動作設定
	//戻り値：
	// true（成功）、false（失敗）
	//引数：
	// enable		[IN] true（処理する）、false（処理しない）
	bool SetScramble(
		DWORD id,
		bool enable
		);

	//字幕とデータ放送含めるかどうか
	//引数：
	// id					[IN]制御識別ID
	// enableCaption		[IN]字幕を true（含める）、false（含めない）
	// enableData			[IN]データ放送を true（含める）、false（含めない）
	void SetServiceMode(
		DWORD id,
		bool enableCaption,
		bool enableData
		);

	//エラーカウントをクリアする
	//引数：
	// id				[IN]制御識別ID
	void ClearErrCount(
		DWORD id
		);

	//ドロップとスクランブルのカウントを取得する
	//引数：
	// id				[IN]制御識別ID
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
	// writeSize			[OUT]出力サイズ
	void GetRecWriteSize(
		DWORD id,
		LONGLONG* writeSize
		);

	//指定サービスの現在or次のEPG情報を取得する
	//戻り値：
	// true（成功）、false（失敗）
	//引数：
	// originalNetworkID		[IN]取得対象のoriginalNetworkID
	// transportStreamID		[IN]取得対象のtransportStreamID
	// serviceID				[IN]取得対象のServiceID
	// nextFlag					[IN]true（次の番組）、false（現在の番組）
	// epgInfo					[OUT]EPG情報
	bool GetEpgInfo(
		WORD originalNetworkID,
		WORD transportStreamID,
		WORD serviceID,
		bool nextFlag,
		EPGDB_EVENT_INFO* epgInfo
		);
	
	//指定イベントのEPG情報を取得する
	//戻り値：
	// true（成功）、false（失敗）
	//引数：
	// originalNetworkID		[IN]取得対象のoriginalNetworkID
	// transportStreamID		[IN]取得対象のtransportStreamID
	// serviceID				[IN]取得対象のServiceID
	// eventID					[IN]取得対象のEventID
	// pfOnlyFlag				[IN]p/fからのみ検索するかどうか
	// epgInfo					[OUT]EPG情報
	bool SearchEpgInfo(
		WORD originalNetworkID,
		WORD transportStreamID,
		WORD serviceID,
		WORD eventID,
		bool pfOnlyFlag,
		EPGDB_EVENT_INFO* epgInfo
		);

	//PC時計を元としたストリーム時間との差を取得する
	//戻り値：
	// 差の秒数
	int GetTimeDelay(
		);
	
	//録画中かどうか
	//戻り値：
	// true（録画中）、false（していない）
	bool IsRec();

	//録画中のファイルのファイルパスを取得する
	//戻り値：
	// ファイルパス
	//引数：
	// id					[IN]制御識別ID
	wstring GetSaveFilePath(
		DWORD id
		);

	//ドロップとスクランブルのカウントを保存する
	//引数：
	// id					[IN]制御識別ID
	// filePath				[IN]保存ファイル名
	// asUtf8				[IN]UTF-8で保存するか
	// dropSaveThresh		[IN]ドロップ数がこれ以上なら保存する
	// drop					[OUT]ドロップ数
	void SaveErrCount(
		DWORD id,
		const wstring& filePath,
		bool asUtf8,
		int dropSaveThresh,
		int scrambleSaveThresh,
		ULONGLONG& drop,
		ULONGLONG& scramble
		);

	void SetSignalLevel(
		float signalLv
		);

	void SetBonDriver(
		const wstring& bonDriver
		);

	void SetNoLogScramble(
		bool noLog
		);

	void SetParseEpgPostProcess(
		bool parsePost
		);
protected:
	//objLock->epgUtilLockの順にロックする
	recursive_mutex_ objLock;
	recursive_mutex_ epgUtilLock;

	CEpgDataCap3Util epgUtil;
	CScrambleDecoderUtil decodeUtil;

	enum { CH_ST_INIT, CH_ST_WAIT_PAT, CH_ST_WAIT_PAT2, CH_ST_WAIT_ID, CH_ST_DONE } chChangeState;
	DWORD chChangeTime;
	WORD chChangePresumedONID;
	WORD lastONID;
	WORD lastTSID;

	vector<BYTE> decodeBuff;

	bool enableDecodeFlag;
	bool emmEnableFlag;

	map<DWORD, std::unique_ptr<COneServiceUtil>> serviceUtilMap; //キー識別ID
	CServiceFilter serviceFilter;

	DWORD nextCtrlID;

	std::unique_ptr<FILE, fclose_deleter> epgFile;
	enum { EPG_FILE_ST_NONE, EPG_FILE_ST_PAT, EPG_FILE_ST_TOT, EPG_FILE_ST_ALL } epgFileState;
	LONGLONG epgFileTotPos;
	wstring epgFilePath;
	wstring epgTempFilePath;
	vector<pair<LONGLONG, DWORD>> logoServiceListSizeMap;
	const WORD* logoAdditionalNeededPids;

	wstring bonFile;
	bool noLogScramble;
	bool parseEpgPostProcess;
protected:
	void ParseEpgPacket(BYTE* data, const CTSPacketUtil& packet);

	void UpdateFilterServiceID();

	static bool EnumLogoListProc(DWORD logoListSize, const LOGO_INFO* logoList, vector<pair<LONGLONG, DWORD>>& serviceListSizeMap, CHECK_LOGO_RESULT& result);

	DWORD GetNextID();

	bool UpdateEnableDecodeFlag();

	void OnChChanged(WORD onid, WORD tsid);
};

