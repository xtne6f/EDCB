#include "StdAfx.h"
#include "HttpServer.h"
#include "../../Common/StringUtil.h"
#include "../../Common/PathUtil.h"
#include "../../Common/ParseTextInstances.h"
#include "civetweb.h"

#define LUA_DLL_NAME L"lua52.dll"

CHttpServer::CHttpServer()
	: mgContext(NULL)
	, hLuaDll(NULL)
{
}

CHttpServer::~CHttpServer()
{
	StopServer();
}

bool CHttpServer::StartServer(const SERVER_OPTIONS& op, int (*initProc)(lua_State*), void* initParam)
{
	StopServer();

	//LuaのDLLが無いとき分かりにくいタイミングでエラーになるので事前に読んでおく(必須ではない)
	this->hLuaDll = LoadLibrary(LUA_DLL_NAME);
	if( this->hLuaDll == NULL ){
		OutputDebugString(L"CHttpServer::StartServer(): " LUA_DLL_NAME L" not found.\r\n");
		return false;
	}
	string ports;
	WtoUTF8(op.ports, ports);
	wstring rootPathW = op.rootPath;
	ChkFolderPath(rootPathW);
	string rootPathU;
	WtoUTF8(rootPathW, rootPathU);
	//パスにASCII範囲外を含むのは(主にLuaが原因で)難ありなので蹴る
	for( size_t i = 0; i < rootPathU.size(); i++ ){
		if( rootPathU[i] & 0x80 ){
			OutputDebugString(L"CHttpServer::StartServer(): path has multibyte.\r\n");
			return false;
		}
	}
	wstring modulePath;
	GetModuleFolderPath(modulePath);
	string accessLogPath;
	//ログは_wfopen()されるのでWtoUTF8()。civetweb.cのACCESS_LOG_FILEとERROR_LOG_FILEの扱いに注意
	WtoUTF8(modulePath, accessLogPath);
	string errorLogPath = accessLogPath + "\\HttpError.log";
	accessLogPath += "\\HttpAccess.log";
	string sslCertPath;
	//認証鍵は実質fopen()されるのでWtoA()
	WtoA(modulePath, sslCertPath);
	sslCertPath += "\\ssl_cert.pem";
	string sslPeerPath;
	WtoA(modulePath, sslPeerPath);
	sslPeerPath += "\\ssl_peer.pem";
	string globalAuthPath;
	//グローバルパスワードは_wfopen()されるのでWtoUTF8()
	WtoUTF8(modulePath, globalAuthPath);
	globalAuthPath += "\\glpasswd";

	//Access Control List
	string acl;
	WtoUTF8(op.accessControlList, acl);
	acl = "-0.0.0.0/0," + acl;

	string authDomain;
	WtoUTF8(op.authenticationDomain, authDomain);
	string sslCipherList;
	WtoUTF8(op.sslCipherList, sslCipherList);
	string numThreads;
	Format(numThreads, "%d", min(max(op.numThreads, 1), 50));
	string requestTimeout;
	Format(requestTimeout, "%d", max(op.requestTimeout, 1));
	string sslProtocolVersion;
	Format(sslProtocolVersion, "%d", op.sslProtocolVersion);

	//追加のMIMEタイプ
	CParseContentTypeText contentType;
	contentType.ParseText((modulePath + L"\\ContentTypeText.txt").c_str());
	wstring extraMimeW;
	for( map<wstring, wstring>::const_iterator itr = contentType.GetMap().begin(); itr != contentType.GetMap().end(); itr++ ){
		extraMimeW += itr->first + L'=' + itr->second + L',';
	}
	string extraMime;
	WtoUTF8(extraMimeW, extraMime);

	const char* options[64] = {
		"ssi_pattern", "",
		"enable_keep_alive", op.keepAlive ? "yes" : "no",
		"access_control_list", acl.c_str(),
		"extra_mime_types", extraMime.c_str(),
		"listening_ports", ports.c_str(),
		"document_root", rootPathU.c_str(),
		"num_threads", numThreads.c_str(),
		"request_timeout_ms", requestTimeout.c_str(),
		"ssl_ca_file", sslPeerPath.c_str(),
		"ssl_default_verify_paths", "no",
		"ssl_cipher_list", sslCipherList.c_str(),
		"ssl_protocol_version", sslProtocolVersion.c_str(),
		"lua_script_pattern", "**.lua$|**.html$|*/api/*$",
	};
	int opCount = 2 * 13;
	if( op.saveLog ){
		options[opCount++] = "access_log_file";
		options[opCount++] = accessLogPath.c_str();
		options[opCount++] = "error_log_file";
		options[opCount++] = errorLogPath.c_str();
	}
	if( authDomain.empty() == false ){
		options[opCount++] = "authentication_domain";
		options[opCount++] = authDomain.c_str();
	}
	if( ports.find('s') != string::npos ){
		//セキュアポートを含むので認証鍵を指定する
		options[opCount++] = "ssl_certificate";
		options[opCount++] = sslCertPath.c_str();
	}
	wstring sslPeerPathW;
	AtoW(sslPeerPath, sslPeerPathW);
	if( GetFileAttributes(sslPeerPathW.c_str()) != INVALID_FILE_ATTRIBUTES || GetLastError() != ERROR_FILE_NOT_FOUND ){
		//信頼済み証明書ファイルが「存在しないことを確信」できなければ有効にする
		options[opCount++] = "ssl_verify_peer";
		options[opCount++] = "yes";
	}
	wstring globalAuthPathW;
	UTF8toW(globalAuthPath, globalAuthPathW);
	if( GetFileAttributes(globalAuthPathW.c_str()) != INVALID_FILE_ATTRIBUTES || GetLastError() != ERROR_FILE_NOT_FOUND ){
		//グローバルパスワードは「存在しないことを確信」できなければ指定しておく
		options[opCount++] = "global_auth_file";
		options[opCount++] = globalAuthPath.c_str();
	}

	this->initLuaProc = initProc;
	this->initLuaParam = initParam;
	mg_callbacks callbacks = {};
	callbacks.init_lua = &InitLua;
	this->mgContext = mg_start(&callbacks, this, options);
	return this->mgContext != NULL;
}

bool CHttpServer::StopServer(bool checkOnly)
{
	if( this->mgContext ){
		if( checkOnly ){
			if( mg_check_stop(this->mgContext) == 0 ){
				return false;
			}
		}else{
			//正常であればmg_stop()はreqToを超えて待機することはない
			DWORD reqTo = atoi(mg_get_option(this->mgContext, "request_timeout_ms"));
			DWORD tick = GetTickCount();
			while( GetTickCount() - tick < reqTo + 10000 ){
				if( mg_check_stop(this->mgContext) ){
					this->mgContext = NULL;
					break;
				}
				Sleep(10);
			}
			if( this->mgContext ){
				OutputDebugString(L"CHttpServer::StopServer(): failed to stop service.\r\n");
			}
		}
		this->mgContext = NULL;
	}
	if( this->hLuaDll ){
		FreeLibrary(this->hLuaDll);
		this->hLuaDll = NULL;
	}
	return true;
}

void CHttpServer::InitLua(const mg_connection* conn, void* luaContext)
{
	const CHttpServer* sys = (CHttpServer*)mg_get_user_data(mg_get_context(conn));
	lua_State* L = (lua_State*)luaContext;
	lua_pushlightuserdata(L, sys->initLuaParam);
	sys->initLuaProc(L);
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
