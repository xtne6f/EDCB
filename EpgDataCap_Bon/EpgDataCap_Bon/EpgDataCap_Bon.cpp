
// EpgDataCap_Bon.cpp : アプリケーションのクラス動作を定義します。
//

#include "stdafx.h"
#include "EpgDataCap_Bon.h"
#include "EpgDataCap_BonDlg.h"

#include "CmdLineUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CEpgDataCap_BonApp

// CEpgDataCap_BonApp コンストラクション

CEpgDataCap_BonApp::CEpgDataCap_BonApp()
{
	// TODO: この位置に構築用コードを追加してください。
	// ここに InitInstance 中の重要な初期化処理をすべて記述してください。
}


// 唯一の CEpgDataCap_BonApp オブジェクトです。

CEpgDataCap_BonApp theApp;


// CEpgDataCap_BonApp 初期化

BOOL CEpgDataCap_BonApp::InitInstance()
{
	// アプリケーション マニフェストが visual スタイルを有効にするために、
	// ComCtl32.dll Version 6 以降の使用を指定する場合は、
	// Windows XP に InitCommonControlsEx() が必要です。さもなければ、ウィンドウ作成はすべて失敗します。
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// アプリケーションで使用するすべてのコモン コントロール クラスを含めるには、
	// これを設定します。
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	SetProcessShutdownParameters(0x300, 0);

	// コマンドオプションを解析
	CCmdLineUtil cCmdUtil;
	int argc;
	LPWSTR *argv = CommandLineToArgvW(GetCommandLine(), &argc);
	if (argv != NULL) {
		for (int i = 1; i < argc; i++) {
			BOOL bFlag = argv[i][0] == L'-' || argv[i][0] == L'/' ? TRUE : FALSE;
			cCmdUtil.ParseParam(&argv[i][bFlag ? 1 : 0], bFlag, i == argc - 1 ? TRUE : FALSE);
		}
		LocalFree(argv);
	}

	CEpgDataCap_BonDlg dlg;

	map<wstring, wstring>::iterator itr;
	dlg.SetIniMin(FALSE);
	dlg.SetIniView(TRUE);
	dlg.SetIniNW(TRUE);
	for( itr = cCmdUtil.m_CmdList.begin(); itr != cCmdUtil.m_CmdList.end(); itr++ ){
		if( lstrcmpi(itr->first.c_str(), L"d") == 0 ){
			dlg.SetInitBon(itr->second.c_str());
			OutputDebugString(itr->second.c_str());
		}else if( lstrcmpi(itr->first.c_str(), L"min") == 0 ){
			dlg.SetIniMin(TRUE);
		}else if( lstrcmpi(itr->first.c_str(), L"noview") == 0 ){
			dlg.SetIniView(FALSE);
		}else if( lstrcmpi(itr->first.c_str(), L"nonw") == 0 ){
			dlg.SetIniNW(FALSE);
		}else if( lstrcmpi(itr->first.c_str(), L"nwudp") == 0 ){
			dlg.SetIniNWUDP(TRUE);
		}else if( lstrcmpi(itr->first.c_str(), L"nwtcp") == 0 ){
			dlg.SetIniNWTCP(TRUE);
		}
	}


	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: ダイアログが <OK> で消された時のコードを
		//  記述してください。
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: ダイアログが <キャンセル> で消された時のコードを
		//  記述してください。
	}

	// ダイアログは閉じられました。アプリケーションのメッセージ ポンプを開始しないで
	//  アプリケーションを終了するために FALSE を返してください。
	return FALSE;
}

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	theApp.InitInstance();
	return 0;
}

BOOL WritePrivateProfileInt(LPCTSTR lpAppName, LPCTSTR lpKeyName, int value, LPCTSTR lpFileName)
{
	TCHAR sz[32];
	wsprintf(sz, TEXT("%d"), value);
	return WritePrivateProfileString(lpAppName, lpKeyName, sz, lpFileName);
}
