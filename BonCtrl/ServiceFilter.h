#pragma once

#include "CATUtil.h"
#include "CreatePATPacket.h"
#include "PMTUtil.h"

class CServiceFilter
{
public:
	CServiceFilter();
	void SetServiceID(bool allServices, const vector<WORD>& sidList);
	void Clear(WORD tsid);
	void FilterPacket(vector<BYTE>& outData, const BYTE* data, CTSPacketUtil& packet);
	bool CatOrPmtUpdated() const { return this->catOrPmtUpdated; }
	const CCATUtil& CatUtil() const { return this->catUtil; }
	const map<WORD, CPMTUtil>& PmtUtilMap() const { return this->pmtUtilMap; }
private:
	WORD transportStreamID;
	bool allServicesFlag;
	bool catOrPmtUpdated;
	vector<WORD> serviceIDList;
	CCATUtil catUtil;
	map<WORD, CPMTUtil> pmtUtilMap; //キーPMTのPID
	vector<WORD> needPIDList;
	CCreatePATPacket pat;

	void CheckNeedPID();
};
