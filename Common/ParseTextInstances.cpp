#include "stdafx.h"
#include "ParseTextInstances.h"
#include "TimeUtil.h"
#include "PathUtil.h"

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

bool ParseDateTime(LPCWSTR* token, SYSTEMTIME& st, DWORD* duration = nullptr)
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
		list.emplace_back();
		list.back().recFolder.assign(token[0], token[1]);
	}
	for( vector<REC_FILE_SET_INFO>::iterator itr = list.begin(); itr != list.end(); ){
		if( itr->recFolder.empty() ){
			itr = list.erase(itr);
		}else{
			//US制御文字があればそれで分割、なければ*で分割
			LPCWSTR sep = itr->recFolder.find(L'\x1F') != wstring::npos ? L"\x1F" : L"*";
			Separate(itr->recFolder, sep, itr->recFolder, itr->writePlugIn);
			Separate(itr->writePlugIn, sep, itr->writePlugIn, itr->recNamePlugIn);
			if( itr->writePlugIn.empty() ){
				itr->writePlugIn = L"Write_Default" EDCB_LIB_EXT;
			}
			//互いに異なる環境でのEDCB_LIB_EXTはこの環境のEDCB_LIB_EXTに読みかえる
#ifdef _WIN32
			WCHAR ext[] = EDCB_UNIX_LIB_EXT;
#else
			WCHAR ext[] = EDCB_WIN_LIB_EXT;
#endif
			size_t extLen = array_size(ext) - 1;
			if( itr->writePlugIn.size() >= extLen && CompareNoCase(itr->writePlugIn.c_str() + itr->writePlugIn.size() - extLen, ext) == 0 ){
				itr->writePlugIn.replace(itr->writePlugIn.size() - extLen, extLen, EDCB_LIB_EXT);
			}
			if( itr->recNamePlugIn.size() >= extLen && CompareNoCase(itr->recNamePlugIn.c_str() + itr->recNamePlugIn.size() - extLen, ext) == 0 ){
				itr->recNamePlugIn.replace(itr->recNamePlugIn.size() - extLen, extLen, EDCB_LIB_EXT);
			}
			itr++;
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
		this->itemMap.emplace(this->itemMap.empty() ? 1 : this->itemMap.rbegin()->first + 1, item).first;
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

void CParseChText4::ParseLine(LPCWSTR parseLine)
{
	if( wcschr(parseLine, L'\t') == nullptr ){
		return;
	}
	LPCWSTR token[3] = {nullptr, nullptr, parseLine};
	CH_DATA4 item;

	NextToken(token);
	item.chName.assign(token[0], token[1]);
	NextToken(token);
	item.serviceName.assign(token[0], token[1]);
	NextToken(token);
	item.networkName.assign(token[0], token[1]);

	item.space = NextTokenToInt(token);
	item.ch = NextTokenToInt(token);
	item.originalNetworkID = (WORD)NextTokenToInt(token);
	item.transportStreamID = (WORD)NextTokenToInt(token);
	item.serviceID = (WORD)NextTokenToInt(token);
	item.serviceType = (WORD)NextTokenToInt(token);
	item.partialFlag = NextTokenToInt(token) != 0;
	item.useViewFlag = NextTokenToInt(token) != 0;
	item.remoconID = (BYTE)NextTokenToInt(token);
	this->itemMap.emplace(this->itemMap.empty() ? 1 : this->itemMap.rbegin()->first + 1, std::move(item));
}

bool CParseChText4::SaveLine(map<DWORD, CH_DATA4>::const_reference item, wstring& saveLine) const
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

void CParseChText5::ParseLine(LPCWSTR parseLine)
{
	if( wcschr(parseLine, L'\t') == nullptr ){
		return;
	}
	LPCWSTR token[3] = {nullptr, nullptr, parseLine};
	CH_DATA5 item;

	NextToken(token);
	item.serviceName.assign(token[0], token[1]);
	NextToken(token);
	item.networkName.assign(token[0], token[1]);

	item.originalNetworkID = (WORD)NextTokenToInt(token);
	item.transportStreamID = (WORD)NextTokenToInt(token);
	item.serviceID = (WORD)NextTokenToInt(token);
	item.serviceType = (WORD)NextTokenToInt(token);
	item.partialFlag = NextTokenToInt(token) != 0;
	item.epgCapFlag = NextTokenToInt(token) != 0;
	item.searchFlag = NextTokenToInt(token) != 0;
	item.remoconID = 0;
	LONGLONG key = (LONGLONG)item.originalNetworkID << 32 | (LONGLONG)item.transportStreamID << 16 | item.serviceID;
	if( this->itemMap.empty() ){
		this->parsedOrder.clear();
	}
	if( this->itemMap.count(key) == 0 ){
		this->parsedOrder.push_back(key);
	}
	this->itemMap.emplace(key, std::move(item));
}

bool CParseChText5::SaveLine(map<LONGLONG, CH_DATA5>::const_reference item, wstring& saveLine) const
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
		for( LONGLONG key : this->parsedOrder ){
			itemList.push_back(this->itemMap.find(key));
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

void CParseContentTypeText::ParseLine(LPCWSTR parseLine)
{
	if( wcschr(parseLine, L'\t') == nullptr || parseLine[0] == L';' ){
		//空行orコメント
		return;
	}
	LPCWSTR token[3] = {nullptr, nullptr, parseLine};

	NextToken(token);
	wstring key(token[0], token[1]);
	NextToken(token);
	this->itemMap.emplace(std::move(key), wstring(token[0], token[1]));
}

void CParseServiceChgText::ChgText(wstring& chgText) const
{
	map<wstring, wstring>::const_iterator itr = this->itemMap.find(chgText);
	if( itr != this->itemMap.end() ){
		chgText = itr->second;
	}
}

void CParseServiceChgText::ParseLine(LPCWSTR parseLine)
{
	if( wcschr(parseLine, L'\t') == nullptr || parseLine[0] == L';' ){
		return;
	}
	LPCWSTR token[3] = {nullptr, nullptr, parseLine};

	NextToken(token);
	wstring key(token[0], token[1]);
	NextToken(token);
	this->itemMap.emplace(std::move(key), wstring(token[0], token[1]));
}

DWORD CParseRecInfoText::AddRecInfo(const REC_FILE_INFO_BASIC& item)
{
	map<DWORD, REC_FILE_INFO_BASIC>::iterator itr = this->itemMap.emplace(this->nextID, item).first;
	this->nextID = this->nextID % 100000000 + 1;
	DWORD id = itr->second.id = itr->first;

	//非プロテクトの要素数がkeepCount以下になるまで削除
	if( this->keepCount < UINT_MAX ){
		size_t protectCount = std::count_if(this->itemMap.begin(), this->itemMap.end(),
			[](const pair<DWORD, REC_FILE_INFO_BASIC>& a) { return a.second.protectFlag != 0; });
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
	map<DWORD, REC_FILE_INFO_BASIC>::const_iterator itr = this->itemMap.find(id);
	if( itr != this->itemMap.end() && itr->second.protectFlag == 0 ){
		OnDelRecInfo(itr->second);
		this->itemMap.erase(itr);
		return true;
	}
	return false;
}

bool CParseRecInfoText::ChgPathRecInfo(DWORD id, LPCWSTR recFilePath)
{
	map<DWORD, REC_FILE_INFO_BASIC>::iterator itr = this->itemMap.find(id);
	if( itr != this->itemMap.end() ){
		itr->second.recFilePath = recFilePath;
		return true;
	}
	return false;
}

bool CParseRecInfoText::ChgProtectRecInfo(DWORD id, BYTE flag)
{
	map<DWORD, REC_FILE_INFO_BASIC>::iterator itr = this->itemMap.find(id);
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

void CParseRecInfoText::ParseLine(LPCWSTR parseLine)
{
	if( this->saveNextID == 1 ){
		this->saveNextID = 0;
	}
	if( parseLine[0] == L';' ){
		if( wcsncmp(parseLine, L";;NextID=", 9) == 0 ){
			DWORD nextID_ = (DWORD)wcstol(parseLine + 9, nullptr, 10);
			if( nextID_ != 0 && nextID_ <= 100000000 ){
				this->nextID = this->nextID > nextID_ + 50000000 ? nextID_ : max(nextID_, this->nextID);
			}
			this->saveNextID = 2;
		}
		return;
	}else if( wcschr(parseLine, L'\t') == nullptr ){
		return;
	}
	LPCWSTR token[3] = {nullptr, nullptr, parseLine};
	REC_FILE_INFO_BASIC item;

	NextToken(token);
	item.recFilePath.assign(token[0], token[1]);
	NextToken(token);
	item.title.assign(token[0], token[1]);

	if( ParseDateTime(token, item.startTime, &item.durationSecond) == false ){
		return;
	}
	NextToken(token);
	item.serviceName.assign(token[0], token[1]);
	item.originalNetworkID = (WORD)NextTokenToInt(token);
	item.transportStreamID = (WORD)NextTokenToInt(token);
	item.serviceID = (WORD)NextTokenToInt(token);
	item.eventID = (WORD)NextTokenToInt(token);
	item.drops = wcstoll(NextToken(token), nullptr, 10);
	item.scrambles = wcstoll(NextToken(token), nullptr, 10);
	item.recStatus = (DWORD)NextTokenToInt(token);

	if( ParseDateTime(token, item.startTimeEpg) == false ){
		return;
	}
	NextToken(token);
	item.protectFlag = NextTokenToInt(token) != 0;
	item.id = (DWORD)NextTokenToInt(token);
	if( item.id == 0 || item.id > 100000000 || this->itemMap.count(item.id) ){
		//新しいIDを与える
		item.id = this->nextID;
	}
	this->nextID = this->nextID > item.id + 50000000 ? item.id + 1 : (max(item.id + 1, this->nextID) - 1) % 100000000 + 1;
	this->itemMap.emplace(item.id, std::move(item));
}

bool CParseRecInfoText::SaveLine(map<DWORD, REC_FILE_INFO_BASIC>::const_reference item, wstring& saveLine) const
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

bool CParseRecInfoText::SelectItemToSave(vector<map<DWORD, REC_FILE_INFO_BASIC>::const_iterator>& itemList) const
{
	if( this->saveNextID != 0 ){
		if( this->itemMap.empty() == false && this->itemMap.rbegin()->first >= this->itemMap.begin()->first + 50000000 ){
			//ID巡回中
			map<DWORD, REC_FILE_INFO_BASIC>::const_iterator itr;
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
	for( map<DWORD, REC_FILE_INFO_BASIC>::const_iterator itr = this->itemMap.begin(); itr != this->itemMap.end(); itr++ ){
		itemList.push_back(itr);
	}
	std::sort(itemList.begin(), itemList.end(), [](map<DWORD, REC_FILE_INFO_BASIC>::const_iterator a, map<DWORD, REC_FILE_INFO_BASIC>::const_iterator b) -> bool {
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

void CParseRecInfoText::OnDelRecInfo(const REC_FILE_INFO_BASIC& item)
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
		for( const wstring& ext : this->customDelExt ){
			wstring delPath = fs_path(item.recFilePath).replace_extension().native();
			DeleteFile((delPath + ext).c_str());
			debug = (debug.empty() ? delPath + L'(' : debug + L'|') + ext;
		}
		if( debug.empty() == false ){
			AddDebugLogFormat(L"★RecInfo Auto Delete : %ls)", debug.c_str());
		}
		if( this->recInfoFolder.empty() == false ){
			//録画情報フォルダにも適用
			debug.clear();
			for( const wstring& ext : this->customDelExt ){
				wstring delPath = fs_path(this->recInfoFolder).append(fs_path(item.recFilePath).stem().native()).native();
				DeleteFile((delPath + ext).c_str());
				debug = (debug.empty() ? delPath + L'(' : debug + L'|') + ext;
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
	this->itemMap.emplace_back((DWORD)(this->itemMap.size() + 1), item);
	return this->itemMap.back().first;
}

void CParseRecInfo2Text::ParseLine(LPCWSTR parseLine)
{
	if( wcschr(parseLine, L'\t') == nullptr || parseLine[0] == L';' ){
		return;
	}
	LPCWSTR token[3] = {nullptr, nullptr, parseLine};
	PARSE_REC_INFO2_ITEM item;

	item.originalNetworkID = (WORD)NextTokenToInt(token);
	item.transportStreamID = (WORD)NextTokenToInt(token);
	item.serviceID = (WORD)NextTokenToInt(token);

	if( ParseDateTime(token, item.startTime) == false ){
		return;
	}
	NextToken(token);
	item.eventName.assign(token[0], token[1]);
	this->itemMap.emplace_back((DWORD)(this->itemMap.size() + 1), std::move(item));
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

bool CParseRecInfo2Text::SelectItemToSave(vector<vector<pair<DWORD, PARSE_REC_INFO2_ITEM>>::const_iterator>& itemList) const
{
	vector<pair<DWORD, PARSE_REC_INFO2_ITEM>>::const_iterator itr = this->itemMap.begin();
	if( this->itemMap.size() > this->keepCount ){
		itr += this->itemMap.size() - this->keepCount;
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
	map<DWORD, RESERVE_DATA>::iterator itr = this->itemMap.emplace(this->nextID, item).first;
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

void CParseReserveText::ParseLine(LPCWSTR parseLine)
{
	if( this->saveNextID == 1 ){
		this->saveNextID = 0;
	}
	if( parseLine[0] == L';' ){
		if( wcsncmp(parseLine, L";;NextID=", 9) == 0 ){
			DWORD nextID_ = (DWORD)wcstol(parseLine + 9, nullptr, 10);
			if( nextID_ != 0 && nextID_ <= 100000000 ){
				this->nextID = this->nextID > nextID_ + 50000000 ? nextID_ : max(nextID_, this->nextID);
			}
			this->saveNextID = 2;
		}
		return;
	}else if( wcschr(parseLine, L'\t') == nullptr ){
		return;
	}
	LPCWSTR token[3] = {nullptr, nullptr, parseLine};
	RESERVE_DATA item;

	if( ParseDateTime(token, item.startTime, &item.durationSecond) == false ){
		return;
	}
	NextToken(token);
	item.title.assign(token[0], token[1]);
	NextToken(token);
	item.stationName.assign(token[0], token[1]);
	item.originalNetworkID = (WORD)NextTokenToInt(token);
	item.transportStreamID = (WORD)NextTokenToInt(token);
	item.serviceID = (WORD)NextTokenToInt(token);
	item.eventID = (WORD)NextTokenToInt(token);
	item.recSetting.priority = (BYTE)NextTokenToInt(token);
	item.recSetting.tuijyuuFlag = NextTokenToInt(token) != 0 && item.eventID != 0xFFFF;
	item.reserveID = (DWORD)NextTokenToInt(token);
	if( item.reserveID == 0 || item.reserveID > 100000000 ){
		return;
	}
	item.recSetting.recMode = (BYTE)NextTokenToInt(token);
	item.recSetting.pittariFlag = NextTokenToInt(token) != 0 && item.eventID != 0xFFFF;
	NextToken(token);
	if( item.recSetting.batFilePath.assign(token[0], token[1]) == L"0" ){
		item.recSetting.batFilePath.clear();
	}
	//将来用
	NextToken(token);
	NextToken(token);
	item.comment.assign(token[0], token[1]);
	NextToken(token);
	//録画フォルダパスの最初の要素だけここにある
	item.recSetting.recFolderList.resize(1);
	item.recSetting.recFolderList[0].recFolder.assign(token[0], token[1]);
	item.recSetting.suspendMode = (BYTE)NextTokenToInt(token);
	item.recSetting.rebootFlag = NextTokenToInt(token) != 0;
	//廃止(旧recFilePath)
	NextToken(token);
	item.recSetting.useMargineFlag = NextTokenToInt(token) != 0;
	item.recSetting.startMargine = NextTokenToInt(token);
	item.recSetting.endMargine = NextTokenToInt(token);
	item.recSetting.serviceMode = (DWORD)NextTokenToInt(token);

	if( ParseDateTime(token, item.startTimeEpg) == false ){
		return;
	}
	ParseRecFolderList(token, item.recSetting.recFolderList);
	item.recSetting.continueRecFlag = NextTokenToInt(token) != 0;
	item.recSetting.partialRecFlag = NextTokenToInt(token) != 0;
	item.recSetting.tunerID = (DWORD)NextTokenToInt(token);
	item.reserveStatus = (DWORD)NextTokenToInt(token);

	ParseRecFolderList(token, item.recSetting.partialRecFolder);
	item.presentFlag = 0;
	item.overlapMode = 0;
	this->nextID = this->nextID > item.reserveID + 50000000 ? item.reserveID + 1 : (max(item.reserveID + 1, this->nextID) - 1) % 100000000 + 1;
	this->sortByEventCache.clear();
	this->itemMap.emplace(item.reserveID, std::move(item));
}

bool CParseReserveText::SaveLine(map<DWORD, RESERVE_DATA>::const_reference item, wstring& saveLine) const
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
		for( const auto& item : sortItemList ){
			itemList.push_back(this->itemMap.find(item.second->reserveID));
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
	for( const auto& item : this->itemMap ){
		LONGLONG startTime = ConvertI64Time(item.second.startTime);
		if( calcMargin != FALSE ){
			LONGLONG endTime = startTime + item.second.durationSecond * I64_1SEC;
			LONGLONG startMargin = defStartMargin * I64_1SEC;
			if( item.second.recSetting.useMargineFlag == TRUE ){
				startMargin = item.second.recSetting.startMargine * I64_1SEC;
			}
			//開始マージンは元の予約終了時刻を超えて負であってはならない
			startTime -= max(startMargin, startTime - endTime);
		}
		retList.emplace_back((startTime / I64_1SEC) << 16 | item.second.transportStreamID, &item.second);
	}
	std::sort(retList.begin(), retList.end());
	return retList;
}

const vector<pair<ULONGLONG, DWORD>>& CParseReserveText::GetSortByEventList() const
{
	if( this->sortByEventCache.empty() || this->itemMap.empty() ){
		this->sortByEventCache.clear();
		this->sortByEventCache.reserve(this->itemMap.size());
		for( const auto& item : this->itemMap ){
			this->sortByEventCache.emplace_back(
				(ULONGLONG)item.second.originalNetworkID << 48 | (ULONGLONG)item.second.transportStreamID << 32 |
				(DWORD)item.second.serviceID << 16 | item.second.eventID, item.first);
		}
		std::sort(this->sortByEventCache.begin(), this->sortByEventCache.end());
	}
	return this->sortByEventCache;
}

DWORD CParseEpgAutoAddText::AddData(const EPG_AUTO_ADD_DATA& item)
{
	map<DWORD, EPG_AUTO_ADD_DATA>::iterator itr = this->itemMap.emplace(this->nextID, item).first;
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

void CParseEpgAutoAddText::ParseLine(LPCWSTR parseLine)
{
	if( this->saveNextID == 1 ){
		this->saveNextID = 0;
	}
	if( parseLine[0] == L';' ){
		if( wcsncmp(parseLine, L";;NextID=", 9) == 0 ){
			DWORD nextID_ = (DWORD)wcstol(parseLine + 9, nullptr, 10);
			if( nextID_ != 0 && nextID_ <= 100000000 ){
				this->nextID = this->nextID > nextID_ + 50000000 ? nextID_ : max(nextID_, this->nextID);
			}
			this->saveNextID = 2;
		}
		return;
	}else if( wcschr(parseLine, L'\t') == nullptr ){
		return;
	}
	LPCWSTR token[3] = {nullptr, nullptr, parseLine};
	EPG_AUTO_ADD_DATA item;

	item.dataID = (DWORD)NextTokenToInt(token);
	if( item.dataID == 0 || item.dataID > 100000000 ){
		return;
	}
	NextToken(token);
	item.searchInfo.andKey.assign(token[0], token[1]);
	NextToken(token);
	item.searchInfo.notKey.assign(token[0], token[1]);
	item.searchInfo.regExpFlag = NextTokenToInt(token) != 0;
	item.searchInfo.titleOnlyFlag = NextTokenToInt(token) != 0;

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
			item.searchInfo.contentList.push_back(addItem);
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
			item.searchInfo.dateList.push_back(addItem);
		}
	}
	for( subToken[2] = NextToken(token); NextToken(subToken, L',') < token[1]; ){
		LPWSTR endp;
		LONGLONG llCh = wcstoll(subToken[0], &endp, 16);
		if( endp != subToken[0] && endp <= subToken[1] ){
			item.searchInfo.serviceList.push_back(llCh & 0xFFFFFFFFFFFFLL);
		}
	}
	item.recSetting.recMode = (BYTE)NextTokenToInt(token);
	item.recSetting.priority = (BYTE)NextTokenToInt(token);
	item.recSetting.tuijyuuFlag = NextTokenToInt(token) != 0;
	item.recSetting.serviceMode = (DWORD)NextTokenToInt(token);
	item.recSetting.pittariFlag = NextTokenToInt(token) != 0;
	NextToken(token);
	item.recSetting.batFilePath.assign(token[0], token[1]);
	item.recSetting.suspendMode = (BYTE)NextTokenToInt(token);
	item.recSetting.rebootFlag = NextTokenToInt(token) != 0;
	item.recSetting.useMargineFlag = NextTokenToInt(token) != 0;
	item.recSetting.startMargine = NextTokenToInt(token);
	item.recSetting.endMargine = NextTokenToInt(token);

	ParseRecFolderList(token, item.recSetting.recFolderList);
	item.recSetting.continueRecFlag = NextTokenToInt(token) != 0;
	item.recSetting.partialRecFlag = NextTokenToInt(token) != 0;
	item.recSetting.tunerID = (DWORD)NextTokenToInt(token);
	item.searchInfo.aimaiFlag = NextTokenToInt(token) != 0;
	item.searchInfo.notContetFlag = NextTokenToInt(token) != 0;
	item.searchInfo.notDateFlag = NextTokenToInt(token) != 0;
	item.searchInfo.freeCAFlag = (BYTE)NextTokenToInt(token);

	ParseRecFolderList(token, item.recSetting.partialRecFolder);
	item.searchInfo.chkRecEnd = NextTokenToInt(token) != 0;
	item.searchInfo.chkRecDay = (WORD)NextTokenToInt(token);
	item.addCount = 0;
	this->nextID = this->nextID > item.dataID + 50000000 ? item.dataID + 1 : (max(item.dataID + 1, this->nextID) - 1) % 100000000 + 1;
	this->itemMap.emplace(item.dataID, std::move(item));
}

bool CParseEpgAutoAddText::SaveLine(map<DWORD, EPG_AUTO_ADD_DATA>::const_reference item, wstring& saveLine) const
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
	map<DWORD, MANUAL_AUTO_ADD_DATA>::iterator itr = this->itemMap.emplace(this->nextID, item).first;
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

void CParseManualAutoAddText::ParseLine(LPCWSTR parseLine)
{
	if( this->saveNextID == 1 ){
		this->saveNextID = 0;
	}
	if( parseLine[0] == L';' ){
		if( wcsncmp(parseLine, L";;NextID=", 9) == 0 ){
			DWORD nextID_ = (DWORD)wcstol(parseLine + 9, nullptr, 10);
			if( nextID_ != 0 && nextID_ <= 100000000 ){
				this->nextID = this->nextID > nextID_ + 50000000 ? nextID_ : max(nextID_, this->nextID);
			}
			this->saveNextID = 2;
		}
		return;
	}else if( wcschr(parseLine, L'\t') == nullptr ){
		return;
	}
	LPCWSTR token[3] = {nullptr, nullptr, parseLine};
	MANUAL_AUTO_ADD_DATA item;

	item.dataID = (DWORD)NextTokenToInt(token);
	if( item.dataID == 0 || item.dataID > 100000000 ){
		return;
	}
	item.dayOfWeekFlag = (BYTE)NextTokenToInt(token);
	item.startTime = (DWORD)NextTokenToInt(token);
	item.durationSecond = (DWORD)NextTokenToInt(token);
	NextToken(token);
	item.title.assign(token[0], token[1]);
	NextToken(token);
	item.stationName.assign(token[0], token[1]);
	item.originalNetworkID = (WORD)NextTokenToInt(token);
	item.transportStreamID = (WORD)NextTokenToInt(token);
	item.serviceID = (WORD)NextTokenToInt(token);
	item.recSetting.recMode = (BYTE)NextTokenToInt(token);
	item.recSetting.priority = (BYTE)NextTokenToInt(token);
	item.recSetting.tuijyuuFlag = NextTokenToInt(token) != 0;
	item.recSetting.serviceMode = (DWORD)NextTokenToInt(token);
	item.recSetting.pittariFlag = NextTokenToInt(token) != 0;
	NextToken(token);
	item.recSetting.batFilePath.assign(token[0], token[1]);
	item.recSetting.suspendMode = (BYTE)NextTokenToInt(token);
	item.recSetting.rebootFlag = NextTokenToInt(token) != 0;
	item.recSetting.useMargineFlag = NextTokenToInt(token) != 0;
	item.recSetting.startMargine = NextTokenToInt(token);
	item.recSetting.endMargine = NextTokenToInt(token);

	ParseRecFolderList(token, item.recSetting.recFolderList);
	item.recSetting.continueRecFlag = NextTokenToInt(token) != 0;
	item.recSetting.partialRecFlag = NextTokenToInt(token) != 0;
	item.recSetting.tunerID = (DWORD)NextTokenToInt(token);

	ParseRecFolderList(token, item.recSetting.partialRecFolder);
	this->nextID = this->nextID > item.dataID + 50000000 ? item.dataID + 1 : (max(item.dataID + 1, this->nextID) - 1) % 100000000 + 1;
	this->itemMap.emplace(item.dataID, std::move(item));
}

bool CParseManualAutoAddText::SaveLine(map<DWORD, MANUAL_AUTO_ADD_DATA>::const_reference item, wstring& saveLine) const
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
