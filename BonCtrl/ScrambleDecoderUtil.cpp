#include "stdafx.h"
#include "ScrambleDecoderUtil.h"


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
	this->decodeIF = func();
	if( this->decodeIF->Initialize() == FALSE ){
		this->decodeIF->Release();
		this->decodeIF = NULL;
		ret = FALSE;
	}else{
		try{
			this->decodeIF2 = dynamic_cast<IB25Decoder2 *>(this->decodeIF);
			if( this->decodeIF2 != NULL ){
				//this->decodeIF2->EnableEmmProcess(false);
				this->decodeIF2->DiscardNullPacket(true);
				this->decodeIF2->DiscardScramblePacket(false);
				this->decodeIF2->EnableEmmProcess(this->emmEnable);
			}
		}
		catch(std::__non_rtti_object){
			this->decodeIF2 = NULL;
		}
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
	wstring folderPath;
	GetModuleFolderPath( folderPath );

	wstring iniPath = folderPath;
	iniPath += L"\\BonCtrl.ini";

	wstring defKey = L"FFFFFFFF";
	wstring networkDefKey = L"";
	wstring key = L"";
	Format(networkDefKey, L"%04XFFFF", ONID);
	Format(key, L"%04X%04X", ONID, TSID);

	wstring defDll = L"";
	wstring networkDefDll = L"";
	wstring loadDll = L"";
	WCHAR buff[512]=L"";
	GetPrivateProfileString( L"SET", defKey.c_str(), L"", buff, 512, iniPath.c_str() );
	defDll = buff;
	GetPrivateProfileString( L"SET", networkDefKey.c_str(), L"", buff, 512, iniPath.c_str() );
	networkDefDll = buff;
	GetPrivateProfileString( L"SET", key.c_str(), L"", buff, 512, iniPath.c_str() );
	loadDll = buff;

	wstring dllPath = folderPath;
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
			_OutputDebugString(L"š%s ‚Ìƒ[ƒh‚ÉŽ¸”s‚µ‚Ü‚µ‚½B\r\n", dllPath.c_str());
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

