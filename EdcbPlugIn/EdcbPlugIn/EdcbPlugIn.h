#pragma once

#ifndef interface
#define interface struct
#endif
#define TVTEST_PLUGIN_CLASS_IMPLEMENT
#define TVTEST_PLUGIN_VERSION TVTEST_PLUGIN_VERSION_(0,0,14)
#include "../../Common/TVTestPlugin.h"
#include "../../Common/PipeServer.h"
#include "../../Common/EpgDataCap3Util.h"
#include "../../BonCtrl/DropCount.h"

class CEdcbPlugIn : public TVTest::CTVTestPlugin
{
public:
	CEdcbPlugIn();
	~CEdcbPlugIn();
	// プラグインの情報を返す
	bool GetPluginInfo(TVTest::PluginInfo *pInfo);
	// 初期化処理
	bool Initialize();
	// 終了処理
	bool Finalize();
private:
	class CMyEventHandler : public TVTest::CTVTestEventHandler
	{
	public:
		CMyEventHandler(CEdcbPlugIn &outer) : m_outer(outer) {}
		static LRESULT CALLBACK EventCallback(UINT ev, LPARAM lp1, LPARAM lp2, void *pc) { return static_cast<CMyEventHandler*>(pc)->HandleEvent(ev, lp1, lp2, pc); }
		// チャンネルが変更された
		bool OnChannelChange();
		// ドライバが変更された
		bool OnDriverChange();
		// 録画状態が変化した
		bool OnRecordStatusChange(int Status);
		// 起動処理が終了した
		void OnStartupDone();
	private:
		CEdcbPlugIn &m_outer;
	};
	struct REC_CTRL
	{
		wstring filePath;
		WORD sid;
		DWORD duplicateTargetID;
		CDropCount dropCount;
	};

	// EPG取得対象のサービス一覧を取得する
	vector<CH_DATA5> GetEpgCheckList(WORD onid, WORD tsid, int sid, bool basicFlag) const;
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT WndProc_(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static int CALLBACK CtrlCmdCallback(void *param, CMD_STREAM *cmdParam, CMD_STREAM *resParam);
	void CtrlCmdCallbackInvoked(CMD_STREAM *cmdParam, CMD_STREAM *resParam);
	// EDCBの設定関係保存フォルダのパスを取得する
	wstring GetEdcbSettingPath() const;
	// 録画停止中かどうか調べる
	bool IsNotRecording() const;
	// EDCBの制御下で録画中かどうか調べる
	bool IsEdcbRecording() const;
	// 現在のBonDriverはチューナかどうか調べる
	bool IsTunerBonDriver() const;
	// EpgTimerSrvにEPG再読み込みを要求するスレッド
	static UINT WINAPI ReloadEpgThread(void *param);
	// ストリームコールバック(別スレッド)
	static BOOL CALLBACK StreamCallback(BYTE *pData, void *pClientData);

	CMyEventHandler m_handler;
	CRITICAL_SECTION m_streamLock;
	CRITICAL_SECTION m_statusLock;
	HWND m_hwnd;
	CPipeServer m_pipeServer;
	vector<CH_DATA5> m_chSet5;
	CEpgDataCap3Util m_epgUtil;
	wstring m_epgUtilPath;
	int m_outCtrlID;
	wstring m_edcbDir;
	wstring m_nonTunerDrivers;
	wstring m_currentBonDriver;
	wstring m_recNamePrefix;
	bool m_noLogScramble;
	DWORD m_statusCode;
	SET_CH_INFO m_lastSetCh;
	bool m_chChangedAfterSetCh;
	DWORD m_chChangeID;
	DWORD m_chChangeTick;
	HANDLE m_epgFile;
	enum { EPG_FILE_ST_NONE, EPG_FILE_ST_PAT, EPG_FILE_ST_TOT, EPG_FILE_ST_ALL } m_epgFileState;
	DWORD m_epgFileTotPos;
	wstring m_epgFilePath;
	HANDLE m_epgReloadThread;
	DWORD m_epgCapTimeout;
	bool m_epgCapSaveTimeout;
	vector<SET_CH_INFO> m_epgCapChList;
	bool m_epgCapBSBasic;
	bool m_epgCapCS1Basic;
	bool m_epgCapCS2Basic;
	bool m_epgCapChkBS;
	bool m_epgCapChkCS1;
	bool m_epgCapChkCS2;
	bool m_epgCapChkNext;
	DWORD m_epgCapStartTick;
	bool m_epgCapBack;
	bool m_epgCapBackBSBasic;
	bool m_epgCapBackCS1Basic;
	bool m_epgCapBackCS2Basic;
	DWORD m_epgCapBackStartWaitSec;
	DWORD m_epgCapBackStartTick;
	DWORD m_recCtrlCount;
	map<DWORD, REC_CTRL> m_recCtrlMap;
	wstring m_duplicateOriginalPath;
};
