#pragma once

/*
12.2.2.2 ダウンロードコンテンツ・セクション伝送方式
ダウンロードコンテンツをセクション形式で伝送する場合はCDT（Common Data Table）を用
いる。
(1) 全受信機共通データテーブル（CDT）（Common Data Table）
CDT は、そのテーブルを受信する全ての受信機を対象として、不揮発性メモリに格納すべき共通
データをセクション形式で伝送するために用いる。地上デジタルテレビジョン放送では、事業者の
サービスロゴデータはCDT 内のdata_module_byte にロゴデータを配置して伝送する。ロゴデー
タフォーマットは「付属(ダウンロード機能)」のAppendix.A を参照のこと。
CDT については、ARIB STD-B10 も参照のこと。
CDT のデータ構造を表12-6 に示す。

common_data_section(){
	table_id					8 uimsbf
	section_syntax_indicator	1 bslbf
	reserved_future_use			1 bslbf
	reserved					2 bslbf
	section_length				12 uimsbf
	download_data_id			16 uimsbf
	reserved					2 bslbf
	version_number				5 uimsbf
	current_next_indicator		1 bslbf
	section_number				8 uimsbf
	last_section_number			8 uimsbf
	original_network_id			16 uimsbf
	data_type					8 uimsbf
	reserved_future_use			4 bslbf
	descriptors_loop_length		12 uimsbf
	for(i=0;i<n;i++){
		descriptor()
	}
	for(j=0;j<m;j++){
		data_module_byte			8 uimsbf
	}
	CRC_32						32 rpchof
}

table_id（テーブル識別）：0xC8
section_syntax_indicator（セクションシンタクス指示）：セクションシンタクス指示は1 ビットの
フィールドで、常に1 とする。
section_length（セクション長）：これは12 ビットのフィールドである。これは、セクション長フ
ィールドの直後からCRC を含むセクションの最後までのセクションのバイト数を規定する。全セ
クションの長さが4096 バイトを超えないようにするため、セクション長は4093 を超えてはならな
い。
download_data_id（ダウンロードデータ識別）： これは16 ビットのフィールドで、全受信機共
通データのダウンロードデータ識別を指定する。ダウンロードデータ識別は、オリジナルネットワ
ーク識別（original_network_id）毎にユニークとする。サービスロゴの場合は、この値はSDT に
配置される後述のロゴ伝送記述子に記載のdownload_data_id の値と一致する。
version_number（バージョン番号）：この5 ビットのフィールドはサブテーブルのバージョン番号
である。バージョン番号はサブテーブル内の情報に変化があった場合に1 加算される。その値が31
になった場合は、その次は0 に戻る。カレントネクスト指示が「1」の場合は、バージョン番号は
テーブル識別とネットワーク識別で定義される現在のサブテーブルのバージョン番号になる。カレ
ントネクスト指示が「0」の場合は、バージョン番号はテーブル識別とネットワーク識別で定義さ
れる次のサブテーブルのバージョン番号になる。
current_next_indicator（カレントネクスト指示）：この1 ビットの指示は、それが「1」の場合は
サブテーブルが現在のサブテーブルであることを示す。「0」の場合は、送られるサブテーブルはま
だ適用されず、次のサブテーブルとして使用されることを示す。
section_number（セクション番号）：この8 ビットのフィールドはセクションの番号を表す。サブ
テーブル中の最初のセクションのセクション番号は、0x00 である。セクション番号は、同一のテー
ブル識別とネットワーク識別を持つセクションの追加ごとに1 加算される。
last_section_number（最終セクション番号）：この8 ビットのフィールドは、そのセクションが属
するサブテーブルの最後のセクション（すなわち、最大のセクション番号を持つセクション）の番
号を規定する。
original_network_id（オリジナルネットワーク識別）：この16 ビットのフィールドは、元の分配シ
ステムのネットワーク識別を規定するラベルの役割をする。
data_type（データ属性）：この8 ビットのフィールドは、伝送されているダウンロードデータの種
類を示す。0x01 はロゴデータとし、運用詳細は本書付属-3「ダウンロード機能」、及び放送事業者
規程を参照のこと。それ以外は将来拡張とする。
descriptors_length（記述子長）：これは、12 ビットのフィールドで、後続の記述子の全バイト長を
規定する。
data_module_byte（データモジュールバイト）：data_type 毎に定義されるシンタックスによりダ
ウンロードデータを記載する。サービスロゴの場合は本書付属-3「ダウンロード機能」、及び放送事
業者規程を参照のこと。
CRC_32（CRC）：これはARIB STD-B10 の付録B で定義するデコーダにおいて、セクション全体
を処理した後にレジスタ出力がゼロになるようなCRC 値を含む、32 ビットのフィールドである。


*/

#include "../../../Common/Util.h"
#include "../AribDescriptor.h"
#include "PSITable.h"

class CCDTTable : public CPSITable
{
public:
	WORD download_data_id;
	BYTE version_number;
	BYTE current_next_indicator;
	BYTE section_number;
	BYTE last_section_number;
	WORD original_network_id;
	BYTE data_type;
	WORD descriptors_loop_length;
	vector<AribDescriptor::CDescriptor*> descriptorList;
	WORD data_module_byteSize;
	BYTE* data_module_byte;

public:
	CCDTTable(void);
	~CCDTTable(void);

	BOOL Decode( BYTE* data, DWORD dataSize, DWORD* decodeReadSize );

protected:
	void Clear();
};
