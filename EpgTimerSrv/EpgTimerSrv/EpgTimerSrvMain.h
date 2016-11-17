#pragma once

#include "EpgDBManager.h"
#include "ReserveManager.h"
#include "FileStreamingManager.h"
#include "NotifyManager.h"
#include "HttpServer.h"
#include "../../Common/ParseTextInstances.h"

//各種サーバと自動予約の管理をおこなう
//必ずオブジェクト生成→Main()→…→破棄の順番で利用しなければならない
class CEpgTimerSrvMain
{
public:
	CEpgTimerSrvMain();
	~CEpgTimerSrvMain();
	//メインループ処理
	//serviceFlag_: サービスとしての起動かどうか
	bool Main(bool serviceFlag_);
	//メイン処理停止
	void StopMain();
	//休止／スタンバイに移行して構わない状況かどうか
	bool IsSuspendOK(); //const;
private:
	//メインウィンドウ
	static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	//シャットダウン問い合わせダイアログ
	static INT_PTR CALLBACK QueryShutdownDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void ReloadNetworkSetting();
	void ReloadSetting();
	//プリセット録画設定を読み込む(旧CRestApiManagerから移動)
	pair<wstring, REC_SETTING_DATA> LoadRecSetData(WORD preset) const;
	//現在の予約状態に応じた復帰タイマをセットする
	bool SetResumeTimer(HANDLE* resumeTimer, __int64* resumeTime, DWORD marginSec);
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
	bool AutoAddReserveEPG(const EPG_AUTO_ADD_DATA& data, const bool noReportNotify = false);
	bool AutoAddReserveProgram(const MANUAL_AUTO_ADD_DATA& data, const bool noReportNotify = false);
	//外部制御コマンド関係
	static int CALLBACK CtrlCmdPipeCallback(void* param, CMD_STREAM* cmdParam, CMD_STREAM* resParam);
	static int CALLBACK CtrlCmdTcpCallback(void* param, CMD_STREAM* cmdParam, CMD_STREAM* resParam);
	static int CtrlCmdCallback(void* param, CMD_STREAM* cmdParam, CMD_STREAM* resParam, bool tcpFlag);
	bool CtrlCmdProcessCompatible(CMD_STREAM& cmdParam, CMD_STREAM& resParam);
	static int InitLuaCallback(lua_State* L);
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
	static int LuaGetGenreName(lua_State* L);
	static int LuaGetComponentTypeName(lua_State* L);
	static int LuaSleep(lua_State* L);
	static int LuaConvert(lua_State* L);
	static int LuaGetPrivateProfile(lua_State* L);
	static int LuaWritePrivateProfile(lua_State* L);
	static int LuaReloadEpg(lua_State* L);
	static int LuaReloadSetting(lua_State* L);
	static int LuaEpgCapNow(lua_State* L);
	static int LuaGetChDataList(lua_State* L);
	static int LuaGetServiceList(lua_State* L);
	static int LuaGetEventMinMaxTime(lua_State* L);
	static int LuaEnumEventInfo(lua_State* L);
	static int LuaSearchEpg(lua_State* L);
	static int LuaAddReserveData(lua_State* L);
	static int LuaChgReserveData(lua_State* L);
	static int LuaDelReserveData(lua_State* L);
	static int LuaGetReserveData(lua_State* L);
	static int LuaGetRecFilePath(lua_State* L);
	static int LuaGetRecFileInfo(lua_State* L);
	static int LuaGetRecFileInfoBasic(lua_State* L);
	static int LuaGetRecFileInfoProc(lua_State* L, bool getExtraInfo);
	static int LuaChgProtectRecFileInfo(lua_State* L);
	static int LuaDelRecFileInfo(lua_State* L);
	static int LuaGetTunerReserveAll(lua_State* L);
	static int LuaEnumRecPresetInfo(lua_State* L);
	static int LuaEnumAutoAdd(lua_State* L);
	static int LuaEnumManuAdd(lua_State* L);
	static int LuaDelAutoAdd(lua_State* L);
	static int LuaDelManuAdd(lua_State* L);
	static int LuaAddOrChgAutoAdd(lua_State* L);
	static int LuaAddOrChgManuAdd(lua_State* L);
	static int LuaGetNotifyUpdateCount(lua_State* L);
	static int LuaListDmsPublicFile(lua_State* L);
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
	CFileStreamingManager streamingManager;

	CParseEpgAutoAddText epgAutoAdd;
	CParseManualAutoAddText manualAutoAdd;

	mutable CRITICAL_SECTION settingLock;
	HWND hwndMain;

	bool residentFlag;
	bool saveNotifyLog;
	DWORD wakeMarginSec;
	unsigned short tcpPort;
	DWORD tcpResponseTimeoutSec;
	wstring tcpAccessControlList;
	CHttpServer::SERVER_OPTIONS httpOptions;
	string httpServerRandom;
	bool enableSsdpServer;
	vector<pair<int, wstring>> dmsPublicFileList;
	int autoAddHour;
	bool chkGroupEvent;
	bool useSyoboi;
	//LOBYTEにモード(1=スタンバイ,2=休止,3=電源断,4=なにもしない)、HIBYTEに再起動フラグ
	WORD defShutdownMode;
	DWORD ngUsePCTime;
	bool ngFileStreaming;
	bool ngShareFile;
	DWORD noStandbySec;
	vector<wstring> noSuspendExeList;
	vector<wstring> tvtestUseBon;
	bool nwtvUdp;
	bool nwtvTcp;
	DWORD notifyUpdateCount[6];

	vector<EPGDB_EVENT_INFO> oldSearchList[2];
};
