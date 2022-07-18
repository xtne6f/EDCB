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
	const bool createPmtFlag;
	WORD transportStreamID;
	bool allServicesFlag;
	bool catOrPmtUpdated;
	vector<WORD> serviceIDList;
	CCATUtil catUtil;
	vector<pair<WORD, CPMTUtil>> pmtUtilMap; //キーPMTのPID
	vector<WORD> needPIDList;
	CCreatePATPacket pat;
	CCreatePMTPacket pmt;

	void CheckNeedPID();
};
