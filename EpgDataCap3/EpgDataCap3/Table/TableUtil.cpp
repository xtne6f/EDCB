#include "StdAfx.h"
#include "TableUtil.h"

CTableUtil::t_type CTableUtil::Decode(BYTE* data, DWORD dataSize, CPSITable** table)
{
	t_type type = TYPE_NONE;
	if( data != NULL && dataSize != 0 && table != NULL ){
		switch( data[0] ){
		case 0x00:
			type = TYPE_PAT;
			*table = new CPATTable;
			break;
		case 0x01:
			type = TYPE_CAT;
			*table = new CCATTable;
			break;
		case 0x02:
			type = TYPE_PMT;
			*table = new CPMTTable;
			break;
		case 0x40:
		case 0x41:
			type = TYPE_NIT;
			*table = new CNITTable;
			break;
		case 0x42:
		case 0x46:
			type = TYPE_SDT;
			*table = new CSDTTable;
			break;
		case 0x4A:
			type = TYPE_BAT;
			*table = new CBATTable;
			break;
		case 0x70:
			type = TYPE_TDT;
			*table = new CTDTTable;
			break;
		case 0x73:
			type = TYPE_TOT;
			*table = new CTOTTable;
			break;
		case 0xC8:
			type = TYPE_CDT;
			*table = new CCDTTable;
			break;
		case 0xC3:
			type = TYPE_SDTT;
			*table = new CSDTTTable;
			break;
		case 0xC4:
			type = TYPE_BIT;
			*table = new CBITTable;
			break;
		case 0x7F:
			type = TYPE_SIT;
			*table = new CSITTable;
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
				*table = new CEITTable;
			}else if( data[0] == 0xFF ){
				//stuffing
			}else{
				//_OutputDebugString(L"++CTableUtil:: err UnknownTable 0x%02X\r\n", data[0]);
			}
			break;
		}
		if( type != TYPE_NONE && (*table)->Decode(data, dataSize, NULL) == FALSE ){
			type = TYPE_NONE;
			SAFE_DELETE(*table);
		}
	}
	return type;
}
