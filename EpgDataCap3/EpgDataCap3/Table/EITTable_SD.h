#pragma once

/*
5.2.7 イベント情報テーブル（EIT）（Event Information Table）
EIT（表5－7 参照）は、各サービスに含まれるイベントに関する時系列情報である。EIT
は4 クラスに分けられ、テーブル識別（表5－2 参照）で区別される。
1) 自トランスポートストリーム、現在／次のイベント情報＝テーブル識別＝「0x4E」
2) 他トランスポートストリーム、現在／次のイベント情報＝テーブル識別＝「0x4F」
3) 自トランスポートストリーム、イベントのスケジュール情報
＝テーブル識別＝「0x50～0x5F」
4) 他トランスポートストリーム、イベントのスケジュール情報
＝テーブル識別＝「0x60～0x6F」
イベント［現在／次］テーブルは、自トランスポートストリームあるいは他トランスポ
ートストリーム上の与えられたサービスによって伝送される、現在のイベントおよび時間
的にその次のイベントに関係した情報を含む。ただし、イベント記述子が2 つ以上存在す
る場合があるNVOD 基準サービスの場合はこの限りではない。また、次のイベントより後
のイベントの情報を任意に含ませることもできる。自トランスポートストリームあるいは
他トランスポートストリームのイベント［スケジュール］テーブルは、スケジュール形式
でイベントの表、すなわち次のイベントより後のイベントを含んでいる。EIT［スケジュ
ール］テーブルの伝送は任意である。イベント情報は時系列に並べなければならない。
EIT は表5－7 に示すデータ構造により、イベント情報セクションに分割される。EIT の
一部分となるセクションは全て、PID 値0x0012 のトランスポートストリームパケットに
より伝送される。

event_information_section(){
	table_id						8 uimsbf
	section_syntax_indicator		1 bslbf
	reserved_future_use				1 bslbf
	reserved						2 bslbf
	section_length					12
	service_id						16 uimsbf
	reserved						2 bslbf
	version_number					5 uimsbf
	current_next_indicator			1 bslbf
	section_number					8 uimsbf
	last_section_number				8 uimsbf
	transport_stream_id				16 uimsbf
	original_network_id				16 uimsbf
	segment_last_section_number		8 uimsbf
	last_table_id					8 uimsbf
	for(i=0;i<N;i++){
		event_id						16 uimsbf
		start_time						40 bslbf
		duration						24 uimsbf
		running_status					3 uimsbf
		free_CA_mode					1 bslbf
		descriptors_loop_length			12 uimsbf
		for(i=0;i<N;i++){
			descriptor()
		}
	}
	CRC_32							32 rpchof
}

table_id（テーブル識別）：表5－2 による。
section_syntax_indicator（セクションシンタクス指示）：セクションシンタクス指示は1
ビットのフィールドで、常に1 とする。
section_length（セクション長）：これは12 ビットのフィールドである。これは、セクシ
ョン長フィールドの直後からCRC を含むセクションの最後までのセクションのバイト数
を規定する。全セクションの長さが4096 バイトを超えないようにするため、セクション長
は4093 を超えてはならない。
service_id（サービス識別）：これは16 ビットのフィールドで、そのトランスポートスト
リーム内の他のサービスからこのサービスを識別するためのラベルの役割をする。サービ
ス識別は、対応するプログラムマップセクション内の放送番組番号識別
（program_number）に等しい。
version_number（バージョン番号）：この5 ビットのフィールドはサブテーブルのバージ
ョン番号である。バージョン番号はサブテーブル内の情報に変化があった場合に1 加算さ
れる。その値が31 になった場合は、その次は0 に戻る。カレントネクスト指示が「1」の
場合は、バージョン番号はテーブル識別とサービス識別で定義される現在有効なサブテー
ブルのバージョン番号になる。カレントネクスト指示が「0」の場合は、バージョン番号は
テーブル識別とサービス識別で定義される次に有効なサブテーブルのバージョン番号にな
る。
current_next_indicator（カレントネクスト指示）：この1 ビットの指示は、それが「1」
の場合はサブテーブルが現在のサブテーブルであることを示す。「0」の場合は、送られる
サブテーブルはまだ適用されず、次のサブテーブルとして使用されることを示す。
section_number（セクション番号）：この8 ビットのフィールドはセクションの番号を表
す。サブテーブル中の最初のセクションのセクション番号は、0x00 である。セクション番
号は、同一のテーブル識別、サービス識別、トランスポートストリーム識別、オリジナル
ネットワーク識別を持つセクションの追加ごとに1 加算される。この場合、サブテーブル
はいくつかのセグメントとして構築してもよい。各セグメント内では、セクションの追加
ごとにセクション番号を1 加算させるが、セグメントの最終セクションと隣接するセグメ
ントの最初のセクションとの間では、番号の隙間があってもよい。
last_section_number（最終セクション番号）：この8 ビットのフィールドは、そのセクシ
ョンが属するサブテーブルの最後のセクション（すなわち、最大のセクション番号を持つ
セクション）の番号を規定する。
transport_stream_id（トランスポートストリーム識別）：これは16 ビットのフィールド
で、EIT が示すこのトランスポートストリームをその分配システム内の他の多重から識別
するラベルの役割をする。
original_network_id（オリジナルネットワーク識別）：この16 ビットのフィールドは、
元の分配システムのネットワーク識別を規定するラベルの役割をする。
segment_last_section_number（セグメント最終セクション番号）：この8 ビットのフィ
ールドは、サブテーブルのこのセグメントの最後のセクションの番号を規定する。分割さ
れないサブテーブルでは、このフィールドは最終セクション番号（last_section_number）
フィールドと同一の値に設定しなければならない。
last_table_id（最終テーブル識別）：この8 ビットのフィールドは、使用されている最終
のテーブル識別を示す。使用されるテーブルが1 個のみの場合は、このフィールドにはこ
のテーブルのテーブル識別を設定する。連続したテーブル識別値に渡って情報は時系列順
でなければならない。
event_id（イベント識別）：この16 ビットのフィールドは、記載されているイベントの識
別番号（1 サービス内で一意的に割り当てられる）を示す。
start_time（開始時間）：この40 ビットのフィールドは、イベントの開始時間を日本標準
時（JST）と修正ユリウス日（MJD）（付録C 参照）で示す。このフィールドは、MJD
の下位16 ビットを16 ビットで符号化し、続く24 ビットを6 個の4 ビット2 進化10 進数
（BCD）で符号化する。開始時間が定義されない場合（例えばNVOD 基準サービスなど）
には、このフィールドの全てのビットは「1」に設定する。
例１：93/10/13 12:45:00 は「0xC079124500」と符号化される。
duration（継続時間）：24 ビットのフィールドで、イベントの継続時間を時、分、秒で表
す。継続時間が定義されない場合（例えば緊急ニュースで終了時刻が未定など）には、こ
のフィールドのすべてのビットを「1」に設定する。
形式：6 個の4 ビットBCD 符号 = 24 ビット
例2：01:45:30 は「0x014530」と符号化される。
running_status（進行状態）：この3 ビットフィールドは、表5－6 に定義されるイベン
トの状態を示す。
free_CA_mode（スクランブル）：この1 ビットのフィールドは、それが「0」の場合はそ
のイベント内の全てのコンポーネントストリームがスクランブルされていないことを示す。
「1」の場合は、一つ以上のストリームへのアクセスがCA システムにより制御されている
ことを示す。
descriptors_loop_length（記述子ループ長）：これは、12 ビットのフィールドで、後続の
記述子の全バイト長を規定する。
CRC_32（CRC）：これは付録B で定義するデコーダにおいて、セクション全体を処理し
た後にレジスタ出力がゼロになるようなCRC 値を含む、32 ビットのフィールドである。

*/

#include "../../../Common/Util.h"
#include "../AribDescriptor.h"
#include "PSITable.h"

class CEITTable_SD : public CPSITable
{
public:
	typedef struct _EVENT_INFO_DATA{
		WORD event_id;
		BYTE StartTimeFlag;	//start_timeの値が有効かどうか
		SYSTEMTIME start_time;
		BYTE DurationFlag; //durationの値が有効かどうか
		WORD durationHH;
		WORD durationMM;
		WORD durationSS;
		BYTE running_status;
		BYTE free_CA_mode;
		WORD descriptors_loop_length;
		vector<AribDescriptor::CDescriptor*> descriptorList;
		~_EVENT_INFO_DATA(void){
			for( size_t i=0; i<descriptorList.size(); i++ ){
				SAFE_DELETE(descriptorList[i]);
			}
			descriptorList.clear();
		};
	} EVENT_INFO_DATA;
	WORD service_id;
	BYTE version_number;
	BYTE current_next_indicator;
	BYTE section_number;
	BYTE last_section_number;
	WORD transport_stream_id;
	WORD original_network_id;
	vector<EVENT_INFO_DATA*> eventInfoList;

public:
	CEITTable_SD(void);
	~CEITTable_SD(void);

	BOOL Decode( BYTE* data, DWORD dataSize, DWORD* decodeReadSize );
protected:
	void Clear();

	BOOL SDDecode( BYTE* data, DWORD dataSize, vector<AribDescriptor::CDescriptor*>* descriptorList, DWORD* decodeReadSize );

};
