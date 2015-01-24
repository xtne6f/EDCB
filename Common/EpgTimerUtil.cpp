#include "stdafx.h"
#include "EpgTimerUtil.h"

#include "PathUtil.h"
#include "StringUtil.h"
#include "TimeUtil.h"

#include <process.h>
#include <tlhelp32.h> 
#include <shlwapi.h>

LONGLONG _Create64Key( WORD OriginalNetworkID, WORD TransportStreamID, WORD ServiceID )
{
	LONGLONG Key = 
		(((LONGLONG)(OriginalNetworkID&0x0000FFFF))<<32) |
		(((LONGLONG)(TransportStreamID&0x0000FFFF))<<16) |
		((LONGLONG)(ServiceID&0x0000FFFF));
	return Key;
}

LONGLONG _Create64Key2( WORD OriginalNetworkID, WORD TransportStreamID, WORD ServiceID, WORD EventID )
{
	LONGLONG Key = 
		(((LONGLONG)(OriginalNetworkID & 0x0000FFFF))<<48) |
		(((LONGLONG)(TransportStreamID & 0x0000FFFF))<<32) |
		(((LONGLONG)(ServiceID & 0x0000FFFF))<<16) |
		((LONGLONG)(EventID & 0x0000FFFF));
	return Key;
}

#define CRCPOLY1 0x04C11DB7UL
typedef unsigned char byte;
static const unsigned long crctable[256] = {
	0x00000000, 0x04C11DB7, 0x09823B6E, 0x0D4326D9,	0x130476DC, 0x17C56B6B, 0x1A864DB2, 0x1E475005,
	0x2608EDB8, 0x22C9F00F, 0x2F8AD6D6, 0x2B4BCB61, 0x350C9B64, 0x31CD86D3, 0x3C8EA00A, 0x384FBDBD,
	0x4C11DB70, 0x48D0C6C7, 0x4593E01E, 0x4152FDA9,	0x5F15ADAC, 0x5BD4B01B, 0x569796C2, 0x52568B75, 
	0x6A1936C8, 0x6ED82B7F, 0x639B0DA6, 0x675A1011,	0x791D4014, 0x7DDC5DA3, 0x709F7B7A, 0x745E66CD,
	0x9823B6E0, 0x9CE2AB57, 0x91A18D8E, 0x95609039,	0x8B27C03C, 0x8FE6DD8B, 0x82A5FB52, 0x8664E6E5,
	0xBE2B5B58, 0xBAEA46EF, 0xB7A96036, 0xB3687D81,	0xAD2F2D84, 0xA9EE3033, 0xA4AD16EA, 0xA06C0B5D,
	0xD4326D90, 0xD0F37027, 0xDDB056FE, 0xD9714B49,	0xC7361B4C, 0xC3F706FB, 0xCEB42022, 0xCA753D95,
	0xF23A8028, 0xF6FB9D9F, 0xFBB8BB46, 0xFF79A6F1,	0xE13EF6F4, 0xE5FFEB43, 0xE8BCCD9A, 0xEC7DD02D,
	0x34867077, 0x30476DC0, 0x3D044B19, 0x39C556AE,	0x278206AB, 0x23431B1C, 0x2E003DC5, 0x2AC12072, 
	0x128E9DCF, 0x164F8078, 0x1B0CA6A1, 0x1FCDBB16,	0x018AEB13, 0x054BF6A4, 0x0808D07D, 0x0CC9CDCA,
	0x7897AB07, 0x7C56B6B0, 0x71159069, 0x75D48DDE,	0x6B93DDDB, 0x6F52C06C, 0x6211E6B5, 0x66D0FB02,
	0x5E9F46BF, 0x5A5E5B08, 0x571D7DD1, 0x53DC6066,	0x4D9B3063, 0x495A2DD4, 0x44190B0D, 0x40D816BA,
	0xACA5C697, 0xA864DB20, 0xA527FDF9, 0xA1E6E04E,	0xBFA1B04B, 0xBB60ADFC, 0xB6238B25, 0xB2E29692,
	0x8AAD2B2F, 0x8E6C3698, 0x832F1041, 0x87EE0DF6,	0x99A95DF3, 0x9D684044, 0x902B669D, 0x94EA7B2A,
	0xE0B41DE7, 0xE4750050, 0xE9362689, 0xEDF73B3E,	0xF3B06B3B, 0xF771768C, 0xFA325055, 0xFEF34DE2, 
	0xC6BCF05F, 0xC27DEDE8, 0xCF3ECB31, 0xCBFFD686,	0xD5B88683, 0xD1799B34, 0xDC3ABDED, 0xD8FBA05A,
	0x690CE0EE, 0x6DCDFD59, 0x608EDB80, 0x644FC637,	0x7A089632, 0x7EC98B85, 0x738AAD5C, 0x774BB0EB,
	0x4F040D56, 0x4BC510E1, 0x46863638, 0x42472B8F,	0x5C007B8A, 0x58C1663D, 0x558240E4, 0x51435D53, 
	0x251D3B9E, 0x21DC2629, 0x2C9F00F0, 0x285E1D47,	0x36194D42, 0x32D850F5, 0x3F9B762C, 0x3B5A6B9B,
	0x0315D626, 0x07D4CB91, 0x0A97ED48, 0x0E56F0FF,	0x1011A0FA, 0x14D0BD4D, 0x19939B94, 0x1D528623,
	0xF12F560E, 0xF5EE4BB9, 0xF8AD6D60, 0xFC6C70D7,	0xE22B20D2, 0xE6EA3D65, 0xEBA91BBC, 0xEF68060B, 
	0xD727BBB6, 0xD3E6A601, 0xDEA580D8, 0xDA649D6F,	0xC423CD6A, 0xC0E2D0DD, 0xCDA1F604, 0xC960EBB3,
	0xBD3E8D7E, 0xB9FF90C9, 0xB4BCB610, 0xB07DABA7,	0xAE3AFBA2, 0xAAFBE615, 0xA7B8C0CC, 0xA379DD7B,
	0x9B3660C6, 0x9FF77D71, 0x92B45BA8, 0x9675461F,	0x8832161A, 0x8CF30BAD, 0x81B02D74, 0x857130C3, 
	0x5D8A9099, 0x594B8D2E, 0x5408ABF7, 0x50C9B640,	0x4E8EE645, 0x4A4FFBF2, 0x470CDD2B, 0x43CDC09C,
	0x7B827D21, 0x7F436096, 0x7200464F, 0x76C15BF8,	0x68860BFD, 0x6C47164A, 0x61043093, 0x65C52D24,
	0x119B4BE9, 0x155A565E, 0x18197087, 0x1CD86D30,	0x029F3D35, 0x065E2082, 0x0B1D065B, 0x0FDC1BEC,
	0x3793A651, 0x3352BBE6, 0x3E119D3F, 0x3AD08088,	0x2497D08D, 0x2056CD3A, 0x2D15EBE3, 0x29D4F654,
	0xC5A92679, 0xC1683BCE, 0xCC2B1D17, 0xC8EA00A0,	0xD6AD50A5, 0xD26C4D12, 0xDF2F6BCB, 0xDBEE767C,
	0xE3A1CBC1, 0xE760D676, 0xEA23F0AF, 0xEEE2ED18,	0xF0A5BD1D, 0xF464A0AA, 0xF9278673, 0xFDE69BC4, 
	0x89B8FD09, 0x8D79E0BE, 0x803AC667, 0x84FBDBD0,	0x9ABC8BD5, 0x9E7D9662, 0x933EB0BB, 0x97FFAD0C,
	0xAFB010B1, 0xAB710D06, 0xA6322BDF, 0xA2F33668,	0xBCB4666D, 0xB8757BDA, 0xB5365D03, 0xB1F740B4
};

unsigned long _Crc32(int n, BYTE c[])
{
	unsigned long r;

	r = 0xFFFFFFFFUL;
	while (--n >= 0)
		r = (r << CHAR_BIT) ^ crctable[(byte)(r >> (32 - CHAR_BIT)) ^ *c++];
	return r & 0xFFFFFFFFUL;
}

LONGLONG _GetRecSize( DWORD OriginalNetworkID, DWORD TransportStreamID, DWORD ServiceID, BOOL ServiceOnlyFlag, DWORD DurationSecond )
{
	LONGLONG RecSize = 0;
	if( OriginalNetworkID == 4 ){
		//BS
		if( ServiceOnlyFlag == TRUE ){
			if( ServiceID == 101 || ServiceID == 102 ){
				//BS1、BS2
				//9Mbpsで計算
				RecSize = ( 9 * 1024 * 1024 * ((LONGLONG)DurationSecond) ) / 8;
			}if( ServiceID == 910 ){
				//WNI・910は2Mbps
				RecSize = ( 2 * 1024 * 1024 * ((LONGLONG)DurationSecond) ) / 8;
			}else{
				//18Mbpsで計算
				RecSize = ( 18 * 1024 * 1024 * ((LONGLONG)DurationSecond) ) / 8;
			}
		}else{
			//20Mbpsで計算
			RecSize = ( 20 * 1024 * 1024 * ((LONGLONG)DurationSecond) ) / 8;
		}
	}else if( OriginalNetworkID == 6 ){
		//CS1
		if( ServiceOnlyFlag == TRUE ){
			switch( ServiceID ){
				case 239: //日本映画専門ｃｈＨＤ
				case 306: //フジテレビＮＥＸＴ
				case 800: //スカチャンＨＤ８００
				case 55: //ショップチャンネル
					//13Mbpsで計算
					RecSize = ( 13 * 1024 * 1024 * ((LONGLONG)DurationSecond) ) / 8;
					break;
				default:
					//5Mbpsで計算
					RecSize = ( 5 * 1024 * 1024 * ((LONGLONG)DurationSecond) ) / 8;
					break;
			}
		}else{
			//40Mbpsで計算
			RecSize = ( 40 * 1024 * 1024 * ((LONGLONG)DurationSecond) ) / 8;
		}
	}else if( OriginalNetworkID == 7 ){
		//CS2
		if( ServiceOnlyFlag == TRUE ){
			switch( ServiceID ){
				case 240: //ムービープラスＨＤ
				case 253: //ＪスポーツＰｌｕｓＨ
				case 314: //ＬａＬａ　ＨＤ
					//13Mbpsで計算
					RecSize = ( 13 * 1024 * 1024 * ((LONGLONG)DurationSecond) ) / 8;
					break;
				case 257: //日テレＧ＋
				case 262: //ゴルフネットワーク
				case 290: //ＳＫＹ・ＳＴＡＧＥ
				case 300: //日テレプラス
				case 307: //フジテレビＯＮＥ
				case 308: //フジテレビＴＷＯ
				case 333: //ＡＴ－Ｘ
				case 350: //日テレＮＥＷＳ２４
				case 354: //ＣＮＮｊ
					//8Mbpsで計算
					RecSize = ( 5 * 1024 * 1024 * ((LONGLONG)DurationSecond) ) / 8;
					break;
				default:
					//5Mbpsで計算
					RecSize = ( 5 * 1024 * 1024 * ((LONGLONG)DurationSecond) ) / 8;
					break;
			}
		}else{
			//40Mbpsで計算
			RecSize = ( 40 * 1024 * 1024 * ((LONGLONG)DurationSecond) ) / 8;
		}
	}else{
		//地デジ
		if( ServiceOnlyFlag == TRUE ){
			//15Mbpsで計算
			RecSize = ( 15 * 1024 * 1024 * ((LONGLONG)DurationSecond) ) / 8;
		}else{
			//18Mbpsで計算
			RecSize = ( 18 * 1024 * 1024 * ((LONGLONG)DurationSecond) ) / 8;
		}
	}
	return RecSize;
}

BOOL _FindOpenExeProcess(DWORD processID)
{
	HANDLE hSnapshot;
	PROCESSENTRY32 procent;

	BOOL bFind = FALSE;
	/* Toolhelpスナップショットを作成する */
	hSnapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS,0 );
	if ( hSnapshot != (HANDLE)-1 ) {
		procent.dwSize = sizeof(PROCESSENTRY32);
		if ( Process32First( hSnapshot,&procent ) != FALSE ){
			do {
				if( procent.th32ProcessID == processID ){
					bFind = TRUE;
					break;
				}
			} while ( Process32Next( hSnapshot,&procent ) != FALSE );
		}
		CloseHandle( hSnapshot );
	}
	return bFind;
}

vector<DWORD> _FindPidListByExeName(LPCTSTR lpExeName)
{
	HANDLE hSnapshot;
	PROCESSENTRY32 procent;

	vector<DWORD> ret;
	/* Toolhelpスナップショットを作成する */
	hSnapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS,0 );
	if ( hSnapshot != (HANDLE)-1 ) {
		procent.dwSize = sizeof(PROCESSENTRY32);
		if ( Process32First( hSnapshot,&procent ) != FALSE ){
			do {
				if( lstrcmpi(procent.szExeFile, lpExeName) == 0 ){
					ret.push_back(procent.th32ProcessID);
				}
			} while ( Process32Next( hSnapshot,&procent ) != FALSE );
		}
		CloseHandle( hSnapshot );
	}
	return ret;
}

DWORD _BCDtoDWORD(BYTE* data, BYTE size, BYTE digit)
{
	if( data == NULL || (size<<1) < digit ){
		return 0;
	}
	DWORD value = 0;
	for( BYTE i=0; i<digit; i++ ){
		value = value*10;
		if( (i & 0x1) == 0 ){
			value += (data[i>>1]&0xF0)>>4;
		}else{
			value += (data[i>>1]&0x0F);
		}
	}
	return value;
}

BOOL _MJDtoYMD(DWORD mjd, WORD* y, WORD* m, WORD* d)
{
	if( y == NULL || m == NULL || d == NULL ){
		return FALSE;
	}
	
	int yy = (int)( ((double)mjd-15078.2)/365.25 );
	int mm = (int)( ((double)mjd-14956.1-(int)(yy*365.25))/30.6001 );
	*d = (WORD)( mjd-14956-(int)(yy*365.25)-(int)(mm*30.6001) );
	WORD k=0;
	if( mm == 14 || mm == 15 ){
		k=1;
	}

	*y = yy + k;
	*m = mm-1-k*12;

	return TRUE;
}

BOOL _MJDtoSYSTEMTIME(DWORD mjd, SYSTEMTIME* time)
{
	if( time == NULL ){
		return FALSE;
	}

	FILETIME fileTime;
	SYSTEMTIME mjdTime;
	LONGLONG oneDay = 24*60*60*(LONGLONG)10000000;

	ZeroMemory(&mjdTime, sizeof(SYSTEMTIME));
	mjdTime.wYear = 1858;
	mjdTime.wMonth = 11;
	mjdTime.wDay = 17;

	SystemTimeToFileTime(&mjdTime, &fileTime);
	LONGLONG tempTime = ((LONGLONG)fileTime.dwHighDateTime)<<32 | fileTime.dwLowDateTime;
	tempTime += (LONGLONG)mjd*oneDay;

	fileTime.dwLowDateTime = (DWORD)(tempTime&0x00000000FFFFFFFF);
	fileTime.dwHighDateTime = (DWORD)((tempTime&0xFFFFFFFF00000000)>>32);

	FileTimeToSystemTime(&fileTime, time);

	return TRUE;
}

BOOL _GetBitrate(WORD ONID, WORD TSID, WORD SID, DWORD* bitrate)
{
	wstring iniPath;
	GetModuleFolderPath(iniPath);

	iniPath += L"\\Bitrate.ini";

	wstring defKey = L"FFFFFFFFFFFF";
	wstring defNWKey = L"";
	Format(defNWKey, L"%04XFFFFFFFF", ONID);
	wstring defTSKey = L"";
	Format(defTSKey, L"%04X%04XFFFF", ONID, TSID);
	wstring key = L"";
	Format(key, L"%04X%04X%04X", ONID, TSID, SID);

	int defRate = GetPrivateProfileInt(L"BITRATE", defKey.c_str(), 0, iniPath.c_str());
	int defNWRate = GetPrivateProfileInt(L"BITRATE", defNWKey.c_str(), 0, iniPath.c_str());
	int defTSRate = GetPrivateProfileInt(L"BITRATE", defTSKey.c_str(), 0, iniPath.c_str());
	int rate = GetPrivateProfileInt(L"BITRATE", key.c_str(), 0, iniPath.c_str());

	if( rate != 0 ){
		*bitrate = (DWORD)rate;
		return TRUE;
	}
	if( defTSRate != 0 ){
		*bitrate = (DWORD)defTSRate;
		return TRUE;
	}
	if( defNWRate != 0 ){
		*bitrate = (DWORD)defNWRate;
		return TRUE;
	}
	if( defRate != 0 ){
		*bitrate = (DWORD)defRate;
		return TRUE;
	}
	*bitrate = 19*1024;
	return TRUE;
}

//EPG情報をTextに変換
void _ConvertEpgInfoText(EPGDB_EVENT_INFO* info, wstring& text)
{
	text = L"";
	if( info == NULL ){
		return ;
	}

	wstring time=L"未定";
	if( info->StartTimeFlag == TRUE && info->DurationFlag == TRUE ){
		GetTimeString3(info->start_time, info->durationSec, time);
	}else if( info->StartTimeFlag == TRUE && info->DurationFlag == FALSE ){
		GetTimeString4(info->start_time, time);
		time += L" ～ 未定";
	}
	text += time;
	text += L"\r\n";

	if(info->shortInfo != NULL ){
		text += info->shortInfo->event_name;
		text += L"\r\n\r\n";
		text += info->shortInfo->text_char;
		text += L"\r\n\r\n";
	}

	if(info->extInfo != NULL ){
		text += L"詳細情報\r\n";
		text += info->extInfo->text_char;
		text += L"\r\n\r\n";
	}

	wstring buff = L"";
	Format(buff, L"OriginalNetworkID:%d(0x%04X)\r\nTransportStreamID:%d(0x%04X)\r\nServiceID:%d(0x%04X)\r\nEventID:%d(0x%04X)\r\n",
		info->original_network_id, info->original_network_id,
		info->transport_stream_id, info->transport_stream_id,
		info->service_id, info->service_id,
		info->event_id, info->event_id);
	text += buff;
}

static map<WORD, wstring> contentKindMap;
static map<WORD, wstring> componentKindMap;
static vector<GENRU_INFO> genruList;

GENRU_INFO CreateGenruInfo(WORD key, wstring name){
	GENRU_INFO info;
	info.key = key;
	info.name = name;
	info.nibble1 = key>>8;
	info.nibble2 = (BYTE)(key&0x00FF);
	return info;
}

void CreateContentKindMap()
{
	contentKindMap.clear();

    contentKindMap.insert(pair<WORD,wstring>(0x00FF, L"ニュース／報道"));
    contentKindMap.insert(pair<WORD,wstring>(0x0000, L"定時・総合"));
    contentKindMap.insert(pair<WORD,wstring>(0x0001, L"天気"));
    contentKindMap.insert(pair<WORD,wstring>(0x0002, L"特集・ドキュメント"));
    contentKindMap.insert(pair<WORD,wstring>(0x0003, L"政治・国会"));
    contentKindMap.insert(pair<WORD,wstring>(0x0004, L"経済・市況"));
    contentKindMap.insert(pair<WORD,wstring>(0x0005, L"海外・国際"));
    contentKindMap.insert(pair<WORD,wstring>(0x0006, L"解説"));
    contentKindMap.insert(pair<WORD,wstring>(0x0007, L"討論・会談"));
    contentKindMap.insert(pair<WORD,wstring>(0x0008, L"報道特番"));
    contentKindMap.insert(pair<WORD,wstring>(0x0009, L"ローカル・地域"));
    contentKindMap.insert(pair<WORD,wstring>(0x000A, L"交通"));
    contentKindMap.insert(pair<WORD,wstring>(0x000F, L"その他"));

    contentKindMap.insert(pair<WORD,wstring>(0x01FF, L"スポーツ"));
    contentKindMap.insert(pair<WORD,wstring>(0x0100, L"スポーツニュース"));
    contentKindMap.insert(pair<WORD,wstring>(0x0101, L"野球"));
    contentKindMap.insert(pair<WORD,wstring>(0x0102, L"サッカー"));
    contentKindMap.insert(pair<WORD,wstring>(0x0103, L"ゴルフ"));
    contentKindMap.insert(pair<WORD,wstring>(0x0104, L"その他の球技"));
    contentKindMap.insert(pair<WORD,wstring>(0x0105, L"相撲・格闘技"));
    contentKindMap.insert(pair<WORD,wstring>(0x0106, L"オリンピック・国際大会"));
    contentKindMap.insert(pair<WORD,wstring>(0x0107, L"マラソン・陸上・水泳"));
    contentKindMap.insert(pair<WORD,wstring>(0x0108, L"モータースポーツ"));
    contentKindMap.insert(pair<WORD,wstring>(0x0109, L"マリン・ウィンタースポーツ"));
    contentKindMap.insert(pair<WORD,wstring>(0x010A, L"競馬・公営競技"));
    contentKindMap.insert(pair<WORD,wstring>(0x010F, L"その他"));

    contentKindMap.insert(pair<WORD,wstring>(0x02FF, L"情報／ワイドショー"));
    contentKindMap.insert(pair<WORD,wstring>(0x0200, L"芸能・ワイドショー"));
    contentKindMap.insert(pair<WORD,wstring>(0x0201, L"ファッション"));
    contentKindMap.insert(pair<WORD,wstring>(0x0202, L"暮らし・住まい"));
    contentKindMap.insert(pair<WORD,wstring>(0x0203, L"健康・医療"));
    contentKindMap.insert(pair<WORD,wstring>(0x0204, L"ショッピング・通販"));
    contentKindMap.insert(pair<WORD,wstring>(0x0205, L"グルメ・料理"));
    contentKindMap.insert(pair<WORD,wstring>(0x0206, L"イベント"));
    contentKindMap.insert(pair<WORD,wstring>(0x0207, L"番組紹介・お知らせ"));
    contentKindMap.insert(pair<WORD,wstring>(0x020F, L"その他"));

    contentKindMap.insert(pair<WORD,wstring>(0x03FF, L"ドラマ"));
    contentKindMap.insert(pair<WORD,wstring>(0x0300, L"国内ドラマ"));
    contentKindMap.insert(pair<WORD,wstring>(0x0301, L"海外ドラマ"));
    contentKindMap.insert(pair<WORD,wstring>(0x0302, L"時代劇"));
    contentKindMap.insert(pair<WORD,wstring>(0x030F, L"その他"));

    contentKindMap.insert(pair<WORD,wstring>(0x04FF, L"音楽"));
    contentKindMap.insert(pair<WORD,wstring>(0x0400, L"国内ロック・ポップス"));
    contentKindMap.insert(pair<WORD,wstring>(0x0401, L"海外ロック・ポップス"));
    contentKindMap.insert(pair<WORD,wstring>(0x0402, L"クラシック・オペラ"));
    contentKindMap.insert(pair<WORD,wstring>(0x0403, L"ジャズ・フュージョン"));
    contentKindMap.insert(pair<WORD,wstring>(0x0404, L"歌謡曲・演歌"));
    contentKindMap.insert(pair<WORD,wstring>(0x0405, L"ライブ・コンサート"));
    contentKindMap.insert(pair<WORD,wstring>(0x0406, L"ランキング・リクエスト"));
    contentKindMap.insert(pair<WORD,wstring>(0x0407, L"カラオケ・のど自慢"));
    contentKindMap.insert(pair<WORD,wstring>(0x0408, L"民謡・邦楽"));
    contentKindMap.insert(pair<WORD,wstring>(0x0409, L"童謡・キッズ"));
    contentKindMap.insert(pair<WORD,wstring>(0x040A, L"民族音楽・ワールドミュージック"));
    contentKindMap.insert(pair<WORD,wstring>(0x040F, L"その他"));

    contentKindMap.insert(pair<WORD,wstring>(0x05FF, L"バラエティ"));
    contentKindMap.insert(pair<WORD,wstring>(0x0500, L"クイズ"));
    contentKindMap.insert(pair<WORD,wstring>(0x0501, L"ゲーム"));
    contentKindMap.insert(pair<WORD,wstring>(0x0502, L"トークバラエティ"));
    contentKindMap.insert(pair<WORD,wstring>(0x0503, L"お笑い・コメディ"));
    contentKindMap.insert(pair<WORD,wstring>(0x0504, L"音楽バラエティ"));
    contentKindMap.insert(pair<WORD,wstring>(0x0505, L"旅バラエティ"));
    contentKindMap.insert(pair<WORD,wstring>(0x0506, L"料理バラエティ"));
    contentKindMap.insert(pair<WORD,wstring>(0x050F, L"その他"));

    contentKindMap.insert(pair<WORD,wstring>(0x06FF, L"映画"));
    contentKindMap.insert(pair<WORD,wstring>(0x0600, L"洋画"));
    contentKindMap.insert(pair<WORD,wstring>(0x0601, L"邦画"));
    contentKindMap.insert(pair<WORD,wstring>(0x0602, L"アニメ"));
    contentKindMap.insert(pair<WORD,wstring>(0x060F, L"その他"));

    contentKindMap.insert(pair<WORD,wstring>(0x07FF, L"アニメ／特撮"));
    contentKindMap.insert(pair<WORD,wstring>(0x0700, L"国内アニメ"));
    contentKindMap.insert(pair<WORD,wstring>(0x0701, L"海外アニメ"));
    contentKindMap.insert(pair<WORD,wstring>(0x0702, L"特撮"));
    contentKindMap.insert(pair<WORD,wstring>(0x070F, L"その他"));

    contentKindMap.insert(pair<WORD,wstring>(0x08FF, L"ドキュメンタリー／教養"));
    contentKindMap.insert(pair<WORD,wstring>(0x0800, L"社会・時事"));
    contentKindMap.insert(pair<WORD,wstring>(0x0801, L"歴史・紀行"));
    contentKindMap.insert(pair<WORD,wstring>(0x0802, L"自然・動物・環境"));
    contentKindMap.insert(pair<WORD,wstring>(0x0803, L"宇宙・科学・医学"));
    contentKindMap.insert(pair<WORD,wstring>(0x0804, L"カルチャー・伝統文化"));
    contentKindMap.insert(pair<WORD,wstring>(0x0805, L"文学・文芸"));
    contentKindMap.insert(pair<WORD,wstring>(0x0806, L"スポーツ"));
    contentKindMap.insert(pair<WORD,wstring>(0x0807, L"ドキュメンタリー全般"));
    contentKindMap.insert(pair<WORD,wstring>(0x0808, L"インタビュー・討論"));
    contentKindMap.insert(pair<WORD,wstring>(0x080F, L"その他"));

    contentKindMap.insert(pair<WORD,wstring>(0x09FF, L"劇場／公演"));
    contentKindMap.insert(pair<WORD,wstring>(0x0900, L"現代劇・新劇"));
    contentKindMap.insert(pair<WORD,wstring>(0x0901, L"ミュージカル"));
    contentKindMap.insert(pair<WORD,wstring>(0x0902, L"ダンス・バレエ"));
    contentKindMap.insert(pair<WORD,wstring>(0x0903, L"落語・演芸"));
    contentKindMap.insert(pair<WORD,wstring>(0x0904, L"歌舞伎・古典"));
    contentKindMap.insert(pair<WORD,wstring>(0x090F, L"その他"));

    contentKindMap.insert(pair<WORD,wstring>(0x0AFF, L"趣味／教育"));
    contentKindMap.insert(pair<WORD,wstring>(0x0A00, L"旅・釣り・アウトドア"));
    contentKindMap.insert(pair<WORD,wstring>(0x0A01, L"園芸・ペット・手芸"));
    contentKindMap.insert(pair<WORD,wstring>(0x0A02, L"音楽・美術・工芸"));
    contentKindMap.insert(pair<WORD,wstring>(0x0A03, L"囲碁・将棋"));
    contentKindMap.insert(pair<WORD,wstring>(0x0A04, L"麻雀・パチンコ"));
    contentKindMap.insert(pair<WORD,wstring>(0x0A05, L"車・オートバイ"));
    contentKindMap.insert(pair<WORD,wstring>(0x0A06, L"コンピュータ・ＴＶゲーム"));
    contentKindMap.insert(pair<WORD,wstring>(0x0A07, L"会話・語学"));
    contentKindMap.insert(pair<WORD,wstring>(0x0A08, L"幼児・小学生"));
    contentKindMap.insert(pair<WORD,wstring>(0x0A09, L"中学生・高校生"));
    contentKindMap.insert(pair<WORD,wstring>(0x0A0A, L"大学生・受験"));
    contentKindMap.insert(pair<WORD,wstring>(0x0A0B, L"生涯教育・資格"));
    contentKindMap.insert(pair<WORD,wstring>(0x0A0C, L"教育問題"));
    contentKindMap.insert(pair<WORD,wstring>(0x0A0F, L"その他"));

    contentKindMap.insert(pair<WORD,wstring>(0x0BFF, L"福祉"));
    contentKindMap.insert(pair<WORD,wstring>(0x0B00, L"高齢者"));
    contentKindMap.insert(pair<WORD,wstring>(0x0B01, L"障害者"));
    contentKindMap.insert(pair<WORD,wstring>(0x0B02, L"社会福祉"));
    contentKindMap.insert(pair<WORD,wstring>(0x0B03, L"ボランティア"));
    contentKindMap.insert(pair<WORD,wstring>(0x0B04, L"手話"));
    contentKindMap.insert(pair<WORD,wstring>(0x0B05, L"文字（字幕）"));
    contentKindMap.insert(pair<WORD,wstring>(0x0B06, L"音声解説"));
    contentKindMap.insert(pair<WORD,wstring>(0x0B0F, L"その他"));

    contentKindMap.insert(pair<WORD,wstring>(0x0FFF, L"その他"));
    contentKindMap.insert(pair<WORD,wstring>(0xFFFF, L"なし"));


	genruList.clear();
    genruList.push_back(CreateGenruInfo(0x00FF, L"ニュース／報道"));
    genruList.push_back(CreateGenruInfo(0x0000, L"定時・総合"));
    genruList.push_back(CreateGenruInfo(0x0001, L"天気"));
    genruList.push_back(CreateGenruInfo(0x0002, L"特集・ドキュメント"));
    genruList.push_back(CreateGenruInfo(0x0003, L"政治・国会"));
    genruList.push_back(CreateGenruInfo(0x0004, L"経済・市況"));
    genruList.push_back(CreateGenruInfo(0x0005, L"海外・国際"));
    genruList.push_back(CreateGenruInfo(0x0006, L"解説"));
    genruList.push_back(CreateGenruInfo(0x0007, L"討論・会談"));
    genruList.push_back(CreateGenruInfo(0x0008, L"報道特番"));
    genruList.push_back(CreateGenruInfo(0x0009, L"ローカル・地域"));
    genruList.push_back(CreateGenruInfo(0x000A, L"交通"));
    genruList.push_back(CreateGenruInfo(0x000F, L"その他"));

    genruList.push_back(CreateGenruInfo(0x01FF, L"スポーツ"));
    genruList.push_back(CreateGenruInfo(0x0100, L"スポーツニュース"));
    genruList.push_back(CreateGenruInfo(0x0101, L"野球"));
    genruList.push_back(CreateGenruInfo(0x0102, L"サッカー"));
    genruList.push_back(CreateGenruInfo(0x0103, L"ゴルフ"));
    genruList.push_back(CreateGenruInfo(0x0104, L"その他の球技"));
    genruList.push_back(CreateGenruInfo(0x0105, L"相撲・格闘技"));
    genruList.push_back(CreateGenruInfo(0x0106, L"オリンピック・国際大会"));
    genruList.push_back(CreateGenruInfo(0x0107, L"マラソン・陸上・水泳"));
    genruList.push_back(CreateGenruInfo(0x0108, L"モータースポーツ"));
    genruList.push_back(CreateGenruInfo(0x0109, L"マリン・ウィンタースポーツ"));
    genruList.push_back(CreateGenruInfo(0x010A, L"競馬・公営競技"));
    genruList.push_back(CreateGenruInfo(0x010F, L"その他"));

    genruList.push_back(CreateGenruInfo(0x02FF, L"情報／ワイドショー"));
    genruList.push_back(CreateGenruInfo(0x0200, L"芸能・ワイドショー"));
    genruList.push_back(CreateGenruInfo(0x0201, L"ファッション"));
    genruList.push_back(CreateGenruInfo(0x0202, L"暮らし・住まい"));
    genruList.push_back(CreateGenruInfo(0x0203, L"健康・医療"));
    genruList.push_back(CreateGenruInfo(0x0204, L"ショッピング・通販"));
    genruList.push_back(CreateGenruInfo(0x0205, L"グルメ・料理"));
    genruList.push_back(CreateGenruInfo(0x0206, L"イベント"));
    genruList.push_back(CreateGenruInfo(0x0207, L"番組紹介・お知らせ"));
    genruList.push_back(CreateGenruInfo(0x020F, L"その他"));

    genruList.push_back(CreateGenruInfo(0x03FF, L"ドラマ"));
    genruList.push_back(CreateGenruInfo(0x0300, L"国内ドラマ"));
    genruList.push_back(CreateGenruInfo(0x0301, L"海外ドラマ"));
    genruList.push_back(CreateGenruInfo(0x0302, L"時代劇"));
    genruList.push_back(CreateGenruInfo(0x030F, L"その他"));

    genruList.push_back(CreateGenruInfo(0x04FF, L"音楽"));
    genruList.push_back(CreateGenruInfo(0x0400, L"国内ロック・ポップス"));
    genruList.push_back(CreateGenruInfo(0x0401, L"海外ロック・ポップス"));
    genruList.push_back(CreateGenruInfo(0x0402, L"クラシック・オペラ"));
    genruList.push_back(CreateGenruInfo(0x0403, L"ジャズ・フュージョン"));
    genruList.push_back(CreateGenruInfo(0x0404, L"歌謡曲・演歌"));
    genruList.push_back(CreateGenruInfo(0x0405, L"ライブ・コンサート"));
    genruList.push_back(CreateGenruInfo(0x0406, L"ランキング・リクエスト"));
    genruList.push_back(CreateGenruInfo(0x0407, L"カラオケ・のど自慢"));
    genruList.push_back(CreateGenruInfo(0x0408, L"民謡・邦楽"));
    genruList.push_back(CreateGenruInfo(0x0409, L"童謡・キッズ"));
    genruList.push_back(CreateGenruInfo(0x040A, L"民族音楽・ワールドミュージック"));
    genruList.push_back(CreateGenruInfo(0x040F, L"その他"));

    genruList.push_back(CreateGenruInfo(0x05FF, L"バラエティ"));
    genruList.push_back(CreateGenruInfo(0x0500, L"クイズ"));
    genruList.push_back(CreateGenruInfo(0x0501, L"ゲーム"));
    genruList.push_back(CreateGenruInfo(0x0502, L"トークバラエティ"));
    genruList.push_back(CreateGenruInfo(0x0503, L"お笑い・コメディ"));
    genruList.push_back(CreateGenruInfo(0x0504, L"音楽バラエティ"));
    genruList.push_back(CreateGenruInfo(0x0505, L"旅バラエティ"));
    genruList.push_back(CreateGenruInfo(0x0506, L"料理バラエティ"));
    genruList.push_back(CreateGenruInfo(0x050F, L"その他"));

    genruList.push_back(CreateGenruInfo(0x06FF, L"映画"));
    genruList.push_back(CreateGenruInfo(0x0600, L"洋画"));
    genruList.push_back(CreateGenruInfo(0x0601, L"邦画"));
    genruList.push_back(CreateGenruInfo(0x0602, L"アニメ"));
    genruList.push_back(CreateGenruInfo(0x060F, L"その他"));

    genruList.push_back(CreateGenruInfo(0x07FF, L"アニメ／特撮"));
    genruList.push_back(CreateGenruInfo(0x0700, L"国内アニメ"));
    genruList.push_back(CreateGenruInfo(0x0701, L"海外アニメ"));
    genruList.push_back(CreateGenruInfo(0x0702, L"特撮"));
    genruList.push_back(CreateGenruInfo(0x070F, L"その他"));

    genruList.push_back(CreateGenruInfo(0x08FF, L"ドキュメンタリー／教養"));
    genruList.push_back(CreateGenruInfo(0x0800, L"社会・時事"));
    genruList.push_back(CreateGenruInfo(0x0801, L"歴史・紀行"));
    genruList.push_back(CreateGenruInfo(0x0802, L"自然・動物・環境"));
    genruList.push_back(CreateGenruInfo(0x0803, L"宇宙・科学・医学"));
    genruList.push_back(CreateGenruInfo(0x0804, L"カルチャー・伝統文化"));
    genruList.push_back(CreateGenruInfo(0x0805, L"文学・文芸"));
    genruList.push_back(CreateGenruInfo(0x0806, L"スポーツ"));
    genruList.push_back(CreateGenruInfo(0x0807, L"ドキュメンタリー全般"));
    genruList.push_back(CreateGenruInfo(0x0808, L"インタビュー・討論"));
    genruList.push_back(CreateGenruInfo(0x080F, L"その他"));

    genruList.push_back(CreateGenruInfo(0x09FF, L"劇場／公演"));
    genruList.push_back(CreateGenruInfo(0x0900, L"現代劇・新劇"));
    genruList.push_back(CreateGenruInfo(0x0901, L"ミュージカル"));
    genruList.push_back(CreateGenruInfo(0x0902, L"ダンス・バレエ"));
    genruList.push_back(CreateGenruInfo(0x0903, L"落語・演芸"));
    genruList.push_back(CreateGenruInfo(0x0904, L"歌舞伎・古典"));
    genruList.push_back(CreateGenruInfo(0x090F, L"その他"));

    genruList.push_back(CreateGenruInfo(0x0AFF, L"趣味／教育"));
    genruList.push_back(CreateGenruInfo(0x0A00, L"旅・釣り・アウトドア"));
    genruList.push_back(CreateGenruInfo(0x0A01, L"園芸・ペット・手芸"));
    genruList.push_back(CreateGenruInfo(0x0A02, L"音楽・美術・工芸"));
    genruList.push_back(CreateGenruInfo(0x0A03, L"囲碁・将棋"));
    genruList.push_back(CreateGenruInfo(0x0A04, L"麻雀・パチンコ"));
    genruList.push_back(CreateGenruInfo(0x0A05, L"車・オートバイ"));
    genruList.push_back(CreateGenruInfo(0x0A06, L"コンピュータ・ＴＶゲーム"));
    genruList.push_back(CreateGenruInfo(0x0A07, L"会話・語学"));
    genruList.push_back(CreateGenruInfo(0x0A08, L"幼児・小学生"));
    genruList.push_back(CreateGenruInfo(0x0A09, L"中学生・高校生"));
    genruList.push_back(CreateGenruInfo(0x0A0A, L"大学生・受験"));
    genruList.push_back(CreateGenruInfo(0x0A0B, L"生涯教育・資格"));
    genruList.push_back(CreateGenruInfo(0x0A0C, L"教育問題"));
    genruList.push_back(CreateGenruInfo(0x0A0F, L"その他"));

    genruList.push_back(CreateGenruInfo(0x0BFF, L"福祉"));
    genruList.push_back(CreateGenruInfo(0x0B00, L"高齢者"));
    genruList.push_back(CreateGenruInfo(0x0B01, L"障害者"));
    genruList.push_back(CreateGenruInfo(0x0B02, L"社会福祉"));
    genruList.push_back(CreateGenruInfo(0x0B03, L"ボランティア"));
    genruList.push_back(CreateGenruInfo(0x0B04, L"手話"));
    genruList.push_back(CreateGenruInfo(0x0B05, L"文字（字幕）"));
    genruList.push_back(CreateGenruInfo(0x0B06, L"音声解説"));
    genruList.push_back(CreateGenruInfo(0x0B0F, L"その他"));

    genruList.push_back(CreateGenruInfo(0x0FFF, L"その他"));

}
void CreateComponentKindMap()
{
	componentKindMap.clear();

    componentKindMap.insert(pair<WORD,wstring>(0x0101, L"480i(525i)、アスペクト比4:3"));
    componentKindMap.insert(pair<WORD,wstring>(0x0102, L"480i(525i)、アスペクト比16:9 パンベクトルあり"));
    componentKindMap.insert(pair<WORD,wstring>(0x0103, L"480i(525i)、アスペクト比16:9 パンベクトルなし"));
    componentKindMap.insert(pair<WORD,wstring>(0x0104, L"480i(525i)、アスペクト比 > 16:9"));
    componentKindMap.insert(pair<WORD,wstring>(0x0191, L"2160p、アスペクト比4:3"));
    componentKindMap.insert(pair<WORD,wstring>(0x0192, L"2160p、アスペクト比16:9 パンベクトルあり"));
    componentKindMap.insert(pair<WORD,wstring>(0x0193, L"2160p、アスペクト比16:9 パンベクトルなし"));
    componentKindMap.insert(pair<WORD,wstring>(0x0194, L"2160p、アスペクト比 > 16:9"));
    componentKindMap.insert(pair<WORD,wstring>(0x01A1, L"480p(525p)、アスペクト比4:3"));
    componentKindMap.insert(pair<WORD,wstring>(0x01A2, L"480p(525p)、アスペクト比16:9 パンベクトルあり"));
    componentKindMap.insert(pair<WORD,wstring>(0x01A3, L"480p(525p)、アスペクト比16:9 パンベクトルなし"));
    componentKindMap.insert(pair<WORD,wstring>(0x01A4, L"480p(525p)、アスペクト比 > 16:9"));
    componentKindMap.insert(pair<WORD,wstring>(0x01B1, L"1080i(1125i)、アスペクト比4:3"));
    componentKindMap.insert(pair<WORD,wstring>(0x01B2, L"1080i(1125i)、アスペクト比16:9 パンベクトルあり"));
    componentKindMap.insert(pair<WORD,wstring>(0x01B3, L"1080i(1125i)、アスペクト比16:9 パンベクトルなし"));
    componentKindMap.insert(pair<WORD,wstring>(0x01B4, L"1080i(1125i)、アスペクト比 > 16:9"));
    componentKindMap.insert(pair<WORD,wstring>(0x01C1, L"720p(750p)、アスペクト比4:3"));
    componentKindMap.insert(pair<WORD,wstring>(0x01C2, L"720p(750p)、アスペクト比16:9 パンベクトルあり"));
    componentKindMap.insert(pair<WORD,wstring>(0x01C3, L"720p(750p)、アスペクト比16:9 パンベクトルなし"));
    componentKindMap.insert(pair<WORD,wstring>(0x01C4, L"720p(750p)、アスペクト比 > 16:9"));
    componentKindMap.insert(pair<WORD,wstring>(0x01D1, L"240p アスペクト比4:3"));
    componentKindMap.insert(pair<WORD,wstring>(0x01D2, L"240p アスペクト比16:9 パンベクトルあり"));
    componentKindMap.insert(pair<WORD,wstring>(0x01D3, L"240p アスペクト比16:9 パンベクトルなし"));
    componentKindMap.insert(pair<WORD,wstring>(0x01D4, L"240p アスペクト比 > 16:9"));
    componentKindMap.insert(pair<WORD,wstring>(0x01E1, L"1080p(1125p)、アスペクト比4:3"));
    componentKindMap.insert(pair<WORD,wstring>(0x01E2, L"1080p(1125p)、アスペクト比16:9 パンベクトルあり"));
    componentKindMap.insert(pair<WORD,wstring>(0x01E3, L"1080p(1125p)、アスペクト比16:9 パンベクトルなし"));
    componentKindMap.insert(pair<WORD,wstring>(0x01E4, L"1080p(1125p)、アスペクト比 > 16:9"));
    componentKindMap.insert(pair<WORD,wstring>(0x0201, L"1/0モード（シングルモノ）"));
    componentKindMap.insert(pair<WORD,wstring>(0x0202, L"1/0＋1/0モード（デュアルモノ）"));
    componentKindMap.insert(pair<WORD,wstring>(0x0203, L"2/0モード（ステレオ）"));
    componentKindMap.insert(pair<WORD,wstring>(0x0204, L"2/1モード"));
    componentKindMap.insert(pair<WORD,wstring>(0x0205, L"3/0モード"));
    componentKindMap.insert(pair<WORD,wstring>(0x0206, L"2/2モード"));
    componentKindMap.insert(pair<WORD,wstring>(0x0207, L"3/1モード"));
    componentKindMap.insert(pair<WORD,wstring>(0x0208, L"3/2モード"));
    componentKindMap.insert(pair<WORD,wstring>(0x0209, L"3/2＋LFEモード（3/2.1モード）"));
    componentKindMap.insert(pair<WORD,wstring>(0x020A, L"3/3.1モード"));
    componentKindMap.insert(pair<WORD,wstring>(0x020B, L"2/0/0-2/0/2-0.1モード"));
    componentKindMap.insert(pair<WORD,wstring>(0x020C, L"5/2.1モード"));
    componentKindMap.insert(pair<WORD,wstring>(0x020D, L"3/2/2.1モード"));
    componentKindMap.insert(pair<WORD,wstring>(0x020E, L"2/0/0-3/0/2-0.1モード"));
    componentKindMap.insert(pair<WORD,wstring>(0x020F, L"0/2/0-3/0/2-0.1モード"));
    componentKindMap.insert(pair<WORD,wstring>(0x0210, L"2/0/0-3/2/3-0.2モード"));
    componentKindMap.insert(pair<WORD,wstring>(0x0211, L"3/3/3-5/2/3-3/0/0.2モード"));
    componentKindMap.insert(pair<WORD,wstring>(0x0240, L"視覚障害者用音声解説"));
    componentKindMap.insert(pair<WORD,wstring>(0x0241, L"聴覚障害者用音声"));
    componentKindMap.insert(pair<WORD,wstring>(0x0501, L"H.264|MPEG-4 AVC、480i(525i)、アスペクト比4:3"));
    componentKindMap.insert(pair<WORD,wstring>(0x0502, L"H.264|MPEG-4 AVC、480i(525i)、アスペクト比16:9 パンベクトルあり"));
    componentKindMap.insert(pair<WORD,wstring>(0x0503, L"H.264|MPEG-4 AVC、480i(525i)、アスペクト比16:9 パンベクトルなし"));
    componentKindMap.insert(pair<WORD,wstring>(0x0504, L"H.264|MPEG-4 AVC、480i(525i)、アスペクト比 > 16:9"));
    componentKindMap.insert(pair<WORD,wstring>(0x0591, L"H.264|MPEG-4 AVC、2160p、アスペクト比4:3"));
    componentKindMap.insert(pair<WORD,wstring>(0x0592, L"H.264|MPEG-4 AVC、2160p、アスペクト比16:9 パンベクトルあり"));
    componentKindMap.insert(pair<WORD,wstring>(0x0593, L"H.264|MPEG-4 AVC、2160p、アスペクト比16:9 パンベクトルなし"));
    componentKindMap.insert(pair<WORD,wstring>(0x0594, L"H.264|MPEG-4 AVC、2160p、アスペクト比 > 16:9"));
    componentKindMap.insert(pair<WORD,wstring>(0x05A1, L"H.264|MPEG-4 AVC、480p(525p)、アスペクト比4:3"));
    componentKindMap.insert(pair<WORD,wstring>(0x05A2, L"H.264|MPEG-4 AVC、480p(525p)、アスペクト比16:9 パンベクトルあり"));
    componentKindMap.insert(pair<WORD,wstring>(0x05A3, L"H.264|MPEG-4 AVC、480p(525p)、アスペクト比16:9 パンベクトルなし"));
    componentKindMap.insert(pair<WORD,wstring>(0x05A4, L"H.264|MPEG-4 AVC、480p(525p)、アスペクト比 > 16:9"));
    componentKindMap.insert(pair<WORD,wstring>(0x05B1, L"H.264|MPEG-4 AVC、1080i(1125i)、アスペクト比4:3"));
    componentKindMap.insert(pair<WORD,wstring>(0x05B2, L"H.264|MPEG-4 AVC、1080i(1125i)、アスペクト比16:9 パンベクトルあり"));
    componentKindMap.insert(pair<WORD,wstring>(0x05B3, L"H.264|MPEG-4 AVC、1080i(1125i)、アスペクト比16:9 パンベクトルなし"));
    componentKindMap.insert(pair<WORD,wstring>(0x05B4, L"H.264|MPEG-4 AVC、1080i(1125i)、アスペクト比 > 16:9"));
    componentKindMap.insert(pair<WORD,wstring>(0x05C1, L"H.264|MPEG-4 AVC、720p(750p)、アスペクト比4:3"));
    componentKindMap.insert(pair<WORD,wstring>(0x05C2, L"H.264|MPEG-4 AVC、720p(750p)、アスペクト比16:9 パンベクトルあり"));
    componentKindMap.insert(pair<WORD,wstring>(0x05C3, L"H.264|MPEG-4 AVC、720p(750p)、アスペクト比16:9 パンベクトルなし"));
    componentKindMap.insert(pair<WORD,wstring>(0x05C4, L"H.264|MPEG-4 AVC、720p(750p)、アスペクト比 > 16:9"));
    componentKindMap.insert(pair<WORD,wstring>(0x05D1, L"H.264|MPEG-4 AVC、240p アスペクト比4:3"));
    componentKindMap.insert(pair<WORD,wstring>(0x05D2, L"H.264|MPEG-4 AVC、240p アスペクト比16:9 パンベクトルあり"));
    componentKindMap.insert(pair<WORD,wstring>(0x05D3, L"H.264|MPEG-4 AVC、240p アスペクト比16:9 パンベクトルなし"));
    componentKindMap.insert(pair<WORD,wstring>(0x05D4, L"H.264|MPEG-4 AVC、240p アスペクト比 > 16:9"));
    componentKindMap.insert(pair<WORD,wstring>(0x05E1, L"H.264|MPEG-4 AVC、1080p(1125p)、アスペクト比4:3"));
    componentKindMap.insert(pair<WORD,wstring>(0x05E2, L"H.264|MPEG-4 AVC、1080p(1125p)、アスペクト比16:9 パンベクトルあり"));
    componentKindMap.insert(pair<WORD,wstring>(0x05E3, L"H.264|MPEG-4 AVC、1080p(1125p)、アスペクト比16:9 パンベクトルなし"));
    componentKindMap.insert(pair<WORD,wstring>(0x05E4, L"H.264|MPEG-4 AVC、1080p(1125p)、アスペクト比 > 16:9"));

}

//EPG情報をTextに変換
void _ConvertEpgInfoText2(EPGDB_EVENT_INFO* info, wstring& text, wstring serviceName)
{
	if( contentKindMap.size() == 0 ){
		CreateContentKindMap();
	}
	if( componentKindMap.size() == 0 ){
		CreateComponentKindMap();
	}
	text = L"";
	if( info == NULL ){
		return ;
	}

	wstring time=L"未定";
	if( info->StartTimeFlag == TRUE && info->DurationFlag == TRUE ){
		GetTimeString3(info->start_time, info->durationSec, time);
	}else if( info->StartTimeFlag == TRUE && info->DurationFlag == FALSE ){
		GetTimeString4(info->start_time, time);
		time += L" ～ 未定";
	}
	text += time;
	text += L"\r\n";
	text += serviceName;
	text += L"\r\n";

	if(info->shortInfo != NULL ){
		text += info->shortInfo->event_name;
		text += L"\r\n\r\n";
		text += info->shortInfo->text_char;
		text += L"\r\n\r\n";
	}

	if(info->extInfo != NULL ){
		text += L"詳細情報\r\n";
		text += info->extInfo->text_char;
		text += L"\r\n\r\n";
	}

	if( info->contentInfo != NULL ){
		text+=L"ジャンル : \r\n";
		for( size_t i=0; i<info->contentInfo->nibbleList.size(); i++ ){
			WORD key1 = ((WORD)info->contentInfo->nibbleList[i].content_nibble_level_1) << 8 | 0xFF;
			WORD key2 = ((WORD)info->contentInfo->nibbleList[i].content_nibble_level_1) << 8 | info->contentInfo->nibbleList[i].content_nibble_level_2;
			map<WORD, wstring>::iterator itr;
			itr = contentKindMap.find(key1);
			if( itr != contentKindMap.end()){
				text+=itr->second;
				itr = contentKindMap.find(key2);
				if( itr != contentKindMap.end()){
					text+=L" - ";
					text+=itr->second;
				}
			}
			text+=L"\r\n";
		}
		text+=L"\r\n";
	}

	if( info->componentInfo != NULL ){
		text+=L"映像 : ";
		WORD key = ((WORD)info->componentInfo->stream_content) << 8 | info->componentInfo->component_type;
		map<WORD, wstring>::iterator itr;
		itr = componentKindMap.find(key);
		if( itr != componentKindMap.end()){
			text+=itr->second;
			if( info->componentInfo->text_char.size() > 0 ){
				text+=L"\r\n";
				text+=info->componentInfo->text_char;
			}
		}
		text+=L"\r\n";
	}

	if( info->audioInfo != NULL ){
		text+=L"音声 : ";
		for( size_t i=0; i<info->audioInfo->componentList.size(); i++ ){
			WORD key = ((WORD)info->audioInfo->componentList[i].stream_content) << 8 | info->audioInfo->componentList[i].component_type;
			map<WORD, wstring>::iterator itr;
			itr = componentKindMap.find(key);
			if( itr != componentKindMap.end()){
				text+=itr->second;
				if( info->audioInfo->componentList[i].text_char.size() > 0 ){
					text+=L"\r\n";
					text+=info->audioInfo->componentList[i].text_char;
				}
			}
			text+=L"\r\n";

			text+=L"サンプリングレート : ";
			switch(info->audioInfo->componentList[i].sampling_rate){
				case 0x01:
					text+= L"16kHz";
					break;
				case 0x02:
					text+= L"22.05kHz";
					break;
				case 0x03:
					text+= L"24kHz";
					break;
				case 0x05:
					text+= L"32kHz";
					break;
				case 0x06:
					text+= L"44.1kHz";
					break;
				case 0x07:
					text+= L"48kHz";
					break;
			}
			text+=L"\r\n";
		}
	}

	text+=L"\r\n";
	if (!(0x7880 <= info->original_network_id && info->original_network_id <= 0x7FE8)){
		if (info->freeCAFlag == 0)
        {
            text += L"無料放送\r\n";
        }
        else
        {
            text += L"有料放送\r\n";
        }
        text += L"\r\n";
    }

	wstring buff = L"";
	Format(buff, L"OriginalNetworkID:%d(0x%04X)\r\nTransportStreamID:%d(0x%04X)\r\nServiceID:%d(0x%04X)\r\nEventID:%d(0x%04X)\r\n",
		info->original_network_id, info->original_network_id,
		info->transport_stream_id, info->transport_stream_id,
		info->service_id, info->service_id,
		info->event_id, info->event_id);
	text += buff;
}

void GetChkDrivePath(wstring directoryPath, wstring& mountPath)
{
	WCHAR szVolumePathName[MAX_PATH] = L"";
	if( GetVolumePathName( directoryPath.c_str(), szVolumePathName, MAX_PATH) == FALSE ){
		mountPath = directoryPath;
		return ;
	}
	WCHAR szMount[MAX_PATH] = L"";
	if( GetVolumeNameForVolumeMountPoint(szVolumePathName, szMount, MAX_PATH) == FALSE ){
		mountPath = szVolumePathName;
		return ;
	}
	mountPath = szMount;
}

void GetGenreName(BYTE nibble1, BYTE nibble2, wstring& name)
{
	if( contentKindMap.size() == 0 ){
		CreateContentKindMap();
	}
	WORD key = ((WORD)nibble1)<<8 | nibble2;
	map<WORD, wstring>::iterator itr;
	itr = contentKindMap.find(key);
	if( itr != contentKindMap.end()){
		name = itr->second;
	}
}

void CopyEpgInfo(EPG_EVENT_INFO* destInfo, EPGDB_EVENT_INFO* srcInfo)
{
	destInfo->event_id = srcInfo->event_id;
	destInfo->StartTimeFlag = srcInfo->StartTimeFlag;
	destInfo->start_time = srcInfo->start_time;
	destInfo->DurationFlag = srcInfo->DurationFlag;
	destInfo->durationSec = srcInfo->durationSec;
	destInfo->freeCAFlag = srcInfo->freeCAFlag;

	if( srcInfo->shortInfo != NULL ){
		EPG_SHORT_EVENT_INFO* item = new EPG_SHORT_EVENT_INFO;
		destInfo->shortInfo = item;

		item->event_nameLength = (WORD)srcInfo->shortInfo->event_name.size();
		item->event_name = new WCHAR[item->event_nameLength+1];
		ZeroMemory(item->event_name, sizeof(WCHAR)*(item->event_nameLength+1));
		if( item->event_nameLength > 0 ){
			wcscpy_s(item->event_name, item->event_nameLength+1, srcInfo->shortInfo->event_name.c_str());
		}

		item->text_charLength = (WORD)srcInfo->shortInfo->text_char.size();
		item->text_char = new WCHAR[item->text_charLength+1];
		ZeroMemory(item->text_char, sizeof(WCHAR)*(item->text_charLength+1));
		if( item->text_charLength > 0 ){
			wcscpy_s(item->text_char, item->text_charLength+1, srcInfo->shortInfo->text_char.c_str());
		}
	}

	if( srcInfo->extInfo != NULL ){
		EPG_EXTENDED_EVENT_INFO* item = new EPG_EXTENDED_EVENT_INFO;
		destInfo->extInfo = item;

		item->text_charLength = (WORD)srcInfo->extInfo->text_char.size();
		item->text_char = new WCHAR[item->text_charLength+1];
		ZeroMemory(item->text_char, sizeof(WCHAR)*(item->text_charLength+1));
		if( item->text_charLength > 0 ){
			wcscpy_s(item->text_char, item->text_charLength+1, srcInfo->extInfo->text_char.c_str());
		}
	}

	if( srcInfo->contentInfo != NULL ){
		EPG_CONTEN_INFO* item = new EPG_CONTEN_INFO;
		destInfo->contentInfo = item;

		item->listSize = (WORD)srcInfo->contentInfo->nibbleList.size();
		if( item->listSize > 0 ){
			item->nibbleList = new EPG_CONTENT[item->listSize];
			for( WORD i=0; i<item->listSize; i++ ){
				item->nibbleList[i].content_nibble_level_1 = srcInfo->contentInfo->nibbleList[i].content_nibble_level_1;
				item->nibbleList[i].content_nibble_level_2 = srcInfo->contentInfo->nibbleList[i].content_nibble_level_2;
				item->nibbleList[i].user_nibble_1 = srcInfo->contentInfo->nibbleList[i].user_nibble_1;
				item->nibbleList[i].user_nibble_2 = srcInfo->contentInfo->nibbleList[i].user_nibble_2;
			}
		}
	}

	if( srcInfo->componentInfo != NULL ){
		EPG_COMPONENT_INFO* item = new EPG_COMPONENT_INFO;
		destInfo->componentInfo = item;

		item->stream_content = srcInfo->componentInfo->stream_content;
		item->component_type = srcInfo->componentInfo->component_type;
		item->component_tag = srcInfo->componentInfo->component_tag;

		item->text_charLength = (WORD)srcInfo->componentInfo->text_char.size();
		item->text_char = new WCHAR[item->text_charLength+1];
		ZeroMemory(item->text_char, sizeof(WCHAR)*(item->text_charLength+1));
		if( item->text_charLength > 0 ){
			wcscpy_s(item->text_char, item->text_charLength+1, srcInfo->componentInfo->text_char.c_str());
		}
	}

	if( srcInfo->audioInfo != NULL ){
		EPG_AUDIO_COMPONENT_INFO* item = new EPG_AUDIO_COMPONENT_INFO;
		destInfo->audioInfo = item;
		item->listSize = (WORD)srcInfo->audioInfo->componentList.size();
		if( item->listSize > 0 ){
			item->audioList = new EPG_AUDIO_COMPONENT_INFO_DATA[item->listSize];
			for( WORD i=0; i<item->listSize; i++ ){
				item->audioList[i].stream_content = srcInfo->audioInfo->componentList[i].stream_content;
				item->audioList[i].component_type = srcInfo->audioInfo->componentList[i].component_type;
				item->audioList[i].component_tag = srcInfo->audioInfo->componentList[i].component_tag;
				item->audioList[i].stream_type = srcInfo->audioInfo->componentList[i].stream_type;
				item->audioList[i].simulcast_group_tag = srcInfo->audioInfo->componentList[i].simulcast_group_tag;
				item->audioList[i].ES_multi_lingual_flag = srcInfo->audioInfo->componentList[i].ES_multi_lingual_flag;
				item->audioList[i].main_component_flag = srcInfo->audioInfo->componentList[i].main_component_flag;
				item->audioList[i].quality_indicator = srcInfo->audioInfo->componentList[i].quality_indicator;
				item->audioList[i].sampling_rate = srcInfo->audioInfo->componentList[i].sampling_rate;

				item->audioList[i].text_charLength = (WORD)srcInfo->audioInfo->componentList[i].text_char.size();
				item->audioList[i].text_char = new WCHAR[item->audioList[i].text_charLength+1];
				ZeroMemory(item->audioList[i].text_char, sizeof(WCHAR)*(item->audioList[i].text_charLength+1));
				if( item->audioList[i].text_charLength > 0 ){
					wcscpy_s(item->audioList[i].text_char, item->audioList[i].text_charLength+1, srcInfo->audioInfo->componentList[i].text_char.c_str());
				}
			}
		}
	}

	if( srcInfo->eventGroupInfo != NULL ){
		EPG_EVENTGROUP_INFO* item = new EPG_EVENTGROUP_INFO;
		destInfo->eventGroupInfo = item;

		item->group_type = srcInfo->eventGroupInfo->group_type;
		item->event_count = (BYTE)srcInfo->eventGroupInfo->eventDataList.size();

		if( item->event_count > 0 ){
			item->eventDataList = new EPG_EVENT_DATA[item->event_count];
			for( BYTE i=0; i<item->event_count; i++ ){
				item->eventDataList[i].original_network_id = srcInfo->eventGroupInfo->eventDataList[i].original_network_id;
				item->eventDataList[i].transport_stream_id = srcInfo->eventGroupInfo->eventDataList[i].transport_stream_id;
				item->eventDataList[i].service_id = srcInfo->eventGroupInfo->eventDataList[i].service_id;
				item->eventDataList[i].event_id = srcInfo->eventGroupInfo->eventDataList[i].event_id;
			}
		}
	}

	if( srcInfo->eventRelayInfo != NULL ){
		EPG_EVENTGROUP_INFO* item = new EPG_EVENTGROUP_INFO;
		destInfo->eventRelayInfo = item;

		item->group_type = srcInfo->eventRelayInfo->group_type;
		//他チャンネルのときevent_countは０になっている
		//item->event_count = srcInfo->eventGroupInfo->event_count;
		item->event_count = (BYTE)srcInfo->eventRelayInfo->eventDataList.size();

		if( item->event_count > 0 ){
			item->eventDataList = new EPG_EVENT_DATA[item->event_count];
			for( BYTE i=0; i<item->event_count; i++ ){
				item->eventDataList[i].original_network_id = srcInfo->eventRelayInfo->eventDataList[i].original_network_id;
				item->eventDataList[i].transport_stream_id = srcInfo->eventRelayInfo->eventDataList[i].transport_stream_id;
				item->eventDataList[i].service_id = srcInfo->eventRelayInfo->eventDataList[i].service_id;
				item->eventDataList[i].event_id = srcInfo->eventRelayInfo->eventDataList[i].event_id;
			}
		}
	}

}

void GetGenreList(vector<GENRU_INFO>* list)
{
	if( genruList.size() == 0 ){
		CreateContentKindMap();
	}
	*list = genruList;
}
