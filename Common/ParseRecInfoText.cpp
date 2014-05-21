#include "StdAfx.h"
#include "ParseRecInfoText.h"
#include "TimeUtil.h"
#include "StringUtil.h"
#include "PathUtil.h"

CParseRecInfoText::CParseRecInfoText(void)
{
	this->loadFilePath = L"";
	this->autoDelFlag = FALSE;
	this->keepCount = 30;
	this->nextID = 1;
}

CParseRecInfoText::~CParseRecInfoText(void)
{
	multimap<wstring, REC_FILE_INFO*>::iterator itr;
	for( itr = this->recInfoMap.begin(); itr != this->recInfoMap.end(); itr++ ){
		SAFE_DELETE(itr->second)
	}
}

BOOL CParseRecInfoText::ParseRecInfoText(LPCWSTR filePath)
{
	if( filePath == NULL ){
		return FALSE;
	}

	multimap<wstring, REC_FILE_INFO*>::iterator itr;
	for( itr = this->recInfoMap.begin(); itr != this->recInfoMap.end(); itr++ ){
		SAFE_DELETE(itr->second)
	}
	this->recInfoMap.clear();
	this->recIDMap.clear();
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
	SAFE_DELETE_ARRAY(pszBuff);

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
				REC_FILE_INFO* item = new REC_FILE_INFO;
				BOOL bRet = Parse1Line(parseLine, item);
				if( bRet == FALSE ){
					SAFE_DELETE(item)
				}else{
					wstring strKey;
					Format(strKey, L"%04d%02d%02d%02d%02d%02d%04X%04X",
							item->startTime.wYear,
							item->startTime.wMonth,
							item->startTime.wDay,
							item->startTime.wHour,
							item->startTime.wMinute,
							item->startTime.wSecond,
							item->originalNetworkID,
							item->transportStreamID);

					item->id = GetNextReserveID();
					this->recInfoMap.insert( pair<wstring, REC_FILE_INFO*>(strKey,item) );
					this->recIDMap.insert( pair<DWORD, REC_FILE_INFO*>(item->id,item) );
				}
			}
		}
	}

	return TRUE;
}

BOOL CParseRecInfoText::Parse1Line(string parseLine, REC_FILE_INFO* item )
{
	if( parseLine.empty() == true || item == NULL ){
		return FALSE;
	}
	string strBuff="";

	//録画ファイルパス
	Separate( parseLine, "\t", strBuff, parseLine);
	AtoW(strBuff, item->recFilePath);

	//番組名
	Separate( parseLine, "\t", strBuff, parseLine);
	AtoW(strBuff, item->title);

	//開始時間
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

	//時間分解
	Separate( parseLine, "\t", strBuff, parseLine);
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

	//総時間
	Separate( parseLine, "\t", strBuff, parseLine);
	Separate( strBuff, ":", strHour, strBuff);
	if( strHour.empty() == true ){
		return FALSE;
	}
	Separate( strBuff, ":", strMinute, strBuff);
	if( strHour.empty() == true ){
		return FALSE;
	}
	item->durationSecond = atoi(strHour.c_str())*60*60 + atoi(strMinute.c_str())*60 + atoi(strBuff.c_str());

	//サービス名
	Separate( parseLine, "\t", strBuff, parseLine);
	AtoW(strBuff, item->serviceName);

	//ONID
	Separate( parseLine, "\t", strBuff, parseLine);
	item->originalNetworkID = atoi(strBuff.c_str());

	//TSID
	Separate( parseLine, "\t", strBuff, parseLine);
	item->transportStreamID = atoi(strBuff.c_str());

	//SID
	Separate( parseLine, "\t", strBuff, parseLine);
	item->serviceID = atoi(strBuff.c_str());

	//EventID
	Separate( parseLine, "\t", strBuff, parseLine);
	item->eventID = atoi(strBuff.c_str());

	//ドロップ数
	Separate( parseLine, "\t", strBuff, parseLine);
	CHAR *endstr;
	item->drops = _strtoi64(strBuff.c_str(), &endstr, 10);

	//スクランブル数
	Separate( parseLine, "\t", strBuff, parseLine);
	item->scrambles = _strtoi64(strBuff.c_str(), &endstr, 10);

	//録画結果のステータス
	Separate( parseLine, "\t", strBuff, parseLine);
	item->recStatus = atoi(strBuff.c_str());

	//予約時の開始時間
	Separate( parseLine, "\t", strBuff, parseLine);
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

	//時間分解
	Separate( parseLine, "\t", strBuff, parseLine);
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

	//コメント
	Separate( parseLine, "\t", strBuff, parseLine);
	if( strBuff.length() > 2 ){
		AtoW(strBuff, item->comment);
	}else{
		item->comment = L"";
	}

	//プロテクト
	Separate( parseLine, "\t", strBuff, parseLine);
	item->protectFlag = atoi(strBuff.c_str());


	if( item->recFilePath.size() > 0 ){
		wstring iniCommonPath = L"";
		GetCommonIniPath(iniCommonPath);
		WCHAR buff[512] = L"";
		GetPrivateProfileString(L"SET", L"RecInfoFolder", L"", buff, 512, iniCommonPath.c_str());
		wstring infoFolder = buff;
		ChkFolderPath(infoFolder);

		wstring tsFileName = L"";
		GetFileName(item->recFilePath, tsFileName);

		wstring strInfoFile = item->recFilePath;
		strInfoFile += L".program.txt";
		HANDLE hFile = CreateFile( strInfoFile.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
		if( hFile == INVALID_HANDLE_VALUE && infoFolder.size() > 0){
			Format(strInfoFile, L"%s\\%s.program.txt", infoFolder.c_str(), tsFileName.c_str());
			hFile = CreateFile( strInfoFile.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
		}
		if( hFile != INVALID_HANDLE_VALUE ){
			DWORD dwFileSize = GetFileSize( hFile, NULL );
			if( dwFileSize != 0 ){
				char* pszBuff = new char[dwFileSize+1];
				if( pszBuff != NULL ){
					ZeroMemory(pszBuff,dwFileSize+1);
					DWORD dwRead=0;
					ReadFile( hFile, pszBuff, dwFileSize, &dwRead, NULL );

					string strRead = pszBuff;
					AtoW(strRead, item->programInfo);
				}
				SAFE_DELETE_ARRAY(pszBuff);
			}
			CloseHandle(hFile);
		}

		strInfoFile = item->recFilePath;
		strInfoFile += L".err";
		hFile = CreateFile( strInfoFile.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
		if( hFile == INVALID_HANDLE_VALUE && infoFolder.size() > 0){
			Format(strInfoFile, L"%s\\%s.err", infoFolder.c_str(), tsFileName.c_str());
			hFile = CreateFile( strInfoFile.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
		}
		if( hFile != INVALID_HANDLE_VALUE ){
			DWORD dwFileSize = GetFileSize( hFile, NULL );
			if( dwFileSize != 0 ){
				char* pszBuff = new char[dwFileSize+1];
				if( pszBuff != NULL ){
					ZeroMemory(pszBuff,dwFileSize+1);
					DWORD dwRead=0;
					ReadFile( hFile, pszBuff, dwFileSize, &dwRead, NULL );

					string strRead = pszBuff;
					AtoW(strRead, item->errInfo);
				}
				SAFE_DELETE_ARRAY(pszBuff);
			}
			CloseHandle(hFile);
		}
	}

	if( item->protectFlag == 1 ){
		if( item->recFilePath.size() > 0 ){
			AddPtotectFileList(item->recFilePath);
		}
	}

	return TRUE;
}

DWORD CParseRecInfoText::GetNextReserveID()
{
	DWORD reserveID = 0xFFFFFFFF;

	map<DWORD, REC_FILE_INFO*>::iterator itr;
	itr = this->recIDMap.find(this->nextID);
	if( itr == this->recIDMap.end() ){
		reserveID = this->nextID;
		this->nextID++;
		if( this->nextID == 0 || this->nextID == 0xFFFFFFFF){
			this->nextID = 1;
		}
	}else{
		for( DWORD i=1; i<0xFFFFFFFF; i++ ){
			itr = this->recIDMap.find(this->nextID);
			if( itr == this->recIDMap.end() ){
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

BOOL CParseRecInfoText::SaveRecInfoText(LPCWSTR filePath)
{
	if( this->loadFilePath.empty() == TRUE && filePath == NULL ){
		return FALSE;
	}

	wstring saveFilePath = L"";
	if( filePath != NULL ){
		saveFilePath = filePath;
	}else{
		saveFilePath = this->loadFilePath;
	}

	HANDLE hFile = _CreateDirectoryAndFile( saveFilePath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if( hFile == INVALID_HANDLE_VALUE ){
		return FALSE;
	}
	if( this->autoDelFlag == TRUE ){
		AutoDelInfo(this->keepCount);
	}
	multimap<wstring, REC_FILE_INFO*>::iterator itr;
	for( itr=this->recInfoMap.begin(); itr != this->recInfoMap.end(); itr++ ){
		string strWrite="";
		string strBuff="";
		DWORD dwWrite;
		//録画ファイルパス
		WtoA(itr->second->recFilePath, strBuff);
		strWrite+=strBuff;
		strWrite+="\t";
		//タイトル
		WtoA(itr->second->title, strBuff);
		strWrite+=strBuff;
		strWrite+="\t";
		//日付
		Format(strBuff, "%04d/%02d/%02d",itr->second->startTime.wYear,itr->second->startTime.wMonth,itr->second->startTime.wDay);
		strWrite+=strBuff +"\t";
		//時間
		Format(strBuff,"%02d:%02d:%02d",itr->second->startTime.wHour,itr->second->startTime.wMinute,itr->second->startTime.wSecond);
		strWrite+=strBuff +"\t";
		//録画時間
		Format(strBuff,"%02d:%02d:%02d",itr->second->durationSecond/(60*60),(itr->second->durationSecond%(60*60))/60,itr->second->durationSecond%60);
		strWrite+=strBuff +"\t";
		//サービス名
		WtoA(itr->second->serviceName, strBuff);
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
		//ドロップ数
		Format(strBuff,"%I64d",itr->second->drops);
		strWrite+=strBuff +"\t";
		//スクランブル数
		Format(strBuff,"%I64d",itr->second->scrambles);
		strWrite+=strBuff +"\t";
		//録画結果のステータス
		Format(strBuff,"%d",itr->second->recStatus);
		strWrite+=strBuff +"\t";
		//予約時の開始日付
		Format(strBuff, "%04d/%02d/%02d",itr->second->startTimeEpg.wYear,itr->second->startTimeEpg.wMonth,itr->second->startTimeEpg.wDay);
		strWrite+=strBuff +"\t";
		//予約時の開始時間
		Format(strBuff,"%02d:%02d:%02d",itr->second->startTimeEpg.wHour,itr->second->startTimeEpg.wMinute,itr->second->startTimeEpg.wSecond);
		strWrite+=strBuff +"\t";
		//コメント
		WtoA(itr->second->comment, strBuff);
		strWrite+=strBuff;
		strWrite+="\t";
		//プロテクト
		Format(strBuff,"%d",itr->second->protectFlag);
		strWrite+=strBuff +"\t";

		strWrite+="\r\n";
		WriteFile(hFile, strWrite.c_str(), (DWORD)strWrite.length(), &dwWrite, NULL);
	}

	CloseHandle(hFile);

	return TRUE;
}

BOOL CParseRecInfoText::AddRecInfo(REC_FILE_INFO* item)
{
	if( item == NULL ){
		return FALSE;
	}

	wstring strKey;
	Format(strKey, L"%04d%02d%02d%02d%02d%02d%04X%04X",
		item->startTime.wYear,
		item->startTime.wMonth,
		item->startTime.wDay,
		item->startTime.wHour,
		item->startTime.wMinute,
		item->startTime.wSecond,
		item->originalNetworkID,
		item->transportStreamID);

	wstring iniCommonPath = L"";
	GetCommonIniPath(iniCommonPath);
	WCHAR buff[512] = L"";
	GetPrivateProfileString(L"SET", L"RecInfoFolder", L"", buff, 512, iniCommonPath.c_str());
	wstring infoFolder = buff;
	ChkFolderPath(infoFolder);

	REC_FILE_INFO* pSetItem = new REC_FILE_INFO;
	*pSetItem = *item;

	wstring tsFileName = L"";
	GetFileName(pSetItem->recFilePath, tsFileName);

	wstring errFile = pSetItem->recFilePath;
	errFile += L".err";
	HANDLE file = CreateFile( errFile.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if( file != INVALID_HANDLE_VALUE ){
		DWORD dwFileSize = GetFileSize( file, NULL );
		if( dwFileSize != 0 ){
			char* pszBuff = new char[dwFileSize+1];
			if( pszBuff != NULL ){
				ZeroMemory(pszBuff,dwFileSize+1);
				DWORD dwRead=0;
				ReadFile( file, pszBuff, dwFileSize, &dwRead, NULL );

				string strRead = pszBuff;
				AtoW(strRead, pSetItem->errInfo);
			}
			SAFE_DELETE_ARRAY(pszBuff);
		}
		CloseHandle(file);
	}else{
		if( infoFolder.size() > 0 ){
			Format(errFile, L"%s\\%s.err", infoFolder.c_str(), tsFileName.c_str());
			file = CreateFile( errFile.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
			if( file != INVALID_HANDLE_VALUE ){
				DWORD dwFileSize = GetFileSize( file, NULL );
				if( dwFileSize != 0 ){
					char* pszBuff = new char[dwFileSize+1];
					if( pszBuff != NULL ){
						ZeroMemory(pszBuff,dwFileSize+1);
						DWORD dwRead=0;
						ReadFile( file, pszBuff, dwFileSize, &dwRead, NULL );

						string strRead = pszBuff;
						AtoW(strRead, pSetItem->errInfo);
					}
					SAFE_DELETE_ARRAY(pszBuff);
				}
				CloseHandle(file);
			}
		}
	}

	wstring pgFile = pSetItem->recFilePath;
	pgFile += L".program.txt";
	file = CreateFile( pgFile.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if( file != INVALID_HANDLE_VALUE ){
		DWORD dwFileSize = GetFileSize( file, NULL );
		if( dwFileSize != 0 ){
			char* pszBuff = new char[dwFileSize+1];
			if( pszBuff != NULL ){
				ZeroMemory(pszBuff,dwFileSize+1);
				DWORD dwRead=0;
				ReadFile( file, pszBuff, dwFileSize, &dwRead, NULL );

				string strRead = pszBuff;
				AtoW(strRead, pSetItem->programInfo);
			}
			SAFE_DELETE_ARRAY(pszBuff);
		}
		CloseHandle(file);
	}else{
		if( infoFolder.size() > 0 ){
			Format(pgFile, L"%s\\%s.program.txt", infoFolder.c_str(), tsFileName.c_str());
			file = CreateFile( pgFile.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
			if( file != INVALID_HANDLE_VALUE ){
				DWORD dwFileSize = GetFileSize( file, NULL );
				if( dwFileSize != 0 ){
					char* pszBuff = new char[dwFileSize+1];
					if( pszBuff != NULL ){
						ZeroMemory(pszBuff,dwFileSize+1);
						DWORD dwRead=0;
						ReadFile( file, pszBuff, dwFileSize, &dwRead, NULL );

						string strRead = pszBuff;
						AtoW(strRead, pSetItem->programInfo);
					}
					SAFE_DELETE_ARRAY(pszBuff);
				}
				CloseHandle(file);
			}
		}
	}

	if( item->protectFlag == 1 ){
		if( item->recFilePath.size() > 0 ){
			AddPtotectFileList(item->recFilePath);
		}
	}

	pSetItem->id = GetNextReserveID();
	this->recInfoMap.insert( pair<wstring, REC_FILE_INFO*>(strKey,pSetItem) );
	this->recIDMap.insert( pair<DWORD, REC_FILE_INFO*>(pSetItem->id,pSetItem) );

	if( this->autoDelFlag == TRUE ){
		AutoDelInfo(this->keepCount);
	}

	return TRUE;
}

BOOL CParseRecInfoText::DelRecInfo(DWORD id)
{
	multimap<wstring, REC_FILE_INFO*>::iterator itr;
	for( itr = this->recInfoMap.begin(); itr != this->recInfoMap.end(); itr++ ){
		if( itr->second->id == id ){
			if( itr->second->protectFlag == 0 ){
				DelTS_InfoFile(itr->second->recFilePath);
				SAFE_DELETE(itr->second);
				this->recInfoMap.erase(itr);
			}
			break;
		}
	}
	map<DWORD, REC_FILE_INFO*>::iterator itr2;
	itr2 = this->recIDMap.find(id);
	if( itr2 != this->recIDMap.end() ){
		if( itr2->second->protectFlag == 0 ){
			this->recIDMap.erase(itr2);
		}
	}

	return TRUE;
}

BOOL CParseRecInfoText::ChgProtectRecInfo(DWORD id, BYTE flag)
{
	multimap<wstring, REC_FILE_INFO*>::iterator itr;
	for( itr = this->recInfoMap.begin(); itr != this->recInfoMap.end(); itr++ ){
		if( itr->second->id == id ){
			itr->second->protectFlag = flag;
			if( itr->second->protectFlag == 1 ){
				if( itr->second->recFilePath.size() > 0 ){
					AddPtotectFileList(itr->second->recFilePath);
				}
			}else{
				if( itr->second->recFilePath.size() > 0 ){
					DelPtotectFileList(itr->second->recFilePath);
				}
			}
			break;
		}
	}

	return TRUE;
}

void CParseRecInfoText::AutoDelInfo(DWORD keepCount)
{
	if( (DWORD)this->recInfoMap.size() <= keepCount ){
		return;
	}

	int iDelCount = (int)this->recInfoMap.size() - (int)keepCount;
	for( int i=0; i<iDelCount; i++ ){
		multimap<wstring, REC_FILE_INFO*>::iterator itr;
		for( itr = this->recInfoMap.begin(); itr != this->recInfoMap.end(); itr++ ){
			if( itr->second->protectFlag == 0 ){
				DelTS_InfoFile(itr->second->recFilePath);
				SAFE_DELETE(itr->second);
				this->recInfoMap.erase(itr);
				break;
			}
		}
	}
	this->recIDMap.clear();
	multimap<wstring, REC_FILE_INFO*>::iterator itr;
	this->nextID = 1;
	for( itr = this->recInfoMap.begin(); itr != this->recInfoMap.end(); itr++ ){
		itr->second->id = this->nextID;
		this->recIDMap.insert(pair<DWORD, REC_FILE_INFO*>(itr->second->id, itr->second));
		this->nextID++;
	}
}

void CParseRecInfoText::DelTS_InfoFile(wstring tsFilePath)
{
	if( tsFilePath.size() <= 0 ){
		return;
	}
	wstring iniCommonPath = L"";
	GetCommonIniPath(iniCommonPath);
	if( GetPrivateProfileInt(L"SET", L"RecInfoDelFile", 0, iniCommonPath.c_str()) != 1 ){
		return ;
	}
	WCHAR buff[512] = L"";
	GetPrivateProfileString(L"SET", L"RecInfoFolder", L"", buff, 512, iniCommonPath.c_str());
	wstring infoFolder = buff;
	ChkFolderPath(infoFolder);

	wstring tsFileName = L"";
	GetFileName(tsFilePath, tsFileName);

	DeleteFile( tsFilePath.c_str() );
	_OutputDebugString(L"★RecInfo Auto Delete : %s", tsFilePath.c_str());

	wstring errFile = tsFilePath;
	errFile += L".err";
	DeleteFile( errFile.c_str() );
	_OutputDebugString(L"★RecInfo Auto Delete : %s", errFile.c_str());
	if( infoFolder.size() > 0 ){
		Format(errFile, L"%s\\%s.err", infoFolder.c_str(), tsFileName.c_str());
		DeleteFile( errFile.c_str() );
		_OutputDebugString(L"★RecInfo Auto Delete : %s", errFile.c_str());
	}

	wstring pgFile = tsFilePath;
	pgFile += L".program.txt";
	DeleteFile( pgFile.c_str() );
	_OutputDebugString(L"★RecInfo Auto Delete : %s", pgFile.c_str());
	if( infoFolder.size() > 0 ){
		Format(pgFile, L"%s\\%s.program.txt", infoFolder.c_str(), tsFileName.c_str());
		DeleteFile( pgFile.c_str() );
		_OutputDebugString(L"★RecInfo Auto Delete : %s", pgFile.c_str());
	}

}

void CParseRecInfoText::GetProtectFiles(map<wstring, wstring>* fileMap)
{
	*fileMap = protectFileList;
}

void CParseRecInfoText::AddPtotectFileList(wstring tsFilePath)
{
	wstring key = tsFilePath;
	transform(key.begin(), key.end(), key.begin(), toupper);
	protectFileList.insert(pair<wstring, wstring>(key, tsFilePath));
}

void CParseRecInfoText::DelPtotectFileList(wstring tsFilePath)
{
	wstring key = tsFilePath;
	transform(key.begin(), key.end(), key.begin(), toupper);

	map<wstring, wstring>::iterator itr;
	itr = protectFileList.find(key);
	if( itr != protectFileList.end() ){
		protectFileList.erase(itr);
	}
}


