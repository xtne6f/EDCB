#pragma once

#include "AribDescriptor.h"
#include "../../Common/EpgDataCap3Def.h"

class CEpgDBUtil
{
public:
	CEpgDBUtil(void);
	~CEpgDBUtil(void);

	BOOL AddEIT(WORD PID, const AribDescriptor::CDescriptor& eit, __int64 streamTime);

	BOOL AddServiceListNIT(const AribDescriptor::CDescriptor& nit);
	BOOL AddServiceListSIT(WORD TSID, const AribDescriptor::CDescriptor& sit);
	BOOL AddSDT(const AribDescriptor::CDescriptor& sdt);

	void SetStreamChangeEvent();

	//EPG�f�[�^�̒~�Ϗ�Ԃ����Z�b�g����
	void ClearSectionStatus();

	//�w��T�[�r�X��EPG�f�[�^�̒~�Ϗ�Ԃ��擾����
	//�߂�l�F
	// �X�e�[�^�X
	//�����F
	// originalNetworkID		[IN]�擾�Ώۂ�OriginalNetworkID
	// transportStreamID		[IN]�擾�Ώۂ�TransportStreamID
	// serviceID				[IN]�擾�Ώۂ�ServiceID
	// l_eitFlag				[IN]L-EIT�̃X�e�[�^�X���擾
	EPG_SECTION_STATUS GetSectionStatusService(
		WORD originalNetworkID,
		WORD transportStreamID,
		WORD serviceID,
		BOOL l_eitFlag
		);

	//EPG�f�[�^�̒~�Ϗ�Ԃ��擾����
	//�߂�l�F
	// �X�e�[�^�X
	//�����F
	// l_eitFlag		[IN]L-EIT�̃X�e�[�^�X���擾
	EPG_SECTION_STATUS GetSectionStatus(BOOL l_eitFlag);

	//�w��T�[�r�X�̑SEPG�����擾����
	//�����F
	// originalNetworkID		[IN]�擾�Ώۂ�originalNetworkID
	// transportStreamID		[IN]�擾�Ώۂ�transportStreamID
	// serviceID				[IN]�擾�Ώۂ�ServiceID
	// epgInfoListSize			[OUT]epgInfoList�̌�
	// epgInfoList				[OUT]EPG���̃��X�g�iDLL���Ŏ����I��delete����B���Ɏ擾���s���܂ŗL���j
	BOOL GetEpgInfoList(
		WORD originalNetworkID,
		WORD transportStreamID,
		WORD serviceID,
		DWORD* epgInfoListSize,
		EPG_EVENT_INFO** epgInfoList_
		);

	//�w��T�[�r�X�̑SEPG����񋓂���
	BOOL EnumEpgInfoList(
		WORD originalNetworkID,
		WORD transportStreamID,
		WORD serviceID,
		BOOL (CALLBACK *enumEpgInfoListProc)(DWORD, EPG_EVENT_INFO*, LPVOID),
		LPVOID param
		);

	//�~�ς��ꂽEPG���̂���T�[�r�X�ꗗ���擾����
	//SERVICE_EXT_INFO�̏��͂Ȃ��ꍇ������
	//�����F
	// serviceListSize			[OUT]serviceList�̌�
	// serviceList				[OUT]�T�[�r�X���̃��X�g�iDLL���Ŏ����I��delete����B���Ɏ擾���s���܂ŗL���j
	void GetServiceListEpgDB(
		DWORD* serviceListSize,
		SERVICE_INFO** serviceList_
		);

	//�w��T�[�r�X�̌���or����EPG�����擾����
	//�����F
	// originalNetworkID		[IN]�擾�Ώۂ�originalNetworkID
	// transportStreamID		[IN]�擾�Ώۂ�transportStreamID
	// serviceID				[IN]�擾�Ώۂ�ServiceID
	// nextFlag					[IN]TRUE�i���̔ԑg�j�AFALSE�i���݂̔ԑg�j
	// epgInfo					[OUT]EPG���iDLL���Ŏ����I��delete����B���Ɏ擾���s���܂ŗL���j
	BOOL GetEpgInfo(
		WORD originalNetworkID,
		WORD transportStreamID,
		WORD serviceID,
		BOOL nextFlag,
		EPG_EVENT_INFO** epgInfo_
		);

	//�w��C�x���g��EPG�����擾����
	//�����F
	// originalNetworkID		[IN]�擾�Ώۂ�originalNetworkID
	// transportStreamID		[IN]�擾�Ώۂ�transportStreamID
	// serviceID				[IN]�擾�Ώۂ�ServiceID
	// EventID					[IN]�擾�Ώۂ�EventID
	// pfOnlyFlag				[IN]p/f����̂݌������邩�ǂ���
	// epgInfo					[OUT]EPG���iDLL���Ŏ����I��delete����B���Ɏ擾���s���܂ŗL���j
	BOOL SearchEpgInfo(
		WORD originalNetworkID,
		WORD transportStreamID,
		WORD serviceID,
		WORD eventID,
		BYTE pfOnlyFlag,
		EPG_EVENT_INFO** epgInfo_
		);

protected:
	CRITICAL_SECTION dbLock;

	struct SI_TAG{
		BYTE tableID;		//�f�[�^�ǉ�����table_id
		BYTE version;		//�f�[�^�ǉ����̃o�[�W����
		DWORD time;			//�f�[�^�̃^�C���X�^���v(�P�ʂ�10�b)
	};
	struct EVENT_INFO : EPG_EVENT_INFO{
		//�f�X�g���N�^�͔񉼑z�Ȃ̂Œ���
		DWORD time;
		SI_TAG tagBasic;
		SI_TAG tagExt;
	};
	struct SECTION_FLAG_INFO{
		BYTE version;
		BYTE flags[32];			//�Z�O�����g(0�`31)���̎�M�ς݃Z�N�V����(0�`7)�̃t���O
		BYTE ignoreFlags[32];	//��������(���o����Ȃ�)�Z�N�V�����̃t���O
	};
	struct SERVICE_EVENT_INFO{
		map<WORD, std::unique_ptr<EVENT_INFO>> eventMap;
		std::unique_ptr<EVENT_INFO> nowEvent;
		std::unique_ptr<EVENT_INFO> nextEvent;
		BYTE lastTableID;
		BYTE lastTableIDExt;
		vector<SECTION_FLAG_INFO> sectionList;	//�Y�����̓e�[�u���ԍ�(0�`7)
		vector<SECTION_FLAG_INFO> sectionExtList;
		SERVICE_EVENT_INFO(void){
			lastTableID = 0;
			lastTableIDExt = 0;
			sectionList.resize(8);
		}
		SERVICE_EVENT_INFO(SERVICE_EVENT_INFO&& o){
			*this = std::move(o);
		}
		SERVICE_EVENT_INFO& operator=(SERVICE_EVENT_INFO&& o){
			eventMap.swap(o.eventMap);
			nowEvent.swap(o.nowEvent);
			nextEvent.swap(o.nextEvent);
			lastTableID = o.lastTableID;
			lastTableIDExt = o.lastTableIDExt;
			sectionList.swap(o.sectionList);
			sectionExtList.swap(o.sectionExtList);
			return *this;
		}
	};
	map<ULONGLONG, SERVICE_EVENT_INFO> serviceEventMap;
	map<ULONGLONG, BYTE> serviceList;

	typedef struct _DB_SERVICE_INFO{
		WORD original_network_id;	//original_network_id
		WORD transport_stream_id;	//transport_stream_id
		WORD service_id;			//service_id
		BYTE service_type;
		BYTE partialReceptionFlag;
		wstring service_provider_name;
		wstring service_name;
		_DB_SERVICE_INFO(void){
			service_type = 0;
			partialReceptionFlag = FALSE;
			service_provider_name = L"";
			service_name = L"";
		};
	}DB_SERVICE_INFO;
	typedef struct _DB_TS_INFO{
		WORD original_network_id;	//original_network_id
		WORD transport_stream_id;	//transport_stream_id
		wstring network_name;
		wstring ts_name;
		BYTE remote_control_key_id;
		map<WORD,DB_SERVICE_INFO> serviceList;
		_DB_TS_INFO(void){
			network_name = L"";
			ts_name = L"";
			remote_control_key_id = 0;
		};
	}DB_TS_INFO;
	map<DWORD, DB_TS_INFO> serviceInfoList;

	std::unique_ptr<EPG_EVENT_INFO[]> epgInfoList;

	std::unique_ptr<EPG_EVENT_INFO> epgInfo;

	std::unique_ptr<EPG_EVENT_INFO> searchEpgInfo;

	std::unique_ptr<SERVICE_INFO[]> serviceDBList;
protected:
	void Clear();
	
	static void AddBasicInfo(EVENT_INFO* eventInfo, const AribDescriptor::CDescriptor& eit, AribDescriptor::CDescriptor::CLoopPointer lpParent, WORD onid, WORD tsid);
	static void AddShortEvent(EVENT_INFO* eventInfo, const AribDescriptor::CDescriptor& eit, AribDescriptor::CDescriptor::CLoopPointer lp);
	static BOOL AddExtEvent(EVENT_INFO* eventInfo, const AribDescriptor::CDescriptor& eit, AribDescriptor::CDescriptor::CLoopPointer lpParent);
	static void AddContent(EVENT_INFO* eventInfo, const AribDescriptor::CDescriptor& eit, AribDescriptor::CDescriptor::CLoopPointer lp);
	static void AddComponent(EVENT_INFO* eventInfo, const AribDescriptor::CDescriptor& eit, AribDescriptor::CDescriptor::CLoopPointer lp);
	static BOOL AddAudioComponent(EVENT_INFO* eventInfo, const AribDescriptor::CDescriptor& eit, AribDescriptor::CDescriptor::CLoopPointer lpParent);
	static void AddEventGroup(EVENT_INFO* eventInfo, const AribDescriptor::CDescriptor& eit, AribDescriptor::CDescriptor::CLoopPointer lp, WORD onid, WORD tsid);
	static void AddEventRelay(EVENT_INFO* eventInfo, const AribDescriptor::CDescriptor& eit, AribDescriptor::CDescriptor::CLoopPointer lp, WORD onid, WORD tsid);

	static BOOL CheckSectionAll(const vector<SECTION_FLAG_INFO>& sectionList);

	void CopyEpgInfo(EPG_EVENT_INFO* destInfo, EVENT_INFO* srcInfo);
};
