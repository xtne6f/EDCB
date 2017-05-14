#include "stdafx.h"
#include "EpgTimerSrvSetting.h"
#include "../../Common/Util.h"
#include "../../Common/StringUtil.h"
#include "../../Common/PathUtil.h"
#include "../../Common/SendCtrlCmd.h"
#include "resource.h"
#include <windowsx.h>
#include <ShlObj.h>
#include <commdlg.h>

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
	s.recInfoFolderOnly = GetPrivateProfileInt(L"SET", L"RecInfoFolderOnly", 1, iniPath) != 0;
	s.applyExtToRecInfoDel = GetPrivateProfileInt(L"SET", L"ApplyExtToRecInfoDel", 0, iniPath) != 0;
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

vector<wstring> CEpgTimerSrvSetting::EnumRecNamePlugInFileName(LPCWSTR moduleFolder)
{
	vector<wstring> ret;
	WIN32_FIND_DATA findData;
	HANDLE hFind = FindFirstFile((wstring(moduleFolder) + L"\\RecName\\RecName*.dll").c_str(), &findData);
	if( hFind != INVALID_HANDLE_VALUE ){
		do{
			if( (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 ){
				ret.push_back(findData.cFileName);
			}
		}while( FindNextFile(hFind, &findData) );
		FindClose(hFind);
	}
	return ret;
}

bool CEpgTimerSrvSetting::GetDlgButtonCheck(HWND hwnd, int id)
{
	return Button_GetCheck(GetDlgItem(hwnd, id)) == BST_CHECKED;
}

void CEpgTimerSrvSetting::SetDlgButtonCheck(HWND hwnd, int id, bool check)
{
	Button_SetCheck(GetDlgItem(hwnd, id), check ? BST_CHECKED : BST_UNCHECKED);
}

void CEpgTimerSrvSetting::AddListBoxItem(HWND hList, HWND hItem)
{
	vector<WCHAR> buff(GetWindowTextLength(hItem) + 1, L'\0');
	GetWindowText(hItem, &buff.front(), (int)buff.size());
	wstring item = &buff.front();
	if( item.empty() == false ){
		for( int i = 0; i < ListBox_GetCount(hList); i++ ){
			int len = ListBox_GetTextLen(hList, i);
			if( len >= (int)item.size() ){
				buff.assign(len + 1, L'\0');
				ListBox_GetText(hList, i, &buff.front());
				if( CompareNoCase(item, &buff.front()) == 0 ){
					//すでにある
					return;
				}
			}
		}
		ListBox_AddString(hList, item.c_str());
	}
}

void CEpgTimerSrvSetting::DeleteListBoxItem(HWND hList)
{
	int sel = ListBox_GetCurSel(hList);
	if( sel >= 0 ){
		ListBox_DeleteString(hList, sel);
	}
}

void CEpgTimerSrvSetting::MoveListBoxItem(HWND hList, int step)
{
	int sel = ListBox_GetCurSel(hList);
	if( sel >= 0 ){
		int len = ListBox_GetTextLen(hList, sel);
		if( len >= 0 ){
			vector<WCHAR> buff(len + 1, L'\0');
			ListBox_GetText(hList, sel, &buff.front());
			LRESULT data = ListBox_GetItemData(hList, sel);
			ListBox_DeleteString(hList, sel);
			int count = ListBox_GetCount(hList);
			sel = min(max(sel + step, 0), count);
			ListBox_InsertString(hList, sel, &buff.front());
			ListBox_SetItemData(hList, sel, data);
			ListBox_SetCurSel(hList, sel);
		}
	}
}

void CEpgTimerSrvSetting::OnSelChangeListBoxItem(HWND hList, HWND hItem)
{
	int sel = ListBox_GetCurSel(hList);
	if( sel >= 0 ){
		int len = ListBox_GetTextLen(hList, sel);
		if( len >= 0 ){
			vector<WCHAR> buff(len + 1, L'\0');
			ListBox_GetText(hList, sel, &buff.front());
			SetWindowText(hItem, &buff.front());
		}
	}
}

INT_PTR CEpgTimerSrvSetting::ShowDialog()
{
	this->hwndTop = NULL;
	this->hwndChild[0] = this->hwndBasic = NULL;
	this->hwndChild[1] = this->hwndEpg = NULL;
	this->hwndChild[2] = this->hwndRec = NULL;
	this->hwndChild[3] = this->hwndReserve = NULL;
	this->hwndChild[4] = this->hwndOther = NULL;
	INITCOMMONCONTROLSEX icce;
	icce.dwSize = sizeof(icce);
	icce.dwICC = ICC_LISTVIEW_CLASSES | ICC_TAB_CLASSES;
	InitCommonControlsEx(&icce);
	return DialogBoxParam(NULL, MAKEINTRESOURCE(IDD_DIALOG_SETTING), NULL, DlgProc, (LPARAM)this);
}

INT_PTR CEpgTimerSrvSetting::OnInitDialog()
{
	HWND hTab = GetDlgItem(this->hwndTop, IDC_TAB);
	TCITEM tci;
	tci.mask = TCIF_TEXT;
	tci.pszText = L"基本設定";
	TabCtrl_InsertItem(hTab, 0, &tci);
	tci.pszText = L"EPG取得";
	TabCtrl_InsertItem(hTab, 1, &tci);
	tci.pszText = L"録画動作";
	TabCtrl_InsertItem(hTab, 2, &tci);
	tci.pszText = L"予約情報管理";
	TabCtrl_InsertItem(hTab, 3, &tci);
	tci.pszText = L"その他";
	TabCtrl_InsertItem(hTab, 4, &tci);
	this->hwndChild[0] = this->hwndBasic = CreateDialogParam(NULL, MAKEINTRESOURCE(IDD_DIALOG_SETTING_BASIC), this->hwndTop, ChildDlgProc, (LPARAM)this);
	this->hwndChild[1] = this->hwndEpg = CreateDialogParam(NULL, MAKEINTRESOURCE(IDD_DIALOG_SETTING_EPG), this->hwndTop, ChildDlgProc, (LPARAM)this);
	this->hwndChild[2] = this->hwndRec = CreateDialogParam(NULL, MAKEINTRESOURCE(IDD_DIALOG_SETTING_REC), this->hwndTop, ChildDlgProc, (LPARAM)this);
	this->hwndChild[3] = this->hwndReserve = CreateDialogParam(NULL, MAKEINTRESOURCE(IDD_DIALOG_SETTING_RESERVE), this->hwndTop, ChildDlgProc, (LPARAM)this);
	this->hwndChild[4] = this->hwndOther = CreateDialogParam(NULL, MAKEINTRESOURCE(IDD_DIALOG_SETTING_OTHER), this->hwndTop, ChildDlgProc, (LPARAM)this);
	RECT rc;
	GetWindowRect(hTab, &rc);
	TabCtrl_AdjustRect(hTab, FALSE, &rc);
	POINT pt;
	pt.x = rc.left;
	pt.y = rc.top;
	ScreenToClient(this->hwndTop, &pt);
	for( int i = 0; i < _countof(this->hwndChild); i++ ){
		MoveWindow(this->hwndChild[i], pt.x, pt.y, rc.right - rc.left, rc.bottom - rc.top, TRUE);
	}

	wstring commonIniPath;
	GetCommonIniPath(commonIniPath);
	wstring settingPath = GetPrivateProfileToString(L"Set", L"DataSavePath", L"", commonIniPath.c_str());
	ChkFolderPath(settingPath);
	if( settingPath.empty() ){
		GetDefSettingPath(settingPath);
		//既定の設定関係保存フォルダが存在しなければ特別に作る
		if( GetFileAttributes(settingPath.c_str()) == INVALID_FILE_ATTRIBUTES ){
			CreateDirectory(settingPath.c_str(), NULL);
		}
	}
	wstring moduleFolder;
	GetModuleFolderPath(moduleFolder);
	wstring iniPath;
	GetModuleIniPath(iniPath);
	wstring viewAppIniPath = moduleFolder + L"\\ViewApp.ini";

	SETTING setting = LoadSetting(iniPath.c_str());
	this->chSet.ParseText((settingPath + L"\\ChSet5.txt").c_str());

	//基本設定
	HWND hwnd = this->hwndBasic;
	SetDlgItemText(hwnd, IDC_EDIT_SET_DATA_SAVE_PATH, settingPath.c_str());
	SetDlgItemText(hwnd, IDC_EDIT_SET_REC_EXE_PATH, GetPrivateProfileToString(L"SET", L"RecExePath", (moduleFolder + L"\\EpgDataCap_Bon.exe").c_str(), commonIniPath.c_str()).c_str());
	SetDlgItemText(hwnd, IDC_EDIT_SET_REC_CMD_BON, GetPrivateProfileToString(L"APP_CMD_OPT", L"Bon", L"-d", viewAppIniPath.c_str()).c_str());
	SetDlgItemText(hwnd, IDC_EDIT_SET_REC_CMD_MIN, GetPrivateProfileToString(L"APP_CMD_OPT", L"Min", L"-min", viewAppIniPath.c_str()).c_str());
	SetDlgItemText(hwnd, IDC_EDIT_SET_REC_CMD_VIEW_OFF, GetPrivateProfileToString(L"APP_CMD_OPT", L"ViewOff", L"-noview", viewAppIniPath.c_str()).c_str());
	int num = GetPrivateProfileInt(L"SET", L"RecFolderNum", 0, commonIniPath.c_str());
	if( num <= 0 ){
		ListBox_AddString(GetDlgItem(hwnd, IDC_LIST_SET_REC_FOLDER), settingPath.c_str());
	}else{
		for( int i = 0; i < num; i++ ){
			WCHAR key[32];
			swprintf_s(key, L"RecFolderPath%d", i);
			ListBox_AddString(GetDlgItem(hwnd, IDC_LIST_SET_REC_FOLDER), GetPrivateProfileToString(L"SET", key, L"", commonIniPath.c_str()).c_str());
		}
	}
	SetDlgItemText(hwnd, IDC_EDIT_SET_REC_INFO_FOLDER, GetPrivateProfileToString(L"SET", L"RecInfoFolder", L"", commonIniPath.c_str()).c_str());

	vector<pair<wstring, wstring>> bonFileNameList = EnumBonFileName(settingPath.c_str());
	vector<WORD> priorityList;
	for( size_t i = 0; i < bonFileNameList.size(); i++ ){
		priorityList.push_back((WORD)GetPrivateProfileInt(bonFileNameList[i].first.c_str(), L"Priority", 0xFFFF, iniPath.c_str()));
	}
	for( size_t i = 1; i < priorityList.size(); i++ ){
		for( size_t j = 1; j < priorityList.size(); j++ ){
			if( priorityList[j] < priorityList[j - 1] ){
				std::swap(priorityList[j], priorityList[j - 1]);
				bonFileNameList[j].swap(bonFileNameList[j - 1]);
			}
		}
	}
	for( int i = 0; i < 100; i++ ){
		WCHAR val[16];
		swprintf_s(val, L"%d", i);
		ComboBox_AddString(GetDlgItem(hwnd, IDC_COMBO_SET_BON_COUNT), val);
		ComboBox_AddString(GetDlgItem(hwnd, IDC_COMBO_SET_BON_EPG_COUNT), val);
	}
	ComboBox_AddString(GetDlgItem(hwnd, IDC_COMBO_SET_BON_EPG_COUNT), L"すべて");

	for( size_t i = 0; i < bonFileNameList.size(); i++ ){
		WORD count = (WORD)GetPrivateProfileInt(bonFileNameList[i].first.c_str(), L"Count", 0, iniPath.c_str()) % 100;
		WORD epgCount = 0;
		if( GetPrivateProfileInt(bonFileNameList[i].first.c_str(), L"GetEpg", 1, iniPath.c_str()) != 0 ){
			epgCount = (WORD)GetPrivateProfileInt(bonFileNameList[i].first.c_str(), L"EPGCount", 0, iniPath.c_str());
			if( epgCount == 0 ){
				epgCount = 100;
			}
		}
		ListBox_AddString(GetDlgItem(hwnd, IDC_LIST_SET_BON), bonFileNameList[i].first.c_str());
		ListBox_SetItemData(GetDlgItem(hwnd, IDC_LIST_SET_BON), i, MAKEWORD(count, epgCount));
		ListBox_SetCurSel(GetDlgItem(hwnd, IDC_LIST_SET_BON), 0);
	}

	//EPG取得
	hwnd = this->hwndEpg;
	ListView_SetExtendedListViewStyleEx(GetDlgItem(hwnd, IDC_LIST_SET_EPG_SERVICE), LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
	GetClientRect(GetDlgItem(hwnd, IDC_LIST_SET_EPG_SERVICE), &rc);
	LVCOLUMN lvc;
	lvc.mask = LVCF_WIDTH;
	lvc.cx = rc.right - GetSystemMetrics(SM_CXVSCROLL) - 8;
	ListView_InsertColumn(GetDlgItem(hwnd, IDC_LIST_SET_EPG_SERVICE), 0, &lvc);
	ListView_SetExtendedListViewStyleEx(GetDlgItem(hwnd, IDC_LIST_SET_EPG_TIME), LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
	lvc.mask = LVCF_WIDTH | LVCF_TEXT;
	lvc.cx /= 2;
	lvc.pszText = L"開始時間";
	ListView_InsertColumn(GetDlgItem(hwnd, IDC_LIST_SET_EPG_TIME), 0, &lvc);
	lvc.pszText = L"種別(BS,CS1,2,3)";
	ListView_InsertColumn(GetDlgItem(hwnd, IDC_LIST_SET_EPG_TIME), 1, &lvc);

	for( int i = 0; i < 8; i++ ){
		static const WCHAR val[8][2] = { L"", L"月", L"火", L"水", L"木", L"金", L"土", L"日" };
		ComboBox_AddString(GetDlgItem(hwnd, IDC_COMBO_SET_EPG_WDAY), val[i]);
	}
	for( int i = 0; i < 24; i++ ){
		WCHAR val[16];
		swprintf_s(val, L"%02d", i);
		ComboBox_AddString(GetDlgItem(hwnd, IDC_COMBO_SET_EPG_HH), val);
	}
	for( int i = 0; i < 60; i++ ){
		WCHAR val[16];
		swprintf_s(val, L"%02d", i);
		ComboBox_AddString(GetDlgItem(hwnd, IDC_COMBO_SET_EPG_MM), val);
	}
	for( auto itr = this->chSet.GetMap().cbegin(); itr != this->chSet.GetMap().end(); itr++ ){
		HWND hItem = GetDlgItem(hwnd, IDC_LIST_SET_EPG_SERVICE);
		LVITEM lvi;
		lvi.mask = LVIF_TEXT;
		lvi.iItem = ListView_GetItemCount(hItem);
		lvi.iSubItem = 0;
		lvi.pszText = (LPWSTR)itr->second.serviceName.c_str();
		ListView_InsertItem(hItem, &lvi);
		ListView_SetCheckState(hItem, lvi.iItem, itr->second.epgCapFlag);
	}
	bool basicOnlyBS = GetPrivateProfileInt(L"SET", L"BSBasicOnly", 1, commonIniPath.c_str()) != 0;
	bool basicOnlyCS1 = GetPrivateProfileInt(L"SET", L"CS1BasicOnly", 1, commonIniPath.c_str()) != 0;
	bool basicOnlyCS2 = GetPrivateProfileInt(L"SET", L"CS2BasicOnly", 1, commonIniPath.c_str()) != 0;
	bool basicOnlyCS3 = GetPrivateProfileInt(L"SET", L"CS3BasicOnly", 0, commonIniPath.c_str()) != 0;
	for( size_t i = 0; i < setting.epgCapTimeList.size(); i++ ){
		int flags = setting.epgCapTimeList[i].second.second;
		SetDlgButtonCheck(hwnd, IDC_CHECK_SET_EPG_BS, flags < 0 ? basicOnlyBS : ((flags & 1) != 0));
		SetDlgButtonCheck(hwnd, IDC_CHECK_SET_EPG_CS1, flags < 0 ? basicOnlyCS1 : ((flags & 2) != 0));
		SetDlgButtonCheck(hwnd, IDC_CHECK_SET_EPG_CS2, flags < 0 ? basicOnlyCS2 : ((flags & 4) != 0));
		SetDlgButtonCheck(hwnd, IDC_CHECK_SET_EPG_CS3, flags < 0 ? basicOnlyCS3 : ((flags & 8) != 0));
		int weekMin = setting.epgCapTimeList[i].second.first;
		ComboBox_SetCurSel(GetDlgItem(hwnd, IDC_COMBO_SET_EPG_WDAY), weekMin / 1440);
		ComboBox_SetCurSel(GetDlgItem(hwnd, IDC_COMBO_SET_EPG_HH), weekMin / 60 % 24);
		ComboBox_SetCurSel(GetDlgItem(hwnd, IDC_COMBO_SET_EPG_MM), weekMin % 60);
		AddEpgTime(setting.epgCapTimeList[i].first);
	}
	SetDlgButtonCheck(hwnd, IDC_CHECK_SET_EPG_BS, basicOnlyBS);
	SetDlgButtonCheck(hwnd, IDC_CHECK_SET_EPG_CS1, basicOnlyCS1);
	SetDlgButtonCheck(hwnd, IDC_CHECK_SET_EPG_CS2, basicOnlyCS2);
	SetDlgButtonCheck(hwnd, IDC_CHECK_SET_EPG_CS3, basicOnlyCS3);
	ComboBox_SetCurSel(GetDlgItem(hwnd, IDC_COMBO_SET_EPG_WDAY), 0);
	ComboBox_SetCurSel(GetDlgItem(hwnd, IDC_COMBO_SET_EPG_HH), 0);
	ComboBox_SetCurSel(GetDlgItem(hwnd, IDC_COMBO_SET_EPG_MM), 0);
	SetDlgItemInt(hwnd, IDC_EDIT_SET_EPG_NG_CAP, setting.ngEpgCapTime, FALSE);
	SetDlgItemInt(hwnd, IDC_EDIT_SET_EPG_NG_CAP_TUNER, setting.ngEpgCapTunerTime, FALSE);

	//録画動作
	hwnd = this->hwndRec;
	CheckRadioButton(hwnd, IDC_RADIO_SET_REC_END_NONE, IDC_RADIO_SET_REC_END_SHUTDOWN, IDC_RADIO_SET_REC_END_NONE + setting.recEndMode % 4);
	SetDlgButtonCheck(hwnd, IDC_CHECK_SET_REBOOT, setting.reboot);
	SetDlgItemInt(hwnd, IDC_EDIT_SET_WAKE_TIME, setting.wakeTime, FALSE);
	for( size_t i = 0; i < setting.noSuspendExeList.size(); i++ ){
		ListBox_AddString(GetDlgItem(hwnd, IDC_LIST_SET_NO_EXE), setting.noSuspendExeList[i].c_str());
	}
	SetDlgItemInt(hwnd, IDC_EDIT_SET_NO_STANDBY, setting.noStandbyTime, FALSE);
	SetDlgButtonCheck(hwnd, IDC_CHECK_SET_NO_USE_PC, setting.noUsePC);
	SetDlgItemInt(hwnd, IDC_EDIT_SET_NO_USE_PC, setting.noUsePCTime, FALSE);
	SetDlgButtonCheck(hwnd, IDC_CHECK_SET_NO_FILE_STREAMING, setting.noFileStreaming);
	SetDlgButtonCheck(hwnd, IDC_CHECK_SET_NO_SHARE_FILE, setting.noShareFile);
	SetDlgItemInt(hwnd, IDC_EDIT_SET_START_MARGIN, setting.startMargin, TRUE);
	SetDlgItemInt(hwnd, IDC_EDIT_SET_END_MARGIN, setting.endMargin, TRUE);
	SetDlgItemInt(hwnd, IDC_EDIT_SET_APP_WAKE_TIME, setting.recAppWakeTime, FALSE);
	for( int i = 0; i < 6; i++ ){
		static const LPCWSTR val[6] = { L"リアルタイム", L"高", L"通常以上", L"通常", L"通常以下", L"低" };
		ComboBox_AddString(GetDlgItem(hwnd, IDC_COMBO_SET_PROCESS_PRIORITY), val[i]);
	}
	ComboBox_SetCurSel(GetDlgItem(hwnd, IDC_COMBO_SET_PROCESS_PRIORITY), min(max(setting.processPriority, 0), 5));
	SetDlgButtonCheck(hwnd, IDC_CHECK_SET_REC_MIN_WAKE, setting.recMinWake);
	SetDlgButtonCheck(hwnd, IDC_CHECK_SET_REC_VIEW, setting.recView);
	SetDlgButtonCheck(hwnd, IDC_CHECK_SET_DROP_LOG, setting.dropLog);
	SetDlgButtonCheck(hwnd, IDC_CHECK_SET_PG_INFO_LOG, setting.pgInfoLog);
	SetDlgButtonCheck(hwnd, IDC_CHECK_SET_REC_NW, setting.recNW);
	SetDlgButtonCheck(hwnd, IDC_CHECK_SET_KEEP_DISK, setting.keepDisk);
	SetDlgButtonCheck(hwnd, IDC_CHECK_SET_REC_OVERWRITE, setting.recOverWrite);

	//予約情報管理
	hwnd = this->hwndReserve;
	SetDlgButtonCheck(hwnd, IDC_CHECK_SET_BACK_PRIORITY, setting.backPriority);
	SetDlgButtonCheck(hwnd, IDC_CHECK_SET_FIXED_TUNER_PRIORITY, setting.fixedTunerPriority);
	SetDlgButtonCheck(hwnd, IDC_CHECK_SET_REC_INFO_FOLDER_ONLY, setting.recInfoFolderOnly);
	SetDlgButtonCheck(hwnd, IDC_CHECK_SET_REC_INFO_DEL_FILE, GetPrivateProfileInt(L"SET", L"RecInfoDelFile", 0, commonIniPath.c_str()) != 0);
	SetDlgButtonCheck(hwnd, IDC_CHECK_SET_APPLY_EXT_TO, setting.applyExtToRecInfoDel);
	SetDlgButtonCheck(hwnd, IDC_CHECK_SET_AUTODEL, setting.autoDel);
	for( size_t i = 0; i < setting.delExtList.size(); i++ ){
		ListBox_AddString(GetDlgItem(hwnd, IDC_LIST_SET_DEL_EXT), setting.delExtList[i].c_str());
	}
	for( size_t i = 0; i < setting.delChkList.size(); i++ ){
		ListBox_AddString(GetDlgItem(hwnd, IDC_LIST_SET_DEL_CHK), setting.delChkList[i].c_str());
	}
	SetDlgButtonCheck(hwnd, IDC_CHECK_SET_RECNAME_PLUGIN, setting.recNamePlugIn);
	vector<wstring> plugInList = EnumRecNamePlugInFileName(moduleFolder.c_str());
	size_t plugInSel = plugInList.size();
	for( size_t i = 0; i < plugInList.size(); i++ ){
		ComboBox_AddString(GetDlgItem(hwnd, IDC_COMBO_SET_RECNAME_PLUGIN), plugInList[i].c_str());
		if( CompareNoCase(plugInList[i], setting.recNamePlugInFile) == 0 ){
			plugInSel = i;
		}
	}
	if( plugInSel == plugInList.size() ){
		ComboBox_AddString(GetDlgItem(hwnd, IDC_COMBO_SET_RECNAME_PLUGIN), setting.recNamePlugInFile.c_str());
	}
	ComboBox_SetCurSel(GetDlgItem(hwnd, IDC_COMBO_SET_RECNAME_PLUGIN), plugInSel);
	SetDlgButtonCheck(hwnd, IDC_CHECK_SET_NO_CHK_YEN, setting.noChkYen);
	CheckRadioButton(hwnd, IDC_RADIO_SET_DEL_RESERVE_DEL, IDC_RADIO_SET_DEL_RESERVE_CANCEL, IDC_RADIO_SET_DEL_RESERVE_DEL + setting.delReserveMode % 3);

	//その他
	hwnd = this->hwndOther;
	SetDlgButtonCheck(hwnd, IDC_CHECK_SET_TCP_SERVER, GetPrivateProfileInt(L"SET", L"EnableTCPSrv", 0, iniPath.c_str()) != 0);
	SetDlgItemInt(hwnd, IDC_EDIT_SET_TCP_PORT, GetPrivateProfileInt(L"SET", L"TCPPort", 4510, iniPath.c_str()), FALSE);
	SetDlgItemText(hwnd, IDC_EDIT_SET_TCP_ACL, GetPrivateProfileToString(L"SET", L"TCPAccessControlList", L"+127.0.0.1,+192.168.0.0/16", iniPath.c_str()).c_str());
	SetDlgItemInt(hwnd, IDC_EDIT_SET_TCP_RES_TO, GetPrivateProfileInt(L"SET", L"TCPResponseTimeoutSec", 120, iniPath.c_str()), FALSE);
	SetDlgButtonCheck(hwnd, IDC_CHECK_SET_AUTODEL_REC_INFO, setting.autoDelRecInfo);
	SetDlgItemInt(hwnd, IDC_EDIT_SET_AUTODEL_REC_INFO, setting.autoDelRecInfoNum, FALSE);
	for( int i = 0; i <= 14; i++ ){
		WCHAR val[16];
		swprintf_s(val, L"%d", i);
		ComboBox_AddString(GetDlgItem(hwnd, IDC_COMBO_SET_EPG_ARCHIVE_PERIOD), val);
	}
	ComboBox_SetCurSel(GetDlgItem(hwnd, IDC_COMBO_SET_EPG_ARCHIVE_PERIOD), min(max(setting.epgArchivePeriodHour / 24, 0), 14));
	SetDlgButtonCheck(hwnd, IDC_CHECK_SET_TIME_SYNC, setting.timeSync);
	SetDlgButtonCheck(hwnd, IDC_CHECK_SET_RESIDENT, setting.residentMode >= 1);
	SetDlgButtonCheck(hwnd, IDC_CHECK_SET_SHOW_TRAY, setting.residentMode != 1);
	SetDlgButtonCheck(hwnd, IDC_CHECK_SET_NO_BALLOON_TIP, setting.noBalloonTip);
	SetDlgButtonCheck(hwnd, IDC_CHECK_SET_SAVE_NOTIFY_LOG, setting.saveNotifyLog);
	SetDlgButtonCheck(hwnd, IDC_CHECK_SET_SAVE_DEBUG_LOG, GetPrivateProfileInt(L"SET", L"SaveDebugLog", 0, iniPath.c_str()) != 0);
	SetDlgButtonCheck(hwnd, IDC_CHECK_SET_COMPAT_TKNTREC, GetPrivateProfileInt(L"SET", L"CompatFlags", 0, iniPath.c_str()) % 4096 == 4095);

	for( size_t i = 0; i < setting.viewBonList.size(); i++ ){
		ListBox_AddString(GetDlgItem(hwnd, IDC_LIST_SET_VIEW_BON), setting.viewBonList[i].c_str());
	}
	for( size_t i = 0; i < bonFileNameList.size(); i++ ){
		ComboBox_AddString(GetDlgItem(hwnd, IDC_COMBO_SET_VIEW_BON), bonFileNameList[i].first.c_str());
		ComboBox_SetCurSel(GetDlgItem(hwnd, IDC_COMBO_SET_VIEW_BON), 0);
	}

	//連動処理のため
	SendMessage(this->hwndBasic, WM_COMMAND, MAKELONG(IDC_LIST_SET_BON, LBN_SELCHANGE), 0);
	SendMessage(this->hwndRec, WM_COMMAND, IDC_CHECK_SET_NO_USE_PC, 0);
	SendMessage(this->hwndReserve, WM_COMMAND, IDC_CHECK_SET_REC_INFO_DEL_FILE, 0);
	SendMessage(this->hwndReserve, WM_COMMAND, IDC_CHECK_SET_AUTODEL, 0);
	SendMessage(this->hwndReserve, WM_COMMAND, IDC_CHECK_SET_RECNAME_PLUGIN, 0);
	SendMessage(this->hwndOther, WM_COMMAND, IDC_CHECK_SET_TCP_SERVER, 0);
	SendMessage(this->hwndOther, WM_COMMAND, IDC_CHECK_SET_AUTODEL_REC_INFO, 0);
	SendMessage(this->hwndOther, WM_COMMAND, IDC_CHECK_SET_RESIDENT, 0);
	ToggleStartup(false, false);

	TabCtrl_SetCurSel(hTab, 0);
	ShowWindow(this->hwndBasic, SW_SHOW);

	return TRUE;
}

void CEpgTimerSrvSetting::OnTcnSelchangeTab()
{
	int sel = TabCtrl_GetCurSel(GetDlgItem(this->hwndTop, IDC_TAB));
	if( 0 <= sel && sel < _countof(this->hwndChild) ){
		ShowWindow(this->hwndChild[sel], SW_SHOW);
		for( int i = 0; i < _countof(this->hwndChild); i++ ){
			if( i != sel ){
				ShowWindow(this->hwndChild[i], SW_HIDE);
			}
		}
	}
}

void CEpgTimerSrvSetting::OnBnClickedOk()
{
	wstring commonIniPath;
	GetCommonIniPath(commonIniPath);
	wstring settingPath;
	GetDefSettingPath(settingPath);
	wstring moduleFolder;
	GetModuleFolderPath(moduleFolder);
	wstring iniPath;
	GetModuleIniPath(iniPath);
	wstring viewAppIniPath = moduleFolder + L"\\ViewApp.ini";

	WCHAR buff[max(1024, MAX_PATH)] = {};
	WCHAR key[32];

	//[SET]セクションをファイル先頭に置くため最初にこれを書く
	WritePrivateProfileInt(L"SET", L"NGEpgCapTime", GetDlgItemInt(this->hwndEpg, IDC_EDIT_SET_EPG_NG_CAP, NULL, FALSE), iniPath.c_str());

	//基本設定
	HWND hwnd = this->hwndBasic;
	GetDlgItemText(hwnd, IDC_EDIT_SET_DATA_SAVE_PATH, buff, MAX_PATH);
	if( CompareNoCase(settingPath, buff) == 0 ){
		//既定値なので記録しない
		WritePrivateProfileString(L"SET", L"DataSavePath", NULL, commonIniPath.c_str());
	}else{
		WritePrivateProfileString(L"SET", L"DataSavePath", buff, commonIniPath.c_str());
		settingPath = buff;
		ChkFolderPath(settingPath);
	}
	GetDlgItemText(hwnd, IDC_EDIT_SET_REC_EXE_PATH, buff, MAX_PATH);
	WritePrivateProfileString(L"SET", L"RecExePath", CompareNoCase(moduleFolder + L"\\EpgDataCap_Bon.exe", buff) == 0 ? NULL : buff, commonIniPath.c_str());
	GetDlgItemText(hwnd, IDC_EDIT_SET_REC_CMD_BON, buff, _countof(buff));
	if( wcscmp(buff, GetPrivateProfileToString(L"APP_CMD_OPT", L"Bon", L"-d", viewAppIniPath.c_str()).c_str()) != 0 ){
		WritePrivateProfileString(L"APP_CMD_OPT", L"Bon", buff, viewAppIniPath.c_str());
	}
	GetDlgItemText(hwnd, IDC_EDIT_SET_REC_CMD_MIN, buff, _countof(buff));
	if( wcscmp(buff, GetPrivateProfileToString(L"APP_CMD_OPT", L"Min", L"-min", viewAppIniPath.c_str()).c_str()) != 0 ){
		WritePrivateProfileString(L"APP_CMD_OPT", L"Min", buff, viewAppIniPath.c_str());
	}
	GetDlgItemText(hwnd, IDC_EDIT_SET_REC_CMD_VIEW_OFF, buff, _countof(buff));
	if( wcscmp(buff, GetPrivateProfileToString(L"APP_CMD_OPT", L"ViewOff", L"-noview", viewAppIniPath.c_str()).c_str()) != 0 ){
		WritePrivateProfileString(L"APP_CMD_OPT", L"ViewOff", buff, viewAppIniPath.c_str());
	}
	int num = 0;
	for( int i = 0; i < ListBox_GetCount(GetDlgItem(hwnd, IDC_LIST_SET_REC_FOLDER)); i++ ){
		if( ListBox_GetTextLen(GetDlgItem(hwnd, IDC_LIST_SET_REC_FOLDER), i) < MAX_PATH ){
			ListBox_GetText(GetDlgItem(hwnd, IDC_LIST_SET_REC_FOLDER), i, buff);
			if( num == 0 && i + 1 >= ListBox_GetCount(GetDlgItem(hwnd, IDC_LIST_SET_REC_FOLDER)) && CompareNoCase(settingPath, buff) == 0 ){
				//既定値なので記録しない
				break;
			}
			swprintf_s(key, L"RecFolderPath%d", num++);
			WritePrivateProfileString(L"SET", key, buff, commonIniPath.c_str());
		}
	}
	WritePrivateProfileInt(L"SET", L"RecFolderNum", num, commonIniPath.c_str());
	for(;;){
		//掃除
		swprintf_s(key, L"RecFolderPath%d", num++);
		if( GetPrivateProfileToString(L"SET", key, L"", commonIniPath.c_str()).empty() ){
			break;
		}
		WritePrivateProfileString(L"SET", key, NULL, commonIniPath.c_str());
	}
	GetDlgItemText(hwnd, IDC_EDIT_SET_REC_INFO_FOLDER, buff, MAX_PATH);
	WritePrivateProfileString(L"SET", L"RecInfoFolder", buff[0] ? buff : NULL, commonIniPath.c_str());
	for( int i = 0; i < ListBox_GetCount(GetDlgItem(hwnd, IDC_LIST_SET_BON)); i++ ){
		if( ListBox_GetTextLen(GetDlgItem(hwnd, IDC_LIST_SET_BON), i) < MAX_PATH ){
			ListBox_GetText(GetDlgItem(hwnd, IDC_LIST_SET_BON), i, buff);
			WORD data = (WORD)ListBox_GetItemData(GetDlgItem(hwnd, IDC_LIST_SET_BON), i);
			WritePrivateProfileInt(buff, L"Count", LOBYTE(data), iniPath.c_str());
			WritePrivateProfileInt(buff, L"GetEpg", HIBYTE(data) != 0, iniPath.c_str());
			WritePrivateProfileInt(buff, L"EPGCount", HIBYTE(data) % 100, iniPath.c_str());
			WritePrivateProfileInt(buff, L"Priority", i, iniPath.c_str());
		}
	}

	//EPG取得
	hwnd = this->hwndEpg;
	bool chSetModified = false;
	num = 0;
	for( int i = 0; i < ListView_GetItemCount(GetDlgItem(hwnd, IDC_LIST_SET_EPG_SERVICE)); i++ ){
		auto itr = this->chSet.GetMap().cbegin();
		std::advance(itr, i);
		if( (ListView_GetCheckState(GetDlgItem(hwnd, IDC_LIST_SET_EPG_SERVICE), i) != FALSE) != (itr->second.epgCapFlag != FALSE) ){
			chSetModified = true;
			this->chSet.SetEpgCapMode(itr->second.originalNetworkID, itr->second.transportStreamID, itr->second.serviceID, !(itr->second.epgCapFlag));
		}
	}
	//チェックを操作したときだけ保存する
	if( chSetModified ){
		this->chSet.SaveText();
	}
	num = 0;
	for( int i = 0; i < ListView_GetItemCount(GetDlgItem(hwnd, IDC_LIST_SET_EPG_TIME)); i++ ){
		WCHAR w[32] = {};
		WCHAR f[32] = {};
		ListView_GetItemText(GetDlgItem(hwnd, IDC_LIST_SET_EPG_TIME), i, 0, w, _countof(w));
		ListView_GetItemText(GetDlgItem(hwnd, IDC_LIST_SET_EPG_TIME), i, 1, f, _countof(f));
		if( wcslen(w) == 6 && wcslen(f) == 7 ){
			swprintf_s(key, L"%d", num);
			WCHAR val[32];
			swprintf_s(val, L"%s%s", &w[1],
			           w[0] == L'月' ? L"w1" : w[0] == L'火' ? L"w2" : w[0] == L'水' ? L"w3" : w[0] == L'木' ? L"w4" :
			           w[0] == L'金' ? L"w5" : w[0] == L'土' ? L"w6" : w[0] == L'日' ? L"w7" : L"");
			WritePrivateProfileString(L"EPG_CAP", key, val, iniPath.c_str());
			swprintf_s(key, L"%dSelect", num);
			WritePrivateProfileInt(L"EPG_CAP", key, ListView_GetCheckState(GetDlgItem(hwnd, IDC_LIST_SET_EPG_TIME), i) != 0, iniPath.c_str());
			swprintf_s(key, L"%dBasicOnlyFlags", num++);
			WritePrivateProfileInt(L"EPG_CAP", key, (f[0] == L'基') + (f[2] == L'基') * 2 + (f[4] == L'基') * 4 + (f[6] == L'基') * 8, iniPath.c_str());
		}
	}
	WritePrivateProfileInt(L"EPG_CAP", L"Count", num, iniPath.c_str());
	for(;;){
		//掃除
		swprintf_s(key, L"%d", num);
		if( GetPrivateProfileToString(L"EPG_CAP", key, L"", iniPath.c_str()).empty() ){
			break;
		}
		WritePrivateProfileString(L"EPG_CAP", key, NULL, iniPath.c_str());
		swprintf_s(key, L"%dSelect", num);
		WritePrivateProfileString(L"EPG_CAP", key, NULL, iniPath.c_str());
		swprintf_s(key, L"%dBasicOnlyFlags", num++);
		WritePrivateProfileString(L"EPG_CAP", key, NULL, iniPath.c_str());
	}
	WritePrivateProfileInt(L"SET", L"BSBasicOnly", GetDlgButtonCheck(hwnd, IDC_CHECK_SET_EPG_BS), commonIniPath.c_str());
	WritePrivateProfileInt(L"SET", L"CS1BasicOnly", GetDlgButtonCheck(hwnd, IDC_CHECK_SET_EPG_CS1), commonIniPath.c_str());
	WritePrivateProfileInt(L"SET", L"CS2BasicOnly", GetDlgButtonCheck(hwnd, IDC_CHECK_SET_EPG_CS2), commonIniPath.c_str());
	WritePrivateProfileInt(L"SET", L"CS3BasicOnly", GetDlgButtonCheck(hwnd, IDC_CHECK_SET_EPG_CS3), commonIniPath.c_str());
	WritePrivateProfileInt(L"SET", L"NGEpgCapTunerTime", GetDlgItemInt(hwnd, IDC_EDIT_SET_EPG_NG_CAP_TUNER, NULL, FALSE), iniPath.c_str());

	//録画動作
	hwnd = this->hwndRec;
	WritePrivateProfileInt(L"SET", L"RecEndMode",
	                       GetDlgButtonCheck(hwnd, IDC_RADIO_SET_REC_END_STANDBY) ? 1 :
	                       GetDlgButtonCheck(hwnd, IDC_RADIO_SET_REC_END_SUSPEND) ? 2 :
	                       GetDlgButtonCheck(hwnd, IDC_RADIO_SET_REC_END_SHUTDOWN) ? 3 : 0, iniPath.c_str());
	WritePrivateProfileInt(L"SET", L"Reboot", GetDlgButtonCheck(hwnd, IDC_CHECK_SET_REBOOT), iniPath.c_str());
	WritePrivateProfileInt(L"SET", L"WakeTime", GetDlgItemInt(hwnd, IDC_EDIT_SET_WAKE_TIME, NULL, FALSE), iniPath.c_str());
	num = 0;
	for( int i = 0; i < ListBox_GetCount(GetDlgItem(hwnd, IDC_LIST_SET_NO_EXE)); i++ ){
		if( ListBox_GetTextLen(GetDlgItem(hwnd, IDC_LIST_SET_NO_EXE), i) < MAX_PATH ){
			ListBox_GetText(GetDlgItem(hwnd, IDC_LIST_SET_NO_EXE), i, buff);
			swprintf_s(key, L"%d", num++);
			WritePrivateProfileString(L"NO_SUSPEND", key, buff, iniPath.c_str());
		}
	}
	WritePrivateProfileInt(L"NO_SUSPEND", L"Count", num, iniPath.c_str());
	for(;;){
		//掃除
		swprintf_s(key, L"%d", num++);
		if( GetPrivateProfileToString(L"NO_SUSPEND", key, L"", iniPath.c_str()).empty() ){
			break;
		}
		WritePrivateProfileString(L"NO_SUSPEND", key, NULL, iniPath.c_str());
	}
	WritePrivateProfileInt(L"NO_SUSPEND", L"NoStandbyTime", GetDlgItemInt(hwnd, IDC_EDIT_SET_NO_STANDBY, NULL, FALSE), iniPath.c_str());
	WritePrivateProfileInt(L"NO_SUSPEND", L"NoUsePC", GetDlgButtonCheck(hwnd, IDC_CHECK_SET_NO_USE_PC), iniPath.c_str());
	WritePrivateProfileInt(L"NO_SUSPEND", L"NoUsePCTime", GetDlgItemInt(hwnd, IDC_EDIT_SET_NO_USE_PC, NULL, FALSE), iniPath.c_str());
	WritePrivateProfileInt(L"NO_SUSPEND", L"NoFileStreaming", GetDlgButtonCheck(hwnd, IDC_CHECK_SET_NO_FILE_STREAMING), iniPath.c_str());
	WritePrivateProfileInt(L"NO_SUSPEND", L"NoShareFile", GetDlgButtonCheck(hwnd, IDC_CHECK_SET_NO_SHARE_FILE), iniPath.c_str());
	WritePrivateProfileInt(L"SET", L"StartMargin", (int)GetDlgItemInt(hwnd, IDC_EDIT_SET_START_MARGIN, NULL, TRUE), iniPath.c_str());
	WritePrivateProfileInt(L"SET", L"EndMargin", (int)GetDlgItemInt(hwnd, IDC_EDIT_SET_END_MARGIN, NULL, TRUE), iniPath.c_str());
	WritePrivateProfileInt(L"SET", L"RecAppWakeTime", GetDlgItemInt(hwnd, IDC_EDIT_SET_APP_WAKE_TIME, NULL, FALSE), iniPath.c_str());
	WritePrivateProfileInt(L"SET", L"ProcessPriority", ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_COMBO_SET_PROCESS_PRIORITY)), iniPath.c_str());
	WritePrivateProfileInt(L"SET", L"RecMinWake", GetDlgButtonCheck(hwnd, IDC_CHECK_SET_REC_MIN_WAKE), iniPath.c_str());
	WritePrivateProfileInt(L"SET", L"RecView", GetDlgButtonCheck(hwnd, IDC_CHECK_SET_REC_VIEW), iniPath.c_str());
	WritePrivateProfileInt(L"SET", L"DropLog", GetDlgButtonCheck(hwnd, IDC_CHECK_SET_DROP_LOG), iniPath.c_str());
	WritePrivateProfileInt(L"SET", L"PgInfoLog", GetDlgButtonCheck(hwnd, IDC_CHECK_SET_PG_INFO_LOG), iniPath.c_str());
	WritePrivateProfileInt(L"SET", L"RecNW", GetDlgButtonCheck(hwnd, IDC_CHECK_SET_REC_NW), iniPath.c_str());
	WritePrivateProfileInt(L"SET", L"KeepDisk", GetDlgButtonCheck(hwnd, IDC_CHECK_SET_KEEP_DISK), iniPath.c_str());
	WritePrivateProfileInt(L"SET", L"RecOverWrite", GetDlgButtonCheck(hwnd, IDC_CHECK_SET_REC_OVERWRITE), iniPath.c_str());

	//予約情報管理
	hwnd = this->hwndReserve;
	WritePrivateProfileInt(L"SET", L"BackPriority", GetDlgButtonCheck(hwnd, IDC_CHECK_SET_BACK_PRIORITY), iniPath.c_str());
	WritePrivateProfileInt(L"SET", L"FixedTunerPriority", GetDlgButtonCheck(hwnd, IDC_CHECK_SET_FIXED_TUNER_PRIORITY), iniPath.c_str());
	WritePrivateProfileInt(L"SET", L"RecInfoFolderOnly", GetDlgButtonCheck(hwnd, IDC_CHECK_SET_REC_INFO_FOLDER_ONLY), iniPath.c_str());
	WritePrivateProfileInt(L"SET", L"RecInfoDelFile", GetDlgButtonCheck(hwnd, IDC_CHECK_SET_REC_INFO_DEL_FILE), commonIniPath.c_str());
	WritePrivateProfileInt(L"SET", L"ApplyExtToRecInfoDel", GetDlgButtonCheck(hwnd, IDC_CHECK_SET_APPLY_EXT_TO), iniPath.c_str());
	WritePrivateProfileInt(L"SET", L"AutoDel", GetDlgButtonCheck(hwnd, IDC_CHECK_SET_AUTODEL), iniPath.c_str());
	num = 0;
	for( int i = 0; i < ListBox_GetCount(GetDlgItem(hwnd, IDC_LIST_SET_DEL_EXT)); i++ ){
		if( ListBox_GetTextLen(GetDlgItem(hwnd, IDC_LIST_SET_DEL_EXT), i) < MAX_PATH ){
			ListBox_GetText(GetDlgItem(hwnd, IDC_LIST_SET_DEL_EXT), i, buff);
			swprintf_s(key, L"%d", num++);
			WritePrivateProfileString(L"DEL_EXT", key, buff, iniPath.c_str());
		}
	}
	WritePrivateProfileInt(L"DEL_EXT", L"Count", num, iniPath.c_str());
	for(;;){
		//掃除
		swprintf_s(key, L"%d", num++);
		if( GetPrivateProfileToString(L"DEL_EXT", key, L"", iniPath.c_str()).empty() ){
			break;
		}
		WritePrivateProfileString(L"DEL_EXT", key, NULL, iniPath.c_str());
	}
	num = 0;
	for( int i = 0; i < ListBox_GetCount(GetDlgItem(hwnd, IDC_LIST_SET_DEL_CHK)); i++ ){
		if( ListBox_GetTextLen(GetDlgItem(hwnd, IDC_LIST_SET_DEL_CHK), i) < MAX_PATH ){
			ListBox_GetText(GetDlgItem(hwnd, IDC_LIST_SET_DEL_CHK), i, buff);
			swprintf_s(key, L"%d", num++);
			WritePrivateProfileString(L"DEL_CHK", key, buff, iniPath.c_str());
		}
	}
	WritePrivateProfileInt(L"DEL_CHK", L"Count", num, iniPath.c_str());
	for(;;){
		//掃除
		swprintf_s(key, L"%d", num++);
		if( GetPrivateProfileToString(L"DEL_CHK", key, L"", iniPath.c_str()).empty() ){
			break;
		}
		WritePrivateProfileString(L"DEL_CHK", key, NULL, iniPath.c_str());
	}
	WritePrivateProfileInt(L"SET", L"RecNamePlugIn", GetDlgButtonCheck(hwnd, IDC_CHECK_SET_RECNAME_PLUGIN), iniPath.c_str());
	GetDlgItemText(hwnd, IDC_COMBO_SET_RECNAME_PLUGIN, buff, MAX_PATH);
	WritePrivateProfileString(L"SET", L"RecNamePlugInFile", buff, iniPath.c_str());
	WritePrivateProfileInt(L"SET", L"NoChkYen", GetDlgButtonCheck(hwnd, IDC_CHECK_SET_NO_CHK_YEN), iniPath.c_str());
	WritePrivateProfileInt(L"SET", L"DelReserveMode",
	                       GetDlgButtonCheck(hwnd, IDC_RADIO_SET_DEL_RESERVE_END) ? 1 :
	                       GetDlgButtonCheck(hwnd, IDC_RADIO_SET_DEL_RESERVE_CANCEL) ? 2 : 0, iniPath.c_str());

	//その他
	hwnd = this->hwndOther;
	WritePrivateProfileInt(L"SET", L"EnableTCPSrv", GetDlgButtonCheck(hwnd, IDC_CHECK_SET_TCP_SERVER), iniPath.c_str());
	WritePrivateProfileInt(L"SET", L"TCPPort", GetDlgItemInt(hwnd, IDC_EDIT_SET_TCP_PORT, NULL, FALSE), iniPath.c_str());
	GetDlgItemText(hwnd, IDC_EDIT_SET_TCP_ACL, buff, _countof(buff));
	WritePrivateProfileString(L"SET", L"TCPAccessControlList", buff, iniPath.c_str());
	WritePrivateProfileInt(L"SET", L"TCPResponseTimeoutSec", GetDlgItemInt(hwnd, IDC_EDIT_SET_TCP_RES_TO, NULL, FALSE), iniPath.c_str());
	WritePrivateProfileInt(L"SET", L"AutoDelRecInfo", GetDlgButtonCheck(hwnd, IDC_CHECK_SET_AUTODEL_REC_INFO), iniPath.c_str());
	WritePrivateProfileInt(L"SET", L"AutoDelRecInfoNum", GetDlgItemInt(hwnd, IDC_EDIT_SET_AUTODEL_REC_INFO, NULL, FALSE), iniPath.c_str());
	WritePrivateProfileInt(L"SET", L"EpgArchivePeriodHour", ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_COMBO_SET_EPG_ARCHIVE_PERIOD)) * 24, iniPath.c_str());
	WritePrivateProfileInt(L"SET", L"TimeSync", GetDlgButtonCheck(hwnd, IDC_CHECK_SET_TIME_SYNC), iniPath.c_str());
	WritePrivateProfileInt(L"SET", L"ResidentMode",
	                       GetDlgButtonCheck(hwnd, IDC_CHECK_SET_RESIDENT) ?
	                           (GetDlgButtonCheck(hwnd, IDC_CHECK_SET_SHOW_TRAY) ? 2 : 1) : 0, iniPath.c_str());
	WritePrivateProfileInt(L"SET", L"NoBalloonTip", GetDlgButtonCheck(hwnd, IDC_CHECK_SET_NO_BALLOON_TIP), iniPath.c_str());
	WritePrivateProfileInt(L"SET", L"SaveNotifyLog", GetDlgButtonCheck(hwnd, IDC_CHECK_SET_SAVE_NOTIFY_LOG), iniPath.c_str());
	WritePrivateProfileInt(L"SET", L"SaveDebugLog", GetDlgButtonCheck(hwnd, IDC_CHECK_SET_SAVE_DEBUG_LOG), iniPath.c_str());
	//チェックを操作したときだけ変化させる
	int compatFlags = GetPrivateProfileInt(L"SET", L"CompatFlags", 0, iniPath.c_str());
	compatFlags = GetDlgButtonCheck(hwnd, IDC_CHECK_SET_COMPAT_TKNTREC) ?
	                  (compatFlags % 4096 == 4095 ? compatFlags : 4095) : (compatFlags % 4096 == 4095 ? 0 : compatFlags);
	WritePrivateProfileInt(L"SET", L"CompatFlags", compatFlags, iniPath.c_str());
	num = 0;
	for( int i = 0; i < ListBox_GetCount(GetDlgItem(hwnd, IDC_LIST_SET_VIEW_BON)); i++ ){
		if( ListBox_GetTextLen(GetDlgItem(hwnd, IDC_LIST_SET_VIEW_BON), i) < MAX_PATH ){
			ListBox_GetText(GetDlgItem(hwnd, IDC_LIST_SET_VIEW_BON), i, buff);
			swprintf_s(key, L"%d", num++);
			WritePrivateProfileString(L"TVTEST", key, buff, iniPath.c_str());
		}
	}
	WritePrivateProfileInt(L"TVTEST", L"Num", num, iniPath.c_str());
	for(;;){
		//掃除
		swprintf_s(key, L"%d", num++);
		if( GetPrivateProfileToString(L"TVTEST", key, L"", iniPath.c_str()).empty() ){
			break;
		}
		WritePrivateProfileString(L"TVTEST", key, NULL, iniPath.c_str());
	}

	//設定を再読み込みさせる
	CSendCtrlCmd ctrlCmd;
	ctrlCmd.SendReloadSetting();
	if( compatFlags & 0x08 ){
		//互換動作: 設定更新通知コマンドを実装する
		ctrlCmd.SendProfileUpdate(L"EpgTimerSrvSetting");
	}
}

void CEpgTimerSrvSetting::OnBnClickedSetRecNamePlugIn()
{
	WCHAR name[MAX_PATH] = {};
	GetDlgItemText(this->hwndReserve, IDC_COMBO_SET_RECNAME_PLUGIN, name, _countof(name));
	if( name[0] ){
		wstring moduleFolder;
		GetModuleFolderPath(moduleFolder);
		HMODULE hModule = LoadLibrary((moduleFolder + L"\\RecName\\" + name).c_str());
		if( hModule ){
			void (WINAPI* pfnSetting)(HWND) = (void (WINAPI*)(HWND))GetProcAddress(hModule, "Setting");
			if( pfnSetting ){
				pfnSetting(this->hwndTop);
			}
			FreeLibrary(hModule);
		}
	}
}

void CEpgTimerSrvSetting::OnBnClickedSetEpgServiceVideo()
{
	int i = 0;
	for( auto itr = this->chSet.GetMap().cbegin(); itr != this->chSet.GetMap().end(); itr++ ){
		ListView_SetCheckState(GetDlgItem(this->hwndEpg, IDC_LIST_SET_EPG_SERVICE), i++,
		                       itr->second.serviceType == 0x01 || //デジタルTV
		                       itr->second.serviceType == 0xA5 || //プロモーション映像
		                       itr->second.serviceType == 0xAD);  //超高精細度4K専用TV
	}
}

void CEpgTimerSrvSetting::OnLbnSelchangeListSetEpgService()
{
	int sel = ListView_GetNextItem(GetDlgItem(this->hwndEpg, IDC_LIST_SET_EPG_SERVICE), -1, LVNI_SELECTED);
	if( 0 <= sel && sel < (int)this->chSet.GetMap().size() ){
		auto itr = this->chSet.GetMap().cbegin();
		std::advance(itr, sel);
		WCHAR val[256];
		swprintf_s(val, L"NetworkID : %d(0x%04X)\r\nTransportStreamID : %d(0x%04X)\r\nServiceID : %d(0x%04X) Type=%d%s",
		           itr->second.originalNetworkID, itr->second.originalNetworkID,
		           itr->second.transportStreamID, itr->second.transportStreamID,
		           itr->second.serviceID, itr->second.serviceID,
		           itr->second.serviceType, itr->second.partialFlag ? L",Partial" : L"");
		SetDlgItemText(this->hwndEpg, IDC_STATIC_SET_EPG_SERVICE, val);
	}
}

void CEpgTimerSrvSetting::AddEpgTime(bool check)
{
	int wday = ComboBox_GetCurSel(GetDlgItem(this->hwndEpg, IDC_COMBO_SET_EPG_WDAY));
	int hh = ComboBox_GetCurSel(GetDlgItem(this->hwndEpg, IDC_COMBO_SET_EPG_HH));
	int mm = ComboBox_GetCurSel(GetDlgItem(this->hwndEpg, IDC_COMBO_SET_EPG_MM));
	if( wday >= 0 && hh >= 0 && mm >= 0 ){
		static const WCHAR week[9] = L" 月火水木金土日";
		static const WCHAR flag[3] = L"詳基";
		WCHAR weekMin[32];
		swprintf_s(weekMin, L"%c%02d:%02d", week[wday % 8], hh, mm);
		WCHAR flags[32];
		swprintf_s(flags, L"%c,%c,%c,%c",
		           flag[GetDlgButtonCheck(this->hwndEpg, IDC_CHECK_SET_EPG_BS)],
		           flag[GetDlgButtonCheck(this->hwndEpg, IDC_CHECK_SET_EPG_CS1)],
		           flag[GetDlgButtonCheck(this->hwndEpg, IDC_CHECK_SET_EPG_CS2)],
		           flag[GetDlgButtonCheck(this->hwndEpg, IDC_CHECK_SET_EPG_CS3)]);
		LVITEM lvi;
		lvi.mask = LVIF_TEXT;
		lvi.iItem = ListView_GetItemCount(GetDlgItem(this->hwndEpg, IDC_LIST_SET_EPG_TIME));
		for( int i = 0; i < lvi.iItem; i++ ){
			WCHAR buff[32] = {};
			ListView_GetItemText(GetDlgItem(this->hwndEpg, IDC_LIST_SET_EPG_TIME), i, 0, buff, _countof(buff));
			if( _wcsicmp(buff, weekMin) == 0 ){
				//すでにある
				return;
			}
		}
		lvi.iSubItem = 0;
		lvi.pszText = weekMin;
		ListView_InsertItem(GetDlgItem(this->hwndEpg, IDC_LIST_SET_EPG_TIME), &lvi);
		lvi.iSubItem = 1;
		lvi.pszText = flags;
		ListView_SetItem(GetDlgItem(this->hwndEpg, IDC_LIST_SET_EPG_TIME), &lvi);
		ListView_SetCheckState(GetDlgItem(this->hwndEpg, IDC_LIST_SET_EPG_TIME), lvi.iItem, check);
	}
}

void CEpgTimerSrvSetting::DeleteEpgTime()
{
	int sel = ListView_GetNextItem(GetDlgItem(this->hwndEpg, IDC_LIST_SET_EPG_TIME), -1, LVNI_SELECTED);
	if( sel >= 0 ){
		ListView_DeleteItem(GetDlgItem(this->hwndEpg, IDC_LIST_SET_EPG_TIME), sel);
	}
}

void CEpgTimerSrvSetting::BrowseFolder(HWND hTarget)
{
	WCHAR buff[MAX_PATH] = {};
	BROWSEINFO bi = {};
	bi.hwndOwner = this->hwndTop;
	bi.pszDisplayName = buff;
	bi.lpszTitle = L"フォルダを選択してください";
	bi.ulFlags = BIF_NEWDIALOGSTYLE;
	PIDLIST_ABSOLUTE pidl = SHBrowseForFolder(&bi);
	if( pidl ){
		if( SHGetPathFromIDList(pidl, buff) == FALSE ){
			buff[0] = L'\0';
		}
		CoTaskMemFree(pidl);
		if( buff[0] ){
			SetWindowText(hTarget, buff);
		}
	}
}

void CEpgTimerSrvSetting::BrowseExeFile(HWND hTarget)
{
	WCHAR buff[MAX_PATH] = {};
	OPENFILENAME ofn = {};
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = this->hwndTop;
	ofn.lpstrFilter = L"exe files (*.exe)\0*.exe\0All files (*.*)\0*.*\0";
	ofn.lpstrFile = buff;
	ofn.nMaxFile = _countof(buff);
	ofn.Flags = OFN_FILEMUSTEXIST;
	if( GetOpenFileName(&ofn) ){
		SetWindowText(hTarget, buff);
	}
}

void CEpgTimerSrvSetting::ToggleStartup(bool execute, bool add)
{
	WCHAR startupFolder[MAX_PATH];
	if( SHGetSpecialFolderPath(NULL, startupFolder, CSIDL_STARTUP, FALSE) ){
		wstring path = startupFolder;
		ChkFolderPath(path);
		path += L"\\EpgTimerSrv.lnk";
		if( GetFileAttributes(path.c_str()) == INVALID_FILE_ATTRIBUTES ){
			if( execute && add ){
				wstring moduleFolder;
				GetModuleFolderPath(moduleFolder);
				WCHAR modulePath[MAX_PATH];
				DWORD len = GetModuleFileName(NULL, modulePath, _countof(modulePath));
				if( len != 0 && len < _countof(modulePath) ){
					IShellLink* psl;
					if( CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void**)&psl) == S_OK ){
						psl->SetWorkingDirectory(moduleFolder.c_str());
						psl->SetPath(modulePath);
						IPersistFile* ppf;
						if( psl->QueryInterface(IID_IPersistFile, (void**)&ppf) == S_OK ){
							ppf->Save(path.c_str(), TRUE);
							ppf->Release();
						}
						psl->Release();
					}
				}
				ToggleStartup(false, false);
			}else{
				ShowWindow(GetDlgItem(this->hwndOther, IDC_BUTTON_SET_STARTUP_ADD), SW_SHOW);
				ShowWindow(GetDlgItem(this->hwndOther, IDC_BUTTON_SET_STARTUP_DEL), SW_HIDE);
			}
		}else{
			if( execute && add == false ){
				DeleteFile(path.c_str());
				ToggleStartup(false, false);
			}else{
				ShowWindow(GetDlgItem(this->hwndOther, IDC_BUTTON_SET_STARTUP_DEL), SW_SHOW);
				ShowWindow(GetDlgItem(this->hwndOther, IDC_BUTTON_SET_STARTUP_ADD), SW_HIDE);
			}
		}
	}else{
		ShowWindow(GetDlgItem(this->hwndOther, IDC_BUTTON_SET_STARTUP_ADD), SW_SHOW);
		ShowWindow(GetDlgItem(this->hwndOther, IDC_BUTTON_SET_STARTUP_DEL), SW_HIDE);
	}
}

INT_PTR CALLBACK CEpgTimerSrvSetting::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CEpgTimerSrvSetting* sys = (CEpgTimerSrvSetting*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
	if( sys == NULL && uMsg != WM_INITDIALOG ){
		return FALSE;
	}
	switch( uMsg ){
	case WM_INITDIALOG:
		SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
		sys = (CEpgTimerSrvSetting*)lParam;
		sys->hwndTop = hDlg;
		return sys->OnInitDialog();
	case WM_NOTIFY:
		if( ((NMHDR*)lParam)->idFrom == IDC_TAB && ((NMHDR*)lParam)->code == TCN_SELCHANGE ){
			sys->OnTcnSelchangeTab();
			SetWindowLongPtr(hDlg, DWLP_MSGRESULT, 0);
			return TRUE;
		}
		break;
	case WM_COMMAND:
		switch( LOWORD(wParam) ){
		case IDC_BUTTON_OK:
			sys->OnBnClickedOk();
			EndDialog(hDlg, IDOK);
			SetWindowLongPtr(hDlg, DWLP_MSGRESULT, 0);
			return TRUE;
		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL);
			//FALL THROUGH!
		case IDOK:
			//無視
			SetWindowLongPtr(hDlg, DWLP_MSGRESULT, 0);
			return TRUE;
		}
		break;
	}
	return FALSE;
}

INT_PTR CALLBACK CEpgTimerSrvSetting::ChildDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CEpgTimerSrvSetting* sys = (CEpgTimerSrvSetting*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
	if( sys == NULL && uMsg != WM_INITDIALOG ){
		return FALSE;
	}
	switch( uMsg ){
	case WM_INITDIALOG:
		SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
		return TRUE;
	case WM_CTLCOLORDLG:
	case WM_CTLCOLORSTATIC:
		return (INT_PTR)GetStockBrush(WHITE_BRUSH);
	case WM_NOTIFY:
		if( ((NMHDR*)lParam)->idFrom == IDC_LIST_SET_EPG_SERVICE && ((NMHDR*)lParam)->code == LVN_ITEMCHANGED ){
			sys->OnLbnSelchangeListSetEpgService();
		}
		break;
	case WM_COMMAND:
		switch( LOWORD(wParam) ){
		case IDC_BUTTON_SET_DATA_SAVE_PATH:
			sys->BrowseFolder(GetDlgItem(hDlg, IDC_EDIT_SET_DATA_SAVE_PATH));
			break;
		case IDC_BUTTON_SET_REC_FOLDER:
			sys->BrowseFolder(GetDlgItem(hDlg, IDC_EDIT_SET_REC_FOLDER));
			break;
		case IDC_BUTTON_SET_REC_INFO_FOLDER:
			sys->BrowseFolder(GetDlgItem(hDlg, IDC_EDIT_SET_REC_INFO_FOLDER));
			break;
		case IDC_BUTTON_SET_DEL_CHK:
			sys->BrowseFolder(GetDlgItem(hDlg, IDC_EDIT_SET_DEL_CHK));
			break;
		case IDC_BUTTON_SET_REC_EXE_PATH:
			sys->BrowseExeFile(GetDlgItem(hDlg, IDC_EDIT_SET_REC_EXE_PATH));
			break;
		case IDC_BUTTON_SET_REC_FOLDER_ADD:
			AddListBoxItem(GetDlgItem(hDlg, IDC_LIST_SET_REC_FOLDER), GetDlgItem(hDlg, IDC_EDIT_SET_REC_FOLDER));
			break;
		case IDC_BUTTON_SET_NO_EXE_ADD:
			AddListBoxItem(GetDlgItem(hDlg, IDC_LIST_SET_NO_EXE), GetDlgItem(hDlg, IDC_EDIT_SET_NO_EXE));
			break;
		case IDC_BUTTON_SET_DEL_EXT_ADD:
			AddListBoxItem(GetDlgItem(hDlg, IDC_LIST_SET_DEL_EXT), GetDlgItem(hDlg, IDC_EDIT_SET_DEL_EXT));
			break;
		case IDC_BUTTON_SET_DEL_CHK_ADD:
			AddListBoxItem(GetDlgItem(hDlg, IDC_LIST_SET_DEL_CHK), GetDlgItem(hDlg, IDC_EDIT_SET_DEL_CHK));
			break;
		case IDC_BUTTON_SET_VIEW_BON_ADD:
			AddListBoxItem(GetDlgItem(hDlg, IDC_LIST_SET_VIEW_BON), GetDlgItem(hDlg, IDC_COMBO_SET_VIEW_BON));
			break;
		case IDC_BUTTON_SET_REC_FOLDER_DEL:
			DeleteListBoxItem(GetDlgItem(hDlg, IDC_LIST_SET_REC_FOLDER));
			break;
		case IDC_BUTTON_SET_NO_EXE_DEL:
			DeleteListBoxItem(GetDlgItem(hDlg, IDC_LIST_SET_NO_EXE));
			break;
		case IDC_BUTTON_SET_DEL_EXT_DEL:
			DeleteListBoxItem(GetDlgItem(hDlg, IDC_LIST_SET_DEL_EXT));
			break;
		case IDC_BUTTON_SET_DEL_CHK_DEL:
			DeleteListBoxItem(GetDlgItem(hDlg, IDC_LIST_SET_DEL_CHK));
			break;
		case IDC_BUTTON_SET_VIEW_BON_DEL:
			DeleteListBoxItem(GetDlgItem(hDlg, IDC_LIST_SET_VIEW_BON));
			break;
		case IDC_BUTTON_SET_REC_FOLDER_UP:
			MoveListBoxItem(GetDlgItem(hDlg, IDC_LIST_SET_REC_FOLDER), -1);
			break;
		case IDC_BUTTON_SET_BON_UP:
			MoveListBoxItem(GetDlgItem(hDlg, IDC_LIST_SET_BON), -1);
			break;
		case IDC_BUTTON_SET_VIEW_BON_UP:
			MoveListBoxItem(GetDlgItem(hDlg, IDC_LIST_SET_VIEW_BON), -1);
			break;
		case IDC_BUTTON_SET_REC_FOLDER_DN:
			MoveListBoxItem(GetDlgItem(hDlg, IDC_LIST_SET_REC_FOLDER), 1);
			break;
		case IDC_BUTTON_SET_BON_DN:
			MoveListBoxItem(GetDlgItem(hDlg, IDC_LIST_SET_BON), 1);
			break;
		case IDC_BUTTON_SET_VIEW_BON_DN:
			MoveListBoxItem(GetDlgItem(hDlg, IDC_LIST_SET_VIEW_BON), 1);
			break;
		case IDC_CHECK_SET_NO_USE_PC:
			EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SET_NO_USE_PC), GetDlgButtonCheck(hDlg, LOWORD(wParam)));
			break;
		case IDC_CHECK_SET_REC_INFO_DEL_FILE:
			EnableWindow(GetDlgItem(hDlg, IDC_CHECK_SET_APPLY_EXT_TO), GetDlgButtonCheck(hDlg, LOWORD(wParam)));
			//連動処理のため
			SendMessage(hDlg, WM_COMMAND, IDC_CHECK_SET_APPLY_EXT_TO, 0);
			break;
		case IDC_CHECK_SET_AUTODEL:
			EnableWindow(GetDlgItem(hDlg, IDC_LIST_SET_DEL_CHK), GetDlgButtonCheck(hDlg, LOWORD(wParam)));
			EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SET_DEL_CHK), GetDlgButtonCheck(hDlg, LOWORD(wParam)));
			EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_SET_DEL_CHK), GetDlgButtonCheck(hDlg, LOWORD(wParam)));
			EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_SET_DEL_CHK_ADD), GetDlgButtonCheck(hDlg, LOWORD(wParam)));
			EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_SET_DEL_CHK_DEL), GetDlgButtonCheck(hDlg, LOWORD(wParam)));
			//FALL THROUGH!
		case IDC_CHECK_SET_APPLY_EXT_TO:
			{
				bool b = GetDlgButtonCheck(hDlg, IDC_CHECK_SET_AUTODEL) ||
					GetDlgButtonCheck(hDlg, IDC_CHECK_SET_REC_INFO_DEL_FILE) && GetDlgButtonCheck(hDlg, IDC_CHECK_SET_APPLY_EXT_TO);
				EnableWindow(GetDlgItem(hDlg, IDC_LIST_SET_DEL_EXT), b);
				EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SET_DEL_EXT), b);
				EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_SET_DEL_EXT_ADD), b);
				EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_SET_DEL_EXT_DEL), b);
			}
			break;
		case IDC_CHECK_SET_RECNAME_PLUGIN:
			EnableWindow(GetDlgItem(hDlg, IDC_COMBO_SET_RECNAME_PLUGIN), GetDlgButtonCheck(hDlg, LOWORD(wParam)));
			EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_SET_RECNAME_PLUGIN), GetDlgButtonCheck(hDlg, LOWORD(wParam)));
			break;
		case IDC_CHECK_SET_TCP_SERVER:
			EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SET_TCP_PORT), GetDlgButtonCheck(hDlg, LOWORD(wParam)));
			EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SET_TCP_RES_TO), GetDlgButtonCheck(hDlg, LOWORD(wParam)));
			EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SET_TCP_ACL), GetDlgButtonCheck(hDlg, LOWORD(wParam)));
			break;
		case IDC_CHECK_SET_AUTODEL_REC_INFO:
			EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SET_AUTODEL_REC_INFO), GetDlgButtonCheck(hDlg, LOWORD(wParam)));
			break;
		case IDC_CHECK_SET_RESIDENT:
			EnableWindow(GetDlgItem(hDlg, IDC_CHECK_SET_SHOW_TRAY), GetDlgButtonCheck(hDlg, LOWORD(wParam)));
			EnableWindow(GetDlgItem(hDlg, IDC_CHECK_SET_NO_BALLOON_TIP), GetDlgButtonCheck(hDlg, LOWORD(wParam)));
			break;
		case IDC_BUTTON_SET_RECNAME_PLUGIN:
			sys->OnBnClickedSetRecNamePlugIn();
			break;
		case IDC_BUTTON_SET_EPG_SERVICE_VIDEO:
			sys->OnBnClickedSetEpgServiceVideo();
			break;
		case IDC_BUTTON_SET_STARTUP_ADD:
			sys->ToggleStartup(true, true);
			break;
		case IDC_BUTTON_SET_STARTUP_DEL:
			sys->ToggleStartup(true, false);
			break;
		case IDC_BUTTON_SET_EPG_TIME_ADD:
			sys->AddEpgTime(true);
			break;
		case IDC_BUTTON_SET_EPG_TIME_DEL:
			sys->DeleteEpgTime();
			break;
		case IDC_LIST_SET_REC_FOLDER:
			if( HIWORD(wParam) == LBN_SELCHANGE ){
				OnSelChangeListBoxItem(GetDlgItem(hDlg, LOWORD(wParam)), GetDlgItem(hDlg, IDC_EDIT_SET_REC_FOLDER));
			}
			break;
		case IDC_LIST_SET_NO_EXE:
			if( HIWORD(wParam) == LBN_SELCHANGE ){
				OnSelChangeListBoxItem(GetDlgItem(hDlg, LOWORD(wParam)), GetDlgItem(hDlg, IDC_EDIT_SET_NO_EXE));
			}
			break;
		case IDC_LIST_SET_DEL_EXT:
			if( HIWORD(wParam) == LBN_SELCHANGE ){
				OnSelChangeListBoxItem(GetDlgItem(hDlg, LOWORD(wParam)), GetDlgItem(hDlg, IDC_EDIT_SET_DEL_EXT));
			}
			break;
		case IDC_LIST_SET_DEL_CHK:
			if( HIWORD(wParam) == LBN_SELCHANGE ){
				OnSelChangeListBoxItem(GetDlgItem(hDlg, LOWORD(wParam)), GetDlgItem(hDlg, IDC_EDIT_SET_DEL_CHK));
			}
			break;
		case IDC_LIST_SET_BON:
			if( HIWORD(wParam) == LBN_SELCHANGE ){
				int sel = ListBox_GetCurSel(GetDlgItem(hDlg, IDC_LIST_SET_BON));
				if( sel >= 0 ){
					WORD data = (WORD)ListBox_GetItemData(GetDlgItem(hDlg, IDC_LIST_SET_BON), sel);
					ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_COMBO_SET_BON_COUNT), LOBYTE(data));
					ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_COMBO_SET_BON_EPG_COUNT), HIBYTE(data));
				}
			}
			break;
		case IDC_COMBO_SET_BON_COUNT:
		case IDC_COMBO_SET_BON_EPG_COUNT:
			if( HIWORD(wParam) == CBN_SELCHANGE ){
				int sel = ListBox_GetCurSel(GetDlgItem(hDlg, IDC_LIST_SET_BON));
				int count = ComboBox_GetCurSel(GetDlgItem(hDlg, LOWORD(wParam)));
				if( sel >= 0 && count >= 0 ){
					WORD data = (WORD)ListBox_GetItemData(GetDlgItem(hDlg, IDC_LIST_SET_BON), sel);
					ListBox_SetItemData(GetDlgItem(hDlg, IDC_LIST_SET_BON), sel,
					                    LOWORD(wParam) == IDC_COMBO_SET_BON_COUNT ? MAKEWORD(count, HIBYTE(data)) : MAKEWORD(LOBYTE(data), count));
				}
			}
			break;
		case IDC_BUTTON_SET_EPG_SERVICE_ALL:
		case IDC_BUTTON_SET_EPG_SERVICE_CLEAR:
			for( int i = 0; i < ListView_GetItemCount(GetDlgItem(hDlg, IDC_LIST_SET_EPG_SERVICE)); i++ ){
				ListView_SetCheckState(GetDlgItem(hDlg, IDC_LIST_SET_EPG_SERVICE), i, LOWORD(wParam) == IDC_BUTTON_SET_EPG_SERVICE_ALL);
			}
			break;
		}
		break;
	}
	return FALSE;
}
