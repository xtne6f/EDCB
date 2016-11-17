#pragma once

#include "EpgDBManager.h"
#include "ReserveManager.h"
#include "FileStreamingManager.h"
#include "NotifyManager.h"
#include "HttpServer.h"
#include "../../Common/ParseTextInstances.h"

//�e��T�[�o�Ǝ����\��̊Ǘ��������Ȃ�
//�K���I�u�W�F�N�g������Main()���c���j���̏��Ԃŗ��p���Ȃ���΂Ȃ�Ȃ�
class CEpgTimerSrvMain
{
public:
	CEpgTimerSrvMain();
	~CEpgTimerSrvMain();
	//���C�����[�v����
	//serviceFlag_: �T�[�r�X�Ƃ��Ă̋N�����ǂ���
	bool Main(bool serviceFlag_);
	//���C��������~
	void StopMain();
	//�x�~�^�X�^���o�C�Ɉڍs���č\��Ȃ��󋵂��ǂ���
	bool IsSuspendOK(); //const;
private:
	//���C���E�B���h�E
	static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	//�V���b�g�_�E���₢���킹�_�C�A���O
	static INT_PTR CALLBACK QueryShutdownDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void ReloadNetworkSetting();
	void ReloadSetting();
	//�v���Z�b�g�^��ݒ��ǂݍ���(��CRestApiManager����ړ�)
	pair<wstring, REC_SETTING_DATA> LoadRecSetData(WORD preset) const;
	//���݂̗\���Ԃɉ��������A�^�C�}���Z�b�g����
	bool SetResumeTimer(HANDLE* resumeTimer, __int64* resumeTime, DWORD marginSec);
	//�V�X�e�����V���b�g�_�E������
	static void SetShutdown(BYTE shutdownMode);
	//GUI�ɃV���b�g�_�E���\���ǂ����̖₢���킹���J�n������
	//suspendMode==0:�ċN��(���rebootFlag==1�Ƃ���)
	//suspendMode!=0:�X�^���o�C�x�~�܂��͓d���f
	bool QueryShutdown(BYTE rebootFlag, BYTE suspendMode);
	//���[�U�[��PC���g�p�����ǂ���
	bool IsUserWorking() const;
	//���L�t�H���_��TS�t�@�C���ɃA�N�Z�X�����邩�ǂ���
	bool IsFindShareTSFile() const;
	//�}�������̃v���Z�X���N�����Ă��邩�ǂ���
	bool IsFindNoSuspendExe() const;
	bool AutoAddReserveEPG(const EPG_AUTO_ADD_DATA& data, const bool noReportNotify = false);
	bool AutoAddReserveProgram(const MANUAL_AUTO_ADD_DATA& data, const bool noReportNotify = false);
	//�O������R�}���h�֌W
	static int CALLBACK CtrlCmdPipeCallback(void* param, CMD_STREAM* cmdParam, CMD_STREAM* resParam);
	static int CALLBACK CtrlCmdTcpCallback(void* param, CMD_STREAM* cmdParam, CMD_STREAM* resParam);
	static int CtrlCmdCallback(void* param, CMD_STREAM* cmdParam, CMD_STREAM* resParam, bool tcpFlag);
	bool CtrlCmdProcessCompatible(CMD_STREAM& cmdParam, CMD_STREAM& resParam);
	static int InitLuaCallback(lua_State* L);
	//Lua-edcb��Ԃ̃R�[���o�b�N
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
	//reserveManager��notifyManager��epgDB�Ɉˑ�����̂ŁA���������ւ��Ă͂����Ȃ�
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
	//LOBYTE�Ƀ��[�h(1=�X�^���o�C,2=�x�~,3=�d���f,4=�Ȃɂ����Ȃ�)�AHIBYTE�ɍċN���t���O
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
