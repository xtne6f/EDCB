#include "stdafx.h"
#include "HttpServer.h"
#include "../../Common/StringUtil.h"
#include "../../Common/TimeUtil.h"
#include "../../Common/PathUtil.h"
#include "../../Common/ParseTextInstances.h"
#include "civetweb.h"
#include <fcntl.h>
#ifdef _WIN32
#include <io.h>
#include <wincrypt.h>
#else
#include <sys/file.h>
#endif

namespace
{
const char UPNP_URN_DMS_1[] = "urn:schemas-upnp-org:device:MediaServer:1";
const char UPNP_URN_CDS_1[] = "urn:schemas-upnp-org:service:ContentDirectory:1";
const char UPNP_URN_CMS_1[] = "urn:schemas-upnp-org:service:ConnectionManager:1";
const char UPNP_URN_AVT_1[] = "urn:schemas-upnp-org:service:AVTransport:1";
}

CHttpServer::CHttpServer()
	: mgContext(NULL)
	, initedLibrary(false)
{
}

CHttpServer::~CHttpServer()
{
	StopServer();
}

bool CHttpServer::StartServer(const SERVER_OPTIONS& op, const std::function<void(lua_State*)>& initProc)
{
	StopServer();

	string ports;
	WtoUTF8(op.ports, ports);
	string rootPathU;
	WtoUTF8(op.rootPath, rootPathU);
#ifdef _WIN32
	//パスにASCII範囲外を含むのは(主にLuaが原因で)難ありなので蹴る
	if( std::find_if(rootPathU.begin(), rootPathU.end(), [](char c) { return (c & 0x80) != 0; }) != rootPathU.end() ){
		AddDebugLog(L"CHttpServer::StartServer(): path has unavailable chars.");
		return false;
	}
#ifndef _MSC_VER
	//Civetweb内部で64ビット整数の書式に%I64dが使われるが、特にMinGWではこの書式が利用可能かやや曖昧なので確かめる
	LONGLONG llCheckSpec = 0;
	char szCheckSpec[32];
	if( _snprintf(szCheckSpec, sizeof(szCheckSpec), "%I64d", -12345678901) != 12 ||
	    sscanf(szCheckSpec, "%I64d", &llCheckSpec) != 1 ||
	    llCheckSpec != -12345678901 ){
		AddDebugLog(L"CHttpServer::StartServer(): Environment error, check your compiler.");
		return false;
	}
#endif
#endif
	string accessLogPath;
	//ログは_wfopen()されるのでWtoUTF8()。civetweb.cのACCESS_LOG_FILEとERROR_LOG_FILEの扱いに注意
	WtoUTF8(GetCommonIniPath().replace_filename(L"HttpAccess.log").native(), accessLogPath);
	string errorLogPath;
	WtoUTF8(GetCommonIniPath().replace_filename(L"HttpError.log").native(), errorLogPath);

	fs_path sslFsPath = GetCommonIniPath().replace_filename(L"ssl_");
	//認証鍵は実質fopen()されるのでCP_ACP
	string sslPathA;
	wstring sslPath;
	WtoA(sslFsPath.native(), sslPathA, UTIL_CONV_ACP);
	AtoW(sslPathA, sslPath, UTIL_CONV_ACP);
	if( sslPath != sslFsPath.native() ){
		AddDebugLog(L"CHttpServer::StartServer(): path has unavailable chars.");
		return false;
	}
	string sslCertPath = sslPathA + "cert.pem";
	string sslPeerPath = sslPathA + "peer.pem";
	fs_path sslPeerFsPath = sslFsPath.concat(L"peer.pem");

	string globalAuthPath;
	//グローバルパスワードは_wfopen()されるのでWtoUTF8()
	fs_path globalAuthFsPath = GetCommonIniPath().replace_filename(L"glpasswd");
	WtoUTF8(globalAuthFsPath.native(), globalAuthPath);

	//Access Control List
	string acl;
	WtoUTF8(op.accessControlList, acl);

	string authDomain;
	WtoUTF8(op.authenticationDomain, authDomain);
	string sslCipherList;
	WtoUTF8(op.sslCipherList, sslCipherList);
	char numThreads[16];
	sprintf_s(numThreads, "%d", min(max(op.numThreads, 1), 50));
	char requestTimeout[16];
	sprintf_s(requestTimeout, "%d", max(op.requestTimeout, 1));
	char sslProtocolVersion[16];
	sprintf_s(sslProtocolVersion, "%d", op.sslProtocolVersion);

	//追加のMIMEタイプ
	CParseContentTypeText contentType;
	contentType.ParseText(GetCommonIniPath().replace_filename(L"ContentTypeText.txt").c_str());
	wstring extraMimeW;
	for( map<wstring, wstring>::const_iterator itr = contentType.GetMap().begin(); itr != contentType.GetMap().end(); itr++ ){
		extraMimeW += itr->first + L'=' + itr->second + L',';
	}
	string extraMime;
	WtoUTF8(extraMimeW, extraMime);

	//"api"ディレクトリの特別扱いはドキュメントルートの直下にかぎる
	string luaScriptPattern = "**.lua$|**.html$|";
	for( size_t i = 0; i < rootPathU.size(); i++ ){
		if( rootPathU[i] == '/' ){
			luaScriptPattern += '/';
		}else if( luaScriptPattern.back() != '*' ){
			luaScriptPattern += '*';
		}
	}
	luaScriptPattern += "/api/*$";

	const char* options[64] = {
		"ssi_pattern", "",
		"enable_keep_alive", op.keepAlive ? "yes" : "no",
		"access_control_list", acl.c_str(),
		"extra_mime_types", extraMime.c_str(),
		"listening_ports", ports.c_str(),
		"document_root", rootPathU.c_str(),
		"num_threads", numThreads,
		"request_timeout_ms", requestTimeout,
		"ssl_ca_file", sslPeerPath.c_str(),
		"ssl_default_verify_paths", "no",
		"ssl_cipher_list", sslCipherList.c_str(),
		"ssl_protocol_version", sslProtocolVersion,
		"lua_script_pattern", luaScriptPattern.c_str(),
		"access_control_allow_origin", "",
	};
	int opCount = 2 * 14;
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
	bool mightExist = false;
	if( UtilFileExists(sslPeerFsPath, &mightExist).first || mightExist ){
		//信頼済み証明書ファイルが「存在しないことを確信」できなければ有効にする
		options[opCount++] = "ssl_verify_peer";
		options[opCount++] = "yes";
	}
	if( UtilFileExists(globalAuthFsPath, &mightExist).first || mightExist ){
		//グローバルパスワードは「存在しないことを確信」できなければ指定しておく
		options[opCount++] = "global_auth_file";
		options[opCount++] = globalAuthPath.c_str();
	}

	unsigned int feat = MG_FEATURES_FILES + MG_FEATURES_IPV6 + MG_FEATURES_LUA + MG_FEATURES_CACHE +
	                    (ports.find('s') != string::npos ? MG_FEATURES_TLS : 0);
	this->initedLibrary = true;
	if( mg_init_library(feat + (op.enableSsdpServer ? MG_FEATURES_X_ALLOW_SUBSCRIBE : 0)) != feat ){
		AddDebugLog(L"CHttpServer::StartServer(): Library initialization failed.");
		StopServer();
		return false;
	}

	this->initLuaProc = initProc;
	mg_callbacks callbacks = {};
	callbacks.init_lua = &InitLua;
	this->mgContext = mg_start(&callbacks, this, options);
	if( this->mgContext == NULL ){
		StopServer();
		return false;
	}

	if( op.enableSsdpServer ){
		//"<UDN>uuid:{UUID}</UDN>"が必要
		string notifyUuid;
		std::unique_ptr<FILE, fclose_deleter> fp(UtilOpenFile(fs_path(op.rootPath).append(L"dlna").append(L"dms").append(L"ddd.xml"), UTIL_SECURE_READ));
		if( fp ){
			char olbuff[257];
			for( size_t n = fread(olbuff, 1, 256, fp.get()); ; n = fread(olbuff + 64, 1, 192, fp.get()) + 64 ){
				olbuff[n] = '\0';
				char* udn = strstr(olbuff, "<UDN>uuid:");
				if( udn && strlen(udn) >= 10 + 36 + 6 && strncmp(udn + 10 + 36, "</UDN>", 6) == 0 ){
					notifyUuid.assign(udn + 5, 41);
					break;
				}
				if( n < 256 ){
					break;
				}
				std::copy(olbuff + 192, olbuff + 256, olbuff);
			}
		}
		if( notifyUuid.empty() == false ){
			//最後にみつかった':'より後ろか先頭をatoiした結果を通知ポートとする
			int notifyPort = atoi(ports.c_str() + (ports.find_last_of(':') == string::npos ? 0 : ports.find_last_of(':') + 1)) & 0xFFFF;
			//UPnPのUDP(Port1900)部分を担当するサーバ
			LPCSTR targetArray[] = { "upnp:rootdevice", UPNP_URN_DMS_1, UPNP_URN_CDS_1, UPNP_URN_CMS_1, UPNP_URN_AVT_1 };
			vector<CUpnpSsdpServer::SSDP_TARGET_INFO> targetList(2 + array_size(targetArray));
			targetList[0].target = notifyUuid;
			char location[64];
			sprintf_s(location, ":%d/dlna/dms/ddd.xml", notifyPort);
			targetList[0].location = location;
			targetList[0].usn = targetList[0].target;
			targetList[0].notifyFlag = true;
			targetList[1].target = "ssdp:all";
			targetList[1].location = targetList[0].location;
			targetList[1].usn = notifyUuid + "::" + "upnp:rootdevice";
			targetList[1].notifyFlag = false;
			for( size_t i = 2; i < targetList.size(); i++ ){
				targetList[i].target = targetArray[i - 2];
				targetList[i].location = targetList[0].location;
				targetList[i].usn = notifyUuid + "::" + targetList[i].target;
				targetList[i].notifyFlag = true;
			}
			this->upnpSsdpServer.Start(targetList, op.ssdpIfTypes, op.ssdpInitialWaitSec);
		}else{
			AddDebugLog(L"CHttpServer::StartServer(): invalid /dlna/dms/ddd.xml");
		}
	}
	return true;
}

bool CHttpServer::StopServer(bool checkOnly)
{
	if( this->mgContext ){
		this->upnpSsdpServer.Stop();
		if( checkOnly ){
			if( mg_check_stop(this->mgContext) == 0 ){
				return false;
			}
		}else{
			//正常であればmg_stop()はreqToを超えて待機することはない
			DWORD reqTo = atoi(mg_get_option(this->mgContext, "request_timeout_ms"));
			DWORD tick = GetU32Tick();
			while( GetU32Tick() - tick < reqTo + 10000 ){
				if( mg_check_stop(this->mgContext) ){
					this->mgContext = NULL;
					break;
				}
				SleepForMsec(10);
			}
			if( this->mgContext ){
				AddDebugLog(L"CHttpServer::StopServer(): failed to stop service.");
			}
		}
		this->mgContext = NULL;
	}
	if( this->initedLibrary ){
		mg_exit_library();
		this->initedLibrary = false;
	}
	return true;
}

CHttpServer::SERVER_OPTIONS CHttpServer::LoadServerOptions(LPCWSTR iniPath)
{
	SERVER_OPTIONS op;
	int enableHttpSrv = GetPrivateProfileInt(L"SET", L"EnableHttpSrv", 0, iniPath);
	if( enableHttpSrv != 0 ){
		op.rootPath = GetPrivateProfileToString(L"SET", L"HttpPublicFolder", L"", iniPath);
		if( op.rootPath.empty() ){
			op.rootPath = GetCommonIniPath().replace_filename(L"HttpPublic").native();
		}
		op.accessControlList = GetPrivateProfileToString(L"SET", L"HttpAccessControlList", L"+127.0.0.1,+::1,+::ffff:127.0.0.1", iniPath);
		op.authenticationDomain = GetPrivateProfileToString(L"SET", L"HttpAuthenticationDomain", L"", iniPath);
		op.numThreads = GetPrivateProfileInt(L"SET", L"HttpNumThreads", 5, iniPath);
		op.requestTimeout = GetPrivateProfileInt(L"SET", L"HttpRequestTimeoutSec", 120, iniPath) * 1000;
		op.sslCipherList = GetPrivateProfileToString(L"SET", L"HttpSslCipherList", L"HIGH:!aNULL:!MD5", iniPath);
		op.sslProtocolVersion = GetPrivateProfileInt(L"SET", L"HttpSslProtocolVersion", 4, iniPath);
		op.keepAlive = GetPrivateProfileInt(L"SET", L"HttpKeepAlive", 0, iniPath) != 0;
		op.ports = GetPrivateProfileToString(L"SET", L"HttpPort", L"5510", iniPath);
		op.saveLog = enableHttpSrv == 2;
		op.enableSsdpServer = GetPrivateProfileInt(L"SET", L"EnableDMS", 0, iniPath) != 0;
		if( op.enableSsdpServer ){
			op.ssdpIfTypes = GetPrivateProfileInt(L"SET", L"DmsIfTypes", CUpnpSsdpServer::SSDP_IF_LOOPBACK | CUpnpSsdpServer::SSDP_IF_C_PRIVATE, iniPath);
			op.ssdpInitialWaitSec = GetPrivateProfileInt(L"SET", L"DmsInitialWaitSec", 20, iniPath);
		}
	}
	return op;
}

string CHttpServer::CreateRandom(size_t len)
{
	string ret;
	ret.reserve(len * 2);
#ifdef _WIN32
	HCRYPTPROV prov;
	if( CryptAcquireContext(&prov, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT) ){
		ULONGLONG r = 0;
		for( size_t i = 0; i < len && CryptGenRandom(prov, 8, (BYTE*)&r); i += 8 ){
			char x[17];
			sprintf_s(x, "%016llx", r);
			ret.append(x, min<size_t>(len - i, 8) * 2);
		}
		CryptReleaseContext(prov, 0);
	}
#else
	std::unique_ptr<FILE, fclose_deleter> fp(UtilOpenFile(fs_path(L"/dev/urandom"), UTIL_SHARED_READ));
	if( fp ){
		ULONGLONG r;
		for( size_t i = 0; i < len && fread(&r, 1, 8, fp.get()) == 8; i += 8 ){
			char x[17];
			sprintf_s(x, "%016llx", r);
			ret.append(x, min<size_t>(len - i, 8) * 2);
		}
	}
#endif
	if( ret.size() < len * 2 ){
		ret.clear();
	}
	return ret;
}

void CHttpServer::InitLua(const mg_connection* conn, void* luaContext, unsigned int contextFlags)
{
	(void)contextFlags;
	const CHttpServer* sys = (CHttpServer*)mg_get_user_data(mg_get_context(conn));
	lua_State* L = (lua_State*)luaContext;
	sys->initLuaProc(L);
}

namespace LuaHelp
{

#if 1 //Refer: civetweb/mod_lua.inl
void reg_string_(lua_State* L, const char* name, size_t size, const char* val)
{
	lua_pushlstring(L, name, size - 1);
	lua_pushstring(L, val);
	lua_rawset(L, -3);
}

void reg_int_(lua_State* L, const char* name, size_t size, int val)
{
	lua_pushlstring(L, name, size - 1);
	lua_pushinteger(L, val);
	lua_rawset(L, -3);
}

void reg_number_(lua_State* L, const char* name, size_t size, double val)
{
	lua_pushlstring(L, name, size - 1);
	lua_pushnumber(L, (lua_Number)val);
	lua_rawset(L, -3);
}

void reg_boolean_(lua_State* L, const char* name, size_t size, bool val)
{
	lua_pushlstring(L, name, size - 1);
	lua_pushboolean(L, val);
	lua_rawset(L, -3);
}
#endif

void reg_time_(lua_State* L, const char* name, size_t size, const SYSTEMTIME& st)
{
	lua_pushlstring(L, name, size - 1);
	lua_createtable(L, 0, 9);
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

LONGLONG get_int64(lua_State* L, const char* name)
{
	lua_getfield(L, -1, name);
	lua_Number ret = lua_tonumber(L, -1);
	lua_pop(L, 1);
	//整数を正しく表現できない範囲の値は制限する
	return (LONGLONG)min(max(ret, -1e+16), 1e+16);
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
	SYSTEMTIME ret = {};
	lua_getfield(L, -1, name);
	if( lua_istable(L, -1) ){
		SYSTEMTIME st;
		st.wYear = (WORD)get_int(L, "year");
		st.wMonth = (WORD)get_int(L, "month");
		st.wDay = (WORD)get_int(L, "day");
		st.wHour = (WORD)get_int(L, "hour");
		st.wMinute = (WORD)get_int(L, "min");
		st.wSecond = (WORD)get_int(L, "sec");
		st.wMilliseconds = (WORD)get_int(L, "msec");
		LONGLONG t = ConvertI64Time(st);
		if( t != 0 && ConvertSystemTime(t, &st) ){
			ret = st;
		}
	}
	lua_pop(L, 1);
	return ret;
}

#ifdef _WIN32
namespace
{

wchar_t* utf8towcsdup(const char* s, const wchar_t* prefix = L"", const wchar_t* suffix = L"")
{
	wstring w;
	try{
		string u;
		UTF8toW(s, w);
		WtoUTF8(w, u);
		if( u != s ){
			return NULL;
		}
		w.insert(0, prefix);
		w.append(suffix);
	}catch(...){
		return NULL;
	}
	wchar_t* ret = (wchar_t*)malloc((w.size() + 1) * sizeof(wchar_t));
	if( ret ){
		wcscpy_s(ret, w.size() + 1, w.c_str());
	}
	return ret;
}

wchar_t* allocenv(lua_State* L, int idx)
{
	wstring wenv;
	LPWCH env = GetEnvironmentStrings();
	if( env ){
		size_t n = 0;
		while( env[n] ){
			n += wcslen(env + n) + 1;
		}
		try{
			wenv.assign(env, env + n);
		}catch(...){
			FreeEnvironmentStrings(env);
			return NULL;
		}
		FreeEnvironmentStrings(env);
	}
	lua_pushnil(L);
	while( lua_next(L, idx) ){
		if( lua_type(L, -2) == LUA_TSTRING ){
			try{
				wstring var;
				UTF8toW(lua_tostring(L, -2), var);
				if( var.find(L'=') == wstring::npos ){
					for( size_t n = 0; n < wenv.size(); ){
						size_t m = wenv.find(L'\0', n);
						if( m - n > var.size() && wenv[n + var.size()] == L'=' ){
							wenv[n + var.size()] = L'\0';
							if( CompareNoCase(var, wenv.c_str() + n) == 0 ){
								//erase an old variable
								wenv.erase(n, m + 1 - n);
							}else{
								wenv[n + var.size()] = L'=';
								n = m + 1;
							}
						}else{
							n = m + 1;
						}
					}
					if( lua_type(L, -1) == LUA_TSTRING ){
						//append a new variable
						wstring val;
						UTF8toW(lua_tostring(L, -1), val);
						wenv += var + L'=' + val + L'\0';
					}
				}
			}catch(...){
				lua_pop(L, 2);
				return NULL;
			}
		}
		lua_pop(L, 1);
	}
	wchar_t* ret = (wchar_t*)malloc((wenv.size() + 1) * sizeof(wchar_t));
	if( ret ){
		std::copy(wenv.c_str(), wenv.c_str() + wenv.size() + 1, ret);
	}
	return ret;
}

void nefree(void* p)
{
	int en = errno;
	free(p);
	errno = en;
}

struct LStream {
	FILE* f; //stream (NULL for incompletely created streams)
	lua_CFunction closef; //to close stream (NULL for closed streams)
	HANDLE procf; //process handle used by f_pclose
};

//Refer: Lua-5.2.4/liolib.c (See Copyright Notice in lua.h)

int checkmode(const char* mode)
{
	return *mode != '\0' && strchr("rwa", *(mode++)) != NULL &&
		(*mode != '+' || ++mode) && //skip if char is '+'
		(*mode != 'b' || ++mode) && //skip if char is 'b'
		(*mode == '\0');
}

LStream* tolstream(lua_State* L)
{
	return (LStream*)luaL_checkudata(L, 1, "EDCB_FILE*");
}

int f_tostring(lua_State* L)
{
	LStream* p = tolstream(L);
	if( p->closef == NULL )
		lua_pushliteral(L, "edcb_file (closed)");
	else
		lua_pushfstring(L, "edcb_file (%p)", p->f);
	return 1;
}

FILE* tofile(lua_State* L)
{
	LStream* p = tolstream(L);
	if( p->closef == NULL )
		luaL_error(L, "attempt to use a closed file");
	lua_assert(p->f);
	return p->f;
}

LStream* newprefile(lua_State* L)
{
	LStream* p = (LStream*)lua_newuserdata(L, sizeof(LStream));
	p->closef = NULL; //mark file handle as 'closed'
	luaL_setmetatable(L, "EDCB_FILE*");
	return p;
}

int aux_close(lua_State* L)
{
	LStream* p = tolstream(L);
	lua_CFunction cf = p->closef;
	p->closef = NULL; //mark stream as closed
	return (*cf)(L); //close it
}

int f_close(lua_State* L)
{
	tofile(L); //make sure argument is an open stream
	return aux_close(L);
}

int f_gc(lua_State* L)
{
	LStream* p = tolstream(L);
	if( p->closef != NULL && p->f != NULL )
		aux_close(L); //ignore closed and incompletely open files
	return 0;
}

int f_fclose(lua_State* L)
{
	LStream* p = tolstream(L);
	return luaL_fileresult(L, (fclose(p->f) == 0), NULL);
}

int f_pclose(lua_State* L)
{
	LStream* p = tolstream(L);
	fclose(p->f);
	DWORD dw;
	int stat = -1;
	if( WaitForSingleObject(p->procf, INFINITE) == WAIT_OBJECT_0 && GetExitCodeProcess(p->procf, &dw) ){
		stat = (int)dw;
	}
	CloseHandle(p->procf);
	errno = ECHILD;
	return luaL_execresult(L, stat);
}

int test_eof(lua_State* L, FILE* f)
{
	int c = getc(f);
	ungetc(c, f);
	lua_pushlstring(L, NULL, 0);
	return (c != EOF);
}

void read_all(lua_State* L, FILE* f)
{
	size_t rlen = LUAL_BUFFERSIZE; //how much to read in each cycle
	luaL_Buffer b;
	luaL_buffinit(L, &b);
	for(;;){
		char* p = luaL_prepbuffsize(&b, rlen);
		size_t nr = fread(p, sizeof(char), rlen, f);
		luaL_addsize(&b, nr);
		if( nr < rlen ) break; //eof?
		else if( rlen <= ((~(size_t)0) / 4) ) //avoid buffers too large
			rlen *= 2; //double buffer size at each iteration
	}
	luaL_pushresult(&b); //close buffer
}

int read_chars(lua_State* L, FILE* f, size_t n)
{
	luaL_Buffer b;
	luaL_buffinit(L, &b);
	char* p = luaL_prepbuffsize(&b, n); //prepare buffer to read whole block
	size_t nr = fread(p, sizeof(char), n, f); //try to read 'n' chars
	luaL_addsize(&b, nr);
	luaL_pushresult(&b); //close buffer
	return (nr > 0); //true iff read something
}

int f_read(lua_State* L)
{
	FILE* f = tofile(L);
	int nargs = lua_gettop(L) - 1;
	int success;
	int n;
	clearerr(f);
	if( nargs == 0 ){ //no arguments?
		return luaL_error(L, "invalid format");
	}else{ //ensure stack space for all results and for auxlib's buffer
		luaL_checkstack(L, nargs + LUA_MINSTACK, "too many arguments");
		success = 1;
		for( n = 2; nargs-- && success; n++ ){
			if( lua_type(L, n) == LUA_TNUMBER ){
				size_t l = (size_t)lua_tointeger(L, n);
				success = (l == 0) ? test_eof(L, f) : read_chars(L, f, l);
			}else{
				const char* p = lua_tostring(L, n);
				luaL_argcheck(L, p && p[0] == '*', n, "invalid option");
				switch( p[1] ){
				case 'a': //file
					read_all(L, f); //read entire file
					success = 1; //always success
					break;
				default:
					return luaL_argerror(L, n, "invalid format");
				}
			}
		}
	}
	if( ferror(f) )
		return luaL_fileresult(L, 0, NULL);
	if( !success ){
		lua_pop(L, 1); //remove last result
		lua_pushnil(L); //push nil instead
	}
	return n - 2;
}

int f_write(lua_State* L)
{
	FILE* f = tofile(L);
	lua_pushvalue(L, 1); //push file at the stack top (to be returned)
	int arg = 2;
	int nargs = lua_gettop(L) - arg;
	int status = 1;
	for( ; nargs--; arg++ ){
		size_t l;
		const char* s = luaL_checklstring(L, arg, &l);
		status = status && (fwrite(s, sizeof(char), l, f) == l);
	}
	if( status ) return 1; //file handle already on stack top
	else return luaL_fileresult(L, status, NULL);
}

int f_seek(lua_State* L)
{
	static const int mode[] = { SEEK_SET, SEEK_CUR, SEEK_END };
	static const char* const modenames[] = { "set", "cur", "end", NULL };
	FILE* f = tofile(L);
	int op = luaL_checkoption(L, 2, "cur", modenames);
	lua_Number p3 = luaL_optnumber(L, 3, 0);
	LONGLONG offset = (LONGLONG)p3;
	luaL_argcheck(L, (lua_Number)offset == p3, 3, "not an integer in proper range");
	op = my_fseek(f, offset, mode[op]);
	if( op )
		return luaL_fileresult(L, 0, NULL); //error
	else{
		lua_pushnumber(L, (lua_Number)my_ftell(f));
		return 1;
	}
}

int f_setvbuf(lua_State* L)
{
	static const int mode[] = { _IONBF, _IOFBF, _IOLBF };
	static const char* const modenames[] = { "no", "full", "line", NULL };
	FILE* f = tofile(L);
	int op = luaL_checkoption(L, 2, NULL, modenames);
	lua_Integer sz = luaL_optinteger(L, 3, LUAL_BUFFERSIZE);
	int res = setvbuf(f, NULL, mode[op], sz);
	return luaL_fileresult(L, res == 0, NULL);
}

int f_flush(lua_State* L)
{
	return luaL_fileresult(L, fflush(tofile(L)) == 0, NULL);
}

}

//Refer: Lua-5.2.4/loslib.c (See Copyright Notice in lua.h)

int os_execute(lua_State* L)
{
	wchar_t cmdexe[MAX_PATH];
	DWORD dw = GetEnvironmentVariable(L"ComSpec", cmdexe, MAX_PATH);
	if( dw == 0 || dw >= MAX_PATH ){
		cmdexe[0] = L'\0';
	}
	const char* cmd = luaL_optstring(L, 1, NULL);
	if( cmd != NULL ){
		//EXTENDED!: show/hide console window
		DWORD creflags = CREATE_UNICODE_ENVIRONMENT | (lua_toboolean(L, 2) ? 0 : CREATE_NO_WINDOW);
		wchar_t* wcmd = utf8towcsdup(cmd, L" /c ");
		luaL_argcheck(L, wcmd != NULL, 1, "utf8towcsdup");
		//EXTENDED!: override environment
		wchar_t* wenv = lua_istable(L, 3) ? allocenv(L, 3) : NULL;
		STARTUPINFO si = {};
		si.cb = sizeof(si);
		PROCESS_INFORMATION pi;
		int stat = -1;
		if( cmdexe[0] && CreateProcess(cmdexe, wcmd, NULL, NULL, FALSE, creflags, wenv, NULL, &si, &pi) ){
			CloseHandle(pi.hThread);
			if( WaitForSingleObject(pi.hProcess, INFINITE) == WAIT_OBJECT_0 && GetExitCodeProcess(pi.hProcess, &dw) ){
				stat = (int)dw;
			}
			CloseHandle(pi.hProcess);
		}
		errno = ENOENT;
		nefree(wenv);
		nefree(wcmd);
		return luaL_execresult(L, stat);
	}else{
		lua_pushboolean(L, cmdexe[0]); //true if there is a shell
		return 1;
	}
}

int os_remove(lua_State* L)
{
	const char* filename = luaL_checkstring(L, 1);
	wchar_t* wfilename = utf8towcsdup(filename);
	luaL_argcheck(L, wfilename != NULL, 1, "utf8towcsdup");
	int stat = _wremove(wfilename);
	nefree(wfilename);
	return luaL_fileresult(L, stat == 0, filename);
}

int os_rename(lua_State* L)
{
	const char* fromname = luaL_checkstring(L, 1);
	const char* toname = luaL_checkstring(L, 2);
	wchar_t* wfromname = utf8towcsdup(fromname);
	luaL_argcheck(L, wfromname != NULL, 1, "utf8towcsdup");
	wchar_t* wtoname = utf8towcsdup(toname);
	if( wtoname == NULL ){
		free(wfromname);
		luaL_argerror(L, 2, "utf8towcsdup");
	}
	int stat = _wrename(wfromname, wtoname);
	nefree(wtoname);
	nefree(wfromname);
	return luaL_fileresult(L, stat == 0, NULL);
}

//Refer: Lua-5.2.4/liolib.c (See Copyright Notice in lua.h)

int io_open(lua_State* L)
{
	const char* filename = luaL_checkstring(L, 1);
	const char* mode = luaL_optstring(L, 2, "r");
	LStream* p = newprefile(L);
	p->f = NULL;
	p->closef = &f_fclose;
	luaL_argcheck(L, checkmode(mode), 2, "invalid mode");
	wchar_t* wfilename = utf8towcsdup(filename);
	luaL_argcheck(L, wfilename != NULL, 1, "utf8towcsdup");
	wchar_t* wmode = utf8towcsdup(mode, L"", L"N");
	if( wmode == NULL ){
		free(wfilename);
		luaL_argerror(L, 2, "utf8towcsdup");
	}
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996)
#endif
	p->f = _wfopen(wfilename, wmode);
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	nefree(wmode);
	nefree(wfilename);
	return (p->f == NULL) ? luaL_fileresult(L, 0, filename) : 1;
}

int io_popen(lua_State* L)
{
	const char* filename = luaL_checkstring(L, 1);
	const char* mode = luaL_optstring(L, 2, "r");
	//EXTENDED!: show/hide console window
	DWORD creflags = CREATE_UNICODE_ENVIRONMENT | (lua_toboolean(L, 3) ? 0 : CREATE_NO_WINDOW);
	int nargs = lua_gettop(L);
	LStream* p = newprefile(L);
	luaL_argcheck(L, (mode[0] == 'r' || mode[0] == 'w') && (!mode[1] || mode[1] == 'b' && !mode[2]), 2, "invalid mode");
	wchar_t* wfilename = utf8towcsdup(filename, L" /c ");
	luaL_argcheck(L, wfilename != NULL, 1, "utf8towcsdup");
	p->f = NULL;
	wchar_t cmdexe[MAX_PATH];
	DWORD dw = GetEnvironmentVariable(L"ComSpec", cmdexe, MAX_PATH);
	if( dw == 0 || dw >= MAX_PATH ){
		cmdexe[0] = L'\0';
	}
	//EXTENDED!: override environment
	wchar_t* wenv = nargs >= 4 && lua_istable(L, 4) ? allocenv(L, 4) : NULL;
	HANDLE ppipe = INVALID_HANDLE_VALUE; //parent
	HANDLE tpipe = INVALID_HANDLE_VALUE; //temporary
	if( cmdexe[0] && CreatePipe(mode[0] == 'r' ? &ppipe : &tpipe, mode[0] == 'r' ? &tpipe : &ppipe, NULL, 0) ){
		HANDLE cpipe; //child
		BOOL b = DuplicateHandle(GetCurrentProcess(), tpipe, GetCurrentProcess(), &cpipe, 0, TRUE, DUPLICATE_SAME_ACCESS);
		CloseHandle(tpipe);
		if( b ){
			SECURITY_ATTRIBUTES sa = {};
			sa.nLength = sizeof(sa);
			sa.bInheritHandle = TRUE;
			STARTUPINFO si = {};
			si.cb = sizeof(si);
			si.dwFlags = STARTF_USESTDHANDLES;
			if( mode[0] == 'r' ){
				si.hStdInput = CreateFile(L"nul", GENERIC_READ, 0, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
				si.hStdOutput = cpipe;
			}else{
				si.hStdInput = cpipe;
				si.hStdOutput = CreateFile(L"nul", GENERIC_WRITE, 0, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			}
			si.hStdError = CreateFile(L"nul", GENERIC_WRITE, 0, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			PROCESS_INFORMATION pi;
			b = CreateProcess(cmdexe, wfilename, NULL, NULL, TRUE, creflags, wenv, NULL, &si, &pi);
			if( si.hStdError != INVALID_HANDLE_VALUE ){
				CloseHandle(si.hStdError);
			}
			if( si.hStdOutput != INVALID_HANDLE_VALUE ){
				CloseHandle(si.hStdOutput);
			}
			if( si.hStdInput != INVALID_HANDLE_VALUE ){
				CloseHandle(si.hStdInput);
			}
			if( b ){
				CloseHandle(pi.hThread);
				int osfd = _open_osfhandle((intptr_t)ppipe, mode[1] ? 0 : _O_TEXT);
				if( osfd != -1 ){
					ppipe = INVALID_HANDLE_VALUE;
					p->f = _wfdopen(osfd, mode[0] == 'r' ? (mode[1] ? L"rb" : L"r") : (mode[1] ? L"wb" : L"w"));
					if( p->f ){
						p->procf = pi.hProcess;
					}else{
						_close(osfd);
						CloseHandle(pi.hProcess);
					}
				}else{
					CloseHandle(pi.hProcess);
				}
			}
		}
		if( ppipe != INVALID_HANDLE_VALUE ){
			CloseHandle(ppipe);
		}
	}
	errno = ENOENT;
	nefree(wenv);
	nefree(wfilename);
	p->closef = &f_pclose;
	return (p->f == NULL) ? luaL_fileresult(L, 0, filename) : 1;
}

void f_createmeta(lua_State* L)
{
	static const luaL_Reg flib[] = {
		{ "close", f_close },
		{ "flush", f_flush },
		{ "read", f_read },
		{ "seek", f_seek },
		{ "setvbuf", f_setvbuf },
		{ "write", f_write },
		{ "__gc", f_gc },
		{ "__tostring", f_tostring },
		{ NULL, NULL }
	};
	luaL_newmetatable(L, "EDCB_FILE*"); //create metatable for file handles
	lua_pushvalue(L, -1); //push metatable
	lua_setfield(L, -2, "__index"); //metatable.__index = metatable
	luaL_setfuncs(L, flib, 0); //add file methods to new metatable
	lua_pop(L, 1); //pop new metatable
}
#else
int io_cloexec(lua_State* L)
{
	luaL_Stream* p = (luaL_Stream*)luaL_checkudata(L, 1, LUA_FILEHANDLE);
	if( p->f ){
		int flags = fcntl(fileno(p->f), F_GETFD);
		if( flags != -1 && fcntl(fileno(p->f), F_SETFD, flags | FD_CLOEXEC) != -1 ){
			return 0;
		}
	}
	return luaL_error(L, "fcntl");
}

int io_flock_nb(lua_State* L)
{
	luaL_Stream* p = (luaL_Stream*)luaL_checkudata(L, 1, LUA_FILEHANDLE);
	const char* mode = luaL_optstring(L, 2, "x");
	luaL_argcheck(L, (*mode && !mode[1] && strchr("sux", *mode)), 2, "invalid mode");
	if( p->f ){
		int ret = flock(fileno(p->f), LOCK_NB | (*mode == 's' ? LOCK_SH : *mode == 'u' ? LOCK_UN : LOCK_EX));
		lua_pushboolean(L, ret == 0);
		return 1;
	}
	return luaL_error(L, "flock");
}
#endif

}
