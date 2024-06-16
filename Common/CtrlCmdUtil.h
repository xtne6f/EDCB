#ifndef INCLUDE_CTRL_CMD_UTIL_H
#define INCLUDE_CTRL_CMD_UTIL_H

#include "StructDef.h"
#include <stdexcept>

namespace CtrlCmdUtilImpl_
{

const BYTE* ReadVectorIntro( const BYTE** buff, const BYTE** buffEnd, DWORD* valCount );
template<class T> DWORD CCUTIL_WriteVectorVALUE_( WORD ver, BYTE* buff, DWORD buffOffset, const vector<T>& val );
template<class T> DWORD CCUTIL_WritePtrVectorVALUE_( WORD ver, BYTE* buff, DWORD buffOffset, const vector<T>& val );
template<class T> bool CCUTIL_ReadVectorVALUE_( WORD ver, const BYTE** buff, const BYTE* buffEnd, vector<T>* val );

#define CCUTIL_VECTOR_WRITE_			return CCUTIL_WriteVectorVALUE_(ver, buff, buffOffset, val)
#define CCUTIL_VECTOR_WRITE_PTR_		return CCUTIL_WritePtrVectorVALUE_(ver, buff, buffOffset, val)
#define CCUTIL_VECTOR_READ_				return CCUTIL_ReadVectorVALUE_(ver, buff, buffEnd, val)

////////////////////////////////////////////////////////////////////////////////////////////
//コマンド送信用バイナリ作成関数

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, BYTE val ){ (void)ver; if( buff ) buff[buffOffset] = val; return 1; }
inline bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, BYTE* val ){ (void)ver; if( buffEnd - *buff < 1 ) return false; *val = *((*buff)++); return true; }

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, char val ){ return WriteVALUE(ver, buff, buffOffset, (BYTE)val); }
inline bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, char* val ){ BYTE v; if( !ReadVALUE(ver, buff, buffEnd, &v) ) return false; *val = v; return true; }

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, WORD val ){ (void)ver; if( buff ){ buff[buffOffset++] = (BYTE)val; buff[buffOffset] = (BYTE)(val >> 8); } return 2; }
inline bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, WORD* val ){ (void)ver; if( buffEnd - *buff < 2 ) return false; *val = **buff | (*buff)[1] << 8; *buff += 2; return true; }

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, short val ){ return WriteVALUE(ver, buff, buffOffset, (WORD)val); }
inline bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, short* val ){ WORD v; if( !ReadVALUE(ver, buff, buffEnd, &v) ) return false; *val = v; return true; }

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, DWORD val );
bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, DWORD* val );

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, int val ){ return WriteVALUE(ver, buff, buffOffset, (DWORD)val); }
inline bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, int* val ){ DWORD v; if( !ReadVALUE(ver, buff, buffEnd, &v) ) return false; *val = v; return true; }

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, ULONGLONG val );
bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, ULONGLONG* val );

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, LONGLONG val ){ return WriteVALUE(ver, buff, buffOffset, (ULONGLONG)val); }
inline bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, LONGLONG* val ){ ULONGLONG v; if( !ReadVALUE(ver, buff, buffEnd, &v) ) return false; *val = v; return true; }

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<WORD>& val ){ CCUTIL_VECTOR_WRITE_; }
inline bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, vector<WORD>* val ){ CCUTIL_VECTOR_READ_; }

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<DWORD>& val ){ CCUTIL_VECTOR_WRITE_; }
inline bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, vector<DWORD>* val ){ CCUTIL_VECTOR_READ_; }

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<LONGLONG>& val ){ CCUTIL_VECTOR_WRITE_; }
inline bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, vector<LONGLONG>* val ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const wstring& val, bool oldFormat = false );
bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, wstring* val, bool oldFormat = false );

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<wstring>& val ){ CCUTIL_VECTOR_WRITE_; }
inline bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, vector<wstring>* val ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const SYSTEMTIME& val );
bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, SYSTEMTIME* val );

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const FILE_DATA& val );
inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<FILE_DATA>& val ){ CCUTIL_VECTOR_WRITE_; }

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const REC_SETTING_DATA& val );
bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, REC_SETTING_DATA* val );

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const RESERVE_DATA& val );
bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, RESERVE_DATA* val );

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<RESERVE_DATA>& val ){ CCUTIL_VECTOR_WRITE_; }
inline bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, vector<RESERVE_DATA>* val ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_SERVICE_INFO& val );
bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, EPGDB_SERVICE_INFO* val );

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<EPGDB_SERVICE_INFO>& val ){ CCUTIL_VECTOR_WRITE_; }
inline bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, vector<EPGDB_SERVICE_INFO>* val ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_SHORT_EVENT_INFO& val );
bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, EPGDB_SHORT_EVENT_INFO* val );

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_EXTENDED_EVENT_INFO& val );
bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, EPGDB_EXTENDED_EVENT_INFO* val );

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_CONTENT_DATA& val );
bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, EPGDB_CONTENT_DATA* val );

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<EPGDB_CONTENT_DATA>& val ){ CCUTIL_VECTOR_WRITE_; }
inline bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, vector<EPGDB_CONTENT_DATA>* val ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_CONTEN_INFO& val );
bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, EPGDB_CONTEN_INFO* val );

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_COMPONENT_INFO& val );
bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, EPGDB_COMPONENT_INFO* val );

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_AUDIO_COMPONENT_INFO_DATA& val );
bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, EPGDB_AUDIO_COMPONENT_INFO_DATA* val );

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<EPGDB_AUDIO_COMPONENT_INFO_DATA>& val ){ CCUTIL_VECTOR_WRITE_; }
inline bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, vector<EPGDB_AUDIO_COMPONENT_INFO_DATA>* val ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_AUDIO_COMPONENT_INFO& val );
bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, EPGDB_AUDIO_COMPONENT_INFO* val );

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_EVENT_DATA& val );
bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, EPGDB_EVENT_DATA* val );

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<EPGDB_EVENT_DATA>& val ){ CCUTIL_VECTOR_WRITE_; }
inline bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, vector<EPGDB_EVENT_DATA>* val ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_EVENTGROUP_INFO& val, BYTE groupType );
bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, EPGDB_EVENTGROUP_INFO* val, BYTE* groupType );

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_EVENT_INFO& val );
bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, EPGDB_EVENT_INFO* val );

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<EPGDB_EVENT_INFO>& val ){ CCUTIL_VECTOR_WRITE_; }
inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<const EPGDB_EVENT_INFO*>& val ){ CCUTIL_VECTOR_WRITE_PTR_; }
inline bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, vector<EPGDB_EVENT_INFO>* val ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_SEARCH_DATE_INFO& val );
bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, EPGDB_SEARCH_DATE_INFO* val );

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<EPGDB_SEARCH_DATE_INFO>& val ){ CCUTIL_VECTOR_WRITE_; }
inline bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, vector<EPGDB_SEARCH_DATE_INFO>* val ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_SEARCH_KEY_INFO& val );
bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, EPGDB_SEARCH_KEY_INFO* val );

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<EPGDB_SEARCH_KEY_INFO>& val ){ CCUTIL_VECTOR_WRITE_; }
inline bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, vector<EPGDB_SEARCH_KEY_INFO>* val ){ CCUTIL_VECTOR_READ_; }

bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, SEARCH_PG_PARAM* val );

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const VIEW_APP_STATUS_INFO& val );
bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, VIEW_APP_STATUS_INFO* val );

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const SET_CH_INFO& val );
bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, SET_CH_INFO* val );

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<SET_CH_INFO>& val ){ CCUTIL_VECTOR_WRITE_; }
inline bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, vector<SET_CH_INFO>* val ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const SET_CTRL_MODE& val );
bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, SET_CTRL_MODE* val );

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const REC_FILE_SET_INFO& val );
bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, REC_FILE_SET_INFO* val );

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<REC_FILE_SET_INFO>& val ){ CCUTIL_VECTOR_WRITE_; }
inline bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, vector<REC_FILE_SET_INFO>* val ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const SET_CTRL_REC_PARAM& val );
bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, SET_CTRL_REC_PARAM* val );

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const SET_CTRL_REC_STOP_PARAM& val );
bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, SET_CTRL_REC_STOP_PARAM* val );

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const SET_CTRL_REC_STOP_RES_PARAM& val );
bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, SET_CTRL_REC_STOP_RES_PARAM* val );

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const REC_FILE_INFO& val );
bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, REC_FILE_INFO* val );

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<REC_FILE_INFO>& val ){ CCUTIL_VECTOR_WRITE_; }
inline bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, vector<REC_FILE_INFO>* val ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPG_AUTO_ADD_DATA& val );
bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, EPG_AUTO_ADD_DATA* val );

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<EPG_AUTO_ADD_DATA>& val ){ CCUTIL_VECTOR_WRITE_; }
inline bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, vector<EPG_AUTO_ADD_DATA>* val ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const SEARCH_EPG_INFO_PARAM& val );
bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, SEARCH_EPG_INFO_PARAM* val );

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const GET_EPG_PF_INFO_PARAM& val );
bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, GET_EPG_PF_INFO_PARAM* val );

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const MANUAL_AUTO_ADD_DATA& val );
bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, MANUAL_AUTO_ADD_DATA* val );

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<MANUAL_AUTO_ADD_DATA>& val ){ CCUTIL_VECTOR_WRITE_; }
inline bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, vector<MANUAL_AUTO_ADD_DATA>* val ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const TUNER_RESERVE_INFO& val );
inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<TUNER_RESERVE_INFO>& val ){ CCUTIL_VECTOR_WRITE_; }

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const TUNER_PROCESS_STATUS_INFO& val );
inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<TUNER_PROCESS_STATUS_INFO>& val ){ CCUTIL_VECTOR_WRITE_; }

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_SERVICE_EVENT_INFO& val );
DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const EPGDB_SERVICE_EVENT_INFO_PTR& val );
bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, EPGDB_SERVICE_EVENT_INFO* val );

inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<const EPGDB_SERVICE_EVENT_INFO*>& val ){ CCUTIL_VECTOR_WRITE_PTR_; }
inline DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const vector<EPGDB_SERVICE_EVENT_INFO_PTR>& val ){ CCUTIL_VECTOR_WRITE_; }
inline bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, vector<EPGDB_SERVICE_EVENT_INFO>* val ){ CCUTIL_VECTOR_READ_; }

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const TVTEST_CH_CHG_INFO& val );

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const NWPLAY_PLAY_INFO& val );
bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, NWPLAY_PLAY_INFO* val );

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const NWPLAY_POS_CMD& val );
bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, NWPLAY_POS_CMD* val );

bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, TVTEST_STREAMING_INFO* val );

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const NWPLAY_TIMESHIFT_INFO& val );

DWORD WriteVALUE( WORD ver, BYTE* buff, DWORD buffOffset, const NOTIFY_SRV_INFO& val );
bool ReadVALUE( WORD ver, const BYTE** buff, const BYTE* buffEnd, NOTIFY_SRV_INFO* val );

////////////////////////////////////////////////////////////////////////////////////////////
//テンプレート定義

template<class T>
DWORD CCUTIL_WriteVectorVALUE_( WORD ver, BYTE* buff, DWORD buffOffset, const vector<T>& val )
{
	DWORD pos = buffOffset + sizeof(DWORD) * 2;
	//リストの個数
	WriteVALUE(0, buff, buffOffset + sizeof(DWORD), (DWORD)val.size());
	//リストの中身
	for( size_t i = 0; i < val.size(); i++ ){
		pos += WriteVALUE(ver, buff, pos, val[i]);
	}
	//全体のサイズ
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

template<class T>
DWORD CCUTIL_WritePtrVectorVALUE_( WORD ver, BYTE* buff, DWORD buffOffset, const vector<T>& val )
{
	DWORD pos = buffOffset + sizeof(DWORD) * 2;
	//リストの個数
	WriteVALUE(0, buff, buffOffset + sizeof(DWORD), (DWORD)val.size());
	//リストの中身
	for( size_t i = 0; i < val.size(); i++ ){
		if( val[i] == NULL ){
			throw std::runtime_error("");
		}
		pos += WriteVALUE(ver, buff, pos, *val[i]);
	}
	//全体のサイズ
	WriteVALUE(0, buff, buffOffset, pos - buffOffset);
	return pos - buffOffset;
}

template<class T>
bool CCUTIL_ReadVectorVALUE_( WORD ver, const BYTE** buff, const BYTE* buffEnd, vector<T>* val )
{
	DWORD valCount;
	const BYTE* rb = ReadVectorIntro(buff, &buffEnd, &valCount);
	if( rb == NULL ){
		return false;
	}
	val->reserve(val->size() + valCount);

	for( DWORD i=0; i < valCount; i++ ){
		val->resize(val->size() + 1);
		if( !ReadVALUE(ver, &rb, buffEnd, &val->back()) ){
			val->pop_back();
			return false;
		}
	}
	return true;
}

}

template<class T>
bool UtilReadVALUE( T* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL ){
		return false;
	}
	const BYTE* rb = buff;
	if( CtrlCmdUtilImpl_::ReadVALUE(0, &rb, buff + buffSize, val) ){
		if( readSize ){
			*readSize = (DWORD)(rb - buff);
		}
		return true;
	}
	return false;
}

template<class T>
bool UtilReadVALUE2( WORD ver, T* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	if( val == NULL || buff == NULL ){
		return false;
	}
	//2未満のコマンドバージョンは2として扱う
	ver = max(ver, (WORD)2);
	const BYTE* rb = buff;
	if( CtrlCmdUtilImpl_::ReadVALUE(ver, &rb, buff + buffSize, val) ){
		if( readSize ){
			*readSize = (DWORD)(rb - buff);
		}
		return true;
	}
	return false;
}

template<class T>
bool UtilReadVALUE2WithVersion( WORD* ver, T* val, const BYTE* buff, DWORD buffSize, DWORD* readSize )
{
	DWORD readVerSize;
	if( UtilReadVALUE(ver, buff, buffSize, &readVerSize) &&
	    UtilReadVALUE2(*ver, val, buff + readVerSize, buffSize - readVerSize, readSize) ){
		if( readSize ){
			*readSize += readVerSize;
		}
		return true;
	}
	return false;
}

//コマンド送受信ストリーム
class CCmdStream
{
public:
	explicit CCmdStream(DWORD param = 0) {
		params[0] = param;
		params[1] = 0;
	}

	//送信時コマンド、受信時エラーコードをセットする
	void SetParam(DWORD param);

	//送信データをリサイズする
	void Resize(DWORD dataSize);

	//送信時コマンド、受信時エラーコード
	DWORD GetParam() const { return params[0]; }

	//送信データのサイズ
	DWORD GetDataSize() const { return params[1]; }

	//ストリーム全体のサイズ
	DWORD GetStreamSize() const { return 8 + params[1]; }

	//ストリーム全体
	const BYTE* GetStream() const { return buff.empty() ? (BYTE*)params : buff.data(); }

	//送信データ
	const BYTE* GetData() const { return GetStream() + 8; }

	//送信データ
	BYTE* GetData() { return (buff.empty() ? (BYTE*)params : buff.data()) + 8; }

	template<class T>
	void WriteVALUE(const T& val) {
		Resize(CtrlCmdUtilImpl_::WriteVALUE(0, NULL, 0, val));
		CtrlCmdUtilImpl_::WriteVALUE(0, GetData(), 0, val);
	}

	template<class T>
	void WriteVALUE2WithVersion(WORD ver, const T& val) {
		//2未満のコマンドバージョンは2として扱う
		ver = max(ver, (WORD)2);
		Resize(CtrlCmdUtilImpl_::WriteVALUE(0, NULL, 0, ver) + CtrlCmdUtilImpl_::WriteVALUE(ver, NULL, 0, val));
		CtrlCmdUtilImpl_::WriteVALUE(ver, GetData(), CtrlCmdUtilImpl_::WriteVALUE(0, GetData(), 0, ver), val);
	}

	template<class T>
	bool ReadVALUE(T* val) const {
		return UtilReadVALUE(val, GetData(), GetDataSize(), NULL);
	}

	template<class T>
	bool ReadVALUE2WithVersion(WORD* ver, T* val) const {
		return UtilReadVALUE2WithVersion(ver, val, GetData(), GetDataSize(), NULL);
	}

private:
	DWORD params[2];
	vector<BYTE> buff;
};

////////////////////////////////////////////////////////////////////////////////////////////
//旧バージョンコマンド送信用バイナリ作成関数
void DeprecatedNewWriteVALUE( const RESERVE_DATA& val, CCmdStream& cmd, BYTE* buff = NULL );
bool DeprecatedReadVALUE( RESERVE_DATA* val, const BYTE* buff, DWORD buffSize );
bool DeprecatedReadVALUE( EPG_AUTO_ADD_DATA* val, const BYTE* buff, DWORD buffSize );
void DeprecatedNewWriteVALUE( const EPGDB_EVENT_INFO& val, CCmdStream& cmd, BYTE* buff = NULL );

#endif
