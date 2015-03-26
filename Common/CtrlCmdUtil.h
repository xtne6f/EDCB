#ifndef __CTRL_CMD_UTIL_H__
#define __CTRL_CMD_UTIL_H__

#include "StructDef.h"

BOOL CCUTIL_WriteStream_( const void* val, DWORD valSize, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL CCUTIL_ReadStream_( void* val, DWORD valSize, const BYTE* buff, DWORD buffSize, DWORD* readSize );
template<class T> DWORD CCUTIL_GetVectorVALUESize_( const vector<T>* val );
template<class T> DWORD CCUTIL_GetPtrVectorVALUESize_( const vector<T>* val );
template<class T> BOOL CCUTIL_WriteVectorVALUE_( const vector<T>* val, BYTE* buff, DWORD buffSize, DWORD* writeSize, bool bTreatNullAsEmpty );
template<class T> BOOL CCUTIL_WritePtrVectorVALUE_( const vector<T>* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
template<class T> BOOL CCUTIL_ReadVectorVALUE_( vector<T>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );
template<class T> BOOL CCUTIL_ReadAndNewVectorVALUE_( vector<T*>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

#define CCUTIL_BASETYPE_GET_SIZE_		{ val; return sizeof(val); }
#define CCUTIL_BASETYPE_GET_SIZE_PTR_	{ val; return sizeof(*val); }
#define CCUTIL_BASETYPE_WRITE_			return CCUTIL_WriteStream_(&val, sizeof(val), buff, buffSize, writeSize)
#define CCUTIL_BASETYPE_WRITE_PTR_		return CCUTIL_WriteStream_(val, sizeof(*val), buff, buffSize, writeSize)
#define CCUTIL_BASETYPE_READ_			return CCUTIL_ReadStream_(val, sizeof(*val), buff, buffSize, readSize)
#define CCUTIL_VECTOR_GET_SIZE_			return CCUTIL_GetVectorVALUESize_(val)
#define CCUTIL_VECTOR_GET_SIZE_PTR_		return CCUTIL_GetPtrVectorVALUESize_(val)
#define CCUTIL_VECTOR_WRITE_(b)			return CCUTIL_WriteVectorVALUE_(val, buff, buffSize, writeSize, b)
#define CCUTIL_VECTOR_WRITE_PTR_		return CCUTIL_WritePtrVectorVALUE_(val, buff, buffSize, writeSize)
#define CCUTIL_VECTOR_READ_				return CCUTIL_ReadVectorVALUE_(val, buff, buffSize, readSize)
#define CCUTIL_VECTOR_READ_AND_NEW_		return CCUTIL_ReadAndNewVectorVALUE_(val, buff, buffSize, readSize)

////////////////////////////////////////////////////////////////////////////////////////////
//コマンド送信用バイナリ作成関数

inline DWORD GetVALUESize( char val ){ CCUTIL_BASETYPE_GET_SIZE_; }
inline BOOL WriteVALUE( char val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL_BASETYPE_WRITE_; }
inline BOOL ReadVALUE( char* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_BASETYPE_READ_; }
inline DWORD GetVALUESize( unsigned char val ){ CCUTIL_BASETYPE_GET_SIZE_; }
inline BOOL WriteVALUE( unsigned char val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL_BASETYPE_WRITE_; }
inline BOOL ReadVALUE( unsigned char* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_BASETYPE_READ_; }

inline DWORD GetVALUESize( short val ){ CCUTIL_BASETYPE_GET_SIZE_; }
inline BOOL WriteVALUE( short val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL_BASETYPE_WRITE_; }
inline BOOL ReadVALUE( short* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_BASETYPE_READ_; }
inline DWORD GetVALUESize( unsigned short val ){ CCUTIL_BASETYPE_GET_SIZE_; }
inline BOOL WriteVALUE( unsigned short val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL_BASETYPE_WRITE_; }
inline BOOL ReadVALUE( unsigned short* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_BASETYPE_READ_; }

inline DWORD GetVALUESize( int val ){ CCUTIL_BASETYPE_GET_SIZE_; }
inline BOOL WriteVALUE( int val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL_BASETYPE_WRITE_; }
inline BOOL ReadVALUE( int* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_BASETYPE_READ_; }
inline DWORD GetVALUESize( unsigned int val ){ CCUTIL_BASETYPE_GET_SIZE_; }
inline BOOL WriteVALUE( unsigned int val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL_BASETYPE_WRITE_; }
inline BOOL ReadVALUE( unsigned int* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_BASETYPE_READ_; }

inline DWORD GetVALUESize( long val ){ CCUTIL_BASETYPE_GET_SIZE_; }
inline BOOL WriteVALUE( long val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL_BASETYPE_WRITE_; }
inline BOOL ReadVALUE( long* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_BASETYPE_READ_; }
inline DWORD GetVALUESize( unsigned long val ){ CCUTIL_BASETYPE_GET_SIZE_; }
inline BOOL WriteVALUE( unsigned long val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL_BASETYPE_WRITE_; }
inline BOOL ReadVALUE( unsigned long* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_BASETYPE_READ_; }

inline DWORD GetVALUESize( __int64 val ){ CCUTIL_BASETYPE_GET_SIZE_; }
inline BOOL WriteVALUE( __int64 val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL_BASETYPE_WRITE_; }
inline BOOL ReadVALUE( __int64* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_BASETYPE_READ_; }
inline DWORD GetVALUESize( unsigned __int64 val ){ CCUTIL_BASETYPE_GET_SIZE_; }
inline BOOL WriteVALUE( unsigned __int64 val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL_BASETYPE_WRITE_; }
inline BOOL ReadVALUE( unsigned __int64* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_BASETYPE_READ_; }

DWORD GetVALUESize( const vector<unsigned short>* val);
inline BOOL WriteVALUE( const vector<unsigned short>* val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL_VECTOR_WRITE_(true); } //val==NULLは空要素ベクタと同じ扱い
inline BOOL ReadVALUE( vector<unsigned short>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD GetVALUESize( const vector<unsigned long>* val);
inline BOOL WriteVALUE( const vector<unsigned long>* val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL_VECTOR_WRITE_(true); } //val==NULLは空要素ベクタと同じ扱い
inline BOOL ReadVALUE( vector<unsigned long>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD GetVALUESize( const vector<__int64>* val);
inline BOOL WriteVALUE( const vector<__int64>* val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL_VECTOR_WRITE_(false); }
inline BOOL ReadVALUE( vector<__int64>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD GetVALUESize( const wstring& val );
BOOL WriteVALUE( const wstring& val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE( wstring* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD GetVALUESize( const vector<wstring>* val ){ CCUTIL_VECTOR_GET_SIZE_; }
inline BOOL WriteVALUE( const vector<wstring>* val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL_VECTOR_WRITE_(false); }
inline BOOL ReadVALUE( vector<wstring>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

inline DWORD GetVALUESize( const SYSTEMTIME* val ){ CCUTIL_BASETYPE_GET_SIZE_PTR_; }
inline BOOL WriteVALUE( const SYSTEMTIME* val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL_BASETYPE_WRITE_PTR_; }
inline BOOL ReadVALUE( SYSTEMTIME* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_BASETYPE_READ_; }

DWORD GetVALUESize( const REC_SETTING_DATA* val );
BOOL WriteVALUE( const REC_SETTING_DATA* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE( REC_SETTING_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD GetVALUESize( const RESERVE_DATA* val );
BOOL WriteVALUE( const RESERVE_DATA* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE( RESERVE_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD GetVALUESize( const vector<RESERVE_DATA>* val ){ CCUTIL_VECTOR_GET_SIZE_PTR_; }
inline BOOL WriteVALUE( const vector<RESERVE_DATA>* val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL_VECTOR_WRITE_PTR_; }
inline BOOL ReadVALUE( vector<RESERVE_DATA>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }
inline DWORD GetVALUESize( const vector<RESERVE_DATA*>* val ){ CCUTIL_VECTOR_GET_SIZE_; }
inline BOOL WriteVALUE( const vector<RESERVE_DATA*>* val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL_VECTOR_WRITE_(false); }
inline BOOL ReadVALUE( vector<RESERVE_DATA*>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_AND_NEW_; }

DWORD GetVALUESize( const EPGDB_SERVICE_INFO* val );
BOOL WriteVALUE( const EPGDB_SERVICE_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE( EPGDB_SERVICE_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD GetVALUESize( const vector<EPGDB_SERVICE_INFO>* val ){ CCUTIL_VECTOR_GET_SIZE_PTR_; }
inline BOOL WriteVALUE( const vector<EPGDB_SERVICE_INFO>* val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL_VECTOR_WRITE_PTR_; }
inline BOOL ReadVALUE( vector<EPGDB_SERVICE_INFO>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD GetVALUESize( const EPGDB_SHORT_EVENT_INFO* val );
BOOL WriteVALUE( const EPGDB_SHORT_EVENT_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE( EPGDB_SHORT_EVENT_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD GetVALUESize( const EPGDB_EXTENDED_EVENT_INFO* val );
BOOL WriteVALUE( const EPGDB_EXTENDED_EVENT_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE( EPGDB_EXTENDED_EVENT_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD GetVALUESize( const EPGDB_CONTENT_DATA* val );
BOOL WriteVALUE( const EPGDB_CONTENT_DATA* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE( EPGDB_CONTENT_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD GetVALUESize( const vector<EPGDB_CONTENT_DATA>* val ){ CCUTIL_VECTOR_GET_SIZE_PTR_; }
inline BOOL WriteVALUE( const vector<EPGDB_CONTENT_DATA>* val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL_VECTOR_WRITE_PTR_; }
inline BOOL ReadVALUE( vector<EPGDB_CONTENT_DATA>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD GetVALUESize( const EPGDB_CONTEN_INFO* val );
BOOL WriteVALUE( const EPGDB_CONTEN_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE( EPGDB_CONTEN_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD GetVALUESize( const EPGDB_COMPONENT_INFO* val );
BOOL WriteVALUE( const EPGDB_COMPONENT_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE( EPGDB_COMPONENT_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD GetVALUESize( const EPGDB_AUDIO_COMPONENT_INFO_DATA* val );
BOOL WriteVALUE( const EPGDB_AUDIO_COMPONENT_INFO_DATA* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE( EPGDB_AUDIO_COMPONENT_INFO_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD GetVALUESize( const vector<EPGDB_AUDIO_COMPONENT_INFO_DATA>* val ){ CCUTIL_VECTOR_GET_SIZE_PTR_; }
inline BOOL WriteVALUE( const vector<EPGDB_AUDIO_COMPONENT_INFO_DATA>* val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL_VECTOR_WRITE_PTR_; }
inline BOOL ReadVALUE( vector<EPGDB_AUDIO_COMPONENT_INFO_DATA>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD GetVALUESize( const EPGDB_AUDIO_COMPONENT_INFO* val );
BOOL WriteVALUE( const EPGDB_AUDIO_COMPONENT_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE( EPGDB_AUDIO_COMPONENT_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD GetVALUESize( const EPGDB_EVENT_DATA* val );
BOOL WriteVALUE( const EPGDB_EVENT_DATA* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE( EPGDB_EVENT_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD GetVALUESize( const vector<EPGDB_EVENT_DATA>* val ){ CCUTIL_VECTOR_GET_SIZE_PTR_; }
inline BOOL WriteVALUE( const vector<EPGDB_EVENT_DATA>* val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL_VECTOR_WRITE_PTR_; }
inline BOOL ReadVALUE( vector<EPGDB_EVENT_DATA>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD GetVALUESize( const EPGDB_EVENTGROUP_INFO* val );
BOOL WriteVALUE( const EPGDB_EVENTGROUP_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE( EPGDB_EVENTGROUP_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD GetVALUESize( const EPGDB_EVENT_INFO* val );
BOOL WriteVALUE( const EPGDB_EVENT_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE( EPGDB_EVENT_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD GetVALUESize( const vector<EPGDB_EVENT_INFO*>* val ){ CCUTIL_VECTOR_GET_SIZE_; }
inline BOOL WriteVALUE( const vector<EPGDB_EVENT_INFO*>* val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL_VECTOR_WRITE_(false); }
inline BOOL ReadVALUE( vector<EPGDB_EVENT_INFO*>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_AND_NEW_; }

DWORD GetVALUESize( const EPGDB_SEARCH_DATE_INFO* val );
BOOL WriteVALUE( const EPGDB_SEARCH_DATE_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE( EPGDB_SEARCH_DATE_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD GetVALUESize( const vector<EPGDB_SEARCH_DATE_INFO>* val ){ CCUTIL_VECTOR_GET_SIZE_PTR_; }
inline BOOL WriteVALUE( const vector<EPGDB_SEARCH_DATE_INFO>* val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL_VECTOR_WRITE_PTR_; }
inline BOOL ReadVALUE( vector<EPGDB_SEARCH_DATE_INFO>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD GetVALUESize( const EPGDB_SEARCH_KEY_INFO* val );
BOOL WriteVALUE( const EPGDB_SEARCH_KEY_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE( EPGDB_SEARCH_KEY_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD GetVALUESize( const vector<EPGDB_SEARCH_KEY_INFO>* val ){ CCUTIL_VECTOR_GET_SIZE_PTR_; }
inline BOOL WriteVALUE( const vector<EPGDB_SEARCH_KEY_INFO>* val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL_VECTOR_WRITE_PTR_; }
inline BOOL ReadVALUE( vector<EPGDB_SEARCH_KEY_INFO>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD GetVALUESize( const SET_CH_INFO* val );
BOOL WriteVALUE( const SET_CH_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE( SET_CH_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD GetVALUESize( const vector<SET_CH_INFO>* val ){ CCUTIL_VECTOR_GET_SIZE_PTR_; }
inline BOOL WriteVALUE( const vector<SET_CH_INFO>* val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL_VECTOR_WRITE_PTR_; }
inline BOOL ReadVALUE( vector<SET_CH_INFO>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD GetVALUESize( const SET_CTRL_MODE* val );
BOOL WriteVALUE( const SET_CTRL_MODE* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE( SET_CTRL_MODE* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD GetVALUESize( const REC_FILE_SET_INFO* val );
BOOL WriteVALUE( const REC_FILE_SET_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE( REC_FILE_SET_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD GetVALUESize( const vector<REC_FILE_SET_INFO>* val ){ CCUTIL_VECTOR_GET_SIZE_PTR_; }
inline BOOL WriteVALUE( const vector<REC_FILE_SET_INFO>* val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL_VECTOR_WRITE_PTR_; }
inline BOOL ReadVALUE( vector<REC_FILE_SET_INFO>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD GetVALUESize( const SET_CTRL_REC_PARAM* val );
BOOL WriteVALUE( const SET_CTRL_REC_PARAM* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE( SET_CTRL_REC_PARAM* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD GetVALUESize( const SET_CTRL_REC_STOP_PARAM* val );
BOOL WriteVALUE( const SET_CTRL_REC_STOP_PARAM* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE( SET_CTRL_REC_STOP_PARAM* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD GetVALUESize( const SET_CTRL_REC_STOP_RES_PARAM* val );
BOOL WriteVALUE( const SET_CTRL_REC_STOP_RES_PARAM* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE( SET_CTRL_REC_STOP_RES_PARAM* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD GetVALUESize( const REC_FILE_INFO* val );
BOOL WriteVALUE( const REC_FILE_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE( REC_FILE_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD GetVALUESize( const vector<REC_FILE_INFO>* val ){ CCUTIL_VECTOR_GET_SIZE_PTR_; }
inline BOOL WriteVALUE( const vector<REC_FILE_INFO>* val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL_VECTOR_WRITE_PTR_; }
inline BOOL ReadVALUE( vector<REC_FILE_INFO>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD GetVALUESize( const EPG_AUTO_ADD_DATA* val );
BOOL WriteVALUE( const EPG_AUTO_ADD_DATA* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE( EPG_AUTO_ADD_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD GetVALUESize( const vector<EPG_AUTO_ADD_DATA>* val ){ CCUTIL_VECTOR_GET_SIZE_PTR_; }
inline BOOL WriteVALUE( const vector<EPG_AUTO_ADD_DATA>* val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL_VECTOR_WRITE_PTR_; }
inline BOOL ReadVALUE( vector<EPG_AUTO_ADD_DATA>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD GetVALUESize( const SEARCH_EPG_INFO_PARAM* val );
BOOL WriteVALUE( const SEARCH_EPG_INFO_PARAM* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE( SEARCH_EPG_INFO_PARAM* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD GetVALUESize( const GET_EPG_PF_INFO_PARAM* val );
BOOL WriteVALUE( const GET_EPG_PF_INFO_PARAM* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE( GET_EPG_PF_INFO_PARAM* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD GetVALUESize( const MANUAL_AUTO_ADD_DATA* val );
BOOL WriteVALUE( const MANUAL_AUTO_ADD_DATA* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE( MANUAL_AUTO_ADD_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD GetVALUESize( const vector<MANUAL_AUTO_ADD_DATA>* val ){ CCUTIL_VECTOR_GET_SIZE_PTR_; }
inline BOOL WriteVALUE( const vector<MANUAL_AUTO_ADD_DATA>* val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL_VECTOR_WRITE_PTR_; }
inline BOOL ReadVALUE( vector<MANUAL_AUTO_ADD_DATA>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD GetVALUESize( const TUNER_RESERVE_INFO* val );
BOOL WriteVALUE( const TUNER_RESERVE_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE( TUNER_RESERVE_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD GetVALUESize( const vector<TUNER_RESERVE_INFO>* val ){ CCUTIL_VECTOR_GET_SIZE_PTR_; }
inline BOOL WriteVALUE( const vector<TUNER_RESERVE_INFO>* val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL_VECTOR_WRITE_PTR_; }
inline BOOL ReadVALUE( vector<TUNER_RESERVE_INFO>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_; }

DWORD GetVALUESize( const REGIST_TCP_INFO* val );
BOOL WriteVALUE( const REGIST_TCP_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE( REGIST_TCP_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD GetVALUESize( const EPGDB_SERVICE_EVENT_INFO* val );
BOOL WriteVALUE( const EPGDB_SERVICE_EVENT_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE( EPGDB_SERVICE_EVENT_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD GetVALUESize( const vector<EPGDB_SERVICE_EVENT_INFO*>* val ){ CCUTIL_VECTOR_GET_SIZE_; }
inline BOOL WriteVALUE( const vector<EPGDB_SERVICE_EVENT_INFO*>* val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL_VECTOR_WRITE_(false); }
inline BOOL ReadVALUE( vector<EPGDB_SERVICE_EVENT_INFO*>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL_VECTOR_READ_AND_NEW_; }

DWORD GetVALUESize( const TVTEST_CH_CHG_INFO* val );
BOOL WriteVALUE( const TVTEST_CH_CHG_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE( TVTEST_CH_CHG_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD GetVALUESize( const NWPLAY_PLAY_INFO* val );
BOOL WriteVALUE( const NWPLAY_PLAY_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE( NWPLAY_PLAY_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD GetVALUESize( const NWPLAY_POS_CMD* val );
BOOL WriteVALUE( const NWPLAY_POS_CMD* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE( NWPLAY_POS_CMD* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD GetVALUESize( const TVTEST_STREAMING_INFO* val );
BOOL WriteVALUE( const TVTEST_STREAMING_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE( TVTEST_STREAMING_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD GetVALUESize( const NWPLAY_TIMESHIFT_INFO* val );
BOOL WriteVALUE( const NWPLAY_TIMESHIFT_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE( NWPLAY_TIMESHIFT_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

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

////////////////////////////////////////////////////////////////////////////////////////////
//テンプレート定義

template<class T>
DWORD CCUTIL_GetVectorVALUESize_( const vector<T>* val )
{
	DWORD size = sizeof(DWORD) * 2;
	if( val != NULL ){
		for( size_t i=0; i < val->size(); i++ ){
			size += GetVALUESize((*val)[i]);
		}
	}
	return size;
}

template<class T>
DWORD CCUTIL_GetPtrVectorVALUESize_( const vector<T>* val )
{
	DWORD size = sizeof(DWORD) * 2;
	if( val != NULL ){
		for( size_t i=0; i < val->size(); i++ ){
			size += GetVALUESize(&(*val)[i]);
		}
	}
	return size;
}

template<class T>
BOOL CCUTIL_WriteVectorVALUE_( const vector<T>* val, BYTE* buff, DWORD buffSize, DWORD* writeSize, bool bTreatNullAsEmpty )
{
	DWORD pos = sizeof(DWORD) * 2;
	DWORD count = 0;
	if( !bTreatNullAsEmpty && val == NULL || buff == NULL || buffSize < pos ){
		return FALSE;
	}
	if( val != NULL ){
		//リストの中身
		DWORD size = 0;
		for( ; count < (DWORD)val->size(); count++ ){
			if( WriteVALUE((*val)[count], buff + pos, buffSize - pos, &size) == FALSE ){
				return FALSE;
			}
			pos += size;
		}
	}
	//全体のサイズ
	WriteVALUE(pos, buff, sizeof(pos), NULL);
	//リストの個数
	WriteVALUE(count, buff + sizeof(pos), sizeof(count), NULL);

	if( writeSize != NULL ){
		*writeSize = pos;
	}
	return TRUE;
}

template<class T>
BOOL CCUTIL_WritePtrVectorVALUE_( const vector<T>* val, BYTE* buff, DWORD buffSize, DWORD* writeSize )
{
	DWORD pos = sizeof(DWORD) * 2;
	DWORD count = 0;
	if( val == NULL || buff == NULL || buffSize < pos ){
		return FALSE;
	}
	//リストの中身
	DWORD size = 0;
	for( ; count < (DWORD)val->size(); count++ ){
		if( WriteVALUE(&(*val)[count], buff + pos, buffSize - pos, &size) == FALSE ){
			return FALSE;
		}
		pos += size;
	}
	//全体のサイズ
	WriteVALUE(pos, buff, sizeof(pos), NULL);
	//リストの個数
	WriteVALUE(count, buff + sizeof(pos), sizeof(count), NULL);

	if( writeSize != NULL ){
		*writeSize = pos;
	}
	return TRUE;
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

#endif
