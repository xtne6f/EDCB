#include "stdafx.h"
#include "EpgDBManager.h"

#include "../../Common/CommonDef.h"
#include "../../Common/TimeUtil.h"
#include "../../Common/StringUtil.h"
#include "../../Common/PathUtil.h"
#include "../../Common/EpgTimerUtil.h"
#include "../../Common/EpgDataCap3Util.h"
#include "../../Common/CtrlCmdUtil.h"
#include "../../Common/TSPacketUtil.h"
#include <list>

CEpgDBManager::CEpgDBManager()
{
	this->epgMapRefLock = std::make_pair(0, &this->epgMapLock);
	this->loadStop = false;
	this->loadForeground = false;
	this->initialLoadDone = false;
	this->archivePeriodSec = 0;
}

CEpgDBManager::~CEpgDBManager()
{
	CancelLoadData();
}

void CEpgDBManager::SetArchivePeriod(int periodSec)
{
	lock_recursive_mutex lock(this->epgMapLock);
	this->archivePeriodSec = periodSec;
}

void CEpgDBManager::ReloadEpgData(bool foreground)
{
	CancelLoadData();

	//フォアグラウンド読み込みを中断した場合は引き継ぐ
	if( this->loadForeground == false ){
		this->loadForeground = foreground;
	}
	this->loadThread = thread_(LoadThread, this);
}

namespace
{

//長期アーカイブ用ファイルを開く
FILE* OpenOldArchive(LPCWSTR dir, LONGLONG t, int flags)
{
	SYSTEMTIME st;
	ConvertSystemTime(t, &st);
	WCHAR name[32];
	swprintf_s(name, L"%04d%02d%02d.dat", st.wYear, st.wMonth, st.wDay);
	return UtilOpenFile(fs_path(dir).append(name), flags);
}

//存在する長期アーカイブの日付情報を昇順でリストする
vector<LONGLONG> ListOldArchive(LPCWSTR dir)
{
	vector<LONGLONG> timeList;
	EnumFindFile(fs_path(dir).append(L"????????.dat"), [&](UTIL_FIND_DATA& findData) -> bool {
		if( findData.isDir == false && findData.fileName.size() == 12 ){
			//日付(必ず日曜日)を解析
			LPWSTR endp;
			DWORD ymd = wcstoul(findData.fileName.c_str(), &endp, 10);
			if( endp && endp - findData.fileName.c_str() == 8 && endp[0] == L'.' ){
				SYSTEMTIME st = {};
				st.wYear = ymd / 10000 % 10000;
				st.wMonth = ymd / 100 % 100;
				st.wDay = ymd % 100;
				LONGLONG t = ConvertI64Time(st);
				if( t != 0 && ConvertSystemTime(t, &st) && st.wDayOfWeek == 0 ){
					timeList.push_back(t);
				}
			}
		}
		return true;
	});
	std::sort(timeList.begin(), timeList.end());
	return timeList;
}

//長期アーカイブ用ファイルのインデックス領域を読む
void ReadOldArchiveIndex(FILE* fp, vector<BYTE>& buff, vector<LONGLONG>& index, DWORD* headerSize)
{
	rewind(fp);
	buff.clear();
	index.clear();
	while( buff.size() < 4096 * 1024 ){
		buff.resize(buff.size() + 1024);
		size_t n = fread(buff.data() + buff.size() - 1024, 1, 1024, fp);
		buff.resize(buff.size() - 1024 + n);
		if( n == 0 || UtilReadVALUE(&index, buff.data(), (DWORD)buff.size(), headerSize) ){
			break;
		}
		index.clear();
	}
}

//長期アーカイブ用ファイルの特定位置のEPGデータを読む
void ReadOldArchiveEventInfo(FILE* fp, const vector<LONGLONG>& index, size_t indexPos, DWORD headerSize, vector<BYTE>& buff, EPGDB_SERVICE_EVENT_INFO& info)
{
	buff.clear();
	info.eventList.clear();
	DWORD buffSize = (DWORD)index[indexPos];
	LONGLONG pos = headerSize;
	for( size_t i = 0; i + 3 < indexPos; i += 4 ){
		pos += (DWORD)index[i];
	}
	if( buffSize > 0 && my_fseek(fp, 0, SEEK_END) == 0 && my_ftell(fp) >= pos + buffSize && my_fseek(fp, pos, SEEK_SET) == 0 ){
		buff.resize(buffSize);
		if( fread(buff.data(), 1, buffSize, fp) == buffSize ){
			WORD ver;
			if( UtilReadVALUE2WithVersion(&ver, &info, buff.data(), buffSize, NULL) == false ){
				info.eventList.clear();
			}
		}
	}
}

}

void CEpgDBManager::LoadThread(CEpgDBManager* sys)
{
	AddDebugLog(L"Start Load EpgData");
	DWORD time = GetU32Tick();

#ifdef _WIN32
	if( sys->loadForeground == false ){
		//バックグラウンドに移行
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
	}
#endif
	CEpgDataCap3Util epgUtil;
	if( epgUtil.Initialize(FALSE) != NO_ERR ){
		AddDebugLog(L"★EpgDataCap3の初期化に失敗しました。");
		sys->loadForeground = false;
		sys->initialLoadDone = true;
		sys->loadStop = true;
		return;
	}

	LONGLONG utcNow = GetNowI64Time() - I64_UTIL_TIMEZONE;

	//EPGファイルの検索
	vector<wstring> epgFileList;
	const fs_path settingPath = GetSettingPath();
	const fs_path epgDataPath = fs_path(settingPath).append(EPG_SAVE_FOLDER);

	//指定フォルダのファイル一覧取得
	EnumFindFile(fs_path(epgDataPath).append(L"*_epg.dat"), [&](UTIL_FIND_DATA& findData) -> bool {
		if( findData.isDir == false && findData.lastWriteTime != 0 ){
			//見つかったファイルを一覧に追加
			//名前順。ただしTSID==0xFFFFの場合は同じチャンネルの連続によりストリームがクリアされない可能性があるので後ろにまとめる
			WCHAR prefix = findData.lastWriteTime + 7 * 24 * 60 * 60 * I64_1SEC < utcNow ? L'0' :
			               UtilPathEndsWith(findData.fileName.c_str(), L"ffff_epg.dat") ? L'2' :  L'1';
			wstring item = prefix + fs_path(epgDataPath).append(findData.fileName).native();
			epgFileList.insert(std::lower_bound(epgFileList.begin(), epgFileList.end(), item), item);
		}
		return true;
	});

	DWORD loadElapsed = 0;
	DWORD loadTick = GetU32Tick();

	//EPGファイルの解析
	for( vector<wstring>::iterator itr = epgFileList.begin(); itr != epgFileList.end(); itr++ ){
		if( sys->loadStop ){
			//キャンセルされた
			return;
		}
		fs_path path = itr->c_str() + 1;
		if( (*itr)[0] == L'0' ){
			//1週間以上前のファイルなので削除
			DeleteFile(path.c_str());
			AddDebugLogFormat(L"★delete %ls", path.c_str());
		}else{
			BYTE readBuff[188*256];
			std::unique_ptr<FILE, fclose_deleter> file;
			//非_WIN32環境では必ずopen(tmp)->close(tmp)->rename(tmp,master)なので複雑なことは不要
#ifdef _WIN32
			//一時ファイルの状態を調べる。取得側のCreateFile(tmp)→CloseHandle(tmp)→CopyFile(tmp,master)→DeleteFile(tmp)の流れをある程度仮定
			bool swapped = false;
			bool mightExist = false;
			if( UtilFileExists(fs_path(path).concat(L".tmp"), &mightExist).first || mightExist ){
				//一時ファイルがある→もうすぐ上書きされるかもしれないので共有で開いて退避させる
				AddDebugLogFormat(L"★lockless read %ls", path.c_str());
				for( int retry = 0; retry < 25; retry++ ){
					std::unique_ptr<FILE, fclose_deleter> masterFile(UtilOpenFile(path, UTIL_SHARED_READ));
					if( !masterFile ){
						SleepForMsec(200);
						continue;
					}
					for( retry = 0; retry < 3; retry++ ){
						file.reset(UtilOpenFile(fs_path(path).concat(L".swp"), UTIL_O_CREAT_RDWR));
						if( file ){
							swapped = true;
							rewind(masterFile.get());
							for( size_t n; (n = fread(readBuff, 1, sizeof(readBuff), masterFile.get())) != 0; ){
								fwrite(readBuff, 1, n, file.get());
							}
							for( int i = 0; i < 25; i++ ){
								//一時ファイルを読み込み共有で開ける→上書き中かもしれないので少し待つ
								if( !std::unique_ptr<FILE, fclose_deleter>(UtilOpenFile(
								        fs_path(path).concat(L".tmp"), UTIL_O_RDONLY | UTIL_SH_READ | UTIL_SH_DELETE)) ){
									break;
								}
								SleepForMsec(200);
							}
							//退避中に上書きされていないことを確認する
							bool matched = false;
							rewind(masterFile.get());
							rewind(file.get());
							for(;;){
								size_t n = fread(readBuff + sizeof(readBuff) / 2, 1, sizeof(readBuff) / 2, file.get());
								if( n == 0 ){
									matched = fread(readBuff, 1, sizeof(readBuff) / 2, masterFile.get()) == 0;
									break;
								}else if( fread(readBuff, 1, n, masterFile.get()) != n || memcmp(readBuff, readBuff + sizeof(readBuff) / 2, n) ){
									break;
								}
							}
							if( matched ){
								rewind(file.get());
								break;
							}
							file.reset();
						}
						SleepForMsec(200);
					}
					break;
				}
			}else
#endif
			{
				//排他で開く
				file.reset(UtilOpenFile(path, UTIL_SECURE_READ));
			}
			if( !file ){
				AddDebugLogFormat(L"Error %ls", path.c_str());
			}else{
				//PATを送る(ストリームを確実にリセットするため)
				DWORD seekPos = 0;
				for( DWORD i = 0; fread(readBuff, 1, 188, file.get()) == 188; i += 188 ){
					if( CTSPacketUtil::GetPidFrom188TS(readBuff) == 0 ){
						if( CTSPacketUtil::GetPayloadUnitStartIndicatorFrom188TS(readBuff) ){
							if( seekPos != 0 ){
								break;
							}
						}else if( seekPos == 0 ){
							continue;
						}
						seekPos = i + 188;
						epgUtil.AddTSPacket(readBuff, 188);
					}
				}
				my_fseek(file.get(), seekPos, SEEK_SET);
				//TOTを先頭に持ってきて送る(ストリームの時刻を確定させるため)
				bool ignoreTOT = false;
				while( fread(readBuff, 1, 188, file.get()) == 188 ){
					if( CTSPacketUtil::GetPidFrom188TS(readBuff) == 0x14 ){
						ignoreTOT = true;
						epgUtil.AddTSPacket(readBuff, 188);
						break;
					}
				}
				my_fseek(file.get(), seekPos, SEEK_SET);
				for( size_t n; (n = fread(readBuff, 1, sizeof(readBuff), file.get())) != 0; ){
					size_t i = 0;
					if( ignoreTOT ){
						for( ; i + 188 <= n; i += 188 ){
							if( CTSPacketUtil::GetPidFrom188TS(readBuff + i) == 0x14 ){
								ignoreTOT = false;
								i += 188;
								break;
							}
							epgUtil.AddTSPacket(readBuff + i, 188);
						}
					}
					if( n - i >= 188 ){
						epgUtil.AddTSPacket(readBuff + i, (DWORD)((n - i) / 188 * 188));
					}
					if( sys->loadForeground == false ){
						//処理速度がだいたい2/3になるように休む。I/O負荷軽減が狙い
						DWORD tick = GetU32Tick();
						loadElapsed += tick - loadTick;
						loadTick = tick;
						if( loadElapsed > 20 ){
							SleepForMsec(min<DWORD>(loadElapsed / 2, 100));
							loadElapsed = 0;
							loadTick = GetU32Tick();
						}
					}
				}
				file.reset();
			}
#ifdef _WIN32
			if( swapped ){
				DeleteFile(fs_path(path).concat(L".swp").c_str());
			}
#endif
		}
	}

	//EPGデータを取得
	DWORD serviceListSize = 0;
	SERVICE_INFO* serviceList = NULL;
	if( epgUtil.GetServiceListEpgDB(&serviceListSize, &serviceList) == FALSE ){
		sys->loadForeground = false;
		sys->initialLoadDone = true;
		sys->loadStop = true;
		return;
	}
	map<LONGLONG, EPGDB_SERVICE_EVENT_INFO> nextMap;
	for( const SERVICE_INFO* info = serviceList; info != serviceList + serviceListSize; info++ ){
		LONGLONG key = Create64Key(info->original_network_id, info->transport_stream_id, info->service_id);
		EPGDB_SERVICE_EVENT_INFO itemZero = {};
		EPGDB_SERVICE_EVENT_INFO& item = nextMap.insert(std::make_pair(key, itemZero)).first->second;
		item.serviceInfo.ONID = info->original_network_id;
		item.serviceInfo.TSID = info->transport_stream_id;
		item.serviceInfo.SID = info->service_id;
		if( info->extInfo != NULL ){
			item.serviceInfo.service_type = info->extInfo->service_type;
			item.serviceInfo.partialReceptionFlag = info->extInfo->partialReceptionFlag;
			if( info->extInfo->service_provider_name != NULL ){
				item.serviceInfo.service_provider_name = info->extInfo->service_provider_name;
			}
			if( info->extInfo->service_name != NULL ){
				item.serviceInfo.service_name = info->extInfo->service_name;
			}
			if( info->extInfo->network_name != NULL ){
				item.serviceInfo.network_name = info->extInfo->network_name;
			}
			if( info->extInfo->ts_name != NULL ){
				item.serviceInfo.ts_name = info->extInfo->ts_name;
			}
			item.serviceInfo.remote_control_key_id = info->extInfo->remote_control_key_id;
		}
		epgUtil.EnumEpgInfoList(item.serviceInfo.ONID, item.serviceInfo.TSID, item.serviceInfo.SID, EnumEpgInfoListProc, &item);
	}
	epgUtil.UnInitialize();

	LONGLONG arcMax = GetNowI64Time() / I64_1SEC * I64_1SEC;
	LONGLONG arcMin = LLONG_MAX;
	LONGLONG oldMax = LLONG_MIN;
	LONGLONG oldMin = LLONG_MIN;
	{
		lock_recursive_mutex lock(sys->epgMapLock);
		if( sys->archivePeriodSec > 0 ){
			//アーカイブする
			arcMin = arcMax - sys->archivePeriodSec * I64_1SEC;
			if( sys->archivePeriodSec > 14 * 24 * 3600 ){
				//長期アーカイブする
				SYSTEMTIME st;
				ConvertSystemTime(arcMax - 14 * 24 * 3600 * I64_1SEC, &st);
				//対象は2週以上前の日曜0時から1週間
				oldMin = arcMax - ((((14 + st.wDayOfWeek) * 24 + st.wHour) * 60 + st.wMinute) * 60 + st.wSecond) * I64_1SEC;
				oldMax = oldMin + 7 * 24 * 3600 * I64_1SEC;
			}
		}
	}
	arcMax += 3600 * I64_1SEC;

	//初回はアーカイブファイルから読み込む
	map<LONGLONG, EPGDB_SERVICE_EVENT_INFO> arcFromFile;
	if( arcMin < LLONG_MAX && sys->epgArchive.empty() ){
		vector<BYTE> buff;
		std::unique_ptr<FILE, fclose_deleter> fp(UtilOpenFile(fs_path(settingPath).append(EPG_ARCHIVE_DATA_NAME), UTIL_SECURE_READ));
		if( fp && my_fseek(fp.get(), 0, SEEK_END) == 0 ){
			LONGLONG fileSize = my_ftell(fp.get());
			if( 0 < fileSize && fileSize < INT_MAX ){
				buff.resize((size_t)fileSize);
				rewind(fp.get());
				if( fread(buff.data(), 1, buff.size(), fp.get()) != buff.size() ){
					buff.clear();
				}
			}
		}
		if( buff.empty() == false ){
			WORD ver;
			vector<EPGDB_SERVICE_EVENT_INFO> list;
			if( UtilReadVALUE2WithVersion(&ver, &list, buff.data(), (DWORD)buff.size(), NULL) ){
				for( size_t i = 0; i < list.size(); i++ ){
					LONGLONG key = Create64Key(list[i].serviceInfo.ONID, list[i].serviceInfo.TSID, list[i].serviceInfo.SID);
					arcFromFile[key] = std::move(list[i]);
				}
			}
		}
	}

	//長期アーカイブのインデックス領域をキャッシュ
	vector<vector<LONGLONG>> oldCache;
	if( oldMin > LLONG_MIN ){
		fs_path epgArcPath = fs_path(settingPath).append(EPG_ARCHIVE_FOLDER);
		//キャッシュの先頭は日付情報
		oldCache.push_back(ListOldArchive(epgArcPath.c_str()));
		oldCache.resize(1 + oldCache.front().size());
		if( sys->epgOldIndexCache.empty() == false ){
			for( size_t i = 0; i < sys->epgOldIndexCache.front().size(); i++ ){
				size_t j = std::lower_bound(oldCache.front().begin(), oldCache.front().end(), sys->epgOldIndexCache.front()[i]) - oldCache.front().begin();
				if( j != oldCache.front().size() && oldCache.front()[j] == sys->epgOldIndexCache.front()[i] ){
					//キャッシュ済みのものを継承
					oldCache[1 + j] = sys->epgOldIndexCache[1 + i];
				}
			}
		}
		vector<BYTE> buff;
		for( size_t i = 0; i < oldCache.front().size(); i++ ){
			if( oldCache[1 + i].empty() ){
				//キャッシュする
				std::unique_ptr<FILE, fclose_deleter> fp(OpenOldArchive(epgArcPath.c_str(), oldCache.front()[i], UTIL_SECURE_READ));
				if( fp ){
					ReadOldArchiveIndex(fp.get(), buff, oldCache[1 + i], NULL);
				}
			}
		}
	}

#ifdef _WIN32
	if( sys->loadForeground == false ){
		//フォアグラウンドに復帰
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
	}
#endif
	for(;;){
		//データベースを排他する
		{
			lock_recursive_mutex lock(sys->epgMapLock);
			if( sys->epgMapRefLock.first == 0 ){
				if( arcFromFile.empty() == false ){
					sys->epgArchive.swap(arcFromFile);
				}
				//イベントをアーカイブに移動する
				for( auto itr = sys->epgMap.begin(); arcMin < LLONG_MAX && itr != sys->epgMap.end(); itr++ ){
					auto itrArc = sys->epgArchive.find(itr->first);
					if( itrArc != sys->epgArchive.end() ){
						itrArc->second.serviceInfo = std::move(itr->second.serviceInfo);
					}
					for( size_t i = 0; i < itr->second.eventList.size(); i++ ){
						if( itr->second.eventList[i].StartTimeFlag &&
						    itr->second.eventList[i].DurationFlag &&
						    ConvertI64Time(itr->second.eventList[i].start_time) < arcMax &&
						    ConvertI64Time(itr->second.eventList[i].start_time) > arcMin ){
							if( itrArc == sys->epgArchive.end() ){
								//サービスを追加
								itrArc = sys->epgArchive.insert(std::make_pair(itr->first, EPGDB_SERVICE_EVENT_INFO())).first;
								itrArc->second.serviceInfo = std::move(itr->second.serviceInfo);
							}
							itrArc->second.eventList.push_back(std::move(itr->second.eventList[i]));
						}
					}
				}

				//EPGデータを更新する
				sys->epgMap.swap(nextMap);

				//アーカイブから不要なイベントを消す
				for( auto itr = sys->epgMap.cbegin(); arcMin < LLONG_MAX && itr != sys->epgMap.end(); itr++ ){
					auto itrArc = sys->epgArchive.find(itr->first);
					if( itrArc != sys->epgArchive.end() ){
						//主データベースの最古より新しいものは不要
						LONGLONG minStart = LLONG_MAX;
						for( size_t i = 0; i < itr->second.eventList.size(); i++ ){
							if( itr->second.eventList[i].StartTimeFlag && ConvertI64Time(itr->second.eventList[i].start_time) < minStart ){
								minStart = ConvertI64Time(itr->second.eventList[i].start_time);
							}
						}
						itrArc->second.eventList.erase(std::remove_if(itrArc->second.eventList.begin(), itrArc->second.eventList.end(), [=](const EPGDB_EVENT_INFO& a) {
							return ConvertI64Time(a.start_time) + a.durationSec * I64_1SEC > minStart;
						}), itrArc->second.eventList.end());
					}
				}
				//アーカイブから古いイベントを消す
				vector<EPGDB_SERVICE_EVENT_INFO> epgOld;
				vector<LONGLONG> oldIndex;
				for( auto itr = sys->epgArchive.begin(); itr != sys->epgArchive.end(); ){
					auto itrOld = epgOld.end();
					size_t j = 0;
					for( size_t i = 0; i < itr->second.eventList.size(); i++ ){
						LONGLONG startTime = ConvertI64Time(itr->second.eventList[i].start_time);
						if( startTime < oldMax && startTime >= oldMin ){
							//旧世代に移動する
							if( itrOld == epgOld.end() ){
								//サービスを追加
								epgOld.push_back(EPGDB_SERVICE_EVENT_INFO());
								itrOld = epgOld.end() - 1;
								itrOld->serviceInfo = itr->second.serviceInfo;
								oldIndex.push_back(0);
								oldIndex.push_back(itr->first);
								oldIndex.push_back(LLONG_MAX);
								oldIndex.push_back(0);
							}
							//開始時間の最小値と最大値
							*(oldIndex.end() - 2) = min(*(oldIndex.end() - 2), startTime - oldMin);
							*(oldIndex.end() - 1) = max(*(oldIndex.end() - 1), startTime - oldMin);
							itrOld->eventList.push_back(std::move(itr->second.eventList[i]));
						}else if( startTime > max(arcMin, oldMin) && startTime < arcMax ){
							//残す
							if( i != j ){
								itr->second.eventList[j] = std::move(itr->second.eventList[i]);
							}
							j++;
						}
					}
					if( j == 0 ){
						//空のサービスを消す
						sys->epgArchive.erase(itr++);
					}else{
						itr->second.eventList.erase(itr->second.eventList.begin() + j, itr->second.eventList.end());
						itr++;
					}
				}

				//長期アーカイブ用ファイルに書き込む
				if( epgOld.empty() == false ){
					fs_path epgArcPath = fs_path(settingPath).append(EPG_ARCHIVE_FOLDER);
					if( UtilFileExists(epgArcPath).first == false ){
						UtilCreateDirectory(epgArcPath);
					}
					std::unique_ptr<FILE, fclose_deleter> fp(OpenOldArchive(epgArcPath.c_str(), oldMin, UTIL_O_EXCL_CREAT_WRONLY));
					if( fp ){
						vector<CCmdStream> buffList;
						buffList.reserve(epgOld.size());
						while( epgOld.empty() == false ){
							//サービス単位で書き込み、シークできるようにインデックスを作る
							buffList.push_back(CCmdStream());
							buffList.back().WriteVALUE2WithVersion(5, epgOld.back());
							epgOld.pop_back();
							oldIndex[epgOld.size() * 4] = buffList.back().GetDataSize();
						}
						CCmdStream buff;
						buff.WriteVALUE(oldIndex);
						fwrite(buff.GetData(), 1, buff.GetDataSize(), fp.get());
						while( buffList.empty() == false ){
							fwrite(buffList.back().GetData(), 1, buffList.back().GetDataSize(), fp.get());
							buffList.pop_back();
						}
						if( oldCache.empty() == false ){
							//キャッシュに追加
							size_t i = std::lower_bound(oldCache.front().begin(), oldCache.front().end(), oldMin) - oldCache.front().begin();
							if( i == oldCache.front().size() || oldCache.front()[i] != oldMin ){
								oldCache.front().insert(oldCache.front().begin() + i, oldMin);
								oldCache.insert(oldCache.begin() + 1 + i, vector<LONGLONG>());
							}
							oldCache[1 + i].swap(oldIndex);
						}
					}
				}
				sys->epgOldIndexCache.swap(oldCache);
				break;
			}
		}
		SleepForMsec(1);
	}
#ifdef _WIN32
	if( sys->loadForeground == false ){
		//バックグラウンドに移行
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
	}
#endif
	nextMap.clear();

	//アーカイブファイルに書き込む
	if( arcMin < LLONG_MAX ){
		vector<const EPGDB_SERVICE_EVENT_INFO*> valp;
		valp.reserve(sys->epgArchive.size());
		for( auto itr = sys->epgArchive.cbegin(); itr != sys->epgArchive.end(); valp.push_back(&(itr++)->second) );
		CCmdStream buff;
		buff.WriteVALUE2WithVersion(5, valp);
		std::unique_ptr<FILE, fclose_deleter> fp(UtilOpenFile(fs_path(settingPath).append(EPG_ARCHIVE_DATA_NAME), UTIL_SECURE_WRITE));
		if( fp ){
			fwrite(buff.GetData(), 1, buff.GetDataSize(), fp.get());
		}
	}

	AddDebugLogFormat(L"End Load EpgData %dmsec", GetU32Tick() - time);

	sys->loadForeground = false;
	sys->initialLoadDone = true;
	sys->loadStop = true;
}

BOOL CALLBACK CEpgDBManager::EnumEpgInfoListProc(DWORD epgInfoListSize, EPG_EVENT_INFO* epgInfoList, void* param)
{
	EPGDB_SERVICE_EVENT_INFO* item = (EPGDB_SERVICE_EVENT_INFO*)param;

	try{
		if( epgInfoList == NULL ){
			item->eventList.reserve(epgInfoListSize);
		}else{
			for( DWORD i=0; i<epgInfoListSize; i++ ){
				item->eventList.resize(item->eventList.size() + 1);
				ConvertEpgInfo(item->serviceInfo.ONID, item->serviceInfo.TSID, item->serviceInfo.SID, &epgInfoList[i], &item->eventList.back());
				if( item->eventList.back().hasShortInfo ){
					//ごく稀にAPR(改行)を含むため
					Replace(item->eventList.back().shortInfo.event_name, L"\r\n", L"");
				}
				//実装上は既ソートだが仕様ではないので挿入ソートしておく
				for( size_t j = item->eventList.size() - 1; j > 0 && item->eventList[j].event_id < item->eventList[j-1].event_id; j-- ){
					std::swap(item->eventList[j], item->eventList[j-1]);
				}
			}
		}
	}catch( std::bad_alloc& ){
		return FALSE;
	}
	return TRUE;
}

bool CEpgDBManager::IsLoadingData()
{
	return this->loadThread.joinable() && this->loadStop == false;
}

void CEpgDBManager::CancelLoadData()
{
	if( this->loadThread.joinable() ){
		this->loadStop = true;
		this->loadThread.join();
		this->loadStop = false;
	}
}

void CEpgDBManager::SearchEpg(const EPGDB_SEARCH_KEY_INFO* keys, size_t keysSize, LONGLONG enumStart, LONGLONG enumEnd, wstring* findKey,
                              const std::function<void(const EPGDB_EVENT_INFO*, wstring*)>& enumProc) const
{
#if !defined(EPGDB_STD_WREGEX) && defined(_WIN32)
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	try
#endif
	{
		std::unique_ptr<SEARCH_CONTEXT[]> ctxs(new SEARCH_CONTEXT[keysSize]);
		size_t ctxsSize = 0;
		vector<LONGLONG> enumServiceKey;
		for( size_t i = 0; i < keysSize; i++ ){
			if( InitializeSearchContext(ctxs[ctxsSize], enumServiceKey, keys + i) ){
				ctxsSize++;
			}
		}
		if( ctxsSize == 0 || EnumEventInfo(enumServiceKey.data(), enumServiceKey.size(), enumStart, enumEnd,
		                                   [=, &enumProc, &ctxs](const EPGDB_EVENT_INFO* info, const EPGDB_SERVICE_INFO*) {
			if( info ){
				if( IsMatchEvent(ctxs.get(), ctxsSize, info, findKey) ){
					enumProc(info, findKey);
				}
			}else{
				//列挙完了
				enumProc(NULL, findKey);
			}
		}) == false ){
			//列挙なしで完了
			enumProc(NULL, findKey);
		}
	}
#if !defined(EPGDB_STD_WREGEX) && defined(_WIN32)
	catch(...){
		CoUninitialize();
		throw;
	}
	CoUninitialize();
#endif
}

void CEpgDBManager::SearchArchiveEpg(const EPGDB_SEARCH_KEY_INFO* keys, size_t keysSize, LONGLONG enumStart, LONGLONG enumEnd, bool deletableBeforeEnumDone,
                                     wstring* findKey, const std::function<void(const EPGDB_EVENT_INFO*, wstring*)>& enumProc) const
{
#if !defined(EPGDB_STD_WREGEX) && defined(_WIN32)
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	try
#endif
	{
		std::unique_ptr<SEARCH_CONTEXT[]> ctxs(new SEARCH_CONTEXT[keysSize]);
		size_t ctxsSize = 0;
		vector<LONGLONG> enumServiceKey;
		for( size_t i = 0; i < keysSize; i++ ){
			if( InitializeSearchContext(ctxs[ctxsSize], enumServiceKey, keys + i) ){
				ctxsSize++;
			}
		}
		if( ctxsSize == 0 ){
			//列挙なしで完了
			enumProc(NULL, findKey);
		}else{
			EnumArchiveEventInfo(enumServiceKey.data(), enumServiceKey.size(), enumStart, enumEnd, deletableBeforeEnumDone,
			                     [=, &enumProc, &ctxs](const EPGDB_EVENT_INFO* info, const EPGDB_SERVICE_INFO*) {
				if( info ){
					if( IsMatchEvent(ctxs.get(), ctxsSize, info, findKey) ){
						enumProc(info, findKey);
					}
				}else{
					//列挙完了
					enumProc(NULL, findKey);
				}
			});
		}
	}
#if !defined(EPGDB_STD_WREGEX) && defined(_WIN32)
	catch(...){
		CoUninitialize();
		throw;
	}
	CoUninitialize();
#endif
}

bool CEpgDBManager::InitializeSearchContext(SEARCH_CONTEXT& ctx, vector<LONGLONG>& enumServiceKey, const EPGDB_SEARCH_KEY_INFO* key)
{
	if( key->andKey.compare(0, 7, L"^!{999}") == 0 ){
		//無効を示すキーワードが指定されているので検索しない
		return false;
	}
	wstring andKey = key->andKey;
	ctx.caseFlag = false;
	if( andKey.compare(0, 7, L"C!{999}") == 0 ){
		//大小文字を区別するキーワードが指定されている
		andKey.erase(0, 7);
		ctx.caseFlag = true;
	}
	ctx.chkDurationMinSec = 0;
	ctx.chkDurationMaxSec = MAXDWORD;
	if( andKey.compare(0, 4, L"D!{1") == 0 ){
		LPWSTR endp;
		DWORD dur = wcstoul(andKey.c_str() + 3, &endp, 10);
		if( endp - andKey.c_str() == 12 && endp[0] == L'}' ){
			//番組長を絞り込むキーワードが指定されている
			andKey.erase(0, 13);
			ctx.chkDurationMinSec = dur / 10000 % 10000 * 60;
			ctx.chkDurationMaxSec = dur % 10000 == 0 ? MAXDWORD : dur % 10000 * 60;
		}
	}
	wstring notKey = key->notKey;
	if( notKey.compare(0, 6, L":note:") == 0 ){
		//メモを除去
		size_t n = notKey.find_first_of(L" 　");
		notKey.erase(0, n + (n == wstring::npos ? 0 : 1));
	}

	//キーワード分解
	ctx.andKeyList.clear();
	ctx.notKeyList.clear();

	if( key->regExpFlag ){
		//正規表現の単独キーワード
		if( andKey.empty() == false ){
			ctx.andKeyList.push_back(vector<pair<wstring, RegExpPtr>>());
			AddKeyword(ctx.andKeyList.back(), andKey, ctx.caseFlag, true, key->titleOnlyFlag != FALSE);
		}
		if( notKey.empty() == false ){
			AddKeyword(ctx.notKeyList, notKey, ctx.caseFlag, true, key->titleOnlyFlag != FALSE);
		}
	}else{
		//正規表現ではないのでキーワードの分解
		Replace(andKey, L"　", L" ");
		while( andKey.empty() == false ){
			wstring buff;
			Separate(andKey, L" ", buff, andKey);
			if( buff == L"|" ){
				//OR条件
				ctx.andKeyList.push_back(vector<pair<wstring, RegExpPtr>>());
			}else if( buff.empty() == false ){
				if( ctx.andKeyList.empty() ){
					ctx.andKeyList.push_back(vector<pair<wstring, RegExpPtr>>());
				}
				AddKeyword(ctx.andKeyList.back(), std::move(buff), ctx.caseFlag, false, key->titleOnlyFlag != FALSE);
			}
		}
		Replace(notKey, L"　", L" ");
		while( notKey.empty() == false ){
			wstring buff;
			Separate(notKey, L" ", buff, notKey);
			if( buff.empty() == false ){
				AddKeyword(ctx.notKeyList, std::move(buff), ctx.caseFlag, false, key->titleOnlyFlag != FALSE);
			}
		}
	}

	ctx.key = key;
	for( auto itr = key->serviceList.begin(); itr != key->serviceList.end(); itr++ ){
		bool found = false;
		for( size_t i = 0; i + 1 < enumServiceKey.size(); i += 2 ){
			if( enumServiceKey[i + 1] == *itr ){
				found = true;
				break;
			}
		}
		if( found == false ){
			enumServiceKey.push_back(0);
			enumServiceKey.push_back(*itr);
		}
	}
	return true;
}

bool CEpgDBManager::IsMatchEvent(SEARCH_CONTEXT* ctxs, size_t ctxsSize, const EPGDB_EVENT_INFO* itrEvent, wstring* findKey)
{
	for( size_t i = 0; i < ctxsSize; i++ ){
		SEARCH_CONTEXT& ctx = ctxs[i];
		const EPGDB_SEARCH_KEY_INFO& key = *ctx.key;
		//検索キーが複数ある場合はサービスも確認
		if( ctxsSize < 2 || std::find(key.serviceList.begin(), key.serviceList.end(),
		        Create64Key(itrEvent->original_network_id, itrEvent->transport_stream_id, itrEvent->service_id)) != key.serviceList.end() ){
			{
				if( key.freeCAFlag == 1 ){
					//無料放送のみ
					if( itrEvent->freeCAFlag != 0 ){
						//有料放送
						continue;
					}
				}else if( key.freeCAFlag == 2 ){
					//有料放送のみ
					if( itrEvent->freeCAFlag == 0 ){
						//無料放送
						continue;
					}
				}
				//ジャンル確認
				if( key.contentList.size() > 0 ){
					//ジャンル指定あるのでジャンルで絞り込み
					if( itrEvent->hasContentInfo == false ){
						if( itrEvent->hasShortInfo == false ){
							//2つめのサービス？対象外とする
							continue;
						}
						//ジャンル情報ない
						bool findNo = false;
						for( size_t j = 0; j < key.contentList.size(); j++ ){
							if( key.contentList[j].content_nibble_level_1 == 0xFF &&
							    key.contentList[j].content_nibble_level_2 == 0xFF ){
								//ジャンルなしの指定あり
								findNo = true;
								break;
							}
						}
						if( key.notContetFlag == 0 ){
							if( findNo == false ){
								continue;
							}
						}else{
							//NOT条件扱い
							if( findNo ){
								continue;
							}
						}
					}else{
						bool equal = IsEqualContent(key.contentList, itrEvent->contentInfo.nibbleList);
						if( key.notContetFlag == 0 ){
							if( equal == false ){
								//ジャンル違うので対象外
								continue;
							}
						}else{
							//NOT条件扱い
							if( equal ){
								continue;
							}
						}
					}
				}

				//映像確認
				if( key.videoList.size() > 0 ){
					if( itrEvent->hasComponentInfo == false ){
						continue;
					}
					WORD type = itrEvent->componentInfo.stream_content << 8 | itrEvent->componentInfo.component_type;
					if( std::find(key.videoList.begin(), key.videoList.end(), type) == key.videoList.end() ){
						continue;
					}
				}

				//音声確認
				if( key.audioList.size() > 0 ){
					if( itrEvent->hasAudioInfo == false ){
						continue;
					}
					bool findContent = false;
					for( size_t j=0; j<itrEvent->audioInfo.componentList.size(); j++ ){
						WORD type = itrEvent->audioInfo.componentList[j].stream_content << 8 | itrEvent->audioInfo.componentList[j].component_type;
						if( std::find(key.audioList.begin(), key.audioList.end(), type) != key.audioList.end() ){
							findContent = true;
							break;
						}
					}
					if( findContent == false ){
						continue;
					}
				}

				//時間確認
				if( key.dateList.size() > 0 ){
					if( itrEvent->StartTimeFlag == FALSE ){
						//開始時間不明なので対象外
						continue;
					}
					bool inTime = IsInDateTime(key.dateList, itrEvent->start_time);
					if( key.notDateFlag == 0 ){
						if( inTime == false ){
							//時間範囲外なので対象外
							continue;
						}
					}else{
						//NOT条件扱い
						if( inTime ){
							continue;
						}
					}
				}

				//番組長で絞り込み
				if( itrEvent->DurationFlag == FALSE ){
					//不明なので絞り込みされていれば対象外
					if( 0 < ctx.chkDurationMinSec || ctx.chkDurationMaxSec < MAXDWORD ){
						continue;
					}
				}else{
					if( itrEvent->durationSec < ctx.chkDurationMinSec || ctx.chkDurationMaxSec < itrEvent->durationSec ){
						continue;
					}
				}

				if( findKey ){
					findKey->clear();
				}

				//キーワード確認
				if( itrEvent->hasShortInfo == false ){
					if( ctx.andKeyList.empty() == false ){
						//内容にかかわらず対象外
						continue;
					}
				}
				if( FindKeyword(ctx.notKeyList, *itrEvent, ctx.targetWord, ctx.distForFind, ctx.caseFlag, false, false) ){
					//notキーワード見つかったので対象外
					continue;
				}
				if( ctx.andKeyList.empty() == false ){
					bool found = false;
					for( size_t j = 0; j < ctx.andKeyList.size(); j++ ){
						if( FindKeyword(ctx.andKeyList[j], *itrEvent, ctx.targetWord, ctx.distForFind, ctx.caseFlag, key.aimaiFlag != 0, true, findKey) ){
							found = true;
							break;
						}
					}
					if( found == false ){
						//andキーワード見つからなかったので対象外
						continue;
					}
				}
				return true;
			}
		}
	}
	return false;
}

bool CEpgDBManager::IsEqualContent(const vector<EPGDB_CONTENT_DATA>& searchKey, const vector<EPGDB_CONTENT_DATA>& eventData)
{
	for( size_t i=0; i<searchKey.size(); i++ ){
		EPGDB_CONTENT_DATA c = searchKey[i];
		if( 0x60 <= c.content_nibble_level_1 && c.content_nibble_level_1 <= 0x7F ){
			//番組付属情報またはCS拡張用情報に変換する
			c.user_nibble_1 = c.content_nibble_level_1 & 0x0F;
			c.user_nibble_2 = c.content_nibble_level_2;
			c.content_nibble_level_2 = (c.content_nibble_level_1 - 0x60) >> 4;
			c.content_nibble_level_1 = 0x0E;
		}
		for( size_t j=0; j<eventData.size(); j++ ){
			if( c.content_nibble_level_1 == eventData[j].content_nibble_level_1 ){
				if( c.content_nibble_level_2 == 0xFF ){
					//中分類すべて
					return true;
				}
				if( c.content_nibble_level_2 == eventData[j].content_nibble_level_2 ){
					if( c.content_nibble_level_1 != 0x0E ){
						//拡張でない
						return true;
					}
					if( c.user_nibble_1 == eventData[j].user_nibble_1 ){
						if( c.user_nibble_2 == 0xFF ){
							//拡張中分類すべて
							return true;
						}
						if( c.user_nibble_2 == eventData[j].user_nibble_2 ){
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}

bool CEpgDBManager::IsInDateTime(const vector<EPGDB_SEARCH_DATE_INFO>& dateList, const SYSTEMTIME& time)
{
	int weekMin = (time.wDayOfWeek * 24 + time.wHour) * 60 + time.wMinute;
	for( size_t i=0; i<dateList.size(); i++ ){
		int start = (dateList[i].startDayOfWeek * 24 + dateList[i].startHour) * 60 + dateList[i].startMin;
		int end = (dateList[i].endDayOfWeek * 24 + dateList[i].endHour) * 60 + dateList[i].endMin;
		if( start >= end ){
			if( start <= weekMin || weekMin <= end ){
				return true;
			}
		}else{
			if( start <= weekMin && weekMin <= end ){
				return true;
			}
		}
	}

	return false;
}

bool CEpgDBManager::FindKeyword(const vector<pair<wstring, RegExpPtr>>& keyList, const EPGDB_EVENT_INFO& info, wstring& word,
                                vector<int>& dist, bool caseFlag, bool aimai, bool andFlag, wstring* findKey)
{
	for( size_t i = 0; i < keyList.size(); i++ ){
		const wstring& key = keyList[i].first;
		if( i == 0 || key.compare(0, 7, keyList[i - 1].first) ){
			//検索対象が変わったので作成
			word.clear();
			if( key.compare(0, 7, L":title:") == 0 ){
				if( info.hasShortInfo ){
					word += info.shortInfo.event_name;
				}
			}else if( key.compare(0, 7, L":event:") == 0 ){
				if( info.hasShortInfo ){
					word += info.shortInfo.event_name;
					word += L"\r\n";
					word += info.shortInfo.text_char;
					if( info.hasExtInfo ){
						word += L"\r\n";
						word += info.extInfo.text_char;
					}
				}
			}else if( key.compare(0, 7, L":genre:") == 0 ){
				AppendEpgContentInfoText(word, info);
			}else if( key.compare(0, 7, L":video:") == 0 ){
				AppendEpgComponentInfoText(word, info);
			}else if( key.compare(0, 7, L":audio:") == 0 ){
				AppendEpgAudioComponentInfoText(word, info);
			}else{
				throw std::runtime_error("");
			}
			ConvertSearchText(word);
		}

		if( keyList[i].second ){
			//正規表現
#if !defined(EPGDB_STD_WREGEX) && defined(_WIN32)
			OleCharPtr target(SysAllocString(word.c_str()), SysFreeString);
			if( target ){
				IDispatch* pMatches;
				if( SUCCEEDED(keyList[i].second->Execute(target.get(), &pMatches)) ){
					std::unique_ptr<IMatchCollection, decltype(&ComRelease)> matches((IMatchCollection*)pMatches, ComRelease);
					long count;
					if( SUCCEEDED(matches->get_Count(&count)) && count > 0 ){
						if( andFlag == false ){
							//見つかったので終了
							return true;
						}
						if( findKey && i + 1 == keyList.size() ){
							//最終キーのマッチを記録
							IDispatch* pMatch;
							if( SUCCEEDED(matches->get_Item(0, &pMatch)) ){
								std::unique_ptr<IMatch2, decltype(&ComRelease)> match((IMatch2*)pMatch, ComRelease);
								BSTR value_;
								if( SUCCEEDED(match->get_Value(&value_)) ){
									OleCharPtr value(value_, SysFreeString);
									*findKey = SysStringLen(value.get()) ? value.get() : L"";
								}
							}
						}
					}else if( andFlag ){
						//見つからなかったので終了
						return false;
					}
				}else if( andFlag ){
					return false;
				}
#else
			std::wsmatch m;
			if( std::regex_search(word, m, *keyList[i].second) ){
				if( andFlag == false ){
					//見つかったので終了
					return true;
				}
				if( findKey && i + 1 == keyList.size() ){
					//最終キーのマッチを記録
					*findKey = m[0];
				}
#endif
			}else if( andFlag ){
				return false;
			}
		}else{
			//通常
			if( key.size() > 7 &&
			    (aimai ? FindLikeKeyword(key, 7, word, dist, caseFlag) :
			     caseFlag ? std::search(word.begin(), word.end(), key.begin() + 7, key.end()) != word.end() :
			                std::search(word.begin(), word.end(), key.begin() + 7, key.end(),
			                            [](wchar_t l, wchar_t r) { return (L'a' <= l && l <= L'z' ? l - L'a' + L'A' : l) ==
			                                                              (L'a' <= r && r <= L'z' ? r - L'a' + L'A' : r); }) != word.end()) ){
				if( andFlag == false ){
					//見つかったので終了
					return true;
				}
			}else if( andFlag ){
				//見つからなかったので終了
				return false;
			}
		}
	}

	if( andFlag && findKey ){
		//見つかったキーを記録
		size_t n = findKey->size();
		for( size_t i = 0; i < keyList.size(); i++ ){
			if( keyList[i].second == NULL ){
				if( n == 0 && findKey->empty() == false ){
					*findKey += L' ';
				}
				findKey->insert(findKey->size() - n, keyList[i].first, 7, wstring::npos);
				if( n != 0 ){
					findKey->insert(findKey->end() - n, L' ');
				}
			}
		}
	}
	return andFlag;
}

bool CEpgDBManager::FindLikeKeyword(const wstring& key, size_t keyPos, const wstring& word, vector<int>& dist, bool caseFlag)
{
	//編集距離がしきい値以下になる文字列が含まれるか調べる
	size_t l = 0;
	size_t curr = key.size() - keyPos + 1;
	dist.assign(curr * 2, 0);
	for( size_t i = 1; i < curr; i++ ){
		dist[i] = dist[i - 1] + 1;
	}
	for( size_t i = 0; i < word.size(); i++ ){
		wchar_t x = word[i];
		for( size_t j = 0; j < key.size() - keyPos; j++ ){
			wchar_t y = key[j + keyPos];
			if( caseFlag && x == y ||
			    caseFlag == false && (L'a' <= x && x <= L'z' ? x - L'a' + L'A' : x) == (L'a' <= y && y <= L'z' ? y - L'a' + L'A' : y) ){
				dist[curr + j + 1] = dist[l + j];
			}else{
				dist[curr + j + 1] = 1 + (dist[l + j] < dist[l + j + 1] ? min(dist[l + j], dist[curr + j]) : min(dist[l + j + 1], dist[curr + j]));
			}
		}
		//75%をしきい値とする
		if( dist[curr + key.size() - keyPos] * 4 <= (int)(key.size() - keyPos) ){
			return true;
		}
		std::swap(l, curr);
	}
	return false;
}

void CEpgDBManager::AddKeyword(vector<pair<wstring, RegExpPtr>>& keyList, wstring key, bool caseFlag, bool regExp, bool titleOnly)
{
	keyList.push_back(std::make_pair(wstring(), RegExpPtr(
#if !defined(EPGDB_STD_WREGEX) && defined(_WIN32)
		NULL, ComRelease
#endif
		)));
	if( regExp ){
		key = (titleOnly ? L"::title:" : L"::event:") + key;
	}
	size_t regPrefix = key.compare(0, 2, L"::") ? 0 : 1;
	if( key.compare(regPrefix, 7, L":title:") &&
	    key.compare(regPrefix, 7, L":event:") &&
	    key.compare(regPrefix, 7, L":genre:") &&
	    key.compare(regPrefix, 7, L":video:") &&
	    key.compare(regPrefix, 7, L":audio:") ){
		//検索対象が不明なので指定する
		key = (titleOnly ? L":title:" : L":event:") + key;
	}else if( regPrefix != 0 ){
		key.erase(0, 1);
		//旧い処理では対象を全角空白のまま比較していたため正規表現も全角のケースが多い。特別に置き換える
		Replace(key, L"　", L" ");
		//RegExpオブジェクトを構築しておく
#if !defined(EPGDB_STD_WREGEX) && defined(_WIN32)
		void* pv;
		if( SUCCEEDED(CoCreateInstance(CLSID_RegExp, NULL, CLSCTX_INPROC_SERVER, IID_IRegExp, &pv)) ){
			keyList.back().second.reset((IRegExp*)pv);
			OleCharPtr pattern(SysAllocString(key.c_str() + 7), SysFreeString);
			if( pattern &&
			    SUCCEEDED(keyList.back().second->put_IgnoreCase(caseFlag ? VARIANT_FALSE : VARIANT_TRUE)) &&
			    SUCCEEDED(keyList.back().second->put_Pattern(pattern.get())) ){
				keyList.back().first.swap(key);
				return;
			}
			keyList.back().second.reset();
		}
#else
		try{
			keyList.back().second.reset(new std::wregex(key.c_str() + 7,
				caseFlag ? std::regex_constants::ECMAScript : std::regex_constants::ECMAScript | std::regex_constants::icase));
			keyList.back().first.swap(key);
			return;
		}catch( std::regex_error& ){
		}
#endif
		//空(常に不一致)にする
		key.erase(7);
	}
	ConvertSearchText(key);
	keyList.back().first.swap(key);
}

bool CEpgDBManager::GetServiceList(vector<EPGDB_SERVICE_INFO>* list) const
{
	CRefLock lock(&this->epgMapRefLock);

	if( this->epgMap.empty() ){
		return false;
	}
	list->reserve(list->size() + this->epgMap.size());
	for( auto itr = this->epgMap.cbegin(); itr != this->epgMap.end(); itr++ ){
		list->push_back(itr->second.serviceInfo);
	}
	return true;
}

pair<LONGLONG, LONGLONG> CEpgDBManager::GetEventMinMaxTimeProc(LONGLONG keyMask, LONGLONG key, bool archive) const
{
	const map<LONGLONG, EPGDB_SERVICE_EVENT_INFO>& target = archive ? this->epgArchive : this->epgMap;
	pair<LONGLONG, LONGLONG> ret(LLONG_MAX, LLONG_MIN);
	auto itr = target.begin();
	auto itrEnd = target.end();
	if( keyMask == 0 ){
		itrEnd = itr = target.find(key);
		if( itr != target.end() ){
			itrEnd++;
		}
	}
	for( ; itr != itrEnd; itr++ ){
		if( (itr->first | keyMask) == key ){
			for( auto jtr = itr->second.eventList.begin(); jtr != itr->second.eventList.end(); jtr++ ){
				if( jtr->StartTimeFlag ){
					LONGLONG startTime = ConvertI64Time(jtr->start_time);
					ret.first = min(ret.first, startTime);
					ret.second = max(ret.second, startTime);
				}
			}
		}
	}
	return ret;
}

pair<LONGLONG, LONGLONG> CEpgDBManager::GetArchiveEventMinMaxTime(LONGLONG keyMask, LONGLONG key) const
{
	CRefLock lock(&this->epgMapRefLock);

	pair<LONGLONG, LONGLONG> ret = GetEventMinMaxTimeProc(keyMask, key, true);
	if( this->epgOldIndexCache.empty() == false ){
		const vector<LONGLONG>& timeList = this->epgOldIndexCache.front();
		//長期アーカイブの最小開始時間を調べる
		bool found = false;
		for( size_t i = 0; found == false && i < timeList.size(); i++ ){
			const vector<LONGLONG>& index = this->epgOldIndexCache[1 + i];
			for( size_t j = 0; j + 3 < index.size(); j += 4 ){
				if( (index[j + 1] | keyMask) == key ){
					ret.first = min(ret.first, timeList[i] + index[j + 2]);
					found = true;
				}
			}
		}
		//長期アーカイブの最大開始時間を調べる
		found = false;
		for( size_t i = timeList.size(); found == false && i > 0; i-- ){
			const vector<LONGLONG>& index = this->epgOldIndexCache[i];
			for( size_t j = 0; j + 3 < index.size(); j += 4 ){
				if( (index[j + 1] | keyMask) == key ){
					ret.second = max(ret.second, timeList[i - 1] + index[j + 3]);
					found = true;
				}
			}
		}
	}
	return ret;
}

bool CEpgDBManager::EnumEventInfoProc(LONGLONG* keys, size_t keysSize, LONGLONG enumStart, LONGLONG enumEnd,
                                      const std::function<void(const EPGDB_EVENT_INFO*, const EPGDB_SERVICE_INFO*)>& enumProc, bool archive) const
{
	const map<LONGLONG, EPGDB_SERVICE_EVENT_INFO>& target = archive ? this->epgArchive : this->epgMap;
	auto itr = target.begin();
	auto itrEnd = target.end();
	if( keysSize == 2 && keys[0] == 0 ){
		itrEnd = itr = target.find(keys[1]);
		if( itr == target.end() || (archive == false && itr->second.eventList.empty()) ){
			return false;
		}
		itrEnd++;
	}
	for( ; itr != itrEnd; itr++ ){
		for( size_t i = 0; i + 1 < keysSize; i += 2 ){
			if( (itr->first | keys[i]) == keys[i + 1] ){
				for( auto jtr = itr->second.eventList.begin(); jtr != itr->second.eventList.end(); jtr++ ){
					//非アーカイブでは時間未定含む列挙と時間未定のみ列挙の特別扱いがある
					if( archive || ((enumStart != 0 || enumEnd != LLONG_MAX) && (enumStart != LLONG_MAX || jtr->StartTimeFlag)) ){
						if( jtr->StartTimeFlag == 0 ){
							continue;
						}
						LONGLONG startTime = ConvertI64Time(jtr->start_time);
						if( startTime < enumStart || enumEnd <= startTime ){
							continue;
						}
					}
					enumProc(&*jtr, &itr->second.serviceInfo);
				}
				break;
			}
		}
	}
	//列挙完了
	enumProc(NULL, NULL);
	return true;
}

void CEpgDBManager::EnumArchiveEventInfo(LONGLONG* keys, size_t keysSize, LONGLONG enumStart, LONGLONG enumEnd, bool deletableBeforeEnumDone,
                                         const std::function<void(const EPGDB_EVENT_INFO*, const EPGDB_SERVICE_INFO*)>& enumProc) const
{
	CRefLock lock(&this->epgMapRefLock);

	std::list<EPGDB_SERVICE_EVENT_INFO> infoPool;
	if( enumStart < enumEnd && this->epgOldIndexCache.size() > 1 ){
		//長期アーカイブも読む。deletableBeforeEnumDone時は列挙中であっても以前に列挙されたデータの生存は保証しない
		fs_path epgArcPath;
		const vector<LONGLONG>& timeList = this->epgOldIndexCache.front();
		//対象期間だけ読めばOK
		auto itr = std::upper_bound(timeList.begin(), timeList.end(), enumStart);
		if( itr != timeList.begin() && enumStart < *(itr - 1) + 7 * 24 * 3600 * I64_1SEC ){
			itr--;
		}
		auto itrEnd = std::lower_bound(itr, timeList.end(), enumEnd);
		vector<BYTE> buff;
		vector<LONGLONG> index;
		EPGDB_SERVICE_EVENT_INFO info;
		for( ; itr != itrEnd; itr++ ){
			if( epgArcPath.empty() ){
				epgArcPath = GetSettingPath().append(EPG_ARCHIVE_FOLDER);
			}
			std::unique_ptr<FILE, fclose_deleter> fp(OpenOldArchive(epgArcPath.c_str(), *itr, UTIL_SECURE_READ));
			if( fp ){
				DWORD headerSize;
				ReadOldArchiveIndex(fp.get(), buff, index, &headerSize);
				for( size_t i = 0; i + 3 < index.size(); i += 4 ){
					for( size_t j = 0; j + 1 < keysSize; j += 2 ){
						if( (index[i + 1] | keys[j]) == keys[j + 1] ){
							//対象サービスだけ読めばOK
							EPGDB_SERVICE_EVENT_INFO* pi = &info;
							if( deletableBeforeEnumDone == false ){
								infoPool.push_back(EPGDB_SERVICE_EVENT_INFO());
								pi = &infoPool.back();
							}
							ReadOldArchiveEventInfo(fp.get(), index, i, headerSize, buff, *pi);
							for( auto jtr = pi->eventList.cbegin(); jtr != pi->eventList.end(); jtr++ ){
								if( jtr->StartTimeFlag ){
									LONGLONG startTime = ConvertI64Time(jtr->start_time);
									if( enumStart <= startTime && startTime < enumEnd ){
										enumProc(&*jtr, &pi->serviceInfo);
									}
								}
							}
							break;
						}
					}
				}
			}
		}
	}
	if( EnumEventInfoProc(keys, keysSize, enumStart, enumEnd, enumProc, true) == false ){
		//列挙完了
		enumProc(NULL, NULL);
	}
}

bool CEpgDBManager::SearchEpg(
	WORD ONID,
	WORD TSID,
	WORD SID,
	WORD EventID,
	EPGDB_EVENT_INFO* result
	) const
{
	CRefLock lock(&this->epgMapRefLock);

	LONGLONG key = Create64Key(ONID, TSID, SID);
	auto itr = this->epgMap.find(key);
	if( itr != this->epgMap.end() ){
		EPGDB_EVENT_INFO infoKey;
		infoKey.event_id = EventID;
		auto itrInfo = std::lower_bound(itr->second.eventList.begin(), itr->second.eventList.end(), infoKey,
		                                [](const EPGDB_EVENT_INFO& a, const EPGDB_EVENT_INFO& b) { return a.event_id < b.event_id; });
		if( itrInfo != itr->second.eventList.end() && itrInfo->event_id == EventID ){
			*result = *itrInfo;
			return true;
		}
	}
	return false;
}

bool CEpgDBManager::SearchEpg(
	WORD ONID,
	WORD TSID,
	WORD SID,
	LONGLONG startTime,
	DWORD durationSec,
	EPGDB_EVENT_INFO* result
	) const
{
	CRefLock lock(&this->epgMapRefLock);

	LONGLONG key = Create64Key(ONID, TSID, SID);
	auto itr = this->epgMap.find(key);
	if( itr != this->epgMap.end() ){
		for( auto itrInfo = itr->second.eventList.cbegin(); itrInfo != itr->second.eventList.end(); itrInfo++ ){
			if( itrInfo->StartTimeFlag != 0 && itrInfo->DurationFlag != 0 ){
				if( startTime == ConvertI64Time(itrInfo->start_time) &&
					durationSec == itrInfo->durationSec
					){
						*result = *itrInfo;
						return true;
				}
			}
		}
	}
	return false;
}

bool CEpgDBManager::SearchServiceName(
	WORD ONID,
	WORD TSID,
	WORD SID,
	wstring& serviceName
	) const
{
	CRefLock lock(&this->epgMapRefLock);

	LONGLONG key = Create64Key(ONID, TSID, SID);
	auto itr = this->epgMap.find(key);
	if( itr != this->epgMap.end() ){
		serviceName = itr->second.serviceInfo.service_name;
		return true;
	}
	return false;
}

//検索対象や検索パターンから全半角の区別を取り除く(旧ConvertText.txtに相当)
//ConvertText.txtと異なり半角濁点カナを(意図通り)置換する点、［］，．全角空白を置換する点、―(U+2015)ゐヰゑヱΖ(U+0396)を置換しない点に注意
void CEpgDBManager::ConvertSearchText(wstring& str)
{
	//全角英数およびこのテーブルにある文字列を置換する
	//最初の文字(UTF-16)をキーとしてソート済み。同一キー内の順序はマッチの優先順
	static const WCHAR convertFrom[][2] = {
		L"’", L"”", L"　",
		L"！", L"＃", L"＄", L"％", L"＆", L"（", L"）", L"＊", L"＋", L"，", L"－", L"．", L"／",
		L"：", L"；", L"＜", L"＝", L"＞", L"？", L"＠", L"［", L"］", L"＾", L"＿", L"｀", L"｛", L"｜", L"｝", L"～",
		L"｡", L"｢", L"｣", L"､", L"･", L"ｦ", L"ｧ", L"ｨ", L"ｩ", L"ｪ", L"ｫ", L"ｬ", L"ｭ", L"ｮ", L"ｯ", L"ｰ", L"ｱ", L"ｲ", L"ｳ", L"ｴ", L"ｵ",
		{L'ｶ', L'ﾞ'}, L"ｶ", {L'ｷ', L'ﾞ'}, L"ｷ", {L'ｸ', L'ﾞ'}, L"ｸ", {L'ｹ', L'ﾞ'}, L"ｹ", {L'ｺ', L'ﾞ'}, L"ｺ",
		{L'ｻ', L'ﾞ'}, L"ｻ", {L'ｼ', L'ﾞ'}, L"ｼ", {L'ｽ', L'ﾞ'}, L"ｽ", {L'ｾ', L'ﾞ'}, L"ｾ", {L'ｿ', L'ﾞ'}, L"ｿ",
		{L'ﾀ', L'ﾞ'}, L"ﾀ", {L'ﾁ', L'ﾞ'}, L"ﾁ", {L'ﾂ', L'ﾞ'}, L"ﾂ", {L'ﾃ', L'ﾞ'}, L"ﾃ", {L'ﾄ', L'ﾞ'}, L"ﾄ",
		L"ﾅ", L"ﾆ", L"ﾇ", L"ﾈ", L"ﾉ",
		{L'ﾊ', L'ﾞ'}, {L'ﾊ', L'ﾟ'}, L"ﾊ", {L'ﾋ', L'ﾞ'}, {L'ﾋ', L'ﾟ'}, L"ﾋ", {L'ﾌ', L'ﾞ'}, {L'ﾌ', L'ﾟ'}, L"ﾌ",
		{L'ﾍ', L'ﾞ'}, {L'ﾍ', L'ﾟ'}, L"ﾍ", {L'ﾎ', L'ﾞ'}, {L'ﾎ', L'ﾟ'}, L"ﾎ",
		L"ﾏ", L"ﾐ", L"ﾑ", L"ﾒ", L"ﾓ", L"ﾔ", L"ﾕ", L"ﾖ", L"ﾗ", L"ﾘ", L"ﾙ", L"ﾚ", L"ﾛ", L"ﾜ", L"ﾝ", L"ﾞ", L"ﾟ",
		L"￥",
	};
	static const WCHAR convertTo[] = {
		L'\'', L'"', L' ',
		L'!', L'#', L'$', L'%', L'&', L'(', L')', L'*', L'+', L',', L'-', L'.', L'/',
		L':', L';', L'<', L'=', L'>', L'?', L'@', L'[', L']', L'^', L'_', L'`', L'{', L'|', L'}', L'~',
		L'。', L'「', L'」', L'、', L'・', L'ヲ', L'ァ', L'ィ', L'ゥ', L'ェ', L'ォ', L'ャ', L'ュ', L'ョ', L'ッ', L'ー', L'ア', L'イ', L'ウ', L'エ', L'オ',
		L'ガ', L'カ', L'ギ', L'キ', L'グ', L'ク', L'ゲ', L'ケ', L'ゴ', L'コ',
		L'ザ', L'サ', L'ジ', L'シ', L'ズ', L'ス', L'ゼ', L'セ', L'ゾ', L'ソ',
		L'ダ', L'タ', L'ヂ', L'チ', L'ヅ', L'ツ', L'デ', L'テ', L'ド', L'ト',
		L'ナ', L'ニ', L'ヌ', L'ネ', L'ノ',
		L'バ', L'パ', L'ハ', L'ビ', L'ピ', L'ヒ', L'ブ', L'プ', L'フ',
		L'ベ', L'ペ', L'ヘ', L'ボ', L'ポ', L'ホ',
		L'マ', L'ミ', L'ム', L'メ', L'モ', L'ヤ', L'ユ', L'ヨ', L'ラ', L'リ', L'ル', L'レ', L'ロ', L'ワ', L'ン', L'゛', L'゜',
		L'\\',
	};

	for( wstring::iterator itr = str.begin(), itrEnd = str.end(); itr != itrEnd; itr++ ){
		//注意: これは符号位置の連続性を利用してテーブル参照を減らすための条件。上記のテーブルを弄る場合はここを確認すること
		WCHAR c = *itr;
		if( (L'！' <= c && c <= L'￥') || c == L'　' || c == L'’' || c == L'”' ){
			if( L'０' <= c && c <= L'９' ){
				*itr = c - L'０' + L'0';
			}else if( L'Ａ' <= c && c <= L'Ｚ' ){
				*itr = c - L'Ａ' + L'A';
			}else if( L'ａ' <= c && c <= L'ｚ' ){
				*itr = c - L'ａ' + L'a';
			}else{
				const WCHAR (*f)[2] = std::lower_bound(convertFrom, convertFrom + array_size(convertFrom), &*itr,
				                                       [](LPCWSTR a, LPCWSTR b) { return (unsigned short)a[0] < (unsigned short)b[0]; });
				for( ; f != convertFrom + array_size(convertFrom) && (*f)[0] == c; f++ ){
					if( (*f)[1] == L'\0' ){
						*itr = convertTo[f - convertFrom];
						break;
					}else if( itr + 1 != itrEnd && *(itr + 1) == (*f)[1] ){
						size_t i = itrEnd - itr - 1;
						str.replace(itr, itr + 2, 1, convertTo[f - convertFrom]);
						//イテレータを再有効化
						itrEnd = str.end();
						itr = itrEnd - i;
						break;
					}
				}
			}
		}
	}
}

