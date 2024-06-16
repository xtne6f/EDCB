#include "stdafx.h"
#include "ARIB8CharDecode.h"

//CP932に存在しない文字も使用する場合はこのマクロを定義する
//#define ARIB8CHAR_USE_UNICODE

const WCHAR* const CARIB8CharDecode::TELETEXT_MARK =
#ifndef ARIB8CHAR_USE_UNICODE
	L"[字]";
#elif WCHAR_MAX > 0xFFFF
	L"\x1F211";
#else
	L"\xD83C\xDE11";
#endif

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
	m_bORNColorIndex = 0;
	m_bDefPalette = 0;

	m_bUnderLine = FALSE;
	m_bBold = FALSE;
	m_bItalic = FALSE;
	m_bFlushMode = 0;
	m_bORN = 0;

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
	m_G0.iMF = MF_KANJI;
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
	m_bORNColorIndex = 8;
	m_bDefPalette = 0;

	m_bUnderLine = FALSE;
	m_bBold = FALSE;
	m_bItalic = FALSE;
	m_bFlushMode = 0;
	m_bORN = 0;

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
	if( pbSrc == NULL || dwSrcSize == 0 ){
		return FALSE;
	}
	InitPSISI();
	DWORD dwReadSize = 0;

	BOOL bRet = Analyze(pbSrc, dwSrcSize, &dwReadSize );
	if( strDec ){
		*strDec = m_strDecode;
	}
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
		BOOL bRet = FALSE;
		DWORD dwReadBuff = 0;
		//1バイト目チェック
		if( pbSrc[dwReadSize] <= 0x20 ){
			//C0制御コード
			bRet = C0( pbSrc+dwReadSize, dwSrcSize-dwReadSize, &dwReadBuff );
		}else if( pbSrc[dwReadSize] < 0x7F ){
			//GL符号領域
			bRet = GL_GR( pbSrc+dwReadSize, dwSrcSize-dwReadSize, &dwReadBuff, m_GL );
		}else if( pbSrc[dwReadSize] <= 0xA0 ){
			//C1制御コード
			bRet = C1( pbSrc+dwReadSize, dwSrcSize-dwReadSize, &dwReadBuff );
		}else if( pbSrc[dwReadSize] < 0xFF ){
			//GR符号領域
			bRet = GL_GR( pbSrc+dwReadSize, dwSrcSize-dwReadSize, &dwReadBuff, m_GR );
		}
		if( !bRet ){
			return FALSE;
		}
		dwReadSize += dwReadBuff;
	}

	*pdwReadSize = dwReadSize;
	return TRUE;
}

BOOL CARIB8CharDecode::C0( const BYTE* pbSrc, DWORD dwSrcSize, DWORD* pdwReadSize )
{
	if( dwSrcSize == 0 ){
		return FALSE;
	}

	DWORD dwReadSize = 1;

	switch(pbSrc[0]){
	case 0x20:
		//SP 空白
		//空白は文字サイズの影響あり
		if( IsSmallCharMode() == FALSE ){
			m_strDecode += L'　';
		}else{
			m_strDecode += L' ';
		}
		break;
	case 0x0D:
		//APR 改行
		m_strDecode += L"\r\n";
		break;
	case 0x0E:
		//LS1 GLにG1セット
		m_GL = &m_G1;
		break;
	case 0x0F:
		//LS0 GLにG0セット
		m_GL = &m_G0;
		break;
	case 0x19:
		//SS2 シングルシフト
		{
			//G2で呼ぶ(マクロ展開を考慮してGLは入れ替えない)
			DWORD dwRead;
			if( GL_GR(pbSrc + dwReadSize, dwSrcSize - dwReadSize, &dwRead, &m_G2) == FALSE ){
				return FALSE;
			}
			dwReadSize += dwRead;
		}
		break;
	case 0x1D:
		//SS3 シングルシフト
		{
			//G3で呼ぶ(マクロ展開を考慮してGLは入れ替えない)
			DWORD dwRead;
			if( GL_GR(pbSrc + dwReadSize, dwSrcSize - dwReadSize, &dwRead, &m_G3) == FALSE ){
				return FALSE;
			}
			dwReadSize += dwRead;
		}
		break;
	case 0x1B:
		//ESC エスケープシーケンス
		{
			DWORD dwRead;
			if( ESC(pbSrc + dwReadSize, dwSrcSize - dwReadSize, &dwRead) == FALSE ){
				return FALSE;
			}
			dwReadSize += dwRead;
		}
		break;
	case 0x16:
		//PAPF 指定動作位置前進
		if( dwSrcSize <= dwReadSize ){
			return FALSE;
		}
		dwReadSize++;
		break;
	case 0x1C:
		//APS 動作位置指定
		{
			if( dwSrcSize <= dwReadSize + 1 ){
				return FALSE;
			}
			CheckModify();
			m_wPosY=m_wCharH*(pbSrc[dwReadSize++]-0x40);
			m_wPosX=m_wCharW*(pbSrc[dwReadSize++]-0x40);
			if( m_emStrSize == STR_SMALL || m_emStrSize == STR_MEDIUM ){
				m_wPosX=m_wPosX/2;
			}
		}
		break;
	case 0x0C:
		//CS
		{
			CAPTION_DATA Item;
			Item.bClear = TRUE;
			Item.dwWaitTime = m_dwWaitTime;
			if( m_pCaptionList != NULL ){
				m_pCaptionList->push_back(Item);
			}
			//TODO: 字幕系ではここで初期化動作が必要
			m_dwWaitTime = 0;
		}
		break;
	default:
		//未サポートの制御コード
		//APB、APF、APD、APU
		break;
	}

	*pdwReadSize = dwReadSize;

	return TRUE;
}

BOOL CARIB8CharDecode::C1( const BYTE* pbSrc, DWORD dwSrcSize, DWORD* pdwReadSize )
{
	if( dwSrcSize == 0 ){
		return FALSE;
	}
	DWORD dwReadSize = 1;

	CheckModify();

	switch(pbSrc[0]){
	case 0x89:
		//MSZ 半角指定
		m_emStrSize = STR_MEDIUM;
		break;
	case 0x8A:
		//NSZ 全角指定
		m_emStrSize = STR_NORMAL;
		break;
	case 0x80:
		//BKF 文字黒
		m_bCharColorIndex = (m_bDefPalette<<4) | 0x00;
		break;
	case 0x81:
		//RDF 文字赤
		m_bCharColorIndex = (m_bDefPalette<<4) | 0x01;
		break;
	case 0x82:
		//GRF 文字緑
		m_bCharColorIndex = (m_bDefPalette<<4) | 0x02;
		break;
	case 0x83:
		//YLF 文字黄
		m_bCharColorIndex = (m_bDefPalette<<4) | 0x03;
		break;
	case 0x84:
		//BLF 文字青
		m_bCharColorIndex = (m_bDefPalette<<4) | 0x04;
		break;
	case 0x85:
		//MGF 文字マゼンタ
		m_bCharColorIndex = (m_bDefPalette<<4) | 0x05;
		break;
	case 0x86:
		//CNF 文字シアン
		m_bCharColorIndex = (m_bDefPalette<<4) | 0x06;
		break;
	case 0x87:
		//WHF 文字白
		m_bCharColorIndex = (m_bDefPalette<<4) | 0x07;
		break;
	case 0x88:
		//SSZ 小型サイズ
		m_emStrSize = STR_SMALL;
		break;
	case 0x8B:
		//SZX 指定サイズ
		if( dwSrcSize <= dwReadSize ) return FALSE;
		if( pbSrc[dwReadSize] == 0x60 ){
			m_emStrSize = STR_MICRO;
		}else if( pbSrc[dwReadSize] == 0x41 ){
			m_emStrSize = STR_HIGH_W;
		}else if( pbSrc[dwReadSize] == 0x44 ){
			m_emStrSize = STR_WIDTH_W;
		}else if( pbSrc[dwReadSize] == 0x45 ){
			m_emStrSize = STR_W;
		}else if( pbSrc[dwReadSize] == 0x6B ){
			m_emStrSize = STR_SPECIAL_1;
		}else if( pbSrc[dwReadSize] == 0x64 ){
			m_emStrSize = STR_SPECIAL_2;
		}
		dwReadSize++;
		break;
	case 0x90:
		//COL 色指定
		if( dwSrcSize <= dwReadSize ) return FALSE;
		if( pbSrc[dwReadSize] == 0x20 ){
			dwReadSize++;
			if( dwSrcSize <= dwReadSize ) return FALSE;
			//規定によりパレットは0から127まで使われる
			m_bDefPalette = pbSrc[dwReadSize++] & 0x07;
		}else{
			switch( pbSrc[dwReadSize] & 0xF0 ){
				case 0x40:
					m_bCharColorIndex = (m_bDefPalette<<4) | (pbSrc[dwReadSize]&0x0F);
					break;
				case 0x50:
					m_bBackColorIndex = (m_bDefPalette<<4) | (pbSrc[dwReadSize]&0x0F);
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
			dwReadSize++;
		}
		break;
	case 0x91:
		//FLC フラッシング制御
		if( dwSrcSize <= dwReadSize ) return FALSE;
		if( pbSrc[dwReadSize] == 0x40 ){
			m_bFlushMode = 1;
		}else if( pbSrc[dwReadSize] == 0x47 ){
			m_bFlushMode = 2;
		}else if( pbSrc[dwReadSize] == 0x4F ){
			m_bFlushMode = 0;
		}
		dwReadSize++;
		break;
	case 0x93:
		//POL パターン極性
		//未サポート
		if( dwSrcSize <= dwReadSize ) return FALSE;
		dwReadSize++;
		break;
	case 0x94:
		//WMM 書き込みモード変更
		//未サポート
		if( dwSrcSize <= dwReadSize ) return FALSE;
		dwReadSize++;
		break;
	case 0x95:
		//MACRO マクロ定義
		//未サポート(MACRO 0x4Fまで送る)
		dwReadSize++;
		do{
			if( ++dwReadSize > dwSrcSize ){
				return FALSE;
			}
		}while( pbSrc[dwReadSize-2] != 0x95 || pbSrc[dwReadSize-1] != 0x4F );
		break;
	case 0x97:
		//HLC 囲み制御
		//未サポート
		if( dwSrcSize <= dwReadSize ) return FALSE;
		dwReadSize++;
		break;
	case 0x98:
		//RPC 文字繰り返し
		//未サポート
		if( dwSrcSize <= dwReadSize ) return FALSE;
		dwReadSize++;
		break;
	case 0x99:
		//SPL アンダーライン モザイクの終了
		m_bUnderLine = FALSE;
		break;
	case 0x9A:
		//STL アンダーライン モザイクの開始
		m_bUnderLine = TRUE;
		break;
	case 0x9D:
		//TIME 時間制御
		if( dwSrcSize <= dwReadSize + 1 ) return FALSE;
		if( pbSrc[dwReadSize] == 0x20 ){
			dwReadSize++;
			m_dwWaitTime += (pbSrc[dwReadSize++] - 0x40) * 100;
		}else{
			//未サポート
			do{
				if( ++dwReadSize > dwSrcSize ){
					return FALSE;
				}
			}while( pbSrc[dwReadSize-1] < 0x40 || 0x43 < pbSrc[dwReadSize-1] );
		}
		break;
	case 0x9B:
		//CSI コントロールシーケンス
		{
			DWORD dwRead;
			if( CSI(pbSrc + dwReadSize, dwSrcSize - dwReadSize, &dwRead) == FALSE ){
				return FALSE;
			}
			dwReadSize += dwRead;
		}
		break;
	default:
		//未サポートの制御コード
		break;
	}

	*pdwReadSize = dwReadSize;

	return TRUE;
}

BOOL CARIB8CharDecode::GL_GR( const BYTE* pbSrc, DWORD dwSrcSize, DWORD* pdwReadSize, const MF_MODE* mode )
{
	if( dwSrcSize == 0 ){
		return FALSE;
	}
	BYTE b = pbSrc[0] & 0x7F;
	if( b < 0x21 || 0x7F <= b ){
		return FALSE;
	}

	DWORD dwReadSize = 0;
	if( mode->iMode == MF_MODE_G ){
		//文字コード
		switch( mode->iMF ){
			case MF_JISX_KANA:
				{
				//JIS X0201片仮名
				m_strDecode += JisXKanaTable[b - 0x21];
				dwReadSize = 1;
				}
				break;
			case MF_ASCII:
			case MF_PROP_ASCII:
				{
				if( IsSmallCharMode() == FALSE ){
					//全角なのでテーブルからコード取得
					m_strDecode += AsciiTable[b - 0x21];
				}else{
					//半角なのでそのまま入れる
					m_strDecode += b;
				}
				dwReadSize = 1;
				}
				break;
			case MF_HIRA:
			case MF_PROP_HIRA:
				{
				//Gセットのひらがな系集合
				m_strDecode += HiraTable[b - 0x21];
				dwReadSize = 1;
				}
				break;
			case MF_KANA:
			case MF_PROP_KANA:
				{
				//Gセットのカタカナ系集合
				m_strDecode += KanaTable[b - 0x21];
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
				BYTE bSecond = pbSrc[1] & 0x7F;
				if( mode->iMF == MF_JIS_KANJI2 ){
					//JIS互換漢字2面。テーブル未実装
					m_strDecode += L'〓';
				}else if( b >= 0x21 + 84 || bSecond < 0x21 || bSecond >= 0x21 + 94 ){
					if( mode->iMF == MF_JIS_KANJI1 ){
						//JIS互換漢字1面の第3水準。テーブル未実装
						m_strDecode += L'〓';
					}else{
						ToCustomFont(b, bSecond);
					}
				}else{
					//テーブルからコード取得
					m_strDecode += m_jisTable[(b - 0x21) * 94 + (bSecond - 0x21)];
					if( mode->iMF == MF_JIS_KANJI1 && m_strDecode.back() == L'・' && (b != 0x21 || bSecond != 0x21 + 5) ){
						//JIS互換漢字1面では1区6点以外の・はテーブル未実装
						m_strDecode.back() = L'〓';
					}
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
			if( 0x60 <= b && b <= 0x6F ){
				if( Analyze(DefaultMacro[b & 0x0F], sizeof(DefaultMacro[0]), &dwTemp) == FALSE ){
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
#if WCHAR_MAX > 0xFFFF
	//テーブル上ではサロゲートペアで表現しているので結合する
	if( 0xD800 <= (WORD)t.strCharUnicode[0] && (WORD)t.strCharUnicode[0] < 0xDC00 ){
		m_strDecode += (WCHAR)(0x10000 + ((WORD)t.strCharUnicode[0] - 0xD800) * 0x400 + ((WORD)t.strCharUnicode[1] - 0xDC00));
	}else
#endif
	{
		m_strDecode += t.strCharUnicode;
	}
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
			if( wP1 <= 3 ){
				BYTE bIndex = (BYTE)((nParam >= 2 && (wP1 == 1 || wP1 == 2) ? (wP2 / 100) << 4 | (wP2 % 100) : 8) & 0x7F);
				if( wP1 != m_bORN || bIndex != m_bORNColorIndex ){
					CheckModify();
				}
				m_bORN = (BYTE)wP1;
				m_bORNColorIndex = bIndex;
			}
			break;
		case 0x64:
			//MDF
			if( wP1 <= 3 ){
				if( ((wP1 & 1) != 0) != m_bBold ||
				    ((wP1 & 2) != 0) != m_bItalic ){
					CheckModify();
				}
				m_bBold = (wP1 & 1) != 0;
				m_bItalic = (wP1 & 2) != 0;
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

void CARIB8CharDecode::CreateCaptionData(CAPTION_DATA* pItem) const
{
	pItem->bClear = FALSE;
	pItem->dwWaitTime = m_dwWaitTime;
	pItem->wSWFMode = m_wSWFMode;
	pItem->wClientX = m_wClientX;
	pItem->wClientY = m_wClientY;
	pItem->wClientW = m_wClientW;
	pItem->wClientH = m_wClientH;
	pItem->wPosX = m_wPosX;
	pItem->wPosY = m_wPosY;
}

void CARIB8CharDecode::CreateCaptionCharData(CAPTION_CHAR_DATA* pItem) const
{
	pItem->strDecode = m_strDecode;

	pItem->stCharColor = DefClut[m_bCharColorIndex];
	pItem->stBackColor = DefClut[m_bBackColorIndex];
	pItem->stRasterColor = DefClut[m_bRasterColorIndex];
	pItem->stORNColor = DefClut[m_bORNColorIndex];

	pItem->bUnderLine = m_bUnderLine;
	pItem->bBold = m_bBold;
	pItem->bItalic = m_bItalic;
	pItem->bFlushMode = m_bFlushMode;
	pItem->bORN = m_bORN;

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

const CLUT_DAT CARIB8CharDecode::DefClut[128] = {
	{  0,   0,   0, 255}, //0
	{255,   0,   0, 255},
	{  0, 255,   0, 255},
	{255, 255,   0, 255},
	{  0,   0, 255, 255},
	{255,   0, 255, 255}, //5
	{  0, 255, 255, 255},
	{255, 255, 255, 255},
	{  0,   0,   0,   0},
	{170,   0,   0, 255},
	{  0, 170,   0, 255}, //10
	{170, 170,   0, 255},
	{  0,   0, 170, 255},
	{170,   0, 170, 255},
	{  0, 170, 170, 255},
	{170, 170, 170, 255}, //15
	{  0,   0,  85, 255},
	{  0,  85,   0, 255},
	{  0,  85,  85, 255},
	{  0,  85, 170, 255},
	{  0,  85, 255, 255}, //20
	{  0, 170,  85, 255},
	{  0, 170, 255, 255},
	{  0, 255,  85, 255},
	{  0, 255, 170, 255},
	{ 85,   0,   0, 255}, //25
	{ 85,   0,  85, 255},
	{ 85,   0, 170, 255},
	{ 85,   0, 255, 255},
	{ 85,  85,   0, 255},
	{ 85,  85,  85, 255}, //30
	{ 85,  85, 170, 255},
	{ 85,  85, 255, 255},
	{ 85, 170,   0, 255},
	{ 85, 170,  85, 255},
	{ 85, 170, 170, 255}, //35
	{ 85, 170, 255, 255},
	{ 85, 255,   0, 255},
	{ 85, 255,  85, 255},
	{ 85, 255, 170, 255},
	{ 85, 255, 255, 255}, //40
	{170,   0,  85, 255},
	{170,   0, 255, 255},
	{170,  85,   0, 255},
	{170,  85,  85, 255},
	{170,  85, 170, 255}, //45
	{170,  85, 255, 255},
	{170, 170,  85, 255},
	{170, 170, 255, 255},
	{170, 255,   0, 255},
	{170, 255,  85, 255}, //50
	{170, 255, 170, 255},
	{170, 255, 255, 255},
	{255,   0,  85, 255},
	{255,   0, 170, 255},
	{255,  85,   0, 255}, //55
	{255,  85,  85, 255},
	{255,  85, 170, 255},
	{255,  85, 255, 255},
	{255, 170,   0, 255},
	{255, 170,  85, 255}, //60
	{255, 170, 170, 255},
	{255, 170, 255, 255},
	{255, 255,  85, 255},
	{255, 255, 170, 255},
	{  0,   0,   0, 128}, //65
	{255,   0,   0, 128},
	{  0, 255,   0, 128},
	{255, 255,   0, 128},
	{  0,   0, 255, 128},
	{255,   0, 255, 128}, //70
	{  0, 255, 255, 128},
	{255, 255, 255, 128},
	{170,   0,   0, 128},
	{  0, 170,   0, 128},
	{170, 170,   0, 128}, //75
	{  0,   0, 170, 128},
	{170,   0, 170, 128},
	{  0, 170, 170, 128},
	{170, 170, 170, 128},
	{  0,   0,  85, 128}, //80
	{  0,  85,   0, 128},
	{  0,  85,  85, 128},
	{  0,  85, 170, 128},
	{  0,  85, 255, 128},
	{  0, 170,  85, 128}, //85
	{  0, 170, 255, 128},
	{  0, 255,  85, 128},
	{  0, 255, 170, 128},
	{ 85,   0,   0, 128},
	{ 85,   0,  85, 128}, //90
	{ 85,   0, 170, 128},
	{ 85,   0, 255, 128},
	{ 85,  85,   0, 128},
	{ 85,  85,  85, 128},
	{ 85,  85, 170, 128}, //95
	{ 85,  85, 255, 128},
	{ 85, 170,   0, 128},
	{ 85, 170,  85, 128},
	{ 85, 170, 170, 128},
	{ 85, 170, 255, 128}, //100
	{ 85, 255,   0, 128},
	{ 85, 255,  85, 128},
	{ 85, 255, 170, 128},
	{ 85, 255, 255, 128},
	{170,   0,  85, 128}, //105
	{170,   0, 255, 128},
	{170,  85,   0, 128},
	{170,  85,  85, 128},
	{170,  85, 170, 128},
	{170,  85, 255, 128}, //110
	{170, 170,  85, 128},
	{170, 170, 255, 128},
	{170, 255,   0, 128},
	{170, 255,  85, 128},
	{170, 255, 170, 128}, //115
	{170, 255, 255, 128},
	{255,   0,  85, 128},
	{255,   0, 170, 128},
	{255,  85,   0, 128},
	{255,  85,  85, 128}, //120
	{255,  85, 170, 128},
	{255,  85, 255, 128},
	{255, 170,   0, 128},
	{255, 170,  85, 128},
	{255, 170, 170, 128}, //125
	{255, 170, 255, 128},
	{255, 255,  85, 128}
};

const WCHAR CARIB8CharDecode::AsciiTable[94] = {
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

const WCHAR CARIB8CharDecode::HiraTable[94] = {
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

const WCHAR CARIB8CharDecode::KanaTable[94] = {
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

const WCHAR CARIB8CharDecode::JisXKanaTable[94] = {
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

//デフォルトマクロ文(NULは効果がないと規定されている)
const BYTE CARIB8CharDecode::DefaultMacro[16][20] = {
	{0x1B, 0x24, 0x42, 0x1B, 0x29, 0x4A, 0x1B, 0x2A, 0x30, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
	{0x1B, 0x24, 0x42, 0x1B, 0x29, 0x31, 0x1B, 0x2A, 0x30, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
	{0x1B, 0x24, 0x42, 0x1B, 0x29, 0x20, 0x41, 0x1B, 0x2A, 0x30, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
	{0x1B, 0x28, 0x32, 0x1B, 0x29, 0x34, 0x1B, 0x2A, 0x35, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
	{0x1B, 0x28, 0x32, 0x1B, 0x29, 0x33, 0x1B, 0x2A, 0x35, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
	{0x1B, 0x28, 0x32, 0x1B, 0x29, 0x20, 0x41, 0x1B, 0x2A, 0x35, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
	{0x1B, 0x28, 0x20, 0x41, 0x1B, 0x29, 0x20, 0x42, 0x1B, 0x2A, 0x20, 0x43, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
	{0x1B, 0x28, 0x20, 0x44, 0x1B, 0x29, 0x20, 0x45, 0x1B, 0x2A, 0x20, 0x46, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
	{0x1B, 0x28, 0x20, 0x47, 0x1B, 0x29, 0x20, 0x48, 0x1B, 0x2A, 0x20, 0x49, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
	{0x1B, 0x28, 0x20, 0x4A, 0x1B, 0x29, 0x20, 0x4B, 0x1B, 0x2A, 0x20, 0x4C, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
	{0x1B, 0x28, 0x20, 0x4D, 0x1B, 0x29, 0x20, 0x4E, 0x1B, 0x2A, 0x20, 0x4F, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
	{0x1B, 0x24, 0x42, 0x1B, 0x29, 0x20, 0x42, 0x1B, 0x2A, 0x30, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
	{0x1B, 0x24, 0x42, 0x1B, 0x29, 0x20, 0x43, 0x1B, 0x2A, 0x30, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
	{0x1B, 0x24, 0x42, 0x1B, 0x29, 0x20, 0x44, 0x1B, 0x2A, 0x30, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
	{0x1B, 0x28, 0x31, 0x1B, 0x29, 0x30, 0x1B, 0x2A, 0x4A, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
	{0x1B, 0x28, 0x4A, 0x1B, 0x29, 0x32, 0x1B, 0x2A, 0x20, 0x41, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D}
};

const GAIJI_TABLE CARIB8CharDecode::GaijiTable[40 + 91 + 63 + 2 + 93] = {
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
	{L"\xD83C\xDD3D", L"[Ｎ]"},
	{L"\x2B1B", L"■"}, //90区64点
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
	{L"\xD83C\xDE24", L"[声]"}, //90区80点
	{L"\xD83C\xDE25", L"[吹]"},
	{L"\xD83C\xDD4E", L"[PPV]"},
	{L"\x3299", L"(秘)"},
	{L"\xD83C\xDE00", L"ほか"},
	//91区は飛ばす
	{L"\x27A1", L"→"}, //92区1点
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
	{L"\xD83C\xDD00", L"０."}, //92区16点
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
	{L"\xD83C\xDD01", L"０,"}, //92区32点
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
	{L"\x25B6", L"▲"}, //92区48点
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
	{L"(ｓ)", L"(ｓ)"}, //92区64点
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
	{L"(sy", L"(sy"}, //92区80点
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
	{L"\x322A", L"(月)"}, //93区1点
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
	{L"\x26BE", L"()()"}, //93区16点
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
	{L"\xD83C\xDE2B", L"[遊]"}, //93区32点
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
	{L"\x00BD", L"1/2"}, //93区48点
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
	{L"\x203C", L"!!"}, //93区78点
	{L"\x2049", L"!?"},
	{L"Ⅰ", L"Ⅰ"}, //94区1点
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
	{L"⑳", L"⑳"}, //94区16点
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
	{L"\x3254", L"(24)"}, //94区32点
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
	{L"\xD83C\xDD1F", L"(Ｐ)"}, //94区48点
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
	{L"\x325A", L"(30)"}, //94区64点
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
	{L"⑯", L"⑯"}, //94区80点
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

const GAIJI_TABLE CARIB8CharDecode::GaijiTbl2[94 + 43] = {
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

const WCHAR CARIB8CharDecode::m_jisTable[84 * 94 + 1] =
	L"　、。，．・：；？！゛゜´｀¨＾￣＿ヽヾゝゞ〃仝々〆〇ー―‐／＼～∥｜…‥‘’“”（）〔〕［］｛｝〈〉《》「」『』【】＋－±×÷＝≠＜＞≦≧∞∴♂♀°′″℃￥＄￠￡％＃＆＊＠§☆★○●◎◇"
	L"◆□■△▲▽▼※〒→←↑↓〓・・・・・・・・・・・∈∋⊆⊇⊂⊃∪∩・・・・・・・・∧∨￢⇒⇔∀∃・・・・・・・・・・・∠⊥⌒∂∇≡≒≪≫√∽∝∵∫∬・・・・・・・Å‰♯♭♪†‡¶・・・・◯"
	L"・・・・・・・・・・・・・・・０１２３４５６７８９・・・・・・・ＡＢＣＤＥＦＧＨＩＪＫＬＭＮＯＰＱＲＳＴＵＶＷＸＹＺ・・・・・・ａｂｃｄｅｆｇｈｉｊｋｌｍｎｏｐｑｒｓｔｕｖｗｘｙｚ・・・・"
	L"ぁあぃいぅうぇえぉおかがきぎくぐけげこごさざしじすずせぜそぞただちぢっつづてでとどなにぬねのはばぱひびぴふぶぷへべぺほぼぽまみむめもゃやゅゆょよらりるれろゎわゐゑをん・・・・・・・・・・・"
	L"ァアィイゥウェエォオカガキギクグケゲコゴサザシジスズセゼソゾタダチヂッツヅテデトドナニヌネノハバパヒビピフブプヘベペホボポマミムメモャヤュユョヨラリルレロヮワヰヱヲンヴヵヶ・・・・・・・・"
	L"ΑΒΓΔΕΖΗΘΙΚΛΜΝΞΟΠΡΣΤΥΦΧΨΩ・・・・・・・・αβγδεζηθικλμνξοπρστυφχψω・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・"
	L"АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ・・・・・・・・・・・・・・・абвгдеёжзийклмнопрстуфхцчшщъыьэюя・・・・・・・・・・・・・"
	L"─│┌┐┘└├┬┤┴┼━┃┏┓┛┗┣┳┫┻╋┠┯┨┷┿┝┰┥┸╂・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・"
	L"・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・"
	L"・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・"
	L"・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・"
	L"・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・"
	L"・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・"
	L"・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・"
	L"・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・"
	L"亜唖娃阿哀愛挨姶逢葵茜穐悪握渥旭葦芦鯵梓圧斡扱宛姐虻飴絢綾鮎或粟袷安庵按暗案闇鞍杏以伊位依偉囲夷委威尉惟意慰易椅為畏異移維緯胃萎衣謂違遺医井亥域育郁磯一壱溢逸稲茨芋鰯允印咽員因姻引飲淫胤蔭"
	L"院陰隠韻吋右宇烏羽迂雨卯鵜窺丑碓臼渦嘘唄欝蔚鰻姥厩浦瓜閏噂云運雲荏餌叡営嬰影映曳栄永泳洩瑛盈穎頴英衛詠鋭液疫益駅悦謁越閲榎厭円園堰奄宴延怨掩援沿演炎焔煙燕猿縁艶苑薗遠鉛鴛塩於汚甥凹央奥往応"
	L"押旺横欧殴王翁襖鴬鴎黄岡沖荻億屋憶臆桶牡乙俺卸恩温穏音下化仮何伽価佳加可嘉夏嫁家寡科暇果架歌河火珂禍禾稼箇花苛茄荷華菓蝦課嘩貨迦過霞蚊俄峨我牙画臥芽蛾賀雅餓駕介会解回塊壊廻快怪悔恢懐戒拐改"
	L"魁晦械海灰界皆絵芥蟹開階貝凱劾外咳害崖慨概涯碍蓋街該鎧骸浬馨蛙垣柿蛎鈎劃嚇各廓拡撹格核殻獲確穫覚角赫較郭閣隔革学岳楽額顎掛笠樫橿梶鰍潟割喝恰括活渇滑葛褐轄且鰹叶椛樺鞄株兜竃蒲釜鎌噛鴨栢茅萱"
	L"粥刈苅瓦乾侃冠寒刊勘勧巻喚堪姦完官寛干幹患感慣憾換敢柑桓棺款歓汗漢澗潅環甘監看竿管簡緩缶翰肝艦莞観諌貫還鑑間閑関陥韓館舘丸含岸巌玩癌眼岩翫贋雁頑顔願企伎危喜器基奇嬉寄岐希幾忌揮机旗既期棋棄"
	L"機帰毅気汽畿祈季稀紀徽規記貴起軌輝飢騎鬼亀偽儀妓宜戯技擬欺犠疑祇義蟻誼議掬菊鞠吉吃喫桔橘詰砧杵黍却客脚虐逆丘久仇休及吸宮弓急救朽求汲泣灸球究窮笈級糾給旧牛去居巨拒拠挙渠虚許距鋸漁禦魚亨享京"
	L"供侠僑兇競共凶協匡卿叫喬境峡強彊怯恐恭挟教橋況狂狭矯胸脅興蕎郷鏡響饗驚仰凝尭暁業局曲極玉桐粁僅勤均巾錦斤欣欽琴禁禽筋緊芹菌衿襟謹近金吟銀九倶句区狗玖矩苦躯駆駈駒具愚虞喰空偶寓遇隅串櫛釧屑屈"
	L"掘窟沓靴轡窪熊隈粂栗繰桑鍬勲君薫訓群軍郡卦袈祁係傾刑兄啓圭珪型契形径恵慶慧憩掲携敬景桂渓畦稽系経継繋罫茎荊蛍計詣警軽頚鶏芸迎鯨劇戟撃激隙桁傑欠決潔穴結血訣月件倹倦健兼券剣喧圏堅嫌建憲懸拳捲"
	L"検権牽犬献研硯絹県肩見謙賢軒遣鍵険顕験鹸元原厳幻弦減源玄現絃舷言諺限乎個古呼固姑孤己庫弧戸故枯湖狐糊袴股胡菰虎誇跨鈷雇顧鼓五互伍午呉吾娯後御悟梧檎瑚碁語誤護醐乞鯉交佼侯候倖光公功効勾厚口向"
	L"后喉坑垢好孔孝宏工巧巷幸広庚康弘恒慌抗拘控攻昂晃更杭校梗構江洪浩港溝甲皇硬稿糠紅紘絞綱耕考肯肱腔膏航荒行衡講貢購郊酵鉱砿鋼閤降項香高鴻剛劫号合壕拷濠豪轟麹克刻告国穀酷鵠黒獄漉腰甑忽惚骨狛込"
	L"此頃今困坤墾婚恨懇昏昆根梱混痕紺艮魂些佐叉唆嵯左差査沙瑳砂詐鎖裟坐座挫債催再最哉塞妻宰彩才採栽歳済災采犀砕砦祭斎細菜裁載際剤在材罪財冴坂阪堺榊肴咲崎埼碕鷺作削咋搾昨朔柵窄策索錯桜鮭笹匙冊刷"
	L"察拶撮擦札殺薩雑皐鯖捌錆鮫皿晒三傘参山惨撒散桟燦珊産算纂蚕讃賛酸餐斬暫残仕仔伺使刺司史嗣四士始姉姿子屍市師志思指支孜斯施旨枝止死氏獅祉私糸紙紫肢脂至視詞詩試誌諮資賜雌飼歯事似侍児字寺慈持時"
	L"次滋治爾璽痔磁示而耳自蒔辞汐鹿式識鴫竺軸宍雫七叱執失嫉室悉湿漆疾質実蔀篠偲柴芝屡蕊縞舎写射捨赦斜煮社紗者謝車遮蛇邪借勺尺杓灼爵酌釈錫若寂弱惹主取守手朱殊狩珠種腫趣酒首儒受呪寿授樹綬需囚収周"
	L"宗就州修愁拾洲秀秋終繍習臭舟蒐衆襲讐蹴輯週酋酬集醜什住充十従戎柔汁渋獣縦重銃叔夙宿淑祝縮粛塾熟出術述俊峻春瞬竣舜駿准循旬楯殉淳準潤盾純巡遵醇順処初所暑曙渚庶緒署書薯藷諸助叙女序徐恕鋤除傷償"
	L"勝匠升召哨商唱嘗奨妾娼宵将小少尚庄床廠彰承抄招掌捷昇昌昭晶松梢樟樵沼消渉湘焼焦照症省硝礁祥称章笑粧紹肖菖蒋蕉衝裳訟証詔詳象賞醤鉦鍾鐘障鞘上丈丞乗冗剰城場壌嬢常情擾条杖浄状畳穣蒸譲醸錠嘱埴飾"
	L"拭植殖燭織職色触食蝕辱尻伸信侵唇娠寝審心慎振新晋森榛浸深申疹真神秦紳臣芯薪親診身辛進針震人仁刃塵壬尋甚尽腎訊迅陣靭笥諏須酢図厨逗吹垂帥推水炊睡粋翠衰遂酔錐錘随瑞髄崇嵩数枢趨雛据杉椙菅頗雀裾"
	L"澄摺寸世瀬畝是凄制勢姓征性成政整星晴棲栖正清牲生盛精聖声製西誠誓請逝醒青静斉税脆隻席惜戚斥昔析石積籍績脊責赤跡蹟碩切拙接摂折設窃節説雪絶舌蝉仙先千占宣専尖川戦扇撰栓栴泉浅洗染潜煎煽旋穿箭線"
	L"繊羨腺舛船薦詮賎践選遷銭銑閃鮮前善漸然全禅繕膳糎噌塑岨措曾曽楚狙疏疎礎祖租粗素組蘇訴阻遡鼠僧創双叢倉喪壮奏爽宋層匝惣想捜掃挿掻操早曹巣槍槽漕燥争痩相窓糟総綜聡草荘葬蒼藻装走送遭鎗霜騒像増憎"
	L"臓蔵贈造促側則即息捉束測足速俗属賊族続卒袖其揃存孫尊損村遜他多太汰詑唾堕妥惰打柁舵楕陀駄騨体堆対耐岱帯待怠態戴替泰滞胎腿苔袋貸退逮隊黛鯛代台大第醍題鷹滝瀧卓啄宅托択拓沢濯琢託鐸濁諾茸凧蛸只"
	L"叩但達辰奪脱巽竪辿棚谷狸鱈樽誰丹単嘆坦担探旦歎淡湛炭短端箪綻耽胆蛋誕鍛団壇弾断暖檀段男談値知地弛恥智池痴稚置致蜘遅馳築畜竹筑蓄逐秩窒茶嫡着中仲宙忠抽昼柱注虫衷註酎鋳駐樗瀦猪苧著貯丁兆凋喋寵"
	L"帖帳庁弔張彫徴懲挑暢朝潮牒町眺聴脹腸蝶調諜超跳銚長頂鳥勅捗直朕沈珍賃鎮陳津墜椎槌追鎚痛通塚栂掴槻佃漬柘辻蔦綴鍔椿潰坪壷嬬紬爪吊釣鶴亭低停偵剃貞呈堤定帝底庭廷弟悌抵挺提梯汀碇禎程締艇訂諦蹄逓"
	L"邸鄭釘鼎泥摘擢敵滴的笛適鏑溺哲徹撤轍迭鉄典填天展店添纏甜貼転顛点伝殿澱田電兎吐堵塗妬屠徒斗杜渡登菟賭途都鍍砥砺努度土奴怒倒党冬凍刀唐塔塘套宕島嶋悼投搭東桃梼棟盗淘湯涛灯燈当痘祷等答筒糖統到"
	L"董蕩藤討謄豆踏逃透鐙陶頭騰闘働動同堂導憧撞洞瞳童胴萄道銅峠鴇匿得徳涜特督禿篤毒独読栃橡凸突椴届鳶苫寅酉瀞噸屯惇敦沌豚遁頓呑曇鈍奈那内乍凪薙謎灘捺鍋楢馴縄畷南楠軟難汝二尼弐迩匂賑肉虹廿日乳入"
	L"如尿韮任妊忍認濡禰祢寧葱猫熱年念捻撚燃粘乃廼之埜嚢悩濃納能脳膿農覗蚤巴把播覇杷波派琶破婆罵芭馬俳廃拝排敗杯盃牌背肺輩配倍培媒梅楳煤狽買売賠陪這蝿秤矧萩伯剥博拍柏泊白箔粕舶薄迫曝漠爆縛莫駁麦"
	L"函箱硲箸肇筈櫨幡肌畑畠八鉢溌発醗髪伐罰抜筏閥鳩噺塙蛤隼伴判半反叛帆搬斑板氾汎版犯班畔繁般藩販範釆煩頒飯挽晩番盤磐蕃蛮匪卑否妃庇彼悲扉批披斐比泌疲皮碑秘緋罷肥被誹費避非飛樋簸備尾微枇毘琵眉美"
	L"鼻柊稗匹疋髭彦膝菱肘弼必畢筆逼桧姫媛紐百謬俵彪標氷漂瓢票表評豹廟描病秒苗錨鋲蒜蛭鰭品彬斌浜瀕貧賓頻敏瓶不付埠夫婦富冨布府怖扶敷斧普浮父符腐膚芙譜負賦赴阜附侮撫武舞葡蕪部封楓風葺蕗伏副復幅服"
	L"福腹複覆淵弗払沸仏物鮒分吻噴墳憤扮焚奮粉糞紛雰文聞丙併兵塀幣平弊柄並蔽閉陛米頁僻壁癖碧別瞥蔑箆偏変片篇編辺返遍便勉娩弁鞭保舗鋪圃捕歩甫補輔穂募墓慕戊暮母簿菩倣俸包呆報奉宝峰峯崩庖抱捧放方朋"
	L"法泡烹砲縫胞芳萌蓬蜂褒訪豊邦鋒飽鳳鵬乏亡傍剖坊妨帽忘忙房暴望某棒冒紡肪膨謀貌貿鉾防吠頬北僕卜墨撲朴牧睦穆釦勃没殆堀幌奔本翻凡盆摩磨魔麻埋妹昧枚毎哩槙幕膜枕鮪柾鱒桝亦俣又抹末沫迄侭繭麿万慢満"
	L"漫蔓味未魅巳箕岬密蜜湊蓑稔脈妙粍民眠務夢無牟矛霧鵡椋婿娘冥名命明盟迷銘鳴姪牝滅免棉綿緬面麺摸模茂妄孟毛猛盲網耗蒙儲木黙目杢勿餅尤戻籾貰問悶紋門匁也冶夜爺耶野弥矢厄役約薬訳躍靖柳薮鑓愉愈油癒"
	L"諭輸唯佑優勇友宥幽悠憂揖有柚湧涌猶猷由祐裕誘遊邑郵雄融夕予余与誉輿預傭幼妖容庸揚揺擁曜楊様洋溶熔用窯羊耀葉蓉要謡踊遥陽養慾抑欲沃浴翌翼淀羅螺裸来莱頼雷洛絡落酪乱卵嵐欄濫藍蘭覧利吏履李梨理璃"
	L"痢裏裡里離陸律率立葎掠略劉流溜琉留硫粒隆竜龍侶慮旅虜了亮僚両凌寮料梁涼猟療瞭稜糧良諒遼量陵領力緑倫厘林淋燐琳臨輪隣鱗麟瑠塁涙累類令伶例冷励嶺怜玲礼苓鈴隷零霊麗齢暦歴列劣烈裂廉恋憐漣煉簾練聯"
	L"蓮連錬呂魯櫓炉賂路露労婁廊弄朗楼榔浪漏牢狼篭老聾蝋郎六麓禄肋録論倭和話歪賄脇惑枠鷲亙亘鰐詫藁蕨椀湾碗腕・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・"
	L"弌丐丕个丱丶丼丿乂乖乘亂亅豫亊舒弍于亞亟亠亢亰亳亶从仍仄仆仂仗仞仭仟价伉佚估佛佝佗佇佶侈侏侘佻佩佰侑佯來侖儘俔俟俎俘俛俑俚俐俤俥倚倨倔倪倥倅伜俶倡倩倬俾俯們倆偃假會偕偐偈做偖偬偸傀傚傅傴傲"
	L"僉僊傳僂僖僞僥僭僣僮價僵儉儁儂儖儕儔儚儡儺儷儼儻儿兀兒兌兔兢竸兩兪兮冀冂囘册冉冏冑冓冕冖冤冦冢冩冪冫决冱冲冰况冽凅凉凛几處凩凭凰凵凾刄刋刔刎刧刪刮刳刹剏剄剋剌剞剔剪剴剩剳剿剽劍劔劒剱劈劑辨"
	L"辧劬劭劼劵勁勍勗勞勣勦飭勠勳勵勸勹匆匈甸匍匐匏匕匚匣匯匱匳匸區卆卅丗卉卍凖卞卩卮夘卻卷厂厖厠厦厥厮厰厶參簒雙叟曼燮叮叨叭叺吁吽呀听吭吼吮吶吩吝呎咏呵咎呟呱呷呰咒呻咀呶咄咐咆哇咢咸咥咬哄哈咨"
	L"咫哂咤咾咼哘哥哦唏唔哽哮哭哺哢唹啀啣啌售啜啅啖啗唸唳啝喙喀咯喊喟啻啾喘喞單啼喃喩喇喨嗚嗅嗟嗄嗜嗤嗔嘔嗷嘖嗾嗽嘛嗹噎噐營嘴嘶嘲嘸噫噤嘯噬噪嚆嚀嚊嚠嚔嚏嚥嚮嚶嚴囂嚼囁囃囀囈囎囑囓囗囮囹圀囿圄圉"
	L"圈國圍圓團圖嗇圜圦圷圸坎圻址坏坩埀垈坡坿垉垓垠垳垤垪垰埃埆埔埒埓堊埖埣堋堙堝塲堡塢塋塰毀塒堽塹墅墹墟墫墺壞墻墸墮壅壓壑壗壙壘壥壜壤壟壯壺壹壻壼壽夂夊夐夛梦夥夬夭夲夸夾竒奕奐奎奚奘奢奠奧奬奩"
	L"奸妁妝佞侫妣妲姆姨姜妍姙姚娥娟娑娜娉娚婀婬婉娵娶婢婪媚媼媾嫋嫂媽嫣嫗嫦嫩嫖嫺嫻嬌嬋嬖嬲嫐嬪嬶嬾孃孅孀孑孕孚孛孥孩孰孳孵學斈孺宀它宦宸寃寇寉寔寐寤實寢寞寥寫寰寶寳尅將專對尓尠尢尨尸尹屁屆屎屓"
	L"屐屏孱屬屮乢屶屹岌岑岔妛岫岻岶岼岷峅岾峇峙峩峽峺峭嶌峪崋崕崗嵜崟崛崑崔崢崚崙崘嵌嵒嵎嵋嵬嵳嵶嶇嶄嶂嶢嶝嶬嶮嶽嶐嶷嶼巉巍巓巒巖巛巫已巵帋帚帙帑帛帶帷幄幃幀幎幗幔幟幢幤幇幵并幺麼广庠廁廂廈廐廏"
	L"廖廣廝廚廛廢廡廨廩廬廱廳廰廴廸廾弃弉彝彜弋弑弖弩弭弸彁彈彌彎弯彑彖彗彙彡彭彳彷徃徂彿徊很徑徇從徙徘徠徨徭徼忖忻忤忸忱忝悳忿怡恠怙怐怩怎怱怛怕怫怦怏怺恚恁恪恷恟恊恆恍恣恃恤恂恬恫恙悁悍惧悃悚"
	L"悄悛悖悗悒悧悋惡悸惠惓悴忰悽惆悵惘慍愕愆惶惷愀惴惺愃愡惻惱愍愎慇愾愨愧慊愿愼愬愴愽慂慄慳慷慘慙慚慫慴慯慥慱慟慝慓慵憙憖憇憬憔憚憊憑憫憮懌懊應懷懈懃懆憺懋罹懍懦懣懶懺懴懿懽懼懾戀戈戉戍戌戔戛"
	L"戞戡截戮戰戲戳扁扎扞扣扛扠扨扼抂抉找抒抓抖拔抃抔拗拑抻拏拿拆擔拈拜拌拊拂拇抛拉挌拮拱挧挂挈拯拵捐挾捍搜捏掖掎掀掫捶掣掏掉掟掵捫捩掾揩揀揆揣揉插揶揄搖搴搆搓搦搶攝搗搨搏摧摯摶摎攪撕撓撥撩撈撼"
	L"據擒擅擇撻擘擂擱擧舉擠擡抬擣擯攬擶擴擲擺攀擽攘攜攅攤攣攫攴攵攷收攸畋效敖敕敍敘敞敝敲數斂斃變斛斟斫斷旃旆旁旄旌旒旛旙无旡旱杲昊昃旻杳昵昶昴昜晏晄晉晁晞晝晤晧晨晟晢晰暃暈暎暉暄暘暝曁暹曉暾暼"
	L"曄暸曖曚曠昿曦曩曰曵曷朏朖朞朦朧霸朮朿朶杁朸朷杆杞杠杙杣杤枉杰枩杼杪枌枋枦枡枅枷柯枴柬枳柩枸柤柞柝柢柮枹柎柆柧檜栞框栩桀桍栲桎梳栫桙档桷桿梟梏梭梔條梛梃檮梹桴梵梠梺椏梍桾椁棊椈棘椢椦棡椌棍"
	L"棔棧棕椶椒椄棗棣椥棹棠棯椨椪椚椣椡棆楹楷楜楸楫楔楾楮椹楴椽楙椰楡楞楝榁楪榲榮槐榿槁槓榾槎寨槊槝榻槃榧樮榑榠榜榕榴槞槨樂樛槿權槹槲槧樅榱樞槭樔槫樊樒櫁樣樓橄樌橲樶橸橇橢橙橦橈樸樢檐檍檠檄檢檣"
	L"檗蘗檻櫃櫂檸檳檬櫞櫑櫟檪櫚櫪櫻欅蘖櫺欒欖鬱欟欸欷盜欹飮歇歃歉歐歙歔歛歟歡歸歹歿殀殄殃殍殘殕殞殤殪殫殯殲殱殳殷殼毆毋毓毟毬毫毳毯麾氈氓气氛氤氣汞汕汢汪沂沍沚沁沛汾汨汳沒沐泄泱泓沽泗泅泝沮沱沾"
	L"沺泛泯泙泪洟衍洶洫洽洸洙洵洳洒洌浣涓浤浚浹浙涎涕濤涅淹渕渊涵淇淦涸淆淬淞淌淨淒淅淺淙淤淕淪淮渭湮渮渙湲湟渾渣湫渫湶湍渟湃渺湎渤滿渝游溂溪溘滉溷滓溽溯滄溲滔滕溏溥滂溟潁漑灌滬滸滾漿滲漱滯漲滌"
	L"漾漓滷澆潺潸澁澀潯潛濳潭澂潼潘澎澑濂潦澳澣澡澤澹濆澪濟濕濬濔濘濱濮濛瀉瀋濺瀑瀁瀏濾瀛瀚潴瀝瀘瀟瀰瀾瀲灑灣炙炒炯烱炬炸炳炮烟烋烝烙焉烽焜焙煥煕熈煦煢煌煖煬熏燻熄熕熨熬燗熹熾燒燉燔燎燠燬燧燵燼"
	L"燹燿爍爐爛爨爭爬爰爲爻爼爿牀牆牋牘牴牾犂犁犇犒犖犢犧犹犲狃狆狄狎狒狢狠狡狹狷倏猗猊猜猖猝猴猯猩猥猾獎獏默獗獪獨獰獸獵獻獺珈玳珎玻珀珥珮珞璢琅瑯琥珸琲琺瑕琿瑟瑙瑁瑜瑩瑰瑣瑪瑶瑾璋璞璧瓊瓏瓔珱"
	L"瓠瓣瓧瓩瓮瓲瓰瓱瓸瓷甄甃甅甌甎甍甕甓甞甦甬甼畄畍畊畉畛畆畚畩畤畧畫畭畸當疆疇畴疊疉疂疔疚疝疥疣痂疳痃疵疽疸疼疱痍痊痒痙痣痞痾痿痼瘁痰痺痲痳瘋瘍瘉瘟瘧瘠瘡瘢瘤瘴瘰瘻癇癈癆癜癘癡癢癨癩癪癧癬癰"
	L"癲癶癸發皀皃皈皋皎皖皓皙皚皰皴皸皹皺盂盍盖盒盞盡盥盧盪蘯盻眈眇眄眩眤眞眥眦眛眷眸睇睚睨睫睛睥睿睾睹瞎瞋瞑瞠瞞瞰瞶瞹瞿瞼瞽瞻矇矍矗矚矜矣矮矼砌砒礦砠礪硅碎硴碆硼碚碌碣碵碪碯磑磆磋磔碾碼磅磊磬"
	L"磧磚磽磴礇礒礑礙礬礫祀祠祗祟祚祕祓祺祿禊禝禧齋禪禮禳禹禺秉秕秧秬秡秣稈稍稘稙稠稟禀稱稻稾稷穃穗穉穡穢穩龝穰穹穽窈窗窕窘窖窩竈窰窶竅竄窿邃竇竊竍竏竕竓站竚竝竡竢竦竭竰笂笏笊笆笳笘笙笞笵笨笶筐"
	L"筺笄筍笋筌筅筵筥筴筧筰筱筬筮箝箘箟箍箜箚箋箒箏筝箙篋篁篌篏箴篆篝篩簑簔篦篥籠簀簇簓篳篷簗簍篶簣簧簪簟簷簫簽籌籃籔籏籀籐籘籟籤籖籥籬籵粃粐粤粭粢粫粡粨粳粲粱粮粹粽糀糅糂糘糒糜糢鬻糯糲糴糶糺紆"
	L"紂紜紕紊絅絋紮紲紿紵絆絳絖絎絲絨絮絏絣經綉絛綏絽綛綺綮綣綵緇綽綫總綢綯緜綸綟綰緘緝緤緞緻緲緡縅縊縣縡縒縱縟縉縋縢繆繦縻縵縹繃縷縲縺繧繝繖繞繙繚繹繪繩繼繻纃緕繽辮繿纈纉續纒纐纓纔纖纎纛纜缸缺"
	L"罅罌罍罎罐网罕罔罘罟罠罨罩罧罸羂羆羃羈羇羌羔羞羝羚羣羯羲羹羮羶羸譱翅翆翊翕翔翡翦翩翳翹飜耆耄耋耒耘耙耜耡耨耿耻聊聆聒聘聚聟聢聨聳聲聰聶聹聽聿肄肆肅肛肓肚肭冐肬胛胥胙胝胄胚胖脉胯胱脛脩脣脯腋"
	L"隋腆脾腓腑胼腱腮腥腦腴膃膈膊膀膂膠膕膤膣腟膓膩膰膵膾膸膽臀臂膺臉臍臑臙臘臈臚臟臠臧臺臻臾舁舂舅與舊舍舐舖舩舫舸舳艀艙艘艝艚艟艤艢艨艪艫舮艱艷艸艾芍芒芫芟芻芬苡苣苟苒苴苳苺莓范苻苹苞茆苜茉苙"
	L"茵茴茖茲茱荀茹荐荅茯茫茗茘莅莚莪莟莢莖茣莎莇莊荼莵荳荵莠莉莨菴萓菫菎菽萃菘萋菁菷萇菠菲萍萢萠莽萸蔆菻葭萪萼蕚蒄葷葫蒭葮蒂葩葆萬葯葹萵蓊葢蒹蒿蒟蓙蓍蒻蓚蓐蓁蓆蓖蒡蔡蓿蓴蔗蔘蔬蔟蔕蔔蓼蕀蕣蕘蕈"
	L"蕁蘂蕋蕕薀薤薈薑薊薨蕭薔薛藪薇薜蕷蕾薐藉薺藏薹藐藕藝藥藜藹蘊蘓蘋藾藺蘆蘢蘚蘰蘿虍乕虔號虧虱蚓蚣蚩蚪蚋蚌蚶蚯蛄蛆蚰蛉蠣蚫蛔蛞蛩蛬蛟蛛蛯蜒蜆蜈蜀蜃蛻蜑蜉蜍蛹蜊蜴蜿蜷蜻蜥蜩蜚蝠蝟蝸蝌蝎蝴蝗蝨蝮蝙"
	L"蝓蝣蝪蠅螢螟螂螯蟋螽蟀蟐雖螫蟄螳蟇蟆螻蟯蟲蟠蠏蠍蟾蟶蟷蠎蟒蠑蠖蠕蠢蠡蠱蠶蠹蠧蠻衄衂衒衙衞衢衫袁衾袞衵衽袵衲袂袗袒袮袙袢袍袤袰袿袱裃裄裔裘裙裝裹褂裼裴裨裲褄褌褊褓襃褞褥褪褫襁襄褻褶褸襌褝襠襞"
	L"襦襤襭襪襯襴襷襾覃覈覊覓覘覡覩覦覬覯覲覺覽覿觀觚觜觝觧觴觸訃訖訐訌訛訝訥訶詁詛詒詆詈詼詭詬詢誅誂誄誨誡誑誥誦誚誣諄諍諂諚諫諳諧諤諱謔諠諢諷諞諛謌謇謚諡謖謐謗謠謳鞫謦謫謾謨譁譌譏譎證譖譛譚譫"
	L"譟譬譯譴譽讀讌讎讒讓讖讙讚谺豁谿豈豌豎豐豕豢豬豸豺貂貉貅貊貍貎貔豼貘戝貭貪貽貲貳貮貶賈賁賤賣賚賽賺賻贄贅贊贇贏贍贐齎贓賍贔贖赧赭赱赳趁趙跂趾趺跏跚跖跌跛跋跪跫跟跣跼踈踉跿踝踞踐踟蹂踵踰踴蹊"
	L"蹇蹉蹌蹐蹈蹙蹤蹠踪蹣蹕蹶蹲蹼躁躇躅躄躋躊躓躑躔躙躪躡躬躰軆躱躾軅軈軋軛軣軼軻軫軾輊輅輕輒輙輓輜輟輛輌輦輳輻輹轅轂輾轌轉轆轎轗轜轢轣轤辜辟辣辭辯辷迚迥迢迪迯邇迴逅迹迺逑逕逡逍逞逖逋逧逶逵逹迸"
	L"遏遐遑遒逎遉逾遖遘遞遨遯遶隨遲邂遽邁邀邊邉邏邨邯邱邵郢郤扈郛鄂鄒鄙鄲鄰酊酖酘酣酥酩酳酲醋醉醂醢醫醯醪醵醴醺釀釁釉釋釐釖釟釡釛釼釵釶鈞釿鈔鈬鈕鈑鉞鉗鉅鉉鉤鉈銕鈿鉋鉐銜銖銓銛鉚鋏銹銷鋩錏鋺鍄錮"
	L"錙錢錚錣錺錵錻鍜鍠鍼鍮鍖鎰鎬鎭鎔鎹鏖鏗鏨鏥鏘鏃鏝鏐鏈鏤鐚鐔鐓鐃鐇鐐鐶鐫鐵鐡鐺鑁鑒鑄鑛鑠鑢鑞鑪鈩鑰鑵鑷鑽鑚鑼鑾钁鑿閂閇閊閔閖閘閙閠閨閧閭閼閻閹閾闊濶闃闍闌闕闔闖關闡闥闢阡阨阮阯陂陌陏陋陷陜陞"
	L"陝陟陦陲陬隍隘隕隗險隧隱隲隰隴隶隸隹雎雋雉雍襍雜霍雕雹霄霆霈霓霎霑霏霖霙霤霪霰霹霽霾靄靆靈靂靉靜靠靤靦靨勒靫靱靹鞅靼鞁靺鞆鞋鞏鞐鞜鞨鞦鞣鞳鞴韃韆韈韋韜韭齏韲竟韶韵頏頌頸頤頡頷頽顆顏顋顫顯顰"
	L"顱顴顳颪颯颱颶飄飃飆飩飫餃餉餒餔餘餡餝餞餤餠餬餮餽餾饂饉饅饐饋饑饒饌饕馗馘馥馭馮馼駟駛駝駘駑駭駮駱駲駻駸騁騏騅駢騙騫騷驅驂驀驃騾驕驍驛驗驟驢驥驤驩驫驪骭骰骼髀髏髑髓體髞髟髢髣髦髯髫髮髴髱髷"
	L"髻鬆鬘鬚鬟鬢鬣鬥鬧鬨鬩鬪鬮鬯鬲魄魃魏魍魎魑魘魴鮓鮃鮑鮖鮗鮟鮠鮨鮴鯀鯊鮹鯆鯏鯑鯒鯣鯢鯤鯔鯡鰺鯲鯱鯰鰕鰔鰉鰓鰌鰆鰈鰒鰊鰄鰮鰛鰥鰤鰡鰰鱇鰲鱆鰾鱚鱠鱧鱶鱸鳧鳬鳰鴉鴈鳫鴃鴆鴪鴦鶯鴣鴟鵄鴕鴒鵁鴿鴾鵆鵈"
	L"鵝鵞鵤鵑鵐鵙鵲鶉鶇鶫鵯鵺鶚鶤鶩鶲鷄鷁鶻鶸鶺鷆鷏鷂鷙鷓鷸鷦鷭鷯鷽鸚鸛鸞鹵鹹鹽麁麈麋麌麒麕麑麝麥麩麸麪麭靡黌黎黏黐黔黜點黝黠黥黨黯黴黶黷黹黻黼黽鼇鼈皷鼕鼡鼬鼾齊齒齔齣齟齠齡齦齧齬齪齷齲齶龕龜龠"
	L"堯槇遙瑤凜熙・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・";
