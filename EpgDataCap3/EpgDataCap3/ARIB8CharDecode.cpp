#include "stdafx.h"
#define COLOR_DEF_H_IMPLEMENT_TABLE
#define ARIB8CHAR_DECODE_H_IMPLEMENT_TABLE
#include "ARIB8CharDecode.h"

//CP932に存在しない文字も使用する場合はこのマクロを定義する
//#define ARIB8CHAR_USE_UNICODE

CARIB8CharDecode::CARIB8CharDecode(void)
{
}

CARIB8CharDecode::~CARIB8CharDecode(void)
{
}

void CARIB8CharDecode::InitPSISI(void)
{
	m_G0.iMF = MF_JIS_KANJI1;
	m_G0.iMode = MF_MODE_G;
	m_G0.iByte = 2;

	m_G1.iMF = MF_ASCII;
	m_G1.iMode = MF_MODE_G;
	m_G1.iByte = 1;

	m_G2.iMF = MF_HIRA;
	m_G2.iMode = MF_MODE_G;
	m_G2.iByte = 1;

	m_G3.iMF = MF_KANA;
	m_G3.iMode = MF_MODE_G;
	m_G3.iByte = 1;

	m_GL = &m_G0;
	m_GR = &m_G2;

	m_strDecode = L"";
	m_emStrSize = STR_NORMAL;

	m_bCharColorIndex = 0;
	m_bBackColorIndex = 0;
	m_bRasterColorIndex = 0;
	m_bDefPalette = 0;

	m_bUnderLine = FALSE;
	m_bShadow = FALSE;
	m_bBold = FALSE;
	m_bItalic = FALSE;
	m_bFlushMode = 0;

	m_wSWFMode=0;
	m_wClientX=0;
	m_wClientY=0;
	m_wClientW=0;
	m_wClientH=0;
	m_wPosX=0;
	m_wPosY=0;
	m_wCharW=0;
	m_wCharH=0;
	m_wCharHInterval=0;
	m_wCharVInterval=0;
	m_wMaxChar = 0;
	m_dwWaitTime = 0;

	m_pCaptionList = NULL;

	m_bPSI = TRUE;
}

void CARIB8CharDecode::InitCaption(void)
{
	m_G0.iMF = MF_JIS_KANJI1;
	m_G0.iMode = MF_MODE_G;
	m_G0.iByte = 2;

	m_G1.iMF = MF_ASCII;
	m_G1.iMode = MF_MODE_G;
	m_G1.iByte = 1;

	m_G2.iMF = MF_HIRA;
	m_G2.iMode = MF_MODE_G;
	m_G2.iByte = 1;

	m_G3.iMF = MF_MACRO;
	m_G3.iMode = MF_MODE_OTHER;
	m_G3.iByte = 1;

	m_GL = &m_G0;
	m_GR = &m_G2;

	m_strDecode = L"";
	m_emStrSize = STR_NORMAL;

	m_bCharColorIndex = 7;
	m_bBackColorIndex = 8;
	m_bRasterColorIndex = 8;
	m_bDefPalette = 0;

	m_bUnderLine = FALSE;
	m_bShadow = FALSE;
	m_bBold = FALSE;
	m_bItalic = FALSE;
	m_bFlushMode = 0;

	m_wSWFMode=0;
	m_wClientX=0;
	m_wClientY=0;
	m_wClientW=0;
	m_wClientH=0;
	m_wPosX=0;
	m_wPosY=0;
	m_wCharW=36;
	m_wCharH=36;
	m_wCharHInterval=0;
	m_wCharVInterval=0;
	m_wMaxChar = 0;
	m_dwWaitTime = 0;

	m_pCaptionList = NULL;

	m_bPSI = FALSE;
}

BOOL CARIB8CharDecode::PSISI( const BYTE* pbSrc, DWORD dwSrcSize, wstring* strDec )
{
	if( pbSrc == NULL || dwSrcSize == 0 || strDec == NULL){
		return FALSE;
	}
	InitPSISI();
	DWORD dwReadSize = 0;

	BOOL bRet = Analyze(pbSrc, dwSrcSize, &dwReadSize );
	*strDec = m_strDecode;
	return bRet;
}

BOOL CARIB8CharDecode::Caption( const BYTE* pbSrc, DWORD dwSrcSize, vector<CAPTION_DATA>* pCaptionList )
{
	if( pbSrc == NULL || dwSrcSize == 0 || pCaptionList == NULL){
		return FALSE;
	}
	InitCaption();
	m_pCaptionList = pCaptionList;

	BOOL bRet = TRUE;
	DWORD dwReadCount = 0;
	while(dwReadCount<dwSrcSize){
		DWORD dwReadSize = 0;
		bRet = Analyze(pbSrc+dwReadCount, dwSrcSize-dwReadCount, &dwReadSize );
		if( bRet == TRUE ){
			if( m_strDecode.size() > 0 ){
				CheckModify();
			}
		}else{
			pCaptionList->clear();
			break;
		}
		m_strDecode = L"";
		dwReadCount+=dwReadSize;
	}
	return bRet;
}

BOOL CARIB8CharDecode::IsSmallCharMode(void)
{
	if( m_bPSI == FALSE ){
		return FALSE;
	}
	BOOL bRet = FALSE;
	switch(m_emStrSize){
		case STR_SMALL:
			bRet = TRUE;
			break;
		case STR_MEDIUM:
			bRet = TRUE;
			break;
		case STR_NORMAL:
			bRet = FALSE;
			break;
		case STR_MICRO:
			bRet = TRUE;
			break;
		case STR_HIGH_W:
			bRet = FALSE;
			break;
		case STR_WIDTH_W:
			bRet = FALSE;
			break;
		case STR_W:
			bRet = FALSE;
			break;
		case STR_SPECIAL_1:
			bRet = FALSE;
			break;
		case STR_SPECIAL_2:
			bRet = FALSE;
			break;
		default:
			break;
	}
	return bRet;
}

//戻り値がFALSEのとき*pdwReadSizeは不定、TRUEのとき*pdwReadSize<=dwSrcSize (C0 C1ほかメソッドも同様)
BOOL CARIB8CharDecode::Analyze( const BYTE* pbSrc, DWORD dwSrcSize, DWORD* pdwReadSize )
{
	if( dwSrcSize == 0 ){
		return FALSE;
	}
	DWORD dwReadSize = 0;

	while( dwReadSize < dwSrcSize ){
		DWORD dwReadBuff = 0;
		//1バイト目チェック
		if( pbSrc[dwReadSize] <= 0x20 ){
			//C0制御コード
			BOOL bRet = C0( pbSrc+dwReadSize, dwSrcSize-dwReadSize, &dwReadBuff );
			dwReadSize += dwReadBuff;
			if( bRet == FALSE ){
				return FALSE;
			}else if( bRet == 2 ){
				break;
			}
		}else if( pbSrc[dwReadSize] < 0x7F ){
			//GL符号領域
			if( GL_GR( pbSrc+dwReadSize, dwSrcSize-dwReadSize, &dwReadBuff, m_GL ) == FALSE ){
				return FALSE;
			}
			dwReadSize += dwReadBuff;
		}else if( pbSrc[dwReadSize] <= 0xA0 ){
			//C1制御コード
			BOOL bRet = C1( pbSrc+dwReadSize, dwSrcSize-dwReadSize, &dwReadBuff );
			dwReadSize += dwReadBuff;
			if( bRet == FALSE ){
				return FALSE;
			}else if( bRet == 2 ){
				break;
			}
		}else if( pbSrc[dwReadSize] < 0xFF ){
			//GR符号領域
			if( GL_GR( pbSrc+dwReadSize, dwSrcSize-dwReadSize, &dwReadBuff, m_GR ) == FALSE ){
				return FALSE;
			}
			dwReadSize += dwReadBuff;
		}else{
			return FALSE;
		}
	}

	*pdwReadSize = dwReadSize;
	return TRUE;
}

BOOL CARIB8CharDecode::C0( const BYTE* pbSrc, DWORD dwSrcSize, DWORD* pdwReadSize )
{
	if( dwSrcSize == 0 ){
		return FALSE;
	}

	DWORD dwReadSize = 0;
	DWORD dwReadBuff = 0;

	BOOL bRet = TRUE;

	switch(pbSrc[0]){
	case 0x20:
		//SP 空白
		//空白は文字サイズの影響あり
		if( IsSmallCharMode() == FALSE ){
			m_strDecode += L'　';
		}else{
			m_strDecode += L' ';
		}
		dwReadSize = 1;
		break;
	case 0x0D:
		//APR 改行
		m_strDecode += L"\r\n";
		dwReadSize = 1;
		break;
	case 0x0E:
		//LS1 GLにG1セット
		m_GL = &m_G1;
		dwReadSize = 1;
		break;
	case 0x0F:
		//LS0 GLにG0セット
		m_GL = &m_G0;
		dwReadSize = 1;
		break;
	case 0x19:
		//SS2 シングルシフト
		//G2で呼ぶ(マクロ展開を考慮してGLは入れ替えない)
		if( GL_GR( pbSrc+1, dwSrcSize-1, &dwReadBuff, &m_G2 ) == FALSE ){
			return FALSE;
		}
		dwReadSize = 1+dwReadBuff;
		break;
	case 0x1D:
		//SS3 シングルシフト
		//G3で呼ぶ(マクロ展開を考慮してGLは入れ替えない)
		if( GL_GR( pbSrc+1, dwSrcSize-1, &dwReadBuff, &m_G3 ) == FALSE ){
			return FALSE;
		}
		dwReadSize = 1+dwReadBuff;
		break;
	case 0x1B:
		//ESC エスケープシーケンス
		if( ESC( pbSrc+1, dwSrcSize-1, &dwReadBuff ) == FALSE ){
			return FALSE;
		}
		dwReadSize = 1+dwReadBuff;
		break;
	default:
		//未サポートの制御コード
		if( pbSrc[0] == 0x16 ){
			//PAPF
			if( dwSrcSize < 2 ){
				return FALSE;
			}
			dwReadSize = 2;
		}else if( pbSrc[0] == 0x1C ){
			//APS
			if( dwSrcSize < 3 ){
				return FALSE;
			}
			CheckModify();
			m_wPosY=m_wCharH*(pbSrc[1]-0x40);
			m_wPosX=m_wCharW*(pbSrc[2]-0x40);
			if( m_emStrSize == STR_SMALL || m_emStrSize == STR_MEDIUM ){
				m_wPosX=m_wPosX/2;
			}
			dwReadSize = 3;
		}else if( pbSrc[0] == 0x0C ){
			//CS
			dwReadSize = 1;
			CAPTION_DATA Item;
			Item.bClear = TRUE;
			Item.dwWaitTime = m_dwWaitTime*100;
			if( m_pCaptionList != NULL ){
				m_pCaptionList->push_back(Item);
			}
			bRet = 2;
			m_dwWaitTime = 0;
		}else{
			//APB、APF、APD、APU
			dwReadSize = 1;
		}
		break;
	}

	*pdwReadSize = dwReadSize;

	return bRet;
}

BOOL CARIB8CharDecode::C1( const BYTE* pbSrc, DWORD dwSrcSize, DWORD* pdwReadSize )
{
	if( dwSrcSize == 0 ){
		return FALSE;
	}
	DWORD dwReadSize = 0;
	DWORD dwReadBuff = 0;

	BOOL bRet = TRUE;

	CheckModify();

	switch(pbSrc[0]){
	case 0x89:
		//MSZ 半角指定
		m_emStrSize = STR_MEDIUM;
		dwReadSize = 1;
		break;
	case 0x8A:
		//NSZ 全角指定
		m_emStrSize = STR_NORMAL;
		dwReadSize = 1;
		break;
	case 0x80:
		//BKF 文字黒
		m_bCharColorIndex = (m_bDefPalette<<4) | 0x00;
		dwReadSize = 1;
		break;
	case 0x81:
		//RDF 文字赤
		m_bCharColorIndex = (m_bDefPalette<<4) | 0x01;
		dwReadSize = 1;
		break;
	case 0x82:
		//GRF 文字緑
		m_bCharColorIndex = (m_bDefPalette<<4) | 0x02;
		dwReadSize = 1;
		break;
	case 0x83:
		//YLF 文字黄
		m_bCharColorIndex = (m_bDefPalette<<4) | 0x03;
		dwReadSize = 1;
		break;
	case 0x84:
		//BLF 文字青
		m_bCharColorIndex = (m_bDefPalette<<4) | 0x04;
		dwReadSize = 1;
		break;
	case 0x85:
		//MGF 文字マゼンタ
		m_bCharColorIndex = (m_bDefPalette<<4) | 0x05;
		dwReadSize = 1;
		break;
	case 0x86:
		//CNF 文字シアン
		m_bCharColorIndex = (m_bDefPalette<<4) | 0x06;
		dwReadSize = 1;
		break;
	case 0x87:
		//WHF 文字白
		m_bCharColorIndex = (m_bDefPalette<<4) | 0x07;
		dwReadSize = 1;
		break;
	case 0x88:
		//SSZ 小型サイズ
		m_emStrSize = STR_SMALL;
		dwReadSize = 1;
		break;
	case 0x8B:
		//SZX 指定サイズ
		if( dwSrcSize < 2 ) return FALSE;
		if( pbSrc[1] == 0x60 ){
			m_emStrSize = STR_MICRO;
		}else if( pbSrc[1] == 0x41 ){
			m_emStrSize = STR_HIGH_W;
		}else if( pbSrc[1] == 0x44 ){
			m_emStrSize = STR_WIDTH_W;
		}else if( pbSrc[1] == 0x45 ){
			m_emStrSize = STR_W;
		}else if( pbSrc[1] == 0x6B ){
			m_emStrSize = STR_SPECIAL_1;
		}else if( pbSrc[1] == 0x64 ){
			m_emStrSize = STR_SPECIAL_2;
		}
		dwReadSize = 2;
		break;
	case 0x90:
		//COL 色指定
		if( dwSrcSize < 2 ) return FALSE;
		if( pbSrc[1] == 0x20 ){
			if( dwSrcSize < 3 ) return FALSE;
			dwReadSize = 3;
			m_bDefPalette = pbSrc[2]&0x07;
		}else{
			switch(pbSrc[1]&0xF0){
				case 0x40:
					m_bCharColorIndex = pbSrc[1]&0x0F;
					break;
				case 0x50:
					m_bBackColorIndex = pbSrc[1]&0x0F;
					break;
				case 0x60:
					//未サポート
					break;
				case 0x70:
					//未サポート
					break;
				default:
					break;
			}
			dwReadSize = 2;
		}
		break;
	case 0x91:
		//FLC フラッシング制御
		if( dwSrcSize < 2 ) return FALSE;
		if( pbSrc[1] == 0x40 ){
			m_bFlushMode = 1;
		}else if( pbSrc[1] == 0x47 ){
			m_bFlushMode = 2;
		}else if( pbSrc[1] == 0x4F ){
			m_bFlushMode = 0;
		}
		dwReadSize = 2;
		break;
	case 0x93:
		//POL パターン極性
		//未サポート
		if( dwSrcSize < 2 ) return FALSE;
		dwReadSize = 2;
		break;
	case 0x94:
		//WMM 書き込みモード変更
		//未サポート
		if( dwSrcSize < 2 ) return FALSE;
		dwReadSize = 2;
		break;
	case 0x95:
		//MACRO マクロ定義
		//未サポート
		dwReadSize = 2;
		do{
			if( ++dwReadSize > dwSrcSize ){
				return FALSE;
			}
		}while( pbSrc[dwReadSize-2] != 0x95 || pbSrc[dwReadSize-1] != 0x4F );
		break;
	case 0x97:
		//HLC 囲み制御
		//未サポート
		if( dwSrcSize < 2 ) return FALSE;
		dwReadSize = 2;
		break;
	case 0x98:
		//RPC 文字繰り返し
		//未サポート
		if( dwSrcSize < 2 ) return FALSE;
		dwReadSize = 2;
		break;
	case 0x99:
		//SPL アンダーライン モザイクの終了
		m_bBold = FALSE;
		bRet = 2;
		dwReadSize = 1;
		break;
	case 0x9A:
		//STL アンダーライン モザイクの開始
		m_bBold = TRUE;
		dwReadSize = 1;
		break;
	case 0x9D:
		//TIME 時間制御
		CheckModify();
		if( dwSrcSize < 3 ) return FALSE;
		if( pbSrc[1] == 0x20 ){
			m_dwWaitTime = pbSrc[2]-0x40;
			dwReadSize = 3;
		}else{
			dwReadSize = 1;
			do{
				if( ++dwReadSize > dwSrcSize ){
					return FALSE;
				}
			}while( pbSrc[dwReadSize-1] < 0x40 || 0x43 < pbSrc[dwReadSize-1] );
		}
		break;
	case 0x9B:
		//CSI コントロールシーケンス
		if( CSI( pbSrc+1, dwSrcSize-1, &dwReadBuff ) == FALSE ){
			return FALSE;
		}
		dwReadSize = 1+dwReadBuff;
		break;
	default:
		//未サポートの制御コード
		dwReadSize = 1;
		break;
	}

	*pdwReadSize = dwReadSize;

	return bRet;
}

BOOL CARIB8CharDecode::GL_GR( const BYTE* pbSrc, DWORD dwSrcSize, DWORD* pdwReadSize, const MF_MODE* mode )
{
	if( dwSrcSize == 0 || (pbSrc[0]&0x7F) <= 0x20 || 0x7F <= (pbSrc[0]&0x7F) ){
		return FALSE;
	}

	DWORD dwReadSize = 0;
	if( mode->iMode == MF_MODE_G ){
		//文字コード
		switch( mode->iMF ){
			case MF_JISX_KANA:
				{
				//JIS X0201片仮名
				m_strDecode += JisXKanaTable[(pbSrc[0]&0x7F)-0x21];
				dwReadSize = 1;
				}
				break;
			case MF_ASCII:
			case MF_PROP_ASCII:
				{
				if( IsSmallCharMode() == FALSE ){
					//全角なのでテーブルからSJISコード取得
					m_strDecode += AsciiTable[(pbSrc[0]&0x7F)-0x21];
				}else{
					//半角なのでそのまま入れる
					m_strDecode += pbSrc[0]&0x7F;
				}
				dwReadSize = 1;
				}
				break;
			case MF_HIRA:
			case MF_PROP_HIRA:
				{
				//半角ひらがな
				//テーブルからSJISコード取得
				m_strDecode += HiraTable[(pbSrc[0]&0x7F)-0x21];
				dwReadSize = 1;
				}
				break;
			case MF_KANA:
			case MF_PROP_KANA:
				{
				//半角カタカナ
				//テーブルからSJISコード取得
				m_strDecode += KanaTable[(pbSrc[0]&0x7F)-0x21];
				dwReadSize = 1;
				}
				break;
			case MF_KANJI:
			case MF_JIS_KANJI1:
			case MF_JIS_KANJI2:
			case MF_KIGOU:
				{
				//漢字
				if( dwSrcSize < 2 ){
					return FALSE;
				}
				if( ToSJIS( (pbSrc[0]&0x7F), (pbSrc[1]&0x7F) ) == FALSE ){
					ToCustomFont( (pbSrc[0]&0x7F), (pbSrc[1]&0x7F) );
				}
				dwReadSize = 2;
				}
				break;
			default:
				dwReadSize = mode->iByte;
				if( dwReadSize > dwSrcSize ){
					return FALSE;
				}
				break;
		}
	}else{
		if( mode->iMF == MF_MACRO ){
			DWORD dwTemp=0;
			//マクロ
			//PSI/SIでは未サポート
			if( 0x60 <= (pbSrc[0]&0x7F) && (pbSrc[0]&0x7F) <= 0x6F ){
				if( Analyze(DefaultMacro[pbSrc[0]&0x0F], sizeof(DefaultMacro[0]), &dwTemp) == FALSE ){
					return FALSE;
				}
			}
			dwReadSize = 1;
		}else{
			dwReadSize = mode->iByte;
			if( dwReadSize > dwSrcSize ){
				return FALSE;
			}
		}
	}

	*pdwReadSize = dwReadSize;

	return TRUE;
}

BOOL CARIB8CharDecode::ToSJIS( const BYTE bFirst, const BYTE bSecond )
{
	if( bFirst >= 0x75 && bSecond >= 0x21 ){
		return FALSE;
	}

	unsigned char ucFirst = bFirst;
	unsigned char ucSecond = bSecond;
	
	ucFirst = ucFirst - 0x21;
	if( ( ucFirst & 0x01 ) == 0 ){
		ucSecond += 0x1F;
		if( ucSecond >= 0x7F ){
			ucSecond += 0x01;
		}
	}else{
		ucSecond += 0x7E;
	}
	ucFirst = ucFirst>>1;
	if( ucFirst >= 0x1F ){
		ucFirst += 0xC1;
	}else{
		ucFirst += 0x81;
	}

	unsigned char ucDec[] = {ucFirst, ucSecond, '\0'};
	WCHAR cDec[3];
	if( MultiByteToWideChar(932, MB_ERR_INVALID_CHARS, (char*)ucDec, -1, cDec, 3) < 2 ){
		m_strDecode += L'・';
	}else{
		m_strDecode += cDec;
	}

	return TRUE;
}

BOOL CARIB8CharDecode::ToCustomFont( const BYTE bFirst, const BYTE bSecond )
{
	unsigned short usSrc = (unsigned short)(bFirst<<8) | bSecond;

	GAIJI_TABLE t;
	if( 0x7521 <= usSrc && usSrc <= 0x757E ){
		t = GaijiTbl2[usSrc - 0x7521];
	}else if( 0x7621 <= usSrc && usSrc <= 0x764B ){
		t = GaijiTbl2[usSrc - 0x7621 + 94];
	}else if( 0x7A4D <= usSrc && usSrc <= 0x7A74 ){
		t = GaijiTable[usSrc - 0x7A4D];
	}else if(0x7C21 <= usSrc && usSrc <= 0x7C7B ){
		t = GaijiTable[usSrc - 0x7C21 + 40];
	}else if(0x7D21 <= usSrc && usSrc <= 0x7D5F ){
		t = GaijiTable[usSrc - 0x7D21 + 131];
	}else if(0x7D6E <= usSrc && usSrc <= 0x7D6F ){
		t = GaijiTable[usSrc - 0x7D6E + 194];
	}else if(0x7E21 <= usSrc && usSrc <= 0x7E7D ){
		t = GaijiTable[usSrc - 0x7E21 + 196];
	}else{
		m_strDecode += L'・';
		return FALSE;
	}
#ifdef ARIB8CHAR_USE_UNICODE
	m_strDecode += t.strCharUnicode;
#else
	m_strDecode += t.strChar;
#endif

	return TRUE;
}


BOOL CARIB8CharDecode::ESC( const BYTE* pbSrc, DWORD dwSrcSize, DWORD* pdwReadSize )
{
	if( dwSrcSize == 0 ){
		return FALSE;
	}

	DWORD dwReadSize = 0;
	if( pbSrc[0] == 0x24 ){
		if( dwSrcSize < 2 ) return FALSE;

		if( pbSrc[1] >= 0x28 && pbSrc[1] <= 0x2B ){
			if( dwSrcSize < 3 ) return FALSE;

			if( pbSrc[2] == 0x20 ){
				if( dwSrcSize < 4 ) return FALSE;
				//2バイトDRCS
				switch(pbSrc[1]){
					case 0x28:
						m_G0.iMF = pbSrc[3];
						m_G0.iMode = MF_MODE_DRCS;
						m_G0.iByte = 2;
						break;
					case 0x29:
						m_G1.iMF = pbSrc[3];
						m_G1.iMode = MF_MODE_DRCS;
						m_G1.iByte = 2;
						break;
					case 0x2A:
						m_G2.iMF = pbSrc[3];
						m_G2.iMode = MF_MODE_DRCS;
						m_G2.iByte = 2;
						break;
					case 0x2B:
						m_G3.iMF = pbSrc[3];
						m_G3.iMode = MF_MODE_DRCS;
						m_G3.iByte = 2;
						break;
					default:
						break;
				}
				dwReadSize = 4;
			}else if( pbSrc[2] == 0x28 ){
				if( dwSrcSize < 4 ) return FALSE;
				//複数バイト、音楽符号
				switch(pbSrc[1]){
					case 0x28:
						m_G0.iMF = pbSrc[3];
						m_G0.iMode = MF_MODE_OTHER;
						m_G0.iByte = 1;
						break;
					case 0x29:
						m_G1.iMF = pbSrc[3];
						m_G1.iMode = MF_MODE_OTHER;
						m_G1.iByte = 1;
						break;
					case 0x2A:
						m_G2.iMF = pbSrc[3];
						m_G2.iMode = MF_MODE_OTHER;
						m_G2.iByte = 1;
						break;
					case 0x2B:
						m_G3.iMF = pbSrc[3];
						m_G3.iMode = MF_MODE_OTHER;
						m_G3.iByte = 1;
						break;
					default:
						break;
				}
				dwReadSize = 4;
			}else{
				//2バイトGセット
				switch(pbSrc[1]){
					case 0x29:
						m_G1.iMF = pbSrc[2];
						m_G1.iMode = MF_MODE_G;
						m_G1.iByte = 2;
						break;
					case 0x2A:
						m_G2.iMF = pbSrc[2];
						m_G2.iMode = MF_MODE_G;
						m_G2.iByte = 2;
						break;
					case 0x2B:
						m_G3.iMF = pbSrc[2];
						m_G3.iMode = MF_MODE_G;
						m_G3.iByte = 2;
						break;
					default:
						break;
				}
				dwReadSize = 3;
			}
		}else{
			//2バイトGセット
			m_G0.iMF = pbSrc[1];
			m_G0.iMode = MF_MODE_G;
			m_G0.iByte = 2;
			dwReadSize = 2;
		}
	}else if( pbSrc[0] >= 0x28 && pbSrc[0] <= 0x2B ){
		if( dwSrcSize < 2 ) return FALSE;

		if( pbSrc[1] == 0x20 ){
			if( dwSrcSize < 3 ) return FALSE;
			//1バイトDRCS
			switch(pbSrc[0]){
				case 0x28:
					m_G0.iMF = pbSrc[2];
					m_G0.iMode = MF_MODE_DRCS;
					m_G0.iByte = 1;
					break;
				case 0x29:
					m_G1.iMF = pbSrc[2];
					m_G1.iMode = MF_MODE_DRCS;
					m_G1.iByte = 1;
					break;
				case 0x2A:
					m_G2.iMF = pbSrc[2];
					m_G2.iMode = MF_MODE_DRCS;
					m_G2.iByte = 1;
					break;
				case 0x2B:
					m_G3.iMF = pbSrc[2];
					m_G3.iMode = MF_MODE_DRCS;
					m_G3.iByte = 1;
					break;
				default:
					break;
			}
			dwReadSize = 3;
		}else{
			//1バイトGセット
			switch(pbSrc[0]){
				case 0x28:
					m_G0.iMF = pbSrc[1];
					m_G0.iMode = MF_MODE_G;
					m_G0.iByte = 1;
					break;
				case 0x29:
					m_G1.iMF = pbSrc[1];
					m_G1.iMode = MF_MODE_G;
					m_G1.iByte = 1;
					break;
				case 0x2A:
					m_G2.iMF = pbSrc[1];
					m_G2.iMode = MF_MODE_G;
					m_G2.iByte = 1;
					break;
				case 0x2B:
					m_G3.iMF = pbSrc[1];
					m_G3.iMode = MF_MODE_G;
					m_G3.iByte = 1;
					break;
				default:
					break;
			}
			dwReadSize = 2;
		}
	}else if( pbSrc[0] == 0x6E ){
		//GLにG2セット
		m_GL = &m_G2;
		dwReadSize = 1;
	}else if( pbSrc[0] == 0x6F ){
		//GLにG3セット
		m_GL = &m_G3;
		dwReadSize = 1;
	}else if( pbSrc[0] == 0x7C ){
		//GRにG3セット
		m_GR = &m_G3;
		dwReadSize = 1;
	}else if( pbSrc[0] == 0x7D ){
		//GRにG2セット
		m_GR = &m_G2;
		dwReadSize = 1;
	}else if( pbSrc[0] == 0x7E ){
		//GRにG1セット
		m_GR = &m_G1;
		dwReadSize = 1;
	}else{
		//未サポート
		return FALSE;
	}

	*pdwReadSize = dwReadSize;

	return TRUE;
}

BOOL CARIB8CharDecode::CSI( const BYTE* pbSrc, DWORD dwSrcSize, DWORD* pdwReadSize )
{
	DWORD dwReadSize = 0;

	//中間文字0x20まで移動
	WORD wP1 = 0;
	WORD wP2 = 0;
	int nParam = 0;
	for( ; dwReadSize+1<dwSrcSize; dwReadSize++ ){
		if( pbSrc[dwReadSize] == 0x20 ){
			if( nParam==0 ){
				wP1 = wP2;
			}
			nParam++;
			break;
		}else if( pbSrc[dwReadSize] == 0x3B ){
			if( nParam==0 ){
				wP1 = wP2;
				wP2 = 0;
			}
			nParam++;
		}else if( 0x30<=pbSrc[dwReadSize] && pbSrc[dwReadSize]<=0x39 ){
			if( nParam<=1 ){
				wP2 = wP2*10+(pbSrc[dwReadSize]&0x0F);
			}
		}
	}
	//終端文字に移動
	if( ++dwReadSize >= dwSrcSize ){
		return FALSE;
	}

	switch(pbSrc[dwReadSize]){
		case 0x53:
			//SWF
			if( nParam==1 ){
				m_wSWFMode = wP1;
			}else{
				//未サポート
			}
			break;
		case 0x6E:
			//RCS
			m_bRasterColorIndex = (BYTE)(wP1&0x7F);
			break;
		case 0x61:
			//ACPS
			m_wPosX = wP1;
			if( nParam>=2 ){
				m_wPosY = wP2;
			}
			break;
		case 0x56:
			//SDF
			m_wClientW = wP1;
			if( nParam>=2 ){
				m_wClientH = wP2;
			}
			break;
		case 0x5F:
			//SDP
			m_wClientX = wP1;
			if( nParam>=2 ){
				m_wClientY = wP2;
			}
			break;
		case 0x57:
			//SSM
			m_wCharW = wP1;
			if( nParam>=2 ){
				m_wCharH = wP2;
			}
			break;
		case 0x58:
			//SHS
			m_wCharHInterval = wP1;
			break;
		case 0x59:
			//SVS
			m_wCharVInterval = wP1;
			break;
		case 0x42:
			//GSM
			//未サポート
			break;
		case 0x5D:
			//GAA
			//未サポート
			break;
		case 0x5E:
			//SRC
			//未サポート
			break;
		case 0x62:
			//TCC
			//未サポート
			break;
		case 0x65:
			//CFS
			//未サポート
			break;
		case 0x63:
			//ORN
			if( wP1 == 0x02 ){
				m_bShadow = TRUE;
			}
			break;
		case 0x64:
			//MDF
			if( wP1 == 0 ){
				m_bBold = FALSE;
				m_bItalic = FALSE;
			}else if( wP1 == 1 ){
				m_bBold = TRUE;
			}else if( wP1 == 2 ){
				m_bItalic = TRUE;
			}else if( wP1 == 3 ){
				m_bBold = TRUE;
				m_bItalic = TRUE;
			}
			break;
		case 0x66:
			//XCS
			//未サポート
			break;
		case 0x68:
			//PRA
			//未サポート
			break;
		case 0x54:
			//CCC
			//未サポート
			break;
		case 0x67:
			//SCR
			//未サポート
			break;
		case 0x69:
			//ACS
			//未サポート
			break;
		default:
			break;
	}
	dwReadSize++;

	*pdwReadSize = dwReadSize;

	return TRUE;
}

void CARIB8CharDecode::CheckModify(void)
{
	if( m_bPSI == TRUE ){
		return;
	}
	if( m_strDecode.length() > 0 ){
		if( IsChgPos() == FALSE ){
			CAPTION_CHAR_DATA CharItem;
			CreateCaptionCharData(&CharItem);
			(*m_pCaptionList)[m_pCaptionList->size()-1].CharList.push_back(CharItem);
			m_strDecode = L"";
		}else{
			CAPTION_DATA Item;
			CreateCaptionData(&Item);
			m_pCaptionList->push_back(Item);

			CAPTION_CHAR_DATA CharItem;
			CreateCaptionCharData(&CharItem);
			(*m_pCaptionList)[m_pCaptionList->size()-1].CharList.push_back(CharItem);
			m_strDecode = L"";
			m_dwWaitTime = 0;
		}
	}
}

void CARIB8CharDecode::CreateCaptionData(CAPTION_DATA* pItem)
{
	pItem->bClear = FALSE;
	pItem->dwWaitTime = m_dwWaitTime*100;
	pItem->wSWFMode = m_wSWFMode;
	pItem->wClientX = m_wClientX;
	pItem->wClientY = m_wClientY;
	pItem->wClientW = m_wClientW;
	pItem->wClientH = m_wClientH;
	pItem->wPosX = m_wPosX;
	pItem->wPosY = m_wPosY;
}

void CARIB8CharDecode::CreateCaptionCharData(CAPTION_CHAR_DATA* pItem)
{
	pItem->strDecode = m_strDecode;
//	OutputDebugStringA(m_strDecode.c_str());

	pItem->stCharColor = DefClut[m_bCharColorIndex];
	pItem->stBackColor = DefClut[m_bBackColorIndex];
	pItem->stRasterColor = DefClut[m_bRasterColorIndex];

	pItem->bUnderLine = m_bUnderLine;
	pItem->bShadow = m_bShadow;
	pItem->bBold = m_bBold;
	pItem->bItalic = m_bItalic;
	pItem->bFlushMode = m_bFlushMode;

	pItem->wCharW = m_wCharW;
	pItem->wCharH = m_wCharH;
	pItem->wCharHInterval = m_wCharHInterval;
	pItem->wCharVInterval = m_wCharVInterval;
	pItem->emCharSizeMode = m_emStrSize;
}

BOOL CARIB8CharDecode::IsChgPos(void)
{
	if( m_pCaptionList == NULL || m_strDecode.length() == 0){
		return FALSE;
	}
	if( m_pCaptionList->size() == 0 ){
		return TRUE;
	}
	int iIndex = (int)m_pCaptionList->size()-1;
	if( (*m_pCaptionList)[iIndex].wClientH != m_wClientH ){
		return TRUE;
	}
	if( (*m_pCaptionList)[iIndex].wClientW != m_wClientW ){
		return TRUE;
	}
	if( (*m_pCaptionList)[iIndex].wClientX != m_wClientX ){
		return TRUE;
	}
	if( (*m_pCaptionList)[iIndex].wClientY != m_wClientY ){
		return TRUE;
	}
	if( (*m_pCaptionList)[iIndex].wPosX != m_wPosX ){
		return TRUE;
	}
	if( (*m_pCaptionList)[iIndex].wPosY != m_wPosY ){
		return TRUE;
	}

	return FALSE;
}

