#pragma once

#include "../Common/MessageManager.h"
#include "../Common/ThreadUtil.h"
#include <functional>

class IBonDriver2;

class CBonDriverUtil
{
public:
	CBonDriverUtil(void);
	~CBonDriverUtil(void);

	//BonDriverをロードしてチャンネル情報などを取得（ファイル名で指定）
	//引数：
	// bonDriverFolder	[IN]BonDriverのフォルダパス
	// bonDriverFile	[IN]BonDriverのファイル名
	// recvFunc_		[IN]ストリーム受信時のコールバック関数
	// statusFunc_		[IN]ステータス(シグナルレベル,Space,Ch)取得時のコールバック関数
	// traceLevel_		[IN]デバッグ出力の詳細レベル
	bool OpenBonDriver(
		LPCWSTR bonDriverFolder,
		LPCWSTR bonDriverFile,
		const std::function<void(BYTE*, DWORD, DWORD)>& recvFunc_,
		const std::function<void(float, int, int)>& statusFunc_,
		int traceLevel_
		);

	//ロードしているBonDriverの開放
	void CloseBonDriver();

	//ロードしたBonDriverの情報取得
	//SpaceとChの一覧を取得する
	//戻り値：
	// SpaceとChの一覧（リストの添え字がそのままチューナー空間やチャンネルの番号になる）
	// ※原作はチャンネル名が空のものをスキップする仕様なので、利用側はこれに従ったほうが良いかもしれない
	const vector<pair<wstring, vector<wstring>>>& GetOriginalChList() { return this->loadChList; }

	//BonDriverのチューナー名を取得
	//戻り値：
	// チューナー名
	const wstring& GetTunerName() { return this->loadTunerName; }

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

	//Openしているかどうか
	//※スレッドセーフ
	bool IsOpen();

	//OpenしたBonDriverのファイル名を取得
	//※スレッドセーフ
	//戻り値：
	// BonDriverのファイル名（拡張子含む）（emptyで未Open）
	wstring GetOpenBonDriverFileName();

private:
	//BonDriverにアクセスするワーカースレッド
	static void DriverThread(CBonDriverUtil* sys);

#ifdef _WIN32
	//ワーカースレッドのメッセージ専用ウィンドウプロシージャ
	static LRESULT CALLBACK DriverWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif
	static bool OnMessage(CMessageManager::PARAMS& pa);

	//BonDriverへのアクセスを監視するスレッド
	static void WatchdogThread(CBonDriverUtil* sys);

	static class CInit { public: CInit(); } s_init;
	recursive_mutex_ utilLock;
	wstring loadDllFolder;
	wstring loadDllFileName;
	wstring loadTunerName;
	vector<pair<wstring, vector<wstring>>> loadChList;
	bool initChSetFlag;
	std::function<void(BYTE*, DWORD, DWORD)> recvFunc;
	std::function<void(float, int, int)> statusFunc;
	int statusTimeout;
	IBonDriver2* bon2IF;
	thread_ driverThread;
	thread_ watchdogThread;
	CAutoResetEvent driverOpenEvent;
	CAutoResetEvent watchdogStopEvent;
	int traceLevel;
	LPCWSTR callingName;
	DWORD callingTick;
	int statGetTsCalls;
	LONGLONG statGetTsBytes;
	CMessageManager msgManager;
	bool openFlag;
};

