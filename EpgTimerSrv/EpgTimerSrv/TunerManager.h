#pragma once

#include "../../Common/ParseTextInstances.h"

#include "TunerBankCtrl.h"

class CNotifyManager;
class CEpgDBManager;

class CTunerManager
{
public:
	//チューナー一覧の読み込みを行う
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	BOOL ReloadTuner();

	//チューナーのID一覧を取得する。
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// idList			[OUT]チューナーのID一覧
	BOOL GetEnumID(
		vector<DWORD>* idList
		) const;

	//チューナー予約制御を取得する
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// ctrlMap			[OUT]チューナー予約制御の一覧
	// notifyManager	[IN]CTunerBankCtrlに渡す引数
	// epgDBManager		[IN]CTunerBankCtrlに渡す引数
	BOOL GetEnumTunerBank(
		map<DWORD, std::unique_ptr<CTunerBankCtrl>>* ctrlMap,
		CNotifyManager& notifyManager,
		CEpgDBManager& epgDBManager
		) const;

	//指定サービスをサポートしていないチューナー一覧を取得する
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// ONID				[IN]確認したいサービスのONID
	// TSID				[IN]確認したいサービスのTSID
	// SID				[IN]確認したいサービスのSID
	// idList			[OUT]チューナーのID一覧
	BOOL GetNotSupportServiceTuner(
		WORD ONID,
		WORD TSID,
		WORD SID,
		vector<DWORD>* idList
		) const;

	BOOL GetSupportServiceTuner(
		WORD ONID,
		WORD TSID,
		WORD SID,
		vector<DWORD>* idList
		) const;

	BOOL GetCh(
		DWORD tunerID,
		WORD ONID,
		WORD TSID,
		WORD SID,
		DWORD* space,
		DWORD* ch
		) const;

	//ドライバ毎のチューナー一覧とEPG取得に使用できるチューナー数のペアを取得する
	BOOL GetEnumEpgCapTuner(
		vector<pair<vector<DWORD>, WORD>>* idList
		) const;

	BOOL IsSupportService(
		DWORD tunerID,
		WORD ONID,
		WORD TSID,
		WORD SID
		) const;

	BOOL GetBonFileName(
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

