#include "StdAfx.h"
#include "TSBuffUtil.h"

CTSBuffUtil::CTSBuffUtil(BOOL supportPES)
{
	this->lastPID = 0xFFFF;
	this->lastCounter = 0xFF;

	this->creatingBuff = NULL;
	this->lastGetBuff = NULL;

	this->duplicateFlag = FALSE;

	this->supportPES = supportPES;
	this->PESMode = FALSE;
}

CTSBuffUtil::~CTSBuffUtil(void)
{
	Clear();
	SAFE_DELETE(this->lastGetBuff);
}

void CTSBuffUtil::Clear()
{
	for(size_t i=0; i<this->packetList.size(); i++ ){
		SAFE_DELETE(this->packetList[i]);
	}
	this->packetList.clear();

	this->lastPID = 0xFFFF;
	this->lastCounter = 0xFF;

	SAFE_DELETE(this->creatingBuff);

	this->PESMode = FALSE;
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
			if( this->supportPES == FALSE ){
				Clear();
				return ERR_NOT_SUPPORT;
			}
			this->PESMode = TRUE;
		}else if( tsPacket->adaptation_field_length > 0 && tsPacket->random_access_indicator == 1 ){
			//PES
			if( this->supportPES == FALSE ){
				Clear();
				return ERR_NOT_SUPPORT;
			}
			this->PESMode = TRUE;
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
			if( this->PESMode == FALSE ){
				return AddSectionBuff(tsPacket);
			}else{
				return AddPESBuff(tsPacket);
			}
		}else{
			//スタート位置ではない
			return ERR_ADD_NEXT;
		}
	}else{
		this->lastPID = tsPacket->PID;
		this->lastCounter = tsPacket->continuity_counter;
		if( this->PESMode == FALSE ){
			return AddSectionBuff(tsPacket);
		}else{
			return AddPESBuff(tsPacket);
		}
	}

}

BOOL CTSBuffUtil::IsPES()
{
	return this->PESMode;
}

BOOL CTSBuffUtil::GetSectionBuff(BYTE** sectionData, DWORD* dataSize)
{
	SAFE_DELETE(lastGetBuff);
	if( packetList.size() == 0 ){
		return FALSE;
	}

	lastGetBuff = packetList[0];
	packetList.erase(packetList.begin());

	*sectionData = lastGetBuff->data;
	*dataSize = lastGetBuff->dataSize;

	return TRUE;
}

DWORD CTSBuffUtil::AddSectionBuff(CTSPacketUtil* tsPacket)
{
	if( tsPacket->data_byteSize == 0 || tsPacket->data_byte == NULL ){
		return ERR_ADD_NEXT;
	}
	if( tsPacket->payload_unit_start_indicator != 1 && creatingBuff == NULL ){
		return ERR_ADD_NEXT;
	}

	BYTE readSize = 0;
	if( tsPacket->payload_unit_start_indicator == 1 ){
		BOOL addBuff = FALSE;
		BYTE pointer_field = tsPacket->data_byte[0];
		readSize++;
		if( pointer_field != 0 ){
			//マルチセクション
			if( creatingBuff != NULL ){
				if( creatingBuff->dataSize - creatingBuff->copySize == pointer_field ){
					memcpy(creatingBuff->data + creatingBuff->copySize, tsPacket->data_byte + readSize, pointer_field);
					creatingBuff->copySize += pointer_field;
					packetList.push_back(creatingBuff);
					creatingBuff = NULL;
					addBuff = TRUE;
				}else{
					//サイズがおかしいのでクリア
					_OutputDebugString(L"★multi section size err PID 0x%04X\r\n", tsPacket->PID);
					SAFE_DELETE(creatingBuff);
				}
			}
		}else{
			if( creatingBuff != NULL ){
				SAFE_DELETE(creatingBuff);
				creatingBuff = NULL;
			}
		}
		readSize+=pointer_field;

		if( pointer_field + 1 > tsPacket->data_byteSize ){
			//サイズが小さすぎる
			SAFE_DELETE(creatingBuff);
			_OutputDebugString(L"★psi size err PID 0x%04X\r\n", tsPacket->PID);
			if( addBuff == TRUE ){
				return TRUE;
			}else{
				return FALSE;
			}
		}

		//マルチセクションチェック
		while( readSize+3 < tsPacket->data_byteSize ){
			if( tsPacket->data_byte[readSize] == 0xFF &&
				tsPacket->data_byte[readSize+1] == 0xFF &&
				tsPacket->data_byte[readSize+2] == 0xFF){
				//残りはスタッフィングバイト
				break;
			}

			SECTION_BUFF* buff = new SECTION_BUFF;
			buff->dataSize = (((DWORD)tsPacket->data_byte[readSize+1]&0x0F) << 8 | tsPacket->data_byte[readSize+2]) + 3;
			buff->data = new BYTE[buff->dataSize];

			if( buff->dataSize <= (DWORD)tsPacket->data_byteSize - readSize ){
				buff->copySize = buff->dataSize;
			}else{
				buff->copySize = tsPacket->data_byteSize - readSize;
			}

			memcpy( buff->data, tsPacket->data_byte+readSize, buff->copySize );
			readSize += (BYTE)buff->copySize;

			if( buff->copySize == buff->dataSize ){
				//このパケットだけで完結
				packetList.push_back(buff);
			}else{
				//次のパケットが必要
				creatingBuff = buff;
				break;
			}
		}
		if( creatingBuff == NULL ){
			return TRUE;
		}else{
			//次のパケット必要
			if( addBuff == TRUE ){
				return TRUE;
			}else{
				return ERR_ADD_NEXT;
			}
		}
	}else{
		//複数パケットにまたがっている
		DWORD copySize = 0;
		DWORD needSize = creatingBuff->dataSize - creatingBuff->copySize;

		if( needSize <= tsPacket->data_byteSize ){
			copySize = needSize;
		}else{
			copySize = tsPacket->data_byteSize;
		}

		memcpy( creatingBuff->data + creatingBuff->copySize, tsPacket->data_byte, copySize );

		creatingBuff->copySize += copySize;
		if( creatingBuff->dataSize == creatingBuff->copySize ){
			packetList.push_back(creatingBuff);
			creatingBuff = NULL;
			return TRUE;
		}else{
			return ERR_ADD_NEXT;
		}
	}
}

DWORD CTSBuffUtil::AddPESBuff(CTSPacketUtil* tsPacket)
{
	if( tsPacket->data_byteSize == 0 || tsPacket->data_byte == NULL ){
		return ERR_ADD_NEXT;
	}
	if( tsPacket->payload_unit_start_indicator != 1 && creatingBuff == NULL ){
		return ERR_ADD_NEXT;
	}

	if( tsPacket->payload_unit_start_indicator == 1 ){
		if(tsPacket->data_byteSize < 6 || tsPacket->data_byte[0] != 0x00 || tsPacket->data_byte[1] != 0x00 || tsPacket->data_byte[2] != 0x01){
			SAFE_DELETE(creatingBuff);
			return FALSE;
		}
		if( creatingBuff != NULL ){
			packetList.push_back(creatingBuff);
			creatingBuff = NULL;
		}

		WORD PES_packet_length = ((WORD)tsPacket->data_byte[4])<<8 | tsPacket->data_byte[5];
		SECTION_BUFF* buff = new SECTION_BUFF;

		if( PES_packet_length == 0 ){
			buff->unknownSize = TRUE;
			buff->dataSize = 1024*1024;
			buff->data = new BYTE[buff->dataSize];
			memcpy(buff->data, tsPacket->data_byte, tsPacket->data_byteSize);
			buff->copySize = tsPacket->data_byteSize;
			creatingBuff = buff;
			return TRUE;
		}else{
			buff->dataSize = PES_packet_length + 6;
			buff->data = new BYTE[buff->dataSize];

			if( buff->dataSize < tsPacket->data_byteSize ){
				buff->copySize = buff->dataSize;
			}else{
				buff->copySize = tsPacket->data_byteSize;
			}

			memcpy(buff->data, tsPacket->data_byte, buff->copySize);
			if( buff->dataSize != buff->copySize ){
				creatingBuff = buff;
				return ERR_ADD_NEXT;
			}else{
				packetList.push_back(buff);
				creatingBuff = NULL;
				return TRUE;
			}
		}
	}else{
		//複数パケットにまたがっている
		if( creatingBuff->unknownSize == TRUE ){
			if( tsPacket->data_byteSize + creatingBuff->copySize > creatingBuff->dataSize ){
				BYTE* newBuff = new BYTE[creatingBuff->dataSize*2];
				memcpy(newBuff, creatingBuff->data, creatingBuff->copySize);
				memcpy(newBuff + creatingBuff->copySize, tsPacket->data_byte, tsPacket->data_byteSize);
				SAFE_DELETE_ARRAY(creatingBuff->data);
				creatingBuff->data = newBuff;
				creatingBuff->dataSize = creatingBuff->dataSize*2;
				creatingBuff->copySize += tsPacket->data_byteSize;
			}else{
				memcpy(creatingBuff->data + creatingBuff->copySize, tsPacket->data_byte, tsPacket->data_byteSize);
				creatingBuff->copySize += tsPacket->data_byteSize;
			}
			return ERR_ADD_NEXT;
		}else{
			DWORD copySize = 0;
			DWORD needSize = creatingBuff->dataSize - creatingBuff->copySize;

			if( needSize <= tsPacket->data_byteSize ){
				copySize = needSize;
			}else{
				copySize = tsPacket->data_byteSize;
			}

			memcpy( creatingBuff->data + creatingBuff->copySize, tsPacket->data_byte, copySize );

			creatingBuff->copySize += copySize;
			if( creatingBuff->dataSize == creatingBuff->copySize ){
				packetList.push_back(creatingBuff);
				creatingBuff = NULL;
				return TRUE;
			}else{
				return ERR_ADD_NEXT;
			}
		}
	}
}
