#pragma once

#include "../../Common/ParseTextInstances.h"

#include "TunerBankCtrl.h"

class CTunerManager
{
public:
	//チューナー一覧の読み込みを行う
	void ReloadTuner();

	//チューナー予約制御を取得する
	//引数：
	// ctrlMap			[OUT]チューナー予約制御の一覧
	// notifyManager	[IN]CTunerBankCtrlに渡す引数
	// epgDBManager		[IN]CTunerBankCtrlに渡す引数
	void GetEnumTunerBank(
		map<DWORD, std::unique_ptr<CTunerBankCtrl>>* ctrlMap,
		CNotifyManager& notifyManager,
		CEpgDBManager& epgDBManager
		) const;

	//指定サービスをサポートするチューナー一覧を取得する
	//戻り値：
	// チューナーのID一覧
	//引数：
	// ONID				[IN]確認したいサービスのONID
	// TSID				[IN]確認したいサービスのTSID
	// SID				[IN]確認したいサービスのSID
	vector<DWORD> GetSupportServiceTuner(
		WORD ONID,
		WORD TSID,
		WORD SID
		) const;

	bool GetCh(
		DWORD tunerID,
		WORD ONID,
		WORD TSID,
		WORD SID,
		DWORD* space,
		DWORD* ch
		) const;

	//ドライバ毎のチューナー一覧とEPG取得に使用できるチューナー数のペアを取得する
	vector<pair<vector<DWORD>, WORD>> GetEnumEpgCapTuner(
		) const;

	bool IsSupportService(
		DWORD tunerID,
		WORD ONID,
		WORD TSID,
		WORD SID
		) const;

	bool GetBonFileName(
		DWORD tunerID,
		wstring& bonFileName
		) const;

protected:
	struct TUNER_INFO {
		wstring bonFileName;
		WORD epgCapMaxOfThisBon;
		vector<CH_DATA4> chList;
	};

	map<DWORD, TUNER_INFO> tunerMap; //キー bonID<<16 | tunerID
};

