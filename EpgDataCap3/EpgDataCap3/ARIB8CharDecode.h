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
	unsigned short usARIB8;
	const WCHAR* strChar;
};

static const GAIJI_TABLE GaijiTable[]={
	{0x7A4D, L"10."},
	{0x7A4E, L"11."},
	{0x7A4F, L"12."},
	{0x7A50, L"[HV]"}, //90ãÊ48ì_
	{0x7A51, L"[SD]"},
	{0x7A52, L"[Ço]"},
	{0x7A53, L"[Çv]"},
	{0x7A54, L"[MV]"},
	{0x7A55, L"[éË]"},
	{0x7A56, L"[éö]"},
	{0x7A57, L"[ëo]"},
	{0x7A58, L"[Éf]"},
	{0x7A59, L"[Çr]"},
	{0x7A5A, L"[ìÒ]"},
	{0x7A5B, L"[ëΩ]"},
	{0x7A5C, L"[â]"},
	{0x7A5D, L"[SS]"},
	{0x7A5E, L"[Ça]"},
	{0x7A5F, L"[Çm]"},//
	{0x7A60, L"Å°"},//90ãÊ64ì_
	{0x7A61, L"Åú"},
	{0x7A62, L"[ìV]"},
	{0x7A63, L"[å]"},
	{0x7A64, L"[âf]"},
	{0x7A65, L"[ñ≥]"},
	{0x7A66, L"[óø]"},
	{0x7A67, L"[ÅE]"},
	{0x7A68, L"[ëO]"},
	{0x7A69, L"[å„]"},
	{0x7A6A, L"[çƒ]"},
	{0x7A6B, L"[êV]"},
	{0x7A6C, L"[èâ]"},
	{0x7A6D, L"[èI]"},
	{0x7A6E, L"[ê∂]"},
	{0x7A6F, L"[îÃ]"},
	{0x7A70, L"[ê∫]"},//90ãÊ80ì_
	{0x7A71, L"[êÅ]"},
	{0x7A72, L"[PPV]"},
	{0x7A73, L"(îÈ)"},
	{0x7A74, L"ÇŸÇ©"},
	//91ãÊÇÕîÚÇŒÇ∑
	{0x7C21, L"Å®"},//92ãÊ1ì_
	{0x7C22, L"Å©"},
	{0x7C23, L"Å™"},
	{0x7C24, L"Å´"},
	{0x7C25, L"Åõ"},
	{0x7C26, L"Åú"},
	{0x7C27, L"îN"},
	{0x7C28, L"åé"},
	{0x7C29, L"ì˙"},
	{0x7C2A, L"â~"},
	{0x7C2B, L"m^2"},
	{0x7C2C, L"m^3"},
	{0x7C2D, L"cm"},
	{0x7C2E, L"cm^2"},
	{0x7C2F, L"cm^3"},
	{0x7C30, L"ÇO."},//92ãÊ16ì_
	{0x7C31, L"ÇP."},
	{0x7C32, L"ÇQ."},
	{0x7C33, L"ÇR."},
	{0x7C34, L"ÇS."},
	{0x7C35, L"ÇT."},
	{0x7C36, L"ÇU."},
	{0x7C37, L"ÇV."},
	{0x7C38, L"ÇW."},
	{0x7C39, L"ÇX."},
	{0x7C3A, L"éÅ"},
	{0x7C3B, L"ïõ"},
	{0x7C3C, L"å≥"},
	{0x7C3D, L"åÃ"},
	{0x7C3E, L"ëO"},
	{0x7C3F, L"êV"},
	{0x7C40, L"ÇO,"},//92ãÊ32ì_
	{0x7C41, L"ÇP,"},
	{0x7C42, L"ÇQ,"},
	{0x7C43, L"ÇR,"},
	{0x7C44, L"ÇS,"},
	{0x7C45, L"ÇT,"},
	{0x7C46, L"ÇU,"},
	{0x7C47, L"ÇV,"},
	{0x7C48, L"ÇW,"},
	{0x7C49, L"ÇX,"},
	{0x7C4A, L"[é–]"},
	{0x7C4B, L"[ç‡]"},
	{0x7C4C, L"[óL]"},
	{0x7C4D, L"[äî]"},
	{0x7C4E, L"[ë„]"},
	{0x7C4F, L"(ñ‚)"},
	{0x7C50, L"Å£"},//92ãÊ48ì_
	{0x7C51, L"Å•"},
	{0x7C52, L"Åy"},
	{0x7C53, L"Åz"},
	{0x7C54, L"Åû"},
	{0x7C55, L"^2"},
	{0x7C56, L"^3"},
	{0x7C57, L"(CD)"},
	{0x7C58, L"(vn)"},
	{0x7C59, L"(ob)"},
	{0x7C5A, L"(cb)"},
	{0x7C5B, L"(ce"},
	{0x7C5C, L"mb)"},
	{0x7C5D, L"(hp)"},
	{0x7C5E, L"(br)"},
	{0x7C5F, L"(Çê)"},
	{0x7C60, L"(Çì)"},//92ãÊ64ì_
	{0x7C61, L"(ms)"},
	{0x7C62, L"(Çî)"},
	{0x7C63, L"(bs)"},
	{0x7C64, L"(ÇÇ)"},
	{0x7C65, L"(tb)"},
	{0x7C66, L"(tp)"},
	{0x7C67, L"(ds)"},
	{0x7C68, L"(ag)"},
	{0x7C69, L"(eg)"},
	{0x7C6A, L"(vo)"},
	{0x7C6B, L"(fl)"},
	{0x7C6C, L"(ke"},
	{0x7C6D, L"y)"},
	{0x7C6E, L"(sa"},
	{0x7C6F, L"x)"},
	{0x7C70, L"(sy"},//92ãÊ80ì_
	{0x7C71, L"n)"},
	{0x7C72, L"(or"},
	{0x7C73, L"g)"},
	{0x7C74, L"(pe"},
	{0x7C75, L"r)"},
	{0x7C76, L"(Çq)"},
	{0x7C77, L"(Çb)"},
	{0x7C78, L"(‚µ)"},
	{0x7C79, L"ÇcÇi"},
	{0x7C7A, L"[ââ]"},
	{0x7C7B, L"Fax"},
	{0x7D21, L"(åé)"},//93ãÊ1ì_
	{0x7D22, L"(âŒ)"},
	{0x7D23, L"(êÖ)"},
	{0x7D24, L"(ñÿ)"},
	{0x7D25, L"(ã‡)"},
	{0x7D26, L"(ìy)"},
	{0x7D27, L"(ì˙)"},
	{0x7D28, L"(èj)"},
	{0x7D29, L"áç"},
	{0x7D2A, L"áé"},
	{0x7D2B, L"áè"},
	{0x7D2C, L"á~"},
	{0x7D2D, L"No."},
	{0x7D2E, L"Tel"},
	{0x7D2F, L"(Åß)"},
	{0x7D30, L"()()"},//93ãÊ16ì_
	{0x7D31, L"[ñ{]"},
	{0x7D32, L"[éO]"},
	{0x7D33, L"[ìÒ]"},
	{0x7D34, L"[à¿]"},
	{0x7D35, L"[ì_]"},
	{0x7D36, L"[ë≈]"},
	{0x7D37, L"[ìê]"},
	{0x7D38, L"[èü]"},
	{0x7D39, L"[îs]"},
	{0x7D3A, L"[Çr]"},
	{0x7D3B, L"[ìä]"},
	{0x7D3C, L"[ïﬂ]"},
	{0x7D3D, L"[àÍ]"},
	{0x7D3E, L"[ìÒ]"},
	{0x7D3F, L"[éO]"},
	{0x7D40, L"[óV]"},//93ãÊ32ì_
	{0x7D41, L"[ç∂]"},
	{0x7D42, L"[íÜ]"},
	{0x7D43, L"[âE]"},
	{0x7D44, L"[éw]"},
	{0x7D45, L"[ëñ]"},
	{0x7D46, L"[ë≈]"},
	{0x7D47, L"l"},
	{0x7D48, L"kg"},
	{0x7D49, L"Hz"},
	{0x7D4A, L"ha"},
	{0x7D4B, L"km"},
	{0x7D4C, L"km^2"},
	{0x7D4D, L"hPa"},
	{0x7D4E, L"ÅE"},
	{0x7D4F, L"ÅE"},
	{0x7D50, L"1/2"},//93ãÊ48ì_
	{0x7D51, L"0/3"},
	{0x7D52, L"1/3"},
	{0x7D53, L"2/3"},
	{0x7D54, L"1/4"},
	{0x7D55, L"3/4"},
	{0x7D56, L"1/5"},
	{0x7D57, L"2/5"},
	{0x7D58, L"3/5"},
	{0x7D59, L"4/5"},
	{0x7D5A, L"1/6"},
	{0x7D5B, L"5/6"},
	{0x7D5C, L"1/7"},
	{0x7D5D, L"1/8"},
	{0x7D5E, L"1/9"},
	{0x7D5F, L"1/10"},
	{0x7D6E, L"!!"},//93ãÊ78ì_
	{0x7D6F, L"!?"},
	{0x7E21, L"áT"},//94ãÊ1ì_
	{0x7E22, L"áU"},
	{0x7E23, L"áV"},
	{0x7E24, L"áW"},
	{0x7E25, L"áX"},
	{0x7E26, L"áY"},
	{0x7E27, L"áZ"},
	{0x7E28, L"á["},
	{0x7E29, L"á\"},
	{0x7E2A, L"á]"},
	{0x7E2B, L"XI"},
	{0x7E2C, L"XII"},
	{0x7E2D, L"áP"},
	{0x7E2E, L"áQ"},
	{0x7E2F, L"áR"},
	{0x7E30, L"áS"},//94ãÊ16ì_
	{0x7E31, L"(ÇP)"},
	{0x7E32, L"(ÇQ)"},
	{0x7E33, L"(ÇR)"},
	{0x7E34, L"(ÇS)"},
	{0x7E35, L"(ÇT)"},
	{0x7E36, L"(ÇU)"},
	{0x7E37, L"(ÇV)"},
	{0x7E38, L"(ÇW)"},
	{0x7E39, L"(ÇX)"},
	{0x7E3A, L"(10)"},
	{0x7E3B, L"(11)"},
	{0x7E3C, L"(12)"},
	{0x7E3D, L"(21)"},
	{0x7E3E, L"(22)"},
	{0x7E3F, L"(23)"},
	{0x7E40, L"(24)"},//94ãÊ32ì_
	{0x7E41, L"(Ç`)"},
	{0x7E42, L"(Ça)"},
	{0x7E43, L"(Çb)"},
	{0x7E44, L"(Çc)"},
	{0x7E45, L"(Çd)"},
	{0x7E46, L"(Çe)"},
	{0x7E47, L"(Çf)"},
	{0x7E48, L"(Çg)"},
	{0x7E49, L"(Çh)"},
	{0x7E4A, L"(Çi)"},
	{0x7E4B, L"(Çj)"},
	{0x7E4C, L"(Çk)"},
	{0x7E4D, L"(Çl)"},
	{0x7E4E, L"(Çm)"},
	{0x7E4F, L"(Çn)"},
	{0x7E50, L"(Ço)"},//94ãÊ48ì_
	{0x7E51, L"(Çp)"},
	{0x7E52, L"(Çq)"},
	{0x7E53, L"(Çr)"},
	{0x7E54, L"(Çs)"},
	{0x7E55, L"(Çt)"},
	{0x7E56, L"(Çu)"},
	{0x7E57, L"(Çv)"},
	{0x7E58, L"(Çw)"},
	{0x7E59, L"(Çx)"},
	{0x7E5A, L"(Çy)"},
	{0x7E5B, L"(25)"},
	{0x7E5C, L"(26)"},
	{0x7E5D, L"(27)"},
	{0x7E5E, L"(28)"},
	{0x7E5F, L"(29)"},
	{0x7E60, L"(30)"},//94ãÊ64ì_
	{0x7E61, L"á@"},
	{0x7E62, L"áA"},
	{0x7E63, L"áB"},
	{0x7E64, L"áC"},
	{0x7E65, L"áD"},
	{0x7E66, L"áE"},
	{0x7E67, L"áF"},
	{0x7E68, L"áG"},
	{0x7E69, L"áH"},
	{0x7E6A, L"áI"},
	{0x7E6B, L"áJ"},
	{0x7E6C, L"áK"},
	{0x7E6D, L"áL"},
	{0x7E6E, L"áM"},
	{0x7E6F, L"áN"},
	{0x7E70, L"áO"},//94ãÊ80ì_
	{0x7E71, L"(ÇP)"},
	{0x7E72, L"(ÇQ)"},
	{0x7E73, L"(ÇR)"},
	{0x7E74, L"(ÇS)"},
	{0x7E75, L"(ÇT)"},
	{0x7E76, L"(ÇU)"},
	{0x7E77, L"(ÇV)"},
	{0x7E78, L"(ÇW)"},
	{0x7E79, L"(ÇX)"},
	{0x7E7A, L"(10)"},
	{0x7E7B, L"(11)"},
	{0x7E7C, L"(12)"},
	{0x7E7D, L"(31)"}
};

static const GAIJI_TABLE GaijiTbl2[]={
	{0x7521, L"Å¨"},
	{0x7522, L"í‡"},
	{0x7523, L"Å¨"},
	{0x7524, L"úf"},
	{0x7525, L"˙q"},
	{0x7526, L"˙a"},
	{0x7527, L"ús"},
	{0x7528, L"Å¨"},
	{0x7529, L"Å¨"},
	{0x752A, L"Å¨"}, //10
	{0x752B, L"˙ä"},
	{0x752C, L"Å¨"},
	{0x752D, L"Å¨"},
	{0x752E, L"˚•"},
	{0x752F, L"ãg"},
	{0x7530, L"Å¨"},
	{0x7531, L"Å¨"},
	{0x7532, L"˙ë"},
	{0x7533, L"˙ì"},
	{0x7534, L"Å¨"}, //20
	{0x7535, L"Å¨"},
	{0x7536, L"Å¨"},
	{0x7537, L"Å¨"},
	{0x7538, L"Å¨"},
	{0x7539, L"Å¨"},
	{0x753A, L"˙ú"},
	{0x753B, L"Å¨"},
	{0x753C, L"Å¨"},
	{0x753D, L"Å¨"},
	{0x753E, L"Å¨"}, //30
	{0x753F, L"˙™"},
	{0x7540, L"˙±"},
	{0x7541, L"Å¨"},
	{0x7542, L"Å¨"},
	{0x7543, L"˙∏"},
	{0x7544, L"˙g"},
	{0x7545, L"˙∫"},
	{0x7546, L"Å¨"},
	{0x7547, L"åb"},
	{0x7548, L"˙≈"}, //40
	{0x7549, L"˙‘"},
	{0x754A, L"Å¨"},
	{0x754B, L"èå"},
	{0x754C, L"˙ﬁ"},
	{0x754D, L"˙f"},
	{0x754E, L"˙„"},
	{0x754F, L"Å¨"},
	{0x7550, L"Å¨"},
	{0x7551, L"Å¨"},
	{0x7552, L"Å¨"}, //50
	{0x7553, L"Å¨"},
	{0x7554, L"ã˘"},
	{0x7555, L"Å¨"},
	{0x7556, L"Å¨"},
	{0x7557, L"Å¨"},
	{0x7558, L"Å¨"},
	{0x7559, L"ó‚"},
	{0x755A, L"Å¨"},
	{0x755B, L"äC"},
	{0x755C, L"Å¨"}, //60
	{0x755D, L"Å¨"},
	{0x755E, L"˚C"},
	{0x755F, L"èç"},
	{0x7560, L"Å¨"},
	{0x7561, L"Å¨"},
	{0x7562, L"Å¨"},
	{0x7563, L"Å¨"},
	{0x7564, L"Å¨"},
	{0x7565, L"˚W"},
	{0x7566, L"˚Y"}, //70
	{0x7567, L"Å¨"},
	{0x7568, L"Å¨"},
	{0x7569, L"Å¨"},
	{0x756A, L"˚a"},
	{0x756B, L"˚b"},
	{0x756C, L"Å¨"},
	{0x756D, L"Å¨"},
	{0x756E, L"ëÙ"},
	{0x756F, L"˚g"},
	{0x7570, L"˚h"}, //80
	{0x7571, L"Å¨"},
	{0x7572, L"Å¨"},
	{0x7573, L"Å¨"},
	{0x7574, L"Å¨"},
	{0x7575, L"·`"},
	{0x7576, L"Å¨"},
	{0x7577, L"Å¨"},
	{0x7578, L"Å¨"},
	{0x7579, L"Å¨"},
	{0x757A, L"Å¨"}, //90
	{0x757B, L"ã_"},
	{0x757C, L"‚X"},
	{0x757D, L"Å¨"},
	{0x757E, L"Å¨"},
	{0x7621, L"Å¨"},
	{0x7622, L"Å¨"},
	{0x7623, L"Å¨"},
	{0x7624, L"Å¨"},
	{0x7625, L"Å¨"},
	{0x7626, L"Å¨"}, //100
	{0x7627, L"Å¨"},
	{0x7628, L"˚ë"},
	{0x7629, L"Å¨"},
	{0x762A, L"Å¨"},
	{0x762B, L"ä⁄"},
	{0x762C, L"Å¨"},
	{0x762D, L"äã"},
	{0x762E, L"˙`"},
	{0x762F, L"ñH"},
	{0x7630, L"˚õ"}, //110
	{0x7631, L"Å¨"},
	{0x7632, L"êI"},
	{0x7633, L"ê‰"},
	{0x7634, L"Å¨"},
	{0x7635, L"˚¢"},
	{0x7636, L"äp"},
	{0x7637, L"˚™"},
	{0x7638, L"Å¨"},
	{0x7639, L"í“"},
	{0x763A, L"Å¨"}, //120
	{0x763B, L"Å¨"},
	{0x763C, L"˚π"},
	{0x763D, L"ìA"},
	{0x763E, L"Å¨"},
	{0x763F, L"Å¨"},
	{0x7640, L"˙_"},
	{0x7641, L"˚ÿ"},
	{0x7642, L"˙^"},
	{0x7643, L"˚Ë"},
	{0x7644, L"Å¨"}, //130
	{0x7645, L"ÈL"},
	{0x7646, L"Å¨"},
	{0x7647, L"˚¸"},
	{0x7648, L"éI"},
	{0x7649, L"â®"},
	{0x764A, L"çç"},
	{0x764B, L"ñÀ"}
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
