#pragma once

#include "../../Common/EpgDataCap3Def.h"

class CConvertMacro2
{
public:
	static wstring Convert(const wstring& macro, const PLUGIN_RESERVE_INFO* info);
private:
	static BOOL ExpandMacro(wstring var, const PLUGIN_RESERVE_INFO* info, wstring& convert);
};

