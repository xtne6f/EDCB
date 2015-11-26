#pragma once

#include "../Common/Util.h"

class IBonDriver2;

class CBonDriverUtil
{
public:
	CBonDriverUtil(void);
	~CBonDriverUtil(void);

	//BonDriverフォルダを指定
	//引数：
	// bonDriverFolderPath		[IN]BonDriverフォルダパス
	void SetBonDriverFolder(
		LPCWSTR bonDriverFolderPath
		);

	//BonDriverフォルダのBonDriver_*.dllを列挙
	//戻り値：
	// 検索できたBonDriver一覧
	vector<wstring> EnumBonDriver();

	//BonDriverをロードしてチャンネル情報などを取得（ファイル名で指定）
	//引数：
	// bonDriverFile	[IN]EnumBonDriverで取得されたBonDriverのファイル名
	// recvFunc_		[IN]ストリーム受信時のコールバック関数
	bool OpenBonDriver(
		LPCWSTR bonDriverFile,
		void (*recvFunc_)(void*, BYTE*, DWORD, DWORD),
		void* recvParam_,
		int openWait = 200
		);

	//ロードしているBonDriverの開放
	void CloseBonDriver();

	//ロードしたBonDriverの情報取得
	//SpaceとChの一覧を取得する
	//戻り値：
	// SpaceとChの一覧（リストの添え字がそのままチューナー空間やチャンネルの番号になる）
	// ※原作はチャンネル名が空のものをスキップする仕様なので、利用側はこれに従ったほうが良いかもしれない
	vector<pair<wstring, vector<wstring>>> GetOriginalChList();

	//BonDriverのチューナー名を取得
	//戻り値：
	// チューナー名
	wstring GetTunerName();

	//チャンネル変更
	//引数：
	// space			[IN]変更チャンネルのSpace
	// ch				[IN]変更チャンネルの物理Ch
	bool SetCh(
		DWORD space,
		DWORD ch
		);

	//現在のチャンネル取得
	//引数：
	// space			[IN]現在のチャンネルのSpace
	// ch				[IN]現在のチャンネルの物理Ch
	bool GetNowCh(
		DWORD* space,
		DWORD* ch
		);

	//シグナルレベルの取得
	//戻り値：
	// シグナルレベル
	float GetSignalLevel();

	//OpenしたBonDriverのファイル名を取得
	//戻り値：
	// BonDriverのファイル名（拡張子含む）（emptyで未Open）
	wstring GetOpenBonDriverFileName();

private:
	CBonDriverUtil(const CBonDriverUtil&);
	CBonDriverUtil& operator=(const CBonDriverUtil&);
	//BonDriverにアクセスするワーカースレッド
	static UINT WINAPI DriverThread(LPVOID param);
	//ワーカースレッドのメッセージ専用ウィンドウプロシージャ
	static LRESULT CALLBACK DriverWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	static class CInit { public: CInit(); } s_init;
	CRITICAL_SECTION utilLock;
	wstring loadDllFolder;
	wstring loadDllFileName;
	wstring loadTunerName;
	vector<pair<wstring, vector<wstring>>> loadChList;
	bool initChSetFlag;
	void (*recvFunc)(void*, BYTE*, DWORD, DWORD);
	void* recvParam;
	IBonDriver2* bon2IF;
	HANDLE hDriverThread;
	HWND hwndDriver;
};

