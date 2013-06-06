#pragma once

// MFCÇ≈égÇ§éûóp
/*#ifdef _DEBUG
#undef new
#endif
#include <string>
#include <vector>
#include <map>
#ifdef _DEBUG
#define new DEBUG_NEW
#endif
*/
#include <string>
#include <vector>
#include <map>
using namespace std;

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

static char AsciiTable[][3]={
	"ÅI","Åh","Åî","Åê","Åì","Åï","Åf",
	"Åi","Åj","Åñ","Å{","ÅC","Å|","ÅD","Å^",
	"ÇO","ÇP","ÇQ","ÇR","ÇS","ÇT","ÇU","ÇV",
	"ÇW","ÇX","ÅF","ÅG","ÅÉ","ÅÅ","ÅÑ","ÅH",
	"Åó","Ç`","Ça","Çb","Çc","Çd","Çe","Çf",
	"Çg","Çh","Çi","Çj","Çk","Çl","Çm","Çn",
	"Ço","Çp","Çq","Çr","Çs","Çt","Çu","Çv",
	"Çw","Çx","Çy","Åm","Åè","Ån","ÅO","ÅQ",
	"Åe","ÇÅ","ÇÇ","ÇÉ","ÇÑ","ÇÖ","ÇÜ","Çá",
	"Çà","Çâ","Çä","Çã","Çå","Çç","Çé","Çè",
	"Çê","Çë","Çí","Çì","Çî","Çï","Çñ","Çó",
	"Çò","Çô","Çö","Åo","Åb","Åp","ÅP"
};
static char HiraTable[][3]={
	"Çü","Ç†","Ç°","Ç¢","Ç£","Ç§","Ç•",
	"Ç¶","Çß","Ç®","Ç©","Ç™","Ç´","Ç¨","Ç≠",
	"ÇÆ","ÇØ","Ç∞","Ç±","Ç≤","Ç≥","Ç¥","Çµ",
	"Ç∂","Ç∑","Ç∏","Çπ","Ç∫","Çª","Çº","ÇΩ",
	"Çæ","Çø","Ç¿","Ç¡","Ç¬","Ç√","Çƒ","Ç≈",
	"Ç∆","Ç«","Ç»","Ç…","Ç ","ÇÀ","ÇÃ","ÇÕ",
	"ÇŒ","Çœ","Ç–","Ç—","Ç“","Ç”","Ç‘","Ç’",
	"Ç÷","Ç◊","Çÿ","ÇŸ","Ç⁄","Ç€","Ç‹","Ç›",
	"Çﬁ","Çﬂ","Ç‡","Ç·","Ç‚","Ç„","Ç‰","ÇÂ",
	"ÇÊ","ÇÁ","ÇË","ÇÈ","ÇÍ","ÇÎ","ÇÏ","ÇÌ",
	"ÇÓ","ÇÔ","Ç","ÇÒ","Å@","Å@","Å@","ÅT",
	"ÅU","Å[","ÅB","Åu","Åv","ÅA","ÅE"
};
static char KanaTable[][3]={
	"É@","ÉA","ÉB","ÉC","ÉD","ÉE","ÉF",
	"ÉG","ÉH","ÉI","ÉJ","ÉK","ÉL","ÉM","ÉN",
	"ÉO","ÉP","ÉQ","ÉR","ÉS","ÉT","ÉU","ÉV",
	"ÉW","ÉX","ÉY","ÉZ","É[","É\","É]","É^",
	"É_","É`","Éa","Éb","Éc","Éd","Ée","Éf",
	"Ég","Éh","Éi","Éj","Ék","Él","Ém","Én",
	"Éo","Ép","Éq","Ér","És","Ét","Éu","Év",
	"Éw","Éx","Éy","Éz","É{","É|","É}","É~",
	"ÉÄ","ÉÅ","ÉÇ","ÉÉ","ÉÑ","ÉÖ","ÉÜ","Éá",
	"Éà","Éâ","Éä","Éã","Éå","Éç","Éé","Éè",
	"Éê","Éë","Éí","Éì","Éî","Éï","Éñ","ÅR",
	"ÅS","Å[","ÅB","Åu","Åv","ÅA","ÅE"
};

typedef struct _GAIJI_TABLE{
	unsigned short usARIB8;
	string strChar;
} GAIJI_TABLE;

static GAIJI_TABLE GaijiTable[]={
	{0x7A4D, "10."},
	{0x7A4E, "11."},
	{0x7A4F, "12."},
	{0x7A50, "[HV]"}, //90ãÊ48ì_
	{0x7A51, "[SD]"},
	{0x7A52, "[Ço]"},
	{0x7A53, "[Çv]"},
	{0x7A54, "[MV]"},
	{0x7A55, "[éË]"},
	{0x7A56, "[éö]"},
	{0x7A57, "[ëo]"},
	{0x7A58, "[Éf]"},
	{0x7A59, "[Çr]"},
	{0x7A5A, "[ìÒ]"},
	{0x7A5B, "[ëΩ]"},
	{0x7A5C, "[â]"},
	{0x7A5D, "[SS]"},
	{0x7A5E, "[Ça]"},
	{0x7A5F, "[Çm]"},//
	{0x7A60, "Å°"},//90ãÊ64ì_
	{0x7A61, "Åú"},
	{0x7A62, "[ìV]"},
	{0x7A63, "[å]"},
	{0x7A64, "[âf]"},
	{0x7A65, "[ñ≥]"},
	{0x7A66, "[óø]"},
	{0x7A67, "[ÅE]"},
	{0x7A68, "[ëO]"},
	{0x7A69, "[å„]"},
	{0x7A6A, "[çƒ]"},
	{0x7A6B, "[êV]"},
	{0x7A6C, "[èâ]"},
	{0x7A6D, "[èI]"},
	{0x7A6E, "[ê∂]"},
	{0x7A6F, "[îÃ]"},
	{0x7A70, "[ê∫]"},//90ãÊ80ì_
	{0x7A71, "[êÅ]"},
	{0x7A72, "[PPV]"},
	{0x7A73, "(îÈ)"},
	{0x7A74, "ÇŸÇ©"},
	//91ãÊÇÕîÚÇŒÇ∑
	{0x7C21, "Å®"},//92ãÊ1ì_
	{0x7C22, "Å©"},
	{0x7C23, "Å™"},
	{0x7C24, "Å´"},
	{0x7C25, "ÅE"},
	{0x7C26, "ÅE"},
	{0x7C27, "îN"},
	{0x7C28, "åé"},
	{0x7C29, "ì˙"},
	{0x7C2A, "â~"},
	{0x7C2B, "m^2"},
	{0x7C2C, "m^3"},
	{0x7C2D, "cm"},
	{0x7C2E, "cm^2"},
	{0x7C2F, "cm^3"},
	{0x7C30, "ÇO."},//92ãÊ16ì_
	{0x7C31, "ÇP."},
	{0x7C32, "ÇQ."},
	{0x7C33, "ÇR."},
	{0x7C34, "ÇS."},
	{0x7C35, "ÇT."},
	{0x7C36, "ÇU."},
	{0x7C37, "ÇV."},
	{0x7C38, "ÇW."},
	{0x7C39, "ÇX."},
	{0x7C3A, "éÅ"},
	{0x7C3B, "ïõ"},
	{0x7C3C, "å≥"},
	{0x7C3D, "åÃ"},
	{0x7C3E, "ëO"},
	{0x7C3F, "å„"},
	{0x7C40, "ÇO,"},//92ãÊ32ì_
	{0x7C41, "ÇP,"},
	{0x7C42, "ÇQ,"},
	{0x7C43, "ÇR,"},
	{0x7C44, "ÇS,"},
	{0x7C45, "ÇT,"},
	{0x7C46, "ÇU,"},
	{0x7C47, "ÇV,"},
	{0x7C48, "ÇW,"},
	{0x7C49, "ÇX,"},
	{0x7C4A, "[é–]"},
	{0x7C4B, "[ç‡]"},
	{0x7C4C, "[óL]"},
	{0x7C4D, "[äî]"},
	{0x7C4E, "[ë„]"},
	{0x7C4F, "(ñ‚)"},
	{0x7C50, "ÅE"},//92ãÊ48ì_
	{0x7C51, "ÅE"},
	{0x7C52, "ÅE"},
	{0x7C53, "ÅE"},
	{0x7C54, "ÅE"},
	{0x7C55, "ÅE"},
	{0x7C56, "ÅE"},
	{0x7C57, "(CD)"},
	{0x7C58, "(vn)"},
	{0x7C59, "(ob)"},
	{0x7C5A, "(cb)"},
	{0x7C5B, "(ce"},
	{0x7C5C, "mb)"},
	{0x7C5D, "(hp)"},
	{0x7C5E, "(br)"},
	{0x7C5F, "(Çê)"},
	{0x7C60, "(Çì)"},//92ãÊ64ì_
	{0x7C61, "(ms)"},
	{0x7C62, "(Çî)"},
	{0x7C63, "(bs)"},
	{0x7C64, "(ÇÇ)"},
	{0x7C65, "(tb)"},
	{0x7C66, "(tp)"},
	{0x7C67, "(ds)"},
	{0x7C68, "(ag)"},
	{0x7C69, "(eg)"},
	{0x7C6A, "(vo)"},
	{0x7C6B, "(fl)"},
	{0x7C6C, "(ke"},
	{0x7C6D, "y)"},
	{0x7C6E, "(sa"},
	{0x7C6F, "x)"},
	{0x7C70, "(sy"},//92ãÊ80ì_
	{0x7C71, "n)"},
	{0x7C72, "(or"},
	{0x7C73, "g)"},
	{0x7C74, "(pe"},
	{0x7C75, "r)"},
	{0x7C76, "(Çq)"},
	{0x7C77, "(Çb)"},
	{0x7C78, "(‚µ)"},
	{0x7C79, "ÇcÇi"},
	{0x7C7A, "[ââ]"},
	{0x7C7B, "Fax"},
	{0x7D21, "(åé)"},//93ãÊ1ì_
	{0x7D22, "(âŒ)"},
	{0x7D23, "(êÖ)"},
	{0x7D24, "(ñÿ)"},
	{0x7D25, "(ã‡)"},
	{0x7D26, "(ìy)"},
	{0x7D27, "(ì˙)"},
	{0x7D28, "(èj)"},
	{0x7D29, "áç"},
	{0x7D2A, "áé"},
	{0x7D2B, "áè"},
	{0x7D2C, "á~"},
	{0x7D2D, "No."},
	{0x7D2E, "Tel"},
	{0x7D2F, "(Åß)"},
	{0x7D30, "()()"},//93ãÊ16ì_
	{0x7D31, "[ñ{]"},
	{0x7D32, "[éO]"},
	{0x7D33, "[ìÒ]"},
	{0x7D34, "[à¿]"},
	{0x7D35, "[ì_]"},
	{0x7D36, "[ë≈]"},
	{0x7D37, "[ìê]"},
	{0x7D38, "[èü]"},
	{0x7D39, "[îs]"},
	{0x7D3A, "[Çr]"},
	{0x7D3B, "[ìä]"},
	{0x7D3C, "[ïﬂ]"},
	{0x7D3D, "[àÍ]"},
	{0x7D3E, "[ìÒ]"},
	{0x7D3F, "[éO]"},
	{0x7D40, "[óV]"},//93ãÊ32ì_
	{0x7D41, "[ç∂]"},
	{0x7D42, "[íÜ]"},
	{0x7D43, "[âE]"},
	{0x7D44, "[éw]"},
	{0x7D45, "[ëñ]"},
	{0x7D46, "[ë≈]"},
	{0x7D47, "l"},
	{0x7D48, "kg"},
	{0x7D49, "Hz"},
	{0x7D4A, "ha"},
	{0x7D4B, "km"},
	{0x7D4C, "km^2"},
	{0x7D4D, "hPa"},
	{0x7D4E, "ÅE"},
	{0x7D4F, "ÅE"},
	{0x7D50, "1/2"},//93ãÊ48ì_
	{0x7D51, "0/3"},
	{0x7D52, "1/3"},
	{0x7D53, "2/3"},
	{0x7D54, "1/4"},
	{0x7D55, "3/4"},
	{0x7D56, "1/5"},
	{0x7D57, "2/5"},
	{0x7D58, "3/5"},
	{0x7D59, "4/5"},
	{0x7D5A, "1/6"},
	{0x7D5B, "5/6"},
	{0x7D5C, "1/7"},
	{0x7D5D, "1/8"},
	{0x7D5E, "1/9"},
	{0x7D5F, "1/10"},
	{0x7D6E, "!!"},//93ãÊ78ì_
	{0x7D6F, "!?"},
	{0x7E21, "áT"},//94ãÊ1ì_
	{0x7E22, "áU"},
	{0x7E23, "áV"},
	{0x7E24, "áW"},
	{0x7E25, "áX"},
	{0x7E26, "áY"},
	{0x7E27, "áZ"},
	{0x7E28, "á["},
	{0x7E29, "á\"},
	{0x7E2A, "á]"},
	{0x7E2B, "XI"},
	{0x7E2C, "XII"},
	{0x7E2D, "áP"},
	{0x7E2E, "áQ"},
	{0x7E2F, "áR"},
	{0x7E30, "áS"},//94ãÊ16ì_
	{0x7E31, "(ÇP)"},
	{0x7E32, "(ÇQ)"},
	{0x7E33, "(ÇR)"},
	{0x7E34, "(ÇS)"},
	{0x7E35, "(ÇT)"},
	{0x7E36, "(ÇU)"},
	{0x7E37, "(ÇV)"},
	{0x7E38, "(ÇW)"},
	{0x7E39, "(ÇX)"},
	{0x7E3A, "(10)"},
	{0x7E3B, "(11)"},
	{0x7E3C, "(12)"},
	{0x7E3D, "(21)"},
	{0x7E3E, "(22)"},
	{0x7E3F, "(23)"},
	{0x7E40, "(24)"},//94ãÊ32ì_
	{0x7E41, "(Ç`)"},
	{0x7E42, "(Ça)"},
	{0x7E43, "(Çb)"},
	{0x7E44, "(Çc)"},
	{0x7E45, "(Çd)"},
	{0x7E46, "(Çe)"},
	{0x7E47, "(Çf)"},
	{0x7E48, "(Çg)"},
	{0x7E49, "(Çh)"},
	{0x7E4A, "(Çi)"},
	{0x7E4B, "(Çj)"},
	{0x7E4C, "(Çk)"},
	{0x7E4D, "(Çl)"},
	{0x7E4E, "(Çm)"},
	{0x7E4F, "(Çn)"},
	{0x7E50, "(Ço)"},//94ãÊ48ì_
	{0x7E51, "(Çp)"},
	{0x7E52, "(Çq)"},
	{0x7E53, "(Çr)"},
	{0x7E54, "(Çs)"},
	{0x7E55, "(Çt)"},
	{0x7E56, "(Çu)"},
	{0x7E57, "(Çv)"},
	{0x7E58, "(Çw)"},
	{0x7E59, "(Çx)"},
	{0x7E5A, "(Çy)"},
	{0x7E5B, "(25)"},
	{0x7E5C, "(26)"},
	{0x7E5D, "(27)"},
	{0x7E5E, "(28)"},
	{0x7E5F, "(29)"},
	{0x7E60, "(30)"},//94ãÊ64ì_
	{0x7E61, "á@"},
	{0x7E62, "áA"},
	{0x7E63, "áB"},
	{0x7E64, "áC"},
	{0x7E65, "áD"},
	{0x7E66, "áE"},
	{0x7E67, "áF"},
	{0x7E68, "áG"},
	{0x7E69, "áH"},
	{0x7E6A, "áI"},
	{0x7E6B, "áJ"},
	{0x7E6C, "áK"},
	{0x7E6D, "áL"},
	{0x7E6E, "áM"},
	{0x7E6F, "áN"},
	{0x7E70, "áO"},//94ãÊ80ì_
	{0x7E71, "(ÇP)"},
	{0x7E72, "(ÇQ)"},
	{0x7E73, "(ÇR)"},
	{0x7E74, "(ÇS)"},
	{0x7E75, "(ÇT)"},
	{0x7E76, "(ÇU)"},
	{0x7E77, "(ÇV)"},
	{0x7E78, "(ÇW)"},
	{0x7E79, "(ÇX)"},
	{0x7E7A, "(10)"},
	{0x7E7B, "(11)"},
	{0x7E7C, "(12)"},
	{0x7E7D, "(31)"}
};

static GAIJI_TABLE GaijiTbl2[]={
	{0x7521, "Å¨"},
	{0x7522, "Å¨"},
	{0x7523, "Å¨"},
	{0x7524, "Å¨"},
	{0x7525, "˙q"},
	{0x7526, "˙a"},
	{0x7527, "Å¨"},
	{0x7528, "Å¨"},
	{0x7529, "Å¨"},
	{0x752A, "Å¨"}, //10
	{0x752B, "˙ä"},
	{0x752C, "Å¨"},
	{0x752D, "Å¨"},
	{0x752E, "˚•"},
	{0x752F, "Å¨"},
	{0x7530, "Å¨"},
	{0x7531, "Å¨"},
	{0x7532, "Å¨"},
	{0x7533, "˙ì"},
	{0x7534, "Å¨"}, //20
	{0x7535, "Å¨"},
	{0x7536, "Å¨"},
	{0x7537, "Å¨"},
	{0x7538, "Å¨"},
	{0x7539, "Å¨"},
	{0x753A, "˙ú"},
	{0x753B, "Å¨"},
	{0x753C, "Å¨"},
	{0x753D, "Å¨"},
	{0x753E, "Å¨"}, //30
	{0x753F, "˙™"},
	{0x7540, "˙±"},
	{0x7541, "Å¨"},
	{0x7542, "Å¨"},
	{0x7543, "˙∏"},
	{0x7544, "˙g"},
	{0x7545, "˙∫"},
	{0x7546, "Å¨"},
	{0x7547, "Å¨"},
	{0x7548, "˙≈"}, //40
	{0x7549, "˙‘"},
	{0x754A, "Å¨"},
	{0x754B, "èå"},
	{0x754C, "˙ﬁ"},
	{0x754D, "˙f"},
	{0x754E, "Å¨"},
	{0x754F, "Å¨"},
	{0x7550, "Å¨"},
	{0x7551, "Å¨"},
	{0x7552, "Å¨"}, //50
	{0x7553, "Å¨"},
	{0x7554, "ã˘"},
	{0x7555, "Å¨"},
	{0x7556, "Å¨"},
	{0x7557, "Å¨"},
	{0x7558, "Å¨"},
	{0x7559, "Å¨"},
	{0x755A, "Å¨"},
	{0x755B, "Å¨"},
	{0x755C, "Å¨"}, //60
	{0x755D, "Å¨"},
	{0x755E, "Å¨"},
	{0x755F, "Å¨"},
	{0x7560, "Å¨"},
	{0x7561, "Å¨"},
	{0x7562, "Å¨"},
	{0x7563, "Å¨"},
	{0x7564, "Å¨"},
	{0x7565, "˚W"},
	{0x7566, "˚Y"}, //70
	{0x7567, "Å¨"},
	{0x7568, "Å¨"},
	{0x7569, "Å¨"},
	{0x756A, "˚a"},
	{0x756B, "˚b"},
	{0x756C, "Å¨"},
	{0x756D, "Å¨"},
	{0x756E, "Å¨"},
	{0x756F, "˚g"},
	{0x7570, "˚h"}, //80
	{0x7571, "Å¨"},
	{0x7572, "Å¨"},
	{0x7573, "Å¨"},
	{0x7574, "Å¨"},
	{0x7575, "Å¨"},
	{0x7576, "Å¨"},
	{0x7577, "Å¨"},
	{0x7578, "Å¨"},
	{0x7579, "Å¨"},
	{0x757A, "Å¨"}, //90
	{0x757B, "ã_"},
	{0x757C, "‚X"},
	{0x757D, "Å¨"},
	{0x757E, "Å¨"},
	{0x7621, "Å¨"},
	{0x7622, "Å¨"},
	{0x7623, "Å¨"},
	{0x7624, "Å¨"},
	{0x7625, "Å¨"},
	{0x7626, "Å¨"}, //100
	{0x7627, "Å¨"},
	{0x7628, "˚ë"},
	{0x7629, "Å¨"},
	{0x762A, "Å¨"},
	{0x762B, "Å¨"},
	{0x762C, "Å¨"},
	{0x762D, "äã"},
	{0x762E, "˙`"},
	{0x762F, "ñH"},
	{0x7630, "˚õ"}, //110
	{0x7631, "Å¨"},
	{0x7632, "êI"},
	{0x7633, "Å¨"},
	{0x7634, "Å¨"},
	{0x7635, "˚¢"},
	{0x7636, "äp"},
	{0x7637, "˚™"},
	{0x7638, "Å¨"},
	{0x7639, "í“"},
	{0x763A, "Å¨"}, //120
	{0x763B, "Å¨"},
	{0x763C, "˚π"},
	{0x763D, "ìA"},
	{0x763E, "Å¨"},
	{0x763F, "Å¨"},
	{0x7640, "˙_"},
	{0x7641, "˚ÿ"},
	{0x7642, "˙^"},
	{0x7643, "˚Ë"},
	{0x7644, "Å¨"}, //130
	{0x7645, "ÈL"},
	{0x7646, "Å¨"},
	{0x7647, "˚¸"},
	{0x7648, "éI"},
	{0x7649, "Å¨"},
	{0x764A, "Å¨"},
	{0x764B, "Å¨"}
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

typedef struct _CAPTION_CHAR_DATA{
	string strDecode;
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
	//=ÉIÉyÉåÅ[É^Å[ÇÃèàóù
	_CAPTION_CHAR_DATA & operator= (const _CAPTION_CHAR_DATA & o) {
		strDecode=o.strDecode;
		emCharSizeMode = o.emCharSizeMode;
		stCharColor = o.stCharColor;
		stBackColor = o.stBackColor;
		stRasterColor = o.stRasterColor;
		bUnderLine = o.bUnderLine;
		bShadow = o.bShadow;
		bBold = o.bBold;
		bItalic = o.bItalic;
		bFlushMode = o.bFlushMode;
		wCharW = o.wCharH;
		wCharHInterval = o.wCharHInterval;
		wCharVInterval = o.wCharVInterval;
		return *this;
	};
} CAPTION_CHAR_DATA;

typedef struct _CAPTION_DATA{
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
	//=ÉIÉyÉåÅ[É^Å[ÇÃèàóù
	_CAPTION_DATA & operator= (const _CAPTION_DATA & o) {
		bClear=o.bClear;
		wSWFMode = o.wSWFMode;
		wClientX = o.wClientX;
		wClientY = o.wClientY;
		wClientW = o.wClientW;
		wClientH = o.wClientH;
		wPosX = o.wPosX;
		wPosY = o.wPosY;
		CharList = o.CharList;
		dwWaitTime = o.dwWaitTime;
		return *this;
	};
} CAPTION_DATA;

class CARIB8CharDecode
{
public:
	CARIB8CharDecode(void);
	~CARIB8CharDecode(void);

	//PSI/SIÇëzíËÇµÇΩSJISÇ÷ÇÃïœä∑
	BOOL PSISI( const BYTE* pbSrc, DWORD dwSrcSize, string* strDec );
	//éöñãÇëzíËÇµÇΩSJISÇ÷ÇÃïœä∑
	BOOL Caption( const BYTE* pbSrc, DWORD dwSrcSize, vector<CAPTION_DATA>* pCaptionList );

protected:
	typedef struct _MF_MODE{
		int iMF; //ï∂éöïÑçÜèWçá
		int iMode; //ïÑçÜèWçáÇÃï™óﬁ
		int iByte; //ì«Ç›çûÇ›ÉoÉCÉgêî
		//=ÉIÉyÉåÅ[É^Å[ÇÃèàóù
		_MF_MODE & operator= (const _MF_MODE & o) {
			iMF = o.iMF;
			iMode = o.iMode;
			iByte = o.iByte;
			return *this;
		}
	} MF_MODE;

	BOOL m_bPSI;

	MF_MODE m_G0;
	MF_MODE m_G1;
	MF_MODE m_G2;
	MF_MODE m_G3;
	MF_MODE* m_GL;
	MF_MODE* m_GR;

	//ÉfÉRÅ[ÉhÇµÇΩï∂éöóÒ
	string m_strDecode;
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
