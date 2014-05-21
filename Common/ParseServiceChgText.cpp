#include "StdAfx.h"
#include "ParseServiceChgText.h"
#include "StringUtil.h"
#include "Util.h"

CParseServiceChgText::CParseServiceChgText(void)
{

}

CParseServiceChgText::~CParseServiceChgText(void)
{
}

BOOL CParseServiceChgText::ParseText(LPCWSTR filePath)
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
					this->chgKey.insert(pair<wstring,CHG_INFO>(Item.oldText, Item));
				}
			}
		}
	}

	return TRUE;
}

BOOL CParseServiceChgText::Parse1Line(string parseLine, wstring& oldText, wstring& newText)
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

void CParseServiceChgText::ChgText(wstring& chgText)
{
	map<wstring,CHG_INFO>::iterator itr;
	itr = this->chgKey.find(chgText);
	if( itr != this->chgKey.end() ){
		chgText = itr->second.newText;
	}
}
