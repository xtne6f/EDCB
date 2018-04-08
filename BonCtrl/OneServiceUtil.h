#pragma once

#include "../Common/ErrDef.h"
#include "../Common/TSPacketUtil.h"
#include "../Common/StringUtil.h"

#include "BonCtrlDef.h"
#include "SendUDP.h"
#include "SendTCP.h"
#include "WriteTSFile.h"
#include "PMTUtil.h"
#include "CATUtil.h"
#include "CreatePMTPacket.h"
#include "CreatePATPacket.h"
#include "DropCount.h"
#include <functional>

class COneServiceUtil
{
public:
	COneServiceUtil(BOOL sendUdpTcp_);
	~COneServiceUtil(void);

	//処理対象ServiceIDを設定
	//引数：
	// SID			[IN]ServiceID。0xFFFFで全サービス対象。
	void SetSID(
		WORD SID_
	);

	//設定されてる処理対象のServiceIDを取得
	//戻り値：
	// ServiceID
	WORD GetSID();

	//UDPで送信を行う
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// sendList		[IN/OUT]送信先リスト。NULLで停止。Portは実際に送信に使用したPortが返る。
	BOOL SendUdp(
		vector<NW_SEND_INFO>* sendList
		);

	//TCPで送信を行う
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// sendList		[IN/OUT]送信先リスト。NULLで停止。Portは実際に送信に使用したPortが返る。
	BOOL SendTcp(
		vector<NW_SEND_INFO>* sendList
		);

	//出力用TSデータを送る
	//引数：
	// data		[IN]TSデータ
	// size		[IN]dataのサイズ
	// funcGetPresent	[IN]EPGの現在番組IDを調べる関数
	void AddTSBuff(
		BYTE* data,
		DWORD size,
		const std::function<int(WORD, WORD, WORD)>& funcGetPresent
		);

	void SetPmtPID(
		WORD TSID,
		WORD pmtPID_
		);

	void SetEmmPID(
		const vector<WORD>& pidList
		);

	//ファイル保存を開始する
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
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
		const wstring& fileName,
		BOOL overWriteFlag,
		BOOL pittariFlag,
		WORD pittariONID_,
		WORD pittariTSID_,
		WORD pittariSID_,
		WORD pittariEventID_,
		ULONGLONG createSize,
		const vector<REC_FILE_SET_INFO>& saveFolder,
		const vector<wstring>& saveFolderSub,
		int maxBuffCount
	);

	//ファイル保存を終了する
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	BOOL EndSave();

	//録画中かどうか
	//戻り値：
	// TRUE（録画中）、FALSE（していない）
	BOOL IsRec();

	//スクランブル解除処理の動作設定
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// enable		[IN] TRUE（処理する）、FALSE（処理しない）
	void SetScramble(
		BOOL enable
		);

	//スクランブル解除処理を行うかどうか
	//戻り値：
	// TRUE（処理する）、FALSE（処理しない）
	BOOL GetScramble(
		);

	//字幕とデータ放送含めるかどうか
	//引数：
	// enableCaption		[IN]字幕を TRUE（含める）、FALSE（含めない）
	// enableData			[IN]データ放送を TRUE（含める）、FALSE（含めない）
	void SetServiceMode(
		BOOL enableCaption,
		BOOL enableData
		);

	//エラーカウントをクリアする
	void ClearErrCount();

	//ドロップとスクランブルのカウントを取得する
	//引数：
	// drop				[OUT]ドロップ数
	// scramble			[OUT]スクランブル数
	void GetErrCount(ULONGLONG* drop, ULONGLONG* scramble);


	//録画中のファイルのファイルパスを取得する
	//引数：
	// filePath			[OUT]保存ファイル名
	// subRecFlag		[OUT]サブ録画が発生したかどうか
	void GetSaveFilePath(
		wstring* filePath,
		BOOL* subRecFlag
		);

	//ドロップとスクランブルのカウントを保存する
	//引数：
	// filePath			[IN]保存ファイル名
	void SaveErrCount(
		const wstring& filePath
		);

	void SetSignalLevel(
		float signalLv
		);

	//録画中のファイルの出力サイズを取得する
	//引数：
	// writeSize			[OUT]出力サイズ
	void GetRecWriteSize(
		__int64* writeSize
		);

	void SetBonDriver(
		const wstring& bonDriver
		);
	void SetPIDName(
		WORD pid,
		LPCSTR name
		);
	void SetNoLogScramble(
		BOOL noLog
		);
protected:
	BOOL sendUdpTcp;
	WORD SID;

	BOOL enableScramble;

	vector<HANDLE> udpPortMutex;
	vector<HANDLE> tcpPortMutex;

	std::unique_ptr<CSendUDP> sendUdp;
	std::unique_ptr<CSendTCP> sendTcp;
	std::unique_ptr<CWriteTSFile> writeFile;

	vector<BYTE> buff;

	CCreatePATPacket createPat;
	CCreatePMTPacket createPmt;

	WORD pmtPID;
	vector<WORD> emmPIDList;

	CDropCount dropCount;

	BOOL pittariStart;
	BOOL pittariEndChk;

	WORD lastPMTVer;

	wstring pittariFileName;
	BOOL pittariOverWriteFlag;
	ULONGLONG pittariCreateSize;
	vector<REC_FILE_SET_INFO> pittariSaveFolder;
	vector<wstring> pittariSaveFolderSub;
	WORD pittariONID;
	WORD pittariTSID;
	WORD pittariSID;
	WORD pittariEventID;
	wstring pittariRecFilePath;
	int pittariMaxBuffCount;

protected:
	void StratPittariRec();
	void StopPittariRec();
};

