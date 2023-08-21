#pragma once

class CWritePlugInUtil
{
public:
	CWritePlugInUtil(void);
	~CWritePlugInUtil(void);

	//DLLの初期化
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// loadDllFilePath		[IN]ロードするDLLパス
	BOOL Initialize(
		const wstring& loadDllFilePath
		);

	//DLLの解放
	void UnInitialize(
		);

	//ファイル保存を開始する
	//戻り値：
	// TRUE（成功）、FALSE（失敗）、ERR_NOT_INIT（未初期化）
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
	// TRUE（成功）、FALSE（失敗）、ERR_NOT_INIT（未初期化）
	BOOL Stop(
		);

	//実際に保存しているファイルパスを取得する（再生やバッチ処理に利用される）
	//filePathがNULL時は必要なサイズをfilePathSizeで返す
	//通常filePathSize=512で呼び出し
	//戻り値：
	// TRUE（成功）、FALSE（失敗）、ERR_NOT_INIT（未初期化）
	//引数：
	// filePath				[OUT]保存ファイルフルパス
	// filePathSize			[IN/OUT]filePathのサイズ(WCHAR単位)
	BOOL GetSavePath(
		WCHAR* filePath,
		DWORD* filePathSize
		);

	//保存用TSデータを送る
	//空き容量不足などで書き出し失敗した場合、writeSizeの値を元に
	//再度保存処理するときの送信開始地点を決める
	//戻り値：
	// TRUE（成功）、FALSE（失敗）、ERR_NOT_INIT（未初期化）
	//引数：
	// data					[IN]TSデータ
	// size					[IN]dataのサイズ
	// writeSize			[OUT]保存に利用したサイズ
	BOOL Write(
		BYTE* data,
		DWORD size,
		DWORD* writeSize
		);

private:
	typedef BOOL (WINAPI *CreateCtrlWP)(DWORD* id);
	typedef BOOL (WINAPI *DeleteCtrlWP)(DWORD id);
	typedef BOOL (WINAPI *StartSaveWP)(DWORD id, LPCWSTR fileName, BOOL overWriteFlag, ULONGLONG createSize);
	typedef BOOL (WINAPI *StopSaveWP)(DWORD id);
	typedef BOOL (WINAPI *GetSaveFilePathWP)(DWORD id, WCHAR* filePath, DWORD* filePathSize);
	typedef BOOL (WINAPI *AddTSBuffWP)(DWORD id, BYTE* data, DWORD size, DWORD* writeSize);

	std::unique_ptr<void, void(*)(void*)> module;
	DWORD id;

	DeleteCtrlWP				pfnDeleteCtrlWP;
	StartSaveWP					pfnStartSaveWP;
	StopSaveWP					pfnStopSaveWP;
	GetSaveFilePathWP			pfnGetSaveFilePathWP;
	AddTSBuffWP					pfnAddTSBuffWP;
};

