#include "stdafx.h"
#include "TSBuffUtil.h"

CTSBuffUtil::CTSBuffUtil()
{
	this->sectionSize = 0;
	this->lastPID = 0xFFFF;
	this->lastCounter = 0xFF;

	this->duplicateFlag = false;
}

void CTSBuffUtil::Clear()
{
	this->sectionSize = 0;
	this->sectionBuff.clear();
	this->carryPacket.clear();

	this->lastPID = 0xFFFF;
	this->lastCounter = 0xFF;
}

bool CTSBuffUtil::CheckCounter(const CTSPacketUtil* tsPacket)
{
	if( tsPacket->PID == 0x1FFF ){
		//NULLパケット時は意味なし
		this->duplicateFlag = false;
		return true;
	}
	if( this->lastPID != 0xFFFF || this->lastCounter != 0xFF ){
		if( this->lastPID != tsPacket->PID ){
			//PID変更されたので不連続
			this->duplicateFlag = false;
			return false;
		}else{
			if( tsPacket->adaptation_field_control == 0x00 || tsPacket->adaptation_field_control == 0x02 ){
				//ペイロードが存在しない場合は意味なし
				this->duplicateFlag = false;
				if( tsPacket->has_adaptation_field_flags ){
					if( tsPacket->transport_scrambling_control == 0 ){
						if(tsPacket->discontinuity_indicator == 1){
							//不連続の判定が必要
							return false;
						}else{
							return true;
						}
					}else{
						return true;
					}
				}else{
					return true;
				}
			}
			if( this->lastCounter == tsPacket->continuity_counter ){
				if( tsPacket->adaptation_field_control == 0x01 || tsPacket->adaptation_field_control == 0x03 ){
					if( tsPacket->transport_scrambling_control == 0 ){
						if( this->duplicateFlag == false ){
							//重送？一応連続と判定
							this->duplicateFlag = true;
							if( tsPacket->has_adaptation_field_flags ){
								if(tsPacket->discontinuity_indicator == 1){
									//不連続の判定が必要
									return false;
								}else{
									return true;
								}
							}else{
								return true;
							}
						}else{
							//前回重送と判断してるので不連続
							this->duplicateFlag = false;
							return false;
						}
					}else{
						return true;
					}
				}
			}
			if( this->lastCounter+1 != tsPacket->continuity_counter ){
				if( this->lastCounter != 0x0F && tsPacket->continuity_counter != 0x00 ){
					//カウンターが飛んだので不連続
					return false;
				}
			}
		}
	}
	return true;
}

CTSBuffUtil::ADD_RESULT CTSBuffUtil::Add188TS(const CTSPacketUtil& tsPacket_)
{
	const CTSPacketUtil* tsPacket = &tsPacket_;

	//バッファをすべて受け取る
	BYTE* sectionData;
	DWORD dataSize;
	while( GetSectionBuff(&sectionData, &dataSize) );

	//カウンターチェック
	if( CheckCounter(tsPacket) == false ){
		Clear();
	}
	//スクランブルのチェック
	if( tsPacket->transport_scrambling_control != 0 ){
		//スクランブルパケットなので解析できない
		Clear();
		return ERR_NOT_SUPPORT;
	}

	if( tsPacket->payload_unit_start_indicator == 1 ){
		if( tsPacket->data_byteSize < 3 ){
			//サイズが小さすぎる
			return ERR_FALSE;
		}
		if(tsPacket->data_byte[0] == 0x00 && tsPacket->data_byte[1] == 0x00 && tsPacket->data_byte[2] == 0x01){
			//PES
			Clear();
			return ERR_NOT_SUPPORT;
		}else if( tsPacket->has_adaptation_field_flags && tsPacket->random_access_indicator ){
			//PES
			Clear();
			return ERR_NOT_SUPPORT;
		}else if( tsPacket->has_adaptation_field_flags && tsPacket->PCR_flag ){
			//PCR
			Clear();
			return ERR_NOT_SUPPORT;
		}else if( tsPacket->has_adaptation_field_flags && tsPacket->OPCR_flag ){
			//OPCR
			Clear();
			return ERR_NOT_SUPPORT;
		}
	}

	if( this->lastPID == 0xFFFF && this->lastCounter == 0xFF ){
		//初回
		if( tsPacket->payload_unit_start_indicator == 1 ){
			//PSI
			this->lastPID = tsPacket->PID;
			this->lastCounter = tsPacket->continuity_counter;
			return AddSectionBuff(tsPacket->data_byte, tsPacket->data_byteSize, 1);
		}else{
			//スタート位置ではない
			return ERR_ADD_NEXT;
		}
	}else{
		this->lastPID = tsPacket->PID;
		this->lastCounter = tsPacket->continuity_counter;
		return AddSectionBuff(tsPacket->data_byte, tsPacket->data_byteSize, tsPacket->payload_unit_start_indicator);
	}

}

bool CTSBuffUtil::GetSectionBuff(BYTE** sectionData, DWORD* dataSize)
{
	if( sectionSize == 0 && carryPacket.empty() == false ){
		//繰り越しパケットを処理
		if( AddSectionBuff(&carryPacket.front(), (BYTE)carryPacket.size(), 1) != INTERNAL_ERR_CARRY_OVER ){
			carryPacket.clear();
		}
	}
	if( sectionSize == 0 || sectionSize != sectionBuff.size() ){
		//sectionBuffはGet済みか作成途中
		carryPacket.clear();
		return false;
	}

	*sectionData = &sectionBuff.front();
	*dataSize = sectionSize;
	//sectionBuffがGet済みであることを示す
	sectionSize = 0;

	return true;
}

CTSBuffUtil::ADD_RESULT CTSBuffUtil::AddSectionBuff(const BYTE* payload, BYTE payloadSize, BYTE unitStartIndicator)
{
	if( payloadSize == 0 || payload == nullptr ){
		return ERR_ADD_NEXT;
	}
	if( unitStartIndicator == 0 && (sectionSize == 0 || sectionSize == sectionBuff.size()) ){
		return ERR_ADD_NEXT;
	}

	if( unitStartIndicator ){
		BYTE pointer_field = payload[0];
		if( pointer_field + 1 > payloadSize ){
			//サイズが小さすぎる
			AddDebugLogFormat(L"★psi size err PID 0x%04X", this->lastPID);
			sectionSize = 0;
			return ERR_FALSE;
		}
		if( sectionSize != 0 && sectionSize != sectionBuff.size() ){
			if( sectionSize - sectionBuff.size() == pointer_field ){
				sectionBuff.insert(sectionBuff.end(), payload + 1, payload + 1 + pointer_field);
				//残りのペイロードを繰り越す
				carryPacket.assign(1, 0);
				carryPacket.insert(carryPacket.end(), payload + 1 + pointer_field, payload + payloadSize);
				return NO_ERR;
			}else{
				//サイズがおかしいのでクリア
				AddDebugLogFormat(L"★psi section size err PID 0x%04X", this->lastPID);
				sectionSize = 0;
			}
		}
		BYTE readSize = pointer_field + 1;

		//マルチセクションチェック
		if( readSize + 2 >= payloadSize ||
		    (payload[readSize] == 0xFF &&
		     payload[readSize + 1] == 0xFF &&
		     payload[readSize + 2] == 0xFF) ){
			//残りはスタッフィングバイト
			return ERR_ADD_NEXT;
		}

		sectionSize = (((DWORD)payload[readSize + 1] & 0x0F) << 8 | payload[readSize + 2]) + 3;
		sectionBuff.assign(payload + readSize, payload + min((DWORD)payloadSize, readSize + sectionSize));
		if( sectionSize == sectionBuff.size() ){
			//このパケットだけで完結。残りのペイロードを繰り越す
			if( carryPacket.empty() == false && payload == &carryPacket.front() ){
				carryPacket.erase(carryPacket.begin() + readSize, carryPacket.begin() + readSize + sectionSize);
				//マルチセクションによる連続繰り越しであることを示す特別な戻り値
				return INTERNAL_ERR_CARRY_OVER;
			}else{
				carryPacket.assign(1, 0);
				carryPacket.insert(carryPacket.end(), payload + readSize + sectionSize, payload + payloadSize);
				return NO_ERR;
			}
		}else{
			//次のパケット必要
			return ERR_ADD_NEXT;
		}
	}else{
		//複数パケットにまたがっている
		sectionBuff.insert(sectionBuff.end(), payload, payload + min((DWORD)payloadSize, sectionSize - (DWORD)sectionBuff.size()));
		if( sectionSize == sectionBuff.size() ){
			return NO_ERR;
		}else{
			return ERR_ADD_NEXT;
		}
	}
}
