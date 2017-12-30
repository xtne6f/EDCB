#include "stdafx.h"
#include "EpgDBManager.h"

#include "../../Common/CommonDef.h"
#include "../../Common/TimeUtil.h"
#include "../../Common/StringUtil.h"
#include "../../Common/PathUtil.h"
#include "../../Common/EpgTimerUtil.h"
#include "../../Common/EpgDataCap3Util.h"
#include "../../Common/CtrlCmdUtil.h"

extern DWORD g_compatFlags;

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
	CBlockLock lock(&this->epgMapLock);
	//それほど長くない期間に制限する。より長期保存を許す場合は負荷に気をつけること
	this->archivePeriodSec = min(periodSec, 14 * 24 * 3600);
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

void CEpgDBManager::LoadThread(CEpgDBManager* sys)
{
	OutputDebugString(L"Start Load EpgData\r\n");
	DWORD time = GetTickCount();

	if( sys->loadForeground == false ){
		//バックグラウンドに移行
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
	}
	CEpgDataCap3Util epgUtil;
	if( epgUtil.Initialize(FALSE) != NO_ERR ){
		OutputDebugString(L"★EpgDataCap3.dllの初期化に失敗しました。\r\n");
		sys->loadForeground = false;
		sys->initialLoadDone = true;
		return;
	}

	__int64 utcNow = GetNowI64Time() - I64_UTIL_TIMEZONE;

	//EPGファイルの検索
	vector<wstring> epgFileList;
	const fs_path settingPath = GetSettingPath();
	const fs_path epgDataPath = fs_path(settingPath).append(EPG_SAVE_FOLDER);

	WIN32_FIND_DATA findData;
	HANDLE find;

	//指定フォルダのファイル一覧取得
	find = FindFirstFile(fs_path(epgDataPath).append(L"*_epg.dat").c_str(), &findData);
	if( find != INVALID_HANDLE_VALUE ){
		do{
			__int64 fileTime = (__int64)findData.ftLastWriteTime.dwHighDateTime << 32 | findData.ftLastWriteTime.dwLowDateTime;
			if( (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 && fileTime != 0 ){
				//見つかったファイルを一覧に追加
				//名前順。ただしTSID==0xFFFFの場合は同じチャンネルの連続によりストリームがクリアされない可能性があるので後ろにまとめる
				WCHAR prefix = fileTime + 7*24*60*60*I64_1SEC < utcNow ? L'0' :
				               wcslen(findData.cFileName) < 12 || _wcsicmp(findData.cFileName + wcslen(findData.cFileName) - 12, L"ffff_epg.dat") ? L'1' : L'2';
				wstring item = prefix + fs_path(epgDataPath).append(findData.cFileName).native();
				epgFileList.insert(std::lower_bound(epgFileList.begin(), epgFileList.end(), item), item);
			}
		}while( FindNextFile(find, &findData) );
		FindClose(find);
	}

	DWORD loadElapsed = 0;
	DWORD loadTick = GetTickCount();

	//EPGファイルの解析
	for( vector<wstring>::iterator itr = epgFileList.begin(); itr != epgFileList.end(); itr++ ){
		if( sys->loadStop ){
			//キャンセルされた
			return;
		}
		//一時ファイルの状態を調べる。取得側のCreateFile(tmp)→CloseHandle(tmp)→CopyFile(tmp,master)→DeleteFile(tmp)の流れをある程度仮定
		wstring path = itr->c_str() + 1;
		HANDLE tmpFile = CreateFile((path + L".tmp").c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		DWORD tmpError = GetLastError();
		if( tmpFile != INVALID_HANDLE_VALUE ){
			tmpError = NO_ERROR;
			FILETIME ft;
			if( GetFileTime(tmpFile, NULL, NULL, &ft) == FALSE || ((LONGLONG)ft.dwHighDateTime << 32 | ft.dwLowDateTime) + 300*I64_1SEC < utcNow ){
				//おそらく後始末されていない一時ファイルなので無視
				tmpError = ERROR_FILE_NOT_FOUND;
			}
			CloseHandle(tmpFile);
		}
		if( (*itr)[0] == L'0' ){
			if( tmpError != NO_ERROR && tmpError != ERROR_SHARING_VIOLATION ){
				//1週間以上前かつ一時ファイルがないので削除
				DeleteFile(path.c_str());
				_OutputDebugString(L"★delete %s\r\n", path.c_str());
			}
		}else{
			BYTE readBuff[188*256];
			bool swapped = false;
			HANDLE file = INVALID_HANDLE_VALUE;
			if( tmpError == NO_ERROR ){
				//一時ファイルがあって書き込み中でない→コピー直前かもしれないので3秒待つ
				Sleep(3000);
			}else if( tmpError == ERROR_SHARING_VIOLATION ){
				//一時ファイルがあって書き込み中→もうすぐ上書きされるかもしれないのでできるだけ退避させる
				HANDLE masterFile = CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
				if( masterFile != INVALID_HANDLE_VALUE ){
					file = CreateFile((path + L".swp").c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
					if( file != INVALID_HANDLE_VALUE ){
						swapped = true;
						DWORD read;
						while( ReadFile(masterFile, readBuff, sizeof(readBuff), &read, NULL) && read != 0 ){
							DWORD written;
							WriteFile(file, readBuff, read, &written, NULL);
						}
						SetFilePointer(file, 0, 0, FILE_BEGIN);
						tmpFile = CreateFile((path + L".tmp").c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
						if( tmpFile != INVALID_HANDLE_VALUE || GetLastError() != ERROR_SHARING_VIOLATION ){
							//退避中に書き込みが終わった
							if( tmpFile != INVALID_HANDLE_VALUE ){
								CloseHandle(tmpFile);
							}
							CloseHandle(file);
							file = INVALID_HANDLE_VALUE;
						}
					}
					CloseHandle(masterFile);
				}
				if( file == INVALID_HANDLE_VALUE ){
					Sleep(3000);
				}
			}
			if( file == INVALID_HANDLE_VALUE ){
				file = CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			}
			if( file == INVALID_HANDLE_VALUE ){
				_OutputDebugString(L"Error %s\r\n", path.c_str());
			}else{
				//PATを送る(ストリームを確実にリセットするため)
				DWORD seekPos = 0;
				DWORD read;
				for( DWORD i=0; ReadFile(file, readBuff, 188, &read, NULL) && read == 188; i+=188 ){
					//PID
					if( ((readBuff[1] & 0x1F) << 8 | readBuff[2]) == 0 ){
						//payload_unit_start_indicator
						if( (readBuff[1] & 0x40) != 0 ){
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
				SetFilePointer(file, seekPos, 0, FILE_BEGIN);
				//TOTを先頭に持ってきて送る(ストリームの時刻を確定させるため)
				bool ignoreTOT = false;
				while( ReadFile(file, readBuff, 188, &read, NULL) && read == 188 ){
					if( ((readBuff[1] & 0x1F) << 8 | readBuff[2]) == 0x14 ){
						ignoreTOT = true;
						epgUtil.AddTSPacket(readBuff, 188);
						break;
					}
				}
				SetFilePointer(file, seekPos, 0, FILE_BEGIN);
				while( ReadFile(file, readBuff, sizeof(readBuff), &read, NULL) && read != 0 ){
					for( DWORD i=0; i<read; i+=188 ){
						if( ignoreTOT && ((readBuff[i+1] & 0x1F) << 8 | readBuff[i+2]) == 0x14 ){
							ignoreTOT = false;
						}else{
							epgUtil.AddTSPacket(readBuff+i, 188);
						}
					}
					if( sys->loadForeground == false ){
						//処理速度がだいたい2/3になるように休む。I/O負荷軽減が狙い
						DWORD tick = GetTickCount();
						loadElapsed += tick - loadTick;
						loadTick = tick;
						if( loadElapsed > 20 ){
							Sleep(min<DWORD>(loadElapsed / 2, 100));
							loadElapsed = 0;
							loadTick = GetTickCount();
						}
					}
				}
				CloseHandle(file);
			}
			if( swapped ){
				DeleteFile((path + L".swp").c_str());
			}
		}
	}

	//EPGデータを取得
	DWORD serviceListSize = 0;
	SERVICE_INFO* serviceList = NULL;
	if( epgUtil.GetServiceListEpgDB(&serviceListSize, &serviceList) == FALSE ){
		sys->loadForeground = false;
		sys->initialLoadDone = true;
		return;
	}
	map<LONGLONG, EPGDB_SERVICE_EVENT_INFO> nextMap;
	for( const SERVICE_INFO* info = serviceList; info != serviceList + serviceListSize; info++ ){
		LONGLONG key = Create64Key(info->original_network_id, info->transport_stream_id, info->service_id);
		EPGDB_SERVICE_EVENT_INFO& item = nextMap.insert(std::make_pair(key, EPGDB_SERVICE_EVENT_INFO())).first->second;
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

	__int64 arcMax = GetNowI64Time();
	__int64 arcMin;
	{
		CBlockLock lock(&sys->epgMapLock);
		arcMin = sys->archivePeriodSec <= 0 ? LLONG_MAX : arcMax - sys->archivePeriodSec * I64_1SEC;
	}
	arcMax += 3600 * I64_1SEC;

	//初回はアーカイブファイルから読み込む
	map<LONGLONG, EPGDB_SERVICE_EVENT_INFO> arcFromFile;
	if( arcMin < LLONG_MAX && sys->epgArchive.empty() ){
		vector<BYTE> buff;
		std::unique_ptr<FILE, decltype(&fclose)> fp(secure_wfopen(fs_path(settingPath).append(L"EpgArc.dat").c_str(), L"rbN"), fclose);
		if( fp && _fseeki64(fp.get(), 0, SEEK_END) == 0 ){
			__int64 fileSize = _ftelli64(fp.get());
			if( 0 < fileSize && fileSize < INT_MAX ){
				buff.resize((size_t)fileSize);
				rewind(fp.get());
				if( fread(&buff.front(), 1, buff.size(), fp.get()) != buff.size() ){
					buff.clear();
				}
			}
		}
		if( buff.empty() == false ){
			WORD ver;
			DWORD readSize;
			vector<EPGDB_SERVICE_EVENT_INFO> list;
			if( ReadVALUE(&ver, &buff.front(), (DWORD)buff.size(), &readSize) &&
			    ReadVALUE2(ver, &list, &buff.front() + readSize, (DWORD)buff.size() - readSize, NULL) ){
				for( size_t i = 0; i < list.size(); i++ ){
					LONGLONG key = Create64Key(list[i].serviceInfo.ONID, list[i].serviceInfo.TSID, list[i].serviceInfo.SID);
					arcFromFile[key] = std::move(list[i]);
				}
			}
		}
	}

	if( sys->loadForeground == false ){
		//フォアグラウンドに復帰
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
	}
	for(;;){
		//データベースを排他する
		if( sys->epgMapRefLock.first == 0 ){
			CBlockLock lock(&sys->epgMapLock);
			if( sys->epgMapRefLock.first == 0 ){
				if( arcFromFile.empty() == false ){
					sys->epgArchive.swap(arcFromFile);
				}
				//アーカイブから古いイベントを消す
				for( auto itr = sys->epgArchive.begin(); itr != sys->epgArchive.end(); itr++ ){
					itr->second.eventList.erase(std::remove_if(itr->second.eventList.begin(), itr->second.eventList.end(), [=](const EPGDB_EVENT_INFO& a) {
						return ConvertI64Time(a.start_time) <= arcMin || ConvertI64Time(a.start_time) >= arcMax;
					}), itr->second.eventList.end());
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
						__int64 minStart = LLONG_MAX;
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
				//アーカイブから空のサービスを消す
				for( auto itr = sys->epgArchive.begin(); itr != sys->epgArchive.end(); ){
					if( itr->second.eventList.empty() ){
						sys->epgArchive.erase(itr++);
					}else{
						itr++;
					}
				}
				break;
			}
		}
		Sleep(1);
	}
	if( sys->loadForeground == false ){
		//バックグラウンドに移行
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
	}
	nextMap.clear();

	//アーカイブファイルに書き込む
	if( arcMin < LLONG_MAX ){
		vector<const EPGDB_SERVICE_EVENT_INFO*> valp;
		valp.reserve(sys->epgArchive.size());
		for( auto itr = sys->epgArchive.cbegin(); itr != sys->epgArchive.end(); valp.push_back(&(itr++)->second) );
		DWORD buffSize;
		std::unique_ptr<BYTE[]> buff = NewWriteVALUE2WithVersion(5, valp, buffSize);
		std::unique_ptr<FILE, decltype(&fclose)> fp(secure_wfopen(fs_path(settingPath).append(L"EpgArc.dat").c_str(), L"wbN"), fclose);
		if( fp ){
			fwrite(buff.get(), 1, buffSize, fp.get());
		}
	}

	_OutputDebugString(L"End Load EpgData %dmsec\r\n", GetTickCount()-time);

	sys->loadForeground = false;
	sys->initialLoadDone = true;
}

BOOL CALLBACK CEpgDBManager::EnumEpgInfoListProc(DWORD epgInfoListSize, EPG_EVENT_INFO* epgInfoList, LPVOID param)
{
	EPGDB_SERVICE_EVENT_INFO* item = (EPGDB_SERVICE_EVENT_INFO*)param;

	try{
		if( epgInfoList == NULL ){
			item->eventList.reserve(epgInfoListSize);
		}else{
			for( DWORD i=0; i<epgInfoListSize; i++ ){
				item->eventList.resize(item->eventList.size() + 1);
				ConvertEpgInfo(item->serviceInfo.ONID, item->serviceInfo.TSID, item->serviceInfo.SID, &epgInfoList[i], &item->eventList.back());
				if( item->eventList.back().shortInfo != NULL ){
					//ごく稀にAPR(改行)を含むため
					Replace(item->eventList.back().shortInfo->event_name, L"\r\n", L"");
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
	return this->loadThread.joinable() && WaitForSingleObject(this->loadThread.native_handle(), 0) == WAIT_TIMEOUT;
}

void CEpgDBManager::CancelLoadData()
{
	if( this->loadThread.joinable() ){
		this->loadStop = true;
		this->loadThread.join();
		this->loadStop = false;
	}
}

bool CEpgDBManager::SearchEpg(const vector<EPGDB_SEARCH_KEY_INFO>* key, vector<SEARCH_RESULT_EVENT_DATA>* result) const
{
	return SearchEpg(key, [=](vector<SEARCH_RESULT_EVENT>& val) {
		result->reserve(result->size() + val.size());
		for( vector<SEARCH_RESULT_EVENT>::iterator itr = val.begin(); itr != val.end(); itr++ ){
			result->resize(result->size() + 1);
			result->back().info.DeepCopy(*itr->info);
			result->back().findKey.swap(itr->findKey);
		}
	});
}

void CEpgDBManager::SearchEvent(const EPGDB_SEARCH_KEY_INFO* key, vector<SEARCH_RESULT_EVENT>& result, std::unique_ptr<IRegExp, decltype(&ComRelease)>& regExp) const
{
	if( key == NULL ){
		return ;
	}
	
	if( key->andKey.compare(0, 7, L"^!{999}") == 0 ){
		//無効を示すキーワードが指定されているので検索しない
		return ;
	}
	wstring andKey = key->andKey;
	bool caseFlag = false;
	if( andKey.compare(0, 7, L"C!{999}") == 0 ){
		//大小文字を区別するキーワードが指定されている
		andKey.erase(0, 7);
		caseFlag = true;
	}
	DWORD chkDurationMinSec = 0;
	DWORD chkDurationMaxSec = MAXDWORD;
	if( andKey.compare(0, 4, L"D!{1") == 0 ){
		LPWSTR endp;
		DWORD dur = wcstoul(andKey.c_str() + 3, &endp, 10);
		if( endp - andKey.c_str() == 12 && endp[0] == L'}' ){
			//番組長を絞り込むキーワードが指定されている
			andKey.erase(0, 13);
			chkDurationMinSec = dur / 10000 % 10000 * 60;
			chkDurationMaxSec = dur % 10000 == 0 ? MAXDWORD : dur % 10000 * 60;
		}
	}
	if( andKey.size() == 0 && key->notKey.size() == 0 && key->contentList.size() == 0 && key->videoList.size() == 0 && key->audioList.size() == 0){
		//キーワードもジャンル指定もないので検索しない
		if( g_compatFlags & 0x02 ){
			//互換動作: キーワードなしの検索を許可する
		}else{
			return;
		}
	}
	
	//キーワード分解
	vector<wstring> andKeyList;
	vector<wstring> notKeyList;

	if( key->regExpFlag == FALSE ){
		//正規表現ではないのでキーワードの分解
		wstring buff = L"";
		if( andKey.size() > 0 ){
			wstring andBuff = andKey;
			Replace(andBuff, L"　", L" ");
			do{
				Separate(andBuff, L" ", buff, andBuff);
				ConvertSearchText(buff);
				if( buff.size() > 0 ){
					andKeyList.push_back(buff);
				}
			}while( andBuff.size() != 0 );
		}
		
		if( key->notKey.size() > 0 ){
			wstring notBuff = key->notKey;
			Replace(notBuff, L"　", L" ");
			do{
				Separate(notBuff, L" ", buff, notBuff);
				ConvertSearchText(buff);
				if( buff.size() > 0 ){
					notKeyList.push_back(buff);
				}
			}while( notBuff.size() != 0 );
		}
	}else{
		if( andKey.size() > 0 ){
			andKeyList.push_back(andKey);
			//旧い処理では対象を全角空白のまま比較していたため正規表現も全角のケースが多い。特別に置き換える
			Replace(andKeyList.back(), L"　", L" ");
		}
		if( key->notKey.size() > 0 ){
			notKeyList.push_back(key->notKey);
			Replace(notKeyList.back(), L"　", L" ");
		}
	}

	size_t resultSize = result.size();
	auto compareResult = [](const SEARCH_RESULT_EVENT& a, const SEARCH_RESULT_EVENT& b) -> bool {
		return Create64PgKey(a.info->original_network_id, a.info->transport_stream_id, a.info->service_id, a.info->event_id) <
		       Create64PgKey(b.info->original_network_id, b.info->transport_stream_id, b.info->service_id, b.info->event_id);
	};
	wstring targetWord;
	vector<int> distForFind;
	
	//サービスごとに検索
	for( size_t i=0; i<key->serviceList.size(); i++ ){
		auto itrService = this->epgMap.find(key->serviceList[i]);
		if( itrService != this->epgMap.end() ){
			//サービス発見
			for( auto itrEvent = itrService->second.eventList.cbegin(); itrEvent != itrService->second.eventList.end(); itrEvent++ ){
				wstring matchKey;
				if( key->freeCAFlag == 1 ){
					//無料放送のみ
					if( itrEvent->freeCAFlag != 0 ){
						//有料放送
						continue;
					}
				}else if( key->freeCAFlag == 2 ){
					//有料放送のみ
					if( itrEvent->freeCAFlag == 0 ){
						//無料放送
						continue;
					}
				}
				//ジャンル確認
				if( key->contentList.size() > 0 ){
					//ジャンル指定あるのでジャンルで絞り込み
					if( itrEvent->contentInfo == NULL ){
						if( itrEvent->shortInfo == NULL ){
							//2つめのサービス？対象外とする
							continue;
						}
						//ジャンル情報ない
						bool findNo = false;
						for( size_t j=0; j<key->contentList.size(); j++ ){
							if( key->contentList[j].content_nibble_level_1 == 0xFF && 
								key->contentList[j].content_nibble_level_2 == 0xFF
								){
									//ジャンルなしの指定あり
									findNo = true;
									break;
							}
						}
						if( key->notContetFlag == 0 ){
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
						bool equal = IsEqualContent(key->contentList, itrEvent->contentInfo->nibbleList);
						if( key->notContetFlag == 0 ){
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
				if( key->videoList.size() > 0 ){
					if( itrEvent->componentInfo == NULL ){
						continue;
					}
					WORD type = itrEvent->componentInfo->stream_content << 8 || itrEvent->componentInfo->component_type;
					if( std::find(key->videoList.begin(), key->videoList.end(), type) == key->videoList.end() ){
						continue;
					}
				}

				//音声確認
				if( key->audioList.size() > 0 ){
					if( itrEvent->audioInfo == NULL ){
						continue;
					}
					bool findContent = false;
					for( size_t j=0; j<itrEvent->audioInfo->componentList.size(); j++ ){
						WORD type = itrEvent->audioInfo->componentList[j].stream_content << 8 | itrEvent->audioInfo->componentList[j].component_type;
						if( std::find(key->audioList.begin(), key->audioList.end(), type) != key->audioList.end() ){
							findContent = true;
							break;
						}
					}
					if( findContent == false ){
						continue;
					}
				}

				//時間確認
				if( key->dateList.size() > 0 ){
					if( itrEvent->StartTimeFlag == FALSE ){
						//開始時間不明なので対象外
						continue;
					}
					bool inTime = IsInDateTime(key->dateList, itrEvent->start_time);
					if( key->notDateFlag == 0 ){
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
					if( 0 < chkDurationMinSec || chkDurationMaxSec < MAXDWORD ){
						continue;
					}
				}else{
					if( itrEvent->durationSec < chkDurationMinSec || chkDurationMaxSec < itrEvent->durationSec ){
						continue;
					}
				}

				//キーワード確認
				if( itrEvent->shortInfo == NULL ){
					if( andKeyList.size() != 0 ){
						//内容にかかわらず対象外
						continue;
					}
				}else if( andKeyList.size() != 0 || notKeyList.size() != 0 ){
					//検索対象の文字列作成
					targetWord = itrEvent->shortInfo->event_name;
					if( key->titleOnlyFlag == FALSE ){
						targetWord += L"\r\n";
						targetWord += itrEvent->shortInfo->text_char;
						if( itrEvent->extInfo != NULL ){
							targetWord += L"\r\n";
							targetWord += itrEvent->extInfo->text_char;
						}
					}
					ConvertSearchText(targetWord);

					if( notKeyList.size() != 0 ){
						if( IsFindKeyword(key->regExpFlag != FALSE, regExp, caseFlag, notKeyList, targetWord, false) ){
							//notキーワード見つかったので対象外
							continue;
						}
					}
					if( andKeyList.size() != 0 ){
						if( key->regExpFlag == FALSE && key->aimaiFlag != 0 ){
							//あいまい検索
							if( IsFindLikeKeyword(caseFlag, andKeyList, targetWord, distForFind, &matchKey) == false ){
								//andキーワード見つからなかったので対象外
								continue;
							}
						}else{
							if( IsFindKeyword(key->regExpFlag != FALSE, regExp, caseFlag, andKeyList, targetWord, true, &matchKey) == false ){
								//andキーワード見つからなかったので対象外
								continue;
							}
						}
					}
				}

				SEARCH_RESULT_EVENT addItem;
				addItem.findKey = matchKey;
				addItem.info = &(*itrEvent);
				//resultSizeまで(既ソート)に存在しないときだけ追加
				vector<SEARCH_RESULT_EVENT>::iterator itrResult = std::lower_bound(result.begin(), result.begin() + resultSize, addItem, compareResult);
				if( itrResult == result.begin() + resultSize || compareResult(addItem, *itrResult) ){
					result.push_back(addItem);
				}

			}
		}
	}
	//全体をソートして重複削除
	std::sort(result.begin(), result.end(), compareResult);
	result.erase(std::unique(result.begin(), result.end(), [](const SEARCH_RESULT_EVENT& a, const SEARCH_RESULT_EVENT& b) {
		return a.info->original_network_id == b.info->original_network_id &&
		       a.info->transport_stream_id == b.info->transport_stream_id &&
		       a.info->service_id == b.info->service_id &&
		       a.info->event_id == b.info->event_id;
	}), result.end());
}

bool CEpgDBManager::IsEqualContent(const vector<EPGDB_CONTENT_DATA>& searchKey, const vector<EPGDB_CONTENT_DATA>& eventData)
{
	for( size_t i=0; i<searchKey.size(); i++ ){
		EPGDB_CONTENT_DATA c = searchKey[i];
		if( (c.content_nibble_level_1 & 0xF0) == 0x70 ){
			//CS拡張用情報に変換する
			c.user_nibble_1 = c.content_nibble_level_1 & 0x0F;
			c.user_nibble_2 = c.content_nibble_level_2;
			c.content_nibble_level_1 = 0x0E;
			c.content_nibble_level_2 = 0x01;
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

static wstring::const_iterator SearchKeyword(const wstring& str, const wstring& key, bool caseFlag)
{
	return caseFlag ?
		std::search(str.begin(), str.end(), key.begin(), key.end()) :
		std::search(str.begin(), str.end(), key.begin(), key.end(),
			[](wchar_t l, wchar_t r) { return (L'a' <= l && l <= L'z' ? l - L'a' + L'A' : l) == (L'a' <= r && r <= L'z' ? r - L'a' + L'A' : r); });
}

bool CEpgDBManager::IsFindKeyword(bool regExpFlag, std::unique_ptr<IRegExp, decltype(&ComRelease)>& regExp,
                                  bool caseFlag, const vector<wstring>& keyList, const wstring& word, bool andMode, wstring* findKey)
{
	if( regExpFlag ){
		//正規表現モード
		if( !regExp ){
			void* pv;
			if( SUCCEEDED(CoCreateInstance(CLSID_RegExp, NULL, CLSCTX_INPROC_SERVER, IID_IRegExp, &pv)) ){
				regExp.reset((IRegExp*)pv);
			}
		}
		if( regExp && word.size() > 0 && keyList.size() > 0 ){
			typedef std::unique_ptr<OLECHAR, decltype(&SysFreeString)> OleCharPtr;
			OleCharPtr pattern(SysAllocString(keyList[0].c_str()), SysFreeString);
			OleCharPtr target(SysAllocString(word.c_str()), SysFreeString);
			if( pattern && target ){
				IDispatch* pMatches;
				if( SUCCEEDED(regExp->put_Global(VARIANT_TRUE)) &&
				    SUCCEEDED(regExp->put_IgnoreCase(caseFlag ? VARIANT_FALSE : VARIANT_TRUE)) &&
				    SUCCEEDED(regExp->put_Pattern(pattern.get())) &&
				    SUCCEEDED(regExp->Execute(target.get(), &pMatches)) ){
					std::unique_ptr<IMatchCollection, decltype(&ComRelease)> matches((IMatchCollection*)pMatches, ComRelease);
					long count;
					if( SUCCEEDED(matches->get_Count(&count)) && count > 0 ){
						if( findKey != NULL ){
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
						return true;
					}
				}
			}
		}
		return false;
	}else{
		//通常
		if( andMode ){
			for( size_t i=0; i<keyList.size(); i++ ){
				if( SearchKeyword(word, keyList[i], caseFlag) == word.end() ){
					//見つからなかったので終了
					return false;
				}
			}
			if( findKey != NULL ){
				for( size_t i=0; i<keyList.size(); i++ ){
					if( findKey->size() > 0 ){
						*findKey += L' ';
					}
					*findKey += keyList[i];
				}
			}
			return true;
		}else{
			for( size_t i=0; i<keyList.size(); i++ ){
				if( SearchKeyword(word, keyList[i], caseFlag) != word.end() ){
					//見つかったので終了
					return true;
				}
			}
			return false;
		}
	}
}

bool CEpgDBManager::IsFindLikeKeyword(bool caseFlag, const vector<wstring>& keyList, const wstring& word, vector<int>& dist, wstring* findKey)
{
	for( vector<wstring>::const_iterator itr = keyList.begin(); itr != keyList.end(); itr++ ){
		//編集距離がしきい値以下になる文字列が含まれるか調べる
		size_t l = 0;
		size_t curr = itr->size() + 1;
		dist.assign(curr * 2, 0);
		for( size_t i = 1; i < curr; i++ ){
			dist[i] = dist[i - 1] + 1;
		}
		bool matched = false;
		for( size_t i = 0; i < word.size(); i++ ){
			wchar_t x = word[i];
			for( size_t j = 0; j < itr->size(); j++ ){
				wchar_t y = (*itr)[j];
				if( caseFlag && x == y ||
				    caseFlag == false && (L'a' <= x && x <= L'z' ? x - L'a' + L'A' : x) == (L'a' <= y && y <= L'z' ? y - L'a' + L'A' : y) ){
					dist[curr + j + 1] = dist[l + j];
				}else{
					dist[curr + j + 1] = 1 + (dist[l + j] < dist[l + j + 1] ? min(dist[l + j], dist[curr + j]) : min(dist[l + j + 1], dist[curr + j]));
				}
			}
			//75%をしきい値とする
			if( dist[curr + itr->size()] * 4 <= (int)itr->size() ){
				matched = true;
				break;
			}
			std::swap(l, curr);
		}
		if( matched == false ){
			return false;
		}
	}
	if( findKey != NULL ){
		for( size_t i = 0; i < keyList.size(); i++ ){
			if( findKey->size() > 0 ){
				*findKey += L' ';
			}
			*findKey += keyList[i];
		}
	}
	return true;
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
			result->DeepCopy(*itrInfo);
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
						result->DeepCopy(*itrInfo);
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
	static const wchar_t convertFrom[][3] = {
		L"’", L"”", L"　",
		L"！", L"＃", L"＄", L"％", L"＆", L"（", L"）", L"＊", L"＋", L"，", L"－", L"．", L"／",
		L"：", L"；", L"＜", L"＝", L"＞", L"？", L"＠", L"［", L"］", L"＾", L"＿", L"｀", L"｛", L"｜", L"｝", L"～",
		L"｡", L"｢", L"｣", L"､", L"･", L"ｦ", L"ｧ", L"ｨ", L"ｩ", L"ｪ", L"ｫ", L"ｬ", L"ｭ", L"ｮ", L"ｯ", L"ｰ", L"ｱ", L"ｲ", L"ｳ", L"ｴ", L"ｵ",
		L"ｶﾞ", L"ｶ", L"ｷﾞ", L"ｷ", L"ｸﾞ", L"ｸ", L"ｹﾞ", L"ｹ", L"ｺﾞ", L"ｺ",
		L"ｻﾞ", L"ｻ", L"ｼﾞ", L"ｼ", L"ｽﾞ", L"ｽ", L"ｾﾞ", L"ｾ", L"ｿﾞ", L"ｿ",
		L"ﾀﾞ", L"ﾀ", L"ﾁﾞ", L"ﾁ", L"ﾂﾞ", L"ﾂ", L"ﾃﾞ", L"ﾃ", L"ﾄﾞ", L"ﾄ",
		L"ﾅ", L"ﾆ", L"ﾇ", L"ﾈ", L"ﾉ",
		L"ﾊﾞ", L"ﾊﾟ", L"ﾊ", L"ﾋﾞ", L"ﾋﾟ", L"ﾋ", L"ﾌﾞ", L"ﾌﾟ", L"ﾌ", L"ﾍﾞ", L"ﾍﾟ", L"ﾍ", L"ﾎﾞ", L"ﾎﾟ", L"ﾎ",
		L"ﾏ", L"ﾐ", L"ﾑ", L"ﾒ", L"ﾓ", L"ﾔ", L"ﾕ", L"ﾖ", L"ﾗ", L"ﾘ", L"ﾙ", L"ﾚ", L"ﾛ", L"ﾜ", L"ﾝ", L"ﾞ", L"ﾟ",
		L"￥",
	};
	static const wchar_t convertTo[][2] = {
		L"'", L"\"", L" ",
		L"!", L"#", L"$", L"%", L"&", L"(", L")", L"*", L"+", L",", L"-", L".", L"/",
		L":", L";", L"<", L"=", L">", L"?", L"@", L"[", L"]", L"^", L"_", L"`", L"{", L"|", L"}", L"~",
		L"。", L"「", L"」", L"、", L"・", L"ヲ", L"ァ", L"ィ", L"ゥ", L"ェ", L"ォ", L"ャ", L"ュ", L"ョ", L"ッ", L"ー", L"ア", L"イ", L"ウ", L"エ", L"オ",
		L"ガ", L"カ", L"ギ", L"キ", L"グ", L"ク", L"ゲ", L"ケ", L"ゴ", L"コ",
		L"ザ", L"サ", L"ジ", L"シ", L"ズ", L"ス", L"ゼ", L"セ", L"ゾ", L"ソ",
		L"ダ", L"タ", L"ヂ", L"チ", L"ヅ", L"ツ", L"デ", L"テ", L"ド", L"ト",
		L"ナ", L"ニ", L"ヌ", L"ネ", L"ノ",
		L"バ", L"パ", L"ハ", L"ビ", L"ピ", L"ヒ", L"ブ", L"プ", L"フ", L"ベ", L"ペ", L"ヘ", L"ボ", L"ポ", L"ホ",
		L"マ", L"ミ", L"ム", L"メ", L"モ", L"ヤ", L"ユ", L"ヨ", L"ラ", L"リ", L"ル", L"レ", L"ロ", L"ワ", L"ン", L"゛", L"゜",
		L"\\",
	};

	for( size_t i = 0; i < str.size(); i++ ){
		if( L'０' <= str[i] && str[i] <= L'９' ){
			str[i] = str[i] - L'０' + L'0';
		}else if( L'Ａ' <= str[i] && str[i] <= L'Ｚ' ){
			str[i] = str[i] - L'Ａ' + L'A';
		}else if( L'ａ' <= str[i] && str[i] <= L'ｚ' ){
			str[i] = str[i] - L'ａ' + L'a';
		}
		//注意: これは符号位置の連続性を利用してテーブル参照を減らすための条件。上記のテーブルを弄る場合はここを確認すること
		else if( str[i] == L'’' || str[i] == L'”' || str[i] == L'　' || L'！' <= str[i] && str[i] <= L'￥' ){
			const wchar_t (*f)[3] = std::lower_bound(convertFrom, convertFrom + _countof(convertFrom), &str[i],
			                                         [](const wchar_t* a, const wchar_t* b) { return (unsigned int)a[0] < (unsigned int)b[0]; });
			for( ; f != convertFrom + _countof(convertFrom) && (*f)[0] == str[i]; f++ ){
				if( (*f)[1] == L'\0' ){
					str.replace(i, 1, convertTo[f - convertFrom]);
					break;
				}else if( i + 1 < str.size() && str[i + 1] == (*f)[1] ){
					str.replace(i, 2, convertTo[f - convertFrom]);
					break;
				}
			}
		}
	}
}

