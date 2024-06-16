#pragma once

// すべてのプロジェクトに適用される追加ヘッダおよび定義

#ifdef _WIN32
// wprintf関数系を規格準拠にする(VC14以降)。ワイド文字列には%sでなく%lsなどを使うこと
#define _CRT_STDIO_ISO_WIDE_SPECIFIERS
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#define my_fseek _fseeki64
#define my_ftell _ftelli64
#else
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif
#define my_fseek fseeko
#define my_ftell ftello
#endif
#include <string>
#include <utility>
#include <map>
#include <vector>
#include <memory>
#include <algorithm>
#include <wchar.h>
#include <stdarg.h>
#include <stddef.h>
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

#ifndef _WIN32
static_assert(sizeof(wchar_t) == 2 || sizeof(wchar_t) == 4);
static_assert(sizeof(short) == 2);
static_assert(sizeof(int) == 4);
static_assert(sizeof(long) == 4 || sizeof(long) == 8);
static_assert(sizeof(long long) == 8);
static_assert(sizeof(size_t) == 4 || sizeof(size_t) == 8);
static_assert(sizeof(size_t) == sizeof(ptrdiff_t));
static_assert(sizeof(size_t) == sizeof(void*));

typedef const char* LPCSTR;
typedef wchar_t WCHAR;
typedef WCHAR* LPWSTR;
typedef const WCHAR* LPCWSTR;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef long long LONGLONG;
typedef unsigned long long ULONGLONG;
typedef int BOOL;
typedef ptrdiff_t INT_PTR;
typedef int SOCKET;

struct SYSTEMTIME {
	WORD wYear;
	WORD wMonth;
	WORD wDayOfWeek;
	WORD wDay;
	WORD wHour;
	WORD wMinute;
	WORD wSecond;
	WORD wMilliseconds;
};

#define FALSE 0
#define TRUE 1
#define MAXDWORD 0xFFFFFFFF
#define CALLBACK
#define WINAPI
#define INVALID_SOCKET (-1)
#define closesocket(sock) close(sock)

// 境界チェックインタフェース

#include <exception>

#define _TRUNCATE ((size_t)-1)

inline void wcscpy_s(wchar_t* dest, size_t size, const wchar_t* src)
{
	if( !dest || !src || wcslen(src) >= size ) std::terminate();
	wcscpy(dest, src);
}

template<size_t size>
void wcscpy_s(wchar_t(&dest)[size], const wchar_t* src)
{
	wcscpy_s(dest, size, src);
}

inline void wcsncpy_s(wchar_t* dest, size_t size, const wchar_t* src, size_t count)
{
	if( !dest || !size || !src ) std::terminate();
	size_t n = 0;
	for( size_t i = (count == _TRUNCATE ? size - 1 : count); i && n < size && src[n]; i--, n++ );
	if( n >= size ) std::terminate();
	wcsncpy(dest, src, n);
	dest[n] = 0;
}

template<size_t size>
void wcsncpy_s(wchar_t(&dest)[size], const wchar_t* src, size_t count)
{
	wcsncpy_s(dest, size, src, count);
}

inline int sprintf_s(char* dest, size_t size, const char* format, ...)
{
	if( !dest || !size || !format ) std::terminate();
	va_list params;
	va_start(params, format);
	int n = vsnprintf(dest, size, format, params);
	va_end(params);
	if( n < 0 || (size_t)n >= size ) std::terminate();
	return n;
}

template<size_t size>
int sprintf_s(char(&dest)[size], const char* format, ...)
{
	if( !format ) std::terminate();
	va_list params;
	va_start(params, format);
	int n = vsnprintf(dest, size, format, params);
	va_end(params);
	if( n < 0 || (size_t)n >= size ) std::terminate();
	return n;
}

inline int swprintf_s(wchar_t* dest, size_t size, const wchar_t* format, ...)
{
	if( !dest || !size || !format ) std::terminate();
	va_list params;
	va_start(params, format);
	int n = vswprintf(dest, size, format, params);
	va_end(params);
	if( n < 0 ) std::terminate();
	return n;
}

template<size_t size>
int swprintf_s(wchar_t(&dest)[size], const wchar_t* format, ...)
{
	if( !format ) std::terminate();
	va_list params;
	va_start(params, format);
	int n = vswprintf(dest, size, format, params);
	va_end(params);
	if( n < 0 ) std::terminate();
	return n;
}
#endif

struct fclose_deleter
{
	void operator()(FILE* fp) { fclose(fp); }
};

inline size_t codepoint_to_utf8(int x, char* dest)
{
	if( x < 0 || x >= 0x110000 ){
		x = 0xFFFD;
	}
	size_t n = 0;
	if( x < 0x80 ){
		dest[n++] = (char)x;
	}else if( x < 0x800 ){
		dest[n++] = (char)(0xC0 | x >> 6);
		dest[n++] = (char)(0x80 | (x & 0x3F));
	}else if( x < 0x10000 ){
		dest[n++] = (char)(0xE0 | x >> 12);
		dest[n++] = (char)(0x80 | (x >> 6 & 0x3F));
		dest[n++] = (char)(0x80 | (x & 0x3F));
	}else{
		dest[n++] = (char)(0xF0 | x >> 18);
		dest[n++] = (char)(0x80 | (x >> 12 & 0x3F));
		dest[n++] = (char)(0x80 | (x >> 6 & 0x3F));
		dest[n++] = (char)(0x80 | (x & 0x3F));
	}
	return n;
}

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
#endif

#ifdef _MSC_VER
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
#ifdef _WIN32
	OutputDebugString(s);
#elif WCHAR_MAX > 0xFFFF && 0
	for( size_t i = 0; s[i]; i++ ){
		char dest[4];
		fwrite(dest, 1, codepoint_to_utf8(s[i], dest), stderr);
	}
#else
	(void)s;
#endif
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
