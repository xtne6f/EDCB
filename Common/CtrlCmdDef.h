#ifndef __CTRL_CMD_DEF_H__
#define __CTRL_CMD_DEF_H__

//�f�t�H���g�R�l�N�g�^�C���A�E�g
#define CONNECT_TIMEOUT 15*1000

//�p�C�v��
#define CMD2_EPG_SRV_PIPE _T("\\\\.\\pipe\\EpgTimerSrvPipe")
#define CMD2_GUI_CTRL_PIPE _T("\\\\.\\pipe\\EpgTimerGUI_Ctrl_BonPipe_") //+�v���Z�XID
#define CMD2_VIEW_CTRL_PIPE _T("\\\\.\\pipe\\View_Ctrl_BonPipe_") //+�v���Z�XID
#define CMD2_TVTEST_CTRL_PIPE _T("\\\\.\\pipe\\TvTest_Ctrl_BonPipe_") //+�v���Z�XID

//�ڑ��ҋ@�p�C�x���g
#define CMD2_EPG_SRV_EVENT_WAIT_CONNECT _T("Global\\EpgTimerSrvConnect")
#define CMD2_GUI_CTRL_WAIT_CONNECT _T("Global\\EpgTimerGUI_Ctrl_BonConnect_") //+�v���Z�XID
#define CMD2_VIEW_CTRL_WAIT_CONNECT _T("Global\\View_Ctrl_BonConnect_") //+�v���Z�XID
#define CMD2_TVTEST_CTRL_WAIT_CONNECT _T("Global\\TvTest_Ctrl_BonConnect_") //+�v���Z�XID

//�R�}���h�o�[�W����
//#define CMD_VER 2	//�o�[�W�������ǉ��Ή��@�^��ݒ�ւ̕�����M�^��t�H���_�w��ǉ�
//#define CMD_VER 3	//���������ɓ���^��`�F�b�N�ǉ�
//#define CMD_VER 4	//�^��ςݏ��Ƀv���e�N�g�ǉ�
#define CMD_VER 5 //�\��t�@�C�����ǉ�

//�R�}���h
#define CMD2_EPG_SRV_ADDLOAD_RESERVE		1 //Program.txt�̒ǉ��ǂݍ��݁i�p�~�j
#define CMD2_EPG_SRV_RELOAD_EPG				2 //EPG�ēǂݍ���
#define CMD2_EPG_SRV_RELOAD_SETTING			3 //�ݒ�̍ēǂݍ���
#define CMD2_EPG_SRV_CLOSE					4 //�A�v���P�[�V�����̏I���iCreateProcess�ŕ��ʂɋN�������ꍇ�Ɏg�p�j
#define CMD2_EPG_SRV_REGIST_GUI				5 //GUI�A�v���P�[�V�����̃p�C�v���Ɛڑ��ҋ@�p�C�x���g����o�^�i�^�C�}�[GUI�p�̃R�}���h����Ԃ悤�ɂȂ�j
#define CMD2_EPG_SRV_UNREGIST_GUI			6 //GUI�A�v���P�[�V�����̃p�C�v���Ɛڑ��ҋ@�p�C�x���g���̓o�^������
#define CMD2_EPG_SRV_REGIST_GUI_TCP			7 //TCP�ڑ���GUI�A�v���P�[�V������IP�ƃ|�[�g��o�^�i�^�C�}�[GUI�p�̃R�}���h����Ԃ悤�ɂȂ�j
#define CMD2_EPG_SRV_UNREGIST_GUI_TCP		8 //TCP�ڑ���GUI�A�v���P�[�V������IP�ƃ|�[�g�̓o�^������
#define CMD2_EPG_SRV_ISREGIST_GUI_TCP		9 //TCP�ڑ���GUI�A�v���P�[�V������IP�ƃ|�[�g�̓o�^�󋵊m�F

#define CMD2_EPG_SRV_ENUM_RESERVE			1011 //�\��ꗗ�擾
#define CMD2_EPG_SRV_GET_RESERVE			1012 //�\����擾
#define CMD2_EPG_SRV_ADD_RESERVE			1013 //�\��ǉ�
#define CMD2_EPG_SRV_DEL_RESERVE			1014 //�\��폜
#define CMD2_EPG_SRV_CHG_RESERVE			1015 //�\��ύX
#define CMD2_EPG_SRV_ENUM_TUNER_RESERVE		1016 //�`���[�i�[���Ƃ̗\��ID�ꗗ�擾
#define CMD2_EPG_SRV_ENUM_RECINFO			1017 //�^��ςݏ��ꗗ�擾
#define CMD2_EPG_SRV_DEL_RECINFO			1018 //�^��ςݏ��폜

//�o�[�W�������ǉ��Ή���
#define CMD2_EPG_SRV_ENUM_RESERVE2			2011 //�\��ꗗ�擾
#define CMD2_EPG_SRV_GET_RESERVE2			2012 //�\����擾
#define CMD2_EPG_SRV_ADD_RESERVE2			2013 //�\��ǉ�
#define CMD2_EPG_SRV_CHG_RESERVE2			2015 //�\��ύX
#define CMD2_EPG_SRV_ENUM_RECINFO2			2017 //�^��ςݏ��ꗗ�擾
#define CMD2_EPG_SRV_CHG_PROTECT_RECINFO2	2019 //�^��ςݏ��̃v���e�N�g�ύX
#define CMD2_EPG_SRV_ENUM_RECINFO_BASIC2	2020 //�^��ςݏ��ꗗ�擾�iprogramInfo��errInfo�������j
#define CMD2_EPG_SRV_GET_RECINFO_LIST2		2022 //�^��ςݏ��ꗗ�擾�i�w��ID���X�g�j
#define CMD2_EPG_SRV_GET_RECINFO2			2024 //�^��ςݏ��擾
#define CMD2_EPG_SRV_ADDCHK_RESERVE2		2030 //�T�[�o�[�A�g�p�@�\��ǉ��ł��邩�̃`�F�b�N�i�߂�l 0:�ǉ��s�� 1:�ǉ��\ 2:�ǉ��\�����J�n���I�����d�Ȃ���̂��� 3:���łɓ�����������j
#define CMD2_EPG_SRV_GET_EPG_FILETIME2		2031 //�T�[�o�[�A�g�p�@EPG�f�[�^�t�@�C���̃^�C���X�^���v�擾
#define CMD2_EPG_SRV_GET_EPG_FILE2			2032 //�T�[�o�[�A�g�p�@EPG�f�[�^�t�@�C���擾
#define CMD2_EPG_SRV_SEARCH_PG2				2125 //�ԑg����
#define CMD2_EPG_SRV_SEARCH_PG_BYKEY2		2127 //�ԑg�����A�L�[���Ƃ̃C�x���g��S�ĕԂ�
#define CMD2_EPG_SRV_ENUM_AUTO_ADD2			2131 //�����\��o�^�̏����ꗗ�擾
#define CMD2_EPG_SRV_ADD_AUTO_ADD2			2132 //�����\��o�^�̏����ǉ�
#define CMD2_EPG_SRV_CHG_AUTO_ADD2			2134 //�����\��o�^�̏����ύX
#define CMD2_EPG_SRV_ENUM_MANU_ADD2			2141 //�v���O�����\�񎩓��o�^�̏����ꗗ�擾
#define CMD2_EPG_SRV_ADD_MANU_ADD2			2142 //�v���O�����\�񎩓��o�^�̏����ǉ�
#define CMD2_EPG_SRV_CHG_MANU_ADD2			2144 //�v���O�����\�񎩓��o�^�̏����ύX
#define CMD2_EPG_SRV_GET_STATUS_NOTIFY2		2200 //�T�[�o�[�̏��ύX�ʒm���擾�i�����O�|�[�����O�j

#define CMD2_EPG_SRV_ENUM_SERVICE			1021 //�ǂݍ��܂ꂽEPG�f�[�^�̃T�[�r�X�̈ꗗ�擾
#define CMD2_EPG_SRV_ENUM_PG_INFO			1022 //�T�[�r�X�w��Ŕԑg���ꗗ���擾����
#define CMD2_EPG_SRV_GET_PG_INFO			1023 //�ԑg���擾
#define CMD2_EPG_SRV_SEARCH_PG				1025 //�ԑg����
#define CMD2_EPG_SRV_ENUM_PG_ALL			1026 //�ԑg���ꗗ�擾
#define CMD2_EPG_SRV_ENUM_PG_ARC_INFO		1028 //�T�[�r�X�w��ŉߋ��ԑg���ꗗ���擾����
#define CMD2_EPG_SRV_ENUM_PG_ARC_ALL		1029 //�ߋ��ԑg���ꗗ�擾

#define CMD2_EPG_SRV_ENUM_AUTO_ADD			1031 //�����\��o�^�̏����ꗗ�擾
#define CMD2_EPG_SRV_ADD_AUTO_ADD			1032 //�����\��o�^�̏����ǉ�
#define CMD2_EPG_SRV_DEL_AUTO_ADD			1033 //�����\��o�^�̏����폜
#define CMD2_EPG_SRV_CHG_AUTO_ADD			1034 //�����\��o�^�̏����ύX

#define CMD2_EPG_SRV_ENUM_MANU_ADD			1041 //�v���O�����\�񎩓��o�^�̏����ꗗ�擾
#define CMD2_EPG_SRV_ADD_MANU_ADD			1042 //�v���O�����\�񎩓��o�^�̏����ǉ�
#define CMD2_EPG_SRV_DEL_MANU_ADD			1043 //�v���O�����\�񎩓��o�^�̏����폜
#define CMD2_EPG_SRV_CHG_MANU_ADD			1044 //�v���O�����\�񎩓��o�^�̏����ύX

#define CMD2_EPG_SRV_CHK_SUSPEND			1050 //�X�^���o�C�A�x�~�A�V���b�g�_�E�����s���Ă������̊m�F
#define CMD2_EPG_SRV_SUSPEND				1051 //�X�^���o�C�A�x�~�A�V���b�g�_�E���Ɉڍs����i1:�X�^���o�C 2:�x�~ 3:�V���b�g�_�E�� | 0x0100:���A��ċN���j
#define CMD2_EPG_SRV_REBOOT					1052 //PC�ċN�����s��
#define CMD2_EPG_SRV_EPG_CAP_NOW			1053 //10�b���EPG�f�[�^�̎擾���s��

#define CMD2_EPG_SRV_FILE_COPY				1060 //�w��t�@�C����]������
#define CMD2_EPG_SRV_FILE_COPY2				2060 //�w��t�@�C�����܂Ƃ߂ē]������
#define CMD2_EPG_SRV_ENUM_PLUGIN			1061 //PlugIn�t�@�C���̈ꗗ���擾����i1:ReName�A2:Write�j
#define CMD2_EPG_SRV_GET_CHG_CH_TVTEST		1062 //TVTest�̃`�����l���؂�ւ��p�̏����擾����
#define CMD2_EPG_SRV_PROFILE_UPDATE			1063 //�ݒ�t�@�C��(ini)�̍X�V��ʒm������

#define CMD2_EPG_SRV_NWTV_SET_CH			1070 //�l�b�g���[�N���[�h��EpgDataCap_Bon�̃`�����l����؂�ւ�
#define CMD2_EPG_SRV_NWTV_CLOSE				1071 //�l�b�g���[�N���[�h�ŋN������EpgDataCap_Bon���I��
#define CMD2_EPG_SRV_NWTV_MODE				1072 //�l�b�g���[�N���[�h�ŋN������Ƃ��̃��[�h�i1:UDP 2:TCP 3:UDP+TCP�j

#define CMD2_EPG_SRV_NWPLAY_OPEN			1080 //�X�g���[���z�M�p�t�@�C�����J��
#define CMD2_EPG_SRV_NWPLAY_CLOSE			1081 //�X�g���[���z�M�p�t�@�C�������
#define CMD2_EPG_SRV_NWPLAY_PLAY			1082 //�X�g���[���z�M�J�n
#define CMD2_EPG_SRV_NWPLAY_STOP			1083 //�X�g���[���z�M��~
#define CMD2_EPG_SRV_NWPLAY_GET_POS			1084 //�X�g���[���z�M�Ō��݂̑��M�ʒu�Ƒ��t�@�C���T�C�Y���擾����
#define CMD2_EPG_SRV_NWPLAY_SET_POS			1085 //�X�g���[���z�M�ő��M�ʒu���V�[�N����
#define CMD2_EPG_SRV_NWPLAY_SET_IP			1086 //�X�g���[���z�M�ő��M���ݒ肷��
#define CMD2_EPG_SRV_NWPLAY_TF_OPEN			1087 //�X�g���[���z�M�p�t�@�C�����^�C���V�t�g���[�h�ŊJ��

//�O���A�v���Đ��p
#define CMD2_EPG_SRV_GET_NETWORK_PATH		1299 //�^��t�@�C���̃l�b�g���[�N�p�X���擾

//�^�C�}�[GUI�iEpgTimer_Bon.exe�j�p
#define CMD2_TIMER_GUI_SHOW_DLG				101 //�_�C�A���O��O�ʂɕ\��
#define CMD2_TIMER_GUI_UPDATE_RESERVE		102 //�\��ꗗ�̏�񂪍X�V���ꂽ
#define CMD2_TIMER_GUI_UPDATE_EPGDATA		103 //EPG�f�[�^�̍ēǂݍ��݂���������
#define CMD2_TIMER_GUI_VIEW_EXECUTE			110 //View�A�v���iEpgDataCap_Bon.exe�j���N��
#define CMD2_TIMER_GUI_QUERY_SUSPEND		120 //�X�^���o�C�A�x�~�A�V���b�g�_�E���ɓ����Ă������̊m�F�����[�U�[�ɍs���i�����Ă����Ȃ�CMD_EPG_SRV_SUSPEND�𑗂�j
#define CMD2_TIMER_GUI_QUERY_REBOOT			121 //PC�ċN���ɓ����Ă������̊m�F�����[�U�[�ɍs���i�����Ă����Ȃ�CMD_EPG_SRV_REBOOT�𑗂�j
#define CMD2_TIMER_GUI_SRV_STATUS_CHG		130 //�T�[�o�[�̃X�e�[�^�X�ύX�ʒm�i1:�ʏ�A2:EPG�f�[�^�擾�J�n�A3:�\��^��J�n�j

//�o�[�W�������ǉ��Ή���
#define CMD2_TIMER_GUI_SRV_STATUS_NOTIFY2	1130 //�T�[�o�[�̏��ύX�ʒm

//View�A�v���iEpgDataCap_Bon.exe�j�p
#define CMD2_VIEW_APP_SET_BONDRIVER			201 //BonDriver�̐؂�ւ�
#define CMD2_VIEW_APP_GET_BONDRIVER			202 //�g�p����BonDriver�̃t�@�C�������擾
#define CMD2_VIEW_APP_SET_CH				205 //Space��Ch or OriginalNetworkID�ATSID�AServieID�Ń`�����l���؂�ւ�
#define CMD2_VIEW_APP_GET_DELAY				206 //�����g�̎��Ԃ�PC���Ԃ̌덷�擾
#define CMD2_VIEW_APP_GET_STATUS			207 //���݂̏�Ԃ��擾
#define CMD2_VIEW_APP_CLOSE					208 //�A�v���P�[�V�����̏I��
#define CMD2_VIEW_APP_SET_ID				1201 //���ʗpID�̐ݒ�
#define CMD2_VIEW_APP_GET_ID				1202 //���ʗpID�̎擾
#define CMD2_VIEW_APP_SET_STANDBY_REC		1203 //�\��^��p��GUI�L�[�v
#define CMD2_VIEW_APP_EXEC_VIEW_APP			1204 //View�{�^���o�^�A�v���̋N��
#define CMD2_VIEW_APP_CREATE_CTRL			1221 //�X�g���[������p�R���g���[���쐬
#define CMD2_VIEW_APP_DELETE_CTRL			1222 //�X�g���[������p�R���g���[���폜
#define CMD2_VIEW_APP_SET_CTRLMODE			1223 //�R���g���[���̓����ݒ�i�ΏۃT�[�r�X�A�X�N�����u���A�����Ώۃf�[�^�j
#define CMD2_VIEW_APP_REC_START_CTRL		1224 //�^�揈���J�n
#define CMD2_VIEW_APP_REC_STOP_CTRL			1225 //�^�揈����~
#define CMD2_VIEW_APP_REC_FILE_PATH			1226 //�^��t�@�C���p�X���擾
#define CMD2_VIEW_APP_REC_STOP_ALL			1227 //�����^����~
#define CMD2_VIEW_APP_REC_WRITE_SIZE		1228 //�t�@�C���o�͂����T�C�Y���擾
#define CMD2_VIEW_APP_EPGCAP_START			1241 //EPG�擾�J�n
#define CMD2_VIEW_APP_EPGCAP_STOP			1242 //EPG�擾��~
#define CMD2_VIEW_APP_SEARCH_EVENT			1251 //EPG���̌���
#define CMD2_VIEW_APP_GET_EVENT_PF			1252 //����or���̔ԑg�����擾����
//TVTest�A�g�̃X�g���[�~���O�z�M��p
#define CMD2_VIEW_APP_TT_SET_CTRL			1261 //�X�g���[�~���O�z�M����ID�̐ݒ�


//���o�[�W�����݊��R�}���h
#define CMD_EPG_SRV_GET_RESERVE_INFO	12 //�\����擾
#define CMD_EPG_SRV_ADD_RESERVE			13 //�\��ǉ�
#define CMD_EPG_SRV_DEL_RESERVE			14 //�\��폜
#define CMD_EPG_SRV_CHG_RESERVE			15 //�\��ύX
#define CMD_EPG_SRV_SEARCH_PG_FIRST		21 //�ԑg�����i�擪�j
#define CMD_EPG_SRV_SEARCH_PG_NEXT		22 //�ԑg�����̑���
#define CMD_EPG_SRV_ADD_AUTO_ADD		32 //�����\��o�^�̏����ǉ�
#define CMD_EPG_SRV_DEL_AUTO_ADD		33 //�����\��o�^�̏����폜
#define CMD_EPG_SRV_CHG_AUTO_ADD		34 //�����\��o�^�̏����ύX

#define OLD_CMD_SUCCESS			0 //����
#define OLD_CMD_ERR				1 //�ėp�G���[
#define OLD_CMD_NEXT			2 //Enum�R�}���h�p�A��������

#endif
