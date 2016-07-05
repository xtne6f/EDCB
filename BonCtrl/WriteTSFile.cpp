#include "StdAfx.h"
#include "WriteTSFile.h"
#include <process.h>

#include "../Common/PathUtil.h"
#include "../Common/BlockLock.h"

CWriteTSFile::CWriteTSFile(void)
{
	InitializeCriticalSection(&this->outThreadLock);

    this->outThread = NULL;
    this->outStopFlag = FALSE;
	this->outStartFlag = FALSE;
	this->overWriteFlag = FALSE;
	this->createSize = 0;
	this->subRecFlag = FALSE;
	this->writeTotalSize = 0;
	this->maxBuffCount = -1;
}

CWriteTSFile::~CWriteTSFile(void)
{
	EndSave();
	DeleteCriticalSection(&this->outThreadLock);
}

//�t�@�C���ۑ����J�n����
//�߂�l�F
// TRUE�i�����j�AFALSE�i���s�j
//�����F
// fileName				[IN]�ۑ��t�@�C����
// overWriteFlag		[IN]����t�@�C�������ݎ��ɏ㏑�����邩�ǂ����iTRUE�F����AFALSE�F���Ȃ��j
// createSize			[IN]�t�@�C���쐬���Ƀf�B�X�N�ɗ\�񂷂�e��
// saveFolder			[IN]�g�p����t�H���_�ꗗ
// saveFolderSub		[IN]HDD�̋󂫂��Ȃ��Ȃ����ꍇ�Ɉꎞ�I�Ɏg�p����t�H���_
BOOL CWriteTSFile::StartSave(
	wstring fileName,
	BOOL overWriteFlag,
	ULONGLONG createSize,
	vector<REC_FILE_SET_INFO>* saveFolder,
	vector<wstring>* saveFolderSub,
	int maxBuffCount
)
{
	if( saveFolder->size() == 0 ){
		OutputDebugString(L"CWriteTSFile::StartSave Err saveFolder 0\r\n");
		return FALSE;
	}
	
	if( this->outThread == NULL ){
		this->fileList.clear();
		this->mainSaveFilePath = L"";
		this->overWriteFlag = overWriteFlag;
		this->createSize = createSize;
		this->maxBuffCount = maxBuffCount;
		this->writeTotalSize = 0;
		this->subRecFlag = FALSE;
		vector<REC_FILE_SET_INFO> saveFolder_ = *saveFolder;
		this->saveFolderSub = *saveFolderSub;
		for( size_t i=0; i<saveFolder_.size(); i++ ){
			SAVE_INFO item;
			item.writeUtil = NULL;
			item.freeChk = FALSE;
			item.writePlugIn = saveFolder_[i].writePlugIn;
			if( item.writePlugIn.size() == 0 ){
				item.writePlugIn = L"Write_Default.dll";
			}
			item.recFolder = saveFolder_[i].recFolder;
			item.recFileName = saveFolder_[i].recFileName;
			if( item.recFileName.size() == 0 ){
				item.recFileName = fileName;
			}
			this->fileList.push_back(item);
		}

		//��M�X���b�h�N��
		this->outStopFlag = FALSE;
		this->outStartFlag = FALSE;
		this->outThread = (HANDLE)_beginthreadex(NULL, 0, OutThread, this, 0, NULL);
		if( this->outThread ){
			//�ۑ��J�n�܂ő҂�
			while( WaitForSingleObject(this->outThread, 10) == WAIT_TIMEOUT && this->outStartFlag == FALSE );
			if( this->outStartFlag ){
				return TRUE;
			}
			CloseHandle(this->outThread);
			this->outThread = NULL;
		}
	}

	OutputDebugString(L"CWriteTSFile::StartSave Err 1\r\n");
	return FALSE;
}

//�ۑ��T�u�t�H���_����󂫂̂���t�H���_�p�X���擾
//�߂�l�F
// TRUE�i�����j�AFALSE�i���s�j
//�����F
// needFreeSize			[IN]�Œ�K�v�ȋ󂫃T�C�Y
// freeFolderPath		[OUT]���������t�H���_
BOOL CWriteTSFile::GetFreeFolder(
	ULONGLONG needFreeSize,
	wstring& freeFolderPath
)
{
	BOOL ret = FALSE;

	for( int i = 0; i < (int)this->saveFolderSub.size(); i++ ){
		ULARGE_INTEGER stFree;
		if( _GetDiskFreeSpaceEx( this->saveFolderSub[i].c_str(), &stFree, NULL, NULL ) != FALSE ){
			if( stFree.QuadPart > needFreeSize ){
				freeFolderPath = this->saveFolderSub[i];
				ChkFolderPath(freeFolderPath);
				ret = TRUE;
				break;
			}
		}
	}
	return ret;
}

//�w��t�H���_�̋󂫂����邩�`�F�b�N
//�߂�l�F
// TRUE�i�����j�AFALSE�i���s�j
//�����F
// needFreeSize			[IN]�Œ�K�v�ȋ󂫃T�C�Y
// chkFolderPath		[IN]�w��t�H���_
BOOL CWriteTSFile::ChkFreeFolder(
	ULONGLONG needFreeSize,
	wstring chkFolderPath
)
{
	BOOL ret = FALSE;

	ULARGE_INTEGER stFree;
	if( _GetDiskFreeSpaceEx( chkFolderPath.c_str(), &stFree, NULL, NULL ) != FALSE ){
		if( stFree.QuadPart > needFreeSize ){
			ret = TRUE;
		}
	}
	return ret;
}

//�t�@�C���ۑ����I������
//�߂�l�F
// TRUE�i�����j�AFALSE�i���s�j
BOOL CWriteTSFile::EndSave()
{
	BOOL ret = TRUE;

	if( this->outThread != NULL ){
		this->outStopFlag = TRUE;
		// �X���b�h�I���҂�
		if ( ::WaitForSingleObject(this->outThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->outThread, 0xffffffff);
		}
		CloseHandle(this->outThread);
		this->outThread = NULL;
	}

	this->tsBuffList.clear();

	return ret;
}

//�o�͗pTS�f�[�^�𑗂�
//�߂�l�F
// TRUE�i�����j�AFALSE�i���s�j
//�����F
// data		[IN]TS�f�[�^
// size		[IN]data�̃T�C�Y
BOOL CWriteTSFile::AddTSBuff(
	BYTE* data,
	DWORD size
	)
{
	if( data == NULL || size == 0 || this->outThread == NULL){
		return FALSE;
	}

	BOOL ret = TRUE;

	{
		CBlockLock lock(&this->outThreadLock);
		for( std::list<vector<BYTE>>::iterator itr = this->tsBuffList.begin(); size != 0; itr++ ){
			if( itr == this->tsBuffList.end() ){
				//�o�b�t�@�𑝂₷
				if( this->maxBuffCount > 0 && this->tsBuffList.size() > (size_t)this->maxBuffCount ){
					_OutputDebugString(L"��writeBuffList MaxOver");
					for( itr = this->tsBuffList.begin(); itr != this->tsBuffList.end(); (itr++)->clear() );
					itr = this->tsBuffList.begin();
				}else{
					this->tsBuffList.push_back(vector<BYTE>());
					itr = this->tsBuffList.end();
					(--itr)->reserve(48128);
				}
			}
			DWORD insertSize = min(48128 - (DWORD)itr->size(), size);
			itr->insert(itr->end(), data, data + insertSize);
			data += insertSize;
			size -= insertSize;
		}
	}
	return ret;
}

UINT WINAPI CWriteTSFile::OutThread(LPVOID param)
{
	//�v���O�C����COM�𗘗p���邩������Ȃ�����
	CoInitialize(NULL);

	CWriteTSFile* sys = (CWriteTSFile*)param;
	BOOL emptyFlag = TRUE;
	for( size_t i=0; i<sys->fileList.size(); i++ ){
		sys->fileList[i].writeUtil = new CWritePlugInUtil;
		wstring moduleFolder;
		GetModuleFolderPath(moduleFolder);
		if( sys->fileList[i].writeUtil->Initialize((moduleFolder + L"\\Write\\" + sys->fileList[i].writePlugIn).c_str()) == FALSE ){
			OutputDebugString(L"CWriteTSFile::StartSave Err 3\r\n");
			SAFE_DELETE(sys->fileList[i].writeUtil);
		}else{
			wstring folderPath = sys->fileList[i].recFolder;
			ChkFolderPath(folderPath);
			if( CompareNoCase(sys->fileList[i].writePlugIn, L"Write_Default.dll") == 0 ){
				//�f�t�H���g�̏ꍇ�͋󂫗e�ʂ����炩���߃`�F�b�N
				if( sys->createSize > 0 ){
					if( sys->ChkFreeFolder(sys->createSize, sys->fileList[i].recFolder) == FALSE ){
						if( sys->GetFreeFolder(sys->createSize, folderPath) ){
							//�󂫂Ȃ������̂ŃT�u�t�H���_�ɘ^��
							sys->subRecFlag = TRUE;
						}
					}
				}
			}
			//�J�n
			BOOL startRes = sys->fileList[i].writeUtil->StartSave(
				(folderPath + L'\\' + sys->fileList[i].recFileName).c_str(), sys->overWriteFlag, sys->createSize);
			if( startRes == FALSE ){
				OutputDebugString(L"CWriteTSFile::StartSave Err 2\r\n");
				//�G���[���T�u�t�H���_�Ń��g���C
				if( sys->GetFreeFolder(sys->createSize, folderPath) ){
					//�󂫂Ȃ������̂ŃT�u�t�H���_�ɘ^��
					sys->subRecFlag = TRUE;
					startRes = sys->fileList[i].writeUtil->StartSave(
						(folderPath + L'\\' + sys->fileList[i].recFileName).c_str(), sys->overWriteFlag, sys->createSize);
				}
			}
			if( startRes == FALSE ){
				SAFE_DELETE(sys->fileList[i].writeUtil);
			}else{
				if( i == 0 ){
					WCHAR saveFilePath[512] = L"";
					DWORD saveFilePathSize = 512;
					sys->fileList[i].writeUtil->GetSaveFilePath(saveFilePath, &saveFilePathSize);
					sys->mainSaveFilePath = saveFilePath;
				}
				sys->fileList[i].freeChk = emptyFlag;
				emptyFlag = FALSE;
			}
		}
	}
	if( emptyFlag ){
		OutputDebugString(L"CWriteTSFile::StartSave Err fileList 0\r\n");
		CoUninitialize();
		return 0;
	}
	sys->outStartFlag = TRUE;
	std::list<vector<BYTE>> data;

	while( sys->outStopFlag == FALSE ){
		//�o�b�t�@����f�[�^���o��
		{
			CBlockLock lock(&sys->outThreadLock);
			if( data.empty() == false ){
				//�ԋp
				data.front().clear();
				std::list<vector<BYTE>>::iterator itr;
				for( itr = sys->tsBuffList.begin(); itr != sys->tsBuffList.end() && itr->empty() == false; itr++ );
				sys->tsBuffList.splice(itr, data);
			}
			if( sys->tsBuffList.empty() == false && sys->tsBuffList.front().size() == 48128 ){
				data.splice(data.end(), sys->tsBuffList, sys->tsBuffList.begin());
			}
		}

		if( data.empty() == false ){
			DWORD dataSize = (DWORD)data.front().size();
			for( size_t i=0; i<sys->fileList.size(); i++ ){
				{
					if( sys->fileList[i].writeUtil != NULL ){
						DWORD write = 0;
						if( sys->fileList[i].writeUtil->AddTSBuff( &data.front().front(), dataSize, &write) == FALSE ){
							//�󂫂��Ȃ��Ȃ���
							if( i == 0 ){
								CBlockLock lock(&sys->outThreadLock);
								if( sys->writeTotalSize >= 0 ){
									//�o�̓T�C�Y�̉��Z���~����
									sys->writeTotalSize = -(sys->writeTotalSize + 1);
								}
							}
							sys->fileList[i].writeUtil->StopSave();

							if( sys->fileList[i].freeChk == TRUE ){
								//���̋󂫂�T��
								wstring freeFolderPath = L"";
								if( sys->GetFreeFolder(200*1024*1024, freeFolderPath) == TRUE ){
									wstring recFilePath = freeFolderPath;
									recFilePath += L"\\";
									recFilePath += sys->fileList[i].recFileName;

									//�J�n
									if( sys->fileList[i].writeUtil->StartSave(recFilePath.c_str(), sys->overWriteFlag, 0) == FALSE ){
										//���s�����̂ŏI���
										SAFE_DELETE(sys->fileList[i].writeUtil);
									}else{
										sys->subRecFlag = TRUE;

										if( dataSize > write ){
											sys->fileList[i].writeUtil->AddTSBuff( &data.front().front()+write, dataSize-write, &write);
										}
									}
								}
							}else{
								//���s�����̂ŏI���
								SAFE_DELETE(sys->fileList[i].writeUtil);
							}
						}else{
							//����ł͐��ۂɂ�����炸writeTotalSize��dataSize�����Z���Ă��邪
							//�o�̓T�C�Y�̗��p�P�[�X�I�ɂ�mainSaveFilePath�ƈ�v�����Ȃ��Ƃ��������Ǝv���̂ŁA���̂悤�ɕύX����
							if( i == 0 ){
								CBlockLock lock(&sys->outThreadLock);
								if( sys->writeTotalSize >= 0 ){
									sys->writeTotalSize += dataSize;
								}
							}
						}
					}
				}
			}
		}else{
			//TODO: �����ɂ̓��b�Z�[�W���f�B�X�p�b�`���ׂ�(�X���b�h���ŒP����COM�I�u�W�F�N�g�����������(����)���Ȃ�)
			Sleep(100);
		}
	}

	//�c���Ă���o�b�t�@�������o��
	{
		CBlockLock lock(&sys->outThreadLock);
		for( std::list<vector<BYTE>>::iterator itr = sys->tsBuffList.begin(); itr != sys->tsBuffList.end() && itr->empty() == false; itr++ ){
			for( size_t i=0; i<sys->fileList.size(); i++ ){
				if( sys->fileList[i].writeUtil ){
					DWORD write = 0;
          if (sys->fileList[i].writeUtil->AddTSBuff(&itr->front(), (DWORD)itr->size(), &write)) {
            sys->writeTotalSize += write;
          }
				}
			}
			itr->clear();
		}
	}
	for( size_t i=0; i<sys->fileList.size(); i++ ){
		if( sys->fileList[i].writeUtil ){
			sys->fileList[i].writeUtil->StopSave();
			SAFE_DELETE(sys->fileList[i].writeUtil);
		}
	}

	CoUninitialize();
	return 0;
}

//�^�撆�̃t�@�C���̃t�@�C���p�X���擾����
//�����F
// filePath			[OUT]�ۑ��t�@�C����
// subRecFlag		[OUT]�T�u�^�悪�����������ǂ���
void CWriteTSFile::GetSaveFilePath(
	wstring* filePath,
	BOOL* subRecFlag
	)
{
	*filePath = this->mainSaveFilePath;
	*subRecFlag = this->subRecFlag;
}

//�^�撆�̃t�@�C���̏o�̓T�C�Y���擾����
//�����F
// writeSize			[OUT]�ۑ��t�@�C����
void CWriteTSFile::GetRecWriteSize(
	__int64* writeSize
	)
{
	if( writeSize != NULL ){
		CBlockLock lock(&this->outThreadLock);
		*writeSize = this->writeTotalSize < 0 ? -(this->writeTotalSize + 1) : this->writeTotalSize;
	}
}
