// tsidmove: EDCBの予約、EPG自動予約、プログラム自動予約に含まれるTransportStreamIDの情報を変更する (2018-04-20)
// ※予約ファイル等にこのフォークと非互換の項目追加等されたフォークでは使いまわし不可能
#include "stdafx.h"
#include "../../../Common/CommonDef.h"
#include "../../../Common/ParseTextInstances.h"
#include "../../../Common/PathUtil.h"
#include <locale.h>

namespace
{
void PrintfInEnvironmentLocale(LPCWSTR format, ...)
{
	fflush(stdout);
	setlocale(LC_ALL, "");
	va_list params;
	va_start(params, format);
#ifdef _WIN32
	vwprintf_s(format, params);
#else
	vwprintf(format, params);
#endif
	va_end(params);
	fflush(stdout);
	setlocale(LC_ALL, "C");
}

const CH_DATA5 *CheckTSID(WORD onid, WORD tsid, WORD sid, const CParseChText5 &chText5)
{
	if (chText5.GetMap().count(static_cast<LONGLONG>(onid) << 32 | static_cast<DWORD>(tsid) << 16 | sid) == 0) {
		// 見つからなかった
		auto itr = std::find_if(chText5.GetMap().begin(), chText5.GetMap().end(),
		                        [=](const pair<LONGLONG, CH_DATA5> &a) { return a.second.originalNetworkID == onid && a.second.serviceID == sid; });
		if (itr != chText5.GetMap().end()) {
			// TSIDを無視すれば見つかった
			return &itr->second;
		}
	}
	return nullptr;
}
}

#ifdef _WIN32
int wmain(int argc, wchar_t **argv)
#else
int main(int argc, char **argv)
#endif
{
	// --dray-run時は書き込みを一切しない
#ifdef _WIN32
	const bool dryrun = (argc >= 2 && wcscmp(argv[1], L"--dry-run") == 0);
	if (argc != 2 || (!dryrun && wcscmp(argv[1], L"--run") != 0)) {
#else
	const bool dryrun = (argc >= 2 && strcmp(argv[1], "--dry-run") == 0);
	if (argc != 2 || (!dryrun && strcmp(argv[1], "--run") != 0)) {
#endif
		fputws(L"Usage: tsidmove --dry-run|--run\n", stdout);
		return 2;
	}

	fs_path iniPath = GetCommonIniPath();
	if (UtilFileExists(iniPath).first == false) {
#ifdef _WIN32
		// このツールはEDCBフォルダかその直下のフォルダに置かれているはず
		iniPath = iniPath.parent_path().replace_filename(L"Common.ini");
		if (UtilFileExists(iniPath).first == false)
#endif
		{
			fputws(L"Error: Common.ini: No such file.\n", stdout);
			return 1;
		}
	}

	// 「設定関係保存フォルダ」
	fs_path settingPath = GetPrivateProfileToString(L"SET", L"DataSavePath", L"", iniPath.c_str());
	if (settingPath.empty()) {
		settingPath = fs_path(iniPath).replace_filename(L"Setting");
	}
	PrintfInEnvironmentLocale(L"Checking... \"%ls\"\n", settingPath.c_str());

	fs_path chSet5Path = fs_path(settingPath).append(L"ChSet5.txt");
	if (UtilFileExists(chSet5Path).first == false) {
		fputws(L"Error: ChSet5.txt: No such file.\n", stdout);
		return 1;
	}
	CParseChText5 chText5;
	if (!chText5.ParseText(chSet5Path.c_str())) {
		fputws(L"Error: ChSet5.txt: Failed to read file.\n", stdout);
		return 1;
	}

	for (auto itr = chText5.GetMap().cbegin(); itr != chText5.GetMap().end(); itr++) {
		for (auto jtr = itr; ++jtr != chText5.GetMap().end(); ) {
			if (itr->second.originalNetworkID == jtr->second.originalNetworkID &&
			    itr->second.serviceID == jtr->second.serviceID) {
				fputws(L"Warning: ** ChSet5.txt ** contains duplicated services with same IDs except for TransportStreamID!\n", stdout);
				itr = chText5.GetMap().end();
				itr--;
				break;
			}
		}
	}

	if (!UtilCreateGlobalMutex(EPG_TIMER_BON_SRV_MUTEX)) {
		fputws(L"Error: Close the EpgTimerSrv process.\n", stdout);
		return 1;
	}

	fputws(RESERVE_TEXT_NAME "...", stdout);
	{
		CParseReserveText text;
		if (!text.ParseText(fs_path(settingPath).append(RESERVE_TEXT_NAME).c_str())) {
			fputws(L" Skipped.\n", stdout);
		} else {
			int n = 0;
			for (size_t i = 0; i < text.GetMap().size(); i++) {
				auto itr = text.GetMap().cbegin();
				std::advance(itr, i);
				RESERVE_DATA r = itr->second;
				const CH_DATA5 *ch = CheckTSID(r.originalNetworkID, r.transportStreamID, r.serviceID, chText5);
				if (ch) {
					PrintfInEnvironmentLocale(L"\n  ID=%d, TSID=%d(0x%04X) -> %d(0x%04X) (%ls)",
					                          r.reserveID, r.transportStreamID, r.transportStreamID,
					                          ch->transportStreamID, ch->transportStreamID, ch->serviceName.c_str());
					r.transportStreamID = ch->transportStreamID;
					text.ChgReserve(r);
					n++;
				}
			}
			if (n == 0) {
				fputws(L" No need to change. Skipped.\n", stdout);
			} else {
				PrintfInEnvironmentLocale(L"\nChanging the above %d items...\n", n);
				if (!dryrun) {
					if (text.SaveText()) {
						fputws(L"Succeeded.\n", stdout);
					} else {
						fputws(L"Error: Failed to write file.\n", stdout);
					}
				}
				fputws(L"\n", stdout);
			}
		}
	}

	fputws(EPG_AUTO_ADD_TEXT_NAME "...", stdout);
	{
		CParseEpgAutoAddText text;
		if (!text.ParseText(fs_path(settingPath).append(EPG_AUTO_ADD_TEXT_NAME).c_str())) {
			fputws(L" Skipped.\n", stdout);
		} else {
			int n = 0;
			for (size_t i = 0; i < text.GetMap().size(); i++) {
				auto itr = text.GetMap().cbegin();
				std::advance(itr, i);
				EPG_AUTO_ADD_DATA a = itr->second;
				bool modified = false;
				for (auto jtr = a.searchInfo.serviceList.begin(); jtr != a.searchInfo.serviceList.end(); jtr++) {
					const CH_DATA5 *ch = CheckTSID(static_cast<WORD>(*jtr >> 32), static_cast<WORD>(*jtr >> 16), static_cast<WORD>(*jtr), chText5);
					if (ch) {
						PrintfInEnvironmentLocale(L"\n  ID=%d, TSID=%d(0x%04X) -> %d(0x%04X) (%ls)",
						                          a.dataID, static_cast<WORD>(*jtr >> 16), static_cast<WORD>(*jtr >> 16),
						                          ch->transportStreamID, ch->transportStreamID, ch->serviceName.c_str());
						*jtr = (*jtr & 0xFFFF0000FFFFLL) | (static_cast<DWORD>(ch->transportStreamID) << 16);
						modified = true;
					}
				}
				if (modified) {
					text.ChgData(a);
					n++;
				}
			}
			if (n == 0) {
				fputws(L" No need to change. Skipped.\n", stdout);
			} else {
				PrintfInEnvironmentLocale(L"\nChanging the above %d items...\n", n);
				if (!dryrun) {
					if (text.SaveText()) {
						fputws(L"Succeeded.\n", stdout);
					} else {
						fputws(L"Error: Failed to write file.\n", stdout);
					}
				}
				fputws(L"\n", stdout);
			}
		}
	}

	fputws(MANUAL_AUTO_ADD_TEXT_NAME "...", stdout);
	{
		CParseManualAutoAddText text;
		if (!text.ParseText(fs_path(settingPath).append(MANUAL_AUTO_ADD_TEXT_NAME).c_str())) {
			fputws(L" Skipped.\n", stdout);
		} else {
			int n = 0;
			for (size_t i = 0; i < text.GetMap().size(); i++) {
				auto itr = text.GetMap().cbegin();
				std::advance(itr, i);
				MANUAL_AUTO_ADD_DATA m = itr->second;
				const CH_DATA5 *ch = CheckTSID(m.originalNetworkID, m.transportStreamID, m.serviceID, chText5);
				if (ch) {
					PrintfInEnvironmentLocale(L"\n  ID=%d, TSID=%d(0x%04X) -> %d(0x%04X) (%ls)",
					                          m.dataID, m.transportStreamID, m.transportStreamID,
					                          ch->transportStreamID, ch->transportStreamID, ch->serviceName.c_str());
					m.transportStreamID = ch->transportStreamID;
					text.ChgData(m);
					n++;
				}
			}
			if (n == 0) {
				fputws(L" No need to change. Skipped.\n", stdout);
			} else {
				PrintfInEnvironmentLocale(L"\nChanging the above %d items...\n", n);
				if (!dryrun) {
					if (text.SaveText()) {
						fputws(L"Succeeded.\n", stdout);
					} else {
						fputws(L"Error: Failed to write file.\n", stdout);
					}
				}
				fputws(L"\n", stdout);
			}
		}
	}

	fputws(L"Done.\n", stdout);
	return 0;
}
