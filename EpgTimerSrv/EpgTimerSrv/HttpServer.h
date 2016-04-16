#pragma once
#include "lua.hpp"
#include <string>

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
		int numThreads;
		int requestTimeout;
		int sslProtocolVersion;
		bool keepAlive;
		bool saveLog;
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
};

namespace LuaHelp
{
	void reg_string(lua_State* L, const char* name, const char* val);
	void reg_int(lua_State* L, const char* name, int val);
	void reg_boolean(lua_State* L, const char* name, bool val);
	void reg_function(lua_State* L, const char* name, lua_CFunction func, void* userdata);
	void reg_time(lua_State* L, const char* name, const SYSTEMTIME &st);
	bool isnil(lua_State* L, const char* name);
	std::string get_string(lua_State* L, const char* name);
	int get_int(lua_State* L, const char* name);
	bool get_boolean(lua_State* L, const char* name);
	SYSTEMTIME get_time(lua_State* L, const char* name);
}
