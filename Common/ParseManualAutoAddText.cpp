#include "StdAfx.h"
#include "ParseManualAutoAddText.h"
#include "TimeUtil.h"
#include "StringUtil.h"

CParseManualAutoAddText::CParseManualAutoAddText(void)
{
	this->loadFilePath = L"";
	this->nextID = 1;
}

CParseManualAutoAddText::~CParseManualAutoAddText(void)
{
	map<DWORD, MANUAL_AUTO_ADD_DATA*>::iterator itr;
	for( itr = this->dataIDMap.begin(); itr != this->dataIDMap.end(); itr++ ){
		SAFE_DELETE(itr->second)
	}
}

BOOL CParseManualAutoAddText::ParseText(LPCWSTR filePath)
{
	if( filePath == NULL ){
		return FALSE;
	}

	map<DWORD, MANUAL_AUTO_ADD_DATA*>::iterator itr;
	for( itr = this->dataIDMap.begin(); itr != this->dataIDMap.end(); itr++ ){
		SAFE_DELETE(itr->second)
	}
	this->dataIDMap.clear();
	this->nextID = 1;

	this->loadFilePath = filePath;

	HANDLE hFile = CreateFile( filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
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
				MANUAL_AUTO_ADD_DATA* item = new MANUAL_AUTO_ADD_DATA;
				BOOL bRet = Parse1Line(parseLine, item);
				if( bRet == FALSE ){
					SAFE_DELETE(item)
				}else{
					item->dataID = GetNextID();
					this->dataIDMap.insert( pair<DWORD, MANUAL_AUTO_ADD_DATA*>(item->dataID,item) );
				}
			}
		}
	}

	return TRUE;
}

BOOL CParseManualAutoAddText::Parse1Line(string parseLine, MANUAL_AUTO_ADD_DATA* item )
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
	//対象曜日
	item->dayOfWeekFlag = (BYTE)atoi(strBuff.c_str());

	Separate( parseLine, "\t", strBuff, parseLine);
	
	//録画開始時間（00:00を0として秒単位）
	item->startTime = (DWORD)atoi(strBuff.c_str());

	Separate( parseLine, "\t", strBuff, parseLine);

	//録画総時間
	item->durationSecond = (DWORD)atoi(strBuff.c_str());

	Separate( parseLine, "\t", strBuff, parseLine);

	//番組名
	AtoW(strBuff, item->title);

	Separate( parseLine, "\t", strBuff, parseLine);

	//サービス名
	AtoW(strBuff, item->stationName);

	Separate( parseLine, "\t", strBuff, parseLine);

	//ONID
	item->originalNetworkID = (WORD)atoi(strBuff.c_str());

	Separate( parseLine, "\t", strBuff, parseLine);

	//TSID
	item->transportStreamID = (WORD)atoi(strBuff.c_str());

	Separate( parseLine, "\t", strBuff, parseLine);

	//SID
	item->serviceID = (WORD)atoi(strBuff.c_str());

	Separate( parseLine, "\t", strBuff, parseLine);


	//録画設定
	//録画モード
	item->recSetting.recMode = (BYTE)atoi(strBuff.c_str());

	Separate( parseLine, "\t", strBuff, parseLine);

	//優先度
	item->recSetting.priority = (BYTE)atoi(strBuff.c_str());

	Separate( parseLine, "\t", strBuff, parseLine);

	//追従
	item->recSetting.tuijyuuFlag = 0;

	Separate( parseLine, "\t", strBuff, parseLine);

	//指定サービスモード
	item->recSetting.serviceMode = (DWORD)atoi(strBuff.c_str());
	
	Separate( parseLine, "\t", strBuff, parseLine);

	//ぴったり録画
	item->recSetting.pittariFlag = 0;
	
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

	return TRUE;
}

DWORD CParseManualAutoAddText::GetNextID()
{
	DWORD dataID = 0xFFFFFFFF;

	map<DWORD, MANUAL_AUTO_ADD_DATA*>::iterator itr;
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

BOOL CParseManualAutoAddText::SaveText(LPCWSTR filePath)
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

	HANDLE hFile = _CreateDirectoryAndFile( save_filePath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if( hFile == INVALID_HANDLE_VALUE ){
		return FALSE;
	}

	map<DWORD, MANUAL_AUTO_ADD_DATA*>::iterator itr;
	for( itr=this->dataIDMap.begin(); itr != this->dataIDMap.end(); itr++ ){
		string strWrite="";
		string strBuff="";
		DWORD dwWrite;
		//dataID
		Format(strBuff, "%d",itr->second->dataID);
		strWrite+=strBuff +"\t";
		//検索キー
		//対象曜日
		Format(strBuff, "%d",itr->second->dayOfWeekFlag);
		strWrite+=strBuff +"\t";
		//録画開始時間（00:00を0として秒単位）
		Format(strBuff, "%d",itr->second->startTime);
		strWrite+=strBuff +"\t";
		//録画総時間
		Format(strBuff, "%d",itr->second->durationSecond);
		strWrite+=strBuff +"\t";
		//番組名
		WtoA(itr->second->title, strBuff);
		strWrite+=strBuff;
		strWrite+="\t";
		//サービス名
		WtoA(itr->second->stationName, strBuff);
		strWrite+=strBuff;
		strWrite+="\t";
		//ONID
		Format(strBuff, "%d",itr->second->originalNetworkID);
		strWrite+=strBuff +"\t";
		//TSID
		Format(strBuff, "%d",itr->second->transportStreamID);
		strWrite+=strBuff +"\t";
		//SID
		Format(strBuff, "%d",itr->second->serviceID);
		strWrite+=strBuff +"\t";

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

BOOL CParseManualAutoAddText::AddData(MANUAL_AUTO_ADD_DATA* item)
{
	if( item == NULL ){
		return FALSE;
	}

	MANUAL_AUTO_ADD_DATA* setItem = new MANUAL_AUTO_ADD_DATA;
	*setItem = *item;
	setItem->recSetting.pittariFlag = 0;
	setItem->recSetting.tuijyuuFlag = 0;

	setItem->dataID = GetNextID();

	this->dataIDMap.insert( pair<DWORD, MANUAL_AUTO_ADD_DATA*>(setItem->dataID, setItem) );

	return TRUE;
}

BOOL CParseManualAutoAddText::ChgData(MANUAL_AUTO_ADD_DATA* item)
{
	if( item == NULL ){
		return FALSE;
	}

	map<DWORD, MANUAL_AUTO_ADD_DATA*>::iterator itr;
	itr = this->dataIDMap.find(item->dataID);
	if( itr == this->dataIDMap.end() ){
		return FALSE;
	}
	*(itr->second) = *item;
	itr->second->recSetting.pittariFlag = 0;
	itr->second->recSetting.tuijyuuFlag = 0;


	return TRUE;
}

BOOL CParseManualAutoAddText::DelData(DWORD dataID)
{
	map<DWORD, MANUAL_AUTO_ADD_DATA*>::iterator itr;
	itr = this->dataIDMap.find(dataID);
	if( itr != this->dataIDMap.end() ){
		this->dataIDMap.erase(itr);
	}

	return TRUE;
}
