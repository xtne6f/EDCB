#pragma once

#include "StructDef.h"

typedef int (CALLBACK *CMD_CALLBACK_PROC)(void* pParam, CMD_STREAM* pCmdParam, CMD_STREAM* pResParam);

class CPipeServer
{
public:
	CPipeServer(void);
	~CPipeServer(void);

	BOOL StartServer(
		LPCWSTR eventName_, 
		LPCWSTR pipeName_, 
		CMD_CALLBACK_PROC cmdCallback, 
		void* callbackParam, 
		BOOL insecureFlag_ = FALSE
		);
	BOOL StopServer(BOOL checkOnlyFlag = FALSE);

protected:
	CMD_CALLBACK_PROC cmdProc;
	void* cmdParam;
	wstring eventName;
	wstring pipeName;

	BOOL insecureFlag;

	HANDLE stopEvent;
	HANDLE workThread;

protected:
	static UINT WINAPI ServerThread(LPVOID pParam);

};
