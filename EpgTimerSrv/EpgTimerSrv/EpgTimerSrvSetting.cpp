#include "stdafx.h"
#include "EpgTimerSrvSetting.h"
#include "../../Common/Util.h"
#include "../../Common/StringUtil.h"

CEpgTimerSrvSetting::SETTING CEpgTimerSrvSetting::LoadSetting(LPCWSTR iniPath)
{
	SETTING s;
	s.epgArchivePeriodHour = GetPrivateProfileInt(L"SET", L"EpgArchivePeriodHour", 0, iniPath);
	s.residentMode = GetPrivateProfileInt(L"SET", L"ResidentMode", 0, iniPath);
	s.noBalloonTip = GetPrivateProfileInt(L"SET", L"NoBalloonTip", 0, iniPath) != 0;
	s.saveNotifyLog = GetPrivateProfileInt(L"SET", L"SaveNotifyLog", 0, iniPath) != 0;
	s.wakeTime = GetPrivateProfileInt(L"SET", L"WakeTime", 5, iniPath);
	s.autoAddHour = GetPrivateProfileInt(L"SET", L"AutoAddDays", 8, iniPath) * 24 +
	                GetPrivateProfileInt(L"SET", L"AutoAddHour", 0, iniPath);
	s.chkGroupEvent = GetPrivateProfileInt(L"SET", L"ChkGroupEvent", 1, iniPath) != 0;
	s.recEndMode = (BYTE)GetPrivateProfileInt(L"SET", L"RecEndMode", 2, iniPath);
	s.reboot = GetPrivateProfileInt(L"SET", L"Reboot", 0, iniPath) != 0;
	s.noUsePC = GetPrivateProfileInt(L"NO_SUSPEND", L"NoUsePC", 0, iniPath) != 0;
	s.noUsePCTime = GetPrivateProfileInt(L"NO_SUSPEND", L"NoUsePCTime", 3, iniPath);
	s.noFileStreaming = GetPrivateProfileInt(L"NO_SUSPEND", L"NoFileStreaming", 0, iniPath) != 0;
	s.noShareFile = GetPrivateProfileInt(L"NO_SUSPEND", L"NoShareFile", 0, iniPath) != 0;
	s.noStandbyTime = GetPrivateProfileInt(L"NO_SUSPEND", L"NoStandbyTime", 10, iniPath);
	s.noSuspendExeList.clear();
	int count = GetPrivateProfileInt(L"NO_SUSPEND", L"Count", INT_MAX, iniPath);
	if( count == INT_MAX ){
		//未設定
		s.noSuspendExeList.push_back(L"EpgDataCap_Bon.exe");
	}else{
		for( int i = 0; i < count; i++ ){
			WCHAR key[16];
			swprintf_s(key, L"%d", i);
			wstring buff = GetPrivateProfileToString(L"NO_SUSPEND", key, L"", iniPath);
			if( buff.empty() == false ){
				s.noSuspendExeList.push_back(buff);
			}
		}
	}
	s.viewBonList.clear();
	count = GetPrivateProfileInt(L"TVTEST", L"Num", 0, iniPath);
	for( int i = 0; i < count; i++ ){
		WCHAR key[16];
		swprintf_s(key, L"%d", i);
		wstring buff = GetPrivateProfileToString(L"TVTEST", key, L"", iniPath);
		if( buff.empty() == false ){
			s.viewBonList.push_back(buff);
		}
	}
	s.ngEpgCapTime = GetPrivateProfileInt(L"SET", L"NGEpgCapTime", 20, iniPath);
	s.ngEpgCapTunerTime = GetPrivateProfileInt(L"SET", L"NGEpgCapTunerTime", 20, iniPath);
	s.timeSync = GetPrivateProfileInt(L"SET", L"TimeSync", 0, iniPath) != 0;
	s.epgCapTimeList.clear();
	count = GetPrivateProfileInt(L"EPG_CAP", L"Count", INT_MAX, iniPath);
	if( count == INT_MAX ){
		//未設定。毎日23:00に取得
		s.epgCapTimeList.resize(1);
		s.epgCapTimeList.back().first = true;
		s.epgCapTimeList.back().second.first = 23 * 60;
		s.epgCapTimeList.back().second.second = -1;
	}else{
		for( int i = 0; i < count; i++ ){
			WCHAR key[32];
			swprintf_s(key, L"%d", i);
			wstring buff = GetPrivateProfileToString(L"EPG_CAP", key, L"", iniPath);
			//曜日指定接尾辞(w1=Mon,...,w7=Sun)
			unsigned int hour, minute, wday = 0;
			if( swscanf_s(buff.c_str(), L"%u:%uw%u", &hour, &minute, &wday) >= 2 ){
				s.epgCapTimeList.resize(s.epgCapTimeList.size() + 1);
				s.epgCapTimeList.back().second.first = ((wday * 24 + hour) * 60 + minute) % (1440 * 8);
				//有効か
				swprintf_s(key, L"%dSelect", i);
				s.epgCapTimeList.back().first = GetPrivateProfileInt(L"EPG_CAP", key, 0, iniPath) != 0;
				//取得種別(bit0(LSB)=BS,bit1=CS1,bit2=CS2,bit3=CS3)。負値のときは共通設定に従う
				swprintf_s(key, L"%dBasicOnlyFlags", i);
				s.epgCapTimeList.back().second.second = GetPrivateProfileInt(L"EPG_CAP", key, -1, iniPath);
			}
		}
	}
	s.autoDel = GetPrivateProfileInt(L"SET", L"AutoDel", 0, iniPath) != 0;
	s.delExtList.clear();
	count = GetPrivateProfileInt(L"DEL_EXT", L"Count", INT_MAX, iniPath);
	if( count == INT_MAX ){
		//未設定
		s.delExtList.push_back(L".ts.err");
		s.delExtList.push_back(L".ts.program.txt");
	}else{
		for( int i = 0; i < count; i++ ){
			WCHAR key[16];
			swprintf_s(key, L"%d", i);
			s.delExtList.push_back(GetPrivateProfileToString(L"DEL_EXT", key, L"", iniPath));
		}
	}
	s.delChkList.clear();
	count = GetPrivateProfileInt(L"DEL_CHK", L"Count", 0, iniPath);
	for( int i = 0; i < count; i++ ){
		WCHAR key[16];
		swprintf_s(key, L"%d", i);
		s.delChkList.push_back(GetPrivateProfileToString(L"DEL_CHK", key, L"", iniPath));
	}
	s.startMargin = GetPrivateProfileInt(L"SET", L"StartMargin", 5, iniPath);
	s.endMargin = GetPrivateProfileInt(L"SET", L"EndMargin", 2, iniPath);
	s.tuijyuHour = GetPrivateProfileInt(L"SET", L"TuijyuHour", 3, iniPath);
	s.backPriority = GetPrivateProfileInt(L"SET", L"BackPriority", 1, iniPath) != 0;
	s.fixedTunerPriority = GetPrivateProfileInt(L"SET", L"FixedTunerPriority", 1, iniPath) != 0;
	s.autoDelRecInfo = GetPrivateProfileInt(L"SET", L"AutoDelRecInfo", 0, iniPath) != 0;
	s.autoDelRecInfoNum = GetPrivateProfileInt(L"SET", L"AutoDelRecInfoNum", 100, iniPath);
	s.recInfo2Max = GetPrivateProfileInt(L"SET", L"RecInfo2Max", 1000, iniPath);
	s.recInfo2DropChk = GetPrivateProfileInt(L"SET", L"RecInfo2DropChk", 2, iniPath);
	s.recInfo2RegExp = GetPrivateProfileToString(L"SET", L"RecInfo2RegExp", L"", iniPath);
	s.errEndBatRun = GetPrivateProfileInt(L"SET", L"ErrEndBatRun", 0, iniPath) != 0;
	s.recNamePlugIn = GetPrivateProfileInt(L"SET", L"RecNamePlugIn", 0, iniPath) != 0;
	s.recNamePlugInFile = GetPrivateProfileToString(L"SET", L"RecNamePlugInFile", L"RecName_Macro.dll", iniPath);
	s.noChkYen = GetPrivateProfileInt(L"SET", L"NoChkYen", 0, iniPath) != 0;
	s.delReserveMode = GetPrivateProfileInt(L"SET", L"DelReserveMode", 2, iniPath);
	s.recAppWakeTime = GetPrivateProfileInt(L"SET", L"RecAppWakeTime", 2, iniPath);
	s.recMinWake = GetPrivateProfileInt(L"SET", L"RecMinWake", 1, iniPath) != 0;
	s.recView = GetPrivateProfileInt(L"SET", L"RecView", 1, iniPath) != 0;
	s.recNW = GetPrivateProfileInt(L"SET", L"RecNW", 0, iniPath) != 0;
	s.pgInfoLog = GetPrivateProfileInt(L"SET", L"PgInfoLog", 1, iniPath) != 0;
	s.dropLog = GetPrivateProfileInt(L"SET", L"DropLog", 1, iniPath) != 0;
	s.recOverWrite = GetPrivateProfileInt(L"SET", L"RecOverWrite", 0, iniPath) != 0;
	s.processPriority = GetPrivateProfileInt(L"SET", L"ProcessPriority", 3, iniPath);
	s.keepDisk = GetPrivateProfileInt(L"SET", L"KeepDisk", 1, iniPath) != 0;
	return s;
}

vector<pair<wstring, wstring>> CEpgTimerSrvSetting::EnumBonFileName(LPCWSTR settingPath)
{
	vector<pair<wstring, wstring>> ret;
	WIN32_FIND_DATA findData;
	HANDLE hFind = FindFirstFile((wstring(settingPath) + L"\\*.ChSet4.txt").c_str(), &findData);
	if( hFind != INVALID_HANDLE_VALUE ){
		do{
			if( (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 ){
				wstring bon = findData.cFileName;
				for( int depth = 0; bon.empty() == false; ){
					if( bon.back() == L')' ){
						depth++;
					}else if( bon.back() == L'(' && depth > 0 ){
						if( --depth == 0 ){
							bon.pop_back();
							break;
						}
					}
					bon.pop_back();
				}
				if( bon.empty() == false ){
					bon += L".dll";
					if( std::find_if(ret.begin(), ret.end(), [&](const pair<wstring, wstring>& a) { return CompareNoCase(a.first, bon) == 0; }) == ret.end() ){
						ret.push_back(pair<wstring, wstring>(bon, findData.cFileName));
					}
				}
			}
		}while( FindNextFile(hFind, &findData) );
		FindClose(hFind);
	}
	return ret;
}
