#pragma once

#include "../Common/TSPacketUtil.h"
#include "../Common/StringUtil.h"
#include "../Common/SendTSTCPDllUtil.h"
#include "../Common/PathUtil.h"

#include "BonCtrlDef.h"
#include "WriteTSFile.h"
#include "DropCount.h"
#include "ServiceFilter.h"
#include <functional>

class COneServiceUtil
{
public:
	COneServiceUtil(bool sendUdpTcp_);
	~COneServiceUtil(void);

	//UDP/TCP送信用かどうか（不変値）
	bool GetSendUdpTcp() {
		return this->sendUdpTcp;
	}

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
	//引数：
	// sendList		[IN/OUT]送信先リスト。nullptrで停止。送信に使用したポート(失敗のものは0x10000)がportにセットされる。
	void SendUdp(
		vector<NW_SEND_INFO>* sendList
		) {
		SendUdpTcp(sendList, false, this->sendUdp, this->udpPortMutex, MUTEX_UDP_PORT_NAME);
	}

	//TCPで送信を行う
	//引数：
	// sendList		[IN/OUT]送信先リスト。nullptrで停止。送信に使用したポート(失敗のものは0x10000)がportにセットされる。
	void SendTcp(
		vector<NW_SEND_INFO>* sendList
		) {
		SendUdpTcp(sendList, true, this->sendTcp, this->tcpPortMutex, MUTEX_TCP_PORT_NAME);
	}

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

	//出力状態をクリアする（チャンネル変更など）
	//引数：
	// tsid		[IN]TransportStreamID
	void Clear(
		WORD tsid
		);

	//ファイル保存を開始する
	//戻り値：
	// true（成功）、false（失敗）
	//引数：
	// recParam				[IN]保存パラメータ（ctrlIDフィールドは無視）
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
	// subRecFlag	[OUT]成功のとき、サブ録画が発生したかどうか
	bool EndSave(bool* subRecFlag = nullptr);

	//録画中かどうか
	//戻り値：
	// true（録画中）、false（していない）
	bool IsRec();

	//スクランブル解除処理の動作設定
	//引数：
	// enable		[IN] true（処理する）、false（処理しない）
	void SetScramble(
		bool enable
		) {
		this->enableScramble = enable;
	}

	//スクランブル解除処理を行うかどうか
	//戻り値：
	// 正（処理する）、0（処理しない）、負（未設定）
	int GetScramble() {
		return this->enableScramble;
	}

	//字幕とデータ放送含めるかどうか
	//引数：
	// enableCaption		[IN]字幕を true（含める）、false（含めない）
	// enableData			[IN]データ放送を true（含める）、false（含めない）
	void SetServiceMode(
		bool enableCaption,
		bool enableData
		);

	//エラーカウントをクリアする
	void ClearErrCount();

	//ドロップとスクランブルのカウントを取得する
	//引数：
	// drop				[OUT]ドロップ数
	// scramble			[OUT]スクランブル数
	void GetErrCount(ULONGLONG* drop, ULONGLONG* scramble);


	//録画中のファイルのファイルパスを取得する
	//戻り値：
	// ファイルパス
	wstring GetSaveFilePath();

	//ドロップとスクランブルのカウントを保存する
	//引数：
	// filePath			[IN]保存ファイル名
	// asUtf8			[IN]UTF-8で保存するか
	// dropSaveThresh	[IN]ドロップ数がこれ以上なら保存する
	// drop				[OUT]ドロップ数
	void SaveErrCount(
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

	//録画中のファイルの出力サイズを取得する
	//引数：
	// writeSize			[OUT]出力サイズ
	void GetRecWriteSize(
		LONGLONG* writeSize
		);

	void SetBonDriver(
		const wstring& bonDriver
		);

	void SetNoLogScramble(
		bool noLog
		);
protected:
	bool sendUdpTcp;
	WORD SID;

	int enableScramble;

	vector<util_unique_handle> udpPortMutex;
	vector<util_unique_handle> tcpPortMutex;

	CSendTSTCPDllUtil sendUdp;
	CSendTSTCPDllUtil sendTcp;
	CWriteTSFile writeFile;

	vector<BYTE> buff;
	CServiceFilter serviceFilter;
	CDropCount dropCount;

	WORD lastPMTVer;

	enum { PITTARI_NONE, PITTARI_START, PITTARI_END_CHK, PITTARI_END } pittariState;
	bool pittariSubRec;
	SET_CTRL_REC_PARAM pittariRecParam;
	vector<wstring> pittariSaveFolderSub;
	int pittariMaxBuffCount;

protected:
	static void SendUdpTcp(
		vector<NW_SEND_INFO>* sendList,
		bool tcpFlag,
		CSendTSTCPDllUtil& sendNW,
		vector<util_unique_handle>& portMutexList,
		LPCWSTR mutexName
		);
	void StratPittariRec();
	void StopPittariRec();
};

