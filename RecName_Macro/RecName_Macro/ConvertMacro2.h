#pragma once

#include "RecName_PlugIn.h"

#include <string>
using namespace std;

class CConvertMacro2
{
public:
	CConvertMacro2(void);
	~CConvertMacro2(void);

	BOOL Convert(wstring macro, PLUGIN_RESERVE_INFO* info, EPG_EVENT_INFO* epgInfo, wstring& convert);
};

