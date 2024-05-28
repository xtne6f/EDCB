#pragma once

#include "CtrlCmdUtil.h"
#include "ThreadUtil.h"
#include <functional>

class CPipeServer
{
public:
	CPipeServer(void);
	~CPipeServer(void);

	bool StartServer(
		const wstring& pipeName,
		const std::function<void(CCmdStream&, CCmdStream&)>& cmdProc_,
		bool insecureFlag = false,
		bool doNotCreateNoWaitPipe = false
		);
	bool StopServer(bool checkOnlyFlag = false);

#ifdef _WIN32
	//SERVICE_NAMEのサービスセキュリティ識別子(Service-specific SID)に対するアクセス許可を追加する
	static BOOL GrantServerAccessToKernelObject(HANDLE handle, DWORD permissions);
#else
	//異常終了などで残ったソケットファイル(名前の接尾にプロセスIDを伴うもの)があれば削除する
	static void DeleteRemainingFiles(LPCWSTR pipeName);
#endif

protected:
	std::function<void(CCmdStream&, CCmdStream&)> cmdProc;
	atomic_bool_ exitingFlag;
	CAutoResetEvent stopEvent;
	thread_ workThread;
#ifdef _WIN32
	//要素1は勧告ロックを求めないパイプ用。約束事が異なるだけで実装は完全に対称
	HANDLE hEventOls[2];
	HANDLE hEventConnects[2];
	HANDLE hPipes[2];

	static BOOL GrantAccessToKernelObject(HANDLE handle, WCHAR* trusteeName, bool trusteeIsSid, DWORD permissions);
#else
	int srvSock;
	string sockPath;
#endif
	static void ServerThread(CPipeServer* pSys);

};
