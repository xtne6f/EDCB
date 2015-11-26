#pragma once

/*
5.2.5 ブーケアソシエーションテーブル（BAT）（Bouquet Association Table）
BAT（表5－4 参照）はブーケに関する情報を提供する。ブーケはサービスの集合であり
ネットワークの境界を越えることもある。
BAT は表5－4 に示すデータ構造により、ブーケアソシエーションセクションに分割さ
れる。BAT の一部分となるセクションは全て、PID 値0x0011 のトランスポートストリー
ムパケットにより伝送される。特定のブーケを表すBAT サブテーブルのセクションは、別
途規定されるブーケに割り当てられた値のブーケ識別フィールドを持つ。全てのBAT セク
ションはテーブル識別値は0x4A である。

bouquet_association_section(){
	table_id						8 uimsbf
	section_syntax_indicator		1 bslbf
	reserved_future_use				1 bslbf
	reserved						2 bslbf
	section_length					12 uimsbf
	bouquet_id						16 uimsbf
	reserved						2 bslbf
	version_number					5 uimsbf
	current_next_indicator			1 bslbf
	section_number					8 uimsbf
	last_section_number				8 uimsbf
	reserved_future_use				4 bslbf
	bouquet_descriptors_length		12 uimsbf
	for(i=0;i<N;i++){
		descriptor()
	}
	reserved_future_use				4 bslbf
	transport_stream_loop_length	12 uimsbf
	for(i=0;i<N;i++){
		transport_stream_id				16 uimsbf
		original_network_id				16 uimsbf
		reserved_future_use				4 bslbf
		transport_descriptors_length	12 uimsbf
		for(j=0;j<N;j++){
			descriptor()
		}
	}
	CRC_32							32 rpchof
}

table_id（テーブル識別）：表5－2 による。
section_syntax_indicator（セクションシンタクス指示）：セクションシンタクス指示は1
ビットのフィールドで、常に1 とする。
section_length（セクション長）：これは12 ビットのフィールドで、先頭の2 ビットは常
に「00」とする。これは、セクション長フィールドの直後からCRC を含むセクションの
最後までのセクションのバイト数を規定する。全セクションの長さが1024 バイトを超えな
いようにするため、セクション長は1021 を超えてはならない。
bouquet_id（ブーケ識別）：これは16 ビットのフィールドで、ブーケを識別するラベルの
役割をする。このフィールド値の割り当ては別途規定する。
version_number（バージョン番号）：この5 ビットのフィールドはサブテーブルのバージ
ョン番号である。バージョン番号はサブテーブル内の情報に変化があった場合に1 加算さ
れる。その値が31 になった場合は、その次は0 に戻る。カレントネクスト指示が「1」の
場合は、バージョン番号はテーブル識別とブーケ識別で定義される現在のサブテーブルの
バージョン番号になる。カレントネクスト指示が「0」の場合は、バージョン番号はテーブ
ル識別とブーケ識別で定義される次のサブテーブルのバージョン番号になる。
current_next_indicator（カレントネクスト指示）：この1 ビットの指示は、それが「1」
の場合はサブテーブルが現在のサブテーブルであることを示す。「0」の場合は、送られる
サブテーブルはまだ適用されず、次のサブテーブルとして使用されることを示す。
section_number（セクション番号）：この8 ビットのフィールドはセクションの番号を表
す。サブテーブル中の最初のセクションのセクション番号は、0x00 である。セクション番
号は同一のテーブル識別とブーケ識別を持つセクションの追加ごとに1 加算される。
last_section_number（最終セクション番号）：この8 ビットのフィールドは、そのセクシ
ョンが属するサブテーブルの最後のセクション（すなわち、最大のセクション番号を持つ
セクション）の番号を規定する。
bouquet_descriptors_length（ブーケ記述子長）：この12 ビットのフィールドは、後に続
く記述子の全バイト数を表す。
transport_stream_loop_length（トランスポートストリームループ長）：これは、CRC_32
の最初のバイトの直前に終わるトランスポートストリームループの全バイト数を規定する
12 ビットのフィールドである。
transport_stream_id（トランスポートストリーム識別）：これは、このトランスポートス
トリームを分配システム中の他の多重から識別するためのラベルの役割をする16 ビット
のフィールドである。
original_network_id（オリジナルネットワーク識別）：この16 ビットのフィールドは、
元の分配システムのネットワーク識別を規定するラベルの役割をする。
transport_descriptors_length（トランスポート記述子長）：これは12 ビットのフィール
ドで、以下に続くトランスポートストリーム記述子の全体のバイト数を規定する。
CRC_32（CRC）：これは付録B で定義するデコーダにおいて、セクション全体を処理し
た後にレジスタ出力がゼロになるようなCRC 値を含む、32 ビットのフィールドである。

*/

#include "../../../Common/Util.h"
#include "../AribDescriptor.h"
#include "PSITable.h"

class CBATTable : public CPSITable
{
public:
	typedef struct _TS_INFO_DATA{
		WORD transport_stream_id;
		WORD original_network_id;
		WORD transport_descriptors_length;
		vector<AribDescriptor::CDescriptor*> descriptorList;
		~_TS_INFO_DATA(void){
			for( size_t i=0; i<descriptorList.size(); i++ ){
				SAFE_DELETE(descriptorList[i]);
			}
			descriptorList.clear();
		};
	} TS_INFO_DATA;
	WORD bouquet_id;
	BYTE version_number;
	BYTE current_next_indicator;
	BYTE section_number;
	BYTE last_section_number;
	WORD bouquet_descriptors_length;
	vector<AribDescriptor::CDescriptor*> descriptorList;
	WORD transport_stream_loop_length;
	vector<TS_INFO_DATA*> TSInfoList;

public:
	CBATTable(void);
	~CBATTable(void);

	BOOL Decode( BYTE* data, DWORD dataSize, DWORD* decodeReadSize );
protected:
	void Clear();
};
