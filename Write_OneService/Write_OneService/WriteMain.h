#pragma once

#include "../../Common/Util.h"
#include "../../Common/StringUtil.h"
#include "../../BonCtrl/PacketInit.h"
#include "../../BonCtrl/CreatePATPacket.h"
#include "../../BonCtrl/CATUtil.h"
#include "../../BonCtrl/PMTUtil.h"

class CWriteMain
{
public:
	CWriteMain();
	~CWriteMain();

	//�t�@�C���ۑ����J�n����
	//�߂�l�F
	// TRUE�i�����j�AFALSE�i���s�j
	//�����F
	// fileName				[IN]�ۑ��t�@�C���t���p�X�i�K�v�ɉ����Ċg���q�ς�����ȂǍs���j
	// overWriteFlag		[IN]����t�@�C�������ݎ��ɏ㏑�����邩�ǂ����iTRUE�F����AFALSE�F���Ȃ��j
	// createSize			[IN]���͗\�z�e�ʁi188�o�C�gTS�ł̗e�ʁB�����^�掞�ȂǑ����Ԗ���̏ꍇ��0�B�����Ȃǂ̉\��������̂Ŗڈ����x�j
	BOOL Start(
		LPCWSTR fileName,
		BOOL overWriteFlag,
		ULONGLONG createSize
		);

	//�t�@�C���ۑ����I������
	//�߂�l�F
	// TRUE�i�����j�AFALSE�i���s�j
	BOOL Stop(
		);

	//���ۂɕۑ����Ă���t�@�C���p�X���擾����i�Đ���o�b�`�����ɗ��p�����j
	//�߂�l�F
	// �ۑ��t�@�C���t���p�X
	wstring GetSavePath(
		);

	//�ۑ��pTS�f�[�^�𑗂�
	//�󂫗e�ʕs���Ȃǂŏ����o�����s�����ꍇ�AwriteSize�̒l������
	//�ēx�ۑ���������Ƃ��̑��M�J�n�n�_�����߂�
	//�߂�l�F
	// TRUE�i�����j�AFALSE�i���s�j
	//�����F
	// data					[IN]TS�f�[�^
	// size					[IN]data�̃T�C�Y
	// writeSize			[OUT]�ۑ��ɗ��p�����T�C�Y
	BOOL Write(
		BYTE* data,
		DWORD size,
		DWORD* writeSize
		);

private:
	struct WRITE_PLUGIN {
		HMODULE hDll;
		DWORD id;
		BOOL (WINAPI *pfnCreateCtrl)(DWORD*);
		BOOL (WINAPI *pfnDeleteCtrl)(DWORD);
		BOOL (WINAPI *pfnStartSave)(DWORD,LPCWSTR,BOOL,ULONGLONG);
		BOOL (WINAPI *pfnStopSave)(DWORD);
		BOOL (WINAPI *pfnGetSaveFilePath)(DWORD,WCHAR*,DWORD*);
		BOOL (WINAPI *pfnAddTSBuff)(DWORD,BYTE*,DWORD,DWORD*);
		BOOL Initialize(LPCWSTR path) {
			hDll = LoadLibrary(path);
			if( hDll ){
				if( (pfnCreateCtrl = (BOOL (WINAPI*)(DWORD*))(GetProcAddress(hDll, "CreateCtrl"))) != NULL &&
				    (pfnDeleteCtrl = (BOOL (WINAPI*)(DWORD))(GetProcAddress(hDll, "DeleteCtrl"))) != NULL &&
				    (pfnStartSave = (BOOL (WINAPI*)(DWORD,LPCWSTR,BOOL,ULONGLONG))(GetProcAddress(hDll, "StartSave"))) != NULL &&
				    (pfnStopSave = (BOOL (WINAPI*)(DWORD))(GetProcAddress(hDll, "StopSave"))) != NULL &&
				    (pfnGetSaveFilePath = (BOOL (WINAPI*)(DWORD,WCHAR*,DWORD*))(GetProcAddress(hDll, "GetSaveFilePath"))) != NULL &&
				    (pfnAddTSBuff = (BOOL (WINAPI*)(DWORD,BYTE*,DWORD,DWORD*))(GetProcAddress(hDll, "AddTSBuff"))) != NULL ){
					if( pfnCreateCtrl(&id) ){
						return TRUE;
					}
				}
				FreeLibrary(hDll);
				hDll = NULL;
			}
			return FALSE;
		}
	};
	HANDLE file;
	WRITE_PLUGIN writePlugin;
	wstring savePath;
	WORD targetSID;
	WORD lastTSID;
	CPacketInit packetInit;
	CCreatePATPacket patUtil;
	CCATUtil catUtil;
	map<WORD, CPMTUtil> pmtUtilMap;
	vector<WORD> needPIDList;
	vector<BYTE> outBuff;

	void AddNeedPID(WORD pid);
	void CheckNeedPID();
};

