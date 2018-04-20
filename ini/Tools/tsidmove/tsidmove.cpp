// tsidmove: EDCBの予約、EPG自動予約、プログラム自動予約に含まれるTransportStreamIDの情報を変更する (2018-04-20)
// ※予約ファイル等にこのフォークと非互換の項目追加等されたフォークでは使いまわし不可能
#include "stdafx.h"
#include "../../../Common/CommonDef.h"
#include "../../../Common/ParseTextInstances.h"
#include "../../../Common/PathUtil.h"

static const CH_DATA5 *CheckTSID(WORD onid, WORD tsid, WORD sid, const CParseChText5 &chText5)
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

int wmain(int argc, wchar_t **argv)
{
	_wsetlocale(LC_ALL, L"");

	// --dray-run時は書き込みを一切しない
	const bool dryrun = (argc >= 2 && wcscmp(argv[1], L"--dry-run") == 0);
	if (argc != 2 || (!dryrun && wcscmp(argv[1], L"--run") != 0)) {
		_putws(L"Usage: tsidmove --dry-run|--run");
		return 2;
	}

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

	for (auto itr = chText5.GetMap().cbegin(); itr != chText5.GetMap().end(); itr++) {
		for (auto jtr = itr; ++jtr != chText5.GetMap().end(); ) {
			if (itr->second.originalNetworkID == jtr->second.originalNetworkID &&
			    itr->second.serviceID == jtr->second.serviceID) {
				_putws(L"Warning: ChSet5.txtにTSID以外のIDが等しいサービスがあります。古い情報が残っていませんか？");
				itr = chText5.GetMap().end();
				itr--;
				break;
			}
		}
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
				const CH_DATA5 *ch = CheckTSID(r.originalNetworkID, r.transportStreamID, r.serviceID, chText5);
				if (ch) {
					wprintf_s(L"\n  ID=%d, TSID=%d(0x%04X) -> %d(0x%04X) (%s)",
					          r.reserveID, r.transportStreamID, r.transportStreamID,
					          ch->transportStreamID, ch->transportStreamID, ch->serviceName.c_str());
					r.transportStreamID = ch->transportStreamID;
					text.ChgReserve(r);
					n++;
				}
			}
			if (n == 0) {
				_putws(L"に変更はありません。");
			} else {
				wprintf_s(L"\n以上の%d項目変更します...\n", n);
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
					const CH_DATA5 *ch = CheckTSID(static_cast<WORD>(*jtr >> 32), static_cast<WORD>(*jtr >> 16), static_cast<WORD>(*jtr), chText5);
					if (ch) {
						wprintf_s(L"\n  ID=%d, TSID=%d(0x%04X) -> %d(0x%04X) (%s)",
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
				_putws(L"に変更はありません。");
			} else {
				wprintf_s(L"\n以上の%d項目変更します...\n", n);
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
				const CH_DATA5 *ch = CheckTSID(m.originalNetworkID, m.transportStreamID, m.serviceID, chText5);
				if (ch) {
					wprintf_s(L"\n  ID=%d, TSID=%d(0x%04X) -> %d(0x%04X) (%s)",
					          m.dataID, m.transportStreamID, m.transportStreamID,
					          ch->transportStreamID, ch->transportStreamID, ch->serviceName.c_str());
					m.transportStreamID = ch->transportStreamID;
					text.ChgData(m);
					n++;
				}
			}
			if (n == 0) {
				_putws(L"に変更はありません。");
			} else {
				wprintf_s(L"\n以上の%d項目変更します...\n", n);
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
