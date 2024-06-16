/* asyncbuf.c: 標準入力をバッファして標準出力する (2020-08-23)
 * WTFPL2 ( http://en.wikipedia.org/wiki/WTFPL#Version_2 )
 */
#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#include <fcntl.h>
#include <io.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>
#define my_mutex_lock EnterCriticalSection
#define my_mutex_unlock LeaveCriticalSection
#else
#include <pthread.h>
#define my_mutex_lock pthread_mutex_lock
#define my_mutex_unlock pthread_mutex_unlock
#endif
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

static const size_t BUF_UNIT_SIZE = 65536;
#ifdef _WIN32
static HANDLE g_evenq;
static HANDLE g_evdeq;
static CRITICAL_SECTION g_cs;
static HANDLE g_th;
#else
static int g_state_enq;
static int g_state_deq;
static pthread_cond_t g_cond_enq;
static pthread_cond_t g_cond_deq;
static pthread_mutex_t g_cs;
static pthread_t g_th;
#endif
static char *g_buf;
static size_t g_size;
static size_t g_fill;
static size_t g_front;
static size_t g_rear;
static int g_closing;

#ifdef _WIN32
static UINT WINAPI worker(void *p)
#else
static void *worker(void *p)
#endif
{
	(void)p;
	for (;;) {
		size_t remain;
		my_mutex_lock(&g_cs);
		remain = (g_size + g_rear - g_front) % g_size;
		if ((g_closing || remain >= g_fill) && remain >= (g_closing ? 1 : BUF_UNIT_SIZE)) {
			size_t n = remain < BUF_UNIT_SIZE ? remain : BUF_UNIT_SIZE;
			my_mutex_unlock(&g_cs);
			g_fill = 0;
			if (n > g_size - g_front) {
				n = g_size - g_front;
			}
			if (fwrite(g_buf + g_front, 1, n, stdout) != n) {
				n = 0;
			}
			my_mutex_lock(&g_cs);
			if (n == 0) {
				g_closing = 1;
			}
			g_front = (g_front + n) % g_size;
#ifdef _WIN32
			SetEvent(g_evdeq);
#else
			g_state_deq = 1;
			pthread_cond_signal(&g_cond_deq);
#endif
			my_mutex_unlock(&g_cs);
			if (n == 0) {
				break;
			}
		} else if (g_closing) {
			my_mutex_unlock(&g_cs);
			break;
		} else {
#ifdef _WIN32
			my_mutex_unlock(&g_cs);
			WaitForSingleObject(g_evenq, INFINITE);
#else
			while (!g_state_enq) {
				pthread_cond_wait(&g_cond_enq, &g_cs);
			}
			g_state_enq = 0;
			my_mutex_unlock(&g_cs);
#endif
		}
	}
	fflush(stdout);
	return 0;
}

#ifdef _WIN32
int wmain(int argc, wchar_t **argv)
#else
int main(int argc, char **argv)
#endif
{
	int ok = 0;

	if (argc != 3) {
		fputs("Usage: asyncbuf buf_size_bytes initial_fill_bytes\n", stderr);
		return 2;
	}
#ifdef _WIN32
	if (_setmode(_fileno(stdin), _O_BINARY) < 0 || _setmode(_fileno(stdout), _O_BINARY) < 0) {
		return 1;
	}
	g_size = wcstoul(argv[1], NULL, 10);
	g_fill = wcstoul(argv[2], NULL, 10);
#else
	g_size = strtoul(argv[1], NULL, 10);
	g_fill = strtoul(argv[2], NULL, 10);
#endif
	if (g_size < BUF_UNIT_SIZE * 2) {
		g_size = BUF_UNIT_SIZE * 2;
	} else if (g_size > 1024 * 1024 * 1024) {
		g_size = 1024 * 1024 * 1024;
	}
	if (g_fill > g_size - BUF_UNIT_SIZE * 2) {
		g_fill = g_size - BUF_UNIT_SIZE * 2;
	}
	g_buf = (char *)malloc(g_size);
	if (!g_buf) {
		return 1;
	}

#ifdef _WIN32
	g_evenq = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (g_evenq) {
		g_evdeq = CreateEvent(NULL, FALSE, FALSE, NULL);
		if (g_evdeq) {
			InitializeCriticalSection(&g_cs);
			g_th = (HANDLE)_beginthreadex(NULL, 0, worker, NULL, 0, NULL);
			if (g_th) {
				ok = 1;
			}
			else {
				DeleteCriticalSection(&g_cs);
				CloseHandle(g_evdeq);
			}
		}
		if (!ok) {
			CloseHandle(g_evenq);
		}
	}
#else
	if (pthread_cond_init(&g_cond_enq, NULL) == 0) {
		if (pthread_cond_init(&g_cond_deq, NULL) == 0) {
			if (pthread_mutex_init(&g_cs, NULL) == 0) {
				if (pthread_create(&g_th, NULL, worker, NULL) == 0) {
					ok = 1;
				}
				else {
					pthread_mutex_destroy(&g_cs);
				}
			}
			if (!ok) {
				pthread_cond_destroy(&g_cond_deq);
			}
		}
		if (!ok) {
			pthread_cond_destroy(&g_cond_enq);
		}
	}
#endif
	if (!ok) {
		free(g_buf);
		return 1;
	}

	for (;;) {
		my_mutex_lock(&g_cs);
		if ((g_size + g_front - g_rear - 1) % g_size >= BUF_UNIT_SIZE) {
			size_t n = BUF_UNIT_SIZE;
			my_mutex_unlock(&g_cs);
			if (n > g_size - g_rear) {
				n = g_size - g_rear;
			}
			n = fread(g_buf + g_rear, 1, n, stdin);
			my_mutex_lock(&g_cs);
			if (n == 0) {
				g_closing = 1;
			}
			g_rear = (g_rear + n) % g_size;
#ifdef _WIN32
			SetEvent(g_evenq);
#else
			g_state_enq = 1;
			pthread_cond_signal(&g_cond_enq);
#endif
			my_mutex_unlock(&g_cs);
			if (n == 0) {
				break;
			}
		} else if (g_closing) {
			my_mutex_unlock(&g_cs);
			break;
		} else {
#ifdef _WIN32
			my_mutex_unlock(&g_cs);
			WaitForSingleObject(g_evdeq, INFINITE);
#else
			while (!g_state_deq) {
				pthread_cond_wait(&g_cond_deq, &g_cs);
			}
			g_state_deq = 0;
			my_mutex_unlock(&g_cs);
#endif
		}
	}

#ifdef _WIN32
	WaitForSingleObject(g_th, INFINITE);
	CloseHandle(g_th);
	DeleteCriticalSection(&g_cs);
	CloseHandle(g_evdeq);
	CloseHandle(g_evenq);
#else
	pthread_join(g_th, NULL);
	pthread_mutex_destroy(&g_cs);
	pthread_cond_destroy(&g_cond_deq);
	pthread_cond_destroy(&g_cond_enq);
#endif
	free(g_buf);
	return 0;
}
