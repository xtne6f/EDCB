#include "StdAfx.h"
#include "EpgDBManager.h"
#include <process.h>

#include "../../Common/CommonDef.h"
#include "../../Common/TimeUtil.h"
#include "../../Common/StringUtil.h"
#include "../../Common/PathUtil.h"
#include "../../Common/EpgTimerUtil.h"
#include "../../Common/EpgDataCap3Util.h"

extern DWORD g_compatFlags;

CEpgDBManager::CEpgDBManager(void)
{
	InitializeCriticalSection(&this->epgMapLock);

    this->loadThread = NULL;
    this->loadStop = FALSE;
    this->initialLoadDone = FALSE;
}

CEpgDBManager::~CEpgDBManager(void)
{
	CancelLoadData();

	ClearEpgData();

	DeleteCriticalSection(&this->epgMapLock);
}

void CEpgDBManager::ClearEpgData()
{
	CBlockLock lock(&this->epgMapLock);
	this->epgMap.clear();
}

BOOL CEpgDBManager::ReloadEpgData()
{
	CancelLoadData();

	CBlockLock lock(&this->epgMapLock);

	BOOL ret = TRUE;
	if( this->loadThread == NULL ){
		//受信スレッド起動
		this->loadThread = (HANDLE)_beginthreadex(NULL, 0, LoadThread, (LPVOID)this, CREATE_SUSPENDED, NULL);
		SetThreadPriority( this->loadThread, THREAD_PRIORITY_NORMAL );
		ResumeThread(this->loadThread);
	}else{
		ret = FALSE;
	}

	return ret;
}

UINT WINAPI CEpgDBManager::LoadThread(LPVOID param)
{
	CEpgDBManager* sys = (CEpgDBManager*)param;

	OutputDebugString(L"Start Load EpgData\r\n");
	DWORD time = GetTickCount();

	CEpgDataCap3Util epgUtil;
	if( epgUtil.Initialize(FALSE) == FALSE ){
		OutputDebugString(L"★EpgDataCap3.dllの初期化に失敗しました。\r\n");
		sys->ClearEpgData();
		return 0;
	}

	//EPGファイルの検索
	vector<wstring> epgFileList;
	wstring epgDataPath = L"";
	GetSettingPath(epgDataPath);
	epgDataPath += EPG_SAVE_FOLDER;

	wstring searchKey = epgDataPath;
	searchKey += L"\\*_epg.dat";

	WIN32_FIND_DATA findData;
	HANDLE find;

	//指定フォルダのファイル一覧取得
	find = FindFirstFile( searchKey.c_str(), &findData);
	if ( find == INVALID_HANDLE_VALUE ) {
		//１つも存在しない
		epgUtil.UnInitialize();
		sys->ClearEpgData();
		return 0;
	}
	do{
		if( (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 ){
			LONGLONG fileTime = (LONGLONG)findData.ftLastWriteTime.dwHighDateTime << 32 | findData.ftLastWriteTime.dwLowDateTime;
			if( fileTime != 0 ){
				//見つかったファイルを一覧に追加
				//名前順。ただしTSID==0xFFFFの場合は同じチャンネルの連続によりストリームがクリアされない可能性があるので後ろにまとめる
				WCHAR prefix = fileTime + 7*24*60*60*I64_1SEC < GetNowI64Time() ? L'0' :
				               lstrlen(findData.cFileName) < 12 || _wcsicmp(findData.cFileName + lstrlen(findData.cFileName) - 12, L"ffff_epg.dat") ? L'1' : L'2';
				wstring item = prefix + epgDataPath + L'\\' + findData.cFileName;
				epgFileList.insert(std::lower_bound(epgFileList.begin(), epgFileList.end(), item), item);
			}
		}
	}while(FindNextFile(find, &findData));

	FindClose(find);

	//EPGファイルの解析
	for( vector<wstring>::iterator itr = epgFileList.begin(); itr != epgFileList.end(); itr++ ){
		if( sys->loadStop ){
			//キャンセルされた
			epgUtil.UnInitialize();
			return 0;
		}
		//一時ファイルの状態を調べる。取得側のCreateFile(tmp)→CloseHandle(tmp)→CopyFile(tmp,master)→DeleteFile(tmp)の流れをある程度仮定
		wstring path = itr->c_str() + 1;
		HANDLE tmpFile = CreateFile((path + L".tmp").c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		DWORD tmpError = GetLastError();
		if( tmpFile != INVALID_HANDLE_VALUE ){
			tmpError = NO_ERROR;
			FILETIME ft;
			if( GetFileTime(tmpFile, NULL, NULL, &ft) == FALSE || ((LONGLONG)ft.dwHighDateTime << 32 | ft.dwLowDateTime) + 300*I64_1SEC < GetNowI64Time() ){
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
			BOOL swapped = FALSE;
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
						swapped = TRUE;
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
				BOOL ignoreTOT = FALSE;
				while( ReadFile(file, readBuff, 188, &read, NULL) && read == 188 ){
					if( ((readBuff[1] & 0x1F) << 8 | readBuff[2]) == 0x14 ){
						ignoreTOT = TRUE;
						epgUtil.AddTSPacket(readBuff, 188);
						break;
					}
				}
				SetFilePointer(file, seekPos, 0, FILE_BEGIN);
				while( ReadFile(file, readBuff, sizeof(readBuff), &read, NULL) && read != 0 ){
					for( DWORD i=0; i<read; i+=188 ){
						if( ignoreTOT && ((readBuff[i+1] & 0x1F) << 8 | readBuff[i+2]) == 0x14 ){
							ignoreTOT = FALSE;
						}else{
							epgUtil.AddTSPacket(readBuff+i, 188);
						}
					}
					Sleep(0);
				}
				CloseHandle(file);
			}
			if( swapped ){
				DeleteFile((path + L".swp").c_str());
			}
		}
		Sleep(0);
	}

	//EPGデータを取得
	DWORD serviceListSize = 0;
	SERVICE_INFO* serviceList = NULL;
	if( epgUtil.GetServiceListEpgDB(&serviceListSize, &serviceList) == FALSE ){
		epgUtil.UnInitialize();
		sys->ClearEpgData();
		return 0;
	}

	{ //CBlockLock
	CBlockLock lock(&sys->epgMapLock);

	sys->ClearEpgData();

	for( DWORD i=0; i<serviceListSize; i++ ){
		LONGLONG key = _Create64Key(serviceList[i].original_network_id, serviceList[i].transport_stream_id, serviceList[i].service_id);
		EPGDB_SERVICE_EVENT_INFO* item = &sys->epgMap.insert(std::make_pair(key, EPGDB_SERVICE_EVENT_INFO())).first->second;
		item->serviceInfo.ONID = serviceList[i].original_network_id;
		item->serviceInfo.TSID = serviceList[i].transport_stream_id;
		item->serviceInfo.SID = serviceList[i].service_id;
		if( serviceList[i].extInfo != NULL ){
			item->serviceInfo.service_type = serviceList[i].extInfo->service_type;
			item->serviceInfo.partialReceptionFlag = serviceList[i].extInfo->partialReceptionFlag;
			if( serviceList[i].extInfo->service_provider_name != NULL ){
				item->serviceInfo.service_provider_name = serviceList[i].extInfo->service_provider_name;
			}
			if( serviceList[i].extInfo->service_name != NULL ){
				item->serviceInfo.service_name = serviceList[i].extInfo->service_name;
			}
			if( serviceList[i].extInfo->network_name != NULL ){
				item->serviceInfo.network_name = serviceList[i].extInfo->network_name;
			}
			if( serviceList[i].extInfo->ts_name != NULL ){
				item->serviceInfo.ts_name = serviceList[i].extInfo->ts_name;
			}
			item->serviceInfo.remote_control_key_id = serviceList[i].extInfo->remote_control_key_id;
		}
		epgUtil.EnumEpgInfoList(item->serviceInfo.ONID, item->serviceInfo.TSID, item->serviceInfo.SID, EnumEpgInfoListProc, item);
	}

	} //CBlockLock

	_OutputDebugString(L"End Load EpgData %dmsec\r\n", GetTickCount()-time);
	epgUtil.UnInitialize();

	return 0;
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

BOOL CEpgDBManager::IsLoadingData()
{
	CBlockLock lock(&this->epgMapLock);
	return this->loadThread != NULL && WaitForSingleObject( this->loadThread, 0 ) == WAIT_TIMEOUT ? TRUE : FALSE;
}

BOOL CEpgDBManager::IsInitialLoadingDataDone()
{
	CBlockLock lock(&this->epgMapLock);
	return this->initialLoadDone != FALSE || this->loadThread != NULL && IsLoadingData() == FALSE ? TRUE : FALSE;
}

BOOL CEpgDBManager::CancelLoadData()
{
	for( int i = 0; i < 150; i++ ){
		{
			CBlockLock lock(&this->epgMapLock);
			if( this->loadThread == NULL ){
				return TRUE;
			}else if( i == 0 ){
				this->loadStop = TRUE;
			}else if( this->loadStop == FALSE ){
				return TRUE;
			}else if( IsLoadingData() == FALSE ){
				CloseHandle(this->loadThread);
				this->loadThread = NULL;
				this->loadStop = FALSE;
				this->initialLoadDone = TRUE;
				return TRUE;
			}
		}
		Sleep(100);
	}
	CBlockLock lock(&this->epgMapLock);
	if( this->loadStop != FALSE && IsLoadingData() != FALSE ){
		TerminateThread(this->loadThread, 0xffffffff);
		CloseHandle(this->loadThread);
		this->loadThread = NULL;
		this->loadStop = FALSE;
		this->initialLoadDone = TRUE;
	}

	return TRUE;
}

BOOL CEpgDBManager::SearchEpg(vector<EPGDB_SEARCH_KEY_INFO>* key, vector<SEARCH_RESULT_EVENT_DATA>* result)
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

void CEpgDBManager::SearchEvent(EPGDB_SEARCH_KEY_INFO* key, vector<SEARCH_RESULT_EVENT>& result, IRegExpPtr& regExp)
{
	if( key == NULL ){
		return ;
	}
	
	if( key->andKey.compare(0, 7, L"^!{999}") == 0 ){
		//無効を示すキーワードが指定されているので検索しない
		return ;
	}
	wstring andKey = key->andKey;
	BOOL caseFlag = FALSE;
	if( andKey.compare(0, 7, L"C!{999}") == 0 ){
		//大小文字を区別するキーワードが指定されている
		andKey.erase(0, 7);
		caseFlag = TRUE;
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
		return _Create64Key2(a.info->original_network_id, a.info->transport_stream_id, a.info->service_id, a.info->event_id) <
		       _Create64Key2(b.info->original_network_id, b.info->transport_stream_id, b.info->service_id, b.info->event_id);
	};
	wstring targetWord;
	
	//サービスごとに検索
	for( size_t i=0; i<key->serviceList.size(); i++ ){
		map<LONGLONG, EPGDB_SERVICE_EVENT_INFO>::iterator itrService;
		itrService = this->epgMap.find(key->serviceList[i]);
		if( itrService != this->epgMap.end() ){
			//サービス発見
			vector<EPGDB_EVENT_INFO>::iterator itrEvent_;
			for( itrEvent_ = itrService->second.eventList.begin(); itrEvent_ != itrService->second.eventList.end(); itrEvent_++ ){
				pair<WORD, EPGDB_EVENT_INFO*> autoEvent(std::make_pair(itrEvent_->event_id, &*itrEvent_));
				pair<WORD, EPGDB_EVENT_INFO*>* itrEvent = &autoEvent;
				wstring matchKey = L"";
				if( key->freeCAFlag == 1 ){
					//無料放送のみ
					if(itrEvent->second->freeCAFlag == 1 ){
						//有料放送
						continue;
					}
				}else if( key->freeCAFlag == 2 ){
					//有料放送のみ
					if(itrEvent->second->freeCAFlag == 0 ){
						//無料放送
						continue;
					}
				}
				//ジャンル確認
				if( key->contentList.size() > 0 ){
					//ジャンル指定あるのでジャンルで絞り込み
					if( itrEvent->second->contentInfo == NULL ){
						if( itrEvent->second->shortInfo == NULL ){
							//2つめのサービス？対象外とする
							continue;
						}
						//ジャンル情報ない
						BOOL findNo = FALSE;
						for( size_t j=0; j<key->contentList.size(); j++ ){
							if( key->contentList[j].content_nibble_level_1 == 0xFF && 
								key->contentList[j].content_nibble_level_2 == 0xFF
								){
									//ジャンルなしの指定あり
									findNo = TRUE;
									break;
							}
						}
						if( key->notContetFlag == 0 ){
							if( findNo == FALSE ){
								continue;
							}
						}else{
							//NOT条件扱い
							if( findNo == TRUE ){
								continue;
							}
						}
					}else{
						BOOL equal = IsEqualContent(&(key->contentList), &(itrEvent->second->contentInfo->nibbleList));
						if( key->notContetFlag == 0 ){
							if( equal == FALSE ){
								//ジャンル違うので対象外
								continue;
							}
						}else{
							//NOT条件扱い
							if( equal == TRUE ){
								continue;
							}
						}
					}
				}

				//映像確認
				if( key->videoList.size() > 0 ){
					if( itrEvent->second->componentInfo == NULL ){
						continue;
					}
					BOOL findContent = FALSE;
					WORD type = ((WORD)itrEvent->second->componentInfo->stream_content) << 8 | itrEvent->second->componentInfo->component_type;
					for( size_t j=0; j<key->videoList.size(); j++ ){
						if( type == key->videoList[j]){
							findContent = TRUE;
							break;
						}
					}
					if( findContent == FALSE ){
						continue;
					}
				}

				//音声確認
				if( key->audioList.size() > 0 ){
					if( itrEvent->second->audioInfo == NULL ){
						continue;
					}
					BOOL findContent = FALSE;
					for( size_t j=0; j<itrEvent->second->audioInfo->componentList.size(); j++){
						WORD type = ((WORD)itrEvent->second->audioInfo->componentList[j].stream_content) << 8 | itrEvent->second->audioInfo->componentList[j].component_type;
						for( size_t k=0; k<key->audioList.size(); k++ ){
							if( type == key->audioList[k]){
								findContent = TRUE;
								break;
							}
						}
					}
					if( findContent == FALSE ){
						continue;
					}
				}

				//時間確認
				if( key->dateList.size() > 0 ){
					if( itrEvent->second->StartTimeFlag == FALSE ){
						//開始時間不明なので対象外
						continue;
					}
					BOOL inTime = IsInDateTime(key->dateList, itrEvent->second->start_time);
					if( key->notDateFlag == 0 ){
						if( inTime == FALSE ){
							//時間範囲外なので対象外
							continue;
						}
					}else{
						//NOT条件扱い
						if( inTime == TRUE ){
							continue;
						}
					}
				}

				//番組長で絞り込み
				if( itrEvent->second->DurationFlag == FALSE ){
					//不明なので絞り込みされていれば対象外
					if( 0 < chkDurationMinSec || chkDurationMaxSec < MAXDWORD ){
						continue;
					}
				}else{
					if( itrEvent->second->durationSec < chkDurationMinSec || chkDurationMaxSec < itrEvent->second->durationSec ){
						continue;
					}
				}

				//キーワード確認
				if( itrEvent->second->shortInfo == NULL ){
					if( andKeyList.size() != 0 ){
						//内容にかかわらず対象外
						continue;
					}
				}else if( andKeyList.size() != 0 || notKeyList.size() != 0 ){
					//検索対象の文字列作成
					targetWord = itrEvent->second->shortInfo->event_name;
					if( key->titleOnlyFlag == FALSE ){
						targetWord += L"\r\n";
						targetWord += itrEvent->second->shortInfo->text_char;
						if( itrEvent->second->extInfo != NULL ){
							targetWord += L"\r\n";
							targetWord += itrEvent->second->extInfo->text_char;
						}
					}
					ConvertSearchText(targetWord);

					if( notKeyList.size() != 0 ){
						if( IsFindKeyword(key->regExpFlag, regExp, caseFlag, &notKeyList, targetWord, FALSE) != FALSE ){
							//notキーワード見つかったので対象外
							continue;
						}
					}
					if( andKeyList.size() != 0 ){
						if( key->regExpFlag == FALSE && key->aimaiFlag == 1 ){
							//あいまい検索
							if( IsFindLikeKeyword(caseFlag, &andKeyList, targetWord, TRUE, &matchKey) == FALSE ){
								//andキーワード見つからなかったので対象外
								continue;
							}
						}else{
							if( IsFindKeyword(key->regExpFlag, regExp, caseFlag, &andKeyList, targetWord, TRUE, &matchKey) == FALSE ){
								//andキーワード見つからなかったので対象外
								continue;
							}
						}
					}
				}

				SEARCH_RESULT_EVENT addItem;
				addItem.findKey = matchKey;
				addItem.info = itrEvent->second;
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

BOOL CEpgDBManager::IsEqualContent(vector<EPGDB_CONTENT_DATA>* searchKey, vector<EPGDB_CONTENT_DATA>* eventData)
{
	for( size_t i=0; i<searchKey->size(); i++ ){
		EPGDB_CONTENT_DATA c = (*searchKey)[i];
		if( (c.content_nibble_level_1 & 0xF0) == 0x70 ){
			//CS拡張用情報に変換する
			c.user_nibble_1 = c.content_nibble_level_1 & 0x0F;
			c.user_nibble_2 = c.content_nibble_level_2;
			c.content_nibble_level_1 = 0x0E;
			c.content_nibble_level_2 = 0x01;
		}
		for( size_t j=0; j<eventData->size(); j++ ){
			if( c.content_nibble_level_1 == (*eventData)[j].content_nibble_level_1 ){
				if( c.content_nibble_level_2 == 0xFF ){
					//中分類すべて
					return TRUE;
				}
				if( c.content_nibble_level_2 == (*eventData)[j].content_nibble_level_2 ){
					if( c.content_nibble_level_1 != 0x0E ){
						//拡張でない
						return TRUE;
					}
					if( c.user_nibble_1 == (*eventData)[j].user_nibble_1 ){
						if( c.user_nibble_2 == 0xFF ){
							//拡張中分類すべて
							return TRUE;
						}
						if( c.user_nibble_2 == (*eventData)[j].user_nibble_2 ){
							return TRUE;
						}
					}
				}
			}
		}
	}
	return FALSE;
}

BOOL CEpgDBManager::IsInDateTime(const vector<EPGDB_SEARCH_DATE_INFO>& dateList, const SYSTEMTIME& time)
{
	int weekMin = (time.wDayOfWeek * 24 + time.wHour) * 60 + time.wMinute;
	for( size_t i=0; i<dateList.size(); i++ ){
		int start = (dateList[i].startDayOfWeek * 24 + dateList[i].startHour) * 60 + dateList[i].startMin;
		int end = (dateList[i].endDayOfWeek * 24 + dateList[i].endHour) * 60 + dateList[i].endMin;
		if( start >= end ){
			if( start <= weekMin || weekMin <= end ){
				return TRUE;
			}
		}else{
			if( start <= weekMin && weekMin <= end ){
				return TRUE;
			}
		}
	}

	return FALSE;
}

static wstring::const_iterator SearchKeyword(const wstring& str, const wstring& key, BOOL caseFlag)
{
	return caseFlag ?
		std::search(str.begin(), str.end(), key.begin(), key.end()) :
		std::search(str.begin(), str.end(), key.begin(), key.end(),
			[](wchar_t l, wchar_t r) { return (L'a' <= l && l <= L'z' ? l - L'a' + L'A' : l) == (L'a' <= r && r <= L'z' ? r - L'a' + L'A' : r); });
}

BOOL CEpgDBManager::IsFindKeyword(BOOL regExpFlag, IRegExpPtr& regExp, BOOL caseFlag, const vector<wstring>* keyList, const wstring& word, BOOL andMode, wstring* findKey)
{
	if( regExpFlag == TRUE ){
		//正規表現モード
		try{
			if( regExp == NULL ){
				regExp.CreateInstance(CLSID_RegExp);
			}
			if( regExp != NULL && word.size() > 0 && keyList->size() > 0 ){
				_bstr_t target( word.c_str() );
				_bstr_t pattern( (*keyList)[0].c_str() );

				regExp->PutGlobal( VARIANT_TRUE );
				regExp->PutIgnoreCase( caseFlag == FALSE ? VARIANT_TRUE : VARIANT_FALSE );
				regExp->PutPattern( pattern );

				IMatchCollectionPtr pMatchCol( regExp->Execute( target ) );

				if( pMatchCol->Count > 0 ){
					if( findKey != NULL ){
						IMatch2Ptr pMatch( pMatchCol->Item[0] );
						_bstr_t value( pMatch->Value );

						*findKey = !value ? L"" : value;
					}
					return TRUE;
				}
			}
		}catch( _com_error& ){
			//_OutputDebugString(L"%s\r\n", e.ErrorMessage());
		}
		return FALSE;
	}else{
		//通常
		if( andMode == TRUE ){
			for( size_t i=0; i<keyList->size(); i++ ){
				if( SearchKeyword(word, (*keyList)[i], caseFlag) == word.end() ){
					//見つからなかったので終了
					return FALSE;
				}else{
					if( findKey != NULL ){
						if( findKey->size() > 0 ){
							*findKey += L" ";
						}
						*findKey += (*keyList)[i];
					}
				}
			}
			return TRUE;
		}else{
			for( size_t i=0; i<keyList->size(); i++ ){
				if( SearchKeyword(word, (*keyList)[i], caseFlag) != word.end() ){
					//見つかったので終了
					return TRUE;
				}
			}
			return FALSE;
		}
	}
}

BOOL CEpgDBManager::IsFindLikeKeyword(BOOL caseFlag, const vector<wstring>* keyList, const wstring& word, BOOL andMode, wstring* findKey)
{
	BOOL ret = FALSE;

	DWORD hitCount = 0;
	DWORD missCount = 0;
	for( size_t i=0; i<keyList->size(); i++ ){
		wstring key= L"";
		for( size_t j=0; j<(*keyList)[i].size(); j++ ){
			key += (*keyList)[i].at(j);
			if( SearchKeyword(word, key, caseFlag) == word.end() ){
				missCount+=1;
				key = (*keyList)[i].at(j);
				if( SearchKeyword(word, key, caseFlag) == word.end() ){
					missCount+=1;
					key = L"";
				}else{
					//hitCount+=1;
				}
			}else{
				hitCount+=(DWORD)key.size();
			}
		}
		if( andMode == FALSE ){
			DWORD totalCount = hitCount+missCount;
			DWORD per = (hitCount*100) / totalCount;
			if( per > 70 ){
				ret = TRUE;
				break;
			}
			hitCount = 0;
			missCount = 0;
		}else{
			if( findKey != NULL ){
				*findKey += (*keyList)[i];
			}
		}
	}
	if( andMode == TRUE ){
		DWORD totalCount = hitCount+missCount;
		DWORD per = (hitCount*100) / totalCount;
		if( per > 70 ){
			ret = TRUE;
		}else{
			ret = FALSE;
		}
	}
	return ret;
}

BOOL CEpgDBManager::GetServiceList(vector<EPGDB_SERVICE_INFO>* list)
{
	CBlockLock lock(&this->epgMapLock);

	BOOL ret = TRUE;
	map<LONGLONG, EPGDB_SERVICE_EVENT_INFO>::iterator itr;
	for( itr = this->epgMap.begin(); itr != this->epgMap.end(); itr++ ){
		list->push_back(itr->second.serviceInfo);
	}
	if( list->size() == 0 ){
		ret = FALSE;
	}

	return ret;
}

BOOL CEpgDBManager::SearchEpg(
	WORD ONID,
	WORD TSID,
	WORD SID,
	WORD EventID,
	EPGDB_EVENT_INFO* result
	)
{
	CBlockLock lock(&this->epgMapLock);

	BOOL ret = FALSE;

	LONGLONG key = _Create64Key(ONID, TSID, SID);
	map<LONGLONG, EPGDB_SERVICE_EVENT_INFO>::iterator itr;
	itr = this->epgMap.find(key);
	if( itr != this->epgMap.end() ){
		EPGDB_EVENT_INFO infoKey;
		infoKey.event_id = EventID;
		vector<EPGDB_EVENT_INFO>::iterator itrInfo;
		itrInfo = std::lower_bound(itr->second.eventList.begin(), itr->second.eventList.end(), infoKey,
		                           [](const EPGDB_EVENT_INFO& a, const EPGDB_EVENT_INFO& b) { return a.event_id < b.event_id; });
		if( itrInfo != itr->second.eventList.end() && itrInfo->event_id == EventID ){
			result->DeepCopy(*itrInfo);
			ret = TRUE;
		}
	}

	return ret;
}

BOOL CEpgDBManager::SearchEpg(
	WORD ONID,
	WORD TSID,
	WORD SID,
	LONGLONG startTime,
	DWORD durationSec,
	EPGDB_EVENT_INFO* result
	)
{
	CBlockLock lock(&this->epgMapLock);

	BOOL ret = FALSE;

	LONGLONG key = _Create64Key(ONID, TSID, SID);
	map<LONGLONG, EPGDB_SERVICE_EVENT_INFO>::iterator itr;
	itr = this->epgMap.find(key);
	if( itr != this->epgMap.end() ){
		vector<EPGDB_EVENT_INFO>::iterator itrInfo;
		for( itrInfo = itr->second.eventList.begin(); itrInfo != itr->second.eventList.end(); itrInfo++ ){
			if( itrInfo->StartTimeFlag == 1 && itrInfo->DurationFlag == 1 ){
				if( startTime == ConvertI64Time(itrInfo->start_time) &&
					durationSec == itrInfo->durationSec
					){
						result->DeepCopy(*itrInfo);
						ret = TRUE;
						break;
				}
			}
		}
	}

	return ret;
}

BOOL CEpgDBManager::SearchServiceName(
	WORD ONID,
	WORD TSID,
	WORD SID,
	wstring& serviceName
	)
{
	CBlockLock lock(&this->epgMapLock);

	BOOL ret = FALSE;

	LONGLONG key = _Create64Key(ONID, TSID, SID);
	map<LONGLONG, EPGDB_SERVICE_EVENT_INFO>::iterator itr;
	itr = this->epgMap.find(key);
	if( itr != this->epgMap.end() ){
		serviceName = itr->second.serviceInfo.service_name;
		ret = TRUE;
	}

	return ret;
}

//検索対象や検索パターンから全半角の区別を取り除く(旧ConvertText.txtに相当)
//ConvertText.txtと異なり半角濁点カナを(意図通り)置換する点、［］，．全角空白を置換する点、―(U+2015)ゐヰゑヱΖ(U+0396)を置換しない点に注意
void CEpgDBManager::ConvertSearchText(wstring& str)
{
	//全角英数およびこのテーブルにある文字列を置換する
	static const wchar_t convertFrom[][3] = {
		L"’", L"”", L"　",
		L"！", L"＃", L"＄", L"％", L"＆", L"（", L"）", L"＊", L"＋", L"，", L"－", L"．", L"／",
		L"：", L"；", L"＜", L"＝", L"＞", L"？", L"＠", L"［", L"］", L"＾", L"＿", L"｀", L"｛", L"｜", L"｝", L"～",
		L"｡", L"｢", L"｣", L"､", L"･", L"ｦ", L"ｧ", L"ｨ", L"ｩ", L"ｪ", L"ｫ", L"ｬ", L"ｭ", L"ｮ", L"ｯ", L"ｰ", L"ｱ", L"ｲ", L"ｳ", L"ｴ", L"ｵ",
		L"ｶﾞ", L"ｷﾞ", L"ｸﾞ", L"ｹﾞ", L"ｺﾞ", L"ｶ", L"ｷ", L"ｸ", L"ｹ", L"ｺ",
		L"ｻﾞ", L"ｼﾞ", L"ｽﾞ", L"ｾﾞ", L"ｿﾞ", L"ｻ", L"ｼ", L"ｽ", L"ｾ", L"ｿ",
		L"ﾀﾞ", L"ﾁﾞ", L"ﾂﾞ", L"ﾃﾞ", L"ﾄﾞ", L"ﾀ", L"ﾁ", L"ﾂ", L"ﾃ", L"ﾄ",
		L"ﾅ", L"ﾆ", L"ﾇ", L"ﾈ", L"ﾉ",
		L"ﾊﾞ", L"ﾋﾞ", L"ﾌﾞ", L"ﾍﾞ", L"ﾎﾞ", L"ﾊﾟ", L"ﾋﾟ", L"ﾌﾟ", L"ﾍﾟ", L"ﾎﾟ", L"ﾊ", L"ﾋ", L"ﾌ", L"ﾍ", L"ﾎ",
		L"ﾏ", L"ﾐ", L"ﾑ", L"ﾒ", L"ﾓ", L"ﾔ", L"ﾕ", L"ﾖ", L"ﾗ", L"ﾘ", L"ﾙ", L"ﾚ", L"ﾛ", L"ﾜ", L"ﾝ", L"ﾞ", L"ﾟ",
		L"￥",
	};
	static const wchar_t convertTo[][2] = {
		L"'", L"\"", L" ",
		L"!", L"#", L"$", L"%", L"&", L"(", L")", L"*", L"+", L",", L"-", L".", L"/",
		L":", L";", L"<", L"=", L">", L"?", L"@", L"[", L"]", L"^", L"_", L"`", L"{", L"|", L"}", L"~",
		L"。", L"「", L"」", L"、", L"・", L"ヲ", L"ァ", L"ィ", L"ゥ", L"ェ", L"ォ", L"ャ", L"ュ", L"ョ", L"ッ", L"ー", L"ア", L"イ", L"ウ", L"エ", L"オ",
		L"ガ", L"ギ", L"グ", L"ゲ", L"ゴ", L"カ", L"キ", L"ク", L"ケ", L"コ",
		L"ザ", L"ジ", L"ズ", L"ゼ", L"ゾ", L"サ", L"シ", L"ス", L"セ", L"ソ",
		L"ダ", L"ヂ", L"ヅ", L"デ", L"ド", L"タ", L"チ", L"ツ", L"テ", L"ト",
		L"ナ", L"ニ", L"ヌ", L"ネ", L"ノ",
		L"バ", L"ビ", L"ブ", L"ベ", L"ボ", L"パ", L"ピ", L"プ", L"ペ", L"ポ", L"ハ", L"ヒ", L"フ", L"ヘ", L"ホ",
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
			for( size_t j = 0; j < _countof(convertFrom); j++ ){
				if( str[i] == convertFrom[j][0] ){
					if( convertFrom[j][1] == L'\0' ){
						str.replace(i, 1, convertTo[j]);
						break;
					}else if( i + 1 < str.size() && str[i + 1] == convertFrom[j][1] ){
						str.replace(i, 2, convertTo[j]);
						break;
					}
				}
			}
		}
	}
}

