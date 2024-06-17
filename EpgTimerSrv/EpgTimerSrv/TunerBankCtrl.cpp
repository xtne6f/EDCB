#include "stdafx.h"
#include "TunerBankCtrl.h"
#include "../../Common/EpgTimerUtil.h"
#include "../../Common/SendCtrlCmd.h"
#include "../../Common/PathUtil.h"
#include "../../Common/TimeUtil.h"
#ifdef _WIN32
#include "../../Common/IniUtil.h"
#include <tlhelp32.h>
#else
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

CTunerBankCtrl::CTunerBankCtrl(DWORD tunerID_, LPCWSTR bonFileName_, WORD epgCapMax, const map<DWORD, CH_DATA4>& chMap_, CNotifyManager& notifyManager_, CEpgDBManager& epgDBManager_)
	: tunerID(tunerID_)
	, bonFileName(bonFileName_)
	, epgCapMaxOfThisBon(epgCapMax)
	, notifyManager(notifyManager_)
	, epgDBManager(epgDBManager_)
	, tunerPid(0)
	, specialState(TR_IDLE)
	, delayTime(0)
	, epgCapDelayTime(0)
{
	for( auto itr = chMap_.begin(); itr != chMap_.end(); itr++ ){
		this->chMap.insert(std::make_pair(Create64Key(itr->second.originalNetworkID, itr->second.transportStreamID, itr->second.serviceID), itr->second));
	}
	this->watchContext.count = 0;
}

CTunerBankCtrl::~CTunerBankCtrl()
{
#ifdef _WIN32
	if( this->tunerPid ){
		CloseHandle(this->hTunerProcess);
	}
#endif
}

void CTunerBankCtrl::ReloadSetting(const CEpgTimerSrvSetting::SETTING& s)
{
	//モジュールini以外のパラメータは必要なときにその場で取得する
	//録画開始のちょうどn分前だと起動と他チューナ録画開始が若干重なりやすくなるので僅かにずらす
	this->recWakeTime = max(s.recAppWakeTime * 60 - 3, READY_MARGIN) * I64_1SEC;
	this->recMinWake = s.recMinWake;
	this->openViewForViewing = s.openViewForViewing && s.openViewAlways == false;
	this->openViewForRec = s.openViewForRec && s.openViewAlways == false;
	this->openViewAlways = s.openViewAlways;
	this->recNW = s.recNW;
	this->backPriority = s.backPriority;
	this->saveProgramInfo = s.pgInfoLog;
	this->saveProgramInfoAsUtf8 = s.pgInfoLogAsUtf8;
	this->saveErrLog = s.dropLog;
	this->recOverWrite = s.recOverWrite;
	this->processPriority = s.processPriority;
	this->keepDisk = s.keepDisk;
	this->recNameNoChkYen = s.noChkYen;
	this->recNamePlugInFileName = s.recNamePlugIn ? s.recNamePlugInFile : wstring();
	this->tsExt = s.tsExt;
}

const CH_DATA4* CTunerBankCtrl::GetCh(WORD onid, WORD tsid, WORD sid) const
{
	auto itr = this->chMap.find(Create64Key(onid, tsid, sid));
	return itr == this->chMap.end() ? NULL : &itr->second;
}

bool CTunerBankCtrl::AddReserve(const TUNER_RESERVE& reserve)
{
	if( reserve.reserveID == 0 ||
	    this->reserveMap.count(reserve.reserveID) != 0 ||
	    reserve.recMode > RECMODE_VIEW ){
		return false;
	}
	TUNER_RESERVE_WORK& r = this->reserveMap.insert(std::make_pair(reserve.reserveID, TUNER_RESERVE_WORK())).first->second;
	static_cast<TUNER_RESERVE&>(r) = reserve;
	r.startOrder = (r.startTime - r.startMargin) / I64_1SEC << 16 | (r.reserveID & 0xFFFF);
	r.effectivePriority = (this->backPriority ? -1 : 1) * ((LONGLONG)((this->backPriority ? r.priority : ~r.priority) & 7) << 60 | r.startOrder);
	r.state = TR_IDLE;
	r.retryOpenCount = 0;
	return true;
}

bool CTunerBankCtrl::ChgCtrlReserve(TUNER_RESERVE* reserve)
{
	auto itr = this->reserveMap.find(reserve->reserveID);
	if( itr != this->reserveMap.end() && itr->second.state != TR_IDLE ){
		const TUNER_RESERVE& save = itr->second;
		//変更できないフィールドを上書き
		reserve->onid = save.onid;
		reserve->tsid = save.tsid;
		reserve->sid = save.sid;
		//プログラム予約への変更のみ認める
		if( reserve->eid != 0xFFFF ){
			reserve->eid = save.eid;
		}
		reserve->recMode = save.recMode;
		reserve->priority = save.priority;
		reserve->enableCaption = save.enableCaption;
		reserve->enableData = save.enableData;
		reserve->pittari = save.pittari;
		reserve->partialRecMode = save.partialRecMode;
		reserve->recFolder = save.recFolder;
		reserve->partialRecFolder = save.partialRecFolder;
		//後方移動は注意。なお前方移動はどれだけ大きくても次のCheck()で予約終了するだけなので問題ない
		if( reserve->startTime - reserve->startMargin > save.startTime - save.startMargin ){
			LONGLONG now = GetNowI64Time() + this->delayTime;
			if( reserve->startTime - reserve->startMargin - 60 * I64_1SEC > now ){
				reserve->startTime = save.startTime;
				reserve->startMargin = save.startMargin;
			}
		}
		TUNER_RESERVE_WORK& r = itr->second;
		static_cast<TUNER_RESERVE&>(r) = *reserve;
		//内部パラメータを再計算
		r.startOrder = (r.startTime - r.startMargin) / I64_1SEC << 16 | (r.reserveID & 0xFFFF);
		r.effectivePriority = (this->backPriority ? -1 : 1) * ((LONGLONG)((this->backPriority ? r.priority : ~r.priority) & 7) << 60 | r.startOrder);
		return true;
	}
	return false;
}

bool CTunerBankCtrl::DelReserve(DWORD reserveID, vector<CHECK_RESULT>* retList)
{
	auto itr = this->reserveMap.find(reserveID);
	if( itr != this->reserveMap.end() ){
		if( itr->second.state != TR_IDLE ){
			//tunerPidは必ず非0
			CWatchBlock watchBlock(&this->watchContext);
			CSendCtrlCmd ctrlCmd;
			ctrlCmd.SetPipeSetting(CMD2_VIEW_CTRL_PIPE, this->tunerPid);
			CHECK_RESULT ret;
			ret.type = 0;
			ret.reserveID = reserveID;
			ret.continueRec = false;
			ret.drops = 0;
			ret.scrambles = 0;
			bool isMainCtrl = true;
			for( int i = 0; i < 2; i++ ){
				if( itr->second.ctrlID[i] != 0 ){
					if( itr->second.state == TR_REC ){
						if( itr->second.recMode == RECMODE_VIEW ){
							if( isMainCtrl ){
								ret.type = CHECK_END;
							}
						}else{
							SET_CTRL_REC_STOP_PARAM param;
							param.ctrlID = itr->second.ctrlID[i];
							param.saveErrLog = this->saveErrLog;
							SET_CTRL_REC_STOP_RES_PARAM resVal;
							if( ctrlCmd.SendViewStopRec(param, &resVal) != CMD_SUCCESS ){
								if( isMainCtrl ){
									ret.type = CHECK_ERR_RECEND;
								}
							}else if( isMainCtrl ){
								ret.type = resVal.subRecFlag ? CHECK_END_END_SUBREC :
								           itr->second.notStartHead ? CHECK_END_NOT_START_HEAD :
								           itr->second.savedPgInfo == false ? CHECK_END_NOT_FIND_PF : CHECK_END;
								ret.recFilePath = resVal.recFilePath;
								ret.drops = resVal.drop;
								ret.scrambles = resVal.scramble;
								ret.epgStartTime = itr->second.epgStartTime;
								ret.epgEventName = itr->second.epgEventName;
							}
						}
					}
					ctrlCmd.SendViewDeleteCtrl(itr->second.ctrlID[i]);
					isMainCtrl = false;
				}
			}
			if( ret.type != 0 && retList ){
				retList->push_back(ret);
			}
			if( itr->second.state == TR_REC ){
				//録画終了に伴ってGUIキープが解除されたかもしれない
				this->tunerResetLock = true;
			}
		}
		this->reserveMap.erase(itr);
		return true;
	}
	return false;
}

void CTunerBankCtrl::ClearNoCtrl(LONGLONG startTime)
{
	for( auto itr = this->reserveMap.begin(); itr != this->reserveMap.end(); ){
		if( itr->second.state == TR_IDLE && itr->second.startTime - itr->second.startMargin >= startTime ){
			this->reserveMap.erase(itr++);
		}else{
			itr++;
		}
	}
}

vector<DWORD> CTunerBankCtrl::GetReserveIDList() const
{
	vector<DWORD> list;
	list.reserve(this->reserveMap.size());
	for( auto itr = this->reserveMap.cbegin(); itr != this->reserveMap.end(); itr++ ){
		list.push_back(itr->first);
	}
	return list;
}

vector<CTunerBankCtrl::CHECK_RESULT> CTunerBankCtrl::Check(vector<DWORD>* startedReserveIDList)
{
	vector<CHECK_RESULT> retList;

	if( this->tunerPid &&
#ifdef _WIN32
	    WaitForSingleObject(this->hTunerProcess, 0) != WAIT_TIMEOUT
#else
	    waitpid(this->tunerPid, NULL, WNOHANG) != 0
#endif
	    ){
		//チューナが予期せず閉じられた
		CloseTuner();
		this->specialState = TR_IDLE;
		//TR_IDLEでない全予約を葬る
		for( auto itr = this->reserveMap.cbegin(); itr != this->reserveMap.end(); ){
			if( itr->second.state != TR_IDLE ){
				CHECK_RESULT ret;
				ret.type = CHECK_ERR_REC;
				ret.reserveID = itr->first;
				retList.push_back(ret);
				this->reserveMap.erase(itr++);
			}else{
				itr++;
			}
		}
	}

	CWatchBlock watchBlock(&this->watchContext);
	CSendCtrlCmd ctrlCmd;
	DWORD getStatus = CMD_ERR;
	if( this->tunerPid ){
		//チューナ起動時にはこれを再度呼ぶこと
		ctrlCmd.SetPipeSetting(CMD2_VIEW_CTRL_PIPE, this->tunerPid);

		//チューナが対応していれば現在の状態、BonDriver、時計の誤差などをまとめて取得する
		if( this->getStatusDetailsNonSupport ){
			getStatus = CMD_NON_SUPPORT;
		}else{
			getStatus = ctrlCmd.SendViewGetStatusDetails(
				(this->specialState == TR_EPGCAP ? VIEW_APP_FLAG_GET_STATUS : 0) | VIEW_APP_FLAG_GET_BONDRIVER |
				(this->specialState != TR_NWTV ? VIEW_APP_FLAG_GET_DELAY : 0), &this->statusInfo);
			this->getStatusDetailsNonSupport = getStatus == CMD_NON_SUPPORT;
		}
	}

	if( this->specialState == TR_EPGCAP ){
		if( (getStatus == CMD_NON_SUPPORT ? ctrlCmd.SendViewGetStatus(&this->statusInfo.status) : getStatus) == CMD_SUCCESS ){
			if( this->statusInfo.status != VIEW_APP_ST_GET_EPG ){
				//取得終わった
				AddDebugLog(L"epg end");
				CloseTuner();
				this->specialState = TR_IDLE;
			}
		}else{
			//エラー
			AddDebugLog(L"epg err");
			CloseTuner();
			this->specialState = TR_IDLE;
		}
	}else if( this->specialState == TR_NWTV ){
		//ネットワークモードではGUIキープできないのでBonDriverが変更されるかもしれない
		//BonDriverが変更されたチューナはこのバンクの管理下に置けないので、ネットワークモードを解除する
		if( (getStatus == CMD_NON_SUPPORT ? ctrlCmd.SendViewGetBonDrivere(&this->statusInfo.bonDriver) : getStatus) == CMD_SUCCESS &&
		    UtilComparePath(this->statusInfo.bonDriver.c_str(), this->bonFileName.c_str()) != 0 ){
			if( ctrlCmd.SendViewSetID(-1) == CMD_SUCCESS ){
				lock_recursive_mutex lock(this->watchContext.lock);
#ifdef _WIN32
				CloseHandle(this->hTunerProcess);
#endif
				this->tunerPid = 0;
				this->specialState = TR_IDLE;
			}else{
				//ID剥奪に失敗したので消えてもらうしかない
				CloseNWTV();
			}
			//TODO: 汎用のログ用メッセージが存在しないので、やむを得ずNOTIFY_UPDATE_REC_ENDで警告する
			this->notifyManager.AddNotifyMsg(NOTIFY_UPDATE_REC_END,
				L"BonDriverが変更されたためNetworkモードを解除しました\r\n変更したBonDriverに録画の予定がないか注意してください");
		}
	}else if( this->tunerPid && this->tunerChLocked == false ){
		//GUIキープされていないのでBonDriverが変更されるかもしれない
		if( (getStatus == CMD_NON_SUPPORT ? ctrlCmd.SendViewGetBonDrivere(&this->statusInfo.bonDriver) : getStatus) == CMD_SUCCESS &&
		    UtilComparePath(this->statusInfo.bonDriver.c_str(), this->bonFileName.c_str()) != 0 ){
			if( ctrlCmd.SendViewSetID(-1) == CMD_SUCCESS ){
				lock_recursive_mutex lock(this->watchContext.lock);
#ifdef _WIN32
				CloseHandle(this->hTunerProcess);
#endif
				this->tunerPid = 0;
			}else{
				//ID剥奪に失敗したので消えてもらうしかない
				CloseTuner();
			}
			//TR_IDLEでない全予約を葬る
			for( auto itr = this->reserveMap.cbegin(); itr != this->reserveMap.end(); ){
				if( itr->second.state != TR_IDLE ){
					CHECK_RESULT ret;
					ret.type = CHECK_ERR_REC;
					ret.reserveID = itr->first;
					retList.push_back(ret);
					this->reserveMap.erase(itr++);
				}else{
					itr++;
				}
			}
		}
	}

	this->delayTime = 0;
	this->epgCapDelayTime = 0;
	if( this->tunerPid && this->specialState != TR_NWTV ){
		//PC時計との誤差取得
		if( (getStatus == CMD_NON_SUPPORT ? ctrlCmd.SendViewGetDelay(&this->statusInfo.delaySec) : getStatus) == CMD_SUCCESS ){
			//誤った値を掴んでおかしなことにならないよう、EPG取得中の値は状態遷移の参考にしない
			if( this->specialState == TR_EPGCAP ){
				this->epgCapDelayTime = this->statusInfo.delaySec * I64_1SEC;
			}else{
				this->delayTime = this->statusInfo.delaySec * I64_1SEC;
			}
		}
	}
	LONGLONG now = GetNowI64Time() + this->delayTime;

	//終了時間を過ぎた予約を回収し、TR_IDLE->TR_READY以外の遷移をする
	vector<pair<LONGLONG, DWORD>> idleList;
	bool ngResetLock = false;
	for( auto itrRes = this->reserveMap.begin(); itrRes != this->reserveMap.end(); ){
		TUNER_RESERVE_WORK& r = itrRes->second;
		CHECK_RESULT ret;
		ret.type = 0;
		switch( r.state ){
		case TR_IDLE:
			if( r.startTime + r.endMargin + r.durationSecond * I64_1SEC < now ){
				ret.type = CHECK_ERR_PASS;
			}
			//開始順が秒精度なので、前後関係を確実にするため開始時間は必ず秒精度で扱う
			else if( (r.startTime - r.startMargin - this->recWakeTime) / I64_1SEC < now / I64_1SEC ){
				//録画開始recWakeTime前～
				idleList.push_back(std::make_pair(r.startOrder, r.reserveID));
			}
			break;
		case TR_READY:
			if( r.startTime + r.endMargin + r.durationSecond * I64_1SEC < now ){
				for( int i = 0; i < 2; i++ ){
					if( r.ctrlID[i] != 0 ){
						ctrlCmd.SendViewDeleteCtrl(r.ctrlID[i]);
					}
				}
				ret.type = CHECK_ERR_PASS;
			}
			//パイプコマンドにはチャンネル変更の完了を調べる仕組みがないので、妥当な時間だけ待つ
			else if( GetU32Tick() - this->tunerChChgTick > 5000 && r.startTime - r.startMargin < now ){
				//録画開始～
				if( RecStart(r, now) ){
					//途中から開始されたか
					r.notStartHead = r.startTime - r.startMargin + 60 * I64_1SEC < now;
					r.appendPgInfo = false;
					r.savedPgInfo = false;
					r.state = TR_REC;
					if( r.recMode == RECMODE_VIEW ){
						//視聴予約でない予約が1つでもあれば「視聴モード」にしない
						auto itr = this->reserveMap.cbegin();
						for( ; itr != this->reserveMap.end(); itr++ ){
							if( itr->second.state != TR_IDLE && itr->second.recMode != RECMODE_VIEW ){
								break;
							}
						}
						if( itr == this->reserveMap.end() ){
							//「視聴モード」にするとGUIキープが解除されてしまうためチャンネルを把握することはできない
							ctrlCmd.SendViewSetStandbyRec(2);
							this->tunerChLocked = false;
							if( this->openViewForViewing ){
								ctrlCmd.SendViewExecViewApp();
							}
						}
					}else{
						//録画ファイルパス取得
						for( int i = 0; i < 2; i++ ){
							if( r.ctrlID[i] != 0 ){
								//ぴったり録画は取得成功が遅れる
								//たとえサブ録画が発生してもこのコマンドで得られるパスは変化しない
								ctrlCmd.SendViewGetRecFilePath(r.ctrlID[i], &r.recFilePath[i]);
							}
						}
						if( this->openViewForRec ){
							ctrlCmd.SendViewExecViewApp();
						}
					}
					if( startedReserveIDList ){
						startedReserveIDList->push_back(r.reserveID);
					}
				}else{
					//開始できなかった
					ret.type = CHECK_ERR_RECSTART;
				}
			}
			break;
		case TR_REC:
			{
				//ステータス確認
				DWORD status;
				if( r.recMode != RECMODE_VIEW && ctrlCmd.SendViewGetStatus(&status) == CMD_SUCCESS && status != VIEW_APP_ST_REC ){
					//キャンセルされた？
					ret.type = CHECK_END_CANCEL;
					ret.recFilePath = r.ctrlID[0] != 0 ? r.recFilePath[0] : r.recFilePath[1];
					ret.continueRec = false;
					ret.drops = 0;
					ret.scrambles = 0;
					this->tunerResetLock = true;
				}else if( r.startTime + r.endMargin + r.durationSecond * I64_1SEC < now ){
					ret.type = CHECK_ERR_REC;
					ret.continueRec = false;
					ret.drops = 0;
					ret.scrambles = 0;
					bool isMainCtrl = true;
					for( int i = 0; i < 2; i++ ){
						if( r.ctrlID[i] != 0 ){
							if( r.recMode == RECMODE_VIEW ){
								if( isMainCtrl ){
									ret.type = CHECK_END;
								}
							}else{
								SET_CTRL_REC_STOP_PARAM param;
								param.ctrlID = r.ctrlID[i];
								param.saveErrLog = this->saveErrLog;
								SET_CTRL_REC_STOP_RES_PARAM resVal;
								if( ctrlCmd.SendViewStopRec(param, &resVal) != CMD_SUCCESS ){
									if( isMainCtrl ){
										ret.type = CHECK_ERR_RECEND;
									}
								}else if( isMainCtrl ){
									ret.type = resVal.subRecFlag ? CHECK_END_END_SUBREC :
									           r.notStartHead ? CHECK_END_NOT_START_HEAD :
									           r.savedPgInfo == false ? CHECK_END_NOT_FIND_PF : CHECK_END;
									ret.recFilePath = resVal.recFilePath;
									ret.drops = resVal.drop;
									ret.scrambles = resVal.scramble;
									ret.epgStartTime = r.epgStartTime;
									ret.epgEventName = r.epgEventName;
								}
							}
							ctrlCmd.SendViewDeleteCtrl(r.ctrlID[i]);
							isMainCtrl = false;
						}
					}
					//録画終了に伴ってGUIキープが解除されたかもしれない
					this->tunerResetLock = true;
				}else{
					//録画ファイルパス取得
					for( int i = 0; i < 2; i++ ){
						if( r.recMode != RECMODE_VIEW && r.ctrlID[i] != 0 && r.recFilePath[i].empty() ){
							ctrlCmd.SendViewGetRecFilePath(r.ctrlID[i], &r.recFilePath[i]);
						}
					}
					//番組情報確認
					if( r.savedPgInfo == false && r.recMode != RECMODE_VIEW ){
						GET_EPG_PF_INFO_PARAM val;
						val.ONID = r.onid;
						val.TSID = r.tsid;
						val.SID = r.sid;
						val.pfNextFlag = FALSE;
						EPGDB_EVENT_INFO resVal;
						if( ctrlCmd.SendViewGetEventPF(val, &resVal) == CMD_SUCCESS &&
						    resVal.StartTimeFlag && resVal.DurationFlag &&
						    ConvertI64Time(resVal.start_time) <= r.startTime + 30 * I64_1SEC &&
						    r.startTime + 30 * I64_1SEC < ConvertI64Time(resVal.start_time) + resVal.durationSec * I64_1SEC &&
						    (r.eid == 0xFFFF || r.eid == resVal.event_id) ){
							//開始時間から30秒は過ぎているのでこの番組情報が録画中のもののはず
							r.savedPgInfo = true;
							r.epgStartTime = resVal.start_time;
							r.epgEventName = resVal.hasShortInfo ? resVal.shortInfo.event_name : L"";
							//ごく稀にAPR(改行)を含むため
							Replace(r.epgEventName, L"\r\n", L"");
							if( this->saveProgramInfo ){
								for( int i = 0; i < 2; i++ ){
									if( r.recFilePath[i].empty() == false ){
										SaveProgramInfo(r.recFilePath[i].c_str(), resVal, r.appendPgInfo);
									}
								}
							}
						}
					}
					//まだ録画中の予約があるのでGUIキープを再設定してはいけない
					ngResetLock = true;
				}
			}
			break;
		}
		if( ret.type != 0 ){
			ret.reserveID = itrRes->first;
			retList.push_back(ret);
			this->reserveMap.erase(itrRes++);
		}else{
			itrRes++;
		}
	}

	//TR_IDLE->TR_READYの遷移を待つ予約を開始順に並べる
	std::sort(idleList.begin(), idleList.end());

	//TR_IDLE->TR_READY(TR_REC)の遷移をする
	for( vector<pair<LONGLONG, DWORD>>::const_iterator itrIdle = idleList.begin(); itrIdle != idleList.end(); itrIdle++ ){
		auto itrRes = this->reserveMap.find(itrIdle->second);
		TUNER_RESERVE_WORK& r = itrRes->second;
		CHECK_RESULT ret;
		ret.type = 0;
		if( this->tunerPid == 0 ){
			//チューナを起動する
			SET_CH_INFO initCh;
			initCh.ONID = r.onid;
			initCh.TSID = r.tsid;
			initCh.SID = r.sid;
			initCh.useSID = TRUE;
			initCh.useBonCh = FALSE;
			bool nwUdpTcp = this->recNW || r.recMode == RECMODE_VIEW;
			if( OpenTuner(this->recMinWake, (this->openViewForViewing == false && this->openViewAlways == false) || r.recMode != RECMODE_VIEW,
			              nwUdpTcp, nwUdpTcp, true, &initCh) ){
				this->tunerONID = r.onid;
				this->tunerTSID = r.tsid;
				this->tunerChLocked = true;
				this->tunerResetLock = false;
				this->tunerChChgTick = GetU32Tick();
				this->notifyManager.AddNotifyMsg(NOTIFY_UPDATE_PRE_REC_START, this->bonFileName);
				ctrlCmd.SetPipeSetting(CMD2_VIEW_CTRL_PIPE, this->tunerPid);
				r.retryOpenCount = 0;
			}else if( ++r.retryOpenCount >= 4 || r.retryOpenCount == 2 && CloseOtherTuner() == false ){
				//試行2回→他チューナ終了成功時さらに2回→起動できなかった
				ret.type = CHECK_ERR_OPEN;
			}
		}else{
			r.retryOpenCount = 0;
		}
		if( this->tunerPid && (r.startTime - r.startMargin) / I64_1SEC - READY_MARGIN < now / I64_1SEC ){
			//録画開始READY_MARGIN秒前～
			//原作では録画制御作成は通常録画時60秒前、割り込み録画時15秒前だが
			//作成を前倒しする必要は特にないのと、チャンネル変更からEIT[p/f]取得までの時間を確保できるようこの秒数にした
			if( this->specialState == TR_EPGCAP ){
				//EPG取得をキャンセル(遷移中断)
				AddDebugLog(L"epg cancel");
				//CSendCtrlCmd::SendViewEpgCapStop()は送らない(即座にチューナ閉じるので意味がないため)
				CloseTuner();
				this->specialState = TR_IDLE;
				break;
			}else if( this->specialState == TR_NWTV ){
				//ネットワークモードを解除
				wstring bonDriver;
				DWORD status;
				if( ctrlCmd.SendViewGetBonDrivere(&bonDriver) == CMD_SUCCESS &&
				    UtilComparePath(bonDriver.c_str(), this->bonFileName.c_str()) == 0 &&
				    ctrlCmd.SendViewGetStatus(&status) == CMD_SUCCESS && (status == VIEW_APP_ST_NORMAL || status == VIEW_APP_ST_ERR_CH_CHG) ){
					//プロセスを引き継ぐ
					this->tunerONID = r.onid;
					this->tunerTSID = r.tsid;
					this->tunerChLocked = false;
					this->tunerResetLock = false;
					this->specialState = TR_IDLE;
				}else{
					//ネットワークモード終了(遷移中断)
					CloseNWTV();
					break;
				}
			}
			if( this->tunerONID != r.onid || this->tunerTSID != r.tsid ){
				//チャンネル違うので、TR_IDLEでない全予約の優先度を比べる
				auto itr = this->reserveMap.cbegin();
				for( ; itr != this->reserveMap.end(); itr++ ){
					if( itr->second.state != TR_IDLE && itr->second.effectivePriority < r.effectivePriority ){
						break;
					}
				}
				if( itr == this->reserveMap.end() ){
					//TR_IDLEでない全予約は自分よりも弱いので葬る
					for( itr = this->reserveMap.begin(); itr != this->reserveMap.end(); ){
						if( itr->second.state != TR_IDLE ){
							CHECK_RESULT retOther;
							retOther.type = CHECK_ERR_REC;
							retOther.reserveID = itr->first;
							retOther.continueRec = false;
							retOther.drops = 0;
							retOther.scrambles = 0;
							bool isMainCtrl = true;
							for( int i = 0; i < 2; i++ ){
								if( itr->second.ctrlID[i] != 0 ){
									if( itr->second.state == TR_REC ){
										if( isMainCtrl ){
											retOther.type = CHECK_END_NEXT_START_END;
										}
										if( itr->second.recMode != RECMODE_VIEW ){
											SET_CTRL_REC_STOP_PARAM param;
											param.ctrlID = itr->second.ctrlID[i];
											param.saveErrLog = this->saveErrLog;
											SET_CTRL_REC_STOP_RES_PARAM resVal;
											if( ctrlCmd.SendViewStopRec(param, &resVal) != CMD_SUCCESS ){
												if( isMainCtrl ){
													retOther.type = CHECK_ERR_RECEND;
												}
											}else if( isMainCtrl ){
												retOther.recFilePath = resVal.recFilePath;
												retOther.drops = resVal.drop;
												retOther.scrambles = resVal.scramble;
											}
										}
									}
									ctrlCmd.SendViewDeleteCtrl(itr->second.ctrlID[i]);
									isMainCtrl = false;
								}
							}
							retList.push_back(retOther);
							this->reserveMap.erase(itr++);
						}else{
							itr++;
						}
					}
					this->tunerONID = r.onid;
					this->tunerTSID = r.tsid;
					this->tunerChLocked = false;
					this->tunerResetLock = false;
				}
			}
			if( this->tunerONID == r.onid && this->tunerTSID == r.tsid ){
				if( this->tunerChLocked == false ){
					//チャンネル変更
					SET_CH_INFO chgCh;
					chgCh.ONID = r.onid;
					chgCh.TSID = r.tsid;
					chgCh.SID = r.sid;
					chgCh.useSID = TRUE;
					chgCh.useBonCh = FALSE;
					//「予約録画待機中」
					ctrlCmd.SendViewSetStandbyRec(1);
					if( ctrlCmd.SendViewSetCh(chgCh) == CMD_SUCCESS ){
						this->tunerChLocked = true;
						this->tunerResetLock = false;
						this->tunerChChgTick = GetU32Tick();
					}
				}
				if( this->tunerChLocked ){
					//同一チャンネルなので録画制御を作成できる
					bool continueRec = false;
					for( auto itr = this->reserveMap.cbegin(); itr != this->reserveMap.end(); itr++ ){
						if( itr->second.continueRecFlag &&
						    itr->second.state == TR_REC &&
						    itr->second.sid == r.sid &&
						    itr->second.recMode != RECMODE_VIEW &&
						    itr->second.recMode == r.recMode &&
						    itr->second.enableCaption == r.enableCaption &&
						    itr->second.enableData == r.enableData &&
						    itr->second.partialRecMode == r.partialRecMode ){
							//連続録画なので、同一制御IDで録画開始されたことにする。TR_RECまで遷移するので注意
							r.state = TR_REC;
							r.ctrlID[0] = itr->second.ctrlID[0];
							r.ctrlID[1] = itr->second.ctrlID[1];
							r.notStartHead = r.startTime - r.startMargin + 60 * I64_1SEC < now;
							r.appendPgInfo = itr->second.appendPgInfo || itr->second.savedPgInfo;
							r.savedPgInfo = false;
							//引継ぎ元を葬る
							CHECK_RESULT retOther;
							retOther.type = CHECK_ERR_REC;
							retOther.reserveID = itr->first;
							retOther.continueRec = true;
							retOther.drops = 0;
							retOther.scrambles = 0;
							for( int i = 0; i < 2; i++ ){
								if( itr->second.ctrlID[i] != 0 ){
									if( itr->second.recFilePath[i].empty() == false ){
										retOther.recFilePath = itr->second.recFilePath[i];
										retOther.type = itr->second.notStartHead ? CHECK_END_NOT_START_HEAD :
										                itr->second.savedPgInfo == false ? CHECK_END_NOT_FIND_PF : CHECK_END;
										retOther.epgStartTime = itr->second.epgStartTime;
										retOther.epgEventName = itr->second.epgEventName;
									}
									break;
								}
							}
							retList.push_back(retOther);
							this->reserveMap.erase(itr);
							continueRec = true;
							break;
						}
					}
					if( continueRec == false ){
						if( CreateCtrl(&r.ctrlID[0], &r.ctrlID[1], r) ){
							r.state = TR_READY;
						}else{
							//作成できなかった
							ret.type = CHECK_ERR_CTRL;
						}
					}
				}
			}
		}
		if( ret.type != 0 ){
			ret.reserveID = itrRes->first;
			retList.push_back(ret);
			this->reserveMap.erase(itrRes);
		}
	}

	if( IsNeedOpenTuner() == false ){
		//チューナが必要なくなった
		CloseTuner();
	}
	if( this->tunerPid && this->specialState == TR_IDLE && this->tunerResetLock ){
		if( ngResetLock == false ){
			//「予約録画待機中」
			ctrlCmd.SendViewSetStandbyRec(1);
		}
		this->tunerResetLock = false;
	}
	return retList;
}

bool CTunerBankCtrl::IsNeedOpenTuner() const
{
	if( this->specialState != TR_IDLE ){
		return true;
	}
	//戻り値の振動を防ぐためdelayTimeを考慮してはいけない
	LONGLONG now = GetNowI64Time();
	for( auto itr = this->reserveMap.cbegin(); itr != this->reserveMap.end(); itr++ ){
		if( itr->second.state != TR_IDLE || (itr->second.startTime - itr->second.startMargin - this->recWakeTime) / I64_1SEC < now / I64_1SEC ){
			return true;
		}
	}
	return false;
}

bool CTunerBankCtrl::FindPartialService(WORD onid, WORD tsid, WORD sid, WORD* partialSID, wstring* serviceName) const
{
	auto itrEnd = this->chMap.upper_bound(Create64Key(onid, tsid, 0xFFFF));
	for( auto itr = this->chMap.lower_bound(Create64Key(onid, tsid, 0)); itr != itrEnd; itr++ ){
		if( itr->second.partialFlag ){
			if( itr->second.serviceID != sid ){
				if( partialSID != NULL ){
					*partialSID = itr->second.serviceID;
				}
				if( serviceName != NULL ){
					*serviceName = itr->second.serviceName;
				}
				return true;
			}
		}
	}
	return false;
}

bool CTunerBankCtrl::CreateCtrl(DWORD* ctrlID, DWORD* partialCtrlID, const TUNER_RESERVE& reserve) const
{
	if( this->tunerPid == 0 ){
		return false;
	}
	BYTE partialRecMode = reserve.recMode == RECMODE_VIEW ? 0 : min<BYTE>(reserve.partialRecMode, 2);
	WORD partialSID = 0;
	if( partialRecMode == 1 || partialRecMode == 2 ){
		if( FindPartialService(reserve.onid, reserve.tsid, reserve.sid, &partialSID, NULL) == false ){
			partialSID = 0;
			if( reserve.partialRecMode == 2 ){
				return false;
			}
		}
	}
	CSendCtrlCmd ctrlCmd;
	ctrlCmd.SetPipeSetting(CMD2_VIEW_CTRL_PIPE, this->tunerPid);
	DWORD newID;
	if( ctrlCmd.SendViewCreateCtrl(&newID) != CMD_SUCCESS ){
		return false;
	}

	if( partialRecMode == 2 ){
		//部分受信のみ
		*ctrlID = 0;
		*partialCtrlID = newID;
	}else{
		*ctrlID = newID;
		*partialCtrlID = 0;
		if( partialRecMode == 1 && partialSID != 0 && ctrlCmd.SendViewCreateCtrl(partialCtrlID) != CMD_SUCCESS ){
			*partialCtrlID = 0;
		}
	}
	SET_CTRL_MODE param;
	if( *ctrlID != 0 ){
		//通常
		param.ctrlID = *ctrlID;
		param.SID = reserve.recMode == RECMODE_ALL || reserve.recMode == RECMODE_ALL_NOB25 ? 0xFFFF : reserve.sid;
		param.enableScramble = reserve.recMode != RECMODE_ALL_NOB25 && reserve.recMode != RECMODE_SERVICE_NOB25;
		param.enableCaption = reserve.enableCaption;
		param.enableData = reserve.enableData;
		ctrlCmd.SendViewSetCtrlMode(param);
	}
	if( *partialCtrlID != 0 ){
		//部分受信
		param.ctrlID = *partialCtrlID;
		param.SID = partialSID;
		param.enableScramble = reserve.recMode != RECMODE_ALL_NOB25 && reserve.recMode != RECMODE_SERVICE_NOB25;
		param.enableCaption = reserve.enableCaption;
		param.enableData = reserve.enableData;
		ctrlCmd.SendViewSetCtrlMode(param);
	}
	SYSTEMTIME st;
	ConvertSystemTime(reserve.startTime, &st);
	wstring msg;
	Format(msg, L"%ls %04d/%02d/%02d %02d:%02d:%02d～ %ls", reserve.stationName.c_str(),
	       st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, reserve.title.c_str());
	this->notifyManager.AddNotifyMsg(NOTIFY_UPDATE_PRE_REC_START, msg);
	return true;
}

void CTunerBankCtrl::SaveProgramInfo(LPCWSTR recPath, const EPGDB_EVENT_INFO& info, bool append) const
{
	fs_path savePath = GetPrivateProfileToString(L"SET", L"RecInfoFolder", L"", GetCommonIniPath().c_str());

	if( savePath.empty() ){
		savePath = fs_path(recPath).concat(L".program.txt");
	}else{
		savePath.append(fs_path(recPath).filename().concat(L".program.txt").native());
	}

	wstring serviceName;
	auto itr = this->chMap.find(Create64Key(info.original_network_id, info.transport_stream_id, info.service_id));
	if( itr != this->chMap.end() ){
		serviceName = itr->second.serviceName;
	}
	wstring outTextW = (append ? L"\r\n-----------------------\r\n" : this->saveProgramInfoAsUtf8 ? L"\xFEFF" : L"") +
	                   ConvertProgramText(info, serviceName);
	if( UTIL_NEWLINE[0] != L'\r' ){
		Replace(outTextW, L"\r\n", L"\n");
	}
	string outText;
	WtoA(outTextW, outText, this->saveProgramInfoAsUtf8 ? UTIL_CONV_UTF8 : UTIL_CONV_DEFAULT);

	//※原作と異なりディレクトリの自動生成はしない
	std::unique_ptr<FILE, fclose_deleter> fp(UtilOpenFile(savePath, append ? UTIL_O_CREAT_APPEND : UTIL_SECURE_WRITE));
	if( fp ){
		fputs(outText.c_str(), fp.get());
	}
}

bool CTunerBankCtrl::RecStart(const TUNER_RESERVE_WORK& reserve, LONGLONG now) const
{
	if( this->tunerPid == 0 ){
		return false;
	}
	if( reserve.recMode == RECMODE_VIEW ){
		return true;
	}
	CReNamePlugInUtil utilCache;
	CSendCtrlCmd ctrlCmd;
	ctrlCmd.SetPipeSetting(CMD2_VIEW_CTRL_PIPE, this->tunerPid);
	bool isMainCtrl = true;
	for( int i = 0; i < 2; i++ ){
		if( reserve.ctrlID[i] != 0 ){
			SET_CTRL_REC_PARAM param;
			param.ctrlID = reserve.ctrlID[i];
			//saveFolder[].recFileNameが空でない限りこのフィールドが利用されることはない
			param.fileName = L"padding.ts";
			//同時出力用ファイル名
			param.saveFolder = i == 0 ? reserve.recFolder : reserve.partialRecFolder;
			if( param.saveFolder.empty() ){
				param.saveFolder.resize(1);
				param.saveFolder[0].recFolder = L"!Default";
				//原作はここでwritePlugInフィールドに"RecWritePlugIn0"設定キーを代入していた。説明はなく利点も思いつかないので省いた
			}
			for( size_t j = 0; j < param.saveFolder.size(); j++ ){
				if( CompareNoCase(param.saveFolder[j].recFolder, L"!Default") == 0 ){
					//注意: この置換は原作にはない
					param.saveFolder[j].recFolder = GetRecFolderPath().native();
				}
				if( param.saveFolder[j].recNamePlugIn.empty() ){
					param.saveFolder[j].recNamePlugIn = this->recNamePlugInFileName;
				}
				//recNamePlugInを展開して実ファイル名をセット
				WORD sid = reserve.sid;
				WORD eid = reserve.eid;
				wstring stationName = reserve.stationName;
				if( i != 0 ){
					FindPartialService(reserve.onid, reserve.tsid, reserve.sid, &sid, &stationName);
					eid = 0xFFFF;
				}
				SYSTEMTIME st;
				ConvertSystemTime(reserve.startTime, &st);
				SYSTEMTIME stDefault;
				ConvertSystemTime(max(reserve.startTime, now), &stDefault);
				param.saveFolder[j].recFileName = ConvertRecName(
					param.saveFolder[j].recNamePlugIn.c_str(), st, reserve.durationSecond, reserve.title.c_str(), reserve.onid, reserve.tsid, sid, eid,
					stationName.c_str(), this->bonFileName.c_str(), this->tunerID, reserve.reserveID, this->epgDBManager,
					stDefault, param.ctrlID, this->tsExt.c_str(), this->recNameNoChkYen, utilCache);
				param.saveFolder[j].recNamePlugIn.clear();
			}
			param.overWriteFlag = this->recOverWrite;
			param.pittariFlag = reserve.eid != 0xFFFF && reserve.pittari;
			param.pittariONID = reserve.onid;
			param.pittariTSID = reserve.tsid;
			param.pittariSID = reserve.sid;
			param.pittariEventID = reserve.eid;
			DWORD bitrate = 0;
			if( this->keepDisk ){
				bitrate = GetBitrateFromIni(reserve.onid, reserve.tsid, reserve.sid);
			}
			param.createSize = (ULONGLONG)(bitrate / 8) * 1000 *
				max(reserve.durationSecond + (reserve.startTime + reserve.endMargin - now) / I64_1SEC, 0LL);
			if( ctrlCmd.SendViewStartRec(param) != CMD_SUCCESS && isMainCtrl ){
				break;
			}
			isMainCtrl = false;
		}
	}
	if( isMainCtrl == false ){
		SYSTEMTIME st;
		ConvertSystemTime(reserve.startTime, &st);
		wstring msg;
		Format(msg, L"%ls %04d/%02d/%02d %02d:%02d:%02d\r\n%ls", reserve.stationName.c_str(),
		       st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, reserve.title.c_str());
		this->notifyManager.AddNotifyMsg(NOTIFY_UPDATE_REC_START, msg);
		return true;
	}
	return false;
}

CTunerBankCtrl::TR_STATE CTunerBankCtrl::GetState() const
{
	TR_STATE state = TR_IDLE;
	if( this->tunerPid ){
		state = TR_OPEN;
		if( this->specialState != TR_IDLE ){
			state = this->specialState;
		}else{
			for( auto itr = this->reserveMap.cbegin(); itr != this->reserveMap.end(); itr++ ){
				if( itr->second.state == TR_REC ){
					state = TR_REC;
					break;
				}else if( itr->second.state == TR_READY ){
					state = TR_READY;
				}
			}
		}
	}
	return state;
}

TUNER_PROCESS_STATUS_INFO CTunerBankCtrl::GetProcessStatusInfo() const
{
	TUNER_PROCESS_STATUS_INFO info;
	info.tunerID = this->tunerID;
	info.processID = this->tunerPid;
	info.drop = this->statusInfo.drop;
	info.scramble = this->statusInfo.scramble;
	info.signalLv = this->statusInfo.signalLv;
	info.space = this->statusInfo.space;
	info.ch = this->statusInfo.ch;
	info.originalNetworkID = this->statusInfo.originalNetworkID;
	info.transportStreamID = this->statusInfo.transportStreamID;
	TR_STATE state = GetState();
	info.recFlag = state == TR_REC;
	info.epgCapFlag = state == TR_EPGCAP;
	info.extraFlags = 0;
	return info;
}

LONGLONG CTunerBankCtrl::GetNearestReserveTime() const
{
	LONGLONG minTime = LLONG_MAX;
	for( auto itr = this->reserveMap.cbegin(); itr != this->reserveMap.end(); itr++ ){
		minTime = min(itr->second.startTime - itr->second.startMargin, minTime);
	}
	return minTime;
}

bool CTunerBankCtrl::StartEpgCap(const vector<SET_CH_INFO>& setChList)
{
	if( setChList.empty() == false && this->tunerPid == 0 ){
		//EPG取得についてはチューナの再利用はしない
		if( OpenTuner(this->recMinWake, true, false, false, true, NULL) ){
			CWatchBlock watchBlock(&this->watchContext);
			CSendCtrlCmd ctrlCmd;
			ctrlCmd.SetPipeSetting(CMD2_VIEW_CTRL_PIPE, this->tunerPid);
			//EPG取得開始
			if( ctrlCmd.SendViewEpgCapStart(setChList) == CMD_SUCCESS ){
				this->specialState = TR_EPGCAP;
				return true;
			}
			CloseTuner();
		}
	}
	return false;
}

bool CTunerBankCtrl::GetCurrentChID(WORD* onid, WORD* tsid) const
{
	if( this->tunerPid && this->specialState == TR_IDLE ){
		*onid = this->tunerONID;
		*tsid = this->tunerTSID;
		return true;
	}
	return false;
}

bool CTunerBankCtrl::SearchEpgInfo(WORD sid, WORD eid, EPGDB_EVENT_INFO* resVal) const
{
	if( this->tunerPid && this->specialState == TR_IDLE ){
		CWatchBlock watchBlock(&this->watchContext);
		CSendCtrlCmd ctrlCmd;
		ctrlCmd.SetPipeSetting(CMD2_VIEW_CTRL_PIPE, this->tunerPid);
		SEARCH_EPG_INFO_PARAM val;
		val.ONID = this->tunerONID;
		val.TSID = this->tunerTSID;
		val.SID = sid;
		val.eventID = eid;
		val.pfOnlyFlag = 0;
		if( ctrlCmd.SendViewSearchEvent(val, resVal) == CMD_SUCCESS ){
			if( resVal->hasShortInfo ){
				//ごく稀にAPR(改行)を含むため
				Replace(resVal->shortInfo.event_name, L"\r\n", L"");
			}
			return true;
		}
	}
	return false;
}

int CTunerBankCtrl::GetEventPF(WORD sid, bool pfNextFlag, EPGDB_EVENT_INFO* resVal) const
{
	//チャンネル変更を要求してから最初のEIT[p/f]が届く妥当な時間だけ待つ
	//TODO: 視聴予約中(=GUIキープされていないとき)にチャンネル変更されると最新の情報でなくなる可能性がある。現仕様では解決策なし
	if( this->tunerPid && this->specialState == TR_IDLE && (this->tunerChLocked == false || GetU32Tick() - this->tunerChChgTick > 8000) ){
		CWatchBlock watchBlock(&this->watchContext);
		CSendCtrlCmd ctrlCmd;
		ctrlCmd.SetPipeSetting(CMD2_VIEW_CTRL_PIPE, this->tunerPid);
		GET_EPG_PF_INFO_PARAM val;
		val.ONID = this->tunerONID;
		val.TSID = this->tunerTSID;
		val.SID = sid;
		val.pfNextFlag = pfNextFlag;
		DWORD ret = ctrlCmd.SendViewGetEventPF(val, resVal);
		if( ret == CMD_SUCCESS ){
			if( resVal->hasShortInfo ){
				//ごく稀にAPR(改行)を含むため
				Replace(resVal->shortInfo.event_name, L"\r\n", L"");
			}
			return 0;
		}else if( ret == CMD_ERR && (this->tunerChLocked == false || GetU32Tick() - this->tunerChChgTick > 15000) ){
			return 1;
		}
		//最初のTOTが届くまでは、あるのに消える可能性がある
	}
	return 2;
}

LONGLONG CTunerBankCtrl::DelayTime() const
{
	return this->specialState == TR_EPGCAP ? this->epgCapDelayTime : this->delayTime;
}

bool CTunerBankCtrl::OpenNWTV(int id, bool nwUdp, bool nwTcp, const SET_CH_INFO& chInfo)
{
	if( this->specialState == TR_EPGCAP ){
		AddDebugLog(L"epg cancel");
		//CSendCtrlCmd::SendViewEpgCapStop()は送らない(即座にチューナ閉じるので意味がないため)
		CloseTuner();
		this->specialState = TR_IDLE;
	}
	if( this->tunerPid == 0 ){
		if( OpenTuner(true, true, nwUdp, nwTcp, false, &chInfo) ){
			this->specialState = TR_NWTV;
			this->nwtvID = id;
			return true;
		}
	}else if( this->specialState == TR_NWTV ){
		this->nwtvID = id;
		CWatchBlock watchBlock(&this->watchContext);
		CSendCtrlCmd ctrlCmd;
		ctrlCmd.SetPipeSetting(CMD2_VIEW_CTRL_PIPE, this->tunerPid);
		ctrlCmd.SendViewSetCh(chInfo);
		return true;
	}
	return false;
}

void CTunerBankCtrl::CloseNWTV()
{
	if( this->tunerPid && this->specialState == TR_NWTV ){
		CloseTuner();
		this->specialState = TR_IDLE;
	}
}

bool CTunerBankCtrl::GetRecFilePath(DWORD reserveID, wstring& filePath) const
{
	auto itr = this->reserveMap.find(reserveID);
	if( itr != this->reserveMap.end() && itr->second.state == TR_REC ){
		int i = itr->second.ctrlID[0] != 0 ? 0 : 1;
		if( itr->second.recFilePath[i].empty() == false ){
			filePath = itr->second.recFilePath[i];
			return true;
		}
	}
	return false;
}

bool CTunerBankCtrl::OpenTuner(bool minWake, bool noView, bool nwUdp, bool nwTcp, bool standbyRec, const SET_CH_INFO* initCh)
{
	if( this->tunerPid ){
		return false;
	}
	fs_path commonIniPath = GetCommonIniPath();

	wstring strExecute = GetPrivateProfileToString(L"SET", L"RecExePath", L"", commonIniPath.c_str());
	if( strExecute.empty() ){
		strExecute = GetModulePath().replace_filename(L"EpgDataCap_Bon"
#ifdef _WIN32
			L".exe"
#endif
			).native();
	}

#ifdef _WIN32
	//セクション単位で処理するほうが軽い
	vector<WCHAR> buffOpt = GetPrivateProfileSectionBuffer(L"APP_CMD_OPT", fs_path(commonIniPath).replace_filename(L"ViewApp.ini").c_str());

	wstring strParam = L" " + GetBufferedProfileToString(buffOpt.data(), L"Bon", L"-d") + L" " + this->bonFileName;

	if( minWake ){
		strParam += L" " + GetBufferedProfileToString(buffOpt.data(), L"Min", L"-min");
	}
	if( noView ){
		strParam += L" " + GetBufferedProfileToString(buffOpt.data(), L"ViewOff", L"-noview");
	}
	if( nwUdp == false && nwTcp == false ){
		strParam += L" " + GetBufferedProfileToString(buffOpt.data(), L"NetworkOff", L"-nonw");
	}else{
		strParam += nwUdp ? L" -nwudp" : L"";
		strParam += nwTcp ? L" -nwtcp" : L"";
	}
#else
	string strParam;
	WtoUTF8(this->bonFileName, strParam);
	//パラメーターは固定。NUL文字区切りなので注意
	strParam.insert(0, "-d", 3);
	size_t replaceSpaceToNulPos = strParam.size();
	if( minWake ){
		strParam += " -min";
	}
	if( noView ){
		strParam += " -noview";
	}
	if( nwUdp == false && nwTcp == false ){
		strParam += " -nonw";
	}else{
		strParam += nwUdp ? " -nwudp" : "";
		strParam += nwTcp ? " -nwtcp" : "";
	}
#endif

	if( initCh && initCh->useSID ){
		//チャンネルの初期値を指定しておく(あくまで補助的なもの)
#ifdef _WIN32
		wstring optONID = GetBufferedProfileToString(buffOpt.data(), L"ONID", L"-nid");
		wstring optTSID = GetBufferedProfileToString(buffOpt.data(), L"TSID", L"-tsid");
		wstring optSID = GetBufferedProfileToString(buffOpt.data(), L"SID", L"-sid");
		if( optONID.empty() == false && optTSID.empty() == false && optSID.empty() == false ){
			wstring ch;
			Format(ch, L" %ls %d %ls %d %ls %d", optONID.c_str(), initCh->ONID, optTSID.c_str(), initCh->TSID, optSID.c_str(), initCh->SID);
			strParam += ch;
		}
#else
		char ch[64];
		sprintf_s(ch, " -nid %d -tsid %d -sid %d", initCh->ONID, initCh->TSID, initCh->SID);
		strParam += ch;
#endif
	}

	//原作と異なりイベントオブジェクト"Global\\EpgTimerSrv_OpenTuner_Event"による排他制御はしない
	//また、サービスモードでない限りGUI経由でなく直接CreateProcess()するので注意
#ifdef _WIN32
	DWORD dwPriority = this->processPriority == 0 ? REALTIME_PRIORITY_CLASS :
	                   this->processPriority == 1 ? HIGH_PRIORITY_CLASS :
	                   this->processPriority == 2 ? ABOVE_NORMAL_PRIORITY_CLASS :
	                   this->processPriority == 3 ? NORMAL_PRIORITY_CLASS :
	                   this->processPriority == 4 ? BELOW_NORMAL_PRIORITY_CLASS : IDLE_PRIORITY_CLASS;
	if( this->notifyManager.IsGUI() == false ){
		//表示できないのでGUI経由で起動してみる
		CSendCtrlCmd ctrlCmd;
		vector<DWORD> registGUI = this->notifyManager.GetRegistGUI();
		for( size_t i = 0; i < registGUI.size(); i++ ){
			ctrlCmd.SetPipeSetting(CMD2_GUI_CTRL_PIPE, registGUI[i]);
			DWORD pid;
			if( ctrlCmd.SendGUIExecute(L'"' + strExecute + L'"' + strParam, &pid) == CMD_SUCCESS && pid ){
				//ハンドル開く前に終了するかもしれない
				this->hTunerProcess = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE | PROCESS_SET_INFORMATION, FALSE, pid);
				if( this->hTunerProcess ){
					this->tunerPid = pid;
					SetPriorityClass(this->hTunerProcess, dwPriority);
					break;
				}
			}
		}
	}
	if( this->tunerPid == 0 ){
		PROCESS_INFORMATION pi;
		STARTUPINFO si = {};
		si.cb = sizeof(si);
		vector<WCHAR> strBuff(strParam.c_str(), strParam.c_str() + strParam.size() + 1);
		if( CreateProcess(strExecute.c_str(), strBuff.data(), NULL, NULL, FALSE, dwPriority, NULL, NULL, &si, &pi) ){
			CloseHandle(pi.hThread);
			this->hTunerProcess = pi.hProcess;
			this->tunerPid = pi.dwProcessId;
		}
	}
#else
	string execU;
	WtoUTF8(strExecute, execU);
	vector<char*> argv;
	argv.push_back((char*)execU.c_str());
	std::replace(strParam.begin() + replaceSpaceToNulPos, strParam.end(), ' ', '\0');
	//NUL文字で分離
	for( size_t i = 0; i < strParam.size(); i += strlen(strParam.c_str() + i) + 1 ){
		argv.push_back((char*)(strParam.c_str() + i));
	}
	argv.push_back(NULL);
	pid_t pid = fork();
	if( pid == 0 ){
		//シグナルマスクを初期化
		sigset_t sset;
		sigemptyset(&sset);
		if( sigprocmask(SIG_SETMASK, &sset, NULL) == 0 ){
			execv(execU.c_str(), argv.data());
		}
		exit(EXIT_FAILURE);
	}else if( pid != -1 ){
		this->tunerPid = (DWORD)pid;
	}
#endif
	if( this->tunerPid ){
		//IDのセット
		CWatchBlock watchBlock(&this->watchContext);
		CSendCtrlCmd ctrlCmd;
		ctrlCmd.SetPipeSetting(CMD2_VIEW_CTRL_PIPE, this->tunerPid);
		ctrlCmd.SetConnectTimeOut(0);
		//起動完了まで最大30秒ほど待つ
		for( DWORD tick = GetU32Tick(); GetU32Tick() - tick < 30000; ){
#ifdef _WIN32
			if( WaitForSingleObject(this->hTunerProcess, 10) != WAIT_TIMEOUT ){
#else
			SleepForMsec(10);
			if( waitpid(this->tunerPid, NULL, WNOHANG) != 0 ){
#endif
				CloseTuner();
				return false;
			}else if( ctrlCmd.SendViewSetID(this->tunerID) == CMD_SUCCESS ){
				ctrlCmd.SetConnectTimeOut(CONNECT_TIMEOUT);
				//起動ステータスを確認
				DWORD status;
				if( ctrlCmd.SendViewGetStatus(&status) == CMD_SUCCESS && status == VIEW_APP_ST_ERR_BON ){
					CloseTuner();
					return false;
				}
				if( standbyRec ){
					//「予約録画待機中」
					ctrlCmd.SendViewSetStandbyRec(1);
				}
				if( initCh ){
					ctrlCmd.SendViewSetCh(*initCh);
					if( this->openViewAlways ){
						ctrlCmd.SendViewExecViewApp();
					}
				}
				VIEW_APP_STATUS_INFO info = {};
				info.space = -1;
				info.ch = -1;
				info.originalNetworkID = -1;
				info.transportStreamID = -1;
				info.appID = -1;
				this->statusInfo = info;
				this->getStatusDetailsNonSupport = false;
				return true;
			}
		}
#ifdef _WIN32
		TerminateProcess(this->hTunerProcess, 0xFFFFFFFF);
		CloseHandle(this->hTunerProcess);
#else
		kill(this->tunerPid, 9);
		//ゾンビの回収に1秒だけ使う
		for( int timeout = 1000; timeout > 0 && waitpid(this->tunerPid, NULL, WNOHANG) == 0; timeout -= 10 ){
			SleepForMsec(10);
		}
#endif
		this->tunerPid = 0;
		AddDebugLogFormat(L"CTunerBankCtrl::%ls: Terminated TunerID=0x%08x", L"OpenTuner()", this->tunerID);
	}
	return false;
}

void CTunerBankCtrl::CloseTuner()
{
	if( this->tunerPid ){
#ifdef _WIN32
		if( WaitForSingleObject(this->hTunerProcess, 0) == WAIT_TIMEOUT ){
#else
		if( waitpid(this->tunerPid, NULL, WNOHANG) == 0 ){
#endif
			CWatchBlock watchBlock(&this->watchContext);
			CSendCtrlCmd ctrlCmd;
			ctrlCmd.SetPipeSetting(CMD2_VIEW_CTRL_PIPE, this->tunerPid);
			ctrlCmd.SendViewAppClose();
#ifdef _WIN32
			if( WaitForSingleObject(this->hTunerProcess, 30000) == WAIT_TIMEOUT ){
				//ぶち殺す
				TerminateProcess(this->hTunerProcess, 0xFFFFFFFF);
#else
			bool killed = false;
			for( int timeout = 30000; timeout > 0 && waitpid(this->tunerPid, NULL, WNOHANG) == 0; timeout -= 10 ){
				if( killed == false && timeout < 1000 ){
					//ぶち殺す。ゾンビの回収に1秒だけ使う
					kill(this->tunerPid, 9);
					killed = true;
				}
				SleepForMsec(10);
			}
			if( killed ){
#endif
				AddDebugLogFormat(L"CTunerBankCtrl::%ls: Terminated TunerID=0x%08x", L"CloseTuner()", this->tunerID);
			}
		}
		lock_recursive_mutex lock(this->watchContext.lock);
#ifdef _WIN32
		CloseHandle(this->hTunerProcess);
#endif
		this->tunerPid = 0;
	}
}

bool CTunerBankCtrl::CloseOtherTuner()
{
	if( this->tunerPid ){
		return false;
	}
	vector<DWORD> pidList;
#ifdef _WIN32
	//Toolhelpスナップショットを作成する
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if( hSnapshot != INVALID_HANDLE_VALUE ){
		PROCESSENTRY32 procent;
		procent.dwSize = sizeof(PROCESSENTRY32);
		if( Process32First(hSnapshot, &procent) != FALSE ){
			do{
				pidList.push_back(procent.th32ProcessID);
			}while( Process32Next(hSnapshot, &procent) != FALSE );
		}
		CloseHandle(hSnapshot);
	}
#else
	EnumFindFile(fs_path(EDCB_INI_ROOT).append(CMD2_VIEW_CTRL_PIPE).concat(L"*"), [&pidList](UTIL_FIND_DATA& findData) -> bool {
		if( findData.fileName.size() > wcslen(CMD2_VIEW_CTRL_PIPE) ){
			int pid = (int)wcstol(findData.fileName.c_str() + wcslen(CMD2_VIEW_CTRL_PIPE), NULL, 10);
			if( pid > 0 && kill(pid, 0) == 0 ){
				pidList.push_back(pid);
			}
		}
		return true;
	});
#endif
	bool closed = false;

	//起動中で使えるもの探す
	for( size_t i = 0; closed == false && i < pidList.size(); i++ ){
		//原作と異なりイメージ名ではなく接続先パイプの有無で判断するので注意
		CSendCtrlCmd ctrlCmd;
		ctrlCmd.SetPipeSetting(CMD2_VIEW_CTRL_PIPE, pidList[i]);
		if( ctrlCmd.PipeExists() ){
			//万一のフリーズに対処するため一時的にこのバンクの管理下に置く
#ifdef _WIN32
			this->hTunerProcess = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, FALSE, pidList[i]);
			if( this->hTunerProcess )
#endif
			{
				this->tunerPid = pidList[i];
				wstring bonDriver;
				int id;
				DWORD status;
				//録画中のものは奪わないので注意
				if( ctrlCmd.SendViewGetBonDrivere(&bonDriver) == CMD_SUCCESS &&
				    UtilComparePath(bonDriver.c_str(), this->bonFileName.c_str()) == 0 &&
				    ctrlCmd.SendViewGetID(&id) == CMD_SUCCESS && id == -1 &&
				    ctrlCmd.SendViewGetStatus(&status) == CMD_SUCCESS &&
				    (status == VIEW_APP_ST_NORMAL || status == VIEW_APP_ST_GET_EPG || status == VIEW_APP_ST_ERR_CH_CHG) ){
					ctrlCmd.SendViewAppClose();
					//10秒だけ終了を待つ
#ifdef _WIN32
					WaitForSingleObject(this->hTunerProcess, 10000);
#else
					for( int timeout = 10000; timeout > 0 && waitpid(this->tunerPid, NULL, WNOHANG) == 0; timeout -= 10 ){
						SleepForMsec(10);
					}
#endif
					closed = true;
				}
				lock_recursive_mutex lock(this->watchContext.lock);
#ifdef _WIN32
				CloseHandle(this->hTunerProcess);
#endif
				this->tunerPid = 0;
			}
		}
	}

#ifdef _WIN32
	//TVTestで使ってるものあるかチェック
	for( size_t i = 0; closed == false && i < pidList.size(); i++ ){
		CSendCtrlCmd ctrlCmd;
		ctrlCmd.SetPipeSetting(CMD2_TVTEST_CTRL_PIPE, pidList[i]);
		if( ctrlCmd.PipeExists() ){
			this->hTunerProcess = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, FALSE, pidList[i]);
			if( this->hTunerProcess ){
				this->tunerPid = pidList[i];
				wstring bonDriver;
				if( ctrlCmd.SendViewGetBonDrivere(&bonDriver) == CMD_SUCCESS &&
				    UtilComparePath(bonDriver.c_str(), this->bonFileName.c_str()) == 0 ){
					ctrlCmd.SendViewAppClose();
					WaitForSingleObject(this->hTunerProcess, 10000);
					closed = true;
				}
				lock_recursive_mutex lock(this->watchContext.lock);
				CloseHandle(this->hTunerProcess);
				this->tunerPid = 0;
			}
		}
	}
#endif
	return closed;
}

wstring CTunerBankCtrl::ConvertRecName(
	LPCWSTR recNamePlugIn, const SYSTEMTIME& startTime, DWORD durationSec, LPCWSTR eventName, WORD onid, WORD tsid, WORD sid, WORD eid,
	LPCWSTR serviceName, LPCWSTR bonDriverName, DWORD tunerID, DWORD reserveID, CEpgDBManager& epgDBManager_,
	const SYSTEMTIME& startTimeForDefault, DWORD ctrlID, LPCWSTR ext, bool noChkYen, CReNamePlugInUtil& util)
{
	wstring ret;
	if( recNamePlugIn[0] ){
		wstring plugInPath =
#ifdef EDCB_LIB_ROOT
			fs_path(EDCB_LIB_ROOT)
#else
			GetModulePath().replace_filename(L"RecName")
#endif
			.append(L"a").native();
		plugInPath.erase(plugInPath.size() - 1);
		PLUGIN_RESERVE_INFO info;
		info.startTime = startTime;
		info.durationSec = durationSec;
		wcsncpy_s(info.eventName, eventName, _TRUNCATE);
		info.ONID = onid;
		info.TSID = tsid;
		info.SID = sid;
		info.EventID = eid;
		wcsncpy_s(info.serviceName, serviceName, _TRUNCATE);
		wcsncpy_s(info.bonDriverName, bonDriverName, _TRUNCATE);
		info.bonDriverID = tunerID >> 16;
		info.tunerID = tunerID & 0xFFFF;
		EPG_EVENT_INFO epgInfo;
		EPGDB_EVENT_INFO epgDBInfo;
		CEpgEventInfoAdapter epgInfoAdapter;
		info.epgInfo = NULL;
		if( eid != 0xFFFF ){
			if( epgDBManager_.SearchEpg(onid, tsid, sid, eid, &epgDBInfo) ){
				epgInfo = epgInfoAdapter.Create(&epgDBInfo);
				info.epgInfo = &epgInfo;
			}
		}
		info.reserveID = reserveID;
		info.sizeOfStruct = 0;
		WCHAR name[512];
		DWORD size = 512;
		if( util.Convert(&info, recNamePlugIn, plugInPath.c_str(), name, &size) ){
			ret = name;
			CheckFileName(ret, noChkYen);
		}
	}
	if( ret.empty() ){
		const SYSTEMTIME& st = startTimeForDefault;
		Format(ret, L"%04d%02d%02d%02d%02d%02X%02X%02d-"
#ifdef _WIN32
		            //パス全体の制限もあるのでNTFSの制限よりやや小さい長さ
		            L"%.159ls"
#else
		            //84文字制限から日付拡張子等のUTF-8換算長を引いた長さ
		            L"%.74ls"
#endif
		            L"%ls",
		       st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, tunerID >> 16, tunerID & 0xFFFF, ctrlID, eventName, ext);
		CheckFileName(ret);
	}
	return ret;
}

void CTunerBankCtrl::Watch()
{
	//チューナがフリーズするような非常事態ではCSendCtrlCmdのタイムアウトは当てにならない
	//CWatchBlockで囲われた区間を40秒のタイムアウトで監視して、必要なら強制終了する
	lock_recursive_mutex lock(this->watchContext.lock);
	if( this->watchContext.count != 0 && GetU32Tick() - this->watchContext.tick > 40000 ){
		if( this->tunerPid ){
#ifdef _WIN32
			//少なくともhTunerProcessはまだCloseHandle()されていない
			TerminateProcess(this->hTunerProcess, 0xFFFFFFFF);
#else
			kill(this->tunerPid, 9);
#endif
			AddDebugLogFormat(L"CTunerBankCtrl::%ls: Terminated TunerID=0x%08x", L"Watch()", this->tunerID);
		}
	}
}

CTunerBankCtrl::CWatchBlock::CWatchBlock(WATCH_CONTEXT* context_)
	: context(context_)
{
	lock_recursive_mutex lock(this->context->lock);
	if( ++this->context->count == 1 ){
		this->context->tick = GetU32Tick();
	}
}

CTunerBankCtrl::CWatchBlock::~CWatchBlock()
{
	lock_recursive_mutex lock(this->context->lock);
	this->context->count--;
}
