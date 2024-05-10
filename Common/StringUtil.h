#ifndef INCLUDE_STRING_UTIL_H
#define INCLUDE_STRING_UTIL_H

enum UTIL_CONV_CODE {
	UTIL_CONV_DEFAULT,
	UTIL_CONV_ACP,
	UTIL_CONV_UTF8,
};

void Format(wstring& strBuff, PRINTF_FORMAT_SZ const WCHAR *format, ...);

void Replace(wstring& strBuff, const wstring& strOld, const wstring& strNew); //alias-safe

void WtoA(const wstring& strIn, string& strOut, UTIL_CONV_CODE code = UTIL_CONV_DEFAULT);
size_t WtoA(const WCHAR* in, size_t inLen, vector<char>& out, UTIL_CONV_CODE code = UTIL_CONV_DEFAULT);
inline void WtoUTF8(const wstring& strIn, string& strOut) { WtoA(strIn, strOut, UTIL_CONV_UTF8); }
inline size_t WtoUTF8(const WCHAR* in, size_t inLen, vector<char>& out) { return WtoA(in, inLen, out, UTIL_CONV_UTF8); }

void AtoW(const string& strIn, wstring& strOut, UTIL_CONV_CODE code = UTIL_CONV_DEFAULT);
size_t AtoW(const char* in, size_t inLen, vector<WCHAR>& out, UTIL_CONV_CODE code = UTIL_CONV_DEFAULT);
inline void UTF8toW(const string& strIn, wstring& strOut) { AtoW(strIn, strOut, UTIL_CONV_UTF8); }
inline size_t UTF8toW(const char* in, size_t inLen, vector<WCHAR>& out) { return AtoW(in, inLen, out, UTIL_CONV_UTF8); }

bool Separate(const wstring& strIn, const WCHAR* sep, wstring& strLeft, wstring& strRight); //alias-safe
inline bool Separate(const wstring& strIn, const wstring& strSep, wstring& strLeft, wstring& strRight) { return Separate(strIn, strSep.c_str(), strLeft, strRight); }

int CompareNoCase(const char* s1, const char* s2);
inline int CompareNoCase(const string& str1, const char* s2) { return CompareNoCase(str1.c_str(), s2); }
inline int CompareNoCase(const string& str1, const string& str2) { return CompareNoCase(str1, str2.c_str()); }

int CompareNoCase(const WCHAR* s1, const WCHAR* s2);
inline int CompareNoCase(const wstring& str1, const WCHAR* s2) { return CompareNoCase(str1.c_str(), s2); }
inline int CompareNoCase(const wstring& str1, const wstring& str2) { return CompareNoCase(str1, str2.c_str()); }

bool ParseIPv4Address(const WCHAR* s, int& n);

inline char UtilToUpper(char c) { return 'a' <= c && c <= 'z' ? c - 'a' + 'A' : c; }
inline WCHAR UtilToUpper(WCHAR c) { return L'a' <= c && c <= L'z' ? c - L'a' + L'A' : c; }

#endif
