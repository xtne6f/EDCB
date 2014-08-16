#ifndef __EPG_TIMER_UTIL_H__
#define __EPG_TIMER_UTIL_H__

#include "Util.h"
#include "StructDef.h"
#include "EpgDataCap3Def.h"

//チャンネルを__int64としてキーにする
LONGLONG _Create64Key( WORD OriginalNetworkID, WORD TransportStreamID, WORD ServiceID );
//EventIDを__int64としてキーにする
LONGLONG _Create64Key2( WORD OriginalNetworkID, WORD TransportStreamID, WORD ServiceID, WORD EventID );
//CRC32をもとめる
unsigned long _Crc32(int n,  BYTE c[]);
//録画予想サイズを取得
LONGLONG _GetRecSize( DWORD OriginalNetworkID, DWORD TransportStreamID, DWORD ServiceID, BOOL ServiceOnlyFlag, DWORD DurationSecond );
//指定プロセスIDが起動しているか確認
BOOL _FindOpenExeProcess(DWORD processID);
//指定プロセス名をもつプロセスIDのリストを取得する
vector<DWORD> _FindPidListByExeName(LPCTSTR lpExeName);
//BCD->DWORD変換
DWORD _BCDtoDWORD(BYTE* data, BYTE size, BYTE digit);
//MJD->YYYY/MM/DD変換
BOOL _MJDtoYMD(DWORD mjd, WORD* y, WORD* m, WORD* d);
//MJD->SYSTEMTIME変換
BOOL _MJDtoSYSTEMTIME(DWORD mjd, SYSTEMTIME* time);

//iniファイルから予想ビットレートを取得する
BOOL _GetBitrate(WORD ONID, WORD TSID, WORD SID, DWORD* bitrate);

//EPG情報をTextに変換
void _ConvertEpgInfoText(EPGDB_EVENT_INFO* info, wstring& text);
void _ConvertEpgInfoText2(EPGDB_EVENT_INFO* info, wstring& text, wstring serviceName);

//フォルダパスから実際のドライブパスを取得
void GetChkDrivePath(wstring directoryPath, wstring& mountPath);

void GetGenreName(BYTE nibble1, BYTE nibble2, wstring& name);
void GetGenreList(vector<GENRU_INFO>* list);

void CopyEpgInfo(EPG_EVENT_INFO* destInfo, EPGDB_EVENT_INFO* srcInfo);

#endif
