#pragma once

#include "RecName_PlugIn.h"

#include <string>
using std::wstring;

class CConvertMacro2
{
public:
	CConvertMacro2(void);
	~CConvertMacro2(void);

	BOOL Convert(wstring macro, PLUGIN_RESERVE_INFO* info, wstring& convert);
};

