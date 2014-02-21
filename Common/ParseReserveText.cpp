#include "StdAfx.h"
#include "ParseReserveText.h"
#include "TimeUtil.h"
#include "StringUtil.h"

CParseReserveText::CParseReserveText(void)
{
	this->loadFilePath = L"";
	this->nextID = 1;
}

CParseReserveText::~CParseReserveText(void)
{
}

BOOL CParseReserveText::ParseReserveText(LPCWSTR filePath)
{
	if( filePath == NULL ){
		return FALSE;
	}

	this->reserveIDMap.clear();
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
				RESERVE_DATA item;
				BOOL bRet = Parse1Line(parseLine, &item);
				if( bRet != FALSE ){
					item.reserveID = GetNextReserveID();
					this->reserveIDMap.insert( pair<DWORD, RESERVE_DATA>(item.reserveID,item) );
				}
			}
		}
	}
	SaveReserveText();

	return TRUE;
}

BOOL CParseReserveText::AddParseReserveText(LPCWSTR filePath)
{
	if( this->loadFilePath.empty() == TRUE && filePath == NULL ){
		return FALSE;
	}
	wstring load_filePath = L"";
	if( filePath != NULL ){
		load_filePath = filePath;
	}else{
		load_filePath = this->loadFilePath;
	}
	if( this->reserveIDMap.size() == 0 ){
		BOOL bRet = ParseReserveText(load_filePath.c_str());
		return bRet;
	}

	HANDLE hFile = CreateFileW( load_filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
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
				RESERVE_DATA item;
				BOOL bRet = Parse1Line(parseLine, &item);
				if( bRet != FALSE && item.reserveID == 0 ){
					item.reserveID = GetNextReserveID();
					this->reserveIDMap.insert( pair<DWORD, RESERVE_DATA>(item.reserveID,item) );
				}
			}
		}
	}
	SaveReserveText();

	return TRUE;
}

BOOL CParseReserveText::Parse1Line(string parseLine, RESERVE_DATA* item )
{
	if( parseLine.empty() == true || item == NULL ){
		return FALSE;
	}
	string strBuff="";

	Separate( parseLine, "\t", strBuff, parseLine);

	//日付分解
	string strY;
	string strM;

	Separate( strBuff, "/", strY, strBuff);
	if( strY.empty() == false ){
		Separate( strBuff, "/", strM, strBuff);
		if( strM.empty() == false ){
			item->startTime.wYear = atoi(strY.c_str());
			item->startTime.wMonth = atoi(strM.c_str());
			item->startTime.wDay = atoi(strBuff.c_str());
		}else{
			return FALSE;
		}
	}else{
		return FALSE;
	}

	Separate( parseLine, "\t", strBuff, parseLine);

	//時間分解
	string strHour;
	string strMinute;

	Separate( strBuff, ":", strHour, strBuff);
	if( strHour.empty() == true ){
		return FALSE;
	}
	Separate( strBuff, ":", strMinute, strBuff);
	if( strHour.empty() == true ){
		return FALSE;
	}

	item->startTime.wHour = atoi(strHour.c_str());
	item->startTime.wMinute = atoi(strMinute.c_str());
	item->startTime.wSecond = atoi(strBuff.c_str());
	item->startTime.wMilliseconds = 0;

	Separate( parseLine, "\t", strBuff, parseLine);

	//総時間
	Separate( strBuff, ":", strHour, strBuff);
	if( strHour.empty() == true ){
		return FALSE;
	}
	Separate( strBuff, ":", strMinute, strBuff);
	if( strHour.empty() == true ){
		return FALSE;
	}

	item->durationSecond = atoi(strHour.c_str())*60*60 + atoi(strMinute.c_str())*60 + atoi(strBuff.c_str());

	Separate( parseLine, "\t", strBuff, parseLine);

	//タイトル
	AtoW(strBuff, item->title);

	Separate( parseLine, "\t", strBuff, parseLine);

	//放送局名
	AtoW(strBuff, item->stationName);

	Separate( parseLine, "\t", strBuff, parseLine);

	//ONID
	item->originalNetworkID = (WORD)atoi(strBuff.c_str());

	Separate( parseLine, "\t", strBuff, parseLine);

	//TSID
	item->transportStreamID = (WORD)atoi(strBuff.c_str());

	Separate( parseLine, "\t", strBuff, parseLine);
	
	//サービスＩＤ
	item->serviceID = (WORD)atoi(strBuff.c_str());

	Separate( parseLine, "\t", strBuff, parseLine);

	//イベントＩＤ
	item->eventID = (WORD)atoi(strBuff.c_str());

	Separate( parseLine, "\t", strBuff, parseLine);


	//優先度
	item->recSetting.priority = (BYTE)atoi(strBuff.c_str());

	Separate( parseLine, "\t", strBuff, parseLine);

	//追従
	item->recSetting.tuijyuuFlag = (BYTE)atoi(strBuff.c_str());
	if( item->eventID == 0xFFFF ){
		item->recSetting.tuijyuuFlag = 0;
	}

	Separate( parseLine, "\t", strBuff, parseLine);

	//ID
	item->reserveID = (DWORD)atoi(strBuff.c_str());
	
	Separate( parseLine, "\t", strBuff, parseLine);

	//録画モード
	item->recSetting.recMode = (BYTE)atoi(strBuff.c_str());

	Separate( parseLine, "\t", strBuff, parseLine);
	
	//将来用1　ぴったり録画
	item->recSetting.pittariFlag = (BYTE)atoi(strBuff.c_str());
	if( item->eventID == 0xFFFF ){
		item->recSetting.pittariFlag = 0;
	}
	
	Separate( parseLine, "\t", strBuff, parseLine);

	//将来用2　Bat処理
	if( strBuff.compare("0") == 0 ){
		item->recSetting.batFilePath = L"";
	}else{
		AtoW(strBuff, item->recSetting.batFilePath);
	}

	Separate( parseLine, "\t", strBuff, parseLine);
	//将来用3

	Separate( parseLine, "\t", strBuff, parseLine);

	//コメント部分
	if( strBuff.length() > 2 ){
		AtoW(strBuff, item->comment);
	}else{
		item->comment = L"";
	}

	Separate( parseLine, "\t", strBuff, parseLine);

	//録画フォルダ
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
		item->recSetting.recFolderList.push_back(folderItem);
	}

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

	//録画済みファイル
	AtoW(strBuff, item->recFilePath);
	
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

	//指定サービスモード
	item->recSetting.serviceMode = (DWORD)atoi(strBuff.c_str());
	
	Separate( parseLine, "\t", strBuff, parseLine);

	//予約登録時の開始時間
	//日付分解
	Separate( strBuff, "/", strY, strBuff);
	if( strY.empty() == false ){
		Separate( strBuff, "/", strM, strBuff);
		if( strM.empty() == false ){
			item->startTimeEpg.wYear = atoi(strY.c_str());
			item->startTimeEpg.wMonth = atoi(strM.c_str());
			item->startTimeEpg.wDay = atoi(strBuff.c_str());
		}else{
			return FALSE;
		}
	}else{
		return FALSE;
	}

	Separate( parseLine, "\t", strBuff, parseLine);

	//時間分解
	Separate( strBuff, ":", strHour, strBuff);
	if( strHour.empty() == true ){
		return FALSE;
	}
	Separate( strBuff, ":", strMinute, strBuff);
	if( strHour.empty() == true ){
		return FALSE;
	}

	item->startTimeEpg.wHour = atoi(strHour.c_str());
	item->startTimeEpg.wMinute = atoi(strMinute.c_str());
	item->startTimeEpg.wSecond = atoi(strBuff.c_str());
	item->startTimeEpg.wMilliseconds = 0;

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
			ChkFolderPath(folderItem.recFolder);
			if( plugin.size() == 0 ){
				folderItem.writePlugIn = L"Write_Default.dll";
			}else{
				folderItem.writePlugIn = plugin;
			}
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

	//予約追加状態
	item->reserveStatus = (DWORD)atoi(strBuff.c_str());
	
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

	item->recWaitFlag = FALSE;
	item->overlapMode = 0;

	return TRUE;
}

DWORD CParseReserveText::GetNextReserveID()
{
	DWORD reserveID = 0xFFFFFFFF;

	map<DWORD, RESERVE_DATA>::iterator itr;
	itr = this->reserveIDMap.find(this->nextID);
	if( itr == this->reserveIDMap.end() ){
		reserveID = this->nextID;
		this->nextID++;
		if( this->nextID == 0 || this->nextID == 0xFFFFFFFF){
			this->nextID = 1;
		}
	}else{
		for( DWORD i=1; i<0xFFFFFFFF; i++ ){
			itr = this->reserveIDMap.find(this->nextID);
			if( itr == this->reserveIDMap.end() ){
				reserveID = this->nextID;
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

	return reserveID;
}

BOOL CParseReserveText::SaveReserveText(LPCWSTR filePath)
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

	vector<pair<LONGLONG, const RESERVE_DATA*> > sortedList = GetReserveList();
	vector<pair<LONGLONG, const RESERVE_DATA*> >::iterator itr;
	for( itr = sortedList.begin(); itr != sortedList.end(); itr++ ){
		string strWrite="";
		string strBuff="";
		DWORD dwWrite;
		//日付
		Format(strBuff, "%04d/%02d/%02d",itr->second->startTime.wYear,itr->second->startTime.wMonth,itr->second->startTime.wDay);
		strWrite+=strBuff +"\t";
		//時間
		Format(strBuff,"%02d:%02d:%02d",itr->second->startTime.wHour,itr->second->startTime.wMinute,itr->second->startTime.wSecond);
		strWrite+=strBuff +"\t";
		//総時間
		Format(strBuff,"%02d:%02d:%02d",itr->second->durationSecond/(60*60),(itr->second->durationSecond%(60*60))/60,itr->second->durationSecond%60);
		strWrite+=strBuff +"\t";
		//タイトル
		WtoA(itr->second->title, strBuff);
		strWrite+=strBuff;
		strWrite+="\t";
		//放送局
		WtoA(itr->second->stationName, strBuff);
		strWrite+=strBuff;
		strWrite+="\t";
		//ONID
		Format(strBuff, "%d",itr->second->originalNetworkID);
		strWrite+=strBuff +"\t";
		//TSID
		Format(strBuff,"%d",itr->second->transportStreamID);
		strWrite+=strBuff +"\t";
		//ServiceID
		Format(strBuff,"%d",itr->second->serviceID);
		strWrite+=strBuff +"\t";
		//EventID
		Format(strBuff,"%d",itr->second->eventID);
		strWrite+=strBuff +"\t";
		//優先度
		Format(strBuff,"%d",itr->second->recSetting.priority);
		strWrite+=strBuff +"\t";
		//追従
		Format(strBuff,"%d",itr->second->recSetting.tuijyuuFlag);
		strWrite+=strBuff +"\t";
		//ID
		Format(strBuff,"%d",itr->second->reserveID);
		strWrite+=strBuff +"\t";
		//録画モード
		Format(strBuff,"%d",itr->second->recSetting.recMode);
		strWrite+=strBuff +"\t";
		//将来用1 ぴったり録画
		Format(strBuff,"%d",itr->second->recSetting.pittariFlag);
		strWrite+=strBuff +"\t";
		//将来用2 Bat処理
		if( itr->second->recSetting.batFilePath.empty() == true ){
			strWrite+="0\t";
		}else{
			WtoA(itr->second->recSetting.batFilePath,strBuff);
			strWrite+=strBuff;
			strWrite+="\t";
		}
		//将来用3
		strWrite+="0\t";
		//コメント
		if( itr->second->comment.length() > 0 ){
			WtoA(itr->second->comment,strBuff);
			strWrite+=strBuff;
		}
		strWrite+="\t";
		//録画フォルダ
		if( itr->second->recSetting.recFolderList.size() > 0 ){
			wstring path = L"";
			path = itr->second->recSetting.recFolderList[0].recFolder;
			path += L"*";
			path += itr->second->recSetting.recFolderList[0].writePlugIn;
			path += L"*";
			path += itr->second->recSetting.recFolderList[0].recNamePlugIn;
			WtoA(path, strBuff);
			strWrite+=strBuff;
		}
		strWrite+="\t";
		//サスペンド
		Format(strBuff,"%d",itr->second->recSetting.suspendMode);
		strWrite+=strBuff +"\t";
		//再起動
		Format(strBuff,"%d",itr->second->recSetting.rebootFlag);
		strWrite+=strBuff +"\t";
		//録画済ファイルパス
		WtoA(itr->second->recFilePath, strBuff);
		strWrite+=strBuff;
		strWrite+="\t";
		//個別マージン
		Format(strBuff,"%d",itr->second->recSetting.useMargineFlag);
		strWrite+=strBuff +"\t";
		//開始マージン
		Format(strBuff,"%d",itr->second->recSetting.startMargine);
		strWrite+=strBuff +"\t";
		//終了マージン
		Format(strBuff,"%d",itr->second->recSetting.endMargine);
		strWrite+=strBuff +"\t";
		//指定サービスモード
		Format(strBuff,"%d",itr->second->recSetting.serviceMode);
		strWrite+=strBuff +"\t";
		//予約登録時の開始時間
		//日付
		Format(strBuff, "%04d/%02d/%02d",itr->second->startTimeEpg.wYear,itr->second->startTimeEpg.wMonth,itr->second->startTimeEpg.wDay);
		strWrite+=strBuff +"\t";
		//時間
		Format(strBuff,"%02d:%02d:%02d",itr->second->startTimeEpg.wHour,itr->second->startTimeEpg.wMinute,itr->second->startTimeEpg.wSecond);
		strWrite+=strBuff +"\t";
		//複数録画フォルダ
		if( itr->second->recSetting.recFolderList.size() > 1 ){
			Format(strBuff,"%d",itr->second->recSetting.recFolderList.size()-1);
			strWrite+=strBuff +"\t";
			for( size_t i=1; i<itr->second->recSetting.recFolderList.size(); i++ ){
				wstring path = L"";
				path = itr->second->recSetting.recFolderList[i].recFolder;
				path += L"*";
				path += itr->second->recSetting.recFolderList[i].writePlugIn;
				path += L"*";
				path += itr->second->recSetting.recFolderList[i].recNamePlugIn;
				WtoA(path, strBuff);
				strWrite+=strBuff +"\t";
			}
		}else{
			strWrite+="0\t";
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
		//予約追加状態
		Format(strBuff,"%d",itr->second->reserveStatus);
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
		strWrite+="\r\n";
		WriteFile(hFile, strWrite.c_str(), (DWORD)strWrite.length(), &dwWrite, NULL);
	}

	CloseHandle(hFile);

	return TRUE;
}

BOOL CParseReserveText::AddReserve(RESERVE_DATA* item, DWORD* reserveID)
{
	if( item == NULL ){
		return FALSE;
	}
	SYSTEMTIME Now;
	GetLocalTime(&Now);

	SYSTEMTIME EndTime;
	GetI64Time(item->startTime, item->durationSecond, NULL, NULL, &EndTime);
	if( IsBigTime(Now, EndTime ) == TRUE){
		return FALSE;
	}

	DWORD reserveID_ = GetNextReserveID();
	if( reserveID != NULL ){
		*reserveID = reserveID_;
	}
	map<DWORD, RESERVE_DATA>::_Pairib ret = this->reserveIDMap.insert( pair<DWORD, RESERVE_DATA>(reserveID_, *item) );
	if( ret.second == false ){
		return FALSE;
	}
	ret.first->second.reserveID = reserveID_;
	return TRUE;
}

BOOL CParseReserveText::ChgReserve(RESERVE_DATA* item)
{
	if( item == NULL ){
		return FALSE;
	}

	map<DWORD, RESERVE_DATA>::iterator itr;
	itr = this->reserveIDMap.find(item->reserveID);
	if( itr == this->reserveIDMap.end() ){
		return FALSE;
	}
	itr->second = *item;

	return TRUE;
}

BOOL CParseReserveText::DelReserve(DWORD reserveID)
{
	this->reserveIDMap.erase(reserveID);

	return TRUE;
}

void CParseReserveText::SwapMap()
{
	map<DWORD, RESERVE_DATA>(this->reserveIDMap).swap(this->reserveIDMap);
}

vector<pair<LONGLONG, const RESERVE_DATA*> > CParseReserveText::GetReserveList(BOOL calcMargin, int defStartMargin) const
{
	vector<pair<LONGLONG, const RESERVE_DATA*> > retList;
	retList.reserve(this->reserveIDMap.size());

	//日時順にソート
	map<DWORD, RESERVE_DATA>::const_iterator itr;
	for( itr = this->reserveIDMap.begin(); itr != this->reserveIDMap.end(); itr++ ){
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
	sort(retList.begin(), retList.end());
	return retList;
}

vector<pair<DWORD, const RESERVE_DATA*> > CParseReserveText::GetReserveIDList() const
{
	vector<pair<DWORD, const RESERVE_DATA*> > retList;
	retList.reserve(this->reserveIDMap.size());

	map<DWORD, RESERVE_DATA>::const_iterator itr;
	for( itr = this->reserveIDMap.begin(); itr != this->reserveIDMap.end(); itr++ ){
		retList.push_back( pair<DWORD, const RESERVE_DATA*>(itr->first, &itr->second) );
	}
	return retList;
}

