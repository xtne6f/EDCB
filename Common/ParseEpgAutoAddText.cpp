#include "StdAfx.h"
#include "ParseEpgAutoAddText.h"
#include "TimeUtil.h"
#include "StringUtil.h"

CParseEpgAutoAddText::CParseEpgAutoAddText(void)
{
	this->loadFilePath = L"";
	this->nextID = 1;
}

CParseEpgAutoAddText::~CParseEpgAutoAddText(void)
{
	map<DWORD, EPG_AUTO_ADD_DATA*>::iterator itr;
	for( itr = this->dataIDMap.begin(); itr != this->dataIDMap.end(); itr++ ){
		SAFE_DELETE(itr->second)
	}
}

BOOL CParseEpgAutoAddText::ParseText(LPCWSTR filePath)
{
	if( filePath == NULL ){
		return FALSE;
	}

	map<DWORD, EPG_AUTO_ADD_DATA*>::iterator itr;
	for( itr = this->dataIDMap.begin(); itr != this->dataIDMap.end(); itr++ ){
		SAFE_DELETE(itr->second)
	}
	this->dataIDMap.clear();
	this->nextID = 1;

	this->loadFilePath = filePath;

	HANDLE hFile = _CreateFile2( filePath, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if( hFile == INVALID_HANDLE_VALUE ){
		return FALSE;
	}
	DWORD dwFileSize = GetFileSize( hFile, NULL );
	if( dwFileSize == 0 ){
		CloseHandle(hFile);
		return TRUE;
	}
	char* pszBuff = new char[dwFileSize+1];
	if( pszBuff == NULL ){
		CloseHandle(hFile);
		return FALSE;
	}
	ZeroMemory(pszBuff,dwFileSize+1);
	DWORD dwRead=0;
	ReadFile( hFile, pszBuff, dwFileSize, &dwRead, NULL );

	string strRead = pszBuff;

	CloseHandle(hFile);
	SAFE_DELETE_ARRAY(pszBuff)

	string parseLine="";
	size_t iIndex = 0;
	size_t iFind = 0;
	while( iFind != string::npos ){
		iFind = strRead.find("\r\n", iIndex);
		if( iFind == (int)string::npos ){
			parseLine = strRead.substr(iIndex);
			//strRead.clear();
		}else{
			parseLine = strRead.substr(iIndex,iFind-iIndex);
			//strRead.erase( 0, iIndex+2 );
			iIndex = iFind + 2;
		}
		//先頭；はコメント行
		if( parseLine.find(";") != 0 ){
			//空行？
			if( parseLine.find("\t") != string::npos ){
				EPG_AUTO_ADD_DATA* item = new EPG_AUTO_ADD_DATA;
				BOOL bRet = Parse1Line(parseLine, item);
				if( bRet == FALSE ){
					SAFE_DELETE(item)
				}else{
					item->dataID = GetNextID();
					this->dataIDMap.insert( pair<DWORD, EPG_AUTO_ADD_DATA*>(item->dataID,item) );
				}
			}
		}
	}

	return TRUE;
}

BOOL CParseEpgAutoAddText::Parse1Line(string parseLine, EPG_AUTO_ADD_DATA* item )
{
	if( parseLine.empty() == true || item == NULL ){
		return FALSE;
	}
	string strBuff="";

	Separate( parseLine, "\t", strBuff, parseLine);

	//dataID
	item->dataID = (DWORD)atoi(strBuff.c_str());

	Separate( parseLine, "\t", strBuff, parseLine);

	//検索キー
	//andキー
	AtoW(strBuff, item->searchInfo.andKey);

	Separate( parseLine, "\t", strBuff, parseLine);

	//notキー
	AtoW(strBuff, item->searchInfo.notKey);

	Separate( parseLine, "\t", strBuff, parseLine);

	//正規表現モード
	item->searchInfo.regExpFlag = (BOOL)atoi(strBuff.c_str());

	Separate( parseLine, "\t", strBuff, parseLine);

	//タイトルのみ
	item->searchInfo.titleOnlyFlag = (BOOL)atoi(strBuff.c_str());

	Separate( parseLine, "\t", strBuff, parseLine);

	//対象ジャンル
	if( strBuff.size() > 0 ){
		while(strBuff.find(",") != string::npos){
			string val = "";
			Separate( strBuff, ",", val, strBuff);

			DWORD flag = (DWORD)atoi(val.c_str());

			EPGDB_CONTENT_DATA addItem;
			addItem.content_nibble_level_1 = (BYTE)((flag&0xFF000000)>>24);
			addItem.content_nibble_level_2 = (BYTE)((flag&0x00FF0000)>>16);
			addItem.user_nibble_1 = (BYTE)((flag&0x0000FF00)>>8);
			addItem.user_nibble_2 = (BYTE)(flag&0x000000FF);

			item->searchInfo.contentList.push_back(addItem);
		}
		DWORD flag = (DWORD)atoi(strBuff.c_str());
		EPGDB_CONTENT_DATA addItem;
		addItem.content_nibble_level_1 = (BYTE)((flag&0xFF000000)>>24);
		addItem.content_nibble_level_2 = (BYTE)((flag&0x00FF0000)>>16);
		addItem.user_nibble_1 = (BYTE)((flag&0x0000FF00)>>8);
		addItem.user_nibble_2 = (BYTE)(flag&0x000000FF);

		item->searchInfo.contentList.push_back(addItem);
	}

	Separate( parseLine, "\t", strBuff, parseLine);

	//対象期間
	if( strBuff.size() > 0 ){
		while(strBuff.find(",") != string::npos){
			string val = "";
			Separate( strBuff, ",", val, strBuff);

			string weekS = "";
			string timeS = "";
			string weekE = "";
			string timeE = "";
			Separate( val, "-", weekS, val);
			Separate( val, "-", timeS, val);
			Separate( val, "-", weekE, timeE);

			DWORD timeStart = (DWORD)atoi(timeS.c_str());
			DWORD timeEnd = (DWORD)atoi(timeE.c_str());

			EPGDB_SEARCH_DATE_INFO addItem;
			addItem.startDayOfWeek = (BYTE)atoi(weekS.c_str());
			addItem.startHour = (WORD)((timeStart&0xFFFF0000)>>16);
			addItem.startMin = (WORD)(timeStart&0x0000FFFF);
			addItem.endDayOfWeek = (BYTE)atoi(weekE.c_str());
			addItem.endHour = (WORD)((timeEnd&0xFFFF0000)>>16);
			addItem.endMin = (WORD)(timeEnd&0x0000FFFF);

			item->searchInfo.dateList.push_back(addItem);
		}
		string weekS = "";
		string timeS = "";
		string weekE = "";
		string timeE = "";
		Separate( strBuff, "-", weekS, strBuff);
		Separate( strBuff, "-", timeS, strBuff);
		Separate( strBuff, "-", weekE, timeE);

		DWORD timeStart = (DWORD)atoi(timeS.c_str());
		DWORD timeEnd = (DWORD)atoi(timeE.c_str());

		EPGDB_SEARCH_DATE_INFO addItem;
		addItem.startDayOfWeek = (BYTE)atoi(weekS.c_str());
		addItem.startHour = (WORD)((timeStart&0xFFFF0000)>>16);
		addItem.startMin = (WORD)(timeStart&0x0000FFFF);
		addItem.endDayOfWeek = (BYTE)atoi(weekE.c_str());
		addItem.endHour = (WORD)((timeEnd&0xFFFF0000)>>16);
		addItem.endMin = (WORD)(timeEnd&0x0000FFFF);

		item->searchInfo.dateList.push_back(addItem);
	}

	Separate( parseLine, "\t", strBuff, parseLine);

	//対象サービス
	if( strBuff.size() > 0 ){
		while(strBuff.find(",") != string::npos){
			string val = "";
			Separate( strBuff, ",", val, strBuff);

			string strH;
			string strL;

			strH = val.substr(0,4);
			strL = val.substr(4);

			__int64 i64Ch=0;
			CHAR *endstr;
			i64Ch = ((__int64)strtol(strH.c_str(), &endstr, 16))<<32 |
				((__int64)strtol(strL.c_str(), &endstr, 16));

			item->searchInfo.serviceList.push_back(i64Ch);
		}
		string strH;
		string strL;

		strH = strBuff.substr(0,4);
		strL = strBuff.substr(4);

		__int64 i64Ch=0;
		CHAR *endstr;
		i64Ch = ((__int64)strtol(strH.c_str(), &endstr, 16))<<32 |
			((__int64)strtol(strL.c_str(), &endstr, 16));

		item->searchInfo.serviceList.push_back(i64Ch);
	}

	Separate( parseLine, "\t", strBuff, parseLine);

	//録画設定
	//録画モード
	item->recSetting.recMode = (BYTE)atoi(strBuff.c_str());

	Separate( parseLine, "\t", strBuff, parseLine);

	//優先度
	item->recSetting.priority = (BYTE)atoi(strBuff.c_str());

	Separate( parseLine, "\t", strBuff, parseLine);

	//追従
	item->recSetting.tuijyuuFlag = (BYTE)atoi(strBuff.c_str());

	Separate( parseLine, "\t", strBuff, parseLine);

	//指定サービスモード
	item->recSetting.serviceMode = (DWORD)atoi(strBuff.c_str());
	
	Separate( parseLine, "\t", strBuff, parseLine);

	//ぴったり録画
	item->recSetting.pittariFlag = (BYTE)atoi(strBuff.c_str());
	
	Separate( parseLine, "\t", strBuff, parseLine);

	//Bat処理
	AtoW(strBuff, item->recSetting.batFilePath);

	Separate( parseLine, "\t", strBuff, parseLine);

	//サスペンド
	if( strBuff.empty() == true ){
		item->recSetting.suspendMode = 4;
	}else{
		item->recSetting.suspendMode = (BYTE)atoi(strBuff.c_str());
	}
	
	Separate( parseLine, "\t", strBuff, parseLine);

	//再起動
	item->recSetting.rebootFlag = (BYTE)atoi(strBuff.c_str());
	
	Separate( parseLine, "\t", strBuff, parseLine);

	//個別マージン
	item->recSetting.useMargineFlag = (BYTE)atoi(strBuff.c_str());
	
	Separate( parseLine, "\t", strBuff, parseLine);

	//開始マージン
	item->recSetting.startMargine = (INT)atoi(strBuff.c_str());
	
	Separate( parseLine, "\t", strBuff, parseLine);

	//終了マージン
	item->recSetting.endMargine = (INT)atoi(strBuff.c_str());
	
	Separate( parseLine, "\t", strBuff, parseLine);

	//複数録画フォルダ
	DWORD recFolderNum = (DWORD)atoi(strBuff.c_str());

	for( DWORD i=0; i<recFolderNum; i++ ){
		Separate( parseLine, "\t", strBuff, parseLine);

		wstring wBuff = L"";
		AtoW(strBuff, wBuff);
		if( wBuff.size() > 0 ){
			wstring folder = L"";
			wstring plugin = L"";
			wstring recname = L"";
			Separate( wBuff, L"*", folder, wBuff);
			Separate( wBuff, L"*", plugin, recname);

			REC_FILE_SET_INFO folderItem;
			folderItem.recFolder = folder;
			if( plugin.size() == 0 ){
				folderItem.writePlugIn = L"Write_Default.dll";
			}else{
				folderItem.writePlugIn = plugin;
			}
			folderItem.writePlugIn = plugin;
			folderItem.recNamePlugIn = recname;

			item->recSetting.recFolderList.push_back(folderItem);
		}
	}

	Separate( parseLine, "\t", strBuff, parseLine);
	

	//連続録画
	item->recSetting.continueRecFlag = (BYTE)atoi(strBuff.c_str());
	
	Separate( parseLine, "\t", strBuff, parseLine);

	//部分受信同時
	item->recSetting.partialRecFlag = (BYTE)atoi(strBuff.c_str());
	
	Separate( parseLine, "\t", strBuff, parseLine);

	//強制的使用BonDrive
	item->recSetting.tunerID = (DWORD)atoi(strBuff.c_str());
	
	Separate( parseLine, "\t", strBuff, parseLine);

	//あいまい検索
	item->searchInfo.aimaiFlag = (BYTE)atoi(strBuff.c_str());
	
	Separate( parseLine, "\t", strBuff, parseLine);

	//ジャンルNOT扱い
	item->searchInfo.notContetFlag = (BYTE)atoi(strBuff.c_str());
	
	Separate( parseLine, "\t", strBuff, parseLine);

	//時間NOT扱い
	item->searchInfo.notDateFlag = (BYTE)atoi(strBuff.c_str());
	
	Separate( parseLine, "\t", strBuff, parseLine);

	//FreeCAモード
	item->searchInfo.freeCAFlag = (BYTE)atoi(strBuff.c_str());
	
	Separate( parseLine, "\t", strBuff, parseLine);

	//部分受信複数録画フォルダ
	DWORD recFolderNum2 = (DWORD)atoi(strBuff.c_str());

	for( DWORD i=0; i<recFolderNum2; i++ ){
		Separate( parseLine, "\t", strBuff, parseLine);

		wstring wBuff = L"";
		AtoW(strBuff, wBuff);
		if( wBuff.size() > 0 ){
			wstring folder = L"";
			wstring plugin = L"";
			wstring recname = L"";
			Separate( wBuff, L"*", folder, wBuff);
			Separate( wBuff, L"*", plugin, recname);

			REC_FILE_SET_INFO folderItem;
			folderItem.recFolder = folder;
			ChkFolderPath(folderItem.recFolder);
			if( plugin.size() == 0 ){
				folderItem.writePlugIn = L"Write_Default.dll";
			}else{
				folderItem.writePlugIn = plugin;
			}
			folderItem.recNamePlugIn = recname;
			item->recSetting.partialRecFolder.push_back(folderItem);
		}
	}

	Separate( parseLine, "\t", strBuff, parseLine);

	//録画済かのチェックあり
	item->searchInfo.chkRecEnd = (BYTE)atoi(strBuff.c_str());
	
	Separate( parseLine, "\t", strBuff, parseLine);

	//録画済かのチェック対象期間
	item->searchInfo.chkRecDay = (WORD)atoi(strBuff.c_str());
	
	Separate( parseLine, "\t", strBuff, parseLine);

	return TRUE;
}

DWORD CParseEpgAutoAddText::GetNextID()
{
	DWORD dataID = 0xFFFFFFFF;

	map<DWORD, EPG_AUTO_ADD_DATA*>::iterator itr;
	itr = this->dataIDMap.find(this->nextID);
	if( itr == this->dataIDMap.end() ){
		dataID = this->nextID;
		this->nextID++;
		if( this->nextID == 0 || this->nextID == 0xFFFFFFFF){
			this->nextID = 1;
		}
	}else{
		for( DWORD i=1; i<0xFFFFFFFF; i++ ){
			itr = this->dataIDMap.find(this->nextID);
			if( itr == this->dataIDMap.end() ){
				dataID = this->nextID;
				this->nextID++;
				if( this->nextID == 0 || this->nextID == 0xFFFFFFFF){
					this->nextID = 1;
				}
				break;
			}else{
				this->nextID++;
			}
			if( this->nextID == 0 || this->nextID == 0xFFFFFFFF){
				this->nextID = 1;
			}
		}
	}

	return dataID;
}

BOOL CParseEpgAutoAddText::SaveText(LPCWSTR filePath)
{
	if( this->loadFilePath.empty() == TRUE && filePath == NULL ){
		return FALSE;
	}

	wstring save_filePath = L"";
	if( filePath != NULL ){
		save_filePath = filePath;
	}else{
		save_filePath = this->loadFilePath;
	}

	HANDLE hFile = _CreateFile2( save_filePath.c_str(), GENERIC_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if( hFile == INVALID_HANDLE_VALUE ){
		return FALSE;
	}

	map<DWORD, EPG_AUTO_ADD_DATA*>::iterator itr;
	for( itr=this->dataIDMap.begin(); itr != this->dataIDMap.end(); itr++ ){
		string strWrite="";
		string strBuff="";
		DWORD dwWrite;
		//dataID
		Format(strBuff, "%d",itr->second->dataID);
		strWrite+=strBuff +"\t";
		//検索キー
		//andキー
		WtoA(itr->second->searchInfo.andKey, strBuff);
		strWrite+=strBuff;
		strWrite+="\t";
		//notキー
		WtoA(itr->second->searchInfo.notKey, strBuff);
		strWrite+=strBuff;
		strWrite+="\t";
		//正規表現モード
		Format(strBuff, "%d",itr->second->searchInfo.regExpFlag);
		strWrite+=strBuff +"\t";
		//タイトルのみ
		Format(strBuff, "%d",itr->second->searchInfo.titleOnlyFlag);
		strWrite+=strBuff +"\t";
		//対象ジャンル
		for(size_t i=0; i<itr->second->searchInfo.contentList.size(); i++ ){
			DWORD val = ((DWORD)itr->second->searchInfo.contentList[i].content_nibble_level_1) << 24 |
				((DWORD)itr->second->searchInfo.contentList[i].content_nibble_level_2) << 16 |
				((DWORD)itr->second->searchInfo.contentList[i].user_nibble_1) << 8 |
				((DWORD)itr->second->searchInfo.contentList[i].user_nibble_2);
			Format(strBuff, "%d",val);
			strWrite+=strBuff;
			if( i+1 < itr->second->searchInfo.contentList.size() ){
				strWrite+=",";
			}
		}
		strWrite+="\t";
		//対象期間
		for(size_t i=0; i<itr->second->searchInfo.dateList.size(); i++ ){
			DWORD start = ((DWORD)itr->second->searchInfo.dateList[i].startHour) << 16 |
				((DWORD)itr->second->searchInfo.dateList[i].startMin);
			DWORD end = ((DWORD)itr->second->searchInfo.dateList[i].endHour) << 16 |
				((DWORD)itr->second->searchInfo.dateList[i].endMin);

			Format(strBuff, "%d-%d-%d-%d",itr->second->searchInfo.dateList[i].startDayOfWeek, start,
				itr->second->searchInfo.dateList[i].endDayOfWeek, end);

			strWrite+=strBuff;
			if( i+1 < itr->second->searchInfo.dateList.size() ){
				strWrite+=",";
			}
		}
		strWrite+="\t";
		//対象サービス
		for(size_t i=0; i<itr->second->searchInfo.serviceList.size(); i++ ){
			DWORD dwH = 0;
			DWORD dwL = 0;
			dwH = (DWORD)(itr->second->searchInfo.serviceList[i]>>32);
			dwL = (DWORD)(itr->second->searchInfo.serviceList[i]&0xFFFFFFFF);
			Format(strBuff, "%04X%08X",dwH,dwL);
			strWrite+=strBuff;

			if( i+1 < itr->second->searchInfo.serviceList.size() ){
				strWrite+=",";
			}
		}
		strWrite+="\t";
		//録画設定
		//録画モード
		Format(strBuff,"%d",itr->second->recSetting.recMode);
		strWrite+=strBuff +"\t";
		//優先度
		Format(strBuff,"%d",itr->second->recSetting.priority);
		strWrite+=strBuff +"\t";
		//追従
		Format(strBuff,"%d",itr->second->recSetting.tuijyuuFlag);
		strWrite+=strBuff +"\t";
		//指定サービスモード
		Format(strBuff,"%d",itr->second->recSetting.serviceMode);
		strWrite+=strBuff +"\t";
		//ぴったり録画
		Format(strBuff,"%d",itr->second->recSetting.pittariFlag);
		strWrite+=strBuff +"\t";
		//Bat処理
		WtoA(itr->second->recSetting.batFilePath,strBuff);
		strWrite+=strBuff;
		strWrite+="\t";
		//サスペンド
		Format(strBuff,"%d",itr->second->recSetting.suspendMode);
		strWrite+=strBuff +"\t";
		//再起動
		Format(strBuff,"%d",itr->second->recSetting.rebootFlag);
		strWrite+=strBuff +"\t";
		//個別マージン
		Format(strBuff,"%d",itr->second->recSetting.useMargineFlag);
		strWrite+=strBuff +"\t";
		//開始マージン
		Format(strBuff,"%d",itr->second->recSetting.startMargine);
		strWrite+=strBuff +"\t";
		//終了マージン
		Format(strBuff,"%d",itr->second->recSetting.endMargine);
		strWrite+=strBuff +"\t";
		//複数録画フォルダ
		Format(strBuff,"%d",itr->second->recSetting.recFolderList.size());
		strWrite+=strBuff +"\t";
		for( size_t i=0; i<itr->second->recSetting.recFolderList.size(); i++ ){
			wstring path = L"";
			path = itr->second->recSetting.recFolderList[i].recFolder;
			path += L"*";
			path += itr->second->recSetting.recFolderList[i].writePlugIn;
			path += L"*";
			path += itr->second->recSetting.recFolderList[i].recNamePlugIn;
			WtoA(path, strBuff);
			strWrite+=strBuff +"\t";
		}
		//連続録画
		Format(strBuff,"%d",itr->second->recSetting.continueRecFlag);
		strWrite+=strBuff +"\t";
		//部分受信同時
		Format(strBuff,"%d",itr->second->recSetting.partialRecFlag);
		strWrite+=strBuff +"\t";
		//強制的使用BonDrive
		Format(strBuff,"%d",itr->second->recSetting.tunerID);
		strWrite+=strBuff +"\t";
		//あいまい検索
		Format(strBuff,"%d",itr->second->searchInfo.aimaiFlag);
		strWrite+=strBuff +"\t";
		//ジャンルNOT扱い
		Format(strBuff,"%d",itr->second->searchInfo.notContetFlag);
		strWrite+=strBuff +"\t";
		//時間NOT扱い
		Format(strBuff,"%d",itr->second->searchInfo.notDateFlag);
		strWrite+=strBuff +"\t";
		//FreeCAモード
		Format(strBuff,"%d",itr->second->searchInfo.freeCAFlag);
		strWrite+=strBuff +"\t";
		//部分受信サービス録画のフォルダ
		if( itr->second->recSetting.partialRecFolder.size() > 0 ){
			Format(strBuff,"%d",itr->second->recSetting.partialRecFolder.size());
			strWrite+=strBuff +"\t";
			for( size_t i=0; i<itr->second->recSetting.partialRecFolder.size(); i++ ){
				wstring path = L"";
				path = itr->second->recSetting.partialRecFolder[i].recFolder;
				path += L"*";
				path += itr->second->recSetting.partialRecFolder[i].writePlugIn;
				path += L"*";
				path += itr->second->recSetting.partialRecFolder[i].recNamePlugIn;
				WtoA(path, strBuff);
				strWrite+=strBuff +"\t";
			}
		}else{
			strWrite+="0\t";
		}
		//録画済かのチェックあり
		Format(strBuff,"%d",itr->second->searchInfo.chkRecEnd);
		strWrite+=strBuff +"\t";
		//録画済かのチェック対象期間
		Format(strBuff,"%d",itr->second->searchInfo.chkRecDay);
		strWrite+=strBuff +"\t";

		strWrite+="\r\n";
		WriteFile(hFile, strWrite.c_str(), (DWORD)strWrite.length(), &dwWrite, NULL);
	}

	CloseHandle(hFile);

	return TRUE;
}

BOOL CParseEpgAutoAddText::AddData(EPG_AUTO_ADD_DATA* item)
{
	if( item == NULL ){
		return FALSE;
	}

	item->dataID = GetNextID();
	EPG_AUTO_ADD_DATA* setItem = new EPG_AUTO_ADD_DATA;
	*setItem = *item;

	this->dataIDMap.insert( pair<DWORD, EPG_AUTO_ADD_DATA*>(setItem->dataID, setItem) );

	return TRUE;
}

BOOL CParseEpgAutoAddText::ChgData(EPG_AUTO_ADD_DATA* item)
{
	if( item == NULL ){
		return FALSE;
	}

	map<DWORD, EPG_AUTO_ADD_DATA*>::iterator itr;
	itr = this->dataIDMap.find(item->dataID);
	if( itr == this->dataIDMap.end() ){
		return FALSE;
	}
	*(itr->second) = *item;


	return TRUE;
}

BOOL CParseEpgAutoAddText::DelData(DWORD dataID)
{
	map<DWORD, EPG_AUTO_ADD_DATA*>::iterator itr;
	itr = this->dataIDMap.find(dataID);
	if( itr != this->dataIDMap.end() ){
		this->dataIDMap.erase(itr);
	}

	return TRUE;
}
