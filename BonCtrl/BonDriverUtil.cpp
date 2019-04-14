#include "stdafx.h"
#include "BonDriverUtil.h"
#include "../Common/PathUtil.h"
#include "../Common/StringUtil.h"
#include "IBonDriver2.h"

namespace
{
enum {
	WM_APP_GET_TS_STREAM = WM_APP,
	WM_APP_GET_STATUS,
	WM_APP_SET_CH,
	WM_APP_GET_NOW_CH,
};

#ifdef USE_IBONCAST
IBonDriver* CastB(IBonDriver2** if2, IBonDriver* (*funcCreate)(), const LPVOID* (WINAPI* funcCast)(LPCSTR, void*))
{
	HMODULE hModule = NULL;
	if( funcCast == NULL ){
		if( (hModule = LoadLibrary(L"IBonCast.dll")) == NULL ){
			OutputDebugString(L"★IBonCast.dllがロードできません\r\n");
			return NULL;
		}
		funcCast = (const LPVOID*(WINAPI*)(LPCSTR,void*))GetProcAddress(hModule, "Cast");
	}
	void* pBase;
	const LPVOID* table;
	if( funcCast == NULL || (pBase = funcCreate()) == NULL || (table = funcCast("IBonDriver@10", pBase)) == NULL ){
		OutputDebugString(L"★Castに失敗しました\r\n");
		if( hModule ){
			FreeLibrary(hModule);
		}
		return NULL;
	}

	class CCastB : public IBonDriver2
	{
	public:
		CCastB(HMODULE h_, void* p_, const LPVOID* t_) : h(h_), p(p_), t(t_) {}
		const BOOL OpenTuner() { return ((BOOL(*)(void*))t[0])(p); }
		void CloseTuner() { ((void(*)(void*))t[1])(p); }
		const BOOL SetChannel(const BYTE bCh) { return ((BOOL(*)(void*,BYTE))t[2])(p, bCh); }
		const float GetSignalLevel() { return ((float(*)(void*))t[3])(p); }
		const DWORD WaitTsStream(const DWORD dwTimeOut = 0) { return ((DWORD(*)(void*,DWORD))t[4])(p, dwTimeOut); }
		const DWORD GetReadyCount() { return ((DWORD(*)(void*))t[5])(p); }
		const BOOL GetTsStream(BYTE* pDst, DWORD* pdwSize, DWORD* pdwRemain) { return ((BOOL(*)(void*,BYTE*,DWORD*,DWORD*))t[6])(p, pDst, pdwSize, pdwRemain); }
		const BOOL GetTsStream(BYTE** ppDst, DWORD* pdwSize, DWORD* pdwRemain) { return ((BOOL(*)(void*,BYTE**,DWORD*,DWORD*))t[7])(p, ppDst, pdwSize, pdwRemain); }
		void PurgeTsStream() { ((void(*)(void*))t[8])(p); }
		void Release() { ((void(*)(void*))t[9])(p); if( h ) FreeLibrary(h); delete this; }
		LPCTSTR GetTunerName() { return ((LPCWSTR(*)(void*))t[10])(p); }
		const BOOL IsTunerOpening() { return ((BOOL(*)(void*))t[11])(p); }
		LPCTSTR EnumTuningSpace(const DWORD dwSpace) { return ((LPCWSTR(*)(void*,DWORD))t[12])(p, dwSpace); }
		LPCTSTR EnumChannelName(const DWORD dwSpace, const DWORD dwChannel) { return ((LPCWSTR(*)(void*,DWORD,DWORD))t[13])(p, dwSpace, dwChannel); }
		const BOOL SetChannel(const DWORD dwSpace, const DWORD dwChannel) { return ((BOOL(*)(void*,DWORD,DWORD))t[14])(p, dwSpace, dwChannel); }
		const DWORD GetCurSpace() { return ((DWORD(*)(void*))t[15])(p); }
		const DWORD GetCurChannel() { return ((DWORD(*)(void*))t[16])(p); }
	private:
		HMODULE h;
		void* p;
		const LPVOID* t;
	};

	CCastB* b = new CCastB(hModule, pBase, table);
	*if2 = NULL;
	if( funcCast("IBonDriver2@17", pBase) == table ){
		*if2 = b;
	}
	return b;
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

void CBonDriverUtil::SetBonDriverFolder(LPCWSTR bonDriverFolderPath)
{
	CBlockLock lock(&this->utilLock);
	this->loadDllFolder = bonDriverFolderPath;
}

vector<wstring> CBonDriverUtil::EnumBonDriver()
{
	CBlockLock lock(&this->utilLock);
	vector<wstring> list;
	if( this->loadDllFolder.empty() == false ){
		//指定フォルダのファイル一覧取得
		EnumFindFile(fs_path(this->loadDllFolder).append(L"BonDriver*.dll").c_str(), [&](WIN32_FIND_DATA& findData) -> bool {
			if( (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 ){
				//見つかったDLLを一覧に追加
				list.push_back(findData.cFileName);
			}
			return true;
		});
	}
	return list;
}

bool CBonDriverUtil::OpenBonDriver(LPCWSTR bonDriverFile, const std::function<void(BYTE*, DWORD, DWORD)>& recvFunc_,
                                   const std::function<void(float, int, int)>& statusFunc_, int openWait)
{
	CBlockLock lock(&this->utilLock);
	CloseBonDriver();
	this->loadDllFileName = bonDriverFile;
	if( this->loadDllFolder.empty() == false && this->loadDllFileName.empty() == false ){
		this->recvFunc = recvFunc_;
		this->statusFunc = statusFunc_;
		this->driverThread = thread_(DriverThread, this);
		//Open処理が完了するまで待つ
		while( WaitForSingleObject(this->driverThread.native_handle(), 10) == WAIT_TIMEOUT && this->hwndDriver == NULL );
		if( this->hwndDriver ){
			Sleep(openWait);
			return true;
		}
		this->driverThread.join();
	}
	return false;
}

void CBonDriverUtil::CloseBonDriver()
{
	CBlockLock lock(&this->utilLock);
	if( this->hwndDriver ){
		PostMessage(this->hwndDriver, WM_CLOSE, 0, 0);
		this->driverThread.join();
		this->hwndDriver = NULL;
	}
}

void CBonDriverUtil::DriverThread(CBonDriverUtil* sys)
{
	//BonDriverがCOMを利用するかもしれないため
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	IBonDriver* bonIF = NULL;
	sys->bon2IF = NULL;
	HMODULE hModule = LoadLibrary(fs_path(sys->loadDllFolder).append(sys->loadDllFileName).c_str());
	if( hModule == NULL ){
		OutputDebugString(L"★BonDriverがロードできません\r\n");
	}else{
		IBonDriver* (*funcCreateBonDriver)() = (IBonDriver*(*)())GetProcAddress(hModule, "CreateBonDriver");
		if( funcCreateBonDriver == NULL ){
			OutputDebugString(L"★GetProcAddressに失敗しました\r\n");
#ifdef USE_IBONCAST
		}else if( (bonIF = CastB(&sys->bon2IF, funcCreateBonDriver, (const LPVOID*(WINAPI*)(LPCSTR,void*))GetProcAddress(hModule, "Cast"))) != NULL &&
		          sys->bon2IF != NULL ){
#else
		}else if( (bonIF = funcCreateBonDriver()) != NULL &&
		          (sys->bon2IF = dynamic_cast<IBonDriver2*>(bonIF)) != NULL ){
#endif
			if( sys->bon2IF->OpenTuner() == FALSE ){
				OutputDebugString(L"★OpenTunerに失敗しました\r\n");
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
				sys->hwndDriver = CreateWindow(L"BonDriverUtilWorker", NULL, WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, HWND_MESSAGE, NULL, GetModuleHandle(NULL), sys);
				if( sys->hwndDriver == NULL ){
					sys->bon2IF->CloseTuner();
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
		return 0;
	case WM_DESTROY:
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
			//TSストリームを取得
			BYTE* data;
			DWORD size;
			DWORD remain;
			if( sys->bon2IF->GetTsStream(&data, &size, &remain) && data && size != 0 ){
				if( sys->recvFunc ){
					sys->recvFunc(data, size, 1);
				}
				PostMessage(hwnd, WM_APP_GET_TS_STREAM, 1, 0);
			}else if( wParam ){
				//EDCBは(伝統的に)GetTsStreamのremainを利用しないので、受け取るものがなくなったらremain=0を知らせる
				if( sys->recvFunc ){
					sys->recvFunc(NULL, 0, 0);
				}
			}
		}
		return 0;
	case WM_APP_GET_STATUS:
		if( sys->statusFunc ){
			if( sys->initChSetFlag ){
				sys->statusFunc(sys->bon2IF->GetSignalLevel(), sys->bon2IF->GetCurSpace(), sys->bon2IF->GetCurChannel());
			}else{
				sys->statusFunc(sys->bon2IF->GetSignalLevel(), -1, -1);
			}
		}
		return 0;
	case WM_APP_SET_CH:
		if( sys->bon2IF->SetChannel((DWORD)wParam, (DWORD)lParam) == FALSE ){
			Sleep(500);
			if( sys->bon2IF->SetChannel((DWORD)wParam, (DWORD)lParam) == FALSE ){
				return FALSE;
			}
		}
		PostMessage(hwnd, WM_APP_GET_STATUS, 0, 0);
		return TRUE;
	case WM_APP_GET_NOW_CH:
		*(DWORD*)wParam = sys->bon2IF->GetCurSpace();
		*(DWORD*)lParam = sys->bon2IF->GetCurChannel();
		return 0;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

vector<pair<wstring, vector<wstring>>> CBonDriverUtil::GetOriginalChList()
{
	CBlockLock lock(&this->utilLock);
	if( this->hwndDriver ){
		return this->loadChList;
	}
	return vector<pair<wstring, vector<wstring>>>();
}

wstring CBonDriverUtil::GetTunerName()
{
	CBlockLock lock(&this->utilLock);
	if( this->hwndDriver ){
		return this->loadTunerName;
	}
	return L"";
}

bool CBonDriverUtil::SetCh(DWORD space, DWORD ch)
{
	CBlockLock lock(&this->utilLock);
	if( this->hwndDriver ){
		if( this->initChSetFlag ){
			//2回目以降は変化のある場合だけチャンネル設定する
			DWORD nowSpace = 0;
			DWORD nowCh = 0;
			SendMessage(this->hwndDriver, WM_APP_GET_NOW_CH, (WPARAM)&nowSpace, (LPARAM)&nowCh);
			if( nowSpace == space && nowCh == ch ){
				return true;
			}
		}
		if( SendMessage(this->hwndDriver, WM_APP_SET_CH, (WPARAM)space, (LPARAM)ch) ){
			this->initChSetFlag = true;
			return true;
		}
	}
	return false;
}

bool CBonDriverUtil::GetNowCh(DWORD* space, DWORD* ch)
{
	CBlockLock lock(&this->utilLock);
	if( this->hwndDriver && this->initChSetFlag ){
		SendMessage(this->hwndDriver, WM_APP_GET_NOW_CH, (WPARAM)space, (LPARAM)ch);
		return true;
	}
	return false;
}

wstring CBonDriverUtil::GetOpenBonDriverFileName()
{
	CBlockLock lock(&this->utilLock);
	if( this->hwndDriver ){
		return this->loadDllFileName;
	}
	return L"";
}
