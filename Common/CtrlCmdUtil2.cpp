#include "stdafx.h"
#include "CtrlCmdUtil2.h"
#include "Util.h"

namespace CtrlCmdUtilImpl_
{

#define READ_VALUE2_OR_FAIL(ver,buff,buffSize,pos,size,val)		{ if( ReadVALUE2((ver),val,(buff)+pos,(buffSize)-pos,&size) == FALSE ) return FALSE; pos+=size; }

DWORD WriteVALUE2(WORD ver, BYTE* buff, DWORD buffOffset, const REC_SETTING_DATA& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE2(ver, buff, pos, val.recMode);
	pos += WriteVALUE2(ver, buff, pos, val.priority);
	pos += WriteVALUE2(ver, buff, pos, val.tuijyuuFlag);
	pos += WriteVALUE2(ver, buff, pos, val.serviceMode);
	pos += WriteVALUE2(ver, buff, pos, val.pittariFlag);
	pos += WriteVALUE2(ver, buff, pos, val.batFilePath);
	pos += WriteVALUE2(ver, buff, pos, val.recFolderList);
	pos += WriteVALUE2(ver, buff, pos, val.suspendMode);
	pos += WriteVALUE2(ver, buff, pos, val.rebootFlag);
	pos += WriteVALUE2(ver, buff, pos, val.useMargineFlag);
	pos += WriteVALUE2(ver, buff, pos, val.startMargine);
	pos += WriteVALUE2(ver, buff, pos, val.endMargine);
	pos += WriteVALUE2(ver, buff, pos, val.continueRecFlag);
	pos += WriteVALUE2(ver, buff, pos, val.partialRecFlag);
	pos += WriteVALUE2(ver, buff, pos, val.tunerID);
	pos += WriteVALUE2(ver, buff, pos, val.partialRecFolder);
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE2(WORD ver, REC_SETTING_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL || buffSize < sizeof(DWORD) ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &valSize);

	if( pos < valSize ){
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->recMode);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->priority);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->tuijyuuFlag);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->serviceMode);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->pittariFlag);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->batFilePath);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->recFolderList);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->suspendMode);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->rebootFlag);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->useMargineFlag);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->startMargine);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->endMargine);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->continueRecFlag);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->partialRecFlag);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->tunerID);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->partialRecFolder);
		if( ver>=2 ){
			goto CMD_END;
		}
	}

CMD_END:
	if( readSize != NULL ){
		*readSize = valSize;
	}

	return TRUE;
}

DWORD WriteVALUE2(WORD ver, BYTE* buff, DWORD buffOffset, const RESERVE_DATA& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE2(ver, buff, pos, val.title);
	pos += WriteVALUE2(ver, buff, pos, val.startTime);
	pos += WriteVALUE2(ver, buff, pos, val.durationSecond);
	pos += WriteVALUE2(ver, buff, pos, val.stationName);
	pos += WriteVALUE2(ver, buff, pos, val.originalNetworkID);
	pos += WriteVALUE2(ver, buff, pos, val.transportStreamID);
	pos += WriteVALUE2(ver, buff, pos, val.serviceID);
	pos += WriteVALUE2(ver, buff, pos, val.eventID);
	pos += WriteVALUE2(ver, buff, pos, val.comment);
	pos += WriteVALUE2(ver, buff, pos, val.reserveID);
	pos += WriteVALUE2(ver, buff, pos, (BYTE)0);
	pos += WriteVALUE2(ver, buff, pos, val.overlapMode);
	pos += WriteVALUE2(ver, buff, pos, wstring());
	pos += WriteVALUE2(ver, buff, pos, val.startTimeEpg);
	pos += WriteVALUE2(ver, buff, pos, val.recSetting);
	pos += WriteVALUE2(ver, buff, pos, val.reserveStatus);
	if( ver >= 5 ){
		pos += WriteVALUE2(ver, buff, pos, val.recFileNameList);
		pos += WriteVALUE2(ver, buff, pos, (DWORD)0);
	}
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE2(WORD ver, RESERVE_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL || buffSize < sizeof(DWORD) ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &valSize);
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->title);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->startTime);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->durationSecond);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->stationName);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->originalNetworkID);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->transportStreamID);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->serviceID);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->eventID);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->comment);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->reserveID);
		BYTE bPadding;
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &bPadding);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->overlapMode);
		wstring strPadding;
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &strPadding);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->startTimeEpg);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->recSetting);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->reserveStatus);

		if( ver<=4 ){
			goto CMD_END;
		}

		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->recFileNameList);
		DWORD dwPadding;
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &dwPadding);

		if( ver>=2 ){
			goto CMD_END;
		}
	}

CMD_END:
	if( readSize != NULL ){
		*readSize = valSize;
	}

	return TRUE;
}

DWORD WriteVALUE2(WORD ver, BYTE* buff, DWORD buffOffset, const NOTIFY_SRV_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE2(ver, buff, pos, val.notifyID);
	pos += WriteVALUE2(ver, buff, pos, val.time);
	pos += WriteVALUE2(ver, buff, pos, val.param1);
	pos += WriteVALUE2(ver, buff, pos, val.param2);
	pos += WriteVALUE2(ver, buff, pos, val.param3);
	pos += WriteVALUE2(ver, buff, pos, val.param4);
	pos += WriteVALUE2(ver, buff, pos, val.param5);
	pos += WriteVALUE2(ver, buff, pos, val.param6);
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE2(WORD ver, NOTIFY_SRV_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL || buffSize < sizeof(DWORD) ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &valSize);
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->notifyID);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->time);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->param1);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->param2);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->param3);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->param4);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->param5);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->param6);

		if( ver>=2 ){
			goto CMD_END;
		}
	}

CMD_END:
	if( readSize != NULL ){
		*readSize = valSize;
	}

	return TRUE;
}

DWORD WriteVALUE2(WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_SEARCH_KEY_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE2(ver, buff, pos, val.andKey);
	pos += WriteVALUE2(ver, buff, pos, val.notKey);
	pos += WriteVALUE2(ver, buff, pos, val.regExpFlag);
	pos += WriteVALUE2(ver, buff, pos, val.titleOnlyFlag);
	pos += WriteVALUE2(ver, buff, pos, val.contentList);
	pos += WriteVALUE2(ver, buff, pos, val.dateList);
	pos += WriteVALUE2(ver, buff, pos, val.serviceList);
	pos += WriteVALUE2(ver, buff, pos, val.videoList);
	pos += WriteVALUE2(ver, buff, pos, val.audioList);
	pos += WriteVALUE2(ver, buff, pos, val.aimaiFlag);
	pos += WriteVALUE2(ver, buff, pos, val.notContetFlag);
	pos += WriteVALUE2(ver, buff, pos, val.notDateFlag);
	pos += WriteVALUE2(ver, buff, pos, val.freeCAFlag);
	if( ver >= 3 ){
		pos += WriteVALUE2(ver, buff, pos, val.chkRecEnd);
		pos += WriteVALUE2(ver, buff, pos, val.chkRecDay);
	}
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE2(WORD ver, EPGDB_SEARCH_KEY_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL || buffSize < sizeof(DWORD) ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &valSize);
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->andKey);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->notKey);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->regExpFlag);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->titleOnlyFlag);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->contentList);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->dateList);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->serviceList);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->videoList);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->audioList);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->aimaiFlag);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->notContetFlag);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->notDateFlag);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->freeCAFlag);
		if( ver<=2 ){
			goto CMD_END;
		}
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->chkRecEnd);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->chkRecDay);

		if( ver>=3 ){
			goto CMD_END;
		}
	}

CMD_END:
	if( readSize != NULL ){
		*readSize = valSize;
	}

	return TRUE;
}

DWORD WriteVALUE2(WORD ver, BYTE* buff, DWORD buffOffset, const EPG_AUTO_ADD_DATA& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE2(ver, buff, pos, val.dataID);
	pos += WriteVALUE2(ver, buff, pos, val.searchInfo);
	pos += WriteVALUE2(ver, buff, pos, val.recSetting);
	if( ver >= 5 ){
		pos += WriteVALUE2(ver, buff, pos, val.addCount);
	}
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE2(WORD ver, EPG_AUTO_ADD_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL || buffSize < sizeof(DWORD) ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &valSize);
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->dataID);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->searchInfo);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->recSetting);

		if( ver<=4 ){
			goto CMD_END;
		}

		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->addCount);

		if( ver>=5 ){
			goto CMD_END;
		}
	}

CMD_END:
	if( readSize != NULL ){
		*readSize = valSize;
	}

	return TRUE;
}

DWORD WriteVALUE2(WORD ver, BYTE* buff, DWORD buffOffset, const MANUAL_AUTO_ADD_DATA& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE2(ver, buff, pos, val.dataID);
	pos += WriteVALUE2(ver, buff, pos, val.dayOfWeekFlag);
	pos += WriteVALUE2(ver, buff, pos, val.startTime);
	pos += WriteVALUE2(ver, buff, pos, val.durationSecond);
	pos += WriteVALUE2(ver, buff, pos, val.title);
	pos += WriteVALUE2(ver, buff, pos, val.stationName);
	pos += WriteVALUE2(ver, buff, pos, val.originalNetworkID);
	pos += WriteVALUE2(ver, buff, pos, val.transportStreamID);
	pos += WriteVALUE2(ver, buff, pos, val.serviceID);
	pos += WriteVALUE2(ver, buff, pos, val.recSetting);
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE2(WORD ver, MANUAL_AUTO_ADD_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL || buffSize < sizeof(DWORD) ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &valSize);
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->dataID);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->dayOfWeekFlag);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->startTime);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->durationSecond);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->title);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->stationName);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->originalNetworkID);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->transportStreamID);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->serviceID);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->recSetting);

		if( ver>=2 ){
			goto CMD_END;
		}
	}

CMD_END:
	if( readSize != NULL ){
		*readSize = valSize;
	}

	return TRUE;
}

DWORD WriteVALUE2(WORD ver, BYTE* buff, DWORD buffOffset, const REC_FILE_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE2(ver, buff, pos, val.id);
	pos += WriteVALUE2(ver, buff, pos, val.recFilePath);
	pos += WriteVALUE2(ver, buff, pos, val.title);
	pos += WriteVALUE2(ver, buff, pos, val.startTime);
	pos += WriteVALUE2(ver, buff, pos, val.durationSecond);
	pos += WriteVALUE2(ver, buff, pos, val.serviceName);
	pos += WriteVALUE2(ver, buff, pos, val.originalNetworkID);
	pos += WriteVALUE2(ver, buff, pos, val.transportStreamID);
	pos += WriteVALUE2(ver, buff, pos, val.serviceID);
	pos += WriteVALUE2(ver, buff, pos, val.eventID);
	pos += WriteVALUE2(ver, buff, pos, val.drops);
	pos += WriteVALUE2(ver, buff, pos, val.scrambles);
	pos += WriteVALUE2(ver, buff, pos, val.recStatus);
	pos += WriteVALUE2(ver, buff, pos, val.startTimeEpg);
	pos += WriteVALUE2(ver, buff, pos, val.comment);
	pos += WriteVALUE2(ver, buff, pos, val.programInfo);
	pos += WriteVALUE2(ver, buff, pos, val.errInfo);
	if( ver >= 4 ){
		pos += WriteVALUE2(ver, buff, pos, val.protectFlag);
	}
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE2(WORD ver, REC_FILE_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL || buffSize < sizeof(DWORD) ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &valSize);
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->id);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->recFilePath);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->title);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->startTime);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->durationSecond);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->serviceName);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->originalNetworkID);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->transportStreamID);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->serviceID);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->eventID);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->drops);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->scrambles);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->recStatus);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->startTimeEpg);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->comment);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->programInfo);
		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->errInfo);
		if( ver<=3 ){
			goto CMD_END;
		}

		READ_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->protectFlag);

		if( ver>=4 ){
			goto CMD_END;
		}
	}

CMD_END:
	if( readSize != NULL ){
		*readSize = valSize;
	}

	return TRUE;
}

}
