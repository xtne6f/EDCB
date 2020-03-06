#include "stdafx.h"
#include "TSBuffUtil.h"

CTSBuffUtil::CTSBuffUtil()
{
	this->sectionSize = 0;
	this->lastPID = 0xFFFF;
	this->lastCounter = 0xFF;

	this->duplicateFlag = FALSE;
}

void CTSBuffUtil::Clear()
{
	this->sectionSize = 0;
	this->sectionBuff.clear();
	this->carryPacket.clear();

	this->lastPID = 0xFFFF;
	this->lastCounter = 0xFF;
}

BOOL CTSBuffUtil::CheckCounter(CTSPacketUtil* tsPacket)
{
	if( tsPacket->PID == 0x1FFF ){
		//NULLパケット時は意味なし
		this->duplicateFlag = FALSE;
		return TRUE;
	}
	if( this->lastPID != 0xFFFF || this->lastCounter != 0xFF ){
		if( this->lastPID != tsPacket->PID ){
			//PID変更されたので不連続
			this->duplicateFlag = FALSE;
			return FALSE;
		}else{
			if( tsPacket->adaptation_field_control == 0x00 || tsPacket->adaptation_field_control == 0x02 ){
				//ペイロードが存在しない場合は意味なし
				this->duplicateFlag = FALSE;
				if( tsPacket->adaptation_field_control == 0x02 || tsPacket->adaptation_field_control == 0x03 ){
					if( tsPacket->transport_scrambling_control == 0 ){
						if(tsPacket->discontinuity_indicator == 1){
							//不連続の判定が必要
							return FALSE;
						}else{
							return TRUE;
						}
					}else{
						return TRUE;
					}
				}else{
					return TRUE;
				}
			}
			if( this->lastCounter == tsPacket->continuity_counter ){
				if( tsPacket->adaptation_field_control == 0x01 || tsPacket->adaptation_field_control == 0x03 ){
					if( tsPacket->transport_scrambling_control == 0 ){
						if( this->duplicateFlag == FALSE ){
							//重送？一応連続と判定
							this->duplicateFlag = TRUE;
							if( tsPacket->adaptation_field_control == 0x02 || tsPacket->adaptation_field_control == 0x03 ){
								if(tsPacket->discontinuity_indicator == 1){
									//不連続の判定が必要
									return FALSE;
								}else{
									return TRUE;
								}
							}else{
								return TRUE;
							}
						}else{
							//前回重送と判断してるので不連続
							this->duplicateFlag = FALSE;
							return FALSE;
						}
					}else{
						return TRUE;
					}
				}
			}
			if( this->lastCounter+1 != tsPacket->continuity_counter ){
				if( this->lastCounter != 0x0F && tsPacket->continuity_counter != 0x00 ){
					//カウンターが飛んだので不連続
					return FALSE;
				}
			}
		}
	}
	return TRUE;
}

DWORD CTSBuffUtil::Add188TS(CTSPacketUtil* tsPacket)
{
	if( tsPacket == NULL ){
		return FALSE;
	}

	//バッファをすべて受け取る
	BYTE* sectionData;
	DWORD dataSize;
	while( GetSectionBuff(&sectionData, &dataSize) != FALSE );

	//カウンターチェック
	if( CheckCounter(tsPacket) == FALSE ){
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
			return FALSE;
		}
		if(tsPacket->data_byte[0] == 0x00 && tsPacket->data_byte[1] == 0x00 && tsPacket->data_byte[2] == 0x01){
			//PES
			Clear();
			return ERR_NOT_SUPPORT;
		}else if( tsPacket->adaptation_field_length > 0 && tsPacket->random_access_indicator == 1 ){
			//PES
			Clear();
			return ERR_NOT_SUPPORT;
		}else if( tsPacket->adaptation_field_length > 0 && tsPacket->PCR_flag == 1 ){
			//PCR
			Clear();
			return ERR_NOT_SUPPORT;
		}else if( tsPacket->adaptation_field_length > 0 && tsPacket->OPCR_flag == 1 ){
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
			return AddSectionBuff(tsPacket);
		}else{
			//スタート位置ではない
			return ERR_ADD_NEXT;
		}
	}else{
		this->lastPID = tsPacket->PID;
		this->lastCounter = tsPacket->continuity_counter;
		return AddSectionBuff(tsPacket);
	}

}

BOOL CTSBuffUtil::GetSectionBuff(BYTE** sectionData, DWORD* dataSize)
{
	if( sectionSize == 0 && carryPacket.empty() == false ){
		//繰り越しパケットを処理
		CTSPacketUtil tsPacket;
		tsPacket.payload_unit_start_indicator = 1;
		tsPacket.data_byteSize = (BYTE)carryPacket.size();
		tsPacket.data_byte = &carryPacket.front();
		if( AddSectionBuff(&tsPacket) != 2 ){
			carryPacket.clear();
		}
	}
	if( sectionSize == 0 || sectionSize != sectionBuff.size() ){
		//sectionBuffはGet済みか作成途中
		carryPacket.clear();
		return FALSE;
	}

	*sectionData = &sectionBuff.front();
	*dataSize = sectionSize;
	//sectionBuffがGet済みであることを示す
	sectionSize = 0;

	return TRUE;
}

DWORD CTSBuffUtil::AddSectionBuff(CTSPacketUtil* tsPacket)
{
	if( tsPacket->data_byteSize == 0 || tsPacket->data_byte == NULL ){
		return ERR_ADD_NEXT;
	}
	if( tsPacket->payload_unit_start_indicator != 1 && (sectionSize == 0 || sectionSize == sectionBuff.size()) ){
		return ERR_ADD_NEXT;
	}

	if( tsPacket->payload_unit_start_indicator == 1 ){
		BYTE pointer_field = tsPacket->data_byte[0];
		if( pointer_field + 1 > tsPacket->data_byteSize ){
			//サイズが小さすぎる
			_OutputDebugString(L"★psi size err PID 0x%04X\r\n", tsPacket->PID);
			sectionSize = 0;
			return FALSE;
		}
		if( sectionSize != 0 && sectionSize != sectionBuff.size() ){
			if( sectionSize - sectionBuff.size() == pointer_field ){
				sectionBuff.insert(sectionBuff.end(), tsPacket->data_byte + 1, tsPacket->data_byte + 1 + pointer_field);
				//残りのペイロードを繰り越す
				carryPacket.assign(1, 0);
				carryPacket.insert(carryPacket.end(), tsPacket->data_byte + 1 + pointer_field, tsPacket->data_byte + tsPacket->data_byteSize);
				return TRUE;
			}else{
				//サイズがおかしいのでクリア
				_OutputDebugString(L"★psi section size err PID 0x%04X\r\n", tsPacket->PID);
				sectionSize = 0;
			}
		}
		BYTE readSize = pointer_field + 1;

		//マルチセクションチェック
		if( readSize + 2 >= tsPacket->data_byteSize ||
		    tsPacket->data_byte[readSize] == 0xFF &&
		    tsPacket->data_byte[readSize+1] == 0xFF &&
		    tsPacket->data_byte[readSize+2] == 0xFF ){
			//残りはスタッフィングバイト
			return ERR_ADD_NEXT;
		}

		sectionSize = (((DWORD)tsPacket->data_byte[readSize+1]&0x0F) << 8 | tsPacket->data_byte[readSize+2]) + 3;
		sectionBuff.assign(tsPacket->data_byte + readSize, tsPacket->data_byte + min((DWORD)tsPacket->data_byteSize, readSize + sectionSize));
		if( sectionSize == sectionBuff.size() ){
			//このパケットだけで完結。残りのペイロードを繰り越す
			if( carryPacket.empty() == false && tsPacket->data_byte == &carryPacket.front() ){
				carryPacket.erase(carryPacket.begin() + readSize, carryPacket.begin() + readSize + sectionSize);
				//マルチセクションによる連続繰り越しであることを示す特別な戻り値
				return 2;
			}else{
				carryPacket.assign(1, 0);
				carryPacket.insert(carryPacket.end(), tsPacket->data_byte + readSize + sectionSize, tsPacket->data_byte + tsPacket->data_byteSize);
				return TRUE;
			}
		}else{
			//次のパケット必要
			return ERR_ADD_NEXT;
		}
	}else{
		//複数パケットにまたがっている
		sectionBuff.insert(sectionBuff.end(), tsPacket->data_byte, tsPacket->data_byte + min((DWORD)tsPacket->data_byteSize, sectionSize - (DWORD)sectionBuff.size()));
		if( sectionSize == sectionBuff.size() ){
			return TRUE;
		}else{
			return ERR_ADD_NEXT;
		}
	}
}
