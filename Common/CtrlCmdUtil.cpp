#include "stdafx.h"
#include "CtrlCmdUtil.h"

namespace CtrlCmdUtilImpl_
{

#define READ_VALUE_OR_FAIL(ver,buff,buffSize,pos,size,val)		{ if( ReadVALUE(ver,val,(buff)+pos,(buffSize)-pos,&size) == FALSE ) return FALSE; pos+=size; }
#define READ_VALUE_OR_FAIL_OLD(ver,buff,buffSize,pos,size,val)	{ if( ReadVALUE(ver,val,(buff)+pos,(buffSize)-pos,&size,true) == FALSE ) return FALSE; pos+=size; }

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const wstring& val, bool oldFormat )
{
	(void)ver;
#if WCHAR_MAX > 0xFFFF
	DWORD size = sizeof(DWORD);
	for( size_t i = 0; i < val.size() + 1; i++ ){
		size += (DWORD)((0x10000 <= val[i] && val[i] < 0x110000 ? 2 : 1) * sizeof(WORD));
	}
#else
	DWORD size = (DWORD)((val.size() + 1) * sizeof(WCHAR) + sizeof(DWORD));
#endif
	if( buff != NULL ){
		//全体のサイズ
		DWORD pos = buffOffset + WriteVALUE(0, buff, buffOffset, oldFormat ? size - (DWORD)sizeof(DWORD) : size);
#if WCHAR_MAX > 0xFFFF
		for( size_t i = 0; i < val.size() + 1; i++ ){
			if( 0x10000 <= val[i] && val[i] < 0x110000 ){
				WORD ww[2] = { (WORD)((val[i] - 0x10000) / 0x400 + 0xD800), (WORD)((val[i] - 0x10000) % 0x400 + 0xDC00) };
				memcpy(buff + pos, ww, sizeof(ww));
				pos += sizeof(ww);
			}else{
				WORD w = (WORD)val[i];
				memcpy(buff + pos, &w, sizeof(w));
				pos += sizeof(w);
			}
		}
#else
		memcpy(buff + pos, val.c_str(), (val.size() + 1) * sizeof(WCHAR));
#endif
	}
	return size;
}

BOOL ReadVALUE( WORD ver, wstring* val, const BYTE* buff, DWORD buffSize, DWORD* readSize, bool oldFormat )
{
	(void)ver;
	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	//全体のサイズ
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &valSize );
	if( oldFormat ){
		//旧形式はサイズフィールド自身のサイズを含まない
		valSize += sizeof(DWORD);
	}
	if( valSize < pos || buffSize < valSize ){
		return FALSE;
	}
	
	val->clear();
	if( valSize > pos + sizeof(WORD) ){
		val->reserve((valSize - pos) / sizeof(WORD) - 1);
	}
	while( pos + 1 < valSize && (buff[pos] || buff[pos + 1]) ){
		union { WORD w; BYTE b[2]; } x;
		x.b[0] = buff[pos++];
		x.b[1] = buff[pos++];
#if WCHAR_MAX > 0xFFFF
		if( 0xD800 <= x.w && x.w < 0xDC00 && pos + 1 < valSize && (buff[pos] || buff[pos + 1]) ){
			val->push_back(0x10000 + (x.w - 0xD800) * 0x400);
			x.b[0] = buff[pos++];
			x.b[1] = buff[pos++];
			val->back() += x.w - 0xDC00;
			continue;
		}
#endif
		val->push_back(x.w);
	}

	*readSize = valSize;
	return TRUE;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const FILE_DATA& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, val.Name);
	pos += WriteVALUE(ver, buff, pos, (DWORD)val.Data.size());
	pos += WriteVALUE(ver, buff, pos, (DWORD)0);
	if( (DWORD)val.Data.size() != 0 ){
		if( buff != NULL ) memcpy(buff + pos, &val.Data.front(), (DWORD)val.Data.size());
		pos += (DWORD)val.Data.size();
	}
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const REC_SETTING_DATA& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, val.recMode);
	pos += WriteVALUE(ver, buff, pos, val.priority);
	pos += WriteVALUE(ver, buff, pos, val.tuijyuuFlag);
	pos += WriteVALUE(ver, buff, pos, val.serviceMode);
	pos += WriteVALUE(ver, buff, pos, val.pittariFlag);
	pos += WriteVALUE(ver, buff, pos, val.batFilePath);
	pos += WriteVALUE(ver, buff, pos, val.recFolderList);
	pos += WriteVALUE(ver, buff, pos, val.suspendMode);
	pos += WriteVALUE(ver, buff, pos, val.rebootFlag);
	pos += WriteVALUE(ver, buff, pos, val.useMargineFlag);
	pos += WriteVALUE(ver, buff, pos, val.startMargine);
	pos += WriteVALUE(ver, buff, pos, val.endMargine);
	pos += WriteVALUE(ver, buff, pos, val.continueRecFlag);
	pos += WriteVALUE(ver, buff, pos, val.partialRecFlag);
	pos += WriteVALUE(ver, buff, pos, val.tunerID);
	if( ver >= 2 ){
		pos += WriteVALUE(ver, buff, pos, val.partialRecFolder);
	}
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( WORD ver, REC_SETTING_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &valSize );
	if( valSize < pos || buffSize < valSize ){
		return FALSE;
	}
	buffSize = valSize;
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->recMode );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->priority );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->tuijyuuFlag );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->serviceMode );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->pittariFlag );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->batFilePath );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->recFolderList );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->suspendMode );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->rebootFlag );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->useMargineFlag );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->startMargine );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->endMargine );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->continueRecFlag );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->partialRecFlag );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->tunerID );
	if( ver >= 2 ){
		READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->partialRecFolder );
	}
	*readSize = valSize;
	return TRUE;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const RESERVE_DATA& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, val.title);
	pos += WriteVALUE(ver, buff, pos, val.startTime);
	pos += WriteVALUE(ver, buff, pos, val.durationSecond);
	pos += WriteVALUE(ver, buff, pos, val.stationName);
	pos += WriteVALUE(ver, buff, pos, val.originalNetworkID);
	pos += WriteVALUE(ver, buff, pos, val.transportStreamID);
	pos += WriteVALUE(ver, buff, pos, val.serviceID);
	pos += WriteVALUE(ver, buff, pos, val.eventID);
	pos += WriteVALUE(ver, buff, pos, val.comment);
	pos += WriteVALUE(ver, buff, pos, val.reserveID);
	pos += WriteVALUE(ver, buff, pos, (BYTE)0);
	pos += WriteVALUE(ver, buff, pos, val.overlapMode);
	pos += WriteVALUE(ver, buff, pos, wstring());
	pos += WriteVALUE(ver, buff, pos, val.startTimeEpg);
	pos += WriteVALUE(ver, buff, pos, val.recSetting);
	pos += WriteVALUE(ver, buff, pos, val.reserveStatus);
	if( ver >= 5 ){
		pos += WriteVALUE(ver, buff, pos, val.recFileNameList);
		pos += WriteVALUE(ver, buff, pos, (DWORD)0);
	}
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( WORD ver, RESERVE_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &valSize );
	if( valSize < pos || buffSize < valSize ){
		return FALSE;
	}
	buffSize = valSize;
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->title );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->startTime );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->durationSecond );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->stationName );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->originalNetworkID );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->transportStreamID );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->serviceID );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->eventID );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->comment );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->reserveID );
	BYTE bPadding;
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &bPadding );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->overlapMode );
	wstring strPadding;
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &strPadding );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->startTimeEpg );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->recSetting );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->reserveStatus );
	if( ver >= 5 ){
		READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->recFileNameList );
		DWORD dwPadding;
		READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &dwPadding );
	}
	*readSize = valSize;
	return TRUE;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_SERVICE_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, val.ONID);
	pos += WriteVALUE(ver, buff, pos, val.TSID);
	pos += WriteVALUE(ver, buff, pos, val.SID);
	pos += WriteVALUE(ver, buff, pos, val.service_type);
	pos += WriteVALUE(ver, buff, pos, val.partialReceptionFlag);
	pos += WriteVALUE(ver, buff, pos, val.service_provider_name);
	pos += WriteVALUE(ver, buff, pos, val.service_name);
	pos += WriteVALUE(ver, buff, pos, val.network_name);
	pos += WriteVALUE(ver, buff, pos, val.ts_name);
	pos += WriteVALUE(ver, buff, pos, val.remote_control_key_id);
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( WORD ver, EPGDB_SERVICE_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &valSize );
	if( valSize < pos || buffSize < valSize ){
		return FALSE;
	}
	buffSize = valSize;
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->ONID );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->TSID );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->SID );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->service_type );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->partialReceptionFlag );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->service_provider_name );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->service_name );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->network_name );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->ts_name );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->remote_control_key_id );
	*readSize = valSize;
	return TRUE;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_SHORT_EVENT_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, val.event_name);
	pos += WriteVALUE(ver, buff, pos, val.text_char);
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( WORD ver, EPGDB_SHORT_EVENT_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &valSize );
	if( valSize < pos || buffSize < valSize ){
		return FALSE;
	}
	buffSize = valSize;
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->event_name );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->text_char );
	*readSize = valSize;
	return TRUE;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_EXTENDED_EVENT_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, val.text_char);
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( WORD ver, EPGDB_EXTENDED_EVENT_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &valSize );
	if( valSize < pos || buffSize < valSize ){
		return FALSE;
	}
	buffSize = valSize;
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->text_char );
	*readSize = valSize;
	return TRUE;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_CONTENT_DATA& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, val.content_nibble_level_1);
	pos += WriteVALUE(ver, buff, pos, val.content_nibble_level_2);
	pos += WriteVALUE(ver, buff, pos, val.user_nibble_1);
	pos += WriteVALUE(ver, buff, pos, val.user_nibble_2);
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( WORD ver, EPGDB_CONTENT_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &valSize );
	if( valSize < pos || buffSize < valSize ){
		return FALSE;
	}
	buffSize = valSize;
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->content_nibble_level_1 );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->content_nibble_level_2 );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->user_nibble_1 );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->user_nibble_2 );
	*readSize = valSize;
	return TRUE;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_CONTEN_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, val.nibbleList);
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( WORD ver, EPGDB_CONTEN_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &valSize );
	if( valSize < pos || buffSize < valSize ){
		return FALSE;
	}
	buffSize = valSize;
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->nibbleList );
	*readSize = valSize;
	return TRUE;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_COMPONENT_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, val.stream_content);
	pos += WriteVALUE(ver, buff, pos, val.component_type);
	pos += WriteVALUE(ver, buff, pos, val.component_tag);
	pos += WriteVALUE(ver, buff, pos, val.text_char);
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( WORD ver, EPGDB_COMPONENT_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &valSize );
	if( valSize < pos || buffSize < valSize ){
		return FALSE;
	}
	buffSize = valSize;
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->stream_content );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->component_type );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->component_tag );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->text_char );
	*readSize = valSize;
	return TRUE;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_AUDIO_COMPONENT_INFO_DATA& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, val.stream_content);
	pos += WriteVALUE(ver, buff, pos, val.component_type);
	pos += WriteVALUE(ver, buff, pos, val.component_tag);
	pos += WriteVALUE(ver, buff, pos, val.stream_type);
	pos += WriteVALUE(ver, buff, pos, val.simulcast_group_tag);
	pos += WriteVALUE(ver, buff, pos, val.ES_multi_lingual_flag);
	pos += WriteVALUE(ver, buff, pos, val.main_component_flag);
	pos += WriteVALUE(ver, buff, pos, val.quality_indicator);
	pos += WriteVALUE(ver, buff, pos, val.sampling_rate);
	pos += WriteVALUE(ver, buff, pos, val.text_char);
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( WORD ver, EPGDB_AUDIO_COMPONENT_INFO_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &valSize );
	if( valSize < pos || buffSize < valSize ){
		return FALSE;
	}
	buffSize = valSize;
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->stream_content );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->component_type );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->component_tag );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->stream_type );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->simulcast_group_tag );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->ES_multi_lingual_flag );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->main_component_flag );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->quality_indicator );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->sampling_rate );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->text_char );
	*readSize = valSize;
	return TRUE;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_AUDIO_COMPONENT_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, val.componentList);
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( WORD ver, EPGDB_AUDIO_COMPONENT_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &valSize );
	if( valSize < pos || buffSize < valSize ){
		return FALSE;
	}
	buffSize = valSize;
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->componentList );
	*readSize = valSize;
	return TRUE;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_EVENT_DATA& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, val.original_network_id);
	pos += WriteVALUE(ver, buff, pos, val.transport_stream_id);
	pos += WriteVALUE(ver, buff, pos, val.service_id);
	pos += WriteVALUE(ver, buff, pos, val.event_id);
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( WORD ver, EPGDB_EVENT_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &valSize );
	if( valSize < pos || buffSize < valSize ){
		return FALSE;
	}
	buffSize = valSize;
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->original_network_id );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->transport_stream_id );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->service_id );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->event_id );
	*readSize = valSize;
	return TRUE;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_EVENTGROUP_INFO& val, BYTE groupType )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, groupType);
	pos += WriteVALUE(ver, buff, pos, val.eventDataList);
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( WORD ver, EPGDB_EVENTGROUP_INFO* val, BYTE* groupType, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &valSize );
	if( valSize < pos || buffSize < valSize ){
		return FALSE;
	}
	buffSize = valSize;
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, groupType );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->eventDataList );
	*readSize = valSize;
	return TRUE;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_EVENT_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, val.original_network_id);
	pos += WriteVALUE(ver, buff, pos, val.transport_stream_id);
	pos += WriteVALUE(ver, buff, pos, val.service_id);
	pos += WriteVALUE(ver, buff, pos, val.event_id);
	pos += WriteVALUE(ver, buff, pos, val.StartTimeFlag);
	pos += WriteVALUE(ver, buff, pos, val.start_time);
	pos += WriteVALUE(ver, buff, pos, val.DurationFlag);
	pos += WriteVALUE(ver, buff, pos, val.durationSec);
	pos += val.hasShortInfo ? WriteVALUE(ver, buff, pos, val.shortInfo) : WriteVALUE(0, buff, pos, (DWORD)sizeof(DWORD));
	pos += val.hasExtInfo ? WriteVALUE(ver, buff, pos, val.extInfo) : WriteVALUE(0, buff, pos, (DWORD)sizeof(DWORD));
	pos += val.hasContentInfo ? WriteVALUE(ver, buff, pos, val.contentInfo) : WriteVALUE(0, buff, pos, (DWORD)sizeof(DWORD));
	pos += val.hasComponentInfo ? WriteVALUE(ver, buff, pos, val.componentInfo) : WriteVALUE(0, buff, pos, (DWORD)sizeof(DWORD));
	pos += val.hasAudioInfo ? WriteVALUE(ver, buff, pos, val.audioInfo) : WriteVALUE(0, buff, pos, (DWORD)sizeof(DWORD));
	pos += val.eventGroupInfoGroupType ? WriteVALUE(ver, buff, pos, val.eventGroupInfo, val.eventGroupInfoGroupType) : WriteVALUE(0, buff, pos, (DWORD)sizeof(DWORD));
	pos += val.eventRelayInfoGroupType ? WriteVALUE(ver, buff, pos, val.eventRelayInfo, val.eventRelayInfoGroupType) : WriteVALUE(0, buff, pos, (DWORD)sizeof(DWORD));
	pos += WriteVALUE(ver, buff, pos, val.freeCAFlag);
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( WORD ver, EPGDB_EVENT_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &valSize );
	if( valSize < pos || buffSize < valSize ){
		return FALSE;
	}
	buffSize = valSize;

	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->original_network_id );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->transport_stream_id );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->service_id );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->event_id );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->StartTimeFlag );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->start_time );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->DurationFlag );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->durationSec );
	{
		DWORD infoValSize;

		val->hasShortInfo = true;
		if( ReadVALUE(ver, &val->shortInfo, buff + pos, buffSize - pos, &size) == FALSE ){
			val->hasShortInfo = false;
			READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &infoValSize );
			if( infoValSize != sizeof(DWORD) ){
				return FALSE;
			}
		}else{
			pos += size;
		}

		val->hasExtInfo = true;
		if( ReadVALUE(ver, &val->extInfo, buff + pos, buffSize - pos, &size) == FALSE ){
			val->hasExtInfo = false;
			READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &infoValSize );
			if( infoValSize != sizeof(DWORD) ){
				return FALSE;
			}
		}else{
			pos += size;
		}

		val->hasContentInfo = true;
		val->contentInfo.nibbleList.clear();
		if( ReadVALUE(ver, &val->contentInfo, buff + pos, buffSize - pos, &size) == FALSE ){
			val->hasContentInfo = false;
			READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &infoValSize );
			if( infoValSize != sizeof(DWORD) ){
				return FALSE;
			}
		}else{
			pos += size;
		}

		val->hasComponentInfo = true;
		if( ReadVALUE(ver, &val->componentInfo, buff + pos, buffSize - pos, &size) == FALSE ){
			val->hasComponentInfo = false;
			READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &infoValSize );
			if( infoValSize != sizeof(DWORD) ){
				return FALSE;
			}
		}else{
			pos += size;
		}

		val->hasAudioInfo = true;
		val->audioInfo.componentList.clear();
		if( ReadVALUE(ver, &val->audioInfo, buff + pos, buffSize - pos, &size) == FALSE ){
			val->hasAudioInfo = false;
			READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &infoValSize );
			if( infoValSize != sizeof(DWORD) ){
				return FALSE;
			}
		}else{
			pos += size;
		}

		val->eventGroupInfo.eventDataList.clear();
		if( ReadVALUE(ver, &val->eventGroupInfo, &val->eventGroupInfoGroupType, buff + pos, buffSize - pos, &size) == FALSE ){
			val->eventGroupInfoGroupType = 0;
			READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &infoValSize );
			if( infoValSize != sizeof(DWORD) ){
				return FALSE;
			}
		}else{
			pos += size;
		}

		val->eventRelayInfo.eventDataList.clear();
		if( ReadVALUE(ver, &val->eventRelayInfo, &val->eventRelayInfoGroupType, buff + pos, buffSize - pos, &size) == FALSE ){
			val->eventRelayInfoGroupType = 0;
			READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &infoValSize );
			if( infoValSize != sizeof(DWORD) ){
				return FALSE;
			}
		}else{
			pos += size;
		}
	}
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->freeCAFlag );

	*readSize = valSize;
	return TRUE;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_SEARCH_DATE_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, val.startDayOfWeek);
	pos += WriteVALUE(ver, buff, pos, val.startHour);
	pos += WriteVALUE(ver, buff, pos, val.startMin);
	pos += WriteVALUE(ver, buff, pos, val.endDayOfWeek);
	pos += WriteVALUE(ver, buff, pos, val.endHour);
	pos += WriteVALUE(ver, buff, pos, val.endMin);
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( WORD ver, EPGDB_SEARCH_DATE_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &valSize );
	if( valSize < pos || buffSize < valSize ){
		return FALSE;
	}
	buffSize = valSize;
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->startDayOfWeek );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->startHour );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->startMin );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->endDayOfWeek );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->endHour );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->endMin );
	*readSize = valSize;
	return TRUE;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_SEARCH_KEY_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, val.andKey);
	pos += WriteVALUE(ver, buff, pos, val.notKey);
	pos += WriteVALUE(ver, buff, pos, val.regExpFlag);
	pos += WriteVALUE(ver, buff, pos, val.titleOnlyFlag);
	pos += WriteVALUE(ver, buff, pos, val.contentList);
	pos += WriteVALUE(ver, buff, pos, val.dateList);
	pos += WriteVALUE(ver, buff, pos, val.serviceList);
	pos += WriteVALUE(ver, buff, pos, val.videoList);
	pos += WriteVALUE(ver, buff, pos, val.audioList);
	pos += WriteVALUE(ver, buff, pos, val.aimaiFlag);
	pos += WriteVALUE(ver, buff, pos, val.notContetFlag);
	pos += WriteVALUE(ver, buff, pos, val.notDateFlag);
	pos += WriteVALUE(ver, buff, pos, val.freeCAFlag);
	if( ver >= 3 ){
		pos += WriteVALUE(ver, buff, pos, val.chkRecEnd);
		pos += WriteVALUE(ver, buff, pos, val.chkRecDay);
	}
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( WORD ver, EPGDB_SEARCH_KEY_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &valSize );
	if( valSize < pos || buffSize < valSize ){
		return FALSE;
	}
	buffSize = valSize;
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->andKey );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->notKey );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->regExpFlag );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->titleOnlyFlag );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->contentList );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->dateList );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->serviceList );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->videoList );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->audioList );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->aimaiFlag );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->notContetFlag );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->notDateFlag );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->freeCAFlag );
	val->chkRecEnd = 0;
	val->chkRecDay = 6;
	if( ver >= 3 ){
		READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->chkRecEnd );
		READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->chkRecDay );
	}
	if( ver >= 5 ){
		if( buffSize - pos >= 5 ){
			//録画済チェックに関する追加のフィールドがある
			BYTE recNoService;
			READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &recNoService );
			if( recNoService ){
				val->chkRecDay = val->chkRecDay % 10000 + 40000;
			}
			WORD durMin;
			WORD durMax;
			READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &durMin );
			READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &durMax );
			if( durMin > 0 || durMax > 0 ){
				WCHAR dur[32];
				swprintf_s(dur, L"D!{%d}", (10000 + min(max((int)durMin, 0), 9999)) * 10000 + min(max((int)durMax, 0), 9999));
				size_t durPos = val->andKey.compare(0, 7, L"^!{999}") ? 0 : 7;
				durPos += val->andKey.compare(durPos, 7, L"C!{999}") ? 0 : 7;
				val->andKey.insert(durPos, dur);
			}
		}
	}
	*readSize = valSize;
	return TRUE;
}

BOOL ReadVALUE( WORD ver, SEARCH_PG_PARAM* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &valSize );
	if( valSize < pos || buffSize < valSize ){
		return FALSE;
	}
	buffSize = valSize;
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->keyList );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->enumStart );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->enumEnd );
	*readSize = valSize;
	return TRUE;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const SET_CH_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, val.useSID);
	pos += WriteVALUE(ver, buff, pos, val.ONID);
	pos += WriteVALUE(ver, buff, pos, val.TSID);
	pos += WriteVALUE(ver, buff, pos, val.SID);
	pos += WriteVALUE(ver, buff, pos, val.useBonCh);
	pos += WriteVALUE(ver, buff, pos, val.space);
	pos += WriteVALUE(ver, buff, pos, val.ch);
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( WORD ver, SET_CH_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &valSize );
	if( valSize < pos || buffSize < valSize ){
		return FALSE;
	}
	buffSize = valSize;
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->useSID );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->ONID );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->TSID );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->SID );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->useBonCh );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->space );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->ch );
	*readSize = valSize;
	return TRUE;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const SET_CTRL_MODE& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, val.ctrlID);
	pos += WriteVALUE(ver, buff, pos, val.SID);
	pos += WriteVALUE(ver, buff, pos, val.enableScramble);
	pos += WriteVALUE(ver, buff, pos, val.enableCaption);
	pos += WriteVALUE(ver, buff, pos, val.enableData);
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( WORD ver, SET_CTRL_MODE* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &valSize );
	if( valSize < pos || buffSize < valSize ){
		return FALSE;
	}
	buffSize = valSize;
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->ctrlID );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->SID );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->enableScramble );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->enableCaption );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->enableData );
	*readSize = valSize;
	return TRUE;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const REC_FILE_SET_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, val.recFolder);
	pos += WriteVALUE(ver, buff, pos, val.writePlugIn);
	pos += WriteVALUE(ver, buff, pos, val.recNamePlugIn);
	pos += WriteVALUE(ver, buff, pos, val.recFileName);
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( WORD ver, REC_FILE_SET_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &valSize );
	if( valSize < pos || buffSize < valSize ){
		return FALSE;
	}
	buffSize = valSize;
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->recFolder );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->writePlugIn );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->recNamePlugIn );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->recFileName );
	*readSize = valSize;
	return TRUE;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const SET_CTRL_REC_PARAM& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, val.ctrlID);
	pos += WriteVALUE(ver, buff, pos, val.fileName);
	pos += WriteVALUE(ver, buff, pos, val.overWriteFlag);
	pos += WriteVALUE(ver, buff, pos, val.createSize);
	pos += WriteVALUE(ver, buff, pos, val.saveFolder);
	pos += WriteVALUE(ver, buff, pos, val.pittariFlag);
	pos += WriteVALUE(ver, buff, pos, val.pittariONID);
	pos += WriteVALUE(ver, buff, pos, val.pittariTSID);
	pos += WriteVALUE(ver, buff, pos, val.pittariSID);
	pos += WriteVALUE(ver, buff, pos, val.pittariEventID);
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( WORD ver, SET_CTRL_REC_PARAM* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &valSize );
	if( valSize < pos || buffSize < valSize ){
		return FALSE;
	}
	buffSize = valSize;
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->ctrlID );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->fileName );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->overWriteFlag );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->createSize );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->saveFolder );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->pittariFlag );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->pittariONID );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->pittariTSID );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->pittariSID );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->pittariEventID );
	*readSize = valSize;
	return TRUE;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const SET_CTRL_REC_STOP_PARAM& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, val.ctrlID);
	pos += WriteVALUE(ver, buff, pos, val.saveErrLog);
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( WORD ver, SET_CTRL_REC_STOP_PARAM* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &valSize );
	if( valSize < pos || buffSize < valSize ){
		return FALSE;
	}
	buffSize = valSize;
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->ctrlID );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->saveErrLog );
	*readSize = valSize;
	return TRUE;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const SET_CTRL_REC_STOP_RES_PARAM& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, val.recFilePath);
	pos += WriteVALUE(ver, buff, pos, val.drop);
	pos += WriteVALUE(ver, buff, pos, val.scramble);
	pos += WriteVALUE(ver, buff, pos, val.subRecFlag);
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( WORD ver, SET_CTRL_REC_STOP_RES_PARAM* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &valSize );
	if( valSize < pos || buffSize < valSize ){
		return FALSE;
	}
	buffSize = valSize;
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->recFilePath );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->drop );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->scramble );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->subRecFlag );
	*readSize = valSize;
	return TRUE;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const REC_FILE_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, val.id);
	pos += WriteVALUE(ver, buff, pos, val.recFilePath);
	pos += WriteVALUE(ver, buff, pos, val.title);
	pos += WriteVALUE(ver, buff, pos, val.startTime);
	pos += WriteVALUE(ver, buff, pos, val.durationSecond);
	pos += WriteVALUE(ver, buff, pos, val.serviceName);
	pos += WriteVALUE(ver, buff, pos, val.originalNetworkID);
	pos += WriteVALUE(ver, buff, pos, val.transportStreamID);
	pos += WriteVALUE(ver, buff, pos, val.serviceID);
	pos += WriteVALUE(ver, buff, pos, val.eventID);
	pos += WriteVALUE(ver, buff, pos, val.drops);
	pos += WriteVALUE(ver, buff, pos, val.scrambles);
	pos += WriteVALUE(ver, buff, pos, val.recStatus);
	pos += WriteVALUE(ver, buff, pos, val.startTimeEpg);
	pos += WriteVALUE(ver, buff, pos, wstring(val.GetComment()));
	pos += WriteVALUE(ver, buff, pos, val.programInfo);
	pos += WriteVALUE(ver, buff, pos, val.errInfo);
	if( ver >= 4 ){
		pos += WriteVALUE(ver, buff, pos, val.protectFlag);
	}
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( WORD ver, REC_FILE_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &valSize );
	if( valSize < pos || buffSize < valSize ){
		return FALSE;
	}
	buffSize = valSize;
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->id );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->recFilePath );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->title );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->startTime );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->durationSecond );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->serviceName );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->originalNetworkID );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->transportStreamID );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->serviceID );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->eventID );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->drops );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->scrambles );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->recStatus );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->startTimeEpg );
	wstring strPadding;
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &strPadding );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->programInfo );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->errInfo );
	val->protectFlag = 0;
	if( ver >= 4 ){
		READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->protectFlag );
	}
	*readSize = valSize;
	return TRUE;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPG_AUTO_ADD_DATA& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, val.dataID);
	pos += WriteVALUE(ver, buff, pos, val.searchInfo);
	pos += WriteVALUE(ver, buff, pos, val.recSetting);
	if( ver >= 5 ){
		pos += WriteVALUE(ver, buff, pos, val.addCount);
	}
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( WORD ver, EPG_AUTO_ADD_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &valSize );
	if( valSize < pos || buffSize < valSize ){
		return FALSE;
	}
	buffSize = valSize;
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->dataID );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->searchInfo );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->recSetting );
	if( ver >= 5 ){
		READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->addCount );
	}
	*readSize = valSize;
	return TRUE;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const SEARCH_EPG_INFO_PARAM& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, val.ONID);
	pos += WriteVALUE(ver, buff, pos, val.TSID);
	pos += WriteVALUE(ver, buff, pos, val.SID);
	pos += WriteVALUE(ver, buff, pos, val.eventID);
	pos += WriteVALUE(ver, buff, pos, val.pfOnlyFlag);
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( WORD ver, SEARCH_EPG_INFO_PARAM* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &valSize );
	if( valSize < pos || buffSize < valSize ){
		return FALSE;
	}
	buffSize = valSize;
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->ONID );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->TSID );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->SID );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->eventID );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->pfOnlyFlag );
	*readSize = valSize;
	return TRUE;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const GET_EPG_PF_INFO_PARAM& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, val.ONID);
	pos += WriteVALUE(ver, buff, pos, val.TSID);
	pos += WriteVALUE(ver, buff, pos, val.SID);
	pos += WriteVALUE(ver, buff, pos, val.pfNextFlag);
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( WORD ver, GET_EPG_PF_INFO_PARAM* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &valSize );
	if( valSize < pos || buffSize < valSize ){
		return FALSE;
	}
	buffSize = valSize;
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->ONID );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->TSID );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->SID );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->pfNextFlag );
	*readSize = valSize;
	return TRUE;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const MANUAL_AUTO_ADD_DATA& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, val.dataID);
	pos += WriteVALUE(ver, buff, pos, val.dayOfWeekFlag);
	pos += WriteVALUE(ver, buff, pos, val.startTime);
	pos += WriteVALUE(ver, buff, pos, val.durationSecond);
	pos += WriteVALUE(ver, buff, pos, val.title);
	pos += WriteVALUE(ver, buff, pos, val.stationName);
	pos += WriteVALUE(ver, buff, pos, val.originalNetworkID);
	pos += WriteVALUE(ver, buff, pos, val.transportStreamID);
	pos += WriteVALUE(ver, buff, pos, val.serviceID);
	pos += WriteVALUE(ver, buff, pos, val.recSetting);
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( WORD ver, MANUAL_AUTO_ADD_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &valSize );
	if( valSize < pos || buffSize < valSize ){
		return FALSE;
	}
	buffSize = valSize;
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->dataID );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->dayOfWeekFlag );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->startTime );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->durationSecond );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->title );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->stationName );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->originalNetworkID );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->transportStreamID );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->serviceID );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->recSetting );
	*readSize = valSize;
	return TRUE;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const TUNER_RESERVE_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, val.tunerID);
	pos += WriteVALUE(ver, buff, pos, val.tunerName);
	pos += WriteVALUE(ver, buff, pos, val.reserveList);
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_SERVICE_EVENT_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, val.serviceInfo);
	pos += WriteVALUE(ver, buff, pos, val.eventList);
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_SERVICE_EVENT_INFO_PTR& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, *val.serviceInfo);
	pos += WriteVALUE(ver, buff, pos, val.eventList);
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( WORD ver, EPGDB_SERVICE_EVENT_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &valSize );
	if( valSize < pos || buffSize < valSize ){
		return FALSE;
	}
	buffSize = valSize;
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->serviceInfo );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->eventList );
	*readSize = valSize;
	return TRUE;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const TVTEST_CH_CHG_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, val.bonDriver);
	pos += WriteVALUE(ver, buff, pos, val.chInfo);
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const NWPLAY_PLAY_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, val.ctrlID);
	pos += WriteVALUE(ver, buff, pos, val.ip);
	pos += WriteVALUE(ver, buff, pos, val.udp);
	pos += WriteVALUE(ver, buff, pos, val.tcp);
	pos += WriteVALUE(ver, buff, pos, val.udpPort);
	pos += WriteVALUE(ver, buff, pos, val.tcpPort);
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( WORD ver, NWPLAY_PLAY_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &valSize );
	if( valSize < pos || buffSize < valSize ){
		return FALSE;
	}
	buffSize = valSize;
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->ctrlID );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->ip );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->udp );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->tcp );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->udpPort );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->tcpPort );
	*readSize = valSize;
	return TRUE;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const NWPLAY_POS_CMD& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, val.ctrlID);
	pos += WriteVALUE(ver, buff, pos, val.currentPos);
	pos += WriteVALUE(ver, buff, pos, val.totalPos);
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( WORD ver, NWPLAY_POS_CMD* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &valSize );
	if( valSize < pos || buffSize < valSize ){
		return FALSE;
	}
	buffSize = valSize;
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->ctrlID );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->currentPos );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->totalPos );
	*readSize = valSize;
	return TRUE;
}

BOOL ReadVALUE( WORD ver, TVTEST_STREAMING_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &valSize );
	if( valSize < pos || buffSize < valSize ){
		return FALSE;
	}
	buffSize = valSize;
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->enableMode );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->ctrlID );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->serverIP );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->serverPort );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->filePath );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->udpSend );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->tcpSend );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->timeShiftMode );
	*readSize = valSize;
	return TRUE;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const NWPLAY_TIMESHIFT_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, val.ctrlID);
	pos += WriteVALUE(ver, buff, pos, val.filePath);
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const NOTIFY_SRV_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, val.notifyID);
	pos += WriteVALUE(ver, buff, pos, val.time);
	pos += WriteVALUE(ver, buff, pos, val.param1);
	pos += WriteVALUE(ver, buff, pos, val.param2);
	pos += WriteVALUE(ver, buff, pos, val.param3);
	pos += WriteVALUE(ver, buff, pos, val.param4);
	pos += WriteVALUE(ver, buff, pos, val.param5);
	pos += WriteVALUE(ver, buff, pos, val.param6);
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

BOOL ReadVALUE( WORD ver, NOTIFY_SRV_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &valSize );
	if( valSize < pos || buffSize < valSize ){
		return FALSE;
	}
	buffSize = valSize;
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->notifyID );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->time );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->param1 );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->param2 );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->param3 );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->param4 );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->param5 );
	READ_VALUE_OR_FAIL( ver, buff, buffSize, pos, size, &val->param6 );
	*readSize = valSize;
	return TRUE;
}

}

////////////////////////////////////////////////////////////////////////////////////////////
//旧バージョンコマンド送信用バイナリ作成関数
std::unique_ptr<BYTE[]> DeprecatedNewWriteVALUE( const RESERVE_DATA& val, DWORD& writeSize, std::unique_ptr<BYTE[]>&& buff_ )
{
	using namespace CtrlCmdUtilImpl_;
	BYTE* buff = buff_.get();
	DWORD pos = 0;
	pos += WriteVALUE(0, buff, pos, val.title, true);
	pos += WriteVALUE(0, buff, pos, val.startTime);
	pos += WriteVALUE(0, buff, pos, val.durationSecond);
	pos += WriteVALUE(0, buff, pos, val.stationName, true);
	pos += WriteVALUE(0, buff, pos, val.originalNetworkID);
	pos += WriteVALUE(0, buff, pos, val.transportStreamID);
	pos += WriteVALUE(0, buff, pos, val.serviceID);
	pos += WriteVALUE(0, buff, pos, val.eventID);
	pos += WriteVALUE(0, buff, pos, val.recSetting.priority);
	pos += WriteVALUE(0, buff, pos, val.recSetting.tuijyuuFlag);
	pos += WriteVALUE(0, buff, pos, val.comment, true);
	pos += WriteVALUE(0, buff, pos, (DWORD)val.recSetting.recMode);
	pos += WriteVALUE(0, buff, pos, (DWORD)val.recSetting.pittariFlag);
	pos += WriteVALUE(0, buff, pos, val.recSetting.batFilePath, true);
	pos += WriteVALUE(0, buff, pos, val.reserveID);
	pos += WriteVALUE(0, buff, pos, (DWORD)0);
	pos += WriteVALUE(0, buff, pos, (DWORD)val.overlapMode);
	pos += WriteVALUE(0, buff, pos, val.recSetting.recFolderList.empty() ? wstring() : val.recSetting.recFolderList[0].recFolder, true);
	pos += WriteVALUE(0, buff, pos, (WORD)val.recSetting.suspendMode);
	pos += WriteVALUE(0, buff, pos, (DWORD)val.recSetting.rebootFlag);
	pos += WriteVALUE(0, buff, pos, wstring(), true);
	pos += WriteVALUE(0, buff, pos, (DWORD)val.recSetting.useMargineFlag);
	pos += WriteVALUE(0, buff, pos, val.recSetting.startMargine);
	pos += WriteVALUE(0, buff, pos, val.recSetting.endMargine);
	pos += WriteVALUE(0, buff, pos, val.recSetting.serviceMode);
	if( buff == NULL ){
		return DeprecatedNewWriteVALUE(val, writeSize, std::unique_ptr<BYTE[]>(new BYTE[pos]));
	}
	writeSize = pos;
	return std::move(buff_);
}

BOOL DeprecatedReadVALUE( RESERVE_DATA* val, const std::unique_ptr<BYTE[]>& buff_, DWORD buffSize )
{
	using namespace CtrlCmdUtilImpl_;
	const BYTE* buff = buff_.get();
	if( val == NULL || buff == NULL ){
		return FALSE;
	}
	DWORD pos = 0;
	DWORD size = 0;
	READ_VALUE_OR_FAIL_OLD( 0, buff, buffSize, pos, size, &val->title );
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &val->startTime );
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &val->durationSecond );
	READ_VALUE_OR_FAIL_OLD( 0, buff, buffSize, pos, size, &val->stationName );
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &val->originalNetworkID );
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &val->transportStreamID );
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &val->serviceID );
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &val->eventID );
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &val->recSetting.priority );
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &val->recSetting.tuijyuuFlag );
	READ_VALUE_OR_FAIL_OLD( 0, buff, buffSize, pos, size, &val->comment );
	DWORD dwRead;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &dwRead );
	val->recSetting.recMode = dwRead & 0xFF;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &dwRead );
	val->recSetting.pittariFlag = dwRead != 0;
	READ_VALUE_OR_FAIL_OLD( 0, buff, buffSize, pos, size, &val->recSetting.batFilePath );
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &val->reserveID );
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &dwRead );
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &dwRead );
	REC_FILE_SET_INFO folder;
	READ_VALUE_OR_FAIL_OLD( 0, buff, buffSize, pos, size, &folder.recFolder );
	if( folder.recFolder.empty() == false ){
		folder.writePlugIn = L"Write_Default.dll";
		val->recSetting.recFolderList.push_back(folder);
	}
	WORD wRead;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &wRead );
	//旧→新のみなぜかこの数値変換が入る(互換のため修正しない)
	val->recSetting.suspendMode = (wRead == 0 ? 4 : wRead == 4 ? 0 : wRead) & 0xFF;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &dwRead );
	val->recSetting.rebootFlag = dwRead != 0;
	wstring strPadding;
	READ_VALUE_OR_FAIL_OLD( 0, buff, buffSize, pos, size, &strPadding );
	if( pos < buffSize ){
		READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &dwRead );
		val->recSetting.useMargineFlag = dwRead != 0;
		READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &val->recSetting.startMargine );
		READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &val->recSetting.endMargine );
		READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &val->recSetting.serviceMode );
	}else{
		val->recSetting.useMargineFlag = 0;
		val->recSetting.startMargine = 0;
		val->recSetting.endMargine = 0;
		val->recSetting.serviceMode = 0;
	}
	val->overlapMode = 0;
	val->startTimeEpg = val->startTime;
	val->recSetting.continueRecFlag = 0;
	val->recSetting.partialRecFlag = 0;
	val->recSetting.tunerID = 0;
	return TRUE;
}

BOOL DeprecatedReadVALUE( EPG_AUTO_ADD_DATA* val, const std::unique_ptr<BYTE[]>& buff_, DWORD buffSize )
{
	using namespace CtrlCmdUtilImpl_;
	const BYTE* buff = buff_.get();
	if( val == NULL || buff == NULL ){
		return FALSE;
	}
	DWORD pos = 0;
	DWORD size = 0;
	READ_VALUE_OR_FAIL_OLD( 0, buff, buffSize, pos, size, &val->searchInfo.andKey );
	READ_VALUE_OR_FAIL_OLD( 0, buff, buffSize, pos, size, &val->searchInfo.notKey );
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &val->searchInfo.titleOnlyFlag );
	int iJanru;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &iJanru );
	if( iJanru != -1 ){
		EPGDB_CONTENT_DATA content;
		//原作と異なりuser_nibbleでなくcontent_nibbleに変換するので注意
		content.content_nibble_level_1 = iJanru & 0xFF;
		content.content_nibble_level_2 = 0xFF;
		content.user_nibble_1 = 0;
		content.user_nibble_2 = 0;
		val->searchInfo.contentList.push_back(content);
	}
	int iSH, iSM, iEH, iEM;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &iSH );
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &iSM );
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &iEH );
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &iEM );
	val->searchInfo.dateList.clear();
	for( DWORD i = 1; i < 8; i++ ){
		DWORD dwChkDayOfWeek;
		READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &dwChkDayOfWeek );
		if( dwChkDayOfWeek == 1 ){
			EPGDB_SEARCH_DATE_INFO date;
			date.startDayOfWeek = i % 7;
			date.startHour = iSH & 0xFF;
			date.startMin = iSM & 0xFF;
			date.endDayOfWeek = i % 7;
			date.endHour = iEH & 0xFF;
			date.endMin = iEM & 0xFF;
			val->searchInfo.dateList.push_back(date);
		}
	}
	if( iSH == 0 && iSM == 0 && iEH == 23 && iEM == 59 && val->searchInfo.dateList.size() == 7 ){
		val->searchInfo.dateList.clear();
	}
	DWORD dwRead;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &dwRead );
	for( DWORD i = 0; i < dwRead; i++ ){
		__int64 item;
		READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &item );
		val->searchInfo.serviceList.push_back(item);
	}
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &val->dataID );
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &dwRead );
	val->recSetting.priority = dwRead & 0xFF;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &dwRead );
	val->recSetting.tuijyuuFlag = dwRead != 0;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &dwRead );
	val->recSetting.recMode = dwRead & 0xFF;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &dwRead );
	val->recSetting.pittariFlag = dwRead != 0;
	READ_VALUE_OR_FAIL_OLD( 0, buff, buffSize, pos, size, &val->recSetting.batFilePath );
	REC_FILE_SET_INFO folder;
	READ_VALUE_OR_FAIL_OLD( 0, buff, buffSize, pos, size, &folder.recFolder );
	if( folder.recFolder.empty() == false ){
		folder.writePlugIn = L"Write_Default.dll";
		val->recSetting.recFolderList.push_back(folder);
	}
	WORD wRead;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &wRead );
	val->recSetting.suspendMode = (wRead == 0 ? 4 : wRead ==4 ? 0 : wRead) & 0xFF;
	READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &dwRead );
	val->recSetting.rebootFlag = dwRead != 0;
	if( pos < buffSize ){
		READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &dwRead );
		val->recSetting.useMargineFlag = dwRead != 0;
		READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &val->recSetting.startMargine );
		READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &val->recSetting.endMargine );
		READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &val->recSetting.serviceMode );
	}else{
		val->recSetting.useMargineFlag = 0;
		val->recSetting.startMargine = 0;
		val->recSetting.endMargine = 0;
		val->recSetting.serviceMode = 0;
	}
	if( pos < buffSize ){
		READ_VALUE_OR_FAIL( 0, buff, buffSize, pos, size, &dwRead );
		val->searchInfo.regExpFlag = dwRead != 0;
		wstring strRead;
		READ_VALUE_OR_FAIL_OLD( 0, buff, buffSize, pos, size, &strRead );
		if( val->searchInfo.regExpFlag ){
			val->searchInfo.andKey = strRead;
			val->searchInfo.notKey = L"";
		}
	}else{
		val->searchInfo.regExpFlag = 0;
	}
	val->searchInfo.aimaiFlag = 0;
	val->searchInfo.notContetFlag = 0;
	val->searchInfo.notDateFlag = 0;
	val->searchInfo.freeCAFlag = 0;
	val->searchInfo.chkRecEnd = 0;
	val->searchInfo.chkRecDay = 6;
	val->recSetting.continueRecFlag = 0;
	val->recSetting.partialRecFlag = 0;
	val->recSetting.tunerID = 0;
	return TRUE;
}

std::unique_ptr<BYTE[]> DeprecatedNewWriteVALUE( const EPGDB_EVENT_INFO& val, DWORD& writeSize, std::unique_ptr<BYTE[]>&& buff_ )
{
	using namespace CtrlCmdUtilImpl_;
	BYTE* buff = buff_.get();
	DWORD pos = 0;
	pos += WriteVALUE(0, buff, pos, val.original_network_id);
	pos += WriteVALUE(0, buff, pos, val.transport_stream_id);
	pos += WriteVALUE(0, buff, pos, val.service_id);
	pos += WriteVALUE(0, buff, pos, val.event_id);
	pos += WriteVALUE(0, buff, pos, val.hasShortInfo ? val.shortInfo.event_name : wstring(), true);
	pos += WriteVALUE(0, buff, pos, val.hasShortInfo ? val.shortInfo.text_char : wstring(), true);
	pos += WriteVALUE(0, buff, pos, val.hasExtInfo ? val.extInfo.text_char : wstring(), true);
	SYSTEMTIME stZero = {};
	pos += WriteVALUE(0, buff, pos, val.StartTimeFlag ? val.start_time : stZero);
	pos += WriteVALUE(0, buff, pos, (DWORD)(val.DurationFlag ? val.durationSec : 0));
	pos += WriteVALUE(0, buff, pos, (BYTE)(val.hasComponentInfo ? val.componentInfo.component_type : 0));
	pos += WriteVALUE(0, buff, pos, val.hasComponentInfo ? val.componentInfo.text_char : wstring(), true);
	const EPGDB_AUDIO_COMPONENT_INFO_DATA* ac = val.hasAudioInfo && val.audioInfo.componentList.empty() == false ? &val.audioInfo.componentList[0] : NULL;
	pos += WriteVALUE(0, buff, pos, (BYTE)(ac ? ac->component_type : 0));
	pos += WriteVALUE(0, buff, pos, (BYTE)(ac ? ac->ES_multi_lingual_flag : 0));
	pos += WriteVALUE(0, buff, pos, (BYTE)(ac ? ac->main_component_flag : 0));
	pos += WriteVALUE(0, buff, pos, (BYTE)(ac ? ac->sampling_rate : 0));
	pos += WriteVALUE(0, buff, pos, ac ? ac->text_char : wstring(), true);
	pos += WriteVALUE(0, buff, pos, (DWORD)(val.hasContentInfo ? val.contentInfo.nibbleList.size() : 0));
	for( size_t i = 0; i < (val.hasContentInfo ? val.contentInfo.nibbleList.size() : 0); i++ ){
		EPGDB_CONTENT_DATA data = val.contentInfo.nibbleList[i];
		pos += WriteVALUE(0, buff, pos, data.content_nibble_level_1);
		pos += WriteVALUE(0, buff, pos, data.content_nibble_level_2);
		pos += WriteVALUE(0, buff, pos, data.user_nibble_1);
		pos += WriteVALUE(0, buff, pos, data.user_nibble_2);
	}
	pos += WriteVALUE(0, buff, pos, (DWORD)(val.eventRelayInfoGroupType ? val.eventRelayInfo.eventDataList.size() : 0));
	for( size_t i = 0; i < (val.eventRelayInfoGroupType ? val.eventRelayInfo.eventDataList.size() : 0); i++ ){
		EPGDB_EVENT_DATA data = val.eventRelayInfo.eventDataList[i];
		pos += WriteVALUE(0, buff, pos, (DWORD)data.original_network_id);
		pos += WriteVALUE(0, buff, pos, (DWORD)data.transport_stream_id);
		pos += WriteVALUE(0, buff, pos, (DWORD)data.service_id);
		pos += WriteVALUE(0, buff, pos, (DWORD)data.event_id);
	}
	pos += WriteVALUE(0, buff, pos, (DWORD)(val.eventGroupInfoGroupType ? val.eventGroupInfo.eventDataList.size() : 0));
	for( size_t i = 0; i < (val.eventGroupInfoGroupType ? val.eventGroupInfo.eventDataList.size() : 0); i++ ){
		EPGDB_EVENT_DATA data = val.eventGroupInfo.eventDataList[i];
		pos += WriteVALUE(0, buff, pos, (DWORD)data.original_network_id);
		pos += WriteVALUE(0, buff, pos, (DWORD)data.transport_stream_id);
		pos += WriteVALUE(0, buff, pos, (DWORD)data.service_id);
		pos += WriteVALUE(0, buff, pos, (DWORD)data.event_id);
	}
	if( buff == NULL ){
		return DeprecatedNewWriteVALUE(val, writeSize, std::unique_ptr<BYTE[]>(new BYTE[pos]));
	}
	writeSize = pos;
	return std::move(buff_);
}
