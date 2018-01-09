#ifndef INCLUDE_THREAD_UTIL_H
#define INCLUDE_THREAD_UTIL_H

#if 1
#include <process.h>
#include <exception>
#include <stdexcept>

class thread_
{
public:
	thread_() : m_h(nullptr) {}
	template<class Arg> thread_(void(*f)(Arg), Arg arg) {
		struct Th {
			static UINT WINAPI func(void* p) {
				Th th = *static_cast<Th*>(p);
				InterlockedDecrement(&static_cast<Th*>(p)->b);
				th.f(th.arg);
				return 0;
			}
			LONG b;
			void(*f)(Arg);
			Arg arg;
		} th = { 1, f, arg };
		m_h = reinterpret_cast<void*>(_beginthreadex(nullptr, 0, Th::func, &th, 0, nullptr));
		if (!m_h) throw std::runtime_error("");
		while (th.b) Sleep(0);
	}
	~thread_() { if (m_h) std::terminate(); }
	bool joinable() const { return !!m_h; }
	void* native_handle() { return m_h; }
	void join() {
		if (!m_h || WaitForSingleObject(m_h, INFINITE) != WAIT_OBJECT_0) throw std::runtime_error("");
		CloseHandle(m_h);
		m_h = nullptr;
	}
	void detach() {
		if (!m_h) throw std::runtime_error("");
		CloseHandle(m_h);
		m_h = nullptr;
	}
	thread_(thread_&& o) : m_h(o.m_h) { o.m_h = nullptr; }
	thread_& operator=(thread_&& o) { if (m_h) std::terminate(); m_h = o.m_h; o.m_h = nullptr; return *this; }
private:
	thread_(const thread_&);
	thread_& operator=(const thread_&);
	void* m_h;
};

class recursive_mutex_
{
public:
	recursive_mutex_() { InitializeCriticalSection(&m_cs); }
	~recursive_mutex_() { DeleteCriticalSection(&m_cs); }
	void lock() { EnterCriticalSection(&m_cs); }
	void unlock() { LeaveCriticalSection(&m_cs); }
private:
	recursive_mutex_(const recursive_mutex_&);
	recursive_mutex_& operator=(const recursive_mutex_&);
	CRITICAL_SECTION m_cs;
};

#else
#include <thread>
#include <mutex>
typedef std::thread thread_;
typedef std::recursive_mutex recursive_mutex_;
#endif

class CBlockLock
{
public:
	CBlockLock(recursive_mutex_* mtx) : m_mtx(mtx) { m_mtx->lock(); }
	~CBlockLock() { m_mtx->unlock(); }
private:
	CBlockLock(const CBlockLock&);
	CBlockLock& operator=(const CBlockLock&);
	recursive_mutex_* m_mtx;
};

class CAutoResetEvent
{
public:
	CAutoResetEvent(bool initialState = false) {
		m_h = CreateEvent(nullptr, FALSE, initialState, nullptr);
		if (!m_h) throw std::runtime_error("");
	}
	~CAutoResetEvent() { CloseHandle(m_h); }
	void Set() { SetEvent(m_h); }
	void Reset() { ResetEvent(m_h); }
	HANDLE Handle() { return m_h; }
private:
	CAutoResetEvent(const CAutoResetEvent&);
	CAutoResetEvent& operator=(const CAutoResetEvent&);
	HANDLE m_h;
};

#endif
