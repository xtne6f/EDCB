#pragma once

#include "BATTable.h"
#include "CATTable.h"
#include "EITTable.h"
#include "NITTable.h"
#include "PATTable.h"
#include "PMTTable.h"
#include "SDTTable.h"
#include "TDTTable.h"
#include "TOTTable.h"
#include "CDTTable.h"
#include "SDTTTable.h"
#include "BITTable.h"
#include "SITTable.h"
#include "EITTable_SD.h"
#include "EITTable_SD2.h"

class CTableUtil
{
public:
	enum t_type{
		TYPE_NONE = 0,
		TYPE_BAT,
		TYPE_CAT,
		TYPE_EIT,
		TYPE_NIT,
		TYPE_PAT,
		TYPE_PMT,
		TYPE_SDT,
		TYPE_TDT,
		TYPE_TOT,
		TYPE_CDT,
		TYPE_SDTT,
		TYPE_BIT,
		TYPE_SIT,
		TYPE_MAX = TYPE_SIT,
	};
	t_type Decode(BYTE* data, DWORD dataSize, CPSITable** table);
	void Putback(t_type type, CPSITable* table) { cache[type].reset(table); }
private:
	std::unique_ptr<CPSITable> cache[TYPE_MAX + 1];
};
