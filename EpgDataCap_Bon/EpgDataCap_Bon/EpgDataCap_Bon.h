
// EpgDataCap_Bon.h : PROJECT_NAME アプリケーションのメイン ヘッダー ファイルです。
//

#pragma once

#include "resource.h"		// メイン シンボル
#include "../../Common/PathUtil.h"
#include <windowsx.h>
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")


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
