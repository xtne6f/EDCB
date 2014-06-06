#pragma once

/*
12.2.1 告知情報の伝送方式
12.2.1.1 ソフトウェアダウンロードトリガーテーブル（Software Donwload Trigger Table）
ダウンロードの告知情報通知のために、ソフトウェアダウンロードトリガーテーブル(Software
Donwload Trigger Table)を用いる。但し、12.2.2.2 節のセクション形式で送出されるダウンロー
ドコンテンツにはソフトウェアダウンロードトリガーテーブルは使用しない。

software_download_trigger_section(){
	table_id						8 uimsbf
	section_syntax_indicator		1 bslbf
	reserved_future_use				1 bslbf
	reserved						2 bslbf
	section_length					12 uimsbf
	table_id_ext					16 uimsbf
	reserved						2 bslbf
	version_number					5 uimsbf
	current_next_indicator			1 uimsbf
	section_number					8 uimsbf
	last_section_number				8 uimsbf
	transport_stream_id				16 uimsbf
	original_network_id				16 uimsbf
	service_id						16 uimsbf
	num_of_contents					8 uimsbf
	for(i=0;i<num_of_contents;i++){
		group							4 bslbf
		target_version					12 uimsbf
		new_version						12 uimsbf
		download_level					2 bslbf
		version_indicator				2 bslbf
		content_description_length		12 uimsbf
		reserved						4 bslbf
		schedule_description_length		12 uimsbf
		schedule_time-shift_information	4 uimsbf
		for(i=0;i<N;i++){
			start_time						40 uimsbf
			duration						24 uimsbf
		}
		for(j=0;j<N2;j++){
			descriptors()
		}
	}
	CRC_32							32 rpchof
}

table_id（テーブル識別）：0xC3
section_syntax_indicator（セクションシンタクス指示）：１
section_length（セクション長）：セクション長フィールドの直後からCRC を含むセクションの最
後までのセクションのバイト数を規定する。セクション長は4093 を超えてはならない。
table_id_ext：
 maker_id	8 uimsbf
 model_id	8 uimsbf
version_number（バージョン番号）：サブテーブルのバージョン番号である。バージョン番号はサ
ブテーブル内の情報に変化があった場合に１加算される。その値が31 になった場合、0 に戻る。
current_next_indicator（カレントネクスト指示）：1
section_number（セクション番号）：セクションの番号を表す。
last_section_number（最終セクション番号）：セクションが属するサブテーブルの最後のセクショ
ンの番号を規定する。
transport_stream_id（トランスポートストリーム識別）：トランスポートストリームを分配システ
ム内の他の多重から識別するラベル。
original_network_id（オリジナルネットワーク識別）：元の分配システムのネットワーク識別を規
定するラベル。
service_id（サービス識別）：ダウンロードの際に、ダウンロードコンテンツが伝送されるサービス
を識別するためのラベル。
num_of_contents：このテーブルで告知されているダウンロードコンテンツの数を示す。
group：group_id が格納される。
target_version：ダウンロードの際に、アップデート対象となるコンテンツのバージョン番号を示す。
new_version：ダウンロードの際に、今回ダウンロードされるコンテンツのバージョン番号を示す
download_level（ダウンロードレベル）：01 の場合は強制ダウンロード、00 の場合は任意ダウンロ
ード。
version_indicator（バージョン指示）：以下の通り
00： 全バージョンが対象（バージョン指定は無効）。
01： 指定されたバージョン以降が対象。
02： 指定されたバージョン以前が対象。
03： 指定されたバージョンのみが対象。
content_description_length()：スケジュールループと記述子ループの合計のバイト長。
schedule_description_length（スケジュール記述長）：スケジュールループのバイト長。全受信機
共通データで、この値が0 の場合は、対象となるダウンロードコンテンツが伝送されていることを
示す。
schedule_time-shift_information(スケジュール時間シフト情報)：以下の通り
0： 複数のservice_id で同一ダウンロードコンテンツを同一スケジュールで送出
1~12： 複数のservice_id で同一ダウンロードコンテンツを各serice_id 毎に1~12 時間の
時間シフトを行って送出
13~14： リザーブ
15： 単一のservice_id でダウンロードコンテンツを送出
なお、複数のservice_id でダウンロードコンテンツを送出する場合のservice_id の特定方法など詳
細運用は本書付属-3「ダウンロード機能」、及び放送事業者運用規程を参照のこと。
start_time（開始時間）：ダウンロードの配信の開始時間を日本標準時(JST)と修正ユリウス日(MJD)
で示す。
duration（継続時間）：配信継続時間

*/


#include "../../../Common/Util.h"
#include "../AribDescriptor.h"
#include "PSITable.h"

class CSDTTTable : public CPSITable
{
public:
	typedef struct _SCHEDULE_INFO_DATA{
		SYSTEMTIME start_time;
		DWORD duration;	//秒数
	}SCHEDULE_INFO_DATA;
	typedef struct _CONTENT_INFO_DATA{
		BYTE group;
		WORD target_version;
		WORD new_version;
		BYTE download_level;
		BYTE version_indicator;
		WORD content_description_length;
		WORD schedule_description_length;
		BYTE schedule_time_shift_information;
		vector<SCHEDULE_INFO_DATA> scheduleList;
		vector<AribDescriptor::CDescriptor*> descriptorList;
		~_CONTENT_INFO_DATA(void){
			for( size_t i=0; i<descriptorList.size(); i++ ){
				SAFE_DELETE(descriptorList[i]);
			}
			descriptorList.clear();
		};
	} CONTENT_INFO_DATA;
	BYTE maker_id;
	BYTE model_id;
	BYTE version_number;
	BYTE current_next_indicator;
	BYTE section_number;
	BYTE last_section_number;
	WORD transport_stream_id;
	WORD original_network_id;
	WORD service_id;
	BYTE num_of_contents;
	vector<CONTENT_INFO_DATA*> contentInfoList;

public:
	CSDTTTable(void);
	~CSDTTTable(void);

	BOOL Decode( BYTE* data, DWORD dataSize, DWORD* decodeReadSize );

protected:
	void Clear();
};
