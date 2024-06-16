#include "stdafx.h"
#include "CtrlCmdUtil.h"
#include <math.h>
#include <limits>

#if defined(_MSC_VER) && _MSC_VER < 1900
#include <float.h>
#define isnan _isnan
#define isfinite _finite
#endif

namespace CtrlCmdUtilImpl_
{

DWORD FloatToDWORD(float f)
{
	//仮数部23bitの単精度浮動小数点数
	//丸め誤差の扱いがラフなので処理系のfloatと厳密には一致しない
	if( isnan(f) ){
		return 0x7FC00000;
	}
	int ex;
	if( !isfinite(f) || (ex = (int)(logl(fabsf(f)) / logl(2) + 127)) > 254 ){
		return f < 0 ? 0xFF800000 : 0x7F800000;
	}
	if( ex < 1 ){
		return 0;
	}
	int frac = (int)((fabsf(f) * powl(2, 127 - ex) - 1) * 0x800000);
	return (f < 0 ? 0x80000000 : 0) | ex << 23 | min(max(frac, 0), 0x7FFFFF);
}

float DWORDToFloat(DWORD n)
{
	if( (n & 0x7FFFFFFF) == 0x7F800000 ){
		return std::numeric_limits<float>::infinity() * (n < 0x80000000 ? 1 : -1);
	}
	int ex = n >> 23 & 0xFF;
	if( ex > 254 ){
		return std::numeric_limits<float>::quiet_NaN();
	}
	if( ex < 1 ){
		return 0;
	}
	int frac = n & 0x7FFFFF;
	return (float)(((double)frac / 0x800000 + 1) * expl((ex - 127) * logl(2)) * (n < 0x80000000 ? 1 : -1));
}

const BYTE* ReadStructIntro( const BYTE** buff, const BYTE** buffEnd )
{
	const BYTE* rb = *buff;
	DWORD valSize;
	if( !ReadVALUE(0, &rb, *buffEnd, &valSize) ||
	    valSize < sizeof(DWORD) ||
	    valSize - sizeof(DWORD) > (size_t)(*buffEnd - rb) ){
		return NULL;
	}
	*buff = *buffEnd = rb + (valSize - sizeof(DWORD));
	return rb;
}

const BYTE* ReadVectorIntro( const BYTE** buff, const BYTE** buffEnd, DWORD* valCount )
{
	const BYTE* rb = *buff;
	DWORD valSize;
	if( !ReadVALUE(0, &rb, *buffEnd, &valSize) ||
	    !ReadVALUE(0, &rb, *buffEnd, valCount) ||
	    valSize < sizeof(DWORD) * 2 ||
	    valSize - sizeof(DWORD) * 2 > (size_t)(*buffEnd - rb) ){
		return NULL;
	}
	*buff = *buffEnd = rb + (valSize - sizeof(DWORD) * 2);
	return rb;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, DWORD val )
{
	WriteVALUE(ver, buff, buffOffset, (WORD)val);
	WriteVALUE(ver, buff, buffOffset + 2, (WORD)(val >> 16));
	return 4;
}

bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, DWORD* val )
{
	(void)ver;
	if( buffEnd - *buff < 4 ){
		return false;
	}
	*val = **buff | (*buff)[1] << 8 | (*buff)[2] << 16 | (DWORD)(*buff)[3] << 24;
	*buff += 4;
	return true;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, ULONGLONG val )
{
	WriteVALUE(ver, buff, buffOffset, (DWORD)val);
	WriteVALUE(ver, buff, buffOffset + 4, (DWORD)(val >> 32));
	return 8;
}

bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, ULONGLONG* val )
{
	(void)ver;
	if( buffEnd - *buff < 8 ){
		return false;
	}
	*val = **buff | (*buff)[1] << 8 | (*buff)[2] << 16 | (DWORD)(*buff)[3] << 24 |
	       (ULONGLONG)((*buff)[4] | (*buff)[5] << 8 | (*buff)[6] << 16 | (DWORD)(*buff)[7] << 24) << 32;
	*buff += 8;
	return true;
}

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
			WORD w;
			if( 0x10000 <= val[i] && val[i] < 0x110000 ){
				w = (WORD)((val[i] - 0x10000) / 0x400 + 0xD800);
				buff[pos++] = (BYTE)w;
				buff[pos++] = (BYTE)(w >> 8);
				w = (WORD)((val[i] - 0x10000) % 0x400 + 0xDC00);
			}else{
				w = (WORD)val[i];
			}
			buff[pos++] = (BYTE)w;
			buff[pos++] = (BYTE)(w >> 8);
		}
#elif defined(_WIN32)
		std::copy((const BYTE*)val.c_str(), (const BYTE*)(val.c_str() + val.size() + 1), buff + pos);
#else
		for( size_t i = 0; i < val.size() + 1; i++ ){
			buff[pos++] = (BYTE)val[i];
			buff[pos++] = (BYTE)(val[i] >> 8);
		}
#endif
	}
	return size;
}

bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, wstring* val, bool oldFormat )
{
	(void)ver;
	const BYTE* rb = *buff;
	DWORD valSize;
	//全体のサイズ
	if( !ReadVALUE(0, &rb, buffEnd, &valSize) ){
		return false;
	}
	if( oldFormat ){
		//旧形式はサイズフィールド自身のサイズを含まない
		valSize += sizeof(DWORD);
	}
	if( valSize < sizeof(DWORD) ||
	    valSize - sizeof(DWORD) > (size_t)(buffEnd - rb) ){
		return false;
	}
	*buff = buffEnd = rb + (valSize - sizeof(DWORD));

	val->clear();
	if( valSize - sizeof(DWORD) > sizeof(WORD) ){
		val->reserve((valSize - sizeof(DWORD)) / sizeof(WORD) - 1);
	}
	while( rb < buffEnd - 1 && (rb[0] || rb[1]) ){
		WORD w = rb[0] | rb[1] << 8;
		rb += 2;
#if WCHAR_MAX > 0xFFFF
		if( 0xD800 <= w && w < 0xDC00 && rb < buffEnd - 1 && (rb[0] || rb[1]) ){
			WORD x = rb[0] | rb[1] << 8;
			rb += 2;
			val->push_back(0x10000 + (w - 0xD800) * 0x400 + (x - 0xDC00));
			continue;
		}
#endif
		val->push_back(w);
	}
	return true;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const SYSTEMTIME& val )
{
	DWORD pos = buffOffset;
	pos += WriteVALUE(ver, buff, pos, val.wYear);
	pos += WriteVALUE(ver, buff, pos, val.wMonth);
	pos += WriteVALUE(ver, buff, pos, val.wDayOfWeek);
	pos += WriteVALUE(ver, buff, pos, val.wDay);
	pos += WriteVALUE(ver, buff, pos, val.wHour);
	pos += WriteVALUE(ver, buff, pos, val.wMinute);
	pos += WriteVALUE(ver, buff, pos, val.wSecond);
	pos += WriteVALUE(ver, buff, pos, val.wMilliseconds);
	return pos - buffOffset;
}

bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, SYSTEMTIME* val )
{
	if( !ReadVALUE(ver, buff, buffEnd, &val->wYear) ||
	    !ReadVALUE(ver, buff, buffEnd, &val->wMonth) ||
	    !ReadVALUE(ver, buff, buffEnd, &val->wDayOfWeek) ||
	    !ReadVALUE(ver, buff, buffEnd, &val->wDay) ||
	    !ReadVALUE(ver, buff, buffEnd, &val->wHour) ||
	    !ReadVALUE(ver, buff, buffEnd, &val->wMinute) ||
	    !ReadVALUE(ver, buff, buffEnd, &val->wSecond) ||
	    !ReadVALUE(ver, buff, buffEnd, &val->wMilliseconds) ){
		return false;
	}
	return true;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const FILE_DATA& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, val.Name);
	pos += WriteVALUE(ver, buff, pos, (DWORD)val.Data.size());
	pos += WriteVALUE(ver, buff, pos, (DWORD)0);
	if( (DWORD)val.Data.size() != 0 ){
		if( buff != NULL ){
			std::copy(val.Data.begin(), val.Data.begin() + (DWORD)val.Data.size(), buff + pos);
		}
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

bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, REC_SETTING_DATA* val )
{
	const BYTE* rb = ReadStructIntro(buff, &buffEnd);
	if( rb == NULL ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->recMode) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->priority) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->tuijyuuFlag) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->serviceMode) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->pittariFlag) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->batFilePath) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->recFolderList) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->suspendMode) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->rebootFlag) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->useMargineFlag) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->startMargine) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->endMargine) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->continueRecFlag) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->partialRecFlag) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->tunerID) ){
		return false;
	}
	if( ver >= 2 ){
		if( !ReadVALUE(ver, &rb, buffEnd, &val->partialRecFolder) ){
			return false;
		}
	}
	return true;
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

bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, RESERVE_DATA* val )
{
	BYTE bPadding;
	wstring strPadding;
	const BYTE* rb = ReadStructIntro(buff, &buffEnd);
	if( rb == NULL ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->title) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->startTime) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->durationSecond) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->stationName) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->originalNetworkID) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->transportStreamID) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->serviceID) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->eventID) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->comment) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->reserveID) ||
	    !ReadVALUE(ver, &rb, buffEnd, &bPadding) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->overlapMode) ||
	    !ReadVALUE(ver, &rb, buffEnd, &strPadding) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->startTimeEpg) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->recSetting) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->reserveStatus) ){
		return false;
	}
	if( ver >= 5 ){
		DWORD dwPadding;
		if( !ReadVALUE(ver, &rb, buffEnd, &val->recFileNameList) ||
		    !ReadVALUE(ver, &rb, buffEnd, &dwPadding) ){
			return false;
		}
	}
	return true;
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

bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, EPGDB_SERVICE_INFO* val )
{
	const BYTE* rb = ReadStructIntro(buff, &buffEnd);
	if( rb == NULL ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->ONID) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->TSID) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->SID) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->service_type) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->partialReceptionFlag) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->service_provider_name) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->service_name) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->network_name) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->ts_name) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->remote_control_key_id) ){
		return false;
	}
	return true;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_SHORT_EVENT_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, val.event_name);
	pos += WriteVALUE(ver, buff, pos, val.text_char);
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, EPGDB_SHORT_EVENT_INFO* val )
{
	const BYTE* rb = ReadStructIntro(buff, &buffEnd);
	if( rb == NULL ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->event_name) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->text_char) ){
		return false;
	}
	return true;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_EXTENDED_EVENT_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, val.text_char);
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, EPGDB_EXTENDED_EVENT_INFO* val )
{
	const BYTE* rb = ReadStructIntro(buff, &buffEnd);
	if( rb == NULL ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->text_char) ){
		return false;
	}
	return true;
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

bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, EPGDB_CONTENT_DATA* val )
{
	const BYTE* rb = ReadStructIntro(buff, &buffEnd);
	if( rb == NULL ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->content_nibble_level_1) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->content_nibble_level_2) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->user_nibble_1) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->user_nibble_2) ){
		return false;
	}
	return true;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_CONTEN_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, val.nibbleList);
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, EPGDB_CONTEN_INFO* val )
{
	const BYTE* rb = ReadStructIntro(buff, &buffEnd);
	if( rb == NULL ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->nibbleList) ){
		return false;
	}
	return true;
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

bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, EPGDB_COMPONENT_INFO* val )
{
	const BYTE* rb = ReadStructIntro(buff, &buffEnd);
	if( rb == NULL ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->stream_content) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->component_type) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->component_tag) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->text_char) ){
		return false;
	}
	return true;
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

bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, EPGDB_AUDIO_COMPONENT_INFO_DATA* val )
{
	const BYTE* rb = ReadStructIntro(buff, &buffEnd);
	if( rb == NULL ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->stream_content) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->component_type) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->component_tag) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->stream_type) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->simulcast_group_tag) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->ES_multi_lingual_flag) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->main_component_flag) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->quality_indicator) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->sampling_rate) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->text_char) ){
		return false;
	}
	return true;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_AUDIO_COMPONENT_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, val.componentList);
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, EPGDB_AUDIO_COMPONENT_INFO* val )
{
	const BYTE* rb = ReadStructIntro(buff, &buffEnd);
	if( rb == NULL ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->componentList) ){
		return false;
	}
	return true;
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

bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, EPGDB_EVENT_DATA* val )
{
	const BYTE* rb = ReadStructIntro(buff, &buffEnd);
	if( rb == NULL ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->original_network_id) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->transport_stream_id) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->service_id) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->event_id) ){
		return false;
	}
	return true;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_EVENTGROUP_INFO& val, BYTE groupType )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, groupType);
	pos += WriteVALUE(ver, buff, pos, val.eventDataList);
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, EPGDB_EVENTGROUP_INFO* val, BYTE* groupType )
{
	const BYTE* rb = ReadStructIntro(buff, &buffEnd);
	if( rb == NULL ||
	    !ReadVALUE(ver, &rb, buffEnd, groupType) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->eventDataList) ){
		return false;
	}
	return true;
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

bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, EPGDB_EVENT_INFO* val )
{
	const BYTE* rb = ReadStructIntro(buff, &buffEnd);
	if( rb == NULL ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->original_network_id) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->transport_stream_id) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->service_id) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->event_id) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->StartTimeFlag) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->start_time) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->DurationFlag) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->durationSec) ){
		return false;
	}

	DWORD infoValSize;
	val->hasShortInfo = true;
	const BYTE* rbSave = rb;
	if( !ReadVALUE(ver, &rb, buffEnd, &val->shortInfo) ){
		val->hasShortInfo = false;
		rb = rbSave;
		if( !ReadVALUE(0, &rb, buffEnd, &infoValSize) || infoValSize != sizeof(DWORD) ){
			return false;
		}
	}
	val->hasExtInfo = true;
	rbSave = rb;
	if( !ReadVALUE(ver, &rb, buffEnd, &val->extInfo) ){
		val->hasExtInfo = false;
		rb = rbSave;
		if( !ReadVALUE(0, &rb, buffEnd, &infoValSize) || infoValSize != sizeof(DWORD) ){
			return false;
		}
	}
	val->hasContentInfo = true;
	rbSave = rb;
	val->contentInfo.nibbleList.clear();
	if( !ReadVALUE(ver, &rb, buffEnd, &val->contentInfo) ){
		val->hasContentInfo = false;
		rb = rbSave;
		if( !ReadVALUE(0, &rb, buffEnd, &infoValSize) || infoValSize != sizeof(DWORD) ){
			return false;
		}
	}
	val->hasComponentInfo = true;
	rbSave = rb;
	if( !ReadVALUE(ver, &rb, buffEnd, &val->componentInfo) ){
		val->hasComponentInfo = false;
		rb = rbSave;
		if( !ReadVALUE(0, &rb, buffEnd, &infoValSize) || infoValSize != sizeof(DWORD) ){
			return false;
		}
	}
	val->hasAudioInfo = true;
	rbSave = rb;
	val->audioInfo.componentList.clear();
	if( !ReadVALUE(ver, &rb, buffEnd, &val->audioInfo) ){
		val->hasAudioInfo = false;
		rb = rbSave;
		if( !ReadVALUE(0, &rb, buffEnd, &infoValSize) || infoValSize != sizeof(DWORD) ){
			return false;
		}
	}
	val->eventGroupInfo.eventDataList.clear();
	rbSave = rb;
	if( !ReadVALUE(ver, &rb, buffEnd, &val->eventGroupInfo, &val->eventGroupInfoGroupType) ){
		val->eventGroupInfoGroupType = 0;
		rb = rbSave;
		if( !ReadVALUE(0, &rb, buffEnd, &infoValSize) || infoValSize != sizeof(DWORD) ){
			return false;
		}
	}
	val->eventRelayInfo.eventDataList.clear();
	rbSave = rb;
	if( !ReadVALUE(ver, &rb, buffEnd, &val->eventRelayInfo, &val->eventRelayInfoGroupType) ){
		val->eventRelayInfoGroupType = 0;
		rb = rbSave;
		if( !ReadVALUE(0, &rb, buffEnd, &infoValSize) || infoValSize != sizeof(DWORD) ){
			return false;
		}
	}

	if( !ReadVALUE(ver, &rb, buffEnd, &val->freeCAFlag) ){
		return false;
	}
	return true;
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

bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, EPGDB_SEARCH_DATE_INFO* val )
{
	const BYTE* rb = ReadStructIntro(buff, &buffEnd);
	if( rb == NULL ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->startDayOfWeek) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->startHour) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->startMin) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->endDayOfWeek) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->endHour) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->endMin) ){
		return false;
	}
	return true;
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

bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, EPGDB_SEARCH_KEY_INFO* val )
{
	const BYTE* rb = ReadStructIntro(buff, &buffEnd);
	if( rb == NULL ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->andKey) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->notKey) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->regExpFlag) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->titleOnlyFlag) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->contentList) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->dateList) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->serviceList) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->videoList) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->audioList) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->aimaiFlag) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->notContetFlag) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->notDateFlag) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->freeCAFlag) ){
		return false;
	}
	val->chkRecEnd = 0;
	val->chkRecDay = 6;
	if( ver >= 3 ){
		if( !ReadVALUE(ver, &rb, buffEnd, &val->chkRecEnd) ||
		    !ReadVALUE(ver, &rb, buffEnd, &val->chkRecDay) ){
			return false;
		}
	}
	if( ver >= 5 ){
		if( buffEnd - rb >= 5 ){
			//録画済チェックに関する追加のフィールドがある
			BYTE recNoService;
			WORD durMin;
			WORD durMax;
			if( !ReadVALUE(ver, &rb, buffEnd, &recNoService) ||
			    !ReadVALUE(ver, &rb, buffEnd, &durMin) ||
			    !ReadVALUE(ver, &rb, buffEnd, &durMax) ){
				return false;
			}
			if( recNoService ){
				val->chkRecDay = val->chkRecDay % 10000 + 40000;
			}
			if( durMin > 0 || durMax > 0 ){
				WCHAR dur[32];
				swprintf_s(dur, L"D!{%d}", (10000 + min(max((int)durMin, 0), 9999)) * 10000 + min(max((int)durMax, 0), 9999));
				size_t durPos = val->andKey.compare(0, 7, L"^!{999}") ? 0 : 7;
				durPos += val->andKey.compare(durPos, 7, L"C!{999}") ? 0 : 7;
				val->andKey.insert(durPos, dur);
			}
		}
	}
	return true;
}

bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, SEARCH_PG_PARAM* val )
{
	const BYTE* rb = ReadStructIntro(buff, &buffEnd);
	if( rb == NULL ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->keyList) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->enumStart) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->enumEnd) ){
		return false;
	}
	return true;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const VIEW_APP_STATUS_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, val.status);
	pos += WriteVALUE(ver, buff, pos, val.delaySec);
	pos += WriteVALUE(ver, buff, pos, val.bonDriver);
	pos += WriteVALUE(ver, buff, pos, val.drop);
	pos += WriteVALUE(ver, buff, pos, val.scramble);
	pos += WriteVALUE(ver, buff, pos, FloatToDWORD(val.signalLv));
	pos += WriteVALUE(ver, buff, pos, val.space);
	pos += WriteVALUE(ver, buff, pos, val.ch);
	pos += WriteVALUE(ver, buff, pos, val.originalNetworkID);
	pos += WriteVALUE(ver, buff, pos, val.transportStreamID);
	pos += WriteVALUE(ver, buff, pos, val.appID);
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, VIEW_APP_STATUS_INFO* val )
{
	const BYTE* rb = ReadStructIntro(buff, &buffEnd);
	DWORD dwSignalLv;
	if( rb == NULL ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->status) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->delaySec) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->bonDriver) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->drop) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->scramble) ||
	    !ReadVALUE(ver, &rb, buffEnd, &dwSignalLv) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->space) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->ch) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->originalNetworkID) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->transportStreamID) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->appID) ){
		return false;
	}
	val->signalLv = DWORDToFloat(dwSignalLv);
	return true;
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

bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, SET_CH_INFO* val )
{
	const BYTE* rb = ReadStructIntro(buff, &buffEnd);
	if( rb == NULL ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->useSID) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->ONID) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->TSID) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->SID) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->useBonCh) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->space) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->ch) ){
		return false;
	}
	return true;
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

bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, SET_CTRL_MODE* val )
{
	const BYTE* rb = ReadStructIntro(buff, &buffEnd);
	if( rb == NULL ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->ctrlID) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->SID) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->enableScramble) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->enableCaption) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->enableData) ){
		return false;
	}
	return true;
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

bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, REC_FILE_SET_INFO* val )
{
	const BYTE* rb = ReadStructIntro(buff, &buffEnd);
	if( rb == NULL ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->recFolder) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->writePlugIn) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->recNamePlugIn) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->recFileName) ){
		return false;
	}
	return true;
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

bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, SET_CTRL_REC_PARAM* val )
{
	const BYTE* rb = ReadStructIntro(buff, &buffEnd);
	if( rb == NULL ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->ctrlID) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->fileName) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->overWriteFlag) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->createSize) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->saveFolder) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->pittariFlag) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->pittariONID) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->pittariTSID) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->pittariSID) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->pittariEventID) ){
		return false;
	}
	return true;
}

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const SET_CTRL_REC_STOP_PARAM& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, val.ctrlID);
	pos += WriteVALUE(ver, buff, pos, val.saveErrLog);
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, SET_CTRL_REC_STOP_PARAM* val )
{
	const BYTE* rb = ReadStructIntro(buff, &buffEnd);
	if( rb == NULL ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->ctrlID) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->saveErrLog) ){
		return false;
	}
	return true;
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

bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, SET_CTRL_REC_STOP_RES_PARAM* val )
{
	const BYTE* rb = ReadStructIntro(buff, &buffEnd);
	if( rb == NULL ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->recFilePath) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->drop) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->scramble) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->subRecFlag) ){
		return false;
	}
	return true;
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

bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, REC_FILE_INFO* val )
{
	wstring strPadding;
	const BYTE* rb = ReadStructIntro(buff, &buffEnd);
	if( rb == NULL ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->id) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->recFilePath) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->title) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->startTime) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->durationSecond) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->serviceName) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->originalNetworkID) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->transportStreamID) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->serviceID) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->eventID) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->drops) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->scrambles) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->recStatus) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->startTimeEpg) ||
	    !ReadVALUE(ver, &rb, buffEnd, &strPadding) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->programInfo) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->errInfo) ){
		return false;
	}
	val->protectFlag = 0;
	if( ver >= 4 ){
		if( !ReadVALUE(ver, &rb, buffEnd, &val->protectFlag) ){
			return false;
		}
	}
	return true;
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

bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, EPG_AUTO_ADD_DATA* val )
{
	const BYTE* rb = ReadStructIntro(buff, &buffEnd);
	if( rb == NULL ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->dataID) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->searchInfo) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->recSetting) ){
		return false;
	}
	if( ver >= 5 ){
		if( !ReadVALUE(ver, &rb, buffEnd, &val->addCount) ){
			return false;
		}
	}
	return true;
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

bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, SEARCH_EPG_INFO_PARAM* val )
{
	const BYTE* rb = ReadStructIntro(buff, &buffEnd);
	if( rb == NULL ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->ONID) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->TSID) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->SID) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->eventID) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->pfOnlyFlag) ){
		return false;
	}
	return true;
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

bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, GET_EPG_PF_INFO_PARAM* val )
{
	const BYTE* rb = ReadStructIntro(buff, &buffEnd);
	if( rb == NULL ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->ONID) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->TSID) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->SID) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->pfNextFlag) ){
		return false;
	}
	return true;
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

bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, MANUAL_AUTO_ADD_DATA* val )
{
	const BYTE* rb = ReadStructIntro(buff, &buffEnd);
	if( rb == NULL ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->dataID) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->dayOfWeekFlag) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->startTime) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->durationSecond) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->title) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->stationName) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->originalNetworkID) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->transportStreamID) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->serviceID) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->recSetting) ){
		return false;
	}
	return true;
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

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const TUNER_PROCESS_STATUS_INFO& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	pos += WriteVALUE(ver, buff, pos, val.tunerID);
	pos += WriteVALUE(ver, buff, pos, val.processID);
	pos += WriteVALUE(ver, buff, pos, val.drop);
	pos += WriteVALUE(ver, buff, pos, val.scramble);
	pos += WriteVALUE(ver, buff, pos, FloatToDWORD(val.signalLv));
	pos += WriteVALUE(ver, buff, pos, val.space);
	pos += WriteVALUE(ver, buff, pos, val.ch);
	pos += WriteVALUE(ver, buff, pos, val.originalNetworkID);
	pos += WriteVALUE(ver, buff, pos, val.transportStreamID);
	pos += WriteVALUE(ver, buff, pos, val.recFlag);
	pos += WriteVALUE(ver, buff, pos, val.epgCapFlag);
	pos += WriteVALUE(ver, buff, pos, val.extraFlags);
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

bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, EPGDB_SERVICE_EVENT_INFO* val )
{
	const BYTE* rb = ReadStructIntro(buff, &buffEnd);
	if( rb == NULL ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->serviceInfo) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->eventList) ){
		return false;
	}
	return true;
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

bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, NWPLAY_PLAY_INFO* val )
{
	const BYTE* rb = ReadStructIntro(buff, &buffEnd);
	if( rb == NULL ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->ctrlID) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->ip) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->udp) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->tcp) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->udpPort) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->tcpPort) ){
		return false;
	}
	return true;
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

bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, NWPLAY_POS_CMD* val )
{
	const BYTE* rb = ReadStructIntro(buff, &buffEnd);
	if( rb == NULL ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->ctrlID) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->currentPos) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->totalPos) ){
		return false;
	}
	return true;
}

bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, TVTEST_STREAMING_INFO* val )
{
	const BYTE* rb = ReadStructIntro(buff, &buffEnd);
	if( rb == NULL ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->enableMode) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->ctrlID) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->serverIP) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->serverPort) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->filePath) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->udpSend) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->tcpSend) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->timeShiftMode) ){
		return false;
	}
	return true;
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

bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, NOTIFY_SRV_INFO* val )
{
	const BYTE* rb = ReadStructIntro(buff, &buffEnd);
	if( rb == NULL ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->notifyID) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->time) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->param1) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->param2) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->param3) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->param4) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->param5) ||
	    !ReadVALUE(ver, &rb, buffEnd, &val->param6) ){
		return false;
	}
	return true;
}

}

void CCmdStream::SetParam(DWORD param)
{
	params[0] = param;
	if( buff.size() >= 4 ){
		std::copy((BYTE*)params, (BYTE*)params + 4, buff.data());
	}
}

void CCmdStream::Resize(DWORD dataSize)
{
	if( dataSize ){
		buff.resize(8 + dataSize);
		params[1] = dataSize;
		std::copy((BYTE*)params, (BYTE*)params + 8, buff.data());
	}else{
		buff.clear();
		params[1] = dataSize;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////
//旧バージョンコマンド送信用バイナリ作成関数
void DeprecatedNewWriteVALUE( const RESERVE_DATA& val, CCmdStream& cmd, BYTE* buff )
{
	using namespace CtrlCmdUtilImpl_;
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
		cmd.Resize(pos);
		DeprecatedNewWriteVALUE(val, cmd, cmd.GetData());
	}
}

bool DeprecatedReadVALUE( RESERVE_DATA* val, const BYTE* buff, DWORD buffSize )
{
	using namespace CtrlCmdUtilImpl_;
	if( val == NULL || buff == NULL ){
		return false;
	}
	const BYTE* buffEnd = buff + buffSize;

	DWORD dwRecMode;
	DWORD dwPittariFlag;
	DWORD dwPadding;
	REC_FILE_SET_INFO folder;
	WORD wSuspendMode;
	DWORD dwRebootFlag;
	wstring strPadding;
	if( !ReadVALUE(0, &buff, buffEnd, &val->title, true) ||
	    !ReadVALUE(0, &buff, buffEnd, &val->startTime) ||
	    !ReadVALUE(0, &buff, buffEnd, &val->durationSecond) ||
	    !ReadVALUE(0, &buff, buffEnd, &val->stationName, true) ||
	    !ReadVALUE(0, &buff, buffEnd, &val->originalNetworkID) ||
	    !ReadVALUE(0, &buff, buffEnd, &val->transportStreamID) ||
	    !ReadVALUE(0, &buff, buffEnd, &val->serviceID) ||
	    !ReadVALUE(0, &buff, buffEnd, &val->eventID) ||
	    !ReadVALUE(0, &buff, buffEnd, &val->recSetting.priority) ||
	    !ReadVALUE(0, &buff, buffEnd, &val->recSetting.tuijyuuFlag) ||
	    !ReadVALUE(0, &buff, buffEnd, &val->comment, true) ||
	    !ReadVALUE(0, &buff, buffEnd, &dwRecMode) ||
	    !ReadVALUE(0, &buff, buffEnd, &dwPittariFlag) ||
	    !ReadVALUE(0, &buff, buffEnd, &val->recSetting.batFilePath, true) ||
	    !ReadVALUE(0, &buff, buffEnd, &val->reserveID) ||
	    !ReadVALUE(0, &buff, buffEnd, &dwPadding) ||
	    !ReadVALUE(0, &buff, buffEnd, &dwPadding) ||
	    !ReadVALUE(0, &buff, buffEnd, &folder.recFolder, true) ||
	    !ReadVALUE(0, &buff, buffEnd, &wSuspendMode) ||
	    !ReadVALUE(0, &buff, buffEnd, &dwRebootFlag) ||
	    !ReadVALUE(0, &buff, buffEnd, &strPadding, true) ){
		return false;
	}
	val->recSetting.recMode = dwRecMode & 0xFF;
	val->recSetting.pittariFlag = dwPittariFlag != 0;
	if( folder.recFolder.empty() == false ){
		folder.writePlugIn = L"Write_Default.dll";
		val->recSetting.recFolderList.push_back(folder);
	}
	//旧→新のみなぜかこの数値変換が入る(互換のため修正しない)
	val->recSetting.suspendMode = (wSuspendMode == 0 ? 4 : wSuspendMode == 4 ? 0 : wSuspendMode) & 0xFF;
	val->recSetting.rebootFlag = dwRebootFlag != 0;

	if( buff < buffEnd ){
		DWORD dwUseMargineFlag;
		if( !ReadVALUE(0, &buff, buffEnd, &dwUseMargineFlag) ||
		    !ReadVALUE(0, &buff, buffEnd, &val->recSetting.startMargine) ||
		    !ReadVALUE(0, &buff, buffEnd, &val->recSetting.endMargine) ||
		    !ReadVALUE(0, &buff, buffEnd, &val->recSetting.serviceMode) ){
			return false;
		}
		val->recSetting.useMargineFlag = dwUseMargineFlag != 0;
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
	return true;
}

bool DeprecatedReadVALUE( EPG_AUTO_ADD_DATA* val, const BYTE* buff, DWORD buffSize )
{
	using namespace CtrlCmdUtilImpl_;
	if( val == NULL || buff == NULL ){
		return false;
	}
	const BYTE* buffEnd = buff + buffSize;

	int iJanru;
	if( !ReadVALUE(0, &buff, buffEnd, &val->searchInfo.andKey, true) ||
	    !ReadVALUE(0, &buff, buffEnd, &val->searchInfo.notKey, true) ||
	    !ReadVALUE(0, &buff, buffEnd, &val->searchInfo.titleOnlyFlag) ||
	    !ReadVALUE(0, &buff, buffEnd, &iJanru) ){
		return false;
	}
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
	if( !ReadVALUE(0, &buff, buffEnd, &iSH) ||
	    !ReadVALUE(0, &buff, buffEnd, &iSM) ||
	    !ReadVALUE(0, &buff, buffEnd, &iEH) ||
	    !ReadVALUE(0, &buff, buffEnd, &iEM) ){
		return false;
	}
	val->searchInfo.dateList.clear();
	for( DWORD i = 1; i < 8; i++ ){
		DWORD dwChkDayOfWeek;
		if( !ReadVALUE(0, &buff, buffEnd, &dwChkDayOfWeek) ){
			return false;
		}
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
	if( !ReadVALUE(0, &buff, buffEnd, &dwRead) ){
		return false;
	}
	for( DWORD i = 0; i < dwRead; i++ ){
		LONGLONG item;
		if( !ReadVALUE(0, &buff, buffEnd, &item) ){
			return false;
		}
		val->searchInfo.serviceList.push_back(item);
	}

	DWORD dwPriority;
	DWORD dwTuijyuuFlag;
	DWORD dwRecMode;
	DWORD dwPittariFlag;
	REC_FILE_SET_INFO folder;
	WORD wSuspendMode;
	DWORD dwRebootFlag;
	if( !ReadVALUE(0, &buff, buffEnd, &val->dataID) ||
	    !ReadVALUE(0, &buff, buffEnd, &dwPriority) ||
	    !ReadVALUE(0, &buff, buffEnd, &dwTuijyuuFlag) ||
	    !ReadVALUE(0, &buff, buffEnd, &dwRecMode) ||
	    !ReadVALUE(0, &buff, buffEnd, &dwPittariFlag) ||
	    !ReadVALUE(0, &buff, buffEnd, &val->recSetting.batFilePath, true) ||
	    !ReadVALUE(0, &buff, buffEnd, &folder.recFolder, true) ||
	    !ReadVALUE(0, &buff, buffEnd, &wSuspendMode) ||
	    !ReadVALUE(0, &buff, buffEnd, &dwRebootFlag) ){
		return false;
	}
	val->recSetting.priority = dwPriority & 0xFF;
	val->recSetting.tuijyuuFlag = dwTuijyuuFlag != 0;
	val->recSetting.recMode = dwRecMode & 0xFF;
	val->recSetting.pittariFlag = dwPittariFlag != 0;
	if( folder.recFolder.empty() == false ){
		folder.writePlugIn = L"Write_Default.dll";
		val->recSetting.recFolderList.push_back(folder);
	}
	val->recSetting.suspendMode = (wSuspendMode == 0 ? 4 : wSuspendMode == 4 ? 0 : wSuspendMode) & 0xFF;
	val->recSetting.rebootFlag = dwRebootFlag != 0;

	if( buff < buffEnd ){
		DWORD dwUseMargineFlag;
		if( !ReadVALUE(0, &buff, buffEnd, &dwUseMargineFlag) ||
		    !ReadVALUE(0, &buff, buffEnd, &val->recSetting.startMargine) ||
		    !ReadVALUE(0, &buff, buffEnd, &val->recSetting.endMargine) ||
		    !ReadVALUE(0, &buff, buffEnd, &val->recSetting.serviceMode) ){
			return false;
		}
		val->recSetting.useMargineFlag = dwUseMargineFlag != 0;
	}else{
		val->recSetting.useMargineFlag = 0;
		val->recSetting.startMargine = 0;
		val->recSetting.endMargine = 0;
		val->recSetting.serviceMode = 0;
	}
	if( buff < buffEnd ){
		DWORD dwRegExpFlag;
		wstring strRead;
		if( !ReadVALUE(0, &buff, buffEnd, &dwRegExpFlag) ||
		    !ReadVALUE(0, &buff, buffEnd, &strRead, true) ){
			return false;
		}
		val->searchInfo.regExpFlag = dwRegExpFlag != 0;
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
	return true;
}

void DeprecatedNewWriteVALUE( const EPGDB_EVENT_INFO& val, CCmdStream& cmd, BYTE* buff )
{
	using namespace CtrlCmdUtilImpl_;
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
		cmd.Resize(pos);
		DeprecatedNewWriteVALUE(val, cmd, cmd.GetData());
	}
}
