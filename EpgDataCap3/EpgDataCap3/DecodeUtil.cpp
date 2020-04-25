#include "stdafx.h"
#include "DecodeUtil.h"

#include "../../Common/StringUtil.h"
#include "ARIB8CharDecode.h"
#include "../../Common/EpgTimerUtil.h"

#define SUPPORT_SKY_SD

namespace Desc = AribDescriptor;

#ifdef SUPPORT_SKY_SD

static bool SDDecodeNIT(const BYTE* section, DWORD sectionSize, Desc::CDescriptor& table)
{
	static const short parser0x82[] = {
		Desc::descriptor_tag, 8,
		Desc::descriptor_length, Desc::D_LOCAL, 8,
		Desc::D_BEGIN, Desc::descriptor_length,
			Desc::reserved, 8,
			Desc::d_char, Desc::D_BINARY_TO_END,
		Desc::D_END,
		Desc::D_FIN,
	};
	static const short parserUnknown[] = {
		Desc::descriptor_tag, 8,
		Desc::descriptor_length, Desc::D_LOCAL, 8,
		Desc::reserved, Desc::D_LOCAL, Desc::descriptor_length,
		Desc::D_FIN,
	};
	//記述子は基本的にunknown扱いとする
	Desc::PARSER_PAIR parserList[256] = {};
	for( BYTE i = 0, j = 0; i < 255; i++ ){
		//ただしサービスリスト記述子は扱う
		if( i != Desc::service_list_descriptor ){
			parserList[j].tag = i;
			parserList[j++].parser = i == 0x82 ? parser0x82 : parserUnknown;
		}
	}
	if( table.DecodeSI(section, sectionSize, NULL, Desc::TYPE_NIT, parserList) == false ||
	    table.GetNumber(Desc::network_id) != 1 ){
		return false;
	}
	Desc::CDescriptor::CLoopPointer lp;
	if( table.EnterLoop(lp) ){
		for( DWORD i = 0; table.SetLoopIndex(lp, i); i++ ){
			if( table.GetNumber(Desc::descriptor_tag, lp) == 0x82 && table.GetNumber(Desc::reserved, lp) == 1 ){
				//日本語版？ネットワーク記述子にキャスト
				table.SetNumber(Desc::descriptor_tag, Desc::network_name_descriptor, lp);
			}
		}
	}
	return true;
}

static bool SDDecodeSDT(const BYTE* section, DWORD sectionSize, Desc::CDescriptor& table)
{
	static const short parser0x82[] = {
		Desc::descriptor_tag, 8,
		Desc::descriptor_length, Desc::D_LOCAL, 8,
		Desc::D_BEGIN, Desc::descriptor_length,
			Desc::service_type, 8,
			Desc::service_name, Desc::D_BINARY_TO_END,
		Desc::D_END,
		Desc::D_FIN,
	};
	static const short parser0x8A[] = {
		Desc::descriptor_tag, 8,
		Desc::descriptor_length, Desc::D_LOCAL, 8,
		Desc::D_BEGIN, Desc::descriptor_length,
			Desc::service_type, 8,
			Desc::reserved, Desc::D_LOCAL_TO_END,
		Desc::D_END,
		Desc::D_FIN,
	};
	static const short parserUnknown[] = {
		Desc::descriptor_tag, 8,
		Desc::descriptor_length, Desc::D_LOCAL, 8,
		Desc::reserved, Desc::D_LOCAL, Desc::descriptor_length,
		Desc::D_FIN,
	};
	//記述子は基本的にunknown扱いとする
	Desc::PARSER_PAIR parserList[256];
	for( BYTE i = 0; i < 255; i++ ){
		parserList[i].tag = i;
		parserList[i].parser = i == 0x82 ? parser0x82 : i == 0x8A ? parser0x8A : parserUnknown;
	}
	parserList[255].tag = 0;
	parserList[255].parser = NULL;
	if( table.DecodeSI(section, sectionSize, NULL, Desc::TYPE_SDT, parserList) == false ||
	    table.GetNumber(Desc::original_network_id) != 1 ){
		return false;
	}
	Desc::CDescriptor::CLoopPointer lp;
	if( table.EnterLoop(lp) ){
		for( DWORD i = 0; table.SetLoopIndex(lp, i); i++ ){
			Desc::CDescriptor::CLoopPointer lp0x82, lp2 = lp;
			if( table.EnterLoop(lp2) ){
				bool found0x82 = false;
				DWORD service_type = 0;
				for( DWORD j = 0; table.SetLoopIndex(lp2, j); j++ ){
					if( table.GetNumber(Desc::descriptor_tag, lp2) == 0x82 ){
						//サービス名
						if( table.GetNumber(Desc::service_type, lp2) == 1 ){
							//日本語版？
							lp0x82 = lp2;
							found0x82 = true;
						}
					}else if( table.GetNumber(Desc::descriptor_tag, lp2) == 0x8A ){
						//サービスタイプ
						service_type = table.GetNumber(Desc::service_type, lp2);
					}
				}
				if( found0x82 ){
					//サービス記述子にキャスト
					table.SetNumber(Desc::descriptor_tag, Desc::service_descriptor, lp0x82);
					table.SetNumber(Desc::service_type, service_type == 0x81 ? 0xA1 : service_type, lp0x82);
				}
			}
		}
	}
	return true;
}

static bool SDDecodeEIT(const BYTE* section, DWORD sectionSize, Desc::CDescriptor& table)
{
	static const short parser0x82[] = {
		Desc::descriptor_tag, 8,
		Desc::descriptor_length, Desc::D_LOCAL, 8,
		Desc::D_BEGIN, Desc::descriptor_length,
			Desc::reserved, 8,
			Desc::event_name_char, Desc::D_BINARY_TO_END,
		Desc::D_END,
		Desc::D_FIN,
	};
	static const short parser0x85[] = {
		Desc::descriptor_tag, 8,
		Desc::descriptor_length, Desc::D_LOCAL, 8,
		Desc::D_BEGIN, Desc::descriptor_length,
			Desc::reserved, Desc::D_LOCAL, 4,
			Desc::stream_content, 4,
			Desc::component_type, 8,
			Desc::component_tag, 8,
			Desc::reserved, Desc::D_LOCAL, 8,
			Desc::text_char, Desc::D_BINARY_TO_END,
		Desc::D_END,
		Desc::D_FIN,
	};
	static const short parserUnknown[] = {
		Desc::descriptor_tag, 8,
		Desc::descriptor_length, Desc::D_LOCAL, 8,
		Desc::reserved, Desc::D_LOCAL, Desc::descriptor_length,
		Desc::D_FIN,
	};
	//記述子は基本的にunknown扱いとする
	Desc::PARSER_PAIR parserList[256];
	for( BYTE i = 0; i < 255; i++ ){
		parserList[i].tag = i;
		parserList[i].parser = i == 0x82 ? parser0x82 : i == 0x85 ? parser0x85 : parserUnknown;
	}
	parserList[255].tag = 0;
	parserList[255].parser = NULL;
	if( table.DecodeSI(section, sectionSize, NULL, Desc::TYPE_EIT, parserList) == false ||
	    table.GetNumber(Desc::original_network_id) != 1 ){
		return false;
	}
	Desc::CDescriptor::CLoopPointer lp;
	if( table.EnterLoop(lp) ){
		for( DWORD i = 0; table.SetLoopIndex(lp, i); i++ ){
			Desc::CDescriptor::CLoopPointer lp0x82, lp2 = lp;
			if( table.EnterLoop(lp2) ){
				bool found0x82 = false;
				for( DWORD j = 0; table.SetLoopIndex(lp2, j); j++ ){
					if( table.GetNumber(Desc::descriptor_tag, lp2) == 0x82 ){
						//番組名
						if( table.GetNumber(Desc::reserved, lp2) == 1 ){
							//日本語版？
							lp0x82 = lp2;
							found0x82 = true;
						}else if( table.GetNumber(Desc::reserved, lp2) == 2 && found0x82 == false ){
							//英語版？
							lp0x82 = lp2;
							found0x82 = true;
						}
					}else if( table.GetNumber(Desc::descriptor_tag, lp2) == 0x85 ){
						//コンポーネント
						if( table.GetNumber(Desc::stream_content, lp2) == 1 ){
							//映像。コンポーネント記述子にキャスト
							table.SetNumber(Desc::descriptor_tag, Desc::component_descriptor, lp2);
						}else if( table.GetNumber(Desc::stream_content, lp2) == 2 ){
							//音声。音声コンポーネント記述子にキャスト
							table.SetNumber(Desc::descriptor_tag, Desc::audio_component_descriptor, lp2);
						}
					}
				}
				if( found0x82 ){
					//短形式イベント記述子にキャスト
					table.SetNumber(Desc::descriptor_tag, Desc::short_event_descriptor, lp0x82);
				}
			}
		}
	}
	return true;
}

#endif //SUPPORT_SKY_SD

CDecodeUtil::CDecodeUtil(void)
{
	this->epgDBUtil = NULL;

	this->totTime = 0;
	this->tdtTime = 0;
	this->sitTime = 0;
}

void CDecodeUtil::SetEpgDB(CEpgDBUtil* epgDBUtil_)
{
	this->epgDBUtil = epgDBUtil_;
}

void CDecodeUtil::Clear()
{
	this->buffUtilMap.clear();

	this->patInfo.reset();

	this->nitActualInfo.clear();
	this->sdtActualInfo.clear();

	this->bitInfo.reset();
	this->sitInfo.reset();

	this->totTime = 0;
	this->tdtTime = 0;
	this->sitTime = 0;

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

	this->nitActualInfo.clear();
	this->sdtActualInfo.clear();

	this->bitInfo.reset();
	this->sitInfo.reset();

	this->totTime = 0;
	this->tdtTime = 0;
	this->sitTime = 0;

	if( this->epgDBUtil != NULL ){
		this->epgDBUtil->SetStreamChangeEvent();
		this->epgDBUtil->ClearSectionStatus();
	}
}

void CDecodeUtil::AddTSData(BYTE* data, DWORD size)
{
	for( DWORD i = 0; i + 188 <= size; i += 188 ){
		CTSPacketUtil tsPacket;
		if( tsPacket.Set188TS(data + i, 188) && tsPacket.PID != 0x1FFF ){
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
					switch( section[0] ){
					case 0x00:
						if( this->tableBuff.DecodeSI(section, sectionSize, NULL, Desc::TYPE_PAT) ){
							CheckPAT(tsPacket.PID, this->tableBuff);
						}
						break;
					case 0x40:
					case 0x41:
						{
							bool ret = this->tableBuff.DecodeSI(section, sectionSize, NULL, Desc::TYPE_NIT);
#ifdef SUPPORT_SKY_SD
							if( ret == false || this->tableBuff.GetNumber(Desc::network_id) == 1 ){
								ret = SDDecodeNIT(section, sectionSize, this->tableBuff);
							}
#endif
							if( ret ){
								CheckNIT(tsPacket.PID, this->tableBuff);
							}
						}
						break;
					case 0x42:
					case 0x46:
						{
							bool ret = this->tableBuff.DecodeSI(section, sectionSize, NULL, Desc::TYPE_SDT);
#ifdef SUPPORT_SKY_SD
							if( ret == false || this->tableBuff.GetNumber(Desc::original_network_id) == 1 ){
								ret = SDDecodeSDT(section, sectionSize, this->tableBuff);
							}
#endif
							if( ret ){
								CheckSDT(tsPacket.PID, this->tableBuff);
							}
						}
						break;
					case 0x70:
						if( this->tableBuff.DecodeSI(section, sectionSize, NULL, Desc::TYPE_TDT) ){
							CheckTDT(this->tableBuff);
						}
						break;
					case 0x73:
						if( this->tableBuff.DecodeSI(section, sectionSize, NULL, Desc::TYPE_TOT) ){
							CheckTDT(this->tableBuff);
						}
						break;
					case 0xC4:
						if( this->tableBuff.DecodeSI(section, sectionSize, NULL, Desc::TYPE_BIT) ){
							CheckBIT(tsPacket.PID, this->tableBuff);
						}
						break;
					case 0x7F:
						if( this->tableBuff.DecodeSI(section, sectionSize, NULL, Desc::TYPE_SIT) ){
							CheckSIT(this->tableBuff);
						}
						break;
					default:
						if( 0x4E <= section[0] && section[0] <= 0x6F ){
							bool ret = this->tableBuff.DecodeSI(section, sectionSize, NULL, Desc::TYPE_EIT);
#ifdef SUPPORT_SKY_SD
							if( ret == false || this->tableBuff.GetNumber(Desc::original_network_id) == 1 ){
								ret = SDDecodeEIT(section, sectionSize, this->tableBuff);
							}
#endif
							if( ret ){
								CheckEIT(tsPacket.PID, this->tableBuff);
							}
						}
						break;
					}
				}
			}
		}
	}
}

void CDecodeUtil::CheckPAT(WORD PID, const Desc::CDescriptor& pat)
{
	if( this->patInfo == NULL ){
		//初回
		this->patInfo.reset(new Desc::CDescriptor(pat));
	}else{
		if( this->patInfo->GetNumber(Desc::transport_stream_id) != pat.GetNumber(Desc::transport_stream_id) ){
			//TSID変わったのでチャンネル変わった
			ChangeTSIDClear(PID);
			this->patInfo.reset(new Desc::CDescriptor(pat));
		}else if( this->patInfo->GetNumber(Desc::version_number) != pat.GetNumber(Desc::version_number) ){
			//バージョン変わった
			this->patInfo.reset(new Desc::CDescriptor(pat));
		}else{
			//変更なし
		}
	}
}

void CDecodeUtil::CheckNIT(WORD PID, const Desc::CDescriptor& nit)
{
	if( epgDBUtil != NULL ){
		epgDBUtil->AddServiceListNIT(nit);
	}

	if( nit.GetNumber(Desc::table_id) == 0x40 ){
		//自ネットワーク
		BYTE section_number = (BYTE)nit.GetNumber(Desc::section_number);
		if( this->nitActualInfo.empty() ){
			//初回
			this->nitActualInfo[section_number] = nit;
		}else{
			if( this->nitActualInfo.begin()->second.GetNumber(Desc::network_id) != nit.GetNumber(Desc::network_id) ){
				//NID変わったのでネットワーク変わった
				ChangeTSIDClear(PID);
				this->nitActualInfo[section_number] = nit;
			}else if( this->nitActualInfo.begin()->second.GetNumber(Desc::version_number) != nit.GetNumber(Desc::version_number) ){
				//バージョン変わった
				this->nitActualInfo.clear();
				this->nitActualInfo[section_number] = nit;
			}else{
				map<BYTE, Desc::CDescriptor>::const_iterator itr = this->nitActualInfo.find(0);
				if( itr != this->nitActualInfo.end() ){
					Desc::CDescriptor::CLoopPointer lpLast, lp;
					if( itr->second.EnterLoop(lpLast, 1) && nit.EnterLoop(lp, 1) && itr->first == section_number ){
						if( itr->second.GetNumber(Desc::original_network_id, lpLast) != nit.GetNumber(Desc::original_network_id, lp) ){
							//ONID変わったのでネットワーク変わった
							ChangeTSIDClear(PID);
							this->nitActualInfo[section_number] = nit;
						}else{
							if( itr->second.GetNumber(Desc::transport_stream_id, lpLast) != nit.GetNumber(Desc::transport_stream_id, lp) ){
								//TSID変わったのでネットワーク変わった
								ChangeTSIDClear(PID);
								this->nitActualInfo[section_number] = nit;
							}else{
								//変化なし
								if( this->nitActualInfo.count(section_number) == 0 ){
									this->nitActualInfo[section_number] = nit;
								}
							}
						}
					}else{
						//変化なし
						if( this->nitActualInfo.count(section_number) == 0 ){
							this->nitActualInfo[section_number] = nit;
						}
					}
				}else{
					//変化なし
					if( this->nitActualInfo.count(section_number) == 0 ){
						this->nitActualInfo[section_number] = nit;
					}
				}
			}
		}
	}else if( nit.GetNumber(Desc::table_id) == 0x41 ){
		//他ネットワーク
		//特に扱う必要性なし
	}
}

void CDecodeUtil::CheckSDT(WORD PID, const Desc::CDescriptor& sdt)
{
	if( epgDBUtil != NULL ){
		epgDBUtil->AddSDT(sdt);
	}

	if( sdt.GetNumber(Desc::table_id) == 0x42 ){
		//自ストリーム
		BYTE section_number = (BYTE)sdt.GetNumber(Desc::section_number);
		if( this->sdtActualInfo.empty() ){
			//初回
			this->sdtActualInfo[section_number] = sdt;
		}else{
			if( this->sdtActualInfo.begin()->second.GetNumber(Desc::original_network_id) != sdt.GetNumber(Desc::original_network_id) ){
				//ONID変わったのでネットワーク変わった
				ChangeTSIDClear(PID);
				this->sdtActualInfo[section_number] = sdt;
			}else if( this->sdtActualInfo.begin()->second.GetNumber(Desc::transport_stream_id) != sdt.GetNumber(Desc::transport_stream_id) ){
				//TSID変わったのでチャンネル変わった
				ChangeTSIDClear(PID);
				this->sdtActualInfo[section_number] = sdt;
			}else if( this->sdtActualInfo.begin()->second.GetNumber(Desc::version_number) != sdt.GetNumber(Desc::version_number) ){
				//バージョン変わった
				this->sdtActualInfo.clear();
				this->sdtActualInfo[section_number] = sdt;
			}else{
				//変化なし
				if( this->sdtActualInfo.count(section_number) == 0 ){
					this->sdtActualInfo[section_number] = sdt;
				}
			}
		}
	}else if( sdt.GetNumber(Desc::table_id) == 0x46 ){
		//他ストリーム
		//特に扱う必要性なし
	}
}

void CDecodeUtil::CheckTDT(const Desc::CDescriptor& tdt)
{
	__int64 time = MJDtoI64Time(tdt.GetNumber(Desc::jst_time_mjd), tdt.GetNumber(Desc::jst_time_bcd));
	if( tdt.GetNumber(Desc::table_id) == 0x73 ){
		//TOT
		this->totTime = time;
		this->totTimeTick = GetTickCount();
	}else{
		this->tdtTime = time;
		this->tdtTimeTick = GetTickCount();
	}
}

void CDecodeUtil::CheckEIT(WORD PID, const Desc::CDescriptor& eit)
{
	if( epgDBUtil != NULL ){
		__int64 time = 0;
		GetNowTime(&time);
		epgDBUtil->AddEIT(PID, eit, time);
	}
}

void CDecodeUtil::CheckBIT(WORD PID, const Desc::CDescriptor& bit)
{
	if( this->bitInfo == NULL ){
		//初回
		this->bitInfo.reset(new Desc::CDescriptor(bit));
	}else{
		if( this->bitInfo->GetNumber(Desc::original_network_id) != bit.GetNumber(Desc::original_network_id) ){
			//ONID変わったのでネットワーク変わった
			ChangeTSIDClear(PID);
			this->bitInfo.reset(new Desc::CDescriptor(bit));
		}else if( this->bitInfo->GetNumber(Desc::version_number) != bit.GetNumber(Desc::version_number) ){
			//バージョン変わった
			this->bitInfo.reset(new Desc::CDescriptor(bit));
		}else{
			//変化なし
		}
	}
}

void CDecodeUtil::CheckSIT(const Desc::CDescriptor& sit)
{
	//時間計算
	Desc::CDescriptor::CLoopPointer lp;
	if( this->totTime == 0 && this->tdtTime == 0 && sit.EnterLoop(lp) ){
		for( DWORD i = 0; sit.SetLoopIndex(lp, i); i++ ){
			if( sit.GetNumber(Desc::descriptor_tag, lp) == Desc::partialTS_time_descriptor ){
				if( sit.GetNumber(Desc::jst_time_flag, lp) == 1 ){
					this->sitTime = MJDtoI64Time(sit.GetNumber(Desc::jst_time_mjd), sit.GetNumber(Desc::jst_time_bcd));
					this->sitTimeTick = GetTickCount();
				}
			}
		}
	}

	if( epgDBUtil != NULL ){
		if( this->patInfo != NULL ){
			epgDBUtil->AddServiceListSIT((WORD)this->patInfo->GetNumber(Desc::transport_stream_id), sit);
		}
	}

	if( this->sitInfo == NULL ){
		//初回
		this->sitInfo.reset(new Desc::CDescriptor(sit));
	}else{
		if( this->sitInfo->GetNumber(Desc::version_number) != sit.GetNumber(Desc::version_number) ){
			//バージョン変わった
			this->sitInfo.reset(new Desc::CDescriptor(sit));
		}else{
			//変化なし
		}
	}
}

//解析データの現在のストリームＩＤを取得する
// originalNetworkID		[OUT]現在のoriginalNetworkID
// transportStreamID		[OUT]現在のtransportStreamID
BOOL CDecodeUtil::GetTSID(
	WORD* originalNetworkID,
	WORD* transportStreamID
	)
{
	if( this->sdtActualInfo.empty() == false ){
		*originalNetworkID = (WORD)this->sdtActualInfo.begin()->second.GetNumber(Desc::original_network_id);
		*transportStreamID = (WORD)this->sdtActualInfo.begin()->second.GetNumber(Desc::transport_stream_id);
		return TRUE;
	}else if( this->sitInfo != NULL && this->patInfo != NULL ){
		//TSID
		*transportStreamID = (WORD)this->patInfo->GetNumber(Desc::transport_stream_id);
		//ONID
		Desc::CDescriptor::CLoopPointer lp;
		if( this->sitInfo->EnterLoop(lp) ){
			for( DWORD i = 0; this->sitInfo->SetLoopIndex(lp, i); i++ ){
				if( this->sitInfo->GetNumber(Desc::descriptor_tag, lp) == Desc::network_identification_descriptor ){
					*originalNetworkID = (WORD)this->sitInfo->GetNumber(Desc::network_id, lp);
					return TRUE;
				}
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
	if( this->nitActualInfo.empty() || this->sdtActualInfo.empty() ){
		return GetServiceListSIT(serviceListSize, serviceList_);
	}else{
		if( this->nitActualInfo.begin()->second.GetNumber(Desc::last_section_number) + 1 != this->nitActualInfo.size() ||
		    this->sdtActualInfo.begin()->second.GetNumber(Desc::last_section_number) + 1 != this->sdtActualInfo.size() ){
			return FALSE;
		}
	}
	*serviceListSize = 0;

	for( auto itr = this->sdtActualInfo.cbegin(); itr != this->sdtActualInfo.end(); itr++ ){
		Desc::CDescriptor::CLoopPointer lp;
		if( itr->second.EnterLoop(lp) ){
			*serviceListSize += itr->second.GetLoopSize(lp);
		}
	}
	this->serviceList.reset(new SERVICE_INFO[*serviceListSize]);
	this->serviceDBList.reset(new EPGDB_SERVICE_INFO[*serviceListSize]);
	this->serviceAdapterList.reset(new CServiceInfoAdapter[*serviceListSize]);


	wstring network_nameW = L"";
	wstring ts_nameW = L"";
	BYTE remote_control_key_id = 0;
	vector<WORD> partialServiceList;

	for( auto itr = this->nitActualInfo.cbegin(); itr != this->nitActualInfo.end(); itr++ ){
		Desc::CDescriptor::CLoopPointer lp;
		if( itr->second.EnterLoop(lp) ){
			for( DWORD i = 0; itr->second.SetLoopIndex(lp, i); i++ ){
				if( itr->second.GetNumber(Desc::descriptor_tag, lp) != Desc::network_name_descriptor ){
					continue;
				}
				DWORD srcSize;
				const BYTE* src = itr->second.GetBinary(Desc::d_char, &srcSize, lp);
				if( src && srcSize > 0 ){
					CARIB8CharDecode arib;
					arib.PSISI(src, srcSize, &network_nameW);
				}
			}
		}
		lp = Desc::CDescriptor::CLoopPointer();
		if( itr->second.EnterLoop(lp, 1) == false ){
			continue;
		}
		for( DWORD i = 0; itr->second.SetLoopIndex(lp, i); i++ ){
			Desc::CDescriptor::CLoopPointer lp2 = lp;
			if( itr->second.EnterLoop(lp2) == false ){
				continue;
			}
			for( DWORD j = 0; itr->second.SetLoopIndex(lp2, j); j++ ){
				if( itr->second.GetNumber(Desc::descriptor_tag, lp2) == Desc::ts_information_descriptor ){
					DWORD srcSize;
					const BYTE* src = itr->second.GetBinary(Desc::ts_name_char, &srcSize, lp2);
					if( src && srcSize > 0 ){
						CARIB8CharDecode arib;
						arib.PSISI(src, srcSize, &ts_nameW);
					}
					remote_control_key_id = (BYTE)itr->second.GetNumber(Desc::remote_control_key_id, lp2);
				}
				if( itr->second.GetNumber(Desc::descriptor_tag, lp2) == Desc::partial_reception_descriptor ){
					partialServiceList.clear();
					Desc::CDescriptor::CLoopPointer lp3 = lp2;
					if( itr->second.EnterLoop(lp3) ){
						for( DWORD k=0; itr->second.SetLoopIndex(lp3, k); k++ ){
							partialServiceList.push_back((WORD)itr->second.GetNumber(Desc::service_id, lp3));
						}
					}
				}
			}
		}
	}

	DWORD count = 0;
	for( auto itr = this->sdtActualInfo.cbegin(); itr != this->sdtActualInfo.end(); itr++ ){
		Desc::CDescriptor::CLoopPointer lp;
		if( itr->second.EnterLoop(lp) == false ){
			continue;
		}
		for( DWORD i = 0; itr->second.SetLoopIndex(lp, i); i++ ){
			this->serviceDBList[count].ONID = (WORD)itr->second.GetNumber(Desc::original_network_id);
			this->serviceDBList[count].TSID = (WORD)itr->second.GetNumber(Desc::transport_stream_id);
			this->serviceDBList[count].SID = (WORD)itr->second.GetNumber(Desc::service_id, lp);
			this->serviceDBList[count].service_type = 0;

			Desc::CDescriptor::CLoopPointer lp2 = lp;
			if( itr->second.EnterLoop(lp2) ){
				for( DWORD j = 0; itr->second.SetLoopIndex(lp2, j); j++ ){
					if( itr->second.GetNumber(Desc::descriptor_tag, lp2) != Desc::service_descriptor ){
						continue;
					}
					CARIB8CharDecode arib;
					wstring service_provider_name;
					wstring service_name;
					const BYTE* src;
					DWORD srcSize;
					src = itr->second.GetBinary(Desc::service_provider_name, &srcSize, lp2);
					if( src && srcSize > 0 ){
						arib.PSISI(src, srcSize, &service_provider_name);
					}
					src = itr->second.GetBinary(Desc::service_name, &srcSize, lp2);
					if( src && srcSize > 0 ){
						arib.PSISI(src, srcSize, &service_name);
					}
					this->serviceDBList[count].service_type = (BYTE)itr->second.GetNumber(Desc::service_type, lp2);
					this->serviceDBList[count].service_provider_name.swap(service_provider_name);
					this->serviceDBList[count].service_name.swap(service_name);
				}
			}

			this->serviceDBList[count].network_name = network_nameW;
			this->serviceDBList[count].ts_name = ts_nameW;
			this->serviceDBList[count].remote_control_key_id = remote_control_key_id;

			this->serviceDBList[count].partialReceptionFlag = FALSE;
			for( size_t j=0; j<partialServiceList.size(); j++ ){
				if( partialServiceList[j] == this->serviceDBList[count].SID ){
					this->serviceDBList[count].partialReceptionFlag = TRUE;
				}
			}

			this->serviceList[count] = this->serviceAdapterList[count].Create(&this->serviceDBList[count]);
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
	*serviceListSize = 0;
	Desc::CDescriptor::CLoopPointer lp;
	if( this->sitInfo->EnterLoop(lp) ){
		for( DWORD i = 0; this->sitInfo->SetLoopIndex(lp, i); i++ ){
			if( this->sitInfo->GetNumber(Desc::descriptor_tag, lp) == Desc::network_identification_descriptor ){
				ONID = (WORD)this->sitInfo->GetNumber(Desc::network_id);
			}
		}
		*serviceListSize = this->sitInfo->GetLoopSize(lp);
	}

	//TSID
	WORD TSID = 0xFFFF;
	TSID = (WORD)this->patInfo->GetNumber(Desc::transport_stream_id);

	this->serviceList.reset(new SERVICE_INFO[*serviceListSize]);
	this->serviceDBList.reset(new EPGDB_SERVICE_INFO[*serviceListSize]);
	this->serviceAdapterList.reset(new CServiceInfoAdapter[*serviceListSize]);

	//サービスリスト
	for( DWORD i=0; i<*serviceListSize; i++ ){
		this->sitInfo->SetLoopIndex(lp, i);
		this->serviceDBList[i].ONID = ONID;
		this->serviceDBList[i].TSID = TSID;
		this->serviceDBList[i].SID = (WORD)this->sitInfo->GetNumber(Desc::service_id, lp);
		this->serviceDBList[i].service_type = 0;

		Desc::CDescriptor::CLoopPointer lp2 = lp;
		if( this->sitInfo->EnterLoop(lp2) ){
			for( DWORD j = 0; this->sitInfo->SetLoopIndex(lp2, j); j++ ){
				if( this->sitInfo->GetNumber(Desc::descriptor_tag, lp2) != Desc::service_descriptor ){
					continue;
				}
				CARIB8CharDecode arib;
				wstring service_provider_name;
				wstring service_name;
				const BYTE* src;
				DWORD srcSize;
				src = this->sitInfo->GetBinary(Desc::service_provider_name, &srcSize, lp2);
				if( src && srcSize > 0 ){
					arib.PSISI(src, srcSize, &service_provider_name);
				}
				src = this->sitInfo->GetBinary(Desc::service_name, &srcSize, lp2);
				if( src && srcSize > 0 ){
					arib.PSISI(src, srcSize, &service_name);
				}
				this->serviceDBList[i].service_type = (BYTE)this->sitInfo->GetNumber(Desc::service_type, lp2);
				this->serviceDBList[i].service_provider_name.swap(service_provider_name);
				this->serviceDBList[i].service_name.swap(service_name);
			}
		}

		//トランスポートの情報は取得できない
		this->serviceDBList[i].remote_control_key_id = 0;

		this->serviceDBList[i].partialReceptionFlag = FALSE;
		this->serviceList[i] = this->serviceAdapterList[i].Create(&this->serviceDBList[i]);
	}


	*serviceList_ = this->serviceList.get();

	return TRUE;
}

//ストリーム内の現在の時間情報を取得する
//引数：
// time				[OUT]ストリーム内の現在の時間
// tick				[OUT]timeを取得した時点のチックカウント
BOOL CDecodeUtil::GetNowTime(
	__int64* time,
	DWORD* tick
	)
{
	DWORD tick_;
	if( tick == NULL ){
		tick = &tick_;
	}
	if( this->totTime != 0 ){
		*time = this->totTime;
		*tick = this->totTimeTick;
		return TRUE;
	}else if( this->tdtTime != 0 ){
		*time = this->tdtTime;
		*tick = this->tdtTimeTick;
		return TRUE;
	}else if( this->sitTime != 0 ){
		*time = this->sitTime;
		*tick = this->sitTimeTick;
		return TRUE;
	}
	return FALSE;
}
