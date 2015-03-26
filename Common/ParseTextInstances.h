#pragma once

#include "ParseText.h"
#include "StructDef.h"

//チャンネル情報ファイル「ChSet4.txt」の読み込みと保存処理を行う
//キーは読み込み順番号
class CParseChText4 : public CParseText<DWORD, CH_DATA4>
{
public:
	using CParseText<DWORD, CH_DATA4>::SaveText;
	//チャンネル情報の内部データを直接取得する。戻り値は次の非const操作まで有効
	vector<CH_DATA4*> GetChDataList();
	//チャンネル情報を追加する(失敗しない)。戻り値は追加されたキー
	DWORD AddCh(const CH_DATA4& item);
	//チャンネル情報を削除する
	void DelChService(int space, int ch, WORD serviceID);
protected:
	bool ParseLine(const wstring& parseLine, pair<DWORD, CH_DATA4>& item);
	bool SaveLine(const pair<DWORD, CH_DATA4>& item, wstring& saveLine) const;
	bool SelectIDToSave(vector<DWORD>& sortList) const;
};

//チャンネル情報ファイル「ChSet5.txt」の読み込みと保存処理を行う
//キーはONID<<32|TSID<<16|SID
class CParseChText5 : public CParseText<LONGLONG, CH_DATA5>
{
public:
	using CParseText<LONGLONG, CH_DATA5>::SaveText;
	LONGLONG AddCh(const CH_DATA5& item);
	//EPGデータの取得対象かを設定する
	bool SetEpgCapMode(WORD originalNetworkID, WORD transportStreamID, WORD serviceID, BOOL epgCapFlag);
protected:
	bool ParseLine(const wstring& parseLine, pair<LONGLONG, CH_DATA5>& item);
	bool SaveLine(const pair<LONGLONG, CH_DATA5>& item, wstring& saveLine) const;
	bool SelectIDToSave(vector<LONGLONG>& sortList) const;
};

//拡張子とContent-Typeの対応ファイル「ContentTypeText.txt」の読み込みを行う
class CParseContentTypeText : public CParseText<wstring, wstring>
{
public:
	void GetMimeType(wstring ext, wstring& mimeType) const;
protected:
	bool ParseLine(const wstring& parseLine, pair<wstring, wstring>& item);
};

//サービス名としょぼいカレンダー放送局名の対応ファイル「SyoboiCh.txt」の読み込みを行う
class CParseServiceChgText : public CParseText<wstring, wstring>
{
public:
	void ChgText(wstring& chgText) const;
protected:
	bool ParseLine(const wstring& parseLine, pair<wstring, wstring>& item);
};

//録画済み情報ファイル「RecInfo.txt」の読み込みと保存処理を行う
//キーはREC_FILE_INFO::id(非0)
class CParseRecInfoText : public CParseText<DWORD, REC_FILE_INFO>
{
public:
	CParseRecInfoText() : keepCount(UINT_MAX) {}
	using CParseText<DWORD, REC_FILE_INFO>::SaveText;
	//録画済み情報を追加する
	DWORD AddRecInfo(const REC_FILE_INFO& item);
	//録画済み情報を削除する
	bool DelRecInfo(DWORD id);
	//プロテクト情報を変更する
	bool ChgProtectRecInfo(DWORD id, BYTE flag);
	void GetProtectFiles(map<wstring, wstring>* fileMap) const;
	//AddRecInfo直後に残しておく非プロテクトの録画済み情報の個数を設定する
	void SetKeepCount(DWORD keepCount = UINT_MAX) { this->keepCount = keepCount; }
	void SetRecInfoDelFile(bool recInfoDelFile) { this->recInfoDelFile = recInfoDelFile; }
	void SetRecInfoFolder(LPCWSTR recInfoFolder) { ChkFolderPath(this->recInfoFolder = recInfoFolder); }
protected:
	bool ParseLine(const wstring& parseLine, pair<DWORD, REC_FILE_INFO>& item);
	bool SaveLine(const pair<DWORD, REC_FILE_INFO>& item, wstring& saveLine) const;
	bool SelectIDToSave(vector<DWORD>& sortList) const;
	//情報が追加される直前の補足作業
	void OnAddRecInfo(REC_FILE_INFO& item);
	//情報が削除される直前の補足作業
	void OnDelRecInfo(const REC_FILE_INFO& item);
	DWORD keepCount;
	bool recInfoDelFile;
	wstring recInfoFolder;
};

struct PARSE_REC_INFO2_ITEM
{
	WORD originalNetworkID;
	WORD transportStreamID;
	WORD serviceID;
	SYSTEMTIME startTime;
	wstring eventName;
};

//録画済みイベント情報ファイル「RecInfo2.txt」の読み込みと保存処理を行う
//キーは読み込み順番号
class CParseRecInfo2Text : public CParseText<DWORD, PARSE_REC_INFO2_ITEM>
{
public:
	CParseRecInfo2Text() : keepCount(UINT_MAX) {}
	using CParseText<DWORD, PARSE_REC_INFO2_ITEM>::SaveText;
	DWORD Add(const PARSE_REC_INFO2_ITEM& item);
	void SetKeepCount(DWORD keepCount = UINT_MAX) { this->keepCount = keepCount; }
protected:
	bool ParseLine(const wstring& parseLine, pair<DWORD, PARSE_REC_INFO2_ITEM>& item);
	bool SaveLine(const pair<DWORD, PARSE_REC_INFO2_ITEM>& item, wstring& saveLine) const;
	bool SelectIDToSave(vector<DWORD>& sortList) const;
	DWORD keepCount;
};

//予約情報ファイル「Reserve.txt」の読み込みと保存処理を行う
//キーはreserveID(非0,永続的)
class CParseReserveText : public CParseText<DWORD, RESERVE_DATA>
{
public:
	CParseReserveText() : nextID(1), saveNextID(1) {}
	using CParseText<DWORD, RESERVE_DATA>::SaveText;
	//予約情報を追加する
	DWORD AddReserve(const RESERVE_DATA& item);
	//予約情報を変更する
	bool ChgReserve(const RESERVE_DATA& item);
	//予約情報を削除する
	bool DelReserve(DWORD id);
	//録画開始日時でソートされた予約一覧を取得する
	vector<pair<LONGLONG, const RESERVE_DATA*>> GetReserveList(BOOL calcMargin = FALSE, int defStartMargin = 0) const;
protected:
	bool ParseLine(const wstring& parseLine, pair<DWORD, RESERVE_DATA>& item);
	bool SaveLine(const pair<DWORD, RESERVE_DATA>& item, wstring& saveLine) const;
	bool SaveFooterLine(wstring& saveLine) const;
	bool SelectIDToSave(vector<DWORD>& sortList) const;
	//過去に追加したIDよりも大きな値。100000000(1億)IDで巡回する(ただし1日に1000ID消費しても200年以上かかるので考えるだけ無駄)
	DWORD nextID;
	DWORD saveNextID;
};

//予約情報ファイル「EpgAutoAdd.txt」の読み込みと保存処理を行う
//キーはdataID(非0,永続的)
class CParseEpgAutoAddText : public CParseText<DWORD, EPG_AUTO_ADD_DATA>
{
public:
	CParseEpgAutoAddText() : nextID(1), saveNextID(1) {}
	using CParseText<DWORD, EPG_AUTO_ADD_DATA>::SaveText;
	DWORD AddData(const EPG_AUTO_ADD_DATA& item);
	bool ChgData(const EPG_AUTO_ADD_DATA& item);
	//予約登録数を変更する(イテレータに影響しない)
	bool SetAddCount(DWORD id, DWORD addCount);
	bool DelData(DWORD id);
protected:
	bool ParseLine(const wstring& parseLine, pair<DWORD, EPG_AUTO_ADD_DATA>& item);
	bool SaveLine(const pair<DWORD, EPG_AUTO_ADD_DATA>& item, wstring& saveLine) const;
	bool SaveFooterLine(wstring& saveLine) const;
	bool SelectIDToSave(vector<DWORD>& sortList) const;
	DWORD nextID;
	DWORD saveNextID;
};

//予約情報ファイル「ManualAutoAdd.txt」の読み込みと保存処理を行う
//キーはdataID(非0,永続的)
class CParseManualAutoAddText : public CParseText<DWORD, MANUAL_AUTO_ADD_DATA>
{
public:
	CParseManualAutoAddText() : nextID(1), saveNextID(1) {}
	using CParseText<DWORD, MANUAL_AUTO_ADD_DATA>::SaveText;
	DWORD AddData(const MANUAL_AUTO_ADD_DATA& item);
	bool ChgData(const MANUAL_AUTO_ADD_DATA& item);
	bool DelData(DWORD id);
protected:
	bool ParseLine(const wstring& parseLine, pair<DWORD, MANUAL_AUTO_ADD_DATA>& item);
	bool SaveLine(const pair<DWORD, MANUAL_AUTO_ADD_DATA>& item, wstring& saveLine) const;
	bool SaveFooterLine(wstring& saveLine) const;
	bool SelectIDToSave(vector<DWORD>& sortList) const;
	DWORD nextID;
	DWORD saveNextID;
};
