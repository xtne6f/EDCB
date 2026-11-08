#pragma once

class CCreatePMTPacket
{
public:
	CCreatePMTPacket(void);

	//PMT作成時のモード
	//引数：
	// needCaption			[IN]字幕データを含めるかどうか（true:含める、false:含めない）
	// needData				[IN]データカルーセルを含めるかどうか（true:含める、false:含めない）
	void SetCreateMode(
		bool needCaption_,
		bool needData_
	);

	//作成元となるPMTセクションを設定
	//戻り値：
	// エラーコード
	//引数：
	// data				[IN]PMTセクションデータ
	// size				[IN]dataのサイズ
	void SetSectionData(
		const BYTE* data,
		DWORD dataSize
	);

	//ECMのPIDかどうか
	bool IsEcmPID(
		WORD pid
	);

	//作成PMTのエレメンタリストリームに含まれるPIDかどうか
	bool IsElementaryPID(
		WORD pid
	);

	//作成PMTのバッファポインタを取得
	//戻り値：
	// true（成功）、false（失敗）
	//引数：
	// buff					[OUT]作成したPMTパケットへのポインタ（次回呼び出し時まで有効）
	// size					[OUT]buffのサイズ
	// pid					[IN]PID
	bool GetPacket(
		BYTE** buff,
		DWORD* size,
		WORD pid
	);

	//内部情報をクリア
	void Clear();

protected:
	void CreatePMT();
protected:
	bool needCaption;
	bool needData;
	vector<BYTE> lastSection;

	struct SECOND_DESC_BUFF {
		BYTE stream_type;
		WORD elementary_PID;
		DWORD descBuffPos;
		WORD descBuffSize;
		WORD quality;
		WORD qualityPID;
	};

	vector<WORD> ecmPIDList;

	vector<WORD> elementaryPIDList;

	vector<BYTE> createPSI;
	
	vector<BYTE> createPacket;

	BYTE createVer;
	BYTE createCounter;
};
