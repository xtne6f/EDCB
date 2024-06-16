// IBonDriver2.h: IBonDriver2 クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include "IBonDriver.h"


// 凡ドライバインタフェース2
class IBonDriver2 : public IBonDriver
{
public:
#if WCHAR_MAX > 0xFFFF
	typedef WORD BON16CHAR;
#else
	typedef WCHAR BON16CHAR;
#endif

	virtual const BON16CHAR* GetTunerName(void) = 0;

	virtual const BOOL IsTunerOpening(void) = 0;
	
	virtual const BON16CHAR* EnumTuningSpace(const DWORD dwSpace) = 0;
	virtual const BON16CHAR* EnumChannelName(const DWORD dwSpace, const DWORD dwChannel) = 0;

	virtual const BOOL SetChannel(const DWORD dwSpace, const DWORD dwChannel) = 0;
	
	virtual const DWORD GetCurSpace(void) = 0;
	virtual const DWORD GetCurChannel(void) = 0;
	
// IBonDriver
	virtual void Release(void) = 0;
};

// IBonDriver2->C互換構造体
struct STRUCT_IBONDRIVER2
{
	STRUCT_IBONDRIVER st;
	const IBonDriver2::BON16CHAR* (*pF10)(void *);
	BOOL (*pF11)(void *);
	const IBonDriver2::BON16CHAR* (*pF12)(void *, DWORD);
	const IBonDriver2::BON16CHAR* (*pF13)(void *, DWORD, DWORD);
	BOOL (*pF14)(void *, DWORD, DWORD);
	DWORD (*pF15)(void *);
	DWORD (*pF16)(void *);
	STRUCT_IBONDRIVER &Initialize(IBonDriver2 *pBon2, const void *pEnd) {
		pF10 = F10;
		pF11 = F11;
		pF12 = F12;
		pF13 = F13;
		pF14 = F14;
		pF15 = F15;
		pF16 = F16;
		return st.Initialize(pBon2, pEnd ? pEnd : this + 1);
	}
	static const IBonDriver2::BON16CHAR* F10(void *p) { return static_cast<IBonDriver2 *>(static_cast<IBonDriver *>(p))->GetTunerName(); }
	static BOOL F11(void *p) { return static_cast<IBonDriver2 *>(static_cast<IBonDriver *>(p))->IsTunerOpening(); }
	static const IBonDriver2::BON16CHAR* F12(void *p, DWORD a0) { return static_cast<IBonDriver2 *>(static_cast<IBonDriver *>(p))->EnumTuningSpace(a0); }
	static const IBonDriver2::BON16CHAR* F13(void *p, DWORD a0, DWORD a1) { return static_cast<IBonDriver2 *>(static_cast<IBonDriver *>(p))->EnumChannelName(a0, a1); }
	static BOOL F14(void *p, DWORD a0, DWORD a1) { return static_cast<IBonDriver2 *>(static_cast<IBonDriver *>(p))->SetChannel(a0, a1); }
	static DWORD F15(void *p) { return static_cast<IBonDriver2 *>(static_cast<IBonDriver *>(p))->GetCurSpace(); }
	static DWORD F16(void *p) { return static_cast<IBonDriver2 *>(static_cast<IBonDriver *>(p))->GetCurChannel(); }
};

#define DEFINE_BON_STRUCT2_ADAPTER(st, p) \
	const BON16CHAR* GetTunerName() { return st.pF10(p); } \
	const BOOL IsTunerOpening() { return st.pF11(p); } \
	const BON16CHAR* EnumTuningSpace(const DWORD dwSpace) { return st.pF12(p, dwSpace); } \
	const BON16CHAR* EnumChannelName(const DWORD dwSpace, const DWORD dwChannel) { return st.pF13(p, dwSpace, dwChannel); } \
	const BOOL SetChannel(const DWORD dwSpace, const DWORD dwChannel) { return st.pF14(p, dwSpace, dwChannel); } \
	const DWORD GetCurSpace() { return st.pF15(p); } \
	const DWORD GetCurChannel() { return st.pF16(p); }

// C互換構造体->IBonDriver2
class CBonStruct2Adapter : public IBonDriver2
{
public:
	bool Adapt(const STRUCT_IBONDRIVER &st) {
		if (static_cast<const char *>(st.pEnd) - reinterpret_cast<const char *>(&st) < static_cast<int>(sizeof(st2))) {
			return false;
		}
		st2 = reinterpret_cast<const STRUCT_IBONDRIVER2 &>(st);
		return true;
	}
	DEFINE_BON_STRUCT_ADAPTER(st2.st, st2.st.pCtx);
	DEFINE_BON_STRUCT2_ADAPTER(st2, st2.st.pCtx);
protected:
	STRUCT_IBONDRIVER2 st2;
};
