#include "stdafx.h"
#include "CtrlCmdUtil2.h"
#include "Util.h"

//^\s*if *\( *WriteVALUE2 *\( *ver *,.*?, *buff *\+ *pos *, *buffSize *- *pos *, *&size *\) *== *FALSE *\) *{ *\r\n^\s*return *FALSE; *\r\n^\s*} *\r\n^\s*pos *\+= *size; *\r\n
#define WRITE_VALUE2_OR_FAIL(ver,buff,buffSize,pos,size,val)	{ if( WriteVALUE2((ver),val,(buff)+pos,(buffSize)-pos,&size) == FALSE ) return FALSE; pos+=size; }
#define READ_VALUE2_OR_FAIL(ver,buff,buffSize,pos,size,val)		{ if( ReadVALUE2((ver),val,(buff)+pos,(buffSize)-pos,&size) == FALSE ) return FALSE; pos+=size; }

DWORD GetVALUESize2(WORD ver, REC_SETTING_DATA* val )
{
	DWORD size = sizeof(DWORD);
	if( val == NULL ){
		return size;
	}

	size += GetVALUESize2(ver,val->recMode);
	size += GetVALUESize2(ver,val->priority);
	size += GetVALUESize2(ver,val->tuijyuuFlag);
	size += GetVALUESize2(ver,val->serviceMode);
	size += GetVALUESize2(ver,val->pittariFlag);
	size += GetVALUESize2(ver,val->batFilePath);
	size += GetVALUESize2(ver,&val->recFolderList);
	size += GetVALUESize2(ver,val->suspendMode);
	size += GetVALUESize2(ver,val->rebootFlag);
	size += GetVALUESize2(ver,val->useMargineFlag);
	size += GetVALUESize2(ver,val->startMargine);
	size += GetVALUESize2(ver,val->endMargine);
	size += GetVALUESize2(ver,val->continueRecFlag);
	size += GetVALUESize2(ver,val->partialRecFlag);
	size += GetVALUESize2(ver,val->tunerID);
	size += GetVALUESize2(ver,&val->partialRecFolder);
	if( ver>=2 ){
		goto CMD_END;
	}

CMD_END:
	return size;
}

BOOL WriteVALUE2(WORD ver, REC_SETTING_DATA* val, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	DWORD valSize = GetVALUESize2(ver, val );
	if( buff == NULL || valSize > buffSize ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, valSize);

	if (val != NULL ){
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->recMode);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->priority);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->tuijyuuFlag);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->serviceMode);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->pittariFlag);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->batFilePath);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->recFolderList);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->suspendMode);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->rebootFlag);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->useMargineFlag);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->startMargine);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->endMargine);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->continueRecFlag);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->partialRecFlag);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->tunerID);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->partialRecFolder);
		if( ver>=2 ){
			goto CMD_END;
		}
	}

CMD_END:
	if( writeSize != NULL ){
		*writeSize = pos;
	}
	return TRUE;
}

BOOL ReadVALUE2(WORD ver, REC_SETTING_DATA* val, BYTE* buff, DWORD buffSize, DWORD* readSize )
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

DWORD GetVALUESize2(WORD ver, RESERVE_DATA* val )
{
	DWORD size = sizeof(DWORD);
	if( val == NULL ){
		return size;
	}

	size += GetVALUESize2(ver,val->title);
	size += GetVALUESize2(ver,&val->startTime);
	size += GetVALUESize2(ver,val->durationSecond);
	size += GetVALUESize2(ver,val->stationName);
	size += GetVALUESize2(ver,val->originalNetworkID);
	size += GetVALUESize2(ver,val->transportStreamID);
	size += GetVALUESize2(ver,val->serviceID);
	size += GetVALUESize2(ver,val->eventID);
	size += GetVALUESize2(ver,val->comment);
	size += GetVALUESize2(ver,val->reserveID);
	size += GetVALUESize2(ver,(BYTE)0);
	size += GetVALUESize2(ver,val->overlapMode);
	size += GetVALUESize2(ver,wstring());
	size += GetVALUESize2(ver,&val->startTimeEpg);
	size += GetVALUESize2(ver,&val->recSetting);
	size += GetVALUESize2(ver,val->reserveStatus);

	if( ver<=4 ){
		goto CMD_END;
	}

	size += GetVALUESize2(ver,&val->recFileNameList);
	size += GetVALUESize2(ver,(DWORD)0);

	if( ver>=5 ){
		goto CMD_END;
	}

CMD_END:
	return size;
}

BOOL WriteVALUE2(WORD ver, RESERVE_DATA* val, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	DWORD valSize = GetVALUESize2(ver, val );
	if( buff == NULL || valSize > buffSize ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, valSize);

	if(val != NULL ){
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->title);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->startTime);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->durationSecond);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->stationName);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->originalNetworkID);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->transportStreamID);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->serviceID);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->eventID);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->comment);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->reserveID);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, (BYTE)0);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->overlapMode);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, wstring());
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->startTimeEpg);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->recSetting);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->reserveStatus);

		if( ver<=4 ){
			goto CMD_END;
		}

		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->recFileNameList);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, (DWORD)0);

		if( ver>=5 ){
			goto CMD_END;
		}
	}

CMD_END:
	if( writeSize != NULL ){
		*writeSize = pos;
	}
	return TRUE;
}

BOOL ReadVALUE2(WORD ver, RESERVE_DATA* val, BYTE* buff, DWORD buffSize, DWORD* readSize )
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

DWORD GetVALUESize2(WORD ver, NOTIFY_SRV_INFO* val )
{
	DWORD size = sizeof(DWORD);
	if( val == NULL ){
		return size;
	}

	size += GetVALUESize2(ver,val->notifyID);
	size += GetVALUESize2(ver,&val->time);
	size += GetVALUESize2(ver,val->param1);
	size += GetVALUESize2(ver,val->param2);
	size += GetVALUESize2(ver,val->param3);
	size += GetVALUESize2(ver,val->param4);
	size += GetVALUESize2(ver,val->param5);
	size += GetVALUESize2(ver,val->param6);
	if( ver>=2 ){
		goto CMD_END;
	}

CMD_END:
	return size;
}

BOOL WriteVALUE2(WORD ver, NOTIFY_SRV_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	DWORD valSize = GetVALUESize2(ver, val );
	if( buff == NULL || valSize > buffSize ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, valSize);

	if(val != NULL ){
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->notifyID);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->time);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->param1);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->param2);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->param3);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->param4);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->param5);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->param6);

		if( ver>=2 ){
			goto CMD_END;
		}
	}

CMD_END:
	if( writeSize != NULL ){
		*writeSize = pos;
	}
	return TRUE;
}

BOOL ReadVALUE2(WORD ver, NOTIFY_SRV_INFO* val, BYTE* buff, DWORD buffSize, DWORD* readSize )
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

DWORD GetVALUESize2(WORD ver, EPGDB_SEARCH_KEY_INFO* val )
{
	DWORD size = sizeof(DWORD);
	if( val == NULL ){
		return size;
	}

	size += GetVALUESize2(ver, val->andKey);
	size += GetVALUESize2(ver, val->notKey);
	size += GetVALUESize2(ver, val->regExpFlag);
	size += GetVALUESize2(ver, val->titleOnlyFlag);
	size += GetVALUESize2(ver, &val->contentList);
	size += GetVALUESize2(ver, &val->dateList);
	size += GetVALUESize2(ver, &val->serviceList);
	size += GetVALUESize2(ver, &val->videoList);
	size += GetVALUESize2(ver, &val->audioList);
	size += GetVALUESize2(ver, val->aimaiFlag);
	size += GetVALUESize2(ver, val->notContetFlag);
	size += GetVALUESize2(ver, val->notDateFlag);
	size += GetVALUESize2(ver, val->freeCAFlag);
	if( ver<=2 ){
		goto CMD_END;
	}
	size += GetVALUESize2(ver, val->chkRecEnd);
	size += GetVALUESize2(ver, val->chkRecDay);
	if( ver>=3 ){
		goto CMD_END;
	}
CMD_END:
	return size;
}

BOOL WriteVALUE2(WORD ver, EPGDB_SEARCH_KEY_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	DWORD valSize = GetVALUESize2(ver, val );
	if( buff == NULL || valSize > buffSize ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, valSize);

	if(val != NULL ){
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->andKey);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->notKey);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->regExpFlag);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->titleOnlyFlag);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->contentList);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->dateList);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->serviceList);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->videoList);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->audioList);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->aimaiFlag);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->notContetFlag);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->notDateFlag);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->freeCAFlag);
		if( ver<=2 ){
			goto CMD_END;
		}
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->chkRecEnd);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->chkRecDay);
		if( ver>=3 ){
			goto CMD_END;
		}
	}

CMD_END:
	if( writeSize != NULL ){
		*writeSize = pos;
	}
	return TRUE;
}

BOOL ReadVALUE2(WORD ver, EPGDB_SEARCH_KEY_INFO* val, BYTE* buff, DWORD buffSize, DWORD* readSize )
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

DWORD GetVALUESize2(WORD ver, EPG_AUTO_ADD_DATA* val )
{
	DWORD size = sizeof(DWORD);
	if( val == NULL ){
		return size;
	}

	size += GetVALUESize2(ver,val->dataID);
	size += GetVALUESize2(ver,&val->searchInfo);
	size += GetVALUESize2(ver,&val->recSetting);

	if( ver<=4 ){
		goto CMD_END;
	}

	size += GetVALUESize2(ver,val->addCount);

	if( ver>=5 ){
		goto CMD_END;
	}

CMD_END:
	return size;
}

BOOL WriteVALUE2(WORD ver, EPG_AUTO_ADD_DATA* val, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	DWORD valSize = GetVALUESize2(ver, val );
	if( buff == NULL || valSize > buffSize ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, valSize);

	if(val != NULL ){
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->dataID);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->searchInfo);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->recSetting);

		if( ver<=4 ){
			goto CMD_END;
		}

		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->addCount);

		if( ver>=5 ){
			goto CMD_END;
		}
	}

CMD_END:
	if( writeSize != NULL ){
		*writeSize = pos;
	}
	return TRUE;
}

BOOL ReadVALUE2(WORD ver, EPG_AUTO_ADD_DATA* val, BYTE* buff, DWORD buffSize, DWORD* readSize )
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

DWORD GetVALUESize2(WORD ver, MANUAL_AUTO_ADD_DATA* val )
{
	DWORD size = sizeof(DWORD);
	if( val == NULL ){
		return size;
	}

	size += GetVALUESize2(ver, val->dataID);
	size += GetVALUESize2(ver, val->dayOfWeekFlag);
	size += GetVALUESize2(ver, val->startTime);
	size += GetVALUESize2(ver, val->durationSecond);
	size += GetVALUESize2(ver, val->title);
	size += GetVALUESize2(ver, val->stationName);
	size += GetVALUESize2(ver, val->originalNetworkID);
	size += GetVALUESize2(ver, val->transportStreamID);
	size += GetVALUESize2(ver, val->serviceID);
	size += GetVALUESize2(ver, &val->recSetting);

	return size;
}

BOOL WriteVALUE2(WORD ver, MANUAL_AUTO_ADD_DATA* val, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	DWORD valSize = GetVALUESize2(ver, val );
	if( buff == NULL || valSize > buffSize ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, valSize);

	if(val != NULL ){
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->dataID);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->dayOfWeekFlag);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->startTime);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->durationSecond);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->title);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->stationName);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->originalNetworkID);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->transportStreamID);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->serviceID);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->recSetting);

		if( ver>=2 ){
			goto CMD_END;
		}
	}

CMD_END:
	if( writeSize != NULL ){
		*writeSize = pos;
	}
	return TRUE;
}

BOOL ReadVALUE2(WORD ver, MANUAL_AUTO_ADD_DATA* val, BYTE* buff, DWORD buffSize, DWORD* readSize )
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

DWORD GetVALUESize2(WORD ver, REC_FILE_INFO* val )
{
	DWORD size = sizeof(DWORD);
	if( val == NULL ){
		return size;
	}

	size += GetVALUESize2(ver, val->id);
	size += GetVALUESize2(ver, val->recFilePath);
	size += GetVALUESize2(ver, val->title);
	size += GetVALUESize2(ver, &val->startTime);
	size += GetVALUESize2(ver, val->durationSecond);
	size += GetVALUESize2(ver, val->serviceName);
	size += GetVALUESize2(ver, val->originalNetworkID);
	size += GetVALUESize2(ver, val->transportStreamID);
	size += GetVALUESize2(ver, val->serviceID);
	size += GetVALUESize2(ver, val->eventID);
	size += GetVALUESize2(ver, val->drops);
	size += GetVALUESize2(ver, val->scrambles);
	size += GetVALUESize2(ver, val->recStatus);
	size += GetVALUESize2(ver, &val->startTimeEpg);
	size += GetVALUESize2(ver, val->comment);
	size += GetVALUESize2(ver, val->programInfo);
	size += GetVALUESize2(ver, val->errInfo);
	if( ver<=3 ){
		goto CMD_END;
	}
	size += GetVALUESize2(ver, val->protectFlag);
	if( ver>=4 ){
		goto CMD_END;
	}

CMD_END:
	return size;
}

BOOL WriteVALUE2(WORD ver, REC_FILE_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	DWORD valSize = GetVALUESize2(ver, val );
	if( buff == NULL || valSize > buffSize ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, valSize);

	if(val != NULL ){
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->id);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->recFilePath);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->title);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->startTime);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->durationSecond);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->serviceName);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->originalNetworkID);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->transportStreamID);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->serviceID);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->eventID);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->drops);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->scrambles);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->recStatus);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, &val->startTimeEpg);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->comment);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->programInfo);
		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->errInfo);

		if( ver<=3 ){
			goto CMD_END;
		}

		WRITE_VALUE2_OR_FAIL(ver, buff, buffSize, pos, size, val->protectFlag);

		if( ver>=4 ){
			goto CMD_END;
		}
	}

CMD_END:
	if( writeSize != NULL ){
		*writeSize = pos;
	}
	return TRUE;
}

BOOL ReadVALUE2(WORD ver, REC_FILE_INFO* val, BYTE* buff, DWORD buffSize, DWORD* readSize )
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
