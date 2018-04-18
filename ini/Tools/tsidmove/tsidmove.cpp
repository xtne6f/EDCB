// tsidmove: EDCBの予約、EPG自動予約、プログラム自動予約に含まれるTransportStreamIDの情報を変更する (2018-04-18)
// ※予約ファイル等にこのフォークと非互換の項目追加等されたフォークでは使いまわし不可能
#include "stdafx.h"
#include "../../../Common/CommonDef.h"
#include "../../../Common/ParseTextInstances.h"
#include "../../../Common/PathUtil.h"

int wmain(int argc, wchar_t **argv)
{
	_wsetlocale(LC_ALL, L"");

	// --dray-run時は書き込みを一切しない
	const bool dryrun = (argc == 5 && wcscmp(argv[1], L"--dry-run") == 0);
	if (argc != 4 && !dryrun) {
		_putws(L"Usage: tsidmove [--dry-run] {NetworkID} {before_TSID} {after_TSID}");
		return 2;
	}
	argv += (dryrun ? 2 : 1);
	const WORD beforeONID = static_cast<WORD>(wcstol(*(argv++), nullptr, 0));
	const WORD beforeTSID = static_cast<WORD>(wcstol(*(argv++), nullptr, 0));
	const WORD afterTSID = static_cast<WORD>(wcstol(*(argv++), nullptr, 0));

	// このツールはEDCBフォルダかその直下のフォルダに置かれているはず
	fs_path iniPath = GetModulePath().replace_filename(L"Common.ini");
	if (GetFileAttributes(iniPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
		iniPath = iniPath.parent_path().replace_filename(L"Common.ini");
		if (GetFileAttributes(iniPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
			_putws(L"Error: Common.iniが見つかりません。");
			return 1;
		}
	}

	// 「設定関係保存フォルダ」
	fs_path settingPath = GetPrivateProfileToFolderPath(L"SET", L"DataSavePath", iniPath.c_str());
	if (settingPath.empty()) {
		settingPath = fs_path(iniPath).replace_filename(L"Setting");
	}
	wprintf_s(L"\"%s\"フォルダをチェックしています...\n", settingPath.c_str());

	fs_path chSet5Path = fs_path(settingPath).append(L"ChSet5.txt");
	if (GetFileAttributes(chSet5Path.c_str()) == INVALID_FILE_ATTRIBUTES) {
		_putws(L"Error: ChSet5.txtが見つかりません。");
		return 1;
	}
	CParseChText5 chText5;
	if (!chText5.ParseText(chSet5Path.c_str())) {
		_putws(L"Error: ChSet5.txtを開けません。");
		return 1;
	}

	// ChSet5.txtは予め更新されていると仮定。矛盾をチェック
	LPCWSTR targetName = nullptr;
	for (auto itr = chText5.GetMap().cbegin(); itr != chText5.GetMap().end(); itr++) {
		if (itr->second.originalNetworkID == beforeONID && itr->second.transportStreamID == beforeTSID) {
			wprintf_s(L"Warning: 移動元のTSID=%d(0x%04x)「%s」がまだChSet5.txtに存在します。\n", beforeTSID, beforeTSID, itr->second.networkName.c_str());
		}
		if (itr->second.originalNetworkID == beforeONID && itr->second.transportStreamID == afterTSID) {
			targetName = itr->second.networkName.c_str();
		}
	}
	if (!targetName) {
		wprintf_s(L"Error: 移動先のTSID=%d(0x%04x) がChSet5.txtに存在しません。\n", afterTSID, afterTSID);
		return 1;
	}

	HANDLE hMutex = CreateMutex(nullptr, FALSE, EPG_TIMER_BON_SRV_MUTEX);
	if (hMutex) {
		if (GetLastError() == ERROR_ALREADY_EXISTS) {
			CloseHandle(hMutex);
			hMutex = nullptr;
		}
	}
	if (!hMutex) {
		_putws(L"Error: EpgTimerSrv.exeを終了させてください。");
		return 1;
	}
	CloseHandle(hMutex);

	wprintf_s(L"NetworkID=%d(0x%04x) の移動元TSID=%d(0x%04x) を移動先TSID=%d(0x%04x) に変更します。\n",
	          beforeONID, beforeONID, beforeTSID, beforeTSID, afterTSID, afterTSID);
	wprintf_s(L"チャンネル名は「%s」。\n\n", targetName);

	wprintf_s(RESERVE_TEXT_NAME L"(予約ファイル)");
	{
		CParseReserveText text;
		if (!text.ParseText(fs_path(settingPath).append(RESERVE_TEXT_NAME).c_str())) {
			_putws(L"をスキップします。");
		} else {
			int n = 0;
			for (size_t i = 0; i < text.GetMap().size(); i++) {
				auto itr = text.GetMap().cbegin();
				std::advance(itr, i);
				RESERVE_DATA r = itr->second;
				if (r.originalNetworkID == beforeONID && r.transportStreamID == beforeTSID) {
					r.transportStreamID = afterTSID;
					text.ChgReserve(r);
					n++;
				}
			}
			if (n == 0) {
				_putws(L"に変更はありません。");
			} else {
				wprintf_s(L"を%d項目変更します...\n", n);
				if (!dryrun) {
					if (text.SaveText()) {
						_putws(L"成功。");
					} else {
						_putws(L"Error: 失敗。");
					}
				}
			}
		}
	}

	wprintf_s(EPG_AUTO_ADD_TEXT_NAME L"(EPG自動予約ファイル)");
	{
		CParseEpgAutoAddText text;
		if (!text.ParseText(fs_path(settingPath).append(EPG_AUTO_ADD_TEXT_NAME).c_str())) {
			_putws(L"をスキップします。");
		} else {
			int n = 0;
			for (size_t i = 0; i < text.GetMap().size(); i++) {
				auto itr = text.GetMap().cbegin();
				std::advance(itr, i);
				EPG_AUTO_ADD_DATA a = itr->second;
				bool modified = false;
				for (auto jtr = a.searchInfo.serviceList.begin(); jtr != a.searchInfo.serviceList.end(); jtr++) {
					if (((*jtr >> 32) & 0xFFFF) == beforeONID && ((*jtr >> 16) & 0xFFFF) == beforeTSID) {
						*jtr = (*jtr & 0xFFFF0000FFFFLL) | (static_cast<DWORD>(afterTSID) << 16);
						modified = true;
					}
				}
				if (modified) {
					text.ChgData(a);
					n++;
				}
			}
			if (n == 0) {
				_putws(L"に変更はありません。");
			} else {
				wprintf_s(L"を%d項目変更します...\n", n);
				if (!dryrun) {
					if (text.SaveText()) {
						_putws(L"成功。");
					} else {
						_putws(L"Error: 失敗。");
					}
				}
			}
		}
	}

	wprintf_s(MANUAL_AUTO_ADD_TEXT_NAME L"(プログラム自動予約ファイル)");
	{
		CParseManualAutoAddText text;
		if (!text.ParseText(fs_path(settingPath).append(MANUAL_AUTO_ADD_TEXT_NAME).c_str())) {
			_putws(L"をスキップします。");
		} else {
			int n = 0;
			for (size_t i = 0; i < text.GetMap().size(); i++) {
				auto itr = text.GetMap().cbegin();
				std::advance(itr, i);
				MANUAL_AUTO_ADD_DATA m = itr->second;
				if (m.originalNetworkID == beforeONID && m.transportStreamID == beforeTSID) {
					m.transportStreamID = afterTSID;
					text.ChgData(m);
					n++;
				}
			}
			if (n == 0) {
				_putws(L"に変更はありません。");
			} else {
				wprintf_s(L"を%d項目変更します...\n", n);
				if (!dryrun) {
					if (text.SaveText()) {
						_putws(L"成功。");
					} else {
						_putws(L"Error: 失敗。");
					}
				}
			}
		}
	}

	_putws(L"終了。");
	return 0;
}
