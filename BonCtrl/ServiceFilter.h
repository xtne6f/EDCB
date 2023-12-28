#pragma once

#include "CATUtil.h"
#include "CreatePATPacket.h"
#include "CreatePMTPacket.h"
#include "PMTUtil.h"

class CServiceFilter
{
public:
	CServiceFilter(bool createPmt = false);
	void SetServiceID(bool allServices, const vector<WORD>& sidList);
	void SetPmtCreateMode(bool enableCaption, bool enableData);
	void Clear(WORD tsid);
	void FilterPacket(vector<BYTE>& outData, const BYTE* data, const CTSPacketUtil& packet);
	bool CatOrPmtUpdated() const { return this->catOrPmtUpdated; }
	const CCATUtil& CatUtil() const { return this->catUtil; }
	const vector<pair<WORD, CPMTUtil>>& PmtUtilMap() const { return this->pmtUtilMap; }
private:
	//このメンバは不変だがオブジェクトを暗黙にコピー可能とするためconstにはしない
	bool createPmtFlag;
	WORD transportStreamID;
	bool allServicesFlag;
	bool catOrPmtUpdated;
	vector<WORD> serviceIDList;
	CCATUtil catUtil;
	vector<pair<WORD, CPMTUtil>> pmtUtilMap; //キーPMTのPID
	CCreatePATPacket pat;
	CCreatePMTPacket pmt;

	//PIDごとに出力の要不要、連続性指標を調整したりアダプテーションを置くための情報
	struct PID_INFO {
		WORD first; //PID;
		bool neededByCatOrPmt;
		bool onceOutputted;
		bool originalOutputted;
		bool originalDropped;
		BYTE patLastCounter;
		BYTE patShiftCounter;
	};
	vector<PID_INFO> pidInfoMap;

	static PID_INFO& InsertPIDInfo(vector<PID_INFO>& infoMap, WORD pid);
	void CheckNeedPID();
};
