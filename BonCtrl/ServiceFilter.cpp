#include "stdafx.h"
#include "ServiceFilter.h"
#include "BonCtrlDef.h"

CServiceFilter::CServiceFilter(bool createPmt)
	: createPmtFlag(createPmt)
{
	this->transportStreamID = 0;
	this->allServicesFlag = false;
}

void CServiceFilter::SetServiceID(bool allServices, const vector<WORD>& sidList)
{
	this->allServicesFlag = allServices;
	this->serviceIDList = sidList;
	CheckNeedPID();
}

void CServiceFilter::SetPmtCreateMode(bool enableCaption, bool enableData)
{
	if( this->createPmtFlag ){
		this->pmt.SetCreateMode(enableCaption, enableData);
		CheckNeedPID();
	}
}

void CServiceFilter::Clear(WORD tsid)
{
	this->transportStreamID = tsid;
	this->catUtil = CCATUtil();
	this->pmtUtilMap.clear();
	this->pat.Clear();
	this->pmt.Clear();
	this->pidInfoMap.clear();
}

CServiceFilter::PID_INFO& CServiceFilter::InsertPIDInfo(vector<PID_INFO>& infoMap, WORD pid)
{
	auto itr = lower_bound_first(infoMap.begin(), infoMap.end(), pid);
	if( itr == infoMap.end() || itr->first != pid ){
		PID_INFO info = {};
		info.first = pid;
		itr = infoMap.insert(itr, info);
	}
	return *itr;
}

void CServiceFilter::FilterPacket(vector<BYTE>& outData, const BYTE* data, const CTSPacketUtil& packet)
{
	this->catOrPmtUpdated = false;

	//指定サービスに必要なPIDを解析
	if( packet.transport_scrambling_control == 0 ){
		//CAT
		if( packet.PID == 1 && this->catUtil.AddPacket(packet) ){
			this->catOrPmtUpdated = true;
			CheckNeedPID();
		}
		if( packet.payload_unit_start_indicator && packet.data_byteSize > 0 ){
			BYTE pointer = packet.data_byte[0];
			if( 1 + pointer < packet.data_byteSize ){
				if( packet.data_byte[1 + pointer] == 2 ){
					//PMT
					vector<pair<WORD, CPMTUtil>>::iterator itr =
						lower_bound_first(this->pmtUtilMap.begin(), this->pmtUtilMap.end(), packet.PID);
					if( itr == this->pmtUtilMap.end() || itr->first != packet.PID ){
						itr = this->pmtUtilMap.insert(itr, std::make_pair(packet.PID, CPMTUtil()));
					}
					if( itr->second.AddPacket(packet) ){
						this->catOrPmtUpdated = true;
						CheckNeedPID();
					}
				}
			}
		}else{
			//PMTの2パケット目かチェック
			vector<pair<WORD, CPMTUtil>>::iterator itr =
				lower_bound_first(this->pmtUtilMap.begin(), this->pmtUtilMap.end(), packet.PID);
			if( itr != this->pmtUtilMap.end() && itr->first == packet.PID && itr->second.AddPacket(packet) ){
				this->catOrPmtUpdated = true;
				CheckNeedPID();
			}
		}
	}

	PID_INFO& pidInfo = InsertPIDInfo(this->pidInfoMap, packet.PID);
	bool fixedDiscontinuity = false;
	bool passthrough = false;

	if( this->allServicesFlag ){
		//全サービス
		if( packet.PID == 0 ){
			//PAT
			if( !pidInfo.originalDropped || packet.payload_unit_start_indicator ){
				if( pidInfo.originalDropped ){
					pidInfo.originalDropped = false;
					if( pidInfo.onceOutputted ){
						//最終出力に対して連続にする
						BYTE shiftCounter = (0x11 + pidInfo.patLastCounter - packet.continuity_counter) & 0x0F;
						if( pidInfo.patShiftCounter != shiftCounter ){
							pidInfo.patShiftCounter = shiftCounter;
							fixedDiscontinuity = true;
						}
					}
				}
				size_t syncPos = outData.size();
				outData.insert(outData.end(), data, data + 188);
				pidInfo.patLastCounter = (packet.continuity_counter + pidInfo.patShiftCounter) & 0x0F;
				outData[syncPos + 3] = (outData[syncPos + 3] & 0xF0) | pidInfo.patLastCounter;
				pidInfo.onceOutputted = true;
			}
		}else{
			passthrough = true;
		}
	}else{
		//指定サービス
		if( packet.PID == 0 ){
			//PATなので必要なサービスのみに絞る
			if( packet.payload_unit_start_indicator ){
				BYTE* patBuff;
				DWORD patBuffSize;
				if( this->pat.GetPacket(&patBuff, &patBuffSize) ){
					for( DWORD i = 0; i + 187 < patBuffSize; i += 188 ){
						size_t syncPos = outData.size();
						outData.insert(outData.end(), patBuff + i, patBuff + i + 188);
						if( pidInfo.onceOutputted ){
							pidInfo.patLastCounter = (pidInfo.patLastCounter + 1) & 0x0F;
						}else{
							//最初にオリジナルと同期させて、全サービス時に無編集で連続になる可能性を高める
							pidInfo.patLastCounter = packet.continuity_counter;
							pidInfo.onceOutputted = true;
						}
						outData[syncPos + 3] = (outData[syncPos + 3] & 0xF0) | pidInfo.patLastCounter;
					}
				}
			}
			pidInfo.originalDropped = true;
		}else if( packet.PID < BON_SELECTIVE_PID || pidInfo.neededByCatOrPmt ){
			if( this->createPmtFlag && this->serviceIDList.empty() == false ){
				//PMT改変が有効なときは改変後のPMTを出力する
				vector<pair<WORD, CPMTUtil>>::iterator itr =
					lower_bound_first(this->pmtUtilMap.begin(), this->pmtUtilMap.end(), packet.PID);
				if( itr != this->pmtUtilMap.end() && itr->first == packet.PID &&
				    itr->second.GetProgramNumber() != 0 &&
				    itr->second.GetProgramNumber() == this->serviceIDList[0] ){
					if( packet.payload_unit_start_indicator ){
						BYTE* pmtBuff;
						DWORD pmtBuffSize;
						if( this->pmt.GetPacket(&pmtBuff, &pmtBuffSize, packet.PID) ){
							if( pidInfo.originalOutputted ){
								pidInfo.originalOutputted = false;
								//フィルタにより最終出力との間に不連続が生じるのでアダプテーションを置く
								BYTE counter = (CTSPacketUtil::GetContinuityCounterFrom188TS(pmtBuff) + 15) & 0x0F;
								outData.push_back(0x47);
								outData.push_back((packet.PID >> 8 & 0x1F) | 0x40);
								outData.push_back(packet.PID & 0xFF);
								outData.push_back(counter | 0x20);
								outData.push_back(1);
								outData.push_back(0x80);
								outData.insert(outData.end(), 182, 0xFF);
								fixedDiscontinuity = true;
							}
							outData.insert(outData.end(), pmtBuff, pmtBuff + pmtBuffSize);
							pidInfo.onceOutputted = true;
						}
					}
					pidInfo.originalDropped = true;
				}else{
					passthrough = true;
				}
			}else{
				passthrough = true;
			}
		}else{
			pidInfo.originalDropped = true;
		}
	}

	if( passthrough ){
		if( pidInfo.originalDropped ){
			pidInfo.originalDropped = false;
			if( pidInfo.onceOutputted && (!packet.has_adaptation_field_flags || !packet.discontinuity_indicator) ){
				//フィルタにより最終出力との間に不連続が生じるのでアダプテーションを置く
				BYTE counter = (packet.continuity_counter + (packet.adaptation_field_control & 1 ? 15 : 0)) & 0x0F;
				outData.push_back(0x47);
				outData.push_back((packet.PID >> 8 & 0x1F) | 0x40);
				outData.push_back(packet.PID & 0xFF);
				outData.push_back(counter | 0x20);
				outData.push_back(1);
				outData.push_back(0x80);
				outData.insert(outData.end(), 182, 0xFF);
				fixedDiscontinuity = true;
			}
		}
		outData.insert(outData.end(), data, data + 188);
		pidInfo.onceOutputted = true;
		pidInfo.originalOutputted = true;
	}

	if( fixedDiscontinuity ){
		AddDebugLogFormat(L"CServiceFilter::FilterPacket Fixed discontinuity PID:0x%04X", packet.PID);
	}
}

void CServiceFilter::CheckNeedPID()
{
	for( auto itr = this->pidInfoMap.begin(); itr != this->pidInfoMap.end(); itr++ ){
		itr->neededByCatOrPmt = false;
	}
	//PAT作成用のPMTリスト
	vector<pair<WORD, WORD>> pidList;
	//NITのPID追加しておく
	pidList.push_back(std::make_pair((WORD)0x10, (WORD)0));

	//EMMのPID
	for( auto itr = this->catUtil.GetPIDList().cbegin(); itr != this->catUtil.GetPIDList().end(); itr++ ){
		InsertPIDInfo(this->pidInfoMap, *itr).neededByCatOrPmt = true;
	}

	for( auto itr = this->pmtUtilMap.cbegin(); itr != this->pmtUtilMap.end(); itr++ ){
		WORD programNumber = itr->second.GetProgramNumber();
		if( programNumber != 0 &&
		    (this->allServicesFlag ||
		     std::find(this->serviceIDList.begin(), this->serviceIDList.end(), programNumber) != this->serviceIDList.end()) ){
			//PAT作成用のPMTリスト作成
			pidList.push_back(std::make_pair(itr->first, programNumber));
			//PMT記載のPIDを登録
			InsertPIDInfo(this->pidInfoMap, itr->first).neededByCatOrPmt = true;
			WORD pcrPID = itr->second.GetPcrPID();
			if( pcrPID != 0x1FFF ){
				InsertPIDInfo(this->pidInfoMap, pcrPID).neededByCatOrPmt = true;
			}
			if( this->createPmtFlag &&
			    this->allServicesFlag == false &&
			    this->serviceIDList[0] == programNumber ){
				this->pmt.SetSectionData(itr->second.GetSectionData().data(), (DWORD)itr->second.GetSectionData().size());
			}
			for( auto itrPID = itr->second.GetPIDTypeList().cbegin(); itrPID != itr->second.GetPIDTypeList().end(); itrPID++ ){
				//PMT改変が有効なときは改変後に含まれるPIDのみ登録
				if( this->createPmtFlag == false ||
				    this->allServicesFlag ||
				    this->serviceIDList[0] != programNumber ||
				    this->pmt.IsEcmPID(itrPID->first) ||
				    this->pmt.IsElementaryPID(itrPID->first) ){
					InsertPIDInfo(this->pidInfoMap, itrPID->first).neededByCatOrPmt = true;
				}
			}
		}
	}
	this->pat.SetParam(this->transportStreamID, pidList);
}
