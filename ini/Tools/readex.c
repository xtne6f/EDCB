/* readex.c: 追記中のファイルを追記が終了するまで標準出力しつづける (2016-02-15)
 * WTFPL2 ( http://en.wikipedia.org/wiki/WTFPL#Version_2 )
 */
#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <windows.h>

int wmain(int argc, wchar_t **argv)
{
	static char buf[65536];
	long long ofs;
	int timeout;
	FILE *fp;
	size_t nr;
	int retry = 0;

	if (argc != 4) {
		fputs("Usage: readex seek_offset|-seek_end_offset close_timeout_sec file_path\n", stderr);
		return 2;
	}
	if (_setmode(_fileno(stdout), _O_BINARY) < 0) {
		return 1;
	}
	ofs = _wtoi64(argv[1]);
	timeout = _wtoi(argv[2]);
	fp = _wfopen(argv[3], L"rbS");
	if (!fp) {
		return 1;
	}
	if (_fseeki64(fp, ofs < 0 ? ofs + 1 : ofs, ofs < 0 ? SEEK_END : SEEK_SET) != 0) {
		fclose(fp);
		return 1;
	}
	for (;;) {
		nr = fread(buf, 1, sizeof(buf), fp);
		if (nr == 0) {
			if (++retry > 5 * timeout) {
				break;
			}
			Sleep(200);
		} else {
			retry = 0;
			if (fwrite(buf, 1, nr, stdout) != nr) {
				break;
			}
		}
	}
	fclose(fp);
	fflush(stdout);
	return 0;
}
