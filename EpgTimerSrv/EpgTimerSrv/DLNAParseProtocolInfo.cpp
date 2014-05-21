#include "StdAfx.h"
#include "DLNAParseProtocolInfo.h"


CDLNAParseProtocolInfo::CDLNAParseProtocolInfo(void)
{
}


CDLNAParseProtocolInfo::~CDLNAParseProtocolInfo(void)
{
}


BOOL CDLNAParseProtocolInfo::ParseText(LPCWSTR filePath)
{
	if( filePath == NULL ){
		return FALSE;
	}

	this->supportList.clear();

	HANDLE hFile = CreateFile( filePath, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
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
		if( parseLine.find(";") != 0 ){
			DLNA_PROTOCOL_INFO Item;
			if( Parse1Line(parseLine, &Item) == TRUE ){
				this->supportList.insert( pair<wstring, DLNA_PROTOCOL_INFO>(Item.ext,Item) );
			}
		}
	}

	return TRUE;
}

BOOL CDLNAParseProtocolInfo::Parse1Line(string parseLine, DLNA_PROTOCOL_INFO* info )
{
	if( parseLine.empty() == true || info == NULL ){
		return FALSE;
	}
	string strBuff="";

	Separate( parseLine, "\t", strBuff, parseLine);

	//
	AtoW(strBuff, info->ext);

	Separate( parseLine, "\t", strBuff, parseLine);

	//
	AtoW(strBuff, info->upnpClass);

	Separate( parseLine, "\t", strBuff, parseLine);

	//
	AtoW(strBuff, info->mimeType);

	Separate( parseLine, "\t", strBuff, parseLine);

	//
	AtoW(strBuff, info->info);

	return TRUE;
}

