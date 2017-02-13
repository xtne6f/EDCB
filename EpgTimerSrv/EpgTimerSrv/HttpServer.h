#pragma once
#include "lua.hpp"
#include "UpnpSsdpServer.h"

struct mg_context;
struct mg_connection;

class CHttpServer
{
public:
	struct SERVER_OPTIONS {
		std::wstring ports;
		std::wstring rootPath;
		std::wstring accessControlList;
		std::wstring authenticationDomain;
		std::wstring sslCipherList;
		int numThreads;
		int requestTimeout;
		int sslProtocolVersion;
		bool keepAlive;
		bool saveLog;
		bool enableSsdpServer;
	};
	CHttpServer();
	~CHttpServer();
	bool StartServer(const SERVER_OPTIONS& op, int (*initProc)(lua_State*), void* initParam);
	bool StopServer(bool checkOnly = false);
private:
	static void InitLua(const mg_connection* conn, void* luaContext);
	mg_context* mgContext;
	int (*initLuaProc)(lua_State*);
	void* initLuaParam;
	HMODULE hLuaDll;
	CUpnpSsdpServer upnpSsdpServer;
};

namespace LuaHelp
{
	void reg_string_(lua_State* L, const char* name, size_t size, const char* val);
	void reg_int_(lua_State* L, const char* name, size_t size, int val);
	void reg_boolean_(lua_State* L, const char* name, size_t size, bool val);
	void reg_function_(lua_State* L, const char* name, size_t size, lua_CFunction func, void* userdata);
	void reg_time_(lua_State* L, const char* name, size_t size, const SYSTEMTIME& st);
	template<size_t size> inline void reg_string(lua_State* L, const char(&name)[size], const char* val) { reg_string_(L, name, size, val); }
	template<size_t size> inline void reg_int(lua_State* L, const char(&name)[size], int val) { reg_int_(L, name, size, val); }
	template<size_t size> inline void reg_boolean(lua_State* L, const char(&name)[size], bool val) { reg_boolean_(L, name, size, val); }
	template<size_t size> inline void reg_function(lua_State* L, const char(&name)[size], lua_CFunction func, void* userdata) { reg_function_(L, name, size, func, userdata); }
	template<size_t size> inline void reg_time(lua_State* L, const char(&name)[size], const SYSTEMTIME& st) { reg_time_(L, name, size, st); }
	bool isnil(lua_State* L, const char* name);
	std::string get_string(lua_State* L, const char* name);
	int get_int(lua_State* L, const char* name);
	bool get_boolean(lua_State* L, const char* name);
	SYSTEMTIME get_time(lua_State* L, const char* name);
}
