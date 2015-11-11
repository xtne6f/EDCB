#ifndef __EPG_TIMER_UTIL_H__
#define __EPG_TIMER_UTIL_H__

#include "StructDef.h"
#include "EpgDataCap3Def.h"

//チャンネルを__int64としてキーにする
LONGLONG _Create64Key( WORD OriginalNetworkID, WORD TransportStreamID, WORD ServiceID );
//EventIDをunsigned __int64としてキーにする
ULONGLONG _Create64Key2( WORD OriginalNetworkID, WORD TransportStreamID, WORD ServiceID, WORD EventID );
//CRC32をもとめる
unsigned long _Crc32(int n,  BYTE c[]);
//BCD->DWORD変換
DWORD _BCDtoDWORD(BYTE* data, BYTE size, BYTE digit);
//MJD->YYYY/MM/DD変換
BOOL _MJDtoYMD(DWORD mjd, WORD* y, WORD* m, WORD* d);
//MJD->SYSTEMTIME変換
BOOL _MJDtoSYSTEMTIME(DWORD mjd, SYSTEMTIME* time);

//iniファイルから予想ビットレートを取得する
BOOL _GetBitrate(WORD ONID, WORD TSID, WORD SID, DWORD* bitrate);

//EPG情報をTextに変換
void _ConvertEpgInfoText(const EPGDB_EVENT_INFO* info, wstring& text);
void _ConvertEpgInfoText2(const EPGDB_EVENT_INFO* info, wstring& text, wstring serviceName);

//フォルダパスから実際のドライブパスを取得
void GetChkDrivePath(wstring directoryPath, wstring& mountPath);

void GetGenreName(BYTE nibble1, BYTE nibble2, wstring& name);
void GetComponentTypeName(BYTE content, BYTE type, wstring& name);

void CopyEpgInfo(EPG_EVENT_INFO* destInfo, const EPGDB_EVENT_INFO* srcInfo);
void ConvertEpgInfo(WORD onid, WORD tsid, WORD sid, const EPG_EVENT_INFO* src, EPGDB_EVENT_INFO* dest);

#endif
