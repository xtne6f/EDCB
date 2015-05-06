#pragma once
#include "lua.hpp"
#include <string>
#include <vector>

struct mg_context;
struct mg_connection;

class CHttpServer
{
public:
	CHttpServer();
	~CHttpServer();
	bool StartServer(unsigned short port, LPCWSTR rootPath_, int (*initProc)(lua_State*), void* initParam, bool saveLog = false, LPCWSTR acl = NULL);
	void StopServer();
private:
	struct REDIRECT_ITEM {
		std::string path;
		const char* data;
		size_t dataLen;
	};
	static void InitLua(const mg_connection* conn, void* luaContext);
	static const char* OpenFile(const mg_connection* conn, const char* path, size_t* dataLen);
	mg_context* mgContext;
	std::string rootPath;
	int (*initLuaProc)(lua_State*);
	void* initLuaParam;
	std::vector<REDIRECT_ITEM> redirectList;
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
