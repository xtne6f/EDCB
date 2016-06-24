#include "StdAfx.h"
#include "DecodeUtil.h"

#include "../../Common/StringUtil.h"
#include "ARIB8CharDecode.h"
#include "../../Common/EpgTimerUtil.h"

CDecodeUtil::CDecodeUtil(void)
{
	this->epgDBUtil = NULL;

	this->totTime.dwHighDateTime = 0;
	this->tdtTime.dwHighDateTime = 0;
	this->sitTime.dwHighDateTime = 0;
}

void CDecodeUtil::SetEpgDB(CEpgDBUtil* epgDBUtil_)
{
	this->epgDBUtil = epgDBUtil_;
}

void CDecodeUtil::Clear()
{
	this->buffUtilMap.clear();

	this->patInfo.reset();

	this->nitActualInfo.nitSection.clear();
	this->sdtActualInfo.sdtSection.clear();

	this->bitInfo.reset();
	this->sitInfo.reset();

	this->totTime.dwHighDateTime = 0;
	this->tdtTime.dwHighDateTime = 0;
	this->sitTime.dwHighDateTime = 0;

	if( this->epgDBUtil != NULL ){
		this->epgDBUtil->SetStreamChangeEvent();
		this->epgDBUtil->ClearSectionStatus();
	}
}

void CDecodeUtil::ClearBuff(WORD noClearPid)
{
	this->buffUtilMap.erase(this->buffUtilMap.begin(), this->buffUtilMap.lower_bound(noClearPid));
	this->buffUtilMap.erase(this->buffUtilMap.upper_bound(noClearPid), this->buffUtilMap.end());
}

void CDecodeUtil::ChangeTSIDClear(WORD noClearPid)
{
	ClearBuff(noClearPid);

	this->patInfo.reset();

	this->nitActualInfo.nitSection.clear();
	this->sdtActualInfo.sdtSection.clear();

	this->bitInfo.reset();
	this->sitInfo.reset();

	this->totTime.dwHighDateTime = 0;
	this->tdtTime.dwHighDateTime = 0;
	this->sitTime.dwHighDateTime = 0;

	if( this->epgDBUtil != NULL ){
		this->epgDBUtil->SetStreamChangeEvent();
		this->epgDBUtil->ClearSectionStatus();
	}
}

void CDecodeUtil::AddTSData(BYTE* data)
{
	{
		CTSPacketUtil tsPacket;
		if( tsPacket.Set188TS(data, 188) == TRUE ){
			if( tsPacket.PID == 0x1FFF ){
				return;
			}
			CTSBuffUtil* buffUtil = NULL;

			map<WORD, CTSBuffUtil>::iterator itr;
			itr = this->buffUtilMap.find( tsPacket.PID );
			if( itr == this->buffUtilMap.end() ){
				//まだPIDがないので新規
				buffUtil = &this->buffUtilMap.insert(std::make_pair(tsPacket.PID, CTSBuffUtil())).first->second;
			}else{
				buffUtil = &itr->second;
			}
			if( buffUtil->Add188TS(&tsPacket) == TRUE ){
				BYTE* section = NULL;
				DWORD sectionSize = 0;
				while( buffUtil->GetSectionBuff( &section, &sectionSize ) == TRUE ){
					if( buffUtil->IsPES() == TRUE ){
						continue;
					}
					CPSITable* table;
					CTableUtil::t_type type = this->tableUtil.Decode(section, sectionSize, &table);
					BOOL ret;
					switch( type ){
					case CTableUtil::TYPE_PAT:
						ret = CheckPAT(tsPacket.PID, static_cast<CPATTable*>(table));
						break;
					case CTableUtil::TYPE_NIT:
						ret = CheckNIT(tsPacket.PID, static_cast<CNITTable*>(table));
						break;
					case CTableUtil::TYPE_SDT:
						ret = CheckSDT(tsPacket.PID, static_cast<CSDTTable*>(table));
						break;
					case CTableUtil::TYPE_TOT:
						ret = CheckTOT(tsPacket.PID, static_cast<CTOTTable*>(table));
						break;
					case CTableUtil::TYPE_TDT:
						ret = CheckTDT(tsPacket.PID, static_cast<CTDTTable*>(table));
						break;
					case CTableUtil::TYPE_EIT:
						ret = CheckEIT(tsPacket.PID, static_cast<CEITTable*>(table));
						break;
					case CTableUtil::TYPE_BIT:
						ret = CheckBIT(tsPacket.PID, static_cast<CBITTable*>(table));
						break;
					case CTableUtil::TYPE_SIT:
						ret = CheckSIT(tsPacket.PID, static_cast<CSITTable*>(table));
						break;
					case CTableUtil::TYPE_NONE:
						if( section[0] == 0 ){
							_OutputDebugString(L"★pid 0x%04X\r\n", tsPacket.PID);
						}
						ret = TRUE;
						break;
					default:
						ret = FALSE;
						break;
					}
					if( ret == FALSE ){
						//奪われなかったので返す
						this->tableUtil.Putback(type, table);
					}
				}
			}
		}
	}
}

BOOL CDecodeUtil::CheckPAT(WORD PID, CPATTable* pat)
{
	if( pat == NULL ){
		return FALSE;
	}

	if( this->patInfo == NULL ){
		//初回
		this->patInfo.reset(pat);
	}else{
		if( this->patInfo->transport_stream_id != pat->transport_stream_id ){
			//TSID変わったのでチャンネル変わった
			ChangeTSIDClear(PID);
			this->patInfo.reset(pat);
		}else if(this->patInfo->version_number != pat->version_number){
			//バージョン変わった
			this->patInfo.reset(pat);
		}else{
			//変更なし
			return FALSE;
		}
	}
	return TRUE;
}

BOOL CDecodeUtil::CheckNIT(WORD PID, CNITTable* nit)
{
	if( nit == NULL ){
		return FALSE;
	}

	if( epgDBUtil != NULL ){
		epgDBUtil->AddServiceList(nit);
	}

	if( nit->table_id == 0x40 ){
		//自ネットワーク
		if( this->nitActualInfo.nitSection.empty() ){
			//初回
			this->nitActualInfo.last_section_number = nit->last_section_number;
			this->nitActualInfo.nitSection[nit->section_number].reset(nit);
		}else{
			if( this->nitActualInfo.nitSection.begin()->second->network_id != nit->network_id ){
				//NID変わったのでネットワーク変わった
				ChangeTSIDClear(PID);
				this->nitActualInfo.last_section_number = nit->last_section_number;
				this->nitActualInfo.nitSection[nit->section_number].reset(nit);
			}else if(this->nitActualInfo.nitSection.begin()->second->version_number != nit->version_number){
				//バージョン変わった
				this->nitActualInfo.nitSection.clear();
				this->nitActualInfo.last_section_number = nit->last_section_number;
				this->nitActualInfo.nitSection[nit->section_number].reset(nit);
			}else{
				map<BYTE, std::unique_ptr<const CNITTable>>::iterator itr;
				itr = this->nitActualInfo.nitSection.find(0);
				if( itr != this->nitActualInfo.nitSection.end() ){
					if( (itr->second->TSInfoList.size() != 0 && nit->TSInfoList.size() != 0) &&
						(itr->first == nit->section_number)
						){
						if( itr->second->TSInfoList[0].original_network_id != nit->TSInfoList[0].original_network_id ){
							//ONID変わったのでネットワーク変わった
							ChangeTSIDClear(PID);
							this->nitActualInfo.last_section_number = nit->last_section_number;
							this->nitActualInfo.nitSection[nit->section_number].reset(nit);
						}else{
							if( itr->second->TSInfoList[0].transport_stream_id != nit->TSInfoList[0].transport_stream_id ){
								//TSID変わったのでネットワーク変わった
								ChangeTSIDClear(PID);
								this->nitActualInfo.last_section_number = nit->last_section_number;
								this->nitActualInfo.nitSection[nit->section_number].reset(nit);
							}else{
								//変化なし
								if( this->nitActualInfo.nitSection.count(nit->section_number) == 0 ){
									this->nitActualInfo.nitSection[nit->section_number].reset(nit);
									return TRUE;
								}
								return FALSE;
							}
						}
					}else{
						//変化なし
						if( this->nitActualInfo.nitSection.count(nit->section_number) == 0 ){
							this->nitActualInfo.nitSection[nit->section_number].reset(nit);
							return TRUE;
						}
						return FALSE;
					}
				}else{
					//変化なし
					if( this->nitActualInfo.nitSection.count(nit->section_number) == 0 ){
						this->nitActualInfo.nitSection[nit->section_number].reset(nit);
						return TRUE;
					}
					return FALSE;
				}
			}
		}
		/*
//		_OutputDebugString(L"find NIT\r\n");
		for( size_t i=0; i<nit->descriptorList.size(); i++ ){
			if( nit->descriptorList[i]->networkName != NULL ){
				if( nit->descriptorList[i]->networkName->char_nameLength > 0 ){
					CARIB8CharDecode arib;
					string network_name = "";
					arib.PSISI((const BYTE*)nit->descriptorList[i]->networkName->char_name, nit->descriptorList[i]->networkName->char_nameLength, &network_name);
					wstring network_nameW = L"";
					AtoW(network_name, network_nameW);
//					_OutputDebugString(L"%s\r\n", network_nameW.c_str());
				}
			}
		}
		for( size_t i=0; i<nit->TSInfoList.size(); i++ ){
			for( size_t j=0; j<nit->TSInfoList[i]->descriptorList.size(); j++ ){
				if( nit->TSInfoList[i]->descriptorList[j]->TSInfo != NULL ){
					CTSInfoDesc* TSInfo = nit->TSInfoList[i]->descriptorList[j]->TSInfo;
					CARIB8CharDecode arib;
					wstring ts_nameW = L"";
					if( TSInfo->length_of_ts_name > 0 ){
						string ts_name = "";
						arib.PSISI((const BYTE*)TSInfo->ts_name_char, TSInfo->length_of_ts_name, &ts_name);
						AtoW(ts_name, ts_nameW);
					}
//					_OutputDebugString(L"remote_control_key_id %d , %s\r\n", TSInfo->remote_control_key_id, ts_nameW.c_str());
				}
			}
		}
		*/
	}else if( nit->table_id == 0x41 ){
		//他ネットワーク
		//特に扱う必要性なし
		return FALSE;
	}else{
		return FALSE;
	}

	return TRUE;
}

BOOL CDecodeUtil::CheckSDT(WORD PID, CSDTTable* sdt)
{
	if( sdt == NULL ){
		return FALSE;
	}

	if( epgDBUtil != NULL ){
		epgDBUtil->AddSDT(sdt);
	}

	if( sdt->table_id == 0x42 ){
		//自ストリーム
		if( this->sdtActualInfo.sdtSection.empty() ){
			//初回
			this->sdtActualInfo.last_section_number = sdt->last_section_number;
			this->sdtActualInfo.sdtSection[sdt->section_number].reset(sdt);
		}else{
			if( this->sdtActualInfo.sdtSection.begin()->second->original_network_id != sdt->original_network_id ){
				//ONID変わったのでネットワーク変わった
				ChangeTSIDClear(PID);
				this->sdtActualInfo.last_section_number = sdt->last_section_number;
				this->sdtActualInfo.sdtSection[sdt->section_number].reset(sdt);
			}else if( this->sdtActualInfo.sdtSection.begin()->second->transport_stream_id != sdt->transport_stream_id ){
				//TSID変わったのでチャンネル変わった
				ChangeTSIDClear(PID);
				this->sdtActualInfo.last_section_number = sdt->last_section_number;
				this->sdtActualInfo.sdtSection[sdt->section_number].reset(sdt);
			}else if( this->sdtActualInfo.sdtSection.begin()->second->version_number != sdt->version_number ){
				//バージョン変わった
				this->sdtActualInfo.sdtSection.clear();
				this->sdtActualInfo.last_section_number = sdt->last_section_number;
				this->sdtActualInfo.sdtSection[sdt->section_number].reset(sdt);
			}else{
				//変化なし
				if( this->sdtActualInfo.sdtSection.count(sdt->section_number) == 0 ){
					this->sdtActualInfo.sdtSection[sdt->section_number].reset(sdt);
					return TRUE;
				}
				return FALSE;
			}
		}
////		_OutputDebugString(L"find SDT\r\n");
////		_OutputDebugString(L"ONID 0x%04X, TSID 0x%04X\r\n", sdt->original_network_id, sdt->transport_stream_id);
//		for(size_t i=0; i<sdt->serviceInfoList.size(); i++ ){
////			_OutputDebugString(L"SID 0x%04X\r\n", sdt->serviceInfoList[i]->service_id);
//			for( size_t j=0; j<sdt->serviceInfoList[i]->descriptorList.size(); j++ ){
//				if( sdt->serviceInfoList[i]->descriptorList[j]->service != NULL ){
//					CServiceDesc* service = sdt->serviceInfoList[i]->descriptorList[j]->service;
//					CARIB8CharDecode arib;
//					string service_provider_name = "";
//					string service_name = "";
//					if( service->service_provider_name_length > 0 ){
//						arib.PSISI((const BYTE*)service->char_service_provider_name, service->service_provider_name_length, &service_provider_name);
//					}
//					if( service->service_name_length > 0 ){
//						arib.PSISI((const BYTE*)service->char_service_name, service->service_name_length, &service_name);
//					}
///*					wstring service_provider_nameW = L"";
//					wstring service_nameW = L"";
//					AtoW(service_provider_name, service_provider_nameW);
//					AtoW(service_name, service_nameW);
//					_OutputDebugString(L"type 0x%04X %s %s\r\n", service->service_type, service_provider_nameW.c_str(), service_nameW.c_str());
//*/				}
//				//logo_transmission
//			}
//		}

	}else if( sdt->table_id == 0x46 ){
		//他ストリーム
		//特に扱う必要性なし
		return FALSE;
	}else{
		return FALSE;
	}

	return TRUE;
}

BOOL CDecodeUtil::CheckTOT(WORD PID, CTOTTable* tot)
{
	if( tot == NULL ){
		return FALSE;
	}

	if( SystemTimeToFileTime(&tot->jst_time, &this->totTime) == FALSE ){
		this->totTime.dwHighDateTime = 0;
	}
	this->totTimeTick = GetTickCount();

/*	_OutputDebugString(L"%d/%02d/%02d %02d:%02d:%02d\r\n",
		tot->jst_time.wYear, 
		tot->jst_time.wMonth, 
		tot->jst_time.wDay, 
		tot->jst_time.wHour, 
		tot->jst_time.wMinute, 
		tot->jst_time.wSecond 
		);
		*/

	return FALSE;
}

BOOL CDecodeUtil::CheckTDT(WORD PID, CTDTTable* tdt)
{
	if( tdt == NULL ){
		return FALSE;
	}

	if( SystemTimeToFileTime(&tdt->jst_time, &this->tdtTime) == FALSE ){
		this->tdtTime.dwHighDateTime = 0;
	}
	this->tdtTimeTick = GetTickCount();
	/*
	_OutputDebugString(L"%d/%02d/%02d %02d:%02d:%02d\r\n",
		tdt->jst_time.wYear, 
		tdt->jst_time.wMonth, 
		tdt->jst_time.wDay, 
		tdt->jst_time.wHour, 
		tdt->jst_time.wMinute, 
		tdt->jst_time.wSecond 
		);*/
		
	return FALSE;
}

BOOL CDecodeUtil::CheckEIT(WORD PID, CEITTable* eit)
{
	if( eit == NULL ){
		return FALSE;
	}
	
	if( epgDBUtil != NULL ){
		FILETIME time = {};
		GetNowTime(&time);
		epgDBUtil->AddEIT(PID, eit, (__int64)time.dwHighDateTime << 32 | time.dwLowDateTime);
	}
	return FALSE;
}

BOOL CDecodeUtil::CheckBIT(WORD PID, CBITTable* bit)
{
	if( bit == NULL ){
		return FALSE;
	}

	if( this->bitInfo == NULL ){
		//初回
		this->bitInfo.reset(bit);
	}else{
		if( this->bitInfo->original_network_id != bit->original_network_id ){
			//ONID変わったのでネットワーク変わった
			ChangeTSIDClear(PID);
			this->bitInfo.reset(bit);
		}else if( this->bitInfo->version_number != bit->version_number ){
			//バージョン変わった
			this->bitInfo.reset(bit);
		}else{
			//変化なし
			return FALSE;
		}
	}

	return TRUE;
}

BOOL CDecodeUtil::CheckSIT(WORD PID, CSITTable* sit)
{
	if( sit == NULL ){
		return FALSE;
	}

	//時間計算
	if( this->totTime.dwHighDateTime == 0 && this->tdtTime.dwHighDateTime == 0 ){
		for( size_t i=0; i<sit->descriptorList.size(); i++ ){
			if( sit->descriptorList[i].GetNumber(AribDescriptor::descriptor_tag) == AribDescriptor::partialTS_time_descriptor ){
				if( sit->descriptorList[i].GetNumber(AribDescriptor::jst_time_flag) == 1 ){
					DWORD timeBytesSize;
					const BYTE* timeBytes = sit->descriptorList[i].GetBinary(AribDescriptor::jst_time, &timeBytesSize);
					if( timeBytes != NULL && timeBytesSize >= 5 ){
						DWORD mjd = timeBytes[0] << 8 | timeBytes[1];
						SYSTEMTIME time;
						_MJDtoSYSTEMTIME(mjd, &time);
						BYTE b = timeBytes[2];
						time.wHour = (WORD)_BCDtoDWORD(&b, 1, 2);
						b = timeBytes[3];
						time.wMinute = (WORD)_BCDtoDWORD(&b, 1, 2);
						b = timeBytes[4];
						time.wSecond = (WORD)_BCDtoDWORD(&b, 1, 2);

						if( SystemTimeToFileTime(&time, &this->sitTime) == FALSE ){
							this->sitTime.dwHighDateTime = 0;
						}
						this->sitTimeTick = GetTickCount();
					}
				}
			}
		}
	}

	if( epgDBUtil != NULL ){
		if( this->patInfo != NULL ){
			epgDBUtil->AddServiceList(this->patInfo->transport_stream_id, sit);
		}
	}

	if( this->sitInfo == NULL ){
		//初回
		this->sitInfo.reset(sit);
	}else{
		if( this->sitInfo->version_number != sit->version_number ){
			//バージョン変わった
			this->sitInfo.reset(sit);
		}else{
			//変化なし
			return FALSE;
		}
	}

	return TRUE;
}

//解析データの現在のストリームＩＤを取得する
// originalNetworkID		[OUT]現在のoriginalNetworkID
// transportStreamID		[OUT]現在のtransportStreamID
BOOL CDecodeUtil::GetTSID(
	WORD* originalNetworkID,
	WORD* transportStreamID
	)
{
	if( sdtActualInfo.sdtSection.empty() == false ){
		*originalNetworkID = sdtActualInfo.sdtSection.begin()->second->original_network_id;
		*transportStreamID = sdtActualInfo.sdtSection.begin()->second->transport_stream_id;
		return TRUE;
	}else if( this->sitInfo != NULL && this->patInfo != NULL ){
		//TSID
		*transportStreamID = this->patInfo->transport_stream_id;
		//ONID
		for( size_t i=0; i<this->sitInfo->descriptorList.size(); i++ ){
			if( this->sitInfo->descriptorList[i].GetNumber(AribDescriptor::descriptor_tag) == AribDescriptor::network_identification_descriptor ){
				*originalNetworkID = (WORD)this->sitInfo->descriptorList[i].GetNumber(AribDescriptor::network_id);
				return TRUE;
			}
		}
	}
	return FALSE;
}

//自ストリームのサービス一覧を取得する
//引数：
// serviceListSize			[OUT]serviceListの個数
// serviceList				[OUT]サービス情報のリスト（DLL内で自動的にdeleteする。次に取得を行うまで有効）
BOOL CDecodeUtil::GetServiceListActual(
	DWORD* serviceListSize,
	SERVICE_INFO** serviceList_
	)
{
	if( this->nitActualInfo.nitSection.empty() || this->sdtActualInfo.sdtSection.empty() ){
		return GetServiceListSIT(serviceListSize, serviceList_);
	}else{
		if( (size_t)(this->nitActualInfo.last_section_number+1) != this->nitActualInfo.nitSection.size() ||
			(size_t)(this->sdtActualInfo.last_section_number+1) != this->sdtActualInfo.sdtSection.size() ){
			return FALSE;
		}
	}
	*serviceListSize = 0;

	map<BYTE, std::unique_ptr<const CSDTTable>>::iterator itrSdt;
	for(itrSdt = this->sdtActualInfo.sdtSection.begin(); itrSdt != this->sdtActualInfo.sdtSection.end(); itrSdt++){
		*serviceListSize += (DWORD)itrSdt->second->serviceInfoList.size();
	}
	this->serviceList.reset(new SERVICE_INFO[*serviceListSize]);


	wstring network_nameW = L"";
	wstring ts_nameW = L"";
	BYTE remote_control_key_id = 0;
	vector<WORD> partialServiceList;

	map<BYTE, std::unique_ptr<const CNITTable>>::iterator itrNit;
	for( itrNit = this->nitActualInfo.nitSection.begin(); itrNit != this->nitActualInfo.nitSection.end(); itrNit++ ){
		for( size_t i=0; i<itrNit->second->descriptorList.size(); i++ ){
			if( itrNit->second->descriptorList[i].GetNumber(AribDescriptor::descriptor_tag) == AribDescriptor::network_name_descriptor ){
				const AribDescriptor::CDescriptor* networkName = &itrNit->second->descriptorList[i];
				DWORD srcSize;
				const char* src = networkName->GetStringOrEmpty(AribDescriptor::d_char, &srcSize);
				if( srcSize > 0 ){
					CARIB8CharDecode arib;
					string network_name = "";
					arib.PSISI((const BYTE*)src, srcSize, &network_name);
					AtoW(network_name, network_nameW);
				}
			}
		}
		for( size_t i=0; i<itrNit->second->TSInfoList.size(); i++ ){
			for( size_t j=0; j<itrNit->second->TSInfoList[i].descriptorList.size(); j++ ){
				if( itrNit->second->TSInfoList[i].descriptorList[j].GetNumber(AribDescriptor::descriptor_tag) == AribDescriptor::ts_information_descriptor ){
					const AribDescriptor::CDescriptor* TSInfo = &itrNit->second->TSInfoList[i].descriptorList[j];
					DWORD srcSize;
					const char* src = TSInfo->GetStringOrEmpty(AribDescriptor::ts_name_char, &srcSize);
					if( srcSize > 0 ){
						CARIB8CharDecode arib;
						string ts_name = "";
						arib.PSISI((const BYTE*)src, srcSize, &ts_name);
						AtoW(ts_name, ts_nameW);
					}
					remote_control_key_id = (BYTE)TSInfo->GetNumber(AribDescriptor::remote_control_key_id);
				}
				if( itrNit->second->TSInfoList[i].descriptorList[j].GetNumber(AribDescriptor::descriptor_tag) == AribDescriptor::partial_reception_descriptor ){
					partialServiceList.clear();
					AribDescriptor::CDescriptor::CLoopPointer lp;
					if( itrNit->second->TSInfoList[i].descriptorList[j].EnterLoop(lp) ){
						for( DWORD k=0; itrNit->second->TSInfoList[i].descriptorList[j].SetLoopIndex(lp, k); k++ ){
							partialServiceList.push_back((WORD)itrNit->second->TSInfoList[i].descriptorList[j].GetNumber(AribDescriptor::service_id, lp));
						}
					}
				}
			}
		}
	}

	DWORD count = 0;
	for(itrSdt = this->sdtActualInfo.sdtSection.begin(); itrSdt != this->sdtActualInfo.sdtSection.end(); itrSdt++){
		for( size_t i=0; i<itrSdt->second->serviceInfoList.size(); i++ ){
			this->serviceList[count].original_network_id = itrSdt->second->original_network_id;
			this->serviceList[count].transport_stream_id = itrSdt->second->transport_stream_id;
			this->serviceList[count].service_id = itrSdt->second->serviceInfoList[i].service_id;
			this->serviceList[count].extInfo = new SERVICE_EXT_INFO;

			for( size_t j=0; j<itrSdt->second->serviceInfoList[i].descriptorList.size(); j++ ){
				if( itrSdt->second->serviceInfoList[i].descriptorList[j].GetNumber(AribDescriptor::descriptor_tag) == AribDescriptor::service_descriptor ){
					const AribDescriptor::CDescriptor* service = &itrSdt->second->serviceInfoList[i].descriptorList[j];
					CARIB8CharDecode arib;
					string service_provider_name = "";
					string service_name = "";
					const char* src;
					DWORD srcSize;
					src = service->GetStringOrEmpty(AribDescriptor::service_provider_name, &srcSize);
					if( srcSize > 0 ){
						arib.PSISI((const BYTE*)src, srcSize, &service_provider_name);
					}
					src = service->GetStringOrEmpty(AribDescriptor::service_name, &srcSize);
					if( srcSize > 0 ){
						arib.PSISI((const BYTE*)src, srcSize, &service_name);
					}
					wstring service_provider_nameW = L"";
					wstring service_nameW = L"";
					AtoW(service_provider_name, service_provider_nameW);
					AtoW(service_name, service_nameW);

					this->serviceList[count].extInfo->service_type = (BYTE)service->GetNumber(AribDescriptor::service_type);
					if( service_provider_nameW.size() > 0 ){
						this->serviceList[count].extInfo->service_provider_name = new WCHAR[service_provider_nameW.size()+1];
						wcscpy_s(this->serviceList[count].extInfo->service_provider_name, service_provider_nameW.size()+1, service_provider_nameW.c_str());
					}
					if( service_nameW.size() > 0 ){
						this->serviceList[count].extInfo->service_name = new WCHAR[service_nameW.size()+1];
						wcscpy_s(this->serviceList[count].extInfo->service_name, service_nameW.size()+1, service_nameW.c_str());
					}
				}
			}

			if( network_nameW.size() > 0 ){
				this->serviceList[count].extInfo->network_name = new WCHAR[network_nameW.size()+1];
				wcscpy_s(this->serviceList[count].extInfo->network_name, network_nameW.size()+1, network_nameW.c_str());
			}
			if( ts_nameW.size() > 0 ){
				this->serviceList[count].extInfo->ts_name = new WCHAR[ts_nameW.size()+1];
				wcscpy_s(this->serviceList[count].extInfo->ts_name, ts_nameW.size()+1, ts_nameW.c_str());
			}
			this->serviceList[count].extInfo->remote_control_key_id = remote_control_key_id;

			this->serviceList[count].extInfo->partialReceptionFlag = FALSE;
			for( size_t j=0; j<partialServiceList.size(); j++ ){
				if( partialServiceList[j] == this->serviceList[count].service_id ){
					this->serviceList[count].extInfo->partialReceptionFlag = TRUE;
				}
			}

			count++;
		}
	}

	*serviceList_ = this->serviceList.get();


	return TRUE;
}

//自ストリームのサービス一覧をSITから取得する
//引数：
// serviceListSize			[OUT]serviceListの個数
// serviceList				[OUT]サービス情報のリスト（DLL内で自動的にdeleteする。次に取得を行うまで有効）
BOOL CDecodeUtil::GetServiceListSIT(
	DWORD* serviceListSize,
	SERVICE_INFO** serviceList_
	)
{
	if( this->sitInfo == NULL || this->patInfo == NULL ){
		return FALSE;
	}

	//ONID
	WORD ONID = 0xFFFF;
	for( size_t i=0; i<this->sitInfo->descriptorList.size(); i++ ){
		if( this->sitInfo->descriptorList[i].GetNumber(AribDescriptor::descriptor_tag) == AribDescriptor::network_identification_descriptor ){
			ONID = (WORD)this->sitInfo->descriptorList[i].GetNumber(AribDescriptor::network_id);
		}
	}

	//TSID
	WORD TSID = 0xFFFF;
	TSID = this->patInfo->transport_stream_id;

	*serviceListSize = (DWORD)this->sitInfo->serviceLoopList.size();
	this->serviceList.reset(new SERVICE_INFO[*serviceListSize]);

	wstring network_nameW = L"";
	wstring ts_nameW = L"";
	BYTE remote_control_key_id = 0;

	//サービスリスト
	for( DWORD i=0; i<*serviceListSize; i++ ){
		this->serviceList[i].original_network_id = ONID;
		this->serviceList[i].transport_stream_id = TSID;
		this->serviceList[i].service_id = this->sitInfo->serviceLoopList[i].service_id;
		this->serviceList[i].extInfo = new SERVICE_EXT_INFO;

		for( size_t j=0; j<this->sitInfo->serviceLoopList[i].descriptorList.size(); j++ ){
			if( this->sitInfo->serviceLoopList[i].descriptorList[j].GetNumber(AribDescriptor::descriptor_tag) == AribDescriptor::service_descriptor ){
				const AribDescriptor::CDescriptor* service = &this->sitInfo->serviceLoopList[i].descriptorList[j];
				CARIB8CharDecode arib;
				string service_provider_name = "";
				string service_name = "";
				const char* src;
				DWORD srcSize;
				src = service->GetStringOrEmpty(AribDescriptor::service_provider_name, &srcSize);
				if( srcSize > 0 ){
					arib.PSISI((const BYTE*)src, srcSize, &service_provider_name);
				}
				src = service->GetStringOrEmpty(AribDescriptor::service_name, &srcSize);
				if( srcSize > 0 ){
					arib.PSISI((const BYTE*)src, srcSize, &service_name);
				}
				wstring service_provider_nameW = L"";
				wstring service_nameW = L"";
				AtoW(service_provider_name, service_provider_nameW);
				AtoW(service_name, service_nameW);

				this->serviceList[i].extInfo->service_type = (BYTE)service->GetNumber(AribDescriptor::service_type);
				if( service_provider_nameW.size() > 0 ){
					this->serviceList[i].extInfo->service_provider_name = new WCHAR[service_provider_nameW.size()+1];
					wcscpy_s(this->serviceList[i].extInfo->service_provider_name, service_provider_nameW.size()+1, service_provider_nameW.c_str());
				}
				if( service_nameW.size() > 0 ){
					this->serviceList[i].extInfo->service_name = new WCHAR[service_nameW.size()+1];
					wcscpy_s(this->serviceList[i].extInfo->service_name, service_nameW.size()+1, service_nameW.c_str());
				}
			}
		}

		if( network_nameW.size() > 0 ){
			this->serviceList[i].extInfo->network_name = new WCHAR[network_nameW.size()+1];
			wcscpy_s(this->serviceList[i].extInfo->network_name, network_nameW.size()+1, network_nameW.c_str());
		}
		if( ts_nameW.size() > 0 ){
			this->serviceList[i].extInfo->ts_name = new WCHAR[ts_nameW.size()+1];
			wcscpy_s(this->serviceList[i].extInfo->ts_name, ts_nameW.size()+1, ts_nameW.c_str());
		}
		this->serviceList[i].extInfo->remote_control_key_id = remote_control_key_id;

		this->serviceList[i].extInfo->partialReceptionFlag = FALSE;
	}


	*serviceList_ = this->serviceList.get();

	return TRUE;
}

//ストリーム内の現在の時間情報を取得する
//引数：
// time				[OUT]ストリーム内の現在の時間
// tick				[OUT]timeを取得した時点のチックカウント
BOOL CDecodeUtil::GetNowTime(
	FILETIME* time,
	DWORD* tick
	)
{
	DWORD tick_;
	if( tick == NULL ){
		tick = &tick_;
	}
	if( this->totTime.dwHighDateTime != 0 ){
		*time = this->totTime;
		*tick = this->totTimeTick;
		return TRUE;
	}else if( this->tdtTime.dwHighDateTime != 0 ){
		*time = this->tdtTime;
		*tick = this->tdtTimeTick;
		return TRUE;
	}else if( this->sitTime.dwHighDateTime != 0 ){
		*time = this->sitTime;
		*tick = this->sitTimeTick;
		return TRUE;
	}
	return FALSE;
}
