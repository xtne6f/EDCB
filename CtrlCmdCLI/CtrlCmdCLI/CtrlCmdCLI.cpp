// これは メイン DLL ファイルです。

#include "stdafx.h"

#include "CtrlCmdCLI.h"
#include "../../Common/ErrDef.h"

#include <vcclr.h>
#include <string>

using namespace CtrlCmdCLI;

CtrlCmdUtil::CtrlCmdUtil(void)
{
	this->sendCmd = new CSendCtrlCmd;
}

CtrlCmdUtil::~CtrlCmdUtil(void)
{
	if( this->sendCmd != NULL ){
		delete this->sendCmd;
		this->sendCmd = NULL;
	}
}

CtrlCmdUtil::!CtrlCmdUtil(void)
{
	if( this->sendCmd != NULL ){
		delete this->sendCmd;
		this->sendCmd = NULL;
	}
}

void CtrlCmdUtil::GetContentKindList(
	Dictionary<UInt16, Def::ContentKind^>^% list
	)
{
	list->Add(0x00FF, gcnew Def::ContentKind("ニュース／報道", "", 0x00, 0xFF));
    list->Add(0x0000, gcnew Def::ContentKind("ニュース／報道", "定時・総合", 0x00, 0x00));
    list->Add(0x0001, gcnew Def::ContentKind("ニュース／報道", "天気", 0x00, 0x01));
    list->Add(0x0002, gcnew Def::ContentKind("ニュース／報道", "特集・ドキュメント", 0x00, 0x02));
    list->Add(0x0003, gcnew Def::ContentKind("ニュース／報道", "政治・国会", 0x00, 0x03));
    list->Add(0x0004, gcnew Def::ContentKind("ニュース／報道", "経済・市況", 0x00, 0x04));
    list->Add(0x0005, gcnew Def::ContentKind("ニュース／報道", "海外・国際", 0x00, 0x05));
    list->Add(0x0006, gcnew Def::ContentKind("ニュース／報道", "解説", 0x00, 0x06));
    list->Add(0x0007, gcnew Def::ContentKind("ニュース／報道", "討論・会談", 0x00, 0x07));
    list->Add(0x0008, gcnew Def::ContentKind("ニュース／報道", "報道特番", 0x00, 0x08));
    list->Add(0x0009, gcnew Def::ContentKind("ニュース／報道", "ローカル・地域", 0x00, 0x09));
    list->Add(0x000A, gcnew Def::ContentKind("ニュース／報道", "交通", 0x00, 0x0A));
    list->Add(0x000F, gcnew Def::ContentKind("ニュース／報道", "その他", 0x00, 0x0F));

    list->Add(0x01FF, gcnew Def::ContentKind("スポーツ", "", 0x01, 0xFF));
    list->Add(0x0100, gcnew Def::ContentKind("スポーツ", "スポーツニュース", 0x01, 0x00));
    list->Add(0x0101, gcnew Def::ContentKind("スポーツ", "野球", 0x01, 0x01));
    list->Add(0x0102, gcnew Def::ContentKind("スポーツ", "サッカー", 0x01, 0x02));
    list->Add(0x0103, gcnew Def::ContentKind("スポーツ", "ゴルフ", 0x01, 0x03));
    list->Add(0x0104, gcnew Def::ContentKind("スポーツ", "その他の球技", 0x01, 0x04));
    list->Add(0x0105, gcnew Def::ContentKind("スポーツ", "相撲・格闘技", 0x01, 0x05));
    list->Add(0x0106, gcnew Def::ContentKind("スポーツ", "オリンピック・国際大会", 0x01, 0x06));
    list->Add(0x0107, gcnew Def::ContentKind("スポーツ", "マラソン・陸上・水泳", 0x01, 0x07));
    list->Add(0x0108, gcnew Def::ContentKind("スポーツ", "モータースポーツ", 0x01, 0x08));
    list->Add(0x0108, gcnew Def::ContentKind("スポーツ", "マリン・ウィンタースポーツ", 0x01, 0x09));
    list->Add(0x0109, gcnew Def::ContentKind("スポーツ", "競馬・公営競技", 0x01, 0x0A));
    list->Add(0x010F, gcnew Def::ContentKind("スポーツ", "その他", 0x01, 0x0F));

    list->Add(0x02FF, gcnew Def::ContentKind("情報／ワイドショー", "", 0x02, 0xFF));
    list->Add(0x0200, gcnew Def::ContentKind("情報／ワイドショー", "芸能・ワイドショー", 0x02, 0x00));
    list->Add(0x0201, gcnew Def::ContentKind("情報／ワイドショー", "ファッション", 0x02, 0x01));
    list->Add(0x0202, gcnew Def::ContentKind("情報／ワイドショー", "暮らし・住まい", 0x02, 0x02));
    list->Add(0x0203, gcnew Def::ContentKind("情報／ワイドショー", "健康・医療", 0x02, 0x03));
    list->Add(0x0204, gcnew Def::ContentKind("情報／ワイドショー", "ショッピング・通販", 0x02, 0x04));
    list->Add(0x0205, gcnew Def::ContentKind("情報／ワイドショー", "グルメ・料理", 0x02, 0x05));
    list->Add(0x0206, gcnew Def::ContentKind("情報／ワイドショー", "イベント", 0x02, 0x06));
    list->Add(0x0207, gcnew Def::ContentKind("情報／ワイドショー", "番組紹介・お知らせ", 0x02, 0x07));
    list->Add(0x020F, gcnew Def::ContentKind("情報／ワイドショー", "その他", 0x02, 0x0F));
                
    list->Add(0x03FF, gcnew Def::ContentKind("ドラマ", "", 0x03, 0xFF));
    list->Add(0x0300, gcnew Def::ContentKind("ドラマ", "国内ドラマ", 0x03, 0x00));
    list->Add(0x0301, gcnew Def::ContentKind("ドラマ", "海外ドラマ", 0x03, 0x01));
    list->Add(0x0302, gcnew Def::ContentKind("ドラマ", "時代劇", 0x03, 0x02));
    list->Add(0x030F, gcnew Def::ContentKind("ドラマ", "その他", 0x03, 0x0F));

    list->Add(0x04FF, gcnew Def::ContentKind("音楽", "", 0x04, 0xFF));
    list->Add(0x0400, gcnew Def::ContentKind("音楽", "国内ロック・ポップス", 0x04, 0x00));
    list->Add(0x0401, gcnew Def::ContentKind("音楽", "海外ロック・ポップス", 0x04, 0x01));
    list->Add(0x0402, gcnew Def::ContentKind("音楽", "クラシック・オペラ", 0x04, 0x02));
    list->Add(0x0403, gcnew Def::ContentKind("音楽", "ジャズ・フュージョン", 0x04, 0x03));
    list->Add(0x0404, gcnew Def::ContentKind("音楽", "歌謡曲・演歌", 0x04, 0x04));
    list->Add(0x0405, gcnew Def::ContentKind("音楽", "ライブ・コンサート", 0x04, 0x05));
    list->Add(0x0406, gcnew Def::ContentKind("音楽", "ランキング・リクエスト", 0x04, 0x06));
    list->Add(0x0407, gcnew Def::ContentKind("音楽", "カラオケ・のど自慢", 0x04, 0x07));
    list->Add(0x0408, gcnew Def::ContentKind("音楽", "民謡・邦楽", 0x04, 0x08));
    list->Add(0x0409, gcnew Def::ContentKind("音楽", "童謡・キッズ", 0x04, 0x09));
    list->Add(0x040A, gcnew Def::ContentKind("音楽", "民族音楽・ワールドミュージック", 0x04, 0x0A));
    list->Add(0x040F, gcnew Def::ContentKind("音楽", "その他", 0x04, 0x0F));

    list->Add(0x05FF, gcnew Def::ContentKind("バラエティ", "", 0x05, 0xFF));
    list->Add(0x0500, gcnew Def::ContentKind("バラエティ", "クイズ", 0x05, 0x00));
    list->Add(0x0501, gcnew Def::ContentKind("バラエティ", "ゲーム", 0x05, 0x01));
    list->Add(0x0502, gcnew Def::ContentKind("バラエティ", "トークバラエティ", 0x05, 0x02));
    list->Add(0x0503, gcnew Def::ContentKind("バラエティ", "お笑い・コメディ", 0x05, 0x03));
    list->Add(0x0504, gcnew Def::ContentKind("バラエティ", "音楽バラエティ", 0x05, 0x04));
    list->Add(0x0505, gcnew Def::ContentKind("バラエティ", "旅バラエティ", 0x05, 0x05));
    list->Add(0x0506, gcnew Def::ContentKind("バラエティ", "料理バラエティ", 0x05, 0x06));
    list->Add(0x050F, gcnew Def::ContentKind("バラエティ", "その他", 0x05, 0x0F));

    list->Add(0x06FF, gcnew Def::ContentKind("映画", "", 0x06, 0xFF));
    list->Add(0x0600, gcnew Def::ContentKind("映画", "洋画", 0x06, 0x00));
    list->Add(0x0601, gcnew Def::ContentKind("映画", "邦画", 0x06, 0x01));
    list->Add(0x0602, gcnew Def::ContentKind("映画", "アニメ", 0x06, 0x02));
    list->Add(0x060F, gcnew Def::ContentKind("映画", "その他", 0x06, 0x0F));

    list->Add(0x07FF, gcnew Def::ContentKind("アニメ／特撮", "", 0x07, 0xFF));
    list->Add(0x0700, gcnew Def::ContentKind("アニメ／特撮", "国内アニメ", 0x07, 0x00));
    list->Add(0x0701, gcnew Def::ContentKind("アニメ／特撮", "海外アニメ", 0x07, 0x01));
    list->Add(0x0702, gcnew Def::ContentKind("アニメ／特撮", "特撮", 0x07, 0x02));
    list->Add(0x070F, gcnew Def::ContentKind("アニメ／特撮", "その他", 0x07, 0x0F));

    list->Add(0x08FF, gcnew Def::ContentKind("ドキュメンタリー／教養", "", 0x08, 0xFF));
    list->Add(0x0800, gcnew Def::ContentKind("ドキュメンタリー／教養", "社会・時事", 0x08, 0x00));
    list->Add(0x0801, gcnew Def::ContentKind("ドキュメンタリー／教養", "歴史・紀行", 0x08, 0x01));
    list->Add(0x0802, gcnew Def::ContentKind("ドキュメンタリー／教養", "自然・動物・環境", 0x08, 0x02));
    list->Add(0x0803, gcnew Def::ContentKind("ドキュメンタリー／教養", "宇宙・科学・医学", 0x08, 0x03));
    list->Add(0x0804, gcnew Def::ContentKind("ドキュメンタリー／教養", "カルチャー・伝統文化", 0x08, 0x04));
    list->Add(0x0805, gcnew Def::ContentKind("ドキュメンタリー／教養", "文学・文芸", 0x08, 0x05));
    list->Add(0x0806, gcnew Def::ContentKind("ドキュメンタリー／教養", "スポーツ", 0x08, 0x06));
    list->Add(0x0807, gcnew Def::ContentKind("ドキュメンタリー／教養", "ドキュメンタリー全般", 0x08, 0x07));
    list->Add(0x0808, gcnew Def::ContentKind("ドキュメンタリー／教養", "インタビュー・討論", 0x08, 0x08));
    list->Add(0x080F, gcnew Def::ContentKind("ドキュメンタリー／教養", "その他", 0x08, 0x0F));

    list->Add(0x09FF, gcnew Def::ContentKind("劇場／公演", "", 0x09, 0xFF));
    list->Add(0x0900, gcnew Def::ContentKind("劇場／公演", "現代劇・新劇", 0x09, 0x00));
    list->Add(0x0901, gcnew Def::ContentKind("劇場／公演", "ミュージカル", 0x09, 0x01));
    list->Add(0x0902, gcnew Def::ContentKind("劇場／公演", "ダンス・バレエ", 0x09, 0x02));
    list->Add(0x0903, gcnew Def::ContentKind("劇場／公演", "落語・演芸", 0x09, 0x03));
    list->Add(0x0904, gcnew Def::ContentKind("劇場／公演", "歌舞伎・古典", 0x09, 0x04));
    list->Add(0x090F, gcnew Def::ContentKind("劇場／公演", "その他", 0x09, 0x0F));

    list->Add(0x0AFF, gcnew Def::ContentKind("趣味／教育", "", 0x0A, 0xFF));
    list->Add(0x0A00, gcnew Def::ContentKind("趣味／教育", "旅・釣り・アウトドア", 0x0A, 0x00));
    list->Add(0x0A01, gcnew Def::ContentKind("趣味／教育", "園芸・ペット・手芸", 0x0A, 0x01));
    list->Add(0x0A02, gcnew Def::ContentKind("趣味／教育", "音楽・美術・工芸", 0x0A, 0x02));
    list->Add(0x0A03, gcnew Def::ContentKind("趣味／教育", "囲碁・将棋", 0x0A, 0x03));
    list->Add(0x0A04, gcnew Def::ContentKind("趣味／教育", "麻雀・パチンコ", 0x0A, 0x04));
    list->Add(0x0A05, gcnew Def::ContentKind("趣味／教育", "車・オートバイ", 0x0A, 0x05));
    list->Add(0x0A06, gcnew Def::ContentKind("趣味／教育", "コンピュータ・ＴＶゲーム", 0x0A, 0x06));
    list->Add(0x0A07, gcnew Def::ContentKind("趣味／教育", "会話・語学", 0x0A, 0x07));
    list->Add(0x0A08, gcnew Def::ContentKind("趣味／教育", "幼児・小学生", 0x0A, 0x08));
    list->Add(0x0A09, gcnew Def::ContentKind("趣味／教育", "中学生・高校生", 0x0A, 0x09));
    list->Add(0x0A0A, gcnew Def::ContentKind("趣味／教育", "大学生・受験", 0x0A, 0x0A));
    list->Add(0x0A0B, gcnew Def::ContentKind("趣味／教育", "生涯教育・資格", 0x0A, 0x0B));
    list->Add(0x0A0C, gcnew Def::ContentKind("趣味／教育", "教育問題", 0x0A, 0x0C));
    list->Add(0x0A0F, gcnew Def::ContentKind("趣味／教育", "その他", 0x0A, 0x0F));

    list->Add(0x0BFF, gcnew Def::ContentKind("福祉", "", 0x0B, 0xFF));
    list->Add(0x0B00, gcnew Def::ContentKind("福祉", "高齢者", 0x0B, 0x00));
    list->Add(0x0B01, gcnew Def::ContentKind("福祉", "障害者", 0x0B, 0x01));
    list->Add(0x0B02, gcnew Def::ContentKind("福祉", "社会福祉", 0x0B, 0x02));
    list->Add(0x0B03, gcnew Def::ContentKind("福祉", "ボランティア", 0x0B, 0x03));
    list->Add(0x0B04, gcnew Def::ContentKind("福祉", "手話", 0x0B, 0x04));
    list->Add(0x0B05, gcnew Def::ContentKind("福祉", "文字（字幕）", 0x0B, 0x05));
    list->Add(0x0B06, gcnew Def::ContentKind("福祉", "音声解説", 0x0B, 0x06));
    list->Add(0x0B0F, gcnew Def::ContentKind("福祉", "その他", 0x0B, 0x0F));

    list->Add(0x0FFF, gcnew Def::ContentKind("その他", "", 0x0F, 0xFF));
    list->Add(0xFFFF, gcnew Def::ContentKind("なし", "", 0xFF, 0xFF));

}
void CtrlCmdUtil::GetComponentKindList(
	Dictionary<UInt16, Def::ComponentKind^>^% list
	)
{
    list->Add(0x0101, gcnew Def::ComponentKind(0x01, 0x01, "480i(525i)、アスペクト比4:3"));
    list->Add(0x0102, gcnew Def::ComponentKind(0x01, 0x02, "480i(525i)、アスペクト比16:9 パンベクトルあり"));
    list->Add(0x0103, gcnew Def::ComponentKind(0x01, 0x03, "480i(525i)、アスペクト比16:9 パンベクトルなし"));
    list->Add(0x0104, gcnew Def::ComponentKind(0x01, 0x04, "480i(525i)、アスペクト比 > 16:9"));
    list->Add(0x0191, gcnew Def::ComponentKind(0x01, 0x91, "2160p、アスペクト比4:3"));
    list->Add(0x0192, gcnew Def::ComponentKind(0x01, 0x92, "2160p、アスペクト比16:9 パンベクトルあり"));
    list->Add(0x0193, gcnew Def::ComponentKind(0x01, 0x93, "2160p、アスペクト比16:9 パンベクトルなし"));
    list->Add(0x0194, gcnew Def::ComponentKind(0x01, 0x94, "2160p、アスペクト比 > 16:9"));
    list->Add(0x01A1, gcnew Def::ComponentKind(0x01, 0xA1, "480p(525p)、アスペクト比4:3"));
    list->Add(0x01A2, gcnew Def::ComponentKind(0x01, 0xA2, "480p(525p)、アスペクト比16:9 パンベクトルあり"));
    list->Add(0x01A3, gcnew Def::ComponentKind(0x01, 0xA3, "480p(525p)、アスペクト比16:9 パンベクトルなし"));
    list->Add(0x01A4, gcnew Def::ComponentKind(0x01, 0xA4, "480p(525p)、アスペクト比 > 16:9"));
    list->Add(0x01B1, gcnew Def::ComponentKind(0x01, 0xB1, "1080i(1125i)、アスペクト比4:3"));
    list->Add(0x01B2, gcnew Def::ComponentKind(0x01, 0xB2, "1080i(1125i)、アスペクト比16:9 パンベクトルあり"));
    list->Add(0x01B3, gcnew Def::ComponentKind(0x01, 0xB3, "1080i(1125i)、アスペクト比16:9 パンベクトルなし"));
    list->Add(0x01B4, gcnew Def::ComponentKind(0x01, 0xB4, "1080i(1125i)、アスペクト比 > 16:9"));
    list->Add(0x01C1, gcnew Def::ComponentKind(0x01, 0xC1, "720p(750p)、アスペクト比4:3"));
    list->Add(0x01C2, gcnew Def::ComponentKind(0x01, 0xC2, "720p(750p)、アスペクト比16:9 パンベクトルあり"));
    list->Add(0x01C3, gcnew Def::ComponentKind(0x01, 0xC3, "720p(750p)、アスペクト比16:9 パンベクトルなし"));
    list->Add(0x01C4, gcnew Def::ComponentKind(0x01, 0xC4, "720p(750p)、アスペクト比 > 16:9"));
    list->Add(0x01D1, gcnew Def::ComponentKind(0x01, 0xD1, "240p アスペクト比4:3"));
    list->Add(0x01D2, gcnew Def::ComponentKind(0x01, 0xD2, "240p アスペクト比16:9 パンベクトルあり"));
    list->Add(0x01D3, gcnew Def::ComponentKind(0x01, 0xD3, "240p アスペクト比16:9 パンベクトルなし"));
    list->Add(0x01D4, gcnew Def::ComponentKind(0x01, 0xD4, "240p アスペクト比 > 16:9"));
    list->Add(0x01E1, gcnew Def::ComponentKind(0x01, 0xE1, "1080p(1125p)、アスペクト比4:3"));
    list->Add(0x01E2, gcnew Def::ComponentKind(0x01, 0xE2, "1080p(1125p)、アスペクト比16:9 パンベクトルあり"));
    list->Add(0x01E3, gcnew Def::ComponentKind(0x01, 0xE3, "1080p(1125p)、アスペクト比16:9 パンベクトルなし"));
    list->Add(0x01E4, gcnew Def::ComponentKind(0x01, 0xE4, "1080p(1125p)、アスペクト比 > 16:9"));
    list->Add(0x0201, gcnew Def::ComponentKind(0x02, 0x01, "1/0モード（シングルモノ）"));
    list->Add(0x0202, gcnew Def::ComponentKind(0x02, 0x02, "1/0＋1/0モード（デュアルモノ）"));
    list->Add(0x0203, gcnew Def::ComponentKind(0x02, 0x03, "2/0モード（ステレオ）"));
    list->Add(0x0204, gcnew Def::ComponentKind(0x02, 0x04, "2/1モード"));
    list->Add(0x0205, gcnew Def::ComponentKind(0x02, 0x05, "3/0モード"));
    list->Add(0x0206, gcnew Def::ComponentKind(0x02, 0x06, "2/2モード"));
    list->Add(0x0207, gcnew Def::ComponentKind(0x02, 0x07, "3/1モード"));
    list->Add(0x0208, gcnew Def::ComponentKind(0x02, 0x08, "3/2モード"));
    list->Add(0x0209, gcnew Def::ComponentKind(0x02, 0x09, "3/2＋LFEモード（3/2.1モード）"));
    list->Add(0x020A, gcnew Def::ComponentKind(0x02, 0x0A, "3/3.1モード"));
    list->Add(0x020B, gcnew Def::ComponentKind(0x02, 0x0B, "2/0/0-2/0/2-0.1モード"));
    list->Add(0x020C, gcnew Def::ComponentKind(0x02, 0x0C, "5/2.1モード"));
    list->Add(0x020D, gcnew Def::ComponentKind(0x02, 0x0D, "3/2/2.1モード"));
    list->Add(0x020E, gcnew Def::ComponentKind(0x02, 0x0E, "2/0/0-3/0/2-0.1モード"));
    list->Add(0x020F, gcnew Def::ComponentKind(0x02, 0x0F, "0/2/0-3/0/2-0.1モード"));
    list->Add(0x0210, gcnew Def::ComponentKind(0x02, 0x10, "2/0/0-3/2/3-0.2モード"));
    list->Add(0x0211, gcnew Def::ComponentKind(0x02, 0x11, "3/3/3-5/2/3-3/0/0.2モード"));
    list->Add(0x0240, gcnew Def::ComponentKind(0x02, 0x40, "視覚障害者用音声解説"));
    list->Add(0x0241, gcnew Def::ComponentKind(0x02, 0x41, "聴覚障害者用音声"));
    list->Add(0x0501, gcnew Def::ComponentKind(0x05, 0x01, "H.264|MPEG-4 AVC、480i(525i)、アスペクト比4:3"));
    list->Add(0x0502, gcnew Def::ComponentKind(0x05, 0x02, "H.264|MPEG-4 AVC、480i(525i)、アスペクト比16:9 パンベクトルあり"));
    list->Add(0x0503, gcnew Def::ComponentKind(0x05, 0x03, "H.264|MPEG-4 AVC、480i(525i)、アスペクト比16:9 パンベクトルなし"));
    list->Add(0x0504, gcnew Def::ComponentKind(0x05, 0x04, "H.264|MPEG-4 AVC、480i(525i)、アスペクト比 > 16:9"));
    list->Add(0x0591, gcnew Def::ComponentKind(0x05, 0x91, "H.264|MPEG-4 AVC、2160p、アスペクト比4:3"));
    list->Add(0x0592, gcnew Def::ComponentKind(0x05, 0x92, "H.264|MPEG-4 AVC、2160p、アスペクト比16:9 パンベクトルあり"));
    list->Add(0x0593, gcnew Def::ComponentKind(0x05, 0x93, "H.264|MPEG-4 AVC、2160p、アスペクト比16:9 パンベクトルなし"));
    list->Add(0x0594, gcnew Def::ComponentKind(0x05, 0x94, "H.264|MPEG-4 AVC、2160p、アスペクト比 > 16:9"));
    list->Add(0x05A1, gcnew Def::ComponentKind(0x05, 0xA1, "H.264|MPEG-4 AVC、480p(525p)、アスペクト比4:3"));
    list->Add(0x05A2, gcnew Def::ComponentKind(0x05, 0xA2, "H.264|MPEG-4 AVC、480p(525p)、アスペクト比16:9 パンベクトルあり"));
    list->Add(0x05A3, gcnew Def::ComponentKind(0x05, 0xA3, "H.264|MPEG-4 AVC、480p(525p)、アスペクト比16:9 パンベクトルなし"));
    list->Add(0x05A4, gcnew Def::ComponentKind(0x05, 0xA4, "H.264|MPEG-4 AVC、480p(525p)、アスペクト比 > 16:9"));
    list->Add(0x05B1, gcnew Def::ComponentKind(0x05, 0xB1, "H.264|MPEG-4 AVC、1080i(1125i)、アスペクト比4:3"));
    list->Add(0x05B2, gcnew Def::ComponentKind(0x05, 0xB2, "H.264|MPEG-4 AVC、1080i(1125i)、アスペクト比16:9 パンベクトルあり"));
    list->Add(0x05B3, gcnew Def::ComponentKind(0x05, 0xB3, "H.264|MPEG-4 AVC、1080i(1125i)、アスペクト比16:9 パンベクトルなし"));
    list->Add(0x05B4, gcnew Def::ComponentKind(0x05, 0xB4, "H.264|MPEG-4 AVC、1080i(1125i)、アスペクト比 > 16:9"));
    list->Add(0x05C1, gcnew Def::ComponentKind(0x05, 0xC1, "H.264|MPEG-4 AVC、720p(750p)、アスペクト比4:3"));
    list->Add(0x05C2, gcnew Def::ComponentKind(0x05, 0xC2, "H.264|MPEG-4 AVC、720p(750p)、アスペクト比16:9 パンベクトルあり"));
    list->Add(0x05C3, gcnew Def::ComponentKind(0x05, 0xC3, "H.264|MPEG-4 AVC、720p(750p)、アスペクト比16:9 パンベクトルなし"));
    list->Add(0x05C4, gcnew Def::ComponentKind(0x05, 0xC4, "H.264|MPEG-4 AVC、720p(750p)、アスペクト比 > 16:9"));
    list->Add(0x05D1, gcnew Def::ComponentKind(0x05, 0xD1, "H.264|MPEG-4 AVC、240p アスペクト比4:3"));
    list->Add(0x05D2, gcnew Def::ComponentKind(0x05, 0xD2, "H.264|MPEG-4 AVC、240p アスペクト比16:9 パンベクトルあり"));
    list->Add(0x05D3, gcnew Def::ComponentKind(0x05, 0xD3, "H.264|MPEG-4 AVC、240p アスペクト比16:9 パンベクトルなし"));
    list->Add(0x05D4, gcnew Def::ComponentKind(0x05, 0xD4, "H.264|MPEG-4 AVC、240p アスペクト比 > 16:9"));
    list->Add(0x05E1, gcnew Def::ComponentKind(0x05, 0xE1, "H.264|MPEG-4 AVC、1080p(1125p)、アスペクト比4:3"));
    list->Add(0x05E2, gcnew Def::ComponentKind(0x05, 0xE2, "H.264|MPEG-4 AVC、1080p(1125p)、アスペクト比16:9 パンベクトルあり"));
    list->Add(0x05E3, gcnew Def::ComponentKind(0x05, 0xE3, "H.264|MPEG-4 AVC、1080p(1125p)、アスペクト比16:9 パンベクトルなし"));
    list->Add(0x05E4, gcnew Def::ComponentKind(0x05, 0xE4, "H.264|MPEG-4 AVC、1080p(1125p)、アスペクト比 > 16:9"));

}


void CtrlCmdUtil::CopyData(DateTime src, SYSTEMTIME* dest)
{
	__int64 i64Time = src.ToFileTime();
	FILETIME fTime;
	fTime.dwHighDateTime = (DWORD)(i64Time>>32);
	fTime.dwLowDateTime = (DWORD)(i64Time&0x00000000FFFFFFFF);
	if( i64Time <= 0 ){
		ZeroMemory(dest, sizeof(SYSTEMTIME));
	}else{
		FileTimeToSystemTime(&fTime,dest);
	}
}

DateTime CtrlCmdUtil::ConvertTime(SYSTEMTIME* src)
{
	FILETIME fTime;
	SystemTimeToFileTime( src, &fTime );
	__int64 i64Time = 0;
	i64Time = ((__int64)fTime.dwHighDateTime)<<32 | fTime.dwLowDateTime;
	if( i64Time <= 0 ){
		return DateTime::MinValue;
	}
	return DateTime::FromFileTimeUtc(i64Time);
}
/*
DateTime CtrlCmdUtil::CopyData(SYSTEMTIME* src, DateTime*& dest)
{
	FILETIME fTime;
	SystemTimeToFileTime( src, &fTime );
	__int64 i64Time = 0;
	i64Time = ((__int64)fTime.dwHighDateTime)<<32 | fTime.dwLowDateTime;

	dest = &DateTime::FromFileTimeUtc(i64Time);
	return DateTime::FromFileTimeUtc(i64Time);
}
*/
void CtrlCmdUtil::CopyData(Def::ReserveData^ src, RESERVE_DATA* dest)
{
	pin_ptr<const wchar_t> titlePin = PtrToStringChars(src->Title);
	pin_ptr<const wchar_t> stationNamePin = PtrToStringChars(src->StationName);
	pin_ptr<const wchar_t> commentPin = PtrToStringChars(src->Comment);

	dest->title = titlePin;
	CopyData(src->StartTime, &dest->startTime);
	dest->durationSecond = src->DurationSecond;
	dest->stationName = stationNamePin;
	dest->originalNetworkID = src->OriginalNetworkID;
	dest->transportStreamID = src->TransportStreamID;
	dest->serviceID = src->ServiceID;
	dest->eventID = src->EventID;
	dest->comment = commentPin;
	dest->reserveID = src->ReserveID;
	dest->overlapMode = src->OverlapMode;
	CopyData(src->StartTimeEpg, &dest->startTimeEpg);
	CopyData(src->RecSetting, &dest->recSetting);
	dest->reserveStatus = src->ReserveStatus;

	for( int i=0; i<src->RecFileNameList->Count; i++){
		pin_ptr<const wchar_t> namePin = PtrToStringChars(src->RecFileNameList[i]);

		dest->recFileNameList.push_back(namePin);
	}
}

void CtrlCmdUtil::CopyData(RESERVE_DATA* src, Def::ReserveData^% dest)
{
	dest->Title = gcnew String(src->title.c_str());
	dest->StartTime = ConvertTime( &src->startTime );
//	CopyData(&src->startTime, dest->StartTime);
	dest->DurationSecond = src->durationSecond;
	dest->StationName = gcnew String(src->stationName.c_str());
	dest->OriginalNetworkID = src->originalNetworkID;
	dest->TransportStreamID = src->transportStreamID;
	dest->ServiceID = src->serviceID;
	dest->EventID = src->eventID;
	dest->Comment = gcnew String(src->comment.c_str());
	dest->ReserveID = src->reserveID;
	dest->OverlapMode = src->overlapMode;
	dest->StartTimeEpg = ConvertTime( &src->startTimeEpg );
//	CopyData(&src->startTimeEpg, dest->StartTimeEpg);
	CopyData(&src->recSetting, dest->RecSetting);
	dest->ReserveStatus = src->reserveStatus;

	for( size_t i=0; i<src->recFileNameList.size(); i++){
		dest->RecFileNameList->Add( gcnew String(src->recFileNameList[i].c_str()) );
	}

}

void CtrlCmdUtil::CopyData(Def::RecSettingData^ src, REC_SETTING_DATA* dest)
{
	pin_ptr<const wchar_t> batFilePathPin = PtrToStringChars(src->BatFilePath);
	for( int i=0; i<src->RecFolderList->Count; i++){
		REC_FILE_SET_INFO item;
		pin_ptr<const wchar_t> recFolderPathPin = PtrToStringChars(src->RecFolderList[i]->RecFolder);
		pin_ptr<const wchar_t> writePlugInPin = PtrToStringChars(src->RecFolderList[i]->WritePlugIn);
		pin_ptr<const wchar_t> recNamePlugInPin = PtrToStringChars(src->RecFolderList[i]->RecNamePlugIn);
		pin_ptr<const wchar_t> recFileNamePin = PtrToStringChars(src->RecFolderList[i]->RecFileName);
		item.recFolder = recFolderPathPin;
		item.writePlugIn = writePlugInPin;
		item.recNamePlugIn = recNamePlugInPin;
		item.recFileName = recFileNamePin;

		dest->recFolderList.push_back(item);
	}
	for( int i=0; i<src->PartialRecFolder->Count; i++){
		REC_FILE_SET_INFO item;
		pin_ptr<const wchar_t> recFolderPathPin = PtrToStringChars(src->PartialRecFolder[i]->RecFolder);
		pin_ptr<const wchar_t> writePlugInPin = PtrToStringChars(src->PartialRecFolder[i]->WritePlugIn);
		pin_ptr<const wchar_t> recNamePlugInPin = PtrToStringChars(src->PartialRecFolder[i]->RecNamePlugIn);
		pin_ptr<const wchar_t> recFileNamePin = PtrToStringChars(src->PartialRecFolder[i]->RecFileName);
		item.recFolder = recFolderPathPin;
		item.writePlugIn = writePlugInPin;
		item.recNamePlugIn = recNamePlugInPin;
		item.recFileName = recFileNamePin;

		dest->partialRecFolder.push_back(item);
	}

	dest->recMode = src->RecMode;
	dest->priority = src->Priority;
	dest->tuijyuuFlag = src->TuijyuuFlag;
	dest->serviceMode = src->ServiceMode;
	dest->pittariFlag = src->PittariFlag;
	dest->batFilePath = batFilePathPin;
	dest->suspendMode = src->SuspendMode;
	dest->rebootFlag = src->RebootFlag;
	dest->useMargineFlag = src->UseMargineFlag;
	dest->startMargine = src->StartMargine;
	dest->endMargine = src->EndMargine;
	dest->continueRecFlag = src->ContinueRecFlag;
	dest->partialRecFlag = src->PartialRecFlag;
	dest->tunerID = src->TunerID;

}

void CtrlCmdUtil::CopyData(REC_SETTING_DATA* src, Def::RecSettingData^% dest)
{
	dest->RecMode = src->recMode;
	dest->Priority = src->priority;
	dest->TuijyuuFlag = src->tuijyuuFlag;
	dest->ServiceMode = src->serviceMode;
	dest->PittariFlag = src->pittariFlag;
	dest->BatFilePath = gcnew String(src->batFilePath.c_str());

	for( size_t i=0; i<src->recFolderList.size(); i++){
		Def::RecFileSetInfo^ item = gcnew Def::RecFileSetInfo();
		item->RecFolder = gcnew String(src->recFolderList[i].recFolder.c_str());
		item->WritePlugIn = gcnew String(src->recFolderList[i].writePlugIn.c_str());
		item->RecNamePlugIn = gcnew String(src->recFolderList[i].recNamePlugIn.c_str());
		item->RecFileName = gcnew String(src->recFolderList[i].recFileName.c_str());
		dest->RecFolderList->Add( item );
	}
	for( size_t i=0; i<src->partialRecFolder.size(); i++){
		Def::RecFileSetInfo^ item = gcnew Def::RecFileSetInfo();
		item->RecFolder = gcnew String(src->partialRecFolder[i].recFolder.c_str());
		item->WritePlugIn = gcnew String(src->partialRecFolder[i].writePlugIn.c_str());
		item->RecNamePlugIn = gcnew String(src->partialRecFolder[i].recNamePlugIn.c_str());
		item->RecFileName = gcnew String(src->partialRecFolder[i].recFileName.c_str());
		dest->PartialRecFolder->Add( item );
	}
	dest->SuspendMode = src->suspendMode;
	dest->RebootFlag = src->rebootFlag;
	dest->UseMargineFlag = src->useMargineFlag;
	dest->StartMargine = src->startMargine;
	dest->EndMargine = src->endMargine;
	dest->ContinueRecFlag = src->continueRecFlag;
	dest->PartialRecFlag = src->partialRecFlag;
	dest->TunerID = src->tunerID;
}

void CtrlCmdUtil::CopyData(Def::EpgShortEventInfo^ src, EPGDB_SHORT_EVENT_INFO* dest)
{
	pin_ptr<const wchar_t> event_namePin = PtrToStringChars(src->event_name);
	pin_ptr<const wchar_t> text_charPin = PtrToStringChars(src->text_char);

	dest->event_name = event_namePin;
	dest->text_char = text_charPin;
}

void CtrlCmdUtil::CopyData(EPGDB_SHORT_EVENT_INFO* src, Def::EpgShortEventInfo^% dest)
{
	dest->event_name = gcnew String(src->event_name.c_str());
	dest->text_char = gcnew String(src->text_char.c_str());

}

void CtrlCmdUtil::CopyData(Def::EpgExtendedEventInfo^ src, EPGDB_EXTENDED_EVENT_INFO* dest)
{
	pin_ptr<const wchar_t> text_charPin = PtrToStringChars(src->text_char);
	dest->text_char = text_charPin;
}

void CtrlCmdUtil::CopyData(EPGDB_EXTENDED_EVENT_INFO* src, Def::EpgExtendedEventInfo^% dest)
{
	dest->text_char = gcnew String(src->text_char.c_str());
}

void CtrlCmdUtil::CopyData(Def::EpgContentData^ src, EPGDB_CONTENT_DATA* dest)
{
	dest->content_nibble_level_1 = src->content_nibble_level_1;
	dest->content_nibble_level_2 = src->content_nibble_level_2;
	dest->user_nibble_1 = src->user_nibble_1;
	dest->user_nibble_2 = src->user_nibble_2;
}

void CtrlCmdUtil::CopyData(EPGDB_CONTENT_DATA* src, Def::EpgContentData^% dest)
{
	dest->content_nibble_level_1 = src->content_nibble_level_1;
	dest->content_nibble_level_2 = src->content_nibble_level_2;
	dest->user_nibble_1 = src->user_nibble_1;
	dest->user_nibble_2 = src->user_nibble_2;
}

void CtrlCmdUtil::CopyData(Def::EpgContentInfo^ src, EPGDB_CONTEN_INFO* dest)
{
	for( int i=0; i<src->nibbleList->Count; i++ ){
		EPGDB_CONTENT_DATA item;
		CopyData(src->nibbleList[i], &item);
		dest->nibbleList.push_back(item);
	}
}

void CtrlCmdUtil::CopyData(EPGDB_CONTEN_INFO* src, Def::EpgContentInfo^% dest)
{
	for( size_t i=0; i<src->nibbleList.size(); i++ ){
		Def::EpgContentData^ item = gcnew Def::EpgContentData();
		CopyData(&src->nibbleList[i], item);
		dest->nibbleList->Add(item);
	}
}

void CtrlCmdUtil::CopyData(Def::EpgComponentInfo^ src, EPGDB_COMPONENT_INFO* dest)
{
	pin_ptr<const wchar_t> text_charPin = PtrToStringChars(src->text_char);

	dest->stream_content = src->stream_content;
	dest->component_type = src->component_type;
	dest->component_tag = src->component_tag;
	dest->text_char = text_charPin;
}

void CtrlCmdUtil::CopyData(EPGDB_COMPONENT_INFO* src, Def::EpgComponentInfo^% dest)
{
	dest->stream_content = src->stream_content;
	dest->component_type = src->component_type;
	dest->component_tag = src->component_tag;
	dest->text_char = gcnew String(src->text_char.c_str());
}

void CtrlCmdUtil::CopyData(Def::EpgAudioComponentInfoData^ src, EPGDB_AUDIO_COMPONENT_INFO_DATA* dest)
{
	pin_ptr<const wchar_t> text_charPin = PtrToStringChars(src->text_char);

	dest->stream_content = src->stream_content;
	dest->component_type = src->component_type;
	dest->component_tag = src->component_tag;
	dest->stream_type = src->stream_type;
	dest->simulcast_group_tag = src->simulcast_group_tag;
	dest->ES_multi_lingual_flag = src->ES_multi_lingual_flag;
	dest->main_component_flag = src->main_component_flag;
	dest->quality_indicator = src->quality_indicator;
	dest->sampling_rate = src->sampling_rate;
	dest->text_char = text_charPin;
}

void CtrlCmdUtil::CopyData(EPGDB_AUDIO_COMPONENT_INFO_DATA* src, Def::EpgAudioComponentInfoData^% dest)
{
	dest->stream_content = src->stream_content;
	dest->component_type = src->component_type;
	dest->component_tag = src->component_tag;
	dest->stream_type = src->stream_type;
	dest->simulcast_group_tag = src->simulcast_group_tag;
	dest->ES_multi_lingual_flag = src->ES_multi_lingual_flag;
	dest->main_component_flag = src->main_component_flag;
	dest->quality_indicator = src->quality_indicator;
	dest->sampling_rate = src->sampling_rate;
	dest->text_char = gcnew String(src->text_char.c_str());
}

void CtrlCmdUtil::CopyData(Def::EpgAudioComponentInfo^ src, EPGDB_AUDIO_COMPONENT_INFO* dest)
{
	for( int i=0; i<src->componentList->Count; i++ ){
		EPGDB_AUDIO_COMPONENT_INFO_DATA item;
		CopyData(src->componentList[i], &item);
		dest->componentList.push_back(item);
	}
}

void CtrlCmdUtil::CopyData(EPGDB_AUDIO_COMPONENT_INFO* src, Def::EpgAudioComponentInfo^% dest)
{
	for( size_t i=0; i<src->componentList.size(); i++ ){
		Def::EpgAudioComponentInfoData^ item = gcnew Def::EpgAudioComponentInfoData();
		CopyData(&src->componentList[i], item);
		dest->componentList->Add(item);
	}
}

void CtrlCmdUtil::CopyData(Def::EpgEventData^ src, EPGDB_EVENT_DATA* dest)
{
	dest->original_network_id = src->original_network_id;
	dest->transport_stream_id = src->transport_stream_id;
	dest->service_id = src->service_id;
	dest->event_id = src->event_id;
}

void CtrlCmdUtil::CopyData(EPGDB_EVENT_DATA* src, Def::EpgEventData^% dest)
{
	dest->original_network_id = src->original_network_id;
	dest->transport_stream_id = src->transport_stream_id;
	dest->service_id = src->service_id;
	dest->event_id = src->event_id;
}

void CtrlCmdUtil::CopyData(Def::EpgEventGroupInfo^ src, EPGDB_EVENTGROUP_INFO* dest)
{
	dest->group_type = src->group_type;
	for( int i=0; i<src->eventDataList->Count; i++ ){
		EPGDB_EVENT_DATA item;
		CopyData(src->eventDataList[i], &item);
		dest->eventDataList.push_back(item);
	}
}

void CtrlCmdUtil::CopyData(EPGDB_EVENTGROUP_INFO* src, Def::EpgEventGroupInfo^% dest)
{
	dest->group_type = src->group_type;
	for( size_t i=0; i<src->eventDataList.size(); i++ ){
		Def::EpgEventData^ item = gcnew Def::EpgEventData();
		CopyData(&src->eventDataList[i], item);
		dest->eventDataList->Add(item);
	}
}

void CtrlCmdUtil::CopyData(Def::EpgEventInfo^ src, EPGDB_EVENT_INFO* dest)
{
	dest->original_network_id = src->original_network_id;
	dest->transport_stream_id = src->transport_stream_id;
	dest->service_id = src->service_id;
	dest->event_id = src->event_id;
	dest->StartTimeFlag = src->StartTimeFlag;
//	CopyData(src->start_time, &dest->start_time);
	dest->DurationFlag = src->DurationFlag;
	dest->durationSec = src->durationSec;
	dest->freeCAFlag = src->FreeCAFlag;

	if( src->ShortInfo != nullptr ){
		dest->shortInfo = new EPGDB_SHORT_EVENT_INFO;
		CopyData(src->ShortInfo, dest->shortInfo);
	}else{
		dest->shortInfo = NULL;
	}
	if( src->ExtInfo != nullptr ){
		dest->extInfo = new EPGDB_EXTENDED_EVENT_INFO;
		CopyData(src->ExtInfo, dest->extInfo);
	}else{
		dest->extInfo = NULL;
	}
	if( src->ContentInfo != nullptr ){
		dest->contentInfo = new EPGDB_CONTEN_INFO;
		CopyData(src->ContentInfo, dest->contentInfo);
	}else{
		dest->contentInfo = NULL;
	}
	if( src->ComponentInfo != nullptr ){
		dest->componentInfo = new EPGDB_COMPONENT_INFO;
		CopyData(src->ComponentInfo, dest->componentInfo);
	}else{
		dest->componentInfo = NULL;
	}
	if( src->AudioInfo != nullptr ){
		dest->audioInfo = new EPGDB_AUDIO_COMPONENT_INFO;
		CopyData(src->AudioInfo, dest->audioInfo);
	}else{
		dest->audioInfo = NULL;
	}
	if( src->EventGroupInfo != nullptr ){
		dest->eventGroupInfo = new EPGDB_EVENTGROUP_INFO;
		CopyData(src->EventGroupInfo, dest->eventGroupInfo);
	}else{
		dest->eventGroupInfo = NULL;
	}
	if( src->EventRelayInfo != nullptr ){
		dest->eventRelayInfo = new EPGDB_EVENTGROUP_INFO;
		CopyData(src->EventRelayInfo, dest->eventRelayInfo);
	}else{
		dest->eventRelayInfo = NULL;
	}

}

void CtrlCmdUtil::CopyData(EPGDB_EVENT_INFO* src, Def::EpgEventInfo^% dest)
{
	dest->original_network_id = src->original_network_id;
	dest->transport_stream_id = src->transport_stream_id;
	dest->service_id = src->service_id;
	dest->event_id = src->event_id;
	dest->StartTimeFlag = src->StartTimeFlag;
	dest->start_time = ConvertTime( &src->start_time );
//	CopyData(&src->start_time, dest->start_time);
	dest->DurationFlag = src->DurationFlag;
	dest->durationSec = src->durationSec;
	dest->FreeCAFlag = src->freeCAFlag;

	if( src->shortInfo != NULL ){
		dest->ShortInfo = gcnew Def::EpgShortEventInfo();
		CopyData(src->shortInfo, dest->ShortInfo);
	}else{
		dest->ShortInfo = nullptr;
	}
	if( src->extInfo != NULL ){
		dest->ExtInfo = gcnew Def::EpgExtendedEventInfo();
		CopyData(src->extInfo, dest->ExtInfo);
	}else{
		dest->ExtInfo = nullptr;
	}
	if( src->contentInfo != NULL ){
		dest->ContentInfo = gcnew Def::EpgContentInfo();
		CopyData(src->contentInfo, dest->ContentInfo);
	}else{
		dest->ContentInfo = nullptr;
	}
	if( src->componentInfo != NULL ){
		dest->ComponentInfo = gcnew Def::EpgComponentInfo();
		CopyData(src->componentInfo, dest->ComponentInfo);
	}else{
		dest->ComponentInfo = nullptr;
	}
	if( src->audioInfo != NULL ){
		dest->AudioInfo = gcnew Def::EpgAudioComponentInfo();
		CopyData(src->audioInfo, dest->AudioInfo);
	}else{
		dest->AudioInfo = nullptr;
	}
	if( src->eventGroupInfo != NULL ){
		dest->EventGroupInfo = gcnew Def::EpgEventGroupInfo();
		CopyData(src->eventGroupInfo, dest->EventGroupInfo);
	}else{
		dest->EventGroupInfo = nullptr;
	}
	if( src->eventRelayInfo != NULL ){
		dest->EventRelayInfo = gcnew Def::EpgEventGroupInfo();
		CopyData(src->eventRelayInfo, dest->EventRelayInfo);
	}else{
		dest->EventRelayInfo = nullptr;
	}

}

void CtrlCmdUtil::CopyData(Def::EpgServiceInfo^ src, EPGDB_SERVICE_INFO* dest)
{
	pin_ptr<const wchar_t> service_provider_namePin = PtrToStringChars(src->service_provider_name);
	pin_ptr<const wchar_t> service_namePin = PtrToStringChars(src->service_name);
	pin_ptr<const wchar_t> network_namePin = PtrToStringChars(src->network_name);
	pin_ptr<const wchar_t> ts_namePin = PtrToStringChars(src->ts_name);

	dest->ONID = src->ONID;
	dest->TSID = src->TSID;
	dest->SID = src->SID;
	dest->service_type = src->service_type;
	dest->partialReceptionFlag = src->partialReceptionFlag;
	dest->service_provider_name = service_provider_namePin;
	dest->service_name = service_namePin;
	dest->network_name = network_namePin;
	dest->ts_name = ts_namePin;
	dest->remote_control_key_id = src->remote_control_key_id;
}

void CtrlCmdUtil::CopyData(EPGDB_SERVICE_INFO* src, Def::EpgServiceInfo^% dest)
{
	dest->ONID = src->ONID;
	dest->TSID = src->TSID;
	dest->SID = src->SID;
	dest->service_type = src->service_type;
	dest->partialReceptionFlag = src->partialReceptionFlag;
	dest->service_provider_name = gcnew String(src->service_provider_name.c_str());
	dest->service_name = gcnew String(src->service_name.c_str());
	dest->network_name = gcnew String(src->network_name.c_str());
	dest->ts_name = gcnew String(src->ts_name.c_str());
	dest->remote_control_key_id = src->remote_control_key_id;
}

void CtrlCmdUtil::CopyData(Def::EpgSearchDateInfo^ src, EPGDB_SEARCH_DATE_INFO* dest)
{
	dest->startDayOfWeek = src->startDayOfWeek;
	dest->startHour = src->startHour;
	dest->startMin = src->startMin;
	dest->endDayOfWeek = src->endDayOfWeek;
	dest->endHour = src->endHour;
	dest->endMin = src->endMin;
}

void CtrlCmdUtil::CopyData(EPGDB_SEARCH_DATE_INFO* src, Def::EpgSearchDateInfo^% dest)
{
	dest->startDayOfWeek = src->startDayOfWeek;
	dest->startHour = src->startHour;
	dest->startMin = src->startMin;
	dest->endDayOfWeek = src->endDayOfWeek;
	dest->endHour = src->endHour;
	dest->endMin = src->endMin;
}

void CtrlCmdUtil::CopyData(Def::EpgSearchKeyInfo^ src, EPGDB_SEARCH_KEY_INFO* dest)
{
	pin_ptr<const wchar_t> andKeyPin = PtrToStringChars(src->andKey);
	pin_ptr<const wchar_t> notKeyPin = PtrToStringChars(src->notKey);

	dest->andKey = andKeyPin;
	dest->notKey = notKeyPin;
	dest->titleOnlyFlag = src->titleOnlyFlag;
	dest->regExpFlag = src->regExpFlag;

	for( int i=0; i<src->contentList->Count; i++ ){
		EPGDB_CONTENT_DATA item;
		CopyData(src->contentList[i], &item);
		dest->contentList.push_back(item);
	}

	for( int i=0; i<src->dateList->Count; i++ ){
		EPGDB_SEARCH_DATE_INFO item;
		CopyData(src->dateList[i], &item);
		dest->dateList.push_back(item);
	}

	for( int i=0; i<src->serviceList->Count; i++ ){
		dest->serviceList.push_back(src->serviceList[i]);
	}

	for( int i=0; i<src->videoList->Count; i++ ){
		dest->videoList.push_back(src->videoList[i]);
	}

	for( int i=0; i<src->audioList->Count; i++ ){
		dest->audioList.push_back(src->audioList[i]);
	}
	dest->aimaiFlag = src->aimaiFlag;
	dest->notContetFlag = src->notContetFlag;
	dest->notDateFlag = src->notDateFlag;
	dest->freeCAFlag = src->freeCAFlag;
	dest->chkRecEnd = src->chkRecEnd;
	dest->chkRecDay = src->chkRecDay;
}

void CtrlCmdUtil::CopyData(EPGDB_SEARCH_KEY_INFO* src, Def::EpgSearchKeyInfo^% dest)
{
	dest->andKey = gcnew String(src->andKey.c_str());
	dest->notKey = gcnew String(src->notKey.c_str());
	dest->titleOnlyFlag = src->titleOnlyFlag;
	dest->regExpFlag = src->regExpFlag;

	for( size_t i=0; i<src->contentList.size(); i++ ){
		Def::EpgContentData^ item = gcnew Def::EpgContentData();
		CopyData(&src->contentList[i], item);
		dest->contentList->Add(item);
	}

	for( size_t i=0; i<src->dateList.size(); i++ ){
		Def::EpgSearchDateInfo^ item = gcnew Def::EpgSearchDateInfo();
		CopyData(&src->dateList[i], item);
		dest->dateList->Add(item);
	}

	for( size_t i=0; i<src->serviceList.size(); i++ ){
		dest->serviceList->Add(src->serviceList[i]);
	}

	for( size_t i=0; i<src->videoList.size(); i++ ){
		dest->videoList->Add(src->videoList[i]);
	}

	for( size_t i=0; i<src->audioList.size(); i++ ){
		dest->audioList->Add(src->audioList[i]);
	}

	dest->aimaiFlag = src->aimaiFlag;
	dest->notContetFlag = src->notContetFlag;
	dest->notDateFlag = src->notDateFlag;
	dest->freeCAFlag = src->freeCAFlag;
	dest->chkRecEnd = src->chkRecEnd;
	dest->chkRecDay = src->chkRecDay;
}

void CtrlCmdUtil::CopyData(Def::RecFileInfo^ src, REC_FILE_INFO* dest)
{
	pin_ptr<const wchar_t> recFilePathPin = PtrToStringChars(src->RecFilePath);
	pin_ptr<const wchar_t> titlePin = PtrToStringChars(src->Title);
	pin_ptr<const wchar_t> serviceNamePin = PtrToStringChars(src->ServiceName);
	pin_ptr<const wchar_t> commentPin = PtrToStringChars(src->Comment);
	pin_ptr<const wchar_t> programInfoPin = PtrToStringChars(src->ProgramInfo);
	pin_ptr<const wchar_t> errInfoPin = PtrToStringChars(src->ErrInfo);

	dest->id = src->ID;
	dest->recFilePath = recFilePathPin;
	dest->title = titlePin;
	CopyData(src->StartTime, &dest->startTime);
	dest->durationSecond = src->DurationSecond;
	dest->serviceName = serviceNamePin;
	dest->originalNetworkID = src->OriginalNetworkID;
	dest->transportStreamID = src->TransportStreamID;
	dest->serviceID = src->ServiceID;
	dest->eventID = src->EventID;
	dest->drops = src->Drops;
	dest->scrambles = src->Scrambles;
	dest->recStatus = src->RecStatus;
	CopyData(src->StartTimeEpg, &dest->startTimeEpg);
	dest->comment = commentPin;
	dest->programInfo = programInfoPin;
	dest->errInfo = errInfoPin;
	dest->protectFlag = src->ProtectFlag;
}

void CtrlCmdUtil::CopyData(REC_FILE_INFO* src, Def::RecFileInfo^% dest)
{
	dest->ID = src->id;
	dest->RecFilePath = gcnew String(src->recFilePath.c_str());
	dest->Title = gcnew String(src->title.c_str());
	dest->StartTime = ConvertTime( &src->startTime );
	dest->DurationSecond = src->durationSecond;
	dest->ServiceName = gcnew String(src->serviceName.c_str());
	dest->OriginalNetworkID = src->originalNetworkID;
	dest->TransportStreamID = src->transportStreamID;
	dest->ServiceID = src->serviceID;
	dest->EventID = src->eventID;
	dest->Drops = src->drops;
	dest->Scrambles = src->scrambles;
	dest->RecStatus = src->recStatus;
	dest->StartTimeEpg = ConvertTime( &src->startTimeEpg );
	dest->Comment = gcnew String(src->comment.c_str());
	dest->ProgramInfo = gcnew String(src->programInfo.c_str());
	dest->ErrInfo = gcnew String(src->errInfo.c_str());
	if( src->protectFlag == 1 ){
		dest->ProtectFlag = true;
	}else{
		dest->ProtectFlag = false;
	}
}

void CtrlCmdUtil::CopyData(Def::EpgAutoAddData^ src, EPG_AUTO_ADD_DATA* dest)
{
	dest->dataID = src->dataID;
	CopyData(src->searchInfo, &dest->searchInfo);
	CopyData(src->recSetting, &dest->recSetting);
	dest->addCount = src->addCount;
}

void CtrlCmdUtil::CopyData(EPG_AUTO_ADD_DATA* src, Def::EpgAutoAddData^% dest)
{
	dest->dataID = src->dataID;
	CopyData(&src->searchInfo, dest->searchInfo);
	CopyData(&src->recSetting, dest->recSetting);
	dest->addCount = src->addCount;
	
}

void CtrlCmdUtil::CopyData(Def::ManualAutoAddData^ src, MANUAL_AUTO_ADD_DATA* dest)
{
	pin_ptr<const wchar_t> titlePin = PtrToStringChars(src->title);
	pin_ptr<const wchar_t> stationNamePin = PtrToStringChars(src->stationName);

	dest->dataID = src->dataID;
	dest->dayOfWeekFlag = src->dayOfWeekFlag;
	dest->startTime = src->startTime;
	dest->durationSecond = src->durationSecond;
	dest->title = titlePin;
	dest->stationName = stationNamePin;
	dest->originalNetworkID = src->originalNetworkID;
	dest->transportStreamID = src->transportStreamID;
	dest->serviceID = src->serviceID;
	CopyData(src->recSetting, &dest->recSetting);
}

void CtrlCmdUtil::CopyData(MANUAL_AUTO_ADD_DATA* src, Def::ManualAutoAddData^% dest)
{
	dest->dataID = src->dataID;
	dest->dayOfWeekFlag = src->dayOfWeekFlag;
	dest->startTime = src->startTime;
	dest->durationSecond = src->durationSecond;
	dest->title = gcnew String(src->title.c_str());
	dest->stationName = gcnew String(src->stationName.c_str());
	dest->originalNetworkID = src->originalNetworkID;
	dest->transportStreamID = src->transportStreamID;
	dest->serviceID = src->serviceID;
	CopyData(&src->recSetting, dest->recSetting);
}

void CtrlCmdUtil::CopyData(Def::TunerReserveInfo^ src, TUNER_RESERVE_INFO* dest)
{
	pin_ptr<const wchar_t> tunerNamePin = PtrToStringChars(src->tunerName);

	dest->tunerID = src->tunerID;
	dest->tunerName = tunerNamePin;
	for( int i=0; i<src->reserveList->Count; i++ ){
		dest->reserveList.push_back(src->reserveList[i]);
	}
}

void CtrlCmdUtil::CopyData(TUNER_RESERVE_INFO* src, Def::TunerReserveInfo^% dest)
{
	dest->tunerID = src->tunerID;
	dest->tunerName = gcnew String(src->tunerName.c_str());
	for( size_t i=0; i<src->reserveList.size(); i++ ){
		dest->reserveList->Add(src->reserveList[i]);
	}
}

void CtrlCmdUtil::CopyData(Def::EpgServiceEventInfo^ src, EPGDB_SERVICE_EVENT_INFO* dest)
{
	CopyData(src->serviceInfo, &dest->serviceInfo);
	for( int i=0; i<src->eventList->Count; i++ ){
		EPGDB_EVENT_INFO* item = new EPGDB_EVENT_INFO;
		CopyData(src->eventList[i], item);
		dest->eventList.push_back(item);
	}
}

void CtrlCmdUtil::CopyData(EPGDB_SERVICE_EVENT_INFO* src, Def::EpgServiceEventInfo^% dest)
{
	CopyData(&src->serviceInfo, dest->serviceInfo);
	for( size_t i=0; i<src->eventList.size(); i++ ){
		Def::EpgEventInfo^ item = gcnew Def::EpgEventInfo();
		CopyData(src->eventList[i], item);
		dest->eventList->Add(item);
	}
}

void CtrlCmdUtil::CopyData(Def::SetChInfo^ src, SET_CH_INFO* dest)
{
	dest->useSID = src->useSID;
	dest->ONID = src->ONID;
	dest->TSID = src->TSID;
	dest->SID = src->SID;
	dest->useBonCh = src->useBonCh;
	dest->space = src->space;
	dest->ch = src->ch;
}

void CtrlCmdUtil::CopyData(SET_CH_INFO* src, Def::SetChInfo^% dest)
{
	dest->useSID = src->useSID;
	dest->ONID = src->ONID;
	dest->TSID = src->TSID;
	dest->SID = src->SID;
	dest->useBonCh = src->useBonCh;
	dest->space = src->space;
	dest->ch = src->ch;
}

void CtrlCmdUtil::CopyData(Def::TvTestChChgInfo^ src, TVTEST_CH_CHG_INFO* dest)
{
	pin_ptr<const wchar_t> bonDriverPin = PtrToStringChars(src->bonDriver);

	dest->bonDriver = bonDriverPin;
	CopyData(src->chInfo, &dest->chInfo);
}

void CtrlCmdUtil::CopyData(TVTEST_CH_CHG_INFO* src, Def::TvTestChChgInfo^% dest)
{
	dest->bonDriver = gcnew String(src->bonDriver.c_str());
	CopyData(&src->chInfo, dest->chInfo);
}

void CtrlCmdUtil::CopyData(Def::TVTestStreamingInfo^ src, TVTEST_STREAMING_INFO* dest)
{
	pin_ptr<const wchar_t> filePathPin = PtrToStringChars(src->filePath);

	dest->filePath = filePathPin;

	dest->enableMode = src->enableMode;
	dest->ctrlID = src->ctrlID;
	dest->serverIP = src->serverIP;
	dest->serverPort = src->serverPort;
	dest->udpSend = src->udpSend;
	dest->tcpSend = src->tcpSend;
	dest->timeShiftMode = src->timeShiftMode;
}

void CtrlCmdUtil::CopyData(TVTEST_STREAMING_INFO* src, Def::TVTestStreamingInfo^% dest)
{
	dest->filePath = gcnew String(src->filePath.c_str());
	dest->enableMode = src->enableMode;
	dest->ctrlID = src->ctrlID;
	dest->serverIP = src->serverIP;
	dest->serverPort = src->serverPort;
	dest->udpSend = src->udpSend;
	dest->tcpSend = src->tcpSend;
	dest->timeShiftMode = src->timeShiftMode;
}

void CtrlCmdUtil::CopyData(Def::NWPlayTimeShiftInfo^ src, NWPLAY_TIMESHIFT_INFO* dest)
{
	pin_ptr<const wchar_t> filePathPin = PtrToStringChars(src->filePath);

	dest->filePath = filePathPin;
	dest->ctrlID = src->ctrlID;
}

void CtrlCmdUtil::CopyData(NWPLAY_TIMESHIFT_INFO* src, Def::NWPlayTimeShiftInfo^% dest)
{
	dest->filePath = gcnew String(src->filePath.c_str());
	dest->ctrlID = src->ctrlID;
}

/// <summary>
/// コマンド送信方法の設定
/// </summary>
/// <param name="tcpFlag">[IN]true：TCP/IPモード、false：名前付きパイプモード</param>
void CtrlCmdUtil::SetSendMode(
	bool tcpFlag
	)
{
	BOOL flag = TRUE;
	if( tcpFlag == false ){
		flag = FALSE;
	}
	this->sendCmd->SetSendMode(flag);
}

/// <summary>
/// 名前付きパイプモード時の接続先を設定
/// </summary>
/// <param name="eventName">[IN]排他制御用Eventの名前</param>
/// <param name="pipeName">[IN]接続パイプの名前</param>
void CtrlCmdUtil::SetPipeSetting(
	String^ eventName,
	String^ pipeName
	)
{
	pin_ptr<const wchar_t> eventNamePin = PtrToStringChars(eventName);
	pin_ptr<const wchar_t> pipeNamePin = PtrToStringChars(pipeName);

	this->sendCmd->SetPipeSetting(eventNamePin, pipeNamePin);
}

/// <summary>
/// TCP/IPモード時の接続先を設定
/// </summary>
/// <param name="ip">[IN]接続先IP</param>
/// <param name="port">[IN]接続先ポート</param>
void CtrlCmdUtil::SetNWSetting(
	String^ ip,
	UInt32 port
	)
{
	pin_ptr<const wchar_t> ipPin = PtrToStringChars(ip);
	std::wstring _ip(ipPin);

	this->sendCmd->SetNWSetting(_ip, port);
}

/// <summary>
/// 接続処理時のタイムアウト設定
/// </summary>
/// <param name="timeOut">[IN]タイムアウト値（単位：ms）</param>
void CtrlCmdUtil::SetConnectTimeOut(
	UInt32 timeOut
	)
{
	this->sendCmd->SetConnectTimeOut(timeOut);
}

/// <summary>
/// Program.txtを追加で再読み込みする
/// </summary>
UInt32 CtrlCmdUtil::SendAddloadReserve()
{
	return this->sendCmd->SendAddloadReserve();
}

/// <summary>
/// EPGデータを再読み込みする
/// </summary>
UInt32 CtrlCmdUtil::SendReloadEpg()
{
	return this->sendCmd->SendReloadEpg();
}

/// <summary>
/// 設定情報を再読み込みする
/// </summary>
UInt32 CtrlCmdUtil::SendReloadSetting()
{
	return this->sendCmd->SendReloadSetting();
}

/// <summary>
/// EpgTimerSrv.exeを終了する
/// </summary>
UInt32 CtrlCmdUtil::SendClose()
{
	return this->sendCmd->SendClose();
}

/// <summary>
/// EpgTimerSrv.exeのパイプ接続GUIとしてプロセスを登録する
/// </summary>
/// <param name="processID">[IN]プロセスID</param>
UInt32 CtrlCmdUtil::SendRegistGUI(
	UInt32 processID
	)
{
	return this->sendCmd->SendRegistGUI(processID);
}

/// <summary>
/// EpgTimerSrv.exeのパイプ接続GUI登録を解除する
/// </summary>
/// <param name="processID">[IN]プロセスID</param>
UInt32 CtrlCmdUtil::SendUnRegistGUI(
	UInt32 processID
	)
{
	return this->sendCmd->SendUnRegistGUI(processID);
}

/// <summary>
/// EpgTimerSrv.exeのTCP接続GUIとしてプロセスを登録する
/// </summary>
/// <param name="port">[IN]ポート</param>
UInt32 CtrlCmdUtil::SendRegistTCP(
	UInt32 port
	)
{
	return this->sendCmd->SendRegistTCP(port);
}

/// <summary>
/// EpgTimerSrv.exeのTCP接続GUI登録を解除する
/// </summary>
/// <param name="port">[IN]ポート</param>
UInt32 CtrlCmdUtil::SendUnRegistTCP(
	UInt32 port
	)
{
	return this->sendCmd->SendUnRegistTCP(port);
}

/// <summary>
/// 予約一覧を取得する
/// </summary>
/// <param name="val">[OUT]予約一覧</param>
UInt32 CtrlCmdUtil::SendEnumReserve(
	List<Def::ReserveData^>^% val
	)
{
	vector<RESERVE_DATA> list;
	DWORD ret = this->sendCmd->SendEnumReserve2(&list);
	for( size_t i=0; i<list.size(); i++ ){
		Def::ReserveData^ item = gcnew Def::ReserveData();
		CopyData(&list[i], item);
		val->Add(item);
	}

	return ret;
}

/// <summary>
/// 予約情報を取得する
/// </summary>
/// <param name="reserveID">[IN]取得する情報の予約ID</param>
/// <param name="val">[OUT]予約情報</param>
UInt32 CtrlCmdUtil::SendGetReserve(
	UInt32 reserveID,
	Def::ReserveData^% val
	)
{
	RESERVE_DATA info;
	DWORD ret = this->sendCmd->SendGetReserve2(reserveID, &info);
	if( ret == CMD_SUCCESS ){
		CopyData(&info, val);
	}
	return ret;
}

/// <summary>
/// 予約を追加する
/// </summary>
/// <param name="val">[IN]追加する予約一覧</param>
UInt32 CtrlCmdUtil::SendAddReserve(
	List<Def::ReserveData^>^ val
	)
{
	vector<RESERVE_DATA> list;
	for( int i=0; i<val->Count; i++ ){
		RESERVE_DATA item;
		CopyData(val[i], &item);
		list.push_back(item);
	}
	DWORD ret = this->sendCmd->SendAddReserve2(&list);

	return ret;
}

/// <summary>
/// 予約を削除する
/// </summary>
/// <param name="val">[IN]削除する予約ID一覧</param>
UInt32 CtrlCmdUtil::SendDelReserve(
	List<UInt32>^ val
	)
{
	vector<DWORD> list;
	for( int i=0; i<val->Count; i++ ){
		list.push_back(val[i]);
	}
	DWORD ret = this->sendCmd->SendDelReserve(&list);

	return ret;
}

/// <summary>
/// 予約を変更する
/// </summary>
/// <param name="val">[IN]変更する予約一覧</param>
UInt32 CtrlCmdUtil::SendChgReserve(
	List<Def::ReserveData^>^ val
	)
{
	vector<RESERVE_DATA> list;
	for( int i=0; i<val->Count; i++ ){
		RESERVE_DATA item;
		CopyData(val[i], &item);
		list.push_back(item);
	}
	DWORD ret = this->sendCmd->SendChgReserve2(&list);

	return ret;
}


/// <summary>
/// チューナーごとの予約一覧を取得する
/// </summary>
/// <param name="val">[OUT]予約情報</param>
UInt32 CtrlCmdUtil::SendEnumTunerReserve(
	List<Def::TunerReserveInfo^>^% val
	)
{
	vector<TUNER_RESERVE_INFO> list;
	DWORD ret = this->sendCmd->SendEnumTunerReserve(&list);
	if( ret == CMD_SUCCESS ){
		for( size_t i=0; i<list.size(); i++ ){
			Def::TunerReserveInfo^ item = gcnew Def::TunerReserveInfo();
			CopyData(&list[i], item);
			val->Add(item);
		}
	}

	return ret;
}

/// <summary>
/// 録画済み情報一覧を取得する
/// </summary>
/// <param name="val">[OUT]録画済み情報一覧</param>
UInt32 CtrlCmdUtil::SendEnumRecInfo(
	List<Def::RecFileInfo^>^% val
	)
{
	vector<REC_FILE_INFO> list;
	DWORD ret = this->sendCmd->SendEnumRecInfo2(&list);
	if( ret == CMD_SUCCESS ){
		for( size_t i=0; i<list.size(); i++ ){
			Def::RecFileInfo^ item = gcnew Def::RecFileInfo();
			CopyData(&list[i], item);
			val->Add(item);
		}
	}

	return ret;
}

/// <summary>
/// 録画済み情報を削除する
/// </summary>
/// <param name="val">[IN]削除するID一覧</param>
UInt32 CtrlCmdUtil::SendDelRecInfo(
	List<UInt32>^ val
	)
{
	vector<DWORD> list;
	for( int i=0; i<val->Count; i++ ){
		list.push_back(val[i]);
	}
	DWORD ret = this->sendCmd->SendDelRecInfo(&list);

	return ret;
}

/// <summary>
/// 予約を変更する
/// </summary>
/// <param name="val">[IN]変更する予約一覧</param>
UInt32 CtrlCmdUtil::SendChgProtectRecInfo(
	List<Def::RecFileInfo^>^ val
	)
{
	vector<REC_FILE_INFO> list;
	for( int i=0; i<val->Count; i++ ){
		REC_FILE_INFO item;
		CopyData(val[i], &item);
		list.push_back(item);
	}
	DWORD ret = this->sendCmd->SendChgProtectRecInfo2(&list);

	return ret;
}

/// <summary>
/// サービス一覧を取得する
/// </summary>
/// <param name="val">[OUT]サービス一覧</param>
UInt32 CtrlCmdUtil::SendEnumService(
	List<Def::EpgServiceInfo^>^% val
	)
{
	vector<EPGDB_SERVICE_INFO> list;
	DWORD ret = this->sendCmd->SendEnumService(&list);
	if( ret == CMD_SUCCESS ){
		for( size_t i=0; i<list.size(); i++ ){
			Def::EpgServiceInfo^ item = gcnew Def::EpgServiceInfo();
			CopyData(&list[i], item);
			val->Add(item);
		}
	}

	return ret;
}

/// <summary>
/// サービス指定で番組情報を一覧を取得する
/// </summary>
/// <param name="service">[IN]ONID＜＜32 | TSID＜＜16 | SIDとしたサービスID</param>
/// <param name="val">[OUT]番組情報一覧</param>
UInt32 CtrlCmdUtil::SendEnumPgInfo(
	UInt64 service,
	List<Def::EpgEventInfo^>^% val
	)
{
	vector<EPGDB_EVENT_INFO*> list;
	DWORD ret = this->sendCmd->SendEnumPgInfo(service, &list);
	if( ret == CMD_SUCCESS ){
		for( size_t i=0; i<list.size(); i++ ){
			Def::EpgEventInfo^ item = gcnew Def::EpgEventInfo();
			CopyData(list[i], item);
			val->Add(item);
			SAFE_DELETE(list[i]);
		}
	}

	return ret;
}

/// <summary>
/// 指定イベントの番組情報を取得する
/// </summary>
/// <param name="pgID">[IN]ONID＜＜48 | TSID＜＜32 | SID＜＜16 | EventIDとしたID</param>
/// <param name="val">[OUT]番組情報</param>
UInt32 CtrlCmdUtil::SendGetPgInfo(
	UInt64 pgID,
	Def::EpgEventInfo^% val
	)
{
	EPGDB_EVENT_INFO info;
	DWORD ret = this->sendCmd->SendGetPgInfo(pgID, &info);
	if( ret == CMD_SUCCESS ){
		CopyData(&info, val);
	}

	return ret;
}

/// <summary>
/// 指定イベントの番組情報を取得する
/// </summary>
/// <param name="key">[IN]検索キー（複数指定時はまとめて検索結果が返る）</param>
/// <param name="val">[OUT]番組情報一覧</param>
UInt32 CtrlCmdUtil::SendSearchPg(
	List<Def::EpgSearchKeyInfo^>^ key,
	List<Def::EpgEventInfo^>^% val
	)
{
	vector<EPGDB_SEARCH_KEY_INFO> keyList;
	vector<EPGDB_EVENT_INFO*> list;

	for( int i=0; i<key->Count; i++ ){
		EPGDB_SEARCH_KEY_INFO item;
		CopyData(key[i], &item);
		keyList.push_back(item);
	}

	DWORD ret = this->sendCmd->SendSearchPg(&keyList, &list);
	if( ret == CMD_SUCCESS ){
		for( size_t i=0; i<list.size(); i++ ){
			Def::EpgEventInfo^ item = gcnew Def::EpgEventInfo();
			CopyData(list[i], item);
			val->Add(item);
			SAFE_DELETE(list[i]);
		}
	}

	return ret;
}

/// <summary>
/// スタンバイ、休止、シャットダウンを行っていいかの確認
/// </summary>
UInt32 CtrlCmdUtil::SendChkSuspend(
	)
{
	return this->sendCmd->SendChkSuspend();
}

/// <summary>
/// スタンバイ、休止、シャットダウンに移行する
/// </summary>
/// <param name="val">[IN]1:スタンバイ 2:休止 3:シャットダウン | 0x0100:復帰後再起動</param>
UInt32 CtrlCmdUtil::SendSuspend(
	UInt16 val
	)
{
	return this->sendCmd->SendSuspend(val);
}

/// <summary>
/// PC再起動を行う
/// </summary>
UInt32 CtrlCmdUtil::SendReboot(
	)
{
	return this->sendCmd->SendReboot();
}

/// <summary>
/// 10秒後にEPGデータの取得を行う
/// </summary>
UInt32 CtrlCmdUtil::SendEpgCapNow(
	)
{
	return this->sendCmd->SendEpgCapNow();
}

/// <summary>
/// 自動予約登録条件一覧を取得する
/// </summary>
/// <param name="val">[OUT]条件一覧</param>
UInt32 CtrlCmdUtil::SendEnumEpgAutoAdd(
	List<Def::EpgAutoAddData^>^% val
	)
{
	vector<EPG_AUTO_ADD_DATA> list;
	DWORD ret = this->sendCmd->SendEnumEpgAutoAdd2(&list);
	for( size_t i=0; i<list.size(); i++ ){
		Def::EpgAutoAddData^ item = gcnew Def::EpgAutoAddData();
		CopyData(&list[i], item);
		val->Add(item);
	}

	return ret;
}

/// <summary>
/// 予約を追加する
/// </summary>
/// <param name="val">[IN]条件一覧</param>
UInt32 CtrlCmdUtil::SendAddEpgAutoAdd(
	List<Def::EpgAutoAddData^>^ val
	)
{
	vector<EPG_AUTO_ADD_DATA> list;
	for( int i=0; i<val->Count; i++ ){
		EPG_AUTO_ADD_DATA item;
		CopyData(val[i], &item);
		list.push_back(item);
	}
	DWORD ret = this->sendCmd->SendAddEpgAutoAdd2(&list);

	return ret;
}

/// <summary>
/// 予約を削除する
/// </summary>
/// <param name="val">[IN]ID一覧</param>
UInt32 CtrlCmdUtil::SendDelEpgAutoAdd(
	List<UInt32>^ val
	)
{
	vector<DWORD> list;
	for( int i=0; i<val->Count; i++ ){
		list.push_back(val[i]);
	}
	DWORD ret = this->sendCmd->SendDelEpgAutoAdd(&list);

	return ret;
}

/// <summary>
/// 予約を変更する
/// </summary>
/// <param name="val">[IN]条件一覧</param>
UInt32 CtrlCmdUtil::SendChgEpgAutoAdd(
	List<Def::EpgAutoAddData^>^ val
	)
{
	vector<EPG_AUTO_ADD_DATA> list;
	for( int i=0; i<val->Count; i++ ){
		EPG_AUTO_ADD_DATA item;
		CopyData(val[i], &item);
		list.push_back(item);
	}
	DWORD ret = this->sendCmd->SendChgEpgAutoAdd2(&list);

	return ret;
}

/// <summary>
/// 自動予約登録条件一覧を取得する
/// </summary>
/// <param name="val">[OUT]条件一覧</param>
UInt32 CtrlCmdUtil::SendEnumManualAdd(
	List<Def::ManualAutoAddData^>^% val
	)
{
	vector<MANUAL_AUTO_ADD_DATA> list;
	DWORD ret = this->sendCmd->SendEnumManualAdd2(&list);
	for( size_t i=0; i<list.size(); i++ ){
		Def::ManualAutoAddData^ item = gcnew Def::ManualAutoAddData();
		CopyData(&list[i], item);
		val->Add(item);
	}

	return ret;
}

/// <summary>
/// 自動予約登録条件を追加する
/// </summary>
/// <param name="val">[IN]条件一覧</param>
UInt32 CtrlCmdUtil::SendAddManualAdd(
	List<Def::ManualAutoAddData^>^ val
	)
{
	vector<MANUAL_AUTO_ADD_DATA> list;
	for( int i=0; i<val->Count; i++ ){
		MANUAL_AUTO_ADD_DATA item;
		CopyData(val[i], &item);
		list.push_back(item);
	}
	DWORD ret = this->sendCmd->SendAddManualAdd2(&list);

	return ret;
}

/// <summary>
/// 自動予約登録条件を削除する
/// </summary>
/// <param name="val">[IN]ID一覧</param>
UInt32 CtrlCmdUtil::SendDelManualAdd(
	List<UInt32>^ val
	)
{
	vector<DWORD> list;
	for( int i=0; i<val->Count; i++ ){
		list.push_back(val[i]);
	}
	DWORD ret = this->sendCmd->SendDelManualAdd(&list);

	return ret;
}

/// <summary>
/// 自動予約登録条件を変更する
/// </summary>
/// <param name="val">[IN]条件一覧</param>
UInt32 CtrlCmdUtil::SendChgManualAdd(
	List<Def::ManualAutoAddData^>^ val
	)
{
	vector<MANUAL_AUTO_ADD_DATA> list;
	for( int i=0; i<val->Count; i++ ){
		MANUAL_AUTO_ADD_DATA item;
		CopyData(val[i], &item);
		list.push_back(item);
	}
	DWORD ret = this->sendCmd->SendChgManualAdd2(&list);

	return ret;
}

/// <summary>
/// 指定ファイルを転送する
/// </summary>
/// <param name="val">[IN]ファイル名</param>
/// <param name="resVal">[OUT]バイナリ</param>
UInt32 CtrlCmdUtil::SendFileCopy(
	String^ val,
	[Runtime::InteropServices::Out]array<byte>^% resVal
	)
{
	pin_ptr<const wchar_t> valPin = PtrToStringChars(val);
	std::wstring _val(valPin);

	DWORD resValSize = 0;
	BYTE* _resVal = NULL;
	DWORD ret = this->sendCmd->SendFileCopy(_val, &_resVal, &resValSize);
	if( ret == CMD_SUCCESS ){
		resVal = gcnew array<byte>(resValSize);
		System::Runtime::InteropServices::Marshal::Copy(IntPtr(_resVal), resVal, 0, resValSize);
	}

	return ret;
}

/// <summary>
/// 番組情報一覧を取得する
/// </summary>
/// <param name="val">[OUT]番組情報一覧</param>
UInt32 CtrlCmdUtil::SendEnumPgAll(
	List<Def::EpgServiceEventInfo^>^% val
	)
{
	vector<EPGDB_SERVICE_EVENT_INFO*> _val;
	DWORD ret = this->sendCmd->SendEnumPgAll(&_val);
	if( ret == CMD_SUCCESS ){
		for( size_t i=0; i<_val.size(); i++ ){
			Def::EpgServiceEventInfo^ item = gcnew Def::EpgServiceEventInfo();
			CopyData(_val[i], item);
			val->Add(item);
			for( size_t j=0; j<_val[i]->eventList.size(); j++ ){
				SAFE_DELETE(_val[i]->eventList[j]);
			}
			SAFE_DELETE(_val[i]);
		}
	}
	return ret;
}


/// <summary>
/// PlugInファイルの一覧を取得する
/// </summary>
/// <param name="val">[IN]1:ReName、2:Write</param>
/// <param name="resVal">[OUT]ファイル一覧</param>
UInt32 CtrlCmdUtil::SendEnumPlugIn(
	UInt16 val,
	List<String^>^% resVal
	)
{
	vector<wstring> list;
	DWORD ret = this->sendCmd->SendEnumPlugIn(val, &list);
	if( ret == CMD_SUCCESS ){
		for( size_t i=0; i<list.size(); i++ ){
			resVal->Add(gcnew String(list[i].c_str()));
		}
	}

	return ret;
}

/// <summary>
/// TVTestのチャンネル切り替え用の情報を取得する
/// </summary>
/// <param name="val">[IN]ONID＜＜32 | TSID＜＜16 | SIDとしたサービスID</param>
/// <param name="resVal">[OUT]チャンネル情報</param>
UInt32 CtrlCmdUtil::SendGetChgChTVTest(
	UInt64 val,
	Def::TvTestChChgInfo^% resVal
	)
{
	TVTEST_CH_CHG_INFO _resVal;
	DWORD ret = this->sendCmd->SendGetChgChTVTest(val, &_resVal);
	if( ret == CMD_SUCCESS ){
		CopyData(&_resVal, resVal);
	}

	return ret;
}

/// <summary>
/// ネットワークモードのEpgDataCap_Bonのチャンネルを切り替え
/// </summary>
/// <param name="val">[IN]チャンネル情報</param>
UInt32 CtrlCmdUtil::SendNwTVSetCh(
	Def::SetChInfo^ val
	)
{
	SET_CH_INFO chInfo;
	CopyData(val, &chInfo);
	DWORD ret = this->sendCmd->SendNwTVSetCh(&chInfo);

	return ret;
}

/// <summary>
/// ネットワークモードで起動中のEpgDataCap_Bonを終了
/// </summary>
UInt32 CtrlCmdUtil::SendNwTVClose(
	)
{
	DWORD ret = this->sendCmd->SendNwTVClose();

	return ret;
}

/// <summary>
/// ネットワークモードで起動するときのモード
/// </summary>
/// <param name="val">[IN]モード（1:UDP 2:TCP 3:UDP+TCP）</param>
UInt32 CtrlCmdUtil::SendNwTVMode(
	UInt32 val
	)
{
	DWORD ret = this->sendCmd->SendNwTVMode(val);

	return ret;
}

/// <summary>
/// BonDriverの切り替え
/// </summary>
/// <param name="val">[IN]BonDriverファイル名</param>
UInt32 CtrlCmdUtil::SendViewSetBonDrivere(
	String^ val
	)
{
	pin_ptr<const wchar_t> valPin = PtrToStringChars(val);
	std::wstring _val(valPin);

	DWORD ret = this->sendCmd->SendViewSetBonDrivere(valPin);

	return ret;
}

/// <summary>
/// 使用中のBonDriverのファイル名を取得
/// </summary>
/// <param name="val">[OUT]BonDriverファイル名</param>
UInt32 CtrlCmdUtil::SendViewGetBonDrivere(
	String^% val
	)
{
	wstring resVal = L"";
	DWORD ret = this->sendCmd->SendViewGetBonDrivere(&resVal);
	if( ret == CMD_SUCCESS ){
		val = gcnew String(resVal.c_str());
	}

	return ret;
}

/// <summary>
/// チャンネル切り替え
/// </summary>
/// <param name="val">[IN]チャンネル情報</param>
UInt32 CtrlCmdUtil::SendViewSetCh(
	Def::SetChInfo^ val
	)
{
	SET_CH_INFO chInfo;
	CopyData(val, &chInfo);
	DWORD ret = this->sendCmd->SendViewSetCh(&chInfo);

	return ret;
}

/// <summary>
/// アプリケーションの終了
/// </summary>
UInt32 CtrlCmdUtil::SendViewAppClose(
	)
{
	DWORD ret = this->sendCmd->SendViewAppClose();

	return ret;
}

/// <summary>
/// ストリーム配信用ファイルを開く
/// </summary>
/// <param name="val">[IN]開くファイルのサーバー側ファイルパス</param>
/// <param name="resVal">[OUT]制御用CtrlID</param>
UInt32 CtrlCmdUtil::SendNwPlayOpen(
	String^ val,
	UInt32% resVal
	)
{
	pin_ptr<const wchar_t> valPin = PtrToStringChars(val);
	std::wstring _val(valPin);

	DWORD ctrlID = 0;
	DWORD ret = this->sendCmd->SendNwPlayOpen(valPin, &ctrlID);
	resVal = ctrlID;

	return ret;
}

/// <summary>
/// ストリーミング配信制御IDの設定
/// </summary>
/// <param name="val">[IN]ストリーミング配信制御情報</param>
UInt32 CtrlCmdUtil::SendViewSetStreamingInfo(
	Def::TVTestStreamingInfo^ val
	)
{
	TVTEST_STREAMING_INFO info;
	CopyData(val, &info);
	DWORD ret = this->sendCmd->SendViewSetStreamingInfo(&info);

	return ret;
}

/// <summary>
/// ストリーム配信用ファイルをタイムシフトモードで開く
/// </summary>
/// <param name="val">[IN]予約ID</param>
/// <param name="resVal">[OUT]制御用CtrlID</param>
UInt32 CtrlCmdUtil::SendNwTimeShiftOpen(
	UInt32 val,
	Def::NWPlayTimeShiftInfo^% resVal
	)
{
	NWPLAY_TIMESHIFT_INFO _resVal;
	DWORD ret = this->sendCmd->SendNwTimeShiftOpen(val, &_resVal);
	if( ret == CMD_SUCCESS ){
		CopyData(&_resVal, resVal);
	}

	return ret;
}
