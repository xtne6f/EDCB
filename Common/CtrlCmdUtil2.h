#ifndef __CTRL_CMD_UTIL2_H__
#define __CTRL_CMD_UTIL2_H__

#include "StructDef.h"
#include "CtrlCmdUtil.h"

#define CCUTIL2_INHERIT_GET_SIZE_		{ ver; return GetVALUESize(val); }
#define CCUTIL2_INHERIT_WRITE_			{ ver; return WriteVALUE(val, buff, buffSize, writeSize); }
#define CCUTIL2_INHERIT_READ_			{ ver; return ReadVALUE(val, buff, buffSize, readSize); }

////////////////////////////////////////////////////////////////////////////////////////////
//コマンド送信用バイナリ作成関数

inline DWORD GetVALUESize2(WORD ver, char val ){ CCUTIL2_INHERIT_GET_SIZE_; }
inline BOOL WriteVALUE2(WORD ver, char val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, char* val, BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }
inline DWORD GetVALUESize2(WORD ver, unsigned char val ){ CCUTIL2_INHERIT_GET_SIZE_; }
inline BOOL WriteVALUE2(WORD ver, unsigned char val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, unsigned char* val, BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }

inline DWORD GetVALUESize2(WORD ver, short val ){ CCUTIL2_INHERIT_GET_SIZE_; }
inline BOOL WriteVALUE2(WORD ver, short val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, short* val, BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }
inline DWORD GetVALUESize2(WORD ver, unsigned short val ){ CCUTIL2_INHERIT_GET_SIZE_; }
inline BOOL WriteVALUE2(WORD ver, unsigned short val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, unsigned short* val, BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }

inline DWORD GetVALUESize2(WORD ver, int val ){ CCUTIL2_INHERIT_GET_SIZE_; }
inline BOOL WriteVALUE2(WORD ver, int val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, int* val, BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }
inline DWORD GetVALUESize2(WORD ver, unsigned int val ){ CCUTIL2_INHERIT_GET_SIZE_; }
inline BOOL WriteVALUE2(WORD ver, unsigned int val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, unsigned int* val, BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }

inline DWORD GetVALUESize2(WORD ver, long val ){ CCUTIL2_INHERIT_GET_SIZE_; }
inline BOOL WriteVALUE2(WORD ver, long val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, long* val, BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }
inline DWORD GetVALUESize2(WORD ver, unsigned long val ){ CCUTIL2_INHERIT_GET_SIZE_; }
inline BOOL WriteVALUE2(WORD ver, unsigned long val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, unsigned long* val, BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }

inline DWORD GetVALUESize2(WORD ver, __int64 val ){ CCUTIL2_INHERIT_GET_SIZE_; }
inline BOOL WriteVALUE2(WORD ver, __int64 val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, __int64* val, BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }
inline DWORD GetVALUESize2(WORD ver, unsigned __int64 val ){ CCUTIL2_INHERIT_GET_SIZE_; }
inline BOOL WriteVALUE2(WORD ver, unsigned __int64 val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, unsigned __int64* val, BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }

inline DWORD GetVALUESize2(WORD ver, vector<unsigned short>* val){ CCUTIL2_INHERIT_GET_SIZE_; }
inline BOOL WriteVALUE2(WORD ver, vector<unsigned short>* val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, vector<unsigned short>* val, BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }

inline DWORD GetVALUESize2(WORD ver, vector<unsigned long>* val){ CCUTIL2_INHERIT_GET_SIZE_; }
inline BOOL WriteVALUE2(WORD ver, vector<unsigned long>* val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, vector<unsigned long>* val, BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }

inline DWORD GetVALUESize2(WORD ver, vector<__int64>* val){ CCUTIL2_INHERIT_GET_SIZE_; }
inline BOOL WriteVALUE2(WORD ver, vector<__int64>* val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, vector<__int64>* val, BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }

inline DWORD GetVALUESize2(WORD ver, const wstring& val ){ CCUTIL2_INHERIT_GET_SIZE_; }
inline BOOL WriteVALUE2(WORD ver, const wstring& val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, wstring* val, BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }

inline DWORD GetVALUESize2(WORD ver, vector<wstring>* val ){ CCUTIL2_INHERIT_GET_SIZE_; }
inline BOOL WriteVALUE2(WORD ver, vector<wstring>* val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, vector<wstring>* val, BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }

inline DWORD GetVALUESize2(WORD ver, SYSTEMTIME* val ){ CCUTIL2_INHERIT_GET_SIZE_; }
inline BOOL WriteVALUE2(WORD ver, SYSTEMTIME* val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, SYSTEMTIME* val, BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }

inline DWORD GetVALUESize2(WORD ver, REC_FILE_SET_INFO* val ){ CCUTIL2_INHERIT_GET_SIZE_; }
inline BOOL WriteVALUE2(WORD ver, REC_FILE_SET_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, REC_FILE_SET_INFO* val, BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }

inline DWORD GetVALUESize2(WORD ver, vector<REC_FILE_SET_INFO>* val ){ CCUTIL2_INHERIT_GET_SIZE_; }
inline BOOL WriteVALUE2(WORD ver, vector<REC_FILE_SET_INFO>* val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, vector<REC_FILE_SET_INFO>* val, BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }

DWORD GetVALUESize2(WORD ver, REC_SETTING_DATA* val );
BOOL WriteVALUE2(WORD ver, REC_SETTING_DATA* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE2(WORD ver, REC_SETTING_DATA* val, BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD GetVALUESize2(WORD ver, RESERVE_DATA* val );
BOOL WriteVALUE2(WORD ver, RESERVE_DATA* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE2(WORD ver, RESERVE_DATA* val, BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD GetVALUESize2(WORD ver, vector<RESERVE_DATA>* val );
BOOL WriteVALUE2(WORD ver, vector<RESERVE_DATA>* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE2(WORD ver, vector<RESERVE_DATA>* val, BYTE* buff, DWORD buffSize, DWORD* readSize );
DWORD GetVALUESize2(WORD ver, vector<RESERVE_DATA*>* val );
BOOL WriteVALUE2(WORD ver, vector<RESERVE_DATA*>* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE2(WORD ver, vector<RESERVE_DATA*>* val, BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD GetVALUESize2(WORD ver, NOTIFY_SRV_INFO* val );
BOOL WriteVALUE2(WORD ver, NOTIFY_SRV_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE2(WORD ver, NOTIFY_SRV_INFO* val, BYTE* buff, DWORD buffSize, DWORD* readSize );

inline DWORD GetVALUESize2(WORD ver, EPGDB_CONTENT_DATA* val ){ CCUTIL2_INHERIT_GET_SIZE_; }
inline BOOL WriteVALUE2(WORD ver, EPGDB_CONTENT_DATA* val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, EPGDB_CONTENT_DATA* val, BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }

inline DWORD GetVALUESize2(WORD ver, vector<EPGDB_CONTENT_DATA>* val ){ CCUTIL2_INHERIT_GET_SIZE_; }
inline BOOL WriteVALUE2(WORD ver, vector<EPGDB_CONTENT_DATA>* val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, vector<EPGDB_CONTENT_DATA>* val, BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }

inline DWORD GetVALUESize2(WORD ver, EPGDB_SEARCH_DATE_INFO* val ){ CCUTIL2_INHERIT_GET_SIZE_; }
inline BOOL WriteVALUE2(WORD ver, EPGDB_SEARCH_DATE_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, EPGDB_SEARCH_DATE_INFO* val, BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }

inline DWORD GetVALUESize2(WORD ver, vector<EPGDB_SEARCH_DATE_INFO>* val ){ CCUTIL2_INHERIT_GET_SIZE_; }
inline BOOL WriteVALUE2(WORD ver, vector<EPGDB_SEARCH_DATE_INFO>* val, BYTE* buff, DWORD buffSize, DWORD* writeSize ){ CCUTIL2_INHERIT_WRITE_; }
inline BOOL ReadVALUE2(WORD ver, vector<EPGDB_SEARCH_DATE_INFO>* val, BYTE* buff, DWORD buffSize, DWORD* readSize ){ CCUTIL2_INHERIT_READ_; }

DWORD GetVALUESize2(WORD ver, EPGDB_SEARCH_KEY_INFO* val );
BOOL WriteVALUE2(WORD ver, EPGDB_SEARCH_KEY_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE2(WORD ver, EPGDB_SEARCH_KEY_INFO* val, BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD GetVALUESize2(WORD ver, EPG_AUTO_ADD_DATA* val );
BOOL WriteVALUE2(WORD ver, EPG_AUTO_ADD_DATA* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE2(WORD ver, EPG_AUTO_ADD_DATA* val, BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD GetVALUESize2(WORD ver, vector<EPG_AUTO_ADD_DATA>* val );
BOOL WriteVALUE2(WORD ver, vector<EPG_AUTO_ADD_DATA>* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE2(WORD ver, vector<EPG_AUTO_ADD_DATA>* val, BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD GetVALUESize2(WORD ver, MANUAL_AUTO_ADD_DATA* val );
BOOL WriteVALUE2(WORD ver, MANUAL_AUTO_ADD_DATA* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE2(WORD ver, MANUAL_AUTO_ADD_DATA* val, BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD GetVALUESize2(WORD ver, vector<MANUAL_AUTO_ADD_DATA>* val );
BOOL WriteVALUE2(WORD ver, vector<MANUAL_AUTO_ADD_DATA>* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE2(WORD ver, vector<MANUAL_AUTO_ADD_DATA>* val, BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD GetVALUESize2(WORD ver, REC_FILE_INFO* val );
BOOL WriteVALUE2(WORD ver, REC_FILE_INFO* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE2(WORD ver, REC_FILE_INFO* val, BYTE* buff, DWORD buffSize, DWORD* readSize );

DWORD GetVALUESize2(WORD ver, vector<REC_FILE_INFO>* val );
BOOL WriteVALUE2(WORD ver, vector<REC_FILE_INFO>* val, BYTE* buff, DWORD buffSize, DWORD* writeSize );
BOOL ReadVALUE2(WORD ver, vector<REC_FILE_INFO>* val, BYTE* buff, DWORD buffSize, DWORD* readSize );

#endif
