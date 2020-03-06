#pragma once

#include "../Common/ErrDef.h"
#include "../Common/EpgTimerUtil.h"
#include "../Common/TSPacketUtil.h"
#include "../Common/TSBuffUtil.h"

class CCreatePMTPacket
{
public:
	//次のTSパケット入れないと解析できない
	static const DWORD ERR_NEED_NEXT_PACKET = 20;
	//バージョンの変更ないため解析不要
	static const DWORD ERR_NO_CHAGE = 30;

	CCreatePMTPacket(void);

	//PMT作成時のモード
	//引数：
	// needCaption			[IN]字幕データを含めるかどうか（TRUE:含める、FALSE：含めない）
	// needData				[IN]データカルーセルを含めるかどうか（TRUE:含める、FALSE：含めない）
	void SetCreateMode(
		BOOL needCaption_,
		BOOL needData_
	);

	//作成元となるPMTのパケットを入力
	//戻り値：
	// エラーコード
	//引数：
	// packet			//[IN] PMTのパケット
	DWORD AddData(
		CTSPacketUtil* packet
	);

	//必要なPIDかを確認
	//戻り値：
	// TRUE（必要）、FALSE（不必要）
	//引数：
	// PID				//[IN]確認するPID
	BOOL IsNeedPID(
		WORD PID
	);

	//作成PMTのバッファポインタを取得
	//戻り値：
	// 作成PMTのバッファポインタ
	//引数：
	// buff					[OUT]作成したPMTパケットへのポインタ（次回呼び出し時まで有効）
	// size					[OUT]buffのサイズ
	// incrementFlag		[IN]TSパケットのCounterをインクリメントするかどうか（TRUE:する、FALSE：しない）
	BOOL GetPacket(
		BYTE** buff,
		DWORD* size,
		BOOL incrementFlag = TRUE
	);

	//内部情報をクリア
	void Clear();

	BYTE GetVersion();

protected:
	DWORD DecodePMT(BYTE* data, DWORD dataSize);

	void CreatePMT();
	void CreatePacket();
	void IncrementCounter();
protected:
	CTSBuffUtil buffUtil;

	BOOL needCaption;
	BOOL needData;

	WORD lastPmtPID;
	WORD lastPcrPID;
	WORD lastPgNumber;
	BYTE lastVersion;

	vector<BYTE> firstDescBuff;

	struct SECOND_DESC_BUFF {
		BYTE stream_type;
		WORD elementary_PID;
		vector<BYTE> descBuff;
		WORD quality;
		WORD qualityPID;
	};
	vector<SECOND_DESC_BUFF> secondDescBuff;

	vector<WORD> emmPIDList;

	vector<WORD> needPIDList;

	vector<BYTE> createPSI;
	
	vector<BYTE> createPacket;

	BYTE createVer;
	BYTE createCounter;
};
