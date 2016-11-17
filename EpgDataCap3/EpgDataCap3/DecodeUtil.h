#pragma once

#include "EpgDBUtil.h"
#include "../../Common/TSPacketUtil.h"
#include "../../Common/TSBuffUtil.h"
#include "../../Common/EpgDataCap3Def.h"

class CDecodeUtil
{
public:
	CDecodeUtil(void);

	void SetEpgDB(CEpgDBUtil* epgDBUtil_);
	void AddTSData(BYTE* data);

	//��̓f�[�^�̌��݂̃X�g���[���h�c���擾����
	//�����F
	// originalNetworkID		[OUT]���݂�originalNetworkID
	// transportStreamID		[OUT]���݂�transportStreamID
	BOOL GetTSID(
		WORD* originalNetworkID,
		WORD* transportStreamID
		);

	//���X�g���[���̃T�[�r�X�ꗗ���擾����
	//�����F
	// serviceListSize			[OUT]serviceList�̌�
	// serviceList				[OUT]�T�[�r�X���̃��X�g�iDLL���Ŏ����I��delete����B���Ɏ擾���s���܂ŗL���j
	BOOL GetServiceListActual(
		DWORD* serviceListSize,
		SERVICE_INFO** serviceList_
		);

	//�X�g���[�����̌��݂̎��ԏ����擾����
	//�����F
	// time				[OUT]�X�g���[�����̌��݂̎���
	// tick				[OUT]time���擾�������_�̃`�b�N�J�E���g
	BOOL GetNowTime(
		FILETIME* time,
		DWORD* tick = NULL
		);

protected:
	CEpgDBUtil* epgDBUtil;

	AribDescriptor::CDescriptor tableBuff;

	//PID���̃o�b�t�@�����O
	//�L�[ PID
	map<WORD, CTSBuffUtil> buffUtilMap;

	std::unique_ptr<const AribDescriptor::CDescriptor> patInfo;
	map<BYTE, AribDescriptor::CDescriptor> nitActualInfo;
	map<BYTE, AribDescriptor::CDescriptor> sdtActualInfo;
	std::unique_ptr<const AribDescriptor::CDescriptor> bitInfo;
	std::unique_ptr<const AribDescriptor::CDescriptor> sitInfo;
	FILETIME totTime;
	FILETIME tdtTime;
	FILETIME sitTime;
	DWORD totTimeTick;
	DWORD tdtTimeTick;
	DWORD sitTimeTick;


	std::unique_ptr<SERVICE_INFO[]> serviceList;

protected:
	void Clear();
	void ClearBuff(WORD noClearPid);
	void ChangeTSIDClear(WORD noClearPid);

	void CheckPAT(WORD PID, const AribDescriptor::CDescriptor& pat);
	void CheckNIT(WORD PID, const AribDescriptor::CDescriptor& nit);
	void CheckSDT(WORD PID, const AribDescriptor::CDescriptor& sdt);
	void CheckTDT(const AribDescriptor::CDescriptor& tdt);
	void CheckEIT(WORD PID, const AribDescriptor::CDescriptor& eit);
	void CheckBIT(WORD PID, const AribDescriptor::CDescriptor& bit);
	void CheckSIT(const AribDescriptor::CDescriptor& sit);

	//���X�g���[���̃T�[�r�X�ꗗ��SIT����擾����
	//�����F
	// serviceListSize			[OUT]serviceList�̌�
	// serviceList				[OUT]�T�[�r�X���̃��X�g�iDLL���Ŏ����I��delete����B���Ɏ擾���s���܂ŗL���j
	BOOL GetServiceListSIT(
		DWORD* serviceListSize,
		SERVICE_INFO** serviceList_
		);

};
