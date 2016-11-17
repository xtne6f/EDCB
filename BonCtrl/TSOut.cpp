#include "StdAfx.h"
#include "TSOut.h"

#include "../Common/TimeUtil.h"
#include "../Common/EpgTimerUtil.h"

CTSOut::CTSOut(void)
{
	this->lockEvent = CreateEvent(NULL, FALSE, TRUE, NULL);

	this->chChangeFlag = FALSE;
	this->chChangeErr = FALSE;
	this->chChangeTime = 0;
	this->lastONID = 0xFFFF;
	this->lastTSID = 0xFFFF;

	this->epgUtil.Initialize(FALSE);

	this->enableDecodeFlag = TRUE;
	this->emmEnableFlag = FALSE;
	this->serviceOnlyFlag = FALSE;


	this->catUtil = NULL;

	this->nextCtrlID = 1;

	this->epgFile = NULL;
}


CTSOut::~CTSOut(void)
{
	if( this->epgFile != NULL ){
		CloseHandle(this->epgFile);
		DeleteFile(this->epgTempFilePath.c_str());
	}
	if( this->lockEvent != NULL ){
		UnLock();
		CloseHandle(this->lockEvent);
		this->lockEvent = NULL;
	}

	map<DWORD, COneServiceUtil*>::iterator itr;
	for( itr = this->serviceUtilMap.begin(); itr != this->serviceUtilMap.end(); itr++ ){
		SAFE_DELETE(itr->second);
	}
	this->serviceUtilMap.clear();

	SAFE_DELETE(this->catUtil);

	this->epgUtil.UnInitialize();
}

BOOL CTSOut::Lock(LPCWSTR log, DWORD timeOut)
{
	if( this->lockEvent == NULL ){
		return FALSE;
	}
	//if( log != NULL ){
	//	OutputDebugString(log);
	//}
	DWORD dwRet = WaitForSingleObject(this->lockEvent, timeOut);
	if( dwRet == WAIT_ABANDONED || 
		dwRet == WAIT_FAILED ||
		dwRet == WAIT_TIMEOUT){
			if( log != NULL ){
				_OutputDebugString(L"��CTSOut::Lock FALSE : %s", log);
			}else{
				OutputDebugString(L"��CTSOut::Lock FALSE");
			}
		return FALSE;
	}
	return TRUE;
}

void CTSOut::UnLock(LPCWSTR log)
{
	if( this->lockEvent != NULL ){
		SetEvent(this->lockEvent);
	}
	if( log != NULL ){
		OutputDebugString(log);
	}
}

DWORD CTSOut::SetChChangeEvent(BOOL resetEpgUtil)
{
	if( Lock(L"SetChChangeEvent") == FALSE ) return ERR_FALSE;

	this->chChangeFlag = TRUE;
	this->chChangeErr = FALSE;
	this->chChangeTime = GetTickCount();

	this->decodeUtil.UnLoadDll();

	SAFE_DELETE(this->catUtil);

	if( resetEpgUtil == TRUE ){
		this->epgUtil.UnInitialize();
		this->epgUtil.Initialize(FALSE);
	}

	UnLock();
	return NO_ERR;
}

BOOL CTSOut::IsChChanging(BOOL* chChgErr)
{
	if( Lock(L"IsChChanging") == FALSE ) return ERR_FALSE;

	BOOL ret = this->chChangeFlag;
	if( chChgErr != NULL ){
		*chChgErr = this->chChangeErr;
	}

	if( this->chChangeFlag == TRUE ){
		if( GetTickCount() - this->chChangeTime > 15000 ){
			ret = FALSE;
			if( chChgErr != NULL ){
				*chChgErr = TRUE;
			}
		}
	}

	UnLock();
	return ret;
}

void CTSOut::ResetChChange()
{
	this->lastONID = 0xFFFF;
	this->lastTSID = 0xFFFF;
}

BOOL CTSOut::GetStreamID(WORD* ONID, WORD* TSID)
{
	if( Lock(L"GetStreamID") == FALSE ) return ERR_FALSE;

	BOOL ret = TRUE;
	if( this->chChangeFlag == TRUE ){
		ret = FALSE;
	}else{
		ret = this->epgUtil.GetTSID(ONID, TSID);
		if( ret == NO_ERR ){
			this->lastONID = *ONID;
			this->lastTSID = *TSID;
		}
	}
	UnLock();
	return ret;
}

void CTSOut::OnChChanged(WORD onid, WORD tsid)
{
	this->chChangeFlag = FALSE;
	this->chChangeErr = FALSE;
	this->lastONID = onid;
	this->lastTSID = tsid;
	this->epgUtil.ClearSectionStatus();

	if( this->enableDecodeFlag != FALSE || this->emmEnableFlag != FALSE ){
		//�X�N�����u��������EMM�������ݒ肳��Ă���ꍇ�������s
		if( this->decodeUtil.SetNetwork(onid, tsid) == FALSE ){
			OutputDebugString(L"����Decode DLL load err [CTSOut::OnChChanged()]\r\n");
			//�Ď��s�͈Ӗ����Ȃ������Ȃ̂Ŕp�~
		}
		this->decodeUtil.SetEmm(this->emmEnableFlag);
	}
	ResetErrCount();

	this->pmtUtilMap.clear();
}

DWORD CTSOut::AddTSBuff(BYTE* data, DWORD dataSize)
{
	//data�͓����ς݂����̃T�C�Y��188�̐����{�ł��邱��

	if( Lock(L"AddTSBuff") == FALSE ) return ERR_FALSE;
	if( dataSize == 0 || data == NULL ){
		UnLock();
		return ERR_FALSE;
	}
	this->decodeBuff.clear();

	BYTE* decodeData = NULL;
	DWORD decodeSize = 0;
	{
		for( DWORD i=0; i<dataSize; i+=188 ){
			CTSPacketUtil packet;
			if( packet.Set188TS(data + i, 188) == TRUE ){
				if( this->chChangeFlag == TRUE ){
					//�`�����l���؂�ւ���
					if( GetTickCount() - this->chChangeTime < 1000 ){
						//1�b�Ԃ͐؂�ւ��O�̃p�P�b�g����\�����l�����Ė�������
						UnLock();
						return NO_ERR;
					}
					//�ȈՃp�P�b�g���
					if( packet.transport_scrambling_control != 0 ){
						//�X�N�����u���p�P�b�g�Ȃ̂ŉ�͂ł��Ȃ�
						continue;
					}
					if( packet.data_byteSize < 3 ){
						//�T�C�Y������������
						continue;
					}
					if(packet.payload_unit_start_indicator == 1 && 
						packet.data_byte[0] == 0x00 &&
						packet.data_byte[1] == 0x00 &&
						packet.data_byte[2] == 0x01){
						//PES
						continue;
					}
					this->epgUtil.AddTSPacket(data + i, 188);
					WORD onid = 0xFFFF;
					WORD tsid = 0xFFFF;
					if( this->epgUtil.GetTSID(&onid, &tsid) == NO_ERR ){
						if( onid != this->lastONID || tsid != this->lastTSID ){
							OutputDebugString(L"��Ch Change Complete\r\n");
						_OutputDebugString(L"��Ch 0x%04X 0x%04X => 0x%04X 0x%04X\r\n", this->lastONID, this->lastTSID, onid, tsid);
							OnChChanged(onid, tsid);

						}else if( GetTickCount() - this->chChangeTime > 7000 ){
							OutputDebugString(L"��Ch NoChange\r\n");
							OnChChanged(onid, tsid);
						}else{
							continue;
						}
					}
					else{
						if( GetTickCount() - this->chChangeTime > 15000 ){
							//15�b�ȏソ���Ă�Ȃ�؂�ւ�����Ƃ���
							OutputDebugString(L"��GetTSID Err\r\n");
							//this->chChangeFlag = FALSE;
							this->chChangeErr = TRUE;
							this->lastONID = onid;
							this->lastTSID = tsid;
							//this->epgUtil.ClearSectionStatus();
							//this->decodeUtil.SetNetwork(onid, tsid);
							//this->decodeUtil.SetEmm(this->emmEnableFlag);
							//ResetErrCount();
						}
						continue;
					}

				}else{
					//�w��T�[�r�X�ɕK�v��PID�����
					if( packet.transport_scrambling_control == 0 ){
						//CAT
						if( packet.PID == 0x0001 ){
							if( this->catUtil == NULL ){
								this->catUtil = new CCATUtil;
							}
							if(this->catUtil->AddPacket(&packet) == TRUE){
								CheckNeedPID();
							}
						}
						//PMT
						if( packet.payload_unit_start_indicator == 1 && packet.data_byteSize > 0){
							BYTE pointer = packet.data_byte[0];
							if( pointer+1 < packet.data_byteSize ){
								if( packet.data_byte[1+pointer] == 0x02 ){
									//PMT
									map<WORD, CPMTUtil>::iterator itrPmt;
									itrPmt = this->pmtUtilMap.find(packet.PID);
									if( itrPmt == this->pmtUtilMap.end() ){
										itrPmt = this->pmtUtilMap.insert(std::make_pair(packet.PID, CPMTUtil())).first;
									}
									if( itrPmt->second.AddPacket(&packet) == TRUE ){
										CheckNeedPID();
									}
								}
							}
						}else{
							//PMT��2�p�P�b�g�ڂ��`�F�b�N
							map<WORD, CPMTUtil>::iterator itrPmt;
							itrPmt = this->pmtUtilMap.find(packet.PID);
							if( itrPmt != this->pmtUtilMap.end() ){
								if( itrPmt->second.AddPacket(&packet) == TRUE ){
									CheckNeedPID();
								}
							}
						}
					}

					//�f�R�[�h�p�̃o�b�t�@�쐬
					if( this->serviceOnlyFlag == FALSE ){
						//�S�T�[�r�X
						this->decodeBuff.insert(this->decodeBuff.end(), data + i, data + i + 188);
					}else{
						//�w��T�[�r�X
						if( IsNeedPID(&packet) == TRUE ){
							if( packet.PID == 0x0000 ){
								//PAT�Ȃ̂ŕK�v�ȃT�[�r�X�݂̂ɍi��
								BYTE* patBuff = NULL;
								DWORD patBuffSize = 0;
								if( patUtil.GetPacket(&patBuff, &patBuffSize) == TRUE ){
									if( packet.payload_unit_start_indicator == 1 ){
										this->decodeBuff.insert(this->decodeBuff.end(), patBuff, patBuff + patBuffSize);
									}
								}
							}else{
								this->decodeBuff.insert(this->decodeBuff.end(), data + i, data + i + 188);
							}
						}
					}
					if( this->epgFile != NULL ){
						DWORD write;
						if( packet.PID == 0 && packet.payload_unit_start_indicator ){
							if( this->epgFileState == EPG_FILE_ST_NONE ){
								this->epgFileState = EPG_FILE_ST_PAT;
							}else if( this->epgFileState == EPG_FILE_ST_PAT ){
								this->epgFileState = EPG_FILE_ST_TOT;
								//�ԑg��񂪕s�����Ȃ��悤���߂Ē~�Ϗ�Ԃ����Z�b�g
								this->epgUtil.ClearSectionStatus();
								//TOT��O�|���ŏ������ނ��߂̏ꏊ���m��
								BYTE nullData[188] = { 0x47, 0x1F, 0xFF, 0x10 };
								memset(nullData + 4, 0xFF, 184);
								this->epgFileTotPos = SetFilePointer(this->epgFile, 0, NULL, FILE_CURRENT);
								WriteFile(this->epgFile, nullData, 188, &write, NULL);
							}
						}
						//�܂�PAT�A����(�����)TOT���������ށB���̏����͕K�{�ł͂Ȃ����ԑg�������m���������I�ɓǂݏo����
						if( packet.PID == 0x14 && this->epgFileState == EPG_FILE_ST_TOT ){
							this->epgFileState = EPG_FILE_ST_ALL;
							if( this->epgFileTotPos != INVALID_SET_FILE_POINTER ){
								SetFilePointer(this->epgFile, this->epgFileTotPos, NULL, FILE_BEGIN);
							}
							WriteFile(this->epgFile, data + i, 188, &write, NULL);
							LONG posHigh = 0;
							SetFilePointer(this->epgFile, 0, &posHigh, FILE_END);
						}else if( packet.PID == 0 && this->epgFileState >= EPG_FILE_ST_PAT || packet.PID <= 0x30 && this->epgFileState >= EPG_FILE_ST_TOT ){
							WriteFile(this->epgFile, data + i, 188, &write, NULL);
						}
					}
				}
			}else{
				if( this->chChangeFlag == TRUE ){
					if( GetTickCount() - this->chChangeTime > 15000 ){
						//15�b�ȏソ���Ă�Ȃ�؂�ւ�����Ƃ���
						//OutputDebugString(L"��Ch Change Err NoPacket\r\n");
						//this->chChangeFlag = FALSE;
						this->chChangeErr = TRUE;
						this->lastONID = 0xFFFF;
						this->lastTSID = 0xFFFF;
						//this->epgUtil.ClearSectionStatus();
						//this->decodeUtil.SetNetwork(onid, tsid);
						//this->decodeUtil.SetEmm(this->emmEnableFlag);
						//ResetErrCount();
					}
				}
				continue;
			}
		}
		if( this->chChangeFlag == FALSE ){
			WORD onid = 0xFFFF;
			WORD tsid = 0xFFFF;
			if( this->epgUtil.GetTSID(&onid, &tsid) == NO_ERR ){
				if( onid != this->lastONID || tsid != this->lastTSID ){
					OutputDebugString(L"��UnKnown Ch Change \r\n");
					_OutputDebugString(L"��Ch 0x%04X 0x%04X => 0x%04X 0x%04X\r\n", this->lastONID, this->lastTSID, onid, tsid);
					OnChChanged(onid, tsid);

				}
			}
		}
	}
	try{
		if( this->decodeBuff.empty() == false ){
			if( this->enableDecodeFlag == TRUE && this->chChangeFlag == FALSE ){
				//�f�R�[�h�K�v

				if( decodeUtil.Decode(&this->decodeBuff.front(), (DWORD)this->decodeBuff.size(), &decodeData, &decodeSize) == FALSE ){
					//�f�R�[�h���s
					decodeData = &this->decodeBuff.front();
					decodeSize = (DWORD)this->decodeBuff.size();
				}else{
					if( decodeData == NULL || decodeSize == 0 ){
						decodeData = NULL;
						decodeSize = 0;
					}
				}
			}else{
				//�f�R�[�h�̕K�v�Ȃ�
				decodeData = &this->decodeBuff.front();
				decodeSize = (DWORD)this->decodeBuff.size();
			}
		}
	}catch(...){
		_OutputDebugString(L"����CTSOut::AddTSBuff Exception2");
		//�f�R�[�h���s
		decodeData = &this->decodeBuff.front();
		decodeSize = (DWORD)this->decodeBuff.size();
	}
	
	//�f�R�[�h�ς݂̃f�[�^����͂�����
	{
		for( DWORD i=0; i<decodeSize; i+=188 ){
			this->epgUtil.AddTSPacket(decodeData + i, 188);
		}
	}

	//�e�T�[�r�X�����Ƀf�[�^�n��
	{
		map<DWORD, COneServiceUtil*>::iterator itrService;
		for( itrService = serviceUtilMap.begin(); itrService != serviceUtilMap.end(); itrService++ ){
			itrService->second->AddTSBuff(decodeData, decodeSize);
		}
	}
	UnLock();
	return NO_ERR;
}

void CTSOut::CheckNeedPID()
{
	this->needPIDMap.clear();
	this->serviceOnlyFlag = TRUE;
	//PAT�쐬�p��PMT���X�g
	map<WORD, CCreatePATPacket::PROGRAM_PID_INFO> PIDMap;
	//NIT��PID�ǉ����Ă���
	PIDMap[0x10].PMTPID = 0x10;
	PIDMap[0x10].SID = 0;

	map<WORD, string> pidName;
	map<WORD, CPMTUtil>::iterator itrPmt;
	for( itrPmt = pmtUtilMap.begin(); itrPmt != pmtUtilMap.end(); itrPmt++ ){
		string name = "";
		Format(name, "PMT(ServiceID 0x%04X)", itrPmt->second.program_number);
		pidName.insert(pair<WORD, string>(itrPmt->first, name));
		map<WORD, WORD>::iterator itrPID;
		for( itrPID = itrPmt->second.PIDList.begin(); itrPID != itrPmt->second.PIDList.end(); itrPID++ ){
			switch(itrPID->second){
			case 0x00:
				name = "ECM";
				break;
			case 0x02:
				name = "MPEG2 VIDEO";
				break;
			case 0x0F:
				name = "MPEG2 AAC";
				break;
			case 0x1B:
				name = "MPEG4 VIDEO";
				break;
			case 0x04:
				name = "MPEG2 AUDIO";
				break;
			case 0x24:
				name = "HEVC VIDEO";
				break;
			case 0x06:
				name = "����";
				break;
			case 0x0D:
				name = "�f�[�^�J���[�Z��";
				break;
			default:
				Format(name, "stream_type 0x%0X", itrPID->second);
				break;
			}
			pidName.insert(pair<WORD, string>(itrPID->first, name));
		}
		pidName.insert(pair<WORD, string>(itrPmt->second.PCR_PID, "PCR"));
		
	}

	//EMM��PID
	if( catUtil != NULL ){
		map<WORD,WORD>::iterator itrPID;
		for( itrPID = catUtil->PIDList.begin(); itrPID != catUtil->PIDList.end(); itrPID++ ){
			this->needPIDMap.insert(pair<WORD,WORD>(itrPID->first, itrPID->second));
			pidName.insert(pair<WORD, string>(itrPID->first, "EMM"));
		}
	}


	//�e�T�[�r�X��PMT��T��
	map<DWORD, COneServiceUtil*>::iterator itrService;
	for( itrService = serviceUtilMap.begin(); itrService != serviceUtilMap.end(); itrService++ ){
		if( itrService->second->GetSID() == 0xFFFF ){
			//�S�T�[�r�X�Ώ�
			this->serviceOnlyFlag = FALSE;
			for( itrPmt = pmtUtilMap.begin(); itrPmt != pmtUtilMap.end(); itrPmt++ ){
				//PAT�쐬�p��PMT���X�g�쐬
				CCreatePATPacket::PROGRAM_PID_INFO item;
				item.PMTPID = itrPmt->first;
				item.SID = itrPmt->second.program_number;
				PIDMap.insert(pair<WORD, CCreatePATPacket::PROGRAM_PID_INFO>(item.PMTPID,item));

				//PMT�L�ڂ�PID��o�^
				this->needPIDMap.insert(pair<WORD,WORD>(itrPmt->first, 0));
				map<WORD,WORD>::iterator itrPID;
				for( itrPID = itrPmt->second.PIDList.begin(); itrPID != itrPmt->second.PIDList.end(); itrPID++ ){
					this->needPIDMap.insert(pair<WORD,WORD>(itrPID->first, itrPID->second));
				}
			}
		}else{
			for( itrPmt = pmtUtilMap.begin(); itrPmt != pmtUtilMap.end(); itrPmt++ ){
				if( itrService->second->GetSID() == itrPmt->second.program_number ){
					//PMT����
					itrService->second->SetPmtPID(this->lastTSID, itrPmt->first);
					if( catUtil != NULL ){
						itrService->second->SetEmmPID(&catUtil->PIDList);
					}


					//PAT�쐬�p��PMT���X�g�쐬
					CCreatePATPacket::PROGRAM_PID_INFO item2;
					item2.PMTPID = itrPmt->first;
					item2.SID = itrPmt->second.program_number;
					PIDMap.insert(pair<WORD, CCreatePATPacket::PROGRAM_PID_INFO>(item2.PMTPID,item2));
					//_OutputDebugString(L"0x%04x, 0x%04x", itrPmt->first,itrPmt->second->program_number);
					//PMT�L�ڂ�PID��o�^
					this->needPIDMap.insert(pair<WORD,WORD>(itrPmt->first, 0));
					this->needPIDMap.insert(pair<WORD,WORD>(itrPmt->second.PCR_PID, 0));
					map<WORD,WORD>::iterator itrPID;
					for( itrPID = itrPmt->second.PIDList.begin(); itrPID != itrPmt->second.PIDList.end(); itrPID++ ){
						this->needPIDMap.insert(pair<WORD,WORD>(itrPID->first, itrPID->second));
					}
				}
			}
		}
		itrService->second->SetPIDName(&pidName);
	}
	this->patUtil.SetParam(this->lastTSID, &PIDMap);

}

BOOL CTSOut::IsNeedPID(CTSPacketUtil* packet)
{
	if( packet == NULL ){
		return FALSE;
	}
	if( packet->PID <= 0x30 ){
		return TRUE;
	}
	map<WORD,WORD>::iterator itrPID;
	itrPID = this->needPIDMap.find(packet->PID);
	if( itrPID != this->needPIDMap.end() ){
		return TRUE;
	}

	return FALSE;
}

//EPG�f�[�^�̕ۑ����J�n����
//�߂�l�F
// TRUE�i�����j�AFALSE�i���s�j
BOOL CTSOut::StartSaveEPG(
	wstring epgFilePath
	)
{
	if( Lock(L"StartSaveEPG") == FALSE ) return FALSE;
	if( this->epgFile != NULL ){
		UnLock();
		return FALSE;
	}
	this->epgFilePath = epgFilePath;
	this->epgTempFilePath = epgFilePath;
	this->epgTempFilePath += L".tmp";

	_OutputDebugString(L"��%s\r\n", this->epgFilePath.c_str());
	_OutputDebugString(L"��%s\r\n", this->epgTempFilePath.c_str());

	this->epgUtil.ClearSectionStatus();
	this->epgFileState = EPG_FILE_ST_NONE;

	BOOL ret = TRUE;
	this->epgFile = _CreateDirectoryAndFile(this->epgTempFilePath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if( this->epgFile == INVALID_HANDLE_VALUE ){
		this->epgFile = NULL;
		ret = FALSE;
		OutputDebugString(L"err\r\n");
	}

	UnLock();
	return ret;
}

//EPG�f�[�^�̕ۑ����I������
//�߂�l�F
// TRUE�i�����j�AFALSE�i���s�j
BOOL CTSOut::StopSaveEPG(
	BOOL copy
	)
{
	if( Lock(L"StopSaveEPG") == FALSE ) return FALSE;
	if( this->epgFile == NULL ){
		UnLock();
		return FALSE;
	}

	CloseHandle(this->epgFile);
	this->epgFile = NULL;

	if( copy == TRUE ){
		CopyFile(this->epgTempFilePath.c_str(), this->epgFilePath.c_str(), FALSE );
	}
	DeleteFile(this->epgTempFilePath.c_str());

	this->epgFilePath = L"";
	this->epgTempFilePath = L"";

	UnLock();
	return TRUE;
}

//EPG�f�[�^�̒~�Ϗ�Ԃ��擾����
//�߂�l�F
// �X�e�[�^�X
//�����F
// l_eitFlag		[IN]L-EIT�̃X�e�[�^�X���擾
EPG_SECTION_STATUS CTSOut::GetSectionStatus(
	BOOL l_eitFlag
	)
{
	if( Lock(L"GetSectionStatus") == FALSE ) return EpgNoData;

	EPG_SECTION_STATUS status = this->epgUtil.GetSectionStatus(l_eitFlag);

	UnLock();
	return status;
}

//�w��T�[�r�X��EPG�f�[�^�̒~�Ϗ�Ԃ��擾����
pair<EPG_SECTION_STATUS, BOOL> CTSOut::GetSectionStatusService(
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	BOOL l_eitFlag
	)
{
	if( Lock(L"GetSectionStatusService") == FALSE ) return pair<EPG_SECTION_STATUS, BOOL>(EpgNoData, FALSE);

	pair<EPG_SECTION_STATUS, BOOL> status = this->epgUtil.GetSectionStatusService(originalNetworkID, transportStreamID, serviceID, l_eitFlag);

	UnLock();
	return status;
}

//EMM�����̓���ݒ�
//�߂�l�F
// TRUE�i�����j�AFALSE�i���s�j
//�����F
// enable		[IN] TRUE�i��������j�AFALSE�i�������Ȃ��j
BOOL CTSOut::SetEmm(
	BOOL enable
	)
{
	if( Lock(L"SetEmm") == FALSE ) return FALSE;

	try{
		if( this->lastONID != 0xFFFF && this->lastTSID != 0xFFFF ){
			//�`���[�j���O�ς݂�
			if( enable != FALSE && this->enableDecodeFlag == FALSE && this->emmEnableFlag == FALSE ){
				//�ŏ��� EMM �������ݒ肳���ꍇ�� DLL ��ǂݍ���
				//�X�N�����u���������ݒ肳��Ă���ꍇ�͓ǂݍ��ݍς݂Ȃ̂ŏ��O
				if( this->decodeUtil.SetNetwork(this->lastONID, this->lastTSID) == FALSE ){
					OutputDebugString(L"����Decode DLL load err [CTSOut::SetEmm()]\r\n");
				}
			}
		}
	}catch(...){
		UnLock();
		return FALSE;
	}

	BOOL err = this->decodeUtil.SetEmm(enable);
	this->emmEnableFlag = enable;

	UnLock();
	return err;
}

//EMM�������s������
//�߂�l�F
// ������
DWORD CTSOut::GetEmmCount()
{
	if( Lock(L"GetEmmCount") == FALSE ) return 0;

	DWORD count = this->decodeUtil.GetEmmCount();

	UnLock();
	return count;
}

//DLL�̃��[�h��Ԃ��擾
//�߂�l�F
// TRUE�i���[�h�ɐ������Ă���j�AFALSE�i���[�h�Ɏ��s���Ă���j
//�����F
// loadErrDll		[OUT]���[�h�Ɏ��s����DLL�t�@�C����
BOOL CTSOut::GetLoadStatus(
	wstring& loadErrDll
	)
{
	if( Lock(L"GetLoadStatus") == FALSE ) return 0;

	BOOL err = this->decodeUtil.GetLoadStatus(loadErrDll);

	UnLock();
	return err;
}

//���X�g���[���̃T�[�r�X�ꗗ���擾����
//�߂�l�F
// �G���[�R�[�h
//�����F
// serviceListSize			[OUT]serviceList�̌�
// serviceList				[OUT]�T�[�r�X���̃��X�g�iDLL���Ŏ����I��delete����B���Ɏ擾���s���܂ŗL���j
DWORD CTSOut::GetServiceListActual(
	DWORD* serviceListSize,
	SERVICE_INFO** serviceList
	)
{
	if( Lock(L"GetServiceListActual") == FALSE ) return 0;

	DWORD err = this->epgUtil.GetServiceListActual(serviceListSize, serviceList);

	UnLock();
	return err;
}

//���Ɏg�p���鐧��ID���擾����
//�߂�l�F
// id
DWORD CTSOut::GetNextID()
{
	DWORD nextID = 0xFFFFFFFF;

	map<DWORD, COneServiceUtil*>::iterator itr;
	itr = this->serviceUtilMap.find(this->nextCtrlID);
	if( itr == this->serviceUtilMap.end() ){
		//���݂��Ȃ�ID�Ȃ̂ł��̂܂܎g�p
		nextID = this->nextCtrlID;
		this->nextCtrlID++;
		if( this->nextCtrlID == 0 || this->nextCtrlID == 0xFFFFFFFF){
			this->nextCtrlID = 1;
		}
	}else{
		//��������H
		for( DWORD i=1; i<0xFFFFFFFF; i++ ){
			//�P���珇�Ԃɑ��݂��Ȃ�ID���m�F
			itr = this->serviceUtilMap.find(this->nextCtrlID);
			if( itr == this->serviceUtilMap.end() ){
				nextID = this->nextCtrlID;
				this->nextCtrlID++;
				if( this->nextCtrlID == 0 || this->nextCtrlID == 0xFFFFFFFF){
					this->nextCtrlID = 1;
				}
				break;
			}else{
				this->nextCtrlID++;
			}
			if( this->nextCtrlID == 0 || this->nextCtrlID == 0xFFFFFFFF){
				this->nextCtrlID = 1;
			}
		}
	}

	return nextID;
}

//TS�X�g���[������p�R���g���[�����쐬����
//�߂�l�F
// �G���[�R�[�h
//�����F
// id			[OUT]���䎯��ID
BOOL CTSOut::CreateServiceCtrl(
	DWORD* id
	)
{
	if( Lock(L"CreateServiceCtrl") == FALSE ) return FALSE;

	COneServiceUtil* serviceUtil = new COneServiceUtil;
	*id = GetNextID();

	serviceUtil->SetEpgUtil(&this->epgUtil);
	serviceUtil->SetBonDriver(bonFile);

	serviceUtilMap.insert(pair<DWORD, COneServiceUtil*>(*id, serviceUtil));

	UnLock();
	return TRUE;
}

//TS�X�g���[������p�R���g���[�����쐬����
//�߂�l�F
// �G���[�R�[�h
//�����F
// id			[IN]���䎯��ID
BOOL CTSOut::DeleteServiceCtrl(
	DWORD id
	)
{
	if( Lock(L"DeleteServiceCtrl") == FALSE ) return FALSE;

	map<DWORD, COneServiceUtil*>::iterator itr;
	itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		UnLock();
		return FALSE;
	}

	SAFE_DELETE(itr->second);
	serviceUtilMap.erase(itr);

	CheckNeedPID();

	UnLock();
	return TRUE;
}

//����Ώۂ̃T�[�r�X��ݒ肷��
//�߂�l�F
// TRUE�i�����j�AFALSE�i���s
//�����F
// id			[IN]���䎯��ID
// serviceID	[IN]�ΏۃT�[�r�XID
BOOL CTSOut::SetServiceID(
	DWORD id,
	WORD serviceID
	)
{
	if( Lock(L"SetServiceID") == FALSE ) return FALSE;

	map<DWORD, COneServiceUtil*>::iterator itr;
	itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		UnLock();
		return FALSE;
	}

	itr->second->SetSID(serviceID);
	CheckNeedPID();

	UnLock();
	return TRUE;
}

BOOL CTSOut::GetServiceID(
	DWORD id,
	WORD* serviceID
	)
{
	if( Lock(L"GetServiceID") == FALSE ) return FALSE;

	map<DWORD, COneServiceUtil*>::iterator itr;
	itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		UnLock();
		return FALSE;
	}
	if( serviceID != NULL ){
		*serviceID = itr->second->GetSID();
	}

	UnLock();
	return TRUE;
}

//UDP�ő��M���s��
//�߂�l�F
// TRUE�i�����j�AFALSE�i���s�j
//�����F
// id			[IN]���䎯��ID
// sendList		[IN/OUT]���M�惊�X�g�BNULL�Œ�~�BPort�͎��ۂɑ��M�Ɏg�p����Port���Ԃ�B
BOOL CTSOut::SendUdp(
	DWORD id,
	vector<NW_SEND_INFO>* sendList
	)
{
	if( Lock(L"SendUdp") == FALSE ) return FALSE;

	map<DWORD, COneServiceUtil*>::iterator itr;
	itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		UnLock();
		return FALSE;
	}

	itr->second->SendUdp(sendList);

	UnLock();
	return TRUE;
}

//TCP�ő��M���s��
//�߂�l�F
// TRUE�i�����j�AFALSE�i���s�j
//�����F
// id			[IN]���䎯��ID
// sendList		[IN/OUT]���M�惊�X�g�BNULL�Œ�~�BPort�͎��ۂɑ��M�Ɏg�p����Port���Ԃ�B
BOOL CTSOut::SendTcp(
	DWORD id,
	vector<NW_SEND_INFO>* sendList
	)
{
	if( Lock(L"SendTcp") == FALSE ) return FALSE;

	map<DWORD, COneServiceUtil*>::iterator itr;
	itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		UnLock();
		return FALSE;
	}

	itr->second->SendTcp(sendList);

	UnLock();
	return TRUE;
}

//�w��T�[�r�X�̌���or����EPG�����擾����
//�߂�l�F
// �G���[�R�[�h
//�����F
// originalNetworkID		[IN]�擾�Ώۂ�originalNetworkID
// transportStreamID		[IN]�擾�Ώۂ�transportStreamID
// serviceID				[IN]�擾�Ώۂ�ServiceID
// nextFlag					[IN]TRUE�i���̔ԑg�j�AFALSE�i���݂̔ԑg�j
// epgInfo					[OUT]EPG���iDLL���Ŏ����I��delete����B���Ɏ擾���s���܂ŗL���j
DWORD CTSOut::GetEpgInfo(
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	BOOL nextFlag,
	EPGDB_EVENT_INFO* epgInfo
	)
{
	if( Lock(L"GetEpgInfo") == FALSE ) return FALSE;

	EPG_EVENT_INFO* _epgInfo;
	DWORD err = this->epgUtil.GetEpgInfo(originalNetworkID, transportStreamID, serviceID, nextFlag, &_epgInfo);
	if( err == NO_ERR ){
		ConvertEpgInfo(originalNetworkID, transportStreamID, serviceID, _epgInfo, epgInfo);
	}

	UnLock();
	return err;
}

//�w��C�x���g��EPG�����擾����
//�߂�l�F
// �G���[�R�[�h
//�����F
// originalNetworkID		[IN]�擾�Ώۂ�originalNetworkID
// transportStreamID		[IN]�擾�Ώۂ�transportStreamID
// serviceID				[IN]�擾�Ώۂ�ServiceID
// eventID					[IN]�擾�Ώۂ�EventID
// pfOnlyFlag				[IN]p/f����̂݌������邩�ǂ���
// epgInfo					[OUT]EPG���iDLL���Ŏ����I��delete����B���Ɏ擾���s���܂ŗL���j
DWORD CTSOut::SearchEpgInfo(
	WORD originalNetworkID,
	WORD transportStreamID,
	WORD serviceID,
	WORD eventID,
	BYTE pfOnlyFlag,
	EPGDB_EVENT_INFO* epgInfo
	)
{
	if( Lock(L"SearchEpgInfo") == FALSE ) return FALSE;

	EPG_EVENT_INFO* _epgInfo;
	DWORD err = this->epgUtil.SearchEpgInfo(originalNetworkID, transportStreamID, serviceID, eventID, pfOnlyFlag, &_epgInfo);
	if( err == NO_ERR ){
		ConvertEpgInfo(originalNetworkID, transportStreamID, serviceID, _epgInfo, epgInfo);
	}

	UnLock();
	return err;
}

//PC���v�����Ƃ����X�g���[�����ԂƂ̍����擾����
//�߂�l�F
// ���̕b��
int CTSOut::GetTimeDelay(
	)
{
	if( Lock(L"GetTimeDelay") == FALSE ) return 0;

	int delay = this->epgUtil.GetTimeDelay();

	UnLock();
	return delay;
}

//�^�撆���ǂ���
//�߂�l�F
// TRUE�i�^�撆�j�AFALSE�i���Ă��Ȃ��j
BOOL CTSOut::IsRec()
{
	if( Lock(L"IsRec") == FALSE ) return FALSE;

	BOOL ret = FALSE;
	map<DWORD, COneServiceUtil*>::iterator itr;
	for( itr = this->serviceUtilMap.begin(); itr != this->serviceUtilMap.end(); itr++ ){
		if( itr->second->IsRec() == TRUE ){
			ret = TRUE;
			break;
		}
	}

	UnLock();
	return ret;
}

//�t�@�C���ۑ����J�n����
//�߂�l�F
// TRUE�i�����j�AFALSE�i���s�j
//�����F
// id					[IN]���䎯��ID
// fileName				[IN]�ۑ��t�@�C���p�X
// overWriteFlag		[IN]����t�@�C�������ݎ��ɏ㏑�����邩�ǂ����iTRUE�F����AFALSE�F���Ȃ��j
// pittariFlag			[IN]�҂����胂�[�h�iTRUE�F����AFALSE�F���Ȃ��j
// pittariONID			[IN]�҂����胂�[�h�Ř^�悷��ONID
// pittariTSID			[IN]�҂����胂�[�h�Ř^�悷��TSID
// pittariSID			[IN]�҂����胂�[�h�Ř^�悷��SID
// pittariEventID		[IN]�҂����胂�[�h�Ř^�悷��C�x���gID
// createSize			[IN]�t�@�C���쐬���Ƀf�B�X�N�ɗ\�񂷂�e��
// saveFolder			[IN]�g�p����t�H���_�ꗗ
// saveFolderSub		[IN]HDD�̋󂫂��Ȃ��Ȃ����ꍇ�Ɉꎞ�I�Ɏg�p����t�H���_
BOOL CTSOut::StartSave(
	DWORD id,
	wstring fileName,
	BOOL overWriteFlag,
	BOOL pittariFlag,
	WORD pittariONID,
	WORD pittariTSID,
	WORD pittariSID,
	WORD pittariEventID,
	ULONGLONG createSize,
	vector<REC_FILE_SET_INFO>* saveFolder,
	vector<wstring>* saveFolderSub,
	int maxBuffCount
)
{
	if( Lock(L"StartSave") == FALSE ) return FALSE;

	map<DWORD, COneServiceUtil*>::iterator itr;
	itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		UnLock();
		return FALSE;
	}

	BOOL ret = itr->second->StartSave(fileName, overWriteFlag, pittariFlag, pittariONID, pittariTSID, pittariSID, pittariEventID, createSize, saveFolder, saveFolderSub, maxBuffCount);
	UnLock();
	return ret;
}

//�t�@�C���ۑ����I������
//�߂�l�F
// TRUE�i�����j�AFALSE�i���s�j
//�����F
// id			[IN]���䎯��ID
BOOL CTSOut::EndSave(
	DWORD id
	)
{
	if( Lock(L"EndSave") == FALSE ) return FALSE;
	BOOL ret = TRUE;
	map<DWORD, COneServiceUtil*>::iterator itr;
	itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		UnLock();
		return FALSE;
	}

	ret = itr->second->EndSave();

	UnLock();
	return ret;
}

//�X�N�����u�����������̓���ݒ�
//�߂�l�F
// TRUE�i�����j�AFALSE�i���s�j
//�����F
// enable		[IN] TRUE�i��������j�AFALSE�i�������Ȃ��j
BOOL CTSOut::SetScramble(
	DWORD id,
	BOOL enable
	)
{
	if( Lock(L"SetScramble") == FALSE ) return FALSE;

	try{
		if( this->lastONID != 0xFFFF && this->lastTSID != 0xFFFF ){
			//�`���[�j���O�ς݂�
			if( enable != FALSE && this->enableDecodeFlag == FALSE && this->emmEnableFlag == FALSE ){
				//�ŏ��ɃX�N�����u���������ݒ肳���ꍇ�� DLL ���ēǂݍ��݂���
				//EMM �������ݒ肳��Ă���ꍇ�͓ǂݍ��ݍς݂Ȃ̂ŏ��O
				if( this->decodeUtil.SetNetwork(this->lastONID, this->lastTSID) == FALSE ){
					OutputDebugString(L"����Decode DLL load err [CTSOut::SetScramble()]\r\n");
				}
			}
		}
	}catch(...){
		UnLock();
		return FALSE;
	}

	map<DWORD, COneServiceUtil*>::iterator itr;
	itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		UnLock();
		return FALSE;
	}

	itr->second->SetScramble(enable);

	BOOL enableScramble = FALSE;
	for( itr = this->serviceUtilMap.begin(); itr != this->serviceUtilMap.end(); itr++ ){
		if( itr->second->GetScramble() == TRUE ){
			enableScramble = TRUE;
			break;
		}
	}

	this->enableDecodeFlag = enableScramble;

	UnLock();
	return TRUE;
}

//�����ƃf�[�^�����܂߂邩�ǂ���
//�����F
// id					[IN]���䎯��ID
// enableCaption		[IN]������ TRUE�i�܂߂�j�AFALSE�i�܂߂Ȃ��j
// enableData			[IN]�f�[�^������ TRUE�i�܂߂�j�AFALSE�i�܂߂Ȃ��j
void CTSOut::SetServiceMode(
	DWORD id,
	BOOL enableCaption,
	BOOL enableData
	)
{
	if( Lock(L"SetServiceMode") == FALSE ) return ;

	map<DWORD, COneServiceUtil*>::iterator itr;
	itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		UnLock();
		return ;
	}

	itr->second->SetServiceMode(enableCaption, enableData);

	UnLock();
}

//�G���[�J�E���g���N���A����
//�����F
// id				[IN]���䎯��ID
void CTSOut::ClearErrCount(
	DWORD id
	)
{
	if( Lock(L"ClearErrCount") == FALSE ) return ;

	map<DWORD, COneServiceUtil*>::iterator itr;
	itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		UnLock();
		return ;
	}

	itr->second->ClearErrCount();

	UnLock();
}

//�h���b�v�ƃX�N�����u���̃J�E���g���擾����
//�����F
// id				[IN]���䎯��ID
// drop				[OUT]�h���b�v��
// scramble			[OUT]�X�N�����u����
void CTSOut::GetErrCount(
	DWORD id,
	ULONGLONG* drop,
	ULONGLONG* scramble
	)
{
	if( Lock(L"GetErrCount") == FALSE ) return ;

	map<DWORD, COneServiceUtil*>::iterator itr;
	itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		UnLock();
		return ;
	}

	itr->second->GetErrCount(drop, scramble);

	UnLock();
}


//�^�撆�̃t�@�C���̏o�̓T�C�Y���擾����
//�����F
// id					[IN]���䎯��ID
// writeSize			[OUT]�ۑ��t�@�C����
void CTSOut::GetRecWriteSize(
	DWORD id,
	__int64* writeSize
	)
{
	if( Lock(L"GetRecWriteSize") == FALSE ) return ;

	map<DWORD, COneServiceUtil*>::iterator itr;
	itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		UnLock();
		return ;
	}

	itr->second->GetRecWriteSize(writeSize);

	UnLock();
}

void CTSOut::ResetErrCount()
{
	map<DWORD, COneServiceUtil*>::iterator itr;
	for( itr = serviceUtilMap.begin(); itr != serviceUtilMap.end(); itr++ ){
		itr->second->ClearErrCount();
	}
}

//�^�撆�̃t�@�C���̃t�@�C���p�X���擾����
//�����F
// id					[IN]���䎯��ID
// filePath				[OUT]�ۑ��t�@�C����
// subRecFlag			[OUT]�T�u�^�悪�����������ǂ���
void CTSOut::GetSaveFilePath(
	DWORD id,
	wstring* filePath,
	BOOL* subRecFlag
	)
{
	if( Lock(L"GetSaveFilePath") == FALSE ) return ;

	map<DWORD, COneServiceUtil*>::iterator itr;
	itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		UnLock();
		return ;
	}

	itr->second->GetSaveFilePath(filePath, subRecFlag);

	UnLock();
}

//�h���b�v�ƃX�N�����u���̃J�E���g��ۑ�����
//�����F
// id					[IN]���䎯��ID
// filePath				[IN]�ۑ��t�@�C����
void CTSOut::SaveErrCount(
	DWORD id,
	wstring filePath
	)
{
	if( Lock(L"SaveErrCount") == FALSE ) return ;

	map<DWORD, COneServiceUtil*>::iterator itr;
	itr = serviceUtilMap.find(id);
	if( itr == serviceUtilMap.end() ){
		UnLock();
		return ;
	}

	itr->second->SaveErrCount(filePath);

	UnLock();
}

void CTSOut::SetSignalLevel(
	float signalLv
	)
{
	if( Lock(L"SetSignalLevel") == FALSE ) return ;

	map<DWORD, COneServiceUtil*>::iterator itr;
	for( itr = serviceUtilMap.begin(); itr != serviceUtilMap.end(); itr++ ){
		itr->second->SetSignalLevel(signalLv);
	}

	UnLock();
}


void CTSOut::SetBonDriver(
	wstring bonDriver
	)
{
	if( Lock(L"SetBonDriver") == FALSE ) return ;

	map<DWORD, COneServiceUtil*>::iterator itr;
	for( itr = serviceUtilMap.begin(); itr != serviceUtilMap.end(); itr++ ){
		itr->second->SetBonDriver(bonDriver);
	}
	bonFile = bonDriver;

	UnLock();
}

