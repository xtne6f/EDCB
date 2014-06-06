#pragma once

/*
5.2.9 時刻日付オフセットテーブル（TOT）（Time Offset Table）
TOT（表5－9 参照）は、JST 時刻と日付（修正ユリウス日）情報、およびサマータイ
ム実施時の時間オフセット値を伝送する。TOT は表5－9 のデータ構造を使用した単一セ
クションから構成される。このTOT セクションは、PID 値0x0014 のトランスポートスト
リームパケットにより伝送され、また、テーブル識別値は0x73 とする。

time_offset_section(){
	table_id					8 uimsbf
	section_syntax_indicator	1 bslbf
	reserved_future_use			1 bslbf
	reserved					2 bslbf
	section_length				12 uimsbf
	JST_time					40 bslbf
	reserved					4 bslbf
	descriptors_loop_length		12 uimsbf
	for (i=0;i<N;i++){
		descriptor()
	}
	CRC_32						32 rpchof
}

table_id（テーブル識別）：表5－2 による。
section_syntax_indicator（セクションシンタクス指示）：これは１ビットの指示で「0」
に設定する。
section_length（セクション長）：これは12 ビットのフィールドで、先頭の2 ビットは
常に「00」とする。これは、セクション長フィールドの直後からCRC を含むセクション
の最後までのセクションのバイト数を規定する。
JST_time（現在日付、現在時刻）：この40 ビットのフィールドは、日本標準時（JST）
と修正ユリウス日（MJD）（付録C 参照）による現在日付と現在時刻を含む。このフィー
ルドは、MJD の下位16 ビットを16 ビットで符号化し、続く24 ビットを6 個の4 ビット
2 進化10 進数（BCD）で符号化する。
descriptors_loop_length（記述子ループ長）：これは、12 ビットのフィールドで、後
続の記述子の全バイト長を規定する。
CRC_32（CRC）：これは付録B で定義するデコーダにおいて、セクション全体を処理
した後にレジスタ出力がゼロになるようなCRC値を含む、32 ビットのフィールドである。


*/

#include "../../../Common/Util.h"
#include "../AribDescriptor.h"
#include "PSITable.h"

class CTOTTable : public CPSITable
{
public:
	SYSTEMTIME jst_time;
	WORD descriptors_loop_length;
	vector<AribDescriptor::CDescriptor*> descriptorList;
public:
	CTOTTable(void);
	~CTOTTable(void);

	BOOL Decode( BYTE* data, DWORD dataSize, DWORD* decodeReadSize );
protected:
	void Clear();
};
