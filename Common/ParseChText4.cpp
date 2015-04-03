#include "stdafx.h"
#include "ParseChText4.h"
#include "StringUtil.h"
#include "EpgTimerUtil.h"

CParseChText4::CParseChText4(void)
{
	filePath = L"";
}

CParseChText4::~CParseChText4(void)
{
}

BOOL CParseChText4::ParseText(LPCWSTR filePath)
{
	if( filePath == NULL ){
		return FALSE;
	}

	this->chList.clear();
	this->filePath = filePath;

	HANDLE hFile = _CreateFile2( filePath, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
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
		CH_DATA4 Item;
		if( Parse1Line(parseLine, &Item) == TRUE ){
			LONGLONG iKey = _Create64Key(Item.originalNetworkID, Item.transportStreamID, Item.serviceID );
			this->chList.insert( pair<LONGLONG, CH_DATA4>(iKey,Item) );
		}
	}

	return TRUE;
}

BOOL CParseChText4::Parse1Line(string parseLine, CH_DATA4* chInfo )
{
	if( parseLine.empty() == true || chInfo == NULL ){
		return FALSE;
	}
	string strBuff="";

	Separate( parseLine, "\t", strBuff, parseLine);

	//Ch名
	AtoW(strBuff, chInfo->chName);

	Separate( parseLine, "\t", strBuff, parseLine);

	//サービス名
	AtoW(strBuff, chInfo->serviceName);

	Separate( parseLine, "\t", strBuff, parseLine);

	//ts_name or network_name
	AtoW(strBuff, chInfo->networkName);

	Separate( parseLine, "\t", strBuff, parseLine);

	//Space
	chInfo->space = atoi(strBuff.c_str());

	Separate( parseLine, "\t", strBuff, parseLine);

	//Ch
	chInfo->ch = atoi(strBuff.c_str());

	Separate( parseLine, "\t", strBuff, parseLine);

	//ONID
	chInfo->originalNetworkID = atoi(strBuff.c_str());

	Separate( parseLine, "\t", strBuff, parseLine);

	//TSID
	chInfo->transportStreamID = (WORD)atoi(strBuff.c_str());

	Separate( parseLine, "\t", strBuff, parseLine);

	//SID
	chInfo->serviceID = (WORD)atoi(strBuff.c_str());

	Separate( parseLine, "\t", strBuff, parseLine);

	//Service Type
	chInfo->serviceType = atoi(strBuff.c_str());

	Separate( parseLine, "\t", strBuff, parseLine);

	//Partial Flag
	chInfo->partialFlag = atoi(strBuff.c_str());

	Separate( parseLine, "\t", strBuff, parseLine);

	//Use
	chInfo->useViewFlag = atoi(strBuff.c_str());

	Separate( parseLine, "\t", strBuff, parseLine);

	//リモコンID
	chInfo->remoconID = atoi(strBuff.c_str());

	return TRUE;
}

BOOL CParseChText4::AddCh(CH_DATA4 chInfo )
{
	LONGLONG iKey = _Create64Key(chInfo.originalNetworkID, chInfo.transportStreamID, chInfo.serviceID );
	multimap<LONGLONG, CH_DATA4>::iterator itrF;
	itrF = this->chList.find(iKey);
	if( itrF == this->chList.end() ){
		this->chList.insert( pair<LONGLONG, CH_DATA4>(iKey,chInfo) );
	}else{
		if( itrF->second.ch == chInfo.ch && itrF->second.space == chInfo.space ){
			return FALSE;
		}else{
			this->chList.insert( pair<LONGLONG, CH_DATA4>(iKey,chInfo) );
		}
	}
	return TRUE;
}

BOOL CParseChText4::DelCh(WORD originalNetworkID, WORD transportStreamID, WORD serviceID)
{
	LONGLONG iKey = _Create64Key(originalNetworkID, transportStreamID, serviceID );
	multimap<LONGLONG, CH_DATA4>::iterator itrF;
	itrF = this->chList.find(iKey);
	if( itrF != this->chList.end() ){
		this->chList.erase(itrF);
	}
	return TRUE;
}

BOOL CParseChText4::DelCh(
	WORD space,
	WORD ch)
{
	multimap<LONGLONG, CH_DATA4>::iterator itrF;
	for( itrF = this->chList.begin(); itrF != this->chList.end(); itrF++ ){
		if( itrF->second.space == space && itrF->second.ch == ch ){
			this->chList.erase(itrF);
			break;
		}
	}
	return TRUE;
}

BOOL CParseChText4::DelChService(
	WORD space,
	WORD ch,
	WORD serviceID)
{
	multimap<LONGLONG, CH_DATA4>::iterator itrF;
	for( itrF = this->chList.begin(); itrF != this->chList.end(); itrF++ ){
		if( itrF->second.space == space && itrF->second.ch == ch && itrF->second.serviceID == serviceID){
			this->chList.erase(itrF);
			break;
		}
	}
	return TRUE;
}

BOOL CParseChText4::SaveChText(LPCWSTR filePath)
{
	wstring loadFilePath = L"";
	if( filePath == NULL ){
		loadFilePath = this->filePath;
	}else{
		loadFilePath = filePath;
	}

	if( loadFilePath.size() == 0 ){
		return FALSE;
	}

	HANDLE hFile = _CreateFile2( loadFilePath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if( hFile == INVALID_HANDLE_VALUE ){
		return FALSE;
	}

	multimap<LONGLONG, CH_DATA4> sortList;
	multimap<LONGLONG, CH_DATA4>::iterator itr;
	for( itr = this->chList.begin(); itr != this->chList.end(); itr++ ){
		LONGLONG Key = ((LONGLONG)itr->second.space)<<32 | ((LONGLONG)itr->second.ch)<<16 | (LONGLONG)itr->second.serviceID;
		sortList.insert(pair<LONGLONG, CH_DATA4>(Key, itr->second));
	}

	for( itr = sortList.begin(); itr != sortList.end(); itr++ ){
		string chName="";
		WtoA(itr->second.chName, chName);
		string serviceName="";
		WtoA(itr->second.serviceName, serviceName);
		string networkName="";
		WtoA(itr->second.networkName, networkName);

		string strBuff;
		Format(strBuff, "%s\t%s\t%s\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\r\n",
			chName.c_str(),
			serviceName.c_str(),
			networkName.c_str(),
			itr->second.space,
			itr->second.ch,
			itr->second.originalNetworkID,
			itr->second.transportStreamID,
			itr->second.serviceID,
			itr->second.serviceType,
			itr->second.partialFlag,
			itr->second.useViewFlag,
			itr->second.remoconID
			);
		DWORD dwWrite = 0;
		WriteFile(hFile, strBuff.c_str(), (DWORD)strBuff.length(), &dwWrite, NULL);
	}

	CloseHandle(hFile);
	return TRUE;
}

