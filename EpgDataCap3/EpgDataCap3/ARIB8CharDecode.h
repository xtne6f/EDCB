#pragma once

// MFCで使う時用
/*#ifdef _DEBUG
#undef new
#endif
#ifdef _DEBUG
#define new DEBUG_NEW
#endif
*/

#include "ColorDef.h"

//文字符号集合
//Gセット
#define MF_JIS_KANJI1 0x39 //JIS互換漢字1面
#define MF_JIS_KANJI2 0x3A //JIS互換漢字2面
#define MF_KIGOU 0x3B //追加記号
#define MF_ASCII 0x4A //英数
#define MF_HIRA  0x30 //平仮名
#define MF_KANA  0x31 //片仮名
#define MF_KANJI 0x42 //漢字
#define MF_MOSAIC_A 0x32 //モザイクA
#define MF_MOSAIC_B 0x33 //モザイクB
#define MF_MOSAIC_C 0x34 //モザイクC
#define MF_MOSAIC_D 0x35 //モザイクD
#define MF_PROP_ASCII 0x36 //プロポーショナル英数
#define MF_PROP_HIRA  0x37 //プロポーショナル平仮名
#define MF_PROP_KANA  0x38 //プロポーショナル片仮名
#define MF_JISX_KANA 0x49 //JIX X0201片仮名
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
#define MF_MACRO 0x70 //マクロ

//符号集合の分類
#define MF_MODE_G 1 //Gセット
#define MF_MODE_DRCS 2 //DRCS
#define MF_MODE_OTHER 3 //その他

#ifdef ARIB8CHAR_DECODE_H_IMPLEMENT_TABLE

static const WCHAR AsciiTable[]={
	L'！', L'”', L'＃', L'＄', L'％', L'＆', L'’',
	L'（', L'）', L'＊', L'＋', L'，', L'－', L'．', L'／',
	L'０', L'１', L'２', L'３', L'４', L'５', L'６', L'７',
	L'８', L'９', L'：', L'；', L'＜', L'＝', L'＞', L'？',
	L'＠', L'Ａ', L'Ｂ', L'Ｃ', L'Ｄ', L'Ｅ', L'Ｆ', L'Ｇ',
	L'Ｈ', L'Ｉ', L'Ｊ', L'Ｋ', L'Ｌ', L'Ｍ', L'Ｎ', L'Ｏ',
	L'Ｐ', L'Ｑ', L'Ｒ', L'Ｓ', L'Ｔ', L'Ｕ', L'Ｖ', L'Ｗ',
	L'Ｘ', L'Ｙ', L'Ｚ', L'［', L'￥', L'］', L'＾', L'＿',
	L'‘', L'ａ', L'ｂ', L'ｃ', L'ｄ', L'ｅ', L'ｆ', L'ｇ',
	L'ｈ', L'ｉ', L'ｊ', L'ｋ', L'ｌ', L'ｍ', L'ｎ', L'ｏ',
	L'ｐ', L'ｑ', L'ｒ', L'ｓ', L'ｔ', L'ｕ', L'ｖ', L'ｗ',
	L'ｘ', L'ｙ', L'ｚ', L'｛', L'｜', L'｝', L'￣'
};
static const WCHAR HiraTable[]={
	L'ぁ', L'あ', L'ぃ', L'い', L'ぅ', L'う', L'ぇ',
	L'え', L'ぉ', L'お', L'か', L'が', L'き', L'ぎ', L'く',
	L'ぐ', L'け', L'げ', L'こ', L'ご', L'さ', L'ざ', L'し',
	L'じ', L'す', L'ず', L'せ', L'ぜ', L'そ', L'ぞ', L'た',
	L'だ', L'ち', L'ぢ', L'っ', L'つ', L'づ', L'て', L'で',
	L'と', L'ど', L'な', L'に', L'ぬ', L'ね', L'の', L'は',
	L'ば', L'ぱ', L'ひ', L'び', L'ぴ', L'ふ', L'ぶ', L'ぷ',
	L'へ', L'べ', L'ぺ', L'ほ', L'ぼ', L'ぽ', L'ま', L'み',
	L'む', L'め', L'も', L'ゃ', L'や', L'ゅ', L'ゆ', L'ょ',
	L'よ', L'ら', L'り', L'る', L'れ', L'ろ', L'ゎ', L'わ',
	L'ゐ', L'ゑ', L'を', L'ん', L'　', L'　', L'　', L'ゝ',
	L'ゞ', L'ー', L'。', L'「', L'」', L'、', L'・'
};
static const WCHAR KanaTable[]={
	L'ァ', L'ア', L'ィ', L'イ', L'ゥ', L'ウ', L'ェ',
	L'エ', L'ォ', L'オ', L'カ', L'ガ', L'キ', L'ギ', L'ク',
	L'グ', L'ケ', L'ゲ', L'コ', L'ゴ', L'サ', L'ザ', L'シ',
	L'ジ', L'ス', L'ズ', L'セ', L'ゼ', L'ソ', L'ゾ', L'タ',
	L'ダ', L'チ', L'ヂ', L'ッ', L'ツ', L'ヅ', L'テ', L'デ',
	L'ト', L'ド', L'ナ', L'ニ', L'ヌ', L'ネ', L'ノ', L'ハ',
	L'バ', L'パ', L'ヒ', L'ビ', L'ピ', L'フ', L'ブ', L'プ',
	L'ヘ', L'ベ', L'ペ', L'ホ', L'ボ', L'ポ', L'マ', L'ミ',
	L'ム', L'メ', L'モ', L'ャ', L'ヤ', L'ュ', L'ユ', L'ョ',
	L'ヨ', L'ラ', L'リ', L'ル', L'レ', L'ロ', L'ヮ', L'ワ',
	L'ヰ', L'ヱ', L'ヲ', L'ン', L'ヴ', L'ヵ', L'ヶ', L'ヽ',
	L'ヾ', L'ー', L'。', L'「', L'」', L'、', L'・'
};
static const WCHAR JisXKanaTable[]={
	L'。', L'「', L'」', L'、', L'・', L'ヲ', L'ァ',
	L'ィ', L'ゥ', L'ェ', L'ォ', L'ャ', L'ュ', L'ョ', L'ッ',
	L'ー', L'ア', L'イ', L'ウ', L'エ', L'オ', L'カ', L'キ',
	L'ク', L'ケ', L'コ', L'サ', L'シ', L'ス', L'セ', L'ソ',
	L'タ', L'チ', L'ツ', L'テ', L'ト', L'ナ', L'ニ', L'ヌ',
	L'ネ', L'ノ', L'ハ', L'ヒ', L'フ', L'ヘ', L'ホ', L'マ',
	L'ミ', L'ム', L'メ', L'モ', L'ヤ', L'ユ', L'ヨ', L'ラ',
	L'リ', L'ル', L'レ', L'ロ', L'ワ', L'ン', L'゛', L'゜',
	L'・', L'・', L'・', L'・', L'・', L'・', L'・', L'・',
	L'・', L'・', L'・', L'・', L'・', L'・', L'・', L'・',
	L'・', L'・', L'・', L'・', L'・', L'・', L'・', L'・',
	L'・', L'・', L'・', L'・', L'・', L'・', L'・'
};

struct GAIJI_TABLE{
	const WCHAR* strCharUnicode;
	const WCHAR* strChar;
};

static const GAIJI_TABLE GaijiTable[]={
	{L"\x2491", L"10."},
	{L"\x2492", L"11."},
	{L"\x2493", L"12."},
	{L"\xD83C\xDD4A", L"[HV]"}, //90区48点
	{L"\xD83C\xDD4C", L"[SD]"},
	{L"\xD83C\xDD3F", L"[Ｐ]"},
	{L"\xD83C\xDD46", L"[Ｗ]"},
	{L"\xD83C\xDD4B", L"[MV]"},
	{L"\xD83C\xDE10", L"[手]"},
	{L"\xD83C\xDE11", L"[字]"},
	{L"\xD83C\xDE12", L"[双]"},
	{L"\xD83C\xDE13", L"[デ]"},
	{L"\xD83C\xDD42", L"[Ｓ]"},
	{L"\xD83C\xDE14", L"[二]"},
	{L"\xD83C\xDE15", L"[多]"},
	{L"\xD83C\xDE16", L"[解]"},
	{L"\xD83C\xDD4D", L"[SS]"},
	{L"\xD83C\xDD31", L"[Ｂ]"},
	{L"\xD83C\xDD3D", L"[Ｎ]"},//
	{L"\x2B1B", L"■"},//90区64点
	{L"\x2B24", L"●"},
	{L"\xD83C\xDE17", L"[天]"},
	{L"\xD83C\xDE18", L"[交]"},
	{L"\xD83C\xDE19", L"[映]"},
	{L"\xD83C\xDE1A", L"[無]"},
	{L"\xD83C\xDE1B", L"[料]"},
	{L"\x26BF", L"[・]"},
	{L"\xD83C\xDE1C", L"[前]"},
	{L"\xD83C\xDE1D", L"[後]"},
	{L"\xD83C\xDE1E", L"[再]"},
	{L"\xD83C\xDE1F", L"[新]"},
	{L"\xD83C\xDE20", L"[初]"},
	{L"\xD83C\xDE21", L"[終]"},
	{L"\xD83C\xDE22", L"[生]"},
	{L"\xD83C\xDE23", L"[販]"},
	{L"\xD83C\xDE24", L"[声]"},//90区80点
	{L"\xD83C\xDE25", L"[吹]"},
	{L"\xD83C\xDD4E", L"[PPV]"},
	{L"\x3299", L"(秘)"},
	{L"\xD83C\xDE00", L"ほか"},
	//91区は飛ばす
	{L"\x27A1", L"→"},//92区1点
	{L"\x2B05", L"←"},
	{L"\x2B06", L"↑"},
	{L"\x2B07", L"↓"},
	{L"\x2B2F", L"○"},
	{L"\x2B2E", L"●"},
	{L"年", L"年"},
	{L"月", L"月"},
	{L"日", L"日"},
	{L"円", L"円"},
	{L"\x33A1", L"m^2"},
	{L"\x33A5", L"m^3"},
	{L"\x339D", L"cm"},
	{L"\x33A0", L"cm^2"},
	{L"\x33A4", L"cm^3"},
	{L"\xD83C\xDD00", L"０."},//92区16点
	{L"\x2488", L"１."},
	{L"\x2489", L"２."},
	{L"\x248A", L"３."},
	{L"\x248B", L"４."},
	{L"\x248C", L"５."},
	{L"\x248D", L"６."},
	{L"\x248E", L"７."},
	{L"\x248F", L"８."},
	{L"\x2490", L"９."},
	{L"氏", L"氏"},
	{L"副", L"副"},
	{L"元", L"元"},
	{L"故", L"故"},
	{L"前", L"前"},
	{L"新", L"新"},
	{L"\xD83C\xDD01", L"０,"},//92区32点
	{L"\xD83C\xDD02", L"１,"},
	{L"\xD83C\xDD03", L"２,"},
	{L"\xD83C\xDD04", L"３,"},
	{L"\xD83C\xDD05", L"４,"},
	{L"\xD83C\xDD06", L"５,"},
	{L"\xD83C\xDD07", L"６,"},
	{L"\xD83C\xDD08", L"７,"},
	{L"\xD83C\xDD09", L"８,"},
	{L"\xD83C\xDD0A", L"９,"},
	{L"\x3233", L"[社]"},
	{L"\x3236", L"[財]"},
	{L"\x3232", L"[有]"},
	{L"\x3231", L"[株]"},
	{L"\x3239", L"[代]"},
	{L"\x3244", L"(問)"},
	{L"\x25B6", L"▲"},//92区48点
	{L"\x25C0", L"▼"},
	{L"\x3016", L"【"},
	{L"\x3017", L"】"},
	{L"\x27D0", L"◇"},
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
	{L"(ｐ)", L"(ｐ)"},
	{L"(ｓ)", L"(ｓ)"},//92区64点
	{L"(ms)", L"(ms)"},
	{L"(ｔ)", L"(ｔ)"},
	{L"(bs)", L"(bs)"},
	{L"(ｂ)", L"(ｂ)"},
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
	{L"(sy", L"(sy"},//92区80点
	{L"n)", L"n)"},
	{L"(or", L"(or"},
	{L"g)", L"g)"},
	{L"(pe", L"(pe"},
	{L"r)", L"r)"},
	{L"\xD83C\xDD2C", L"(Ｒ)"},
	{L"\xD83C\xDD2B", L"(Ｃ)"},
	{L"\x3247", L"(箏)"},
	{L"\xD83C\xDD90", L"ＤＪ"},
	{L"\xD83C\xDE26", L"[演]"},
	{L"\x213B", L"Fax"},
	{L"\x322A", L"(月)"},//93区1点
	{L"\x322B", L"(火)"},
	{L"\x322C", L"(水)"},
	{L"\x322D", L"(木)"},
	{L"\x322E", L"(金)"},
	{L"\x322F", L"(土)"},
	{L"\x3230", L"(日)"},
	{L"\x3237", L"(祝)"},
	{L"㍾", L"㍾"},
	{L"㍽", L"㍽"},
	{L"㍼", L"㍼"},
	{L"㍻", L"㍻"},
	{L"\x2116", L"No."},
	{L"\x2121", L"Tel"},
	{L"\x3036", L"(〒)"},
	{L"\x26BE", L"()()"},//93区16点
	{L"\xD83C\xDE40", L"[本]"},
	{L"\xD83C\xDE41", L"[三]"},
	{L"\xD83C\xDE42", L"[二]"},
	{L"\xD83C\xDE43", L"[安]"},
	{L"\xD83C\xDE44", L"[点]"},
	{L"\xD83C\xDE45", L"[打]"},
	{L"\xD83C\xDE46", L"[盗]"},
	{L"\xD83C\xDE47", L"[勝]"},
	{L"\xD83C\xDE48", L"[敗]"},
	{L"\xD83C\xDD2A", L"[Ｓ]"},
	{L"\xD83C\xDE27", L"[投]"},
	{L"\xD83C\xDE28", L"[捕]"},
	{L"\xD83C\xDE29", L"[一]"},
	{L"\xD83C\xDE14", L"[二]"},
	{L"\xD83C\xDE2A", L"[三]"},
	{L"\xD83C\xDE2B", L"[遊]"},//93区32点
	{L"\xD83C\xDE2C", L"[左]"},
	{L"\xD83C\xDE2D", L"[中]"},
	{L"\xD83C\xDE2E", L"[右]"},
	{L"\xD83C\xDE2F", L"[指]"},
	{L"\xD83C\xDE30", L"[走]"},
	{L"\xD83C\xDE31", L"[打]"},
	{L"\x2113", L"l"},
	{L"\x338F", L"kg"},
	{L"\x3390", L"Hz"},
	{L"\x33CA", L"ha"},
	{L"\x339E", L"km"},
	{L"\x33A2", L"km^2"},
	{L"\x3371", L"hPa"},
	{L"・", L"・"},
	{L"・", L"・"},
	{L"\x00BD", L"1/2"},//93区48点
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
	{L"\x203C", L"!!"},//93区78点
	{L"\x2049", L"!?"},
	{L"Ⅰ", L"Ⅰ"},//94区1点
	{L"Ⅱ", L"Ⅱ"},
	{L"Ⅲ", L"Ⅲ"},
	{L"Ⅳ", L"Ⅳ"},
	{L"Ⅴ", L"Ⅴ"},
	{L"Ⅵ", L"Ⅵ"},
	{L"Ⅶ", L"Ⅶ"},
	{L"Ⅷ", L"Ⅷ"},
	{L"Ⅸ", L"Ⅸ"},
	{L"Ⅹ", L"Ⅹ"},
	{L"\x216A", L"XI"},
	{L"\x216B", L"XII"},
	{L"⑰", L"⑰"},
	{L"⑱", L"⑱"},
	{L"⑲", L"⑲"},
	{L"⑳", L"⑳"},//94区16点
	{L"\x2474", L"(１)"},
	{L"\x2475", L"(２)"},
	{L"\x2476", L"(３)"},
	{L"\x2477", L"(４)"},
	{L"\x2478", L"(５)"},
	{L"\x2479", L"(６)"},
	{L"\x247A", L"(７)"},
	{L"\x247B", L"(８)"},
	{L"\x247C", L"(９)"},
	{L"\x247D", L"(10)"},
	{L"\x247E", L"(11)"},
	{L"\x247F", L"(12)"},
	{L"\x3251", L"(21)"},
	{L"\x3252", L"(22)"},
	{L"\x3253", L"(23)"},
	{L"\x3254", L"(24)"},//94区32点
	{L"\xD83C\xDD10", L"(Ａ)"},
	{L"\xD83C\xDD11", L"(Ｂ)"},
	{L"\xD83C\xDD12", L"(Ｃ)"},
	{L"\xD83C\xDD13", L"(Ｄ)"},
	{L"\xD83C\xDD14", L"(Ｅ)"},
	{L"\xD83C\xDD15", L"(Ｆ)"},
	{L"\xD83C\xDD16", L"(Ｇ)"},
	{L"\xD83C\xDD17", L"(Ｈ)"},
	{L"\xD83C\xDD18", L"(Ｉ)"},
	{L"\xD83C\xDD19", L"(Ｊ)"},
	{L"\xD83C\xDD1A", L"(Ｋ)"},
	{L"\xD83C\xDD1B", L"(Ｌ)"},
	{L"\xD83C\xDD1C", L"(Ｍ)"},
	{L"\xD83C\xDD1D", L"(Ｎ)"},
	{L"\xD83C\xDD1E", L"(Ｏ)"},
	{L"\xD83C\xDD1F", L"(Ｐ)"},//94区48点
	{L"\xD83C\xDD20", L"(Ｑ)"},
	{L"\xD83C\xDD21", L"(Ｒ)"},
	{L"\xD83C\xDD22", L"(Ｓ)"},
	{L"\xD83C\xDD23", L"(Ｔ)"},
	{L"\xD83C\xDD24", L"(Ｕ)"},
	{L"\xD83C\xDD25", L"(Ｖ)"},
	{L"\xD83C\xDD26", L"(Ｗ)"},
	{L"\xD83C\xDD27", L"(Ｘ)"},
	{L"\xD83C\xDD28", L"(Ｙ)"},
	{L"\xD83C\xDD29", L"(Ｚ)"},
	{L"\x3255", L"(25)"},
	{L"\x3256", L"(26)"},
	{L"\x3257", L"(27)"},
	{L"\x3258", L"(28)"},
	{L"\x3259", L"(29)"},
	{L"\x325A", L"(30)"},//94区64点
	{L"①", L"①"},
	{L"②", L"②"},
	{L"③", L"③"},
	{L"④", L"④"},
	{L"⑤", L"⑤"},
	{L"⑥", L"⑥"},
	{L"⑦", L"⑦"},
	{L"⑧", L"⑧"},
	{L"⑨", L"⑨"},
	{L"⑩", L"⑩"},
	{L"⑪", L"⑪"},
	{L"⑫", L"⑫"},
	{L"⑬", L"⑬"},
	{L"⑭", L"⑭"},
	{L"⑮", L"⑮"},
	{L"⑯", L"⑯"},//94区80点
	{L"\x2776", L"(１)"},
	{L"\x2777", L"(２)"},
	{L"\x2778", L"(３)"},
	{L"\x2779", L"(４)"},
	{L"\x277A", L"(５)"},
	{L"\x277B", L"(６)"},
	{L"\x277C", L"(７)"},
	{L"\x277D", L"(８)"},
	{L"\x277E", L"(９)"},
	{L"\x277F", L"(10)"},
	{L"\x24EB", L"(11)"},
	{L"\x24EC", L"(12)"},
	{L"\x325B", L"(31)"}
};

static const GAIJI_TABLE GaijiTbl2[]={
	{L"\x3402", L"〓"},
	{L"\xD840\xDD58", L"亭"},
	{L"\x4EFD", L"〓"},
	{L"\x4EFF", L"彷"},
	{L"侚", L"侚"},
	{L"俉", L"俉"},
	{L"\x509C", L"徭"},
	{L"\x511E", L"〓"},
	{L"\x51BC", L"〓"},
	{L"\x351F", L"〓"}, //10
	{L"匇", L"匇"},
	{L"\x5361", L"〓"},
	{L"\x536C", L"〓"},
	{L"詹", L"詹"},
	{L"\xD842\xDFB7", L"吉"},
	{L"\x544D", L"〓"},
	{L"\x5496", L"〓"},
	{L"咜", L"咜"},
	{L"咩", L"咩"},
	{L"\x550E", L"〓"}, //20
	{L"\x554A", L"〓"},
	{L"\x5672", L"〓"},
	{L"\x56E4", L"〓"},
	{L"\x5733", L"〓"},
	{L"\x5734", L"〓"},
	{L"塚", L"塚"},
	{L"\x5880", L"〓"},
	{L"\x59E4", L"〓"},
	{L"\x5A23", L"〓"},
	{L"\x5A55", L"〓"}, //30
	{L"寬", L"寬"},
	{L"﨑", L"﨑"},
	{L"\x37E2", L"〓"},
	{L"\x5EAC", L"〓"},
	{L"弴", L"弴"},
	{L"彅", L"彅"},
	{L"德", L"德"},
	{L"\x6017", L"〓"},
	{L"\xFA6B", L"恵"},
	{L"愰", L"愰"}, //40
	{L"昤", L"昤"},
	{L"\x66C8", L"〓"},
	{L"曙", L"曙"},
	{L"曺", L"曺"},
	{L"曻", L"曻"},
	{L"桒", L"桒"},
	{L"\x9FC4", L"〓"},
	{L"\x6911", L"〓"},
	{L"\x693B", L"〓"},
	{L"\x6A45", L"〓"}, //50
	{L"\x6A91", L"〓"},
	{L"櫛", L"櫛"},
	{L"\xD84C\xDFCC", L"〓"},
	{L"\xD84C\xDFFE", L"〓"},
	{L"\xD84D\xDDC4", L"〓"},
	{L"\x6BF1", L"〓"},
	{L"\x6CE0", L"冷"},
	{L"\x6D2E", L"〓"},
	{L"\xFA45", L"海"},
	{L"\x6DBF", L"〓"}, //60
	{L"\x6DCA", L"〓"},
	{L"淸", L"淸"},
	{L"\xFA46", L"渚"},
	{L"\x6F5E", L"〓"},
	{L"\x6FF9", L"〓"},
	{L"\x7064", L"〓"},
	{L"\xFA6C", L"〓"},
	{L"\xD850\xDEEE", L"〓"},
	{L"煇", L"煇"},
	{L"燁", L"燁"}, //70
	{L"\x7200", L"〓"},
	{L"\x739F", L"〓"},
	{L"\x73A8", L"〓"},
	{L"珉", L"珉"},
	{L"珖", L"珖"},
	{L"\x741B", L"〓"},
	{L"\x7421", L"〓"},
	{L"\xFA4A", L"琢"},
	{L"琦", L"琦"},
	{L"琪", L"琪"}, //80
	{L"\x742C", L"〓"},
	{L"\x7439", L"〓"},
	{L"\x744B", L"〓"},
	{L"\x3EDA", L"〓"},
	{L"\x7575", L"畫"},
	{L"\x7581", L"〓"},
	{L"\x7772", L"〓"},
	{L"\x4093", L"〓"},
	{L"\x78C8", L"〓"},
	{L"\x78E0", L"〓"}, //90
	{L"祇", L"祇"},
	{L"禮", L"禮"},
	{L"\x9FC6", L"〓"},
	{L"\x4103", L"〓"},
	{L"\x9FC5", L"〓"},
	{L"\x79DA", L"〓"},
	{L"\x7A1E", L"〓"},
	{L"\x7B7F", L"〓"},
	{L"\x7C31", L"〓"},
	{L"\x4264", L"〓"}, //100
	{L"\x7D8B", L"〓"},
	{L"羡", L"羡"},
	{L"\x8118", L"〓"},
	{L"\x813A", L"〓"},
	{L"\xFA6D", L"舘"},
	{L"\x82AE", L"〓"},
	{L"葛", L"葛"},
	{L"蓜", L"蓜"},
	{L"蓬", L"蓬"},
	{L"蕙", L"蕙"}, //110
	{L"\x85CE", L"〓"},
	{L"蝕", L"蝕"},
	{L"\x87EC", L"蝉"},
	{L"\x880B", L"〓"},
	{L"裵", L"裵"},
	{L"角", L"角"},
	{L"諶", L"諶"},
	{L"\x8DCE", L"〓"},
	{L"辻", L"辻"},
	{L"\x8FF6", L"〓"}, //120
	{L"\x90DD", L"〓"},
	{L"鄧", L"鄧"},
	{L"鄭", L"鄭"},
	{L"\x91B2", L"〓"},
	{L"\x9233", L"〓"},
	{L"銈", L"銈"},
	{L"錡", L"錡"},
	{L"鍈", L"鍈"},
	{L"閒", L"閒"},
	{L"\x96DE", L"〓"}, //130
	{L"餃", L"餃"},
	{L"\x9940", L"〓"},
	{L"髙", L"髙"},
	{L"鯖", L"鯖"},
	{L"\x9DD7", L"鴎"},
	{L"\x9EB4", L"麹"},
	{L"\x9EB5", L"麺"}
};

//デフォルトマクロ文(NULは効果がないと規定されている)
static const BYTE DefaultMacro[][20]={
	{ 0x1B,0x24,0x39,0x1B,0x29,0x4A,0x1B,0x2A,0x30,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D },
	{ 0x1B,0x24,0x39,0x1B,0x29,0x31,0x1B,0x2A,0x30,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D },
	{ 0x1B,0x24,0x39,0x1B,0x29,0x20,0x41,0x1B,0x2A,0x30,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D },
	{ 0x1B,0x28,0x32,0x1B,0x29,0x34,0x1B,0x2A,0x35,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D },
	{ 0x1B,0x28,0x32,0x1B,0x29,0x33,0x1B,0x2A,0x35,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D },
	{ 0x1B,0x28,0x32,0x1B,0x29,0x20,0x41,0x1B,0x2A,0x35,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D },
	{ 0x1B,0x28,0x20,0x41,0x1B,0x29,0x20,0x42,0x1B,0x2A,0x20,0x43,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D },
	{ 0x1B,0x28,0x20,0x44,0x1B,0x29,0x20,0x45,0x1B,0x2A,0x20,0x46,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D },
	{ 0x1B,0x28,0x20,0x47,0x1B,0x29,0x20,0x48,0x1B,0x2A,0x20,0x49,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D },
	{ 0x1B,0x28,0x20,0x4A,0x1B,0x29,0x20,0x4B,0x1B,0x2A,0x20,0x4C,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D },
	{ 0x1B,0x28,0x20,0x4D,0x1B,0x29,0x20,0x4E,0x1B,0x2A,0x20,0x4F,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D },
	{ 0x1B,0x24,0x39,0x1B,0x29,0x20,0x42,0x1B,0x2A,0x30,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D },
	{ 0x1B,0x24,0x39,0x1B,0x29,0x20,0x43,0x1B,0x2A,0x30,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D },
	{ 0x1B,0x24,0x39,0x1B,0x29,0x20,0x44,0x1B,0x2A,0x30,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D },
	{ 0x1B,0x28,0x31,0x1B,0x29,0x30,0x1B,0x2A,0x4A,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D },
	{ 0x1B,0x28,0x4A,0x1B,0x29,0x32,0x1B,0x2A,0x20,0x41,0x1B,0x2B,0x20,0x70,0x0F,0x1B,0x7D }
};

#endif //ARIB8CHAR_DECODE_H_IMPLEMENT_TABLE

//文字サイズ
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
	static const WCHAR* TELETEXT_MARK;

	//PSI/SIを想定したwstringへの変換
	BOOL PSISI( const BYTE* pbSrc, DWORD dwSrcSize, wstring* strDec );
	//字幕を想定したwstringへの変換
	BOOL Caption( const BYTE* pbSrc, DWORD dwSrcSize, vector<CAPTION_DATA>* pCaptionList );

protected:
	struct MF_MODE{
		int iMF; //文字符号集合
		int iMode; //符号集合の分類
		int iByte; //読み込みバイト数
	};

	BOOL m_bPSI;

	MF_MODE m_G0;
	MF_MODE m_G1;
	MF_MODE m_G2;
	MF_MODE m_G3;
	MF_MODE* m_GL;
	MF_MODE* m_GR;

	//デコードした文字列
	wstring m_strDecode;
	//文字サイズ
	STRING_SIZE m_emStrSize;

	//CLUTのインデックス
	BYTE m_bCharColorIndex;
	BYTE m_bBackColorIndex;
	BYTE m_bRasterColorIndex;
	BYTE m_bDefPalette;

	BOOL m_bUnderLine;
	BOOL m_bShadow;
	BOOL m_bBold;
	BOOL m_bItalic;
	BYTE m_bFlushMode;

	//表示書式
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
#ifndef _WIN32
	static const WCHAR m_jisTable[84 * 94 + 1];
#endif
protected:
	void InitPSISI(void);
	void InitCaption(void);
	BOOL Analyze( const BYTE* pbSrc, DWORD dwSrcSize, DWORD* pdwReadSize );

	BOOL IsSmallCharMode(void);
	BOOL IsChgPos(void);
	void CreateCaptionData(CAPTION_DATA* pItem);
	void CreateCaptionCharData(CAPTION_CHAR_DATA* pItem);
	void CheckModify(void);

	//制御符号
	BOOL C0( const BYTE* pbSrc, DWORD dwSrcSize, DWORD* pdwReadSize );
	BOOL C1( const BYTE* pbSrc, DWORD dwSrcSize, DWORD* pdwReadSize );
	BOOL GL_GR( const BYTE* pbSrc, DWORD dwSrcSize, DWORD* pdwReadSize, const MF_MODE* mode );
	//エスケープシーケンス
	BOOL ESC( const BYTE* pbSrc, DWORD dwSrcSize, DWORD* pdwReadSize );
	//２バイト文字変換
	BOOL ToSJIS( const BYTE bFirst, const BYTE bSecond );
	BOOL ToCustomFont( const BYTE bFirst, const BYTE bSecond );

	BOOL CSI( const BYTE* pbSrc, DWORD dwSrcSize, DWORD* pdwReadSize );

};
