#ifndef INCLUDE_INI_UTIL_H
#define INCLUDE_INI_UTIL_H

// 必要なバッファを確保してGetPrivateProfileSection()を呼ぶ
vector<WCHAR> GetPrivateProfileSectionBuffer(LPCWSTR appName, LPCWSTR fileName);
// GetPrivateProfileSection()で取得したバッファから、キーに対応する文字列を取得する
void GetBufferedProfileString(LPCWSTR buff, LPCWSTR keyName, LPCWSTR lpDefault, LPWSTR returnedString, DWORD nSize);
// GetPrivateProfileSection()で取得したバッファから、キーに対応する文字列をwstringで取得する
wstring GetBufferedProfileToString(LPCWSTR buff, LPCWSTR keyName, LPCWSTR lpDefault);
// GetPrivateProfileSection()で取得したバッファから、キーに対応する数値を取得する
int GetBufferedProfileInt(LPCWSTR buff, LPCWSTR keyName, int nDefault);

#endif
