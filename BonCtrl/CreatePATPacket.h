#pragma once

#include "../Common/EpgTimerUtil.h"
#include "../Common/ErrDef.h"

class CCreatePATPacket
{
public:
	CCreatePATPacket(void);

	//作成PATのパラメータを設定
	//引数：
	// TSID				[IN]TransportStreamID
	// PIDList			[IN]PMTのPIDとServiceIDのリスト
	void SetParam(
		WORD TSID_,
		const vector<pair<WORD, WORD>>& PIDList_
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
	vector<pair<WORD, WORD>> PIDList;

	vector<BYTE> packet;

	vector<BYTE> PSI;
};
