#pragma once
#include "PathUtil.h"
#include "SendTSTCPDllUtil.h"
#include "ThreadUtil.h"

class CTimeShiftUtil
{
public:
	CTimeShiftUtil(void);
	~CTimeShiftUtil(void);

	//UDP/TCP送信を行う
	//戻り値：
	// 成功：udpPortとtcpPortに開始ポート番号（終了or失敗：値は不変）
	//引数：
	// ip		[IN]送信先アドレス
	// udpPort	[OUT]開始UDPポート番号（NULLで送信停止）
	// tcpPort	[OUT]開始TCPポート番号（NULLで送信停止）
	void Send(
		LPCWSTR ip,
		DWORD* udpPort,
		DWORD* tcpPort
		);

	//タイムシフト用ファイルを開く
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// filePath		[IN]タイムシフト用バッファファイルのパス
	// fileMode		[IN]録画済みファイル再生モード
	BOOL OpenTimeShift(
		LPCWSTR filePath_,
		BOOL fileMode_
		);

	//タイムシフト送信を開始する
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	BOOL StartTimeShift();

	//タイムシフト送信を停止する
	void StopTimeShift();

	//現在の送信位置と有効なファイルサイズを取得する
	//引数：
	// filePos		[OUT]ファイル位置
	// fileSize		[OUT]ファイルサイズ
	void GetFilePos(LONGLONG* filePos, LONGLONG* fileSize);

	//送信開始位置を変更する
	//引数：
	// filePos		[IN]ファイル位置
	void SetFilePos(LONGLONG filePos);

protected:
	recursive_mutex_ utilLock;
	recursive_mutex_ ioLock;
	util_unique_handle udpMutex;
	util_unique_handle tcpMutex;
	CSendTSTCPDllUtil sendUdp;
	CSendTSTCPDllUtil sendTcp;
	struct SEND_INFO {
		wstring ip;
		DWORD port;
		wstring key;
	} sendInfo[2];

	wstring filePath;
	WORD PCR_PID;

	BOOL fileMode;
	int seekJitter;
	LONGLONG currentFilePos;

	thread_ readThread;
	atomic_bool_ readStopFlag;
	std::unique_ptr<FILE, fclose_deleter> readFile;
	std::unique_ptr<FILE, fclose_deleter> seekFile;
protected:
	static void ReadThread(CTimeShiftUtil* sys);
	LONGLONG GetAvailableFileSize() const;
};

