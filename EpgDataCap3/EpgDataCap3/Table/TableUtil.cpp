#include "StdAfx.h"
#include "TableUtil.h"

CTableUtil::CTableUtil(void)
{
}

CTableUtil::~CTableUtil(void)
{
}

void CTableUtil::Clear(vector<TABLE_DATA*>* tableList)
{
	if( tableList != NULL ){
		vector<TABLE_DATA*>::iterator itr;
		for( itr = tableList->begin(); itr != tableList->end(); itr++ ){
			SAFE_DELETE(*itr);
		}
		tableList->clear();
	}
}

BOOL CTableUtil::Decode( BYTE* data, DWORD dataSize, vector<TABLE_DATA*>* tableList, DWORD* decodeReadSize )
{
	if( data == NULL || dataSize == 0 || tableList == NULL ){
		return FALSE;
	}
	DWORD decodeSize = 0;
	while( decodeSize < dataSize ){
		DWORD readSize = 0;
		TABLE_DATA* item = new TABLE_DATA;
		if( data[decodeSize] == 0x00 ){
			item->PATTable = new CPATTable;
			if( item->PATTable->Decode(data + decodeSize, dataSize-decodeSize, &readSize) == FALSE ){
				SAFE_DELETE(item);
				Clear(tableList);
				return FALSE;
			}else{
				tableList->push_back(item);
			}
		}else if( data[decodeSize] == 0x01 ){
			item->CATTable = new CCATTable;
			if( item->CATTable->Decode(data + decodeSize, dataSize-decodeSize, &readSize) == FALSE ){
				SAFE_DELETE(item);
				Clear(tableList);
				return FALSE;
			}else{
				tableList->push_back(item);
			}
		}else if( data[decodeSize] == 0x02 ){
			item->PMTTable = new CPMTTable;
			if( item->PMTTable->Decode(data + decodeSize, dataSize-decodeSize, &readSize) == FALSE ){
				SAFE_DELETE(item);
				Clear(tableList);
				return FALSE;
			}else{
				tableList->push_back(item);
			}
		}else if( data[decodeSize] == 0x40 || data[decodeSize] == 0x41 ){
			item->NITTable = new CNITTable;
			if( item->NITTable->Decode(data + decodeSize, dataSize-decodeSize, &readSize) == FALSE ){
				SAFE_DELETE(item);
				Clear(tableList);
				return FALSE;
			}else{
				tableList->push_back(item);
			}
		}else if( data[decodeSize] == 0x42 || data[decodeSize] == 0x46 ){
			item->SDTTable = new CSDTTable;
			if( item->SDTTable->Decode(data + decodeSize, dataSize-decodeSize, &readSize) == FALSE ){
				SAFE_DELETE(item);
				Clear(tableList);
				return FALSE;
			}else{
				tableList->push_back(item);
			}
		}else if( data[decodeSize] == 0x4A ){
			item->BATTable = new CBATTable;
			if( item->BATTable->Decode(data + decodeSize, dataSize-decodeSize, &readSize) == FALSE ){
				SAFE_DELETE(item);
				Clear(tableList);
				return FALSE;
			}else{
				tableList->push_back(item);
			}
		}else if( 0x4E <= data[decodeSize] && data[decodeSize] <= 0x6F ){
			item->EITTable = new CEITTable;
			if( item->EITTable->Decode(data + decodeSize, dataSize-decodeSize, &readSize) == FALSE ){
				SAFE_DELETE(item);
				Clear(tableList);
				return FALSE;
			}else{
				tableList->push_back(item);
			}
		}else if( data[decodeSize] == 0x70 ){
			item->TDTTable = new CTDTTable;
			if( item->TDTTable->Decode(data + decodeSize, dataSize-decodeSize, &readSize) == FALSE ){
				SAFE_DELETE(item);
				Clear(tableList);
				return FALSE;
			}else{
				tableList->push_back(item);
			}
		}else if( data[decodeSize] == 0x73 ){
			item->TOTTable = new CTOTTable;
			if( item->TOTTable->Decode(data + decodeSize, dataSize-decodeSize, &readSize) == FALSE ){
				SAFE_DELETE(item);
				Clear(tableList);
				return FALSE;
			}else{
				tableList->push_back(item);
			}
		}else if( data[decodeSize] == 0xC8 ){
			item->CDTTable = new CCDTTable;
			if( item->CDTTable->Decode(data + decodeSize, dataSize-decodeSize, &readSize) == FALSE ){
				SAFE_DELETE(item);
				Clear(tableList);
				return FALSE;
			}else{
				tableList->push_back(item);
			}
		}else if( data[decodeSize] == 0xC3 ){
			item->SDTTTable = new CSDTTTable;
			if( item->SDTTTable->Decode(data + decodeSize, dataSize-decodeSize, &readSize) == FALSE ){
				SAFE_DELETE(item);
				Clear(tableList);
				return FALSE;
			}else{
				tableList->push_back(item);
			}
		}else if( data[decodeSize] == 0xC4 ){
			item->BITTable = new CBITTable;
			if( item->BITTable->Decode(data + decodeSize, dataSize-decodeSize, &readSize) == FALSE ){
				SAFE_DELETE(item);
				Clear(tableList);
				return FALSE;
			}else{
				tableList->push_back(item);
			}
		}else if( data[decodeSize] == 0x7F ){
			item->SITTable = new CSITTable;
			if( item->SITTable->Decode(data + decodeSize, dataSize-decodeSize, &readSize) == FALSE ){
				SAFE_DELETE(item);
				Clear(tableList);
				return FALSE;
			}else{
				tableList->push_back(item);
			}
		}else if( 0xA4 == data[decodeSize] || 0xA7 == data[decodeSize]){
			item->EITTable_SD = new CEITTable_SD;
			if( item->EITTable_SD->Decode(data + decodeSize, dataSize-decodeSize, &readSize) == FALSE ){
				SAFE_DELETE(item);
				Clear(tableList);
				return FALSE;
			}else{
				tableList->push_back(item);
			}
		}else if( 0xA2 == data[decodeSize] || 0xA3 == data[decodeSize]){
			item->EITTable_SD2 = new CEITTable_SD2;
			if( item->EITTable_SD2->Decode(data + decodeSize, dataSize-decodeSize, &readSize) == FALSE ){
				SAFE_DELETE(item);
				Clear(tableList);
				return FALSE;
			}else{
				tableList->push_back(item);
			}
		}else if( data[decodeSize] == 0xFF ){
			//stuffing
			decodeSize = dataSize;
		}else{
			//_OutputDebugString(L"++CTableUtil:: err UnknownTable 0x%02X\r\n", data[decodeSize]);
			SAFE_DELETE(item);
			Clear(tableList);
			return FALSE;
		}
		decodeSize += readSize;
	}
	if( decodeReadSize != NULL ){
		*decodeReadSize = dataSize;
	}
	return TRUE;
}
