#pragma once

#include "../Common/PathUtil.h"
#include "../Common/ErrDef.h"

#include "IB25Decoder.h"

class CScrambleDecoderUtil
{
public:
	CScrambleDecoderUtil(void);
	~CScrambleDecoderUtil(void);

	void UnLoadDll();

	BOOL SetNetwork(WORD ONID, WORD TSID);

	BOOL Decode(BYTE* src, DWORD srcSize, BYTE** dest, DWORD* destSize);

	BOOL SetEmm(BOOL enable);

	DWORD GetEmmCount();

	BOOL GetLoadStatus(wstring& loadErrDll);

	void Reset();
protected:
	wstring currentDll;
	wstring loadDll;

	IB25Decoder* decodeIF;
	IB25Decoder2* decodeIF2;
	std::unique_ptr<void, decltype(&UtilFreeLibrary)> module;

	bool emmEnable;

protected:
	BOOL LoadDll(const wstring& dllPath);
};

