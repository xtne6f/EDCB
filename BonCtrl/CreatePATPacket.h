#pragma once

#include "../Common/EpgTimerUtil.h"
#include "../Common/ErrDef.h"

class CCreatePATPacket
{
public:
	typedef struct _PROGRAM_PID_INFO{
		WORD SID;
		WORD PMTPID;
	}PROGRAM_PID_INFO;
public:
	CCreatePATPacket(void);

	//作成PATのパラメータを設定
	//引数：
	// TSID				[IN]TransportStreamID
	// PIDMap			[IN]PMTのリスト（キーPMTのPID）
	void SetParam(
		WORD TSID_,
		map<WORD, PROGRAM_PID_INFO>* PIDMap_
	);

	//作成PATのバッファポインタを取得
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// buff				[OUT]作成したPATパケットへのポインタ（次回呼び出し時まで有効）
	// buffSize			[OUT]buffのサイズ
	// incrementFlag	[IN]TSパケットのCounterをインクリメントするかどうか（TRUE:する、FALSE：しない）
	BOOL GetPacket(
		BYTE** buff,
		DWORD* buffSize,
		BOOL incrementFlag = TRUE
	);

	//作成PATのバッファをクリア
	void Clear();

protected:
	void CreatePAT();
	void CreatePacket();
	void IncrementCounter();

protected:
	BYTE version;
	BYTE counter;

	WORD TSID;
	map<WORD, PROGRAM_PID_INFO> PIDMap; //キーPMTのPID

	vector<BYTE> packet;

	vector<BYTE> PSI;
};
