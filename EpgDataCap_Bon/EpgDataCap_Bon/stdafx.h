
// stdafx.h : 標準のシステム インクルード ファイルのインクルード ファイル、または
// 参照回数が多く、かつあまり変更されない、プロジェクト専用のインクルード ファイル
// を記述します。

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Windows ヘッダーから使用されていない部分を除外します。
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
#include <tchar.h>
#include <string>

#define afx_msg









#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif


#if defined(_UNICODE) && defined(OutputDebugString)
#undef OutputDebugString
#define OutputDebugString OutputDebugStringWrapper
// OutputDebugStringWのラッパー関数
// APIフックによる高度なものでなく単なる置換。OutputDebugStringAやDLLからの呼び出しはラップされない
void OutputDebugStringWrapper(LPCWSTR lpOutputString);
#endif

#include "../../Common/Common.h"

// TODO: この警告は可能なら解決すべき
// declaration of 'identifier' hides class member
#pragma warning(disable : 4458)
