// stdafx.h : 標準のシステム インクルード ファイルのインクルード ファイル、または
// 参照回数が多く、かつあまり変更されない、プロジェクト専用のインクルード ファイル
// を記述します。
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Windows ヘッダーから使用されていない部分を除外します。
#define NOMINMAX
// Windows ヘッダー ファイル:
#include <windows.h>
#include <commctrl.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "Ws2_32.lib")

// C ランタイム ヘッダー ファイル
#include <stdlib.h>
#include <stdio.h>

static inline FILE* secure_wfopen(const wchar_t* name, const wchar_t* mode)
{
	FILE* fp;
	return _wfopen_s(&fp, name, mode) == 0 ? fp : NULL;
}

static inline FILE* shared_wfopen(const wchar_t* name, const wchar_t* mode)
{
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#else
#pragma warning(push)
#pragma warning(disable : 4996)
#endif
	return _wfopen(name, mode);
#ifdef __clang__
#pragma clang diagnostic pop
#else
#pragma warning(pop)
#endif
}

// TODO: プログラムに必要な追加ヘッダーをここで参照してください。

#if defined(_UNICODE) && defined(OutputDebugString)
#undef OutputDebugString
#define OutputDebugString OutputDebugStringWrapper
// OutputDebugStringWのラッパー関数
// APIフックによる高度なものでなく単なる置換。OutputDebugStringAやDLLからの呼び出しはラップされない
void OutputDebugStringWrapper(LPCWSTR lpOutputString);
#endif
void SetSaveDebugLog(bool saveDebugLog);

#include "../../Common/Common.h"
