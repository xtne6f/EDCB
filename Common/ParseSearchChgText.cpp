#include "StdAfx.h"
#include "ParseSearchChgText.h"
#include "StringUtil.h"
#include "Util.h"

CParseSearchChgText::CParseSearchChgText(void)
{

}

CParseSearchChgText::~CParseSearchChgText(void)
{
}

BOOL CParseSearchChgText::ParseReserveText(LPCWSTR filePath)
{
	if( filePath == NULL ){
		return FALSE;
	}

	this->chgKey.clear();

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
	int iIndex = 0;
	DWORD dwCount = 1;
	while( iIndex != (int)string::npos ){
		iIndex = (int)strRead.find("\r\n");
		if( iIndex == (int)string::npos ){
			parseLine = strRead;
			strRead.clear();
		}else{
			parseLine = strRead.substr(0,iIndex);
			strRead.erase( 0, iIndex+2 );
		}
		//先頭；はコメント行
		if( parseLine.find(";") != 0 ){
			//空行？
			if( parseLine.find("\t") != string::npos ){
				CHG_INFO Item;
				BOOL bRet = Parse1Line(parseLine, Item.oldText, Item.newText);
				if( bRet == TRUE ){
					this->chgKey.push_back(Item);
				}
			}
		}
	}

	return TRUE;
}

BOOL CParseSearchChgText::Parse1Line(string parseLine, wstring& oldText, wstring& newText)
{
	if( parseLine.empty() == true ){
		return FALSE;
	}
	string strBuff="";

	Separate( parseLine, "\t", strBuff, parseLine);
	
	AtoW(strBuff, oldText);
	AtoW(parseLine, newText);

	return TRUE;
}

void CParseSearchChgText::ChgText(wstring& chgText)
{
	for( size_t i = 0; i<this->chgKey.size(); i++ ){
		if( chgText.find( this->chgKey[i].oldText ) != string::npos ){
			Replace(chgText, this->chgKey[i].oldText, this->chgKey[i].newText);
		}
	}
}
