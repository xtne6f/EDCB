#pragma once

#include "../../Common/WritePlugInUtil.h"
#include "../../BonCtrl/PacketInit.h"
#include "../../BonCtrl/ServiceFilter.h"

class CWriteMain
{
public:
	CWriteMain();
	~CWriteMain();

	//数珠繋ぎする出力プラグインを初期化する
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// pluginPath			[IN]ロードするプラグインのフルパス
	BOOL InitializeDownstreamPlugin(
		const wstring& pluginPath
		);

	//ファイル保存を開始する
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// fileName				[IN]保存ファイルフルパス（必要に応じて拡張子変えたりなど行う）
	// overWriteFlag		[IN]同一ファイル名存在時に上書きするかどうか（TRUE：する、FALSE：しない）
	// createSize			[IN]入力予想容量（188バイトTSでの容量。即時録画時など総時間未定の場合は0。延長などの可能性もあるので目安程度）
	BOOL Start(
		LPCWSTR fileName,
		BOOL overWriteFlag,
		ULONGLONG createSize
		);

	//ファイル保存を終了する
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	BOOL Stop(
		);

	//実際に保存しているファイルパスを取得する（再生やバッチ処理に利用される）
	//戻り値：
	// 保存ファイルフルパス
	wstring GetSavePath(
		);

	//保存用TSデータを送る
	//空き容量不足などで書き出し失敗した場合、writeSizeの値を元に
	//再度保存処理するときの送信開始地点を決める
	//戻り値：
	// TRUE（成功）、FALSE（失敗）
	//引数：
	// data					[IN]TSデータ
	// size					[IN]dataのサイズ
	// writeSize			[OUT]保存に利用したサイズ
	BOOL Write(
		BYTE* data,
		DWORD size,
		DWORD* writeSize
		);

private:
	std::unique_ptr<FILE, fclose_deleter> file;
	std::unique_ptr<CWritePlugInUtil> writePlugin;
	wstring savePath;
	WORD targetSID;
	WORD lastTSID;
	CPacketInit packetInit;
	CServiceFilter serviceFilter;
	vector<BYTE> outBuff;
};

