#include "stdafx.h"
#include "BonDriverUtil.h"
#include "../Common/PathUtil.h"
#include "../Common/StringUtil.h"
#include "../Common/TimeUtil.h"
#include "IBonDriver2.h"
#ifdef _WIN32
#include <objbase.h>
#endif

namespace
{
enum {
	ID_APP_GET_TS_STREAM = CMessageManager::ID_APP,
	ID_APP_GET_STATUS,
	ID_APP_SET_CH,
	ID_APP_GET_NOW_CH,
};

wstring Bon16CharToWString(const IBonDriver2::BON16CHAR* src)
{
#if WCHAR_MAX > 0xFFFF
	wstring dest;
	for( size_t i = 0; src[i]; i++ ){
		if( src[i] < 0xD800 || src[i] >= 0xE000 ){
			dest += (WCHAR)src[i];
		}else if( src[i] >= 0xDC00 || src[i + 1] < 0xDC00 || src[i + 1] >= 0xE000 ){
			dest += (WCHAR)0xFFFD;
		}else{
			dest += (WCHAR)(0x10000 + (src[i] - 0xD800) * 0x400 + (src[i + 1] - 0xDC00));
			i++;
		}
	}
	return dest;
#else
	return src;
#endif
}

#if defined(_WIN32) && !defined(_MSC_VER)
IBonDriver* CastB(IBonDriver2** if2, IBonDriver* (*funcCreate)())
{
	void* hModule = UtilLoadLibrary(wstring(L"IBonCast.dll"));
	if( hModule == NULL ){
		AddDebugLog(L"★IBonCast.dllがロードできません");
		return NULL;
	}
	void* const* (WINAPI* funcCast)(LPCSTR, void*);
	void* pBase;
	void* const* table;
	if( UtilGetProcAddress(hModule, "Cast", funcCast) == false ||
	    (pBase = funcCreate()) == NULL || (table = funcCast("IBonDriver@10", pBase)) == NULL ){
		AddDebugLog(L"★Castに失敗しました");
		UtilFreeLibrary(hModule);
		return NULL;
	}

	class CCastB : public CBonStruct2Adapter
	{
	public:
		CCastB(void* h_, void* p, void* const* t, void* const* t2) : h(h_) {
			st2.st.pCtx = p;
			//アダプタの関数ポインタフィールドを上書き
			std::copy(t, t + 10, (void**)&st2.st.pF00);
			if( t2 ) std::copy(t2, t2 + 7, (void**)&st2.pF10);
		}
		void Release() {
			CBonStruct2Adapter::Release();
			UtilFreeLibrary(h);
			delete this;
		}
	private:
		void* h;
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
#ifdef _WIN32
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(wc);
	wc.lpfnWndProc = DriverWindowProc;
	wc.hInstance = GetModuleHandle(NULL);
	wc.lpszClassName = L"BonDriverUtilWorker";
	RegisterClassEx(&wc);
#endif
}

CBonDriverUtil::CBonDriverUtil(void)
	: msgManager(OnMessage, this)
	, openFlag(false)
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
			this->callingTick = GetU32Tick();
			this->statGetTsCalls = 0;
			this->statGetTsBytes = 0;
			this->watchdogStopEvent.Reset();
			this->watchdogThread = thread_(WatchdogThread, this);
		}
		this->driverOpenEvent.Reset();
		this->driverThread = thread_(DriverThread, this);
		//Open処理が完了するまで待つ
		this->driverOpenEvent.WaitOne();
		if( this->openFlag ){
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
	if( this->openFlag ){
		this->msgManager.SendNotify(CMessageManager::ID_CLOSE);
		this->driverThread.join();
		if( this->watchdogThread.joinable() ){
			this->watchdogStopEvent.Set();
			this->watchdogThread.join();
		}
		this->loadChList.clear();
		this->loadTunerName.clear();
		lock_recursive_mutex lock(this->utilLock);
		this->openFlag = false;
	}
}

void CBonDriverUtil::DriverThread(CBonDriverUtil* sys)
{
#ifdef _WIN32
	//BonDriverがCOMを利用するかもしれないため
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
#endif

	IBonDriver* bonIF = NULL;
	sys->bon2IF = NULL;
	CBonStructAdapter bonAdapter;
	CBonStruct2Adapter bon2Adapter;
	void* hModule = UtilLoadLibrary(fs_path(sys->loadDllFolder).append(sys->loadDllFileName));
	if( hModule == NULL ){
		AddDebugLog(L"★BonDriverがロードできません");
	}else{
		const STRUCT_IBONDRIVER* (*funcCreateBonStruct)();
		if( UtilGetProcAddress(hModule, "CreateBonStruct", funcCreateBonStruct) ){
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
			IBonDriver* (*funcCreateBonDriver)();
			if( UtilGetProcAddress(hModule, "CreateBonDriver", funcCreateBonDriver) == false ){
				AddDebugLog(L"★GetProcAddressに失敗しました");
			}else{
#if !defined(_WIN32) || defined(_MSC_VER)
				//受け取ったC++オブジェクトに互換性があると仮定する
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
				const IBonDriver2::BON16CHAR* tunerName = sys->bon2IF->GetTunerName();
				sys->loadTunerName = tunerName ? Bon16CharToWString(tunerName) : L"";
				Replace(sys->loadTunerName, L"(",L"（");
				Replace(sys->loadTunerName, L")",L"）");
				//チャンネル一覧の取得
				sys->loadChList.clear();
				for( DWORD countSpace = 0; ; countSpace++ ){
					const IBonDriver2::BON16CHAR* spaceName = sys->bon2IF->EnumTuningSpace(countSpace);
					if( spaceName == NULL ){
						break;
					}
					sys->loadChList.push_back(std::make_pair(Bon16CharToWString(spaceName), vector<wstring>()));
					for( DWORD countCh = 0; ; countCh++ ){
						const IBonDriver2::BON16CHAR* chName = sys->bon2IF->EnumChannelName(countSpace, countCh);
						if( chName == NULL ){
							break;
						}
						sys->loadChList.back().second.push_back(Bon16CharToWString(chName));
					}
				}
#ifdef _WIN32
				CreateWindow(L"BonDriverUtilWorker", NULL, WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, HWND_MESSAGE, NULL, GetModuleHandle(NULL), sys);
				if( sys->openFlag == false ){
					sys->loadChList.clear();
					sys->loadTunerName.clear();
					sys->bon2IF->CloseTuner();
				}
#else
				lock_recursive_mutex lock(sys->utilLock);
				sys->openFlag = true;
#endif
			}
		}
	}
	if( sys->openFlag == false ){
		//Openできなかった
		if( bonIF ){
			bonIF->Release();
		}
		if( hModule ){
			UtilFreeLibrary(hModule);
		}
#ifdef _WIN32
		CoUninitialize();
#endif
		sys->driverOpenEvent.Set();
		return;
	}

#ifdef _WIN32
	//割り込み遅延への耐性はBonDriverのバッファ能力に依存するので、相対優先順位を上げておく
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

	//メッセージループ
	MSG msg;
	while( GetMessage(&msg, NULL, 0, 0) > 0 ){
		DispatchMessage(&msg);
	}
#else
	sys->msgManager.MessageLoop();
#endif

	sys->bon2IF->CloseTuner();
	bonIF->Release();
	UtilFreeLibrary(hModule);

#ifdef _WIN32
	CoUninitialize();
#endif
}

#ifdef _WIN32
LRESULT CALLBACK CBonDriverUtil::DriverWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CBonDriverUtil* sys = (CBonDriverUtil*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	if( uMsg == WM_CREATE ){
		sys = (CBonDriverUtil*)((LPCREATESTRUCT)lParam)->lpCreateParams;
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)sys);
	}
	LRESULT lResult;
	if( sys && sys->msgManager.ProcessWindowMessage(lResult, hwnd, uMsg, wParam, lParam) ){
		if( uMsg == WM_DESTROY ){
			SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
			PostQuitMessage(0);
		}
		return lResult;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
#endif

bool CBonDriverUtil::OnMessage(CMessageManager::PARAMS& pa)
{
	CBonDriverUtil* sys = (CBonDriverUtil*)pa.ctx;
	switch( pa.id ){
	case CMessageManager::ID_INITIALIZED:
		sys->msgManager.SetTimer(1, 20);
		sys->statusTimeout = 0;
		if( sys->traceLevel ){
			AddDebugLog(L"CBonDriverUtil: #Open");
			lock_recursive_mutex lock(sys->utilLock);
			sys->callingName = NULL;
		}
		{
			lock_recursive_mutex lock(sys->utilLock);
			sys->openFlag = true;
		}
		sys->driverOpenEvent.Set();
		return true;
	case CMessageManager::ID_DESTROY:
		if( sys->traceLevel ){
			AddDebugLog(L"CBonDriverUtil: #Closing");
			lock_recursive_mutex lock(sys->utilLock);
			sys->callingName = L"Closing";
			sys->callingTick = GetU32Tick();
		}
		if( sys->statusFunc ){
			sys->statusFunc(0.0f, -1, -1);
		}
		return true;
	case CMessageManager::ID_TIMER:
		if( pa.param1 == 1 ){
			sys->msgManager.Send(ID_APP_GET_TS_STREAM);
			//従来の取得間隔が概ね1秒なので、それよりやや短い間隔
			if( ++sys->statusTimeout > 600 / 20 ){
				sys->msgManager.Send(ID_APP_GET_STATUS);
				sys->statusTimeout = 0;
			}
			return true;
		}
		return false;
	case ID_APP_GET_TS_STREAM:
		{
			if( sys->traceLevel ){
				lock_recursive_mutex lock(sys->utilLock);
				sys->callingName = L"GetTs";
				sys->callingTick = GetU32Tick();
			}
			//TSストリームを取得
			BYTE* data;
			DWORD size;
			DWORD remain;
			if( sys->bon2IF->GetTsStream(&data, &size, &remain) && data && size != 0 ){
				if( sys->recvFunc ){
					sys->recvFunc(data, size, 1);
				}
				sys->msgManager.Post(ID_APP_GET_TS_STREAM, 1);
			}else{
				size = 0;
				if( pa.param1 ){
					//EDCBは(伝統的に)GetTsStreamのremainを利用しないので、受け取るものがなくなったらremain=0を知らせる
					if( sys->recvFunc ){
						sys->recvFunc(NULL, 0, 0);
					}
				}
			}
			if( sys->traceLevel ){
				lock_recursive_mutex lock(sys->utilLock);
				sys->callingName = NULL;
				sys->statGetTsCalls++;
				sys->statGetTsBytes += size;
			}
		}
		return true;
	case ID_APP_GET_STATUS:
		if( sys->statusFunc ){
			if( sys->traceLevel ){
				lock_recursive_mutex lock(sys->utilLock);
				sys->callingName = L"GetStatus";
				sys->callingTick = GetU32Tick();
			}
			if( sys->initChSetFlag ){
				sys->statusFunc(sys->bon2IF->GetSignalLevel(), sys->bon2IF->GetCurSpace(), sys->bon2IF->GetCurChannel());
			}else{
				sys->statusFunc(sys->bon2IF->GetSignalLevel(), -1, -1);
			}
			if( sys->traceLevel ){
				lock_recursive_mutex lock(sys->utilLock);
				sys->callingName = NULL;
			}
		}
		return true;
	case ID_APP_SET_CH:
		if( sys->traceLevel ){
			AddDebugLog(L"CBonDriverUtil: #SetCh");
			lock_recursive_mutex lock(sys->utilLock);
			sys->callingName = L"SetCh";
			sys->callingTick = GetU32Tick();
		}
		if( sys->bon2IF->SetChannel((DWORD)pa.param1, (DWORD)pa.param2) == FALSE ){
			SleepForMsec(500);
			if( sys->traceLevel ){
				AddDebugLog(L"CBonDriverUtil: #SetCh2");
			}
			if( sys->bon2IF->SetChannel((DWORD)pa.param1, (DWORD)pa.param2) == FALSE ){
				if( sys->traceLevel ){
					lock_recursive_mutex lock(sys->utilLock);
					sys->callingName = NULL;
				}
				return true;
			}
		}
		if( sys->traceLevel ){
			lock_recursive_mutex lock(sys->utilLock);
			sys->callingName = NULL;
		}
		sys->initChSetFlag = true;
		sys->msgManager.Post(ID_APP_GET_STATUS);
		pa.result = TRUE;
		return true;
	case ID_APP_GET_NOW_CH:
		if( sys->initChSetFlag ){
			if( sys->traceLevel ){
				lock_recursive_mutex lock(sys->utilLock);
				sys->callingName = L"GetNowCh";
				sys->callingTick = GetU32Tick();
			}
			*(DWORD*)pa.param1 = sys->bon2IF->GetCurSpace();
			*(DWORD*)pa.param2 = sys->bon2IF->GetCurChannel();
			if( sys->traceLevel ){
				lock_recursive_mutex lock(sys->utilLock);
				sys->callingName = NULL;
			}
			pa.result = TRUE;
		}
		return true;
	default:
		return false;
	}
}

void CBonDriverUtil::WatchdogThread(CBonDriverUtil* sys)
{
	int statTimeout = 0;
	while( sys->watchdogStopEvent.WaitOne(2000) == false ){
		if( sys->traceLevel ){
			//統計を1分ごとにデバッグ出力
			if( ++statTimeout >= 30 ){
				int calls;
				LONGLONG bytes;
				{
					lock_recursive_mutex lock(sys->utilLock);
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
			lock_recursive_mutex lock(sys->utilLock);
			if( sys->callingName ){
				DWORD tick = GetU32Tick();
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
	//同一チャンネル時の命令省略はしない。必要なら利用側で行うこと
	if( this->msgManager.Send(ID_APP_SET_CH, (INT_PTR)space, (INT_PTR)ch) ){
		return true;
	}
	return false;
}

bool CBonDriverUtil::GetNowCh(DWORD* space, DWORD* ch)
{
	if( this->msgManager.Send(ID_APP_GET_NOW_CH, (INT_PTR)space, (INT_PTR)ch) ){
		return true;
	}
	return false;
}

bool CBonDriverUtil::IsOpen()
{
	lock_recursive_mutex lock(this->utilLock);
	return this->openFlag;
}

wstring CBonDriverUtil::GetOpenBonDriverFileName()
{
	lock_recursive_mutex lock(this->utilLock);
	if( this->openFlag ){
		//Open中はconst
		return this->loadDllFileName;
	}
	return L"";
}
