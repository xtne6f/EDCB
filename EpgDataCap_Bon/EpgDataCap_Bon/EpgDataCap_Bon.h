
// EpgDataCap_Bon.h : PROJECT_NAME アプリケーションのメイン ヘッダー ファイルです。
//

#pragma once

#ifdef _WIN32

#include "resource.h"		// メイン シンボル
#include <windowsx.h>
#include <commctrl.h>


// CEpgDataCap_BonApp:
// このクラスの実装については、EpgDataCap_Bon.cpp を参照してください。
//

class CEpgDataCap_BonApp
{
public:
	CEpgDataCap_BonApp();

public:
	BOOL InitInstance();
};

#endif
