#ifndef __STRUCT_DEF_H__
#define __STRUCT_DEF_H__

//�]���t�@�C���f�[�^
typedef struct _FILE_DATA{
	wstring Name;				//�t�@�C����
	vector<BYTE> Data;			//�t�@�C���f�[�^
} FILE_DATA;

//�^��t�H���_���
typedef struct _REC_FILE_SET_INFO{
	wstring recFolder;			//�^��t�H���_
	wstring writePlugIn;		//�o��PlugIn
	wstring recNamePlugIn;		//�t�@�C�����ϊ�PlugIn�̎g�p
	wstring recFileName;		//�t�@�C�����ʑΉ� �^��J�n�������ɓ����Ŏg�p�B�\����Ƃ��Ă͕K�v�Ȃ�
}REC_FILE_SET_INFO;

//�^��ݒ���
typedef struct _REC_SETTING_DATA{
	BYTE recMode;				//�^�惂�[�h
	BYTE priority;				//�D��x
	BYTE tuijyuuFlag;			//�C�x���g�����[�Ǐ]���邩�ǂ���
	DWORD serviceMode;			//�����Ώۃf�[�^���[�h
	BYTE pittariFlag;			//�҂�����H�^��
	wstring batFilePath;		//�^���BAT�t�@�C���p�X
	vector<REC_FILE_SET_INFO> recFolderList;		//�^��t�H���_�p�X
	BYTE suspendMode;			//�x�~���[�h
	BYTE rebootFlag;			//�^���ċN������
	BYTE useMargineFlag;		//�^��}�[�W�����ʎw��
	INT startMargine;			//�^��J�n���̃}�[�W��
	INT endMargine;				//�^��I�����̃}�[�W��
	BYTE continueRecFlag;		//�㑱����T�[�r�X���A����t�@�C���Ř^��
	BYTE partialRecFlag;		//����CH�ɕ�����M�T�[�r�X������ꍇ�A�����^�悷�邩�ǂ���
	DWORD tunerID;				//�����I�Ɏg�pTuner���Œ�
	//CMD_VER 2�ȍ~
	vector<REC_FILE_SET_INFO> partialRecFolder;	//������M�T�[�r�X�^��̃t�H���_
	_REC_SETTING_DATA(void){
		recMode = 1;
		priority = 1;
		tuijyuuFlag = 1;
		serviceMode = 0;
		pittariFlag = FALSE;
		batFilePath = L"";
		suspendMode = 0;
		rebootFlag = FALSE;
		useMargineFlag = FALSE;
		startMargine = 10;
		endMargine = 5;
		continueRecFlag = 0;
		partialRecFlag = 0;
		tunerID = 0;
	};
} REC_SETTING_DATA;

//�o�^�\����
typedef struct _RESERVE_DATA{
	wstring title;					//�ԑg��
	SYSTEMTIME startTime;			//�^��J�n����
	DWORD durationSecond;			//�^�摍����
	wstring stationName;			//�T�[�r�X��
	WORD originalNetworkID;			//ONID
	WORD transportStreamID;			//TSID
	WORD serviceID;					//SID
	WORD eventID;					//EventID
	wstring comment;				//�R�����g
	DWORD reserveID;				//�\�񎯕�ID �\��o�^����0
	//BYTE recWaitFlag;				//�\��ҋ@�������H �����Ŏg�p�i�p�~�j
	BYTE presentFlag;				//EIT[present]�Ń`�F�b�N�ς݁H �����ɓ����Ŏg�p
	BYTE overlapMode;				//���Ԃ��� 1:���Ԃ��ă`���[�i�[����Ȃ��\�񂠂� 2:�`���[�i�[����Ȃ��ė\��ł��Ȃ�
	//wstring recFilePath;			//�^��t�@�C���p�X ���o�[�W�����݊��p ���g�p�i�p�~�j
	SYSTEMTIME startTimeEpg;		//�\�񎞂̊J�n����
	REC_SETTING_DATA recSetting;	//�^��ݒ�
	DWORD reserveStatus;			//�\��ǉ���� �����Ŏg�p
	//CMD_VER 5�ȍ~
	vector<wstring> recFileNameList;	//�^��\��t�@�C����
	//DWORD param1;					//�����p
	_RESERVE_DATA(void){
		title=L"";
		ZeroMemory(&startTime, sizeof(SYSTEMTIME));
		durationSecond = 0;
		stationName = L"";
		originalNetworkID = 0;
		transportStreamID = 0;
		serviceID = 0;
		eventID = 0;
		comment = L"";
		reserveID = 0;
		presentFlag = 0;
		overlapMode = 0;
		ZeroMemory(&startTimeEpg, sizeof(SYSTEMTIME));
		reserveStatus = 0;
	};
} RESERVE_DATA;

typedef struct _REC_FILE_INFO{
	DWORD id;					//ID
	wstring recFilePath;		//�^��t�@�C���p�X
	wstring title;				//�ԑg��
	SYSTEMTIME startTime;		//�J�n����
	DWORD durationSecond;		//�^�掞��
	wstring serviceName;		//�T�[�r�X��
	WORD originalNetworkID;		//ONID
	WORD transportStreamID;		//TSID
	WORD serviceID;				//SID
	WORD eventID;				//EventID
	__int64 drops;				//�h���b�v��
	__int64 scrambles;			//�X�N�����u����
	DWORD recStatus;			//�^�挋�ʂ̃X�e�[�^�X
	SYSTEMTIME startTimeEpg;	//�\�񎞂̊J�n����
	wstring comment;			//�R�����g
	wstring programInfo;		//.program.txt�t�@�C���̓��e
	wstring errInfo;			//.err�t�@�C���̓��e
	//CMD_VER 4�ȍ~
	BYTE protectFlag;
	_REC_FILE_INFO(void){
		id = 0;
		recFilePath = L"";
		title = L"";
		ZeroMemory(&startTime, sizeof(SYSTEMTIME));
		durationSecond = 0;
		serviceName = L"";
		originalNetworkID = 0;
		transportStreamID = 0;
		serviceID = 0;
		eventID = 0;
		drops = 0;
		scrambles = 0;
		recStatus = 0;
		ZeroMemory(&startTimeEpg, sizeof(SYSTEMTIME));
		comment = L"";
		programInfo = L"";
		errInfo = L"";
		protectFlag = 0;
	};
	_REC_FILE_INFO & operator= (const _RESERVE_DATA & o) {
		id = 0;
		recFilePath = L"";
		title = o.title;
		startTime = o.startTime;
		durationSecond = o.durationSecond;
		serviceName = o.stationName;
		originalNetworkID = o.originalNetworkID;
		transportStreamID = o.transportStreamID;
		serviceID = o.serviceID;
		eventID = o.eventID;
		drops = 0;
		scrambles = 0;
		recStatus = 0;
		startTimeEpg = o.startTimeEpg;
		comment = L"";
		programInfo = L"";
		errInfo = L"";
		protectFlag = 0;
		return *this;
	};
} REC_FILE_INFO;

typedef struct _TUNER_RESERVE_INFO{
	DWORD tunerID;
	wstring tunerName;
	vector<DWORD> reserveList;
} TUNER_RESERVE_INFO;

//�`���[�i�[���T�[�r�X���
typedef struct _CH_DATA4{
	int space;						//�`���[�i�[���
	int ch;							//�����`�����l��
	WORD originalNetworkID;			//ONID
	WORD transportStreamID;			//TSID
	WORD serviceID;					//�T�[�r�XID
	WORD serviceType;				//�T�[�r�X�^�C�v
	BOOL partialFlag;				//������M�T�[�r�X�i�����Z�O�j���ǂ���
	BOOL useViewFlag;				//�ꗗ�\���Ɏg�p���邩�ǂ���
	wstring serviceName;			//�T�[�r�X��
	wstring chName;					//�`�����l����
	wstring networkName;			//ts_name or network_name
	BYTE remoconID;					//�����R��ID
	_CH_DATA4(void){
		space = 0;
		ch = 0;
		originalNetworkID = 0;
		transportStreamID = 0;
		serviceID = 0;
		serviceType = 0;
		partialFlag = FALSE;
		useViewFlag = TRUE;
		serviceName = L"";
		chName = L"";
		networkName = L"";
		remoconID = 0;
	};
} CH_DATA4;

//�S�`���[�i�[�ŔF�������T�[�r�X�ꗗ
typedef struct _CH_DATA5{
	WORD originalNetworkID;			//ONID
	WORD transportStreamID;			//TSID
	WORD serviceID;					//�T�[�r�XID
	WORD serviceType;				//�T�[�r�X�^�C�v
	BOOL partialFlag;				//������M�T�[�r�X�i�����Z�O�j���ǂ���
	wstring serviceName;			//�T�[�r�X��
	wstring networkName;			//ts_name or network_name
	BOOL epgCapFlag;				//EPG�f�[�^�擾�Ώۂ��ǂ���
	BOOL searchFlag;				//�������̃f�t�H���g�����ΏۃT�[�r�X���ǂ���
	_CH_DATA5(void){
		originalNetworkID = 0;
		transportStreamID = 0;
		serviceID = 0;
		serviceType = 0;
		partialFlag = FALSE;
		serviceName = L"";
		networkName = L"";
		epgCapFlag = TRUE;
		searchFlag = TRUE;
	};
} CH_DATA5;

typedef struct _REGIST_TCP_INFO{
	wstring ip;
	DWORD port;
}REGIST_TCP_INFO;

//�R�}���h����M�X�g���[��
typedef struct _CMD_STREAM{
	DWORD param;	//���M���R�}���h�A��M���G���[�R�[�h
	DWORD dataSize;	//data�̃T�C�Y�iBYTE�P�ʁj
	std::unique_ptr<BYTE[]> data;	//����M����o�C�i���f�[�^�idataSize>0�̂Ƃ��K����NULL�j
	_CMD_STREAM(void){
		param = 0;
		dataSize = 0;
	}
} CMD_STREAM;

//EPG��{���
typedef struct _EPGDB_SHORT_EVENT_INFO{
	wstring event_name;			//�C�x���g��
	wstring text_char;			//���
} EPGDB_SHORT_EVENT_INFO;

//EPG�g�����
typedef struct _EPGDB_EXTENDED_EVENT_INFO{
	wstring text_char;			//�ڍ׏��
} EPGDB_EXTENDED_EVENT_INFO;

//EPG�W�������f�[�^
typedef struct _EPGDB_CONTENT_DATA{
	BYTE content_nibble_level_1;
	BYTE content_nibble_level_2;
	BYTE user_nibble_1;
	BYTE user_nibble_2;
}EPGDB_CONTENT_DATA;

//EPG�W���������
typedef struct _EPGDB_CONTENT_INFO{
	vector<EPGDB_CONTENT_DATA> nibbleList;
} EPGDB_CONTEN_INFO;

//EPG�f�����
typedef struct _EPGDB_COMPONENT_INFO{
	BYTE stream_content;
	BYTE component_type;
	BYTE component_tag;
	wstring text_char;			//���
} EPGDB_COMPONENT_INFO;

//EPG�������f�[�^
typedef struct _EPGDB_AUDIO_COMPONENT_INFO_DATA{
	BYTE stream_content;
	BYTE component_type;
	BYTE component_tag;
	BYTE stream_type;
	BYTE simulcast_group_tag;
	BYTE ES_multi_lingual_flag;
	BYTE main_component_flag;
	BYTE quality_indicator;
	BYTE sampling_rate;
	wstring text_char;			//�ڍ׏��
} EPGDB_AUDIO_COMPONENT_INFO_DATA;

//EPG�������
typedef struct _EPGDB_AUDIO_COMPONENT_INFO{
	vector<EPGDB_AUDIO_COMPONENT_INFO_DATA> componentList;
} EPGDB_AUDIO_COMPONENT_INFO;

//EPG�C�x���g�f�[�^
typedef struct _EPGDB_EVENT_DATA{
	WORD original_network_id;
	WORD transport_stream_id;
	WORD service_id;
	WORD event_id;
}EPGDB_EVENT_DATA;

//EPG�C�x���g�O���[�v���
typedef struct _EPGDB_EVENTGROUP_INFO{
	BYTE group_type;
	vector<EPGDB_EVENT_DATA> eventDataList;
} EPGDB_EVENTGROUP_INFO;

typedef struct _EPGDB_EVENT_INFO{
	WORD original_network_id;
	WORD transport_stream_id;
	WORD service_id;
	WORD event_id;							//�C�x���gID
	BYTE StartTimeFlag;						//start_time�̒l���L�����ǂ���
	SYSTEMTIME start_time;					//�J�n����
	BYTE DurationFlag;						//duration�̒l���L�����ǂ���
	DWORD durationSec;						//�����ԁi�P�ʁF�b�j

	std::unique_ptr<EPGDB_SHORT_EVENT_INFO> shortInfo;		//��{���
	std::unique_ptr<EPGDB_EXTENDED_EVENT_INFO> extInfo;		//�g�����
	std::unique_ptr<EPGDB_CONTEN_INFO> contentInfo;			//�W���������
	std::unique_ptr<EPGDB_COMPONENT_INFO> componentInfo;	//�f�����
	std::unique_ptr<EPGDB_AUDIO_COMPONENT_INFO> audioInfo;	//�������
	std::unique_ptr<EPGDB_EVENTGROUP_INFO> eventGroupInfo;	//�C�x���g�O���[�v���
	std::unique_ptr<EPGDB_EVENTGROUP_INFO> eventRelayInfo;	//�C�x���g�����[���

	BYTE freeCAFlag;						//�m���X�N�����u���t���O
	_EPGDB_EVENT_INFO(void){
	};
	void DeepCopy(const _EPGDB_EVENT_INFO & o){
		original_network_id = o.original_network_id;
		transport_stream_id = o.transport_stream_id;
		service_id = o.service_id;
		event_id = o.event_id;
		StartTimeFlag = o.StartTimeFlag;
		start_time = o.start_time;
		DurationFlag = o.DurationFlag;
		durationSec = o.durationSec;
		freeCAFlag = o.freeCAFlag;
		shortInfo.reset(o.shortInfo ? new EPGDB_SHORT_EVENT_INFO(*o.shortInfo) : NULL);
		extInfo.reset(o.extInfo ? new EPGDB_EXTENDED_EVENT_INFO(*o.extInfo) : NULL);
		contentInfo.reset(o.contentInfo ? new EPGDB_CONTEN_INFO(*o.contentInfo) : NULL);
		componentInfo.reset(o.componentInfo ? new EPGDB_COMPONENT_INFO(*o.componentInfo) : NULL);
		audioInfo.reset(o.audioInfo ? new EPGDB_AUDIO_COMPONENT_INFO(*o.audioInfo) : NULL);
		eventGroupInfo.reset(o.eventGroupInfo ? new EPGDB_EVENTGROUP_INFO(*o.eventGroupInfo) : NULL);
		eventRelayInfo.reset(o.eventRelayInfo ? new EPGDB_EVENTGROUP_INFO(*o.eventRelayInfo) : NULL);
	};
#if defined(_MSC_VER) && _MSC_VER < 1900
	//�Öك��[�u���Ή��̌Â��R���p�C���Ɍ���R�s�[���`���Ă���
	//�R���e�i�Ŗ��ʂȃR�s�[������Ȃ��悤�ɏ��������l�����ׂ�(�_�o���ɂȂ�K�v�͂Ȃ�)
	_EPGDB_EVENT_INFO(const _EPGDB_EVENT_INFO & o){ DeepCopy(o); }
	_EPGDB_EVENT_INFO & operator= (const _EPGDB_EVENT_INFO & o){ DeepCopy(o); return *this; }
#endif
}EPGDB_EVENT_INFO;

typedef struct _EPGDB_SERVICE_INFO{
	WORD ONID;
	WORD TSID;
	WORD SID;
	BYTE service_type;
	BYTE partialReceptionFlag;
	wstring service_provider_name;
	wstring service_name;
	wstring network_name;
	wstring ts_name;
	BYTE remote_control_key_id;
	_EPGDB_SERVICE_INFO(void){
		ONID = 0;
		TSID = 0;
		SID = 0;
		service_type = 0;
		partialReceptionFlag = 0;
		service_provider_name = L"";
		service_name = L"";
		network_name = L"";
		ts_name = L"";
		remote_control_key_id = 0;
	};
}EPGDB_SERVICE_INFO;

typedef struct _EPGDB_SERVICE_EVENT_INFO{
	EPGDB_SERVICE_INFO serviceInfo;
	vector<EPGDB_EVENT_INFO> eventList;
}EPGDB_SERVICE_EVENT_INFO;

typedef struct _EPGDB_SEARCH_DATE_INFO{
	BYTE startDayOfWeek;
	WORD startHour;
	WORD startMin;
	BYTE endDayOfWeek;
	WORD endHour;
	WORD endMin;
} EPGDB_SEARCH_DATE_INFO;

//��������
typedef struct _EPGDB_SEARCH_KEY_INFO{
	wstring andKey;
	wstring notKey;
	BOOL regExpFlag;
	BOOL titleOnlyFlag;
	vector<EPGDB_CONTENT_DATA> contentList;
	vector<EPGDB_SEARCH_DATE_INFO> dateList;
	vector<__int64> serviceList;
	vector<WORD> videoList;
	vector<WORD> audioList;
	BYTE aimaiFlag;
	BYTE notContetFlag;
	BYTE notDateFlag;
	BYTE freeCAFlag;
	//CMD_VER 3�ȍ~
	//�����\��o�^�̏�����p
	BYTE chkRecEnd;					//�^��ς��̃`�F�b�N����
	WORD chkRecDay;					//�^��ς��̃`�F�b�N�Ώۊ��ԁi+20000=SID����,+30000=TS|SID����,+40000=ON|TS|SID�����j
	_EPGDB_SEARCH_KEY_INFO(void){
		andKey = L"";
		notKey = L"";
		regExpFlag = FALSE;
		titleOnlyFlag = FALSE;
		aimaiFlag = 0;
		notContetFlag = 0;
		notDateFlag = 0;
		freeCAFlag = 0;
		chkRecEnd = 0;
		chkRecDay = 6;
	};
}EPGDB_SEARCH_KEY_INFO;

//�����\��o�^���
typedef struct _EPG_AUTO_ADD_DATA{
	DWORD dataID;
	EPGDB_SEARCH_KEY_INFO searchInfo;	//�����L�[
	REC_SETTING_DATA recSetting;	//�^��ݒ�
	DWORD addCount;		//�\��o�^��
} EPG_AUTO_ADD_DATA;

typedef struct _MANUAL_AUTO_ADD_DATA{
	DWORD dataID;
	BYTE dayOfWeekFlag;				//�Ώۗj��
	DWORD startTime;				//�^��J�n���ԁi00:00��0�Ƃ��ĕb�P�ʁj
	DWORD durationSecond;			//�^�摍����
	wstring title;					//�ԑg��
	wstring stationName;			//�T�[�r�X��
	WORD originalNetworkID;			//ONID
	WORD transportStreamID;			//TSID
	WORD serviceID;					//SID
	REC_SETTING_DATA recSetting;	//�^��ݒ�
} MANUAL_AUTO_ADD_DATA;

//�R�}���h���M�p
//�`�����l���ύX���
typedef struct _SET_CH_INFO{
	BOOL useSID;//wONID��wTSID��wSID�̒l���g�p�ł��邩�ǂ���
	WORD ONID;
	WORD TSID;
	WORD SID;
	BOOL useBonCh;//dwSpace��dwCh�̒l���g�p�ł��邩�ǂ���
	DWORD space;
	DWORD ch;
}SET_CH_INFO;

typedef struct _SET_CTRL_MODE{
	DWORD ctrlID;
	WORD SID;
	BYTE enableScramble;
	BYTE enableCaption;
	BYTE enableData;
} SET_CTRL_MODE;

typedef struct _SET_CTRL_REC_PARAM{
	DWORD ctrlID;
	wstring fileName;
	BYTE overWriteFlag;
	ULONGLONG createSize;
	vector<REC_FILE_SET_INFO> saveFolder;
	BYTE pittariFlag;
	WORD pittariONID;
	WORD pittariTSID;
	WORD pittariSID;
	WORD pittariEventID;
} SET_CTRL_REC_PARAM;

typedef struct _SET_CTRL_REC_STOP_PARAM{
	DWORD ctrlID;
	BOOL saveErrLog;
} SET_CTRL_REC_STOP_PARAM;

typedef struct _SET_CTRL_REC_STOP_RES_PARAM{
	wstring recFilePath;
	ULONGLONG drop;
	ULONGLONG scramble;
	BYTE subRecFlag;
} SET_CTRL_REC_STOP_RES_PARAM;

typedef struct _SEARCH_EPG_INFO_PARAM{
	WORD ONID;
	WORD TSID;
	WORD SID;
	WORD eventID;
	BYTE pfOnlyFlag;
} SEARCH_EPG_INFO_PARAM;

typedef struct _GET_EPG_PF_INFO_PARAM{
	WORD ONID;
	WORD TSID;
	WORD SID;
	BYTE pfNextFlag;
} GET_EPG_PF_INFO_PARAM;

typedef struct _TVTEST_CH_CHG_INFO{
	wstring bonDriver;
	SET_CH_INFO chInfo;
} TVTEST_CH_CHG_INFO;


typedef struct _TVTEST_STREAMING_INFO{
	BOOL enableMode;
	DWORD ctrlID;
	DWORD serverIP;
	DWORD serverPort;
	wstring filePath;
	BOOL udpSend;
	BOOL tcpSend;
	BOOL timeShiftMode;
} TVTEST_STREAMING_INFO;

typedef struct _NWPLAY_PLAY_INFO{
	DWORD ctrlID;
	DWORD ip;
	BYTE udp;
	BYTE tcp;
	DWORD udpPort;//out�Ŏ��ۂ̊J�n�|�[�g
	DWORD tcpPort;//out�Ŏ��ۂ̊J�n�|�[�g
} NWPLAY_PLAY_INFO;

typedef struct _NWPLAY_POS_CMD{
	DWORD ctrlID;
	__int64 currentPos;
	__int64 totalPos;//CMD2_EPG_SRV_NWPLAY_SET_POS���͖���
} NWPLAY_POS_CMD;

typedef struct _NWPLAY_TIMESHIFT_INFO{
	DWORD ctrlID;
	wstring filePath;
} NWPLAY_TIMESHIFT_INFO;

//���ʒm�p�p�����[�^�[
typedef struct _NOTIFY_SRV_INFO{
	DWORD notifyID;		//�ʒm���̎��
	SYSTEMTIME time;	//�ʒm��Ԃ̔�����������
	DWORD param1;		//�p�����[�^�[�P�i��ނɂ���ē��e�ύX�j
	DWORD param2;		//�p�����[�^�[�Q�i��ނɂ���ē��e�ύX�j
	DWORD param3;		//�p�����[�^�[�R�i�ʒm�̏���J�E���^�j
	wstring param4;		//�p�����[�^�[�S�i��ނɂ���ē��e�ύX�j
	wstring param5;		//�p�����[�^�[�T�i��ނɂ���ē��e�ύX�j
	wstring param6;		//�p�����[�^�[�U�i��ނɂ���ē��e�ύX�j
	_NOTIFY_SRV_INFO(void){
		notifyID= 0;
		param1 = 0;
		param2 = 0;
		param3 = 0;
	};
} NOTIFY_SRV_INFO;

#endif
