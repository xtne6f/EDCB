#ifndef INCLUDE_THREAD_UTIL_H
#define INCLUDE_THREAD_UTIL_H

#include <atomic>
#include <mutex>
#include <stdexcept>
#include <thread>
#ifndef _WIN32
#include <chrono>
#include <errno.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#endif

typedef std::lock_guard<std::recursive_mutex> lock_recursive_mutex;

inline void SleepForMsec(DWORD msec)
{
#ifdef _WIN32
	Sleep(msec);
#else
	std::this_thread::sleep_for(std::chrono::milliseconds(msec));
#endif
}

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
