#ifndef INCLUDE_MESSAGE_MANAGER_H
#define INCLUDE_MESSAGE_MANAGER_H

#ifndef _WIN32
#include "ThreadUtil.h"
#include <queue>
#include <unordered_set>
#endif

class CMessageManager
{
public:
	static const int ID_INITIALIZED = -1;
	static const int ID_SIGNAL = -2;
#ifdef _WIN32
	static const int ID_DESTROY = WM_DESTROY;
	static const int ID_CLOSE = WM_CLOSE;
	static const int ID_TIMER = WM_TIMER;
	static const int ID_APP = WM_APP;
#else
	static const int ID_DESTROY = 0x0002;
	static const int ID_CLOSE = 0x0010;
	static const int ID_TIMER = 0x0113;
	static const int ID_APP = 0x8000;
#endif
	static const int ID_APP_MAX = ID_APP + 0x3FFF;

	struct PARAMS
	{
		int id;
		INT_PTR param1;
		INT_PTR param2;
		INT_PTR result;
		void* ctx;
	};

	CMessageManager(bool (*onMsg)(PARAMS&), void* ctx);
	void SetContext(void* ctx) { m_ctx = ctx; }
	bool SetTimer(int timerID, int msec);
	bool KillTimer(int timerID);
	INT_PTR Send(int msgID, INT_PTR param1 = 0, INT_PTR param2 = 0);
	bool Post(int msgID, INT_PTR param1 = 0, INT_PTR param2 = 0);
	bool SendNotify(int msgID, INT_PTR param1 = 0, INT_PTR param2 = 0);
#ifdef _WIN32
	bool IsInitialized() const { return !!m_hwnd; }
	HWND GetHwnd() const { return m_hwnd; }
	bool ProcessWindowMessage(LRESULT& ret, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#else
	bool IsInitialized() const { return m_initialized; }
	bool MessageLoop(bool handleSignal = false);
#endif
	static bool IsProcessingTargetID(int msgID) { return msgID == ID_DESTROY || msgID == ID_CLOSE || msgID == ID_TIMER || (msgID >= ID_APP && msgID <= ID_APP_MAX); }

private:
	bool (*m_onMsg)(PARAMS&);
	void* m_ctx;
#ifdef _WIN32
	HWND m_hwnd;
#else
	struct POST {
		int id;
		bool byTimer;
		INT_PTR param1;
		INT_PTR param2;
	};

	struct TIMER {
		int timerID;
		int msec;
		DWORD tick;
	};

	std::atomic_bool m_initialized;
	std::atomic_bool m_sending;
	std::atomic_bool m_destroy;
	std::atomic_bool m_handleSignal;
	std::thread::id m_tid;
	std::recursive_mutex m_sendLock;
	std::recursive_mutex m_queueLock;
	std::queue<POST> m_postQueue;
	std::unordered_set<int> m_postByTimerSet;
	vector<TIMER> m_timerList;
	CAutoResetEvent m_pushEvent;
	CAutoResetEvent m_sentEvent;
	PARAMS m_sendParams;
#endif
};

#endif
