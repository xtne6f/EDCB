#pragma once

// MFC�Ŏg�����p
/*#ifdef _DEBUG
#undef new
#endif
#include <string>
#include <vector>
#ifdef _DEBUG
#define new DEBUG_NEW
#endif
*/
#include <string>
#include <vector>
using std::string;
using std::vector;

#include "ColorDef.h"

//���������W��
//G�Z�b�g
#define MF_JIS_KANJI1 0x39 //JIS�݊�����1��
#define MF_JIS_KANJI2 0x3A //JIS�݊�����2��
#define MF_KIGOU 0x3B //�ǉ��L��
#define MF_ASCII 0x4A //�p��
#define MF_HIRA  0x30 //������
#define MF_KANA  0x31 //�Љ���
#define MF_KANJI 0x42 //����
#define MF_MOSAIC_A 0x32 //���U�C�NA
#define MF_MOSAIC_B 0x33 //���U�C�NB
#define MF_MOSAIC_C 0x34 //���U�C�NC
#define MF_MOSAIC_D 0x35 //���U�C�ND
#define MF_PROP_ASCII 0x36 //�v���|�[�V���i���p��
#define MF_PROP_HIRA  0x37 //�v���|�[�V���i��������
#define MF_PROP_KANA  0x38 //�v���|�[�V���i���Љ���
#define MF_JISX_KANA 0x49 //JIX X0201�Љ���
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
#define MF_MACRO 0x70 //�}�N��

//�����W���̕���
#define MF_MODE_G 1 //G�Z�b�g
#define MF_MODE_DRCS 2 //DRCS
#define MF_MODE_OTHER 3 //���̑�

#ifdef ARIB8CHAR_DECODE_H_IMPLEMENT_TABLE

static char AsciiTable[][3]={
	"�I","�h","��","��","��","��","�f",
	"�i","�j","��","�{","�C","�|","�D","�^",
	"�O","�P","�Q","�R","�S","�T","�U","�V",
	"�W","�X","�F","�G","��","��","��","�H",
	"��","�`","�a","�b","�c","�d","�e","�f",
	"�g","�h","�i","�j","�k","�l","�m","�n",
	"�o","�p","�q","�r","�s","�t","�u","�v",
	"�w","�x","�y","�m","��","�n","�O","�Q",
	"�e","��","��","��","��","��","��","��",
	"��","��","��","��","��","��","��","��",
	"��","��","��","��","��","��","��","��",
	"��","��","��","�o","�b","�p","�P"
};
static char HiraTable[][3]={
	"��","��","��","��","��","��","��",
	"��","��","��","��","��","��","��","��",
	"��","��","��","��","��","��","��","��",
	"��","��","��","��","��","��","��","��",
	"��","��","��","��","��","��","��","��",
	"��","��","��","��","��","��","��","��",
	"��","��","��","��","��","��","��","��",
	"��","��","��","��","��","��","��","��",
	"��","��","��","��","��","��","��","��",
	"��","��","��","��","��","��","��","��",
	"��","��","��","��","�@","�@","�@","�T",
	"�U","�[","�B","�u","�v","�A","�E"
};
static char KanaTable[][3]={
	"�@","�A","�B","�C","�D","�E","�F",
	"�G","�H","�I","�J","�K","�L","�M","�N",
	"�O","�P","�Q","�R","�S","�T","�U","�V",
	"�W","�X","�Y","�Z","�[","�\","�]","�^",
	"�_","�`","�a","�b","�c","�d","�e","�f",
	"�g","�h","�i","�j","�k","�l","�m","�n",
	"�o","�p","�q","�r","�s","�t","�u","�v",
	"�w","�x","�y","�z","�{","�|","�}","�~",
	"��","��","��","��","��","��","��","��",
	"��","��","��","��","��","��","��","��",
	"��","��","��","��","��","��","��","�R",
	"�S","�[","�B","�u","�v","�A","�E"
};

typedef struct _GAIJI_TABLE{
	unsigned short usARIB8;
	const char* strChar;
} GAIJI_TABLE;

static GAIJI_TABLE GaijiTable[]={
	{0x7A4D, "10."},
	{0x7A4E, "11."},
	{0x7A4F, "12."},
	{0x7A50, "[HV]"}, //90��48�_
	{0x7A51, "[SD]"},
	{0x7A52, "[�o]"},
	{0x7A53, "[�v]"},
	{0x7A54, "[MV]"},
	{0x7A55, "[��]"},
	{0x7A56, "[��]"},
	{0x7A57, "[�o]"},
	{0x7A58, "[�f]"},
	{0x7A59, "[�r]"},
	{0x7A5A, "[��]"},
	{0x7A5B, "[��]"},
	{0x7A5C, "[��]"},
	{0x7A5D, "[SS]"},
	{0x7A5E, "[�a]"},
	{0x7A5F, "[�m]"},//
	{0x7A60, "��"},//90��64�_
	{0x7A61, "��"},
	{0x7A62, "[�V]"},
	{0x7A63, "[��]"},
	{0x7A64, "[�f]"},
	{0x7A65, "[��]"},
	{0x7A66, "[��]"},
	{0x7A67, "[�E]"},
	{0x7A68, "[�O]"},
	{0x7A69, "[��]"},
	{0x7A6A, "[��]"},
	{0x7A6B, "[�V]"},
	{0x7A6C, "[��]"},
	{0x7A6D, "[�I]"},
	{0x7A6E, "[��]"},
	{0x7A6F, "[��]"},
	{0x7A70, "[��]"},//90��80�_
	{0x7A71, "[��]"},
	{0x7A72, "[PPV]"},
	{0x7A73, "(��)"},
	{0x7A74, "�ق�"},
	//91��͔�΂�
	{0x7C21, "��"},//92��1�_
	{0x7C22, "��"},
	{0x7C23, "��"},
	{0x7C24, "��"},
	{0x7C25, "��"},
	{0x7C26, "��"},
	{0x7C27, "�N"},
	{0x7C28, "��"},
	{0x7C29, "��"},
	{0x7C2A, "�~"},
	{0x7C2B, "m^2"},
	{0x7C2C, "m^3"},
	{0x7C2D, "cm"},
	{0x7C2E, "cm^2"},
	{0x7C2F, "cm^3"},
	{0x7C30, "�O."},//92��16�_
	{0x7C31, "�P."},
	{0x7C32, "�Q."},
	{0x7C33, "�R."},
	{0x7C34, "�S."},
	{0x7C35, "�T."},
	{0x7C36, "�U."},
	{0x7C37, "�V."},
	{0x7C38, "�W."},
	{0x7C39, "�X."},
	{0x7C3A, "��"},
	{0x7C3B, "��"},
	{0x7C3C, "��"},
	{0x7C3D, "��"},
	{0x7C3E, "�O"},
	{0x7C3F, "�V"},
	{0x7C40, "�O,"},//92��32�_
	{0x7C41, "�P,"},
	{0x7C42, "�Q,"},
	{0x7C43, "�R,"},
	{0x7C44, "�S,"},
	{0x7C45, "�T,"},
	{0x7C46, "�U,"},
	{0x7C47, "�V,"},
	{0x7C48, "�W,"},
	{0x7C49, "�X,"},
	{0x7C4A, "[��]"},
	{0x7C4B, "[��]"},
	{0x7C4C, "[�L]"},
	{0x7C4D, "[��]"},
	{0x7C4E, "[��]"},
	{0x7C4F, "(��)"},
	{0x7C50, "��"},//92��48�_
	{0x7C51, "��"},
	{0x7C52, "�y"},
	{0x7C53, "�z"},
	{0x7C54, "��"},
	{0x7C55, "^2"},
	{0x7C56, "^3"},
	{0x7C57, "(CD)"},
	{0x7C58, "(vn)"},
	{0x7C59, "(ob)"},
	{0x7C5A, "(cb)"},
	{0x7C5B, "(ce"},
	{0x7C5C, "mb)"},
	{0x7C5D, "(hp)"},
	{0x7C5E, "(br)"},
	{0x7C5F, "(��)"},
	{0x7C60, "(��)"},//92��64�_
	{0x7C61, "(ms)"},
	{0x7C62, "(��)"},
	{0x7C63, "(bs)"},
	{0x7C64, "(��)"},
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
	{0x7C70, "(sy"},//92��80�_
	{0x7C71, "n)"},
	{0x7C72, "(or"},
	{0x7C73, "g)"},
	{0x7C74, "(pe"},
	{0x7C75, "r)"},
	{0x7C76, "(�q)"},
	{0x7C77, "(�b)"},
	{0x7C78, "(�)"},
	{0x7C79, "�c�i"},
	{0x7C7A, "[��]"},
	{0x7C7B, "Fax"},
	{0x7D21, "(��)"},//93��1�_
	{0x7D22, "(��)"},
	{0x7D23, "(��)"},
	{0x7D24, "(��)"},
	{0x7D25, "(��)"},
	{0x7D26, "(�y)"},
	{0x7D27, "(��)"},
	{0x7D28, "(�j)"},
	{0x7D29, "��"},
	{0x7D2A, "��"},
	{0x7D2B, "��"},
	{0x7D2C, "�~"},
	{0x7D2D, "No."},
	{0x7D2E, "Tel"},
	{0x7D2F, "(��)"},
	{0x7D30, "()()"},//93��16�_
	{0x7D31, "[�{]"},
	{0x7D32, "[�O]"},
	{0x7D33, "[��]"},
	{0x7D34, "[��]"},
	{0x7D35, "[�_]"},
	{0x7D36, "[��]"},
	{0x7D37, "[��]"},
	{0x7D38, "[��]"},
	{0x7D39, "[�s]"},
	{0x7D3A, "[�r]"},
	{0x7D3B, "[��]"},
	{0x7D3C, "[��]"},
	{0x7D3D, "[��]"},
	{0x7D3E, "[��]"},
	{0x7D3F, "[�O]"},
	{0x7D40, "[�V]"},//93��32�_
	{0x7D41, "[��]"},
	{0x7D42, "[��]"},
	{0x7D43, "[�E]"},
	{0x7D44, "[�w]"},
	{0x7D45, "[��]"},
	{0x7D46, "[��]"},
	{0x7D47, "l"},
	{0x7D48, "kg"},
	{0x7D49, "Hz"},
	{0x7D4A, "ha"},
	{0x7D4B, "km"},
	{0x7D4C, "km^2"},
	{0x7D4D, "hPa"},
	{0x7D4E, "�E"},
	{0x7D4F, "�E"},
	{0x7D50, "1/2"},//93��48�_
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
	{0x7D6E, "!!"},//93��78�_
	{0x7D6F, "!?"},
	{0x7E21, "�T"},//94��1�_
	{0x7E22, "�U"},
	{0x7E23, "�V"},
	{0x7E24, "�W"},
	{0x7E25, "�X"},
	{0x7E26, "�Y"},
	{0x7E27, "�Z"},
	{0x7E28, "�["},
	{0x7E29, "�\"},
	{0x7E2A, "�]"},
	{0x7E2B, "XI"},
	{0x7E2C, "XII"},
	{0x7E2D, "�P"},
	{0x7E2E, "�Q"},
	{0x7E2F, "�R"},
	{0x7E30, "�S"},//94��16�_
	{0x7E31, "(�P)"},
	{0x7E32, "(�Q)"},
	{0x7E33, "(�R)"},
	{0x7E34, "(�S)"},
	{0x7E35, "(�T)"},
	{0x7E36, "(�U)"},
	{0x7E37, "(�V)"},
	{0x7E38, "(�W)"},
	{0x7E39, "(�X)"},
	{0x7E3A, "(10)"},
	{0x7E3B, "(11)"},
	{0x7E3C, "(12)"},
	{0x7E3D, "(21)"},
	{0x7E3E, "(22)"},
	{0x7E3F, "(23)"},
	{0x7E40, "(24)"},//94��32�_
	{0x7E41, "(�`)"},
	{0x7E42, "(�a)"},
	{0x7E43, "(�b)"},
	{0x7E44, "(�c)"},
	{0x7E45, "(�d)"},
	{0x7E46, "(�e)"},
	{0x7E47, "(�f)"},
	{0x7E48, "(�g)"},
	{0x7E49, "(�h)"},
	{0x7E4A, "(�i)"},
	{0x7E4B, "(�j)"},
	{0x7E4C, "(�k)"},
	{0x7E4D, "(�l)"},
	{0x7E4E, "(�m)"},
	{0x7E4F, "(�n)"},
	{0x7E50, "(�o)"},//94��48�_
	{0x7E51, "(�p)"},
	{0x7E52, "(�q)"},
	{0x7E53, "(�r)"},
	{0x7E54, "(�s)"},
	{0x7E55, "(�t)"},
	{0x7E56, "(�u)"},
	{0x7E57, "(�v)"},
	{0x7E58, "(�w)"},
	{0x7E59, "(�x)"},
	{0x7E5A, "(�y)"},
	{0x7E5B, "(25)"},
	{0x7E5C, "(26)"},
	{0x7E5D, "(27)"},
	{0x7E5E, "(28)"},
	{0x7E5F, "(29)"},
	{0x7E60, "(30)"},//94��64�_
	{0x7E61, "�@"},
	{0x7E62, "�A"},
	{0x7E63, "�B"},
	{0x7E64, "�C"},
	{0x7E65, "�D"},
	{0x7E66, "�E"},
	{0x7E67, "�F"},
	{0x7E68, "�G"},
	{0x7E69, "�H"},
	{0x7E6A, "�I"},
	{0x7E6B, "�J"},
	{0x7E6C, "�K"},
	{0x7E6D, "�L"},
	{0x7E6E, "�M"},
	{0x7E6F, "�N"},
	{0x7E70, "�O"},//94��80�_
	{0x7E71, "(�P)"},
	{0x7E72, "(�Q)"},
	{0x7E73, "(�R)"},
	{0x7E74, "(�S)"},
	{0x7E75, "(�T)"},
	{0x7E76, "(�U)"},
	{0x7E77, "(�V)"},
	{0x7E78, "(�W)"},
	{0x7E79, "(�X)"},
	{0x7E7A, "(10)"},
	{0x7E7B, "(11)"},
	{0x7E7C, "(12)"},
	{0x7E7D, "(31)"}
};

static GAIJI_TABLE GaijiTbl2[]={
	{0x7521, "��"},
	{0x7522, "��"},
	{0x7523, "��"},
	{0x7524, "�f"},
	{0x7525, "�q"},
	{0x7526, "�a"},
	{0x7527, "�s"},
	{0x7528, "��"},
	{0x7529, "��"},
	{0x752A, "��"}, //10
	{0x752B, "��"},
	{0x752C, "��"},
	{0x752D, "��"},
	{0x752E, "��"},
	{0x752F, "�g"},
	{0x7530, "��"},
	{0x7531, "��"},
	{0x7532, "��"},
	{0x7533, "��"},
	{0x7534, "��"}, //20
	{0x7535, "��"},
	{0x7536, "��"},
	{0x7537, "��"},
	{0x7538, "��"},
	{0x7539, "��"},
	{0x753A, "��"},
	{0x753B, "��"},
	{0x753C, "��"},
	{0x753D, "��"},
	{0x753E, "��"}, //30
	{0x753F, "��"},
	{0x7540, "��"},
	{0x7541, "��"},
	{0x7542, "��"},
	{0x7543, "��"},
	{0x7544, "�g"},
	{0x7545, "��"},
	{0x7546, "��"},
	{0x7547, "�b"},
	{0x7548, "��"}, //40
	{0x7549, "��"},
	{0x754A, "��"},
	{0x754B, "��"},
	{0x754C, "��"},
	{0x754D, "�f"},
	{0x754E, "��"},
	{0x754F, "��"},
	{0x7550, "��"},
	{0x7551, "��"},
	{0x7552, "��"}, //50
	{0x7553, "��"},
	{0x7554, "��"},
	{0x7555, "��"},
	{0x7556, "��"},
	{0x7557, "��"},
	{0x7558, "��"},
	{0x7559, "��"},
	{0x755A, "��"},
	{0x755B, "�C"},
	{0x755C, "��"}, //60
	{0x755D, "��"},
	{0x755E, "�C"},
	{0x755F, "��"},
	{0x7560, "��"},
	{0x7561, "��"},
	{0x7562, "��"},
	{0x7563, "��"},
	{0x7564, "��"},
	{0x7565, "�W"},
	{0x7566, "�Y"}, //70
	{0x7567, "��"},
	{0x7568, "��"},
	{0x7569, "��"},
	{0x756A, "�a"},
	{0x756B, "�b"},
	{0x756C, "��"},
	{0x756D, "��"},
	{0x756E, "��"},
	{0x756F, "�g"},
	{0x7570, "�h"}, //80
	{0x7571, "��"},
	{0x7572, "��"},
	{0x7573, "��"},
	{0x7574, "��"},
	{0x7575, "�`"},
	{0x7576, "��"},
	{0x7577, "��"},
	{0x7578, "��"},
	{0x7579, "��"},
	{0x757A, "��"}, //90
	{0x757B, "�_"},
	{0x757C, "�X"},
	{0x757D, "��"},
	{0x757E, "��"},
	{0x7621, "��"},
	{0x7622, "��"},
	{0x7623, "��"},
	{0x7624, "��"},
	{0x7625, "��"},
	{0x7626, "��"}, //100
	{0x7627, "��"},
	{0x7628, "��"},
	{0x7629, "��"},
	{0x762A, "��"},
	{0x762B, "��"},
	{0x762C, "��"},
	{0x762D, "��"},
	{0x762E, "�`"},
	{0x762F, "�H"},
	{0x7630, "��"}, //110
	{0x7631, "��"},
	{0x7632, "�I"},
	{0x7633, "��"},
	{0x7634, "��"},
	{0x7635, "��"},
	{0x7636, "�p"},
	{0x7637, "��"},
	{0x7638, "��"},
	{0x7639, "��"},
	{0x763A, "��"}, //120
	{0x763B, "��"},
	{0x763C, "��"},
	{0x763D, "�A"},
	{0x763E, "��"},
	{0x763F, "��"},
	{0x7640, "�_"},
	{0x7641, "��"},
	{0x7642, "�^"},
	{0x7643, "��"},
	{0x7644, "��"}, //130
	{0x7645, "�L"},
	{0x7646, "��"},
	{0x7647, "��"},
	{0x7648, "�I"},
	{0x7649, "��"},
	{0x764A, "��"},
	{0x764B, "��"}
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

//�����T�C�Y
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
	//=�I�y���[�^�[�̏���
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
	//=�I�y���[�^�[�̏���
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

	//PSI/SI��z�肵��SJIS�ւ̕ϊ�
	BOOL PSISI( const BYTE* pbSrc, DWORD dwSrcSize, string* strDec );
	//������z�肵��SJIS�ւ̕ϊ�
	BOOL Caption( const BYTE* pbSrc, DWORD dwSrcSize, vector<CAPTION_DATA>* pCaptionList );

protected:
	typedef struct _MF_MODE{
		int iMF; //���������W��
		int iMode; //�����W���̕���
		int iByte; //�ǂݍ��݃o�C�g��
		//=�I�y���[�^�[�̏���
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

	//�f�R�[�h����������
	string m_strDecode;
	//�����T�C�Y
	STRING_SIZE m_emStrSize;

	//CLUT�̃C���f�b�N�X
	BYTE m_bCharColorIndex;
	BYTE m_bBackColorIndex;
	BYTE m_bRasterColorIndex;
	BYTE m_bDefPalette;

	BOOL m_bUnderLine;
	BOOL m_bShadow;
	BOOL m_bBold;
	BOOL m_bItalic;
	BYTE m_bFlushMode;

	//�\������
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

	//���䕄��
	BOOL C0( const BYTE* pbSrc, DWORD* pdwReadSize );
	BOOL C1( const BYTE* pbSrc, DWORD* pdwReadSize );
	BOOL GL( const BYTE* pbSrc, DWORD* pdwReadSize );
	BOOL GR( const BYTE* pbSrc, DWORD* pdwReadSize );
	//�V���O���V�t�g
	BOOL SS2( const BYTE* pbSrc, DWORD* pdwReadSize );
	BOOL SS3( const BYTE* pbSrc, DWORD* pdwReadSize );
	//�G�X�P�[�v�V�[�P���X
	BOOL ESC( const BYTE* pbSrc, DWORD* pdwReadSize );
	//�Q�o�C�g�����ϊ�
	BOOL ToSJIS( const BYTE bFirst, const BYTE bSecond );
	BOOL ToCustomFont( const BYTE bFirst, const BYTE bSecond );

	BOOL CSI( const BYTE* pbSrc, DWORD* pdwReadSize );

};
