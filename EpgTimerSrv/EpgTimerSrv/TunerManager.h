#pragma once

#include "../../Common/Util.h"
#include "../../Common/EpgTimerUtil.h"
#include "../../Common/PathUtil.h"
#include "../../Common/StringUtil.h"
#include "../../Common/ParseTextInstances.h"

#include "TunerBankCtrl.h"

class CTunerManager
{
public:
	CTunerManager(void);
	~CTunerManager(void);

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
	BOOL GetEnumTunerBank(
		map<DWORD, CTunerBankCtrl*>* ctrlMap
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
	typedef struct _TUNER_INFO{
		wstring bonFileName;
		WORD epgCapMaxOfThisBon;
		vector<CH_DATA4> chList;
		wstring chSet4FilePath;
	}TUNER_INFO;

	map<DWORD, TUNER_INFO> tunerMap; //キー bonID<<16 | tunerID
protected:
	static BOOL FindBonFileName(wstring src, wstring& dllName);
};

