#pragma once

#include "../Common/StructDef.h"
#include "../Common/ErrDef.h"
#include "../Common/StringUtil.h"
#include "../Common/ThreadUtil.h"
#include "../Common/WritePlugInUtil.h"
#include <list>

class CWriteTSFile
{
public:
	//フォルダに空きがあるとみなす最低容量
	static const int FREE_FOLDER_MIN_BYTES = 200 * 1024 * 1024;

	CWriteTSFile(void);
	~CWriteTSFile(void);

	//ファイル保存を開始する
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// fileName				[IN]保存ファイルパス
	// overWriteFlag		[IN]同一ファイル名存在時に上書きするかどうか（TRUE：する、FALSE：しない）
	// createSize			[IN]ファイル作成時にディスクに予約する容量
	// saveFolder			[IN]使用するフォルダ一覧
	// saveFolderSub		[IN]HDDの空きがなくなった場合に一時的に使用するフォルダ
	BOOL StartSave(
		const wstring& fileName,
		BOOL overWriteFlag_,
		ULONGLONG createSize_,
		const vector<REC_FILE_SET_INFO>& saveFolder,
		const vector<wstring>& saveFolderSub_,
		int maxBuffCount_
	);

	//ファイル保存を終了する
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// subRecFlag			[OUT]成功のとき、サブ録画が発生したかどうか
	BOOL EndSave(BOOL* subRecFlag_ = NULL);

	//保存中かどうか
	BOOL IsRec() { return this->outThread.joinable(); }

	//出力用TSデータを送る
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// data		[IN]TSデータ
	// size		[IN]dataのサイズ
	BOOL AddTSBuff(
		BYTE* data,
		DWORD size
		);

	//録画中のファイルのファイルパスを取得する
	//戻り値：
	// ファイルパス
	wstring GetSaveFilePath();

	//録画中のファイルの出力サイズを取得する
	//引数：
	// writeSize			[OUT]保存ファイル名
	void GetRecWriteSize(
		LONGLONG* writeSize
		);

protected:
	static void OutThread(CWriteTSFile* sys);

protected:
	recursive_mutex_ outThreadLock;
	std::list<vector<BYTE>> tsBuffList;
	std::list<vector<BYTE>> tsFreeList;

	thread_ outThread;
	atomic_int_ outStopState;
	CAutoResetEvent outStopEvent;

	struct SAVE_INFO {
		CWritePlugInUtil writeUtil;
		BOOL freeChk;
		wstring writePlugIn;
		wstring recFolder;
		wstring recFileName;
	};
	vector<std::unique_ptr<SAVE_INFO>> fileList;

	BOOL overWriteFlag;
	ULONGLONG createSize;
	vector<wstring> saveFolderSub;

	BOOL subRecFlag;
	LONGLONG writeTotalSize;
	wstring mainSaveFilePath;

	int maxBuffCount;
};

