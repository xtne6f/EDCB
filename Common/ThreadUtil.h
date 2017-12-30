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
		if (!m_h) std::runtime_error("");
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

#else
#include <thread>
typedef std::thread thread_;
#endif

#endif
