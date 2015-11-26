#pragma once

/*
8.2.8.2.2.2 選択情報テーブル(SIT)
SIT はパーシャルトランスポートストリームで伝送されるサービスとイベントを表す。
SIT は表8-2-9 のデータ構造を使用して選択情報セクションに分割される。SIT の一部
分となるセクションはすべて、PID 値0x001F のトランスポートパケットにより伝送され、
また、テーブル識別値は0x7F とする。

selection_information_section(){
	table_id					8 uimsbf
	section_syntax_indicator	1 bslbf
	reserved_future_use			1 bslbf
	ISO_reserved				2 bslbf
	section_length				12 uimsbf
	reserved_future_use			16 bslbf
	ISO_reserved				2 bslbf
	version_number				5 uimsbf
	current_next_indicator		1 bslbf
	section_number				8 uimsbf
	last_section_number			8 uimsbf
	reserved_future_use			4 uimsbf
	transmission_info_loop_length	12 uimsbf
	for(i=0;i<N;i++){
		descriptor()
	}
	for(i=0;i<N;i++){
		service_id					16 uimsbf
		reserved_future_use			1 uimsbf
		running_status				3 bslbf
		service_loop_length			12 uimsbf
		for(j=0;j<N;j++){
			descriptor()
		}
	}
	CRC_32 32 rpchof
}

table_id (テーブル識別)：表8-2-7 による。
section_syntax_indicator (セクションシンタクス指示)：セクションシンタクス指示は1
ビットのフィールドで、常に"1"とする。
section_length (セクション長)：これは12 ビットフィールドとする。section_length は
このフィールドの直後からCRC を含むセクションの最後までのセクションのバイト数を
規定する。全セクション長が最大4096 バイトを越えないために、セクション長は4093
バイトを越えてはならない。
version_number (バージョン番号)：この5 ビットフィールドはテーブルのバージョン番
号である。バージョン番号はテーブル内の情報に変化があった場合に１加算される。その
値が31 になった場合は、0 に戻る。カレントネクスト指示が"1"の場合は、バージョン番
号は現在のテーブルのバージョン番号になる。カレントネクスト指示が"0"の場合は、バー
ジョン番号は次のテーブルのバージョン番号になる。
current_next_indicator (カレントネクスト指示)：この1 ビットの指示が"1"の場合、テ
ーブルが現在のテーブルであることを示し、"0"の場合は、送られるテーブルはまだ適応さ
れず、次のテーブルとして使用されることを示す。
section_number (セクション番号)：この8 ビットフィールドはセクションの番号を示す。
セクション番号は、0x00 である。
last_section_number (最終セクション番号)：この8 ビットフィールドは最終のセクショ
ン番号を示す。最終セクション番号は、0x00 である。
transmission_info_loop_length (伝送情報ループ長)：この12 ビットフィールドはパー
シャルトランスポートストリームの伝送パラメータを記述するデスクリプタの全バイト数
を表わす。
service_id (サービス識別)：これは16 ビットフィールドで、トランスポートストリーム
内の他のサービスからこのサービスを識別するためのラベルの役割をする。サービス識別
はプログラムマップセクション内の放送番組番号識別 (program_number)に等しい。
running_status (進行状態)：この3 ビットフィールドは元のストリームのイベントの進
行状態を表す。これは、元の現在のイベントの進行状態である。もし元のストリームに現
在のイベントが存在しなければ、状態は非実行中と考えられる。走行状態の値の意味は
ARIB STD-B10 に定義されている。
service_loop_length (サービスループ長)：この12 ビットフィールドは以下に続くパー
シャルトランスポートストリームに含まれるサービスとイベントのSI 関連情報を含むデ
スクリプタループの全バイト長を規定する。
CRC_32 (CRC)：これはARIB STD-B10 の付録B で定義するデコーダにおいて、セク
ション全体を処理した後にレジスタ出力がゼロになるようなCRC 値を含む、32 ビットフ
ィールドである。

*/

#include "../../../Common/Util.h"
#include "../AribDescriptor.h"
#include "PSITable.h"

class CSITTable : public CPSITable
{
public:
	typedef struct _SERVICE_LOOP_DATA{
		WORD service_id;
		BYTE running_status;
		WORD service_loop_length;
		vector<AribDescriptor::CDescriptor*> descriptorList;
		~_SERVICE_LOOP_DATA(void){
			for( size_t i=0; i<descriptorList.size(); i++ ){
				SAFE_DELETE(descriptorList[i]);
			}
			descriptorList.clear();
		};
	} SERVICE_LOOP_DATA;
	BYTE version_number;
	BYTE current_next_indicator;
	BYTE section_number;
	BYTE last_section_number;
	WORD transmission_info_loop_length;
	vector<AribDescriptor::CDescriptor*> descriptorList;
	vector<SERVICE_LOOP_DATA*> serviceLoopList;

public:
	CSITTable(void);
	~CSITTable(void);

	BOOL Decode( BYTE* data, DWORD dataSize, DWORD* decodeReadSize );
protected:
	void Clear();
};

