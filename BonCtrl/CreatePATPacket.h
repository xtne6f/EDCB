#pragma once

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
	BOOL GetPacket(
		BYTE** buff,
		DWORD* buffSize
	);

	//内部情報をクリア
	void Clear();

protected:
	void CreatePAT();
protected:
	BYTE version;
	BYTE counter;

	WORD TSID;
	vector<pair<WORD, WORD>> PIDList;

	vector<BYTE> packet;

	vector<BYTE> PSI;
};
