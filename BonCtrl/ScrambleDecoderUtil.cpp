#include "stdafx.h"
#include "ScrambleDecoderUtil.h"

namespace
{
IB25Decoder* CastB(IB25Decoder2** if2, IB25Decoder* (*funcCreate)(), void* const* (WINAPI* funcCast)(LPCSTR, void*))
{
	void* hModule = nullptr;
#if defined(_WIN32) && !defined(_MSC_VER)
	if( funcCast == nullptr ){
		if( (hModule = UtilLoadLibrary(wstring(L"IBonCast.dll"))) == nullptr ){
			AddDebugLog(L"★IBonCast.dllがロードできません");
			return nullptr;
		}
		UtilGetProcAddress(hModule, "Cast", funcCast);
	}
#endif
	void* pBase;
	void* const* table;
	if( funcCast == nullptr || (pBase = funcCreate()) == nullptr || (table = funcCast("IB25Decoder@5", pBase)) == nullptr ){
		AddDebugLog(L"★Castに失敗しました");
#if defined(_WIN32) && !defined(_MSC_VER)
		if( hModule ){
			UtilFreeLibrary(hModule);
		}
#endif
		return nullptr;
	}

	class CCastB : public IB25Decoder2
	{
	public:
		CCastB(void* h_, void* p_, void* const* t_) : h(h_), p(p_), t(t_) {}
		const BOOL Initialize(DWORD dwRound = 4) { return ((BOOL(*)(void*,DWORD))t[0])(p, dwRound); }
		void Release() { ((void(*)(void*))t[1])(p); if( h ) UtilFreeLibrary(h); delete this; }
		const BOOL Decode(BYTE* pSrcBuf, const DWORD dwSrcSize, BYTE** ppDstBuf, DWORD* pdwDstSize) { return ((BOOL(*)(void*,BYTE*,DWORD,BYTE**,DWORD*))t[2])(p, pSrcBuf, dwSrcSize, ppDstBuf, pdwDstSize); }
		const BOOL Flush(BYTE** ppDstBuf, DWORD* pdwDstSize) { return ((BOOL(*)(void*,BYTE**,DWORD*))t[3])(p, ppDstBuf, pdwDstSize); }
		const BOOL Reset() { return ((BOOL(*)(void*))t[4])(p); }
		void DiscardNullPacket(const bool bEnable = true) { ((void(*)(void*,BOOL))t[5])(p, bEnable); }
		void DiscardScramblePacket(const bool bEnable = true) { ((void(*)(void*,BOOL))t[6])(p,bEnable); }
		void EnableEmmProcess(const bool bEnable = true) { ((void(*)(void*,BOOL))t[7])(p,bEnable); }
		const DWORD GetDescramblingState(const WORD wProgramID) const { return ((DWORD(*)(void*,WORD))t[8])(p, wProgramID); }
		void ResetStatistics() { ((void(*)(void*))t[9])(p); }
		const DWORD GetPacketStride() const { return ((DWORD(*)(void*))t[10])(p); }
		const DWORD GetInputPacketNum(const WORD wPID = TS_INVALID_PID) const { return ((DWORD(*)(void*,WORD))t[11])(p, wPID); }
		const DWORD GetOutputPacketNum(const WORD wPID = TS_INVALID_PID) const { return ((DWORD(*)(void*,WORD))t[12])(p, wPID); }
		const DWORD GetSyncErrNum() const { return ((DWORD(*)(void*))t[13])(p); }
		const DWORD GetFormatErrNum() const { return ((DWORD(*)(void*))t[14])(p); }
		const DWORD GetTransportErrNum() const { return ((DWORD(*)(void*))t[15])(p); }
		const DWORD GetContinuityErrNum(const WORD wPID = TS_INVALID_PID) const { return ((DWORD(*)(void*,WORD))t[16])(p, wPID); }
		const DWORD GetScramblePacketNum(const WORD wPID = TS_INVALID_PID) const { return ((DWORD(*)(void*,WORD))t[17])(p, wPID); }
		const DWORD GetEcmProcessNum() const { return ((DWORD(*)(void*))t[18])(p); }
		const DWORD GetEmmProcessNum() const { return ((DWORD(*)(void*))t[19])(p); }
	private:
		void* h;
		void* p;
		void* const* t;
	};

	CCastB* b = new CCastB(hModule, pBase, table);
	*if2 = nullptr;
	if( funcCast("IB25Decoder2@20", pBase) == table ){
		*if2 = b;
	}
	return b;
}
}

CScrambleDecoderUtil::CScrambleDecoderUtil(void)
	: module(nullptr, UtilFreeLibrary)
{
	this->currentDll = L"";

	this->decodeIF = nullptr;
	this->decodeIF2 = nullptr;

	this->emmEnable = false;
}

CScrambleDecoderUtil::~CScrambleDecoderUtil(void)
{
	UnLoadDll();
}

BOOL CScrambleDecoderUtil::LoadDll(const wstring& dllPath)
{
	UnLoadDll();
	BOOL ret = TRUE;

	this->module.reset(UtilLoadLibrary(dllPath));
	if( this->module == nullptr ){
		return FALSE;
	}
	IB25Decoder* (*func)();
	if( UtilGetProcAddress(this->module.get(), "CreateB25Decoder", func) == false ){
		ret = FALSE;
		goto ERR_END;
	}
	void* const* (WINAPI* funcCast)(LPCSTR, void*);
	UtilGetProcAddress(this->module.get(), "Cast", funcCast);
#if !defined(_WIN32) || defined(_MSC_VER)
	if( !funcCast ){
		this->decodeIF = func();
	}else
#endif
	if( (this->decodeIF = CastB(&this->decodeIF2, func, funcCast)) == nullptr ){
		ret = FALSE;
		goto ERR_END;
	}
	if( this->decodeIF->Initialize() == FALSE ){
		ret = FALSE;
	}else{
#ifndef _MSC_VER
		{
#else
		try{
			if( !funcCast ){
				this->decodeIF2 = dynamic_cast<IB25Decoder2 *>(this->decodeIF);
			}
#endif
			if( this->decodeIF2 != nullptr ){
				//this->decodeIF2->EnableEmmProcess(false);
				this->decodeIF2->DiscardNullPacket(true);
				this->decodeIF2->DiscardScramblePacket(false);
				this->decodeIF2->EnableEmmProcess(this->emmEnable);
			}
		}
#ifdef _MSC_VER
		catch(std::__non_rtti_object){
			this->decodeIF2 = nullptr;
		}
#endif
	}
ERR_END:
	if( ret == FALSE ){
		UnLoadDll();
	}

	return ret;
}

void CScrambleDecoderUtil::UnLoadDll()
{
	if( this->decodeIF != nullptr ){
		this->decodeIF->Release();
		this->decodeIF = nullptr;
		this->decodeIF2 = nullptr;
	}
	this->module.reset();
	this->currentDll = L"";
}

BOOL CScrambleDecoderUtil::SetNetwork(WORD ONID, WORD TSID)
{
	BOOL ret = FALSE;
	fs_path iniPath = GetCommonIniPath().replace_filename(L"BonCtrl.ini");

	this->loadDll = L"";
	for( int i = 0; i < 3 && this->loadDll.empty(); i++ ){
		WCHAR key[32];
		swprintf_s(key, L"%04X%04X", i > 1 ? 0xFFFF : ONID, i > 0 ? 0xFFFF : TSID);
		this->loadDll = GetPrivateProfileToString(L"SET", key, L"", iniPath.c_str());
	}
	if( this->loadDll.empty() ){
		this->loadDll = L"B25Decoder" EDCB_LIB_EXT;
	}

#ifdef EDCB_LIB_ROOT
	wstring dllPath = fs_path(EDCB_LIB_ROOT).append(this->loadDll).native();
#else
	wstring dllPath = GetModulePath().replace_filename(this->loadDll).native();
#endif

	if( UtilComparePath(dllPath.c_str(), this->currentDll.c_str()) != 0 ){
		if( LoadDll(dllPath) == FALSE ){
			AddDebugLogFormat(L"★%ls のロードに失敗しました。", dllPath.c_str());
			this->currentDll = L"";
		}else{
			this->currentDll = dllPath;
			ret = TRUE;
		}
	}

	return ret;
}

BOOL CScrambleDecoderUtil::Decode(BYTE* src, DWORD srcSize, BYTE** dest, DWORD* destSize)
{
	if( this->decodeIF == nullptr ){
		return FALSE;
	}
	return this->decodeIF->Decode(src, srcSize, dest, destSize);
}

void CScrambleDecoderUtil::Reset()
{
	if( this->decodeIF == nullptr ){
		return ;
	}
	this->decodeIF->Reset();
}


BOOL CScrambleDecoderUtil::SetEmm(BOOL enable)
{
	if( this->decodeIF2 == nullptr ){
		return FALSE;
	}
	if( enable == TRUE ){
		this->decodeIF2->EnableEmmProcess(true);
		this->emmEnable = true;
	}else{
		this->decodeIF2->EnableEmmProcess(false);
		this->emmEnable = false;
	}
	return TRUE;
}

DWORD CScrambleDecoderUtil::GetEmmCount()
{
	if( this->decodeIF2 == nullptr ){
		return 0;
	}
	return this->decodeIF2->GetEmmProcessNum();
}

BOOL CScrambleDecoderUtil::GetLoadStatus(wstring& loadErrDll)
{
	if( this->currentDll.size() == 0 ){
		loadErrDll = this->loadDll;
		return FALSE;
	}else{
		return TRUE;
	}
}

