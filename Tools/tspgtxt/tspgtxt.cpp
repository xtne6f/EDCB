// tspgtxt: TSファイルから番組情報(.program.txt)を取り出す (2025-03-16)
// ※コマンドと同じ場所かその1階層上にEpgDataCap3.dllが必要
// ※無引数で呼び出すと"Usage:"を標準エラー出力する
//
// 引数:
// -s seek_ratio, 0<=range<=1, default=0.5
//   ファイル終端を1とするシーク位置の比率。
// -l limit_bytes, range>=48128, default=33554432 (32*1024*1024)
//   番組情報が見つからない場合に打ち切るバイト数。
// -n prog_num_or_index, range<=-1 or 1<=range<=65535, default=-1
//   サービスID(1以上)かPAT(Program Association Table)上の並び順(先頭を-1として-1,-2,..)。
// -o dest|-
//   出力ファイル名、または"-"で標準出力。未指定だと入力ファイル名に".program.txt"を付加したものが出力ファイル名になる。
// -f
//   次番組の番組情報を取り出す。
// -w
//   出力ファイルがすでにあるとき上書きする(未指定のときは上書きしない)。
// -x
//   出力ファイル名が未指定のとき、入力ファイル名から拡張子を除いたものに".program.txt"を付加したものを出力ファイル名にする。
// src|-
//   入力TSファイル名、または"-"で標準入力。
//
#include "stdafx.h"
#include "../../BonCtrl/BonCtrlDef.h"
#include "../../BonCtrl/PacketInit.h"
#include "../../BonCtrl/PATUtil.h"
#include "../../Common/EpgDataCap3Util.h"
#include "../../Common/EpgTimerUtil.h"
#include "../../Common/ErrDef.h"
#include "../../Common/PathUtil.h"
#include "../../Common/StringUtil.h"
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#ifdef _WIN32
int wmain(int argc, wchar_t **argv)
#else
int main(int argc, char **argv)
#endif
{
	double seekRatio = 0.5;
	int limitReadBytes = 32 * 1024 * 1024;
	int progNumOrIndex = -1;
	bool setDestPath = false;
	bool following = false;
	bool overwrite = false;
	bool dropExtension = false;
	fs_path destPath;
	fs_path srcPath;
	for (int i = 1; i < argc; i++) {
#ifdef _WIN32
		wstring s = argv[i];
#else
		wstring s;
		UTF8toW(argv[i], s);
#endif
		if (seekRatio < 0) {
			seekRatio = wcstod(s.c_str(), nullptr);
			if (seekRatio < 0 || seekRatio > 1) {
				fputws(L"Error: -s specified value is out of range.\n", stderr);
				return 1;
			}
		} else if (limitReadBytes == 0) {
			limitReadBytes = (int)wcstol(s.c_str(), nullptr, 10);
			if (limitReadBytes < 48128) {
				fputws(L"Error: -l specified value is out of range.\n", stderr);
				return 1;
			}
		} else if (progNumOrIndex == 0) {
			progNumOrIndex = (int)wcstol(s.c_str(), nullptr, 10);
			if (progNumOrIndex == 0 || progNumOrIndex > 65535) {
				fputws(L"Error: -n specified value is out of range.\n", stderr);
				return 1;
			}
		} else if (setDestPath) {
			setDestPath = false;
			destPath = s;
		} else if (i < argc - 1) {
			if (s == L"-s") {
				seekRatio = -1;
			} else if (s == L"-l") {
				limitReadBytes = 0;
			}else if (s == L"-n") {
				progNumOrIndex = 0;
			} else if (s == L"-o") {
				setDestPath = true;
			} else if (s == L"-f") {
				following = true;
			} else if (s == L"-w") {
				overwrite = true;
			} else if (s == L"-x") {
				dropExtension = true;
			}
		} else {
			srcPath = s;
		}
	}
	if (srcPath.empty()) {
		fputws(L"Usage: tspgtxt [-s seek_ratio][-l limit_bytes][-n prog_num_or_index][-o dest|-][-f][-w][-x] src|-\n", stderr);
		return 2;
	}

	if (destPath.empty()) {
		if (srcPath.native() == L"-") {
			fputws(L"Error: cannot determine dest path.\n", stderr);
			return 1;
		}
		destPath = srcPath;
		if (dropExtension) {
			// 拡張子を置換
			destPath.replace_extension(L".program.txt");
		} else {
			// 拡張子を追加
			destPath.concat(L".program.txt");
		}
	}

	if (!overwrite && destPath.native() != L"-" && UtilFileExists(destPath).first) {
		// 上書き禁止なので終了
		fputws(L"Info: dest file already exists.\n", stderr);
		return 0;
	}

	std::unique_ptr<FILE, fclose_deleter> srcFile;
	if (srcPath.native() != L"-") {
		srcFile.reset(UtilOpenFile(srcPath, UTIL_SHARED_READ));
		if (!srcFile) {
			fputws(L"Error: cannot open file.\n", stderr);
			return 1;
		}
		if (seekRatio > 0) {
			// シーク
			LONGLONG fileSize;
			if (my_fseek(srcFile.get(), 0, SEEK_END) != 0 || (fileSize = my_ftell(srcFile.get())) < 0 ||
			    my_fseek(srcFile.get(), (LONGLONG)(fileSize * seekRatio) / 188 * 188, SEEK_SET) != 0) {
				fputws(L"Error: cannot seek file.\n", stderr);
				return 1;
			}
		}
	}
#ifdef _WIN32
	else if (_setmode(_fileno(stdin), _O_BINARY) < 0) {
		fputws(L"Error: _setmode.\n", stderr);
		return 1;
	}
#endif

	CEpgDataCap3Util epgUtil;
	bool initialized = epgUtil.Initialize(FALSE) == NO_ERR;
#ifdef _WIN32
	if (!initialized) {
		// 1階層上にDLLがあれば使う
		fs_path path = GetModulePath().parent_path().parent_path();
		if (!path.empty()) {
			path.append(L"EpgDataCap3" EDCB_LIB_EXT);
			initialized = epgUtil.Initialize(FALSE, path.c_str()) == NO_ERR;
		}
	}
#endif
	if (!initialized) {
		fputws(L"Error: EpgDataCap3" EDCB_LIB_EXT " not found.\n", stderr);
		return 1;
	}

	WORD nitPid = 0;
	BYTE nitCounter = 0;
	SERVICE_INFO *service = nullptr;
	CPacketInit packetInit;
	CPATUtil patUtil;
	BYTE inData[48128];
	DWORD inSize;
	DWORD readBytes = 0;
	while ((inSize = (DWORD)fread(inData, 1, sizeof(inData), srcFile ? srcFile.get() : stdin)) != 0) {
		BYTE *outData;
		DWORD outSize;
		if (!packetInit.GetTSData(inData, inSize, &outData, &outSize)) {
			fputws(L"Error: CPacketInit::GetTSData().\n", stderr);
			return 1;
		}
		for (DWORD i = 0; i < outSize; i += 188) {
			WORD pid = CTSPacketUtil::GetPidFrom188TS(outData + i);
			if (pid < BON_SELECTIVE_PID) {
				if (pid == 0) {
					// PAT
					CTSPacketUtil packet;
					if (packet.Set188TS(outData + i, 188)) {
						patUtil.AddPacket(packet);
					}
				}
				if (nitPid != 0 && pid != nitPid && epgUtil.AddTSPacket(outData + i, 188) != NO_ERR) {
					fputws(L"Error: CEpgDataCap3Util::AddTSPacket().\n", stderr);
					return 1;
				}
			}
		}

		// NITのPIDを取得(常に0x0010だが一応)
		for (pair<WORD, WORD> item : patUtil.GetPIDProgramNumberList()) {
			if (item.second == 0) {
				nitPid = item.first;
				break;
			}
		}
		if (nitPid != 0) {
			// NITから取るべき情報はないが送出頻度が低くGetServiceListActual()の成功が遅れるのでダミーを送る
			static const BYTE nitTable[] = {
				0x40, // table_id
				0xF0, 13, // section_length
				0x00, 0x00, // network_id
				0xC1, 0x00, 0x00, // version_number
				0xF0, 0, // network_descriptors_length
				0xF0, 0, // transport_stream_loop_length
				0x60, 0x24, 0x58, 0xC8, // CRC_32
			};
			BYTE packet[188];
			packet[0] = 0x47;
			packet[1] = nitPid >> 8 | 0x40;
			packet[2] = nitPid & 0xFF;
			packet[3] = 0x10 | ((++nitCounter) & 0x0F);
			packet[4] = 0; // pointer_field
			std::copy(nitTable, nitTable + sizeof(nitTable), packet + 5);
			std::fill(packet + 5 + sizeof(nitTable), packet + 188, (BYTE)0xFF);
			if (epgUtil.AddTSPacket(packet, 188) != NO_ERR) {
				fputws(L"Error: CEpgDataCap3Util::AddTSPacket().\n", stderr);
				return 1;
			}
		}

		// PAT上の並び順をサービスIDに変換
		if (progNumOrIndex < 0) {
			int index = -progNumOrIndex;
			for (pair<WORD, WORD> item : patUtil.GetPIDProgramNumberList()) {
				if (item.second != 0) {
					if (--index == 0) {
						progNumOrIndex = item.second;
						break;
					}
				}
			}
		}

		// 指定されたサービスIDについての情報を取得
		DWORD serviceListSize;
		SERVICE_INFO *serviceList;
		if (!service && progNumOrIndex > 0 && epgUtil.GetServiceListActual(&serviceListSize, &serviceList) == NO_ERR) {
			for (DWORD i = 0; i < serviceListSize; i++) {
				if (serviceList[i].service_id == progNumOrIndex) {
					service = serviceList + i;
					break;
				}
			}
			if (!service) {
				fputws(L"Error: service info not found.\n", stderr);
				return 1;
			}
		}

		// 番組情報を取得
		EPG_EVENT_INFO *epgInfo;
		if (service && epgUtil.GetEpgInfo(service->original_network_id, service->transport_stream_id, service->service_id, following, &epgInfo) == NO_ERR) {
			EPGDB_EVENT_INFO epgdbInfo;
			ConvertEpgInfo(service->original_network_id, service->transport_stream_id, service->service_id, epgInfo, &epgdbInfo);
			wstring outTextW = L"\xFEFF" +
				ConvertProgramText(epgdbInfo, [service](WORD onid, WORD tsid, WORD sid) -> LPCWSTR {
					return onid == service->original_network_id &&
					       tsid == service->transport_stream_id &&
					       sid == service->service_id &&
					       service->extInfo ? service->extInfo->service_name : nullptr;
				});
			if (UTIL_NEWLINE[0] != L'\r') {
				Replace(outTextW, L"\r\n", L"\n");
			}
			string outText;
			WtoUTF8(outTextW, outText);
			std::unique_ptr<FILE, fclose_deleter> destFile;
			if (destPath.native() != L"-") {
				destFile.reset(UtilOpenFile(destPath, overwrite ? UTIL_SECURE_WRITE : UTIL_O_EXCL_CREAT_WRONLY));
				if (!destFile) {
					fputws(L"Error: cannot create file.\n", stderr);
					return 1;
				}
			}
#ifdef _WIN32
			else if (_setmode(_fileno(stdout), _O_BINARY) < 0) {
				fputws(L"Error: _setmode.\n", stderr);
				return 1;
			}
#endif
			if (fputs(outText.c_str(), destFile ? destFile.get() : stdout) < 0 || fflush(destFile ? destFile.get() : stdout) != 0) {
				fputws(L"Error: writing failed.\n", stderr);
				return 1;
			}
			return 0;
		}

		readBytes += inSize;
		if (readBytes + sizeof(inData) > (DWORD)limitReadBytes) {
			break;
		}
	}

	fputws(L"Error: program info not found.\n", stderr);
	return 1;
}
