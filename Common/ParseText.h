#pragma once

#include "PathUtil.h"
#include "StringUtil.h"
#include "ThreadUtil.h"

template <class K, class V>
class CParseText
{
public:
	CParseText() : isUtf8(true) {}
	bool ParseText(LPCWSTR path = NULL);
	const map<K, V>& GetMap() const { return this->itemMap; }
	const wstring& GetFilePath() const { return this->filePath; }
protected:
	typedef CParseText<K, V> Base;
	bool SaveText(string* saveToStr = NULL) const;
	virtual bool ParseLine(LPCWSTR parseLine, pair<K, V>& item) = 0;
	virtual bool SaveLine(const pair<K, V>& item, wstring& saveLine) const { (void)item; (void)saveLine; return false; }
	virtual bool SaveFooterLine(wstring& saveLine) const { (void)saveLine; return false; }
	virtual bool SelectItemToSave(vector<typename map<K, V>::const_iterator>& itemList) const { (void)itemList; return false; }
	map<K, V> itemMap;
	wstring filePath;
	bool isUtf8;
};

template <class K, class V>
bool CParseText<K, V>::ParseText(LPCWSTR path)
{
	this->itemMap.clear();
	this->isUtf8 = true;
	if( path != NULL ){
		this->filePath = path;
	}
	if( this->filePath.empty() ){
		return false;
	}
	std::unique_ptr<FILE, fclose_deleter> fp;
	for( int retry = 0;; ){
		bool mightExist = false;
		fp.reset(UtilOpenFile(this->filePath, UTIL_SECURE_READ));
		if( fp ){
			break;
		}else if( UtilFileExists(this->filePath, &mightExist).first == false && mightExist == false ){
			return true;
		}else if( ++retry > 5 ){
			//6回トライしてそれでもダメなら失敗
			AddDebugLog(L"CParseText<>::ParseText(): Error: Cannot open file");
			return false;
		}
		SleepForMsec(200 * retry);
	}

	this->isUtf8 = false;
	bool checkBom = false;
	vector<char> buf;
	vector<WCHAR> parseBuf;
	for(;;){
		//4KB単位で読み込む
		buf.resize(buf.size() + 4096);
		size_t n = fread(&buf.front() + buf.size() - 4096, 1, 4096, fp.get());
		if( n == 0 ){
			buf.resize(buf.size() - 4096);
			buf.push_back('\0');
		}else{
			buf.resize(buf.size() - 4096 + n);
		}
		if( checkBom == false ){
			checkBom = true;
			if( buf.size() >= 3 && buf[0] == '\xEF' && buf[1] == '\xBB' && buf[2] == '\xBF' ){
				this->isUtf8 = true;
				buf.erase(buf.begin(), buf.begin() + 3);
			}
		}
		//完全に読み込まれた行をできるだけ解析
		size_t offset = 0;
		for( size_t i = 0; i < buf.size(); i++ ){
			char c = buf[i];
			if( c == '\0' || c == '\n' || (c == '\r' && i + 1 < buf.size() && buf[i + 1] == '\n') ){
				buf[i] = '\0';
				if( this->isUtf8 ){
					UTF8toW(&buf[offset], i - offset, parseBuf);
				}else{
					AtoW(&buf[offset], i - offset, parseBuf);
				}
				pair<K, V> item;
				if( ParseLine(&parseBuf.front(), item) ){
					this->itemMap.insert(std::move(item));
				}
				if( c == '\0' ){
					offset = i;
					break;
				}
				if( c == '\r' ){
					++i;
				}
				offset = i + 1;
			}
		}
		buf.erase(buf.begin(), buf.begin() + offset);
		if( buf.empty() == false && buf[0] == '\0' ){
			break;
		}
	}
	return true;
}

template <class K, class V>
bool CParseText<K, V>::SaveText(string* saveToStr) const
{
	if( this->filePath.empty() ){
		return false;
	}
	std::unique_ptr<FILE, fclose_deleter> fp;
	if( saveToStr ){
		saveToStr->clear();
	}else{
		for( int retry = 0;; ){
			UtilCreateDirectories(fs_path(this->filePath).parent_path());
			fp.reset(UtilOpenFile(this->filePath + L".tmp", UTIL_SECURE_WRITE));
			if( fp ){
				break;
			}else if( ++retry > 5 ){
				AddDebugLog(L"CParseText<>::SaveText(): Error: Cannot open file");
				return false;
			}
			SleepForMsec(200 * retry);
		}
	}

	bool ret = true;
	if( this->isUtf8 ){
		if( saveToStr ){
			saveToStr->append("\xEF\xBB\xBF");
		}else{
			ret = ret && fputs("\xEF\xBB\xBF", fp.get()) >= 0;
		}
	}
	wstring saveLine;
	vector<char> saveBuf;
	vector<typename map<K, V>::const_iterator> itemList;
	if( SelectItemToSave(itemList) ){
		for( size_t i = 0; i < itemList.size(); i++ ){
			saveLine.clear();
			if( SaveLine(*itemList[i], saveLine) ){
				saveLine += UTIL_NEWLINE;
				size_t len;
				if( this->isUtf8 ){
					len = WtoUTF8(saveLine.c_str(), saveLine.size(), saveBuf);
				}else{
					len = WtoA(saveLine.c_str(), saveLine.size(), saveBuf);
				}
				if( saveToStr ){
					saveToStr->append(saveBuf.data(), saveBuf.data() + len);
				}else{
					ret = ret && fwrite(saveBuf.data(), 1, len, fp.get()) == len;
				}
			}
		}
	}else{
		for( auto itr = this->itemMap.cbegin(); itr != this->itemMap.end(); itr++ ){
			saveLine.clear();
			if( SaveLine(*itr, saveLine) ){
				saveLine += UTIL_NEWLINE;
				size_t len;
				if( this->isUtf8 ){
					len = WtoUTF8(saveLine.c_str(), saveLine.size(), saveBuf);
				}else{
					len = WtoA(saveLine.c_str(), saveLine.size(), saveBuf);
				}
				if( saveToStr ){
					saveToStr->append(saveBuf.data(), saveBuf.data() + len);
				}else{
					ret = ret && fwrite(saveBuf.data(), 1, len, fp.get()) == len;
				}
			}
		}
	}
	saveLine.clear();
	if( SaveFooterLine(saveLine) ){
		saveLine += UTIL_NEWLINE;
		size_t len;
		if( this->isUtf8 ){
			len = WtoUTF8(saveLine.c_str(), saveLine.size(), saveBuf);
		}else{
			len = WtoA(saveLine.c_str(), saveLine.size(), saveBuf);
		}
		if( saveToStr ){
			saveToStr->append(saveBuf.data(), saveBuf.data() + len);
		}else{
			ret = ret && fwrite(saveBuf.data(), 1, len, fp.get()) == len;
		}
	}
	fp.reset();

	if( ret ){
		if( saveToStr ){
			return true;
		}
		for( int retry = 0;; ){
#ifdef _WIN32
			if( MoveFileEx((this->filePath + L".tmp").c_str(), this->filePath.c_str(), MOVEFILE_REPLACE_EXISTING) ){
#else
			string strPath;
			WtoUTF8(this->filePath, strPath);
			if( rename((strPath + ".tmp").c_str(), strPath.c_str()) == 0 ){
#endif
				return true;
			}else if( ++retry > 5 ){
				AddDebugLog(L"CParseText<>::SaveText(): Error: Cannot open file");
				break;
			}
			SleepForMsec(200 * retry);
		}
	}
	return false;
}
