#include "StdAfx.h"
#include "ParseContentTypeText.h"
#include "StringUtil.h"
#include "Util.h"

CParseContentTypeText::CParseContentTypeText(void)
{

}

CParseContentTypeText::~CParseContentTypeText(void)
{
}

BOOL CParseContentTypeText::ParseText(LPCWSTR filePath)
{
	if( filePath == NULL ){
		return FALSE;
	}

	this->typeList.clear();

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
				wstring ext;
				wstring mimeType;
				BOOL bRet = Parse1Line(parseLine, ext, mimeType);
				if( bRet == TRUE ){
					this->typeList.insert(pair<wstring,wstring>(ext, mimeType));
				}
			}
		}
	}

	return TRUE;
}

BOOL CParseContentTypeText::Parse1Line(string parseLine, wstring& ext, wstring& mimeType)
{
	if( parseLine.empty() == true ){
		return FALSE;
	}
	string strBuff="";

	Separate( parseLine, "\t", strBuff, parseLine);
	
	AtoW(strBuff, ext);
	AtoW(parseLine, mimeType);

	return TRUE;
}

void CParseContentTypeText::GetMimeType(wstring ext, wstring& mimeType)
{
	mimeType = L"application/octet-stream";

	map<wstring,wstring>::iterator itr;
	itr = this->typeList.find(ext);
	if( itr != this->typeList.end() ){
		mimeType = itr->second;
	}
}
