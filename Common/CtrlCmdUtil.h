#ifndef __CTRL_CMD_UTIL_H__
#define __CTRL_CMD_UTIL_H__

#include "StructDef.h"

namespace CtrlCmdUtilImpl_
{

BOOL CCUTIL_ReadStream_( void* val, DWORD valSize, const BYTE* buff, DWORD buffSize, DWORD* readSize );
template<class T> DWORD CCUTIL_WriteVectorVALUE_( BYTE* buff, DWORD buffOffset, const vector<T>& val );
template<class T> DWORD CCUTIL_WritePtrVectorVALUE_( BYTE* buff, DWORD buffOffset, const vector<T>& val );
template<class T> BOOL CCUTIL_ReadVectorVALUE_( vector<T>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );
template<class T> BOOL CCUTIL_ReadAndNewVectorVALUE_( vector<T*>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

#define CCUTIL_BASETYPE_WRITE_			{ if( buff != NULL ) memcpy(buff + buffOffset, &val, sizeof(val)); return sizeof(val); }
#define CCUTIL_BASETYPE_READ_			return CCUTIL_ReadStream_(val, sizeof(*val), buff, buffSize, readSize)
#define CCUTIL_VECTOR_WRITE_			return CCUTIL_WriteVectorVALUE_(buff, buffOffset, val)
#define CCUTIL_VECTOR_WRITE_PTR_		return CCUTIL_WritePtrVectorVALUE_(buff, buffOffset, val)
#define CCUTIL_VECTOR_READ_				return CCUTIL_ReadVectorVALUE_(val, buff, buffSize, readSize)
#define CCUTIL_VECTOR_READ_AND_NEW_		return CCUTIL_ReadAndNewVectorVALUE_(val, buff, buffSize, readSize)

////////////////////////////////////////////////////////////////////////////////////////////
//コマンド送信用バイナリ作成関数

inline DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, char val ){ CCUTIL_BASETYPE_WRITE_; }
inline BOOL ReadVALUE( char* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_BASETYPE_READ_; }
inline DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, unsigned char val ){ CCUTIL_BASETYPE_WRITE_; }
inline BOOL ReadVALUE( unsigned char* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_BASETYPE_READ_; }

inline DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, short val ){ CCUTIL_BASETYPE_WRITE_; }
inline BOOL ReadVALUE( short* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_BASETYPE_READ_; }
inline DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, unsigned short val ){ CCUTIL_BASETYPE_WRITE_; }
inline BOOL ReadVALUE( unsigned short* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_BASETYPE_READ_; }

inline DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, int val ){ CCUTIL_BASETYPE_WRITE_; }
inline BOOL ReadVALUE( int* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_BASETYPE_READ_; }
inline DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, unsigned int val ){ CCUTIL_BASETYPE_WRITE_; }
inline BOOL ReadVALUE( unsigned int* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_BASETYPE_READ_; }

inline DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, long val ){ CCUTIL_BASETYPE_WRITE_; }
inline BOOL ReadVALUE( long* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_BASETYPE_READ_; }
inline DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, unsigned long val ){ CCUTIL_BASETYPE_WRITE_; }
inline BOOL ReadVALUE( unsigned long* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_BASETYPE_READ_; }

inline DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, __int64 val ){ CCUTIL_BASETYPE_WRITE_; }
inline BOOL ReadVALUE( __int64* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_BASETYPE_READ_; }
inline DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, unsigned __int64 val ){ CCUTIL_BASETYPE_WRITE_; }
inline BOOL ReadVALUE( unsigned __int64* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_BASETYPE_READ_; }

inline DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const vector<unsigned short>& val ){ CCUTIL_VECTOR_WRITE_; }
inline BOOL ReadVALUE( vector<unsigned short>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

inline DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const vector<unsigned long>& val ){ CCUTIL_VECTOR_WRITE_; }
inline BOOL ReadVALUE( vector<unsigned long>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

inline DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const vector<__int64>& val ){ CCUTIL_VECTOR_WRITE_; }
inline BOOL ReadVALUE( vector<__int64>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const wstring& val );
BOOL ReadVALUE( wstring* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const vector<wstring>& val ){ CCUTIL_VECTOR_WRITE_; }
inline BOOL ReadVALUE( vector<wstring>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

inline DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const SYSTEMTIME& val ){ CCUTIL_BASETYPE_WRITE_; }
inline BOOL ReadVALUE( SYSTEMTIME* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_BASETYPE_READ_; }

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const REC_SETTING_DATA& val );
BOOL ReadVALUE( REC_SETTING_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const RESERVE_DATA& val );
BOOL ReadVALUE( RESERVE_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const vector<RESERVE_DATA>& val ){ CCUTIL_VECTOR_WRITE_; }
inline BOOL ReadVALUE( vector<RESERVE_DATA>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const EPGDB_SERVICE_INFO& val );
BOOL ReadVALUE( EPGDB_SERVICE_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const vector<EPGDB_SERVICE_INFO>& val ){ CCUTIL_VECTOR_WRITE_; }
inline BOOL ReadVALUE( vector<EPGDB_SERVICE_INFO>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const EPGDB_SHORT_EVENT_INFO& val );
BOOL ReadVALUE( EPGDB_SHORT_EVENT_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const EPGDB_EXTENDED_EVENT_INFO& val );
BOOL ReadVALUE( EPGDB_EXTENDED_EVENT_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const EPGDB_CONTENT_DATA& val );
BOOL ReadVALUE( EPGDB_CONTENT_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const vector<EPGDB_CONTENT_DATA>& val ){ CCUTIL_VECTOR_WRITE_; }
inline BOOL ReadVALUE( vector<EPGDB_CONTENT_DATA>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const EPGDB_CONTEN_INFO& val );
BOOL ReadVALUE( EPGDB_CONTEN_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const EPGDB_COMPONENT_INFO& val );
BOOL ReadVALUE( EPGDB_COMPONENT_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const EPGDB_AUDIO_COMPONENT_INFO_DATA& val );
BOOL ReadVALUE( EPGDB_AUDIO_COMPONENT_INFO_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const vector<EPGDB_AUDIO_COMPONENT_INFO_DATA>& val ){ CCUTIL_VECTOR_WRITE_; }
inline BOOL ReadVALUE( vector<EPGDB_AUDIO_COMPONENT_INFO_DATA>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const EPGDB_AUDIO_COMPONENT_INFO& val );
BOOL ReadVALUE( EPGDB_AUDIO_COMPONENT_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const EPGDB_EVENT_DATA& val );
BOOL ReadVALUE( EPGDB_EVENT_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const vector<EPGDB_EVENT_DATA>& val ){ CCUTIL_VECTOR_WRITE_; }
inline BOOL ReadVALUE( vector<EPGDB_EVENT_DATA>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const EPGDB_EVENTGROUP_INFO& val );
BOOL ReadVALUE( EPGDB_EVENTGROUP_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const EPGDB_EVENT_INFO& val );
BOOL ReadVALUE( EPGDB_EVENT_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const vector<EPGDB_EVENT_INFO*>& val ){ CCUTIL_VECTOR_WRITE_PTR_; }
inline BOOL ReadVALUE( vector<EPGDB_EVENT_INFO*>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_AND_NEW_; }

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const EPGDB_SEARCH_DATE_INFO& val );
BOOL ReadVALUE( EPGDB_SEARCH_DATE_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const vector<EPGDB_SEARCH_DATE_INFO>& val ){ CCUTIL_VECTOR_WRITE_; }
inline BOOL ReadVALUE( vector<EPGDB_SEARCH_DATE_INFO>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const EPGDB_SEARCH_KEY_INFO& val );
BOOL ReadVALUE( EPGDB_SEARCH_KEY_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const vector<EPGDB_SEARCH_KEY_INFO>& val ){ CCUTIL_VECTOR_WRITE_; }
inline BOOL ReadVALUE( vector<EPGDB_SEARCH_KEY_INFO>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const SET_CH_INFO& val );
BOOL ReadVALUE( SET_CH_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const vector<SET_CH_INFO>& val ){ CCUTIL_VECTOR_WRITE_; }
inline BOOL ReadVALUE( vector<SET_CH_INFO>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const SET_CTRL_MODE& val );
BOOL ReadVALUE( SET_CTRL_MODE* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const REC_FILE_SET_INFO& val );
BOOL ReadVALUE( REC_FILE_SET_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const vector<REC_FILE_SET_INFO>& val ){ CCUTIL_VECTOR_WRITE_; }
inline BOOL ReadVALUE( vector<REC_FILE_SET_INFO>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const SET_CTRL_REC_PARAM& val );
BOOL ReadVALUE( SET_CTRL_REC_PARAM* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const SET_CTRL_REC_STOP_PARAM& val );
BOOL ReadVALUE( SET_CTRL_REC_STOP_PARAM* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const SET_CTRL_REC_STOP_RES_PARAM& val );
BOOL ReadVALUE( SET_CTRL_REC_STOP_RES_PARAM* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const REC_FILE_INFO& val );
BOOL ReadVALUE( REC_FILE_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const vector<REC_FILE_INFO>& val ){ CCUTIL_VECTOR_WRITE_; }
inline BOOL ReadVALUE( vector<REC_FILE_INFO>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const EPG_AUTO_ADD_DATA& val );
BOOL ReadVALUE( EPG_AUTO_ADD_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const vector<EPG_AUTO_ADD_DATA>& val ){ CCUTIL_VECTOR_WRITE_; }
inline BOOL ReadVALUE( vector<EPG_AUTO_ADD_DATA>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const SEARCH_EPG_INFO_PARAM& val );
BOOL ReadVALUE( SEARCH_EPG_INFO_PARAM* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const GET_EPG_PF_INFO_PARAM& val );
BOOL ReadVALUE( GET_EPG_PF_INFO_PARAM* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const MANUAL_AUTO_ADD_DATA& val );
BOOL ReadVALUE( MANUAL_AUTO_ADD_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const vector<MANUAL_AUTO_ADD_DATA>& val ){ CCUTIL_VECTOR_WRITE_; }
inline BOOL ReadVALUE( vector<MANUAL_AUTO_ADD_DATA>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const TUNER_RESERVE_INFO& val );
BOOL ReadVALUE( TUNER_RESERVE_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const vector<TUNER_RESERVE_INFO>& val ){ CCUTIL_VECTOR_WRITE_; }
inline BOOL ReadVALUE( vector<TUNER_RESERVE_INFO>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const REGIST_TCP_INFO& val );
BOOL ReadVALUE( REGIST_TCP_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const EPGDB_SERVICE_EVENT_INFO& val );
BOOL ReadVALUE( EPGDB_SERVICE_EVENT_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const vector<EPGDB_SERVICE_EVENT_INFO>& val ){ CCUTIL_VECTOR_WRITE_; }
inline BOOL ReadVALUE( vector<EPGDB_SERVICE_EVENT_INFO>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const TVTEST_CH_CHG_INFO& val );
BOOL ReadVALUE( TVTEST_CH_CHG_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const NWPLAY_PLAY_INFO& val );
BOOL ReadVALUE( NWPLAY_PLAY_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const NWPLAY_POS_CMD& val );
BOOL ReadVALUE( NWPLAY_POS_CMD* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const TVTEST_STREAMING_INFO& val );
BOOL ReadVALUE( TVTEST_STREAMING_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD WriteVALUE( BYTE* buff, DWORD buffOffset, const NWPLAY_TIMESHIFT_INFO& val );
BOOL ReadVALUE( NWPLAY_TIMESHIFT_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

////////////////////////////////////////////////////////////////////////////////////////////
//テンプレート定義

template<class T>
DWORD CCUTIL_WriteVectorVALUE_( BYTE* buff, DWORD buffOffset, const vector<T>& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	//リストの個数
	pos += WriteVALUE(buff, pos, (DWORD)val.size());
	//リストの中身
	for( size_t i = 0; i < val.size(); i++ ){
		pos += WriteVALUE(buff, pos, val[i]);
	}
	//全体のサイズ
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

template<class T>
DWORD CCUTIL_WritePtrVectorVALUE_( BYTE* buff, DWORD buffOffset, const vector<T>& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	//リストの個数
	pos += WriteVALUE(buff, pos, (DWORD)val.size());
	//リストの中身
	for( size_t i = 0; i < val.size(); i++ ){
		if( val[i] == NULL ){
			throw std::runtime_error("");
		}
		pos += WriteVALUE(buff, pos, *val[i]);
	}
	//全体のサイズ
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

template<class T>
BOOL CCUTIL_ReadVectorVALUE_( vector<T>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL || buffSize < sizeof(DWORD)*2 ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	DWORD valCount = 0;
	//全体のサイズ
	ReadVALUE(&valSize, buff + pos, buffSize - pos, &size);
	pos += size;
	if( buffSize < valSize ){
		return FALSE;
	}
	//リストの個数
	ReadVALUE(&valCount, buff + pos, buffSize - pos, &size);
	pos += size;

	for( DWORD i=0; i < valCount; i++ ){
		T data;
		if( ReadVALUE(&data, buff + pos, buffSize - pos, &size) == FALSE ){
			return FALSE;
		}
		pos += size;
		val->push_back(data);
	}
	if( readSize != NULL ){
		*readSize = pos;
	}
	return TRUE;
}

template<class T>
BOOL CCUTIL_ReadAndNewVectorVALUE_( vector<T*>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL || buffSize < sizeof(DWORD)*2 ){
		return FALSE;
	}

	DWORD pos = 0;
	DWORD size = 0;
	DWORD valSize = 0;
	DWORD valCount = 0;
	//全体のサイズ
	ReadVALUE(&valSize, buff + pos, buffSize - pos, &size);
	pos += size;
	if( buffSize < valSize ){
		return FALSE;
	}
	//リストの個数
	ReadVALUE(&valCount, buff + pos, buffSize - pos, &size);
	pos += size;

	for( DWORD i=0; i < valCount; i++ ){
		T* data = new T;
		if( ReadVALUE(data, buff + pos, buffSize - pos, &size) == FALSE ){
			delete data;
			return FALSE;
		}
		pos += size;
		val->push_back(data);
	}
	if( readSize != NULL ){
		*readSize = pos;
	}
	return TRUE;
}

}

template<class T>
inline BOOL ReadVALUE( T* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	return CtrlCmdUtilImpl_::ReadVALUE(val, buff, buffSize, readSize);
}

template<class T>
BYTE* NewWriteVALUE( const T& val, DWORD& writeSize )
{
	DWORD buffSize = CtrlCmdUtilImpl_::WriteVALUE(NULL, 0, val);
	BYTE* buff = new BYTE[buffSize];
	try{
		CtrlCmdUtilImpl_::WriteVALUE(buff, 0, val);
	}catch( ... ){
		delete[] buff;
		throw;
	}
	writeSize = buffSize;
	return buff;
}

template<class T>
BYTE* NewWriteVALUE( T* val, DWORD& writeSize )
{
	if( val == NULL ){
		return NULL;
	}
	return NewWriteVALUE(*val, writeSize);
}

////////////////////////////////////////////////////////////////////////////////////////////
//旧バージョンコマンド送信用バイナリ作成関数
BOOL CreateReserveDataStream(OLD_RESERVE_DATA* pData, CMD_STREAM* pCmd);
BOOL CopyReserveData(OLD_RESERVE_DATA* pstData, CMD_STREAM* pCmd);

BOOL CreateSearchKeyDataStream(OLD_SEARCH_KEY* pData, CMD_STREAM* pCmd);
BOOL CopySearchKeyData(OLD_SEARCH_KEY* pstData, CMD_STREAM* pCmd);

BOOL CreateEventInfoData3Stream(OLD_EVENT_INFO_DATA3* pData, CMD_STREAM* pCmd);
BOOL CopyEventInfoData3(OLD_EVENT_INFO_DATA3* pstData, CMD_STREAM* pCmd);

void CopyOldNew(OLD_RESERVE_DATA* src, RESERVE_DATA* dest);
void CopyOldNew(OLD_SEARCH_KEY* src, EPG_AUTO_ADD_DATA* dest);
void CopyOldNew(OLD_SEARCH_KEY* src, EPGDB_SEARCH_KEY_INFO* dest);

#endif
