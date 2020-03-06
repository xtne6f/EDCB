#include <sdkddkver.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "../../../BonCtrl/IB25Decoder.h"
#include "../../../BonCtrl/IBonDriver2.h"

extern "C" __declspec(dllexport) const LPVOID *WINAPI Cast(LPCSTR pType, void *pBase)
{
	if (lstrcmpA(pType, "IBonDriver@10") == 0 || lstrcmpA(pType, "IBonDriver2@17") == 0) {
		static const LPVOID t[] = {
			STRUCT_IBONDRIVER::F00,
			STRUCT_IBONDRIVER::F01,
			STRUCT_IBONDRIVER::F02,
			STRUCT_IBONDRIVER::F03,
			STRUCT_IBONDRIVER::F04,
			STRUCT_IBONDRIVER::F05,
			STRUCT_IBONDRIVER::F06,
			STRUCT_IBONDRIVER::F07,
			STRUCT_IBONDRIVER::F08,
			STRUCT_IBONDRIVER::F09,
			STRUCT_IBONDRIVER2::F10,
			STRUCT_IBONDRIVER2::F11,
			STRUCT_IBONDRIVER2::F12,
			STRUCT_IBONDRIVER2::F13,
			STRUCT_IBONDRIVER2::F14,
			STRUCT_IBONDRIVER2::F15,
			STRUCT_IBONDRIVER2::F16
		};
		if (lstrcmpA(pType, "IBonDriver2@17") == 0) {
			if (dynamic_cast<IBonDriver2*>(static_cast<IBonDriver*>(pBase)) == nullptr) {
				return nullptr;
			}
		}
		return t;
	}

	if (lstrcmpA(pType, "IB25Decoder@5") == 0 || lstrcmpA(pType, "IB25Decoder2@20") == 0) {
		struct S {
			static BOOL F00(void *p, DWORD a0) { return static_cast<IB25Decoder*>(p)->Initialize(a0); }
			static void F01(void *p) { static_cast<IB25Decoder*>(p)->Release(); }
			static BOOL F02(void *p, BYTE *a0, DWORD a1, BYTE **a2, DWORD *a3) { return static_cast<IB25Decoder*>(p)->Decode(a0, a1, a2, a3); }
			static BOOL F03(void *p, BYTE **a0, DWORD *a1) { return static_cast<IB25Decoder*>(p)->Flush(a0, a1); }
			static BOOL F04(void *p) { return static_cast<IB25Decoder*>(p)->Reset(); }
			static void F05(void *p, BOOL a0) { static_cast<IB25Decoder2*>(p)->DiscardNullPacket(a0 != 0); }
			static void F06(void *p, BOOL a0) { static_cast<IB25Decoder2*>(p)->DiscardScramblePacket(a0 != 0); }
			static void F07(void *p, BOOL a0) { static_cast<IB25Decoder2*>(p)->EnableEmmProcess(a0 != 0); }
			static DWORD F08(void *p, WORD a0) { return static_cast<IB25Decoder2*>(p)->GetDescramblingState(a0); }
			static void F09(void *p) { static_cast<IB25Decoder2*>(p)->ResetStatistics(); }
			static DWORD F10(void *p) { return static_cast<IB25Decoder2*>(p)->GetPacketStride(); }
			static DWORD F11(void *p, WORD a0) { return static_cast<IB25Decoder2*>(p)->GetInputPacketNum(a0); }
			static DWORD F12(void *p, WORD a0) { return static_cast<IB25Decoder2*>(p)->GetOutputPacketNum(a0); }
			static DWORD F13(void *p) { return static_cast<IB25Decoder2*>(p)->GetSyncErrNum(); }
			static DWORD F14(void *p) { return static_cast<IB25Decoder2*>(p)->GetFormatErrNum(); }
			static DWORD F15(void *p) { return static_cast<IB25Decoder2*>(p)->GetTransportErrNum(); }
			static DWORD F16(void *p, WORD a0) { return static_cast<IB25Decoder2*>(p)->GetContinuityErrNum(a0); }
			static DWORD F17(void *p, WORD a0) { return static_cast<IB25Decoder2*>(p)->GetScramblePacketNum(a0); }
			static DWORD F18(void *p) { return static_cast<IB25Decoder2*>(p)->GetEcmProcessNum(); }
			static DWORD F19(void *p) { return static_cast<IB25Decoder2*>(p)->GetEmmProcessNum(); }
		};
		static const LPVOID t[] = {
			S::F00, S::F01, S::F02, S::F03, S::F04, S::F05, S::F06, S::F07, S::F08, S::F09,
			S::F10, S::F11, S::F12, S::F13, S::F14, S::F15, S::F16, S::F17, S::F18, S::F19
		};
		if (lstrcmpA(pType, "IB25Decoder2@20") == 0) {
			if (dynamic_cast<IB25Decoder2*>(static_cast<IB25Decoder*>(pBase)) == nullptr) {
				return nullptr;
			}
		}
		return t;
	}

	return nullptr;
}
