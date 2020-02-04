/* readex.c: 追記中のファイルを追記が終了するまで標準出力しつづける (2016-02-15)
 *             ->同期語検査による終端判定ができるようにした (2017-04-29)
 * WTFPL2 ( http://en.wikipedia.org/wiki/WTFPL#Version_2 )
 */
#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int wmain(int argc, wchar_t **argv)
{
	static unsigned char buf[65536];
	long long ofs;
	int timeout;
	wchar_t *endp;
	int preintv = 0;
	unsigned char prebyte = 0;
	FILE *fp;
	size_t ntr;
	size_t nr;
	int i;
	int retry = 0;

	if (argc != 4) {
		fputs("Usage: readex seek_offset|-seek_end_offset close_timeout_sec[p{interval*256+preamble_byte}] file_path\n", stderr);
		return 2;
	}
	if (_setmode(_fileno(stdout), _O_BINARY) < 0) {
		return 1;
	}
	ofs = _wcstoi64(argv[1], NULL, 10);
	timeout = wcstol(argv[2], &endp, 10);
	if (*endp == L'p') {
		// 終端判定に同期語検査を加える
		preintv = wcstol(endp + 1, NULL, 10);
		if (preintv > 0) {
			prebyte = (unsigned char)preintv;
			preintv >>= 8;
		}
		if (preintv <= 0 || preintv > sizeof(buf)) {
			return 1;
		}
	}
	fp = _wfopen(argv[3], L"rbS");
	if (!fp) {
		return 1;
	}
	if (_fseeki64(fp, ofs < 0 ? ofs + 1 : ofs, ofs < 0 ? SEEK_END : SEEK_SET) != 0) {
		fclose(fp);
		return 1;
	}
	ntr = preintv ? 1 : sizeof(buf);
	for (;;) {
		nr = fread(buf, 1, ntr, fp);
		i = -1;
		if (preintv && nr == ntr) {
			for (i = (int)nr - 1; i >= 0 && buf[i] == prebyte; i -= preintv);
		}
		if (nr == 0 || i >= 0) {
			if (nr == 0) {
				// 同期語検査を終了
				preintv = 0;
				ntr = sizeof(buf);
			} else {
				_fseeki64(fp, -(int)nr, SEEK_CUR);
			}
			if (++retry > 5 * timeout) {
				break;
			}
			Sleep(200);
		} else {
			retry = 0;
			if (fwrite(buf, 1, nr, stdout) != nr) {
				break;
			}
			if (nr != ntr) {
				// 同期語検査を終了
				preintv = 0;
			}
			ntr = preintv ? preintv * (sizeof(buf) / preintv) : sizeof(buf);
		}
	}
	fclose(fp);
	fflush(stdout);
	return 0;
}

#ifdef __MINGW32__
#include <shellapi.h>

int main(void)
{
	int argc;
	wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (argv) {
		int ret = wmain(argc, argv);
		LocalFree(argv);
		return ret;
	}
	return 1;
}
#endif
