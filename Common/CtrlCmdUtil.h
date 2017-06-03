#ifndef __CTRL_CMD_UTIL_H__
#define __CTRL_CMD_UTIL_H__

#include "StructDef.h"

namespace CtrlCmdUtilImpl_
{

template<class T> DWORD CCUTIL_WriteVectorVALUE_( WORD ver, BYTE* buff, DWORD buffOffset, const vector<T>& val );
template<class T> DWORD CCUTIL_WritePtrVectorVALUE_( WORD ver, BYTE* buff, DWORD buffOffset, const vector<T>& val );
template<class T> BOOL CCUTIL_ReadVectorVALUE_( WORD ver, vector<T>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

#define CCUTIL_BASETYPE_WRITE_			{ (void)ver; if( buff != NULL ) memcpy(buff + buffOffset, &val, sizeof(val)); return sizeof(val); }
#define CCUTIL_BASETYPE_READ_			{ (void)ver; if( buffSize < sizeof(*val) ) return FALSE; memcpy(val, buff, sizeof(*val)); *readSize = sizeof(*val); return TRUE; }
#define CCUTIL_VECTOR_WRITE_			return CCUTIL_WriteVectorVALUE_(ver, buff, buffOffset, val)
#define CCUTIL_VECTOR_WRITE_PTR_		return CCUTIL_WritePtrVectorVALUE_(ver, buff, buffOffset, val)
#define CCUTIL_VECTOR_READ_				return CCUTIL_ReadVectorVALUE_(ver, val, buff, buffSize, readSize)

////////////////////////////////////////////////////////////////////////////////////////////
//コマンド送信用バイナリ作成関数

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, char val ){ CCUTIL_BASETYPE_WRITE_; }
inline BOOL ReadVALUE( WORD ver, char* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_BASETYPE_READ_; }
inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, unsigned char val ){ CCUTIL_BASETYPE_WRITE_; }
inline BOOL ReadVALUE( WORD ver, unsigned char* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_BASETYPE_READ_; }

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, short val ){ CCUTIL_BASETYPE_WRITE_; }
inline BOOL ReadVALUE( WORD ver, short* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_BASETYPE_READ_; }
inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, unsigned short val ){ CCUTIL_BASETYPE_WRITE_; }
inline BOOL ReadVALUE( WORD ver, unsigned short* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_BASETYPE_READ_; }

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, int val ){ CCUTIL_BASETYPE_WRITE_; }
inline BOOL ReadVALUE( WORD ver, int* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_BASETYPE_READ_; }
inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, unsigned int val ){ CCUTIL_BASETYPE_WRITE_; }
inline BOOL ReadVALUE( WORD ver, unsigned int* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_BASETYPE_READ_; }

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, long val ){ CCUTIL_BASETYPE_WRITE_; }
inline BOOL ReadVALUE( WORD ver, long* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_BASETYPE_READ_; }
inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, unsigned long val ){ CCUTIL_BASETYPE_WRITE_; }
inline BOOL ReadVALUE( WORD ver, unsigned long* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_BASETYPE_READ_; }

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, __int64 val ){ CCUTIL_BASETYPE_WRITE_; }
inline BOOL ReadVALUE( WORD ver, __int64* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_BASETYPE_READ_; }
inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, unsigned __int64 val ){ CCUTIL_BASETYPE_WRITE_; }
inline BOOL ReadVALUE( WORD ver, unsigned __int64* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_BASETYPE_READ_; }

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<unsigned short>& val ){ CCUTIL_VECTOR_WRITE_; }
inline BOOL ReadVALUE( WORD ver, vector<unsigned short>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<unsigned long>& val ){ CCUTIL_VECTOR_WRITE_; }
inline BOOL ReadVALUE( WORD ver, vector<unsigned long>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<__int64>& val ){ CCUTIL_VECTOR_WRITE_; }
inline BOOL ReadVALUE( WORD ver, vector<__int64>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const wstring& val, bool oldFormat = false );
BOOL ReadVALUE( WORD ver, wstring* val, const BYTE* buff, DWORD buffSize, DWORD* readSize, bool oldFormat = false );

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<wstring>& val ){ CCUTIL_VECTOR_WRITE_; }
inline BOOL ReadVALUE( WORD ver, vector<wstring>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const SYSTEMTIME& val ){ CCUTIL_BASETYPE_WRITE_; }
inline BOOL ReadVALUE( WORD ver, SYSTEMTIME* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_BASETYPE_READ_; }

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const FILE_DATA& val );
inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<FILE_DATA>& val ){ CCUTIL_VECTOR_WRITE_; }

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const REC_SETTING_DATA& val );
BOOL ReadVALUE( WORD ver, REC_SETTING_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const RESERVE_DATA& val );
BOOL ReadVALUE( WORD ver, RESERVE_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<RESERVE_DATA>& val ){ CCUTIL_VECTOR_WRITE_; }
inline BOOL ReadVALUE( WORD ver, vector<RESERVE_DATA>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_SERVICE_INFO& val );
BOOL ReadVALUE( WORD ver, EPGDB_SERVICE_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<EPGDB_SERVICE_INFO>& val ){ CCUTIL_VECTOR_WRITE_; }
inline BOOL ReadVALUE( WORD ver, vector<EPGDB_SERVICE_INFO>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_SHORT_EVENT_INFO& val );
BOOL ReadVALUE( WORD ver, EPGDB_SHORT_EVENT_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_EXTENDED_EVENT_INFO& val );
BOOL ReadVALUE( WORD ver, EPGDB_EXTENDED_EVENT_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_CONTENT_DATA& val );
BOOL ReadVALUE( WORD ver, EPGDB_CONTENT_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<EPGDB_CONTENT_DATA>& val ){ CCUTIL_VECTOR_WRITE_; }
inline BOOL ReadVALUE( WORD ver, vector<EPGDB_CONTENT_DATA>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_CONTEN_INFO& val );
BOOL ReadVALUE( WORD ver, EPGDB_CONTEN_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_COMPONENT_INFO& val );
BOOL ReadVALUE( WORD ver, EPGDB_COMPONENT_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_AUDIO_COMPONENT_INFO_DATA& val );
BOOL ReadVALUE( WORD ver, EPGDB_AUDIO_COMPONENT_INFO_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<EPGDB_AUDIO_COMPONENT_INFO_DATA>& val ){ CCUTIL_VECTOR_WRITE_; }
inline BOOL ReadVALUE( WORD ver, vector<EPGDB_AUDIO_COMPONENT_INFO_DATA>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_AUDIO_COMPONENT_INFO& val );
BOOL ReadVALUE( WORD ver, EPGDB_AUDIO_COMPONENT_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_EVENT_DATA& val );
BOOL ReadVALUE( WORD ver, EPGDB_EVENT_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<EPGDB_EVENT_DATA>& val ){ CCUTIL_VECTOR_WRITE_; }
inline BOOL ReadVALUE( WORD ver, vector<EPGDB_EVENT_DATA>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_EVENTGROUP_INFO& val );
BOOL ReadVALUE( WORD ver, EPGDB_EVENTGROUP_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_EVENT_INFO& val );
BOOL ReadVALUE( WORD ver, EPGDB_EVENT_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<EPGDB_EVENT_INFO>& val ){ CCUTIL_VECTOR_WRITE_; }
inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<const EPGDB_EVENT_INFO*>& val ){ CCUTIL_VECTOR_WRITE_PTR_; }
inline BOOL ReadVALUE( WORD ver, vector<EPGDB_EVENT_INFO>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_SEARCH_DATE_INFO& val );
BOOL ReadVALUE( WORD ver, EPGDB_SEARCH_DATE_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<EPGDB_SEARCH_DATE_INFO>& val ){ CCUTIL_VECTOR_WRITE_; }
inline BOOL ReadVALUE( WORD ver, vector<EPGDB_SEARCH_DATE_INFO>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_SEARCH_KEY_INFO& val );
BOOL ReadVALUE( WORD ver, EPGDB_SEARCH_KEY_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<EPGDB_SEARCH_KEY_INFO>& val ){ CCUTIL_VECTOR_WRITE_; }
inline BOOL ReadVALUE( WORD ver, vector<EPGDB_SEARCH_KEY_INFO>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const SET_CH_INFO& val );
BOOL ReadVALUE( WORD ver, SET_CH_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<SET_CH_INFO>& val ){ CCUTIL_VECTOR_WRITE_; }
inline BOOL ReadVALUE( WORD ver, vector<SET_CH_INFO>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const SET_CTRL_MODE& val );
BOOL ReadVALUE( WORD ver, SET_CTRL_MODE* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const REC_FILE_SET_INFO& val );
BOOL ReadVALUE( WORD ver, REC_FILE_SET_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<REC_FILE_SET_INFO>& val ){ CCUTIL_VECTOR_WRITE_; }
inline BOOL ReadVALUE( WORD ver, vector<REC_FILE_SET_INFO>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const SET_CTRL_REC_PARAM& val );
BOOL ReadVALUE( WORD ver, SET_CTRL_REC_PARAM* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const SET_CTRL_REC_STOP_PARAM& val );
BOOL ReadVALUE( WORD ver, SET_CTRL_REC_STOP_PARAM* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const SET_CTRL_REC_STOP_RES_PARAM& val );
BOOL ReadVALUE( WORD ver, SET_CTRL_REC_STOP_RES_PARAM* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const REC_FILE_INFO& val );
BOOL ReadVALUE( WORD ver, REC_FILE_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<REC_FILE_INFO>& val ){ CCUTIL_VECTOR_WRITE_; }
inline BOOL ReadVALUE( WORD ver, vector<REC_FILE_INFO>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPG_AUTO_ADD_DATA& val );
BOOL ReadVALUE( WORD ver, EPG_AUTO_ADD_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<EPG_AUTO_ADD_DATA>& val ){ CCUTIL_VECTOR_WRITE_; }
inline BOOL ReadVALUE( WORD ver, vector<EPG_AUTO_ADD_DATA>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const SEARCH_EPG_INFO_PARAM& val );
BOOL ReadVALUE( WORD ver, SEARCH_EPG_INFO_PARAM* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const GET_EPG_PF_INFO_PARAM& val );
BOOL ReadVALUE( WORD ver, GET_EPG_PF_INFO_PARAM* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const MANUAL_AUTO_ADD_DATA& val );
BOOL ReadVALUE( WORD ver, MANUAL_AUTO_ADD_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<MANUAL_AUTO_ADD_DATA>& val ){ CCUTIL_VECTOR_WRITE_; }
inline BOOL ReadVALUE( WORD ver, vector<MANUAL_AUTO_ADD_DATA>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const TUNER_RESERVE_INFO& val );
BOOL ReadVALUE( WORD ver, TUNER_RESERVE_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<TUNER_RESERVE_INFO>& val ){ CCUTIL_VECTOR_WRITE_; }
inline BOOL ReadVALUE( WORD ver, vector<TUNER_RESERVE_INFO>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const REGIST_TCP_INFO& val );
BOOL ReadVALUE( WORD ver, REGIST_TCP_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_SERVICE_EVENT_INFO& val );
BOOL ReadVALUE( WORD ver, EPGDB_SERVICE_EVENT_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<const EPGDB_SERVICE_EVENT_INFO*>& val ){ CCUTIL_VECTOR_WRITE_PTR_; }
inline BOOL ReadVALUE( WORD ver, vector<EPGDB_SERVICE_EVENT_INFO>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const TVTEST_CH_CHG_INFO& val );
BOOL ReadVALUE( WORD ver, TVTEST_CH_CHG_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const NWPLAY_PLAY_INFO& val );
BOOL ReadVALUE( WORD ver, NWPLAY_PLAY_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const NWPLAY_POS_CMD& val );
BOOL ReadVALUE( WORD ver, NWPLAY_POS_CMD* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const TVTEST_STREAMING_INFO& val );
BOOL ReadVALUE( WORD ver, TVTEST_STREAMING_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const NWPLAY_TIMESHIFT_INFO& val );
BOOL ReadVALUE( WORD ver, NWPLAY_TIMESHIFT_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const NOTIFY_SRV_INFO& val );
BOOL ReadVALUE( WORD ver, NOTIFY_SRV_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

////////////////////////////////////////////////////////////////////////////////////////////
//テンプレート定義

template<class T>
DWORD CCUTIL_WriteVectorVALUE_( WORD ver, BYTE* buff, DWORD buffOffset, const vector<T>& val )
{
	DWORD pos = buffOffset + sizeof(DWORD) * 2;
	//リストの個数
	WriteVALUE(0, buff, buffOffset + sizeof(DWORD), (DWORD)val.size());
	//リストの中身
	for( size_t i = 0; i < val.size(); i++ ){
		pos += WriteVALUE(ver, buff, pos, val[i]);
	}
	//全体のサイズ
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

template<class T>
DWORD CCUTIL_WritePtrVectorVALUE_( WORD ver, BYTE* buff, DWORD buffOffset, const vector<T>& val )
{
	DWORD pos = buffOffset + sizeof(DWORD) * 2;
	//リストの個数
	WriteVALUE(0, buff, buffOffset + sizeof(DWORD), (DWORD)val.size());
	//リストの中身
	for( size_t i = 0; i < val.size(); i++ ){
		if( val[i] == NULL ){
			throw std::runtime_error("");
		}
		pos += WriteVALUE(ver, buff, pos, *val[i]);
	}
	//全体のサイズ
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

template<class T>
BOOL CCUTIL_ReadVectorVALUE_( WORD ver, vector<T>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( buffSize < sizeof(DWORD)*2 ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	DWORD valCount = 0;
	//全体のサイズ
	ReadVALUE(0, &valSize, buff + pos, buffSize - pos, &size);
	pos += size;
	//リストの個数
	ReadVALUE(0, &valCount, buff + pos, buffSize - pos, &size);
	pos += size;
	if( valSize < pos || buffSize < valSize ){
		return FALSE;
	}
	buffSize = valSize;
	val->reserve(val->size() + valCount);

	for( DWORD i=0; i < valCount; i++ ){
		val->resize(val->size() + 1);
		if( ReadVALUE(ver, &val->back(), buff + pos, buffSize - pos, &size) == FALSE ){
			val->pop_back();
			return FALSE;
		}
		pos += size;
	}
	*readSize = valSize;
	return TRUE;
}

}

template<class T>
inline BOOL ReadVALUE( T* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL ){
		return FALSE;
	}
	DWORD readSize_;
	return CtrlCmdUtilImpl_::ReadVALUE(0, val, buff, buffSize, readSize ? readSize : &readSize_);
}

template<class T>
inline BOOL ReadVALUE( T* val, const std::unique_ptr<BYTE[]>& buff, DWORD buffSize, DWORD* readSize )
{
	return ReadVALUE(val, buff.get(), buffSize, readSize);
}

template<class T>
std::unique_ptr<BYTE[]> NewWriteVALUE( const T& val, DWORD& writeSize )
{
	DWORD buffSize = CtrlCmdUtilImpl_::WriteVALUE(0, NULL, 0, val);
	std::unique_ptr<BYTE[]> buff(new BYTE[buffSize]);
	CtrlCmdUtilImpl_::WriteVALUE(0, buff.get(), 0, val);
	writeSize = buffSize;
	return buff;
}

template<class T>
inline BOOL ReadVALUE2( WORD ver, T* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL ){
		return FALSE;
	}
	//2未満のコマンドバージョンは2として扱う
	ver = max(ver, 2);
	DWORD readSize_;
	return CtrlCmdUtilImpl_::ReadVALUE(ver, val, buff, buffSize, readSize ? readSize : &readSize_);
}

template<class T>
std::unique_ptr<BYTE[]> NewWriteVALUE2WithVersion( WORD ver, const T& val, DWORD& writeSize )
{
	//2未満のコマンドバージョンは2として扱う
	ver = max(ver, 2);
	DWORD buffSize = CtrlCmdUtilImpl_::WriteVALUE(0, NULL, 0, ver) + CtrlCmdUtilImpl_::WriteVALUE(ver, NULL, 0, val);
	std::unique_ptr<BYTE[]> buff(new BYTE[buffSize]);
	CtrlCmdUtilImpl_::WriteVALUE(ver, buff.get(), CtrlCmdUtilImpl_::WriteVALUE(0, buff.get(), 0, ver), val);
	writeSize = buffSize;
	return buff;
}

////////////////////////////////////////////////////////////////////////////////////////////
//旧バージョンコマンド送信用バイナリ作成関数
std::unique_ptr<BYTE[]> DeprecatedNewWriteVALUE( const RESERVE_DATA& val, DWORD& writeSize, std::unique_ptr<BYTE[]>&& buff_ = NULL );
BOOL DeprecatedReadVALUE( RESERVE_DATA* val, const std::unique_ptr<BYTE[]>& buff_, DWORD buffSize );
BOOL DeprecatedReadVALUE( EPG_AUTO_ADD_DATA* val, const std::unique_ptr<BYTE[]>& buff_, DWORD buffSize );
std::unique_ptr<BYTE[]> DeprecatedNewWriteVALUE( const EPGDB_EVENT_INFO& val, DWORD& writeSize, std::unique_ptr<BYTE[]>&& buff_ = NULL );

#endif
