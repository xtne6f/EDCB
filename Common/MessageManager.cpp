#include "stdafx.h"
#include "MessageManager.h"
#ifndef _WIN32
#include "TimeUtil.h"
#include <signal.h>
#endif

CMessageManager::CMessageManager(bool (*onMsg)(PARAMS&), void* ctx)
	: m_onMsg(onMsg)
	, m_ctx(ctx)
#ifdef _WIN32
	, m_hwnd(NULL)
#else
	, m_initialized(false)
#endif
{
}

#ifdef _WIN32

bool CMessageManager::SetTimer(int timerID, int msec)
{
	HWND hwnd = m_hwnd;
	return hwnd && timerID > 0 && ::SetTimer(hwnd, timerID, msec, NULL);
}

bool CMessageManager::KillTimer(int timerID)
{
	HWND hwnd = m_hwnd;
	return hwnd && timerID > 0 && ::KillTimer(hwnd, timerID);
}

INT_PTR CMessageManager::Send(int msgID, INT_PTR param1, INT_PTR param2)
{
	HWND hwnd = m_hwnd;
	return hwnd && msgID >= 0 ? SendMessage(hwnd, msgID, param1, param2) : 0;
}

bool CMessageManager::Post(int msgID, INT_PTR param1, INT_PTR param2)
{
	HWND hwnd = m_hwnd;
	return hwnd && msgID >= 0 && PostMessage(hwnd, msgID, param1, param2);
}

bool CMessageManager::SendNotify(int msgID, INT_PTR param1, INT_PTR param2)
{
	HWND hwnd = m_hwnd;
	return hwnd && msgID >= 0 && SendNotifyMessage(hwnd, msgID, param1, param2);
}

bool CMessageManager::ProcessWindowMessage(LRESULT& ret, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if( !m_hwnd && uMsg == WM_CREATE ){
		PARAMS pa = { ID_INITIALIZED, (INT_PTR)wParam, lParam, 0, m_ctx };
		m_hwnd = hwnd;
		if( m_onMsg(pa) ){
			ret = 0;
			return true;
		}
	}else if( m_hwnd && uMsg <= 0xFFFF && IsProcessingTargetID((int)uMsg) ){
		PARAMS pa = { (int)uMsg, (INT_PTR)wParam, lParam, 0, m_ctx };
		if( m_onMsg(pa) ){
			if( uMsg == WM_DESTROY ){
				m_hwnd = NULL;
			}
			ret = pa.result;
			return true;
		}
		if( uMsg == WM_DESTROY ){
			m_hwnd = NULL;
		}
	}
	return false;
}

#else

bool CMessageManager::SetTimer(int timerID, int msec)
{
	if( m_initialized && timerID > 0 && msec >= 0 ){
		lock_recursive_mutex lock(m_queueLock);
		KillTimer(timerID);
		TIMER t = { timerID, msec, GetU32Tick() };
		m_timerList.push_back(t);
		m_pushEvent.Set();
		if( m_handleSignal ){
			kill(getpid(), SIGUSR2);
		}
		return true;
	}
	return false;
}

bool CMessageManager::KillTimer(int timerID)
{
	if( m_initialized && timerID > 0 ){
		lock_recursive_mutex lock(m_queueLock);
		auto itr = std::find_if(m_timerList.begin(), m_timerList.end(), [=](TIMER t) { return t.timerID == timerID; });
		if( itr != m_timerList.end() ){
			m_timerList.erase(itr);
			return true;
		}
	}
	return false;
}

INT_PTR CMessageManager::Send(int msgID, INT_PTR param1, INT_PTR param2)
{
	if( m_initialized && IsProcessingTargetID(msgID) ){
		PARAMS pa = { msgID, param1, param2, 0, m_ctx };
		if( m_tid == std::this_thread::get_id() ){
			if( msgID == ID_DESTROY ){
				m_destroy = true;
			}
			if( m_onMsg(pa) ){
				return pa.result;
			}else if( msgID == ID_CLOSE ){
				Send(ID_DESTROY);
			}
		}else{
			lock_recursive_mutex lock(m_sendLock);
			if( m_initialized ){
				m_sendParams = pa;
				m_sending = true;
				m_pushEvent.Set();
				if( m_handleSignal ){
					kill(getpid(), SIGUSR2);
				}
				m_sentEvent.WaitOne();
				return m_sendParams.result;
			}
		}
	}
	return 0;
}

bool CMessageManager::Post(int msgID, INT_PTR param1, INT_PTR param2)
{
	if( m_initialized && IsProcessingTargetID(msgID) ){
		lock_recursive_mutex lock(m_queueLock);
		POST post = { msgID, param1, param2 };
		m_postQueue.push(post);
		m_pushEvent.Set();
		if( m_handleSignal ){
			kill(getpid(), SIGUSR2);
		}
		return true;
	}
	return false;
}

bool CMessageManager::SendNotify(int msgID, INT_PTR param1, INT_PTR param2)
{
	if( m_initialized && IsProcessingTargetID(msgID) ){
		if( m_tid == std::this_thread::get_id() ){
			Send(msgID, param1, param2);
			return true;
		}
		return Post(msgID, param1, param2);
	}
	return false;
}

bool CMessageManager::MessageLoop(bool handleSignal)
{
	m_sending = false;
	m_destroy = false;
	m_handleSignal = handleSignal;
	m_tid = std::this_thread::get_id();
	while( !m_postQueue.empty() ){
		m_postQueue.pop();
	}
	m_timerList.clear();
	m_pushEvent.Reset();
	m_sentEvent.Reset();

	sigset_t sset;
	if( handleSignal ){
		// このスレッドへの特定のシグナルの配送を止める
		sigemptyset(&sset);
		sigaddset(&sset, SIGHUP);
		sigaddset(&sset, SIGINT);
		sigaddset(&sset, SIGTERM);
		sigaddset(&sset, SIGUSR2);
		if( sigprocmask(SIG_BLOCK, &sset, NULL) != 0 ){
			return false;
		}
	}

	m_initialized = true;

	{
		PARAMS pa = { ID_INITIALIZED, 0, 0, 0, m_ctx };
		m_onMsg(pa);
	}

	DWORD timeout = 0xFFFFFFFF;
	while( !m_destroy ){
		if( handleSignal ){
			if( !m_pushEvent.WaitOne(0) ){
				// 特定のシグナルを待つ
				timespec ts;
				ts.tv_sec = min<DWORD>(timeout, 2000) / 1000;
				ts.tv_nsec = min<DWORD>(timeout, 2000) % 1000 * 1000000;
				int signum = sigtimedwait(&sset, NULL, &ts);
				if( signum == SIGHUP || signum == SIGINT || signum == SIGTERM ){
					PARAMS pa = { ID_SIGNAL, signum, 0, 0, m_ctx };
					if( !m_onMsg(pa) ){
						Send(ID_CLOSE);
					}
					continue;
				}
			}
		}else{
			m_pushEvent.WaitOne(timeout);
		}
		if( m_sending ){
			m_sendParams.result = Send(m_sendParams.id, m_sendParams.param1, m_sendParams.param2);
			m_sending = false;
			m_sentEvent.Set();
		}
		size_t queueSize;
		POST post;
		{
			lock_recursive_mutex lock(m_queueLock);
			timeout = 0xFFFFFFFF;
			DWORD now = GetU32Tick();
			for( auto itr = m_timerList.begin(); itr != m_timerList.end(); ++itr ){
				DWORD remain = itr->tick + itr->msec - now;
				if( remain >= 0x80000000 ){
					POST post2 = { ID_TIMER, itr->timerID, 0 };
					m_postQueue.push(post2);
					itr->tick = now;
					remain = itr->msec;
				}
				timeout = min(timeout, remain);
			}
			queueSize = m_postQueue.size();
			if( queueSize > 0 ){
				post = m_postQueue.front();
				m_postQueue.pop();
				if( queueSize > 1 ){
					m_pushEvent.Set();
				}
			}
		}
		if( queueSize > 0 && !m_destroy ){
			Send(post.id, post.param1, post.param2);
		}
	}

	if( handleSignal ){
		sigprocmask(SIG_UNBLOCK, &sset, NULL);
	}
	m_initialized = false;
	m_sentEvent.Set();
	return true;
}

#endif
