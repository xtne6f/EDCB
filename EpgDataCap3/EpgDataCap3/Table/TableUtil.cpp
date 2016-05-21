#include "StdAfx.h"
#include "TableUtil.h"

CTableUtil::t_type CTableUtil::Decode(BYTE* data, DWORD dataSize, CPSITable** table)
{
	t_type type = TYPE_NONE;
	if( data != NULL && dataSize != 0 && table != NULL ){
		switch( data[0] ){
		case 0x00:
			type = TYPE_PAT;
			*table = cache[type] ? cache[type].release() : new CPATTable;
			break;
		case 0x01:
			type = TYPE_CAT;
			*table = cache[type] ? cache[type].release() : new CCATTable;
			break;
		case 0x02:
			type = TYPE_PMT;
			*table = cache[type] ? cache[type].release() : new CPMTTable;
			break;
		case 0x40:
		case 0x41:
			type = TYPE_NIT;
			*table = cache[type] ? cache[type].release() : new CNITTable;
			break;
		case 0x42:
		case 0x46:
			type = TYPE_SDT;
			*table = cache[type] ? cache[type].release() : new CSDTTable;
			break;
		case 0x4A:
			type = TYPE_BAT;
			*table = cache[type] ? cache[type].release() : new CBATTable;
			break;
		case 0x70:
			type = TYPE_TDT;
			*table = cache[type] ? cache[type].release() : new CTDTTable;
			break;
		case 0x73:
			type = TYPE_TOT;
			*table = cache[type] ? cache[type].release() : new CTOTTable;
			break;
		case 0xC8:
			type = TYPE_CDT;
			*table = cache[type] ? cache[type].release() : new CCDTTable;
			break;
		case 0xC3:
			type = TYPE_SDTT;
			*table = cache[type] ? cache[type].release() : new CSDTTTable;
			break;
		case 0xC4:
			type = TYPE_BIT;
			*table = cache[type] ? cache[type].release() : new CBITTable;
			break;
		case 0x7F:
			type = TYPE_SIT;
			*table = cache[type] ? cache[type].release() : new CSITTable;
			break;
		case 0xA4:
		case 0xA7:
			//type = TYPE_EIT_SD;
			//*table = new CEITTable_SD;
			break;
		case 0xA2:
		case 0xA3:
			//type = TYPE_EIT_SD2;
			//*table = new CEITTable_SD2;
			break;
		default:
			if( 0x4E <= data[0] && data[0] <= 0x6F ){
				type = TYPE_EIT;
				*table = cache[type] ? cache[type].release() : new CEITTable;
			}else if( data[0] == 0xFF ){
				//stuffing
			}else{
				//_OutputDebugString(L"++CTableUtil:: err UnknownTable 0x%02X\r\n", data[0]);
			}
			break;
		}
		if( type != TYPE_NONE && (*table)->Decode(data, dataSize, NULL) == FALSE ){
			cache[type].reset(*table);
			type = TYPE_NONE;
			*table = NULL;
		}
	}
	return type;
}
