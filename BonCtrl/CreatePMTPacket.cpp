#include "stdafx.h"
#include "CreatePMTPacket.h"
#include "../Common/EpgTimerUtil.h"

CCreatePMTPacket::CCreatePMTPacket(void)
{
	this->needCaption = TRUE;
	this->needData = TRUE;
	this->createVer = 0;
	this->createCounter = 0;
}

//PMT作成時のモード
//引数：
// needCaption			[IN]字幕データを含めるかどうか（TRUE:含める、FALSE：含めない）
// needData				[IN]データカルーセルを含めるかどうか（TRUE:含める、FALSE：含めない）
void CCreatePMTPacket::SetCreateMode(
	BOOL needCaption_,
	BOOL needData_
)
{
	if( this->needCaption != needCaption_ || this->needData != needData_ ){
		this->needCaption = needCaption_;
		this->needData = needData_;
		CreatePMT();
	}
}

void CCreatePMTPacket::SetSectionData(
	const BYTE* data,
	DWORD dataSize
)
{
	if( dataSize != this->lastSection.size() ||
	    std::equal(data, data + dataSize, this->lastSection.begin()) == false ){
		this->lastSection.assign(data, data + dataSize);
		CreatePMT();
	}
}

void CCreatePMTPacket::CreatePMT()
{
	this->elementaryPIDList.clear();
	this->ecmPIDList.clear();
	this->createPSI.clear();
	const vector<BYTE>& data = this->lastSection;

	if( data.size() < 3 ){
		return;
	}
	//有効なPMTセクションであるはずなので、ここで固定値などの検査はしない

	DWORD readSize = 0;
	//////////////////////////////////////////////////////
	//解析処理
	WORD section_length = ((WORD)data[1]&0x0F)<<8 | data[2];
	readSize+=3;

	if( readSize+section_length > data.size() || section_length < 9 + 4 ){
		//サイズ異常
		AddDebugLog(L"CCreatePMTPacket::section_length Err");
		return;
	}

	WORD program_info_length = ((WORD)data[readSize+7]&0x0F)<<8 | data[readSize+8];
	readSize += 9;

	if( readSize + program_info_length > (DWORD)section_length+3-4 ){
		AddDebugLogFormat(L"CCreatePMTPacket::program_info_length %d Err", program_info_length);
		return;
	}

	//pointer_field
	this->createPSI.push_back(0);

	//descriptor1
	//最初のDescriptorループまでコピー
	this->createPSI.insert(this->createPSI.end(), data.begin(), data.begin() + readSize + program_info_length);

	//ECMあるかだけチェック
	WORD infoRead = 0;
	while(infoRead+1 < program_info_length){
		BYTE descriptor_tag = data[readSize];
		BYTE descriptor_length = data[readSize+1];
		readSize+=2;

		if( descriptor_tag == 0x09 && descriptor_length >= 4 && infoRead+2+3 < program_info_length){
			//CA
			WORD CA_PID = ((WORD)data[readSize+2]&0x1F)<<8 | (WORD)data[readSize+3];
			if (CA_PID != 0x1fff) {
				this->ecmPIDList.push_back(CA_PID);
			}
		}
		readSize += descriptor_length;

		infoRead+= 2+descriptor_length;
	}

	//descriptor2
	vector<SECOND_DESC_BUFF> secondDescBuff;
	while( readSize+4 < (DWORD)section_length+3-4 ){
		WORD ES_info_length = ((WORD)data[readSize+3]&0x0F)<<8 | data[readSize+4];
		if( readSize+ES_info_length+5 > (DWORD)section_length+3-4 ){
			break;
		}
		SECOND_DESC_BUFF item;
		item.stream_type = data[readSize];
		item.elementary_PID = ((WORD)data[readSize+1]&0x1F)<<8 | data[readSize+2];
		item.quality = 0;
		item.qualityPID = 0;
		item.descBuffPos = readSize;
		item.descBuffSize = ES_info_length + 5;
		readSize += item.descBuffSize;

		//descriptor
		const BYTE* descBuff = data.data() + item.descBuffPos;
		infoRead = 5;
		while(infoRead+1 < item.descBuffSize){
			BYTE descriptor_tag = descBuff[infoRead];
			BYTE descriptor_length = descBuff[infoRead+1];

			if( descriptor_tag == 0x09 && descriptor_length >= 4 && infoRead+5 < item.descBuffSize ){
				//CA
				WORD CA_PID = ((WORD)descBuff[2+infoRead+2]&0x1F)<<8 | (WORD)descBuff[2+infoRead+3];
				if (CA_PID != 0x1fff) {
					this->ecmPIDList.push_back(CA_PID);
				}
			}else if( descriptor_tag == 0xC0 && descriptor_length >= 3 && infoRead+4 < item.descBuffSize ){
				//階層伝送記述子
				item.quality = descBuff[2+infoRead]&0x01;
				item.qualityPID = ((WORD)descBuff[2+infoRead+1]&0x1F)<<8 | descBuff[2+infoRead+2];
			}
			infoRead += 2+descriptor_length;
		}
		secondDescBuff.push_back(item);
	}

	BOOL findVHighQ = FALSE;
	BOOL findAHighQ = FALSE;
	BOOL findMPEG2V = FALSE;
	BOOL findAAC = FALSE;

	this->createVer++;
	if( this->createVer > 31 ){
		this->createVer = 0;
	}

	//データ一覧チェック
	for( size_t i=0; i<secondDescBuff.size(); i++ ){
		if( secondDescBuff[i].quality == 1 ){
			//高階層あり
			if( secondDescBuff[i].stream_type == 0x02 ){
				findVHighQ = TRUE;
			}else if( secondDescBuff[i].stream_type == 0x0F ){
				findAHighQ = TRUE;
			}
		}
		if( secondDescBuff[i].stream_type == 0x02 ){
			findMPEG2V = TRUE;
		}
		else if( secondDescBuff[i].stream_type == 0x0F ){
			findAAC = TRUE;
		}
	}

	for( size_t i=0; i<secondDescBuff.size(); i++ ){
		BOOL matched = FALSE;
		switch(secondDescBuff[i].stream_type){
			case 0x02:
				//MPEG2 VIDEO
				if( findVHighQ == TRUE ){
					if( secondDescBuff[i].quality == 1 ){
						matched = TRUE;
					}
				}else{
					matched = TRUE;
				}
				break;
			case 0x0F:
				//MPEG2 AAC
				if( findAHighQ == TRUE ){
					if( secondDescBuff[i].quality == 1 ){
						matched = TRUE;
					}
				}else{
					matched = TRUE;
				}
				break;
			case 0x1B:
				//MPEG4 VIDEO
				if( findMPEG2V == FALSE ){
					matched = TRUE;
				}
				break;
			case 0x04:
				//MPEG2 AUDIO
				if( findAAC == FALSE ){
					matched = TRUE;
				}
				break;
			case 0x24:
				//HEVC VIDEO
				matched = TRUE;
				break;
			case 0x06:
				//字幕
				if( this->needCaption == TRUE ){
					matched = TRUE;
				}
				break;
			case 0x0D:
				//データカルーセル
				if( this->needData == TRUE ){
					matched = TRUE;
				}
				break;
			default:
				break;
		}
		if( matched != FALSE ){
			this->createPSI.insert(this->createPSI.end(),
			                       data.begin() + secondDescBuff[i].descBuffPos,
			                       data.begin() + secondDescBuff[i].descBuffPos + secondDescBuff[i].descBuffSize);
			this->elementaryPIDList.push_back(secondDescBuff[i].elementary_PID);
		}
	}

	//SectionLength
	this->createPSI[2] = (this->createPSI.size()+4-4)>>8&0x0F;
	this->createPSI[2] |= 0xB0; 
	this->createPSI[3] = (this->createPSI.size()+4-4)&0xFF;
	//バージョン
	this->createPSI[6] = this->createVer<<1;
	this->createPSI[6] |= 0xC1;

	DWORD ulCrc = CalcCrc32((int)this->createPSI.size()-1, &this->createPSI[1]);
	this->createPSI.push_back(ulCrc>>24&0xFF);
	this->createPSI.push_back(ulCrc>>16&0xFF);
	this->createPSI.push_back(ulCrc>>8&0xFF);
	this->createPSI.push_back(ulCrc&0xFF);
}

BOOL CCreatePMTPacket::IsEcmPID(
	WORD pid
)
{
	return std::find(this->ecmPIDList.begin(), this->ecmPIDList.end(), pid) != this->ecmPIDList.end();
}

BOOL CCreatePMTPacket::IsElementaryPID(
	WORD pid
)
{
	return std::find(this->elementaryPIDList.begin(), this->elementaryPIDList.end(), pid) != this->elementaryPIDList.end();
}

BOOL CCreatePMTPacket::GetPacket(
	BYTE** buff,
	DWORD* size,
	WORD pid
)
{
	//TSパケットを作成
	this->createPacket.clear();
	for( size_t i = 0; i < this->createPSI.size(); i += 184 ){
		this->createPacket.push_back(0x47);
		this->createPacket.push_back((pid >> 8 & 0x1F) | (i == 0 ? 0x40 : 0x00));
		this->createPacket.push_back(pid & 0xFF);
		this->createPacket.push_back(this->createCounter | 0x10);
		this->createCounter = (this->createCounter + 1) & 0x0F;
		this->createPacket.insert(this->createPacket.end(), this->createPSI.begin() + i, this->createPSI.begin() + min(i + 184, this->createPSI.size()));
		this->createPacket.resize(((this->createPacket.size() - 1) / 188 + 1) * 188, 0xFF);
	}
	if( this->createPacket.empty() == false ){
		*buff = &this->createPacket[0];
		*size = (DWORD)this->createPacket.size();
	}else{
		return FALSE;
	}
	return TRUE;
}

//内部情報をクリア
void CCreatePMTPacket::Clear()
{
	this->lastSection.clear();
	this->elementaryPIDList.clear();
	this->ecmPIDList.clear();
	this->createPSI.clear();
}
