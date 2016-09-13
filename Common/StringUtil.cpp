#include "stdafx.h"
#include "StringUtil.h"

namespace
{

size_t WtoMB(UINT codePage, const WCHAR *in, vector<char>& out, size_t outLenHint)
{
	if( out.size() < outLenHint + 1 ){
		out.resize(outLenHint + 1);
	}
	size_t len = WideCharToMultiByte(codePage, 0, in, -1, &out.front(), (int)out.size(), NULL, NULL);
	if( len == 0 ){
		//rare case
		len = WideCharToMultiByte(codePage, 0, in, -1, NULL, 0, NULL, NULL);
		if( len < out.size() ){
			len = 0;
		}else{
			out.resize(len);
			len = WideCharToMultiByte(codePage, 0, in, -1, &out.front(), (int)out.size(), NULL, NULL);
		}
		if( len == 0 ){
			out[0] = '\0';
			len = 1;
		}
	}
	return len - 1;
}

size_t MBtoW(UINT codePage, const char *in, vector<WCHAR>& out, size_t outLenHint)
{
	if( out.size() < outLenHint + 1 ){
		out.resize(outLenHint + 1);
	}
	size_t len = MultiByteToWideChar(codePage, 0, in, -1, &out.front(), (int)out.size());
	if( len == 0 ){
		//rare case
		len = MultiByteToWideChar(codePage, 0, in, -1, NULL, 0);
		if( len < out.size() ){
			len = 0;
		}else{
			out.resize(len);
			len = MultiByteToWideChar(codePage, 0, in, -1, &out.front(), (int)out.size());
		}
		if( len == 0 ){
			out[0] = L'\0';
			len = 1;
		}
	}
	return len - 1;
}

}

void Format(string& strBuff, const char *format, ...)
{
	va_list params;

	va_start(params, format);
	try{
		int length = _vscprintf(format, params);
		if( length < 0 ){
			throw std::runtime_error("");
		}else if( length < 64 ){
			char szSmall[64];
			vsprintf_s(szSmall, format, params);
			strBuff = szSmall;
		}else{
			vector<char> buff(length + 1);
			vsprintf_s(&buff.front(), buff.size(), format, params);
			strBuff = &buff.front();
		}
	}catch(...){
		va_end(params);
		throw;
	}

	va_end(params);
}

void Format(wstring& strBuff, const WCHAR *format, ...)
{
	va_list params;

	va_start(params, format);

	try{
		int length = _vscwprintf(format, params);
		if( length < 0 ){
			throw std::runtime_error("");
		}else if( length < 64 ){
			WCHAR szSmall[64];
			vswprintf_s(szSmall, format, params);
			strBuff = szSmall;
		}else{
			vector<WCHAR> buff(length + 1);
			vswprintf_s(&buff.front(), buff.size(), format, params);
			strBuff = &buff.front();
		}
	}catch(...){
		va_end(params);
		throw;
	}

    va_end(params);
}

void Replace(string& strBuff, const string& strOld, const string& strNew)
{
	string::size_type Pos = 0;
	string* strWork = &strBuff;
	string strForAlias;

	if( strWork == &strOld || strWork == &strNew ){
		strForAlias = strBuff;
		strWork = &strForAlias;
	}
	while ((Pos = strWork->find(strOld,Pos)) != string::npos)
	{
		strWork->replace(Pos,strOld.size(),strNew);
		Pos += strNew.size();
	}
	if( strWork == &strForAlias ){
		strBuff = strForAlias;
	}
}

void Replace(wstring& strBuff, const wstring& strOld, const wstring& strNew)
{
	string::size_type Pos = 0;
	wstring* strWork = &strBuff;
	wstring strForAlias;

	if( strWork == &strOld || strWork == &strNew ){
		strForAlias = strBuff;
		strWork = &strForAlias;
	}
	while ((Pos = strWork->find(strOld,Pos)) != string::npos)
	{
		strWork->replace(Pos,strOld.size(),strNew);
		Pos += strNew.size();
	}
	if( strWork == &strForAlias ){
		strBuff = strForAlias;
	}
}

void WtoA(const wstring& strIn, string& strOut)
{
	vector<char> buff;
	size_t len = WtoA(strIn.c_str(), strIn.size(), buff);
	strOut.assign(&buff.front(), &buff.front() + len);
}

size_t WtoA(const WCHAR *in, size_t inLenHint, vector<char>& out)
{
	return WtoMB(932, in, out, inLenHint * 2);
}

void WtoUTF8(const wstring& strIn, string& strOut)
{
	vector<char> buff;
	size_t len = WtoUTF8(strIn.c_str(), strIn.size(), buff);
	strOut.assign(&buff.front(), &buff.front() + len);
}

size_t WtoUTF8(const WCHAR *in, size_t inLenHint, vector<char>& out)
{
	return WtoMB(CP_UTF8, in, out, inLenHint * 3);
}

void AtoW(const string& strIn, wstring& strOut)
{
	vector<WCHAR> buff;
	size_t len = AtoW(strIn.c_str(), strIn.size(), buff);
	strOut.assign(&buff.front(), &buff.front() + len);
}

size_t AtoW(const char *in, size_t inLenHint, vector<WCHAR>& out)
{
	return MBtoW(932, in, out, inLenHint);
}

void UTF8toW(const string& strIn, wstring& strOut)
{
	vector<WCHAR> buff;
	size_t len = UTF8toW(strIn.c_str(), strIn.size(), buff);
	strOut.assign(&buff.front(), &buff.front() + len);
}

size_t UTF8toW(const char *in, size_t inLenHint, vector<WCHAR>& out)
{
	return MBtoW(CP_UTF8, in, out, inLenHint);
}

BOOL Separate(const string& strIn, const char *sep, string& strLeft, string& strRight)
{
	string::size_type Pos = strIn.find(sep);
	string strL(strIn, 0, Pos);
	if( Pos == string::npos ){
		strRight = "";
		strLeft = strL;
		return FALSE;
	}
	strRight = strIn.substr(Pos+strlen(sep));
	strLeft = strL;
	
	return TRUE;
}

BOOL Separate(const wstring& strIn, const WCHAR *sep, wstring& strLeft, wstring& strRight)
{
	wstring::size_type Pos = strIn.find(sep);
	wstring strL(strIn, 0, Pos);
	if( Pos == string::npos ){
		strRight = L"";
		strLeft = strL;
		return FALSE;
	}
	strRight = strIn.substr(Pos+wcslen(sep));
	strLeft = strL;
	
	return TRUE;
}

int CompareNoCase(const string& str1, const char *str2)
{
	return _stricmp(str1.c_str(), str2);
}

int CompareNoCase(const wstring& str1, const WCHAR *str2)
{
	return _wcsicmp(str1.c_str(), str2);
}
