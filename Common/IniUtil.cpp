#include "stdafx.h"
#include "IniUtil.h"
#include "StringUtil.h"

vector<WCHAR> GetPrivateProfileSectionBuffer(LPCWSTR appName, LPCWSTR fileName)
{
	vector<WCHAR> buff(4096);
	for(;;){
		DWORD len = GetPrivateProfileSection(appName, buff.data(), (DWORD)buff.size(), fileName);
		if( len < buff.size() - 2 ){
			buff.resize(len + 1);
			break;
		}
		if( buff.size() >= 1024 * 1024 * 1024 ){
			buff.assign(1, L'\0');
			break;
		}
		buff.resize(buff.size() * 2);
	}
	return buff;
}

void GetBufferedProfileString(LPCWSTR buff, LPCWSTR keyName, LPCWSTR lpDefault, LPWSTR returnedString, DWORD nSize)
{
	size_t nKeyLen = wcslen(keyName);
	while( *buff ){
		size_t nLen = wcslen(buff);
		if( nLen > nKeyLen && buff[nKeyLen] == L'=' &&
		    std::equal(buff, buff + nKeyLen, keyName, [](WCHAR a, WCHAR b) { return UtilToUpper(a) == UtilToUpper(b); }) ){
			if( (buff[nKeyLen + 1] == L'\'' || buff[nKeyLen + 1] == L'"') &&
			    nLen >= nKeyLen + 3 && buff[nKeyLen + 1] == buff[nLen - 1] ){
				wcsncpy_s(returnedString, nSize, buff + nKeyLen + 2, min(nLen - nKeyLen - 3, (size_t)(nSize - 1)));
			}else{
				wcsncpy_s(returnedString, nSize, buff + nKeyLen + 1, _TRUNCATE);
			}
			return;
		}
		buff += nLen + 1;
	}
	wcsncpy_s(returnedString, nSize, lpDefault, _TRUNCATE);
}

wstring GetBufferedProfileToString(LPCWSTR buff, LPCWSTR keyName, LPCWSTR lpDefault)
{
	size_t nKeyLen = wcslen(keyName);
	while( *buff ){
		size_t nLen = wcslen(buff);
		if( nLen > nKeyLen && buff[nKeyLen] == L'=' &&
		    std::equal(buff, buff + nKeyLen, keyName, [](WCHAR a, WCHAR b) { return UtilToUpper(a) == UtilToUpper(b); }) ){
			if( (buff[nKeyLen + 1] == L'\'' || buff[nKeyLen + 1] == L'"') &&
			    nLen >= nKeyLen + 3 && buff[nKeyLen + 1] == buff[nLen - 1] ){
				return wstring(buff + nKeyLen + 2, nLen - nKeyLen - 3);
			}else{
				return wstring(buff + nKeyLen + 1, nLen - nKeyLen - 1);
			}
		}
		buff += nLen + 1;
	}
	return lpDefault;
}

int GetBufferedProfileInt(LPCWSTR buff, LPCWSTR keyName, int nDefault)
{
	WCHAR sz[16];
	GetBufferedProfileString(buff, keyName, L"", sz, 16);
	LPWSTR endp;
	int nRet = (int)wcstol(sz, &endp, 10);
	return endp == sz ? nDefault : nRet;
}
