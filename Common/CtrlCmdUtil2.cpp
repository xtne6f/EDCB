#include "stdafx.h"
#include "CtrlCmdUtil2.h"
#include "Util.h"


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
	if( WriteVALUE2(ver, valSize, buff + pos, buffSize - pos, &size ) == FALSE ){
		return FALSE;
	}
	pos += size;

	if (val != NULL ){
		if( WriteVALUE2(ver, val->recMode, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->priority, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->tuijyuuFlag, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->serviceMode, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->pittariFlag, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->batFilePath, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, &val->recFolderList, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->suspendMode, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->rebootFlag, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->useMargineFlag, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->startMargine, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->endMargine, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->continueRecFlag, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->partialRecFlag, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->tunerID, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, &val->partialRecFolder, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
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
	if( ReadVALUE2(ver, &valSize, buff + pos, buffSize - pos, &size ) == FALSE ){
		return FALSE;
	}
	pos += size;

	if( pos < valSize ){
		if( ReadVALUE2(ver, &val->recMode, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->priority, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->tuijyuuFlag, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->serviceMode, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->pittariFlag, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->batFilePath, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->recFolderList, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->suspendMode, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->rebootFlag, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->useMargineFlag, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->startMargine, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->endMargine, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->continueRecFlag, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->partialRecFlag, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->tunerID, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->partialRecFolder, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
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
	size += GetVALUESize2(ver,val->recWaitFlag);
	size += GetVALUESize2(ver,val->overlapMode);
	size += GetVALUESize2(ver,val->recFilePath);
	size += GetVALUESize2(ver,&val->startTimeEpg);
	size += GetVALUESize2(ver,&val->recSetting);
	size += GetVALUESize2(ver,val->reserveStatus);

	if( ver<=4 ){
		goto CMD_END;
	}

	size += GetVALUESize2(ver,&val->recFileNameList);
	size += GetVALUESize2(ver,val->param1);

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
	if( WriteVALUE2(ver, valSize, buff + pos, buffSize - pos, &size ) == FALSE ){
		return FALSE;
	}
	pos += size;

	if(val != NULL ){
		if( WriteVALUE2(ver, val->title, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, &val->startTime, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->durationSecond, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->stationName, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->originalNetworkID, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->transportStreamID, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->serviceID, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->eventID, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->comment, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->reserveID, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->recWaitFlag, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->overlapMode, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->recFilePath, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, &val->startTimeEpg, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, &val->recSetting, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->reserveStatus, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;

		if( ver<=4 ){
			goto CMD_END;
		}

		if( WriteVALUE2(ver, &val->recFileNameList, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->param1, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;

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
	if( ReadVALUE2(ver, &valSize, buff + pos, buffSize - pos, &size ) == FALSE ){
		return FALSE;
	}
	pos += size;
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		if( ReadVALUE2(ver, &val->title, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->startTime, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->durationSecond, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->stationName, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->originalNetworkID, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->transportStreamID, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->serviceID, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->eventID, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->comment, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->reserveID, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->recWaitFlag, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->overlapMode, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->recFilePath, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->startTimeEpg, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->recSetting, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->reserveStatus, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;

		if( ver<=4 ){
			goto CMD_END;
		}

		if( ReadVALUE2(ver, &val->recFileNameList, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->param1, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;

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
	if( WriteVALUE2(ver, valSize, buff + pos, buffSize - pos, &size ) == FALSE ){
		return FALSE;
	}
	pos += size;

	if(val != NULL ){
		if( WriteVALUE2(ver, val->notifyID, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, &val->time, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->param1, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->param2, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->param3, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->param4, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->param5, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->param6, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;

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
	if( ReadVALUE2(ver, &valSize, buff + pos, buffSize - pos, &size ) == FALSE ){
		return FALSE;
	}
	pos += size;
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		if( ReadVALUE2(ver, &val->notifyID, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->time, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->param1, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->param2, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->param3, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->param4, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->param5, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->param6, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;

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
	if( WriteVALUE2(ver, valSize, buff + pos, buffSize - pos, &size ) == FALSE ){
		return FALSE;
	}
	pos += size;

	if(val != NULL ){
		if( WriteVALUE2(ver, val->andKey, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->notKey, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->regExpFlag, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->titleOnlyFlag, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, &val->contentList, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, &val->dateList, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, &val->serviceList, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, &val->videoList, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, &val->audioList, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->aimaiFlag, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->notContetFlag, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->notDateFlag, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->freeCAFlag, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ver<=2 ){
			goto CMD_END;
		}
		if( WriteVALUE2(ver, val->chkRecEnd, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->chkRecDay, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
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
	if( ReadVALUE2(ver, &valSize, buff + pos, buffSize - pos, &size ) == FALSE ){
		return FALSE;
	}
	pos += size;
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		if( ReadVALUE2(ver, &val->andKey, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->notKey, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->regExpFlag, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->titleOnlyFlag, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->contentList, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->dateList, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->serviceList, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->videoList, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->audioList, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->aimaiFlag, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->notContetFlag, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->notDateFlag, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->freeCAFlag, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ver<=2 ){
			goto CMD_END;
		}
		if( ReadVALUE2(ver, &val->chkRecEnd, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->chkRecDay, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;

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
	if( WriteVALUE2(ver, valSize, buff + pos, buffSize - pos, &size ) == FALSE ){
		return FALSE;
	}
	pos += size;

	if(val != NULL ){
		if( WriteVALUE2(ver, val->dataID, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, &val->searchInfo, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, &val->recSetting, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;

		if( ver<=4 ){
			goto CMD_END;
		}

		if( WriteVALUE2(ver, val->addCount, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;

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
	if( ReadVALUE2(ver, &valSize, buff + pos, buffSize - pos, &size ) == FALSE ){
		return FALSE;
	}
	pos += size;
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		if( ReadVALUE2(ver, &val->dataID, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->searchInfo, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->recSetting, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;

		if( ver<=4 ){
			goto CMD_END;
		}

		if( ReadVALUE2(ver, &val->addCount, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;

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
	if( WriteVALUE2(ver, valSize, buff + pos, buffSize - pos, &size ) == FALSE ){
		return FALSE;
	}
	pos += size;

	if(val != NULL ){
		if( WriteVALUE2(ver, val->dataID, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->dayOfWeekFlag, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->startTime, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->durationSecond, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->title, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->stationName, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->originalNetworkID, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->transportStreamID, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->serviceID, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, &val->recSetting, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;

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
	if( ReadVALUE2(ver, &valSize, buff + pos, buffSize - pos, &size ) == FALSE ){
		return FALSE;
	}
	pos += size;
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		if( ReadVALUE2(ver, &val->dataID, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->dayOfWeekFlag, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->startTime, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->durationSecond, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->title, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->stationName, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->originalNetworkID, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->transportStreamID, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->serviceID, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->recSetting, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;

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
	if( WriteVALUE2(ver, valSize, buff + pos, buffSize - pos, &size ) == FALSE ){
		return FALSE;
	}
	pos += size;

	if(val != NULL ){
		if( WriteVALUE2(ver, val->id, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->recFilePath, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->title, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, &val->startTime, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->durationSecond, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->serviceName, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->originalNetworkID, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->transportStreamID, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->serviceID, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->eventID, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->drops, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->scrambles, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->recStatus, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, &val->startTimeEpg, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->comment, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->programInfo, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( WriteVALUE2(ver, val->errInfo, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;

		if( ver<=3 ){
			goto CMD_END;
		}

		if( WriteVALUE2(ver, val->protectFlag, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;

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
	if( ReadVALUE2(ver, &valSize, buff + pos, buffSize - pos, &size ) == FALSE ){
		return FALSE;
	}
	pos += size;
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		if( ReadVALUE2(ver, &val->id, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->recFilePath, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->title, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->startTime, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->durationSecond, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->serviceName, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->originalNetworkID, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->transportStreamID, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->serviceID, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->eventID, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->drops, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->scrambles, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->recStatus, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->startTimeEpg, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->comment, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->programInfo, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ReadVALUE2(ver, &val->errInfo, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;
		if( ver<=3 ){
			goto CMD_END;
		}

		if( ReadVALUE2(ver, &val->protectFlag, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;

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
