#pragma once

/*
5.2.6 サービス記述テーブル（SDT）（Service Description Table）
SDT（表5－5 参照）の各サブテーブルは、特定のトランスポートストリームに含まれる
サービスを表す。サービスは自トランスポートストリームの一部であるか、他トランスポ
ートストリームの一部であるかのいずれかであり、これらは、テーブル識別（表5－2 参照）
で識別される。
SDT は表5－5 のデータ構造を使用して、サービス記述セクションに分割される。SDT
の一部分となるセクションは全て、PID 値0x0011 のトランスポートストリームパケット
により伝送される。自トランスポートストリーム（つまりそのSDT を含むトランスポート
ストリーム）の内容を記述するSDT のセクションは全てテーブル識別値は0x42 とし、ま
た、自トランスポートストリーム以外のトランスポートストリームのSDT セクションのテ
ーブル識別値は0x46 とする。

service description section(){
	table_id						8 uimsbf
	section_syntax_indicator		1 bslbf
	reserved_future_use				1 bslbf
	reserved						2 bslbf
	section_length					12 uimsbf
	transport_stream_id				16 uimsbf
	reserved						2 bslbf
	version_number					5 uimsbf
	current_next_indicator			1 bslbf
	section_number					8 uimsbf
	last_section_number				8 uimsbf
	original_network_id				16 uimsbf
	reserved_future_use				8 bslbf
	for (i=0;i<N;i++){
		service_id						16 uimsbf
		reserved_future_use				3 bslbf
		EIT_user_defined_flags			3 bslbf
		EIT_schedule_flag				1 bslbf
		EIT_present_following_flag		1 bslbf
		running_status					3 uimsbf
		free_CA_mode					1 bslbf
		descriptors_loop_length			12 uimsbf
		for (j=0;j<N;j++){
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
transport_stream_id（トランスポートストリーム識別）：これは16 ビットのフィールド
で、SDT が示すトランスポートストリームをその分配システム内の他の多重から識別する
ラベルの役割をする。
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
号は、同一のテーブル識別、トランスポートストリーム識別、オリジナルネットワーク識
別を持つセクションの追加ごとに1 加算される。
last_section_number（最終セクション番号）：この8 ビットのフィールドは、そのセクシ
ョンが属するサブテーブルの最後のセクション（すなわち、最大のセクション番号を持つ
セクション）の番号を規定する。
original_network_id（オリジナルネットワーク識別）：この16 ビットのフィールドは、
元の分配システムのネットワーク識別を規定するラベルの役割をする。
service_id（サービス識別）：これは16 ビットのフィールドで、そのトランスポートスト
リーム内の他のサービスからこのサービスを識別するためのラベルの役割をする。サービ
ス識別は、対応するプログラムマップセクション内の放送番組番号識別
（program_number）に等しい。
EIT_user_defined_flags（EIT 事業者定義フラグ）：この３ビットのフィールドは、EIT
の送出有無を示す拡張として、放送事業者が独自に定義することができる。使用しない場
合は、’111’とする。
EIT_schedule_flag（EIT［スケジュール］フラグ）：この1 ビットのフィールドは、それ
が「1」の場合はそのサービスのEIT［スケジュール］情報が現在のトランスポートストリ
ーム内にあることを示す。（EIT［スケジュール］サブテーブルの最大送出間隔について
は「ガイドライン」を参照。）このフラグが「0」の場合は、そのサービスのEIT［スケジ
ュール］情報が現在のトランスポートストリーム内には存在しないことを表す。
EIT_present_following_flag（EIT［現在／次］フラグ）：この1 ビットのフィールドは、
それが「1」の場合はそのサービスのEIT［現在／次］情報が現在のトランスポートストリ
ーム内にあることを示す。（EIT［現在／次］サブテーブルの最大送出間隔については「ガ
イドライン」を参照。）このフラグが「0」の場合は、そのサービスのEIT［現在／次］情
報が現在のトランスポートストリーム内には存在しないことを表す。
running_status（進行状態）：この3 ビットフィールドは、表5－6 に定義されるサービ
スの状態を示す。
free_CA_mode（スクランブル）：この1 ビットのフィールドは、それが「0」の場合はそ
のサービス内の全てのコンポーネントストリームがスクランブルされていないことを示す。
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

class CSDTTable : public CPSITable
{
public:
	typedef struct _SERVICE_INFO_DATA{
		WORD service_id;
		BYTE EIT_user_defined_flags;
		BYTE EIT_schedule_flag;
		BYTE EIT_present_following_flag;
		BYTE running_status;
		BYTE free_CA_mode;
		WORD descriptors_loop_length;
		vector<AribDescriptor::CDescriptor*> descriptorList;
		~_SERVICE_INFO_DATA(void){
			for( size_t i=0; i<descriptorList.size(); i++ ){
				SAFE_DELETE(descriptorList[i]);
			}
			descriptorList.clear();
		};
	} SERVICE_INFO_DATA;
	WORD transport_stream_id;
	BYTE version_number;
	BYTE current_next_indicator;
	BYTE section_number;
	BYTE last_section_number;
	WORD original_network_id;
	vector<SERVICE_INFO_DATA*> serviceInfoList;

public:
	CSDTTable(void);
	~CSDTTable(void);

	BOOL Decode( BYTE* data, DWORD dataSize, DWORD* decodeReadSize );
protected:
	void Clear();

	BOOL SDDecode( BYTE* data, DWORD dataSize, vector<AribDescriptor::CDescriptor*>* descriptorList, DWORD* decodeReadSize );
};
