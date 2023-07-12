#define _WIN32_WINNT _WIN32_WINNT_VISTA
#include <sdkddkver.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>
#include <lm.h>

static const WCHAR CLASS_NAME[] = L"EpgTimerAdminProxy";

static bool g_denySetTime;

enum {
	// システム日時を設定する
	WM_APP_SETTIME = WM_APP,
	// 特定拡張子のファイルに共有アクセスがあるかどうか調べる
	WM_APP_NETFIND,
};

static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_TIMER:
		if (wParam == 1) {
			KillTimer(hwnd, 1);
			g_denySetTime = false;
		}
		break;
	case WM_APP_SETTIME:
		if (!g_denySetTime) {
			FILETIME ft;
			ft.dwLowDateTime = static_cast<DWORD>(wParam);
			ft.dwHighDateTime = static_cast<DWORD>(lParam);
			FILETIME ftNow;
			GetSystemTimeAsFileTime(&ftNow);
			LONGLONG t = ft.dwLowDateTime | static_cast<LONGLONG>(ft.dwHighDateTime) << 32;
			LONGLONG tNow = ftNow.dwLowDateTime | static_cast<LONGLONG>(ftNow.dwHighDateTime) << 32;
			// 激変禁止(24時間)
			if (tNow - 24 * 3600 * 1000000LL < t && t < tNow + 24 * 3600 * 10000000LL) {
				SYSTEMTIME st;
				if (FileTimeToSystemTime(&ft, &st) && SetSystemTime(&st)) {
					// 安全のため30秒間は再設定を拒否する
					g_denySetTime = true;
					SetTimer(hwnd, 1, 30000, nullptr);
					return TRUE;
				}
			}
		}
		return -1;
	case WM_APP_NETFIND:
		{
			// 8文字以下の拡張子をwParamとlParamで受け取る
			WCHAR ext[10] = L".";
			for (int i = 1; i < 9; ++i) {
				ext[i] = (static_cast<DWORD>(i < 5 ? wParam : lParam) >> ((i - 1) % 4 * 8)) & 0xFF;
				// 英数字に限る
				if (ext[i] && (ext[i] < L'0' || L'9' < ext[i]) && (ext[i] < L'A' || L'Z' < ext[i]) && (ext[i] < L'a' || L'z' < ext[i])) {
					ext[1] = L'\0';
					break;
				}
			}
			LRESULT ret = -1;
			if (ext[1]) {
				FILE_INFO_3 *info;
				DWORD entriesread;
				DWORD totalentries;
				if (NetFileEnum(nullptr, nullptr, nullptr, 3, reinterpret_cast<LPBYTE*>(&info), MAX_PREFERRED_LENGTH, &entriesread, &totalentries, nullptr) == NERR_Success) {
					for (DWORD i = 0; i < entriesread; ++i) {
						if (wcslen(info[i].fi3_pathname) >= wcslen(ext) && _wcsicmp(info[i].fi3_pathname + wcslen(info[i].fi3_pathname) - wcslen(ext), ext) == 0) {
							ret = TRUE;
							break;
						}
					}
					NetApiBufferFree(info);
				}
			}
			return ret;
		}
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

#ifdef __MINGW32__
__declspec(dllexport) // ASLRを無効にしないため(CVE-2018-5392)
#endif
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	static_cast<void>(hPrevInstance);
	static_cast<void>(nCmdShow);

	SetDllDirectory(L"");

	if (_wcsicmp(lpCmdLine, L"/TestSetTime") == 0) {
		// 動作テスト: 現在時刻でシステム日時を設定する
		LPCWSTR text = L"Proxy not found.";
		HWND hwnd = FindWindowEx(HWND_MESSAGE, nullptr, CLASS_NAME, nullptr);
		if (hwnd) {
			FILETIME ft;
			GetSystemTimeAsFileTime(&ft);
			LRESULT ret = SendMessage(hwnd, WM_APP_SETTIME, ft.dwLowDateTime, ft.dwHighDateTime);
			text = (ret > 0 ? L"Succeeded." : ret < 0 ? L"Failed." : L"Denied.");
		}
		MessageBox(nullptr, text, CLASS_NAME, MB_OK);
		return 0;
	}
	else if (_wcsicmp(lpCmdLine, L"/TestNetFind") == 0) {
		// 動作テスト: 拡張子.txtについて共有アクセスがあるかどうか調べる
		LPCWSTR text = L"Proxy not found.";
		HWND hwnd = FindWindowEx(HWND_MESSAGE, nullptr, CLASS_NAME, nullptr);
		if (hwnd) {
			LRESULT ret = SendMessage(hwnd, WM_APP_NETFIND, 0x00747874, 0x00000000);
			text = (ret > 0 ? L"Succeeded." : ret < 0 ? L"Failed or no access." : L"Denied.");
		}
		MessageBox(nullptr, text, CLASS_NAME, MB_OK);
		return 0;
	}

	// メッセージ専用ウィンドウを作成
	WNDCLASS wc = {};
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;
	RegisterClass(&wc);
	if (CreateWindow(CLASS_NAME, nullptr, 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, hInstance, nullptr)) {
		// 整合性レベルの低いプロセスからのメッセージを受け取るようにする
		ChangeWindowMessageFilter(WM_APP_SETTIME, MSGFLT_ADD);
		ChangeWindowMessageFilter(WM_APP_NETFIND, MSGFLT_ADD);
		// メッセージループ
		MSG msg;
		while (GetMessage(&msg, nullptr, 0, 0) > 0) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return 0;
}
