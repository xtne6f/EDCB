
// EpgDataCap_BonDlg.h : ヘッダー ファイル
//

#pragma once

#include "../../BonCtrl/BonCtrl.h"
#include "../../Common/PipeServer.h"
#include "AppSetting.h"
#include "EpgDataCap_BonDef.h"
#include "SettingDlg.h"

// CEpgDataCap_BonDlg ダイアログ
class CEpgDataCap_BonDlg
{
// コンストラクション
public:
	CEpgDataCap_BonDlg();	// 標準コンストラクター
	~CEpgDataCap_BonDlg();
	INT_PTR DoModal();
	void ParseCommandLine(LPWSTR* argv, int argc);

// ダイアログ データ
	enum { IDD = IDD_EPGDATACAP_BON_DIALOG };

protected:
	static UINT taskbarCreated;
	static BOOL disableKeyboardHook;
protected:
	static HICON LoadLargeOrSmallIcon(int iconID, bool isLarge);
	//現在値と異なるときだけSetDlgItemText()を呼ぶ
	static void CheckAndSetDlgItemText(HWND wnd, int id, LPCWSTR text);
	void ReloadSetting();
	void BtnUpdate(DWORD guiMode);
	//タスクトレイ
	BOOL DeleteTaskBar(HWND wnd, UINT id);
	BOOL AddTaskBar(HWND wnd, UINT msg, UINT id, HICON icon, wstring tips);
	BOOL ChgTipsTaskBar(HWND wnd, UINT id, HICON icon, wstring tips);
	void ChgIconStatus();

	void SetOverlayIcon(HICON icon);
	void UpdateTitleBarText();
	int ReloadServiceList(int selONID = -1, int selTSID = -1, int selSID = -1);
	void ReloadNWSet();
	BOOL SelectBonDriver(LPCWSTR fileName);
	BOOL SelectService(const CH_DATA4& chData);

	void StartPipeServer();
	void CtrlCmdCallbackInvoked();
// 実装
protected:
	HWND m_hWnd;
	HHOOK m_hKeyboardHook;
	HICON m_hIcon;
	HICON m_hIcon2;
	HANDLE m_hViewProcess;

	APP_SETTING setting;
	vector<wstring> recFolderList;

	wstring iniBonDriver;
	BOOL iniMin;
	BOOL iniView;
	BOOL iniNetwork;
	BOOL iniUDP;
	BOOL iniTCP;
	int iniONID;
	int iniTSID;
	int iniSID;

	CBonCtrl bonCtrl;
	CPipeServer pipeServer;
	vector<DWORD> cmdCtrlList;
	const CCmdStream* cmdCapture;
	CCmdStream* resCapture;

	recursive_mutex_ statusInfoLock;
	VIEW_APP_STATUS_INFO statusInfo;

	//サービス一覧の表示と同期する。ただしこのリストには非表示サービスも含む
	vector<CH_DATA4> serviceList;
	DWORD recCtrlID;
	vector<NW_SEND_INFO> udpSendList;
	vector<NW_SEND_INFO> tcpSendList;
	BOOL chScanWorking;
	BOOL epgCapWorking;

	// 生成された、メッセージ割り当て関数
	BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam, BOOL* pbProcessed);
	static BOOL CALLBACK CloseViewWindowsProc(HWND hwnd, LPARAM lParam);
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
