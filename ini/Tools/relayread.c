/* relayread.c: まず標準入力をすべて出力し、出力サイズだけシークした位置からファイルを出力する (2016-02-15)
 * WTFPL2 ( http://en.wikipedia.org/wiki/WTFPL#Version_2 )
 */
#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#include <fcntl.h>
#include <io.h>
#define my_fseek _fseeki64
#define my_fopen _wfopen
#define FOPEN_BINARY(mode) L ## mode L"bS"
#else
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif
#define my_fseek fseeko
#define my_fopen fopen
#define FOPEN_BINARY(mode) mode
#endif
#include <stddef.h>
#include <stdio.h>

#ifdef _WIN32
int wmain(int argc, wchar_t **argv)
#else
int main(int argc, char **argv)
#endif
{
	static char buf[65536];
	size_t nr;
	long long ofs = 0;
	FILE *fp;

	if (argc != 2) {
		fputs("Usage: relayread file_path\n", stderr);
		return 2;
	}
#ifdef _WIN32
	if (_setmode(_fileno(stdin), _O_BINARY) < 0 || _setmode(_fileno(stdout), _O_BINARY) < 0) {
		return 1;
	}
#endif
	while ((nr = fread(buf, 1, sizeof(buf), stdin)) != 0) {
		if (fwrite(buf, 1, nr, stdout) != nr) {
			return 1;
		}
		ofs += nr;
	}
	fp = my_fopen(argv[1], FOPEN_BINARY("r"));
	if (!fp) {
		return 1;
	}
	if (my_fseek(fp, ofs, SEEK_SET) != 0) {
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
