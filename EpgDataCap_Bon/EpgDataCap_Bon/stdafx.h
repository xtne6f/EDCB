
// stdafx.h : 標準のシステム インクルード ファイルのインクルード ファイル、または
// 参照回数が多く、かつあまり変更されない、プロジェクト専用のインクルード ファイル
// を記述します。

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Windows ヘッダーから使用されていない部分を除外します。
#define NOMINMAX
// Windows ヘッダー ファイル:
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shellapi.h>
#include <commdlg.h>
#include <shlobj.h>
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "ws2_32.lib")

// C ランタイム ヘッダー ファイル
#include <stdio.h>

#define afx_msg

#if defined(_UNICODE) && defined(OutputDebugString)
#undef OutputDebugString
#define OutputDebugString OutputDebugStringWrapper
// OutputDebugStringWのラッパー関数
// APIフックによる高度なものでなく単なる置換。OutputDebugStringAやDLLからの呼び出しはラップされない
void OutputDebugStringWrapper(LPCWSTR lpOutputString);
#endif
void SetSaveDebugLog(bool saveDebugLog);

#include "../../Common/Common.h"
