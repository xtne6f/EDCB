
// EpgTimerTask.h : PROJECT_NAME アプリケーションのメイン ヘッダー ファイルです。
//

#pragma once

#include "resource.h"		// メイン シンボル


// CEpgTimerTaskApp:
// このクラスの実装については、EpgTimerTask.cpp を参照してください。
//

class CEpgTimerTaskApp
{
public:
	CEpgTimerTaskApp();

public:
	BOOL InitInstance();
};

extern CEpgTimerTaskApp theApp;