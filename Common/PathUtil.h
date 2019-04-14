#ifndef INCLUDE_PATH_UTIL_H
#define INCLUDE_PATH_UTIL_H

namespace filesystem_
{
// Extract from Boost.Filesystem(1.64.0) www.boost.org/libs/filesystem
// "Use, modification, and distribution are subject to the Boost Software License, Version 1.0. See www.boost.org/LICENSE_1_0.txt"
class path
{
public:
	//  -----  constructors  -----
	path() {}
	path(const wchar_t* s) : m_pathname(s) {}
	path(const wstring& s) : m_pathname(s) {}
	//  -----  concatenation  -----
	path& concat(const wchar_t* source) { m_pathname += source; return *this; }
	path& concat(const wstring& source) { m_pathname += source; return *this; }
	//  -----  appends  -----
	path& append(const wchar_t* source);
	path& append(const wstring& source);
	//  -----  modifiers  -----
	void clear() { m_pathname.clear(); }
	path& remove_filename();
	path& replace_filename(const path& p) { return remove_filename().append(p.native()); }
	path& replace_extension(const path& new_extension = path());
	//  -----  observers  -----
	const wstring& native() const { return m_pathname; }
	const wchar_t* c_str() const { return m_pathname.c_str(); }
	//  -----  decomposition  -----
	path root_path() const { return root_name().concat(root_directory().native()); }
	path root_name() const;
	path root_directory() const;
	path parent_path() const;
	path filename() const;
	path stem() const;
	path extension() const { return path(filename().c_str() + stem().native().size()); }
	//  -----  query  -----
	bool empty() const { return m_pathname.empty(); }
	bool has_root_path() const { return has_root_directory() || has_root_name(); }
	bool has_root_name() const { return !root_name().empty(); }
	bool has_root_directory() const { return !root_directory().empty(); }
	bool has_parent_path() const { return !parent_path().empty(); }
	bool has_filename() const { return !m_pathname.empty(); }
	bool has_stem() const { return !stem().empty(); }
	bool has_extension() const { return !extension().empty(); }
	bool is_relative() const { return !is_absolute(); }
	bool is_absolute() const { return has_root_name() && has_root_directory(); }
private:
	wstring m_pathname; // Windows: as input; backslashes NOT converted to slashes,
	                    // slashes NOT converted to backslashes
	size_t m_append_separator_if_needed();
	void m_erase_redundant_separator(size_t sep_pos);
	size_t m_parent_path_end() const;
	static bool is_root_separator(const wstring& str, size_t pos);
	static size_t filename_pos(const wstring& str);
	static size_t root_directory_start(const wstring& str, size_t size);
	static void first_element(const wstring& src, size_t& element_pos, size_t& element_size);
	static bool is_dir_sep(wchar_t c) { return c == L'/' || c == L'\\'; }
	static bool is_letter(wchar_t c) { return (c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z'); }
};
}

typedef filesystem_::path fs_path;
//#include <filesystem>
//typedef std::experimental::filesystem::path fs_path;

fs_path GetDefSettingPath();
fs_path GetSettingPath();
void GetModuleFolderPath(wstring& strPath);
fs_path GetModulePath(HMODULE hModule = NULL);
fs_path GetPrivateProfileToFolderPath(LPCWSTR appName, LPCWSTR keyName, LPCWSTR fileName);
fs_path GetModuleIniPath(HMODULE hModule = NULL);
fs_path GetCommonIniPath();
fs_path GetRecFolderPath(int index = 0);
BOOL IsExt(const fs_path& filePath, const WCHAR* ext);
void CheckFileName(wstring& fileName, BOOL noChkYen = FALSE);
void ChkFolderPath(fs_path& path);

// 存在しなければBOM付きの空ファイルを作成する
void TouchFileAsUnicode(LPCWSTR path);
// 再帰的にディレクトリを生成する
BOOL UtilCreateDirectories(const fs_path& path);
// フォルダパスから実際のドライブパスを取得
wstring GetChkDrivePath(const wstring& directoryPath);
// 必要なバッファを確保してGetPrivateProfileSection()を呼ぶ
vector<WCHAR> GetPrivateProfileSectionBuffer(LPCWSTR appName, LPCWSTR fileName);
wstring GetPrivateProfileToString(LPCWSTR appName, LPCWSTR keyName, LPCWSTR lpDefault, LPCWSTR fileName);
BOOL WritePrivateProfileInt(LPCWSTR appName, LPCWSTR keyName, int value, LPCWSTR fileName);

// FindFirstFile()の結果を列挙する
template<class P>
void EnumFindFile(LPCWSTR pattern, P enumProc)
{
	WIN32_FIND_DATA findData;
	HANDLE hFind = FindFirstFile(pattern, &findData);
	if( hFind != INVALID_HANDLE_VALUE ){
		try{
			while( enumProc(findData) && FindNextFile(hFind, &findData) );
		}catch(...){
			FindClose(hFind);
			throw;
		}
		FindClose(hFind);
	}
}

#endif
