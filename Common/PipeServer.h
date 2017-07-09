#pragma once

#include "StructDef.h"
#include <functional>

class CPipeServer
{
public:
	CPipeServer(void);
	~CPipeServer(void);

	BOOL StartServer(
		LPCWSTR eventName_, 
		LPCWSTR pipeName_, 
		const std::function<void(CMD_STREAM*, CMD_STREAM*)>& cmdProc_,
		BOOL insecureFlag_ = FALSE
		);
	BOOL StopServer(BOOL checkOnlyFlag = FALSE);

protected:
	std::function<void(CMD_STREAM*, CMD_STREAM*)> cmdProc;
	wstring eventName;
	wstring pipeName;

	BOOL insecureFlag;

	HANDLE stopEvent;
	HANDLE workThread;

protected:
	static UINT WINAPI ServerThread(LPVOID pParam);

};
