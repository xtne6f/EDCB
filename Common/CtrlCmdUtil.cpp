#include "stdafx.h"
#include "CtrlCmdUtil.h"
#include "Util.h"

namespace CtrlCmdUtilImpl_
{

#define READ_VALUE_OR_FAIL(buff,buffSize,pos,size,val)		{ if( ReadVALUE(val,(buff)+pos,(buffSize)-pos,&size) == FALSE ) return FALSE; pos+=size; }

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const wstring& val )
{
	DWORD size = (DWORD)((val.size() + 1) * sizeof(WCHAR) + sizeof(DWORD));
	if( buff != NULL ){
		//全体のサイズ
		WriteVALUE(buff, buffOffset, size);
		memcpy(buff + buffOffset + sizeof(DWORD), val.c_str(), (val.size() + 1) * sizeof(WCHAR));
	}
	return size;
}

BOOL ReadVALUE( wstring* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
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
	
	//TODO: あまりよろしくない(がx86なら辛うじて動く)
	*val = (WCHAR*)(buff + size);

	if( readSize != NULL ){
		*readSize = valSize;
	}
	return TRUE;
}

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const REC_SETTING_DATA& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(buff, pos, val.recMode);
	pos += WriteVALUE(buff, pos, val.priority);
	pos += WriteVALUE(buff, pos, val.tuijyuuFlag);
	pos += WriteVALUE(buff, pos, val.serviceMode);
	pos += WriteVALUE(buff, pos, val.pittariFlag);
	pos += WriteVALUE(buff, pos, val.batFilePath);
	pos += WriteVALUE(buff, pos, val.recFolderList);
	pos += WriteVALUE(buff, pos, val.suspendMode);
	pos += WriteVALUE(buff, pos, val.rebootFlag);
	pos += WriteVALUE(buff, pos, val.useMargineFlag);
	pos += WriteVALUE(buff, pos, val.startMargine);
	pos += WriteVALUE(buff, pos, val.endMargine);
	pos += WriteVALUE(buff, pos, val.continueRecFlag);
	pos += WriteVALUE(buff, pos, val.partialRecFlag);
	pos += WriteVALUE(buff, pos, val.tunerID);
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( REC_SETTING_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
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

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const RESERVE_DATA& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(buff, pos, val.title);
	pos += WriteVALUE(buff, pos, val.startTime);
	pos += WriteVALUE(buff, pos, val.durationSecond);
	pos += WriteVALUE(buff, pos, val.stationName);
	pos += WriteVALUE(buff, pos, val.originalNetworkID);
	pos += WriteVALUE(buff, pos, val.transportStreamID);
	pos += WriteVALUE(buff, pos, val.serviceID);
	pos += WriteVALUE(buff, pos, val.eventID);
	pos += WriteVALUE(buff, pos, val.comment);
	pos += WriteVALUE(buff, pos, val.reserveID);
	pos += WriteVALUE(buff, pos, (BYTE)0);
	pos += WriteVALUE(buff, pos, val.overlapMode);
	pos += WriteVALUE(buff, pos, wstring());
	pos += WriteVALUE(buff, pos, val.startTimeEpg);
	pos += WriteVALUE(buff, pos, val.recSetting);
	pos += WriteVALUE(buff, pos, val.reserveStatus);
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( RESERVE_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
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
		BYTE bPadding;
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &bPadding );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->overlapMode );
		wstring strPadding;
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &strPadding );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->startTimeEpg );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->recSetting );
		READ_VALUE_OR_FAIL( buff, buffSize, pos, size, &val->reserveStatus );
	}

	if( readSize != NULL ){
		*readSize = pos;
	}

	return TRUE;
}

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const EPGDB_SERVICE_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(buff, pos, val.ONID);
	pos += WriteVALUE(buff, pos, val.TSID);
	pos += WriteVALUE(buff, pos, val.SID);
	pos += WriteVALUE(buff, pos, val.service_type);
	pos += WriteVALUE(buff, pos, val.partialReceptionFlag);
	pos += WriteVALUE(buff, pos, val.service_provider_name);
	pos += WriteVALUE(buff, pos, val.service_name);
	pos += WriteVALUE(buff, pos, val.network_name);
	pos += WriteVALUE(buff, pos, val.ts_name);
	pos += WriteVALUE(buff, pos, val.remote_control_key_id);
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( EPGDB_SERVICE_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
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

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const EPGDB_SHORT_EVENT_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(buff, pos, val.event_name);
	pos += WriteVALUE(buff, pos, val.text_char);
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( EPGDB_SHORT_EVENT_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
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

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const EPGDB_EXTENDED_EVENT_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(buff, pos, val.text_char);
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( EPGDB_EXTENDED_EVENT_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
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

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const EPGDB_CONTENT_DATA& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(buff, pos, val.content_nibble_level_1);
	pos += WriteVALUE(buff, pos, val.content_nibble_level_2);
	pos += WriteVALUE(buff, pos, val.user_nibble_1);
	pos += WriteVALUE(buff, pos, val.user_nibble_2);
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( EPGDB_CONTENT_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
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

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const EPGDB_CONTEN_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(buff, pos, val.nibbleList);
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( EPGDB_CONTEN_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
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

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const EPGDB_COMPONENT_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(buff, pos, val.stream_content);
	pos += WriteVALUE(buff, pos, val.component_type);
	pos += WriteVALUE(buff, pos, val.component_tag);
	pos += WriteVALUE(buff, pos, val.text_char);
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( EPGDB_COMPONENT_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
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

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const EPGDB_AUDIO_COMPONENT_INFO_DATA& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(buff, pos, val.stream_content);
	pos += WriteVALUE(buff, pos, val.component_type);
	pos += WriteVALUE(buff, pos, val.component_tag);
	pos += WriteVALUE(buff, pos, val.stream_type);
	pos += WriteVALUE(buff, pos, val.simulcast_group_tag);
	pos += WriteVALUE(buff, pos, val.ES_multi_lingual_flag);
	pos += WriteVALUE(buff, pos, val.main_component_flag);
	pos += WriteVALUE(buff, pos, val.quality_indicator);
	pos += WriteVALUE(buff, pos, val.sampling_rate);
	pos += WriteVALUE(buff, pos, val.text_char);
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( EPGDB_AUDIO_COMPONENT_INFO_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
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

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const EPGDB_AUDIO_COMPONENT_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(buff, pos, val.componentList);
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( EPGDB_AUDIO_COMPONENT_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
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

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const EPGDB_EVENT_DATA& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(buff, pos, val.original_network_id);
	pos += WriteVALUE(buff, pos, val.transport_stream_id);
	pos += WriteVALUE(buff, pos, val.service_id);
	pos += WriteVALUE(buff, pos, val.event_id);
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( EPGDB_EVENT_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
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

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const EPGDB_EVENTGROUP_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(buff, pos, val.group_type);
	pos += WriteVALUE(buff, pos, val.eventDataList);
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( EPGDB_EVENTGROUP_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
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

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const EPGDB_EVENT_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(buff, pos, val.original_network_id);
	pos += WriteVALUE(buff, pos, val.transport_stream_id);
	pos += WriteVALUE(buff, pos, val.service_id);
	pos += WriteVALUE(buff, pos, val.event_id);
	pos += WriteVALUE(buff, pos, val.StartTimeFlag);
	pos += WriteVALUE(buff, pos, val.start_time);
	pos += WriteVALUE(buff, pos, val.DurationFlag);
	pos += WriteVALUE(buff, pos, val.durationSec);
	pos += val.shortInfo ? WriteVALUE(buff, pos, *val.shortInfo) : WriteVALUE(buff, pos, (DWORD)sizeof(DWORD));
	pos += val.extInfo ? WriteVALUE(buff, pos, *val.extInfo) : WriteVALUE(buff, pos, (DWORD)sizeof(DWORD));
	pos += val.contentInfo ? WriteVALUE(buff, pos, *val.contentInfo) : WriteVALUE(buff, pos, (DWORD)sizeof(DWORD));
	pos += val.componentInfo ? WriteVALUE(buff, pos, *val.componentInfo) : WriteVALUE(buff, pos, (DWORD)sizeof(DWORD));
	pos += val.audioInfo ? WriteVALUE(buff, pos, *val.audioInfo) : WriteVALUE(buff, pos, (DWORD)sizeof(DWORD));
	pos += val.eventGroupInfo ? WriteVALUE(buff, pos, *val.eventGroupInfo) : WriteVALUE(buff, pos, (DWORD)sizeof(DWORD));
	pos += val.eventRelayInfo ? WriteVALUE(buff, pos, *val.eventRelayInfo) : WriteVALUE(buff, pos, (DWORD)sizeof(DWORD));
	pos += WriteVALUE(buff, pos, val.freeCAFlag);
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( EPGDB_EVENT_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
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

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const EPGDB_SEARCH_DATE_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(buff, pos, val.startDayOfWeek);
	pos += WriteVALUE(buff, pos, val.startHour);
	pos += WriteVALUE(buff, pos, val.startMin);
	pos += WriteVALUE(buff, pos, val.endDayOfWeek);
	pos += WriteVALUE(buff, pos, val.endHour);
	pos += WriteVALUE(buff, pos, val.endMin);
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( EPGDB_SEARCH_DATE_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
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

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const EPGDB_SEARCH_KEY_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(buff, pos, val.andKey);
	pos += WriteVALUE(buff, pos, val.notKey);
	pos += WriteVALUE(buff, pos, val.regExpFlag);
	pos += WriteVALUE(buff, pos, val.titleOnlyFlag);
	pos += WriteVALUE(buff, pos, val.contentList);
	pos += WriteVALUE(buff, pos, val.dateList);
	pos += WriteVALUE(buff, pos, val.serviceList);
	pos += WriteVALUE(buff, pos, val.videoList);
	pos += WriteVALUE(buff, pos, val.audioList);
	pos += WriteVALUE(buff, pos, val.aimaiFlag);
	pos += WriteVALUE(buff, pos, val.notContetFlag);
	pos += WriteVALUE(buff, pos, val.notDateFlag);
	pos += WriteVALUE(buff, pos, val.freeCAFlag);
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( EPGDB_SEARCH_KEY_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
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

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const SET_CH_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(buff, pos, val.useSID);
	pos += WriteVALUE(buff, pos, val.ONID);
	pos += WriteVALUE(buff, pos, val.TSID);
	pos += WriteVALUE(buff, pos, val.SID);
	pos += WriteVALUE(buff, pos, val.useBonCh);
	pos += WriteVALUE(buff, pos, val.space);
	pos += WriteVALUE(buff, pos, val.ch);
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( SET_CH_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
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

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const SET_CTRL_MODE& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(buff, pos, val.ctrlID);
	pos += WriteVALUE(buff, pos, val.SID);
	pos += WriteVALUE(buff, pos, val.enableScramble);
	pos += WriteVALUE(buff, pos, val.enableCaption);
	pos += WriteVALUE(buff, pos, val.enableData);
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( SET_CTRL_MODE* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
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

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const REC_FILE_SET_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(buff, pos, val.recFolder);
	pos += WriteVALUE(buff, pos, val.writePlugIn);
	pos += WriteVALUE(buff, pos, val.recNamePlugIn);
	pos += WriteVALUE(buff, pos, val.recFileName);
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( REC_FILE_SET_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
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

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const SET_CTRL_REC_PARAM& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(buff, pos, val.ctrlID);
	pos += WriteVALUE(buff, pos, val.fileName);
	pos += WriteVALUE(buff, pos, val.overWriteFlag);
	pos += WriteVALUE(buff, pos, val.createSize);
	pos += WriteVALUE(buff, pos, val.saveFolder);
	pos += WriteVALUE(buff, pos, val.pittariFlag);
	pos += WriteVALUE(buff, pos, val.pittariONID);
	pos += WriteVALUE(buff, pos, val.pittariTSID);
	pos += WriteVALUE(buff, pos, val.pittariSID);
	pos += WriteVALUE(buff, pos, val.pittariEventID);
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( SET_CTRL_REC_PARAM* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
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

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const SET_CTRL_REC_STOP_PARAM& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(buff, pos, val.ctrlID);
	pos += WriteVALUE(buff, pos, val.saveErrLog);
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( SET_CTRL_REC_STOP_PARAM* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
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

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const SET_CTRL_REC_STOP_RES_PARAM& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(buff, pos, val.recFilePath);
	pos += WriteVALUE(buff, pos, val.drop);
	pos += WriteVALUE(buff, pos, val.scramble);
	pos += WriteVALUE(buff, pos, val.subRecFlag);
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( SET_CTRL_REC_STOP_RES_PARAM* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
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

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const REC_FILE_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(buff, pos, val.id);
	pos += WriteVALUE(buff, pos, val.recFilePath);
	pos += WriteVALUE(buff, pos, val.title);
	pos += WriteVALUE(buff, pos, val.startTime);
	pos += WriteVALUE(buff, pos, val.durationSecond);
	pos += WriteVALUE(buff, pos, val.serviceName);
	pos += WriteVALUE(buff, pos, val.originalNetworkID);
	pos += WriteVALUE(buff, pos, val.transportStreamID);
	pos += WriteVALUE(buff, pos, val.serviceID);
	pos += WriteVALUE(buff, pos, val.eventID);
	pos += WriteVALUE(buff, pos, val.drops);
	pos += WriteVALUE(buff, pos, val.scrambles);
	pos += WriteVALUE(buff, pos, val.recStatus);
	pos += WriteVALUE(buff, pos, val.startTimeEpg);
	pos += WriteVALUE(buff, pos, val.comment);
	pos += WriteVALUE(buff, pos, val.programInfo);
	pos += WriteVALUE(buff, pos, val.errInfo);
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( REC_FILE_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
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

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const EPG_AUTO_ADD_DATA& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(buff, pos, val.dataID);
	pos += WriteVALUE(buff, pos, val.searchInfo);
	pos += WriteVALUE(buff, pos, val.recSetting);
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( EPG_AUTO_ADD_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
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

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const SEARCH_EPG_INFO_PARAM& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(buff, pos, val.ONID);
	pos += WriteVALUE(buff, pos, val.TSID);
	pos += WriteVALUE(buff, pos, val.SID);
	pos += WriteVALUE(buff, pos, val.eventID);
	pos += WriteVALUE(buff, pos, val.pfOnlyFlag);
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( SEARCH_EPG_INFO_PARAM* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
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

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const GET_EPG_PF_INFO_PARAM& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(buff, pos, val.ONID);
	pos += WriteVALUE(buff, pos, val.TSID);
	pos += WriteVALUE(buff, pos, val.SID);
	pos += WriteVALUE(buff, pos, val.pfNextFlag);
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( GET_EPG_PF_INFO_PARAM* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
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

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const MANUAL_AUTO_ADD_DATA& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(buff, pos, val.dataID);
	pos += WriteVALUE(buff, pos, val.dayOfWeekFlag);
	pos += WriteVALUE(buff, pos, val.startTime);
	pos += WriteVALUE(buff, pos, val.durationSecond);
	pos += WriteVALUE(buff, pos, val.title);
	pos += WriteVALUE(buff, pos, val.stationName);
	pos += WriteVALUE(buff, pos, val.originalNetworkID);
	pos += WriteVALUE(buff, pos, val.transportStreamID);
	pos += WriteVALUE(buff, pos, val.serviceID);
	pos += WriteVALUE(buff, pos, val.recSetting);
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( MANUAL_AUTO_ADD_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
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

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const TUNER_RESERVE_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(buff, pos, val.tunerID);
	pos += WriteVALUE(buff, pos, val.tunerName);
	pos += WriteVALUE(buff, pos, val.reserveList);
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( TUNER_RESERVE_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
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

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const REGIST_TCP_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(buff, pos, val.ip);
	pos += WriteVALUE(buff, pos, val.port);
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( REGIST_TCP_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
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

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const EPGDB_SERVICE_EVENT_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(buff, pos, val.serviceInfo);
	pos += WriteVALUE(buff, pos, val.eventList);
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( EPGDB_SERVICE_EVENT_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
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

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const TVTEST_CH_CHG_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(buff, pos, val.bonDriver);
	pos += WriteVALUE(buff, pos, val.chInfo);
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( TVTEST_CH_CHG_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
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

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const NWPLAY_PLAY_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(buff, pos, val.ctrlID);
	pos += WriteVALUE(buff, pos, val.ip);
	pos += WriteVALUE(buff, pos, val.udp);
	pos += WriteVALUE(buff, pos, val.tcp);
	pos += WriteVALUE(buff, pos, val.udpPort);
	pos += WriteVALUE(buff, pos, val.tcpPort);
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( NWPLAY_PLAY_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
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

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const NWPLAY_POS_CMD& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(buff, pos, val.ctrlID);
	pos += WriteVALUE(buff, pos, val.currentPos);
	pos += WriteVALUE(buff, pos, val.totalPos);
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( NWPLAY_POS_CMD* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
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

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const TVTEST_STREAMING_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(buff, pos, val.enableMode);
	pos += WriteVALUE(buff, pos, val.ctrlID);
	pos += WriteVALUE(buff, pos, val.serverIP);
	pos += WriteVALUE(buff, pos, val.serverPort);
	pos += WriteVALUE(buff, pos, val.filePath);
	pos += WriteVALUE(buff, pos, val.udpSend);
	pos += WriteVALUE(buff, pos, val.tcpSend);
	pos += WriteVALUE(buff, pos, val.timeShiftMode);
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( TVTEST_STREAMING_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
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

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const NWPLAY_TIMESHIFT_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(buff, pos, val.ctrlID);
	pos += WriteVALUE(buff, pos, val.filePath);
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( NWPLAY_TIMESHIFT_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
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

}

////////////////////////////////////////////////////////////////////////////////////////////
//旧バージョンコマンド送信用バイナリ作成関数
BOOL CreateReserveDataStream(OLD_RESERVE_DATA* pData, CMD_STREAM* pCmd)
{
	if( pData == NULL || pCmd == NULL ){
		return FALSE;
	}
	pCmd->dataSize = sizeof(DWORD)*11 + sizeof(SYSTEMTIME) + sizeof(unsigned short)*4
		+ sizeof(unsigned char)*2 + sizeof(BOOL)*4 + sizeof(WORD)*1+ sizeof(int)*2;
	pCmd->dataSize += (DWORD)(pData->strTitle.length()+1)*sizeof(WCHAR);
	pCmd->dataSize += (DWORD)(pData->strStationName.length()+1)*sizeof(WCHAR);
	pCmd->dataSize += (DWORD)(pData->strComment.length()+1)*sizeof(WCHAR);
	pCmd->dataSize += (DWORD)(pData->strBatPath.length()+1)*sizeof(WCHAR);
	pCmd->dataSize += (DWORD)(pData->strRecFolder.length()+1)*sizeof(WCHAR);
	pCmd->dataSize += (DWORD)(pData->strRecFilePath.length()+1)*sizeof(WCHAR);
	pCmd->data = new BYTE[pCmd->dataSize];
	ZeroMemory(pCmd->data, pCmd->dataSize);

	DWORD dwStrSize = 0;
	DWORD dwPos = 0;

	dwStrSize = (DWORD)(pData->strTitle.length()+1)*sizeof(WCHAR);
	memcpy(pCmd->data + dwPos, &dwStrSize, sizeof(DWORD));
	dwPos+=sizeof(DWORD);
	memcpy(pCmd->data + dwPos, pData->strTitle.c_str(), dwStrSize);
	dwPos+=dwStrSize;

	memcpy(pCmd->data + dwPos, &pData->StartTime, sizeof(SYSTEMTIME));
	dwPos+=sizeof(SYSTEMTIME);

	memcpy(pCmd->data + dwPos, &pData->dwDurationSec, sizeof(DWORD));
	dwPos+=sizeof(DWORD);

	dwStrSize = (DWORD)(pData->strStationName.length()+1)*sizeof(WCHAR);
	memcpy(pCmd->data + dwPos, &dwStrSize, sizeof(DWORD));
	dwPos+=sizeof(DWORD);
	memcpy(pCmd->data + dwPos, pData->strStationName.c_str(), dwStrSize);
	dwPos+=dwStrSize;

	memcpy(pCmd->data + dwPos, &pData->usONID, sizeof(unsigned short));
	dwPos+=sizeof(unsigned short);

	memcpy(pCmd->data + dwPos, &pData->usTSID, sizeof(unsigned short));
	dwPos+=sizeof(unsigned short);

	memcpy(pCmd->data + dwPos, &pData->usServiceID, sizeof(unsigned short));
	dwPos+=sizeof(unsigned short);

	memcpy(pCmd->data + dwPos, &pData->usEventID, sizeof(unsigned short));
	dwPos+=sizeof(unsigned short);

	memcpy(pCmd->data + dwPos, &pData->ucPriority, sizeof(unsigned char));
	dwPos+=sizeof(unsigned char);

	memcpy(pCmd->data + dwPos, &pData->ucTuijyuu, sizeof(unsigned char));
	dwPos+=sizeof(unsigned char);

	dwStrSize = (DWORD)(pData->strComment.length()+1)*sizeof(WCHAR);
	memcpy(pCmd->data + dwPos, &dwStrSize, sizeof(DWORD));
	dwPos+=sizeof(DWORD);
	memcpy(pCmd->data + dwPos, pData->strComment.c_str(), dwStrSize);
	dwPos+=dwStrSize;

	memcpy(pCmd->data + dwPos, &pData->dwRecMode, sizeof(DWORD));
	dwPos+=sizeof(DWORD);

	memcpy(pCmd->data + dwPos, &pData->bPittari, sizeof(BOOL));
	dwPos+=sizeof(BOOL);

	dwStrSize = (DWORD)(pData->strBatPath.length()+1)*sizeof(WCHAR);
	memcpy(pCmd->data + dwPos, &dwStrSize, sizeof(DWORD));
	dwPos+=sizeof(DWORD);
	memcpy(pCmd->data + dwPos, pData->strBatPath.c_str(), dwStrSize);
	dwPos+=dwStrSize;

	memcpy(pCmd->data + dwPos, &pData->dwReserveID, sizeof(DWORD));
	dwPos+=sizeof(DWORD);

	memcpy(pCmd->data + dwPos, &pData->bSetWait, sizeof(BOOL));
	dwPos+=sizeof(BOOL);

	memcpy(pCmd->data + dwPos, &pData->dwPiledUpMode, sizeof(DWORD));
	dwPos+=sizeof(BOOL);

	dwStrSize = (DWORD)(pData->strRecFolder.length()+1)*sizeof(WCHAR);
	memcpy(pCmd->data + dwPos, &dwStrSize, sizeof(DWORD));
	dwPos+=sizeof(DWORD);
	memcpy(pCmd->data + dwPos, pData->strRecFolder.c_str(), dwStrSize);
	dwPos+=dwStrSize;

	memcpy(pCmd->data + dwPos, &pData->wSuspendMode, sizeof(WORD));
	dwPos+=sizeof(WORD);

	memcpy(pCmd->data + dwPos, &pData->bReboot, sizeof(BOOL));
	dwPos+=sizeof(BOOL);

	dwStrSize = (DWORD)(pData->strRecFilePath.length()+1)*sizeof(WCHAR);
	memcpy(pCmd->data + dwPos, &dwStrSize, sizeof(DWORD));
	dwPos+=sizeof(DWORD);
	memcpy(pCmd->data + dwPos, pData->strRecFilePath.c_str(), dwStrSize);
	dwPos+=dwStrSize;

	memcpy(pCmd->data + dwPos, &pData->bUseMargine, sizeof(BOOL));
	dwPos+=sizeof(BOOL);
	memcpy(pCmd->data + dwPos, &pData->iStartMargine, sizeof(int));
	dwPos+=sizeof(int);
	memcpy(pCmd->data + dwPos, &pData->iEndMargine, sizeof(int));
	dwPos+=sizeof(int);
	memcpy(pCmd->data + dwPos, &pData->dwServiceMode, sizeof(DWORD));
	dwPos+=sizeof(DWORD);
	

	return TRUE;
}

BOOL CopyReserveData(OLD_RESERVE_DATA* pstData, CMD_STREAM* pCmd)
{
	if( pstData == NULL || pCmd == NULL ){
		return FALSE;
	}
	if(pCmd->data == NULL ){
		OutputDebugString(L"●CopyReserveData　NULL");
		return FALSE;
	}
	if(pCmd->dataSize == 0 ){
		OutputDebugString(L"●CopyReserveData　0");
		return FALSE;
	}
	BYTE* pBuff = pCmd->data;
	DWORD dwPos = 0;
	DWORD dwStrSize=0;

	dwStrSize = *(DWORD*)(pBuff+dwPos);
	dwPos+=sizeof(DWORD);
	pstData->strTitle = (WCHAR*)(pBuff+dwPos);
	dwPos+=dwStrSize;

	pstData->StartTime = *(SYSTEMTIME*)(pBuff+dwPos);
	dwPos+=sizeof(SYSTEMTIME);

	pstData->dwDurationSec = *(DWORD*)(pBuff+dwPos);
	dwPos+=sizeof(DWORD);

	dwStrSize = *(DWORD*)(pBuff+dwPos);
	dwPos+=sizeof(DWORD);
	pstData->strStationName = (WCHAR*)(pBuff+dwPos);
	dwPos+=dwStrSize;

	pstData->usONID = *(unsigned short*)(pBuff+dwPos);
	dwPos+=sizeof(unsigned short);

	pstData->usTSID = *(unsigned short*)(pBuff+dwPos);
	dwPos+=sizeof(unsigned short);

	pstData->usServiceID = *(unsigned short*)(pBuff+dwPos);
	dwPos+=sizeof(unsigned short);

	pstData->usEventID = *(unsigned short*)(pBuff+dwPos);
	dwPos+=sizeof(unsigned short);

	pstData->ucPriority = *(unsigned char*)(pBuff+dwPos);
	dwPos+=sizeof(unsigned char);

	pstData->ucTuijyuu = *(unsigned char*)(pBuff+dwPos);
	dwPos+=sizeof(unsigned char);

	dwStrSize = *(DWORD*)(pBuff+dwPos);
	dwPos+=sizeof(DWORD);
	pstData->strComment = (WCHAR*)(pBuff+dwPos);
	dwPos+=dwStrSize;

	pstData->dwRecMode = *(DWORD*)(pBuff+dwPos);
	dwPos+=sizeof(DWORD);

	pstData->bPittari = *(BOOL*)(pBuff+dwPos);
	dwPos+=sizeof(BOOL);

	dwStrSize = *(DWORD*)(pBuff+dwPos);
	dwPos+=sizeof(DWORD);
	pstData->strBatPath = (WCHAR*)(pBuff+dwPos);
	dwPos+=dwStrSize;

	pstData->dwReserveID = *(DWORD*)(pBuff+dwPos);
	dwPos+=sizeof(DWORD);

	pstData->bSetWait = *(BOOL*)(pBuff+dwPos);
	dwPos+=sizeof(BOOL);

	pstData->dwPiledUpMode = *(DWORD*)(pBuff+dwPos);
	dwPos+=sizeof(DWORD);

	dwStrSize = *(DWORD*)(pBuff+dwPos);
	dwPos+=sizeof(DWORD);
	pstData->strRecFolder = (WCHAR*)(pBuff+dwPos);
	dwPos+=dwStrSize;

	pstData->wSuspendMode = *(WORD*)(pBuff+dwPos);
	dwPos+=sizeof(WORD);

	pstData->bReboot = *(BOOL*)(pBuff+dwPos);
	dwPos+=sizeof(BOOL);

	dwStrSize = *(DWORD*)(pBuff+dwPos);
	dwPos+=sizeof(DWORD);
	pstData->strRecFilePath = (WCHAR*)(pBuff+dwPos);
	dwPos+=dwStrSize;

	if( dwPos < pCmd->dataSize ){
		pstData->bUseMargine = *(BOOL*)(pBuff+dwPos);
		dwPos+=sizeof(BOOL);
		pstData->iStartMargine = *(int*)(pBuff+dwPos);
		dwPos+=sizeof(int);
		pstData->iEndMargine = *(int*)(pBuff+dwPos);
		dwPos+=sizeof(int);
		pstData->dwServiceMode = *(DWORD*)(pBuff+dwPos);
		dwPos+=sizeof(DWORD);
	}else{
		pstData->bUseMargine = FALSE;
		pstData->iStartMargine = 10;
		pstData->iEndMargine = 2;
		pstData->dwServiceMode = 0;
	}


	return TRUE;
}

BOOL CreateSearchKeyDataStream(OLD_SEARCH_KEY* pData, CMD_STREAM* pCmd)
{
	if( pData == NULL || pCmd == NULL ){
		return FALSE;
	}
	pCmd->dataSize = sizeof(DWORD)*7 + sizeof(BOOL)*11 + sizeof(int)*12 + sizeof(WORD)*1;
	pCmd->dataSize += (DWORD)(pData->strAnd.length()+1)*sizeof(WCHAR);
	pCmd->dataSize += (DWORD)(pData->strNot.length()+1)*sizeof(WCHAR);
	pCmd->dataSize += (DWORD)(pData->CHIDList.size())*sizeof(__int64);
	pCmd->dataSize += (DWORD)(pData->strBat.length()+1)*sizeof(WCHAR);
	pCmd->dataSize += (DWORD)(pData->strRecFolder.length()+1)*sizeof(WCHAR);
	pCmd->dataSize += (DWORD)(pData->strPattern.length()+1)*sizeof(WCHAR);
	pCmd->data = new BYTE[pCmd->dataSize];
	ZeroMemory(pCmd->data, pCmd->dataSize);

	DWORD dwStrSize = 0;
	DWORD dwPos = 0;

	dwStrSize = (DWORD)(pData->strAnd.length()+1)*sizeof(WCHAR);
	memcpy(pCmd->data + dwPos, &dwStrSize, sizeof(DWORD));
	dwPos+=sizeof(DWORD);
	memcpy(pCmd->data + dwPos, pData->strAnd.c_str(), dwStrSize);
	dwPos+=dwStrSize;

	dwStrSize = (DWORD)(pData->strNot.length()+1)*sizeof(WCHAR);
	memcpy(pCmd->data + dwPos, &dwStrSize, sizeof(DWORD));
	dwPos+=sizeof(DWORD);
	memcpy(pCmd->data + dwPos, pData->strNot.c_str(), dwStrSize);
	dwPos+=dwStrSize;

	memcpy(pCmd->data + dwPos, &pData->bTitle, sizeof(BOOL));
	dwPos+=sizeof(BOOL);

	memcpy(pCmd->data + dwPos, &pData->iJanru, sizeof(int));
	dwPos+=sizeof(int);

	memcpy(pCmd->data + dwPos, &pData->iSH, sizeof(int));
	dwPos+=sizeof(int);

	memcpy(pCmd->data + dwPos, &pData->iSM, sizeof(int));
	dwPos+=sizeof(int);

	memcpy(pCmd->data + dwPos, &pData->iEH, sizeof(int));
	dwPos+=sizeof(int);

	memcpy(pCmd->data + dwPos, &pData->iEM, sizeof(int));
	dwPos+=sizeof(int);

	memcpy(pCmd->data + dwPos, &pData->bChkMon, sizeof(BOOL));
	dwPos+=sizeof(BOOL);

	memcpy(pCmd->data + dwPos, &pData->bChkTue, sizeof(BOOL));
	dwPos+=sizeof(BOOL);

	memcpy(pCmd->data + dwPos, &pData->bChkWed, sizeof(BOOL));
	dwPos+=sizeof(BOOL);

	memcpy(pCmd->data + dwPos, &pData->bChkThu, sizeof(BOOL));
	dwPos+=sizeof(BOOL);

	memcpy(pCmd->data + dwPos, &pData->bChkFri, sizeof(BOOL));
	dwPos+=sizeof(BOOL);

	memcpy(pCmd->data + dwPos, &pData->bChkSat, sizeof(BOOL));
	dwPos+=sizeof(BOOL);

	memcpy(pCmd->data + dwPos, &pData->bChkSun, sizeof(BOOL));
	dwPos+=sizeof(BOOL);

	DWORD dwTemp = (DWORD)pData->CHIDList.size();
	memcpy(pCmd->data + dwPos, &dwTemp, sizeof(__int64));
	dwPos+=sizeof(DWORD);

	for( int i=0; i<(int)pData->CHIDList.size(); i++ ){
		memcpy(pCmd->data + dwPos, &pData->CHIDList[i], sizeof(__int64));
		dwPos+=sizeof(__int64);
	}

	memcpy(pCmd->data + dwPos, &pData->iAutoAddID, sizeof(int));
	dwPos+=sizeof(int);

	memcpy(pCmd->data + dwPos, &pData->iPriority, sizeof(int));
	dwPos+=sizeof(int);

	memcpy(pCmd->data + dwPos, &pData->iTuijyuu, sizeof(int));
	dwPos+=sizeof(int);

	memcpy(pCmd->data + dwPos, &pData->iRecMode, sizeof(int));
	dwPos+=sizeof(int);

	memcpy(pCmd->data + dwPos, &pData->iPittari, sizeof(int));
	dwPos+=sizeof(int);

	dwStrSize = (DWORD)(pData->strBat.length()+1)*sizeof(WCHAR);
	memcpy(pCmd->data + dwPos, &dwStrSize, sizeof(DWORD));
	dwPos+=sizeof(DWORD);
	memcpy(pCmd->data + dwPos, pData->strBat.c_str(), dwStrSize);
	dwPos+=dwStrSize;

	dwStrSize = (DWORD)(pData->strRecFolder.length()+1)*sizeof(WCHAR);
	memcpy(pCmd->data + dwPos, &dwStrSize, sizeof(DWORD));
	dwPos+=sizeof(DWORD);
	memcpy(pCmd->data + dwPos, pData->strRecFolder.c_str(), dwStrSize);
	dwPos+=dwStrSize;

	memcpy(pCmd->data + dwPos, &pData->wSuspendMode, sizeof(WORD));
	dwPos+=sizeof(WORD);

	memcpy(pCmd->data + dwPos, &pData->bReboot, sizeof(BOOL));
	dwPos+=sizeof(BOOL);

	memcpy(pCmd->data + dwPos, &pData->bUseMargine, sizeof(BOOL));
	dwPos+=sizeof(BOOL);
	memcpy(pCmd->data + dwPos, &pData->iStartMargine, sizeof(int));
	dwPos+=sizeof(int);
	memcpy(pCmd->data + dwPos, &pData->iEndMargine, sizeof(int));
	dwPos+=sizeof(int);
	memcpy(pCmd->data + dwPos, &pData->dwServiceMode, sizeof(DWORD));
	dwPos+=sizeof(DWORD);

	memcpy(pCmd->data + dwPos, &pData->bRegExp, sizeof(BOOL));
	dwPos+=sizeof(BOOL);
	dwStrSize = (DWORD)(pData->strPattern.length()+1)*sizeof(WCHAR);
	memcpy(pCmd->data + dwPos, &dwStrSize, sizeof(DWORD));
	dwPos+=sizeof(DWORD);
	memcpy(pCmd->data + dwPos, pData->strPattern.c_str(), dwStrSize);
	dwPos+=dwStrSize;

	return TRUE;
}

BOOL CopySearchKeyData(OLD_SEARCH_KEY* pstData, CMD_STREAM* pCmd)
{
	if( pstData == NULL || pCmd == NULL ){
		return FALSE;
	}
	if(pCmd->data == NULL ){
		OutputDebugString(L"●CopySearchKeyData　NULL");
		return FALSE;
	}
	if(pCmd->dataSize == 0 ){
		OutputDebugString(L"●CopySearchKeyData　0");
		return FALSE;
	}
	BYTE* pBuff = pCmd->data;
	DWORD dwPos = 0;
	DWORD dwStrSize=0;

	dwStrSize = *(DWORD*)(pBuff+dwPos);
	dwPos+=sizeof(DWORD);
	pstData->strAnd = (WCHAR*)(pBuff+dwPos);
	dwPos+=dwStrSize;

	dwStrSize = *(DWORD*)(pBuff+dwPos);
	dwPos+=sizeof(DWORD);
	pstData->strNot = (WCHAR*)(pBuff+dwPos);
	dwPos+=dwStrSize;

	pstData->bTitle = *(BOOL*)(pBuff+dwPos);
	dwPos+=sizeof(BOOL);

	pstData->iJanru = *(int*)(pBuff+dwPos);
	dwPos+=sizeof(int);

	pstData->iSH = *(int*)(pBuff+dwPos);
	dwPos+=sizeof(int);

	pstData->iSM = *(int*)(pBuff+dwPos);
	dwPos+=sizeof(int);

	pstData->iEH = *(int*)(pBuff+dwPos);
	dwPos+=sizeof(int);

	pstData->iEM = *(int*)(pBuff+dwPos);
	dwPos+=sizeof(int);

	pstData->bChkMon = *(BOOL*)(pBuff+dwPos);
	dwPos+=sizeof(BOOL);

	pstData->bChkTue = *(BOOL*)(pBuff+dwPos);
	dwPos+=sizeof(BOOL);

	pstData->bChkWed = *(BOOL*)(pBuff+dwPos);
	dwPos+=sizeof(BOOL);

	pstData->bChkThu = *(BOOL*)(pBuff+dwPos);
	dwPos+=sizeof(BOOL);

	pstData->bChkFri = *(BOOL*)(pBuff+dwPos);
	dwPos+=sizeof(BOOL);

	pstData->bChkSat = *(BOOL*)(pBuff+dwPos);
	dwPos+=sizeof(BOOL);

	pstData->bChkSun = *(BOOL*)(pBuff+dwPos);
	dwPos+=sizeof(BOOL);

	DWORD dwCount = *(DWORD*)(pBuff+dwPos);
	dwPos+=sizeof(DWORD);
	for( DWORD i=0; i<dwCount; i++ ){
		__int64 Item;
		Item = *(__int64*)(pBuff+dwPos);
		dwPos+=sizeof(__int64);
		pstData->CHIDList.push_back(Item);
	}

	pstData->iAutoAddID = *(int*)(pBuff+dwPos);
	dwPos+=sizeof(int);

	pstData->iPriority = *(int*)(pBuff+dwPos);
	dwPos+=sizeof(int);

	pstData->iTuijyuu = *(int*)(pBuff+dwPos);
	dwPos+=sizeof(int);

	pstData->iRecMode = *(int*)(pBuff+dwPos);
	dwPos+=sizeof(int);

	pstData->iPittari = *(int*)(pBuff+dwPos);
	dwPos+=sizeof(int);

	dwStrSize = *(DWORD*)(pBuff+dwPos);
	dwPos+=sizeof(DWORD);
	pstData->strBat = (WCHAR*)(pBuff+dwPos);
	dwPos+=dwStrSize;

	dwStrSize = *(DWORD*)(pBuff+dwPos);
	dwPos+=sizeof(DWORD);
	pstData->strRecFolder = (WCHAR*)(pBuff+dwPos);
	dwPos+=dwStrSize;

	pstData->wSuspendMode = *(WORD*)(pBuff+dwPos);
	dwPos+=sizeof(WORD);

	pstData->bReboot = *(BOOL*)(pBuff+dwPos);
	dwPos+=sizeof(BOOL);

	if( dwPos < pCmd->dataSize ){
		pstData->bUseMargine = *(BOOL*)(pBuff+dwPos);
		dwPos+=sizeof(BOOL);
		pstData->iStartMargine = *(int*)(pBuff+dwPos);
		dwPos+=sizeof(int);
		pstData->iEndMargine = *(int*)(pBuff+dwPos);
		dwPos+=sizeof(int);
		pstData->dwServiceMode = *(DWORD*)(pBuff+dwPos);
		dwPos+=sizeof(DWORD);
		if( dwPos < pCmd->dataSize ){
			pstData->bRegExp = *(BOOL*)(pBuff+dwPos);
			dwPos+=sizeof(BOOL);

			dwStrSize = *(DWORD*)(pBuff+dwPos);
			dwPos+=sizeof(DWORD);
			pstData->strPattern = (WCHAR*)(pBuff+dwPos);
			dwPos+=dwStrSize;
		}else{
			pstData->bRegExp = FALSE;
			pstData->strPattern = L"";
		}
	}else{
		pstData->bUseMargine = FALSE;
		pstData->iStartMargine = 10;
		pstData->iEndMargine = 2;
		pstData->dwServiceMode = 0;
		pstData->bRegExp = FALSE;
		pstData->strPattern = L"";
	}

	return TRUE;
}

BOOL CreateEventInfoData3Stream(OLD_EVENT_INFO_DATA3* pData, CMD_STREAM* pCmd)
{
	if( pData == NULL || pCmd == NULL ){
		return FALSE;
	}
	pCmd->dataSize = sizeof(DWORD)*9 + sizeof(SYSTEMTIME) + sizeof(WORD)*4
		+ sizeof(unsigned char)*5;
	pCmd->dataSize += (DWORD)(pData->strEventName.length()+1)*sizeof(WCHAR);
	pCmd->dataSize += (DWORD)(pData->strEventText.length()+1)*sizeof(WCHAR);
	pCmd->dataSize += (DWORD)(pData->strEventExtText.length()+1)*sizeof(WCHAR);
	pCmd->dataSize += (DWORD)(pData->strComponentTypeText.length()+1)*sizeof(WCHAR);
	pCmd->dataSize += (DWORD)(pData->strAudioComponentTypeText.length()+1)*sizeof(WCHAR);
	pCmd->dataSize += (DWORD)(pData->NibbleList.size())*sizeof(OLD_NIBBLE_DATA);
	pCmd->dataSize += (DWORD)(pData->EventRelayList.size())*sizeof(OLD_EVENT_ID_INFO);
	pCmd->dataSize += (DWORD)(pData->EventGroupList.size())*sizeof(OLD_EVENT_ID_INFO);
	pCmd->data = new BYTE[pCmd->dataSize];
	ZeroMemory(pCmd->data, pCmd->dataSize);

	DWORD dwStrSize = 0;
	DWORD dwPos = 0;

	memcpy(pCmd->data + dwPos, &pData->wOriginalNID, sizeof(WORD));
	dwPos+=sizeof(WORD);

	memcpy(pCmd->data + dwPos, &pData->wTSID, sizeof(WORD));
	dwPos+=sizeof(WORD);

	memcpy(pCmd->data + dwPos, &pData->wServiceID, sizeof(WORD));
	dwPos+=sizeof(WORD);

	memcpy(pCmd->data + dwPos, &pData->wEventID, sizeof(WORD));
	dwPos+=sizeof(WORD);

	dwStrSize = (DWORD)(pData->strEventName.length()+1)*sizeof(WCHAR);
	memcpy(pCmd->data + dwPos, &dwStrSize, sizeof(DWORD));
	dwPos+=sizeof(DWORD);
	memcpy(pCmd->data + dwPos, pData->strEventName.c_str(), dwStrSize);
	dwPos+=dwStrSize;

	dwStrSize = (DWORD)(pData->strEventText.length()+1)*sizeof(WCHAR);
	memcpy(pCmd->data + dwPos, &dwStrSize, sizeof(DWORD));
	dwPos+=sizeof(DWORD);
	memcpy(pCmd->data + dwPos, pData->strEventText.c_str(), dwStrSize);
	dwPos+=dwStrSize;

	dwStrSize = (DWORD)(pData->strEventExtText.length()+1)*sizeof(WCHAR);
	memcpy(pCmd->data + dwPos, &dwStrSize, sizeof(DWORD));
	dwPos+=sizeof(DWORD);
	memcpy(pCmd->data + dwPos, pData->strEventExtText.c_str(), dwStrSize);
	dwPos+=dwStrSize;

	memcpy(pCmd->data + dwPos, &pData->stStartTime, sizeof(SYSTEMTIME));
	dwPos+=sizeof(SYSTEMTIME);

	memcpy(pCmd->data + dwPos, &pData->dwDurationSec, sizeof(DWORD));
	dwPos+=sizeof(DWORD);

	memcpy(pCmd->data + dwPos, &pData->ucComponentType, sizeof(unsigned char));
	dwPos+=sizeof(unsigned char);

	dwStrSize = (DWORD)(pData->strComponentTypeText.length()+1)*sizeof(WCHAR);
	memcpy(pCmd->data + dwPos, &dwStrSize, sizeof(DWORD));
	dwPos+=sizeof(DWORD);
	memcpy(pCmd->data + dwPos, pData->strComponentTypeText.c_str(), dwStrSize);
	dwPos+=dwStrSize;

	memcpy(pCmd->data + dwPos, &pData->ucAudioComponentType, sizeof(unsigned char));
	dwPos+=sizeof(unsigned char);

	memcpy(pCmd->data + dwPos, &pData->ucESMultiLangFlag, sizeof(unsigned char));
	dwPos+=sizeof(unsigned char);

	memcpy(pCmd->data + dwPos, &pData->ucMainComponentFlag, sizeof(unsigned char));
	dwPos+=sizeof(unsigned char);

	memcpy(pCmd->data + dwPos, &pData->ucSamplingRate, sizeof(unsigned char));
	dwPos+=sizeof(unsigned char);

	dwStrSize = (DWORD)(pData->strAudioComponentTypeText.length()+1)*sizeof(WCHAR);
	memcpy(pCmd->data + dwPos, &dwStrSize, sizeof(DWORD));
	dwPos+=sizeof(DWORD);
	memcpy(pCmd->data + dwPos, pData->strAudioComponentTypeText.c_str(), dwStrSize);
	dwPos+=dwStrSize;

	DWORD dwTemp = (DWORD)pData->NibbleList.size();
	memcpy(pCmd->data + dwPos, &dwTemp, sizeof(DWORD));
	dwPos+=sizeof(DWORD);

	for( int i=0; i<(int)pData->NibbleList.size(); i++ ){
		memcpy(pCmd->data + dwPos, &pData->NibbleList[i], sizeof(OLD_NIBBLE_DATA));
		dwPos+=sizeof(OLD_NIBBLE_DATA);
	}

	dwTemp = (DWORD)pData->EventRelayList.size();
	memcpy(pCmd->data + dwPos, &dwTemp, sizeof(DWORD));
	dwPos+=sizeof(DWORD);

	for( int i=0; i<(int)pData->EventRelayList.size(); i++ ){
		memcpy(pCmd->data + dwPos, &pData->EventRelayList[i], sizeof(OLD_EVENT_ID_INFO));
		dwPos+=sizeof(OLD_EVENT_ID_INFO);
	}

	dwTemp = (DWORD)pData->EventGroupList.size();
	memcpy(pCmd->data + dwPos, &dwTemp, sizeof(DWORD));
	dwPos+=sizeof(DWORD);

	for( int i=0; i<(int)pData->EventGroupList.size(); i++ ){
		memcpy(pCmd->data + dwPos, &pData->EventGroupList[i], sizeof(OLD_EVENT_ID_INFO));
		dwPos+=sizeof(OLD_EVENT_ID_INFO);
	}

	return TRUE;
}

BOOL CopyEventInfoData3(OLD_EVENT_INFO_DATA3* pstData, CMD_STREAM* pCmd)
{
	if( pstData == NULL || pCmd == NULL ){
		return FALSE;
	}
	if(pCmd->data == NULL ){
		OutputDebugString(L"●CopyEventInfoData3　NULL");
		return FALSE;
	}
	if(pCmd->dataSize == 0 ){
		OutputDebugString(L"●CopyEventInfoData3　0");
		return FALSE;
	}
	BYTE* pBuff = pCmd->data;
	DWORD dwPos = 0;
	DWORD dwStrSize=0;

	pstData->wOriginalNID = *(WORD*)(pBuff+dwPos);
	dwPos+=sizeof(WORD);

	pstData->wTSID = *(WORD*)(pBuff+dwPos);
	dwPos+=sizeof(WORD);

	pstData->wServiceID = *(WORD*)(pBuff+dwPos);
	dwPos+=sizeof(WORD);

	pstData->wEventID = *(WORD*)(pBuff+dwPos);
	dwPos+=sizeof(WORD);

	dwStrSize = *(DWORD*)(pBuff+dwPos);
	dwPos+=sizeof(DWORD);
	pstData->strEventName = (WCHAR*)(pBuff+dwPos);
	dwPos+=dwStrSize;

	dwStrSize = *(DWORD*)(pBuff+dwPos);
	dwPos+=sizeof(DWORD);
	pstData->strEventText = (WCHAR*)(pBuff+dwPos);
	dwPos+=dwStrSize;

	dwStrSize = *(DWORD*)(pBuff+dwPos);
	dwPos+=sizeof(DWORD);
	pstData->strEventExtText = (WCHAR*)(pBuff+dwPos);
	dwPos+=dwStrSize;

	pstData->stStartTime = *(SYSTEMTIME*)(pBuff+dwPos);
	dwPos+=sizeof(SYSTEMTIME);

	pstData->dwDurationSec = *(DWORD*)(pBuff+dwPos);
	dwPos+=sizeof(DWORD);

	pstData->ucComponentType = *(unsigned char*)(pBuff+dwPos);
	dwPos+=sizeof(unsigned char);

	dwStrSize = *(DWORD*)(pBuff+dwPos);
	dwPos+=sizeof(DWORD);
	pstData->strComponentTypeText = (WCHAR*)(pBuff+dwPos);
	dwPos+=dwStrSize;

	pstData->ucAudioComponentType = *(unsigned char*)(pBuff+dwPos);
	dwPos+=sizeof(unsigned char);

	pstData->ucESMultiLangFlag = *(unsigned char*)(pBuff+dwPos);
	dwPos+=sizeof(unsigned char);

	pstData->ucMainComponentFlag = *(unsigned char*)(pBuff+dwPos);
	dwPos+=sizeof(unsigned char);

	pstData->ucSamplingRate = *(unsigned char*)(pBuff+dwPos);
	dwPos+=sizeof(unsigned char);

	dwStrSize = *(DWORD*)(pBuff+dwPos);
	dwPos+=sizeof(DWORD);
	pstData->strAudioComponentTypeText = (WCHAR*)(pBuff+dwPos);
	dwPos+=dwStrSize;

	DWORD dwCount = *(DWORD*)(pBuff+dwPos);
	dwPos+=sizeof(DWORD);
	for( DWORD i=0; i<dwCount; i++ ){
		OLD_NIBBLE_DATA Item;
		Item = *(OLD_NIBBLE_DATA*)(pBuff+dwPos);
		dwPos+=sizeof(OLD_NIBBLE_DATA);
		pstData->NibbleList.push_back(Item);
	}

	dwCount = *(DWORD*)(pBuff+dwPos);
	dwPos+=sizeof(DWORD);
	for( DWORD i=0; i<dwCount; i++ ){
		OLD_EVENT_ID_INFO Item;
		Item = *(OLD_EVENT_ID_INFO*)(pBuff+dwPos);
		dwPos+=sizeof(OLD_EVENT_ID_INFO);
		pstData->EventRelayList.push_back(Item);
	}

	dwCount = *(DWORD*)(pBuff+dwPos);
	dwPos+=sizeof(DWORD);
	for( DWORD i=0; i<dwCount; i++ ){
		OLD_EVENT_ID_INFO Item;
		Item = *(OLD_EVENT_ID_INFO*)(pBuff+dwPos);
		dwPos+=sizeof(OLD_EVENT_ID_INFO);
		pstData->EventGroupList.push_back(Item);
	}
	return TRUE;
}

void CopyOldNew(OLD_RESERVE_DATA* src, RESERVE_DATA* dest)
{
	dest->title = src->strTitle;
	dest->startTime = src->StartTime;
	dest->durationSecond = src->dwDurationSec;
	dest->stationName = src->strStationName;
	dest->originalNetworkID = src->usONID;
	dest->transportStreamID = src->usTSID;
	dest->serviceID = src->usServiceID;
	dest->eventID = src->usEventID;
	dest->comment = src->strComment;
	dest->reserveID = src->dwReserveID;
	dest->overlapMode = 0;
	dest->startTimeEpg = src->StartTime;
	dest->recSetting.recMode = (BYTE)src->dwRecMode;
	dest->recSetting.priority = src->ucPriority;
	dest->recSetting.tuijyuuFlag = src->ucTuijyuu;
	dest->recSetting.serviceMode = src->dwServiceMode;
	dest->recSetting.pittariFlag = src->bPittari;
	dest->recSetting.batFilePath = src->strBatPath;
	if( src->wSuspendMode == 0 ){
		dest->recSetting.suspendMode = 4;
	}else if( src->wSuspendMode == 4 ){
		dest->recSetting.suspendMode = 0;
	}else{
		dest->recSetting.suspendMode = (BYTE)src->wSuspendMode;
	}
	dest->recSetting.rebootFlag = src->bReboot;
	dest->recSetting.useMargineFlag = src->bUseMargine;
	dest->recSetting.startMargine = src->iStartMargine;
	dest->recSetting.endMargine = src->iEndMargine;
	dest->recSetting.continueRecFlag = 0;
	dest->recSetting.partialRecFlag = 0;
	dest->recSetting.tunerID = 0;
	if( src->strRecFolder.size() > 0 ){
		REC_FILE_SET_INFO folder;
		folder.recFolder = src->strRecFolder;
		folder.writePlugIn = L"Write_Default.dll";
		dest->recSetting.recFolderList.push_back(folder);
	}
}

void CopyOldNew(OLD_SEARCH_KEY* src, EPG_AUTO_ADD_DATA* dest)
{
	dest->dataID = (DWORD)src->iAutoAddID;
	if( src->bRegExp == FALSE ){
		dest->searchInfo.andKey = src->strAnd;
		dest->searchInfo.notKey = src->strNot;
	}else{
		dest->searchInfo.andKey = src->strPattern;
	}
	dest->searchInfo.regExpFlag = (BYTE)src->bRegExp;
	dest->searchInfo.titleOnlyFlag = src->bTitle;
	if( src->iJanru != -1 ){
		EPGDB_CONTENT_DATA content;
		content.user_nibble_1 = (BYTE)src->iJanru;
		content.user_nibble_2 = 0xFF;
		dest->searchInfo.contentList.push_back(content);
	}
	if( !(src->bChkSun == TRUE && src->bChkMon == TRUE && src->bChkTue == TRUE && src->bChkWed == TRUE &&
		src->bChkThu == TRUE && src->bChkFri == TRUE && src->bChkSat == TRUE && 
		src->iSH == 0 && src->iSM == 0 && src->iEH == 23 && src->iEM == 59)
		){
			if(src->bChkSun == TRUE){
				EPGDB_SEARCH_DATE_INFO date;
				date.startDayOfWeek = 0;
				date.endDayOfWeek = 0;
				date.startHour = (WORD)src->iSH;
				date.startMin = (WORD)src->iSM;
				date.endHour = (WORD)src->iEH;
				date.endMin = (WORD)src->iEM;
				dest->searchInfo.dateList.push_back(date);
			}
			if(src->bChkMon == TRUE){
				EPGDB_SEARCH_DATE_INFO date;
				date.startDayOfWeek = 1;
				date.endDayOfWeek = 1;
				date.startHour = (WORD)src->iSH;
				date.startMin = (WORD)src->iSM;
				date.endHour = (WORD)src->iEH;
				date.endMin = (WORD)src->iEM;
				dest->searchInfo.dateList.push_back(date);
			}
			if(src->bChkTue == TRUE){
				EPGDB_SEARCH_DATE_INFO date;
				date.startDayOfWeek = 2;
				date.endDayOfWeek = 2;
				date.startHour = (WORD)src->iSH;
				date.startMin = (WORD)src->iSM;
				date.endHour = (WORD)src->iEH;
				date.endMin = (WORD)src->iEM;
				dest->searchInfo.dateList.push_back(date);
			}
			if(src->bChkWed == TRUE){
				EPGDB_SEARCH_DATE_INFO date;
				date.startDayOfWeek = 3;
				date.endDayOfWeek = 3;
				date.startHour = (WORD)src->iSH;
				date.startMin = (WORD)src->iSM;
				date.endHour = (WORD)src->iEH;
				date.endMin = (WORD)src->iEM;
				dest->searchInfo.dateList.push_back(date);
			}
			if(src->bChkThu == TRUE){
				EPGDB_SEARCH_DATE_INFO date;
				date.startDayOfWeek = 4;
				date.endDayOfWeek = 4;
				date.startHour = (WORD)src->iSH;
				date.startMin = (WORD)src->iSM;
				date.endHour = (WORD)src->iEH;
				date.endMin = (WORD)src->iEM;
				dest->searchInfo.dateList.push_back(date);
			}
			if(src->bChkFri == TRUE){
				EPGDB_SEARCH_DATE_INFO date;
				date.startDayOfWeek = 5;
				date.endDayOfWeek = 5;
				date.startHour = (WORD)src->iSH;
				date.startMin = (WORD)src->iSM;
				date.endHour = (WORD)src->iEH;
				date.endMin = (WORD)src->iEM;
				dest->searchInfo.dateList.push_back(date);
			}
			if(src->bChkSat == TRUE){
				EPGDB_SEARCH_DATE_INFO date;
				date.startDayOfWeek = 6;
				date.endDayOfWeek = 6;
				date.startHour = (WORD)src->iSH;
				date.startMin = (WORD)src->iSM;
				date.endHour = (WORD)src->iEH;
				date.endMin = (WORD)src->iEM;
				dest->searchInfo.dateList.push_back(date);
			}
	}
	dest->searchInfo.serviceList = src->CHIDList;

	dest->recSetting.recMode = (BYTE)src->iRecMode;
	dest->recSetting.priority = (BYTE)src->iPriority;
	dest->recSetting.tuijyuuFlag = (BYTE)src->iTuijyuu;
	dest->recSetting.serviceMode = src->dwServiceMode;
	dest->recSetting.pittariFlag = (BYTE)src->iPittari;
	dest->recSetting.batFilePath = src->strBat;
	if( src->wSuspendMode == 0 ){
		dest->recSetting.suspendMode = 4;
	}else if( src->wSuspendMode == 4 ){
		dest->recSetting.suspendMode = 0;
	}else{
		dest->recSetting.suspendMode = (BYTE)src->wSuspendMode;
	}
	dest->recSetting.rebootFlag = src->bReboot;
	dest->recSetting.useMargineFlag = src->bUseMargine;
	dest->recSetting.startMargine = src->iStartMargine;
	dest->recSetting.endMargine = src->iEndMargine;
	dest->recSetting.continueRecFlag = 0;
	dest->recSetting.partialRecFlag = 0;
	dest->recSetting.tunerID = 0;
	if( src->strRecFolder.size() > 0 ){
		REC_FILE_SET_INFO folder;
		folder.recFolder = src->strRecFolder;
		folder.writePlugIn = L"Write_Default.dll";
		dest->recSetting.recFolderList.push_back(folder);
	}
}

void CopyOldNew(OLD_SEARCH_KEY* src, EPGDB_SEARCH_KEY_INFO* dest)
{
	if( src->bRegExp == FALSE ){
		dest->andKey = src->strAnd;
		dest->notKey = src->strNot;
	}else{
		dest->andKey = src->strPattern;
	}
	dest->regExpFlag = (BYTE)src->bRegExp;
	dest->titleOnlyFlag = src->bTitle;
	if( src->iJanru != -1 ){
		EPGDB_CONTENT_DATA content;
		content.user_nibble_1 = (BYTE)src->iJanru;
		content.user_nibble_2 = 0xFF;
		dest->contentList.push_back(content);
	}
	if( !(src->bChkSun == TRUE && src->bChkMon == TRUE && src->bChkTue == TRUE && src->bChkWed == TRUE &&
		src->bChkThu == TRUE && src->bChkFri == TRUE && src->bChkSat == TRUE && 
		src->iSH == 0 && src->iSM == 0 && src->iEH == 23 && src->iEM == 59)
		){
			if(src->bChkSun == TRUE){
				EPGDB_SEARCH_DATE_INFO date;
				date.startDayOfWeek = 0;
				date.endDayOfWeek = 0;
				date.startHour = (WORD)src->iSH;
				date.startMin = (WORD)src->iSM;
				date.endHour = (WORD)src->iEH;
				date.endMin = (WORD)src->iEM;
				dest->dateList.push_back(date);
			}
			if(src->bChkMon == TRUE){
				EPGDB_SEARCH_DATE_INFO date;
				date.startDayOfWeek = 1;
				date.endDayOfWeek = 1;
				date.startHour = (WORD)src->iSH;
				date.startMin = (WORD)src->iSM;
				date.endHour = (WORD)src->iEH;
				date.endMin = (WORD)src->iEM;
				dest->dateList.push_back(date);
			}
			if(src->bChkTue == TRUE){
				EPGDB_SEARCH_DATE_INFO date;
				date.startDayOfWeek = 2;
				date.endDayOfWeek = 2;
				date.startHour = (WORD)src->iSH;
				date.startMin = (WORD)src->iSM;
				date.endHour = (WORD)src->iEH;
				date.endMin = (WORD)src->iEM;
				dest->dateList.push_back(date);
			}
			if(src->bChkWed == TRUE){
				EPGDB_SEARCH_DATE_INFO date;
				date.startDayOfWeek = 3;
				date.endDayOfWeek = 3;
				date.startHour = (WORD)src->iSH;
				date.startMin = (WORD)src->iSM;
				date.endHour = (WORD)src->iEH;
				date.endMin = (WORD)src->iEM;
				dest->dateList.push_back(date);
			}
			if(src->bChkThu == TRUE){
				EPGDB_SEARCH_DATE_INFO date;
				date.startDayOfWeek = 4;
				date.endDayOfWeek = 4;
				date.startHour = (WORD)src->iSH;
				date.startMin = (WORD)src->iSM;
				date.endHour = (WORD)src->iEH;
				date.endMin = (WORD)src->iEM;
				dest->dateList.push_back(date);
			}
			if(src->bChkFri == TRUE){
				EPGDB_SEARCH_DATE_INFO date;
				date.startDayOfWeek = 5;
				date.endDayOfWeek = 5;
				date.startHour = (WORD)src->iSH;
				date.startMin = (WORD)src->iSM;
				date.endHour = (WORD)src->iEH;
				date.endMin = (WORD)src->iEM;
				dest->dateList.push_back(date);
			}
			if(src->bChkSat == TRUE){
				EPGDB_SEARCH_DATE_INFO date;
				date.startDayOfWeek = 6;
				date.endDayOfWeek = 6;
				date.startHour = (WORD)src->iSH;
				date.startMin = (WORD)src->iSM;
				date.endHour = (WORD)src->iEH;
				date.endMin = (WORD)src->iEM;
				dest->dateList.push_back(date);
			}
	}
	dest->serviceList = src->CHIDList;
}
