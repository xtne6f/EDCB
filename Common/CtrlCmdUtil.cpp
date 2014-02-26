#include "stdafx.h"
#include "CtrlCmdUtil.h"
#include "Util.h"

//^\s*if *\( *WriteVALUE *\(.*?, *buff *\+ *pos *, *buffSize *- *pos *, *&size *\) *== *FALSE *\) *{ *\r\n^\s*return *FALSE; *\r\n^\s*} *\r\n^\s*pos *\+= *size; *\r\n
#define WRITE_VALUE_OR_FAIL(buff,buffSize,pos,size,val)		{ if( WriteVALUE(val,(buff)+pos,(buffSize)-pos,&size) == FALSE ) return FALSE; pos+=size; }
#define READ_VALUE_OR_FAIL(buff,buffSize,pos,size,val)		{ if( ReadVALUE(val,(buff)+pos,(buffSize)-pos,&size) == FALSE ) return FALSE; pos+=size; }

DWORD GetVALUESize( vector<unsigned short>* val)
{
	DWORD size = sizeof(DWORD)*2;
	if( val == NULL ){
		return size;
	}

	size += sizeof(unsigned short) * (DWORD)val->size();

	return size;
}

DWORD GetVALUESize( vector<unsigned long>* val)
{
	DWORD size = sizeof(DWORD)*2;
	if( val == NULL ){
		return size;
	}

	size += sizeof(unsigned long) * (DWORD)val->size();

	return size;
}

DWORD GetVALUESize( vector<__int64>* val)
{
	DWORD size = sizeof(DWORD)*2;
	if( val == NULL ){
		return size;
	}

	size += sizeof(__int64) * (DWORD)val->size();

	return size;
}

DWORD GetVALUESize( const wstring& val )
{
	return ( (DWORD)val.size() + 1 ) * sizeof(WCHAR) + sizeof(DWORD);
}

BOOL WriteVALUE( const wstring& val, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	DWORD stringBuffSize = GetVALUESize( val );
	if( buff == NULL || stringBuffSize > buffSize ){
		return FALSE;
	}

	ZeroMemory( buff, stringBuffSize );
	//まず全体のサイズ
	DWORD size = 0;
	if( WriteVALUE( stringBuffSize, buff, stringBuffSize, &size ) == FALSE ){
		return FALSE;
	}
	//文字あれば
	if( val.size() > 0 ){
		memcpy(buff + size, val.c_str(), val.size()*sizeof(WCHAR));
	}
	if( writeSize != NULL ){
		*writeSize = stringBuffSize;
	}
	return TRUE;
}

BOOL ReadVALUE( wstring* val, BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	//全体のサイズ
	READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &valSize );
	if( buffSize < valSize ){
		return FALSE;
	}
	
	*val = (WCHAR*)(buff + size);

	if( readSize != NULL ){
		*readSize = valSize;
	}
	return TRUE;
}

DWORD GetVALUESize( REC_SETTING_DATA* val )
{
	DWORD size = sizeof(DWORD);
	if( val == NULL ){
		return size;
	}

	size += GetVALUESize(val->recMode);
	size += GetVALUESize(val->priority);
	size += GetVALUESize(val->tuijyuuFlag);
	size += GetVALUESize(val->serviceMode);
	size += GetVALUESize(val->pittariFlag);
	size += GetVALUESize(val->batFilePath);
	size += GetVALUESize(&val->recFolderList);
	size += GetVALUESize(val->suspendMode);
	size += GetVALUESize(val->rebootFlag);
	size += GetVALUESize(val->useMargineFlag);
	size += GetVALUESize(val->startMargine);
	size += GetVALUESize(val->endMargine);
	size += GetVALUESize(val->continueRecFlag);
	size += GetVALUESize(val->partialRecFlag);
	size += GetVALUESize(val->tunerID);

	return size;
}

BOOL WriteVALUE( REC_SETTING_DATA* val, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	DWORD valSize = GetVALUESize( val );
	if( buff == NULL || valSize > buffSize ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, valSize );

	if (val != NULL ){
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->recMode );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->priority );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->tuijyuuFlag );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->serviceMode );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->pittariFlag );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->batFilePath );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->recFolderList );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->suspendMode );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->rebootFlag );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->useMargineFlag );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->startMargine );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->endMargine );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->continueRecFlag );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->partialRecFlag );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->tunerID );
	}

	if( writeSize != NULL ){
		*writeSize = pos;
	}
	return TRUE;
}

BOOL ReadVALUE( REC_SETTING_DATA* val, BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL || buffSize < sizeof(DWORD) ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &valSize );
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->recMode );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->priority );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->tuijyuuFlag );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->serviceMode );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->pittariFlag );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->batFilePath );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->recFolderList );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->suspendMode );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->rebootFlag );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->useMargineFlag );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->startMargine );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->endMargine );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->continueRecFlag );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->partialRecFlag );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->tunerID );
	}

	if( readSize != NULL ){
		*readSize = pos;
	}

	return TRUE;
}

DWORD GetVALUESize( RESERVE_DATA* val )
{
	DWORD size = sizeof(DWORD);
	if( val == NULL ){
		return size;
	}

	size += GetVALUESize(val->title);
	size += GetVALUESize(&val->startTime);
	size += GetVALUESize(val->durationSecond);
	size += GetVALUESize(val->stationName);
	size += GetVALUESize(val->originalNetworkID);
	size += GetVALUESize(val->transportStreamID);
	size += GetVALUESize(val->serviceID);
	size += GetVALUESize(val->eventID);
	size += GetVALUESize(val->comment);
	size += GetVALUESize(val->reserveID);
	size += GetVALUESize(val->recWaitFlag);
	size += GetVALUESize(val->overlapMode);
	size += GetVALUESize(val->recFilePath);
	size += GetVALUESize(&val->startTimeEpg);
	size += GetVALUESize(&val->recSetting);
	size += GetVALUESize(val->reserveStatus);

	return size;
}

BOOL WriteVALUE( RESERVE_DATA* val, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	DWORD valSize = GetVALUESize( val );
	if( buff == NULL || valSize > buffSize ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, valSize );

	if(val != NULL ){
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->title );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->startTime );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->durationSecond );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->stationName );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->originalNetworkID );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->transportStreamID );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->serviceID );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->eventID );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->comment );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->reserveID );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->recWaitFlag );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->overlapMode );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->recFilePath );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->startTimeEpg );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->recSetting );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->reserveStatus );
	}

	if( writeSize != NULL ){
		*writeSize = pos;
	}
	return TRUE;
}

BOOL ReadVALUE( RESERVE_DATA* val, BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL || buffSize < sizeof(DWORD) ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &valSize );
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->title );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->startTime );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->durationSecond );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->stationName );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->originalNetworkID );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->transportStreamID );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->serviceID );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->eventID );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->comment );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->reserveID );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->recWaitFlag );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->overlapMode );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->recFilePath );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->startTimeEpg );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->recSetting );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->reserveStatus );
	}

	if( readSize != NULL ){
		*readSize = pos;
	}

	return TRUE;
}

DWORD GetVALUESize( EPGDB_SERVICE_INFO* val )
{
	DWORD size = sizeof(DWORD);
	if( val == NULL ){
		return size;
	}

	size += GetVALUESize(val->ONID);
	size += GetVALUESize(val->TSID);
	size += GetVALUESize(val->SID);
	size += GetVALUESize(val->service_type);
	size += GetVALUESize(val->partialReceptionFlag);
	size += GetVALUESize(val->service_provider_name);
	size += GetVALUESize(val->service_name);
	size += GetVALUESize(val->network_name);
	size += GetVALUESize(val->ts_name);
	size += GetVALUESize(val->remote_control_key_id);

	return size;
}

BOOL WriteVALUE( EPGDB_SERVICE_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	DWORD valSize = GetVALUESize( val );
	if( buff == NULL || valSize > buffSize ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, valSize );

	if(val != NULL ){
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->ONID );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->TSID );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->SID );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->service_type );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->partialReceptionFlag );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->service_provider_name );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->service_name );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->network_name );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->ts_name );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->remote_control_key_id );
	}

	if( writeSize != NULL ){
		*writeSize = pos;
	}
	return TRUE;
}

BOOL ReadVALUE( EPGDB_SERVICE_INFO* val, BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL || buffSize < sizeof(DWORD) ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &valSize );
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->ONID );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->TSID );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->SID );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->service_type );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->partialReceptionFlag );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->service_provider_name );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->service_name );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->network_name );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->ts_name );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->remote_control_key_id );
	}

	if( readSize != NULL ){
		*readSize = pos;
	}

	return TRUE;
}

DWORD GetVALUESize( EPGDB_SHORT_EVENT_INFO* val )
{
	DWORD size = sizeof(DWORD);
	if( val == NULL ){
		return size;
	}

	size += GetVALUESize(val->event_name);
	size += GetVALUESize(val->text_char);

	return size;
}

BOOL WriteVALUE( EPGDB_SHORT_EVENT_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	DWORD valSize = GetVALUESize( val );
	if( buff == NULL || valSize > buffSize ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, valSize );

	if(val != NULL ){
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->event_name );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->text_char );
	}

	if( writeSize != NULL ){
		*writeSize = pos;
	}
	return TRUE;
}

BOOL ReadVALUE( EPGDB_SHORT_EVENT_INFO* val, BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL || buffSize < sizeof(DWORD) ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &valSize );
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->event_name );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->text_char );
	}

	if( readSize != NULL ){
		*readSize = pos;
	}

	return TRUE;
}

DWORD GetVALUESize( EPGDB_EXTENDED_EVENT_INFO* val )
{
	DWORD size = sizeof(DWORD);
	if( val == NULL ){
		return size;
	}

	size += GetVALUESize(val->text_char);

	return size;
}

BOOL WriteVALUE( EPGDB_EXTENDED_EVENT_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	DWORD valSize = GetVALUESize( val );
	if( buff == NULL || valSize > buffSize ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, valSize );

	if(val != NULL ){
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->text_char );
	}

	if( writeSize != NULL ){
		*writeSize = pos;
	}
	return TRUE;
}

BOOL ReadVALUE( EPGDB_EXTENDED_EVENT_INFO* val, BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL || buffSize < sizeof(DWORD) ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &valSize );
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->text_char );
	}

	if( readSize != NULL ){
		*readSize = pos;
	}

	return TRUE;
}

DWORD GetVALUESize( EPGDB_CONTENT_DATA* val )
{
	DWORD size = sizeof(DWORD);
	if( val == NULL ){
		return size;
	}

	size += GetVALUESize(val->content_nibble_level_1);
	size += GetVALUESize(val->content_nibble_level_2);
	size += GetVALUESize(val->user_nibble_1);
	size += GetVALUESize(val->user_nibble_2);

	return size;
}

BOOL WriteVALUE( EPGDB_CONTENT_DATA* val, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	DWORD valSize = GetVALUESize( val );
	if( buff == NULL || valSize > buffSize ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, valSize );

	if(val != NULL ){
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->content_nibble_level_1 );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->content_nibble_level_2 );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->user_nibble_1 );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->user_nibble_2 );
	}

	if( writeSize != NULL ){
		*writeSize = pos;
	}
	return TRUE;
}

BOOL ReadVALUE( EPGDB_CONTENT_DATA* val, BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL || buffSize < sizeof(DWORD) ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &valSize );
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->content_nibble_level_1 );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->content_nibble_level_2 );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->user_nibble_1 );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->user_nibble_2 );
	}

	if( readSize != NULL ){
		*readSize = pos;
	}

	return TRUE;
}

DWORD GetVALUESize( EPGDB_CONTEN_INFO* val )
{
	DWORD size = sizeof(DWORD);
	if( val == NULL ){
		return size;
	}

	size += GetVALUESize(&val->nibbleList);

	return size;
}

BOOL WriteVALUE( EPGDB_CONTEN_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	DWORD valSize = GetVALUESize( val );
	if( buff == NULL || valSize > buffSize ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, valSize );

	if(val != NULL ){
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->nibbleList );
	}

	if( writeSize != NULL ){
		*writeSize = pos;
	}
	return TRUE;
}

BOOL ReadVALUE( EPGDB_CONTEN_INFO* val, BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL || buffSize < sizeof(DWORD) ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &valSize );
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->nibbleList );
	}

	if( readSize != NULL ){
		*readSize = pos;
	}

	return TRUE;
}

DWORD GetVALUESize( EPGDB_COMPONENT_INFO* val )
{
	DWORD size = sizeof(DWORD);
	if( val == NULL ){
		return size;
	}

	size += GetVALUESize(val->stream_content);
	size += GetVALUESize(val->component_type);
	size += GetVALUESize(val->component_tag);
	size += GetVALUESize(val->text_char);

	return size;
}

BOOL WriteVALUE( EPGDB_COMPONENT_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	DWORD valSize = GetVALUESize( val );
	if( buff == NULL || valSize > buffSize ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, valSize );

	if(val != NULL ){
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->stream_content );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->component_type );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->component_tag );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->text_char );
	}

	if( writeSize != NULL ){
		*writeSize = pos;
	}
	return TRUE;
}

BOOL ReadVALUE( EPGDB_COMPONENT_INFO* val, BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL || buffSize < sizeof(DWORD) ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &valSize );
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->stream_content );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->component_type );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->component_tag );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->text_char );
	}

	if( readSize != NULL ){
		*readSize = pos;
	}

	return TRUE;
}

DWORD GetVALUESize( EPGDB_AUDIO_COMPONENT_INFO_DATA* val )
{
	DWORD size = sizeof(DWORD);
	if( val == NULL ){
		return size;
	}

	size += GetVALUESize(val->stream_content);
	size += GetVALUESize(val->component_type);
	size += GetVALUESize(val->component_tag);
	size += GetVALUESize(val->stream_type);
	size += GetVALUESize(val->simulcast_group_tag);
	size += GetVALUESize(val->ES_multi_lingual_flag);
	size += GetVALUESize(val->main_component_flag);
	size += GetVALUESize(val->quality_indicator);
	size += GetVALUESize(val->sampling_rate);
	size += GetVALUESize(val->text_char);

	return size;
}

BOOL WriteVALUE( EPGDB_AUDIO_COMPONENT_INFO_DATA* val, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	DWORD valSize = GetVALUESize( val );
	if( buff == NULL || valSize > buffSize ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, valSize );

	if(val != NULL ){
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->stream_content );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->component_type );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->component_tag );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->stream_type );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->simulcast_group_tag );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->ES_multi_lingual_flag );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->main_component_flag );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->quality_indicator );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->sampling_rate );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->text_char );
	}

	if( writeSize != NULL ){
		*writeSize = pos;
	}
	return TRUE;
}

BOOL ReadVALUE( EPGDB_AUDIO_COMPONENT_INFO_DATA* val, BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL || buffSize < sizeof(DWORD) ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &valSize );
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->stream_content );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->component_type );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->component_tag );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->stream_type );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->simulcast_group_tag );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->ES_multi_lingual_flag );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->main_component_flag );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->quality_indicator );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->sampling_rate );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->text_char );
	}

	if( readSize != NULL ){
		*readSize = pos;
	}

	return TRUE;
}

DWORD GetVALUESize( EPGDB_AUDIO_COMPONENT_INFO* val )
{
	DWORD size = sizeof(DWORD);
	if( val == NULL ){
		return size;
	}

	size += GetVALUESize(&val->componentList);

	return size;
}

BOOL WriteVALUE( EPGDB_AUDIO_COMPONENT_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	DWORD valSize = GetVALUESize( val );
	if( buff == NULL || valSize > buffSize ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, valSize );

	if(val != NULL ){
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->componentList );
	}

	if( writeSize != NULL ){
		*writeSize = pos;
	}
	return TRUE;
}

BOOL ReadVALUE( EPGDB_AUDIO_COMPONENT_INFO* val, BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL || buffSize < sizeof(DWORD) ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &valSize );
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->componentList );
	}

	if( readSize != NULL ){
		*readSize = pos;
	}

	return TRUE;
}

DWORD GetVALUESize( EPGDB_EVENT_DATA* val )
{
	DWORD size = sizeof(DWORD);
	if( val == NULL ){
		return size;
	}

	size += GetVALUESize(val->original_network_id);
	size += GetVALUESize(val->transport_stream_id);
	size += GetVALUESize(val->service_id);
	size += GetVALUESize(val->event_id);

	return size;
}

BOOL WriteVALUE( EPGDB_EVENT_DATA* val, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	DWORD valSize = GetVALUESize( val );
	if( buff == NULL || valSize > buffSize ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, valSize );

	if(val != NULL ){
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->original_network_id );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->transport_stream_id );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->service_id );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->event_id );
	}

	if( writeSize != NULL ){
		*writeSize = pos;
	}
	return TRUE;
}

BOOL ReadVALUE( EPGDB_EVENT_DATA* val, BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL || buffSize < sizeof(DWORD) ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &valSize );
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->original_network_id );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->transport_stream_id );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->service_id );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->event_id );
	}

	if( readSize != NULL ){
		*readSize = pos;
	}

	return TRUE;
}

DWORD GetVALUESize( EPGDB_EVENTGROUP_INFO* val )
{
	DWORD size = sizeof(DWORD);
	if( val == NULL ){
		return size;
	}

	size += GetVALUESize(val->group_type);
	size += GetVALUESize(&val->eventDataList);

	return size;
}

BOOL WriteVALUE( EPGDB_EVENTGROUP_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	DWORD valSize = GetVALUESize( val );
	if( buff == NULL || valSize > buffSize ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, valSize );

	if(val != NULL ){
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->group_type );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->eventDataList );
	}

	if( writeSize != NULL ){
		*writeSize = pos;
	}
	return TRUE;
}

BOOL ReadVALUE( EPGDB_EVENTGROUP_INFO* val, BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL || buffSize < sizeof(DWORD) ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &valSize );
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->group_type );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->eventDataList );
	}

	if( readSize != NULL ){
		*readSize = pos;
	}

	return TRUE;
}

DWORD GetVALUESize( EPGDB_EVENT_INFO* val )
{
	DWORD size = sizeof(DWORD);
	if( val == NULL ){
		return size;
	}

	size += GetVALUESize(val->original_network_id);
	size += GetVALUESize(val->transport_stream_id);
	size += GetVALUESize(val->service_id);
	size += GetVALUESize(val->event_id);
	size += GetVALUESize(val->StartTimeFlag);
	size += GetVALUESize(&val->start_time);
	size += GetVALUESize(val->DurationFlag);
	size += GetVALUESize(val->durationSec);

	size += GetVALUESize(val->shortInfo);
	size += GetVALUESize(val->extInfo);
	size += GetVALUESize(val->contentInfo);
	size += GetVALUESize(val->componentInfo);
	size += GetVALUESize(val->audioInfo);
	size += GetVALUESize(val->eventGroupInfo);
	size += GetVALUESize(val->eventRelayInfo);

	size += GetVALUESize(val->freeCAFlag);

	return size;
}

BOOL WriteVALUE( EPGDB_EVENT_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	DWORD valSize = GetVALUESize( val );
	if( buff == NULL || valSize > buffSize ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, valSize );

	if( val != NULL ){
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->original_network_id );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->transport_stream_id );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->service_id );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->event_id );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->StartTimeFlag );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->start_time );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->DurationFlag );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->durationSec );

		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->shortInfo );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->extInfo );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->contentInfo );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->componentInfo );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->audioInfo );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->eventGroupInfo );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->eventRelayInfo );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->freeCAFlag );
	}

	if( writeSize != NULL ){
		*writeSize = pos;
	}
	return TRUE;
}

BOOL ReadVALUE( EPGDB_EVENT_INFO* val, BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL || buffSize < sizeof(DWORD) ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &valSize );
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->original_network_id );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->transport_stream_id );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->service_id );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->event_id );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->StartTimeFlag );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->start_time );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->DurationFlag );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->durationSec );

		{
			EPGDB_SHORT_EVENT_INFO* info = new EPGDB_SHORT_EVENT_INFO;
			if( ReadVALUE( info, buff + pos, buffSize - pos, &size ) == FALSE ){
				SAFE_DELETE(info);
				return FALSE;
			}
			if( size == sizeof(DWORD) ){
				SAFE_DELETE(info);
				val->shortInfo = NULL;
			}else{
				val->shortInfo = info;
			}
			pos += size;
		}

		{
			EPGDB_EXTENDED_EVENT_INFO* info = new EPGDB_EXTENDED_EVENT_INFO;
			if( ReadVALUE( info, buff + pos, buffSize - pos, &size ) == FALSE ){
				SAFE_DELETE(info);
				return FALSE;
			}
			if( size == sizeof(DWORD) ){
				SAFE_DELETE(info);
				val->extInfo = NULL;
			}else{
				val->extInfo = info;
			}
			pos += size;
		}

		{
			EPGDB_CONTEN_INFO* info = new EPGDB_CONTEN_INFO;
			if( ReadVALUE( info, buff + pos, buffSize - pos, &size ) == FALSE ){
				SAFE_DELETE(info);
				return FALSE;
			}
			if( size == sizeof(DWORD) ){
				SAFE_DELETE(info);
				val->contentInfo = NULL;
			}else{
				val->contentInfo = info;
			}
			pos += size;
		}

		{
			EPGDB_COMPONENT_INFO* info = new EPGDB_COMPONENT_INFO;
			if( ReadVALUE( info, buff + pos, buffSize - pos, &size ) == FALSE ){
				SAFE_DELETE(info);
				return FALSE;
			}
			if( size == sizeof(DWORD) ){
				SAFE_DELETE(info);
				val->componentInfo = NULL;
			}else{
				val->componentInfo = info;
			}
			pos += size;
		}

		{
			EPGDB_AUDIO_COMPONENT_INFO* info = new EPGDB_AUDIO_COMPONENT_INFO;
			if( ReadVALUE( info, buff + pos, buffSize - pos, &size ) == FALSE ){
				SAFE_DELETE(info);
				return FALSE;
			}
			if( size == sizeof(DWORD) ){
				SAFE_DELETE(info);
				val->audioInfo = NULL;
			}else{
				val->audioInfo = info;
			}
			pos += size;
		}

		{
			EPGDB_EVENTGROUP_INFO* info = new EPGDB_EVENTGROUP_INFO;
			if( ReadVALUE( info, buff + pos, buffSize - pos, &size ) == FALSE ){
				SAFE_DELETE(info);
				return FALSE;
			}
			if( size == sizeof(DWORD) ){
				SAFE_DELETE(info);
				val->eventGroupInfo = NULL;
			}else{
				val->eventGroupInfo = info;
			}
			pos += size;
		}

		{
			EPGDB_EVENTGROUP_INFO* info = new EPGDB_EVENTGROUP_INFO;
			if( ReadVALUE( info, buff + pos, buffSize - pos, &size ) == FALSE ){
				SAFE_DELETE(info);
				return FALSE;
			}
			if( size == sizeof(DWORD) ){
				SAFE_DELETE(info);
				val->eventRelayInfo = NULL;
			}else{
				val->eventRelayInfo = info;
			}
			pos += size;
		}

		if( ReadVALUE( &val->freeCAFlag, buff + pos, buffSize - pos, &size ) == FALSE ){
			return FALSE;
		}
		pos += size;

	}

	if( readSize != NULL ){
		*readSize = pos;
	}

	return TRUE;
}

DWORD GetVALUESize( EPGDB_SEARCH_DATE_INFO* val )
{
	DWORD size = sizeof(DWORD);
	if( val == NULL ){
		return size;
	}

	size += GetVALUESize(val->startDayOfWeek);
	size += GetVALUESize(val->startHour);
	size += GetVALUESize(val->startMin);
	size += GetVALUESize(val->endDayOfWeek);
	size += GetVALUESize(val->endHour);
	size += GetVALUESize(val->endMin);

	return size;
}

BOOL WriteVALUE( EPGDB_SEARCH_DATE_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	DWORD valSize = GetVALUESize( val );
	if( buff == NULL || valSize > buffSize ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, valSize );

	if(val != NULL ){
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->startDayOfWeek );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->startHour );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->startMin );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->endDayOfWeek );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->endHour );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->endMin );
	}

	if( writeSize != NULL ){
		*writeSize = pos;
	}
	return TRUE;
}

BOOL ReadVALUE( EPGDB_SEARCH_DATE_INFO* val, BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL || buffSize < sizeof(DWORD) ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &valSize );
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->startDayOfWeek );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->startHour );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->startMin );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->endDayOfWeek );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->endHour );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->endMin );
	}

	if( readSize != NULL ){
		*readSize = pos;
	}

	return TRUE;
}

DWORD GetVALUESize( EPGDB_SEARCH_KEY_INFO* val )
{
	DWORD size = sizeof(DWORD);
	if( val == NULL ){
		return size;
	}

	size += GetVALUESize(val->andKey);
	size += GetVALUESize(val->notKey);
	size += GetVALUESize(val->regExpFlag);
	size += GetVALUESize(val->titleOnlyFlag);
	size += GetVALUESize(&val->contentList);
	size += GetVALUESize(&val->dateList);
	size += GetVALUESize(&val->serviceList);
	size += GetVALUESize(&val->videoList);
	size += GetVALUESize(&val->audioList);
	size += GetVALUESize(val->aimaiFlag);
	size += GetVALUESize(val->notContetFlag);
	size += GetVALUESize(val->notDateFlag);
	size += GetVALUESize(val->freeCAFlag);

	return size;
}

BOOL WriteVALUE( EPGDB_SEARCH_KEY_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	DWORD valSize = GetVALUESize( val );
	if( buff == NULL || valSize > buffSize ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, valSize );

	if(val != NULL ){
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->andKey );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->notKey );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->regExpFlag );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->titleOnlyFlag );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->contentList );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->dateList );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->serviceList );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->videoList );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->audioList );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->aimaiFlag );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->notContetFlag );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->notDateFlag );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->freeCAFlag );
	}

	if( writeSize != NULL ){
		*writeSize = pos;
	}
	return TRUE;
}

BOOL ReadVALUE( EPGDB_SEARCH_KEY_INFO* val, BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL || buffSize < sizeof(DWORD) ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &valSize );
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->andKey );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->notKey );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->regExpFlag );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->titleOnlyFlag );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->contentList );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->dateList );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->serviceList );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->videoList );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->audioList );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->aimaiFlag );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->notContetFlag );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->notDateFlag );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->freeCAFlag );
	}

	if( readSize != NULL ){
		*readSize = pos;
	}

	return TRUE;
}

DWORD GetVALUESize( SET_CH_INFO* val )
{
	DWORD size = sizeof(DWORD);
	if( val == NULL ){
		return size;
	}

	size += GetVALUESize(val->useSID);
	size += GetVALUESize(val->ONID);
	size += GetVALUESize(val->TSID);
	size += GetVALUESize(val->SID);
	size += GetVALUESize(val->useBonCh);
	size += GetVALUESize(val->space);
	size += GetVALUESize(val->ch);

	return size;
}

BOOL WriteVALUE( SET_CH_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	DWORD valSize = GetVALUESize( val );
	if( buff == NULL || valSize > buffSize ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, valSize );

	if(val != NULL ){
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->useSID );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->ONID );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->TSID );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->SID );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->useBonCh );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->space );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->ch );
	}

	if( writeSize != NULL ){
		*writeSize = pos;
	}
	return TRUE;
}

BOOL ReadVALUE( SET_CH_INFO* val, BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL || buffSize < sizeof(DWORD) ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &valSize );
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->useSID );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->ONID );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->TSID );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->SID );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->useBonCh );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->space );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->ch );
	}

	if( readSize != NULL ){
		*readSize = pos;
	}

	return TRUE;
}

DWORD GetVALUESize( SET_CTRL_MODE* val )
{
	DWORD size = sizeof(DWORD);
	if( val == NULL ){
		return size;
	}

	size += GetVALUESize(val->ctrlID);
	size += GetVALUESize(val->SID);
	size += GetVALUESize(val->enableScramble);
	size += GetVALUESize(val->enableCaption);
	size += GetVALUESize(val->enableData);

	return size;
}

BOOL WriteVALUE( SET_CTRL_MODE* val, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	DWORD valSize = GetVALUESize( val );
	if( buff == NULL || valSize > buffSize ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, valSize );

	if(val != NULL ){
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->ctrlID );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->SID );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->enableScramble );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->enableCaption );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->enableData );

	}

	if( writeSize != NULL ){
		*writeSize = pos;
	}
	return TRUE;
}

BOOL ReadVALUE( SET_CTRL_MODE* val, BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL || buffSize < sizeof(DWORD) ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &valSize );
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->ctrlID );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->SID );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->enableScramble );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->enableCaption );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->enableData );

	}

	if( readSize != NULL ){
		*readSize = pos;
	}

	return TRUE;
}

DWORD GetVALUESize( REC_FILE_SET_INFO* val )
{
	DWORD size = sizeof(DWORD);
	if( val == NULL ){
		return size;
	}

	size += GetVALUESize(val->recFolder);
	size += GetVALUESize(val->writePlugIn);
	size += GetVALUESize(val->recNamePlugIn);
	size += GetVALUESize(val->recFileName);

	return size;
}

BOOL WriteVALUE( REC_FILE_SET_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	DWORD valSize = GetVALUESize( val );
	if( buff == NULL || valSize > buffSize ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, valSize );

	if(val != NULL ){
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->recFolder );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->writePlugIn );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->recNamePlugIn );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->recFileName );
	}

	if( writeSize != NULL ){
		*writeSize = pos;
	}
	return TRUE;
}

BOOL ReadVALUE( REC_FILE_SET_INFO* val, BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL || buffSize < sizeof(DWORD) ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &valSize );
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->recFolder );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->writePlugIn );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->recNamePlugIn );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->recFileName );
	}

	if( readSize != NULL ){
		*readSize = pos;
	}

	return TRUE;
}

DWORD GetVALUESize( SET_CTRL_REC_PARAM* val )
{
	DWORD size = sizeof(DWORD);
	if( val == NULL ){
		return size;
	}

	size += GetVALUESize(val->ctrlID);
	size += GetVALUESize(val->fileName);
	size += GetVALUESize(val->overWriteFlag);
	size += GetVALUESize(val->createSize);
	size += GetVALUESize(&val->saveFolder);
	size += GetVALUESize(val->pittariFlag);
	size += GetVALUESize(val->pittariONID);
	size += GetVALUESize(val->pittariTSID);
	size += GetVALUESize(val->pittariSID);
	size += GetVALUESize(val->pittariEventID);

	return size;
}

BOOL WriteVALUE( SET_CTRL_REC_PARAM* val, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	DWORD valSize = GetVALUESize( val );
	if( buff == NULL || valSize > buffSize ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, valSize );

	if(val != NULL ){
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->ctrlID );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->fileName );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->overWriteFlag );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->createSize );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->saveFolder );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->pittariFlag );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->pittariONID );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->pittariTSID );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->pittariSID );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->pittariEventID );

	}

	if( writeSize != NULL ){
		*writeSize = pos;
	}
	return TRUE;
}

BOOL ReadVALUE( SET_CTRL_REC_PARAM* val, BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL || buffSize < sizeof(DWORD) ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &valSize );
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->ctrlID );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->fileName );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->overWriteFlag );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->createSize );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->saveFolder );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->pittariFlag );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->pittariONID );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->pittariTSID );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->pittariSID );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->pittariEventID );

	}

	if( readSize != NULL ){
		*readSize = pos;
	}

	return TRUE;
}

DWORD GetVALUESize( SET_CTRL_REC_STOP_PARAM* val )
{
	DWORD size = sizeof(DWORD);
	if( val == NULL ){
		return size;
	}

	size += GetVALUESize(val->ctrlID);
	size += GetVALUESize(val->saveErrLog);

	return size;
}

BOOL WriteVALUE( SET_CTRL_REC_STOP_PARAM* val, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	DWORD valSize = GetVALUESize( val );
	if( buff == NULL || valSize > buffSize ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, valSize );

	if(val != NULL ){
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->ctrlID );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->saveErrLog );
	}

	if( writeSize != NULL ){
		*writeSize = pos;
	}
	return TRUE;
}

BOOL ReadVALUE( SET_CTRL_REC_STOP_PARAM* val, BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL || buffSize < sizeof(DWORD) ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &valSize );
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->ctrlID );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->saveErrLog );
	}

	if( readSize != NULL ){
		*readSize = pos;
	}

	return TRUE;
}

DWORD GetVALUESize( SET_CTRL_REC_STOP_RES_PARAM* val )
{
	DWORD size = sizeof(DWORD);
	if( val == NULL ){
		return size;
	}

	size += GetVALUESize(val->recFilePath);
	size += GetVALUESize(val->drop);
	size += GetVALUESize(val->scramble);
	size += GetVALUESize(val->subRecFlag);

	return size;
}

BOOL WriteVALUE( SET_CTRL_REC_STOP_RES_PARAM* val, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	DWORD valSize = GetVALUESize( val );
	if( buff == NULL || valSize > buffSize ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, valSize );

	if(val != NULL ){
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->recFilePath );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->drop );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->scramble );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->subRecFlag );
	}

	if( writeSize != NULL ){
		*writeSize = pos;
	}
	return TRUE;
}

BOOL ReadVALUE( SET_CTRL_REC_STOP_RES_PARAM* val, BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL || buffSize < sizeof(DWORD) ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &valSize );
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->recFilePath );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->drop );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->scramble );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->subRecFlag );
	}

	if( readSize != NULL ){
		*readSize = pos;
	}

	return TRUE;
}

DWORD GetVALUESize( REC_FILE_INFO* val )
{
	DWORD size = sizeof(DWORD);
	if( val == NULL ){
		return size;
	}

	size += GetVALUESize(val->id);
	size += GetVALUESize(val->recFilePath);
	size += GetVALUESize(val->title);
	size += GetVALUESize(&val->startTime);
	size += GetVALUESize(val->durationSecond);
	size += GetVALUESize(val->serviceName);
	size += GetVALUESize(val->originalNetworkID);
	size += GetVALUESize(val->transportStreamID);
	size += GetVALUESize(val->serviceID);
	size += GetVALUESize(val->eventID);
	size += GetVALUESize(val->drops);
	size += GetVALUESize(val->scrambles);
	size += GetVALUESize(val->recStatus);
	size += GetVALUESize(&val->startTimeEpg);
	size += GetVALUESize(val->comment);
	size += GetVALUESize(val->programInfo);
	size += GetVALUESize(val->errInfo);

	return size;
}

BOOL WriteVALUE( REC_FILE_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	DWORD valSize = GetVALUESize( val );
	if( buff == NULL || valSize > buffSize ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, valSize );

	if(val != NULL ){
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->id );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->recFilePath );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->title );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->startTime );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->durationSecond );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->serviceName );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->originalNetworkID );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->transportStreamID );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->serviceID );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->eventID );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->drops );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->scrambles );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->recStatus );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->startTimeEpg );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->comment );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->programInfo );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->errInfo );
	}

	if( writeSize != NULL ){
		*writeSize = pos;
	}
	return TRUE;
}

BOOL ReadVALUE( REC_FILE_INFO* val, BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL || buffSize < sizeof(DWORD) ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &valSize );
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->id );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->recFilePath );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->title );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->startTime );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->durationSecond );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->serviceName );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->originalNetworkID );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->transportStreamID );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->serviceID );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->eventID );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->drops );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->scrambles );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->recStatus );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->startTimeEpg );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->comment );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->programInfo );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->errInfo );
	}

	if( readSize != NULL ){
		*readSize = pos;
	}

	return TRUE;
}

DWORD GetVALUESize( EPG_AUTO_ADD_DATA* val )
{
	DWORD size = sizeof(DWORD);
	if( val == NULL ){
		return size;
	}

	size += GetVALUESize(val->dataID);
	size += GetVALUESize(&val->searchInfo);
	size += GetVALUESize(&val->recSetting);

	return size;
}

BOOL WriteVALUE( EPG_AUTO_ADD_DATA* val, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	DWORD valSize = GetVALUESize( val );
	if( buff == NULL || valSize > buffSize ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, valSize );

	if(val != NULL ){
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->dataID );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->searchInfo );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->recSetting );
	}

	if( writeSize != NULL ){
		*writeSize = pos;
	}
	return TRUE;
}

BOOL ReadVALUE( EPG_AUTO_ADD_DATA* val, BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL || buffSize < sizeof(DWORD) ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &valSize );
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->dataID );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->searchInfo );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->recSetting );
	}

	if( readSize != NULL ){
		*readSize = pos;
	}

	return TRUE;
}

DWORD GetVALUESize( SEARCH_EPG_INFO_PARAM* val )
{
	DWORD size = sizeof(DWORD);
	if( val == NULL ){
		return size;
	}

	size += GetVALUESize(val->ONID);
	size += GetVALUESize(val->TSID);
	size += GetVALUESize(val->SID);
	size += GetVALUESize(val->eventID);
	size += GetVALUESize(val->pfOnlyFlag);

	return size;
}

BOOL WriteVALUE( SEARCH_EPG_INFO_PARAM* val, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	DWORD valSize = GetVALUESize( val );
	if( buff == NULL || valSize > buffSize ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, valSize );

	if(val != NULL ){
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->ONID );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->TSID );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->SID );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->eventID );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->pfOnlyFlag );
	}

	if( writeSize != NULL ){
		*writeSize = pos;
	}
	return TRUE;
}

BOOL ReadVALUE( SEARCH_EPG_INFO_PARAM* val, BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL || buffSize < sizeof(DWORD) ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &valSize );
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->ONID );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->TSID );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->SID );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->eventID );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->pfOnlyFlag );
	}

	if( readSize != NULL ){
		*readSize = pos;
	}

	return TRUE;
}

DWORD GetVALUESize( GET_EPG_PF_INFO_PARAM* val )
{
	DWORD size = sizeof(DWORD);
	if( val == NULL ){
		return size;
	}

	size += GetVALUESize(val->ONID);
	size += GetVALUESize(val->TSID);
	size += GetVALUESize(val->SID);
	size += GetVALUESize(val->pfNextFlag);

	return size;
}

BOOL WriteVALUE( GET_EPG_PF_INFO_PARAM* val, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	DWORD valSize = GetVALUESize( val );
	if( buff == NULL || valSize > buffSize ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, valSize );

	if(val != NULL ){
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->ONID );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->TSID );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->SID );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->pfNextFlag );
	}

	if( writeSize != NULL ){
		*writeSize = pos;
	}
	return TRUE;
}

BOOL ReadVALUE( GET_EPG_PF_INFO_PARAM* val, BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL || buffSize < sizeof(DWORD) ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &valSize );
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->ONID );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->TSID );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->SID );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->pfNextFlag );
	}

	if( readSize != NULL ){
		*readSize = pos;
	}

	return TRUE;
}

DWORD GetVALUESize( MANUAL_AUTO_ADD_DATA* val )
{
	DWORD size = sizeof(DWORD);
	if( val == NULL ){
		return size;
	}

	size += GetVALUESize(val->dataID);
	size += GetVALUESize(val->dayOfWeekFlag);
	size += GetVALUESize(val->startTime);
	size += GetVALUESize(val->durationSecond);
	size += GetVALUESize(val->title);
	size += GetVALUESize(val->stationName);
	size += GetVALUESize(val->originalNetworkID);
	size += GetVALUESize(val->transportStreamID);
	size += GetVALUESize(val->serviceID);
	size += GetVALUESize(&val->recSetting);

	return size;
}

BOOL WriteVALUE( MANUAL_AUTO_ADD_DATA* val, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	DWORD valSize = GetVALUESize( val );
	if( buff == NULL || valSize > buffSize ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, valSize );

	if(val != NULL ){
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->dataID );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->dayOfWeekFlag );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->startTime );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->durationSecond );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->title );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->stationName );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->originalNetworkID );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->transportStreamID );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->serviceID );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->recSetting );
	}

	if( writeSize != NULL ){
		*writeSize = pos;
	}
	return TRUE;
}

BOOL ReadVALUE( MANUAL_AUTO_ADD_DATA* val, BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL || buffSize < sizeof(DWORD) ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &valSize );
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->dataID );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->dayOfWeekFlag );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->startTime );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->durationSecond );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->title );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->stationName );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->originalNetworkID );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->transportStreamID );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->serviceID );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->recSetting );
	}

	if( readSize != NULL ){
		*readSize = pos;
	}

	return TRUE;
}

DWORD GetVALUESize( TUNER_RESERVE_INFO* val )
{
	DWORD size = sizeof(DWORD);
	if( val == NULL ){
		return size;
	}

	size += GetVALUESize(val->tunerID);
	size += GetVALUESize(val->tunerName);
	size += GetVALUESize(&val->reserveList);

	return size;
}

BOOL WriteVALUE( TUNER_RESERVE_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	DWORD valSize = GetVALUESize( val );
	if( buff == NULL || valSize > buffSize ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, valSize );

	if(val != NULL ){
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->tunerID );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->tunerName );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->reserveList );

	}

	if( writeSize != NULL ){
		*writeSize = pos;
	}
	return TRUE;
}

BOOL ReadVALUE( TUNER_RESERVE_INFO* val, BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL || buffSize < sizeof(DWORD) ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &valSize );
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->tunerID );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->tunerName );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->reserveList );

	}

	if( readSize != NULL ){
		*readSize = pos;
	}

	return TRUE;
}

DWORD GetVALUESize( REGIST_TCP_INFO* val )
{
	DWORD size = sizeof(DWORD);
	if( val == NULL ){
		return size;
	}

	size += GetVALUESize(val->ip);
	size += GetVALUESize(val->port);

	return size;
}

BOOL WriteVALUE( REGIST_TCP_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	DWORD valSize = GetVALUESize( val );
	if( buff == NULL || valSize > buffSize ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, valSize );

	if(val != NULL ){
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->ip );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->port );
	}

	if( writeSize != NULL ){
		*writeSize = pos;
	}
	return TRUE;
}

BOOL ReadVALUE( REGIST_TCP_INFO* val, BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL || buffSize < sizeof(DWORD) ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &valSize );
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->ip );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->port );
	}

	if( readSize != NULL ){
		*readSize = pos;
	}

	return TRUE;
}

DWORD GetVALUESize( EPGDB_SERVICE_EVENT_INFO* val )
{
	DWORD size = sizeof(DWORD);
	if( val == NULL ){
		return size;
	}

	size += GetVALUESize(&val->serviceInfo);
	size += GetVALUESize(&val->eventList);

	return size;
}

BOOL WriteVALUE( EPGDB_SERVICE_EVENT_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	DWORD valSize = GetVALUESize( val );
	if( buff == NULL || valSize > buffSize ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, valSize );

	if(val != NULL ){
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->serviceInfo );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->eventList );
	}

	if( writeSize != NULL ){
		*writeSize = pos;
	}
	return TRUE;
}

BOOL ReadVALUE( EPGDB_SERVICE_EVENT_INFO* val, BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL || buffSize < sizeof(DWORD) ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &valSize );
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->serviceInfo );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->eventList );
	}

	if( readSize != NULL ){
		*readSize = pos;
	}

	return TRUE;
}

DWORD GetVALUESize( TVTEST_CH_CHG_INFO* val )
{
	DWORD size = sizeof(DWORD);
	if( val == NULL ){
		return size;
	}

	size += GetVALUESize(val->bonDriver);
	size += GetVALUESize(&val->chInfo);

	return size;
}

BOOL WriteVALUE( TVTEST_CH_CHG_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	DWORD valSize = GetVALUESize( val );
	if( buff == NULL || valSize > buffSize ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, valSize );

	if(val != NULL ){
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->bonDriver );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->chInfo );
	}

	if( writeSize != NULL ){
		*writeSize = pos;
	}
	return TRUE;
}

BOOL ReadVALUE( TVTEST_CH_CHG_INFO* val, BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL || buffSize < sizeof(DWORD) ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &valSize );
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->bonDriver );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->chInfo );
	}

	if( readSize != NULL ){
		*readSize = pos;
	}

	return TRUE;
}

DWORD GetVALUESize( NWPLAY_PLAY_INFO* val )
{
	DWORD size = sizeof(DWORD);
	if( val == NULL ){
		return size;
	}

	size += GetVALUESize(val->ctrlID);
	size += GetVALUESize(val->ip);
	size += GetVALUESize(val->udp);
	size += GetVALUESize(val->tcp);
	size += GetVALUESize(val->udpPort);
	size += GetVALUESize(val->tcpPort);

	return size;
}

BOOL WriteVALUE( NWPLAY_PLAY_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	DWORD valSize = GetVALUESize( val );
	if( buff == NULL || valSize > buffSize ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, valSize );

	if(val != NULL ){
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->ctrlID );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->ip );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->udp );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->tcp );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->udpPort );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->tcpPort );
	}

	if( writeSize != NULL ){
		*writeSize = pos;
	}
	return TRUE;
}

BOOL ReadVALUE( NWPLAY_PLAY_INFO* val, BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL || buffSize < sizeof(DWORD) ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &valSize );
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->ctrlID );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->ip );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->udp );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->tcp );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->udpPort );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->tcpPort );
	}

	if( readSize != NULL ){
		*readSize = pos;
	}

	return TRUE;
}

DWORD GetVALUESize( NWPLAY_POS_CMD* val )
{
	DWORD size = sizeof(DWORD);
	if( val == NULL ){
		return size;
	}

	size += GetVALUESize(val->ctrlID);
	size += GetVALUESize(val->currentPos);
	size += GetVALUESize(val->totalPos);

	return size;
}

BOOL WriteVALUE( NWPLAY_POS_CMD* val, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	DWORD valSize = GetVALUESize( val );
	if( buff == NULL || valSize > buffSize ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, valSize );

	if(val != NULL ){
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->ctrlID );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->currentPos );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->totalPos );
	}

	if( writeSize != NULL ){
		*writeSize = pos;
	}
	return TRUE;
}

BOOL ReadVALUE( NWPLAY_POS_CMD* val, BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL || buffSize < sizeof(DWORD) ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &valSize );
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->ctrlID );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->currentPos );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->totalPos );
	}

	if( readSize != NULL ){
		*readSize = pos;
	}

	return TRUE;
}

DWORD GetVALUESize( TVTEST_STREAMING_INFO* val )
{
	DWORD size = sizeof(DWORD);
	if( val == NULL ){
		return size;
	}

	size += GetVALUESize(val->enableMode);
	size += GetVALUESize(val->ctrlID);
	size += GetVALUESize(val->serverIP);
	size += GetVALUESize(val->serverPort);
	size += GetVALUESize(val->filePath);
	size += GetVALUESize(val->udpSend);
	size += GetVALUESize(val->tcpSend);
	size += GetVALUESize(val->timeShiftMode);

	return size;
}

BOOL WriteVALUE( TVTEST_STREAMING_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	DWORD valSize = GetVALUESize( val );
	if( buff == NULL || valSize > buffSize ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, valSize );

	if(val != NULL ){
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->enableMode );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->ctrlID );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->serverIP );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->serverPort );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->filePath );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->udpSend );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->tcpSend );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->timeShiftMode );
	}

	if( writeSize != NULL ){
		*writeSize = pos;
	}
	return TRUE;
}

BOOL ReadVALUE( TVTEST_STREAMING_INFO* val, BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL || buffSize < sizeof(DWORD) ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &valSize );
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->enableMode );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->ctrlID );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->serverIP );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->serverPort );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->filePath );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->udpSend );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->tcpSend );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->timeShiftMode );
	}

	if( readSize != NULL ){
		*readSize = pos;
	}

	return TRUE;
}

DWORD GetVALUESize( NWPLAY_TIMESHIFT_INFO* val )
{
	DWORD size = sizeof(DWORD);
	if( val == NULL ){
		return size;
	}

	size += GetVALUESize(val->ctrlID);
	size += GetVALUESize(val->filePath);

	return size;
}

BOOL WriteVALUE( NWPLAY_TIMESHIFT_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	DWORD valSize = GetVALUESize( val );
	if( buff == NULL || valSize > buffSize ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, valSize );

	if(val != NULL ){
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->ctrlID );
		WRITE_VALUE_OR_FAIL( buff, buffSize, pos, size, val->filePath );
	}

	if( writeSize != NULL ){
		*writeSize = pos;
	}
	return TRUE;
}

BOOL ReadVALUE( NWPLAY_TIMESHIFT_INFO* val, BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL || buffSize < sizeof(DWORD) ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &valSize );
	if( buffSize < valSize ){
		return FALSE;
	}

	if( pos < valSize ){
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->ctrlID );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->filePath );
	}

	if( readSize != NULL ){
		*readSize = pos;
	}

	return TRUE;
}

BOOL CCUTIL_WriteStream_( const void* val, DWORD valSize, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	if( val == NULL || buff == NULL || valSize > buffSize ){
		return FALSE;
	}
	memcpy(buff, val, valSize);
	if( writeSize != NULL ){
		*writeSize = valSize;
	}
	return TRUE;
}

BOOL CCUTIL_ReadStream_( void* val, DWORD valSize, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( buff == NULL || valSize > buffSize ){
		return FALSE;
	}
	if( val != NULL ){
		memcpy(val, buff, valSize);
	}
	if( readSize != NULL ){
		*readSize = valSize;
	}
	return TRUE;
}
