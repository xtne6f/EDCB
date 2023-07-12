#pragma once
#include "lua/lua.hpp"
#include "UpnpSsdpServer.h"
#include <functional>

#define LUA_DLL_NAME L"lua52.dll"

struct mg_context;
struct mg_connection;

class CHttpServer
{
public:
	struct SERVER_OPTIONS {
		wstring ports;
		wstring rootPath;
		wstring accessControlList;
		wstring authenticationDomain;
		wstring sslCipherList;
		int numThreads;
		int requestTimeout;
		int sslProtocolVersion;
		bool keepAlive;
		bool saveLog;
		bool enableSsdpServer;
		int ssdpIfTypes;
		int ssdpInitialWaitSec;
	};
	CHttpServer();
	~CHttpServer();
	bool StartServer(const SERVER_OPTIONS& op, const std::function<void(lua_State*)>& initProc);
	bool StopServer(bool checkOnly = false);
	static SERVER_OPTIONS LoadServerOptions(LPCWSTR iniPath);
	static string CreateRandom();
private:
	static void InitLua(const mg_connection* conn, void* luaContext, unsigned int contextFlags);
	mg_context* mgContext;
	std::function<void(lua_State*)> initLuaProc;
	HMODULE hLuaDll;
	bool initedLibrary;
	CUpnpSsdpServer upnpSsdpServer;
};

namespace LuaHelp
{
	void reg_string_(lua_State* L, const char* name, size_t size, const char* val);
	void reg_int_(lua_State* L, const char* name, size_t size, int val);
	void reg_int64_(lua_State* L, const char* name, size_t size, LONGLONG val);
	void reg_boolean_(lua_State* L, const char* name, size_t size, bool val);
	void reg_time_(lua_State* L, const char* name, size_t size, const SYSTEMTIME& st);
	template<size_t size> inline void reg_string(lua_State* L, const char(&name)[size], const char* val) { reg_string_(L, name, size, val); }
	template<size_t size> inline void reg_int(lua_State* L, const char(&name)[size], int val) { reg_int_(L, name, size, val); }
	template<size_t size> inline void reg_int64(lua_State* L, const char(&name)[size], LONGLONG val) { reg_int64_(L, name, size, val); }
	template<size_t size> inline void reg_boolean(lua_State* L, const char(&name)[size], bool val) { reg_boolean_(L, name, size, val); }
	template<size_t size> inline void reg_time(lua_State* L, const char(&name)[size], const SYSTEMTIME& st) { reg_time_(L, name, size, st); }
	bool isnil(lua_State* L, const char* name);
	string get_string(lua_State* L, const char* name);
	int get_int(lua_State* L, const char* name);
	LONGLONG get_int64(lua_State* L, const char* name);
	bool get_boolean(lua_State* L, const char* name);
	SYSTEMTIME get_time(lua_State* L, const char* name);
#ifdef _WIN32
	int os_execute(lua_State* L);
	int os_remove(lua_State* L);
	int os_rename(lua_State* L);
	int io_open(lua_State* L);
	int io_popen(lua_State* L);
	void f_createmeta(lua_State* L);
#endif
}
