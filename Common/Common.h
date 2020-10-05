#pragma once

// すべてのプロジェクトに適用される追加ヘッダおよび定義

// wprintf関数系を規格準拠にする(VC14以降)。ワイド文字列には%sでなく%lsなどを使うこと
#define _CRT_STDIO_ISO_WIDE_SPECIFIERS
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <string>
#include <utility>
#include <map>
#include <vector>
#include <memory>
#include <algorithm>
#include <wchar.h>
#include <stdarg.h>
#include <limits.h>
#include <string.h>

using std::min;
using std::max;
using std::string;
using std::wstring;
using std::pair;
using std::map;
using std::multimap;
using std::vector;

template<class RndIt, class T>
RndIt lower_bound_first(RndIt first, RndIt last, const T& key)
{
	while( last != first ){
		RndIt it = first + (last - first) / 2;
		if( it->first < key ) first = ++it; else last = it;
	}
	return first;
}

template<class RndIt, class T>
RndIt upper_bound_first(RndIt first, RndIt last, const T& key)
{
	while( last != first ){
		RndIt it = first + (last - first) / 2;
		if( !(key < it->first) ) first = ++it; else last = it;
	}
	return first;
}

#ifdef __cpp_lib_nonmember_container_access
#define array_size std::size
#else
#define array_size _countof
#endif

#ifdef __clang__
#pragma clang diagnostic ignored "-Wlogical-op-parentheses"
#pragma clang diagnostic ignored "-Wunused-parameter"
#endif

#ifdef _MSC_VER
// 'identifier': unreferenced formal parameter
#pragma warning(disable : 4100)
#if _MSC_VER < 1900
// 'class': assignment operator was implicitly defined as deleted
#pragma warning(disable : 4512)
#endif
#endif

// 適切でないNULLの検出用
//#undef NULL
//#define NULL nullptr

#ifdef _MSC_VER
#include <sal.h>
#define PRINTF_FORMAT_SZ _In_z_ _Printf_format_string_
#else
#define PRINTF_FORMAT_SZ
#endif

#ifdef _WIN32
#define UTIL_NEWLINE L"\r\n"
#else
#define UTIL_NEWLINE L"\n"
#endif

#ifndef WRAP_DEBUG_OUTPUT
inline void AddDebugLogNoNewline(const WCHAR* s)
{
	OutputDebugString(s);
}
#endif

inline void AddDebugLogFormatNoNewline(PRINTF_FORMAT_SZ const WCHAR* format, ...)
{
	va_list params;
	va_start(params, format);
	// 長すぎる等エラー時は書式文字列の展開を省略する
	WCHAR buff[1024];
#ifdef _WIN32
	const WCHAR* p = _vsnwprintf_s(buff, 1024, _TRUNCATE, format, params) < 0 ? format : buff;
#else
	const WCHAR* p = vswprintf(buff, 1024, format, params) < 0 ? format : buff;
#endif
	va_end(params);
	AddDebugLogNoNewline(p);
}

#define AddDebugLog(s) AddDebugLogNoNewline(s UTIL_NEWLINE)
#define AddDebugLogFormat(format, ...) AddDebugLogFormatNoNewline(format UTIL_NEWLINE, __VA_ARGS__)

// 適切でない書式文字列の検出用
//#define AddDebugLogFormatNoNewline(...) (void)wprintf_s(__VA_ARGS__)
