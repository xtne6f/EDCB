#pragma once

// TVTestのフォルダにSendTSTCP.dllがあれば、EpgDataCap_Bonで送信先に"0.0.0.1:0"を設定したときと似たような動作をする
#define SEND_PIPE_TEST

#define TVTEST_PLUGIN_CLASS_IMPLEMENT
#define TVTEST_PLUGIN_VERSION TVTEST_PLUGIN_VERSION_(0,0,14)
#include "../../Common/TVTestPlugin.h"
#include "../../Common/PipeServer.h"
#include "../../Common/EpgDataCap3Util.h"
#include "../../BonCtrl/DropCount.h"
#include "../../Common/PathUtil.h"
#include "../../Common/ThreadUtil.h"
#include "../../BonCtrl/BonCtrlDef.h"
#include "../../BonCtrl/ServiceFilter.h"
#ifdef SEND_PIPE_TEST
#include "../../Common/SendTSTCPDllUtil.h"
#endif

class CEdcbPlugIn : public TVTest::CTVTestPlugin
{
public:
	CEdcbPlugIn();
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
		// サービスが変更された
		bool OnServiceChange();
		// サービスの構成が変化した
		bool OnServiceUpdate();
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
		WORD filterSID;
		bool filterStarted;
		CServiceFilter filterForDropCount;
		CDropCount dropCount;
	};

	// EPG取得対象のサービス一覧を取得する
	vector<CH_DATA5> GetEpgCheckList(WORD onid, WORD tsid, int sid, bool basicFlag) const;
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT WndProc_(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void CtrlCmdCallback(const CCmdStream &cmd, CCmdStream &res);
	void CtrlCmdCallbackInvoked(const CCmdStream &cmd, CCmdStream &res);
	// EDCBの設定関係保存フォルダのパスを取得する
	fs_path GetEdcbSettingPath() const;
	// 録画停止中かどうか調べる
	bool IsNotRecording() const;
	// EDCBの制御下で録画中かどうか調べる
	bool IsEdcbRecording() const;
	// 現在のBonDriverはチューナかどうか調べる
	bool IsTunerBonDriver() const;
	// EpgTimerSrvにEPG再読み込みを要求するスレッド
	static void ReloadEpgThread();
	// ストリームコールバック(別スレッド)
	static BOOL CALLBACK StreamCallback(BYTE *pData, void *pClientData);
	static BOOL CALLBACK EnumLogoListProc(DWORD logoListSize, const LOGO_INFO *logoList, LPVOID param);

	CMyEventHandler m_handler;
	recursive_mutex_ m_streamLock;
	recursive_mutex_ m_statusLock;
	VIEW_APP_STATUS_INFO m_statusInfo;
	HWND m_hwnd;
	CPipeServer m_pipeServer;
	vector<CH_DATA5> m_chSet5;
	CEpgDataCap3Util m_epgUtil;
	wstring m_epgUtilPath;
	wstring m_edcbDir;
	wstring m_nonTunerDrivers;
	wstring m_recNamePrefix;
	int m_dropSaveThresh;
	int m_scrambleSaveThresh;
	bool m_noLogScramble;
	bool m_dropLogAsUtf8;
	SET_CH_INFO m_lastSetCh;
	bool m_chChangedAfterSetCh;
	DWORD m_chChangeID;
	DWORD m_chChangeTick;
	std::unique_ptr<FILE, fclose_deleter> m_epgFile;
	enum { EPG_FILE_ST_NONE, EPG_FILE_ST_PAT, EPG_FILE_ST_TOT, EPG_FILE_ST_ALL } m_epgFileState;
	LONGLONG m_epgFileTotPos;
	wstring m_epgFilePath;
	thread_ m_epgReloadThread;
	DWORD m_epgCapTimeout;
	bool m_epgCapSaveTimeout;
	vector<SET_CH_INFO> m_epgCapChList;
	bool m_epgCapBasicOnlyONIDs[16];
	bool m_epgCapChkONIDs[16];
	bool m_epgCapChkNext;
	DWORD m_epgCapStartTick;
	bool m_epgCapBack;
	bool m_epgCapBackBasicOnlyONIDs[16];
	DWORD m_epgCapBackStartWaitSec;
	DWORD m_epgCapBackStartTick;
	DWORD m_recCtrlCount;
	map<DWORD, REC_CTRL> m_recCtrlMap;
	vector<BYTE> m_bufForDropCount;
	wstring m_duplicateOriginalPath;
	vector<pair<LONGLONG, DWORD>> m_logoServiceListSizeMap;
	const WORD *m_logoAdditionalNeededPids;
	DWORD m_logoTick;
	DWORD m_logoTypeFlags;
#ifdef SEND_PIPE_TEST
	CSendTSTCPDllUtil m_sendPipe;
	util_unique_handle m_sendPipeMutex;
	vector<BYTE> m_sendPipeBuf;
	CServiceFilter m_serviceFilter;
#endif
};
