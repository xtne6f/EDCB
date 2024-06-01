#include "stdafx.h"
#include "AppSetting.h"
#include "../../Common/PathUtil.h"
#include "../../Common/StringUtil.h"

#ifdef _WIN32
#include "../../Common/IniUtil.h"

#define GET_SETTING_INT(buff, key, def) GetBufferedProfileInt((buff).data(), key, def)
#define GET_SETTING_TO_STRING(buff, key, def) GetBufferedProfileToString((buff).data(), key, def)
#else
#define GET_SETTING_INT(app, key, def) GetPrivateProfileInt((app).first, key, def, (app).second)
#define GET_SETTING_TO_STRING(app, key, def) GetPrivateProfileToString((app).first, key, def, (app).second)
#endif

APP_SETTING APP_SETTING::Load(LPCWSTR iniPath)
{
	APP_SETTING s;
#ifdef _WIN32
	// セクション単位で処理するほうが軽い
	vector<WCHAR> appSet = GetPrivateProfileSectionBuffer(L"SET", iniPath);
#else
	// 未実装なので普通に読む
	pair<LPCWSTR, LPCWSTR> appSet(L"SET", iniPath);
#endif

#ifdef _WIN32
	s.modifyTitleBarText = GET_SETTING_INT(appSet, L"ModifyTitleBarText", 1) != 0;
	s.overlayTaskIcon = GET_SETTING_INT(appSet, L"OverlayTaskIcon", 1) != 0;
	s.minTask = GET_SETTING_INT(appSet, L"MinTask", 0) != 0;
	s.dialogTemplate = GET_SETTING_INT(appSet, L"DialogTemplate", 1);
	s.viewPath = GET_SETTING_TO_STRING(appSet, L"ViewPath", L"");
	s.viewOption = GET_SETTING_TO_STRING(appSet, L"ViewOption", L"");
	s.viewSingle = GET_SETTING_INT(appSet, L"ViewSingle", 1) != 0;
	s.viewCloseOnExit = GET_SETTING_INT(appSet, L"ViewCloseOnExit", 0) != 0;
#endif
	s.allService = GET_SETTING_INT(appSet, L"AllService", 0) != 0;
	s.scramble = GET_SETTING_INT(appSet, L"Scramble", 1) != 0;
	s.emm = GET_SETTING_INT(appSet, L"EMM", 0) != 0;
	s.enableCaption = GET_SETTING_INT(appSet, L"Caption", 1) != 0;
	s.enableData = GET_SETTING_INT(appSet, L"Data", 0) != 0;
	s.overWrite = GET_SETTING_INT(appSet, L"OverWrite", 0) != 0;
	s.openWait = GET_SETTING_INT(appSet, L"OpenWait", 200);
	s.dropSaveThresh = GET_SETTING_INT(appSet, L"DropSaveThresh", 0);
	s.scrambleSaveThresh = GET_SETTING_INT(appSet, L"ScrambleSaveThresh", -1);
	s.noLogScramble = GET_SETTING_INT(appSet, L"NoLogScramble", 0) != 0;
	s.recFileName = GET_SETTING_TO_STRING(appSet, L"RecFileName", L"$DYYYY$$DMM$$DDD$-$THH$$TMM$$TSS$-$ServiceName$.ts");
#ifdef _WIN32
	s.openLast = GET_SETTING_INT(appSet, L"OpenLast", 1) != 0;
	s.dropLogAsUtf8 = GET_SETTING_INT(appSet, L"DropLogAsUtf8", 0) != 0;
#endif
	s.saveDebugLog = GET_SETTING_INT(appSet, L"SaveDebugLog", 0) != 0;
	s.traceBonDriverLevel = GET_SETTING_INT(appSet, L"TraceBonDriverLevel", 0);
	s.tsBuffMaxCount = (DWORD)GET_SETTING_INT(appSet, L"TsBuffMaxCount", 5000);
	s.writeBuffMaxCount = GET_SETTING_INT(appSet, L"WriteBuffMaxCount", -1);
	s.epgCapBackBSBasic = GET_SETTING_INT(appSet, L"EpgCapBackBSBasicOnly", 1) != 0;
	s.epgCapBackCS1Basic = GET_SETTING_INT(appSet, L"EpgCapBackCS1BasicOnly", 1) != 0;
	s.epgCapBackCS2Basic = GET_SETTING_INT(appSet, L"EpgCapBackCS2BasicOnly", 1) != 0;
	s.epgCapBackCS3Basic = GET_SETTING_INT(appSet, L"EpgCapBackCS3BasicOnly", 0) != 0;
	s.epgCapLive = GET_SETTING_INT(appSet, L"EpgCapLive", 1) != 0;
	s.epgCapRec = GET_SETTING_INT(appSet, L"EpgCapRec", 1) != 0;
	s.parseEpgPostProcess = GET_SETTING_INT(appSet, L"ParseEpgPostProcess", 0) != 0;
	s.epgCapBackStartWaitSec = (DWORD)GET_SETTING_INT(appSet, L"EpgCapBackStartWaitSec", 30);
	s.saveLogo = GET_SETTING_INT(appSet, L"SaveLogo", 0) != 0;
	s.saveLogoTypeFlags = (DWORD)GET_SETTING_INT(appSet, L"SaveLogoTypeFlags", 32);

	for( int tcp = 0; tcp < 2; tcp++ ){
#ifdef _WIN32
		vector<WCHAR> appSetNW = GetPrivateProfileSectionBuffer(tcp ? L"SET_TCP" : L"SET_UDP", iniPath);
#else
		pair<LPCWSTR, LPCWSTR> appSetNW(tcp ? L"SET_TCP" : L"SET_UDP", iniPath);
#endif
		int count = GET_SETTING_INT(appSetNW, L"Count", 0);
		for( int i = 0; i < count; i++ ){
			NW_SEND_INFO item;
			WCHAR key[64];
			swprintf_s(key, L"IP%d", i);
			item.ipString = GET_SETTING_TO_STRING(appSetNW, key, L"2130706433");
			if( item.ipString.size() >= 2 && item.ipString[0] == L'[' ){
				item.ipString.erase(0, 1).pop_back();
			}else{
				DWORD ip = (int)wcstol(item.ipString.c_str(), NULL, 10);
				Format(item.ipString, L"%d.%d.%d.%d", ip >> 24, ip >> 16 & 0xFF, ip >> 8 & 0xFF, ip & 0xFF);
			}
			swprintf_s(key, L"Port%d", i);
			item.port = 0;
			if( item.ipString != BON_NW_SRV_PIPE_IP ){
				item.port = GET_SETTING_INT(appSetNW, key, tcp ? BON_TCP_PORT_BEGIN : BON_UDP_PORT_BEGIN);
			}
			swprintf_s(key, L"BroadCast%d", i);
			item.broadcastFlag = tcp ? 0 : GET_SETTING_INT(appSetNW, key, 0);
			item.udpMaxSendSize = tcp ? 0 : GET_SETTING_INT(appSet, L"UDPPacket", 128) * 188;
			(tcp ? s.tcpSendList : s.udpSendList).push_back(item);
		}
	}

	return s;
}
