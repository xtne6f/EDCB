#include "StdAfx.h"
#include "HttpServer.h"
#include "../../Common/StringUtil.h"
#include "../../Common/PathUtil.h"
#include "../../Common/ParseTextInstances.h"
#include "civetweb.h"
#include "resource.h"

#define LUA_DLL_NAME L"lua52.dll"

//以下のパスはリソースにリダイレクト
static const struct {
	const char* path;
	int res;
} PATH_RES_MAP[] = {
	{ "\\api\\AddReserveEPG", IDR_LUA_ADD_RESERVE_EPG },
	{ "\\api\\EnumEventInfo", IDR_LUA_ENUM_EVENT_INFO },
	{ "\\api\\EnumRecPreset", IDR_LUA_ENUM_REC_PRESET },
	{ "\\api\\EnumReserveInfo", IDR_LUA_ENUM_RESERVE_INFO },
	{ "\\api\\EnumService", IDR_LUA_ENUM_SERVICE },
	{ "\\api\\SearchEvent", IDR_LUA_SEARCH_EVENT },
};

CHttpServer::CHttpServer()
	: mgContext(NULL)
	, hLuaDll(NULL)
{
}

CHttpServer::~CHttpServer()
{
	StopServer();
}

bool CHttpServer::StartServer(unsigned short port, LPCWSTR rootPath_, int (*initProc)(lua_State*), void* initParam, bool saveLog, LPCWSTR acl)
{
	StopServer();

	//LuaのDLLが無いとき分かりにくいタイミングでエラーになるので事前に読んでおく(必須ではない)
	this->hLuaDll = LoadLibrary(LUA_DLL_NAME);
	if( this->hLuaDll == NULL ){
		OutputDebugString(L"CHttpServer::StartServer(): " LUA_DLL_NAME L" not found.\r\n");
		return false;
	}
	string strPort;
	Format(strPort, "%d", port);
	WtoUTF8(rootPath_, this->rootPath);
	ChkFolderPath(this->rootPath);
	//パスにASCII範囲外を含むのは(主にLuaが原因で)難ありなので蹴る
	for( size_t i = 0; i < this->rootPath.size(); i++ ){
		if( this->rootPath[i] & 0x80 ){
			OutputDebugString(L"CHttpServer::StartServer(): path has multibyte.\r\n");
			return false;
		}
	}
	wstring modulePath;
	GetModuleFolderPath(modulePath);
	string accessLogPath;
	//ログはfopen()されるのでWtoA()。civetweb.cのACCESS_LOG_FILEとERROR_LOG_FILEの扱いに注意
	WtoA(modulePath, accessLogPath);
	string errorLogPath = accessLogPath + "\\HttpError.log";
	accessLogPath += "\\HttpAccess.log";

	//Access Control List
	string aclU;
	WtoUTF8(acl ? acl : L"", aclU);
	aclU = "-0.0.0.0/0," + aclU;

	//追加のMIMEタイプ
	CParseContentTypeText contentType;
	contentType.ParseText((modulePath + L"\\ContentTypeText.txt").c_str());
	wstring extraMimeW;
	for( map<wstring, wstring>::const_iterator itr = contentType.GetMap().begin(); itr != contentType.GetMap().end(); itr++ ){
		extraMimeW += itr->first + L'=' + itr->second + L',';
	}
	string extraMime;
	WtoUTF8(extraMimeW, extraMime);

	this->redirectList.clear();
	for( size_t i = 0; i < _countof(PATH_RES_MAP); i++ ){
		HRSRC hResInfo = FindResource(NULL, MAKEINTRESOURCE(PATH_RES_MAP[i].res), L"LUA");
		if( hResInfo ){
			REDIRECT_ITEM item;
			item.path = this->rootPath + PATH_RES_MAP[i].path;
			item.dataLen = SizeofResource(NULL, hResInfo);
			HGLOBAL hResData = LoadResource(NULL, hResInfo);
			if( hResData && (item.data = (const char*)LockResource(hResData)) != NULL ){
				this->redirectList.push_back(item);
			}
		}
	}
	const char* options[] = {
		//mg_stop()の待ちが長くなりすぎるので(残念だが)オフ
		//"enable_keep_alive", "yes",
		"access_control_list", aclU.c_str(),
		"extra_mime_types", extraMime.c_str(),
		"listening_ports", strPort.c_str(),
		"document_root", this->rootPath.c_str(),
		//必要に応じて増やしてもいい
		"num_threads", "3",
		"lua_script_pattern", "**.lua$|**.html$",
		//TODO: open_fileコールバックは今のところ.luaを扱えないため、"*/api/*$"は.lspとみなす
		"lua_server_page_pattern", "**.lp$|**.lsp$|*/api/*$",
		saveLog ? "access_log_file" : NULL, accessLogPath.c_str(),
		"error_log_file", errorLogPath.c_str(),
		NULL,
	};
	this->initLuaProc = initProc;
	this->initLuaParam = initParam;
	mg_callbacks callbacks = {};
	callbacks.init_lua = &InitLua;
	callbacks.open_file = &OpenFile;
	this->mgContext = mg_start(&callbacks, this, options);
	return this->mgContext != NULL;
}

void CHttpServer::StopServer()
{
	if( this->mgContext ){
		mg_stop(this->mgContext);
		this->mgContext = NULL;
	}
	if( this->hLuaDll ){
		FreeLibrary(this->hLuaDll);
		this->hLuaDll = NULL;
	}
}

void CHttpServer::InitLua(const mg_connection* conn, void* luaContext)
{
	const CHttpServer* sys = (CHttpServer*)mg_get_user_data(mg_get_context(conn));
	lua_State* L = (lua_State*)luaContext;
	lua_pushlightuserdata(L, sys->initLuaParam);
	sys->initLuaProc(L);
}

const char* CHttpServer::OpenFile(const mg_connection* conn, const char* path, size_t* dataLen)
{
	const CHttpServer* sys = (CHttpServer*)mg_get_user_data(mg_get_context(conn));
	string pathB = path;
	Replace(pathB, "/", "\\");
	for( size_t i = 0; i < sys->redirectList.size(); i++ ){
		if( CompareNoCase(sys->redirectList[i].path, pathB) == 0 ){
			*dataLen = sys->redirectList[i].dataLen;
			return sys->redirectList[i].data;
		}
	}
	return NULL;
}

namespace LuaHelp
{

#if 1 //From: civetweb/mod_lua.inl
void reg_string(lua_State* L, const char* name, const char* val)
{
	lua_pushstring(L, name);
	lua_pushstring(L, val);
	lua_rawset(L, -3);
}

void reg_int(lua_State* L, const char* name, int val)
{
	lua_pushstring(L, name);
	lua_pushinteger(L, val);
	lua_rawset(L, -3);
}

void reg_boolean(lua_State* L, const char* name, bool val)
{
	lua_pushstring(L, name);
	lua_pushboolean(L, val);
	lua_rawset(L, -3);
}

void reg_function(lua_State* L, const char* name, lua_CFunction func, void* userdata)
{
	lua_pushstring(L, name);
	lua_pushlightuserdata(L, userdata);
	lua_pushcclosure(L, func, 1);
	lua_rawset(L, -3);
}
#endif

void reg_time(lua_State* L, const char* name, const SYSTEMTIME &st)
{
	lua_pushstring(L, name);
	lua_newtable(L);
	reg_int(L, "year", st.wYear);
	reg_int(L, "month", st.wMonth);
	reg_int(L, "day", st.wDay);
	reg_int(L, "hour", st.wHour);
	reg_int(L, "min", st.wMinute);
	reg_int(L, "sec", st.wSecond);
	reg_int(L, "msec", st.wMilliseconds);
	reg_int(L, "wday", st.wDayOfWeek + 1);
	reg_boolean(L, "isdst", 0);
	lua_rawset(L, -3);
}

bool isnil(lua_State* L, const char* name)
{
	lua_getfield(L, -1, name);
	bool ret = lua_isnil(L, -1);
	lua_pop(L, 1);
	return ret;
}

string get_string(lua_State* L, const char* name)
{
	lua_getfield(L, -1, name);
	const char* p = lua_tostring(L, -1);
	string ret = p ? p : "";
	lua_pop(L, 1);
	return ret;
}

int get_int(lua_State* L, const char* name)
{
	lua_getfield(L, -1, name);
	int ret = (int)lua_tointeger(L, -1);
	lua_pop(L, 1);
	return ret;
}

bool get_boolean(lua_State* L, const char* name)
{
	lua_getfield(L, -1, name);
	bool ret = lua_toboolean(L, -1) != 0;
	lua_pop(L, 1);
	return ret;
}

SYSTEMTIME get_time(lua_State* L, const char* name)
{
	SYSTEMTIME st = {};
	lua_getfield(L, -1, name);
	if( lua_istable(L, -1) ){
		st.wYear = (WCHAR)get_int(L, "year");
		st.wMonth = (WCHAR)get_int(L, "month");
		st.wDay = (WCHAR)get_int(L, "day");
		st.wHour = (WCHAR)get_int(L, "hour");
		st.wMinute = (WCHAR)get_int(L, "min");
		st.wSecond = (WCHAR)get_int(L, "sec");
		st.wMilliseconds = (WCHAR)get_int(L, "msec");
		st.wDayOfWeek = (WORD)(get_int(L, "wday") - 1);
	}
	lua_pop(L, 1);
	return st;
}

}
