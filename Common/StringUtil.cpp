#include "stdafx.h"
#include "StringUtil.h"

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
	strOut.clear();
	int iLen = (int)strIn.size() * 2 + 1;
	char* pszBuff = new char[iLen];
	if( WideCharToMultiByte( 932, 0, strIn.c_str(), -1, pszBuff, iLen, NULL, NULL ) != 0 ){
		strOut = pszBuff;
		delete[] pszBuff;
	}else{
		//rare case
		delete[] pszBuff;
		iLen = WideCharToMultiByte( 932, 0, strIn.c_str(), -1, NULL, 0, NULL, NULL );
		if( iLen > 0 ){
			pszBuff = new char[iLen];
			if( WideCharToMultiByte( 932, 0, strIn.c_str(), -1, pszBuff, iLen, NULL, NULL ) != 0 ){
				strOut = pszBuff;
			}
			delete[] pszBuff;
		}
	}
}

void WtoUTF8(const wstring& strIn, string& strOut)
{
	strOut.clear();
	int iLen = (int)strIn.size() * 3 + 1;
	char* pszBuff = new char[iLen];
	if( WideCharToMultiByte( CP_UTF8, 0, strIn.c_str(), -1, pszBuff, iLen, NULL, NULL ) != 0 ){
		strOut = pszBuff;
		delete[] pszBuff;
	}else{
		//rare case
		delete[] pszBuff;
		iLen = WideCharToMultiByte( CP_UTF8, 0, strIn.c_str(), -1, NULL, 0, NULL, NULL );
		if( iLen > 0 ){
			pszBuff = new char[iLen];
			if( WideCharToMultiByte( CP_UTF8, 0, strIn.c_str(), -1, pszBuff, iLen, NULL, NULL ) != 0 ){
				strOut = pszBuff;
			}
			delete[] pszBuff;
		}
	}
}

void AtoW(const string& strIn, wstring& strOut)
{
	strOut.clear();
	int iLen = (int)strIn.size() + 1;
	WCHAR* pwszBuff = new WCHAR[iLen];
	if( MultiByteToWideChar( 932, 0, strIn.c_str(), -1, pwszBuff, iLen ) != 0 ){
		strOut = pwszBuff;
		delete[] pwszBuff;
	}else{
		//rare case
		delete[] pwszBuff;
		iLen = MultiByteToWideChar( 932, 0, strIn.c_str(), -1, NULL, 0 );
		if( iLen > 0 ){
			pwszBuff = new WCHAR[iLen];
			if( MultiByteToWideChar( 932, 0, strIn.c_str(), -1, pwszBuff, iLen ) != 0 ){
				strOut = pwszBuff;
			}
			delete[] pwszBuff;
		}
	}
}

void UTF8toW(const string& strIn, wstring& strOut)
{
	strOut.clear();
	int iLen = (int)strIn.size() + 1;
	WCHAR* pwszBuff = new WCHAR[iLen];
	if( MultiByteToWideChar( CP_UTF8, 0, strIn.c_str(), -1, pwszBuff, iLen ) != 0 ){
		strOut = pwszBuff;
		delete[] pwszBuff;
	}else{
		//rare case
		delete[] pwszBuff;
		iLen = MultiByteToWideChar( CP_UTF8, 0, strIn.c_str(), -1, NULL, 0 );
		if( iLen > 0 ){
			pwszBuff = new WCHAR[iLen];
			if( MultiByteToWideChar( CP_UTF8, 0, strIn.c_str(), -1, pwszBuff, iLen ) != 0 ){
				strOut = pwszBuff;
			}
			delete[] pwszBuff;
		}
	}
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
