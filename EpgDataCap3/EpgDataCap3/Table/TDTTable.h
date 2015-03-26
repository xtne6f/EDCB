#pragma once

/*
5.2.8 時刻日付テーブル（TDT）（Time and Date Table）
TDT（表5－8 参照）は、JST 時刻と日付情報のみを伝送する。
TDT は表5－8 のデータ構造を使用した単一セクションから構成される。このTDT セク
ションは、PID 値0x0014 のトランスポートストリームパケットにより伝送され、また、
テーブル識別値は0x70 とする。

time_date_section(){
	table_id					8 uimsbf
	section_syntax_indicator	1 bslbf
	reserved_future_use			1 bslbf
	reserved					2 bslbf
	section_length				12 uimsbf
	JST_time					40 bslbf
}

table_id（テーブル識別）：表5－2 による。
section_syntax_indicator（セクションシンタクス指示）：これは1 ビットの指示で「0」
に設定する。
section_length（セクション長）：これは12 ビットのフィールドで、この値は常に「0x005」
とする。これは、セクション長フィールドの直後からセクションの最後までのセクション
のバイト数を規定する。
JST_time（現在日付、現在時刻）：この40 ビットのフィールドは、日本標準時（JST）
と修正ユリウス日（MJD）（付録C 参照）による現在日付と現在時刻を含む。このフィー
ルドは、MJD の下位16 ビットを16 ビットで符号化し、続く24 ビットを6 個の4 ビット
2 進化10 進数（BCD）で符号化する。
例：93/10/13 12:45:00 は0xC079124500 と符号化される。
［注］MJD のフィールドは16 ビットのため、現在日付は2038 年4 月22 日まで表現可能。

*/

#include "../../../Common/Util.h"
#include "PSITable.h"

class CTDTTable : public CPSITable
{
public:
	SYSTEMTIME jst_time;
public:
	CTDTTable(void);
	~CTDTTable(void);

	BOOL Decode( BYTE* data, DWORD dataSize, DWORD* decodeReadSize );
};
