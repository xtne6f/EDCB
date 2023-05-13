#pragma once

#include "../../Common/PipeServer.h"
#include "../../Common/StructDef.h"

//タスクトレイに常駐してサーバプロセスの情報を表示したり処理の一部を代理する
class CEpgTimerTask
{
public:
	CEpgTimerTask();
	//メインループ処理
	bool Main();
private:
	//メインウィンドウ
	static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	//シャットダウン問い合わせダイアログ
	static INT_PTR CALLBACK QueryShutdownDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	//アイコンを読み込む
	static HICON LoadSmallIcon(int iconID);
	//GUI(EpgTimer)を起動する
	static void OpenGUI();
	//「予約削除」ポップアップを作成する
	static void InitReserveMenuPopup(HMENU hMenu, vector<RESERVE_DATA>& list);

	const UINT msgTaskbarCreated;
	CPipeServer pipeServer;
	pair<HWND, pair<BYTE, bool>> queryShutdownContext;
	DWORD notifySrvStatus;
};
