#include "stdafx.h"
#include "WriteMain.h"
#include "../../Common/PathUtil.h"
#include "../../Common/StringUtil.h"
#include <errno.h>
#include <io.h>

CWriteMain::CWriteMain(void)
	: file(NULL, fclose)
	, teeFile(NULL, fclose)
{
	this->writeBuffSize = 0;
}


CWriteMain::~CWriteMain(void)
{
	Stop();
}

void CWriteMain::SetBufferSize(
	DWORD buffSize
	)
{
	if( !this->file ){
		this->writeBuff.clear();
		this->writeBuff.reserve(buffSize);
		this->writeBuffSize = buffSize;
	}
}

void CWriteMain::SetTeeCommand(
	LPCWSTR cmd,
	DWORD buffSize,
	DWORD delayBytes
	)
{
	if( !this->teeFile ){
		this->teeCmd = cmd;
		if( this->teeCmd.empty() == false ){
			this->teeBuff.resize(max<DWORD>(buffSize, 1));
			this->teeDelay = delayBytes;
		}
	}
}

BOOL CWriteMain::Start(
	LPCWSTR fileName,
	BOOL overWriteFlag,
	ULONGLONG createSize
	)
{
	Stop();

	this->savePath = fileName;
	AddDebugLogFormat(L"★CWriteMain::Start CreateFile:%ls", this->savePath.c_str());
	UtilCreateDirectories(fs_path(this->savePath).parent_path());
	int apiErr;
	this->file.reset(UtilOpenFile(this->savePath, (overWriteFlag ? UTIL_O_CREAT_WRONLY : UTIL_O_EXCL_CREAT_WRONLY) | UTIL_SH_READ | UTIL_F_IONBF, &apiErr));
	if( !this->file ){
		int err = apiErr ? apiErr : errno;
		AddDebugLogFormat(L"★CWriteMain::Start Err:%ls%d", apiErr ? L"" : L"errno=", err);
		fs_path pathWoExt = this->savePath;
		fs_path ext = pathWoExt.extension();
		pathWoExt.replace_extension();
		for( int i = 1; ; i++ ){
			Format(this->savePath, L"%ls-(%d)%ls", pathWoExt.c_str(), i, ext.c_str());
			this->file.reset(UtilOpenFile(this->savePath, (overWriteFlag ? UTIL_O_CREAT_WRONLY : UTIL_O_EXCL_CREAT_WRONLY) | UTIL_SH_READ | UTIL_F_IONBF, &apiErr));
			if( this->file || i >= 999 ){
				err = apiErr ? apiErr : errno;
				AddDebugLogFormat(L"★CWriteMain::Start CreateFile:%ls", this->savePath.c_str());
				if( this->file ){
					break;
				}
				AddDebugLogFormat(L"★CWriteMain::Start Err:%ls%d", apiErr ? L"" : L"errno=", err);
				this->savePath = L"";
				return FALSE;
			}
		}
	}

	//ディスクに容量を確保
	if( createSize > 0 ){
		if( my_fseek(this->file.get(), createSize, SEEK_SET) == 0 ){
			SetEndOfFile((HANDLE)_get_osfhandle(_fileno(this->file.get())));
		}
		rewind(this->file.get());
	}
	this->wrotePos = 0;

	//コマンドに分岐出力
	if( this->teeCmd.empty() == false ){
		this->teeFile.reset(UtilOpenFile(this->savePath, UTIL_SHARED_READ | UTIL_F_SEQUENTIAL | UTIL_F_IONBF));
		if( this->teeFile ){
			this->teeThreadStopEvent.Reset();
			this->teeThread = thread_(TeeThread, this);
		}
	}

	return TRUE;
}

BOOL CWriteMain::Stop(
	)
{
	if( this->file ){
		if( this->writeBuff.empty() == false ){
			size_t n = fwrite(this->writeBuff.data(), 1, this->writeBuff.size(), this->file.get());
			if( n == 0 ){
				AddDebugLogFormat(L"★WriteFile Err:errno=%d", errno);
			}else{
				this->writeBuff.erase(this->writeBuff.begin(), this->writeBuff.begin() + n);
				lock_recursive_mutex lock(this->wroteLock);
				this->wrotePos += n;
			}
			//未出力のバッファは再Start()に備えて繰り越す
		}
		TruncateFile(this->file.get());
		this->file.reset();
	}
	if( this->teeThread.joinable() ){
		this->teeThreadStopEvent.Set();
		this->teeThread.join();
	}
	this->teeFile.reset();

	return TRUE;
}

wstring CWriteMain::GetSavePath(
	)
{
	return this->savePath;
}

BOOL CWriteMain::Write(
	BYTE* data,
	DWORD size,
	DWORD* writeSize
	)
{
	if( this->file && data != NULL && size > 0 ){
		*writeSize = 0;
		if( this->writeBuff.empty() == false ){
			//できるだけバッファにコピー。コピー済みデータは呼び出し側にとっては「保存済み」となる
			*writeSize = min(size, this->writeBuffSize - (DWORD)this->writeBuff.size());
			this->writeBuff.insert(this->writeBuff.end(), data, data + *writeSize);
			data += *writeSize;
			size -= *writeSize;
			if( this->writeBuff.size() >= this->writeBuffSize ){
				//バッファが埋まったので出力
				size_t n = fwrite(this->writeBuff.data(), 1, this->writeBuff.size(), this->file.get());
				if( n == 0 ){
					AddDebugLogFormat(L"★WriteFile Err:errno=%d", errno);
					TruncateFile(this->file.get());
					this->file.reset();
					return FALSE;
				}
				this->writeBuff.erase(this->writeBuff.begin(), this->writeBuff.begin() + n);
				lock_recursive_mutex lock(this->wroteLock);
				this->wrotePos += n;
			}
			if( this->writeBuff.empty() == false || size == 0 ){
				return TRUE;
			}
		}
		if( size > this->writeBuffSize ){
			//バッファサイズより大きいのでそのまま出力
			size_t n = fwrite(data, 1, size, this->file.get());
			if( n == 0 ){
				AddDebugLogFormat(L"★WriteFile Err:errno=%d", errno);
				TruncateFile(this->file.get());
				this->file.reset();
				return FALSE;
			}
			*writeSize += (DWORD)n;
			lock_recursive_mutex lock(this->wroteLock);
			this->wrotePos += n;
		}else{
			//バッファにコピー
			*writeSize += size;
			this->writeBuff.insert(this->writeBuff.end(), data, data + size);
		}
		return TRUE;
	}
	return FALSE;
}

void CWriteMain::TruncateFile(FILE* fp)
{
	SetEndOfFile((HANDLE)_get_osfhandle(_fileno(fp)));
}

void CWriteMain::TeeThread(CWriteMain* sys)
{
	wstring cmd = sys->teeCmd;
	Replace(cmd, L"$FilePath$", sys->savePath);
	vector<WCHAR> cmdBuff(cmd.c_str(), cmd.c_str() + cmd.size() + 1);
	//カレントは実行ファイルのあるフォルダ
	fs_path currentDir = GetModulePath().parent_path();

	HANDLE olEvents[] = { sys->teeThreadStopEvent.Handle(), CreateEvent(NULL, TRUE, FALSE, NULL) };
	if( olEvents[1] ){
		WCHAR pipeName[64];
		swprintf_s(pipeName, L"\\\\.\\pipe\\anon_%08x_%08x", GetCurrentProcessId(), GetCurrentThreadId());
		SECURITY_ATTRIBUTES sa = {};
		sa.nLength = sizeof(sa);
		sa.bInheritHandle = TRUE;

		//出力を速やかに打ち切るために非同期書き込みのパイプを作成する。CreatePipe()は非同期にできない
		HANDLE readPipe = CreateNamedPipe(pipeName, PIPE_ACCESS_INBOUND, 0, 1, 8192, 8192, 0, &sa);
		if( readPipe != INVALID_HANDLE_VALUE ){
			HANDLE writePipe = CreateFile(pipeName, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
			if( writePipe != INVALID_HANDLE_VALUE ){
				//標準入力にパイプしたプロセスを起動する
				STARTUPINFO si = {};
				si.cb = sizeof(si);
				si.dwFlags = STARTF_USESTDHANDLES;
				si.hStdInput = readPipe;
				//標準(エラー)出力はnulデバイスに捨てる
				si.hStdOutput = CreateFile(L"nul", GENERIC_WRITE, 0, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
				si.hStdError = CreateFile(L"nul", GENERIC_WRITE, 0, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
				PROCESS_INFORMATION pi;
				BOOL bRet = CreateProcess(NULL, cmdBuff.data(), NULL, NULL, TRUE, BELOW_NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW, NULL, currentDir.c_str(), &si, &pi);
				CloseHandle(readPipe);
				if( si.hStdOutput != INVALID_HANDLE_VALUE ){
					CloseHandle(si.hStdOutput);
				}
				if( si.hStdError != INVALID_HANDLE_VALUE ){
					CloseHandle(si.hStdError);
				}
				if( bRet ){
					CloseHandle(pi.hThread);
					CloseHandle(pi.hProcess);
					for(;;){
						LONGLONG readablePos;
						{
							lock_recursive_mutex lock(sys->wroteLock);
							readablePos = sys->wrotePos - sys->teeDelay;
						}
						LONGLONG pos;
						size_t n;
						if( (pos = my_ftell(sys->teeFile.get())) >= 0 &&
						    readablePos - pos >= (LONGLONG)sys->teeBuff.size() &&
						    (n = fread(sys->teeBuff.data(), 1, sys->teeBuff.size(), sys->teeFile.get())) > 0 ){
							OVERLAPPED ol = {};
							ol.hEvent = olEvents[1];
							if( WriteFile(writePipe, sys->teeBuff.data(), (DWORD)n, NULL, &ol) == FALSE && GetLastError() != ERROR_IO_PENDING ){
								//出力完了
								break;
							}
							if( WaitForMultipleObjects(2, olEvents, FALSE, INFINITE) != WAIT_OBJECT_0 + 1 ){
								//打ち切り
								CancelIo(writePipe);
								WaitForSingleObject(olEvents[1], INFINITE);
								break;
							}
							DWORD xferred;
							if( GetOverlappedResult(writePipe, &ol, &xferred, FALSE) == FALSE || xferred < n ){
								//出力完了
								break;
							}
						}else{
							if( WaitForSingleObject(olEvents[0], 200) != WAIT_TIMEOUT ){
								//打ち切り
								break;
							}
						}
					}
					//プロセスは回収しない(標準入力が閉じられた後にどうするかはプロセスの判断に任せる)
				}
				CloseHandle(writePipe);
			}else{
				CloseHandle(readPipe);
			}
		}
		CloseHandle(olEvents[1]);
	}
}
