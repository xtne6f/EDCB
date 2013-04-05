#pragma once

#include "../../Common/Util.h"
#include "../../Common/StructDef.h"
#include "../../Common/StringUtil.h"
#include "../../Common/PathUtil.h"

#import "RegExp.tlb" no_namespace named_guids

class CRecInfoDBManager
{
public:
	CRecInfoDBManager(void);
	~CRecInfoDBManager(void);

	void LoadRecInfo();
	void SaveRecInfo();

	void AddInfo(EPGDB_EVENT_INFO* info);

	BOOL IsFindTitleInfo(EPGDB_EVENT_INFO* info, WORD chkDay);
	void ReplaceRegExp(wstring &strBuff, wstring strReg, wstring strNew);
protected:
	typedef struct _RECINFO_LIST_ITEM{
		vector<EPGDB_EVENT_INFO*> infoList;
	}RECINFO_LIST_ITEM;

	HANDLE lockEvent;

	vector<EPGDB_EVENT_INFO*> recInfoList;
	map<wstring, RECINFO_LIST_ITEM*> titleMap;

	IRegExpPtr regExp;
protected:
	//PublicAPIîrëºêßå‰óp
	BOOL Lock(LPCWSTR log = NULL, DWORD timeOut = 60*1000);
	void UnLock(LPCWSTR log = NULL);

	void Clear();
	void CreateKeyMap();
};

