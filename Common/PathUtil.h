#ifndef INCLUDE_PATH_UTIL_H
#define INCLUDE_PATH_UTIL_H

#include <functional>

#ifdef _WIN32
#define EDCB_LIB_EXT L".dll"
#else
#ifndef EDCB_INI_ROOT
#define EDCB_INI_ROOT L"/var/local/edcb"
#endif
#ifndef EDCB_LIB_ROOT
#define EDCB_LIB_ROOT L"/usr/local/lib/edcb"
#endif
#ifndef EDCB_LIB_EXT
#define EDCB_LIB_EXT L".so"
#endif
#endif

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
#ifdef _WIN32
	bool is_absolute() const { return has_root_name() && has_root_directory(); }
	static const wchar_t preferred_separator = L'\\';
#else
	bool is_absolute() const { return has_root_directory(); }
	static const wchar_t preferred_separator = L'/';
#endif
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
#ifdef _WIN32
	static const wchar_t* separators() { return L"/\\"; }
	static bool is_dir_sep(wchar_t c) { return c == L'/' || c == L'\\'; }
	static bool is_letter(wchar_t c) { return (c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z'); }
#else
	static const wchar_t* separators() { return L"/"; }
	static bool is_dir_sep(wchar_t c) { return c == L'/'; }
#endif
};
}

typedef filesystem_::path fs_path;
//#include <filesystem>
//typedef std::experimental::filesystem::path fs_path;

typedef std::unique_ptr<void, void (*)(void*)> util_unique_handle;

enum {
	UTIL_O_RDONLY = 1, // r
	UTIL_O_RDWR = 3, // r+
	UTIL_O_CREAT_WRONLY = 10, // w
	UTIL_O_CREAT_RDWR = 11, // w+
	UTIL_O_CREAT_APPEND = 22, // a
	UTIL_O_EXCL_CREAT_WRONLY = 26, // wx
	UTIL_O_EXCL_CREAT_RDWR = 27, // w+x
	UTIL_O_EXCL_CREAT_APPEND = 30, // ax
	UTIL_SH_READ = 32,
	UTIL_SH_DELETE = 128,
	UTIL_F_SEQUENTIAL = 256, // S
	UTIL_F_IONBF = 512, // setvbuf(_IONBF)
	UTIL_SECURE_WRITE = UTIL_O_CREAT_WRONLY, // fopen_s(w)
	UTIL_SECURE_READ = UTIL_O_RDONLY | UTIL_SH_READ, // fopen_s(r)
	UTIL_SHARED_READ = UTIL_O_RDONLY | UTIL_SH_READ | 64, // fopen(r)
};

// ファイルを開く(継承不能、共有モード制御可)
FILE* UtilOpenFile(const wstring& path, int flags, int* apiError = NULL);
inline FILE* UtilOpenFile(const fs_path& path, int flags, int* apiError = NULL) { return UtilOpenFile(path.native(), flags, apiError); }

// 共有ライブラリをロードする
void* UtilLoadLibrary(const wstring& path);
inline void* UtilLoadLibrary(const fs_path& path) { return UtilLoadLibrary(path.native()); }

// 共有ライブラリを解放する
void UtilFreeLibrary(void* hModule);

// 共有ライブラリのエクスポート関数や変数のアドレスを取得する
void* UtilGetProcAddress(void* hModule, const char* name);
template<class T>
bool UtilGetProcAddress(void* hModule, const char* name, T& proc) { return (proc = (T)UtilGetProcAddress(hModule, name)) != NULL; }

#ifndef _WIN32
BOOL DeleteFile(LPCWSTR path);
#endif

fs_path GetDefSettingPath();
fs_path GetSettingPath();
#ifdef _WIN32
fs_path GetModulePath(HMODULE hModule = NULL);
fs_path GetModuleIniPath(HMODULE hModule = NULL);
#else
fs_path GetModulePath(void* funcAddr = NULL);
fs_path GetModuleIniPath(void* funcAddr = NULL);
#endif
fs_path GetCommonIniPath();
fs_path GetRecFolderPath(int index = 0);
int UtilComparePath(LPCWSTR path1, LPCWSTR path2);
bool UtilPathEndsWith(LPCWSTR path, LPCWSTR suffix);
void CheckFileName(wstring& fileName, bool noChkYen = false);

#ifdef _WIN32
// 存在しなければBOM付きの空ファイルを作成する
void TouchFileAsUnicode(const fs_path& path);
#endif
// ファイルの存在を確認する。それがディレクトリでなければ第2返値もtrue
pair<bool, bool> UtilFileExists(const fs_path& path, bool* mightExist = NULL);
bool UtilCreateDirectory(const fs_path& path);
// 再帰的にディレクトリを生成する
bool UtilCreateDirectories(const fs_path& path);
// フォルダがあるストレージの空き容量を取得する。失敗時は負値
LONGLONG UtilGetStorageFreeBytes(const fs_path& directoryPath);
// フォルダがあるストレージの識別子を取得する。失敗時は空
wstring UtilGetStorageID(const fs_path& directoryPath);
// グローバルミューテックスを生成する。失敗時(既に存在するときを含む)やname未指定のときは空のハンドルを返す
util_unique_handle UtilCreateGlobalMutex(LPCWSTR name = NULL, bool* alreadyExists = NULL);

#ifndef _WIN32
int GetPrivateProfileInt(LPCWSTR appName, LPCWSTR keyName, int nDefault, LPCWSTR fileName);
BOOL WritePrivateProfileString(LPCWSTR appName, LPCWSTR keyName, LPCWSTR lpString, LPCWSTR fileName);
#endif
wstring GetPrivateProfileToString(LPCWSTR appName, LPCWSTR keyName, LPCWSTR lpDefault, LPCWSTR fileName);
BOOL WritePrivateProfileInt(LPCWSTR appName, LPCWSTR keyName, int value, LPCWSTR fileName);

struct UTIL_FIND_DATA {
	bool isDir;
	LONGLONG lastWriteTime;
	LONGLONG fileSize;
	wstring fileName;
};

// FindFirstFile()の結果を列挙する
void EnumFindFile(const fs_path& pattern, const std::function<bool(UTIL_FIND_DATA&)>& enumProc);

#endif
