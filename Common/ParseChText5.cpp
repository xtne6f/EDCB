#include "stdafx.h"
#include "ParseChText5.h"
#include "StringUtil.h"
#include "EpgTimerUtil.h"

CParseChText5::CParseChText5(void)
{
	this->filePath = L"";
}

CParseChText5::~CParseChText5(void)
{
}

BOOL CParseChText5::ParseText(LPCWSTR filePath)
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
		CH_DATA5 Item;
		if( Parse1Line(parseLine, &Item) == TRUE ){
			LONGLONG iKey = _Create64Key(Item.originalNetworkID, Item.transportStreamID, Item.serviceID );
			this->chList.insert( pair<LONGLONG, CH_DATA5>(iKey,Item) );
		}
	}

	return TRUE;
}

BOOL CParseChText5::Parse1Line(string parseLine, CH_DATA5* chInfo )
{
	if( parseLine.empty() == true || chInfo == NULL ){
		return FALSE;
	}
	string strBuff="";

	Separate( parseLine, "\t", strBuff, parseLine);

	//名前
	AtoW(strBuff, chInfo->serviceName);

	Separate( parseLine, "\t", strBuff, parseLine);

	//ts_name or network_name
	AtoW(strBuff, chInfo->networkName);

	Separate( parseLine, "\t", strBuff, parseLine);

	//ONID
	chInfo->originalNetworkID = (WORD)atoi(strBuff.c_str());

	Separate( parseLine, "\t", strBuff, parseLine);

	//TSID
	chInfo->transportStreamID = (WORD)atoi(strBuff.c_str());

	Separate( parseLine, "\t", strBuff, parseLine);

	//SID
	chInfo->serviceID = (WORD)atoi(strBuff.c_str());

	Separate( parseLine, "\t", strBuff, parseLine);

	//Service Type
	chInfo->serviceType = (WORD)atoi(strBuff.c_str());

	Separate( parseLine, "\t", strBuff, parseLine);

	//Partial Flag
	chInfo->partialFlag = (WORD)atoi(strBuff.c_str());

	Separate( parseLine, "\t", strBuff, parseLine);

	//EPG取得対象
	chInfo->epgCapFlag = atoi(strBuff.c_str());

	//検索のデフォルト対象
	chInfo->searchFlag = atoi(parseLine.c_str());

	return TRUE;
}

BOOL CParseChText5::AddCh(CH_DATA5 chInfo )
{
	LONGLONG iKey = _Create64Key(chInfo.originalNetworkID, chInfo.transportStreamID, chInfo.serviceID );
	map<LONGLONG, CH_DATA5>::iterator itrF;
	itrF = this->chList.find(iKey);
	if( itrF == this->chList.end() ){
		this->chList.insert( pair<LONGLONG, CH_DATA5>(iKey,chInfo) );
	}else{
		itrF->second = chInfo;
		return FALSE;
	}
	return TRUE;
}

BOOL CParseChText5::DelCh(WORD originalNetworkID, WORD transportStreamID, WORD serviceID)
{
	LONGLONG iKey = _Create64Key(originalNetworkID, transportStreamID, serviceID );
	map<LONGLONG, CH_DATA5>::iterator itrF;
	itrF = this->chList.find(iKey);
	if( itrF != this->chList.end() ){
		this->chList.erase(itrF);
	}
	return TRUE;
}

BOOL CParseChText5::SaveChText(LPCWSTR filePath)
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

	if( this->chList.size() == 0 ){
		return FALSE;
	}

	HANDLE hFile = _CreateFile2( loadFilePath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if( hFile == INVALID_HANDLE_VALUE ){
		return FALSE;
	}

	multimap<LONGLONG, CH_DATA5> sortList;
	map<LONGLONG, CH_DATA5>::iterator itrCh;
	for( itrCh = this->chList.begin(); itrCh != this->chList.end(); itrCh++ ){
		int network;
		if( 0x7880 <= itrCh->second.originalNetworkID && itrCh->second.originalNetworkID <= 0x7FE8 ){
			if( itrCh->second.partialFlag == 0 ){
				network = 0; //地デジ
			}else{
				network = 1; //ワンセグ
			}
		}else if( itrCh->second.originalNetworkID == 0x04 ){
			network = 2; //BS
		}else if( itrCh->second.originalNetworkID == 0x06 || itrCh->second.originalNetworkID == 0x07 ){
			network = 3; //CS
		}else{
			network = 4; //その他
		}

		LONGLONG Key = ((LONGLONG)network)<<16 | (LONGLONG)itrCh->second.serviceID;
		sortList.insert(pair<LONGLONG, CH_DATA5>(Key, itrCh->second));
	}

	multimap<LONGLONG, CH_DATA5>::iterator itr;
	for( itr = sortList.begin(); itr != sortList.end(); itr++ ){
		string serviceName="";
		WtoA(itr->second.serviceName, serviceName);
		string networkName="";
		WtoA(itr->second.networkName, networkName);

		string strBuff;
		Format(strBuff, "%s\t%s\t%d\t%d\t%d\t%d\t%d\t%d\t%d\r\n",
			serviceName.c_str(),
			networkName.c_str(),
			itr->second.originalNetworkID,
			itr->second.transportStreamID,
			itr->second.serviceID,
			itr->second.serviceType,
			itr->second.partialFlag,
			itr->second.epgCapFlag,
			itr->second.searchFlag
			);
		DWORD dwWrite = 0;
		WriteFile(hFile, strBuff.c_str(), (DWORD)strBuff.length(), &dwWrite, NULL);
	}

	CloseHandle(hFile);
	return TRUE;
}

WORD CParseChText5::GetTSID(WORD originalNetworkID, WORD serviceID)
{
	int iRet = -1;
	map<LONGLONG, CH_DATA5>::iterator itr;
	for( itr = this->chList.begin(); itr != this->chList.end(); itr++ ){
		if( itr->second.originalNetworkID == originalNetworkID && itr->second.serviceID == serviceID ){
			iRet = itr->second.transportStreamID;
			break;
		}
	}
	return iRet;
}

//EPGデータの取得対象かを設定する
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// originalNetworkID		[IN]ONID
// transportStreamID		[IN]TSID
// serviceID				[IN]ServiceID
// enable					[IN]EPGデータの取得対象かどうか
BOOL CParseChText5::SetEpgCapMode(
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	BOOL enable
	)
{
	LONGLONG iKey = _Create64Key(originalNetworkID, transportStreamID, serviceID );
	map<LONGLONG, CH_DATA5>::iterator itrF;
	itrF = this->chList.find(iKey);
	if( itrF != this->chList.end() ){
		itrF->second.epgCapFlag = enable;
	}
	return TRUE;
}
