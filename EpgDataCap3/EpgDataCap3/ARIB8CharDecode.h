#pragma once

// MFCÇ≈égÇ§éûóp
/*#ifdef _DEBUG
#undef new
#endif
#ifdef _DEBUG
#define new DEBUG_NEW
#endif
*/

#include "ColorDef.h"

//ï∂éöïÑçÜèWçá
//GÉZÉbÉg
#define MF_JIS_KANJI1 0x39 //JISå›ä∑äøéö1ñ 
#define MF_JIS_KANJI2 0x3A //JISå›ä∑äøéö2ñ 
#define MF_KIGOU 0x3B //í«â¡ãLçÜ
#define MF_ASCII 0x4A //âpêî
#define MF_HIRA  0x30 //ïΩâºñº
#define MF_KANA  0x31 //ï–âºñº
#define MF_KANJI 0x42 //äøéö
#define MF_MOSAIC_A 0x32 //ÉÇÉUÉCÉNA
#define MF_MOSAIC_B 0x33 //ÉÇÉUÉCÉNB
#define MF_MOSAIC_C 0x34 //ÉÇÉUÉCÉNC
#define MF_MOSAIC_D 0x35 //ÉÇÉUÉCÉND
#define MF_PROP_ASCII 0x36 //ÉvÉçÉ|Å[ÉVÉáÉiÉãâpêî
#define MF_PROP_HIRA  0x37 //ÉvÉçÉ|Å[ÉVÉáÉiÉãïΩâºñº
#define MF_PROP_KANA  0x38 //ÉvÉçÉ|Å[ÉVÉáÉiÉãï–âºñº
#define MF_JISX_KANA 0x49 //JIX X0201ï–âºñº
//DRCS
#define MF_DRCS_0 0x40 //DRCS-0
#define MF_DRCS_1 0x41 //DRCS-1
#define MF_DRCS_2 0x42 //DRCS-2
#define MF_DRCS_3 0x43 //DRCS-3
#define MF_DRCS_4 0x44 //DRCS-4
#define MF_DRCS_5 0x45 //DRCS-5
#define MF_DRCS_6 0x46 //DRCS-6
#define MF_DRCS_7 0x47 //DRCS-7
#define MF_DRCS_8 0x48 //DRCS-8
#define MF_DRCS_9 0x49 //DRCS-9
#define MF_DRCS_10 0x4A //DRCS-10
#define MF_DRCS_11 0x4B //DRCS-11
#define MF_DRCS_12 0x4C //DRCS-12
#define MF_DRCS_13 0x4D //DRCS-13
#define MF_DRCS_14 0x4E //DRCS-14
#define MF_DRCS_15 0x4F //DRCS-15
#define MF_MACRO 0x70 //É}ÉNÉç

//ïÑçÜèWçáÇÃï™óﬁ
#define MF_MODE_G 1 //GÉZÉbÉg
#define MF_MODE_DRCS 2 //DRCS
#define MF_MODE_OTHER 3 //ÇªÇÃëº

#ifdef ARIB8CHAR_DECODE_H_IMPLEMENT_TABLE

static const WCHAR AsciiTable[]={
	L'ÅI', L'Åh', L'Åî', L'Åê', L'Åì', L'Åï', L'Åf',
	L'Åi', L'Åj', L'Åñ', L'Å{', L'ÅC', L'Å|', L'ÅD', L'Å^',
	L'ÇO', L'ÇP', L'ÇQ', L'ÇR', L'ÇS', L'ÇT', L'ÇU', L'ÇV',
	L'ÇW', L'ÇX', L'ÅF', L'ÅG', L'ÅÉ', L'ÅÅ', L'ÅÑ', L'ÅH',
	L'Åó', L'Ç`', L'Ça', L'Çb', L'Çc', L'Çd', L'Çe', L'Çf',
	L'Çg', L'Çh', L'Çi', L'Çj', L'Çk', L'Çl', L'Çm', L'Çn',
	L'Ço', L'Çp', L'Çq', L'Çr', L'Çs', L'Çt', L'Çu', L'Çv',
	L'Çw', L'Çx', L'Çy', L'Åm', L'Åè', L'Ån', L'ÅO', L'ÅQ',
	L'Åe', L'ÇÅ', L'ÇÇ', L'ÇÉ', L'ÇÑ', L'ÇÖ', L'ÇÜ', L'Çá',
	L'Çà', L'Çâ', L'Çä', L'Çã', L'Çå', L'Çç', L'Çé', L'Çè',
	L'Çê', L'Çë', L'Çí', L'Çì', L'Çî', L'Çï', L'Çñ', L'Çó',
	L'Çò', L'Çô', L'Çö', L'Åo', L'Åb', L'Åp', L'ÅP'
};
static const WCHAR HiraTable[]={
	L'Çü', L'Ç†', L'Ç°', L'Ç¢', L'Ç£', L'Ç§', L'Ç•',
	L'Ç¶', L'Çß', L'Ç®', L'Ç©', L'Ç™', L'Ç´', L'Ç¨', L'Ç≠',
	L'ÇÆ', L'ÇØ', L'Ç∞', L'Ç±', L'Ç≤', L'Ç≥', L'Ç¥', L'Çµ',
	L'Ç∂', L'Ç∑', L'Ç∏', L'Çπ', L'Ç∫', L'Çª', L'Çº', L'ÇΩ',
	L'Çæ', L'Çø', L'Ç¿', L'Ç¡', L'Ç¬', L'Ç√', L'Çƒ', L'Ç≈',
	L'Ç∆', L'Ç«', L'Ç»', L'Ç…', L'Ç ', L'ÇÀ', L'ÇÃ', L'ÇÕ',
	L'ÇŒ', L'Çœ', L'Ç–', L'Ç—', L'Ç“', L'Ç”', L'Ç‘', L'Ç’',
	L'Ç÷', L'Ç◊', L'Çÿ', L'ÇŸ', L'Ç⁄', L'Ç€', L'Ç‹', L'Ç›',
	L'Çﬁ', L'Çﬂ', L'Ç‡', L'Ç·', L'Ç‚', L'Ç„', L'Ç‰', L'ÇÂ',
	L'ÇÊ', L'ÇÁ', L'ÇË', L'ÇÈ', L'ÇÍ', L'ÇÎ', L'ÇÏ', L'ÇÌ',
	L'ÇÓ', L'ÇÔ', L'Ç', L'ÇÒ', L'Å@', L'Å@', L'Å@', L'ÅT',
	L'ÅU', L'Å[', L'ÅB', L'Åu', L'Åv', L'ÅA', L'ÅE'
};
static const WCHAR KanaTable[]={
	L'É@', L'ÉA', L'ÉB', L'ÉC', L'ÉD', L'ÉE', L'ÉF',
	L'ÉG', L'ÉH', L'ÉI', L'ÉJ', L'ÉK', L'ÉL', L'ÉM', L'ÉN',
	L'ÉO', L'ÉP', L'ÉQ', L'ÉR', L'ÉS', L'ÉT', L'ÉU', L'ÉV',
	L'ÉW', L'ÉX', L'ÉY', L'ÉZ', L'É[', L'É\', L'É]', L'É^',
	L'É_', L'É`', L'Éa', L'Éb', L'Éc', L'Éd', L'Ée', L'Éf',
	L'Ég', L'Éh', L'Éi', L'Éj', L'Ék', L'Él', L'Ém', L'Én',
	L'Éo', L'Ép', L'Éq', L'Ér', L'És', L'Ét', L'Éu', L'Év',
	L'Éw', L'Éx', L'Éy', L'Éz', L'É{', L'É|', L'É}', L'É~',
	L'ÉÄ', L'ÉÅ', L'ÉÇ', L'ÉÉ', L'ÉÑ', L'ÉÖ', L'ÉÜ', L'Éá',
	L'Éà', L'Éâ', L'Éä', L'Éã', L'Éå', L'Éç', L'Éé', L'Éè',
	L'Éê', L'Éë', L'Éí', L'Éì', L'Éî', L'Éï', L'Éñ', L'ÅR',
	L'ÅS', L'Å[', L'ÅB', L'Åu', L'Åv', L'ÅA', L'ÅE'
};

struct GAIJI_TABLE{
	const WCHAR* strCharUnicode;
	const WCHAR* strChar;
};

static const GAIJI_TABLE GaijiTable[]={
	{L"\x2491", L"10."},
	{L"\x2492", L"11."},
	{L"\x2493", L"12."},
	{L"\xD83C\xDD4A", L"[HV]"}, //90ãÊ48ì_
	{L"\xD83C\xDD4C", L"[SD]"},
	{L"\xD83C\xDD3F", L"[Ço]"},
	{L"\xD83C\xDD46", L"[Çv]"},
	{L"\xD83C\xDD4B", L"[MV]"},
	{L"\xD83C\xDE10", L"[éË]"},
	{L"\xD83C\xDE11", L"[éö]"},
	{L"\xD83C\xDE12", L"[ëo]"},
	{L"\xD83C\xDE13", L"[Éf]"},
	{L"\xD83C\xDD42", L"[Çr]"},
	{L"\xD83C\xDE14", L"[ìÒ]"},
	{L"\xD83C\xDE15", L"[ëΩ]"},
	{L"\xD83C\xDE16", L"[â]"},
	{L"\xD83C\xDD4D", L"[SS]"},
	{L"\xD83C\xDD31", L"[Ça]"},
	{L"\xD83C\xDD3D", L"[Çm]"},//
	{L"\x2B1B", L"Å°"},//90ãÊ64ì_
	{L"\x2B24", L"Åú"},
	{L"\xD83C\xDE17", L"[ìV]"},
	{L"\xD83C\xDE18", L"[å]"},
	{L"\xD83C\xDE19", L"[âf]"},
	{L"\xD83C\xDE1A", L"[ñ≥]"},
	{L"\xD83C\xDE1B", L"[óø]"},
	{L"\x26BF", L"[ÅE]"},
	{L"\xD83C\xDE1C", L"[ëO]"},
	{L"\xD83C\xDE1D", L"[å„]"},
	{L"\xD83C\xDE1E", L"[çƒ]"},
	{L"\xD83C\xDE1F", L"[êV]"},
	{L"\xD83C\xDE20", L"[èâ]"},
	{L"\xD83C\xDE21", L"[èI]"},
	{L"\xD83C\xDE22", L"[ê∂]"},
	{L"\xD83C\xDE23", L"[îÃ]"},
	{L"\xD83C\xDE24", L"[ê∫]"},//90ãÊ80ì_
	{L"\xD83C\xDE25", L"[êÅ]"},
	{L"\xD83C\xDD4E", L"[PPV]"},
	{L"\x3299", L"(îÈ)"},
	{L"\xD83C\xDE00", L"ÇŸÇ©"},
	//91ãÊÇÕîÚÇŒÇ∑
	{L"\x27A1", L"Å®"},//92ãÊ1ì_
	{L"\x2B05", L"Å©"},
	{L"\x2B06", L"Å™"},
	{L"\x2B07", L"Å´"},
	{L"\x2B2F", L"Åõ"},
	{L"\x2B2E", L"Åú"},
	{L"îN", L"îN"},
	{L"åé", L"åé"},
	{L"ì˙", L"ì˙"},
	{L"â~", L"â~"},
	{L"\x33A1", L"m^2"},
	{L"\x33A5", L"m^3"},
	{L"\x339D", L"cm"},
	{L"\x33A0", L"cm^2"},
	{L"\x33A4", L"cm^3"},
	{L"\xD83C\xDD00", L"ÇO."},//92ãÊ16ì_
	{L"\x2488", L"ÇP."},
	{L"\x2489", L"ÇQ."},
	{L"\x248A", L"ÇR."},
	{L"\x248B", L"ÇS."},
	{L"\x248C", L"ÇT."},
	{L"\x248D", L"ÇU."},
	{L"\x248E", L"ÇV."},
	{L"\x248F", L"ÇW."},
	{L"\x2490", L"ÇX."},
	{L"éÅ", L"éÅ"},
	{L"ïõ", L"ïõ"},
	{L"å≥", L"å≥"},
	{L"åÃ", L"åÃ"},
	{L"ëO", L"ëO"},
	{L"êV", L"êV"},
	{L"\xD83C\xDD01", L"ÇO,"},//92ãÊ32ì_
	{L"\xD83C\xDD02", L"ÇP,"},
	{L"\xD83C\xDD03", L"ÇQ,"},
	{L"\xD83C\xDD04", L"ÇR,"},
	{L"\xD83C\xDD05", L"ÇS,"},
	{L"\xD83C\xDD06", L"ÇT,"},
	{L"\xD83C\xDD07", L"ÇU,"},
	{L"\xD83C\xDD08", L"ÇV,"},
	{L"\xD83C\xDD09", L"ÇW,"},
	{L"\xD83C\xDD0A", L"ÇX,"},
	{L"\x3233", L"[é–]"},
	{L"\x3236", L"[ç‡]"},
	{L"\x3232", L"[óL]"},
	{L"\x3231", L"[äî]"},
	{L"\x3239", L"[ë„]"},
	{L"\x3244", L"(ñ‚)"},
	{L"\x25B6", L"Å£"},//92ãÊ48ì_
	{L"\x25C0", L"Å•"},
	{L"\x3016", L"Åy"},
	{L"\x3017", L"Åz"},
	{L"\x27D0", L"Åû"},
	{L"\x00B2", L"^2"},
	{L"\x00B3", L"^3"},
	{L"\xD83C\xDD2D", L"(CD)"},
	{L"(vn)", L"(vn)"},
	{L"(ob)", L"(ob)"},
	{L"(cb)", L"(cb)"},
	{L"(ce", L"(ce"},
	{L"mb)", L"mb)"},
	{L"(hp)", L"(hp)"},
	{L"(br)", L"(br)"},
	{L"(Çê)", L"(Çê)"},
	{L"(Çì)", L"(Çì)"},//92ãÊ64ì_
	{L"(ms)", L"(ms)"},
	{L"(Çî)", L"(Çî)"},
	{L"(bs)", L"(bs)"},
	{L"(ÇÇ)", L"(ÇÇ)"},
	{L"(tb)", L"(tb)"},
	{L"(tp)", L"(tp)"},
	{L"(ds)", L"(ds)"},
	{L"(ag)", L"(ag)"},
	{L"(eg)", L"(eg)"},
	{L"(vo)", L"(vo)"},
	{L"(fl)", L"(fl)"},
	{L"(ke", L"(ke"},
	{L"y)", L"y)"},
	{L"(sa", L"(sa"},
	{L"x)", L"x)"},
	{L"(sy", L"(sy"},//92ãÊ80ì_
	{L"n)", L"n)"},
	{L"(or", L"(or"},
	{L"g)", L"g)"},
	{L"(pe", L"(pe"},
	{L"r)", L"r)"},
	{L"\xD83C\xDD2C", L"(Çq)"},
	{L"\xD83C\xDD2B", L"(Çb)"},
	{L"\x3247", L"(‚µ)"},
	{L"\xD83C\xDD90", L"ÇcÇi"},
	{L"\xD83C\xDE26", L"[ââ]"},
	{L"\x213B", L"Fax"},
	{L"\x322A", L"(åé)"},//93ãÊ1ì_
	{L"\x322B", L"(âŒ)"},
	{L"\x322C", L"(êÖ)"},
	{L"\x322D", L"(ñÿ)"},
	{L"\x322E", L"(ã‡)"},
	{L"\x322F", L"(ìy)"},
	{L"\x3230", L"(ì˙)"},
	{L"\x3237", L"(èj)"},
	{L"áç", L"áç"},
	{L"áé", L"áé"},
	{L"áè", L"áè"},
	{L"á~", L"á~"},
	{L"\x2116", L"No."},
	{L"\x2121", L"Tel"},
	{L"\x3036", L"(Åß)"},
	{L"\x26BE", L"()()"},//93ãÊ16ì_
	{L"\xD83C\xDE40", L"[ñ{]"},
	{L"\xD83C\xDE41", L"[éO]"},
	{L"\xD83C\xDE42", L"[ìÒ]"},
	{L"\xD83C\xDE43", L"[à¿]"},
	{L"\xD83C\xDE44", L"[ì_]"},
	{L"\xD83C\xDE45", L"[ë≈]"},
	{L"\xD83C\xDE46", L"[ìê]"},
	{L"\xD83C\xDE47", L"[èü]"},
	{L"\xD83C\xDE48", L"[îs]"},
	{L"\xD83C\xDD2A", L"[Çr]"},
	{L"\xD83C\xDE27", L"[ìä]"},
	{L"\xD83C\xDE28", L"[ïﬂ]"},
	{L"\xD83C\xDE29", L"[àÍ]"},
	{L"\xD83C\xDE14", L"[ìÒ]"},
	{L"\xD83C\xDE2A", L"[éO]"},
	{L"\xD83C\xDE2B", L"[óV]"},//93ãÊ32ì_
	{L"\xD83C\xDE2C", L"[ç∂]"},
	{L"\xD83C\xDE2D", L"[íÜ]"},
	{L"\xD83C\xDE2E", L"[âE]"},
	{L"\xD83C\xDE2F", L"[éw]"},
	{L"\xD83C\xDE30", L"[ëñ]"},
	{L"\xD83C\xDE31", L"[ë≈]"},
	{L"\x2113", L"l"},
	{L"\x338F", L"kg"},
	{L"\x3390", L"Hz"},
	{L"\x33CA", L"ha"},
	{L"\x339E", L"km"},
	{L"\x33A2", L"km^2"},
	{L"\x3371", L"hPa"},
	{L"ÅE", L"ÅE"},
	{L"ÅE", L"ÅE"},
	{L"\x00BD", L"1/2"},//93ãÊ48ì_
	{L"\x2189", L"0/3"},
	{L"\x2153", L"1/3"},
	{L"\x2154", L"2/3"},
	{L"\x00BC", L"1/4"},
	{L"\x00BE", L"3/4"},
	{L"\x2155", L"1/5"},
	{L"\x2156", L"2/5"},
	{L"\x2157", L"3/5"},
	{L"\x2158", L"4/5"},
	{L"\x2159", L"1/6"},
	{L"\x215A", L"5/6"},
	{L"\x2150", L"1/7"},
	{L"\x215B", L"1/8"},
	{L"\x2151", L"1/9"},
	{L"\x2152", L"1/10"},
	{L"\x203C", L"!!"},//93ãÊ78ì_
	{L"\x2049", L"!?"},
	{L"áT", L"áT"},//94ãÊ1ì_
	{L"áU", L"áU"},
	{L"áV", L"áV"},
	{L"áW", L"áW"},
	{L"áX", L"áX"},
	{L"áY", L"áY"},
	{L"áZ", L"áZ"},
	{L"á[", L"á["},
	{L"á\", L"á\"},
	{L"á]", L"á]"},
	{L"\x216A", L"XI"},
	{L"\x216B", L"XII"},
	{L"áP", L"áP"},
	{L"áQ", L"áQ"},
	{L"áR", L"áR"},
	{L"áS", L"áS"},//94ãÊ16ì_
	{L"\x2474", L"(ÇP)"},
	{L"\x2475", L"(ÇQ)"},
	{L"\x2476", L"(ÇR)"},
	{L"\x2477", L"(ÇS)"},
	{L"\x2478", L"(ÇT)"},
	{L"\x2479", L"(ÇU)"},
	{L"\x247A", L"(ÇV)"},
	{L"\x247B", L"(ÇW)"},
	{L"\x247C", L"(ÇX)"},
	{L"\x247D", L"(10)"},
	{L"\x247E", L"(11)"},
	{L"\x247F", L"(12)"},
	{L"\x3251", L"(21)"},
	{L"\x3252", L"(22)"},
	{L"\x3253", L"(23)"},
	{L"\x3254", L"(24)"},//94ãÊ32ì_
	{L"\xD83C\xDD10", L"(Ç`)"},
	{L"\xD83C\xDD11", L"(Ça)"},
	{L"\xD83C\xDD12", L"(Çb)"},
	{L"\xD83C\xDD13", L"(Çc)"},
	{L"\xD83C\xDD14", L"(Çd)"},
	{L"\xD83C\xDD15", L"(Çe)"},
	{L"\xD83C\xDD16", L"(Çf)"},
	{L"\xD83C\xDD17", L"(Çg)"},
	{L"\xD83C\xDD18", L"(Çh)"},
	{L"\xD83C\xDD19", L"(Çi)"},
	{L"\xD83C\xDD1A", L"(Çj)"},
	{L"\xD83C\xDD1B", L"(Çk)"},
	{L"\xD83C\xDD1C", L"(Çl)"},
	{L"\xD83C\xDD1D", L"(Çm)"},
	{L"\xD83C\xDD1E", L"(Çn)"},
	{L"\xD83C\xDD1F", L"(Ço)"},//94ãÊ48ì_
	{L"\xD83C\xDD20", L"(Çp)"},
	{L"\xD83C\xDD21", L"(Çq)"},
	{L"\xD83C\xDD22", L"(Çr)"},
	{L"\xD83C\xDD23", L"(Çs)"},
	{L"\xD83C\xDD24", L"(Çt)"},
	{L"\xD83C\xDD25", L"(Çu)"},
	{L"\xD83C\xDD26", L"(Çv)"},
	{L"\xD83C\xDD27", L"(Çw)"},
	{L"\xD83C\xDD28", L"(Çx)"},
	{L"\xD83C\xDD29", L"(Çy)"},
	{L"\x3255", L"(25)"},
	{L"\x3256", L"(26)"},
	{L"\x3257", L"(27)"},
	{L"\x3258", L"(28)"},
	{L"\x3259", L"(29)"},
	{L"\x325A", L"(30)"},//94ãÊ64ì_
	{L"á@", L"á@"},
	{L"áA", L"áA"},
	{L"áB", L"áB"},
	{L"áC", L"áC"},
	{L"áD", L"áD"},
	{L"áE", L"áE"},
	{L"áF", L"áF"},
	{L"áG", L"áG"},
	{L"áH", L"áH"},
	{L"áI", L"áI"},
	{L"áJ", L"áJ"},
	{L"áK", L"áK"},
	{L"áL", L"áL"},
	{L"áM", L"áM"},
	{L"áN", L"áN"},
	{L"áO", L"áO"},//94ãÊ80ì_
	{L"\x2776", L"(ÇP)"},
	{L"\x2777", L"(ÇQ)"},
	{L"\x2778", L"(ÇR)"},
	{L"\x2779", L"(ÇS)"},
	{L"\x277A", L"(ÇT)"},
	{L"\x277B", L"(ÇU)"},
	{L"\x277C", L"(ÇV)"},
	{L"\x277D", L"(ÇW)"},
	{L"\x277E", L"(ÇX)"},
	{L"\x277F", L"(10)"},
	{L"\x24EB", L"(11)"},
	{L"\x24EC", L"(12)"},
	{L"\x325B", L"(31)"}
};

static const GAIJI_TABLE GaijiTbl2[]={
	{L"\x3402", L"Å¨"},
	{L"\xD840\xDD58", L"í‡"},
	{L"\x4EFD", L"Å¨"},
	{L"\x4EFF", L"úf"},
	{L"˙q", L"˙q"},
	{L"˙a", L"˙a"},
	{L"\x509C", L"ús"},
	{L"\x511E", L"Å¨"},
	{L"\x51BC", L"Å¨"},
	{L"\x351F", L"Å¨"}, //10
	{L"˙ä", L"˙ä"},
	{L"\x5361", L"Å¨"},
	{L"\x536C", L"Å¨"},
	{L"˚•", L"˚•"},
	{L"\xD842\xDFB7", L"ãg"},
	{L"\x544D", L"Å¨"},
	{L"\x5496", L"Å¨"},
	{L"˙ë", L"˙ë"},
	{L"˙ì", L"˙ì"},
	{L"\x550E", L"Å¨"}, //20
	{L"\x554A", L"Å¨"},
	{L"\x5672", L"Å¨"},
	{L"\x56E4", L"Å¨"},
	{L"\x5733", L"Å¨"},
	{L"\x5734", L"Å¨"},
	{L"˙ú", L"˙ú"},
	{L"\x5880", L"Å¨"},
	{L"\x59E4", L"Å¨"},
	{L"\x5A23", L"Å¨"},
	{L"\x5A55", L"Å¨"}, //30
	{L"˙™", L"˙™"},
	{L"˙±", L"˙±"},
	{L"\x37E2", L"Å¨"},
	{L"\x5EAC", L"Å¨"},
	{L"˙∏", L"˙∏"},
	{L"˙g", L"˙g"},
	{L"˙∫", L"˙∫"},
	{L"\x6017", L"Å¨"},
	{L"\xFA6B", L"åb"},
	{L"˙≈", L"˙≈"}, //40
	{L"˙‘", L"˙‘"},
	{L"\x66C8", L"Å¨"},
	{L"èå", L"èå"},
	{L"˙ﬁ", L"˙ﬁ"},
	{L"˙f", L"˙f"},
	{L"˙„", L"˙„"},
	{L"\x9FC4", L"Å¨"},
	{L"\x6911", L"Å¨"},
	{L"\x693B", L"Å¨"},
	{L"\x6A45", L"Å¨"}, //50
	{L"\x6A91", L"Å¨"},
	{L"ã˘", L"ã˘"},
	{L"\xD84C\xDFCC", L"Å¨"},
	{L"\xD84C\xDFFE", L"Å¨"},
	{L"\xD84D\xDDC4", L"Å¨"},
	{L"\x6BF1", L"Å¨"},
	{L"\x6CE0", L"ó‚"},
	{L"\x6D2E", L"Å¨"},
	{L"\xFA45", L"äC"},
	{L"\x6DBF", L"Å¨"}, //60
	{L"\x6DCA", L"Å¨"},
	{L"˚C", L"˚C"},
	{L"\xFA46", L"èç"},
	{L"\x6F5E", L"Å¨"},
	{L"\x6FF9", L"Å¨"},
	{L"\x7064", L"Å¨"},
	{L"\xFA6C", L"Å¨"},
	{L"\xD850\xDEEE", L"Å¨"},
	{L"˚W", L"˚W"},
	{L"˚Y", L"˚Y"}, //70
	{L"\x7200", L"Å¨"},
	{L"\x739F", L"Å¨"},
	{L"\x73A8", L"Å¨"},
	{L"˚a", L"˚a"},
	{L"˚b", L"˚b"},
	{L"\x741B", L"Å¨"},
	{L"\x7421", L"Å¨"},
	{L"\xFA4A", L"ëÙ"},
	{L"˚g", L"˚g"},
	{L"˚h", L"˚h"}, //80
	{L"\x742C", L"Å¨"},
	{L"\x7439", L"Å¨"},
	{L"\x744B", L"Å¨"},
	{L"\x3EDA", L"Å¨"},
	{L"\x7575", L"·`"},
	{L"\x7581", L"Å¨"},
	{L"\x7772", L"Å¨"},
	{L"\x4093", L"Å¨"},
	{L"\x78C8", L"Å¨"},
	{L"\x78E0", L"Å¨"}, //90
	{L"ã_", L"ã_"},
	{L"‚X", L"‚X"},
	{L"\x9FC6", L"Å¨"},
	{L"\x4103", L"Å¨"},
	{L"\x9FC5", L"Å¨"},
	{L"\x79DA", L"Å¨"},
	{L"\x7A1E", L"Å¨"},
	{L"\x7B7F", L"Å¨"},
	{L"\x7C31", L"Å¨"},
	{L"\x4264", L"Å¨"}, //100
	{L"\x7D8B", L"Å¨"},
	{L"˚ë", L"˚ë"},
	{L"\x8118", L"Å¨"},
	{L"\x813A", L"Å¨"},
	{L"\xFA6D", L"ä⁄"},
	{L"\x82AE", L"Å¨"},
	{L"äã", L"äã"},
	{L"˙`", L"˙`"},
	{L"ñH", L"ñH"},
	{L"˚õ", L"˚õ"}, //110
	{L"\x85CE", L"Å¨"},
	{L"êI", L"êI"},
	{L"\x87EC", L"ê‰"},
	{L"\x880B", L"Å¨"},
	{L"˚¢", L"˚¢"},
	{L"äp", L"äp"},
	{L"˚™", L"˚™"},
	{L"\x8DCE", L"Å¨"},
	{L"í“", L"í“"},
	{L"\x8FF6", L"Å¨"}, //120
	{L"\x90DD", L"Å¨"},
	{L"˚π", L"˚π"},
	{L"ìA", L"ìA"},
	{L"\x91B2", L"Å¨"},
	{L"\x9233", L"Å¨"},
	{L"˙_", L"˙_"},
	{L"˚ÿ", L"˚ÿ"},
	{L"˙^", L"˙^"},
	{L"˚Ë", L"˚Ë"},
	{L"\x96DE", L"Å¨"}, //130
	{L"ÈL", L"ÈL"},
	{L"\x9940", L"Å¨"},
	{L"˚¸", L"˚¸"},
	{L"éI", L"éI"},
	{L"\x9DD7", L"â®"},
	{L"\x9EB4", L"çç"},
	{L"\x9EB5", L"ñÀ"}
};

static BYTE DefaultMacro0[]={
	0x1B,0x24,0x39,0x1B,0x29,0x4A,0x1B,0x2A,0x30,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D
};
static BYTE DefaultMacro1[]={
	0x1B,0x24,0x39,0x1B,0x29,0x31,0x1B,0x2A,0x30,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D
};
static BYTE DefaultMacro2[]={
	0x1B,0x24,0x39,0x1B,0x29,0x20,0x41,0x1B,0x2A,0x30,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D
};
static BYTE DefaultMacro3[]={
	0x1B,0x28,0x32,0x1B,0x29,0x34,0x1B,0x2A,0x35,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D
};
static BYTE DefaultMacro4[]={
	0x1B,0x28,0x32,0x1B,0x29,0x33,0x1B,0x2A,0x35,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D
};
static BYTE DefaultMacro5[]={
	0x1B,0x28,0x32,0x1B,0x29,0x20,0x41,0x1B,0x2A,0x35,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D
};
static BYTE DefaultMacro6[]={
	0x1B,0x28,0x20,0x41,0x1B,0x29,0x20,0x42,0x1B,0x2A,0x20,0x43,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D
};
static BYTE DefaultMacro7[]={
	0x1B,0x28,0x20,0x44,0x1B,0x29,0x20,0x45,0x1B,0x2A,0x20,0x46,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D
};
static BYTE DefaultMacro8[]={
	0x1B,0x28,0x20,0x47,0x1B,0x29,0x20,0x48,0x1B,0x2A,0x20,0x49,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D
};
static BYTE DefaultMacro9[]={
	0x1B,0x28,0x20,0x4A,0x1B,0x29,0x20,0x4B,0x1B,0x2A,0x20,0x4C,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D
};
static BYTE DefaultMacroA[]={
	0x1B,0x28,0x20,0x4D,0x1B,0x29,0x20,0x4E,0x1B,0x2A,0x20,0x4F,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D
};
static BYTE DefaultMacroB[]={
	0x1B,0x24,0x39,0x1B,0x29,0x20,0x42,0x1B,0x2A,0x30,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D
};
static BYTE DefaultMacroC[]={
	0x1B,0x24,0x39,0x1B,0x29,0x20,0x43,0x1B,0x2A,0x30,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D
};
static BYTE DefaultMacroD[]={
	0x1B,0x24,0x39,0x1B,0x29,0x20,0x44,0x1B,0x2A,0x30,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D
};
static BYTE DefaultMacroE[]={
	0x1B,0x28,0x31,0x1B,0x29,0x30,0x1B,0x2A,0x4A,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D
};
static BYTE DefaultMacroF[]={
	0x1B,0x28,0x4A,0x1B,0x29,0x32,0x1B,0x2A,0x20,0x41,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D
};

#endif //ARIB8CHAR_DECODE_H_IMPLEMENT_TABLE

//ï∂éöÉTÉCÉY
typedef enum{
	STR_SMALL = 0, //SSZ
	STR_MEDIUM, //MSZ
	STR_NORMAL, //NSZ
	STR_MICRO, //SZX 0x60
	STR_HIGH_W, //SZX 0x41
	STR_WIDTH_W, //SZX 0x44
	STR_W, //SZX 0x45
	STR_SPECIAL_1, //SZX 0x6B
	STR_SPECIAL_2, //SZX 0x64
} STRING_SIZE;

struct CAPTION_CHAR_DATA{
	wstring strDecode;
	STRING_SIZE emCharSizeMode;

	CLUT_DAT stCharColor;
	CLUT_DAT stBackColor;
	CLUT_DAT stRasterColor;

	BOOL bUnderLine;
	BOOL bShadow;
	BOOL bBold;
	BOOL bItalic;
	BYTE bFlushMode;

	WORD wCharW;
	WORD wCharH;
	WORD wCharHInterval;
	WORD wCharVInterval;
};

struct CAPTION_DATA{
	BOOL bClear;
	WORD wSWFMode;
	WORD wClientX;
	WORD wClientY;
	WORD wClientW;
	WORD wClientH;
	WORD wPosX;
	WORD wPosY;
	vector<CAPTION_CHAR_DATA> CharList;
	DWORD dwWaitTime;
};

class CARIB8CharDecode
{
public:
	CARIB8CharDecode(void);
	~CARIB8CharDecode(void);

	//PSI/SIÇëzíËÇµÇΩwstringÇ÷ÇÃïœä∑
	BOOL PSISI( const BYTE* pbSrc, DWORD dwSrcSize, wstring* strDec );
	//éöñãÇëzíËÇµÇΩwstringÇ÷ÇÃïœä∑
	BOOL Caption( const BYTE* pbSrc, DWORD dwSrcSize, vector<CAPTION_DATA>* pCaptionList );

protected:
	struct MF_MODE{
		int iMF; //ï∂éöïÑçÜèWçá
		int iMode; //ïÑçÜèWçáÇÃï™óﬁ
		int iByte; //ì«Ç›çûÇ›ÉoÉCÉgêî
	};

	BOOL m_bPSI;

	MF_MODE m_G0;
	MF_MODE m_G1;
	MF_MODE m_G2;
	MF_MODE m_G3;
	MF_MODE* m_GL;
	MF_MODE* m_GR;

	//ÉfÉRÅ[ÉhÇµÇΩï∂éöóÒ
	wstring m_strDecode;
	//ï∂éöÉTÉCÉY
	STRING_SIZE m_emStrSize;

	//CLUTÇÃÉCÉìÉfÉbÉNÉX
	BYTE m_bCharColorIndex;
	BYTE m_bBackColorIndex;
	BYTE m_bRasterColorIndex;
	BYTE m_bDefPalette;

	BOOL m_bUnderLine;
	BOOL m_bShadow;
	BOOL m_bBold;
	BOOL m_bItalic;
	BYTE m_bFlushMode;

	//ï\é¶èëéÆ
	WORD m_wSWFMode;
	WORD m_wClientX;
	WORD m_wClientY;
	WORD m_wClientW;
	WORD m_wClientH;
	WORD m_wPosX;
	WORD m_wPosY;
	WORD m_wCharW;
	WORD m_wCharH;
	WORD m_wCharHInterval;
	WORD m_wCharVInterval;
	WORD m_wMaxChar;

	DWORD m_dwWaitTime;

	vector<CAPTION_DATA>* m_pCaptionList;
protected:
	void InitPSISI(void);
	void InitCaption(void);
	BOOL Analyze( const BYTE* pbSrc, DWORD dwSrcSize, DWORD* pdwReadSize );

	BOOL IsSmallCharMode(void);
	BOOL IsChgPos(void);
	void CreateCaptionData(CAPTION_DATA* pItem);
	void CreateCaptionCharData(CAPTION_CHAR_DATA* pItem);
	void CheckModify(void);

	//êßå‰ïÑçÜ
	BOOL C0( const BYTE* pbSrc, DWORD* pdwReadSize );
	BOOL C1( const BYTE* pbSrc, DWORD* pdwReadSize );
	BOOL GL( const BYTE* pbSrc, DWORD* pdwReadSize );
	BOOL GR( const BYTE* pbSrc, DWORD* pdwReadSize );
	//ÉVÉìÉOÉãÉVÉtÉg
	BOOL SS2( const BYTE* pbSrc, DWORD* pdwReadSize );
	BOOL SS3( const BYTE* pbSrc, DWORD* pdwReadSize );
	//ÉGÉXÉPÅ[ÉvÉVÅ[ÉPÉìÉX
	BOOL ESC( const BYTE* pbSrc, DWORD* pdwReadSize );
	//ÇQÉoÉCÉgï∂éöïœä∑
	BOOL ToSJIS( const BYTE bFirst, const BYTE bSecond );
	BOOL ToCustomFont( const BYTE bFirst, const BYTE bSecond );

	BOOL CSI( const BYTE* pbSrc, DWORD* pdwReadSize );

};
