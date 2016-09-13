#pragma once

#include "../../Common/TimeShiftUtil.h"
#include "../../Common/InstanceManager.h"

class CFileStreamingManager
{
public:
	void CloseAllFile(
		);
	BOOL IsStreaming();

	BOOL OpenTimeShift(
		LPCWSTR filePath,
		DWORD* ctrlID
		);
	BOOL OpenFile(
		LPCWSTR filePath,
		DWORD* ctrlID
		);
	BOOL CloseFile(
		DWORD ctrlID
		);
	BOOL StartSend(
		DWORD ctrlID
		);
	BOOL StopSend(
		DWORD ctrlID
		);

	//ストリーム配信で現在の送信位置と総ファイルサイズを取得する
	//戻り値：
	// エラーコード
	//引数：
	// val				[IN/OUT]サイズ情報
	BOOL GetPos(
		NWPLAY_POS_CMD* val
		);

	//ストリーム配信で送信位置をシークする
	//戻り値：
	// エラーコード
	//引数：
	// val				[IN]サイズ情報
	BOOL SetPos(
		NWPLAY_POS_CMD* val
		);

	//ストリーム配信で送信先を設定する
	//戻り値：
	// エラーコード
	//引数：
	// val				[IN]サイズ情報
	BOOL SetIP(
		NWPLAY_PLAY_INFO* val
		);

protected:
	//非仮想デストラクタ注意!
	class CMyInstanceManager : public CInstanceManager<CTimeShiftUtil>
	{
	public:
		void clear() {
			CBlockLock lock(&this->m_lock);
			this->m_list.clear();
		}
		bool empty() {
			CBlockLock lock(&this->m_lock);
			return this->m_list.empty();
		}
	};
	CMyInstanceManager utilMng;
};

