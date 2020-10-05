#include "stdafx.h"
#include "BonDriverUtil.h"
#include "../Common/PathUtil.h"
#include "../Common/StringUtil.h"
#include "IBonDriver2.h"
#include <objbase.h>

namespace
{
enum {
	WM_APP_GET_TS_STREAM = WM_APP,
	WM_APP_GET_STATUS,
	WM_APP_SET_CH,
	WM_APP_GET_NOW_CH,
};

#ifndef _MSC_VER
IBonDriver* CastB(IBonDriver2** if2, IBonDriver* (*funcCreate)())
{
	HMODULE hModule = LoadLibrary(L"IBonCast.dll");
	if( hModule == NULL ){
		AddDebugLog(L"★IBonCast.dllがロードできません");
		return NULL;
	}
	const LPVOID* (WINAPI* funcCast)(LPCSTR, void*) = (const LPVOID*(WINAPI*)(LPCSTR,void*))GetProcAddress(hModule, "Cast");
	void* pBase;
	const LPVOID* table;
	if( funcCast == NULL || (pBase = funcCreate()) == NULL || (table = funcCast("IBonDriver@10", pBase)) == NULL ){
		AddDebugLog(L"★Castに失敗しました");
		FreeLibrary(hModule);
		return NULL;
	}

	class CCastB : public CBonStruct2Adapter
	{
	public:
		CCastB(HMODULE h_, void* p, const LPVOID* t, const LPVOID* t2) : h(h_) {
			st2.st.pCtx = p;
			//アダプタの関数ポインタフィールドを上書き
			memcpy(&st2.st.pF00, t, sizeof(LPVOID) * 10);
			if( t2 ) memcpy(&st2.pF10, t2, sizeof(LPVOID) * 7);
		}
		void Release() {
			CBonStruct2Adapter::Release();
			FreeLibrary(h);
			delete this;
		}
	private:
		HMODULE h;
	};

	if( funcCast("IBonDriver2@17", pBase) == table ){
		*if2 = new CCastB(hModule, pBase, table, table + 10);
		return *if2;
	}
	//IBonDriver2部分は未初期化なのでダウンキャストしてはならない
	*if2 = NULL;
	return new CCastB(hModule, pBase, table, NULL);
}
#endif
}

CBonDriverUtil::CInit CBonDriverUtil::s_init;

CBonDriverUtil::CInit::CInit()
{
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(wc);
	wc.lpfnWndProc = DriverWindowProc;
	wc.hInstance = GetModuleHandle(NULL);
	wc.lpszClassName = L"BonDriverUtilWorker";
	RegisterClassEx(&wc);
}

CBonDriverUtil::CBonDriverUtil(void)
	: hwndDriver(NULL)
{
}

CBonDriverUtil::~CBonDriverUtil(void)
{
	CloseBonDriver();
}

bool CBonDriverUtil::OpenBonDriver(LPCWSTR bonDriverFolder, LPCWSTR bonDriverFile,
                                   const std::function<void(BYTE*, DWORD, DWORD)>& recvFunc_,
                                   const std::function<void(float, int, int)>& statusFunc_, int traceLevel_)
{
	CloseBonDriver();
	this->loadDllFolder = bonDriverFolder;
	this->loadDllFileName = bonDriverFile;
	if( this->loadDllFolder.empty() == false && this->loadDllFileName.empty() == false ){
		this->recvFunc = recvFunc_;
		this->statusFunc = statusFunc_;
		this->traceLevel = max(traceLevel_, 0);
		if( this->traceLevel ){
			this->callingName = L"Opening";
			this->callingTick = GetTickCount();
			this->statGetTsCalls = 0;
			this->statGetTsBytes = 0;
			this->watchdogStopEvent.Reset();
			this->watchdogThread = thread_(WatchdogThread, this);
		}
		this->driverThread = thread_(DriverThread, this);
		//Open処理が完了するまで待つ
		while( WaitForSingleObject(this->driverThread.native_handle(), 10) == WAIT_TIMEOUT ){
			CBlockLock lock(&this->utilLock);
			if( this->hwndDriver ){
				break;
			}
		}
		if( this->hwndDriver ){
			return true;
		}
		this->driverThread.join();
		if( this->watchdogThread.joinable() ){
			this->watchdogStopEvent.Set();
			this->watchdogThread.join();
		}
	}
	return false;
}

void CBonDriverUtil::CloseBonDriver()
{
	if( this->hwndDriver ){
		PostMessage(this->hwndDriver, WM_CLOSE, 0, 0);
		this->driverThread.join();
		if( this->watchdogThread.joinable() ){
			this->watchdogStopEvent.Set();
			this->watchdogThread.join();
		}
		this->loadChList.clear();
		this->loadTunerName.clear();
		CBlockLock lock(&this->utilLock);
		this->hwndDriver = NULL;
	}
}

void CBonDriverUtil::DriverThread(CBonDriverUtil* sys)
{
	//BonDriverがCOMを利用するかもしれないため
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	IBonDriver* bonIF = NULL;
	sys->bon2IF = NULL;
	CBonStructAdapter bonAdapter;
	CBonStruct2Adapter bon2Adapter;
	HMODULE hModule = LoadLibrary(fs_path(sys->loadDllFolder).append(sys->loadDllFileName).c_str());
	if( hModule == NULL ){
		AddDebugLog(L"★BonDriverがロードできません");
	}else{
		const STRUCT_IBONDRIVER* (*funcCreateBonStruct)() = (const STRUCT_IBONDRIVER*(*)())GetProcAddress(hModule, "CreateBonStruct");
		if( funcCreateBonStruct ){
			//特定コンパイラに依存しないI/Fを使う
			const STRUCT_IBONDRIVER* st = funcCreateBonStruct();
			if( st ){
				if( bon2Adapter.Adapt(*st) ){
					bonIF = sys->bon2IF = &bon2Adapter;
				}else{
					bonAdapter.Adapt(*st);
					bonIF = &bonAdapter;
				}
			}
		}else{
			IBonDriver* (*funcCreateBonDriver)() = (IBonDriver*(*)())GetProcAddress(hModule, "CreateBonDriver");
			if( funcCreateBonDriver == NULL ){
				AddDebugLog(L"★GetProcAddressに失敗しました");
			}else{
#ifdef _MSC_VER
				if( (bonIF = funcCreateBonDriver()) != NULL ){
					sys->bon2IF = dynamic_cast<IBonDriver2*>(bonIF);
				}
#else
				//MSVC++オブジェクトを変換する
				bonIF = CastB(&sys->bon2IF, funcCreateBonDriver);
#endif
			}
		}
		if( sys->bon2IF ){
			if( sys->bon2IF->OpenTuner() == FALSE ){
				AddDebugLog(L"★OpenTunerに失敗しました");
			}else{
				sys->initChSetFlag = false;
				//チューナー名の取得
				LPCWSTR tunerName = sys->bon2IF->GetTunerName();
				sys->loadTunerName = tunerName ? tunerName : L"";
				Replace(sys->loadTunerName, L"(",L"（");
				Replace(sys->loadTunerName, L")",L"）");
				//チャンネル一覧の取得
				sys->loadChList.clear();
				for( DWORD countSpace = 0; ; countSpace++ ){
					LPCWSTR spaceName = sys->bon2IF->EnumTuningSpace(countSpace);
					if( spaceName == NULL ){
						break;
					}
					sys->loadChList.push_back(pair<wstring, vector<wstring>>(spaceName, vector<wstring>()));
					for( DWORD countCh = 0; ; countCh++ ){
						LPCWSTR chName = sys->bon2IF->EnumChannelName(countSpace, countCh);
						if( chName == NULL ){
							break;
						}
						sys->loadChList.back().second.push_back(chName);
					}
				}
				HWND hwnd = CreateWindow(L"BonDriverUtilWorker", NULL, WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, HWND_MESSAGE, NULL, GetModuleHandle(NULL), sys);
				if( hwnd == NULL ){
					sys->loadChList.clear();
					sys->loadTunerName.clear();
					sys->bon2IF->CloseTuner();
				}else{
					CBlockLock lock(&sys->utilLock);
					sys->hwndDriver = hwnd;
				}
			}
		}
	}
	if( sys->hwndDriver == NULL ){
		//Openできなかった
		if( bonIF ){
			bonIF->Release();
		}
		if( hModule ){
			FreeLibrary(hModule);
		}
		CoUninitialize();
		return;
	}
	//割り込み遅延への耐性はBonDriverのバッファ能力に依存するので、相対優先順位を上げておく
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

	//メッセージループ
	MSG msg;
	while( GetMessage(&msg, NULL, 0, 0) > 0 ){
		DispatchMessage(&msg);
	}
	sys->bon2IF->CloseTuner();
	bonIF->Release();
	FreeLibrary(hModule);

	CoUninitialize();
}

LRESULT CALLBACK CBonDriverUtil::DriverWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CBonDriverUtil* sys = (CBonDriverUtil*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	if( uMsg != WM_CREATE && sys == NULL ){
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
	switch( uMsg ){
	case WM_CREATE:
		sys = (CBonDriverUtil*)((LPCREATESTRUCT)lParam)->lpCreateParams;
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)sys);
		SetTimer(hwnd, 1, 20, NULL);
		sys->statusTimeout = 0;
		if( sys->traceLevel ){
			AddDebugLog(L"CBonDriverUtil: #Open");
			CBlockLock lock(&sys->utilLock);
			sys->callingName = NULL;
		}
		return 0;
	case WM_DESTROY:
		if( sys->traceLevel ){
			AddDebugLog(L"CBonDriverUtil: #Closing");
			CBlockLock lock(&sys->utilLock);
			sys->callingName = L"Closing";
			sys->callingTick = GetTickCount();
		}
		if( sys->statusFunc ){
			sys->statusFunc(0.0f, -1, -1);
		}
		SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
		PostQuitMessage(0);
		return 0;
	case WM_TIMER:
		if( wParam == 1 ){
			SendMessage(hwnd, WM_APP_GET_TS_STREAM, 0, 0);
			//従来の取得間隔が概ね1秒なので、それよりやや短い間隔
			if( ++sys->statusTimeout > 600 / 20 ){
				SendMessage(hwnd, WM_APP_GET_STATUS, 0, 0);
				sys->statusTimeout = 0;
			}
			return 0;
		}
		break;
	case WM_APP_GET_TS_STREAM:
		{
			if( sys->traceLevel ){
				CBlockLock lock(&sys->utilLock);
				sys->callingName = L"GetTs";
				sys->callingTick = GetTickCount();
			}
			//TSストリームを取得
			BYTE* data;
			DWORD size;
			DWORD remain;
			if( sys->bon2IF->GetTsStream(&data, &size, &remain) && data && size != 0 ){
				if( sys->recvFunc ){
					sys->recvFunc(data, size, 1);
				}
				PostMessage(hwnd, WM_APP_GET_TS_STREAM, 1, 0);
			}else{
				size = 0;
				if( wParam ){
					//EDCBは(伝統的に)GetTsStreamのremainを利用しないので、受け取るものがなくなったらremain=0を知らせる
					if( sys->recvFunc ){
						sys->recvFunc(NULL, 0, 0);
					}
				}
			}
			if( sys->traceLevel ){
				CBlockLock lock(&sys->utilLock);
				sys->callingName = NULL;
				sys->statGetTsCalls++;
				sys->statGetTsBytes += size;
			}
		}
		return 0;
	case WM_APP_GET_STATUS:
		if( sys->statusFunc ){
			if( sys->traceLevel ){
				CBlockLock lock(&sys->utilLock);
				sys->callingName = L"GetStatus";
				sys->callingTick = GetTickCount();
			}
			if( sys->initChSetFlag ){
				sys->statusFunc(sys->bon2IF->GetSignalLevel(), sys->bon2IF->GetCurSpace(), sys->bon2IF->GetCurChannel());
			}else{
				sys->statusFunc(sys->bon2IF->GetSignalLevel(), -1, -1);
			}
			if( sys->traceLevel ){
				CBlockLock lock(&sys->utilLock);
				sys->callingName = NULL;
			}
		}
		return 0;
	case WM_APP_SET_CH:
		if( sys->traceLevel ){
			AddDebugLog(L"CBonDriverUtil: #SetCh");
			CBlockLock lock(&sys->utilLock);
			sys->callingName = L"SetCh";
			sys->callingTick = GetTickCount();
		}
		if( sys->bon2IF->SetChannel((DWORD)wParam, (DWORD)lParam) == FALSE ){
			Sleep(500);
			if( sys->traceLevel ){
				AddDebugLog(L"CBonDriverUtil: #SetCh2");
			}
			if( sys->bon2IF->SetChannel((DWORD)wParam, (DWORD)lParam) == FALSE ){
				if( sys->traceLevel ){
					CBlockLock lock(&sys->utilLock);
					sys->callingName = NULL;
				}
				return FALSE;
			}
		}
		if( sys->traceLevel ){
			CBlockLock lock(&sys->utilLock);
			sys->callingName = NULL;
		}
		sys->initChSetFlag = true;
		PostMessage(hwnd, WM_APP_GET_STATUS, 0, 0);
		return TRUE;
	case WM_APP_GET_NOW_CH:
		if( sys->initChSetFlag ){
			if( sys->traceLevel ){
				CBlockLock lock(&sys->utilLock);
				sys->callingName = L"GetNowCh";
				sys->callingTick = GetTickCount();
			}
			*(DWORD*)wParam = sys->bon2IF->GetCurSpace();
			*(DWORD*)lParam = sys->bon2IF->GetCurChannel();
			if( sys->traceLevel ){
				CBlockLock lock(&sys->utilLock);
				sys->callingName = NULL;
			}
			return TRUE;
		}
		return FALSE;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void CBonDriverUtil::WatchdogThread(CBonDriverUtil* sys)
{
	int statTimeout = 0;
	while( sys->watchdogStopEvent.WaitOne(2000) == false ){
		if( sys->traceLevel ){
			//統計を1分ごとにデバッグ出力
			if( ++statTimeout >= 30 ){
				int calls;
				__int64 bytes;
				{
					CBlockLock lock(&sys->utilLock);
					calls = sys->statGetTsCalls;
					bytes = sys->statGetTsBytes;
					sys->statGetTsCalls = 0;
					sys->statGetTsBytes = 0;
				}
				if( sys->traceLevel > 1 ){
					AddDebugLogFormat(L"CBonDriverUtil: #GetTs %d calls, %lld bytes", calls, bytes);
				}
				statTimeout = 0;
			}
			//BonDriver呼び出しに10秒以上かかっていればデバッグ出力
			CBlockLock lock(&sys->utilLock);
			if( sys->callingName ){
				DWORD tick = GetTickCount();
				if( tick - sys->callingTick > 10000 ){
					AddDebugLogFormat(L"CBonDriverUtil: #%ls takes more than 10 seconds!", sys->callingName);
					sys->callingTick = tick;
				}
			}
		}
	}
}

bool CBonDriverUtil::SetCh(DWORD space, DWORD ch)
{
	if( this->hwndDriver ){
		//同一チャンネル時の命令省略はしない。必要なら利用側で行うこと
		if( SendMessage(this->hwndDriver, WM_APP_SET_CH, (WPARAM)space, (LPARAM)ch) ){
			return true;
		}
	}
	return false;
}

bool CBonDriverUtil::GetNowCh(DWORD* space, DWORD* ch)
{
	if( this->hwndDriver ){
		if( SendMessage(this->hwndDriver, WM_APP_GET_NOW_CH, (WPARAM)space, (LPARAM)ch) ){
			return true;
		}
	}
	return false;
}

wstring CBonDriverUtil::GetOpenBonDriverFileName()
{
	CBlockLock lock(&this->utilLock);
	if( this->hwndDriver ){
		//Open中はconst
		return this->loadDllFileName;
	}
	return L"";
}
