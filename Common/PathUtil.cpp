#include "stdafx.h"
#include "PathUtil.h"
#include <stdexcept>

namespace filesystem_
{
// Extract from Boost.Filesystem(1.64.0) www.boost.org/libs/filesystem
// "Use, modification, and distribution are subject to the Boost Software License, Version 1.0. See www.boost.org/LICENSE_1_0.txt"

path& path::append(const wchar_t* source)
{
	if (*source) {
		size_t sep_pos(m_append_separator_if_needed());
		m_pathname += source;
		m_erase_redundant_separator(sep_pos);
	}
	return *this;
}

path& path::append(const wstring& source)
{
	if (!source.empty()) {
		size_t sep_pos(m_append_separator_if_needed());
		m_pathname += source;
		m_erase_redundant_separator(sep_pos);
	}
	return *this;
}

// Returns: If separator is to be appended, m_pathname.size() before append. Otherwise 0.
// Note: An append is never performed if size()==0, so a returned 0 is unambiguous.
size_t path::m_append_separator_if_needed()
{
	if (!m_pathname.empty() && m_pathname.back() != L':' && !is_dir_sep(m_pathname.back())) {
		m_pathname += L'\\';
		return m_pathname.size() - 1;
	}
	return 0;
}

void path::m_erase_redundant_separator(size_t sep_pos)
{
	if (sep_pos && sep_pos + 1 < m_pathname.size() && is_dir_sep(m_pathname[sep_pos + 1])) {
		m_pathname.erase(sep_pos, 1);
	}
}

path& path::remove_filename()
{
	size_t end_pos(m_parent_path_end());
	if (end_pos != wstring::npos) {
		m_pathname.erase(end_pos);
	}
	return *this;
}

path& path::replace_extension(const path& new_extension)
{
	// erase existing extension, including the dot, if any
	m_pathname.erase(m_pathname.size() - extension().m_pathname.size());
	if (!new_extension.empty()) {
		// append new_extension, adding the dot if necessary
		if (new_extension.m_pathname[0] != L'.') {
			m_pathname += L'.';
		}
		m_pathname += new_extension.m_pathname;
	}
	return *this;
}

path path::root_name() const
{
	size_t itr_pos;
	size_t itr_element_size;
	first_element(m_pathname, itr_pos, itr_element_size);
	return (itr_pos != m_pathname.size()
		&& ((itr_element_size > 1 && is_dir_sep(m_pathname[itr_pos]) && is_dir_sep(m_pathname[itr_pos + 1]))
			|| m_pathname[itr_pos + itr_element_size - 1] == L':'
		)) ? path(m_pathname.substr(itr_pos, itr_element_size)) : path();
}

path path::root_directory() const
{
	size_t pos(root_directory_start(m_pathname, m_pathname.size()));
	return pos == wstring::npos ? path() : path(m_pathname.substr(pos, 1));
}

size_t path::m_parent_path_end() const
{
	size_t end_pos(filename_pos(m_pathname));
	bool filename_was_separator(m_pathname.size() && is_dir_sep(m_pathname[end_pos]));
	// skip separators unless root directory
	size_t root_dir_pos(root_directory_start(m_pathname, end_pos));
	for (; end_pos > 0 && end_pos - 1 != root_dir_pos && is_dir_sep(m_pathname[end_pos - 1]); --end_pos);
	return end_pos == 1 && root_dir_pos == 0 && filename_was_separator ? wstring::npos : end_pos;
}

path path::parent_path() const
{
	size_t end_pos(m_parent_path_end());
	return end_pos == wstring::npos ? path() : path(m_pathname.substr(0, end_pos));
}

path path::filename() const
{
	size_t pos(filename_pos(m_pathname));
	return (m_pathname.size() && pos && is_dir_sep(m_pathname[pos]) && !is_root_separator(m_pathname, pos)
		) ? path(L".") : path(m_pathname.c_str() + pos);
}

path path::stem() const
{
	path name(filename());
	if (name.native() == L"." || name.native() == L"..") {
		return name;
	}
	size_t pos(name.m_pathname.rfind(L'.'));
	if (pos != wstring::npos) {
		name.m_pathname.erase(pos);
	}
	return name;
}

// pos is position of the separator
bool path::is_root_separator(const wstring& str, size_t pos)
{
	// ASSERT(!str.empty() && is_dir_sep(str[pos]));
	// subsequent logic expects pos to be for leftmost slash of a set
	for (; pos > 0 && is_dir_sep(str[pos - 1]); --pos);
	//  "/" [...]
	if (pos == 0) {
		return true;
	}
	//  "c:/" [...]
	if (pos == 2 && is_letter(str[0]) && str[1] == L':') {
		return true;
	}
	//  "//" name "/"
	if (pos < 3 || !is_dir_sep(str[0]) || !is_dir_sep(str[1])) {
		return false;
	}
	return str.find_first_of(L"/\\", 2) == pos;
}

// return 0 if str itself is filename (or empty)
size_t path::filename_pos(const wstring& str)
{
	// case: "//"
	if (str.size() == 2 && is_dir_sep(str[0]) && is_dir_sep(str[1])) {
		return 0;
	}
	// case: ends in "/"
	if (str.size() && is_dir_sep(str[str.size() - 1])) {
		return str.size() - 1;
	}
	// set pos to start of last element
	size_t pos(str.find_last_of(L"/\\", str.size() - 1));
	if (pos == wstring::npos && str.size() > 1) {
		pos = str.find_last_of(L':', str.size() - 2);
	}
	return (pos == wstring::npos // path itself must be a filename (or empty)
		|| (pos == 1 && is_dir_sep(str[0]))) // or net
			? 0 // so filename is entire string
			: pos + 1; // or starts after delimiter
}

// return npos if no root_directory found
size_t path::root_directory_start(const wstring& str, size_t size)
{
	// case "c:/"
	if (size > 2 && str[1] == L':' && is_dir_sep(str[2])) {
		return 2;
	}
	// case "//"
	if (size == 2 && is_dir_sep(str[0]) && is_dir_sep(str[1])) {
		return wstring::npos;
	}
	// case "\\?\"
	if (size > 4 && is_dir_sep(str[0]) && is_dir_sep(str[1]) && str[2] == L'?' && is_dir_sep(str[3])) {
		size_t pos(str.find_first_of(L"/\\", 4));
		return pos < size ? pos : wstring::npos;
	}
	// case "//net {/}"
	if (size > 3 && is_dir_sep(str[0]) && is_dir_sep(str[1]) && !is_dir_sep(str[2])) {
		size_t pos(str.find_first_of(L"/\\", 2));
		return pos < size ? pos : wstring::npos;
	}
	// case "/"
	if (size > 0 && is_dir_sep(str[0])) {
		return 0;
	}
	return wstring::npos;
}

// sets pos and len of first element, excluding extra separators
// if src.empty(), sets pos,len, to 0,0.
void path::first_element(const wstring& src, size_t& element_pos, size_t& element_size)
{
	element_pos = 0;
	element_size = 0;
	if (src.empty()) {
		return;
	}
	size_t cur(0);
	// deal with // [network]
	if (src.size() >= 2 && is_dir_sep(src[0]) && is_dir_sep(src[1]) && (src.size() == 2 || !is_dir_sep(src[2]))) {
		cur += 2;
		element_size += 2;
	}
	// leading (not non-network) separator
	else if (is_dir_sep(src[0])) {
		++element_size;
		// bypass extra leading separators
		for (; cur + 1 < src.size() && is_dir_sep(src[cur + 1]); ++cur, ++element_pos);
		return;
	}
	// at this point, we have either a plain name, a network name,
	// or (on Windows only) a device name
	// find the end
	for (; cur < src.size() && src[cur] != L':' && !is_dir_sep(src[cur]); ++cur, ++element_size);
	// include device delimiter
	if (cur < src.size() && src[cur] == L':') {
		++element_size;
	}
}

} // namespace filesystem_


fs_path GetDefSettingPath()
{
	return GetModulePath().replace_filename(L"Setting");
}

fs_path GetSettingPath()
{
	fs_path path = GetPrivateProfileToFolderPath(L"SET", L"DataSavePath", GetCommonIniPath().c_str());
	return path.empty() ? GetDefSettingPath() : path;
}

void GetModuleFolderPath(wstring& strPath)
{
	strPath = GetModulePath().parent_path().native();
}

fs_path GetModulePath(HMODULE hModule)
{
	WCHAR szPath[MAX_PATH];
	DWORD len = GetModuleFileName(hModule, szPath, MAX_PATH);
	if( len == 0 || len >= MAX_PATH ){
		throw std::runtime_error("");
	}
	fs_path path(szPath);
	if( path.is_relative() || path.has_filename() == false ){
		throw std::runtime_error("");
	}
	return path;
}

fs_path GetPrivateProfileToFolderPath(LPCWSTR appName, LPCWSTR keyName, LPCWSTR fileName)
{
	if( !appName || !keyName ){
		return fs_path();
	}
	fs_path path(GetPrivateProfileToString(appName, keyName, L"", fileName));
	ChkFolderPath(path);
	return path;
}

fs_path GetModuleIniPath(HMODULE hModule)
{
	return GetModulePath(hModule).replace_extension(L".ini");
}

fs_path GetCommonIniPath()
{
	return GetModulePath().replace_filename(L"Common.ini");
}

fs_path GetRecFolderPath(int index)
{
	fs_path iniPath = GetCommonIniPath();
	int num = GetPrivateProfileInt(L"SET", L"RecFolderNum", 0, iniPath.c_str());
	if( index <= 0 ){
		// 必ず返す
		fs_path path;
		if( num > 0 ){
			path = GetPrivateProfileToFolderPath(L"SET", L"RecFolderPath0", iniPath.c_str());
		}
		return path.empty() ? GetSettingPath() : path;
	}
	for( int i = 1; i < num; i++ ){
		WCHAR szKey[32];
		swprintf_s(szKey, L"RecFolderPath%d", i);
		fs_path path = GetPrivateProfileToFolderPath(L"SET", szKey, iniPath.c_str());
		// 空要素は詰める
		if( path.empty() == false ){
			if( --index <= 0 ){
				return path;
			}
		}
	}
	// 範囲外
	return fs_path();
}

BOOL IsExt(const fs_path& filePath, const WCHAR* ext)
{
	return _wcsicmp(filePath.extension().c_str(), ext) == 0;
}

void CheckFileName(wstring& fileName, BOOL noChkYen)
{
	static const WCHAR s[10] = L"\\/:*?\"<>|";
	static const WCHAR r[10] = L"￥／：＊？”＜＞｜";
	// トリム
	size_t j = fileName.find_last_not_of(L' ');
	fileName.erase(j == wstring::npos ? 0 : j + 1);
	fileName.erase(0, fileName.find_first_not_of(L' '));
	for( size_t i = 0; i < fileName.size(); i++ ){
		if( L'\x1' <= fileName[i] && fileName[i] <= L'\x1f' || fileName[i] == L'\x7f' ){
			// 制御文字
			fileName[i] = L'〓';
		}
		// ".\"となるときはnoChkYenでも全角に
		const WCHAR* p = wcschr(s, fileName[i]);
		if( p && (noChkYen == FALSE || *p != L'\\' || (i > 0 && fileName[i - 1] == L'.')) ){
			fileName[i] = r[p - s];
		}
	}
	// 冒頭'\'はトリム
	fileName.erase(0, fileName.find_first_not_of(L'\\'));
	// すべて'.'のときは全角に
	if( fileName.find_first_not_of(L'.') == wstring::npos ){
		for( size_t i = 0; i < fileName.size(); fileName[i++] = L'．' );
	}
}

void ChkFolderPath(fs_path& path)
{
	// 過去にルートディレクトリ区切りを失った状態("C:"など)で設定などに保存していたので、これに対応する
	if( path.empty() == false && path.native().back() != L'/' && path.native().back() != L'\\' ){
		// 一時的に下層を作って上がる
		// TODO: remove_filename()の末尾区切りについて標準の仕様が揺れている。状況によってはparent_path()に変更すること
		path.concat(L"\\a").remove_filename();
	}
}

void TouchFileAsUnicode(LPCWSTR path)
{
	if( GetFileAttributes(path) == INVALID_FILE_ATTRIBUTES ){
		FILE* fp;
		if( _wfopen_s(&fp, path, L"abN") == 0 && fp ){
			_fseeki64(fp, 0, SEEK_END);
			if( _ftelli64(fp) == 0 ){
				fputwc(L'\xFEFF', fp);
			}
			fclose(fp);
		}
	}
}

BOOL UtilCreateDirectories(const fs_path& path)
{
	if( path.empty() || path.is_relative() && path.has_root_path() ){
		// パスが不完全
		return FALSE;
	}
	if( GetFileAttributes(path.c_str()) != INVALID_FILE_ATTRIBUTES ){
		// 既存
		return FALSE;
	}
	DWORD err = GetLastError();
	if( err != ERROR_FILE_NOT_FOUND && err != ERROR_PATH_NOT_FOUND ){
		// 特殊な理由
		_OutputDebugString(L"UtilCreateDirectories(): Error=%08x\r\n", err);
		return FALSE;
	}
	UtilCreateDirectories(path.parent_path());
	return CreateDirectory(path.c_str(), NULL);
}

wstring GetChkDrivePath(const wstring& directoryPath)
{
	WCHAR szVolumePathName[MAX_PATH];
	if( GetVolumePathName(directoryPath.c_str(), szVolumePathName, MAX_PATH) == FALSE ){
		return directoryPath;
	}
	WCHAR szMount[MAX_PATH];
	if( GetVolumeNameForVolumeMountPoint(szVolumePathName, szMount, MAX_PATH) == FALSE ){
		return szVolumePathName;
	}
	return szMount;
}

vector<WCHAR> GetPrivateProfileSectionBuffer(LPCWSTR appName, LPCWSTR fileName)
{
	vector<WCHAR> buff(4096);
	for(;;){
		DWORD n = GetPrivateProfileSection(appName, &buff.front(), (DWORD)buff.size(), fileName);
		if( n < buff.size() - 2 ){
			buff.resize(n + 1);
			break;
		}
		if( buff.size() >= 16 * 1024 * 1024 ){
			buff.assign(1, L'\0');
			break;
		}
		buff.resize(buff.size() * 2);
	}
	return buff;
}

wstring GetPrivateProfileToString(LPCWSTR appName, LPCWSTR keyName, LPCWSTR lpDefault, LPCWSTR fileName)
{
	WCHAR szBuff[512];
	DWORD n = GetPrivateProfileString(appName, keyName, lpDefault, szBuff, 512, fileName);
	if( n >= 510 ){
		vector<WCHAR> buff(512);
		do{
			buff.resize(buff.size() * 2);
			n = GetPrivateProfileString(appName, keyName, lpDefault, &buff.front(), (DWORD)buff.size(), fileName);
		}while( n >= buff.size() - 2 && buff.size() < 1024 * 1024 );
		return wstring(buff.begin(), buff.begin() + n);
	}
	return wstring(szBuff, szBuff + n);
}

BOOL WritePrivateProfileInt(LPCWSTR appName, LPCWSTR keyName, int value, LPCWSTR fileName)
{
	WCHAR sz[32];
	swprintf_s(sz, L"%d", value);
	return WritePrivateProfileString(appName, keyName, sz, fileName);
}
