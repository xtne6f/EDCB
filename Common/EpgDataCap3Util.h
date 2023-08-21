#pragma once

#include "EpgDataCap3Def.h"

class CEpgDataCap3Util
{
public:
	CEpgDataCap3Util(void);
	~CEpgDataCap3Util(void);

	//DLLの初期化
	//戻り値：
	// エラーコード
	//引数：
	// asyncMode		[IN]TRUE:非同期モード、FALSE:同期モード
	// loadDllFilePath	[IN]ロードするDLLパス
	DWORD Initialize(
		BOOL asyncFlag,
		LPCWSTR loadDllFilePath = NULL
		);

	//DLLの開放
	//戻り値：
	// エラーコード
	DWORD UnInitialize(
		);

	//解析対象のTSパケットを読み込ませる
	//戻り値：
	// エラーコード
	// data		[IN]TSパケット
	// size		[IN]dataのサイズ（188の整数倍であること。size>188のとき、内部メソッドが未対応ならば、メソッド呼び出しは分割される）
	DWORD AddTSPacket(
		BYTE* data,
		DWORD size
		);

	//解析データの現在のストリームＩＤを取得する
	//戻り値：
	// エラーコード
	// originalNetworkID		[OUT]現在のoriginalNetworkID。NULL可
	// transportStreamID		[OUT]現在のtransportStreamID
	DWORD GetTSID(
		WORD* originalNetworkID,
		WORD* transportStreamID
		);

	//自ストリームのサービス一覧を取得する
	//戻り値：
	// エラーコード
	//引数：
	// serviceListSize			[OUT]serviceListの個数
	// serviceList				[OUT]サービス情報のリスト（DLL内で自動的にdeleteする。次に取得を行うまで有効）
	DWORD GetServiceListActual(
		DWORD* serviceListSize,
		SERVICE_INFO** serviceList
		);

	//蓄積されたEPG情報のあるサービス一覧を取得する
	//SERVICE_EXT_INFOの情報はない
	//戻り値：
	// エラーコード
	//引数：
	// serviceListSize			[OUT]serviceListの個数
	// serviceList				[OUT]サービス情報のリスト（DLL内で自動的にdeleteする。次に取得を行うまで有効）
	DWORD GetServiceListEpgDB(
		DWORD* serviceListSize,
		SERVICE_INFO** serviceList
		);

	//指定サービスの全EPG情報を取得する
	//戻り値：
	// エラーコード
	// originalNetworkID		[IN]取得対象のoriginalNetworkID
	// transportStreamID		[IN]取得対象のtransportStreamID
	// serviceID				[IN]取得対象のServiceID
	// epgInfoListSize			[OUT]epgInfoListの個数
	// epgInfoList				[OUT]EPG情報のリスト（DLL内で自動的にdeleteする。次に取得を行うまで有効）
	DWORD GetEpgInfoList(
		WORD originalNetworkID,
		WORD transportStreamID,
		WORD serviceID,
		DWORD* epgInfoListSize,
		EPG_EVENT_INFO** epgInfoList
		);

	//指定サービスの全EPG情報を列挙する
	//引数：
	// enumEpgInfoListProc		[IN]EPG情報のリストを取得するコールバック関数
	// param					[IN]コールバック引数
	DWORD EnumEpgInfoList(
		WORD originalNetworkID,
		WORD transportStreamID,
		WORD serviceID,
		BOOL (CALLBACK *enumEpgInfoListProc)(DWORD epgInfoListSize, EPG_EVENT_INFO* epgInfoList, void* param),
		void* param
		);

	//EPGデータの蓄積状態をリセットする
	void ClearSectionStatus();

	//EPGデータの蓄積状態を取得する
	//戻り値：
	// ステータス
	//引数：
	// l_eitFlag		[IN]L-EITのステータスを取得
	EPG_SECTION_STATUS GetSectionStatus(
		BOOL l_eitFlag
		);

	//指定サービスのEPGデータの蓄積状態を取得する
	//戻り値：
	// ステータス,取得できたかどうか
	//引数：
	// originalNetworkID		[IN]取得対象のOriginalNetworkID
	// transportStreamID		[IN]取得対象のTransportStreamID
	// serviceID				[IN]取得対象のServiceID
	// l_eitFlag				[IN]L-EITのステータスを取得
	pair<EPG_SECTION_STATUS, BOOL> GetSectionStatusService(
		WORD originalNetworkID,
		WORD transportStreamID,
		WORD serviceID,
		BOOL l_eitFlag
		);

	//指定サービスの現在or次のEPG情報を取得する
	//戻り値：
	// エラーコード
	//引数：
	// originalNetworkID		[IN]取得対象のoriginalNetworkID
	// transportStreamID		[IN]取得対象のtransportStreamID
	// serviceID				[IN]取得対象のServiceID
	// nextFlag					[IN]TRUE（次の番組）、FALSE（現在の番組）
	// epgInfo					[OUT]EPG情報（DLL内で自動的にdeleteする。次に取得を行うまで有効）
	DWORD GetEpgInfo(
		WORD originalNetworkID,
		WORD transportStreamID,
		WORD serviceID,
		BOOL nextFlag,
		EPG_EVENT_INFO** epgInfo
		);

	//指定イベントのEPG情報を取得する
	//戻り値：
	// エラーコード
	//引数：
	// originalNetworkID		[IN]取得対象のoriginalNetworkID
	// transportStreamID		[IN]取得対象のtransportStreamID
	// serviceID				[IN]取得対象のServiceID
	// EventID					[IN]取得対象のEventID
	// pfOnlyFlag				[IN]p/fからのみ検索するかどうか
	// epgInfo					[OUT]EPG情報（DLL内で自動的にdeleteする。次に取得を行うまで有効）
	DWORD SearchEpgInfo(
		WORD originalNetworkID,
		WORD transportStreamID,
		WORD serviceID,
		WORD eventID,
		BYTE pfOnlyFlag,
		EPG_EVENT_INFO** epgInfo
		);

	//取得するロゴタイプをフラグで指定する
	//引数：
	// flags					[IN]フラグ(ロゴタイプnを取得するとき下位からnビット目をセットする)
	// additionalNeededPids		[OUT]追加で解析したいTSパケットのPIDのリスト(0終端)。NULL可。次の呼び出しまで有効
	void SetLogoTypeFlags(
		DWORD flags,
		const WORD** additionalNeededPids
		);

	//全ロゴを列挙する
	//戻り値：
	// エラーコード
	//引数：
	// enumLogoListProc			[IN]ロゴのリストを取得するコールバック関数
	// param					[IN]コールバック引数
	DWORD EnumLogoList(
		BOOL (CALLBACK *enumLogoListProc)(DWORD logoListSize, const LOGO_INFO* logoList, void* param),
		void* param
		);

	//PC時計を元としたストリーム時間との差を取得する
	//戻り値：
	// 差の秒数
	int GetTimeDelay(
		);

private:
	typedef DWORD (WINAPI *InitializeEP3)(BOOL asyncFlag, DWORD* id);
	typedef DWORD (WINAPI *UnInitializeEP3)(DWORD id);
	typedef DWORD (WINAPI *AddTSPacketEP3)(DWORD id, BYTE* data, DWORD size);
	typedef DWORD (WINAPI *GetTSIDEP3)(DWORD id, WORD* originalNetworkID, WORD* transportStreamID);
	typedef DWORD (WINAPI *GetServiceListActualEP3)(DWORD id, DWORD* serviceListSize, SERVICE_INFO** serviceList);
	typedef DWORD (WINAPI *GetServiceListEpgDBEP3)(DWORD id, DWORD* serviceListSize, SERVICE_INFO** serviceList);
	typedef DWORD (WINAPI *GetEpgInfoListEP3)(DWORD id, WORD originalNetworkID, WORD transportStreamID, WORD serviceID, DWORD* epgInfoListSize, EPG_EVENT_INFO** epgInfoList);
	typedef DWORD (WINAPI *EnumEpgInfoListEP3)(DWORD id, WORD originalNetworkID, WORD transportStreamID, WORD serviceID,
	                                           BOOL (CALLBACK *enumEpgInfoListEP3Proc)(DWORD epgInfoListSize, EPG_EVENT_INFO* epgInfoList, void* param), void* param);
	typedef void (WINAPI *ClearSectionStatusEP3)(DWORD id);
	typedef EPG_SECTION_STATUS (WINAPI *GetSectionStatusEP3)(DWORD id, BOOL l_eitFlag);
	typedef EPG_SECTION_STATUS (WINAPI *GetSectionStatusServiceEP3)(DWORD id, WORD originalNetworkID, WORD transportStreamID, WORD serviceID, BOOL l_eitFlag);
	typedef DWORD (WINAPI *GetEpgInfoEP3)(DWORD id, WORD originalNetworkID, WORD transportStreamID, WORD serviceID, BOOL nextFlag, EPG_EVENT_INFO** epgInfo);
	typedef DWORD (WINAPI *SearchEpgInfoEP3)(DWORD id, WORD originalNetworkID, WORD transportStreamID, WORD serviceID, WORD eventID, BYTE pfOnlyFlag, EPG_EVENT_INFO** epgInfo);
	typedef void (WINAPI *SetLogoTypeFlagsEP3)(DWORD id, DWORD flags, const WORD** additionalNeededPids);
	typedef DWORD (WINAPI *EnumLogoListEP3)(DWORD id, BOOL (CALLBACK *enumLogoListProc)(DWORD logoListSize, const LOGO_INFO* logoList, void* param), void* param);
	typedef int (WINAPI *GetTimeDelayEP3)(DWORD id);
	typedef DWORD (WINAPI *SetDebugLogCallbackEP3)(void (CALLBACK *debugLogProc)(const WCHAR* s));

	std::unique_ptr<void, void(*)(void*)> module;
	DWORD id;
	UnInitializeEP3			pfnUnInitializeEP3;
	AddTSPacketEP3			pfnAddTSPacketEP3;
	GetTSIDEP3				pfnGetTSIDEP3;
	GetEpgInfoListEP3		pfnGetEpgInfoListEP3;
	EnumEpgInfoListEP3		pfnEnumEpgInfoListEP3;
	ClearSectionStatusEP3	pfnClearSectionStatusEP3;
	GetSectionStatusEP3		pfnGetSectionStatusEP3;
	GetSectionStatusServiceEP3	pfnGetSectionStatusServiceEP3;
	GetServiceListActualEP3	pfnGetServiceListActualEP3;
	GetServiceListEpgDBEP3	pfnGetServiceListEpgDBEP3;
	GetEpgInfoEP3			pfnGetEpgInfoEP3;
	SearchEpgInfoEP3		pfnSearchEpgInfoEP3;
	SetLogoTypeFlagsEP3		pfnSetLogoTypeFlagsEP3;
	EnumLogoListEP3			pfnEnumLogoListEP3;
	GetTimeDelayEP3			pfnGetTimeDelayEP3;
	SetDebugLogCallbackEP3	pfnSetDebugLogCallbackEP3;

	static void CALLBACK DebugLogCallback(const WCHAR* s);
};

