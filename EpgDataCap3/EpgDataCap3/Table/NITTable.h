#pragma once

/*
5.2.4 ネットワーク情報テーブル（NIT）（Network Information Table）
［注］本項目は、総務省告示平 21 第88 号(8)でも説明されている。
NIT（表5－3 参照）は、ネットワークにより運ばれる多重／トランスポートストリーム
の物理的構成に関する情報、およびネットワーク自身の特性を表わす。オリジナルネット
ワーク識別とトランスポートストリーム識別の組み合わせにより、各トランスポートスト
リームを本規格の適用範囲全体において唯一に定義することが可能である。ネットワーク
は固有のネットワーク識別を割り当てられ、それがネットワークの固有識別符号として機
能する。ネットワーク識別の値は標準化機関の規定による。また、トランスポート識別の
値は事業者が独自に選定することができる。トランスポートストリームが発生したネット
ワークにおいてNIT が伝送された場合には、ネットワーク識別とオリジナルネットワーク
識別は同一値である。
例えば衛星からケーブルへの間など、伝送媒体の境界における番組配列情報の処理につ
いてのガイドラインは別途定める。
受信機はNIT 情報を不揮発性メモリに記憶することにより、チャンネル間の切り替え
（「チャンネルホッピング」）時のアクセス時間を最小にすることができる。自ネットワ
ークだけでなく、それに付け加えて他ネットワークのNIT を送信することも可能である。
自ネットワークのNIT と他ネットワークのNIT との区別は、テーブル識別値の違いを利
用して行われる。（表5－2 参照）
NIT は、表5－3 のデータ構造を使用して、ネットワーク情報セクションに分割される。
NIT の一部分となるセクションは全て、PID 値0x0010 のトランスポートストリームパケ
ットにより伝送される。自ネットワーク（すなわちNIT を含むトランスポートストリーム
がその一部をなすネットワーク）を記述するNIT のセクションは全てテーブル識別値は
0x40 とし、ネットワーク識別フィールドは標準化機関の規定により自ネットワークに割り
当てられている値としなければならない。自ネットワーク以外のネットワークに関する
NIT のセクションは、テーブル識別値0x41 および標準化機関の規定により他ネットワー
クに割り当てられているネットワーク識別値を持たなければならない。

network_information_section(){
	table_id						8 uimsbf
	section_syntax_indicator		1 bslbf
	reserved_future_use				1 bslbf
	reserved						2 bslbf
	section_length					12 uimsbf
	network_id						16 uimsbf
	reserved						2 bslbf
	version_number					5 uimsbf
	current_next_indicator			1 bslbf
	section_number					8 uimsbf
	last_section_number				8 uimsbf
	reserved_future_use				4 bslbf
	network_descriptors_length		12 uimsbf
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
		for(j=0;j<N;j++)
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
network_id（ネットワーク識別）：これは16 ビットのフィールドで、NIT が示す分配シ
ステムを他の分配システムと区別して識別するラベルの役割をする。このフィールド値の
割り当ては標準化機関の規定による。（付録Ｎ参照）
version_number（バージョン番号）：この5 ビットのフィールドはサブテーブルのバージ
ョン番号である。バージョン番号はサブテーブル内の情報に変化があった場合に1 加算さ
れる。その値が31 になった場合は、その次は0 に戻る。カレントネクスト指示が「1」の
場合は、バージョン番号はテーブル識別とネットワーク識別で定義される現在のサブテー
ブルのバージョン番号になる。カレントネクスト指示が「0」の場合は、バージョン番号は
テーブル識別とネットワーク識別で定義される次のサブテーブルのバージョン番号になる。
current_next_indicator（カレントネクスト指示）：この1 ビットの指示は、それが「1」
の場合はサブテーブルが現在のサブテーブルであることを示す。「0」の場合は、送られる
サブテーブルはまだ適用されず、次のサブテーブルとして使用されることを示す。
section_number（セクション番号）：この8 ビットのフィールドはセクションの番号を表
す。サブテーブル中の最初のセクションのセクション番号は、0x00 である。セクション番
号は、同一のテーブル識別とネットワーク識別を持つセクションの追加ごとに1 加算され
る。
last_section_number（最終セクション番号）：この8 ビットのフィールドは、そのセクシ
ョンが属するサブテーブルの最後のセクション（すなわち、最大のセクション番号を持つ
セクション）の番号を規定する。
network_descriptors_length（ネットワーク記述子長）：この12 ビットのフィールドは、
後に続くネットワーク記述子の全バイト数を表す。
transport_stream_loop_length（トランスポートストリームループ長）：これは、CRC_32
の最初のバイトの直前に終わるトランスポートストリームループの全バイト数を規定する
12 ビットのフィールドである。
transport_stream_id（トランスポートストリーム識別）：これは、このトランスポートス
トリームを分配システム内の他の多重から識別するためのラベルの役割をする16 ビット
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

class CNITTable : public CPSITable
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
	WORD network_id;
	BYTE version_number;
	BYTE current_next_indicator;
	BYTE section_number;
	BYTE last_section_number;
	WORD network_descriptors_length;
	vector<AribDescriptor::CDescriptor*> descriptorList;
	WORD transport_stream_loop_length;
	vector<TS_INFO_DATA*> TSInfoList;

public:
	CNITTable(void);
	~CNITTable(void);

	BOOL Decode( BYTE* data, DWORD dataSize, DWORD* decodeReadSize );
protected:
	void Clear();

	BOOL SDDecode( BYTE* data, DWORD dataSize, vector<AribDescriptor::CDescriptor*>* descriptorList, DWORD* decodeReadSize );
};
