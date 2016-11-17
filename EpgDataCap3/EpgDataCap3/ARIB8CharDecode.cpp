#include "StdAfx.h"
#define COLOR_DEF_H_IMPLEMENT_TABLE
#define ARIB8CHAR_DECODE_H_IMPLEMENT_TABLE
#include "ARIB8CharDecode.h"

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

	m_strDecode = "";
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

	m_strDecode = "";
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

BOOL CARIB8CharDecode::PSISI( const BYTE* pbSrc, DWORD dwSrcSize, string* strDec )
{
	if( pbSrc == NULL || dwSrcSize == 0 || strDec == NULL){
		return FALSE;
	}
	InitPSISI();
	DWORD dwReadSize = 0;

	//���͈͂̔͌������e���ŏȗ����ꕄ����ɂ���Ă͐�byte�I�[�o�[����������̂őΏ�
	vector<BYTE> bSrcList;
	bSrcList.reserve(dwSrcSize + 8);
	bSrcList.assign(pbSrc, pbSrc + dwSrcSize);
	//�e���̖���while�𐡎~�߂���ԕ����܂���
	static const BYTE bMargin[8] = {0,0,0x20,0,0x4F,0x43,0x9B,0};
	bSrcList.insert(bSrcList.end(), bMargin, bMargin + 8);

	BOOL bRet = Analyze(bSrcList.data(), dwSrcSize, &dwReadSize );
	*strDec = std::move(m_strDecode);
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
		m_strDecode = "";
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

BOOL CARIB8CharDecode::Analyze( const BYTE* pbSrc, DWORD dwSrcSize, DWORD* pdwReadSize )
{
	if( pbSrc == NULL || dwSrcSize == 0 || pdwReadSize == NULL){
		return FALSE;
	}
	BOOL bRet = TRUE;
	DWORD dwReadSize = 0;

	while( dwReadSize < dwSrcSize ){
		DWORD dwReadBuff = 0;
		//1�o�C�g�ڃ`�F�b�N
		if( pbSrc[dwReadSize] <= 0x20 ){
			//C0����R�[�h
			bRet = C0( pbSrc+dwReadSize, &dwReadBuff );
			dwReadSize += dwReadBuff;
			if( bRet == FALSE ){
				return FALSE;
			}else if( bRet == 2 ){
				bRet = TRUE;
				break;
			}
		}else if( pbSrc[dwReadSize] > 0x20 && pbSrc[dwReadSize] < 0x7F ){
			//GL�����̈�
			if( GL( pbSrc+dwReadSize, &dwReadBuff ) == FALSE ){
				return FALSE;
			}
			dwReadSize += dwReadBuff;
		}else if( pbSrc[dwReadSize] >= 0x7F && pbSrc[dwReadSize] <= 0xA0 ){
			//C1����R�[�h
			bRet = C1( pbSrc+dwReadSize, &dwReadBuff );
			dwReadSize += dwReadBuff;
			if( bRet == FALSE ){
				return FALSE;
			}else if( bRet == 2 ){
				bRet = TRUE;
				break;
			}
		}else if( pbSrc[dwReadSize] > 0xA0 && pbSrc[dwReadSize] < 0xFF ){
			//GR�����̈�
			if( GR( pbSrc+dwReadSize, &dwReadBuff ) == FALSE ){
				return FALSE;
			}
			dwReadSize += dwReadBuff;
		}
		if( dwReadBuff == 0 ){
			return FALSE;
		}
	}

	*pdwReadSize = dwReadSize;
	return bRet;
}

BOOL CARIB8CharDecode::C0( const BYTE* pbSrc, DWORD* pdwReadSize )
{
	if( pbSrc == NULL || pdwReadSize == NULL ){
		return FALSE;
	}

	DWORD dwReadSize = 0;
	DWORD dwReadBuff = 0;

	BOOL bRet = TRUE;

	switch(pbSrc[0]){
	case 0x20:
		//SP ��
		//�󔒂͕����T�C�Y�̉e������
		if( IsSmallCharMode() == FALSE ){
			m_strDecode += "�@";
		}else{
			m_strDecode += ' ';
		}
		dwReadSize = 1;
		break;
	case 0x0D:
		//APR ���s
		m_strDecode += "\r\n";
		dwReadSize = 1;
		break;
	case 0x0E:
		//LS1 GL��G1�Z�b�g
		m_GL = &m_G1;
		dwReadSize = 1;
		break;
	case 0x0F:
		//LS0 GL��G0�Z�b�g
		m_GL = &m_G0;
		dwReadSize = 1;
		break;
	case 0x19:
		//SS2 �V���O���V�t�g
		if( SS2( pbSrc+1, &dwReadBuff ) == FALSE ){
			return FALSE;
		}
		dwReadSize = 1+dwReadBuff;
		break;
	case 0x1D:
		//SS3 �V���O���V�t�g
		if( SS3( pbSrc+1, &dwReadBuff ) == FALSE ){
			return FALSE;
		}
		dwReadSize = 1+dwReadBuff;
		break;
	case 0x1B:
		//ESC �G�X�P�[�v�V�[�P���X
		if( ESC( pbSrc+1, &dwReadBuff ) == FALSE ){
			return FALSE;
		}
		dwReadSize = 1+dwReadBuff;
		break;
	default:
		//���T�|�[�g�̐���R�[�h
		if( pbSrc[0] == 0x16 ){
			//PAPF
			dwReadSize = 2;
		}else if( pbSrc[0] == 0x1C ){
			//APS
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
			//APB�AAPF�AAPD�AAPU
			dwReadSize = 1;
		}
		break;
	}

	*pdwReadSize = dwReadSize;

	return bRet;
}

BOOL CARIB8CharDecode::C1( const BYTE* pbSrc, DWORD* pdwReadSize )
{
	if( pbSrc == NULL || pdwReadSize == NULL ){
		return FALSE;
	}
	DWORD dwReadSize = 0;
	DWORD dwReadBuff = 0;

	BOOL bRet = TRUE;

	CheckModify();

	switch(pbSrc[0]){
	case 0x89:
		//MSZ ���p�w��
		m_emStrSize = STR_MEDIUM;
		dwReadSize = 1;
		break;
	case 0x8A:
		//NSZ �S�p�w��
		m_emStrSize = STR_NORMAL;
		dwReadSize = 1;
		break;
	case 0x80:
		//BKF ������
		m_bCharColorIndex = (m_bDefPalette<<4) | 0x00;
		dwReadSize = 1;
		break;
	case 0x81:
		//RDF ������
		m_bCharColorIndex = (m_bDefPalette<<4) | 0x01;
		dwReadSize = 1;
		break;
	case 0x82:
		//GRF ������
		m_bCharColorIndex = (m_bDefPalette<<4) | 0x02;
		dwReadSize = 1;
		break;
	case 0x83:
		//YLF ������
		m_bCharColorIndex = (m_bDefPalette<<4) | 0x03;
		dwReadSize = 1;
		break;
	case 0x84:
		//BLF ������
		m_bCharColorIndex = (m_bDefPalette<<4) | 0x04;
		dwReadSize = 1;
		break;
	case 0x85:
		//MGF �����}�[���^
		m_bCharColorIndex = (m_bDefPalette<<4) | 0x05;
		dwReadSize = 1;
		break;
	case 0x86:
		//CNF �����V�A��
		m_bCharColorIndex = (m_bDefPalette<<4) | 0x06;
		dwReadSize = 1;
		break;
	case 0x87:
		//WHF ������
		m_bCharColorIndex = (m_bDefPalette<<4) | 0x07;
		dwReadSize = 1;
		break;
	case 0x88:
		//SSZ ���^�T�C�Y
		m_emStrSize = STR_SMALL;
		dwReadSize = 1;
		break;
	case 0x8B:
		//SZX �w��T�C�Y
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
		//COL �F�w��
		if( pbSrc[1] == 0x20 ){
			dwReadSize = 3;
			m_bDefPalette = pbSrc[2]&0x0F;
		}else{
			switch(pbSrc[1]&0xF0){
				case 0x40:
					m_bCharColorIndex = pbSrc[1]&0x0F;
					break;
				case 0x50:
					m_bBackColorIndex = pbSrc[1]&0x0F;
					break;
				case 0x60:
					//���T�|�[�g
					break;
				case 0x70:
					//���T�|�[�g
					break;
				default:
					break;
			}
			dwReadSize = 2;
		}
		break;
	case 0x91:
		//FLC �t���b�V���O����
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
		//POL �p�^�[���ɐ�
		//���T�|�[�g
		dwReadSize = 2;
		break;
	case 0x94:
		//WMM �������݃��[�h�ύX
		//���T�|�[�g
		dwReadSize = 2;
		break;
	case 0x95:
		//MACRO �}�N����`
		//���T�|�[�g
		{
		DWORD dwCount = 0;
		do{
			dwCount++;
		}while( pbSrc[dwCount] != 0x4F );
		dwReadSize = dwCount;
		}
		break;
	case 0x97:
		//HLC �͂ݐ���
		//���T�|�[�g
		dwReadSize = 2;
		break;
	case 0x98:
		//RPC �����J��Ԃ�
		//���T�|�[�g
		dwReadSize = 2;
		break;
	case 0x99:
		//SPL �A���_�[���C�� ���U�C�N�̏I��
		m_bBold = FALSE;
		bRet = 2;
		dwReadSize = 1;
		break;
	case 0x9A:
		//STL �A���_�[���C�� ���U�C�N�̊J�n
		m_bBold = TRUE;
		dwReadSize = 1;
		break;
	case 0x9D:
		//TIME ���Ԑ���
		CheckModify();
		if( pbSrc[1] == 0x20 ){
			m_dwWaitTime = pbSrc[2]-0x40;
			dwReadSize = 3;
		}else{
			DWORD dwCount = 0;
			do{
				dwCount++;
			}while( pbSrc[dwCount] != 0x43 && pbSrc[dwCount] != 0x40 && pbSrc[dwCount] != 0x41 && pbSrc[dwCount] != 0x42 );
			dwReadSize = dwCount;
		}
		break;
	case 0x9B:
		//CSI �R���g���[���V�[�P���X
		if( CSI( pbSrc, &dwReadBuff ) == FALSE ){
			return FALSE;
		}
		dwReadSize = dwReadBuff;
		break;
	default:
		//���T�|�[�g�̐���R�[�h
		dwReadSize = 1;
		break;
	}

	*pdwReadSize = dwReadSize;

	return bRet;
}

BOOL CARIB8CharDecode::GL( const BYTE* pbSrc, DWORD* pdwReadSize )
{
	if( pbSrc == NULL || pdwReadSize == NULL ){
		return FALSE;
	}

	DWORD dwReadSize = 0;
	if( m_GL->iMode == MF_MODE_G ){
		//�����R�[�h
		switch( m_GL->iMF ){
			case MF_JISX_KANA:
				//JISX X0201��0x7F�܂ł�ASCII�Ɠ���
			case MF_ASCII:
			case MF_PROP_ASCII:
				{
				if( IsSmallCharMode() == FALSE ){
					//�S�p�Ȃ̂Ńe�[�u������SJIS�R�[�h�擾
					m_strDecode += AsciiTable[pbSrc[0]-0x21];
				}else{
					//���p�Ȃ̂ł��̂܂ܓ����
					m_strDecode += pbSrc[0];
				}
				dwReadSize = 1;
				}
				break;
			case MF_HIRA:
			case MF_PROP_HIRA:
				{
				//���p�Ђ炪��
				//�e�[�u������SJIS�R�[�h�擾
				m_strDecode += HiraTable[pbSrc[0]-0x21];
				dwReadSize = 1;
				}
				break;
			case MF_KANA:
			case MF_PROP_KANA:
				{
				//���p�J�^�J�i
				//�e�[�u������SJIS�R�[�h�擾
				m_strDecode += KanaTable[pbSrc[0]-0x21];
				dwReadSize = 1;
				}
				break;
			case MF_MACRO:
				{
				DWORD dwTemp=0;
				//�}�N��
				//PSI/SI�ł͖��T�|�[�g
				switch(pbSrc[0]){
					case 0x60:
						Analyze(DefaultMacro0, sizeof(DefaultMacro0),&dwTemp);
						break;
					case 0x61:
						Analyze(DefaultMacro1, sizeof(DefaultMacro1),&dwTemp);
						break;
					case 0x62:
						Analyze(DefaultMacro2, sizeof(DefaultMacro2),&dwTemp);
						break;
					case 0x63:
						Analyze(DefaultMacro3, sizeof(DefaultMacro3),&dwTemp);
						break;
					case 0x64:
						Analyze(DefaultMacro4, sizeof(DefaultMacro4),&dwTemp);
						break;
					case 0x65:
						Analyze(DefaultMacro5, sizeof(DefaultMacro5),&dwTemp);
						break;
					case 0x66:
						Analyze(DefaultMacro6, sizeof(DefaultMacro6),&dwTemp);
						break;
					case 0x67:
						Analyze(DefaultMacro7, sizeof(DefaultMacro7),&dwTemp);
						break;
					case 0x68:
						Analyze(DefaultMacro8, sizeof(DefaultMacro8),&dwTemp);
						break;
					case 0x69:
						Analyze(DefaultMacro9, sizeof(DefaultMacro9),&dwTemp);
						break;
					case 0x6A:
						Analyze(DefaultMacroA, sizeof(DefaultMacroA),&dwTemp);
						break;
					case 0x6B:
						Analyze(DefaultMacroB, sizeof(DefaultMacroB),&dwTemp);
						break;
					case 0x6C:
						Analyze(DefaultMacroC, sizeof(DefaultMacroC),&dwTemp);
						break;
					case 0x6D:
						Analyze(DefaultMacroD, sizeof(DefaultMacroD),&dwTemp);
						break;
					case 0x6E:
						Analyze(DefaultMacroE, sizeof(DefaultMacroE),&dwTemp);
						break;
					case 0x6F:
						Analyze(DefaultMacroF, sizeof(DefaultMacroF),&dwTemp);
						break;
					default:
						break;
				}
				dwReadSize = 1;
				}
				break;
			case MF_KANJI:
			case MF_JIS_KANJI1:
			case MF_JIS_KANJI2:
			case MF_KIGOU:
				//����
				{
				if( ToSJIS( pbSrc[0], pbSrc[1] ) == FALSE ){
					ToCustomFont( pbSrc[0], pbSrc[1] );
				}
				dwReadSize = 2;
				}
				break;
			default:
				dwReadSize = m_GL->iByte;
				break;
		}
	}else{
		if( m_GL->iMF == MF_MACRO){
			DWORD dwTemp=0;
			//�}�N��
			//PSI/SI�ł͖��T�|�[�g
			switch(pbSrc[0]){
				case 0x60:
					Analyze(DefaultMacro0, sizeof(DefaultMacro0),&dwTemp);
					break;
				case 0x61:
					Analyze(DefaultMacro1, sizeof(DefaultMacro1),&dwTemp);
					break;
				case 0x62:
					Analyze(DefaultMacro2, sizeof(DefaultMacro2),&dwTemp);
					break;
				case 0x63:
					Analyze(DefaultMacro3, sizeof(DefaultMacro3),&dwTemp);
					break;
				case 0x64:
					Analyze(DefaultMacro4, sizeof(DefaultMacro4),&dwTemp);
					break;
				case 0x65:
					Analyze(DefaultMacro5, sizeof(DefaultMacro5),&dwTemp);
					break;
				case 0x66:
					Analyze(DefaultMacro6, sizeof(DefaultMacro6),&dwTemp);
					break;
				case 0x67:
					Analyze(DefaultMacro7, sizeof(DefaultMacro7),&dwTemp);
					break;
				case 0x68:
					Analyze(DefaultMacro8, sizeof(DefaultMacro8),&dwTemp);
					break;
				case 0x69:
					Analyze(DefaultMacro9, sizeof(DefaultMacro9),&dwTemp);
					break;
				case 0x6A:
					Analyze(DefaultMacroA, sizeof(DefaultMacroA),&dwTemp);
					break;
				case 0x6B:
					Analyze(DefaultMacroB, sizeof(DefaultMacroB),&dwTemp);
					break;
				case 0x6C:
					Analyze(DefaultMacroC, sizeof(DefaultMacroC),&dwTemp);
					break;
				case 0x6D:
					Analyze(DefaultMacroD, sizeof(DefaultMacroD),&dwTemp);
					break;
				case 0x6E:
					Analyze(DefaultMacroE, sizeof(DefaultMacroE),&dwTemp);
					break;
				case 0x6F:
					Analyze(DefaultMacroF, sizeof(DefaultMacroF),&dwTemp);
					break;
				default:
					break;
			}
			dwReadSize = 1;
		}else{
			dwReadSize = m_GL->iByte;
		}
	}

	*pdwReadSize = dwReadSize;

	return TRUE;
}


BOOL CARIB8CharDecode::GR( const BYTE* pbSrc, DWORD* pdwReadSize )
{
	if( pbSrc == NULL || pdwReadSize == NULL ){
		return FALSE;
	}

	DWORD dwReadSize = 0;
	if( m_GR->iMode == MF_MODE_G ){
		//�����R�[�h
		switch( m_GR->iMF ){
			case MF_ASCII:
			case MF_PROP_ASCII:
				{
				if( IsSmallCharMode() == FALSE ){
					//�S�p�Ȃ̂Ńe�[�u������SJIS�R�[�h�擾
					m_strDecode += AsciiTable[(pbSrc[0]&0x7F)-0x21];
				}else{
					//���p�Ȃ̂ł��̂܂ܓ����
					m_strDecode += pbSrc[0]&0x7F;
				}
				dwReadSize = 1;
				}
				break;
			case MF_HIRA:
			case MF_PROP_HIRA:
				{
				//���p�Ђ炪��
				//�e�[�u������SJIS�R�[�h�擾
				m_strDecode += HiraTable[(pbSrc[0]&0x7F)-0x21];
				dwReadSize = 1;
				}
				break;
			case MF_JISX_KANA:
				//JIX X0201��0x80�ȍ~�͔��p�J�i
			case MF_KANA:
			case MF_PROP_KANA:
				{
				//���p�J�^�J�i
				//�e�[�u������SJIS�R�[�h�擾
				m_strDecode += KanaTable[(pbSrc[0]&0x7F)-0x21];
				dwReadSize = 1;
				}
				break;
			case MF_MACRO:
				{
				DWORD dwTemp=0;
				//�}�N��
				//PSI/SI�ł͖��T�|�[�g
				switch(pbSrc[0]){
					case 0x60:
						Analyze(DefaultMacro0, sizeof(DefaultMacro0),&dwTemp);
						break;
					case 0x61:
						Analyze(DefaultMacro1, sizeof(DefaultMacro1),&dwTemp);
						break;
					case 0x62:
						Analyze(DefaultMacro2, sizeof(DefaultMacro2),&dwTemp);
						break;
					case 0x63:
						Analyze(DefaultMacro3, sizeof(DefaultMacro3),&dwTemp);
						break;
					case 0x64:
						Analyze(DefaultMacro4, sizeof(DefaultMacro4),&dwTemp);
						break;
					case 0x65:
						Analyze(DefaultMacro5, sizeof(DefaultMacro5),&dwTemp);
						break;
					case 0x66:
						Analyze(DefaultMacro6, sizeof(DefaultMacro6),&dwTemp);
						break;
					case 0x67:
						Analyze(DefaultMacro7, sizeof(DefaultMacro7),&dwTemp);
						break;
					case 0x68:
						Analyze(DefaultMacro8, sizeof(DefaultMacro8),&dwTemp);
						break;
					case 0x69:
						Analyze(DefaultMacro9, sizeof(DefaultMacro9),&dwTemp);
						break;
					case 0x6A:
						Analyze(DefaultMacroA, sizeof(DefaultMacroA),&dwTemp);
						break;
					case 0x6B:
						Analyze(DefaultMacroB, sizeof(DefaultMacroB),&dwTemp);
						break;
					case 0x6C:
						Analyze(DefaultMacroC, sizeof(DefaultMacroC),&dwTemp);
						break;
					case 0x6D:
						Analyze(DefaultMacroD, sizeof(DefaultMacroD),&dwTemp);
						break;
					case 0x6E:
						Analyze(DefaultMacroE, sizeof(DefaultMacroE),&dwTemp);
						break;
					case 0x6F:
						Analyze(DefaultMacroF, sizeof(DefaultMacroF),&dwTemp);
						break;
					default:
						break;
				}
				dwReadSize = 1;
				}
				break;
			case MF_KANJI:
			case MF_JIS_KANJI1:
			case MF_JIS_KANJI2:
			case MF_KIGOU:
				{
				//����
				if( ToSJIS( (pbSrc[0]&0x7F), (pbSrc[1]&0x7F) ) == FALSE ){
					ToCustomFont( (pbSrc[0]&0x7F), (pbSrc[1]&0x7F) );
				}
				dwReadSize = 2;
				}
				break;
			default:
				dwReadSize = m_GR->iByte;
				break;
		}
	}else{
		if( m_GR->iMF == MF_MACRO ){
			DWORD dwTemp=0;
			//�}�N��
			//PSI/SI�ł͖��T�|�[�g
			switch(pbSrc[0]){
				case 0x60:
					Analyze(DefaultMacro0, sizeof(DefaultMacro0),&dwTemp);
					break;
				case 0x61:
					Analyze(DefaultMacro1, sizeof(DefaultMacro1),&dwTemp);
					break;
				case 0x62:
					Analyze(DefaultMacro2, sizeof(DefaultMacro2),&dwTemp);
					break;
				case 0x63:
					Analyze(DefaultMacro3, sizeof(DefaultMacro3),&dwTemp);
					break;
				case 0x64:
					Analyze(DefaultMacro4, sizeof(DefaultMacro4),&dwTemp);
					break;
				case 0x65:
					Analyze(DefaultMacro5, sizeof(DefaultMacro5),&dwTemp);
					break;
				case 0x66:
					Analyze(DefaultMacro6, sizeof(DefaultMacro6),&dwTemp);
					break;
				case 0x67:
					Analyze(DefaultMacro7, sizeof(DefaultMacro7),&dwTemp);
					break;
				case 0x68:
					Analyze(DefaultMacro8, sizeof(DefaultMacro8),&dwTemp);
					break;
				case 0x69:
					Analyze(DefaultMacro9, sizeof(DefaultMacro9),&dwTemp);
					break;
				case 0x6A:
					Analyze(DefaultMacroA, sizeof(DefaultMacroA),&dwTemp);
					break;
				case 0x6B:
					Analyze(DefaultMacroB, sizeof(DefaultMacroB),&dwTemp);
					break;
				case 0x6C:
					Analyze(DefaultMacroC, sizeof(DefaultMacroC),&dwTemp);
					break;
				case 0x6D:
					Analyze(DefaultMacroD, sizeof(DefaultMacroD),&dwTemp);
					break;
				case 0x6E:
					Analyze(DefaultMacroE, sizeof(DefaultMacroE),&dwTemp);
					break;
				case 0x6F:
					Analyze(DefaultMacroF, sizeof(DefaultMacroF),&dwTemp);
					break;
				default:
					break;
			}
			dwReadSize = 1;
		}else{
			dwReadSize = m_GR->iByte;
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

	char cDec[3] = "";
	cDec[0] = ucFirst;
	cDec[1] = ucSecond;

	m_strDecode += cDec;

	return TRUE;
}

BOOL CARIB8CharDecode::ToCustomFont( const BYTE bFirst, const BYTE bSecond )
{
	unsigned short usSrc = (unsigned short)(bFirst<<8) | bSecond;

	if( 0x7521 <= usSrc && usSrc <= 0x757E ){
		m_strDecode +=GaijiTbl2[usSrc-0x7521].strChar;
	}else if( 0x7621 <= usSrc && usSrc <= 0x764B ){
		m_strDecode +=GaijiTbl2[usSrc-0x7621+94].strChar;
	}else if( 0x7A4D <= usSrc && usSrc <= 0x7A74 ){
		m_strDecode +=GaijiTable[usSrc-0x7A4D].strChar;
	}else if(0x7C21 <= usSrc && usSrc <= 0x7C7B ){
		m_strDecode +=GaijiTable[usSrc-0x7C21+40].strChar;
	}else if(0x7D21 <= usSrc && usSrc <= 0x7D5F ){
		m_strDecode +=GaijiTable[usSrc-0x7D21+131].strChar;
	}else if(0x7D6E <= usSrc && usSrc <= 0x7D6F ){
		m_strDecode +=GaijiTable[usSrc-0x7D6E+194].strChar;
	}else if(0x7E21 <= usSrc && usSrc <= 0x7E7D ){
		m_strDecode +=GaijiTable[usSrc-0x7E21+196].strChar;
	}else{
		m_strDecode +="�E";
		return FALSE;
	}

	return TRUE;
}


BOOL CARIB8CharDecode::ESC( const BYTE* pbSrc, DWORD* pdwReadSize )
{
	if( pbSrc == NULL ){
		return FALSE;
	}

	DWORD dwReadSize = 0;
	if( pbSrc[0] == 0x24 ){
		if( pbSrc[1] >= 0x28 && pbSrc[1] <= 0x2B ){
			if( pbSrc[2] == 0x20 ){
				//2�o�C�gDRCS
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
				//�����o�C�g�A���y����
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
				//2�o�C�gG�Z�b�g
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
			//2�o�C�gG�Z�b�g
			m_G0.iMF = pbSrc[1];
			m_G0.iMode = MF_MODE_G;
			m_G0.iByte = 2;
			dwReadSize = 2;
		}
	}else if( pbSrc[0] >= 0x28 && pbSrc[0] <= 0x2B ){
		if( pbSrc[1] == 0x20 ){
			//1�o�C�gDRCS
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
			//1�o�C�gG�Z�b�g
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
		//GL��G2�Z�b�g
		m_GL = &m_G2;
		dwReadSize = 1;
	}else if( pbSrc[0] == 0x6F ){
		//GL��G3�Z�b�g
		m_GL = &m_G3;
		dwReadSize = 1;
	}else if( pbSrc[0] == 0x7C ){
		//GR��G3�Z�b�g
		m_GR = &m_G3;
		dwReadSize = 1;
	}else if( pbSrc[0] == 0x7D ){
		//GR��G2�Z�b�g
		m_GR = &m_G2;
		dwReadSize = 1;
	}else if( pbSrc[0] == 0x7E ){
		//GR��G1�Z�b�g
		m_GR = &m_G1;
		dwReadSize = 1;
	}else{
		//���T�|�[�g
		dwReadSize = 1;
		*pdwReadSize = dwReadSize;
		return FALSE;
	}

	*pdwReadSize = dwReadSize;

	return TRUE;
}

BOOL CARIB8CharDecode::SS2( const BYTE* pbSrc, DWORD* pdwReadSize )
{
	MF_MODE* LastG;
	LastG = m_GL;
	//�ꎞ�I�ɓ���ւ���
	m_GL = &m_G2;
	//GL�����̈�
	BOOL bRet = GL( pbSrc, pdwReadSize );
	if( bRet != FALSE ){
		//���ɖ߂�
		m_GL = LastG;
		return bRet;
	}
	//���ɖ߂�
	m_GL = LastG;
	return TRUE;
}

BOOL CARIB8CharDecode::SS3( const BYTE* pbSrc, DWORD* pdwReadSize )
{
	MF_MODE* LastG;
	LastG = m_GL;
	//�ꎞ�I�ɓ���ւ���
	m_GL = &m_G3;
	//GL�����̈�
	BOOL bRet = GL( pbSrc, pdwReadSize );
	if( bRet != FALSE ){
		//���ɖ߂�
		m_GL = LastG;
		return bRet;
	}
	//���ɖ߂�
	m_GL = LastG;
	return TRUE;
}

BOOL CARIB8CharDecode::CSI( const BYTE* pbSrc, DWORD* pdwReadSize )
{
	if( pbSrc == NULL || pdwReadSize == NULL ){
		return FALSE;
	}
	DWORD dwReadSize = 0;

	do{
		dwReadSize++;
	}while( pbSrc[dwReadSize] != 0x20 );
	dwReadSize++;
	
	switch(pbSrc[dwReadSize]){
		case 0x53:
			//SWF
			{
				BOOL bCharMode = FALSE;
				WORD wParam = 0;
				for( DWORD i=1; i<dwReadSize; i++ ){
					if( pbSrc[i] == 0x20 ){
						if( bCharMode == FALSE ){
							m_wSWFMode = wParam;
						}else{
							//���T�|�[�g
						}
					}else if(pbSrc[i] == 0x3B){
						bCharMode = TRUE;
					}else{
						wParam = wParam*10+(pbSrc[i]&0x0F);
					}
				}
			}
			break;
		case 0x6E:
			//RCS
			{
				WORD wParam = 0;
				for( DWORD i=1; i<dwReadSize; i++ ){
					if( pbSrc[i] == 0x20 ){
						m_bRasterColorIndex = (BYTE)wParam;
					}else{
						wParam = wParam*10+(pbSrc[i]&0x0F);
					}
				}
			}
			break;
		case 0x61:
			//ACPS
			{
				BOOL bSeparate = FALSE;
				WORD wParam = 0;
				for( DWORD i=1; i<dwReadSize; i++ ){
					if( pbSrc[i] == 0x20 ){
						if( bSeparate == FALSE ){
							m_wPosX = wParam;
						}else{
							m_wPosY = wParam;
						}
					}else if(pbSrc[i] == 0x3B){
						bSeparate = TRUE;
						m_wPosX = wParam;
						wParam = 0;
					}else{
						wParam = wParam*10+(pbSrc[i]&0x0F);
					}
				}
			}
			break;
		case 0x56:
			//SDF
			{
				BOOL bSeparate = FALSE;
				WORD wParam = 0;
				for( DWORD i=1; i<dwReadSize; i++ ){
					if( pbSrc[i] == 0x20 ){
						if( bSeparate == FALSE ){
							m_wClientW = wParam;
						}else{
							m_wClientH = wParam;
						}
					}else if(pbSrc[i] == 0x3B){
						bSeparate = TRUE;
						m_wClientW = wParam;
						wParam = 0;
					}else{
						wParam = wParam*10+(pbSrc[i]&0x0F);
					}
				}
			}
			break;
		case 0x5F:
			//SDP
			{
				BOOL bSeparate = FALSE;
				WORD wParam = 0;
				for( DWORD i=1; i<dwReadSize; i++ ){
					if( pbSrc[i] == 0x20 ){
						if( bSeparate == FALSE ){
							m_wClientX = wParam;
						}else{
							m_wClientY = wParam;
						}
					}else if(pbSrc[i] == 0x3B){
						bSeparate = TRUE;
						m_wClientX = wParam;
						wParam = 0;
					}else{
						wParam = wParam*10+(pbSrc[i]&0x0F);
					}
				}
			}
			break;
		case 0x57:
			//SSM
			{
				BOOL bSeparate = FALSE;
				WORD wParam = 0;
				for( DWORD i=1; i<dwReadSize; i++ ){
					if( pbSrc[i] == 0x20 ){
						if( bSeparate == FALSE ){
							m_wCharW = wParam;
						}else{
							m_wCharH = wParam;
						}
					}else if(pbSrc[i] == 0x3B){
						bSeparate = TRUE;
						m_wCharW = wParam;
						wParam = 0;
					}else{
						wParam = wParam*10+(pbSrc[i]&0x0F);
					}
				}
			}
			break;
		case 0x58:
			//SHS
			{
				WORD wParam = 0;
				for( DWORD i=1; i<dwReadSize; i++ ){
					if( pbSrc[i] == 0x20 ){
						m_wCharHInterval = wParam;
					}else{
						wParam = wParam*10+(pbSrc[i]&0x0F);
					}
				}
			}
			break;
		case 0x59:
			//SVS
			{
				WORD wParam = 0;
				for( DWORD i=1; i<dwReadSize; i++ ){
					if( pbSrc[i] == 0x20 ){
						m_wCharVInterval = wParam;
					}else{
						wParam = wParam*10+(pbSrc[i]&0x0F);
					}
				}
			}
			break;
		case 0x42:
			//GSM
			//���T�|�[�g
			break;
		case 0x5D:
			//GAA
			//���T�|�[�g
			break;
		case 0x5E:
			//SRC
			//���T�|�[�g
			break;
		case 0x62:
			//TCC
			//���T�|�[�g
			break;
		case 0x65:
			//CFS
			//���T�|�[�g
			break;
		case 0x63:
			//ORN
			{
				BOOL bSeparate = FALSE;
				WORD wParam = 0;
				for( DWORD i=1; i<dwReadSize; i++ ){
					if( pbSrc[i] == 0x20 ){
						if( bSeparate == FALSE ){
							if( wParam == 0x02 ){
								m_bShadow = TRUE;
							}
						}
					}else if(pbSrc[i] == 0x3B){
						bSeparate = TRUE;
						if( wParam == 0x02 ){
							m_bShadow = TRUE;
						}
						wParam = 0;
					}else{
						wParam = wParam*10+(pbSrc[i]&0x0F);
					}
				}
			}
			break;
		case 0x64:
			//MDF
			{
				WORD wParam = 0;
				for( DWORD i=1; i<dwReadSize; i++ ){
					if( pbSrc[i] == 0x20 ){
						if( wParam == 0 ){
							m_bBold = FALSE;
							m_bItalic = FALSE;
						}else if( wParam == 1 ){
							m_bBold = TRUE;
						}else if( wParam == 2 ){
							m_bItalic = TRUE;
						}else if( wParam == 3 ){
							m_bBold = TRUE;
							m_bItalic = TRUE;
						}
					}else{
						wParam = wParam*10+(pbSrc[i]&0x0F);
					}
				}
			}
			break;
		case 0x66:
			//XCS
			//���T�|�[�g
			break;
		case 0x68:
			//PRA
			//���T�|�[�g
			break;
		case 0x54:
			//CCC
			//���T�|�[�g
			break;
		case 0x67:
			//SCR
			//���T�|�[�g
			break;
		case 0x69:
			//ACS
			//���T�|�[�g
			if (dwReadSize != 3 || pbSrc[dwReadSize-2] <= 0x31)
				break;
			while (pbSrc[++dwReadSize] != 0x9B)
				;
			dwReadSize += 3;
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
			m_strDecode = "";
		}else{
			CAPTION_DATA Item;
			CreateCaptionData(&Item);
			m_pCaptionList->push_back(Item);

			CAPTION_CHAR_DATA CharItem;
			CreateCaptionCharData(&CharItem);
			(*m_pCaptionList)[m_pCaptionList->size()-1].CharList.push_back(CharItem);
			m_strDecode = "";
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

