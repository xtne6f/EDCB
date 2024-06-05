#include "stdafx.h"
#include "ParseTextInstances.h"
#include "TimeUtil.h"
#include "PathUtil.h"

#if defined(_MSC_VER) && _MSC_VER < 1900
#define wcstoll _wcstoi64
#endif

namespace
{
//タブ区切りの次のトークンに移動する
LPCWSTR NextToken(LPCWSTR* token, WCHAR extraDelimiter = L'\0')
{
	//tokenには現在のトークン先頭/末尾/次のトークン先頭を格納する
	token[0] = token[2];
	for( ; *token[2] != L'\0'; token[2]++ ){
		if( *token[2] == L'\t' || *token[2] == extraDelimiter ){
			token[1] = token[2]++;
			return token[0];
		}
	}
	token[1] = token[2];
	return token[0];
}

//改行をタブに、タブを空白に置換して、残ったタブの数を返す
DWORD FinalizeField(wstring& str)
{
	DWORD tabCount = 0;
	for( size_t i = 0; i < str.size(); i++ ){
		if( str[i] == L'\n' ){
			str[i] = L'\t';
			tabCount++;
		}else if( str[i] == L'\t' ){
			str[i] = L' ';
		}
	}
	return tabCount;
}

bool ParseDateTime(LPCWSTR* token, SYSTEMTIME& st, DWORD* duration = NULL)
{
	LONGLONG t;
	st.wMilliseconds = 0;
	for( int i = 0; i < (duration ? 3 : 2); i++ ){
		NextToken(token);
		LPWSTR endp0, endp1, endp2;
		WORD w0 = (WORD)wcstoul(token[0], &endp0, 10);
		if( endp0 == token[0] || *endp0 != (i == 0 ? L'/' : L':') ){
			return false;
		}
		WORD w1 = (WORD)wcstoul(++endp0, &endp1, 10);
		if( endp1 == endp0 || *endp1 != (i == 0 ? L'/' : L':') ){
			return false;
		}
		WORD w2 = (WORD)wcstoul(++endp1, &endp2, 10);
		if( endp2 == endp1 || endp2 > token[1] ){
			return false;
		}
		if( i == 0 ){
			st.wYear = w0;
			st.wMonth = w1;
			st.wDay = w2;
		}else if( i == 1 ){
			st.wHour = w0;
			st.wMinute = w1;
			st.wSecond = w2;
		}else{
			*duration = (w0 * 60 + w1) * 60 + w2;
		}
	}
	return (t = ConvertI64Time(st)) != 0 && ConvertSystemTime(t, &st);
}

int NextTokenToInt(LPCWSTR* token)
{
	LPWSTR endp;
	int n = (int)wcstol(NextToken(token), &endp, 10);
	return endp > token[1] ? 0 : n;
}

void ParseRecFolderList(LPCWSTR* token, vector<REC_FILE_SET_INFO>& list)
{
	for( int n = NextTokenToInt(token); n > 0; n-- ){
		NextToken(token);
		list.resize(list.size() + 1);
		list.back().recFolder.assign(token[0], token[1]);
	}
	for( size_t i = 0; i < list.size(); ){
		if( list[i].recFolder.empty() ){
			list.erase(list.begin() + i);
		}else{
			//US制御文字があればそれで分割、なければ*で分割
			LPCWSTR sep = list[i].recFolder.find(L'\x1F') != wstring::npos ? L"\x1F" : L"*";
			Separate(list[i].recFolder, sep, list[i].recFolder, list[i].writePlugIn);
			Separate(list[i].writePlugIn, sep, list[i].writePlugIn, list[i].recNamePlugIn);
			if( list[i].writePlugIn.empty() ){
				list[i].writePlugIn = L"Write_Default.dll";
			}
			i++;
		}
	}
}

bool SerializeRecFolder(const REC_FILE_SET_INFO& info, wstring& str)
{
	if( info.recFolder.find(L'\x1F') != wstring::npos ||
	    info.writePlugIn.find(L'\x1F') != wstring::npos ){
		//分割できない場所にUS制御文字がある
		return false;
	}
	//US制御文字を含むか*で分割できないならUS制御文字で区切る
	WCHAR sep = info.recFolder.find(L'*') != wstring::npos ||
	            info.writePlugIn.find(L'*') != wstring::npos ||
	            info.recNamePlugIn.find(L'\x1F') != wstring::npos ? L'\x1F' : L'*';
	str += info.recFolder;
	str += sep;
	str += info.writePlugIn;
	str += sep;
	str += info.recNamePlugIn;
	str += L'\n';
	return true;
}
}

void CParseChText4::SetFilePath(LPCWSTR path)
{
	this->filePath = path;
	this->isUtf8 = true;
	if( this->filePath.empty() == false ){
		//文字コードを維持する
		std::unique_ptr<FILE, fclose_deleter> fp(UtilOpenFile(this->filePath, UTIL_SECURE_READ));
		if( fp ){
			char buf[3];
			this->isUtf8 = fread(buf, 1, 3, fp.get()) == 3 && buf[0] == '\xEF' && buf[1] == '\xBB' && buf[2] == '\xBF';
		}
	}
}

DWORD CParseChText4::AddCh(const CH_DATA4& item)
{
	map<DWORD, CH_DATA4>::const_iterator itr =
		this->itemMap.insert(pair<DWORD, CH_DATA4>(this->itemMap.empty() ? 1 : this->itemMap.rbegin()->first + 1, item)).first;
	return itr->first;
}

void CParseChText4::DelCh(DWORD key)
{
	this->itemMap.erase(key);
}

void CParseChText4::SetUseViewFlag(DWORD key, BOOL useViewFlag)
{
	map<DWORD, CH_DATA4>::iterator itr = this->itemMap.find(key);
	if( itr != this->itemMap.end() ){
		itr->second.useViewFlag = useViewFlag;
	}
}

bool CParseChText4::ParseLine(LPCWSTR parseLine, pair<DWORD, CH_DATA4>& item)
{
	if( wcschr(parseLine, L'\t') == NULL ){
		return false;
	}
	LPCWSTR token[3] = {NULL, NULL, parseLine};

	NextToken(token);
	item.second.chName.assign(token[0], token[1]);
	NextToken(token);
	item.second.serviceName.assign(token[0], token[1]);
	NextToken(token);
	item.second.networkName.assign(token[0], token[1]);

	item.second.space = NextTokenToInt(token);
	item.second.ch = NextTokenToInt(token);
	item.second.originalNetworkID = (WORD)NextTokenToInt(token);
	item.second.transportStreamID = (WORD)NextTokenToInt(token);
	item.second.serviceID = (WORD)NextTokenToInt(token);
	item.second.serviceType = (WORD)NextTokenToInt(token);
	item.second.partialFlag = NextTokenToInt(token) != 0;
	item.second.useViewFlag = NextTokenToInt(token) != 0;
	item.second.remoconID = (BYTE)NextTokenToInt(token);
	item.first = this->itemMap.empty() ? 1 : this->itemMap.rbegin()->first + 1;
	return true;
}

bool CParseChText4::SaveLine(const pair<DWORD, CH_DATA4>& item, wstring& saveLine) const
{
	Format(saveLine, L"%ls\n%ls\n%ls\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d",
		item.second.chName.c_str(),
		item.second.serviceName.c_str(),
		item.second.networkName.c_str(),
		item.second.space,
		item.second.ch,
		item.second.originalNetworkID,
		item.second.transportStreamID,
		item.second.serviceID,
		item.second.serviceType,
		item.second.partialFlag,
		item.second.useViewFlag,
		item.second.remoconID
		);
	return FinalizeField(saveLine) == 11;
}

LONGLONG CParseChText5::AddCh(const CH_DATA5& item)
{
	this->parsedOrder.clear();
	LONGLONG key = (LONGLONG)item.originalNetworkID << 32 | (LONGLONG)item.transportStreamID << 16 | item.serviceID;
	this->itemMap[key] = item;
	return key;
}

void CParseChText5::DelCh(LONGLONG key)
{
	this->itemMap.erase(key);
	vector<LONGLONG>::iterator itr = std::find(this->parsedOrder.begin(), this->parsedOrder.end(), key);
	if( itr != this->parsedOrder.end() ){
		this->parsedOrder.erase(itr);
	}
}

bool CParseChText5::SetEpgCapMode(WORD originalNetworkID, WORD transportStreamID, WORD serviceID, BOOL epgCapFlag)
{
	map<LONGLONG, CH_DATA5>::iterator itr = this->itemMap.find((LONGLONG)originalNetworkID << 32 | (LONGLONG)transportStreamID << 16 | serviceID);
	if( itr != this->itemMap.end() ){
		itr->second.epgCapFlag = epgCapFlag;
		return true;
	}
	return false;
}

bool CParseChText5::SetRemoconID(WORD originalNetworkID, WORD transportStreamID, WORD serviceID, BYTE remoconID)
{
	map<LONGLONG, CH_DATA5>::iterator itr = this->itemMap.find((LONGLONG)originalNetworkID << 32 | (LONGLONG)transportStreamID << 16 | serviceID);
	if( itr != this->itemMap.end() ){
		itr->second.remoconID = remoconID;
		return true;
	}
	return false;
}

bool CParseChText5::SaveTextWithExtraFields(string* saveToStr) const
{
	saveWithExtraFields = true;
	bool ret = SaveText(saveToStr);
	saveWithExtraFields = false;
	return ret;
}

bool CParseChText5::ParseLine(LPCWSTR parseLine, pair<LONGLONG, CH_DATA5>& item)
{
	if( wcschr(parseLine, L'\t') == NULL ){
		return false;
	}
	LPCWSTR token[3] = {NULL, NULL, parseLine};

	NextToken(token);
	item.second.serviceName.assign(token[0], token[1]);
	NextToken(token);
	item.second.networkName.assign(token[0], token[1]);

	item.second.originalNetworkID = (WORD)NextTokenToInt(token);
	item.second.transportStreamID = (WORD)NextTokenToInt(token);
	item.second.serviceID = (WORD)NextTokenToInt(token);
	item.second.serviceType = (WORD)NextTokenToInt(token);
	item.second.partialFlag = NextTokenToInt(token) != 0;
	item.second.epgCapFlag = NextTokenToInt(token) != 0;
	item.second.searchFlag = NextTokenToInt(token) != 0;
	item.second.remoconID = 0;
	item.first = (LONGLONG)item.second.originalNetworkID << 32 | (LONGLONG)item.second.transportStreamID << 16 | item.second.serviceID;
	if( this->itemMap.empty() ){
		this->parsedOrder.clear();
	}
	if( this->itemMap.count(item.first) == 0 ){
		this->parsedOrder.push_back(item.first);
	}
	return true;
}

bool CParseChText5::SaveLine(const pair<LONGLONG, CH_DATA5>& item, wstring& saveLine) const
{
	WCHAR extra[16];
	swprintf_s(extra, L"\n%d", item.second.remoconID);
	Format(saveLine, L"%ls\n%ls\n%d\n%d\n%d\n%d\n%d\n%d\n%d%ls",
		item.second.serviceName.c_str(),
		item.second.networkName.c_str(),
		item.second.originalNetworkID,
		item.second.transportStreamID,
		item.second.serviceID,
		item.second.serviceType,
		item.second.partialFlag,
		item.second.epgCapFlag,
		item.second.searchFlag,
		saveWithExtraFields ? extra : L""
		);
	return FinalizeField(saveLine) == (DWORD)(saveWithExtraFields ? 9 : 8);
}

bool CParseChText5::SelectItemToSave(vector<map<LONGLONG, CH_DATA5>::const_iterator>& itemList) const
{
	//情報の追加がなければ読み込み順を維持
	if( this->parsedOrder.size() == this->itemMap.size() ){
		itemList.reserve(this->parsedOrder.size());
		for( size_t i = 0; i < this->parsedOrder.size(); i++ ){
			itemList.push_back(this->itemMap.find(this->parsedOrder[i]));
		}
		return true;
	}
	return false;
}

void CParseContentTypeText::GetMimeType(wstring ext, wstring& mimeType) const
{
	map<wstring, wstring>::const_iterator itr = this->itemMap.find(ext);
	if( itr == this->itemMap.end() ){
		mimeType = L"application/octet-stream";
	}else{
		mimeType = itr->second;
	}
}

bool CParseContentTypeText::ParseLine(LPCWSTR parseLine, pair<wstring, wstring>& item)
{
	if( wcschr(parseLine, L'\t') == NULL || parseLine[0] == L';' ){
		//空行orコメント
		return false;
	}
	LPCWSTR token[3] = {NULL, NULL, parseLine};

	NextToken(token);
	item.first.assign(token[0], token[1]);
	NextToken(token);
	item.second.assign(token[0], token[1]);
	return true;
}

void CParseServiceChgText::ChgText(wstring& chgText) const
{
	map<wstring, wstring>::const_iterator itr = this->itemMap.find(chgText);
	if( itr != this->itemMap.end() ){
		chgText = itr->second;
	}
}

bool CParseServiceChgText::ParseLine(LPCWSTR parseLine, pair<wstring, wstring>& item)
{
	if( wcschr(parseLine, L'\t') == NULL || parseLine[0] == L';' ){
		return false;
	}
	LPCWSTR token[3] = {NULL, NULL, parseLine};

	NextToken(token);
	item.first.assign(token[0], token[1]);
	NextToken(token);
	item.second.assign(token[0], token[1]);
	return true;
}

DWORD CParseRecInfoText::AddRecInfo(const REC_FILE_INFO& item)
{
	map<DWORD, REC_FILE_INFO>::iterator itr = this->itemMap.insert(std::make_pair(this->nextID, item)).first;
	this->nextID = this->nextID % 100000000 + 1;
	DWORD id = itr->second.id = itr->first;

	//非プロテクトの要素数がkeepCount以下になるまで削除
	if( this->keepCount < UINT_MAX ){
		size_t protectCount = std::count_if(this->itemMap.begin(), this->itemMap.end(),
			[](const pair<DWORD, REC_FILE_INFO>& a) { return a.second.protectFlag != 0; });
		itr++;
		while( this->itemMap.size() - protectCount > this->keepCount ){
			if( itr == this->itemMap.end() ){
				itr = this->itemMap.begin();
			}
			if( itr->second.protectFlag == 0 ){
				OnDelRecInfo(itr->second);
				itr = this->itemMap.erase(itr);
			}else{
				itr++;
			}
		}
	}
	return id;
}

bool CParseRecInfoText::DelRecInfo(DWORD id)
{
	map<DWORD, REC_FILE_INFO>::const_iterator itr = this->itemMap.find(id);
	if( itr != this->itemMap.end() && itr->second.protectFlag == 0 ){
		OnDelRecInfo(itr->second);
		this->itemMap.erase(itr);
		return true;
	}
	return false;
}

bool CParseRecInfoText::ChgPathRecInfo(DWORD id, LPCWSTR recFilePath)
{
	map<DWORD, REC_FILE_INFO>::iterator itr = this->itemMap.find(id);
	if( itr != this->itemMap.end() ){
		itr->second.recFilePath = recFilePath;
		return true;
	}
	return false;
}

bool CParseRecInfoText::ChgProtectRecInfo(DWORD id, BYTE flag)
{
	map<DWORD, REC_FILE_INFO>::iterator itr = this->itemMap.find(id);
	if( itr != this->itemMap.end() ){
		itr->second.protectFlag = flag;
		return true;
	}
	return false;
}

void CParseRecInfoText::SetRecInfoFolder(LPCWSTR folder)
{
	this->recInfoFolder = folder;
}

bool CParseRecInfoText::ParseLine(LPCWSTR parseLine, pair<DWORD, REC_FILE_INFO>& item)
{
	if( this->saveNextID == 1 ){
		this->saveNextID = 0;
	}
	if( parseLine[0] == L';' ){
		if( wcsncmp(parseLine, L";;NextID=", 9) == 0 ){
			DWORD nextID_ = (DWORD)wcstol(parseLine + 9, NULL, 10);
			if( nextID_ != 0 && nextID_ <= 100000000 ){
				this->nextID = this->nextID > nextID_ + 50000000 ? nextID_ : max(nextID_, this->nextID);
			}
			this->saveNextID = 2;
		}
		return false;
	}else if( wcschr(parseLine, L'\t') == NULL ){
		return false;
	}
	LPCWSTR token[3] = {NULL, NULL, parseLine};

	NextToken(token);
	item.second.recFilePath.assign(token[0], token[1]);
	NextToken(token);
	item.second.title.assign(token[0], token[1]);

	if( ParseDateTime(token, item.second.startTime, &item.second.durationSecond) == false ){
		return false;
	}
	NextToken(token);
	item.second.serviceName.assign(token[0], token[1]);
	item.second.originalNetworkID = (WORD)NextTokenToInt(token);
	item.second.transportStreamID = (WORD)NextTokenToInt(token);
	item.second.serviceID = (WORD)NextTokenToInt(token);
	item.second.eventID = (WORD)NextTokenToInt(token);
	item.second.drops = wcstoll(NextToken(token), NULL, 10);
	item.second.scrambles = wcstoll(NextToken(token), NULL, 10);
	item.second.recStatus = (DWORD)NextTokenToInt(token);

	if( ParseDateTime(token, item.second.startTimeEpg) == false ){
		return false;
	}
	NextToken(token);
	item.second.protectFlag = NextTokenToInt(token) != 0;
	item.second.id = item.first = (DWORD)NextTokenToInt(token);
	if( item.first == 0 || item.first > 100000000 || this->itemMap.count(item.first) ){
		//新しいIDを与える
		item.second.id = item.first = this->nextID;
	}
	this->nextID = this->nextID > item.first + 50000000 ? item.first + 1 : (max(item.first + 1, this->nextID) - 1) % 100000000 + 1;
	return true;
}

bool CParseRecInfoText::SaveLine(const pair<DWORD, REC_FILE_INFO>& item, wstring& saveLine) const
{
	WCHAR id[32];
	swprintf_s(id, L"%d", item.second.id);
	Format(saveLine, L"%ls\n%ls\n%04d/%02d/%02d\n%02d:%02d:%02d\n%02d:%02d:%02d\n%ls\n%d\n%d\n%d\n%d\n%lld\n%lld\n%d\n%04d/%02d/%02d\n%02d:%02d:%02d\n%ls\n%d\n%ls",
		item.second.recFilePath.c_str(),
		item.second.title.c_str(),
		item.second.startTime.wYear, item.second.startTime.wMonth, item.second.startTime.wDay,
		item.second.startTime.wHour, item.second.startTime.wMinute, item.second.startTime.wSecond,
		item.second.durationSecond / 60 / 60, item.second.durationSecond / 60 % 60, item.second.durationSecond % 60,
		item.second.serviceName.c_str(),
		item.second.originalNetworkID,
		item.second.transportStreamID,
		item.second.serviceID,
		item.second.eventID,
		item.second.drops,
		item.second.scrambles,
		item.second.recStatus,
		item.second.startTimeEpg.wYear, item.second.startTimeEpg.wMonth, item.second.startTimeEpg.wDay,
		item.second.startTimeEpg.wHour, item.second.startTimeEpg.wMinute, item.second.startTimeEpg.wSecond,
		item.second.GetComment(),
		item.second.protectFlag,
		this->saveNextID != 0 ? id : L""
		);
	return FinalizeField(saveLine) == 17;
}

bool CParseRecInfoText::SaveFooterLine(wstring& saveLine) const
{
	//次の読み込み時にnextIDを復元するためのフッタコメント
	//このコメントはもし削除されても大きな問題はない
	Format(saveLine, L";;NextID=%d", this->nextID);
	//読み込み時にこのコメントが無かったときは保存しない
	return this->saveNextID != 0;
}

bool CParseRecInfoText::SelectItemToSave(vector<map<DWORD, REC_FILE_INFO>::const_iterator>& itemList) const
{
	if( this->saveNextID != 0 ){
		if( this->itemMap.empty() == false && this->itemMap.rbegin()->first >= this->itemMap.begin()->first + 50000000 ){
			//ID巡回中
			map<DWORD, REC_FILE_INFO>::const_iterator itr;
			for( itr = this->itemMap.upper_bound(50000000); itr != this->itemMap.end(); itr++ ){
				itemList.push_back(itr);
			}
			for( itr = this->itemMap.begin(); itr->first <= 50000000; itr++ ){
				itemList.push_back(itr);
			}
			return true;
		}
		return false;
	}
	//NextIDコメントが無かったときは従来どおり開始日時順で保存する
	itemList.reserve(this->itemMap.size());
	for( map<DWORD, REC_FILE_INFO>::const_iterator itr = this->itemMap.begin(); itr != this->itemMap.end(); itr++ ){
		itemList.push_back(itr);
	}
	std::sort(itemList.begin(), itemList.end(), [](map<DWORD, REC_FILE_INFO>::const_iterator a, map<DWORD, REC_FILE_INFO>::const_iterator b) -> bool {
		const SYSTEMTIME& sa = a->second.startTime;
		const SYSTEMTIME& sb = b->second.startTime;
		return sa.wYear < sb.wYear || sa.wYear == sb.wYear && (
		       sa.wMonth < sb.wMonth || sa.wMonth == sb.wMonth && (
		       sa.wDay < sb.wDay || sa.wDay == sb.wDay && (
		       sa.wHour < sb.wHour || sa.wHour == sb.wHour && (
		       sa.wMinute < sb.wMinute || sa.wMinute == sb.wMinute && (
		       sa.wSecond < sb.wSecond || sa.wSecond == sb.wSecond && (
		       a->second.originalNetworkID < b->second.originalNetworkID || a->second.originalNetworkID == b->second.originalNetworkID && (
		       a->second.transportStreamID < b->second.transportStreamID)))))));
	});
	return true;
}

wstring CParseRecInfoText::GetExtraInfo(LPCWSTR recFilePath, LPCWSTR extension, const wstring& resultOfGetRecInfoFolder, bool recInfoFolderOnly)
{
	wstring info;
	if( recFilePath[0] != L'\0' ){
		//補足の録画情報ファイルを読み込む
		std::unique_ptr<FILE, fclose_deleter> fp;
		if( resultOfGetRecInfoFolder.empty() || recInfoFolderOnly == false ){
			fp.reset(UtilOpenFile(fs_path(recFilePath).concat(extension), UTIL_SHARED_READ | UTIL_SH_DELETE));
		}
		if( !fp && resultOfGetRecInfoFolder.empty() == false ){
			fs_path infoPath = fs_path(resultOfGetRecInfoFolder).append(fs_path(recFilePath).filename().concat(extension).native());
			fp.reset(UtilOpenFile(infoPath, UTIL_SHARED_READ | UTIL_SH_DELETE));
		}
		if( fp && my_fseek(fp.get(), 0, SEEK_END) == 0 ){
			LONGLONG fileSize = my_ftell(fp.get());
			if( 0 < fileSize && fileSize < 1024 * 1024 ){
				vector<char> buf((size_t)fileSize + 1, '\0');
				rewind(fp.get());
				if( fread(buf.data(), 1, (size_t)fileSize, fp.get()) == (size_t)fileSize ){
					if( fileSize >= 3 && buf[0] == '\xEF' && buf[1] == '\xBB' && buf[2] == '\xBF' ){
						UTF8toW(buf.data() + 3, info);
					}else{
						AtoW(buf.data(), info);
					}
				}
			}
		}
	}
	return info;
}

void CParseRecInfoText::OnDelRecInfo(const REC_FILE_INFO& item)
{
	if( item.recFilePath.empty() || this->recInfoDelFile == false ){
		return;
	}
	//録画ファイルを削除する
	DeleteFile(item.recFilePath.c_str());
	if( this->customizeDelExt ){
		//カスタムルール
		AddDebugLogFormat(L"★RecInfo Auto Delete : %ls", item.recFilePath.c_str());
		wstring debug;
		for( size_t i = 0; i < this->customDelExt.size(); i++ ){
			wstring delPath = fs_path(item.recFilePath).replace_extension().native();
			DeleteFile((delPath + this->customDelExt[i]).c_str());
			debug = (debug.empty() ? delPath + L'(' : debug + L'|') + this->customDelExt[i];
		}
		if( debug.empty() == false ){
			AddDebugLogFormat(L"★RecInfo Auto Delete : %ls)", debug.c_str());
		}
		if( this->recInfoFolder.empty() == false ){
			//録画情報フォルダにも適用
			debug.clear();
			for( size_t i = 0; i < this->customDelExt.size(); i++ ){
				wstring delPath = fs_path(this->recInfoFolder).append(fs_path(item.recFilePath).stem().native()).native();
				DeleteFile((delPath + this->customDelExt[i]).c_str());
				debug = (debug.empty() ? delPath + L'(' : debug + L'|') + this->customDelExt[i];
			}
			if( debug.empty() == false ){
				AddDebugLogFormat(L"★RecInfo Auto Delete : %ls)", debug.c_str());
			}
		}
	}else{
		//標準のルール
		DeleteFile((item.recFilePath + L".err").c_str());
		DeleteFile((item.recFilePath + L".program.txt").c_str());
		AddDebugLogFormat(L"★RecInfo Auto Delete : %ls(.err|.program.txt)", item.recFilePath.c_str());
		if( this->recInfoFolder.empty() == false ){
			//録画情報フォルダにも適用
			wstring delPath = fs_path(this->recInfoFolder).append(fs_path(item.recFilePath).filename().native()).native();
			DeleteFile((delPath + L".err").c_str());
			DeleteFile((delPath + L".program.txt").c_str());
			AddDebugLogFormat(L"★RecInfo Auto Delete : %ls(.err|.program.txt)", delPath.c_str());
		}
	}
}

DWORD CParseRecInfo2Text::Add(const PARSE_REC_INFO2_ITEM& item)
{
	map<DWORD, PARSE_REC_INFO2_ITEM>::const_iterator itr =
		this->itemMap.insert(pair<DWORD, PARSE_REC_INFO2_ITEM>(this->itemMap.empty() ? 1 : this->itemMap.rbegin()->first + 1, item)).first;
	return itr->first;
}

bool CParseRecInfo2Text::ParseLine(LPCWSTR parseLine, pair<DWORD, PARSE_REC_INFO2_ITEM>& item)
{
	if( wcschr(parseLine, L'\t') == NULL || parseLine[0] == L';' ){
		return false;
	}
	LPCWSTR token[3] = {NULL, NULL, parseLine};

	item.second.originalNetworkID = (WORD)NextTokenToInt(token);
	item.second.transportStreamID = (WORD)NextTokenToInt(token);
	item.second.serviceID = (WORD)NextTokenToInt(token);

	if( ParseDateTime(token, item.second.startTime) == false ){
		return false;
	}
	NextToken(token);
	item.second.eventName.assign(token[0], token[1]);
	item.first = this->itemMap.empty() ? 1 : this->itemMap.rbegin()->first + 1;
	return true;
}

bool CParseRecInfo2Text::SaveLine(const pair<DWORD, PARSE_REC_INFO2_ITEM>& item, wstring& saveLine) const
{
	Format(saveLine, L"%d\n%d\n%d\n%04d/%02d/%02d\n%02d:%02d:%02d\n%ls",
		item.second.originalNetworkID,
		item.second.transportStreamID,
		item.second.serviceID,
		item.second.startTime.wYear, item.second.startTime.wMonth, item.second.startTime.wDay,
		item.second.startTime.wHour, item.second.startTime.wMinute, item.second.startTime.wSecond,
		item.second.eventName.c_str()
		);
	return FinalizeField(saveLine) == 5;
}

bool CParseRecInfo2Text::SelectItemToSave(vector<map<DWORD, PARSE_REC_INFO2_ITEM>::const_iterator>& itemList) const
{
	map<DWORD, PARSE_REC_INFO2_ITEM>::const_iterator itr = this->itemMap.begin();
	if( this->itemMap.size() > this->keepCount ){
		advance(itr, this->itemMap.size() - this->keepCount);
		itemList.reserve(this->keepCount);
		for( ; itr != this->itemMap.end(); itr++ ){
			itemList.push_back(itr);
		}
		return true;
	}
	return false;
}

DWORD CParseReserveText::AddReserve(const RESERVE_DATA& item)
{
	map<DWORD, RESERVE_DATA>::iterator itr = this->itemMap.insert(std::make_pair(this->nextID, item)).first;
	this->nextID = this->nextID % 100000000 + 1;
	this->sortByEventCache.clear();
	return itr->second.reserveID = itr->first;
}

bool CParseReserveText::ChgReserve(const RESERVE_DATA& item)
{
	map<DWORD, RESERVE_DATA>::iterator itr = this->itemMap.find(item.reserveID);
	if( itr != this->itemMap.end() ){
		itr->second = item;
		this->sortByEventCache.clear();
		return true;
	}
	return false;
}

bool CParseReserveText::SetPresentFlag(DWORD id, BYTE presentFlag)
{
	map<DWORD, RESERVE_DATA>::iterator itr = this->itemMap.find(id);
	if( itr != this->itemMap.end() ){
		itr->second.presentFlag = presentFlag;
		return true;
	}
	return false;
}

bool CParseReserveText::SetOverlapMode(DWORD id, BYTE overlapMode)
{
	map<DWORD, RESERVE_DATA>::iterator itr = this->itemMap.find(id);
	if( itr != this->itemMap.end() ){
		itr->second.overlapMode = overlapMode;
		return true;
	}
	return false;
}

bool CParseReserveText::AddNGTunerID(DWORD id, DWORD tunerID)
{
	map<DWORD, RESERVE_DATA>::iterator itr = this->itemMap.find(id);
	if( itr != this->itemMap.end() ){
		itr->second.ngTunerIDList.push_back(tunerID);
		return true;
	}
	return false;
}

bool CParseReserveText::DelReserve(DWORD id)
{
	if( this->itemMap.erase(id) != 0 ){
		this->sortByEventCache.clear();
		return true;
	}
	return false;
}

bool CParseReserveText::ParseLine(LPCWSTR parseLine, pair<DWORD, RESERVE_DATA>& item)
{
	if( this->saveNextID == 1 ){
		this->saveNextID = 0;
	}
	if( parseLine[0] == L';' ){
		if( wcsncmp(parseLine, L";;NextID=", 9) == 0 ){
			DWORD nextID_ = (DWORD)wcstol(parseLine + 9, NULL, 10);
			if( nextID_ != 0 && nextID_ <= 100000000 ){
				this->nextID = this->nextID > nextID_ + 50000000 ? nextID_ : max(nextID_, this->nextID);
			}
			this->saveNextID = 2;
		}
		return false;
	}else if( wcschr(parseLine, L'\t') == NULL ){
		return false;
	}
	LPCWSTR token[3] = {NULL, NULL, parseLine};

	if( ParseDateTime(token, item.second.startTime, &item.second.durationSecond) == false ){
		return false;
	}
	NextToken(token);
	item.second.title.assign(token[0], token[1]);
	NextToken(token);
	item.second.stationName.assign(token[0], token[1]);
	item.second.originalNetworkID = (WORD)NextTokenToInt(token);
	item.second.transportStreamID = (WORD)NextTokenToInt(token);
	item.second.serviceID = (WORD)NextTokenToInt(token);
	item.second.eventID = (WORD)NextTokenToInt(token);
	item.second.recSetting.priority = (BYTE)NextTokenToInt(token);
	item.second.recSetting.tuijyuuFlag = NextTokenToInt(token) != 0 && item.second.eventID != 0xFFFF;
	item.second.reserveID = item.first = (DWORD)NextTokenToInt(token);
	if( item.first == 0 || item.first > 100000000 ){
		return false;
	}
	item.second.recSetting.recMode = (BYTE)NextTokenToInt(token);
	item.second.recSetting.pittariFlag = NextTokenToInt(token) != 0 && item.second.eventID != 0xFFFF;
	NextToken(token);
	if( item.second.recSetting.batFilePath.assign(token[0], token[1]) == L"0" ){
		item.second.recSetting.batFilePath.clear();
	}
	//将来用
	NextToken(token);
	NextToken(token);
	item.second.comment.assign(token[0], token[1]);
	NextToken(token);
	//録画フォルダパスの最初の要素だけここにある
	item.second.recSetting.recFolderList.resize(1);
	item.second.recSetting.recFolderList[0].recFolder.assign(token[0], token[1]);
	item.second.recSetting.suspendMode = (BYTE)NextTokenToInt(token);
	item.second.recSetting.rebootFlag = NextTokenToInt(token) != 0;
	//廃止(旧recFilePath)
	NextToken(token);
	item.second.recSetting.useMargineFlag = NextTokenToInt(token) != 0;
	item.second.recSetting.startMargine = NextTokenToInt(token);
	item.second.recSetting.endMargine = NextTokenToInt(token);
	item.second.recSetting.serviceMode = (DWORD)NextTokenToInt(token);

	if( ParseDateTime(token, item.second.startTimeEpg) == false ){
		return false;
	}
	ParseRecFolderList(token, item.second.recSetting.recFolderList);
	item.second.recSetting.continueRecFlag = NextTokenToInt(token) != 0;
	item.second.recSetting.partialRecFlag = NextTokenToInt(token) != 0;
	item.second.recSetting.tunerID = (DWORD)NextTokenToInt(token);
	item.second.reserveStatus = (DWORD)NextTokenToInt(token);

	ParseRecFolderList(token, item.second.recSetting.partialRecFolder);
	item.second.presentFlag = 0;
	item.second.overlapMode = 0;
	this->nextID = this->nextID > item.first + 50000000 ? item.first + 1 : (max(item.first + 1, this->nextID) - 1) % 100000000 + 1;
	this->sortByEventCache.clear();
	return true;
}

bool CParseReserveText::SaveLine(const pair<DWORD, RESERVE_DATA>& item, wstring& saveLine) const
{
	wstring strFirstRecFolder;
	if( item.second.recSetting.recFolderList.empty() == false ){
		if( SerializeRecFolder(item.second.recSetting.recFolderList[0], strFirstRecFolder) ){
			//'\n'を除去
			strFirstRecFolder.pop_back();
		}else{
			return false;
		}
	}
	wstring strRecFolder;
	for( size_t i = 1; i < item.second.recSetting.recFolderList.size(); i++ ){
		if( SerializeRecFolder(item.second.recSetting.recFolderList[i], strRecFolder) == false ){
			return false;
		}
	}
	wstring strPartialRecFolder;
	for( size_t i = 0; i < item.second.recSetting.partialRecFolder.size(); i++ ){
		if( SerializeRecFolder(item.second.recSetting.partialRecFolder[i], strPartialRecFolder) == false ){
			return false;
		}
	}
	Format(saveLine, L"%04d/%02d/%02d\n%02d:%02d:%02d\n%02d:%02d:%02d\n%ls\n%ls\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%ls\n%ls\n%ls\n%ls\n%d\n%d\n%ls\n%d\n%d\n%d\n%d\n%04d/%02d/%02d\n%02d:%02d:%02d\n%d\n%ls%d\n%d\n%d\n%d\n%d\n%ls",
		item.second.startTime.wYear, item.second.startTime.wMonth, item.second.startTime.wDay,
		item.second.startTime.wHour, item.second.startTime.wMinute, item.second.startTime.wSecond,
		item.second.durationSecond / 60 / 60, item.second.durationSecond / 60 % 60, item.second.durationSecond % 60,
		item.second.title.c_str(),
		item.second.stationName.c_str(),
		item.second.originalNetworkID,
		item.second.transportStreamID,
		item.second.serviceID,
		item.second.eventID,
		item.second.recSetting.priority,
		item.second.recSetting.tuijyuuFlag,
		item.second.reserveID,
		item.second.recSetting.recMode,
		item.second.recSetting.pittariFlag,
		item.second.recSetting.batFilePath.empty() ? L"0" : item.second.recSetting.batFilePath.c_str(),
		L"0",
		item.second.comment.c_str(),
		strFirstRecFolder.c_str(),
		item.second.recSetting.suspendMode,
		item.second.recSetting.rebootFlag,
		L"",
		item.second.recSetting.useMargineFlag,
		item.second.recSetting.startMargine,
		item.second.recSetting.endMargine,
		item.second.recSetting.serviceMode,
		item.second.startTimeEpg.wYear, item.second.startTimeEpg.wMonth, item.second.startTimeEpg.wDay,
		item.second.startTimeEpg.wHour, item.second.startTimeEpg.wMinute, item.second.startTimeEpg.wSecond,
		(int)(item.second.recSetting.recFolderList.empty() ? 0 : item.second.recSetting.recFolderList.size() - 1),
		strRecFolder.c_str(),
		item.second.recSetting.continueRecFlag,
		item.second.recSetting.partialRecFlag,
		item.second.recSetting.tunerID,
		item.second.reserveStatus,
		(int)item.second.recSetting.partialRecFolder.size(),
		strPartialRecFolder.c_str()
		);
	return FinalizeField(saveLine) == 33 + max((int)item.second.recSetting.recFolderList.size() - 1, 0) + item.second.recSetting.partialRecFolder.size();
}

bool CParseReserveText::SaveFooterLine(wstring& saveLine) const
{
	Format(saveLine, L";;NextID=%d", this->nextID);
	return this->saveNextID != 0;
}

bool CParseReserveText::SelectItemToSave(vector<map<DWORD, RESERVE_DATA>::const_iterator>& itemList) const
{
	if( this->saveNextID == 0 ){
		//NextIDコメントが無かったときは従来どおり予約日時順で保存する
		vector<pair<LONGLONG, const RESERVE_DATA*>> sortItemList = GetReserveList();
		vector<pair<LONGLONG, const RESERVE_DATA*>>::const_iterator itr;
		for( itr = sortItemList.begin(); itr != sortItemList.end(); itr++ ){
			itemList.push_back(this->itemMap.find(itr->second->reserveID));
		}
		return true;
	}
	if( this->itemMap.empty() == false && this->itemMap.rbegin()->first >= this->itemMap.begin()->first + 50000000 ){
		//ID巡回中
		map<DWORD, RESERVE_DATA>::const_iterator itr;
		for( itr = this->itemMap.upper_bound(50000000); itr != this->itemMap.end(); itr++ ){
			itemList.push_back(itr);
		}
		for( itr = this->itemMap.begin(); itr->first <= 50000000; itr++ ){
			itemList.push_back(itr);
		}
		return true;
	}
	return false;
}

vector<pair<LONGLONG, const RESERVE_DATA*>> CParseReserveText::GetReserveList(BOOL calcMargin, int defStartMargin) const
{
	vector<pair<LONGLONG, const RESERVE_DATA*>> retList;
	retList.reserve(this->itemMap.size());

	//日時順にソート
	map<DWORD, RESERVE_DATA>::const_iterator itr;
	for( itr = this->itemMap.begin(); itr != this->itemMap.end(); itr++ ){
		LONGLONG startTime = ConvertI64Time(itr->second.startTime);
		if( calcMargin != FALSE ){
			LONGLONG endTime = startTime + itr->second.durationSecond * I64_1SEC;
			LONGLONG startMargin = defStartMargin * I64_1SEC;
			if( itr->second.recSetting.useMargineFlag == TRUE ){
				startMargin = itr->second.recSetting.startMargine * I64_1SEC;
			}
			//開始マージンは元の予約終了時刻を超えて負であってはならない
			startTime -= max(startMargin, startTime - endTime);
		}
		retList.push_back( pair<LONGLONG, const RESERVE_DATA*>((startTime / I64_1SEC) << 16 | itr->second.transportStreamID, &itr->second) );
	}
	std::sort(retList.begin(), retList.end());
	return retList;
}

const vector<pair<ULONGLONG, DWORD>>& CParseReserveText::GetSortByEventList() const
{
	if( this->sortByEventCache.empty() || this->itemMap.empty() ){
		this->sortByEventCache.clear();
		this->sortByEventCache.reserve(this->itemMap.size());
		for( map<DWORD, RESERVE_DATA>::const_iterator itr = this->itemMap.begin(); itr != this->itemMap.end(); itr++ ){
			this->sortByEventCache.push_back(std::make_pair(
				(ULONGLONG)itr->second.originalNetworkID << 48 | (ULONGLONG)itr->second.transportStreamID << 32 |
				(DWORD)itr->second.serviceID << 16 | itr->second.eventID, itr->first));
		}
		std::sort(this->sortByEventCache.begin(), this->sortByEventCache.end());
	}
	return this->sortByEventCache;
}

DWORD CParseEpgAutoAddText::AddData(const EPG_AUTO_ADD_DATA& item)
{
	map<DWORD, EPG_AUTO_ADD_DATA>::iterator itr = this->itemMap.insert(std::make_pair(this->nextID, item)).first;
	this->nextID = this->nextID % 100000000 + 1;
	return itr->second.dataID = itr->first;
}

bool CParseEpgAutoAddText::ChgData(const EPG_AUTO_ADD_DATA& item)
{
	map<DWORD, EPG_AUTO_ADD_DATA>::iterator itr = this->itemMap.find(item.dataID);
	if( itr != this->itemMap.end() ){
		itr->second = item;
		return true;
	}
	return false;
}

bool CParseEpgAutoAddText::SetAddCount(DWORD id, DWORD addCount)
{
	map<DWORD, EPG_AUTO_ADD_DATA>::iterator itr = this->itemMap.find(id);
	if( itr != this->itemMap.end() ){
		itr->second.addCount = addCount;
		return true;
	}
	return false;
}

bool CParseEpgAutoAddText::DelData(DWORD id)
{
	return this->itemMap.erase(id) != 0;
}

bool CParseEpgAutoAddText::ParseLine(LPCWSTR parseLine, pair<DWORD, EPG_AUTO_ADD_DATA>& item)
{
	if( this->saveNextID == 1 ){
		this->saveNextID = 0;
	}
	if( parseLine[0] == L';' ){
		if( wcsncmp(parseLine, L";;NextID=", 9) == 0 ){
			DWORD nextID_ = (DWORD)wcstol(parseLine + 9, NULL, 10);
			if( nextID_ != 0 && nextID_ <= 100000000 ){
				this->nextID = this->nextID > nextID_ + 50000000 ? nextID_ : max(nextID_, this->nextID);
			}
			this->saveNextID = 2;
		}
		return false;
	}else if( wcschr(parseLine, L'\t') == NULL ){
		return false;
	}
	LPCWSTR token[3] = {NULL, NULL, parseLine};

	item.second.dataID = item.first = (DWORD)NextTokenToInt(token);
	if( item.first == 0 || item.first > 100000000 ){
		return false;
	}
	NextToken(token);
	item.second.searchInfo.andKey.assign(token[0], token[1]);
	NextToken(token);
	item.second.searchInfo.notKey.assign(token[0], token[1]);
	item.second.searchInfo.regExpFlag = NextTokenToInt(token) != 0;
	item.second.searchInfo.titleOnlyFlag = NextTokenToInt(token) != 0;

	LPCWSTR subToken[3] = {};
	for( subToken[2] = NextToken(token); NextToken(subToken, L',') < token[1]; ){
		//注意: 互換のためwcstol
		LPWSTR endp;
		int flag = (int)wcstol(subToken[0], &endp, 10);
		if( endp != subToken[0] && endp <= subToken[1] ){
			EPGDB_CONTENT_DATA addItem;
			addItem.content_nibble_level_1 = (BYTE)((DWORD)flag >> 24);
			addItem.content_nibble_level_2 = (BYTE)((DWORD)flag >> 16);
			addItem.user_nibble_1 = (BYTE)((DWORD)flag >> 8);
			addItem.user_nibble_2 = (BYTE)((DWORD)flag);
			item.second.searchInfo.contentList.push_back(addItem);
		}
	}
	for( subToken[2] = NextToken(token); NextToken(subToken, L',') < token[1]; ){
		DWORD dwTime[4];
		for( int i = 0; i < 4; i++ ){
			LPWSTR endp;
			dwTime[i] = (DWORD)wcstoul(subToken[0], &endp, 10);
			if( endp == subToken[0] || endp > subToken[1] || (i < 3 && *endp != L'-') ){
				break;
			}else if( i < 3 ){
				subToken[0] = endp + 1;
				continue;
			}
			EPGDB_SEARCH_DATE_INFO addItem;
			addItem.startDayOfWeek = dwTime[0] % 7;
			addItem.startHour = (dwTime[1] >> 16) % 24;
			addItem.startMin = (dwTime[1] & 0xFFFF) % 60;
			addItem.endDayOfWeek = dwTime[2] % 7;
			addItem.endHour = (dwTime[3] >> 16) % 24;
			addItem.endMin = (dwTime[3] & 0xFFFF) % 60;
			item.second.searchInfo.dateList.push_back(addItem);
		}
	}
	for( subToken[2] = NextToken(token); NextToken(subToken, L',') < token[1]; ){
		LPWSTR endp;
		LONGLONG llCh = wcstoll(subToken[0], &endp, 16);
		if( endp != subToken[0] && endp <= subToken[1] ){
			item.second.searchInfo.serviceList.push_back(llCh & 0xFFFFFFFFFFFFLL);
		}
	}
	item.second.recSetting.recMode = (BYTE)NextTokenToInt(token);
	item.second.recSetting.priority = (BYTE)NextTokenToInt(token);
	item.second.recSetting.tuijyuuFlag = NextTokenToInt(token) != 0;
	item.second.recSetting.serviceMode = (DWORD)NextTokenToInt(token);
	item.second.recSetting.pittariFlag = NextTokenToInt(token) != 0;
	NextToken(token);
	item.second.recSetting.batFilePath.assign(token[0], token[1]);
	item.second.recSetting.suspendMode = (BYTE)NextTokenToInt(token);
	item.second.recSetting.rebootFlag = NextTokenToInt(token) != 0;
	item.second.recSetting.useMargineFlag = NextTokenToInt(token) != 0;
	item.second.recSetting.startMargine = NextTokenToInt(token);
	item.second.recSetting.endMargine = NextTokenToInt(token);

	ParseRecFolderList(token, item.second.recSetting.recFolderList);
	item.second.recSetting.continueRecFlag = NextTokenToInt(token) != 0;
	item.second.recSetting.partialRecFlag = NextTokenToInt(token) != 0;
	item.second.recSetting.tunerID = (DWORD)NextTokenToInt(token);
	item.second.searchInfo.aimaiFlag = NextTokenToInt(token) != 0;
	item.second.searchInfo.notContetFlag = NextTokenToInt(token) != 0;
	item.second.searchInfo.notDateFlag = NextTokenToInt(token) != 0;
	item.second.searchInfo.freeCAFlag = (BYTE)NextTokenToInt(token);

	ParseRecFolderList(token, item.second.recSetting.partialRecFolder);
	item.second.searchInfo.chkRecEnd = NextTokenToInt(token) != 0;
	item.second.searchInfo.chkRecDay = (WORD)NextTokenToInt(token);
	item.second.addCount = 0;
	this->nextID = this->nextID > item.first + 50000000 ? item.first + 1 : (max(item.first + 1, this->nextID) - 1) % 100000000 + 1;
	return true;
}

bool CParseEpgAutoAddText::SaveLine(const pair<DWORD, EPG_AUTO_ADD_DATA>& item, wstring& saveLine) const
{
	wstring strContent;
	for( size_t i = 0; i < item.second.searchInfo.contentList.size(); i++ ){
		WCHAR s[64];
		swprintf_s(s, L"%d",
			(DWORD)item.second.searchInfo.contentList[i].content_nibble_level_1 << 24 |
			item.second.searchInfo.contentList[i].content_nibble_level_2 << 16 |
			item.second.searchInfo.contentList[i].user_nibble_1 << 8 |
			item.second.searchInfo.contentList[i].user_nibble_2);
		if( i != 0 ){
			strContent += L',';
		}
		strContent += s;
	}
	wstring strDate;
	for( size_t i = 0; i < item.second.searchInfo.dateList.size(); i++ ){
		WCHAR s[64];
		swprintf_s(s, L"%d-%u-%d-%u",
			item.second.searchInfo.dateList[i].startDayOfWeek,
			(DWORD)item.second.searchInfo.dateList[i].startHour << 16 | item.second.searchInfo.dateList[i].startMin,
			item.second.searchInfo.dateList[i].endDayOfWeek,
			(DWORD)item.second.searchInfo.dateList[i].endHour << 16 | item.second.searchInfo.dateList[i].endMin);
		if( i != 0 ){
			strDate += L',';
		}
		strDate += s;
	}
	wstring strService;
	for( size_t i = 0; i < item.second.searchInfo.serviceList.size(); i++ ){
		WCHAR s[64];
		swprintf_s(s, L"%012llX", item.second.searchInfo.serviceList[i]);
		if( i != 0 ){
			strService += L',';
		}
		strService += s;
	}
	wstring strRecFolder;
	for( size_t i = 0; i < item.second.recSetting.recFolderList.size(); i++ ){
		if( SerializeRecFolder(item.second.recSetting.recFolderList[i], strRecFolder) == false ){
			return false;
		}
	}
	wstring strPartialRecFolder;
	for( size_t i = 0; i < item.second.recSetting.partialRecFolder.size(); i++ ){
		if( SerializeRecFolder(item.second.recSetting.partialRecFolder[i], strPartialRecFolder) == false ){
			return false;
		}
	}
	Format(saveLine, L"%d\n%ls\n%ls\n%d\n%d\n%ls\n%ls\n%ls\n%d\n%d\n%d\n%d\n%d\n%ls\n%d\n%d\n%d\n%d\n%d\n%d\n%ls%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%ls%d\n%d\n",
		item.second.dataID,
		item.second.searchInfo.andKey.c_str(),
		item.second.searchInfo.notKey.c_str(),
		item.second.searchInfo.regExpFlag,
		item.second.searchInfo.titleOnlyFlag,
		strContent.c_str(),
		strDate.c_str(),
		strService.c_str(),
		item.second.recSetting.recMode,
		item.second.recSetting.priority,
		item.second.recSetting.tuijyuuFlag,
		item.second.recSetting.serviceMode,
		item.second.recSetting.pittariFlag,
		item.second.recSetting.batFilePath.c_str(),
		item.second.recSetting.suspendMode,
		item.second.recSetting.rebootFlag,
		item.second.recSetting.useMargineFlag,
		item.second.recSetting.startMargine,
		item.second.recSetting.endMargine,
		(int)item.second.recSetting.recFolderList.size(),
		strRecFolder.c_str(),
		item.second.recSetting.continueRecFlag,
		item.second.recSetting.partialRecFlag,
		item.second.recSetting.tunerID,
		item.second.searchInfo.aimaiFlag,
		item.second.searchInfo.notContetFlag,
		item.second.searchInfo.notDateFlag,
		item.second.searchInfo.freeCAFlag,
		(int)item.second.recSetting.partialRecFolder.size(),
		strPartialRecFolder.c_str(),
		item.second.searchInfo.chkRecEnd,
		item.second.searchInfo.chkRecDay
		);
	return FinalizeField(saveLine) == 30 + item.second.recSetting.recFolderList.size() + item.second.recSetting.partialRecFolder.size();
}

bool CParseEpgAutoAddText::SaveFooterLine(wstring& saveLine) const
{
	Format(saveLine, L";;NextID=%d", this->nextID);
	return this->saveNextID != 0;
}

bool CParseEpgAutoAddText::SelectItemToSave(vector<map<DWORD, EPG_AUTO_ADD_DATA>::const_iterator>& itemList) const
{
	if( this->itemMap.empty() == false && this->itemMap.rbegin()->first >= this->itemMap.begin()->first + 50000000 ){
		map<DWORD, EPG_AUTO_ADD_DATA>::const_iterator itr;
		for( itr = this->itemMap.upper_bound(50000000); itr != this->itemMap.end(); itr++ ){
			itemList.push_back(itr);
		}
		for( itr = this->itemMap.begin(); itr->first <= 50000000; itr++ ){
			itemList.push_back(itr);
		}
		return true;
	}
	return false;
}

DWORD CParseManualAutoAddText::AddData(const MANUAL_AUTO_ADD_DATA& item)
{
	map<DWORD, MANUAL_AUTO_ADD_DATA>::iterator itr = this->itemMap.insert(std::make_pair(this->nextID, item)).first;
	this->nextID = this->nextID % 100000000 + 1;
	itr->second.recSetting.pittariFlag = 0;
	itr->second.recSetting.tuijyuuFlag = 0;
	return itr->second.dataID = itr->first;
}

bool CParseManualAutoAddText::ChgData(const MANUAL_AUTO_ADD_DATA& item)
{
	map<DWORD, MANUAL_AUTO_ADD_DATA>::iterator itr = this->itemMap.find(item.dataID);
	if( itr != this->itemMap.end() ){
		itr->second = item;
		itr->second.recSetting.pittariFlag = 0;
		itr->second.recSetting.tuijyuuFlag = 0;
		return true;
	}
	return false;
}

bool CParseManualAutoAddText::DelData(DWORD id)
{
	return this->itemMap.erase(id) != 0;
}

bool CParseManualAutoAddText::ParseLine(LPCWSTR parseLine, pair<DWORD, MANUAL_AUTO_ADD_DATA>& item)
{
	if( this->saveNextID == 1 ){
		this->saveNextID = 0;
	}
	if( parseLine[0] == L';' ){
		if( wcsncmp(parseLine, L";;NextID=", 9) == 0 ){
			DWORD nextID_ = (DWORD)wcstol(parseLine + 9, NULL, 10);
			if( nextID_ != 0 && nextID_ <= 100000000 ){
				this->nextID = this->nextID > nextID_ + 50000000 ? nextID_ : max(nextID_, this->nextID);
			}
			this->saveNextID = 2;
		}
		return false;
	}else if( wcschr(parseLine, L'\t') == NULL ){
		return false;
	}
	LPCWSTR token[3] = {NULL, NULL, parseLine};

	item.second.dataID = item.first = (DWORD)NextTokenToInt(token);
	if( item.first == 0 || item.first > 100000000 ){
		return false;
	}
	item.second.dayOfWeekFlag = (BYTE)NextTokenToInt(token);
	item.second.startTime = (DWORD)NextTokenToInt(token);
	item.second.durationSecond = (DWORD)NextTokenToInt(token);
	NextToken(token);
	item.second.title.assign(token[0], token[1]);
	NextToken(token);
	item.second.stationName.assign(token[0], token[1]);
	item.second.originalNetworkID = (WORD)NextTokenToInt(token);
	item.second.transportStreamID = (WORD)NextTokenToInt(token);
	item.second.serviceID = (WORD)NextTokenToInt(token);
	item.second.recSetting.recMode = (BYTE)NextTokenToInt(token);
	item.second.recSetting.priority = (BYTE)NextTokenToInt(token);
	item.second.recSetting.tuijyuuFlag = NextTokenToInt(token) != 0;
	item.second.recSetting.serviceMode = (DWORD)NextTokenToInt(token);
	item.second.recSetting.pittariFlag = NextTokenToInt(token) != 0;
	NextToken(token);
	item.second.recSetting.batFilePath.assign(token[0], token[1]);
	item.second.recSetting.suspendMode = (BYTE)NextTokenToInt(token);
	item.second.recSetting.rebootFlag = NextTokenToInt(token) != 0;
	item.second.recSetting.useMargineFlag = NextTokenToInt(token) != 0;
	item.second.recSetting.startMargine = NextTokenToInt(token);
	item.second.recSetting.endMargine = NextTokenToInt(token);

	ParseRecFolderList(token, item.second.recSetting.recFolderList);
	item.second.recSetting.continueRecFlag = NextTokenToInt(token) != 0;
	item.second.recSetting.partialRecFlag = NextTokenToInt(token) != 0;
	item.second.recSetting.tunerID = (DWORD)NextTokenToInt(token);

	ParseRecFolderList(token, item.second.recSetting.partialRecFolder);
	this->nextID = this->nextID > item.first + 50000000 ? item.first + 1 : (max(item.first + 1, this->nextID) - 1) % 100000000 + 1;
	return true;
}

bool CParseManualAutoAddText::SaveLine(const pair<DWORD, MANUAL_AUTO_ADD_DATA>& item, wstring& saveLine) const
{
	wstring strRecFolder;
	for( size_t i = 0; i < item.second.recSetting.recFolderList.size(); i++ ){
		if( SerializeRecFolder(item.second.recSetting.recFolderList[i], strRecFolder) == false ){
			return false;
		}
	}
	wstring strPartialRecFolder;
	for( size_t i = 0; i < item.second.recSetting.partialRecFolder.size(); i++ ){
		if( SerializeRecFolder(item.second.recSetting.partialRecFolder[i], strPartialRecFolder) == false ){
			return false;
		}
	}
	Format(saveLine, L"%d\n%d\n%d\n%d\n%ls\n%ls\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%ls\n%d\n%d\n%d\n%d\n%d\n%d\n%ls%d\n%d\n%d\n%d\n%ls",
		item.second.dataID,
		item.second.dayOfWeekFlag,
		item.second.startTime,
		item.second.durationSecond,
		item.second.title.c_str(),
		item.second.stationName.c_str(),
		item.second.originalNetworkID,
		item.second.transportStreamID,
		item.second.serviceID,
		item.second.recSetting.recMode,
		item.second.recSetting.priority,
		item.second.recSetting.tuijyuuFlag,
		item.second.recSetting.serviceMode,
		item.second.recSetting.pittariFlag,
		item.second.recSetting.batFilePath.c_str(),
		item.second.recSetting.suspendMode,
		item.second.recSetting.rebootFlag,
		item.second.recSetting.useMargineFlag,
		item.second.recSetting.startMargine,
		item.second.recSetting.endMargine,
		(int)item.second.recSetting.recFolderList.size(),
		strRecFolder.c_str(),
		item.second.recSetting.continueRecFlag,
		item.second.recSetting.partialRecFlag,
		item.second.recSetting.tunerID,
		(int)item.second.recSetting.partialRecFolder.size(),
		strPartialRecFolder.c_str()
		);
	return FinalizeField(saveLine) == 25 + item.second.recSetting.recFolderList.size() + item.second.recSetting.partialRecFolder.size();
}

bool CParseManualAutoAddText::SaveFooterLine(wstring& saveLine) const
{
	Format(saveLine, L";;NextID=%d", this->nextID);
	return this->saveNextID != 0;
}

bool CParseManualAutoAddText::SelectItemToSave(vector<map<DWORD, MANUAL_AUTO_ADD_DATA>::const_iterator>& itemList) const
{
	if( this->itemMap.empty() == false && this->itemMap.rbegin()->first >= this->itemMap.begin()->first + 50000000 ){
		map<DWORD, MANUAL_AUTO_ADD_DATA>::const_iterator itr;
		for( itr = this->itemMap.upper_bound(50000000); itr != this->itemMap.end(); itr++ ){
			itemList.push_back(itr);
		}
		for( itr = this->itemMap.begin(); itr->first <= 50000000; itr++ ){
			itemList.push_back(itr);
		}
		return true;
	}
	return false;
}
