/* relayread.c: まず標準入力をすべて出力し、出力サイズだけシークした位置からファイルを出力する (2016-02-15)
 * WTFPL2 ( http://en.wikipedia.org/wiki/WTFPL#Version_2 )
 */
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <fcntl.h>
#include <io.h>

int wmain(int argc, wchar_t **argv)
{
	static char buf[65536];
	size_t nr;
	long long ofs = 0;
	FILE *fp;

	if (argc != 2) {
		fputs("Usage: relayread file_path\n", stderr);
		return 2;
	}
	if (_setmode(_fileno(stdin), _O_BINARY) < 0 || _setmode(_fileno(stdout), _O_BINARY) < 0) {
		return 1;
	}
	while ((nr = fread(buf, 1, sizeof(buf), stdin)) != 0) {
		if (fwrite(buf, 1, nr, stdout) != nr) {
			return 1;
		}
		ofs += nr;
	}
	fp = _wfopen(argv[1], L"rbS");
	if (!fp) {
		return 1;
	}
	if (_fseeki64(fp, ofs, SEEK_SET) != 0) {
		fclose(fp);
		return 1;
	}
	while ((nr = fread(buf, 1, sizeof(buf), fp)) != 0) {
		if (fwrite(buf, 1, nr, stdout) != nr) {
			fclose(fp);
			return 1;
		}
	}
	fclose(fp);
	fflush(stdout);
	return 0;
}
