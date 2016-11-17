#include "stdafx.h"
#include "EpgTimerUtil.h"

#include "PathUtil.h"
#include "StringUtil.h"
#include "TimeUtil.h"

LONGLONG _Create64Key( WORD OriginalNetworkID, WORD TransportStreamID, WORD ServiceID )
{
	LONGLONG Key = 
		(((LONGLONG)(OriginalNetworkID&0x0000FFFF))<<32) |
		(((LONGLONG)(TransportStreamID&0x0000FFFF))<<16) |
		((LONGLONG)(ServiceID&0x0000FFFF));
	return Key;
}

ULONGLONG _Create64Key2( WORD OriginalNetworkID, WORD TransportStreamID, WORD ServiceID, WORD EventID )
{
	ULONGLONG Key = 
		(((ULONGLONG)(OriginalNetworkID & 0x0000FFFF))<<48) |
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

	*y = (WORD)(yy + k);
	*m = (WORD)(mm-1-k*12);

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

//EPG����Text�ɕϊ�
void _ConvertEpgInfoText(const EPGDB_EVENT_INFO* info, wstring& text)
{
	text = L"";
	if( info == NULL ){
		return ;
	}

	wstring time=L"����";
	if( info->StartTimeFlag == TRUE && info->DurationFlag == TRUE ){
		GetTimeString3(info->start_time, info->durationSec, time);
	}else if( info->StartTimeFlag == TRUE && info->DurationFlag == FALSE ){
		GetTimeString4(info->start_time, time);
		time += L" �` ����";
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
		text += L"�ڍ׏��\r\n";
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

static const struct KIND_INFO {
	WORD key;
	LPCWSTR str;
} contentKindSortedArray[] = {
	{ 0x0000, L"�莞�E����" },
	{ 0x0001, L"�V�C" },
	{ 0x0002, L"���W�E�h�L�������g" },
	{ 0x0003, L"�����E����" },
	{ 0x0004, L"�o�ρE�s��" },
	{ 0x0005, L"�C�O�E����" },
	{ 0x0006, L"���" },
	{ 0x0007, L"���_�E��k" },
	{ 0x0008, L"�񓹓���" },
	{ 0x0009, L"���[�J���E�n��" },
	{ 0x000A, L"���" },
	{ 0x000F, L"���̑�" },
	{ 0x00FF, L"�j���[�X�^��" },

	{ 0x0100, L"�X�|�[�c�j���[�X" },
	{ 0x0101, L"�싅" },
	{ 0x0102, L"�T�b�J�[" },
	{ 0x0103, L"�S���t" },
	{ 0x0104, L"���̑��̋��Z" },
	{ 0x0105, L"���o�E�i���Z" },
	{ 0x0106, L"�I�����s�b�N�E���ۑ��" },
	{ 0x0107, L"�}���\���E����E���j" },
	{ 0x0108, L"���[�^�[�X�|�[�c" },
	{ 0x0109, L"�}�����E�E�B���^�[�X�|�[�c" },
	{ 0x010A, L"���n�E���c���Z" },
	{ 0x010F, L"���̑�" },
	{ 0x01FF, L"�X�|�[�c" },

	{ 0x0200, L"�|�\�E���C�h�V���[" },
	{ 0x0201, L"�t�@�b�V����" },
	{ 0x0202, L"��炵�E�Z�܂�" },
	{ 0x0203, L"���N�E���" },
	{ 0x0204, L"�V���b�s���O�E�ʔ�" },
	{ 0x0205, L"�O�����E����" },
	{ 0x0206, L"�C�x���g" },
	{ 0x0207, L"�ԑg�Љ�E���m�点" },
	{ 0x020F, L"���̑�" },
	{ 0x02FF, L"���^���C�h�V���[" },

	{ 0x0300, L"�����h���}" },
	{ 0x0301, L"�C�O�h���}" },
	{ 0x0302, L"���㌀" },
	{ 0x030F, L"���̑�" },
	{ 0x03FF, L"�h���}" },

	{ 0x0400, L"�������b�N�E�|�b�v�X" },
	{ 0x0401, L"�C�O���b�N�E�|�b�v�X" },
	{ 0x0402, L"�N���V�b�N�E�I�y��" },
	{ 0x0403, L"�W���Y�E�t���[�W����" },
	{ 0x0404, L"�̗w�ȁE����" },
	{ 0x0405, L"���C�u�E�R���T�[�g" },
	{ 0x0406, L"�����L���O�E���N�G�X�g" },
	{ 0x0407, L"�J���I�P�E�̂ǎ���" },
	{ 0x0408, L"���w�E�M�y" },
	{ 0x0409, L"���w�E�L�b�Y" },
	{ 0x040A, L"�������y�E���[���h�~���[�W�b�N" },
	{ 0x040F, L"���̑�" },
	{ 0x04FF, L"���y" },

	{ 0x0500, L"�N�C�Y" },
	{ 0x0501, L"�Q�[��" },
	{ 0x0502, L"�g�[�N�o���G�e�B" },
	{ 0x0503, L"���΂��E�R���f�B" },
	{ 0x0504, L"���y�o���G�e�B" },
	{ 0x0505, L"���o���G�e�B" },
	{ 0x0506, L"�����o���G�e�B" },
	{ 0x050F, L"���̑�" },
	{ 0x05FF, L"�o���G�e�B" },

	{ 0x0600, L"�m��" },
	{ 0x0601, L"�M��" },
	{ 0x0602, L"�A�j��" },
	{ 0x060F, L"���̑�" },
	{ 0x06FF, L"�f��" },

	{ 0x0700, L"�����A�j��" },
	{ 0x0701, L"�C�O�A�j��" },
	{ 0x0702, L"���B" },
	{ 0x070F, L"���̑�" },
	{ 0x07FF, L"�A�j���^���B" },

	{ 0x0800, L"�Љ�E����" },
	{ 0x0801, L"���j�E�I�s" },
	{ 0x0802, L"���R�E�����E��" },
	{ 0x0803, L"�F���E�Ȋw�E��w" },
	{ 0x0804, L"�J���`���[�E�`������" },
	{ 0x0805, L"���w�E���|" },
	{ 0x0806, L"�X�|�[�c" },
	{ 0x0807, L"�h�L�������^���[�S��" },
	{ 0x0808, L"�C���^�r���[�E���_" },
	{ 0x080F, L"���̑�" },
	{ 0x08FF, L"�h�L�������^���[�^���{" },

	{ 0x0900, L"���㌀�E�V��" },
	{ 0x0901, L"�~���[�W�J��" },
	{ 0x0902, L"�_���X�E�o���G" },
	{ 0x0903, L"����E���|" },
	{ 0x0904, L"�̕���E�ÓT" },
	{ 0x090F, L"���̑�" },
	{ 0x09FF, L"����^����" },

	{ 0x0A00, L"���E�ނ�E�A�E�g�h�A" },
	{ 0x0A01, L"���|�E�y�b�g�E��|" },
	{ 0x0A02, L"���y�E���p�E�H�|" },
	{ 0x0A03, L"�͌�E����" },
	{ 0x0A04, L"�����E�p�`���R" },
	{ 0x0A05, L"�ԁE�I�[�g�o�C" },
	{ 0x0A06, L"�R���s���[�^�E�s�u�Q�[��" },
	{ 0x0A07, L"��b�E��w" },
	{ 0x0A08, L"�c���E���w��" },
	{ 0x0A09, L"���w���E���Z��" },
	{ 0x0A0A, L"��w���E��" },
	{ 0x0A0B, L"���U����E���i" },
	{ 0x0A0C, L"������" },
	{ 0x0A0F, L"���̑�" },
	{ 0x0AFF, L"��^����" },

	{ 0x0B00, L"�����" },
	{ 0x0B01, L"��Q��" },
	{ 0x0B02, L"�Љ��" },
	{ 0x0B03, L"�{�����e�B�A" },
	{ 0x0B04, L"��b" },
	{ 0x0B05, L"�����i�����j" },
	{ 0x0B06, L"�������" },
	{ 0x0B0F, L"���̑�" },
	{ 0x0BFF, L"����" },

	{ 0x0FFF, L"���̑�" },

	{ 0x7000, L"�e�j�X" },
	{ 0x7001, L"�o�X�P�b�g�{�[��" },
	{ 0x7002, L"���O�r�[" },
	{ 0x7003, L"�A�����J���t�b�g�{�[��" },
	{ 0x7004, L"�{�N�V���O" },
	{ 0x7005, L"�v�����X" },
	{ 0x700F, L"���̑�" },
	{ 0x70FF, L"�X�|�[�c(CS)" },

	{ 0x7100, L"�A�N�V����" },
	{ 0x7101, L"SF�^�t�@���^�W�[" },
	{ 0x7102, L"�R���f�B�[" },
	{ 0x7103, L"�T�X�y���X�^�~�X�e���[" },
	{ 0x7104, L"�����^���}���X" },
	{ 0x7105, L"�z���[�^�X�����[" },
	{ 0x7106, L"�E�G�X�^��" },
	{ 0x7107, L"�h���}�^�Љ�h�h���}" },
	{ 0x7108, L"�A�j���[�V����" },
	{ 0x7109, L"�h�L�������^���[" },
	{ 0x710A, L"�A�h�x���`���[�^�`��" },
	{ 0x710B, L"�~���[�W�J���^���y�f��" },
	{ 0x710C, L"�z�[���h���}" },
	{ 0x710F, L"���̑�" },
	{ 0x71FF, L"�m��(CS)" },

	{ 0x7200, L"�A�N�V����" },
	{ 0x7201, L"SF�^�t�@���^�W�[" },
	{ 0x7202, L"���΂��^�R���f�B�[" },
	{ 0x7203, L"�T�X�y���X�^�~�X�e���[" },
	{ 0x7204, L"�����^���}���X" },
	{ 0x7205, L"�z���[�^�X�����[" },
	{ 0x7206, L"�t�^�w���^�A�C�h��" },
	{ 0x7207, L"�C���^���㌀" },
	{ 0x7208, L"�A�j���[�V����" },
	{ 0x7209, L"�h�L�������^���[" },
	{ 0x720A, L"�A�h�x���`���[�^�`��" },
	{ 0x720B, L"�~���[�W�J���^���y�f��" },
	{ 0x720C, L"�z�[���h���}" },
	{ 0x720F, L"���̑�" },
	{ 0x72FF, L"�M��(CS)" },

	{ 0xFFFF, L"�Ȃ�" },
},
componentKindSortedArray[] = {
	{ 0x0101, L"480i(525i)�A�A�X�y�N�g��4:3" },
	{ 0x0102, L"480i(525i)�A�A�X�y�N�g��16:9 �p���x�N�g������" },
	{ 0x0103, L"480i(525i)�A�A�X�y�N�g��16:9 �p���x�N�g���Ȃ�" },
	{ 0x0104, L"480i(525i)�A�A�X�y�N�g�� > 16:9" },
	{ 0x0191, L"2160p�A�A�X�y�N�g��4:3" },
	{ 0x0192, L"2160p�A�A�X�y�N�g��16:9 �p���x�N�g������" },
	{ 0x0193, L"2160p�A�A�X�y�N�g��16:9 �p���x�N�g���Ȃ�" },
	{ 0x0194, L"2160p�A�A�X�y�N�g�� > 16:9" },
	{ 0x01A1, L"480p(525p)�A�A�X�y�N�g��4:3" },
	{ 0x01A2, L"480p(525p)�A�A�X�y�N�g��16:9 �p���x�N�g������" },
	{ 0x01A3, L"480p(525p)�A�A�X�y�N�g��16:9 �p���x�N�g���Ȃ�" },
	{ 0x01A4, L"480p(525p)�A�A�X�y�N�g�� > 16:9" },
	{ 0x01B1, L"1080i(1125i)�A�A�X�y�N�g��4:3" },
	{ 0x01B2, L"1080i(1125i)�A�A�X�y�N�g��16:9 �p���x�N�g������" },
	{ 0x01B3, L"1080i(1125i)�A�A�X�y�N�g��16:9 �p���x�N�g���Ȃ�" },
	{ 0x01B4, L"1080i(1125i)�A�A�X�y�N�g�� > 16:9" },
	{ 0x01C1, L"720p(750p)�A�A�X�y�N�g��4:3" },
	{ 0x01C2, L"720p(750p)�A�A�X�y�N�g��16:9 �p���x�N�g������" },
	{ 0x01C3, L"720p(750p)�A�A�X�y�N�g��16:9 �p���x�N�g���Ȃ�" },
	{ 0x01C4, L"720p(750p)�A�A�X�y�N�g�� > 16:9" },
	{ 0x01D1, L"240p �A�X�y�N�g��4:3" },
	{ 0x01D2, L"240p �A�X�y�N�g��16:9 �p���x�N�g������" },
	{ 0x01D3, L"240p �A�X�y�N�g��16:9 �p���x�N�g���Ȃ�" },
	{ 0x01D4, L"240p �A�X�y�N�g�� > 16:9" },
	{ 0x01E1, L"1080p(1125p)�A�A�X�y�N�g��4:3" },
	{ 0x01E2, L"1080p(1125p)�A�A�X�y�N�g��16:9 �p���x�N�g������" },
	{ 0x01E3, L"1080p(1125p)�A�A�X�y�N�g��16:9 �p���x�N�g���Ȃ�" },
	{ 0x01E4, L"1080p(1125p)�A�A�X�y�N�g�� > 16:9" },
	{ 0x0201, L"1/0���[�h�i�V���O�����m�j" },
	{ 0x0202, L"1/0�{1/0���[�h�i�f���A�����m�j" },
	{ 0x0203, L"2/0���[�h�i�X�e���I�j" },
	{ 0x0204, L"2/1���[�h" },
	{ 0x0205, L"3/0���[�h" },
	{ 0x0206, L"2/2���[�h" },
	{ 0x0207, L"3/1���[�h" },
	{ 0x0208, L"3/2���[�h" },
	{ 0x0209, L"3/2�{LFE���[�h�i3/2.1���[�h�j" },
	{ 0x020A, L"3/3.1���[�h" },
	{ 0x020B, L"2/0/0-2/0/2-0.1���[�h" },
	{ 0x020C, L"5/2.1���[�h" },
	{ 0x020D, L"3/2/2.1���[�h" },
	{ 0x020E, L"2/0/0-3/0/2-0.1���[�h" },
	{ 0x020F, L"0/2/0-3/0/2-0.1���[�h" },
	{ 0x0210, L"2/0/0-3/2/3-0.2���[�h" },
	{ 0x0211, L"3/3/3-5/2/3-3/0/0.2���[�h" },
	{ 0x0240, L"���o��Q�җp�������" },
	{ 0x0241, L"���o��Q�җp����" },
	{ 0x0501, L"H.264|MPEG-4 AVC�A480i(525i)�A�A�X�y�N�g��4:3" },
	{ 0x0502, L"H.264|MPEG-4 AVC�A480i(525i)�A�A�X�y�N�g��16:9 �p���x�N�g������" },
	{ 0x0503, L"H.264|MPEG-4 AVC�A480i(525i)�A�A�X�y�N�g��16:9 �p���x�N�g���Ȃ�" },
	{ 0x0504, L"H.264|MPEG-4 AVC�A480i(525i)�A�A�X�y�N�g�� > 16:9" },
	{ 0x0591, L"H.264|MPEG-4 AVC�A2160p�A�A�X�y�N�g��4:3" },
	{ 0x0592, L"H.264|MPEG-4 AVC�A2160p�A�A�X�y�N�g��16:9 �p���x�N�g������" },
	{ 0x0593, L"H.264|MPEG-4 AVC�A2160p�A�A�X�y�N�g��16:9 �p���x�N�g���Ȃ�" },
	{ 0x0594, L"H.264|MPEG-4 AVC�A2160p�A�A�X�y�N�g�� > 16:9" },
	{ 0x05A1, L"H.264|MPEG-4 AVC�A480p(525p)�A�A�X�y�N�g��4:3" },
	{ 0x05A2, L"H.264|MPEG-4 AVC�A480p(525p)�A�A�X�y�N�g��16:9 �p���x�N�g������" },
	{ 0x05A3, L"H.264|MPEG-4 AVC�A480p(525p)�A�A�X�y�N�g��16:9 �p���x�N�g���Ȃ�" },
	{ 0x05A4, L"H.264|MPEG-4 AVC�A480p(525p)�A�A�X�y�N�g�� > 16:9" },
	{ 0x05B1, L"H.264|MPEG-4 AVC�A1080i(1125i)�A�A�X�y�N�g��4:3" },
	{ 0x05B2, L"H.264|MPEG-4 AVC�A1080i(1125i)�A�A�X�y�N�g��16:9 �p���x�N�g������" },
	{ 0x05B3, L"H.264|MPEG-4 AVC�A1080i(1125i)�A�A�X�y�N�g��16:9 �p���x�N�g���Ȃ�" },
	{ 0x05B4, L"H.264|MPEG-4 AVC�A1080i(1125i)�A�A�X�y�N�g�� > 16:9" },
	{ 0x05C1, L"H.264|MPEG-4 AVC�A720p(750p)�A�A�X�y�N�g��4:3" },
	{ 0x05C2, L"H.264|MPEG-4 AVC�A720p(750p)�A�A�X�y�N�g��16:9 �p���x�N�g������" },
	{ 0x05C3, L"H.264|MPEG-4 AVC�A720p(750p)�A�A�X�y�N�g��16:9 �p���x�N�g���Ȃ�" },
	{ 0x05C4, L"H.264|MPEG-4 AVC�A720p(750p)�A�A�X�y�N�g�� > 16:9" },
	{ 0x05D1, L"H.264|MPEG-4 AVC�A240p �A�X�y�N�g��4:3" },
	{ 0x05D2, L"H.264|MPEG-4 AVC�A240p �A�X�y�N�g��16:9 �p���x�N�g������" },
	{ 0x05D3, L"H.264|MPEG-4 AVC�A240p �A�X�y�N�g��16:9 �p���x�N�g���Ȃ�" },
	{ 0x05D4, L"H.264|MPEG-4 AVC�A240p �A�X�y�N�g�� > 16:9" },
	{ 0x05E1, L"H.264|MPEG-4 AVC�A1080p(1125p)�A�A�X�y�N�g��4:3" },
	{ 0x05E2, L"H.264|MPEG-4 AVC�A1080p(1125p)�A�A�X�y�N�g��16:9 �p���x�N�g������" },
	{ 0x05E3, L"H.264|MPEG-4 AVC�A1080p(1125p)�A�A�X�y�N�g��16:9 �p���x�N�g���Ȃ�" },
	{ 0x05E4, L"H.264|MPEG-4 AVC�A1080p(1125p)�A�A�X�y�N�g�� > 16:9" },
};

static LPCWSTR SearchKindInfoArray(WORD key, const KIND_INFO* arr, size_t len)
{
	KIND_INFO info = { key };
	const KIND_INFO* ret = std::lower_bound(arr, arr + len, info, [](const KIND_INFO& a, const KIND_INFO& b) { return a.key < b.key; });
	return ret != arr + len && ret->key == key ? ret->str : NULL;
}

//EPG����Text�ɕϊ�
void _ConvertEpgInfoText2(const EPGDB_EVENT_INFO* info, wstring& text, wstring serviceName)
{
	text = L"";
	if( info == NULL ){
		return ;
	}

	wstring time=L"����";
	if( info->StartTimeFlag == TRUE && info->DurationFlag == TRUE ){
		GetTimeString3(info->start_time, info->durationSec, time);
	}else if( info->StartTimeFlag == TRUE && info->DurationFlag == FALSE ){
		GetTimeString4(info->start_time, time);
		time += L" �` ����";
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
		text += L"�ڍ׏��\r\n";
		text += info->extInfo->text_char;
		text += L"\r\n\r\n";
	}

	if( info->contentInfo != NULL ){
		text+=L"�W������ : \r\n";
		for( size_t i=0; i<info->contentInfo->nibbleList.size(); i++ ){
			BYTE nibble1 = info->contentInfo->nibbleList[i].content_nibble_level_1;
			BYTE nibble2 = info->contentInfo->nibbleList[i].content_nibble_level_2;
			if( nibble1 == 0x0E && nibble2 == 0x01 ){
				//CS�g���p���
				nibble1 = info->contentInfo->nibbleList[i].user_nibble_1 | 0x70;
				nibble2 = info->contentInfo->nibbleList[i].user_nibble_2;
			}
			WCHAR buff[32];
			LPCWSTR retStr = SearchKindInfoArray(nibble1 << 8 | 0xFF, contentKindSortedArray, _countof(contentKindSortedArray));
			if( retStr != NULL ){
				text+=retStr;
				retStr = SearchKindInfoArray(nibble1 << 8 | nibble2, contentKindSortedArray, _countof(contentKindSortedArray));
				if( retStr != NULL ){
					text+=L" - ";
					text+=retStr;
				}else if( nibble1 != 0x0F ){
					swprintf_s(buff, L" - (0x%02X)", nibble2);
					text += buff;
				}
			}else{
				swprintf_s(buff, L"(0x%02X) - (0x%02X)", nibble1, nibble2);
				text += buff;
			}
			text+=L"\r\n";
		}
		text+=L"\r\n";
	}

	if( info->componentInfo != NULL ){
		text+=L"�f�� : ";
		WORD key = ((WORD)info->componentInfo->stream_content) << 8 | info->componentInfo->component_type;
		LPCWSTR retStr = SearchKindInfoArray(key, componentKindSortedArray, _countof(componentKindSortedArray));
		if( retStr != NULL ){
			text+=retStr;
			if( info->componentInfo->text_char.size() > 0 ){
				text+=L"\r\n";
				text+=info->componentInfo->text_char;
			}
		}
		text+=L"\r\n";
	}

	if( info->audioInfo != NULL ){
		text+=L"���� : ";
		for( size_t i=0; i<info->audioInfo->componentList.size(); i++ ){
			WORD key = ((WORD)info->audioInfo->componentList[i].stream_content) << 8 | info->audioInfo->componentList[i].component_type;
			LPCWSTR retStr = SearchKindInfoArray(key, componentKindSortedArray, _countof(componentKindSortedArray));
			if( retStr != NULL ){
				text+=retStr;
				if( info->audioInfo->componentList[i].text_char.size() > 0 ){
					text+=L"\r\n";
					text+=info->audioInfo->componentList[i].text_char;
				}
			}
			text+=L"\r\n";

			text+=L"�T���v�����O���[�g : ";
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
            text += L"��������\r\n";
        }
        else
        {
            text += L"�L������\r\n";
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
	LPCWSTR retStr = SearchKindInfoArray(nibble1 << 8 | nibble2, contentKindSortedArray, _countof(contentKindSortedArray));
	if( retStr != NULL ){
		name = retStr;
	}
}

void GetComponentTypeName(BYTE content, BYTE type, wstring& name)
{
	LPCWSTR retStr = SearchKindInfoArray(content << 8 | type, componentKindSortedArray, _countof(componentKindSortedArray));
	if( retStr != NULL ){
		name = retStr;
	}
}

void CopyEpgInfo(EPG_EVENT_INFO* destInfo, const EPGDB_EVENT_INFO* srcInfo)
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
		//���`�����l���̂Ƃ�event_count�͂O�ɂȂ��Ă���
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

void ConvertEpgInfo(WORD onid, WORD tsid, WORD sid, const EPG_EVENT_INFO* src, EPGDB_EVENT_INFO* dest)
{
	dest->original_network_id = onid;
	dest->transport_stream_id = tsid;
	dest->service_id = sid;
	dest->event_id = src->event_id;
	dest->StartTimeFlag = src->StartTimeFlag;
	dest->start_time = src->start_time;
	dest->DurationFlag = src->DurationFlag;
	dest->durationSec = src->durationSec;
	dest->freeCAFlag = src->freeCAFlag;

	dest->shortInfo = NULL;
	if( src->shortInfo != NULL ){
		dest->shortInfo.reset(new EPGDB_SHORT_EVENT_INFO);
		dest->shortInfo->event_name = src->shortInfo->event_name;
		dest->shortInfo->text_char = src->shortInfo->text_char;
	}
	dest->extInfo = NULL;
	if( src->extInfo != NULL ){
		dest->extInfo.reset(new EPGDB_EXTENDED_EVENT_INFO);
		dest->extInfo->text_char = src->extInfo->text_char;
	}
	dest->contentInfo = NULL;
	if( src->contentInfo != NULL ){
		dest->contentInfo.reset(new EPGDB_CONTEN_INFO);
		for( BYTE i=0; i<src->contentInfo->listSize; i++ ){
			EPGDB_CONTENT_DATA item;
			item.content_nibble_level_1 = src->contentInfo->nibbleList[i].content_nibble_level_1;
			item.content_nibble_level_2 = src->contentInfo->nibbleList[i].content_nibble_level_2;
			item.user_nibble_1 = src->contentInfo->nibbleList[i].user_nibble_1;
			item.user_nibble_2 = src->contentInfo->nibbleList[i].user_nibble_2;
			dest->contentInfo->nibbleList.push_back(item);
		}
	}
	dest->componentInfo = NULL;
	if( src->componentInfo != NULL ){
		dest->componentInfo.reset(new EPGDB_COMPONENT_INFO);
		dest->componentInfo->stream_content = src->componentInfo->stream_content;
		dest->componentInfo->component_type = src->componentInfo->component_type;
		dest->componentInfo->component_tag = src->componentInfo->component_tag;
		dest->componentInfo->text_char = src->componentInfo->text_char;
	}
	dest->audioInfo = NULL;
	if( src->audioInfo != NULL ){
		dest->audioInfo.reset(new EPGDB_AUDIO_COMPONENT_INFO);
		for( WORD i=0; i<src->audioInfo->listSize; i++ ){
			EPGDB_AUDIO_COMPONENT_INFO_DATA item;
			item.stream_content = src->audioInfo->audioList[i].stream_content;
			item.component_type = src->audioInfo->audioList[i].component_type;
			item.component_tag = src->audioInfo->audioList[i].component_tag;
			item.stream_type = src->audioInfo->audioList[i].stream_type;
			item.simulcast_group_tag = src->audioInfo->audioList[i].simulcast_group_tag;
			item.ES_multi_lingual_flag = src->audioInfo->audioList[i].ES_multi_lingual_flag;
			item.main_component_flag = src->audioInfo->audioList[i].main_component_flag;
			item.quality_indicator = src->audioInfo->audioList[i].quality_indicator;
			item.sampling_rate = src->audioInfo->audioList[i].sampling_rate;
			item.text_char = src->audioInfo->audioList[i].text_char;
			dest->audioInfo->componentList.push_back(item);
		}
	}
	dest->eventGroupInfo = NULL;
	if( src->eventGroupInfo != NULL ){
		dest->eventGroupInfo.reset(new EPGDB_EVENTGROUP_INFO);
		dest->eventGroupInfo->group_type = src->eventGroupInfo->group_type;
		for( BYTE i = 0; i<src->eventGroupInfo->event_count; i++ ){
			EPGDB_EVENT_DATA item;
			item.original_network_id = src->eventGroupInfo->eventDataList[i].original_network_id;
			item.transport_stream_id = src->eventGroupInfo->eventDataList[i].transport_stream_id;
			item.service_id = src->eventGroupInfo->eventDataList[i].service_id;
			item.event_id = src->eventGroupInfo->eventDataList[i].event_id;
			dest->eventGroupInfo->eventDataList.push_back(item);
		}
	}
	dest->eventRelayInfo = NULL;
	if( src->eventRelayInfo != NULL ){
		dest->eventRelayInfo.reset(new EPGDB_EVENTGROUP_INFO);
		dest->eventRelayInfo->group_type = src->eventRelayInfo->group_type;
		for( BYTE i = 0; i<src->eventRelayInfo->event_count; i++ ){
			EPGDB_EVENT_DATA item;
			item.original_network_id = src->eventRelayInfo->eventDataList[i].original_network_id;
			item.transport_stream_id = src->eventRelayInfo->eventDataList[i].transport_stream_id;
			item.service_id = src->eventRelayInfo->eventDataList[i].service_id;
			item.event_id = src->eventRelayInfo->eventDataList[i].event_id;
			dest->eventRelayInfo->eventDataList.push_back(item);
		}
	}
}
