#ifndef INCLUDE_THREAD_UTIL_H
#define INCLUDE_THREAD_UTIL_H

#ifdef _WIN32
#include <process.h>
#include <exception>
#include <stdexcept>

class atomic_int_
{
public:
	atomic_int_() {}
	atomic_int_(int val) : m_val(val) {}
	int operator=(int val) { exchange(val); return val; }
	operator int() const { return InterlockedExchangeAdd(&m_val, 0); }
	int exchange(int val) { return InterlockedExchange(&m_val, val); }
	int operator++(int) { return InterlockedIncrement(&m_val); }
	int operator--(int) { return InterlockedDecrement(&m_val); }
private:
	atomic_int_(const atomic_int_&);
	atomic_int_& operator=(const atomic_int_&);
	mutable LONG m_val;
};

class atomic_bool_
{
public:
	atomic_bool_() {}
	atomic_bool_(bool val) : m_val(val) {}
	bool operator=(bool val) { return !!(m_val = val); }
	operator bool() const { return !!m_val; }
	bool exchange(bool val) { return !!m_val.exchange(val); }
private:
	atomic_int_ m_val;
};

class thread_
{
public:
	thread_() : m_h(nullptr) {}
	explicit thread_(void(*f)()) {
		struct Th {
			static UINT WINAPI func(void* p) {
				reinterpret_cast<void(*)()>(p)();
				return 0;
			}
		};
		m_h = reinterpret_cast<void*>(_beginthreadex(nullptr, 0, Th::func, reinterpret_cast<void*>(f), 0, nullptr));
		if (!m_h) throw std::runtime_error("");
	}
	template<class Arg> thread_(void(*f)(Arg), Arg arg) {
		struct Th {
			static UINT WINAPI func(void* p) {
				std::unique_ptr<Th> th(static_cast<Th*>(p));
				th->f(std::move(th->arg));
				return 0;
			}
			Th(void(*thf)(Arg), Arg tharg) : f(thf), arg(tharg) {}
			void(*f)(Arg);
			Arg arg;
		};
		Th* pth = new Th(f, arg);
		m_h = reinterpret_cast<void*>(_beginthreadex(nullptr, 0, Th::func, pth, 0, nullptr));
		if (!m_h) {
			delete pth;
			throw std::runtime_error("");
		}
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

class lock_recursive_mutex
{
public:
	lock_recursive_mutex(recursive_mutex_& mtx) : m_mtx(mtx) { m_mtx.lock(); }
	~lock_recursive_mutex() { m_mtx.unlock(); }
private:
	lock_recursive_mutex(const lock_recursive_mutex&);
	lock_recursive_mutex& operator=(const lock_recursive_mutex&);
	recursive_mutex_& m_mtx;
};

inline void SleepForMsec(DWORD msec)
{
	Sleep(msec);
}

#else
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <stdexcept>
#include <errno.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <unistd.h>
typedef std::atomic_int atomic_int_;
typedef std::atomic_bool atomic_bool_;
typedef std::thread thread_;
typedef std::recursive_mutex recursive_mutex_;
typedef std::lock_guard<recursive_mutex_> lock_recursive_mutex;

inline void SleepForMsec(DWORD msec)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(msec));
}

#endif

class CAutoResetEvent
{
public:
#ifdef _WIN32
	CAutoResetEvent(bool initialState = false) {
		m_h = CreateEvent(nullptr, FALSE, initialState, nullptr);
		if (!m_h) throw std::runtime_error("");
	}
	~CAutoResetEvent() { CloseHandle(m_h); }
	void Set() { SetEvent(m_h); }
	void Reset() { ResetEvent(m_h); }
	HANDLE Handle() { return m_h; }
	bool WaitOne(DWORD timeout = 0xFFFFFFFF) { return WaitForSingleObject(m_h, timeout) == WAIT_OBJECT_0; }
#else
	CAutoResetEvent(bool initialState = false) {
		m_efd = eventfd(initialState, EFD_CLOEXEC | EFD_NONBLOCK);
		if (m_efd == -1) throw std::runtime_error("");
	}
	~CAutoResetEvent() { close(m_efd); }
	void Set() {
		LONGLONG n = 1;
		while (write(m_efd, &n, sizeof(n)) < 0) {
			if (errno != EAGAIN) throw std::runtime_error("");
		}
	}
	void Reset() { WaitOne(0); }
	int Handle() { return m_efd; }
	bool WaitOne(DWORD timeout = 0xFFFFFFFF) {
		LONGLONG n;
		while (read(m_efd, &n, sizeof(n)) < 0) {
			if (errno != EAGAIN) throw std::runtime_error("");
			if (!timeout) return false;
			pollfd pfd;
			pfd.fd = m_efd;
			pfd.events = POLLIN;
			if (poll(&pfd, 1, timeout < 0x80000000 ? (int)timeout : -1) < 0 && errno != EINTR) {
				throw std::runtime_error("");
			}
			if (timeout < 0x80000000) {
				// シグナル発生時や競合時はtimeoutよりも早くタイムアウトするので注意
				timeout = 0;
			}
		}
		return true;
	}
#endif
private:
	CAutoResetEvent(const CAutoResetEvent&);
	CAutoResetEvent& operator=(const CAutoResetEvent&);
#ifdef _WIN32
	HANDLE m_h;
#else
	int m_efd;
#endif
};

#endif
