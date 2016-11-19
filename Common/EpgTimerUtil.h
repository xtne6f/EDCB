#ifndef __EPG_TIMER_UTIL_H__
#define __EPG_TIMER_UTIL_H__

#include "StructDef.h"
#include "EpgDataCap3Def.h"

//チャンネルを__int64としてキーにする
static inline LONGLONG _Create64Key(WORD onid, WORD tsid, WORD sid) { return sid | (DWORD)tsid << 16 | (LONGLONG)onid << 32; }
//EventIDをunsigned __int64としてキーにする
static inline ULONGLONG _Create64Key2(WORD onid, WORD tsid, WORD sid, WORD eid) { return eid | (DWORD)sid << 16 | (ULONGLONG)tsid << 32 | (ULONGLONG)onid << 48; }
//CRC32をもとめる
unsigned long CalcCrc32(int n, const BYTE* c);
//MJD->FILETIME変換
FILETIME MJDtoFILETIME(DWORD mjd, DWORD bcdTime = 0);

//iniファイルから予想ビットレートを取得する
DWORD GetBitrateFromIni(WORD onid, WORD tsid, WORD sid);

//EPG情報をTextに変換
wstring ConvertEpgInfoText(const EPGDB_EVENT_INFO* info, const wstring* serviceName = NULL, const wstring* extraText = NULL);
wstring ConvertEpgInfoText2(const EPGDB_EVENT_INFO* info, const wstring& serviceName);

//フォルダパスから実際のドライブパスを取得
void GetChkDrivePath(wstring directoryPath, wstring& mountPath);

void GetGenreName(BYTE nibble1, BYTE nibble2, wstring& name);
void GetComponentTypeName(BYTE content, BYTE type, wstring& name);

void CopyEpgInfo(EPG_EVENT_INFO* destInfo, const EPGDB_EVENT_INFO* srcInfo);
void ConvertEpgInfo(WORD onid, WORD tsid, WORD sid, const EPG_EVENT_INFO* src, EPGDB_EVENT_INFO* dest);

#endif
