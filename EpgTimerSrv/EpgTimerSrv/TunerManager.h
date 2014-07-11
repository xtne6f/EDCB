#pragma once

#include "../../Common/Util.h"
#include "../../Common/EpgTimerUtil.h"
#include "../../Common/PathUtil.h"
#include "../../Common/StringUtil.h"
#include "../../Common/ParseTextInstances.h"

#include "TunerCtrl.h"
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
		);

	//チューナー予約制御を取得する
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// ctrlMap			[OUT]チューナー予約制御の一覧
	BOOL GetEnumTunerBank(
		map<DWORD, CTunerBankCtrl*>* ctrlMap
		);

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
		);

	BOOL GetSupportServiceTuner(
		WORD ONID,
		WORD TSID,
		WORD SID,
		vector<DWORD>* idList
		);

	BOOL GetCh(
		DWORD tunerID,
		WORD ONID,
		WORD TSID,
		WORD SID,
		DWORD* space,
		DWORD* ch
		);

	BOOL GetEnumEpgCapTuner(
		vector<DWORD>* idList
		);

	BOOL IsSupportService(
		DWORD tunerID,
		WORD ONID,
		WORD TSID,
		WORD SID
		);

	BOOL GetBonFileName(
		DWORD tunerID,
		wstring& bonFileName
		);

protected:
	typedef struct _TUNER_INFO{
		WORD bonID;
		WORD tunerID;
		wstring bonFileName;
		BOOL epgCapFlag;
		CParseChText4 chUtil;
		wstring chSet4FilePath;
	}TUNER_INFO;

	map<DWORD, TUNER_INFO*> tunerMap; //キー bonID<<16 | tunerID
protected:
	BOOL FindBonFileName(wstring src, wstring& dllName);
};

