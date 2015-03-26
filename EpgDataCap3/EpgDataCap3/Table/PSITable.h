#pragma once

class CPSITable
{
public:
	BYTE table_id;
	BYTE section_syntax_indicator;
	WORD section_length;
public:
	CPSITable(void);
	virtual ~CPSITable(void);
	virtual BOOL Decode( BYTE* data, DWORD dataSize, DWORD* decodeReadSize );
protected:
	DWORD readSize;
protected:
	BOOL InitDecode( BYTE* data, DWORD dataSize, DWORD* decodeReadSize, BOOL hasCrc32 );
};
