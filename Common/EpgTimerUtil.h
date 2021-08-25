#ifndef INCLUDE_EPG_TIMER_UTIL_H
#define INCLUDE_EPG_TIMER_UTIL_H

#include "StructDef.h"
#include "EpgDataCap3Def.h"

//チャンネルを__int64としてキーにする
static inline LONGLONG Create64Key(WORD onid, WORD tsid, WORD sid) { return sid | (DWORD)tsid << 16 | (LONGLONG)onid << 32; }
//EventIDをunsigned __int64としてキーにする
static inline ULONGLONG Create64PgKey(WORD onid, WORD tsid, WORD sid, WORD eid) { return eid | (DWORD)sid << 16 | (ULONGLONG)tsid << 32 | (ULONGLONG)onid << 48; }
//CRC32をもとめる
DWORD CalcCrc32(int n, const BYTE* c);
//MJD->I64Time変換
__int64 MJDtoI64Time(DWORD mjd, DWORD bcdTime);

//iniファイルから予想ビットレートを取得する
DWORD GetBitrateFromIni(WORD onid, WORD tsid, WORD sid);

//EPG情報をTextに変換
wstring ConvertEpgInfoText(const EPGDB_EVENT_INFO& info, const wstring* serviceName = NULL, const wstring* extraText = NULL);
wstring ConvertProgramText(const EPGDB_EVENT_INFO& info, const wstring& serviceName);
void AppendEpgContentInfoText(wstring& text, const EPGDB_EVENT_INFO& info);
void AppendEpgComponentInfoText(wstring& text, const EPGDB_EVENT_INFO& info);
void AppendEpgAudioComponentInfoText(wstring& text, const EPGDB_EVENT_INFO& info);

LPCWSTR GetGenreName(BYTE nibble1, BYTE nibble2);
LPCWSTR GetComponentTypeName(BYTE content, BYTE type);

void ConvertEpgInfo(WORD onid, WORD tsid, WORD sid, const EPG_EVENT_INFO* src, EPGDB_EVENT_INFO* dest);

class CEpgEventInfoAdapter
{
public:
	//EPGDB_EVENT_INFOを参照してEPG_EVENT_INFOを構築する
	EPG_EVENT_INFO Create(EPGDB_EVENT_INFO* ref);
private:
	EPG_SHORT_EVENT_INFO shortInfo;
	EPG_EXTENDED_EVENT_INFO extInfo;
	EPG_CONTEN_INFO contentInfo;
	EPG_COMPONENT_INFO componentInfo;
	EPG_AUDIO_COMPONENT_INFO audioInfo;
	vector<EPG_AUDIO_COMPONENT_INFO_DATA> audioList;
	EPG_EVENTGROUP_INFO eventGroupInfo;
	EPG_EVENTGROUP_INFO eventRelayInfo;
};

class CServiceInfoAdapter
{
public:
	//EPGDB_SERVICE_INFOを参照してSERVICE_INFOを構築する
	SERVICE_INFO Create(const EPGDB_SERVICE_INFO* ref);
private:
	SERVICE_EXT_INFO extInfo;
};

#endif
