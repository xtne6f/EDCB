#include "stdafx.h"
#include "PathUtil.h"
#include "StringUtil.h"
#include <stdexcept>
#include <fcntl.h>
#ifdef _WIN32
#include <io.h>
#else
#include "ThreadUtil.h"
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>
#endif

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
	if (!m_pathname.empty() &&
#ifdef _WIN32
	    m_pathname.back() != L':' &&
#endif
	    !is_dir_sep(m_pathname.back())) {
		m_pathname += preferred_separator;
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
#ifdef _WIN32
			|| m_pathname[itr_pos + itr_element_size - 1] == L':'
#endif
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
#ifdef _WIN32
	//  "c:/" [...]
	if (pos == 2 && is_letter(str[0]) && str[1] == L':') {
		return true;
	}
#endif
	//  "//" name "/"
	if (pos < 3 || !is_dir_sep(str[0]) || !is_dir_sep(str[1])) {
		return false;
	}
	return str.find_first_of(separators(), 2) == pos;
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
	size_t pos(str.find_last_of(separators(), str.size() - 1));
#ifdef _WIN32
	if (pos == wstring::npos && str.size() > 1) {
		pos = str.find_last_of(L':', str.size() - 2);
	}
#endif
	return (pos == wstring::npos // path itself must be a filename (or empty)
		|| (pos == 1 && is_dir_sep(str[0]))) // or net
			? 0 // so filename is entire string
			: pos + 1; // or starts after delimiter
}

// return npos if no root_directory found
size_t path::root_directory_start(const wstring& str, size_t size)
{
#ifdef _WIN32
	// case "c:/"
	if (size > 2 && str[1] == L':' && is_dir_sep(str[2])) {
		return 2;
	}
#endif
	// case "//"
	if (size == 2 && is_dir_sep(str[0]) && is_dir_sep(str[1])) {
		return wstring::npos;
	}
#ifdef _WIN32
	// case "\\?\"
	if (size > 4 && is_dir_sep(str[0]) && is_dir_sep(str[1]) && str[2] == L'?' && is_dir_sep(str[3])) {
		size_t pos(str.find_first_of(separators(), 4));
		return pos < size ? pos : wstring::npos;
	}
#endif
	// case "//net {/}"
	if (size > 3 && is_dir_sep(str[0]) && is_dir_sep(str[1]) && !is_dir_sep(str[2])) {
		size_t pos(str.find_first_of(separators(), 2));
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
	for (; cur < src.size()
#ifdef _WIN32
		&& src[cur] != L':'
#endif
		&& !is_dir_sep(src[cur]); ++cur, ++element_size);
#ifdef _WIN32
	// include device delimiter
	if (cur < src.size() && src[cur] == L':') {
		++element_size;
	}
#endif
}

} // namespace filesystem_


FILE* UtilOpenFile(const wstring& path, int flags, int* apiError)
{
	if( apiError ){
		*apiError = 0;
	}
#ifdef _WIN32
	LPCWSTR mode = (flags & 31) == UTIL_O_RDONLY ? L"rb" :
	               (flags & 31) == UTIL_O_RDWR ? L"r+b" :
	               (flags & 31) == UTIL_O_CREAT_WRONLY ? L"wb" :
	               (flags & 31) == UTIL_O_CREAT_RDWR ? L"w+b" :
	               (flags & 31) == UTIL_O_CREAT_APPEND ? L"ab" :
	               (flags & 31) == UTIL_O_EXCL_CREAT_WRONLY ? L"wb" :
	               (flags & 31) == UTIL_O_EXCL_CREAT_RDWR ? L"w+b" :
	               (flags & 31) == UTIL_O_EXCL_CREAT_APPEND ? L"ab" : NULL;
	if( mode ){
		// 共有モード制御のためAPIで開く
		// 参考:後続が開くことのできる共有モードのパターン(双方向。*は未サポート)
		// (RD,SH_READ)<=>(RD,SH_READ)
		// (RD,SH_READ)<=>(RD,SH_RDWR)
		// (RD,SH_RDWR)<=>(RD,SH_RDWR)
		// (RD,SH_RDWR)<=>(WR,SH_READ)
		// (RD,SH_RDWR)<=>(WR,SH_RDWR)*
		// (WR,SH_RDWR)<=>(WR,SH_RDWR)*
		HANDLE h = CreateFile(path.c_str(), ((flags & 1) ? GENERIC_READ : 0) | ((flags & 2) ? GENERIC_WRITE : 0),
		                      ((flags & 32) ? FILE_SHARE_READ : 0) | ((flags & 64) ? FILE_SHARE_WRITE : 0) | ((flags & 128) ? FILE_SHARE_DELETE : 0),
		                      NULL, (flags & 24) == 8 ? CREATE_ALWAYS : (flags & 24) == 16 ? OPEN_ALWAYS : (flags & 24) == 24 ? CREATE_NEW : OPEN_EXISTING,
		                      FILE_ATTRIBUTE_NORMAL | ((flags & 256) ? FILE_FLAG_SEQUENTIAL_SCAN : 0), NULL);
		if( h != INVALID_HANDLE_VALUE ){
			int fd = _open_osfhandle((intptr_t)h, (flags & 4) ? _O_APPEND : 0);
			if( fd != -1 ){
				FILE* fp = _wfdopen(fd, mode);
				if( fp ){
					if( flags & 512 ){
						setvbuf(fp, NULL, _IONBF, 0);
					}
					return fp;
				}
				_close(fd);
			}else{
				CloseHandle(h);
			}
		}else if( apiError ){
			*apiError = GetLastError();
		}
	}
#else
	const char* mode = (flags & 31) == UTIL_O_RDONLY ? "r" :
	                   (flags & 31) == UTIL_O_RDWR ? "r+" :
	                   (flags & 31) == UTIL_O_CREAT_WRONLY ? "w" :
	                   (flags & 31) == UTIL_O_CREAT_RDWR ? "w+" :
	                   (flags & 31) == UTIL_O_CREAT_APPEND ? "a" :
	                   (flags & 31) == UTIL_O_EXCL_CREAT_WRONLY ? "w" :
	                   (flags & 31) == UTIL_O_EXCL_CREAT_RDWR ? "w+" :
	                   (flags & 31) == UTIL_O_EXCL_CREAT_APPEND ? "a" : NULL;
	if( mode ){
		string strPath;
		WtoUTF8(path, strPath);
		// 切り捨てとロックの関係を正しく表現できないので低水準で開く
		int fd = open(strPath.c_str(), ((flags & 3) == 3 ? O_RDWR : (flags & 2) ? O_WRONLY : O_RDONLY) |
		                               ((flags & 24) ? O_CREAT : 0) |
		                               ((flags & 24) == 24 ? O_EXCL : 0) |
		                               ((flags & 4) ? O_APPEND : 0) | O_CLOEXEC,
		              S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
		if( fd >= 0 ){
			// flock()は勧告ロックに過ぎないので注意
			// UTIL_SHARED_READは無ロック相当で、既にかかっている排他ロックを無視するので注意
			// UTIL_SH_READは非書き込み時のみ共有ロックとする
			if( ((flags & 32) && (flags & 64)) ||
			    flock(fd, ((flags & 32) && (flags & 2) == 0 ? LOCK_SH : LOCK_EX) | LOCK_NB) == 0 ){
				if( (flags & 24) == 8 ){
					ftruncate(fd, 0);
				}
				FILE* fp = fdopen(fd, mode);
				if( fp ){
					if( flags & 4 ){
						my_fseek(fp, 0, SEEK_END);
					}
					if( flags & 512 ){
						setvbuf(fp, NULL, _IONBF, 0);
					}
					return fp;
				}
			}
			if( apiError ){
				*apiError = errno == EACCES ? EAGAIN : errno;
			}
			// (ほぼないが)ファイル生成後にロックに失敗した場合のロールバックはしない
			close(fd);
		}else if( apiError ){
			*apiError = errno;
		}
	}
#endif
	return NULL;
}

void* UtilLoadLibrary(const wstring& path)
{
#ifdef _WIN32
	return LoadLibrary(path.c_str());
#else
	string strPath;
	WtoUTF8(path, strPath);
	return dlopen(strPath.c_str(), RTLD_NOW);
#endif
}

void UtilFreeLibrary(void* hModule)
{
#ifdef _WIN32
	FreeLibrary((HMODULE)hModule);
#else
	dlclose(hModule);
#endif
}

void* UtilGetProcAddress(void* hModule, const char* name)
{
	if( hModule ){
#ifdef _WIN32
		return (void*)GetProcAddress((HMODULE)hModule, name);
#else
		return dlsym(hModule, name);
#endif
	}
	return NULL;
}

#ifndef _WIN32
BOOL DeleteFile(LPCWSTR path)
{
	string strPath;
	WtoUTF8(path, strPath);
	return remove(strPath.c_str()) == 0;
}
#endif

fs_path GetDefSettingPath()
{
	return GetCommonIniPath().replace_filename(L"Setting");
}

fs_path GetSettingPath()
{
	fs_path path = GetPrivateProfileToString(L"SET", L"DataSavePath", L"", GetCommonIniPath().c_str());
	return path.empty() ? GetDefSettingPath() : path;
}

#ifdef _WIN32
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

fs_path GetModuleIniPath(HMODULE hModule)
{
	if( hModule ){
		return GetModulePath(hModule).concat(L".ini");
	}
	return GetModulePath().replace_extension(L".ini");
}
#else
fs_path GetModulePath(void* funcAddr)
{
	wstring strPath;
	if( funcAddr ){
		Dl_info info;
		if( dladdr(funcAddr, &info) == 0 ){
			throw std::runtime_error("dladdr");
		}
		UTF8toW(info.dli_fname, strPath);
	}else{
#ifdef PATH_UTIL_FIX_SELF_EXE
		strPath = PATH_UTIL_FIX_SELF_EXE;
#else
		char szPath[1024];
		ssize_t len = readlink("/proc/self/exe", szPath, 1024);
		if( len < 0 || len >= 1024 ){
			throw std::runtime_error("readlink");
		}
		szPath[len] = '\0';
		UTF8toW(szPath, strPath);
#endif
	}
	fs_path path(strPath);
	if( path.is_relative() || path.has_filename() == false ){
		throw std::runtime_error("");
	}
	return path;
}

fs_path GetModuleIniPath(void* funcAddr)
{
	return fs_path(EDCB_INI_ROOT).append(GetModulePath(funcAddr).filename().native()).concat(L".ini");
}
#endif

fs_path GetCommonIniPath()
{
#ifdef _WIN32
	return GetModulePath().replace_filename(L"Common.ini");
#else
	return fs_path(EDCB_INI_ROOT).append(L"Common.ini");
#endif
}

fs_path GetRecFolderPath(int index)
{
	fs_path iniPath = GetCommonIniPath();
	int num = GetPrivateProfileInt(L"SET", L"RecFolderNum", 0, iniPath.c_str());
	if( index <= 0 ){
		// 必ず返す
		fs_path path;
		if( num > 0 ){
			path = GetPrivateProfileToString(L"SET", L"RecFolderPath0", L"", iniPath.c_str());
		}
		return path.empty() ? GetSettingPath() : path;
	}
	for( int i = 1; i < num; i++ ){
		WCHAR szKey[32];
		swprintf_s(szKey, L"RecFolderPath%d", i);
		fs_path path = GetPrivateProfileToString(L"SET", szKey, L"", iniPath.c_str());
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

int UtilComparePath(LPCWSTR path1, LPCWSTR path2)
{
#ifdef _WIN32
	return CompareNoCase(path1, path2);
#else
	return wcscmp(path1, path2);
#endif
}

bool UtilPathEndsWith(LPCWSTR path, LPCWSTR suffix)
{
	size_t n = wcslen(path);
	size_t m = wcslen(suffix);
	return n >= m && UtilComparePath(path + n - m, suffix) == 0;
}

void CheckFileName(wstring& fileName, bool noChkYen)
{
#ifdef _WIN32
	static const WCHAR s[10] = L"\\/:*?\"<>|";
	static const WCHAR r[10] = L"￥／：＊？”＜＞｜";
#else
	static const WCHAR s[2] = L"/";
	static const WCHAR r[2] = L"／";
#endif
	// トリム
	size_t j = fileName.find_last_not_of(L' ');
	fileName.erase(j == wstring::npos ? 0 : j + 1);
	fileName.erase(0, fileName.find_first_not_of(L' '));
	for( size_t i = 0; i < fileName.size(); i++ ){
		if( (L'\x1' <= fileName[i] && fileName[i] <= L'\x1f') || fileName[i] == L'\x7f' ){
			// 制御文字
			fileName[i] = L'〓';
		}
		// ".\"("./")となるときはnoChkYenでも全角に
		const WCHAR* p = wcschr(s, fileName[i]);
		if( p && (noChkYen == false || *p != fs_path::preferred_separator || (i > 0 && fileName[i - 1] == L'.')) ){
			fileName[i] = r[p - s];
		}
	}
	// 冒頭'\'('/')はトリム
	fileName.erase(0, fileName.find_first_not_of(fs_path::preferred_separator));
	// すべて'.'のときは全角に
	if( fileName.find_first_not_of(L'.') == wstring::npos ){
		for( size_t i = 0; i < fileName.size(); fileName[i++] = L'．' );
	}
}

#ifdef _WIN32
void TouchFileAsUnicode(const fs_path& path)
{
	std::unique_ptr<FILE, fclose_deleter> fp(UtilOpenFile(path, UTIL_O_EXCL_CREAT_WRONLY));
	if( fp ){
		fputwc(L'\xFEFF', fp.get());
	}
}
#endif

pair<bool, bool> UtilFileExists(const fs_path& path, bool* mightExist)
{
#ifdef _WIN32
	DWORD attr = GetFileAttributes(path.c_str());
	if( attr != INVALID_FILE_ATTRIBUTES ){
		return std::make_pair(true, (attr & FILE_ATTRIBUTE_DIRECTORY) == 0);
	}
	if( mightExist ){
		DWORD err = GetLastError();
		*mightExist = (err != ERROR_FILE_NOT_FOUND && err != ERROR_PATH_NOT_FOUND);
	}
#else
	string strPath;
	WtoUTF8(path.native(), strPath);
	struct stat st;
	if( stat(strPath.c_str(), &st) == 0 ){
		return std::make_pair(true, S_ISDIR(st.st_mode) == 0);
	}
	if( mightExist ){
		*mightExist = errno != ENOENT;
	}
#endif
	return std::make_pair(false, false);
}

bool UtilCreateDirectory(const fs_path& path)
{
#ifdef _WIN32
	return CreateDirectory(path.c_str(), NULL) != FALSE;
#else
	string strPath;
	WtoUTF8(path.native(), strPath);
	return mkdir(strPath.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) == 0;
#endif
}

bool UtilCreateDirectories(const fs_path& path)
{
	if( path.empty() || path.is_relative() && path.has_root_path() ){
		// パスが不完全
		return false;
	}
	bool mightExist = false;
	if( UtilFileExists(path, &mightExist).first ){
		// 既存
		return false;
	}
	if( mightExist ){
		// 特殊な理由
		AddDebugLog(L"UtilCreateDirectories(): Error");
		return false;
	}
	UtilCreateDirectories(path.parent_path());
	return UtilCreateDirectory(path);
}

LONGLONG UtilGetStorageFreeBytes(const fs_path& directoryPath)
{
#ifdef _WIN32
	ULARGE_INTEGER li;
	return GetDiskFreeSpaceEx(UtilGetStorageID(directoryPath).c_str(), &li, NULL, NULL) ? (LONGLONG)li.QuadPart : -1;
#else
	if( directoryPath.empty() || (directoryPath.is_relative() && directoryPath.has_root_path()) ){
		// パスが不完全
		return -1;
	}
	bool mightExist = false;
	if( UtilFileExists(directoryPath, &mightExist).first ){
		string strPath;
		WtoUTF8(directoryPath.native(), strPath);
		struct statvfs st;
		return statvfs(strPath.c_str(), &st) == 0 ? (LONGLONG)st.f_frsize * (LONGLONG)st.f_bavail : -1;
	}
	if( mightExist ){
		// 特殊な理由
		AddDebugLog(L"UtilGetStorageFreeBytes(): Error");
		return -1;
	}
	return UtilGetStorageFreeBytes(directoryPath.parent_path());
#endif
}

wstring UtilGetStorageID(const fs_path& directoryPath)
{
#ifdef _WIN32
	WCHAR szVolumePathName[MAX_PATH];
	if( GetVolumePathName(directoryPath.c_str(), szVolumePathName, MAX_PATH) == FALSE ){
		return directoryPath.native();
	}
	WCHAR szMount[MAX_PATH];
	if( GetVolumeNameForVolumeMountPoint(szVolumePathName, szMount, MAX_PATH) == FALSE ){
		return szVolumePathName;
	}
	return szMount;
#else
	wstring ret;
	if( directoryPath.empty() || (directoryPath.is_relative() && directoryPath.has_root_path()) ){
		// パスが不完全
		return ret;
	}
	bool mightExist = false;
	if( UtilFileExists(directoryPath, &mightExist).first ){
		string strPath;
		WtoUTF8(directoryPath.native(), strPath);
		struct stat st;
		if( stat(strPath.c_str(), &st) == 0 ){
			Format(ret, L"%08X", (DWORD)st.st_dev);
		}
		return ret;
	}
	if( mightExist ){
		// 特殊な理由
		AddDebugLog(L"UtilGetStorageID(): Error");
		return ret;
	}
	return UtilGetStorageID(directoryPath.parent_path());
#endif
}

namespace
{
void CloseGlobalMutex(void* p)
{
#ifdef _WIN32
	CloseHandle(p);
#else
	std::unique_ptr<pair<FILE*, string>> f((pair<FILE*, string>*)p);
	remove(f->second.c_str());
	fclose(f->first);
#endif
}
}

util_unique_handle UtilCreateGlobalMutex(LPCWSTR name, bool* alreadyExists)
{
	if( alreadyExists ){
		*alreadyExists = false;
	}
#ifdef _WIN32
	HANDLE h = NULL;
	if( name && wcslen(name) < MAX_PATH - 7 ){
		WCHAR sz[MAX_PATH];
		swprintf_s(sz, L"Global\\%ls", name);
		h = CreateMutex(NULL, FALSE, sz);
		if( h && GetLastError() == ERROR_ALREADY_EXISTS ){
			if( alreadyExists ){
				*alreadyExists = true;
			}
			CloseHandle(h);
			h = NULL;
		}
	}
	return util_unique_handle(h, CloseGlobalMutex);
#else
	std::unique_ptr<pair<FILE*, string>> f;
	if( name ){
		f.reset(new pair<FILE*, string>());
		fs_path path = fs_path(EDCB_INI_ROOT).append(L"Global_").concat(name);
		WtoUTF8(path.native(), f->second);
		int apiError;
		f->first = UtilOpenFile(path, UTIL_SECURE_WRITE, &apiError);
		struct stat st[2];
		if( f->first == NULL ){
			if( alreadyExists ){
				*alreadyExists = apiError == EAGAIN;
			}
			f.reset();
		}else if( fstat(fileno(f->first), st) != 0 ){
			fclose(f->first);
			f.reset();
		}else if( stat(f->second.c_str(), st + 1) != 0 || st[0].st_ino != st[1].st_ino ){
			// 競合状態
			if( alreadyExists ){
				*alreadyExists = true;
			}
			fclose(f->first);
			f.reset();
		}
	}
	return util_unique_handle(f.release(), CloseGlobalMutex);
#endif
}

#ifndef _WIN32
int GetPrivateProfileInt(LPCWSTR appName, LPCWSTR keyName, int nDefault, LPCWSTR fileName)
{
	wstring ret = GetPrivateProfileToString(appName, keyName, L"", fileName);
	LPWSTR endp;
	int n = (int)wcstol(ret.c_str(), &endp, 10);
	return endp != ret.c_str() ? n : nDefault;
}

BOOL WritePrivateProfileString(LPCWSTR appName, LPCWSTR keyName, LPCWSTR lpString, LPCWSTR fileName)
{
	for( int retry = 0;; ){
		std::unique_ptr<FILE, fclose_deleter> fp(UtilOpenFile(wstring(fileName), UTIL_O_RDWR));
		if( !fp ){
			fp.reset(UtilOpenFile(wstring(fileName), UTIL_O_EXCL_CREAT_RDWR));
		}
		if( fp ){
			string app, key, val, line, buff;
			WtoUTF8(L'[' + wstring(appName) + L']', app);
			WtoUTF8(keyName ? keyName : L"", key);
			WtoUTF8(lpString ? lpString : L"", val);
			bool isApp = false;
			bool hasApp = false;
			bool hasKey = false;
			int c;
			do{
				c = fgetc(fp.get());
				c = c >= 0 && (char)c ? c : -1;
				if( c >= 0 && (char)c != '\n' ){
					line += (char)c;
					continue;
				}
				size_t n = line.find_last_not_of("\t\r ");
				line.erase(n == string::npos ? 0 : n + 1);
				line.erase(0, line.find_first_not_of("\t\r "));
				bool isKey = false;
				if( CompareNoCase(app, line) == 0 ){
					hasApp = isApp = true;
				}else if( line[0] == '[' ){
					if( keyName && lpString && isApp && hasKey == false ){
						buff += key + '=' + val + '\n';
						hasKey = true;
					}
					isApp = false;
				}else if( isApp && keyName && (n = line.find('=')) != string::npos ){
					size_t m = line.find_last_not_of("\t\r =", n);
					m = (m == string::npos ? 0 : m + 1);
					char d = line[m];
					line[m] = '\0';
					isKey = CompareNoCase(key, line.c_str()) == 0;
					hasKey = hasKey || isKey;
					line[m] = d;
					if( isKey && lpString ){
						line.replace(n + 1, string::npos, val);
					}
				}
				if( (isApp == false || keyName) && (isKey == false || lpString) && (c >= 0 || line.empty() == false) ){
					buff += line;
					buff += '\n';
				}
				line.clear();
			}while( c >= 0 );

			if( keyName && lpString ){
				if( hasApp == false ){
					buff += app + '\n';
				}
				if( hasKey == false ){
					buff += key + '=' + val + '\n';
				}
			}
			if( ftruncate(fileno(fp.get()), 0) == 0 ){
				rewind(fp.get());
				if( fwrite(buff.c_str(), 1, buff.size(), fp.get()) == buff.size() && fflush(fp.get()) == 0 ){
					return TRUE;
				}
			}
			AddDebugLog(L"WritePrivateProfileString(): Error");
			break;
		}else if( ++retry > 1000 ){
			AddDebugLog(L"WritePrivateProfileString(): Error: Cannot open file");
			break;
		}
		SleepForMsec(10);
	}
	return FALSE;
}
#endif

wstring GetPrivateProfileToString(LPCWSTR appName, LPCWSTR keyName, LPCWSTR lpDefault, LPCWSTR fileName)
{
#ifdef _WIN32
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
#else
	for( int retry = 0; appName && keyName; ){
		bool mightExist = false;
		std::unique_ptr<FILE, fclose_deleter> fp(UtilOpenFile(wstring(fileName), UTIL_SECURE_READ));
		if( fp ){
			string app, key, line;
			WtoUTF8(L'[' + wstring(appName) + L']', app);
			WtoUTF8(keyName, key);
			bool isApp = false;
			int c;
			do{
				c = fgetc(fp.get());
				c = c >= 0 && (char)c ? c : -1;
				if( c >= 0 && (char)c != '\n' ){
					line += (char)c;
					continue;
				}
				size_t n = line.find_last_not_of("\t\r ");
				line.erase(n == string::npos ? 0 : n + 1);
				line.erase(0, line.find_first_not_of("\t\r "));
				if( CompareNoCase(app, line) == 0 ){
					isApp = true;
				}else if( line[0] == '[' ){
					isApp = false;
				}else if( isApp && (n = line.find('=')) != string::npos ){
					size_t m = line.find_last_not_of("\t\r =", n);
					line[m == string::npos ? 0 : m + 1] = '\0';
					if( CompareNoCase(key, line.c_str()) == 0 ){
						line.erase(0, line.find_first_not_of("\t\r ", n + 1));
						if( line.size() > 1 && (line[0] == '"' || line[0] == '\'') && line[0] == line.back() ){
							line.pop_back();
							line.erase(line.begin());
						}
						wstring ret;
						UTF8toW(line, ret);
						return ret;
					}
				}
				line.clear();
			}while( c >= 0 );
			break;
		}else if( UtilFileExists(fileName, &mightExist).first == false && mightExist == false ){
			break;
		}else if( ++retry > 1000 ){
			AddDebugLog(L"GetPrivateProfileToString(): Error: Cannot open file");
			break;
		}
		SleepForMsec(10);
	}
	return lpDefault ? lpDefault : L"";
#endif
}

BOOL WritePrivateProfileInt(LPCWSTR appName, LPCWSTR keyName, int value, LPCWSTR fileName)
{
	WCHAR sz[32];
	swprintf_s(sz, L"%d", value);
	return WritePrivateProfileString(appName, keyName, sz, fileName);
}

void EnumFindFile(const fs_path& pattern, const std::function<bool(UTIL_FIND_DATA&)>& enumProc)
{
#ifdef _WIN32
	WIN32_FIND_DATA findData;
	HANDLE hFind = FindFirstFile(pattern.c_str(), &findData);
	if( hFind != INVALID_HANDLE_VALUE ){
		try{
			UTIL_FIND_DATA ufd;
			do{
				ufd.isDir = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
				ufd.lastWriteTime = (LONGLONG)findData.ftLastWriteTime.dwHighDateTime << 32 | findData.ftLastWriteTime.dwLowDateTime;
				ufd.fileSize = (LONGLONG)findData.nFileSizeHigh << 32 | findData.nFileSizeLow;
				ufd.fileName = findData.cFileName;
			}while( enumProc(ufd) && FindNextFile(hFind, &findData) );
		}catch(...){
			FindClose(hFind);
			throw;
		}
		FindClose(hFind);
	}
#else
	string strPath;
	fs_path pat = pattern.filename();
	if( pat.native().find_first_of(L"*?") == wstring::npos ){
		// 完全一致
		WtoUTF8(pattern.native(), strPath);
		struct stat st;
		if( stat(strPath.c_str(), &st) == 0 ){
			UTIL_FIND_DATA ufd;
			ufd.isDir = S_ISDIR(st.st_mode) != 0;
			ufd.lastWriteTime = (LONGLONG)st.st_mtime * 10000000 + 116444736000000000;
			ufd.fileSize = (LONGLONG)st.st_size;
			ufd.fileName = pat.native();
			enumProc(ufd);
		}
	}else{
		// ワイルドカード('*'3個まで)
		fs_path parent = pattern.parent_path();
		WtoUTF8(parent.native(), strPath);
		DIR* dir = opendir(strPath.c_str());
		if( dir ){
			try{
				UTIL_FIND_DATA ufd;
				dirent* ent;
				while( (ent = readdir(dir)) != NULL ){
					UTF8toW(ent->d_name, ufd.fileName);
					LPCWSTR p[4] = {pat.c_str(), NULL, NULL};
					LPCWSTR t[4] = {ufd.fileName.c_str(), NULL, NULL};
					size_t i = 0;
					while( *p[i] || *t[i] ){
						if( *p[i] == L'*' && (!p[i][1] || p[i][1] == L'*' || *t[i]) ){
							if( i == 3 ){
								break;
							}
							p[i + 1] = p[i] + 1;
							t[i + 1] = t[i];
							i++;
						}else if( *p[i] != L'*' && *t[i] && (*p[i] == L'?' || *p[i] == *t[i]) ){
							p[i]++;
							t[i]++;
						}else{
							if( i == 0 ){
								break;
							}
							t[--i]++;
						}
					}
					if( !*p[i] && !*t[i] ){
						WtoUTF8(fs_path(parent).append(ufd.fileName).native(), strPath);
						struct stat st;
						if( stat(strPath.c_str(), &st) == 0 ){
							ufd.isDir = S_ISDIR(st.st_mode) != 0;
							ufd.lastWriteTime = (LONGLONG)st.st_mtime * 10000000 + 116444736000000000;
							ufd.fileSize = (LONGLONG)st.st_size;
							if( enumProc(ufd) == false ){
								break;
							}
						}
					}
				}
			}catch(...){
				closedir(dir);
				throw;
			}
			closedir(dir);
		}
	}
#endif
}
