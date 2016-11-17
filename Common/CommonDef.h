#ifndef __COMMON_DEF_H__
#define __COMMON_DEF_H__

#include "ErrDef.h"
#include "StructDef.h"

#define SAVE_FOLDER L"\\EpgTimerBon"
#define EPG_SAVE_FOLDER L"\\EpgData"
#define LOGO_SAVE_FOLDER L"\\LogoData"
#define BON_DLL_FOLDER L"\\BonDriver"

#define RESERVE_TEXT_NAME L"Reserve.txt"
#define REC_INFO_TEXT_NAME L"RecInfo.txt"
#define REC_INFO2_TEXT_NAME L"RecInfo2.txt"
#define EPG_AUTO_ADD_TEXT_NAME L"EpgAutoAdd.txt"
#define MANUAL_AUTO_ADD_TEXT_NAME L"ManualAutoAdd.txt"

#define EPG_TIMER_SERVICE_EXE L"EpgTimerSrv.exe"

#define EPG_TIMER_BON_MUTEX L"Global\\EpgTimer_Bon2"
#define EPG_TIMER_BON_LITE_MUTEX L"Global\\EpgTimer_Bon2_Lite"
#define EPG_TIMER_BON_SRV_MUTEX L"Global\\EpgTimer_Bon_Service"
#define SERVICE_NAME L"EpgTimer Service"

#define RECMODE_ALL 0 //�S�T�[�r�X
#define RECMODE_SERVICE 1 //�w��T�[�r�X�̂�
#define RECMODE_ALL_NOB25 2 //�S�T�[�r�X�iB25�����Ȃ��j
#define RECMODE_SERVICE_NOB25 3 //�w��T�[�r�X�̂݁iB25�����Ȃ��j
#define RECMODE_VIEW 4 //����
#define RECMODE_NO 5 //����

#define RESERVE_EXECUTE 0 //���ʂɗ\����s
#define RESERVE_PILED_UP 1 //�d�Ȃ��Ď��s�ł��Ȃ��\�񂠂�
#define RESERVE_NO_EXECUTE 2 //�d�Ȃ��Ď��s�ł��Ȃ�

#define RECSERVICEMODE_DEF	0x00000000	//�f�t�H���g�ݒ�
#define RECSERVICEMODE_SET	0x00000001	//�ݒ�l�g�p
#define RECSERVICEMODE_CAP	0x00000010	//�����f�[�^�܂�
#define RECSERVICEMODE_DATA	0x00000020	//�f�[�^�J���[�Z���܂�

//�\��ǉ����
#define ADD_RESERVE_NORMAL		0x00	//�ʏ�
#define ADD_RESERVE_RELAY		0x01	//�C�x���g�����[�Œǉ�
#define ADD_RESERVE_NO_FIND		0x02	//6���ԒǏ]���[�h
#define ADD_RESERVE_CHG_PF		0x04	//�ŐVEPG�ŕύX�ς�(p/f�`�F�b�N)
#define ADD_RESERVE_CHG_PF2		0x08	//�ŐVEPG�ŕύX�ς�(�ʏ�`�F�b�N)
#define ADD_RESERVE_NO_EPG		0x10	//EPG�Ȃ��ŉ����ς�
#define ADD_RESERVE_UNKNOWN_END	0x20	//�I��������

//View�A�v���iEpgDataCap_Bon�j�̃X�e�[�^�X
#define VIEW_APP_ST_NORMAL				0 //�ʏ���
#define VIEW_APP_ST_ERR_BON				1 //BonDriver�̏������Ɏ��s
#define VIEW_APP_ST_REC					2 //�^����
#define VIEW_APP_ST_GET_EPG				3 //EPG�擾���
#define VIEW_APP_ST_ERR_CH_CHG			4 //�`�����l���؂�ւ����s���

#define REC_END_STATUS_NORMAL		1		//����I��
#define REC_END_STATUS_OPEN_ERR		2		//�`���[�i�[�̃I�[�v�����ł��Ȃ�����
#define REC_END_STATUS_ERR_END		3		//�^�撆�ɃG���[����������
#define REC_END_STATUS_NEXT_START_END	4	//���̗\��J�n�̂��ߏI��
#define REC_END_STATUS_START_ERR	5		//�J�n���Ԃ��߂��Ă���
#define REC_END_STATUS_CHG_TIME		6		//�J�n���Ԃ��ύX���ꂽ
#define REC_END_STATUS_NO_TUNER		7		//�`���[�i�[������Ȃ�����
#define REC_END_STATUS_NO_RECMODE	8		//��������������
#define REC_END_STATUS_NOT_FIND_PF	9		//p/f�ɔԑg���m�F�ł��Ȃ�����
#define REC_END_STATUS_NOT_FIND_6H	10		//6���Ԕԑg���m�F�ł��Ȃ�����
#define REC_END_STATUS_END_SUBREC	11		//�T�u�t�H���_�ւ̘^�悪��������
#define REC_END_STATUS_ERR_RECSTART 12		//�^��J�n�Ɏ��s����
#define REC_END_STATUS_NOT_START_HEAD 13	//�ꕔ�̂ݘ^�悳�ꂽ
#define REC_END_STATUS_ERR_CH_CHG	14		//�`�����l���؂�ւ��Ɏ��s����
#define REC_END_STATUS_ERR_END2		15		//�^�撆�ɃG���[����������(Write��exception)

//NotifyID
#define NOTIFY_UPDATE_EPGDATA		1		//EPG�f�[�^���X�V���ꂽ
#define NOTIFY_UPDATE_RESERVE_INFO	2		//�\���񂪍X�V���ꂽ
#define NOTIFY_UPDATE_REC_INFO	3			//�^�挋�ʏ�񂪍X�V���ꂽ
#define NOTIFY_UPDATE_AUTOADD_EPG	4		//EPG�����\��o�^��񂪍X�V���ꂽ
#define NOTIFY_UPDATE_AUTOADD_MANUAL	5	//�v���O���������\��o�^��񂪍X�V���ꂽ
#define NOTIFY_UPDATE_PROFILE		51		//�ݒ�t�@�C��(ini)���X�V���ꂽ
#define NOTIFY_UPDATE_SRV_STATUS	100		//Srv�̓���󋵂��ύX�iparam1:�X�e�[�^�X 0:�ʏ�A1:�^�撆�A2:EPG�擾���j
#define NOTIFY_UPDATE_PRE_REC_START	101		//�^�揀���J�n�iparam4:���O�p���b�Z�[�W�j
#define NOTIFY_UPDATE_REC_START		102		//�^��J�n�iparam4:���O�p���b�Z�[�W�j
#define NOTIFY_UPDATE_REC_END		103		//�^��I���iparam4:���O�p���b�Z�[�W�j
#define NOTIFY_UPDATE_REC_TUIJYU	104		//�^�撆�ɒǏ]�������iparam4:���O�p���b�Z�[�W�j
#define NOTIFY_UPDATE_CHG_TUIJYU	105		//EPG�����\��o�^�ŒǏ]�������iparam4:���O�p���b�Z�[�W�j
#define NOTIFY_UPDATE_PRE_EPGCAP_START	106	//EPG�擾�����J�n
#define NOTIFY_UPDATE_EPGCAP_START	107		//EPG�擾�J�n
#define NOTIFY_UPDATE_EPGCAP_END	108		//EPG�擾�I��

#endif