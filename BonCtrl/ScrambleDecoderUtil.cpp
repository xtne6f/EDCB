#include "stdafx.h"
#include "ScrambleDecoderUtil.h"

namespace
{
IB25Decoder* CastB(IB25Decoder2** if2, IB25Decoder* (*funcCreate)(), const LPVOID* (WINAPI* funcCast)(LPCSTR, void*))
{
	HMODULE hModule = NULL;
#ifndef _MSC_VER
	if( funcCast == NULL ){
		if( (hModule = LoadLibrary(L"IBonCast.dll")) == NULL ){
			AddDebugLog(L"★IBonCast.dllがロードできません");
			return NULL;
		}
		funcCast = (const LPVOID*(WINAPI*)(LPCSTR,void*))GetProcAddress(hModule, "Cast");
	}
#endif
	void* pBase;
	const LPVOID* table;
	if( funcCast == NULL || (pBase = funcCreate()) == NULL || (table = funcCast("IB25Decoder@5", pBase)) == NULL ){
		AddDebugLog(L"★Castに失敗しました");
#ifndef _MSC_VER
		if( hModule ){
			FreeLibrary(hModule);
		}
#endif
		return NULL;
	}

	class CCastB : public IB25Decoder2
	{
	public:
		CCastB(HMODULE h_, void* p_, const LPVOID* t_) : h(h_), p(p_), t(t_) {}
		const BOOL Initialize(DWORD dwRound = 4) { return ((BOOL(*)(void*,DWORD))t[0])(p, dwRound); }
		void Release() { ((void(*)(void*))t[1])(p); if( h ) FreeLibrary(h); delete this; }
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
		HMODULE h;
		void* p;
		const LPVOID* t;
	};

	CCastB* b = new CCastB(hModule, pBase, table);
	*if2 = NULL;
	if( funcCast("IB25Decoder2@20", pBase) == table ){
		*if2 = b;
	}
	return b;
}
}

CScrambleDecoderUtil::CScrambleDecoderUtil(void)
{
	this->currentDll = L"";

	this->decodeIF = NULL;
	this->decodeIF2 = NULL;
	this->module = NULL;

	this->emmEnable = false;
}

CScrambleDecoderUtil::~CScrambleDecoderUtil(void)
{
	UnLoadDll();
}

BOOL CScrambleDecoderUtil::LoadDll(LPCWSTR dllPath)
{
	UnLoadDll();
	BOOL ret = TRUE;

	this->module = ::LoadLibrary(dllPath);
	if( this->module == NULL ){
		return FALSE;
	}
	IB25Decoder* (*func)();
	func = (IB25Decoder* (*)())::GetProcAddress( this->module, "CreateB25Decoder");
	if( !func ){
		ret = FALSE;
		goto ERR_END;
	}
	const LPVOID* (WINAPI* funcCast)(LPCSTR, void*);
	funcCast = (const LPVOID*(WINAPI*)(LPCSTR,void*))GetProcAddress(this->module, "Cast");
#ifdef _MSC_VER
	if( !funcCast ){
		this->decodeIF = func();
	}else
#endif
	if( (this->decodeIF = CastB(&this->decodeIF2, func, funcCast)) == NULL ){
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
			if( this->decodeIF2 != NULL ){
				//this->decodeIF2->EnableEmmProcess(false);
				this->decodeIF2->DiscardNullPacket(true);
				this->decodeIF2->DiscardScramblePacket(false);
				this->decodeIF2->EnableEmmProcess(this->emmEnable);
			}
		}
#ifdef _MSC_VER
		catch(std::__non_rtti_object){
			this->decodeIF2 = NULL;
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
	if( this->decodeIF != NULL ){
		this->decodeIF->Release();
		this->decodeIF = NULL;
		this->decodeIF2 = NULL;
	}
	if( this->module != NULL ){
		::FreeLibrary( this->module );
		this->module = NULL;
	}
	this->currentDll = L"";
}

BOOL CScrambleDecoderUtil::SetNetwork(WORD ONID, WORD TSID)
{
	BOOL ret = FALSE;
	fs_path iniPath = GetCommonIniPath().replace_filename(L"BonCtrl.ini");

	wstring defKey = L"FFFFFFFF";
	wstring networkDefKey = L"";
	wstring key = L"";
	Format(networkDefKey, L"%04XFFFF", ONID);
	Format(key, L"%04X%04X", ONID, TSID);

	wstring defDll = GetPrivateProfileToString(L"SET", defKey.c_str(), L"", iniPath.c_str());
	wstring networkDefDll = GetPrivateProfileToString(L"SET", networkDefKey.c_str(), L"", iniPath.c_str());
	wstring loadDll = GetPrivateProfileToString(L"SET", key.c_str(), L"", iniPath.c_str());

	wstring dllPath = GetModulePath().parent_path().native();
	if( loadDll.size() > 0 ){
		dllPath += L"\\";
		dllPath += loadDll;
		this->loadDll = loadDll;
	}else if( networkDefDll.size() > 0 ){
		dllPath += L"\\";
		dllPath += networkDefDll;
		this->loadDll = networkDefDll;
	}else if( defDll.size() > 0 ){
		dllPath += L"\\";
		dllPath += defDll;
		this->loadDll = defDll;
	}else{
		dllPath += L"\\B25Decoder.dll";
		this->loadDll = L"B25Decoder.dll";
	}

	if( CompareNoCase(dllPath, this->currentDll) != 0 ){
		if( LoadDll(dllPath.c_str()) == FALSE ){
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
	if( this->decodeIF == NULL ){
		return FALSE;
	}
	return this->decodeIF->Decode(src, srcSize, dest, destSize);
}

void CScrambleDecoderUtil::Reset()
{
	if( this->decodeIF == NULL ){
		return ;
	}
	this->decodeIF->Reset();
}


BOOL CScrambleDecoderUtil::SetEmm(BOOL enable)
{
	if( this->decodeIF2 == NULL ){
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
	if( this->decodeIF2 == NULL ){
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

