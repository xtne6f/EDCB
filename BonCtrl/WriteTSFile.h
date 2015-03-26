#pragma once

#include "../Common/Util.h"
#include "../Common/StructDef.h"
#include "../Common/ErrDef.h"
#include "../Common/StringUtil.h"
#include "../Common//WritePlugInUtil.h"

#include "BonCtrlDef.h"

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
		wstring fileName,
		BOOL overWriteFlag,
		ULONGLONG createSize,
		vector<REC_FILE_SET_INFO>* saveFolder,
		vector<wstring>* saveFolderSub,
		int maxBuffCount
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
		BOOL* subRecFlag
		);

	//録画中のファイルの出力サイズを取得する
	//引数：
	// writeSize			[OUT]保存ファイル名
	void GetRecWriteSize(
		__int64* writeSize
		);

protected:
	/*
	HANDLE OpenFile(
		wstring recFolderPath,
		wstring recFileName,
		BOOL overWriteFlag,
		ULONGLONG createSize,
		BOOL chkFree,
		wstring& recFilePath
		);
*/
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
		wstring chkFolderPath
	);

	static UINT WINAPI OutThread(LPVOID param);

	BOOL GetNextFileName(wstring recFolder, wstring fileName, wstring& recPath);

protected:
	CRITICAL_SECTION outThreadLock;
	vector<TS_DATA*> TSBuff;
	DWORD totalTSBuffSize;

	HANDLE outThread;
	HANDLE outStopEvent;

	typedef struct _SAVE_INFO{
		CWritePlugInUtil* writeUtil;
		BOOL freeChk;
		BOOL overWriteFlag;
		wstring recFilePath;
		vector<wstring> subRecPath;
		wstring recFileName;
		_SAVE_INFO(void){
			freeChk = FALSE;
			overWriteFlag = FALSE;
			recFilePath = L"";
			writeUtil = NULL;
			recFileName = L"";
		};
		~_SAVE_INFO(void){
			if( writeUtil != NULL ){
				writeUtil->UnInitialize();
				SAFE_DELETE(writeUtil);
			}
		};
	}SAVE_INFO;
	vector<SAVE_INFO*> fileList;

	wstring saveFileName;
	BOOL overWriteFlag;
	vector<REC_FILE_SET_INFO> saveFolder;
	vector<wstring> saveFolderSub;

	BOOL subRecFlag;
	__int64 writeTotalSize;
	wstring mainSaveFilePath;

	BOOL exceptionErr;
	int maxBuffCount;
	BOOL buffOverErr;
};

