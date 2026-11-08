#pragma once

#include "../Common/PathUtil.h"
#include "IB25Decoder.h"

class CScrambleDecoderUtil
{
public:
	CScrambleDecoderUtil(void);
	~CScrambleDecoderUtil(void);

	void UnLoadDll();

	bool SetNetwork(WORD ONID, WORD TSID);

	bool Decode(BYTE* src, DWORD srcSize, BYTE** dest, DWORD* destSize);

	bool SetEmm(bool enable);

	DWORD GetEmmCount();

	bool GetLoadStatus(wstring& loadErrDll);

	void Reset();
protected:
	wstring currentDll;
	wstring loadDll;

	IB25Decoder* decodeIF;
	IB25Decoder2* decodeIF2;
	std::unique_ptr<void, decltype(&UtilFreeLibrary)> module;

	bool emmEnable;

protected:
	bool LoadDll(const wstring& dllPath);
};

