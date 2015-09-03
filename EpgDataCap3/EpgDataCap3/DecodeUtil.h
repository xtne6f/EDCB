#pragma once

#include "EpgDBUtil.h"
#include "../../Common/TSPacketUtil.h"
#include "../../Common/TSBuffUtil.h"
#include "../../Common/Util.h"
#include "../../Common/EpgDataCap3Def.h"

#include "./Table/TableUtil.h"

class CDecodeUtil
{
public:
	CDecodeUtil(void);
	~CDecodeUtil(void);

	void SetEpgDB(CEpgDBUtil* epgDBUtil);
	void AddTSData(BYTE* data);

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
		SERVICE_INFO** serviceList
		);

	//ストリーム内の現在の時間情報を取得する
	//引数：
	// time				[OUT]ストリーム内の現在の時間
	// tick				[OUT]timeを取得した時点のチックカウント
	BOOL GetNowTime(
		FILETIME* time,
		DWORD* tick = NULL
		);

protected:
	typedef struct _NIT_SECTION_INFO{
		WORD network_id;
		BYTE version_number;
		BYTE last_section_number;
		map<BYTE, CNITTable*> nitSection;
		~_NIT_SECTION_INFO(void){
			map<BYTE, CNITTable*>::iterator itr;
			for( itr=nitSection.begin(); itr != nitSection.end(); itr++ ){
				SAFE_DELETE(itr->second);
			}
			nitSection.clear();
		};
	}NIT_SECTION_INFO;
	typedef struct _SDT_SECTION_INFO{
		WORD original_network_id;
		WORD transport_stream_id;
		BYTE version_number;
		BYTE last_section_number;
		map<BYTE, CSDTTable*> sdtSection;
		~_SDT_SECTION_INFO(void){
			map<BYTE, CSDTTable*>::iterator itr;
			for( itr=sdtSection.begin(); itr != sdtSection.end(); itr++ ){
				SAFE_DELETE(itr->second);
			}
			sdtSection.clear();
		};
	}SDT_SECTION_INFO;

	CEpgDBUtil* epgDBUtil;

	//PID毎のバッファリング
	//キー PID
	map<WORD, CTSBuffUtil> buffUtilMap;

	CPATTable* patInfo;
	NIT_SECTION_INFO* nitActualInfo;
	SDT_SECTION_INFO* sdtActualInfo;
	CBITTable* bitInfo;
	CSITTable* sitInfo;
	FILETIME totTime;
	FILETIME tdtTime;
	FILETIME sitTime;
	DWORD totTimeTick;
	DWORD tdtTimeTick;
	DWORD sitTimeTick;


	SERVICE_INFO* serviceList;

protected:
	void Clear();
	void ClearBuff(WORD noClearPid);
	void ChangeTSIDClear(WORD noClearPid);

	BOOL CheckPAT(WORD PID, CPATTable* pat);
	BOOL CheckNIT(WORD PID, CNITTable* nit);
	BOOL CheckSDT(WORD PID, CSDTTable* sdt);
	BOOL CheckTOT(WORD PID, CTOTTable* tot);
	BOOL CheckTDT(WORD PID, CTDTTable* tdt);
	BOOL CheckEIT(WORD PID, CEITTable* eit);
	BOOL CheckBIT(WORD PID, CBITTable* bit);
	BOOL CheckSIT(WORD PID, CSITTable* sit);

	//自ストリームのサービス一覧をSITから取得する
	//引数：
	// serviceListSize			[OUT]serviceListの個数
	// serviceList				[OUT]サービス情報のリスト（DLL内で自動的にdeleteする。次に取得を行うまで有効）
	BOOL GetServiceListSIT(
		DWORD* serviceListSize,
		SERVICE_INFO** serviceList
		);

};
