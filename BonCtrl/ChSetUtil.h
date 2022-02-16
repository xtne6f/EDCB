#pragma once

#include "../Common/StructDef.h"
#include "../Common/EpgDataCap3Def.h"
#include "../Common/ParseTextInstances.h"

class CChSetUtil
{
public:
	//チャンネル設定ファイルを読み込む
	BOOL LoadChSet(
		const wstring& settingPath,
		const wstring& driverName,
		wstring tunerName
		);

	//チャンネル設定ファイルを保存する
	BOOL SaveChSet(
		const wstring& settingPath,
		const wstring& driverName,
		wstring tunerName
		);

	//チャンネルスキャン用にクリアする
	BOOL Clear();

	//チャンネル情報を追加する
	BOOL AddServiceInfo(
		DWORD space,
		DWORD ch,
		const wstring& chName,
		SERVICE_INFO* serviceInfo
		);

	//サービス一覧を取得する
	//戻り値は非constメソッドを呼び出すまで有効。mapのキーは読み込み順番号
	const map<DWORD, CH_DATA4>& GetServiceList() const {
		return chText4.GetMap();
	}

	//IDから物理チャンネルを検索する
	BOOL GetCh(
		WORD ONID,
		WORD TSID,
		WORD SID,
		DWORD& space,
		DWORD& ch
		) const;

	//EPG取得対象のサービス一覧を取得する
	vector<SET_CH_INFO> GetEpgCapService() const;

	//現在のチューナに限定されないEPG取得対象のサービス一覧を取得する
	vector<SET_CH_INFO> GetEpgCapServiceAll(
		int ONID = -1,
		int TSID = -1
		) const;

	//部分受信サービスかどうか
	BOOL IsPartial(
		WORD ONID,
		WORD TSID,
		WORD SID
		) const;

	//サービスタイプが映像サービスかどうか
	static BOOL IsVideoServiceType(
		WORD serviceType
		){
		return serviceType == 0x01 //デジタルTV
			|| serviceType == 0xA5 //プロモーション映像
			|| serviceType == 0xAD //超高精細度4K専用TV
			;
	}

protected:
	CParseChText4 chText4;
	CParseChText5 chText5;
};

