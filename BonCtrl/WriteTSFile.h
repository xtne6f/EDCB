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
	BOOL EndSave();

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
	//引数：
	// filePath			[OUT]保存ファイル名
	// subRecFlag		[OUT]サブ録画が発生したかどうか
	void GetSaveFilePath(
		wstring* filePath,
		BOOL* subRecFlag_
		);

	//録画中のファイルの出力サイズを取得する
	//引数：
	// writeSize			[OUT]保存ファイル名
	void GetRecWriteSize(
		__int64* writeSize
		);

protected:
	//保存サブフォルダから空きのあるフォルダパスを取得
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// needFreeSize			[IN]最低必要な空きサイズ
	// freeFolderPath		[OUT]見つかったフォルダ
	BOOL GetFreeFolder(
		ULONGLONG needFreeSize,
		wstring& freeFolderPath
	);

	//指定フォルダの空きがあるかチェック
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// needFreeSize			[IN]最低必要な空きサイズ
	// chkFolderPath		[IN]指定フォルダ
	BOOL ChkFreeFolder(
		ULONGLONG needFreeSize,
		const wstring& chkFolderPath
	);

	static void OutThread(CWriteTSFile* sys);

protected:
	recursive_mutex_ outThreadLock;
	std::list<vector<BYTE>> tsBuffList;
	std::list<vector<BYTE>> tsFreeList;

	thread_ outThread;
	BOOL outStopFlag;
	BOOL outStartFlag;

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
	__int64 writeTotalSize;
	wstring mainSaveFilePath;

	int maxBuffCount;
};

