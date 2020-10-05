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
		do{
			if( table.GetNumber(Desc::descriptor_tag, lp) == 0x82 && table.GetNumber(Desc::reserved, lp) == 1 ){
				//日本語版？ネットワーク記述子にキャスト
				table.SetNumber(Desc::descriptor_tag, Desc::network_name_descriptor, lp);
			}
		}while( table.NextLoopIndex(lp) );
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
		do{
			Desc::CDescriptor::CLoopPointer lp0x82, lp2 = lp;
			if( table.EnterLoop(lp2) ){
				bool found0x82 = false;
				DWORD service_type = 0;
				do{
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
				}while( table.NextLoopIndex(lp2) );
				if( found0x82 ){
					//サービス記述子にキャスト
					table.SetNumber(Desc::descriptor_tag, Desc::service_descriptor, lp0x82);
					table.SetNumber(Desc::service_type, service_type == 0x81 ? 0xA1 : service_type, lp0x82);
				}
			}
		}while( table.NextLoopIndex(lp) );
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
		do{
			Desc::CDescriptor::CLoopPointer lp0x82, lp2 = lp;
			if( table.EnterLoop(lp2) ){
				bool found0x82 = false;
				do{
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
				}while( table.NextLoopIndex(lp2) );
				if( found0x82 ){
					//短形式イベント記述子にキャスト
					table.SetNumber(Desc::descriptor_tag, Desc::short_event_descriptor, lp0x82);
				}
			}
		}while( table.NextLoopIndex(lp) );
	}
	return true;
}

#endif //SUPPORT_SKY_SD

static const struct {
	BYTE type;
	const char* name;
} DOWNLOAD_LOGO_TYPE_NAMES[] = {
	{0, "LOGO-00"},
	{1, "LOGO-01"},
	{2, "LOGO-02"},
	{3, "LOGO-03"},
	{4, "LOGO-04"},
	{5, "LOGO-05"},
	{0, "CS_LOGO-00"},
	{1, "CS_LOGO-01"},
	{2, "CS_LOGO-02"},
	{3, "CS_LOGO-03"},
	{4, "CS_LOGO-04"},
	{5, "CS_LOGO-05"},
};

CDecodeUtil::CDecodeUtil(void)
{
	this->epgDBUtil = NULL;

	this->totTime = 0;
	this->tdtTime = 0;
	this->sitTime = 0;
	this->logoTypeFlags = 0;
}

void CDecodeUtil::SetEpgDB(CEpgDBUtil* epgDBUtil_)
{
	this->epgDBUtil = epgDBUtil_;
}

void CDecodeUtil::ClearBuff(WORD noClearPid)
{
	this->buffUtilMap.erase(upper_bound_first(this->buffUtilMap.begin(), this->buffUtilMap.end(), noClearPid),
	                        this->buffUtilMap.end());
	this->buffUtilMap.erase(this->buffUtilMap.begin(),
	                        lower_bound_first(this->buffUtilMap.begin(), this->buffUtilMap.end(), noClearPid));
}

void CDecodeUtil::ChangeTSIDClear(WORD noClearPid)
{
	ClearBuff(noClearPid);

	this->patInfo.reset();
	this->engineeringPmtMap.clear();

	this->nitActualInfo.clear();
	this->sdtActualInfo.clear();

	this->bitInfo.reset();
	this->sitInfo.reset();

	this->totTime = 0;
	this->tdtTime = 0;
	this->sitTime = 0;
	this->logoMap.clear();
	this->downloadModuleList.clear();

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
			vector<pair<WORD, CTSBuffUtil>>::iterator itr =
				lower_bound_first(this->buffUtilMap.begin(), this->buffUtilMap.end(), tsPacket.PID);
			if( itr == this->buffUtilMap.end() || itr->first != tsPacket.PID ){
				//まだPIDがないので新規
				itr = this->buffUtilMap.insert(itr, std::make_pair(tsPacket.PID, CTSBuffUtil()));
			}
			if( itr->second.Add188TS(tsPacket) == TRUE ){
				BYTE* section = NULL;
				DWORD sectionSize = 0;
				while( itr->second.GetSectionBuff(&section, &sectionSize) == TRUE ){
					switch( section[0] ){
					case 0x00:
						if( this->tableBuff.DecodeSI(section, sectionSize, NULL, Desc::TYPE_PAT) ){
							CheckPAT(tsPacket.PID, this->tableBuff);
						}
						break;
					case 0x02:
						//ロゴを取得するときだけ
						if( this->logoTypeFlags &&
						    this->tableBuff.DecodeSI(section, sectionSize, NULL, Desc::TYPE_PMT) ){
							CheckPMT(this->tableBuff);
						}
						break;
					case 0x3B:
					case 0x3C:
						//ロゴを取得するときだけ
						if( this->logoTypeFlags &&
						    this->tableBuff.DecodeSI(section, sectionSize, NULL, Desc::TYPE_DSMCC_HEAD) &&
						    sectionSize >= 12 ){
							CheckDsmcc(tsPacket.PID, this->tableBuff, section + 8, sectionSize - 12);
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
					case 0xC8:
						//ロゴを取得するときだけ
						if( this->logoTypeFlags &&
						    this->tableBuff.DecodeSI(section, sectionSize, NULL, Desc::TYPE_CDT) ){
							CheckCDT(this->tableBuff);
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
					//ChangeTSIDClear()によってイテレータが無効になるかもしれない
					itr = lower_bound_first(this->buffUtilMap.begin(), this->buffUtilMap.end(), tsPacket.PID);
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

void CDecodeUtil::CheckPMT(const Desc::CDescriptor& pmt)
{
	WORD pnum = (WORD)pmt.GetNumber(Desc::program_number);
	auto itr = lower_bound_first(this->engineeringPmtMap.begin(), this->engineeringPmtMap.end(), pnum);
	if( itr != this->engineeringPmtMap.end() && itr->first == pnum &&
	    (itr->second == NULL ||
	     itr->second->GetNumber(Desc::version_number) != pmt.GetNumber(Desc::version_number)) ){
		//運用規則によりマルチセクションでない
		itr->second.reset(new Desc::CDescriptor(pmt));
	}
}

void CDecodeUtil::CheckDsmcc(WORD PID, const Desc::CDescriptor& dsmccHead, const BYTE* data, size_t dataSize)
{
	if( dataSize < 12 ){
		return;
	}
	BYTE protocolDiscriminator = data[0];
	BYTE dsmccType = data[1];
	WORD messageID = data[2] << 8 | data[3];
	size_t adaptationLength = data[9];
	size_t messageLength = data[10] << 8 | data[11];
	if( protocolDiscriminator == 0x11 && dsmccType == 0x03 &&
	    (messageID == 0x1002 || messageID == 0x1003) &&
	    dataSize >= 12 + messageLength && messageLength >= adaptationLength ){
		//MPEG-2 DSM-CC, STD-B24 DII or DDB (データカルーセル)
		const BYTE* body = data + 12 + adaptationLength;
		size_t bodySize = messageLength - adaptationLength;
		if( messageID == 0x1002 ){
			CheckDsmccDII(PID, body, bodySize);
		}else{
			DWORD downloadID = (DWORD)data[4] << 24 | data[5] << 16 | data[6] << 8 | data[7];
			CheckDsmccDDB(PID, downloadID, body, bodySize);
		}
	}
}

void CDecodeUtil::CheckDsmccDII(WORD PID, const BYTE* body, size_t bodySize)
{
	if( bodySize < 18 ){
		return;
	}
	DWORD downloadID = (DWORD)body[0] << 24 | body[1] << 16 | body[2] << 8 | body[3];
	size_t blockSize = body[4] << 8 | body[5];
	size_t compatDescLength = body[16] << 8 | body[17];
	if( blockSize > 0 && bodySize >= compatDescLength + 20 ){
		size_t numberOfModules = body[compatDescLength + 18] << 8 | body[compatDescLength + 19];
		const BYTE* modules = body + compatDescLength + 20;
		size_t modulesSize = bodySize - compatDescLength - 20;
		for( size_t i = 0; numberOfModules > 0 && i + 8 <= modulesSize; numberOfModules-- ){
			WORD moduleID = modules[i] << 8 | modules[i + 1];
			size_t moduleSize = (DWORD)modules[i + 2] << 24 | modules[i + 3] << 16 | modules[i + 4] << 8 | modules[i + 5];
			BYTE moduleVersion = modules[i + 6];
			size_t moduleInfoLength = modules[i + 7];
			i += 8;
			const char* moduleName = NULL;
			if( i + moduleInfoLength <= modulesSize ){
				for( size_t j = 0; j + 1 < moduleInfoLength; ){
					size_t len = modules[i + j + 1];
					if( modules[i + j] == 0x02 && j + 2 + len <= moduleInfoLength ){
						//DII Name記述子
						const BYTE* name = modules + i + j + 2;
						for( size_t k = 0; k < array_size(DOWNLOAD_LOGO_TYPE_NAMES); k++ ){
							if( (this->logoTypeFlags & (1 << DOWNLOAD_LOGO_TYPE_NAMES[k].type)) &&
							    len == strlen(DOWNLOAD_LOGO_TYPE_NAMES[k].name) &&
							    memcmp(name, DOWNLOAD_LOGO_TYPE_NAMES[k].name, len) == 0 ){
								//ダウンロード対象
								moduleName = DOWNLOAD_LOGO_TYPE_NAMES[k].name;
								break;
							}
						}
						break;
					}
					j += 2 + len;
				}
			}
			//ロゴデータの最大見積もりから現実的なサイズに制限
			if( moduleName && moduleSize > 0 && moduleSize < 4 * 1024 * 1024 ){
				vector<DOWNLOAD_MODULE_DATA>::iterator itr =
					std::find_if(this->downloadModuleList.begin(), this->downloadModuleList.end(),
					             [=](const DOWNLOAD_MODULE_DATA& a) { return a.name == moduleName; });
				if( itr == this->downloadModuleList.end() ){
					this->downloadModuleList.resize(this->downloadModuleList.size() + 1);
					itr = this->downloadModuleList.end() - 1;
					itr->name = moduleName;
				}
				size_t blockNum = (moduleSize + blockSize - 1) / blockSize;
				if( itr->moduleData.size() != moduleSize ||
				    itr->blockGetList.size() != blockNum ||
				    itr->downloadID != downloadID ||
				    itr->pid != PID ||
				    itr->moduleID != moduleID ||
				    itr->moduleVersion != moduleVersion ){
					//ブロック取得状況をリセット
					itr->moduleData.assign(moduleSize, 0);
					itr->blockGetList.assign(blockNum, 0);
					itr->downloadID = downloadID;
					itr->pid = PID;
					itr->moduleID = moduleID;
					itr->moduleVersion = moduleVersion;
				}
			}
			i += moduleInfoLength;
		}
	}
}

void CDecodeUtil::CheckDsmccDDB(WORD PID, DWORD downloadID, const BYTE* body, size_t bodySize)
{
	if( bodySize < 6 ){
		return;
	}
	WORD moduleID = body[0] << 8 | body[1];
	BYTE moduleVersion = body[2];
	size_t blockNumber = body[4] << 8 | body[5];
	for( vector<DOWNLOAD_MODULE_DATA>::iterator itr = this->downloadModuleList.begin(); itr != this->downloadModuleList.end(); itr++ ){
		if( itr->downloadID == downloadID &&
		    itr->pid == PID &&
		    itr->moduleID == moduleID &&
		    itr->moduleVersion == moduleVersion &&
		    itr->blockGetList.size() > blockNumber &&
		    itr->blockGetList[blockNumber] == 0 ){
			//ブロック取得
			size_t blockSize = bodySize - 6;
			if( blockNumber == itr->blockGetList.size() - 1 ){
				//最終ブロック
				if( itr->moduleData.size() >= blockSize ){
					std::copy(body + 6, body + bodySize, itr->moduleData.end() - blockSize);
					itr->blockGetList[blockNumber] = 1;
				}
			}else{
				//最終ブロック以外
				if( itr->moduleData.size() >= blockSize * blockNumber + blockSize ){
					std::copy(body + 6, body + bodySize, itr->moduleData.begin() + blockSize * blockNumber);
					itr->blockGetList[blockNumber] = 1;
				}
			}
			if( std::find(itr->blockGetList.begin(), itr->blockGetList.end(), 0) == itr->blockGetList.end() ){
				//すべて取得した
				CheckDownloadedModule(*itr);
			}
		}
	}
}

void CDecodeUtil::CheckDownloadedModule(const DOWNLOAD_MODULE_DATA& dl)
{
	//STD-B21 LogoDataModuleのダウンロードだけなので、nameの確認は省略
	if( dl.moduleData.size() < 3 ){
		return;
	}
	BYTE type = dl.moduleData[0];
	size_t numberOfLoop = dl.moduleData[1] << 8 | dl.moduleData[2];
	for( size_t i = 3; numberOfLoop > 0 && i + 3 <= dl.moduleData.size(); numberOfLoop-- ){
		WORD id = (dl.moduleData[i] << 8 | dl.moduleData[i + 1]) & 0x1FF;
		size_t numberOfServices = dl.moduleData[i + 2];
		i += 3;
		if( i + numberOfServices * 6 + 2 > dl.moduleData.size() ){
			break;
		}
		size_t logoSize = dl.moduleData[i + numberOfServices * 6] << 8 |
		                  dl.moduleData[i + numberOfServices * 6 + 1];
		const BYTE* logo = dl.moduleData.data() + i + numberOfServices * 6 + 2;
		if( i + numberOfServices * 6 + 2 + logoSize > dl.moduleData.size() ){
			break;
		}
		for( ; numberOfServices > 0; numberOfServices-- ){
			WORD onid = dl.moduleData[i] << 8 | dl.moduleData[i + 1];
			WORD sid = dl.moduleData[i + 4] << 8 | dl.moduleData[i + 5];
			UpdateLogoData(onid, id, type, logo, logoSize);
			LONGLONG key = (LONGLONG)onid << 32 | (LONGLONG)id << 16 | type;
			vector<LOGO_DATA>::iterator itr = lower_bound_first(this->logoMap.begin(), this->logoMap.end(), key);
			if( itr != this->logoMap.end() && itr->first == key &&
			    std::find(itr->serviceList.begin(), itr->serviceList.end(), sid) == itr->serviceList.end() ){
				itr->serviceList.push_back(sid);
			}
			i += 6;
		}
		i += 2 + logoSize;
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
			UpdateEngineeringPmtMap();
		}else{
			if( this->nitActualInfo.begin()->second.GetNumber(Desc::network_id) != nit.GetNumber(Desc::network_id) ){
				//NID変わったのでネットワーク変わった
				ChangeTSIDClear(PID);
				this->nitActualInfo[section_number] = nit;
				UpdateEngineeringPmtMap();
			}else if( this->nitActualInfo.begin()->second.GetNumber(Desc::version_number) != nit.GetNumber(Desc::version_number) ){
				//バージョン変わった
				this->nitActualInfo.clear();
				this->nitActualInfo[section_number] = nit;
				UpdateEngineeringPmtMap();
			}else{
				map<BYTE, Desc::CDescriptor>::const_iterator itr = this->nitActualInfo.find(0);
				Desc::CDescriptor::CLoopPointer lpLast, lp;
				if( itr != this->nitActualInfo.end() &&
				    itr->first == section_number &&
				    itr->second.EnterLoop(lpLast, 1) && nit.EnterLoop(lp, 1) &&
				    (itr->second.GetNumber(Desc::original_network_id, lpLast) != nit.GetNumber(Desc::original_network_id, lp) ||
				     itr->second.GetNumber(Desc::transport_stream_id, lpLast) != nit.GetNumber(Desc::transport_stream_id, lp)) ){
					//ONID変わったのでネットワーク変わった
					//TSID変わったのでネットワーク変わった
					ChangeTSIDClear(PID);
					this->nitActualInfo[section_number] = nit;
					UpdateEngineeringPmtMap();
				}else{
					//変化なし
					if( this->nitActualInfo.count(section_number) == 0 ){
						this->nitActualInfo[section_number] = nit;
						UpdateEngineeringPmtMap();
					}
				}
			}
		}
	}else if( nit.GetNumber(Desc::table_id) == 0x41 ){
		//他ネットワーク
		//特に扱う必要性なし
	}
}

void CDecodeUtil::UpdateEngineeringPmtMap()
{
	if( this->nitActualInfo.empty() ||
	    this->nitActualInfo.begin()->second.GetNumber(Desc::last_section_number) + 1 != this->nitActualInfo.size() ){
		//セクション不足
		return;
	}

	//エンジニアリングサービスを探す
	vector<WORD> engList;
	for( auto itr = this->nitActualInfo.cbegin(); itr != this->nitActualInfo.end(); itr++ ){
		Desc::CDescriptor::CLoopPointer lp;
		if( itr->second.EnterLoop(lp, 1) ){
			do{
				Desc::CDescriptor::CLoopPointer lp2 = lp;
				if( itr->second.EnterLoop(lp2) ){
					do{
						Desc::CDescriptor::CLoopPointer lp3 = lp2;
						if( itr->second.GetNumber(Desc::descriptor_tag, lp2) == Desc::service_list_descriptor &&
						    itr->second.EnterLoop(lp3) ){
							do{
								//STD-B10,TR-B15
								if( itr->second.GetNumber(Desc::service_type, lp3) == 0xA4 ){
									engList.push_back((WORD)itr->second.GetNumber(Desc::service_id, lp3));
									if( engList.back() != 0 ){
										auto jtr = lower_bound_first(this->engineeringPmtMap.begin(), this->engineeringPmtMap.end(), engList.back());
										if( jtr == this->engineeringPmtMap.end() || jtr->first != engList.back() ){
											this->engineeringPmtMap.insert(jtr, std::make_pair(engList.back(), std::unique_ptr<const Desc::CDescriptor>()));
										}
									}
								}
							}while( itr->second.NextLoopIndex(lp3) );
						}
					}while( itr->second.NextLoopIndex(lp2) );
				}
			}while( itr->second.NextLoopIndex(lp) );
		}
	}

	for( auto itr = this->engineeringPmtMap.begin(); itr != this->engineeringPmtMap.end(); ){
		if( std::find(engList.begin(), engList.end(), itr->first) == engList.end() ){
			itr = this->engineeringPmtMap.erase(itr);
		}else{
			itr++;
		}
	}
}

void CDecodeUtil::CheckSDT(WORD PID, const Desc::CDescriptor& sdt)
{
	if( epgDBUtil != NULL ){
		epgDBUtil->AddSDT(sdt);
	}
	//ロゴを取得するときだけ
	if( this->logoTypeFlags ){
		UpdateLogoServiceList(sdt);
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
		do{
			if( sit.GetNumber(Desc::descriptor_tag, lp) == Desc::partialTS_time_descriptor ){
				if( sit.GetNumber(Desc::jst_time_flag, lp) == 1 ){
					this->sitTime = MJDtoI64Time(sit.GetNumber(Desc::jst_time_mjd), sit.GetNumber(Desc::jst_time_bcd));
					this->sitTimeTick = GetTickCount();
				}
			}
		}while( sit.NextLoopIndex(lp) );
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

void CDecodeUtil::CheckCDT(const Desc::CDescriptor& cdt)
{
	if( cdt.GetNumber(Desc::data_type) == 1 ){
		//ロゴデータ
		DWORD dataModuleSize;
		const BYTE* dataModule = cdt.GetBinary(Desc::data_module_byte, &dataModuleSize);
		if( dataModule && dataModuleSize >= 7 ){
			BYTE type = dataModule[0];
			WORD id = (dataModule[1] << 8 | dataModule[2]) & 0x1FF;
			//ここでバージョンも取得できるが省略
			size_t logoSize = dataModule[5] << 8 | dataModule[6];
			if( dataModuleSize >= 7 + logoSize ){
				UpdateLogoData((WORD)cdt.GetNumber(Desc::original_network_id), id, type, dataModule + 7, logoSize);
			}
		}
	}
}

void CDecodeUtil::UpdateLogoData(WORD onid, WORD id, BYTE type, const BYTE* logo, size_t logoSize)
{
	if( type <= 5 && (this->logoTypeFlags & (1 << type)) &&
	    logoSize >= 33 &&
	    memcmp(logo, "\x89PNG\r\n\x1a\n\0\0\0\x0dIHDR", 16) == 0 &&
	    (logo[24] == 8 || logo[25] != 3) ){
		//ロゴタイプ0～5、パレット指定のとき必ずビット深度8、のPNGデータ
		LONGLONG key = (LONGLONG)onid << 32 | (LONGLONG)id << 16 | type;
		vector<LOGO_DATA>::iterator itr = lower_bound_first(this->logoMap.begin(), this->logoMap.end(), key);
		if( itr == this->logoMap.end() || itr->first != key ){
			itr = this->logoMap.insert(itr, LOGO_DATA());
			itr->first = key;
			bool insertPalette = false;
			if( logo[25] == 3 ){
				//パレット指定
				insertPalette = true;
				for( size_t i = 33; i + 7 < logoSize; ){
					if( memcmp(logo + i + 4, "PLTE", 4) == 0 ){
						insertPalette = false;
						break;
					}
					i += (logo[i + 2] << 8 | logo[i + 3]) + 12;
				}
				if( insertPalette ){
					//パレットがないので挿入
					itr->data.reserve(logoSize + 12 + 12 + 4 * array_size(CARIB8CharDecode::DefClut));
					itr->data.assign(logo, logo + 33);
					AppendPngPalette(itr->data);
					itr->data.insert(itr->data.end(), logo + 33, logo + logoSize);
				}
			}
			if( insertPalette == false ){
				itr->data.assign(logo, logo + logoSize);
			}
		}
	}
}

void CDecodeUtil::UpdateLogoServiceList(const Desc::CDescriptor& sdt)
{
	//サービスからロゴへのポインティングを調べる
	Desc::CDescriptor::CLoopPointer lp;
	if( sdt.EnterLoop(lp) ){
		DWORD onid = sdt.GetNumber(Desc::original_network_id);
		do{
			Desc::CDescriptor::CLoopPointer lp2 = lp;
			if( sdt.EnterLoop(lp2) ){
				WORD sid = (WORD)sdt.GetNumber(Desc::service_id, lp);
				do{
					if( sdt.GetNumber(Desc::descriptor_tag, lp2) != Desc::logo_transmission_descriptor ){
						continue;
					}
					DWORD type = sdt.GetNumber(Desc::logo_transmission_type, lp2);
					if( type != 1 && type != 2 ){
						continue;
					}
					LONGLONG key = onid << 16 | sdt.GetNumber(Desc::logo_id, lp2);
					vector<LOGO_DATA>::iterator itr = lower_bound_first(this->logoMap.begin(), this->logoMap.end(), key << 16);
					for( ; itr != this->logoMap.end() && (itr->first >> 16) == key; itr++ ){
						if( std::find(itr->serviceList.begin(), itr->serviceList.end(), sid) == itr->serviceList.end() ){
							itr->serviceList.push_back(sid);
						}
					}
				}while( sdt.NextLoopIndex(lp2) );
			}
		}while( sdt.NextLoopIndex(lp) );
	}
}

void CDecodeUtil::AppendPngPalette(vector<BYTE>& dest)
{
	size_t clutSize = array_size(CARIB8CharDecode::DefClut);
	dest.push_back(0);
	dest.push_back(0);
	dest.push_back((BYTE)((3 * clutSize) >> 8));
	dest.push_back((BYTE)(3 * clutSize));
	dest.push_back('P');
	dest.push_back('L');
	dest.push_back('T');
	dest.push_back('E');
	for( size_t i = 0; i < clutSize; i++ ){
		dest.push_back(CARIB8CharDecode::DefClut[i].ucR);
		dest.push_back(CARIB8CharDecode::DefClut[i].ucG);
		dest.push_back(CARIB8CharDecode::DefClut[i].ucB);
	}
	//事前計算のCRCを埋め込む
	dest.push_back(0x91);
	dest.push_back(0xFB);
	dest.push_back(0x1F);
	dest.push_back(0xA7);

	dest.push_back(0);
	dest.push_back(0);
	dest.push_back((BYTE)(clutSize >> 8));
	dest.push_back((BYTE)clutSize);
	dest.push_back('t');
	dest.push_back('R');
	dest.push_back('N');
	dest.push_back('S');
	for( size_t i = 0; i < clutSize; i++ ){
		dest.push_back(CARIB8CharDecode::DefClut[i].ucAlpha);
	}
	dest.push_back(0xCE);
	dest.push_back(0xB6);
	dest.push_back(0xB1);
	dest.push_back(0x6C);
}

//取得するロゴタイプをフラグで指定する
void CDecodeUtil::SetLogoTypeFlags(
	DWORD flags,
	const WORD** additionalNeededPids
	)
{
	this->logoTypeFlags = flags & 0x3F;

	this->additionalNeededPidList.clear();
	if( additionalNeededPids ){
		for( auto itr = this->engineeringPmtMap.cbegin(); itr != this->engineeringPmtMap.end(); itr++ ){
			//エンジニアリングサービスのPMTも見たい
			if( this->patInfo ){
				Desc::CDescriptor::CLoopPointer lp;
				if( this->patInfo->EnterLoop(lp) ){
					do{
						if( this->patInfo->GetNumber(Desc::program_number, lp) == itr->first ){
							this->additionalNeededPidList.push_back((WORD)this->patInfo->GetNumber(Desc::program_map_PID, lp));
							break;
						}
					}while( this->patInfo->NextLoopIndex(lp) );
				}
			}
			//ロゴが含まれるデータカルーセルも見たい
			Desc::CDescriptor::CLoopPointer lp;
			if( itr->second && itr->second->EnterLoop(lp, 1) ){
				do{
					Desc::CDescriptor::CLoopPointer lp2 = lp;
					if( itr->second->GetNumber(Desc::stream_type, lp) == 0x0D && itr->second->EnterLoop(lp2) ){
						//DSM-CC type D (データカルーセル)
						do{
							if( itr->second->GetNumber(Desc::descriptor_tag, lp2) == Desc::stream_identifier_descriptor ){
								if( itr->second->GetNumber(Desc::component_tag, lp2) == 0x79 ){
									//TR-B15 全受信機共通データ
									this->additionalNeededPidList.push_back((WORD)itr->second->GetNumber(Desc::elementary_PID, lp));
								}
								break;
							}
						}while( itr->second->NextLoopIndex(lp2) );
					}
				}while( itr->second->NextLoopIndex(lp) );
			}
		}

		this->additionalNeededPidList.push_back(0);
		*additionalNeededPids = this->additionalNeededPidList.data();
	}
}

//全ロゴを列挙する
BOOL CDecodeUtil::EnumLogoList(
	BOOL (CALLBACK *enumLogoListProc)(DWORD, const LOGO_INFO*, LPVOID),
	LPVOID param
	)
{
	if( this->logoMap.empty() ){
		return FALSE;
	}
	if( enumLogoListProc((DWORD)this->logoMap.size(), NULL, param) ){
		for( vector<LOGO_DATA>::const_iterator itr = this->logoMap.begin(); itr != this->logoMap.end(); itr++ ){
			LOGO_INFO info;
			info.onid = (WORD)(itr->first >> 32);
			info.id = (WORD)(itr->first >> 16);
			info.type = (BYTE)itr->first;
			info.bReserved = 0;
			info.wReserved = 0;
			info.dataSize = (DWORD)itr->data.size();
			info.serviceListSize = (DWORD)itr->serviceList.size();
			info.data = itr->data.data();
			info.serviceList = itr->serviceList.data();
			if( enumLogoListProc(1, &info, param) == FALSE ){
				break;
			}
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
			do{
				if( this->sitInfo->GetNumber(Desc::descriptor_tag, lp) == Desc::network_identification_descriptor ){
					*originalNetworkID = (WORD)this->sitInfo->GetNumber(Desc::network_id, lp);
					return TRUE;
				}
			}while( this->sitInfo->NextLoopIndex(lp) );
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
			do{
				if( itr->second.GetNumber(Desc::descriptor_tag, lp) != Desc::network_name_descriptor ){
					continue;
				}
				DWORD srcSize;
				const BYTE* src = itr->second.GetBinary(Desc::d_char, &srcSize, lp);
				if( src && srcSize > 0 ){
					CARIB8CharDecode arib;
					arib.PSISI(src, srcSize, &network_nameW);
				}
			}while( itr->second.NextLoopIndex(lp) );
		}
		lp = Desc::CDescriptor::CLoopPointer();
		if( itr->second.EnterLoop(lp, 1) == false ){
			continue;
		}
		do{
			Desc::CDescriptor::CLoopPointer lp2 = lp;
			if( itr->second.EnterLoop(lp2) == false ){
				continue;
			}
			do{
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
						do{
							partialServiceList.push_back((WORD)itr->second.GetNumber(Desc::service_id, lp3));
						}while( itr->second.NextLoopIndex(lp3) );
					}
				}
			}while( itr->second.NextLoopIndex(lp2) );
		}while( itr->second.NextLoopIndex(lp) );
	}

	DWORD count = 0;
	for( auto itr = this->sdtActualInfo.cbegin(); itr != this->sdtActualInfo.end(); itr++ ){
		Desc::CDescriptor::CLoopPointer lp;
		if( itr->second.EnterLoop(lp) == false ){
			continue;
		}
		do{
			this->serviceDBList[count].ONID = (WORD)itr->second.GetNumber(Desc::original_network_id);
			this->serviceDBList[count].TSID = (WORD)itr->second.GetNumber(Desc::transport_stream_id);
			this->serviceDBList[count].SID = (WORD)itr->second.GetNumber(Desc::service_id, lp);
			this->serviceDBList[count].service_type = 0;

			Desc::CDescriptor::CLoopPointer lp2 = lp;
			if( itr->second.EnterLoop(lp2) ){
				do{
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
				}while( itr->second.NextLoopIndex(lp2) );
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
		}while( itr->second.NextLoopIndex(lp) );
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
		do{
			if( this->sitInfo->GetNumber(Desc::descriptor_tag, lp) == Desc::network_identification_descriptor ){
				ONID = (WORD)this->sitInfo->GetNumber(Desc::network_id);
			}
		}while( this->sitInfo->NextLoopIndex(lp) );
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
			do{
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
			}while( this->sitInfo->NextLoopIndex(lp2) );
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
