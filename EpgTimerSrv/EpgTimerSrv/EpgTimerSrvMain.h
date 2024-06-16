#pragma once

#include "EpgDBManager.h"
#include "ReserveManager.h"
#include "NotifyManager.h"
#include "HttpServer.h"
#include "../../Common/CtrlCmdUtil.h"
#include "../../Common/MessageManager.h"
#include "../../Common/ParseTextInstances.h"
#include "../../Common/TimeShiftUtil.h"
#include "../../Common/InstanceManager.h"

#ifdef _WIN32
#define LUA_DLL_NAME L"lua52.dll"
#endif

//各種サーバと自動予約の管理をおこなう
//必ずオブジェクト生成→Main()→…→破棄の順番で利用しなければならない
class CEpgTimerSrvMain
{
public:
	CEpgTimerSrvMain();
	//メインループ処理
	//serviceFlag_: サービスとしての起動かどうか
	bool Main(bool serviceFlag_);
	//メイン処理停止
	void StopMain();
	//休止／スタンバイに移行して構わない状況かどうか
	bool IsSuspendOK() const;
private:
	static bool OnMessage(CMessageManager::PARAMS& pa);
#ifdef _WIN32
	//メインウィンドウ
	static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	//シャットダウン問い合わせダイアログ
	static INT_PTR CALLBACK QueryShutdownDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	//アイコンを読み込む
	static HICON LoadSmallIcon(int iconID);
	//GUI(EpgTimer)を起動する
	static void OpenGUI();
	//「予約削除」ポップアップを作成する
	static void InitReserveMenuPopup(HMENU hMenu, vector<RESERVE_DATA>& list);
	//「配信停止」ポップアップを作成する
	void InitStreamingMenuPopup(HMENU hMenu) const;
#endif
	void ReloadNetworkSetting();
	void ReloadSetting(bool initialize = false);
	//デフォルト指定可能なフィールドのデフォルト値を特別な予約情報(ID=0x7FFFFFFF)として取得する
	RESERVE_DATA GetDefaultReserveData(LONGLONG startTime) const;
	//REC_SETTING_DATA::recModeの値域を調整する
	void AdjustRecModeRange(REC_SETTING_DATA& recSetting) const;
#ifdef _WIN32
	//現在の予約状態に応じた復帰タイマをセットする
	bool SetResumeTimer(HANDLE* resumeTimer, LONGLONG* resumeTime, DWORD marginSec);
	//システムをシャットダウンする
	static void SetShutdown(BYTE shutdownMode);
	//GUIにシャットダウン可能かどうかの問い合わせを開始させる
	//suspendMode==0:再起動(常にrebootFlag==1とする)
	//suspendMode!=0:スタンバイ休止または電源断
	bool QueryShutdown(BYTE rebootFlag, BYTE suspendMode);
	//ユーザーがPCを使用中かどうか
	bool IsUserWorking() const;
	//共有フォルダのTSファイルにアクセスがあるかどうか
	bool IsFindShareTSFile() const;
	//抑制条件のプロセスが起動しているかどうか
	bool IsFindNoSuspendExe() const;
#endif
	//変更直前の予約を調整する
	vector<RESERVE_DATA>& PreChgReserveData(vector<RESERVE_DATA>& reserveList) const;
	void AutoAddReserveEPG(const EPG_AUTO_ADD_DATA& data, vector<RESERVE_DATA>& setList);
	void AutoAddReserveProgram(const MANUAL_AUTO_ADD_DATA& data, vector<RESERVE_DATA>& setList) const;
	//外部制御コマンド関係
	static void CtrlCmdCallback(CEpgTimerSrvMain* sys, const CCmdStream& cmd, CCmdStream& res, int threadIndex, bool tcpFlag, LPCWSTR clientIP);
	bool CtrlCmdProcessCompatible(const CCmdStream& cmd, CCmdStream& res, LPCWSTR clientIP);
	void InitLuaCallback(lua_State* L, LPCSTR serverRandom);
	void DoLuaBat(CBatManager::BAT_WORK_INFO& work, vector<char>& buff);
#ifdef _WIN32
	static void DoLuaWorker(CEpgTimerSrvMain* sys);
#else
	static void ProcessLuaPost(CEpgTimerSrvMain* sys);
#endif
	//Lua-edcb空間のコールバック
	class CLuaWorkspace
	{
	public:
		CLuaWorkspace(lua_State* L_);
		const char* WtoUTF8(const wstring& strIn);
		lua_State* const L;
		CEpgTimerSrvMain* const sys;
		int htmlEscape;
	private:
		vector<char> strOut;
	};
	static int LuaCreateRandom(lua_State* L);
	static int LuaGetGenreName(lua_State* L);
	static int LuaGetComponentTypeName(lua_State* L);
	static int LuaSleep(lua_State* L);
	static int LuaConvert(lua_State* L);
	static void RedirectRelativeIniPath(wstring& path);
	static int LuaGetPrivateProfile(lua_State* L);
	static int LuaWritePrivateProfile(lua_State* L);
	static int LuaReloadEpg(lua_State* L);
	static int LuaReloadSetting(lua_State* L);
	static int LuaEpgCapNow(lua_State* L);
	static int LuaGetChDataList(lua_State* L);
	static int LuaGetServiceList(lua_State* L);
	static int LuaGetEventMinMaxTime(lua_State* L);
	static int LuaGetEventMinMaxTimeArchive(lua_State* L);
	static int LuaGetEventMinMaxTimeProc(lua_State* L, bool archive);
	static int LuaEnumEventInfo(lua_State* L);
	static int LuaEnumEventInfoArchive(lua_State* L);
	static int LuaEnumEventInfoProc(lua_State* L, bool archive);
	static int LuaSearchEpg(lua_State* L);
	static int LuaSearchEpgArchive(lua_State* L);
	static int LuaSearchEpgProc(lua_State* L, bool archive);
	static int LuaAddReserveData(lua_State* L);
	static int LuaChgReserveData(lua_State* L);
	static int LuaDelReserveData(lua_State* L);
	static int LuaGetReserveData(lua_State* L);
	static int LuaGetRecFilePath(lua_State* L);
	static int LuaGetRecFileInfo(lua_State* L);
	static int LuaGetRecFileInfoBasic(lua_State* L);
	static int LuaGetRecFileInfoProc(lua_State* L, bool getExtraInfo);
	static int LuaChgPathRecFileInfo(lua_State* L);
	static int LuaChgProtectRecFileInfo(lua_State* L);
	static int LuaDelRecFileInfo(lua_State* L);
	static int LuaGetTunerReserveAll(lua_State* L);
	static int LuaGetTunerProcessStatusAll(lua_State* L);
	static int LuaEnumAutoAdd(lua_State* L);
	static int LuaEnumManuAdd(lua_State* L);
	static int LuaDelAutoAdd(lua_State* L);
	static int LuaDelManuAdd(lua_State* L);
	static int LuaAddOrChgAutoAdd(lua_State* L);
	static int LuaAddOrChgManuAdd(lua_State* L);
	static int LuaGetNotifyUpdateCount(lua_State* L);
	static int LuaFindFile(lua_State* L);
	static int LuaOpenNetworkTV(lua_State* L);
	static int LuaIsOpenNetworkTV(lua_State* L);
	static int LuaCloseNetworkTV(lua_State* L);
	static void PushEpgEventInfo(CLuaWorkspace& ws, const EPGDB_EVENT_INFO& e);
	static void PushReserveData(CLuaWorkspace& ws, const RESERVE_DATA& r);
	static void PushRecSettingData(CLuaWorkspace& ws, const REC_SETTING_DATA& rs);
	static void PushEpgSearchKeyInfo(CLuaWorkspace& ws, const EPGDB_SEARCH_KEY_INFO& k);
	static bool FetchReserveData(CLuaWorkspace& ws, RESERVE_DATA& r);
	static void FetchRecSettingData(CLuaWorkspace& ws, REC_SETTING_DATA& rs);
	static void FetchEpgSearchKeyInfo(CLuaWorkspace& ws, EPGDB_SEARCH_KEY_INFO& k);

	CNotifyManager notifyManager;
	CEpgDBManager epgDB;
	//reserveManagerはnotifyManagerとepgDBに依存するので、順序を入れ替えてはいけない
	CReserveManager reserveManager;
	CInstanceManager<CTimeShiftUtil> streamingManager;

	CParseEpgAutoAddText epgAutoAdd;
	CParseManualAutoAddText manualAutoAdd;
	map<DWORD, EPG_AUTO_ADD_DATA>::const_iterator autoAddCheckItr;

	//autoAddLock->settingLockの順にロックする
	mutable recursive_mutex_ autoAddLock;
	mutable recursive_mutex_ settingLock;
	CMessageManager msgManager;
#ifdef _WIN32
	std::unique_ptr<void, void(*)(void*)> luaDllHolder;
#endif
	atomic_bool_ stoppingFlag;

	atomic_bool_ residentFlag;
	CEpgTimerSrvSetting::SETTING setting;
	unsigned short tcpPort;
	bool tcpIPv6;
	DWORD tcpResponseTimeoutSec;
	wstring tcpAccessControlList;
	CHttpServer::SERVER_OPTIONS httpOptions;
	string httpServerRandom;
#ifdef _WIN32
	atomic_bool_ useSyoboi;
#endif
	bool nwtvUdp;
	bool nwtvTcp;
	DWORD compatFlags;

#ifdef _WIN32
	thread_ doLuaWorkerThread;
	recursive_mutex_ doLuaWorkerLock;
	vector<string> doLuaScriptQueue;
#else
	thread_ processLuaPostThread;
	CAutoResetEvent processLuaPostStopEvent;
#endif

	//CPipeServer用に2つとCTCPServer用に1つ
	vector<EPGDB_EVENT_INFO> oldSearchList[3];
};
