#pragma once

#include "EpgDBUtil.h"
#include "../../Common/TSPacketUtil.h"
#include "../../Common/TSBuffUtil.h"
#include "../../Common/EpgDataCap3Def.h"

class CDecodeUtil
{
public:
	CDecodeUtil(void);

	void SetEpgDB(CEpgDBUtil* epgDBUtil_);
	void AddTSData(BYTE* data, DWORD size);

	//解析データの現在のストリームＩＤを取得する
	//引数：
	// originalNetworkID		[OUT]現在のoriginalNetworkID
	// transportStreamID		[OUT]現在のtransportStreamID
	BOOL GetTSID(
		WORD* originalNetworkID,
		WORD* transportStreamID
		);

	//自ストリームのサービス一覧を取得する
	//引数：
	// serviceListSize			[OUT]serviceListの個数
	// serviceList				[OUT]サービス情報のリスト（DLL内で自動的にdeleteする。次に取得を行うまで有効）
	BOOL GetServiceListActual(
		DWORD* serviceListSize,
		SERVICE_INFO** serviceList_
		);

	//ストリーム内の現在の時間情報を取得する
	//引数：
	// time				[OUT]ストリーム内の現在の時間
	// tick				[OUT]timeを取得した時点のチックカウント
	BOOL GetNowTime(
		__int64* time,
		DWORD* tick = NULL
		);

protected:
	CEpgDBUtil* epgDBUtil;

	AribDescriptor::CDescriptor tableBuff;

	//PID毎のバッファリング
	//キー PID
	map<WORD, CTSBuffUtil> buffUtilMap;

	std::unique_ptr<const AribDescriptor::CDescriptor> patInfo;
	map<BYTE, AribDescriptor::CDescriptor> nitActualInfo;
	map<BYTE, AribDescriptor::CDescriptor> sdtActualInfo;
	std::unique_ptr<const AribDescriptor::CDescriptor> bitInfo;
	std::unique_ptr<const AribDescriptor::CDescriptor> sitInfo;
	__int64 totTime;
	__int64 tdtTime;
	__int64 sitTime;
	DWORD totTimeTick;
	DWORD tdtTimeTick;
	DWORD sitTimeTick;


	std::unique_ptr<SERVICE_INFO[]> serviceList;
	std::unique_ptr<EPGDB_SERVICE_INFO[]> serviceDBList;
	std::unique_ptr<CServiceInfoAdapter[]> serviceAdapterList;

protected:
	void Clear();
	void ClearBuff(WORD noClearPid);
	void ChangeTSIDClear(WORD noClearPid);

	void CheckPAT(WORD PID, const AribDescriptor::CDescriptor& pat);
	void CheckNIT(WORD PID, const AribDescriptor::CDescriptor& nit);
	void CheckSDT(WORD PID, const AribDescriptor::CDescriptor& sdt);
	void CheckTDT(const AribDescriptor::CDescriptor& tdt);
	void CheckEIT(WORD PID, const AribDescriptor::CDescriptor& eit);
	void CheckBIT(WORD PID, const AribDescriptor::CDescriptor& bit);
	void CheckSIT(const AribDescriptor::CDescriptor& sit);

	//自ストリームのサービス一覧をSITから取得する
	//引数：
	// serviceListSize			[OUT]serviceListの個数
	// serviceList				[OUT]サービス情報のリスト（DLL内で自動的にdeleteする。次に取得を行うまで有効）
	BOOL GetServiceListSIT(
		DWORD* serviceListSize,
		SERVICE_INFO** serviceList_
		);

};
