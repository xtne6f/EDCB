#pragma once

#include "../../Common/ThreadUtil.h"

class CWriteMain
{
public:
	CWriteMain(void);
	~CWriteMain(void);

	//出力バッファサイズを設定する
	//引数：
	// buffSize				[IN]バッファサイズ
	void SetBufferSize(
		DWORD buffSize
		);

	//同時出力用のコマンドを設定する
	//引数：
	// cmd					[IN]コマンド
	// buffSize				[IN]出力のコマンド転送用のバッファサイズ
	// delayBytes			[IN]出力のコマンド転送までの遅延量
	void SetTeeCommand(
		LPCWSTR cmd,
		DWORD buffSize,
		DWORD delayBytes
		);

	//ファイル保存を開始する
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// fileName				[IN]保存ファイルフルパス（必要に応じて拡張子変えたりなど行う）
	// overWriteFlag		[IN]同一ファイル名存在時に上書きするかどうか（TRUE：する、FALSE：しない）
	// createSize			[IN]入力予想容量（188バイトTSでの容量。即時録画時など総時間未定の場合は0。延長などの可能性もあるので目安程度）
	BOOL Start(
		LPCWSTR fileName,
		BOOL overWriteFlag,
		ULONGLONG createSize
		);

	//ファイル保存を終了する
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	BOOL Stop(
		);

	//実際に保存しているファイルパスを取得する（再生やバッチ処理に利用される）
	//戻り値：
	// 保存ファイルフルパス
	wstring GetSavePath(
		);

	//保存用TSデータを送る
	//空き容量不足などで書き出し失敗した場合、writeSizeの値を元に
	//再度保存処理するときの送信開始地点を決める
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// data					[IN]TSデータ
	// size					[IN]dataのサイズ
	// writeSize			[OUT]保存に利用したサイズ
	BOOL Write(
		BYTE* data,
		DWORD size,
		DWORD* writeSize
		);

protected:
	std::unique_ptr<FILE, fclose_deleter> file;
	wstring savePath;

	vector<BYTE> writeBuff;
	DWORD writeBuffSize;
	LONGLONG wrotePos;
	recursive_mutex_ wroteLock;

	std::unique_ptr<FILE, fclose_deleter> teeFile;
	thread_ teeThread;
	CAutoResetEvent teeThreadStopEvent;
	wstring teeCmd;
	vector<BYTE> teeBuff;
	DWORD teeDelay;

	static void TruncateFile(FILE* fp);
	static void TeeThread(CWriteMain* sys);
};

