// stdafx.h : 標準のシステム インクルード ファイルのインクルード ファイル、または
// 参照回数が多く、かつあまり変更されない、プロジェクト専用のインクルード ファイル
// を記述します。
//

#pragma once

#ifdef _WIN32
#include "targetver.h"
#endif

void AddDebugLogNoNewline(const wchar_t* lpOutputString, bool suppressDebugOutput = false);
void SetSaveDebugLog(bool saveDebugLog);

#define WRAP_DEBUG_OUTPUT
#include "../../Common/Common.h"
