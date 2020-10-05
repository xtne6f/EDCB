#pragma once

// MFCで使う時用
/*#ifdef _DEBUG
#undef new
#endif
#ifdef _DEBUG
#define new DEBUG_NEW
#endif
*/

struct CLUT_DAT{
	unsigned char ucR;
	unsigned char ucG;
	unsigned char ucB;
	unsigned char ucAlpha;
};

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

struct GAIJI_TABLE{
	const WCHAR* strCharUnicode;
	const WCHAR* strChar;
};

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
	static const WCHAR* const TELETEXT_MARK;
	static const CLUT_DAT DefClut[128];

	//PSI/SIを想定したwstringへの変換
	BOOL PSISI( const BYTE* pbSrc, DWORD dwSrcSize, wstring* strDec );
	//字幕を想定したwstringへの変換
	BOOL Caption( const BYTE* pbSrc, DWORD dwSrcSize, vector<CAPTION_DATA>* pCaptionList );
	//PSISI()の結果が真のとき、引数*strDecに格納される文字列
	const wstring& GetDecodedString() const { return m_strDecode; }

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

	static const WCHAR AsciiTable[94];
	static const WCHAR HiraTable[94];
	static const WCHAR KanaTable[94];
	static const WCHAR JisXKanaTable[94];
	static const BYTE DefaultMacro[16][20];
	static const GAIJI_TABLE GaijiTable[40 + 91 + 63 + 2 + 93];
	static const GAIJI_TABLE GaijiTbl2[94 + 43];
	static const WCHAR m_jisTable[84 * 94 + 1];
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
	BOOL ToCustomFont( const BYTE bFirst, const BYTE bSecond );

	BOOL CSI( const BYTE* pbSrc, DWORD dwSrcSize, DWORD* pdwReadSize );

};
