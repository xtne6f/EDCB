#include "StdAfx.h"
#include "TunerBankCtrl.h"

#include <process.h>

#include "../../Common/ReNamePlugInUtil.h"
#include "../../Common/BlockLock.h"

CTunerBankCtrl::CTunerBankCtrl(DWORD tunerID_, LPCWSTR bonFileName_, const vector<CH_DATA4>& chList_, CNotifyManager& notifyManager_, CEpgDBManager& epgDBManager_, CReserveInfoManager& reserveInfoManager_)
	: tunerID(tunerID_)
	, bonFileName(bonFileName_)
	, chList(chList_)
	, notifyManager(notifyManager_)
	, epgDBManager(epgDBManager_)
	, reserveInfoManager(reserveInfoManager_)
{
	InitializeCriticalSection(&this->bankLock);

	this->checkThread = NULL;
	this->checkStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	this->openTuner = FALSE;
	this->processID = 0;
	this->openErrFlag = FALSE;

	this->currentChID = 0xFFFFFFFF;

	this->sendCtrl.SetConnectTimeOut(5*1000);

	this->epgCapWork = FALSE;

	this->delayTime = 0;

	this->twitterManager = NULL;

	ReloadSetting();
}


CTunerBankCtrl::~CTunerBankCtrl(void)
{
	if( this->checkThread != NULL ){
		::SetEvent(this->checkStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(this->checkThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(this->checkThread, 0xffffffff);
		}
		CloseHandle(this->checkThread);
		this->checkThread = NULL;
	}
	if( this->checkStopEvent != NULL ){
		CloseHandle(this->checkStopEvent);
		this->checkStopEvent = NULL;
	}

	map<DWORD, RESERVE_WORK*>::iterator itr;
	for( itr = this->reserveWork.begin(); itr != this->reserveWork.end(); itr++ ){
		SAFE_DELETE(itr->second);
	}
	this->reserveWork.clear();

	DeleteCriticalSection(&this->bankLock);
}


void CTunerBankCtrl::SetTwitterCtrl(CTwitterManager* twitterManager)
{
	this->twitterManager = twitterManager;
}

void CTunerBankCtrl::ReloadSetting()
{
	wstring iniPath = L"";
	GetModuleIniPath(iniPath);

	wstring commonIniPath = L"";
	GetCommonIniPath(commonIniPath);

	wstring viewIniPath = L"";
	GetModuleFolderPath(viewIniPath);
	viewIniPath += L"\\EpgDataCap_Bon.ini";

	this->defStartMargin = GetPrivateProfileInt(L"SET", L"StartMargin", 5, iniPath.c_str());
	this->defEndMargin = GetPrivateProfileInt(L"SET", L"EndMargin", 2, iniPath.c_str());
	this->recWakeTime = GetPrivateProfileInt(L"SET", L"RecAppWakeTime", 2, iniPath.c_str()) * 60;
	this->recMinWake = GetPrivateProfileInt(L"SET", L"RecMinWake", 1, iniPath.c_str());
	this->recView = GetPrivateProfileInt(L"SET", L"RecView", 1, iniPath.c_str());
	this->recNW = GetPrivateProfileInt(L"SET", L"RecNW", 0, iniPath.c_str());
	this->backPriority = GetPrivateProfileInt(L"SET", L"BackPriority", 1, iniPath.c_str());
	this->saveProgramInfo = GetPrivateProfileInt(L"SET", L"PgInfoLog", 0, iniPath.c_str());
	this->saveErrLog = GetPrivateProfileInt(L"SET", L"DropLog", 0, iniPath.c_str());

	this->recOverWrite = GetPrivateProfileInt(L"SET", L"RecOverWrite", 0, iniPath.c_str());
	this->useRecNamePlugIn = GetPrivateProfileInt(L"SET", L"RecNamePlugIn", 0, iniPath.c_str());

	WCHAR buff[512] = L"";
	GetPrivateProfileString(L"SET", L"RecNamePlugInFile", L"RecName_Macro.dll", buff, 512, iniPath.c_str());

	GetModuleFolderPath(this->recNamePlugInFilePath);
	this->recNamePlugInFilePath += L"\\RecName\\";
	this->recNamePlugInFilePath += buff;

	GetPrivateProfileString( L"SET", L"RecFolderPath0", L"", buff, 512, commonIniPath.c_str() );
	this->recFolderPath = buff;
	if( this->recFolderPath.size() == 0 ){
		GetDefSettingPath(this->recFolderPath);
	}
	GetPrivateProfileString( L"SET", L"RecWritePlugIn0", L"", buff, 512, commonIniPath.c_str() );
	this->recWritePlugIn = buff;

	GetPrivateProfileString( L"SET", L"RecExePath", L"", buff, 512, commonIniPath.c_str() );
	this->recExePath = buff;
	if( this->recExePath.size() == 0 ){
		GetModuleFolderPath(this->recExePath);
		this->recExePath += L"\\EpgDataCap_Bon.exe";
	}

	this->enableCaption = GetPrivateProfileInt(L"SET", L"Caption", 1, viewIniPath.c_str());
	this->enableData = GetPrivateProfileInt(L"SET", L"Data", 0, viewIniPath.c_str());

	this->processPriority = (DWORD)GetPrivateProfileInt(L"SET", L"ProcessPriority", 3, iniPath.c_str());
	this->keepDisk = (BOOL)GetPrivateProfileInt(L"SET", L"KeepDisk", 1, iniPath.c_str());

}

void CTunerBankCtrl::AddReserve(
	vector<CReserveInfo*>* reserveInfo
	)
{
	CBlockLock lock(&this->bankLock);

	if( this->checkThread != NULL ){
		if( ::WaitForSingleObject(this->checkThread, 0) == WAIT_OBJECT_0 ){
			CloseHandle(this->checkThread);
			this->checkThread = NULL;
		}
	}
	
	for( size_t i=0; i<reserveInfo->size(); i++ ){
		RESERVE_DATA data;
		(*reserveInfo)[i]->GetData(&data);
		map<DWORD, RESERVE_WORK*>::iterator itr;
		itr = this->reserveWork.find(data.reserveID);
		if( itr == this->reserveWork.end() ){
			RESERVE_WORK* item = new RESERVE_WORK;
			item->reserveInfo = (*reserveInfo)[i];
			item->reserveID = data.reserveID;
			//item->ctrlID = 0;
			item->recStartFlag = FALSE;

			this->reserveWork.insert(pair<DWORD, RESERVE_WORK*>(item->reserveID, item));
		}
	}

	if( this->checkThread == NULL ){
		ResetEvent(this->checkStopEvent);
		this->checkThread = (HANDLE)_beginthreadex(NULL, 0, CheckReserveThread, (LPVOID)this, CREATE_SUSPENDED, NULL);
		SetThreadPriority( this->checkThread, THREAD_PRIORITY_NORMAL );
		ResumeThread(this->checkThread);
	}
}

void CTunerBankCtrl::ChgReserve(
	const RESERVE_DATA& reserve
	)
{
	CBlockLock lock(&this->bankLock);

	map<DWORD, RESERVE_WORK*>::iterator itr;
	itr = this->createCtrlList.find(reserve.reserveID);
	if( itr != this->createCtrlList.end() ){
		//起動中
		itr->second->reserveInfo->SetData(reserve);

		LONGLONG stratTime = ConvertI64Time(reserve.startTime);
		LONGLONG endTime = stratTime + reserve.durationSecond * I64_1SEC;
		LONGLONG startMargin;
		if( reserve.recSetting.useMargineFlag == TRUE ){
			startMargin = ((LONGLONG)reserve.recSetting.startMargine) * I64_1SEC;
		}else{
			startMargin = this->defStartMargin * I64_1SEC;
		}
		//開始マージンは元の予約終了時刻を超えて負であってはならない
		stratTime -= max(startMargin, stratTime - endTime);

		if( GetNowI64Time() < stratTime ){
			//開始時間遅くなったのでコントロール削除の必要あり
			for( size_t i=0; i<itr->second->ctrlID.size(); i++ ){
				if( itr->second->recStartFlag == TRUE ){
					SET_CTRL_REC_STOP_PARAM param;
					param.ctrlID = itr->second->ctrlID[i];
					param.saveErrLog = this->saveErrLog;
					SET_CTRL_REC_STOP_RES_PARAM resVal;
					this->sendCtrl.SendViewStopRec(param, &resVal);
				}
				itr->second->recStartFlag = FALSE;
				this->sendCtrl.SendViewDeleteCtrl(itr->second->ctrlID[i]);
			}
			itr->second->ctrlID.clear();

			this->reserveInfoManager.SetRecWaitMode(reserve.reserveID, FALSE, 0);

			this->createCtrlList.erase(itr);
		}
	}else{
		itr = this->reserveWork.find(reserve.reserveID);
		if( itr != this->reserveWork.end() ){
			itr->second->reserveInfo->SetData(reserve);
		}
	}
	itr = this->openErrReserveList.find(reserve.reserveID);
	if( itr != this->openErrReserveList.end() ){
		this->openErrReserveList.erase(itr);
	}
}

void CTunerBankCtrl::DeleteReserve(
	DWORD reserveID
	)
{
	CBlockLock lock(&this->bankLock);

	map<DWORD, RESERVE_WORK*>::iterator itr;

	//処理中なら停止
	itr = this->createCtrlList.find(reserveID);
	if( itr != this->createCtrlList.end() ){
		for( size_t i=0; i<itr->second->ctrlID.size(); i++ ){
			SET_CTRL_REC_STOP_PARAM param;
			param.ctrlID = itr->second->ctrlID[i];
			param.saveErrLog = this->saveErrLog;
			SET_CTRL_REC_STOP_RES_PARAM resVal;

			this->sendCtrl.SendViewStopRec(param, &resVal);

			this->sendCtrl.SendViewDeleteCtrl(itr->second->ctrlID[i]);
		}

		this->createCtrlList.erase(itr);
	}

	itr = this->reserveWork.find(reserveID);
	if( itr != this->reserveWork.end() ){
		SAFE_DELETE(itr->second);
		this->reserveWork.erase(itr);
	}

	itr = this->openErrReserveList.find(reserveID);
	if( itr != this->openErrReserveList.end() ){
		this->openErrReserveList.erase(itr);
	}
}

void CTunerBankCtrl::ClearNoCtrl()
{
	CBlockLock lock(&this->bankLock);

	map<DWORD, RESERVE_WORK*>::iterator itr;
	itr = this->reserveWork.begin();
	while(itr != this->reserveWork.end() )
	{
		if( itr->second->ctrlID.size() == 0 ){
			SAFE_DELETE(itr->second);
			this->reserveWork.erase(itr++);
		}else{
			itr++;
		}
	}
}

BOOL CTunerBankCtrl::IsOpenErr()
{
	CBlockLock lock(&this->bankLock);

	return this->openErrFlag;
}

void CTunerBankCtrl::GetOpenErrReserve(vector<CReserveInfo*>* reserveInfo)
{
	CBlockLock lock(&this->bankLock);

	map<DWORD, RESERVE_WORK*>::iterator itr;
	for(itr = this->openErrReserveList.begin(); itr != this->openErrReserveList.end(); itr++ ){
		reserveInfo->push_back(itr->second->reserveInfo);
	}
}

void CTunerBankCtrl::ResetOpenErr()
{
	CBlockLock lock(&this->bankLock);

	this->openErrReserveList.clear();
	this->openErrFlag = FALSE;
}

UINT WINAPI CTunerBankCtrl::CheckReserveThread(LPVOID param)
{
	CTunerBankCtrl* sys = (CTunerBankCtrl*)param;
	DWORD wait = 1000;

	BOOL startEpgCap = FALSE;

	while(1){
		if( ::WaitForSingleObject(sys->checkStopEvent, wait) != WAIT_TIMEOUT ){
			//キャンセルされた
			break;
		}

		if( sys->openErrFlag == FALSE ){
			{
				CBlockLock lock(&sys->bankLock);
				multimap<LONGLONG, RESERVE_WORK*> sortList;
				sys->GetCheckList(&sortList);

				BOOL viewMode = FALSE;
				SET_CH_INFO initCh;
				BOOL needOpenTuner = sys->IsNeedOpenTuner(&sortList, &viewMode, &initCh);
				//起動チェック
				if( needOpenTuner == TRUE ){
					//起動必要
					if( sys->openTuner == FALSE ){
						//まだ起動されてないので起動
						if( sys->OpenTuner(viewMode, &initCh) == FALSE ){
							//起動できなかった
							wait = 1000;
							multimap<LONGLONG, RESERVE_WORK*>::iterator itrErr;
							itrErr = sortList.begin();
							if( itrErr != sortList.end() ){
								sys->reserveInfoManager.AddNGTunerID(itrErr->second->reserveID, sys->tunerID);
								sys->openErrReserveList.insert(pair<DWORD, RESERVE_WORK*>(itrErr->second->reserveID, itrErr->second));
							}
							sys->openErrFlag = TRUE;
							continue;
						}else{
							sys->currentChID = ((DWORD)initCh.ONID) << 16 | initCh.TSID;
							sys->notifyManager.AddNotifyMsg(NOTIFY_UPDATE_PRE_REC_START, sys->bonFileName);
						}
					}
				}else{
					if( startEpgCap ==FALSE ){
						if( sys->openTuner == TRUE ){
							sys->CloseTuner();
						}
						wait = 1000;
					}
				}

				if( sys->openTuner == TRUE && startEpgCap == FALSE ){
					//PC時計との誤差取得
					int delaySec = 0;
					if( sys->sendCtrl.SendViewGetDelay(&delaySec) == CMD_ERR_CONNECT ){
						//EXE消されたかも
						wait = 1000;
						sys->ErrStop();
						continue;
					}
					sys->delayTime = delaySec * I64_1SEC;

					//制御コントロールまだ作成されていないものを作成
					sys->CreateCtrl(&sortList, sys->delayTime);

					BOOL needShortCheck = FALSE;
					//録画時間のチェック
					sys->CheckRec(sys->delayTime, &needShortCheck, wait);

					if( needShortCheck == TRUE ){
						wait = 100;
					}else{
						wait = 500;
					}
				}
			}

			if( sys->epgCapWork == TRUE ){
				if( startEpgCap == FALSE ){
					if( sys->openTuner == TRUE ){
						//予約用にすでに起動中なので終了
						sys->epgCapWork = FALSE;
					}else{
						//チューナー起動
						{
							CBlockLock lock(&sys->bankLock);
							if( sys->OpenTuner(FALSE, NULL) == FALSE ){
								sys->epgCapWork = FALSE;
							}else{
								//EPG取得開始
								Sleep(1000);
								if(sys->sendCtrl.SendViewEpgCapStart(&sys->epgCapItem) != CMD_SUCCESS){
									sys->CloseTuner();
									sys->epgCapWork = FALSE;
								}else{
									startEpgCap = TRUE;
								}
							}
						}
					}
				}else{
					//ステータス確認
					DWORD status = 0;
					if( sys->sendCtrl.SendViewGetStatus(&status) == CMD_SUCCESS ){
						if( status != VIEW_APP_ST_GET_EPG ){
						OutputDebugString(L"epg end");
							//取得終わった
							if( sys->openTuner == TRUE ){
								{
									CBlockLock lock(&sys->bankLock);
									sys->sendCtrl.SendViewEpgCapStop();
									sys->CloseTuner();
								}
							}
							startEpgCap = FALSE;
							sys->epgCapWork = FALSE;
						}else{
							//PC時計との誤差取得
							int delaySec = 0;
							if( sys->sendCtrl.SendViewGetDelay(&delaySec) == CMD_SUCCESS ){
								sys->delayTime = delaySec * I64_1SEC;
							}
						}
					}else{
						//エラー？
						OutputDebugString(L"epg err");
						if( sys->openTuner == TRUE ){
							{
								CBlockLock lock(&sys->bankLock);
								sys->CloseTuner();
							}
						}
						startEpgCap = FALSE;
						sys->epgCapWork = FALSE;
					}
				}
			}else{
				if( startEpgCap == TRUE ){
					OutputDebugString(L"epg cancel");
					//キャンセル？
					if( sys->openTuner == TRUE ){
						{
							CBlockLock lock(&sys->bankLock);
							sys->sendCtrl.SendViewEpgCapStop();
							sys->CloseTuner();
						}
					}
					startEpgCap = FALSE;
				}
			}
		}
	}
	return 0;
}

void CTunerBankCtrl::GetCheckList(multimap<LONGLONG, RESERVE_WORK*>* sortList)
{
	map<DWORD, RESERVE_WORK*>::iterator itr;
	for( itr = this->reserveWork.begin(); itr != this->reserveWork.end(); itr++ ){
		RESERVE_DATA data;
		itr->second->reserveInfo->GetData(&data);
		if( data.recSetting.recMode == RECMODE_NO ){
			continue;
		}

		itr->second->stratTime = ConvertI64Time(data.startTime);
		itr->second->endTime = GetSumTime(data.startTime, data.durationSecond);

		if( data.recSetting.useMargineFlag == TRUE ){
			itr->second->startMargine = ((LONGLONG)data.recSetting.startMargine) * I64_1SEC;
			itr->second->endMargine = ((LONGLONG)data.recSetting.endMargine) * I64_1SEC;
		}else{
			itr->second->startMargine = this->defStartMargin * I64_1SEC;
			itr->second->endMargine = this->defEndMargin * I64_1SEC;
		}

		itr->second->chID = ((DWORD)data.originalNetworkID)<<16 | data.transportStreamID;
		itr->second->priority = data.recSetting.priority;


		if( data.recSetting.recMode == 2 || data.recSetting.recMode == 3 ){
			itr->second->enableScramble = 0;
		}else{
			itr->second->enableScramble = 1;
		}

		if( data.recSetting.serviceMode & RECSERVICEMODE_SET ){
			if( data.recSetting.serviceMode & RECSERVICEMODE_CAP ){
				itr->second->enableCaption = 1;
			}else{
				itr->second->enableCaption = 0;
			}
			if( data.recSetting.serviceMode & RECSERVICEMODE_DATA ){
				itr->second->enableData = 1;
			}else{
				itr->second->enableData = 0;
			}
		}else{
			itr->second->enableCaption = this->enableCaption;
			itr->second->enableData = this->enableData;
		}

		itr->second->partialRecFlag = data.recSetting.partialRecFlag;
		itr->second->continueRecFlag = data.recSetting.continueRecFlag;

		itr->second->ONID = data.originalNetworkID;
		itr->second->TSID = data.transportStreamID;
		itr->second->SID = data.serviceID;

		LONGLONG sortKey = itr->second->stratTime - max(itr->second->startMargine, itr->second->stratTime - itr->second->endTime);

		sortList->insert(pair<LONGLONG, RESERVE_WORK*>(sortKey, itr->second));
	}
}

BOOL CTunerBankCtrl::IsNeedOpenTuner(multimap<LONGLONG, RESERVE_WORK*>* sortList, BOOL* viewMode, SET_CH_INFO* initCh)
{
	if( sortList == NULL ){
		return FALSE;
	}
	if( sortList->size() == 0 ){
		return FALSE;
	}

	LONGLONG nowTime = GetNowI64Time();

	BOOL ret = FALSE;
	multimap<LONGLONG, RESERVE_WORK*>::iterator itr;
	itr = sortList->begin();
	if( itr->second->stratTime - this->recWakeTime * I64_1SEC - max(itr->second->startMargine, itr->second->stratTime - itr->second->endTime) < nowTime ){
		ret =  TRUE;
		BYTE recMode=0;
		itr->second->reserveInfo->GetRecMode(&recMode);
		if( recMode == RECMODE_VIEW ){
			*viewMode = TRUE;
		}
		itr->second->reserveInfo->GetService(&(initCh->ONID), &(initCh->TSID), &(initCh->SID) );
		initCh->useSID = TRUE;
		initCh->useBonCh = FALSE;
	}

	return ret;
}


BOOL CTunerBankCtrl::OpenTuner(BOOL viewMode, SET_CH_INFO* initCh)
{
	BOOL noView = TRUE;
	BOOL noNW = TRUE;
	BOOL UDP = FALSE;
	BOOL TCP = FALSE;
	if( this->recView == TRUE && viewMode == TRUE ){
		noView = FALSE;
	}
	if( this->recNW == TRUE || viewMode == TRUE ){
		noNW = FALSE;
		UDP = TRUE;
		TCP = TRUE;
	}
	BOOL reusedTuner = FALSE;
	map<DWORD, DWORD> registGUIMap;
	this->notifyManager.GetRegistGUI(&registGUIMap);

	BOOL ret = OpenTunerExe(this->recExePath.c_str(), this->bonFileName.c_str(), tunerID, this->recMinWake, noView, noNW, UDP, TCP, this->processPriority, registGUIMap, &this->processID);
	if( ret == FALSE ){
		Sleep(500);
		ret = OpenTunerExe(this->recExePath.c_str(), this->bonFileName.c_str(), tunerID, this->recMinWake, noView, noNW, UDP, TCP, this->processPriority, registGUIMap, &this->processID);
	}
	if( ret == TRUE ){
		this->sendCtrl.SetPipeSetting(CMD2_VIEW_CTRL_WAIT_CONNECT, CMD2_VIEW_CTRL_PIPE, this->processID);

		this->sendCtrl.SendViewSetID(this->tunerID);

		this->sendCtrl.SendViewSetStandbyRec(1);
		if( initCh != NULL ){
			this->sendCtrl.SendViewSetCh(initCh);
		}
	}else{
		//CHがNULLならEPG取得用
		//EPG取得では奪わない
		if( initCh != NULL ){
			//起動中で使えるもの探す
			wstring exeName;
			GetFileName(this->recExePath, exeName);
			vector<DWORD> IDList = _FindPidListByExeName(exeName.c_str());
			for(size_t i=0; i<IDList.size(); i++ ){
				this->sendCtrl.SetPipeSetting(CMD2_VIEW_CTRL_WAIT_CONNECT, CMD2_VIEW_CTRL_PIPE, IDList[i]);

				wstring bonDriver = L"";
				this->sendCtrl.SendViewGetBonDrivere(&bonDriver);
				if( bonDriver.size() > 0 && CompareNoCase(bonDriver, this->bonFileName) == 0 ){
					int id=0;
					if(this->sendCtrl.SendViewGetID(&id) == CMD_SUCCESS){
						if( id == -1 ){
							DWORD status = 0;
							if(this->sendCtrl.SendViewGetStatus(&status) == CMD_SUCCESS){
								if( status == VIEW_APP_ST_NORMAL || status == VIEW_APP_ST_ERR_CH_CHG){
									this->sendCtrl.SendViewSetID(this->tunerID);

									this->sendCtrl.SendViewSetStandbyRec(1);

									if( initCh != NULL ){
										this->sendCtrl.SendViewSetCh(initCh);
									}
									this->processID = IDList[i];
									reusedTuner = TRUE;
									ret = TRUE;
								
									break;
								}
							}
						}
					}
				}
			}
			if( reusedTuner == FALSE ){
				//TVTestで使ってるものあるかチェック
				IDList = _FindPidListByExeName(L"tvtest.exe");
				map<DWORD, DWORD> registGUIMap;
				this->notifyManager.GetRegistGUI(&registGUIMap);

				for(size_t i=0; i<IDList.size(); i++ ){
					CSendCtrlCmd send;
					send.SetPipeSetting(CMD2_TVTEST_CTRL_WAIT_CONNECT, CMD2_TVTEST_CTRL_PIPE, IDList[i]);
					send.SetConnectTimeOut(1000);

					wstring bonDriver = L"";
					if( send.SendViewGetBonDrivere(&bonDriver) == CMD_SUCCESS){
						if( bonDriver.size() > 0 && CompareNoCase(bonDriver, this->bonFileName) == 0 ){
							send.SendViewAppClose();
							Sleep(5000);
							ret = OpenTunerExe(this->recExePath.c_str(), this->bonFileName.c_str(), tunerID, this->recMinWake, noView, noNW, UDP, TCP, this->processPriority, registGUIMap, &this->processID);
							if( ret == FALSE ){
								Sleep(500);
								ret = OpenTunerExe(this->recExePath.c_str(), this->bonFileName.c_str(), tunerID, this->recMinWake, noView, noNW, UDP, TCP, this->processPriority, registGUIMap, &this->processID);
							}
							if( ret == TRUE ){
								this->sendCtrl.SetPipeSetting(CMD2_VIEW_CTRL_WAIT_CONNECT, CMD2_VIEW_CTRL_PIPE, this->processID);

								this->sendCtrl.SendViewSetID(this->tunerID);

								this->sendCtrl.SendViewSetStandbyRec(1);
								if( initCh != NULL ){
									this->sendCtrl.SendViewSetCh(initCh);
								}
								break;
							}
						}
					}
				}
			}
			//EPG取得中のもの奪う
			if( reusedTuner == FALSE ){
				wstring exeName;
				GetFileName(this->recExePath, exeName);
				IDList = _FindPidListByExeName(exeName.c_str());
				for(size_t i=0; i<IDList.size(); i++ ){
					this->sendCtrl.SetPipeSetting(CMD2_VIEW_CTRL_WAIT_CONNECT, CMD2_VIEW_CTRL_PIPE, IDList[i]);

					wstring bonDriver = L"";
					this->sendCtrl.SendViewGetBonDrivere(&bonDriver);
					if( bonDriver.size() > 0 && CompareNoCase(bonDriver, this->bonFileName) == 0 ){
						int id=0;
						if(this->sendCtrl.SendViewGetID(&id) == CMD_SUCCESS){
							if( id == -1 ){
								DWORD status = 0;
								if(this->sendCtrl.SendViewGetStatus(&status) == CMD_SUCCESS){
									if( status == VIEW_APP_ST_GET_EPG ){
										this->sendCtrl.SendViewEpgCapStop();
										this->sendCtrl.SendViewSetID(this->tunerID);

										this->sendCtrl.SendViewSetStandbyRec(1);

										if( initCh != NULL ){
											this->sendCtrl.SendViewSetCh(initCh);
										}
										this->processID = IDList[i];
										reusedTuner = TRUE;
										ret = TRUE;
								
										break;
									}
								}
							}
						}
					}
				}
			}
			//録画中のもの奪う
			if( reusedTuner == FALSE ){
				wstring exeName;
				GetFileName(this->recExePath, exeName);
				IDList = _FindPidListByExeName(exeName.c_str());
				for(size_t i=0; i<IDList.size(); i++ ){
					this->sendCtrl.SetPipeSetting(CMD2_VIEW_CTRL_WAIT_CONNECT, CMD2_VIEW_CTRL_PIPE, IDList[i]);

					wstring bonDriver = L"";
					this->sendCtrl.SendViewGetBonDrivere(&bonDriver);
					if( bonDriver.size() > 0 && CompareNoCase(bonDriver, this->bonFileName) == 0 ){
						int id=0;
						if(this->sendCtrl.SendViewGetID(&id) == CMD_SUCCESS){
							if( id == -1 ){
								DWORD status = 0;
								if(this->sendCtrl.SendViewGetStatus(&status) == CMD_SUCCESS){
									if( status == VIEW_APP_ST_REC ){
										this->sendCtrl.SendViewStopRecAll();
										this->sendCtrl.SendViewSetID(this->tunerID);

										this->sendCtrl.SendViewSetStandbyRec(1);

										if( initCh != NULL ){
											this->sendCtrl.SendViewSetCh(initCh);
										}
										this->processID = IDList[i];
										reusedTuner = TRUE;
										ret = TRUE;
								
										break;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	if( ret == TRUE ){
		this->openTuner = TRUE;
	}else{
		this->openTuner = FALSE;
	}

	return ret;
}

BOOL CTunerBankCtrl::FindPartialService(WORD ONID, WORD TSID, WORD SID, WORD* partialSID, wstring* serviceName)
{
	vector<CH_DATA4>::const_iterator itr;
	for( itr = this->chList.begin(); itr != this->chList.end(); itr++ ){
		if( itr->originalNetworkID == ONID && itr->transportStreamID == TSID && itr->partialFlag == TRUE ){
			if( itr->serviceID != SID ){
				if( partialSID != NULL ){
					*partialSID = itr->serviceID;
				}
				if( serviceName != NULL ){
					*serviceName = itr->serviceName;
				}
				return TRUE;
			}
		}
	}
	return FALSE;
}

void CTunerBankCtrl::CreateCtrl(multimap<LONGLONG, RESERVE_WORK*>* sortList, LONGLONG delay)
{
	LONGLONG nowTime = GetNowI64Time();
	nowTime += delay;

	multimap<LONGLONG, RESERVE_WORK*>::iterator itr;
	for( itr = sortList->begin(); itr != sortList->end(); itr++ ){
		if( itr->second->ctrlID.size() == 0 ){
			//録画開始１分前になったらコントロール作成
			LONGLONG chkStartTime = itr->second->stratTime - (60*I64_1SEC) - max(itr->second->startMargine, itr->second->stratTime - itr->second->endTime);

			if( chkStartTime < nowTime ){
				BOOL createFlag = FALSE;
				if( this->currentChID == itr->second->chID ){
					//録画中のものに同一サービスで連続録画設定のものある？
					if( ContinueRec(itr->second) == FALSE ){
						CreateCtrl(itr->second);
						createFlag = TRUE;
					}
				}else{
					//チャンネル違うので録画時間考慮する必要あり
					createFlag = CheckOtherChCreate(nowTime, itr->second);

					if( createFlag == TRUE ){
						//作成タイミングになったので作成
						StopAllRec();

						SET_CH_INFO chgCh;
						itr->second->reserveInfo->GetService(&chgCh.ONID, &chgCh.TSID, &chgCh.SID);
						chgCh.useSID = TRUE;
						chgCh.useBonCh = FALSE;

						this->sendCtrl.SendViewSetStandbyRec(1);

						if( this->sendCtrl.SendViewSetCh(&chgCh) != CMD_SUCCESS ){
							//失敗時もう一度リトライ
							Sleep(200);
							this->sendCtrl.SendViewSetCh(&chgCh);
						}
						this->currentChID = ((DWORD)chgCh.ONID) << 16 | chgCh.TSID;

						//作成
						CreateCtrl(itr->second);
					}
				}

				if( createFlag == TRUE ){
					RESERVE_DATA data;
					itr->second->reserveInfo->GetData(&data);
					wstring msg;
					Format(msg, L"%s %04d/%02d/%02d %02d:%02d:%02d～ %s", 
						data.stationName.c_str(),
						data.startTime.wYear,
						data.startTime.wMonth,
						data.startTime.wDay,
						data.startTime.wHour,
						data.startTime.wMinute,
						data.startTime.wMilliseconds,
						data.title.c_str()
						);
					this->notifyManager.AddNotifyMsg(NOTIFY_UPDATE_PRE_REC_START, msg);
				}
			}
		}
	}
}

void CTunerBankCtrl::CreateCtrl(RESERVE_WORK* info)
{
	//作成
	DWORD newCtrlID = 0;
	BOOL createFull = TRUE;
	BYTE recMode = 0;
	info->reserveInfo->GetRecMode(&recMode);
	if( (info->partialRecFlag == 1 || info->partialRecFlag == 2) && recMode != RECMODE_VIEW){
		//部分受信サービスも
		WORD partialSID = 0;
		if( FindPartialService(info->ONID, info->TSID, info->SID, &partialSID, NULL) == TRUE ){
			//部分受信
			if( sendCtrl.SendViewCreateCtrl(&newCtrlID) == CMD_SUCCESS){
				info->ctrlID.push_back(newCtrlID);
				//コントロールに対して設定
				SET_CTRL_MODE param;
				param.ctrlID = newCtrlID;
				param.SID = partialSID;
				param.enableScramble = info->enableScramble;
				param.enableCaption = info->enableCaption;
				param.enableData = info->enableData;

				if( this->sendCtrl.SendViewSetCtrlMode(param) != CMD_SUCCESS){
					//失敗時もう一度リトライ
					Sleep(200);
					this->sendCtrl.SendViewSetCtrlMode(param);
				}

				info->partialCtrlID = newCtrlID;
				if(info->partialRecFlag == 2){
					createFull = FALSE;
				}
			}
		}
	}
	if( createFull == FALSE ){
		info->mainCtrlID = newCtrlID;
		this->reserveInfoManager.SetRecWaitMode(info->reserveID, TRUE, this->tunerID);
		this->createCtrlList.insert(pair<DWORD, RESERVE_WORK*>(info->reserveID,info));
	}else{
		//通常
		if( sendCtrl.SendViewCreateCtrl(&newCtrlID) == CMD_SUCCESS){
			info->ctrlID.push_back(newCtrlID);
			info->mainCtrlID = newCtrlID;
			this->reserveInfoManager.SetRecWaitMode(info->reserveID, TRUE, this->tunerID);
			//コントロールに対して設定
			SET_CTRL_MODE param;
			param.ctrlID = newCtrlID;

			BYTE recMode = 0;
			info->reserveInfo->GetRecMode(&recMode);
			if( recMode == RECMODE_ALL || recMode == RECMODE_ALL_NOB25 ){
				param.SID = 0xFFFF;
			}else{
				info->reserveInfo->GetService(NULL,NULL,&param.SID);
			}
			param.enableScramble = info->enableScramble;
			param.enableCaption = info->enableCaption;
			param.enableData = info->enableData;

			if( this->sendCtrl.SendViewSetCtrlMode(param) != CMD_SUCCESS){
				//失敗時もう一度リトライ
				Sleep(200);
				this->sendCtrl.SendViewSetCtrlMode(param);
			}
						
			this->createCtrlList.insert(pair<DWORD, RESERVE_WORK*>(info->reserveID, info));
		}
	}
}

BOOL CTunerBankCtrl::ContinueRec(RESERVE_WORK* info)
{
	BOOL ret = FALSE;
	map<DWORD, RESERVE_WORK*>::iterator itr;
	for( itr = this->createCtrlList.begin(); itr != this->createCtrlList.end(); itr++ ){
		RESERVE_DATA data;
		itr->second->reserveInfo->GetData(&data);
		if( data.reserveStatus == ADD_RESERVE_NO_FIND ||
			data.reserveStatus == ADD_RESERVE_UNKNOWN_END
			){
				continue;
		}
		if( this->reserveInfoManager.IsChkPfInfo(data.reserveID) == FALSE && data.recSetting.tuijyuuFlag == 1 ){
			continue;
		}
		if( itr->second->ONID == info->ONID &&
			itr->second->TSID == info->TSID &&
			itr->second->SID == info->SID &&
			itr->second->continueRecFlag == 1 ){
				if( itr->second->stratTime < info->stratTime &&
					info->stratTime <= itr->second->endTime &&
					itr->second->endTime < info->endTime
					){
					//連続録画なので、同一制御IDで録画開始されたことにする
					info->ctrlID = itr->second->ctrlID;
					info->mainCtrlID = itr->second->mainCtrlID;
					this->reserveInfoManager.SetRecWaitMode(info->reserveID, TRUE, this->tunerID);
					info->pfInfoAddFlag = TRUE;
					itr->second->continueRecStartFlag = TRUE;

					info->recStartFlag = TRUE;

					this->createCtrlList.insert(pair<DWORD, RESERVE_WORK*>(info->reserveID, info));


					ret = TRUE;
					break;
				}
		}
	}

	return ret;
}

BOOL CTunerBankCtrl::CheckOtherChCreate(LONGLONG nowTime, RESERVE_WORK* reserve)
{
	BOOL createFlag = FALSE;

	LONGLONG chkStartTime = reserve->stratTime;
	chkStartTime -= max(reserve->startMargine, reserve->stratTime - reserve->endTime);

	LONGLONG chgTimeHPriority = 0;
	LONGLONG chgTimeLPriority = 0;

	if( this->createCtrlList.size() != 0 ){
		map<DWORD, RESERVE_WORK*>::iterator itr;
		for( itr = this->createCtrlList.begin(); itr != this->createCtrlList.end(); itr++ ){
			LONGLONG chkEndTime = itr->second->endTime;
			chkEndTime += max(itr->second->endMargine, itr->second->stratTime - min(itr->second->startMargine, 0) - itr->second->endTime);
			if( chkEndTime + 15*I64_1SEC >= chkStartTime ){
				//このコントロールの終了はreserveの録画開始と重なる
				if( itr->second->priority < reserve->priority ||
					(itr->second->priority == reserve->priority && this->backPriority == TRUE )){
					//後ろ優先なので開始15秒前に停止させる
					if( chgTimeHPriority > chkStartTime - 15*I64_1SEC || chgTimeHPriority == 0 ){
						chgTimeHPriority = chkStartTime - 15*I64_1SEC;
					}
				}else{
					//前の録画優先なので終わりまで待つ
					if( chgTimeLPriority < chkEndTime || chgTimeLPriority == 0 ){
						chgTimeLPriority = chkEndTime;
					}
				}
			}
		}

		LONGLONG chgTime = 0;
		if( chgTimeHPriority == 0 && chgTimeLPriority != 0 ){
			chgTime = chgTimeLPriority;
		}else if( chgTimeHPriority != 0 && chgTimeLPriority == 0 ){
			chgTime = chgTimeHPriority;
		}else if( chgTimeHPriority != 0 && chgTimeLPriority != 0 ){
			//自分より高い優先度存在するのでそちら優先
			chgTime = chgTimeLPriority;
		}

		if( chgTime != 0 && chgTime <= nowTime ){
			createFlag = TRUE;
		}
	}else{
		createFlag = TRUE;
	}

	return createFlag;
}

void CTunerBankCtrl::StopAllRec()
{
	map<DWORD, RESERVE_WORK*>::iterator itr;
	for( itr = this->createCtrlList.begin(); itr != this->createCtrlList.end(); itr++ ){
		for( size_t i=0; i<itr->second->ctrlID.size(); i++ ){
			SET_CTRL_REC_STOP_PARAM param;
			param.ctrlID = itr->second->ctrlID[i];
			param.saveErrLog = this->saveErrLog;
			SET_CTRL_REC_STOP_RES_PARAM resVal;

			this->sendCtrl.SendViewStopRec(param, &resVal);

			this->sendCtrl.SendViewDeleteCtrl(itr->second->ctrlID[i]);

			if( itr->second->ctrlID[i] == itr->second->mainCtrlID ){
				if( itr->second->endTime > GetNowI64Time() + 60*I64_1SEC ){
					AddEndReserve(itr->second, REC_END_STATUS_NEXT_START_END, resVal);
				}else{
					if( itr->second->notStartHeadFlag == FALSE ){
						AddEndReserve(itr->second, REC_END_STATUS_NORMAL, resVal);
					}else{
						AddEndReserve(itr->second, REC_END_STATUS_NOT_START_HEAD, resVal);
					}
				}
			}
		}
//		this->endReserveList.insert(pair<DWORD, RESERVE_WORK*>(itr->first, itr->second));
	}
	this->createCtrlList.clear();
}

void CTunerBankCtrl::ErrStop()
{
	map<DWORD, RESERVE_WORK*>::iterator itr;
	for( itr = this->createCtrlList.begin(); itr != this->createCtrlList.end(); itr++ ){
		SET_CTRL_REC_STOP_RES_PARAM resVal;
		resVal.drop = 0;
		resVal.scramble = 0;
		AddEndReserve(itr->second, REC_END_STATUS_ERR_END, resVal);
//		this->endReserveList.insert(pair<DWORD, RESERVE_WORK*>(itr->first, itr->second));
	}
	this->createCtrlList.clear();
	this->processID = 0;
	this->openTuner = FALSE;
}

void CTunerBankCtrl::CheckRec(LONGLONG delay, BOOL* needShortCheck, DWORD wait)
{
	LONGLONG nowTime = GetNowI64Time();
	nowTime += delay;

	*needShortCheck = FALSE;
	vector<DWORD> endList;

	map<DWORD, RESERVE_WORK*>::iterator itr;
	for( itr = this->createCtrlList.begin(); itr != this->createCtrlList.end(); itr++ ){
		RESERVE_DATA data;
		itr->second->reserveInfo->GetData(&data);

		LONGLONG chkStartTime = ConvertI64Time(data.startTime);
		LONGLONG chkEndTime = chkStartTime + data.durationSecond * I64_1SEC;

		LONGLONG startMargine = 0;
		LONGLONG endMargine = 0;
		if( data.recSetting.useMargineFlag == TRUE ){
			startMargine = ((LONGLONG)data.recSetting.startMargine)*I64_1SEC;
			endMargine = ((LONGLONG)data.recSetting.endMargine)*I64_1SEC;
		}else{
			startMargine = this->defStartMargin * I64_1SEC;
			endMargine = this->defEndMargin * I64_1SEC;
		}

		startMargine = max(startMargine, chkStartTime - chkEndTime);
		endMargine = max(endMargine, chkStartTime - min(startMargine, 0) - chkEndTime);
		chkStartTime -= startMargine + I64_1SEC;
		chkEndTime += endMargine;

		if( nowTime < chkStartTime ){
			if( chkStartTime - 5*I64_1SEC < nowTime){
				//開始5秒前になったらチェック間隔を短くする
				*needShortCheck = TRUE;
			}else{
				//ステータス確認
				DWORD status = 0;
				if( this->sendCtrl.SendViewGetStatus(&status) == CMD_SUCCESS ){
					if( status == VIEW_APP_ST_ERR_CH_CHG ){
						//チャンネル切り替え失敗してるようなのでリトライ
						SET_CH_INFO chgCh;
						itr->second->reserveInfo->GetService(&chgCh.ONID, &chgCh.TSID, &chgCh.SID);
						chgCh.useSID = TRUE;
						chgCh.useBonCh = FALSE;

						this->sendCtrl.SendViewSetCh(&chgCh);
						this->currentChID = ((DWORD)chgCh.ONID) << 16 | chgCh.TSID;
					}
				}
			}
		}else if( chkStartTime <= nowTime && nowTime < chkEndTime-30*I64_1SEC){
			if( itr->second->recStartFlag == FALSE ){
				//開始時間過ぎているので録画開始
				if( RecStart(nowTime, itr->second, TRUE) == TRUE ){
					itr->second->recStartFlag = TRUE;
					if( chkStartTime + 60*I64_1SEC < nowTime ){
						//途中から開始された
						itr->second->notStartHeadFlag = TRUE;
					}
				}else{
					//録画に失敗した？
					SET_CTRL_REC_STOP_RES_PARAM resVal;
					resVal.drop = 0;
					resVal.scramble = 0;
					AddEndReserve(itr->second, REC_END_STATUS_ERR_RECSTART, resVal);
				}
			}else{
				if( itr->second->savedPgInfo == FALSE){
					GET_EPG_PF_INFO_PARAM val;
					itr->second->reserveInfo->GetService(&val.ONID, &val.TSID, &val.SID);
					val.pfNextFlag = 0;

					EPGDB_EVENT_INFO resVal;
					if( this->sendCtrl.SendViewGetEventPF(&val, &resVal) == CMD_SUCCESS ){
						if( resVal.StartTimeFlag == 1 && resVal.DurationFlag == 1 ){
							if( ConvertI64Time(resVal.start_time) <= GetSumTime(data.startTime, 30) &&
								GetSumTime(data.startTime, 30) < GetSumTime(resVal.start_time, resVal.durationSec)
								){
								//開始時間から30秒は過ぎているのでこの番組情報が録画中のもののはず
								itr->second->savedPgInfo = TRUE;
								if(data.eventID != 0xFFFF ){
									itr->second->eventInfo = new  EPGDB_EVENT_INFO;

									itr->second->eventInfo->original_network_id = resVal.original_network_id;
									itr->second->eventInfo->transport_stream_id = resVal.transport_stream_id;
									itr->second->eventInfo->service_id = resVal.service_id;
									itr->second->eventInfo->event_id = resVal.event_id;
									itr->second->eventInfo->StartTimeFlag = resVal.StartTimeFlag;
									itr->second->eventInfo->start_time = resVal.start_time;
									itr->second->eventInfo->DurationFlag = resVal.DurationFlag;
									itr->second->eventInfo->durationSec = resVal.durationSec;
									if( resVal.shortInfo != NULL ){
										itr->second->eventInfo->shortInfo = new EPGDB_SHORT_EVENT_INFO;
										*itr->second->eventInfo->shortInfo = *resVal.shortInfo;
									}
									if( resVal.extInfo != NULL ){
										itr->second->eventInfo->extInfo = new EPGDB_EXTENDED_EVENT_INFO;
										*itr->second->eventInfo->extInfo = *resVal.extInfo;
									}
									if( resVal.contentInfo != NULL ){
										itr->second->eventInfo->contentInfo = new EPGDB_CONTEN_INFO;
										*itr->second->eventInfo->contentInfo = *resVal.contentInfo;
									}
									if( resVal.componentInfo != NULL ){
										itr->second->eventInfo->componentInfo = new EPGDB_COMPONENT_INFO;
										*itr->second->eventInfo->componentInfo = *resVal.componentInfo;
									}
									if( resVal.audioInfo != NULL ){
										itr->second->eventInfo->audioInfo = new EPGDB_AUDIO_COMPONENT_INFO;
										*itr->second->eventInfo->audioInfo = *resVal.audioInfo;
									}
									if( resVal.eventGroupInfo != NULL ){
										itr->second->eventInfo->eventGroupInfo = new EPGDB_EVENTGROUP_INFO;
										*itr->second->eventInfo->eventGroupInfo = *resVal.eventGroupInfo;
									}
									if( resVal.eventRelayInfo != NULL ){
										itr->second->eventInfo->eventRelayInfo = new EPGDB_EVENTGROUP_INFO;
										*itr->second->eventInfo->eventRelayInfo = *resVal.eventRelayInfo;
									}
								}
								if( this->saveProgramInfo == TRUE ){
									//録画ファイルのパス取得
									for(size_t i=0; i<itr->second->ctrlID.size(); i++ ){
										wstring recFilePath = L"";
										if( this->sendCtrl.SendViewGetRecFilePath(itr->second->ctrlID[i], &recFilePath) == CMD_SUCCESS ){
											//番組情報保存
											wstring iniCommonPath = L"";
											GetCommonIniPath(iniCommonPath);

											WCHAR buff[512] = L"";
											GetPrivateProfileString(L"SET", L"RecInfoFolder", L"", buff, 512, iniCommonPath.c_str());
											wstring infoFolder = buff;
											ChkFolderPath(infoFolder);

											if( infoFolder.size() > 0 ){
												wstring tsFileName = L"";
												GetFileName(recFilePath, tsFileName);
												wstring pgFile = L"";
												Format(pgFile, L"%s\\%s.program.txt", infoFolder.c_str(), tsFileName.c_str());
												SaveProgramInfo(pgFile, &resVal, 0, itr->second->pfInfoAddFlag);
											}else{
												recFilePath += L".program.txt";
												SaveProgramInfo(recFilePath, &resVal, 0, itr->second->pfInfoAddFlag);
											}
										}
									}
								}
							}
						}
					}
				}
				//ステータス確認
				DWORD status = 0;
				if( this->sendCtrl.SendViewGetStatus(&status) == CMD_SUCCESS ){
					if( status != VIEW_APP_ST_REC && data.recSetting.recMode != RECMODE_VIEW){
						//キャンセルされた？
						SET_CTRL_REC_STOP_RES_PARAM resVal;
						resVal.drop = 0;
						resVal.scramble = 0;
						if( status == VIEW_APP_ST_ERR_CH_CHG ){
							__int64 chkTime = ConvertI64Time(data.startTime);
							if( startMargine < 0 ){
								chkTime -= startMargine;
							}
							if( nowTime > chkTime + (60*I64_1SEC) ){
								AddEndReserve(itr->second, REC_END_STATUS_ERR_CH_CHG, resVal);
							}
						}else{
							AddEndReserve(itr->second, REC_END_STATUS_ERR_END, resVal);
						}
					}
				}
			}
		}else if( chkEndTime-5*I64_1SEC < nowTime && nowTime <chkEndTime ){
			//終了5秒前になったらチェック間隔を短くする
			*needShortCheck = TRUE;
		}else if( chkEndTime < nowTime){
			//終了時間過ぎている
			if( itr->second->continueRecStartFlag == FALSE ){
				if( data.recSetting.recMode == RECMODE_VIEW ){
					for(size_t i=0; i<itr->second->ctrlID.size(); i++ ){
						this->sendCtrl.SendViewDeleteCtrl(itr->second->ctrlID[i]);
						if( itr->second->ctrlID[i] == itr->second->mainCtrlID ){
							SET_CTRL_REC_STOP_RES_PARAM resVal;
							resVal.drop=0;
							resVal.scramble=0;
							resVal.subRecFlag=0;
							resVal.recFilePath = L"";
							AddEndReserve(itr->second, REC_END_STATUS_NORMAL, resVal);
						}
					}
				}else{
					for(size_t i=0; i<itr->second->ctrlID.size(); i++ ){
						SET_CTRL_REC_STOP_PARAM param;
						param.ctrlID = itr->second->ctrlID[i];
						param.saveErrLog = this->saveErrLog;
						SET_CTRL_REC_STOP_RES_PARAM resVal;
						BOOL errEnd = FALSE;
						if( this->sendCtrl.SendViewStopRec(param, &resVal) == CMD_ERR ){
							errEnd = TRUE;
						}

						this->sendCtrl.SendViewDeleteCtrl(itr->second->ctrlID[i]);
						if( itr->second->ctrlID[i] == itr->second->mainCtrlID ){
							DWORD endType = REC_END_STATUS_NORMAL;
							if( itr->second->notStartHeadFlag == TRUE ){
								endType = REC_END_STATUS_NOT_START_HEAD;
							}
							if( resVal.subRecFlag == 1 ){
								endType = REC_END_STATUS_END_SUBREC;
							}
							if( data.recSetting.tuijyuuFlag == 1 && data.eventID != 0xFFFF ){
								if( this->reserveInfoManager.IsChkPfInfo(itr->second->reserveID) == FALSE ){
									endType = REC_END_STATUS_NOT_FIND_PF;
								}
							}
							if( errEnd == TRUE ){
								endType = REC_END_STATUS_ERR_END2;
								resVal.drop=0;
								resVal.scramble=0;
								resVal.subRecFlag=0;
								resVal.recFilePath = L"";
							}
							AddEndReserve(itr->second, endType, resVal);
						}
					}
				}
			}else{
				for(size_t i=0; i<itr->second->ctrlID.size(); i++ ){
					SET_CTRL_REC_STOP_RES_PARAM resVal;
					resVal.drop = 0;
					resVal.scramble = 0;
					resVal.recFilePath = L"";
					this->sendCtrl.SendViewGetRecFilePath(itr->second->mainCtrlID, &resVal.recFilePath);

					DWORD endType = REC_END_STATUS_NORMAL;
					if( resVal.subRecFlag == 1 ){
						endType = REC_END_STATUS_END_SUBREC;
					}
					if( data.recSetting.tuijyuuFlag == 1 && data.eventID != 0xFFFF ){
						if( this->reserveInfoManager.IsChkPfInfo(itr->second->reserveID) == FALSE ){
							endType = REC_END_STATUS_NOT_FIND_PF;
						}
					}
					AddEndReserve(itr->second, endType, resVal);
				}
			}

			endList.push_back(itr->first);
		}
	}

	//終了リストに移行
	for( size_t i=0; i<endList.size(); i++ ){
		itr = this->createCtrlList.find(endList[i]);
		if( itr != this->createCtrlList.end()){
			this->createCtrlList.erase(itr);
		}
	}
}

void CTunerBankCtrl::SaveProgramInfo(wstring savePath, EPGDB_EVENT_INFO* info, BYTE mode, BOOL addMode)
{
	wstring outText = L"";
	wstring serviceName = L"";
	vector<CH_DATA4>::const_iterator itr;
	for( itr = this->chList.begin(); itr != this->chList.end(); itr++ ){
		if( itr->originalNetworkID == info->original_network_id &&
		    itr->transportStreamID == info->transport_stream_id &&
		    itr->serviceID == info->service_id ){
			serviceName = itr->serviceName;
			break;
		}
	}
	_ConvertEpgInfoText2(info, outText, serviceName);

	string buff = "";
	WtoA(outText, buff);

	HANDLE file = INVALID_HANDLE_VALUE;
	if(addMode == TRUE ){
		file = _CreateDirectoryAndFile( savePath.c_str(), GENERIC_WRITE|GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
		if( file != INVALID_HANDLE_VALUE ){
			SetFilePointer(file, 0, NULL, FILE_END);
			string buff2 = "\r\n-----------------------\r\n";
			DWORD dwWrite;
			WriteFile(file, buff2.c_str(), (DWORD)buff2.size(), &dwWrite, NULL);
		}
	}else{
		file = _CreateDirectoryAndFile( savePath.c_str(), GENERIC_WRITE|GENERIC_READ, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	}
	if( file != INVALID_HANDLE_VALUE ){
		DWORD dwWrite;
		WriteFile(file, buff.c_str(), (DWORD)buff.size(), &dwWrite, NULL);
		CloseHandle(file);
	}
}

BOOL CTunerBankCtrl::RecStart(LONGLONG nowTime, RESERVE_WORK* reserve, BOOL sendNoyify)
{
	RESERVE_DATA data;
	reserve->reserveInfo->GetData(&data);
	BOOL ret = TRUE;

	wstring iniPath = L"";
	GetModuleIniPath(iniPath);

	BOOL noChkYen = (BOOL)GetPrivateProfileInt(L"SET", L"NoChkYen", 0, iniPath.c_str());

	if( data.recSetting.recMode == RECMODE_VIEW ){
		this->sendCtrl.SendViewSetStandbyRec(2);
		if( this->recView == TRUE ){
			this->sendCtrl.SendViewExecViewApp();
		}
		return TRUE;
	}

	for( size_t i=0; i<reserve->ctrlID.size(); i++ ){
		SET_CTRL_REC_PARAM param;
		param.ctrlID = reserve->ctrlID[i];
		//デフォルトファイル名
		if( this->useRecNamePlugIn == TRUE ){
			CReNamePlugInUtil plugIn;
			if( plugIn.Initialize(this->recNamePlugInFilePath.c_str()) == TRUE ){
				WCHAR name[512] = L"";
				DWORD size = 512;
				PLUGIN_RESERVE_INFO info;

				info.startTime = data.startTime;
				info.durationSec = data.durationSecond;
				wcscpy_s(info.eventName, 512, data.title.c_str());
				info.ONID = data.originalNetworkID;
				info.TSID = data.transportStreamID;
				info.SID = data.serviceID;
				info.EventID = data.eventID;
				wcscpy_s(info.serviceName, 256, data.stationName.c_str());
				wcscpy_s(info.bonDriverName, 256, this->bonFileName.c_str());
				info.bonDriverID = (this->tunerID & 0xFFFF0000)>>16;
				info.tunerID = this->tunerID & 0x0000FFFF;

				EPG_EVENT_INFO* epgInfo = NULL;
				EPGDB_EVENT_INFO epgDBInfo;
				if( info.EventID != 0xFFFF ){
					if( this->epgDBManager.SearchEpg(info.ONID, info.TSID, info.SID, info.EventID, &epgDBInfo) == TRUE ){
						epgInfo = new EPG_EVENT_INFO;
						CopyEpgInfo(epgInfo, &epgDBInfo);
					}
				}
				if( epgInfo != NULL ){
					if( plugIn.ConvertRecName2(&info, epgInfo, name, &size) == TRUE ){
						param.fileName = name;
					}
					SAFE_DELETE(epgInfo);
				}else{
					if( plugIn.ConvertRecName(&info, name, &size) == TRUE ){
						param.fileName = name;
					}
				}
			}
		}
		if( param.fileName.size() == 0 ){
			if( reserve->stratTime < nowTime ){
				SYSTEMTIME now;
				GetLocalTime(&now);
				//たぶん開始時間過ぎてる
				Format(param.fileName, L"%04d%02d%02d%02d%02d%02X%02X%02d-%s.ts",
					now.wYear,
					now.wMonth,
					now.wDay,
					now.wHour,
					now.wMinute,
					((this->tunerID & 0xFFFF0000)>>16),
					(this->tunerID & 0x0000FFFF),
					param.ctrlID,
					data.title.c_str()
					);
			}else{
				Format(param.fileName, L"%04d%02d%02d%02d%02d%02X%02X%02d-%s.ts",
					data.startTime.wYear,
					data.startTime.wMonth,
					data.startTime.wDay,
					data.startTime.wHour,
					data.startTime.wMinute,
					((this->tunerID & 0xFFFF0000)>>16),
					(this->tunerID & 0x0000FFFF),
					param.ctrlID,
					data.title.c_str()
					);
			}
		}
		//同時出力用ファイル名
		if( param.ctrlID == reserve->partialCtrlID && reserve->partialCtrlID != 0 ){
			//部分受信同時録画用
			if( data.recSetting.partialRecFolder.size() == 0 ){
				REC_FILE_SET_INFO folderItem;
				folderItem.recFolder = this->recFolderPath;
				folderItem.writePlugIn = this->recWritePlugIn;

				param.saveFolder.push_back(folderItem);
			}else{
				WORD partialSID = 0;
				wstring partialName = L"";
				if( FindPartialService(data.originalNetworkID, data.transportStreamID, data.serviceID, &partialSID, &partialName) == FALSE ){
					partialSID = data.serviceID;
					partialName = data.stationName;
				}
				for( size_t j=0; j<data.recSetting.partialRecFolder.size(); j++ ){
					if( data.recSetting.partialRecFolder[j].recNamePlugIn.size() > 0 ){
						CReNamePlugInUtil plugIn;
						wstring plugInPath;
						GetModuleFolderPath(plugInPath);
						plugInPath += L"\\RecName\\";
						plugInPath += data.recSetting.partialRecFolder[j].recNamePlugIn;

						if( plugIn.Initialize(plugInPath.c_str()) == TRUE ){
							WCHAR name[512] = L"";
							DWORD size = 512;
							PLUGIN_RESERVE_INFO info;

							info.startTime = data.startTime;
							info.durationSec = data.durationSecond;
							wcscpy_s(info.eventName, 512, data.title.c_str());
							info.ONID = data.originalNetworkID;
							info.TSID = data.transportStreamID;
							info.SID = partialSID;
							info.EventID = 0xFFFF;
							wcscpy_s(info.serviceName, 256, partialName.c_str());
							wcscpy_s(info.bonDriverName, 256, this->bonFileName.c_str());
							info.bonDriverID = (this->tunerID & 0xFFFF0000)>>16;
							info.tunerID = this->tunerID & 0x0000FFFF;

							EPG_EVENT_INFO* epgInfo = NULL;
							EPGDB_EVENT_INFO epgDBInfo;
							if( info.EventID != 0xFFFF ){
								if( this->epgDBManager.SearchEpg(info.ONID, info.TSID, info.SID, info.EventID, &epgDBInfo) == TRUE ){
									epgInfo = new EPG_EVENT_INFO;
									CopyEpgInfo(epgInfo, &epgDBInfo);
								}
							}
							if( epgInfo != NULL ){
								if( plugIn.ConvertRecName2(&info, epgInfo, name, &size) == TRUE ){
									wstring fileName = name;
									CheckFileName(fileName, noChkYen);
									data.recSetting.partialRecFolder[j].recFileName = fileName;
								}
								SAFE_DELETE(epgInfo);
							}else{
								if( plugIn.ConvertRecName(&info, name, &size) == TRUE ){
									wstring fileName = name;
									CheckFileName(fileName, noChkYen);
									data.recSetting.partialRecFolder[j].recFileName = fileName;
								}
							}
						}
					}
				}
				param.saveFolder = data.recSetting.partialRecFolder;
			}
		}else{
			//通常録画
			if( data.recSetting.recFolderList.size() == 0 ){
				REC_FILE_SET_INFO folderItem;
				folderItem.recFolder = this->recFolderPath;
				folderItem.writePlugIn = this->recWritePlugIn;

				param.saveFolder.push_back(folderItem);
			}else{
				for( size_t j=0; j<data.recSetting.recFolderList.size(); j++ ){
					if( data.recSetting.recFolderList[j].recNamePlugIn.size() > 0 ){
						CReNamePlugInUtil plugIn;
						wstring plugInPath;
						GetModuleFolderPath(plugInPath);
						plugInPath += L"\\RecName\\";
						plugInPath += data.recSetting.recFolderList[j].recNamePlugIn;

						if( plugIn.Initialize(plugInPath.c_str()) == TRUE ){
							WCHAR name[512] = L"";
							DWORD size = 512;
							PLUGIN_RESERVE_INFO info;

							info.startTime = data.startTime;
							info.durationSec = data.durationSecond;
							wcscpy_s(info.eventName, 512, data.title.c_str());
							info.ONID = data.originalNetworkID;
							info.TSID = data.transportStreamID;
							info.SID = data.serviceID;
							info.EventID = data.eventID;
							wcscpy_s(info.serviceName, 256, data.stationName.c_str());
							wcscpy_s(info.bonDriverName, 256, this->bonFileName.c_str());
							info.bonDriverID = (this->tunerID & 0xFFFF0000)>>16;
							info.tunerID = this->tunerID & 0x0000FFFF;

							EPG_EVENT_INFO* epgInfo = NULL;
							EPGDB_EVENT_INFO epgDBInfo;
							if( info.EventID != 0xFFFF ){
								if( this->epgDBManager.SearchEpg(info.ONID, info.TSID, info.SID, info.EventID, &epgDBInfo) == TRUE ){
									epgInfo = new EPG_EVENT_INFO;
									CopyEpgInfo(epgInfo, &epgDBInfo);
								}
							}
							if( epgInfo != NULL ){
								if( plugIn.ConvertRecName2(&info, epgInfo, name, &size) == TRUE ){
									wstring fileName = name;
									CheckFileName(fileName, noChkYen);
									data.recSetting.recFolderList[j].recFileName = fileName;
								}
								SAFE_DELETE(epgInfo);
							}else{
								if( plugIn.ConvertRecName(&info, name, &size) == TRUE ){
									wstring fileName = name;
									CheckFileName(fileName, noChkYen);
									data.recSetting.recFolderList[j].recFileName = fileName;
								}
							}
						}
					}
				}
				param.saveFolder = data.recSetting.recFolderList;
			}
		}
		param.overWriteFlag = this->recOverWrite;
		param.pittariFlag = data.recSetting.pittariFlag;
		param.pittariONID = data.originalNetworkID;
		param.pittariTSID = data.transportStreamID;
		param.pittariSID = data.serviceID;
		param.pittariEventID = data.eventID;

		CheckFileName(param.fileName, noChkYen);

		DWORD durationSec = data.durationSecond;
		if( data.recSetting.continueRecFlag == 1 ){
			DWORD sumSec = 0;
			IsFindContinueReserve(reserve, &sumSec);
			durationSec += sumSec;
		}

		if( this->keepDisk == 1 ){
			DWORD bitrate = 0;
			_GetBitrate(data.originalNetworkID, data.transportStreamID, data.serviceID, &bitrate);
			param.createSize = ((ULONGLONG)(bitrate/8)*1000) * durationSec;
		}else{
			param.createSize = 0;
		}

		if( this->sendCtrl.SendViewStartRec(param) != CMD_SUCCESS){
			if( reserve->ctrlID[i] == reserve->mainCtrlID ){
				ret = FALSE;
			}
		}
	}

	if( this->twitterManager != NULL && sendNoyify == TRUE){
		this->twitterManager->SendTweet(TW_REC_START, &data, NULL, NULL);
	}

	if( sendNoyify == TRUE){
		wstring msg;
		Format(msg, L"%s %04d/%02d/%02d %02d:%02d:%02d\r\n%s", 
			data.stationName.c_str(),
			data.startTime.wYear,
			data.startTime.wMonth,
			data.startTime.wDay,
			data.startTime.wHour,
			data.startTime.wMinute,
			data.startTime.wSecond,
			data.title.c_str()
			);
		this->notifyManager.AddNotifyMsg(NOTIFY_UPDATE_REC_START, msg);
	}
	return ret;
}

BOOL CTunerBankCtrl::IsFindContinueReserve(RESERVE_WORK* reserve, DWORD* continueSec)
{
	if( reserve->continueRecFlag == 0 ){
		return FALSE;
	}
	BOOL ret = FALSE;

	map<DWORD, RESERVE_WORK*>::iterator itr;
	for( itr = this->reserveWork.begin(); itr != this->reserveWork.end(); itr++ ){
		if( itr->second->ONID == reserve->ONID &&
			itr->second->TSID == reserve->TSID &&
			itr->second->SID == reserve->SID &&
			itr->second->stratTime != reserve->stratTime
			){
				if( reserve->stratTime < itr->second->stratTime &&
					itr->second->stratTime <= reserve->endTime &&
					reserve->endTime < itr->second->endTime){
					//開始時間が現在の予約中にあるので連続
					RESERVE_DATA data;
					itr->second->reserveInfo->GetData(&data);
					*continueSec = data.durationSecond;
					if( itr->second->continueRecFlag == 1 ){
						DWORD sumSec = 0;
						IsFindContinueReserve(itr->second, &sumSec);
						*continueSec += sumSec;
					}
					ret = TRUE;
					break;
				}
		}
	}

	return ret;
}

BOOL CTunerBankCtrl::CloseTuner()
{
	CloseTunerExe(this->processID);
	this->processID = 0;
	this->openTuner = FALSE;
	this->delayTime = 0;

	return TRUE;
}

void CTunerBankCtrl::AddEndReserve(RESERVE_WORK* reserve, DWORD endType, SET_CTRL_REC_STOP_RES_PARAM resVal)
{
	END_RESERVE_INFO* item = new END_RESERVE_INFO;
	item->reserveInfo = reserve->reserveInfo;
	item->tunerID = this->tunerID;
	item->reserveID = reserve->reserveID;
	item->endType = endType;
	item->recFilePath = resVal.recFilePath;
	item->drop = resVal.drop;
	item->scramble = resVal.scramble;
	item->continueRecStartFlag = reserve->continueRecStartFlag;

	if( reserve->eventInfo != NULL && reserve->eventInfo->shortInfo != NULL && reserve->eventInfo->StartTimeFlag != 0 ){
		item->epgEventName = reserve->eventInfo->shortInfo->event_name;
		item->epgOriginalNetworkID = reserve->eventInfo->original_network_id;
		item->epgTransportStreamID = reserve->eventInfo->transport_stream_id;
		item->epgServiceID = reserve->eventInfo->service_id;
		item->epgStartTime = reserve->eventInfo->start_time;
	}

	endList.push_back(item);
}

void CTunerBankCtrl::GetEndReserve(map<DWORD, END_RESERVE_INFO*>* reserveMap)
{
	CBlockLock lock(&this->bankLock);

	for( size_t i=0; i<this->endList.size(); i++ ){

		reserveMap->insert(pair<DWORD, END_RESERVE_INFO*>(this->endList[i]->reserveID, this->endList[i]));
	}
	this->endList.clear();
}

BOOL CTunerBankCtrl::IsRecWork()
{
	CBlockLock lock(&this->bankLock);

	BOOL ret = FALSE;
	map<DWORD, RESERVE_WORK*>::iterator itr;
	for( itr = this->reserveWork.begin(); itr != this->reserveWork.end(); itr++ ){
		if( itr->second->recStartFlag == TRUE ){
			ret = TRUE;
			break;
		}
	}

	return ret;
}

BOOL CTunerBankCtrl::IsOpenTuner()
{
	CBlockLock lock(&this->bankLock);

	return this->openTuner;
}

BOOL CTunerBankCtrl::GetCurrentChID(DWORD* currentChID)
{
	CBlockLock lock(&this->bankLock);

	DWORD ret = FALSE;
	if( this->openTuner == TRUE && this->epgCapWork == FALSE ){
		ret = TRUE;
		if( currentChID != NULL ){
			*currentChID = this->currentChID;
		}
	}

	return ret;
}

BOOL CTunerBankCtrl::IsSuspendOK()
{
	CBlockLock lock(&this->bankLock);

	BOOL ret = FALSE;

	multimap<LONGLONG, RESERVE_WORK*> sortList;
	this->GetCheckList(&sortList);

	BOOL viewMode = FALSE;
	SET_CH_INFO initCh;
	BOOL needOpenTuner = IsNeedOpenTuner(&sortList, &viewMode, &initCh);
	if( needOpenTuner == FALSE && this->epgCapWork == FALSE){
		ret = TRUE;
		if( this->openTuner == TRUE ){
			CloseTuner();
		}
	}

	return ret;
}

BOOL CTunerBankCtrl::IsEpgCapOK(int ngCapMin)
{
	CBlockLock lock(&this->bankLock);

	BOOL ret = TRUE;
	if( this->openTuner == TRUE ){
		return FALSE;
	}

	multimap<LONGLONG, RESERVE_WORK*> sortList;
	this->GetCheckList(&sortList);
	if( sortList.size() == 0 ){
		return TRUE;
	}

	multimap<LONGLONG, RESERVE_WORK*>::iterator itr;
	itr = sortList.begin();
	LONGLONG startTime = itr->second->stratTime - max(itr->second->startMargine, itr->second->stratTime - itr->second->endTime);

	if( startTime < GetNowI64Time() + (ngCapMin*60*I64_1SEC) ){
		ret = FALSE;
	}

	return ret;
}

BOOL CTunerBankCtrl::IsEpgCapWorking()
{
	CBlockLock lock(&this->bankLock);

	return this->epgCapWork;
}

void CTunerBankCtrl::ClearEpgCapItem()
{
	CBlockLock lock(&this->bankLock);

	this->epgCapItem.clear();
}

void CTunerBankCtrl::AddEpgCapItem(const SET_CH_INFO& info)
{
	CBlockLock lock(&this->bankLock);

	this->epgCapItem.push_back(info);
}

void CTunerBankCtrl::StartEpgCap()
{
	CBlockLock lock(&this->bankLock);

	if( epgCapItem.size() == 0 ){
		return ;
	}

	if( this->checkThread == NULL ){
		ResetEvent(this->checkStopEvent);
		this->checkThread = (HANDLE)_beginthreadex(NULL, 0, CheckReserveThread, (LPVOID)this, CREATE_SUSPENDED, NULL);
		SetThreadPriority( this->checkThread, THREAD_PRIORITY_NORMAL );
		ResumeThread(this->checkThread);
	}

	this->epgCapWork = TRUE;
}

void CTunerBankCtrl::StopEpgCap()
{
	CBlockLock lock(&this->bankLock);

	this->epgCapWork = FALSE;
}

//起動中のチューナーからEPGデータの検索
//戻り値：
// エラーコード
// val					[IN]取得番組
// resVal				[OUT]番組情報
BOOL CTunerBankCtrl::SearchEpgInfo(
	SEARCH_EPG_INFO_PARAM* val,
	EPGDB_EVENT_INFO* resVal
	)
{
	CBlockLock lock(&this->bankLock);

	BOOL ret = FALSE;
	if( this->openTuner == TRUE && this->epgCapWork == FALSE ){
		if( this->sendCtrl.SendViewSearchEvent(val, resVal) == CMD_SUCCESS ){
			ret = TRUE;
		}
	}

	return ret;
}

//起動中のチューナーから現在or次の番組情報を取得する
//戻り値：
// エラーコード
// val					[IN]取得番組
// resVal				[OUT]番組情報
DWORD CTunerBankCtrl::GetEventPF(
	GET_EPG_PF_INFO_PARAM* val,
	EPGDB_EVENT_INFO* resVal
	)
{
	CBlockLock lock(&this->bankLock);

	BOOL ret = FALSE;
	if( this->openTuner == TRUE && this->epgCapWork == FALSE ){
		if( this->sendCtrl.SendViewGetEventPF(val, resVal) == CMD_SUCCESS ){
			ret = TRUE;
		}
	}

	return ret;
}

LONGLONG CTunerBankCtrl::DelayTime()
{
	CBlockLock lock(&this->bankLock);

	LONGLONG ret = 0;
	if( this->openTuner == TRUE ){
		ret = this->delayTime;
	}

	return ret;
}

BOOL CTunerBankCtrl::ReRec(DWORD reserveID, BOOL deleteFile)
{
	CBlockLock lock(&this->bankLock);

	LONGLONG nowTime = GetNowI64Time();
	nowTime += this->delayTime;

	BOOL ret = FALSE;
	map<DWORD, RESERVE_WORK*>::iterator itr;
	itr = this->createCtrlList.find(reserveID);
	if( itr != this->createCtrlList.end() ){
		for(size_t i=0; i<itr->second->ctrlID.size(); i++ ){
			SET_CTRL_REC_STOP_PARAM param;
			param.ctrlID = itr->second->ctrlID[i];
			param.saveErrLog = FALSE;
			SET_CTRL_REC_STOP_RES_PARAM resVal;
			this->sendCtrl.SendViewStopRec(param, &resVal);
			if( resVal.recFilePath.size() > 0 ){
				if(deleteFile == TRUE ){
					DeleteFile( resVal.recFilePath.c_str() );

					wstring errFile = resVal.recFilePath;
					errFile += L".err";
					DeleteFile( errFile.c_str() );

					wstring pgFile = resVal.recFilePath;
					pgFile += L".program.txt";
					DeleteFile( pgFile.c_str() );
				}
			}
		}
		//開始時間過ぎているので録画開始
		if( RecStart(nowTime, itr->second, FALSE) == TRUE ){
			itr->second->recStartFlag = TRUE;
		}
	}

	return ret;
}

BOOL CTunerBankCtrl::GetRecFilePath(
	DWORD reserveID,
	wstring& filePath,
	DWORD* ctrlID,
	DWORD* processID
	)
{
	CBlockLock lock(&this->bankLock);
	BOOL ret = FALSE;

	map<DWORD, RESERVE_WORK*>::iterator itr;
	itr = this->createCtrlList.find(reserveID);
	if( itr != this->createCtrlList.end() ){
		wstring recFilePath = L"";
		if( this->sendCtrl.SendViewGetRecFilePath(itr->second->mainCtrlID, &recFilePath) == CMD_SUCCESS ){
			filePath = recFilePath;
			*ctrlID = itr->second->mainCtrlID;
			*processID = this->processID;
			ret = TRUE;
		}
	}

	return ret;
}

BOOL CTunerBankCtrl::OpenTunerExe(
	LPCWSTR exePath,
	LPCWSTR bonDriver,
	DWORD id,
	BOOL minWake, BOOL noView, BOOL noNW, BOOL nwUdp, BOOL nwTcp,
	DWORD priority,
	const map<DWORD, DWORD>& registGUIMap,
	DWORD* pid
	)
{
	wstring strExecute;
	strExecute += L"\"";
	strExecute += exePath;
	strExecute += L"\" ";

	wstring strIni;
	GetModuleFolderPath(strIni);
	strIni += L"\\ViewApp.ini";

	WCHAR buff[512];
	GetPrivateProfileString(L"APP_CMD_OPT", L"Bon", L"-d", buff, 512, strIni.c_str());
	strExecute += buff;
	strExecute += L" ";
	strExecute += bonDriver;

	if( minWake != FALSE ){
		GetPrivateProfileString(L"APP_CMD_OPT", L"Min", L"-min", buff, 512, strIni.c_str());
		strExecute += L" ";
		strExecute += buff;
	}
	if( noView != FALSE ){
		GetPrivateProfileString(L"APP_CMD_OPT", L"ViewOff", L"-noview", buff, 512, strIni.c_str());
		strExecute += L" ";
		strExecute += buff;
	}
	if( noNW != FALSE ){
		GetPrivateProfileString(L"APP_CMD_OPT", L"NetworkOff", L"-nonw", buff, 512, strIni.c_str());
		strExecute += L" ";
		strExecute += buff;
	}else{
		if( nwUdp != FALSE ){
			strExecute += L" -nwudp";
		}
		if( nwTcp != FALSE ){
			strExecute += L" -nwtcp";
		}
	}

	BOOL bRet = FALSE;
	HANDLE openWaitEvent = _CreateEvent(FALSE, TRUE, _T("Global\\EpgTimerSrv_OpenTuner_Event"));
	if( openWaitEvent != NULL ){
		if( WaitForSingleObject(openWaitEvent, INFINITE) == WAIT_OBJECT_0 ){
			map<DWORD, DWORD>::const_iterator itr;
			for( itr = registGUIMap.begin(); itr != registGUIMap.end(); itr++ ){
				CSendCtrlCmd cmdSend;
				cmdSend.SetPipeSetting(CMD2_GUI_CTRL_WAIT_CONNECT, CMD2_GUI_CTRL_PIPE, itr->first);
				cmdSend.SetConnectTimeOut(20 * 1000);
				if( cmdSend.SendGUIExecute(strExecute, pid) == CMD_SUCCESS ){
					bRet = TRUE;
					break;
				}
			}
			if( bRet == FALSE ){
				OutputDebugString(L"gui exec err");
				//GUI経由で起動できなかった
				PROCESS_INFORMATION pi;
				STARTUPINFO si = {};
				si.cb = sizeof(si);
				vector<WCHAR> strBuff(strExecute.c_str(), strExecute.c_str() + strExecute.size() + 1);
				if( CreateProcess(NULL, &strBuff.front(), NULL, NULL, FALSE, GetPriorityClass(GetCurrentProcess()), NULL, NULL, &si, &pi) != FALSE ){
					bRet = TRUE;
					*pid = pi.dwProcessId;
					CloseHandle(pi.hThread);
					CloseHandle(pi.hProcess);
				}
			}

			if( bRet != FALSE ){
				//IDのセット
				CSendCtrlCmd cmdSend;
				cmdSend.SetPipeSetting(CMD2_VIEW_CTRL_WAIT_CONNECT, CMD2_VIEW_CTRL_PIPE, *pid);
				ctrlCmd.SetConnectTimeOut(0);
				bRet = FALSE;
				for( int i = 0; i < 300; i++ ){
					Sleep(100);
					if( cmdSend.SendViewSetID(id) == CMD_SUCCESS ){
						bRet = TRUE;
						break;
					}
				}
				ctrlCmd.SetConnectTimeOut(CONNECT_TIMEOUT);
				if( bRet == FALSE ){
					CloseTunerExe(*pid);
				}else{
					//起動ステータスを確認
					DWORD status = 0;
					if( cmdSend.SendViewGetStatus(&status) == CMD_SUCCESS ){
						if( status == VIEW_APP_ST_ERR_BON ){
							CloseTunerExe(*pid);
							bRet = FALSE;
						}else if( 0 <= priority && priority <= 5 ){
							HANDLE hProcess = OpenProcess(PROCESS_SET_INFORMATION, FALSE, *pid);
							if( hProcess != NULL ){
								SetPriorityClass(hProcess,
									priority == 0 ? REALTIME_PRIORITY_CLASS :
									priority == 1 ? HIGH_PRIORITY_CLASS :
									priority == 2 ? ABOVE_NORMAL_PRIORITY_CLASS :
									priority == 3 ? NORMAL_PRIORITY_CLASS :
									priority == 4 ? BELOW_NORMAL_PRIORITY_CLASS : IDLE_PRIORITY_CLASS);
								CloseHandle(hProcess);
							}
						}
					}
				}
			}
			SetEvent(openWaitEvent);
		}
		CloseHandle(openWaitEvent);
	}
	return bRet;
}

void CTunerBankCtrl::CloseTunerExe(
	DWORD pid
	)
{
	if( _FindOpenExeProcess(pid) == FALSE ){
		return;
	}

	CSendCtrlCmd cmdSend;
	cmdSend.SetPipeSetting(CMD2_VIEW_CTRL_WAIT_CONNECT, CMD2_VIEW_CTRL_PIPE, pid);
	cmdSend.SendViewAppClose();
	for( int i = 0; i < 60; i++ ){
		if( _FindOpenExeProcess(pid) == FALSE ){
			return;
		}
		Sleep(500);
	}
	//ぶち殺す
	HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
	if( hProcess != NULL ){
		TerminateProcess(hProcess, 0xFFFFFFFF);
		CloseHandle(hProcess);
		Sleep(500);
	}
}
