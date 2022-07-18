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
	this->needPIDList.clear();
	this->pat.Clear();
	this->pmt.Clear();
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

	if( this->allServicesFlag ){
		//全サービス
		outData.insert(outData.end(), data, data + 188);
	}else{
		//指定サービス
		if( packet.PID == 0 ){
			//PATなので必要なサービスのみに絞る
			if( packet.payload_unit_start_indicator ){
				BYTE* patBuff;
				DWORD patBuffSize;
				if( this->pat.GetPacket(&patBuff, &patBuffSize) ){
					outData.insert(outData.end(), patBuff, patBuff + patBuffSize);
				}
			}
		}else if( packet.PID < BON_SELECTIVE_PID ||
		          std::binary_search(this->needPIDList.begin(), this->needPIDList.end(), packet.PID) ){
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
							outData.insert(outData.end(), pmtBuff, pmtBuff + pmtBuffSize);
						}
					}
				}else{
					outData.insert(outData.end(), data, data + 188);
				}
			}else{
				outData.insert(outData.end(), data, data + 188);
			}
		}
	}
}

void CServiceFilter::CheckNeedPID()
{
	this->needPIDList.clear();
	//PAT作成用のPMTリスト
	vector<pair<WORD, WORD>> pidList;
	//NITのPID追加しておく
	pidList.push_back(std::make_pair((WORD)0x10, (WORD)0));

	//EMMのPID
	for( auto itr = this->catUtil.GetPIDList().cbegin(); itr != this->catUtil.GetPIDList().end(); itr++ ){
		if( std::find(this->needPIDList.begin(), this->needPIDList.end(), *itr) == this->needPIDList.end() ){
			this->needPIDList.push_back(*itr);
		}
	}

	for( auto itr = this->pmtUtilMap.cbegin(); itr != this->pmtUtilMap.end(); itr++ ){
		WORD programNumber = itr->second.GetProgramNumber();
		if( programNumber != 0 &&
		    (this->allServicesFlag ||
		     std::find(this->serviceIDList.begin(), this->serviceIDList.end(), programNumber) != this->serviceIDList.end()) ){
			//PAT作成用のPMTリスト作成
			pidList.push_back(std::make_pair(itr->first, programNumber));
			//PMT記載のPIDを登録
			if( std::find(this->needPIDList.begin(), this->needPIDList.end(), itr->first) == this->needPIDList.end() ){
				this->needPIDList.push_back(itr->first);
			}
			WORD pcrPID = itr->second.GetPcrPID();
			if( pcrPID != 0x1FFF &&
			    std::find(this->needPIDList.begin(), this->needPIDList.end(), pcrPID) == this->needPIDList.end() ){
				this->needPIDList.push_back(pcrPID);
			}
			if( this->createPmtFlag &&
			    this->allServicesFlag == false &&
			    this->serviceIDList[0] == programNumber ){
				this->pmt.SetSectionData(itr->second.GetSectionData().data(), (DWORD)itr->second.GetSectionData().size());
			}
			for( auto itrPID = itr->second.GetPIDTypeList().cbegin(); itrPID != itr->second.GetPIDTypeList().end(); itrPID++ ){
				//PMT改変が有効なときは改変後に含まれるPIDのみ登録
				if( (this->createPmtFlag == false ||
				     this->allServicesFlag ||
				     this->serviceIDList[0] != programNumber ||
				     this->pmt.IsEcmPID(itrPID->first) ||
				     this->pmt.IsElementaryPID(itrPID->first)) &&
				    std::find(this->needPIDList.begin(), this->needPIDList.end(), itrPID->first) == this->needPIDList.end() ){
					this->needPIDList.push_back(itrPID->first);
				}
			}
		}
	}
	std::sort(this->needPIDList.begin(), this->needPIDList.end());
	this->pat.SetParam(this->transportStreamID, pidList);
}
