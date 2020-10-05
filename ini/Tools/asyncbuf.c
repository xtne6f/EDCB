/* asyncbuf.c: 標準入力をバッファして標準出力する (2020-08-23)
 * WTFPL2 ( http://en.wikipedia.org/wiki/WTFPL#Version_2 )
 */
#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>

static const size_t BUF_UNIT_SIZE = 65536;
static HANDLE g_evenq;
static HANDLE g_evdeq;
static CRITICAL_SECTION g_cs;
static HANDLE g_th;
static char *g_buf;
static size_t g_size;
static size_t g_fill;
static size_t g_front;
static size_t g_rear;
static int g_closing;

static UINT WINAPI worker(void *p)
{
	(void)p;
	for (;;) {
		size_t remain;
		EnterCriticalSection(&g_cs);
		remain = (g_size + g_rear - g_front) % g_size;
		if ((g_closing || remain >= g_fill) && remain >= (g_closing ? 1 : BUF_UNIT_SIZE)) {
			size_t n = remain < BUF_UNIT_SIZE ? remain : BUF_UNIT_SIZE;
			LeaveCriticalSection(&g_cs);
			g_fill = 0;
			if (n > g_size - g_front) {
				n = g_size - g_front;
			}
			if (fwrite(g_buf + g_front, 1, n, stdout) != n) {
				EnterCriticalSection(&g_cs);
				g_closing = 1;
				LeaveCriticalSection(&g_cs);
				SetEvent(g_evdeq);
				break;
			}
			EnterCriticalSection(&g_cs);
			g_front = (g_front + n) % g_size;
			LeaveCriticalSection(&g_cs);
			SetEvent(g_evdeq);
		} else if (g_closing) {
			LeaveCriticalSection(&g_cs);
			break;
		} else {
			LeaveCriticalSection(&g_cs);
			WaitForSingleObject(g_evenq, INFINITE);
		}
	}
	fflush(stdout);
	return 0;
}

int wmain(int argc, wchar_t **argv)
{
	if (argc != 3) {
		fputs("Usage: asyncbuf buf_size_bytes initial_fill_bytes\n", stderr);
		return 2;
	}
	if (_setmode(_fileno(stdin), _O_BINARY) < 0 || _setmode(_fileno(stdout), _O_BINARY) < 0) {
		return 1;
	}
	g_size = wcstoul(argv[1], NULL, 10);
	if (g_size < BUF_UNIT_SIZE * 2) {
		g_size = BUF_UNIT_SIZE * 2;
	} else if (g_size > 1024 * 1024 * 1024) {
		g_size = 1024 * 1024 * 1024;
	}
	g_fill = wcstoul(argv[2], NULL, 10);
	if (g_fill > g_size - BUF_UNIT_SIZE * 2) {
		g_fill = g_size - BUF_UNIT_SIZE * 2;
	}
	g_buf = (char *)malloc(g_size);
	if (!g_buf) {
		return 1;
	}
	g_evenq = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (!g_evenq) {
		free(g_buf);
		return 1;
	}
	g_evdeq = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (!g_evdeq) {
		CloseHandle(g_evenq);
		free(g_buf);
		return 1;
	}
	InitializeCriticalSection(&g_cs);
	g_th = (HANDLE)_beginthreadex(NULL, 0, worker, NULL, 0, NULL);
	if (!g_th) {
		DeleteCriticalSection(&g_cs);
		CloseHandle(g_evdeq);
		CloseHandle(g_evenq);
		free(g_buf);
		return 1;
	}

	for (;;) {
		EnterCriticalSection(&g_cs);
		if ((g_size + g_front - g_rear - 1) % g_size >= BUF_UNIT_SIZE) {
			size_t n = BUF_UNIT_SIZE;
			LeaveCriticalSection(&g_cs);
			if (n > g_size - g_rear) {
				n = g_size - g_rear;
			}
			n = fread(g_buf + g_rear, 1, n, stdin);
			if (n == 0) {
				EnterCriticalSection(&g_cs);
				g_closing = 1;
				LeaveCriticalSection(&g_cs);
				SetEvent(g_evenq);
				break;
			}
			EnterCriticalSection(&g_cs);
			g_rear = (g_rear + n) % g_size;
			LeaveCriticalSection(&g_cs);
			SetEvent(g_evenq);
		} else if (g_closing) {
			LeaveCriticalSection(&g_cs);
			break;
		} else {
			LeaveCriticalSection(&g_cs);
			WaitForSingleObject(g_evdeq, INFINITE);
		}
	}

	WaitForSingleObject(g_th, INFINITE);
	CloseHandle(g_th);
	DeleteCriticalSection(&g_cs);
	CloseHandle(g_evdeq);
	CloseHandle(g_evenq);
	free(g_buf);
	return 0;
}
