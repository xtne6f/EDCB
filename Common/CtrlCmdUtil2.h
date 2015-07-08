#ifndef __CTRL_CMD_UTIL2_H__
#define __CTRL_CMD_UTIL2_H__

#include "StructDef.h"
#include "CtrlCmdUtil.h"

namespace CtrlCmdUtilImpl_
{

template<class T> DWORD CCUTIL2_WriteVectorVALUE2_( WORD ver, BYTE* buff, DWORD buffOffset, const vector<T>& val );
template<class T> BOOL CCUTIL2_ReadVectorVALUE2_( WORD ver, vector<T>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

#define CCUTIL2_INHERIT_WRITE_			{ ver; return WriteVALUE(buff, buffOffset, val); }
#define CCUTIL2_INHERIT_READ_			{ ver; return ReadVALUE(val, buff, buffSize, readSize); }
#define CCUTIL2_VECTOR_WRITE_			return CCUTIL2_WriteVectorVALUE2_(ver, buff, buffOffset, val)
#define CCUTIL2_VECTOR_READ_			return CCUTIL2_ReadVectorVALUE2_(ver, val, buff, buffSize, readSize)

////////////////////////////////////////////////////////////////////////////////////////////
//コマンド送信用バイナリ作成関数

inline DWORD WriteVALUE2(WORD ver, BYTE* buff, DWORD buffOffset, char val ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, char* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }
inline DWORD WriteVALUE2(WORD ver, BYTE* buff, DWORD buffOffset, unsigned char val ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, unsigned char* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }

inline DWORD WriteVALUE2(WORD ver, BYTE* buff, DWORD buffOffset, short val ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, short* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }
inline DWORD WriteVALUE2(WORD ver, BYTE* buff, DWORD buffOffset, unsigned short val ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, unsigned short* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }

inline DWORD WriteVALUE2(WORD ver, BYTE* buff, DWORD buffOffset, int val ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, int* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }
inline DWORD WriteVALUE2(WORD ver, BYTE* buff, DWORD buffOffset, unsigned int val ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, unsigned int* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }

inline DWORD WriteVALUE2(WORD ver, BYTE* buff, DWORD buffOffset, long val ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, long* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }
inline DWORD WriteVALUE2(WORD ver, BYTE* buff, DWORD buffOffset, unsigned long val ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, unsigned long* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }

inline DWORD WriteVALUE2(WORD ver, BYTE* buff, DWORD buffOffset, __int64 val ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, __int64* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }
inline DWORD WriteVALUE2(WORD ver, BYTE* buff, DWORD buffOffset, unsigned __int64 val ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, unsigned __int64* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }

inline DWORD WriteVALUE2(WORD ver, BYTE* buff, DWORD buffOffset, const vector<unsigned short>& val ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, vector<unsigned short>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }

inline DWORD WriteVALUE2(WORD ver, BYTE* buff, DWORD buffOffset, const vector<unsigned long>& val ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, vector<unsigned long>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }

inline DWORD WriteVALUE2(WORD ver, BYTE* buff, DWORD buffOffset, const vector<__int64>& val ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, vector<__int64>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }

inline DWORD WriteVALUE2(WORD ver, BYTE* buff, DWORD buffOffset, const wstring& val ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, wstring* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }

inline DWORD WriteVALUE2(WORD ver, BYTE* buff, DWORD buffOffset, const vector<wstring>& val ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, vector<wstring>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }

inline DWORD WriteVALUE2(WORD ver, BYTE* buff, DWORD buffOffset, const SYSTEMTIME& val ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, SYSTEMTIME* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }

inline DWORD WriteVALUE2(WORD ver, BYTE* buff, DWORD buffOffset, const REC_FILE_SET_INFO& val ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, REC_FILE_SET_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }

inline DWORD WriteVALUE2(WORD ver, BYTE* buff, DWORD buffOffset, const vector<REC_FILE_SET_INFO>& val ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, vector<REC_FILE_SET_INFO>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }

DWORD WriteVALUE2(WORD ver, BYTE* buff, DWORD buffOffset, const REC_SETTING_DATA& val );
BOOL ReadVALUE2(WORD ver, REC_SETTING_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD WriteVALUE2(WORD ver, BYTE* buff, DWORD buffOffset, const RESERVE_DATA& val );
BOOL ReadVALUE2(WORD ver, RESERVE_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD WriteVALUE2(WORD ver, BYTE* buff, DWORD buffOffset, const vector<RESERVE_DATA>& val ){ CCUTIL2_VECTOR_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, vector<RESERVE_DATA>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_VECTOR_READ_; }

DWORD WriteVALUE2(WORD ver, BYTE* buff, DWORD buffOffset, const NOTIFY_SRV_INFO& val );
BOOL ReadVALUE2(WORD ver, NOTIFY_SRV_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD WriteVALUE2(WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_CONTENT_DATA& val ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, EPGDB_CONTENT_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }

inline DWORD WriteVALUE2(WORD ver, BYTE* buff, DWORD buffOffset, const vector<EPGDB_CONTENT_DATA>& val ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, vector<EPGDB_CONTENT_DATA>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }

inline DWORD WriteVALUE2(WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_SEARCH_DATE_INFO& val ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, EPGDB_SEARCH_DATE_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }

inline DWORD WriteVALUE2(WORD ver, BYTE* buff, DWORD buffOffset, const vector<EPGDB_SEARCH_DATE_INFO>& val ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, vector<EPGDB_SEARCH_DATE_INFO>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }

DWORD WriteVALUE2(WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_SEARCH_KEY_INFO& val );
BOOL ReadVALUE2(WORD ver, EPGDB_SEARCH_KEY_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD WriteVALUE2(WORD ver, BYTE* buff, DWORD buffOffset, const EPG_AUTO_ADD_DATA& val );
BOOL ReadVALUE2(WORD ver, EPG_AUTO_ADD_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD WriteVALUE2(WORD ver, BYTE* buff, DWORD buffOffset, const vector<EPG_AUTO_ADD_DATA>& val ){ CCUTIL2_VECTOR_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, vector<EPG_AUTO_ADD_DATA>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_VECTOR_READ_; }

DWORD WriteVALUE2(WORD ver, BYTE* buff, DWORD buffOffset, const MANUAL_AUTO_ADD_DATA& val );
BOOL ReadVALUE2(WORD ver, MANUAL_AUTO_ADD_DATA* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD WriteVALUE2(WORD ver, BYTE* buff, DWORD buffOffset, const vector<MANUAL_AUTO_ADD_DATA>& val ){ CCUTIL2_VECTOR_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, vector<MANUAL_AUTO_ADD_DATA>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_VECTOR_READ_; }

DWORD WriteVALUE2(WORD ver, BYTE* buff, DWORD buffOffset, const REC_FILE_INFO& val );
BOOL ReadVALUE2(WORD ver, REC_FILE_INFO* val, const BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD WriteVALUE2(WORD ver, BYTE* buff, DWORD buffOffset, const vector<REC_FILE_INFO>& val ){ CCUTIL2_VECTOR_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, vector<REC_FILE_INFO>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_VECTOR_READ_; }

////////////////////////////////////////////////////////////////////////////////////////////
//テンプレート定義

template<class T>
DWORD CCUTIL2_WriteVectorVALUE2_( WORD ver, BYTE* buff, DWORD buffOffset, const vector<T>& val )
{
	DWORD pos = buffOffset + sizeof(DWORD);
	//リストの個数
	pos += WriteVALUE(buff, pos, (DWORD)val.size());
	//リストの中身
	for( size_t i = 0; i < val.size(); i++ ){
		pos += WriteVALUE2(ver, buff, pos, val[i]);
	}
	//全体のサイズ
	WriteVALUE(buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

template<class T>
BOOL CCUTIL2_ReadVectorVALUE2_( WORD ver, vector<T>* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
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
		if( ReadVALUE2(ver, &data, buff + pos, buffSize - pos, &size) == FALSE ){
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
inline BOOL ReadVALUE2( WORD ver, T* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	return CtrlCmdUtilImpl_::ReadVALUE2(ver, val, buff, buffSize, readSize);
}

template<class T>
BYTE* NewWriteVALUE2WithVersion( WORD ver, const T& val, DWORD& writeSize )
{
	DWORD buffSize = CtrlCmdUtilImpl_::WriteVALUE(NULL, 0, ver) + CtrlCmdUtilImpl_::WriteVALUE2(ver, NULL, 0, val);
	BYTE* buff = new BYTE[buffSize];
	try{
		CtrlCmdUtilImpl_::WriteVALUE2(ver, buff, CtrlCmdUtilImpl_::WriteVALUE(buff, 0, ver), val);
	}catch( ... ){
		delete[] buff;
		throw;
	}
	writeSize = buffSize;
	return buff;
}

template<class T>
BYTE* NewWriteVALUE2WithVersion( WORD ver, T* val, DWORD& writeSize )
{
	if( val == NULL ){
		return NULL;
	}
	return NewWriteVALUE2WithVersion(ver, *val, writeSize);
}

#endif
