
// EpgDataCap_BonDlg.h : ヘッダー ファイル
//

#pragma once

#include "EpgDataCap_BonDef.h"
#include "EpgDataCap_BonMain.h"
#include "SettingDlg.h"

// CEpgDataCap_BonDlg ダイアログ
class CEpgDataCap_BonDlg
{
// コンストラクション
public:
	CEpgDataCap_BonDlg();	// 標準コンストラクター
	INT_PTR DoModal();
	HWND GetSafeHwnd() const{ return m_hWnd; }


	void UnInit();
	void SetInitBon(LPCWSTR bonFile);
	void SetIniMin(BOOL minFlag){ iniMin = minFlag; };
	void SetIniNW(BOOL networkFlag){ iniNetwork = networkFlag; };
	void SetIniView(BOOL viewFlag){ iniView = viewFlag; };
	void SetIniNWUDP(BOOL udpFlag){ iniUDP = udpFlag; };
	void SetIniNWTCP(BOOL tcpFlag){ iniTCP = tcpFlag; };

// ダイアログ データ
	enum { IDD = IDD_EPGDATACAP_BON_DIALOG };

protected:
	static UINT taskbarCreated;
	static BOOL disableKeyboardHook;
protected:
	void BtnUpdate(DWORD guiMode);
	//タスクトレイ
	BOOL DeleteTaskBar(HWND wnd, UINT id);
	BOOL AddTaskBar(HWND wnd, UINT msg, UINT id, HICON icon, wstring tips);
	BOOL ChgTipsTaskBar(HWND wnd, UINT id, HICON icon, wstring tips);
	void ChgIconStatus();

	void ReloadBonDriver();
	void ReloadServiceList(BOOL ini = FALSE);
	void ReloadNWSet();
	DWORD SelectBonDriver(LPCWSTR fileName, BOOL ini = FALSE);
	DWORD SelectService(WORD ONID, WORD TSID, WORD SID);
	DWORD SelectService(WORD ONID, WORD TSID, WORD SID,	DWORD space, DWORD ch);
// 実装
protected:
	HWND m_hWnd;
	HHOOK m_hKeyboardHook;
	HICON m_hIcon;
	HICON m_hIcon2;

	HICON iconRed;
	HICON iconBlue;
	HICON iconGreen;
	HICON iconGray;
	BOOL minTask;

	wstring moduleIniPath;
	wstring commonIniPath;
	wstring timerSrvIniPath;

	wstring iniBonDriver;
	int initONID;
	int initTSID;
	int initSID;
	int initOpenWait;
	int initChgWait;
	BOOL iniMin;
	BOOL iniView;
	BOOL iniNetwork;
	BOOL iniUDP;
	BOOL iniTCP;
	int openLastCh;

	CEpgDataCap_BonMain main;

	vector<wstring> bonList;
	vector<CH_DATA4> serviceList;

	// 生成された、メッセージ割り当て関数
	BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam, BOOL* pbProcessed);
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnTaskbarCreated(WPARAM, LPARAM);
	afx_msg void OnCbnSelchangeComboTuner();
	afx_msg void OnCbnSelchangeComboService();
	afx_msg void OnBnClickedButtonSet();
	afx_msg void OnBnClickedButtonChscan();
	afx_msg void OnBnClickedButtonEpg();
	afx_msg void OnBnClickedButtonRec();
	afx_msg void OnBnClickedButtonCancel();
	afx_msg void OnBnClickedButtonView();
	afx_msg void OnBnClickedCheckUdp();
	afx_msg void OnBnClickedCheckTcp();
	afx_msg void OnBnClickedCheckRecSet();
	afx_msg void OnBnClickedCheckNextpg();
	afx_msg BOOL OnQueryEndSession();
	afx_msg void OnEndSession(BOOL bEnding);
	static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	HWND GetDlgItem(int nID) const{ return ::GetDlgItem(m_hWnd, nID); }
	UINT_PTR SetTimer(UINT_PTR nIDEvent, UINT uElapse, TIMERPROC lpTimerFunc){ return ::SetTimer(m_hWnd, nIDEvent, uElapse, lpTimerFunc); }
	BOOL KillTimer(UINT_PTR uIDEvent){ return ::KillTimer(m_hWnd, uIDEvent); }
};
