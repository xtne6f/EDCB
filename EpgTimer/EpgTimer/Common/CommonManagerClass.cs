using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Media;
using System.Collections;
using System.IO;

namespace EpgTimer
{
    class CommonManager
    {
        public CtrlCmdUtil CtrlCmd
        {
            get;
            set;
        }
        public DBManager DB
        {
            get;
            set;
        }
        public TVTestCtrlClass TVTestCtrl
        {
            get;
            set;
        }
        public Dictionary<UInt16, ContentKindInfo> ContentKindDictionary
        {
            get;
            set;
        }
        public Dictionary<UInt16, ContentKindInfo> ContentKindDictionary2
        {
            get;
            set;
        }
        public Dictionary<UInt16, ComponentKindInfo> ComponentKindDictionary
        {
            get;
            set;
        }
        public Dictionary<byte, DayOfWeekInfo> DayOfWeekDictionary
        {
            get;
            set;
        }
        public Dictionary<UInt16, UInt16> HourDictionary
        {
            get;
            set;
        }
        public Dictionary<UInt16, UInt16> MinDictionary
        {
            get;
            set;
        }
        public Dictionary<byte, RecModeInfo> RecModeDictionary
        {
            get;
            set;
        }
        public Dictionary<byte, YesNoInfo> YesNoDictionary
        {
            get;
            set;
        }
        public Dictionary<byte, PriorityInfo> PriorityDictionary
        {
            get;
            set;
        }
        public bool NWMode
        {
            get;
            set;
        }
        public List<NotifySrvInfo> NotifyLogList
        {
            get;
            set;
        }
        public NWConnect NW
        {
            get;
            set;
        }
        public List<Brush> CustContentColorList
        {
            get;
            set;
        }
        public SolidColorBrush CustTitle1Color
        {
            get;
            set;
        }
        public SolidColorBrush CustTitle2Color
        {
            get;
            set;
        }
        public List<Brush> CustTimeColorList
        {
            get;
            set;
        }
        public Brush CustServiceColor
        {
            get;
            set;
        }

        private static CommonManager _instance;
        public static CommonManager Instance
        {
            get
            {
                if (_instance == null)
                    _instance = new CommonManager();
                return _instance;
            }
            set { _instance = value; }
        }

        public CommonManager()
        {
            if (CtrlCmd == null)
            {
                CtrlCmd = new CtrlCmdUtil();
            }
            if (DB == null)
            {
                DB = new DBManager(CtrlCmd);
            }
            if (TVTestCtrl == null)
            {
                TVTestCtrl = new TVTestCtrlClass(CtrlCmd);
            }
            if (NW == null)
            {
                NW = new NWConnect(CtrlCmd);
            }
            if (ContentKindDictionary == null)
            {
                ContentKindDictionary = new Dictionary<UInt16, ContentKindInfo>();
                ContentKindDictionary.Add(0x00FF, new ContentKindInfo("ニュース／報道", "", 0x00, 0xFF));
                ContentKindDictionary.Add(0x0000, new ContentKindInfo("ニュース／報道", "定時・総合", 0x00, 0x00));
                ContentKindDictionary.Add(0x0001, new ContentKindInfo("ニュース／報道", "天気", 0x00, 0x01));
                ContentKindDictionary.Add(0x0002, new ContentKindInfo("ニュース／報道", "特集・ドキュメント", 0x00, 0x02));
                ContentKindDictionary.Add(0x0003, new ContentKindInfo("ニュース／報道", "政治・国会", 0x00, 0x03));
                ContentKindDictionary.Add(0x0004, new ContentKindInfo("ニュース／報道", "経済・市況", 0x00, 0x04));
                ContentKindDictionary.Add(0x0005, new ContentKindInfo("ニュース／報道", "海外・国際", 0x00, 0x05));
                ContentKindDictionary.Add(0x0006, new ContentKindInfo("ニュース／報道", "解説", 0x00, 0x06));
                ContentKindDictionary.Add(0x0007, new ContentKindInfo("ニュース／報道", "討論・会談", 0x00, 0x07));
                ContentKindDictionary.Add(0x0008, new ContentKindInfo("ニュース／報道", "報道特番", 0x00, 0x08));
                ContentKindDictionary.Add(0x0009, new ContentKindInfo("ニュース／報道", "ローカル・地域", 0x00, 0x09));
                ContentKindDictionary.Add(0x000A, new ContentKindInfo("ニュース／報道", "交通", 0x00, 0x0A));
                ContentKindDictionary.Add(0x000F, new ContentKindInfo("ニュース／報道", "その他", 0x00, 0x0F));

                ContentKindDictionary.Add(0x01FF, new ContentKindInfo("スポーツ", "", 0x01, 0xFF));
                ContentKindDictionary.Add(0x0100, new ContentKindInfo("スポーツ", "スポーツニュース", 0x01, 0x00));
                ContentKindDictionary.Add(0x0101, new ContentKindInfo("スポーツ", "野球", 0x01, 0x01));
                ContentKindDictionary.Add(0x0102, new ContentKindInfo("スポーツ", "サッカー", 0x01, 0x02));
                ContentKindDictionary.Add(0x0103, new ContentKindInfo("スポーツ", "ゴルフ", 0x01, 0x03));
                ContentKindDictionary.Add(0x0104, new ContentKindInfo("スポーツ", "その他の球技", 0x01, 0x04));
                ContentKindDictionary.Add(0x0105, new ContentKindInfo("スポーツ", "相撲・格闘技", 0x01, 0x05));
                ContentKindDictionary.Add(0x0106, new ContentKindInfo("スポーツ", "オリンピック・国際大会", 0x01, 0x06));
                ContentKindDictionary.Add(0x0107, new ContentKindInfo("スポーツ", "マラソン・陸上・水泳", 0x01, 0x07));
                ContentKindDictionary.Add(0x0108, new ContentKindInfo("スポーツ", "モータースポーツ", 0x01, 0x08));
                ContentKindDictionary.Add(0x0109, new ContentKindInfo("スポーツ", "マリン・ウィンタースポーツ", 0x01, 0x09));
                ContentKindDictionary.Add(0x010A, new ContentKindInfo("スポーツ", "競馬・公営競技", 0x01, 0x0A));
                ContentKindDictionary.Add(0x010F, new ContentKindInfo("スポーツ", "その他", 0x01, 0x0F));

                ContentKindDictionary.Add(0x02FF, new ContentKindInfo("情報／ワイドショー", "", 0x02, 0xFF));
                ContentKindDictionary.Add(0x0200, new ContentKindInfo("情報／ワイドショー", "芸能・ワイドショー", 0x02, 0x00));
                ContentKindDictionary.Add(0x0201, new ContentKindInfo("情報／ワイドショー", "ファッション", 0x02, 0x01));
                ContentKindDictionary.Add(0x0202, new ContentKindInfo("情報／ワイドショー", "暮らし・住まい", 0x02, 0x02));
                ContentKindDictionary.Add(0x0203, new ContentKindInfo("情報／ワイドショー", "健康・医療", 0x02, 0x03));
                ContentKindDictionary.Add(0x0204, new ContentKindInfo("情報／ワイドショー", "ショッピング・通販", 0x02, 0x04));
                ContentKindDictionary.Add(0x0205, new ContentKindInfo("情報／ワイドショー", "グルメ・料理", 0x02, 0x05));
                ContentKindDictionary.Add(0x0206, new ContentKindInfo("情報／ワイドショー", "イベント", 0x02, 0x06));
                ContentKindDictionary.Add(0x0207, new ContentKindInfo("情報／ワイドショー", "番組紹介・お知らせ", 0x02, 0x07));
                ContentKindDictionary.Add(0x020F, new ContentKindInfo("情報／ワイドショー", "その他", 0x02, 0x0F));

                ContentKindDictionary.Add(0x03FF, new ContentKindInfo("ドラマ", "", 0x03, 0xFF));
                ContentKindDictionary.Add(0x0300, new ContentKindInfo("ドラマ", "国内ドラマ", 0x03, 0x00));
                ContentKindDictionary.Add(0x0301, new ContentKindInfo("ドラマ", "海外ドラマ", 0x03, 0x01));
                ContentKindDictionary.Add(0x0302, new ContentKindInfo("ドラマ", "時代劇", 0x03, 0x02));
                ContentKindDictionary.Add(0x030F, new ContentKindInfo("ドラマ", "その他", 0x03, 0x0F));

                ContentKindDictionary.Add(0x04FF, new ContentKindInfo("音楽", "", 0x04, 0xFF));
                ContentKindDictionary.Add(0x0400, new ContentKindInfo("音楽", "国内ロック・ポップス", 0x04, 0x00));
                ContentKindDictionary.Add(0x0401, new ContentKindInfo("音楽", "海外ロック・ポップス", 0x04, 0x01));
                ContentKindDictionary.Add(0x0402, new ContentKindInfo("音楽", "クラシック・オペラ", 0x04, 0x02));
                ContentKindDictionary.Add(0x0403, new ContentKindInfo("音楽", "ジャズ・フュージョン", 0x04, 0x03));
                ContentKindDictionary.Add(0x0404, new ContentKindInfo("音楽", "歌謡曲・演歌", 0x04, 0x04));
                ContentKindDictionary.Add(0x0405, new ContentKindInfo("音楽", "ライブ・コンサート", 0x04, 0x05));
                ContentKindDictionary.Add(0x0406, new ContentKindInfo("音楽", "ランキング・リクエスト", 0x04, 0x06));
                ContentKindDictionary.Add(0x0407, new ContentKindInfo("音楽", "カラオケ・のど自慢", 0x04, 0x07));
                ContentKindDictionary.Add(0x0408, new ContentKindInfo("音楽", "民謡・邦楽", 0x04, 0x08));
                ContentKindDictionary.Add(0x0409, new ContentKindInfo("音楽", "童謡・キッズ", 0x04, 0x09));
                ContentKindDictionary.Add(0x040A, new ContentKindInfo("音楽", "民族音楽・ワールドミュージック", 0x04, 0x0A));
                ContentKindDictionary.Add(0x040F, new ContentKindInfo("音楽", "その他", 0x04, 0x0F));

                ContentKindDictionary.Add(0x05FF, new ContentKindInfo("バラエティ", "", 0x05, 0xFF));
                ContentKindDictionary.Add(0x0500, new ContentKindInfo("バラエティ", "クイズ", 0x05, 0x00));
                ContentKindDictionary.Add(0x0501, new ContentKindInfo("バラエティ", "ゲーム", 0x05, 0x01));
                ContentKindDictionary.Add(0x0502, new ContentKindInfo("バラエティ", "トークバラエティ", 0x05, 0x02));
                ContentKindDictionary.Add(0x0503, new ContentKindInfo("バラエティ", "お笑い・コメディ", 0x05, 0x03));
                ContentKindDictionary.Add(0x0504, new ContentKindInfo("バラエティ", "音楽バラエティ", 0x05, 0x04));
                ContentKindDictionary.Add(0x0505, new ContentKindInfo("バラエティ", "旅バラエティ", 0x05, 0x05));
                ContentKindDictionary.Add(0x0506, new ContentKindInfo("バラエティ", "料理バラエティ", 0x05, 0x06));
                ContentKindDictionary.Add(0x050F, new ContentKindInfo("バラエティ", "その他", 0x05, 0x0F));

                ContentKindDictionary.Add(0x06FF, new ContentKindInfo("映画", "", 0x06, 0xFF));
                ContentKindDictionary.Add(0x0600, new ContentKindInfo("映画", "洋画", 0x06, 0x00));
                ContentKindDictionary.Add(0x0601, new ContentKindInfo("映画", "邦画", 0x06, 0x01));
                ContentKindDictionary.Add(0x0602, new ContentKindInfo("映画", "アニメ", 0x06, 0x02));
                ContentKindDictionary.Add(0x060F, new ContentKindInfo("映画", "その他", 0x06, 0x0F));

                ContentKindDictionary.Add(0x07FF, new ContentKindInfo("アニメ／特撮", "", 0x07, 0xFF));
                ContentKindDictionary.Add(0x0700, new ContentKindInfo("アニメ／特撮", "国内アニメ", 0x07, 0x00));
                ContentKindDictionary.Add(0x0701, new ContentKindInfo("アニメ／特撮", "海外アニメ", 0x07, 0x01));
                ContentKindDictionary.Add(0x0702, new ContentKindInfo("アニメ／特撮", "特撮", 0x07, 0x02));
                ContentKindDictionary.Add(0x070F, new ContentKindInfo("アニメ／特撮", "その他", 0x07, 0x0F));

                ContentKindDictionary.Add(0x08FF, new ContentKindInfo("ドキュメンタリー／教養", "", 0x08, 0xFF));
                ContentKindDictionary.Add(0x0800, new ContentKindInfo("ドキュメンタリー／教養", "社会・時事", 0x08, 0x00));
                ContentKindDictionary.Add(0x0801, new ContentKindInfo("ドキュメンタリー／教養", "歴史・紀行", 0x08, 0x01));
                ContentKindDictionary.Add(0x0802, new ContentKindInfo("ドキュメンタリー／教養", "自然・動物・環境", 0x08, 0x02));
                ContentKindDictionary.Add(0x0803, new ContentKindInfo("ドキュメンタリー／教養", "宇宙・科学・医学", 0x08, 0x03));
                ContentKindDictionary.Add(0x0804, new ContentKindInfo("ドキュメンタリー／教養", "カルチャー・伝統文化", 0x08, 0x04));
                ContentKindDictionary.Add(0x0805, new ContentKindInfo("ドキュメンタリー／教養", "文学・文芸", 0x08, 0x05));
                ContentKindDictionary.Add(0x0806, new ContentKindInfo("ドキュメンタリー／教養", "スポーツ", 0x08, 0x06));
                ContentKindDictionary.Add(0x0807, new ContentKindInfo("ドキュメンタリー／教養", "ドキュメンタリー全般", 0x08, 0x07));
                ContentKindDictionary.Add(0x0808, new ContentKindInfo("ドキュメンタリー／教養", "インタビュー・討論", 0x08, 0x08));
                ContentKindDictionary.Add(0x080F, new ContentKindInfo("ドキュメンタリー／教養", "その他", 0x08, 0x0F));

                ContentKindDictionary.Add(0x09FF, new ContentKindInfo("劇場／公演", "", 0x09, 0xFF));
                ContentKindDictionary.Add(0x0900, new ContentKindInfo("劇場／公演", "現代劇・新劇", 0x09, 0x00));
                ContentKindDictionary.Add(0x0901, new ContentKindInfo("劇場／公演", "ミュージカル", 0x09, 0x01));
                ContentKindDictionary.Add(0x0902, new ContentKindInfo("劇場／公演", "ダンス・バレエ", 0x09, 0x02));
                ContentKindDictionary.Add(0x0903, new ContentKindInfo("劇場／公演", "落語・演芸", 0x09, 0x03));
                ContentKindDictionary.Add(0x0904, new ContentKindInfo("劇場／公演", "歌舞伎・古典", 0x09, 0x04));
                ContentKindDictionary.Add(0x090F, new ContentKindInfo("劇場／公演", "その他", 0x09, 0x0F));

                ContentKindDictionary.Add(0x0AFF, new ContentKindInfo("趣味／教育", "", 0x0A, 0xFF));
                ContentKindDictionary.Add(0x0A00, new ContentKindInfo("趣味／教育", "旅・釣り・アウトドア", 0x0A, 0x00));
                ContentKindDictionary.Add(0x0A01, new ContentKindInfo("趣味／教育", "園芸・ペット・手芸", 0x0A, 0x01));
                ContentKindDictionary.Add(0x0A02, new ContentKindInfo("趣味／教育", "音楽・美術・工芸", 0x0A, 0x02));
                ContentKindDictionary.Add(0x0A03, new ContentKindInfo("趣味／教育", "囲碁・将棋", 0x0A, 0x03));
                ContentKindDictionary.Add(0x0A04, new ContentKindInfo("趣味／教育", "麻雀・パチンコ", 0x0A, 0x04));
                ContentKindDictionary.Add(0x0A05, new ContentKindInfo("趣味／教育", "車・オートバイ", 0x0A, 0x05));
                ContentKindDictionary.Add(0x0A06, new ContentKindInfo("趣味／教育", "コンピュータ・ＴＶゲーム", 0x0A, 0x06));
                ContentKindDictionary.Add(0x0A07, new ContentKindInfo("趣味／教育", "会話・語学", 0x0A, 0x07));
                ContentKindDictionary.Add(0x0A08, new ContentKindInfo("趣味／教育", "幼児・小学生", 0x0A, 0x08));
                ContentKindDictionary.Add(0x0A09, new ContentKindInfo("趣味／教育", "中学生・高校生", 0x0A, 0x09));
                ContentKindDictionary.Add(0x0A0A, new ContentKindInfo("趣味／教育", "大学生・受験", 0x0A, 0x0A));
                ContentKindDictionary.Add(0x0A0B, new ContentKindInfo("趣味／教育", "生涯教育・資格", 0x0A, 0x0B));
                ContentKindDictionary.Add(0x0A0C, new ContentKindInfo("趣味／教育", "教育問題", 0x0A, 0x0C));
                ContentKindDictionary.Add(0x0A0F, new ContentKindInfo("趣味／教育", "その他", 0x0A, 0x0F));

                ContentKindDictionary.Add(0x0BFF, new ContentKindInfo("福祉", "", 0x0B, 0xFF));
                ContentKindDictionary.Add(0x0B00, new ContentKindInfo("福祉", "高齢者", 0x0B, 0x00));
                ContentKindDictionary.Add(0x0B01, new ContentKindInfo("福祉", "障害者", 0x0B, 0x01));
                ContentKindDictionary.Add(0x0B02, new ContentKindInfo("福祉", "社会福祉", 0x0B, 0x02));
                ContentKindDictionary.Add(0x0B03, new ContentKindInfo("福祉", "ボランティア", 0x0B, 0x03));
                ContentKindDictionary.Add(0x0B04, new ContentKindInfo("福祉", "手話", 0x0B, 0x04));
                ContentKindDictionary.Add(0x0B05, new ContentKindInfo("福祉", "文字（字幕）", 0x0B, 0x05));
                ContentKindDictionary.Add(0x0B06, new ContentKindInfo("福祉", "音声解説", 0x0B, 0x06));
                ContentKindDictionary.Add(0x0B0F, new ContentKindInfo("福祉", "その他", 0x0B, 0x0F));

                ContentKindDictionary.Add(0x0FFF, new ContentKindInfo("その他", "", 0x0F, 0xFF));
                ContentKindDictionary.Add(0xFFFF, new ContentKindInfo("なし", "", 0xFF, 0xFF));
            }
            if (ContentKindDictionary2 == null)
            {
                ContentKindDictionary2 = new Dictionary<UInt16, ContentKindInfo>();
                ContentKindDictionary2.Add(0x00FF, new ContentKindInfo("スポーツ(CS)", "", 0x00, 0xFF));
                ContentKindDictionary2.Add(0x0000, new ContentKindInfo("スポーツ(CS)", "テニス", 0x00, 0x00));
                ContentKindDictionary2.Add(0x0001, new ContentKindInfo("スポーツ(CS)", "バスケットボール", 0x00, 0x01));
                ContentKindDictionary2.Add(0x0002, new ContentKindInfo("スポーツ(CS)", "ラグビー", 0x00, 0x02));
                ContentKindDictionary2.Add(0x0003, new ContentKindInfo("スポーツ(CS)", "アメリカンフットボール", 0x00, 0x03));
                ContentKindDictionary2.Add(0x0004, new ContentKindInfo("スポーツ(CS)", "ボクシング", 0x00, 0x04));
                ContentKindDictionary2.Add(0x0005, new ContentKindInfo("スポーツ(CS)", "プロレス", 0x00, 0x05));
                ContentKindDictionary2.Add(0x000F, new ContentKindInfo("スポーツ(CS)", "その他", 0x00, 0x0F));

                ContentKindDictionary2.Add(0x01FF, new ContentKindInfo("洋画(CS)", "", 0x01, 0xFF));
                ContentKindDictionary2.Add(0x0100, new ContentKindInfo("洋画(CS)", "アクション", 0x01, 0x00));
                ContentKindDictionary2.Add(0x0101, new ContentKindInfo("洋画(CS)", "SF／ファンタジー", 0x01, 0x01));
                ContentKindDictionary2.Add(0x0102, new ContentKindInfo("洋画(CS)", "コメディー", 0x01, 0x02));
                ContentKindDictionary2.Add(0x0103, new ContentKindInfo("洋画(CS)", "サスペンス／ミステリー", 0x01, 0x03));
                ContentKindDictionary2.Add(0x0104, new ContentKindInfo("洋画(CS)", "恋愛／ロマンス", 0x01, 0x04));
                ContentKindDictionary2.Add(0x0105, new ContentKindInfo("洋画(CS)", "ホラー／スリラー", 0x01, 0x05));
                ContentKindDictionary2.Add(0x0106, new ContentKindInfo("洋画(CS)", "ウエスタン", 0x01, 0x06));
                ContentKindDictionary2.Add(0x0107, new ContentKindInfo("洋画(CS)", "ドラマ／社会派ドラマ", 0x01, 0x07));
                ContentKindDictionary2.Add(0x0108, new ContentKindInfo("洋画(CS)", "アニメーション", 0x01, 0x08));
                ContentKindDictionary2.Add(0x0109, new ContentKindInfo("洋画(CS)", "ドキュメンタリー", 0x01, 0x09));
                ContentKindDictionary2.Add(0x010A, new ContentKindInfo("洋画(CS)", "アドベンチャー／冒険", 0x01, 0x0A));
                ContentKindDictionary2.Add(0x010B, new ContentKindInfo("洋画(CS)", "ミュージカル／音楽映画", 0x01, 0x0B));
                ContentKindDictionary2.Add(0x010C, new ContentKindInfo("洋画(CS)", "ホームドラマ", 0x01, 0x0C));
                ContentKindDictionary2.Add(0x010F, new ContentKindInfo("洋画(CS)", "その他", 0x01, 0x0F));

                ContentKindDictionary2.Add(0x02FF, new ContentKindInfo("邦画(CS)", "", 0x02, 0xFF));
                ContentKindDictionary2.Add(0x0200, new ContentKindInfo("邦画(CS)", "アクション", 0x02, 0x00));
                ContentKindDictionary2.Add(0x0201, new ContentKindInfo("邦画(CS)", "SF／ファンタジー", 0x02, 0x01));
                ContentKindDictionary2.Add(0x0202, new ContentKindInfo("邦画(CS)", "お笑い／コメディー", 0x02, 0x02));
                ContentKindDictionary2.Add(0x0203, new ContentKindInfo("邦画(CS)", "サスペンス／ミステリー", 0x02, 0x03));
                ContentKindDictionary2.Add(0x0204, new ContentKindInfo("邦画(CS)", "恋愛／ロマンス", 0x02, 0x04));
                ContentKindDictionary2.Add(0x0205, new ContentKindInfo("邦画(CS)", "ホラー／スリラー", 0x02, 0x05));
                ContentKindDictionary2.Add(0x0206, new ContentKindInfo("邦画(CS)", "青春／学園／アイドル", 0x02, 0x06));
                ContentKindDictionary2.Add(0x0207, new ContentKindInfo("邦画(CS)", "任侠／時代劇", 0x02, 0x07));
                ContentKindDictionary2.Add(0x0208, new ContentKindInfo("邦画(CS)", "アニメーション", 0x02, 0x08));
                ContentKindDictionary2.Add(0x0209, new ContentKindInfo("邦画(CS)", "ドキュメンタリー", 0x02, 0x09));
                ContentKindDictionary2.Add(0x020A, new ContentKindInfo("邦画(CS)", "アドベンチャー／冒険", 0x02, 0x0A));
                ContentKindDictionary2.Add(0x020B, new ContentKindInfo("邦画(CS)", "ミュージカル／音楽映画", 0x02, 0x0B));
                ContentKindDictionary2.Add(0x020C, new ContentKindInfo("邦画(CS)", "ホームドラマ", 0x02, 0x0C));
                ContentKindDictionary2.Add(0x020F, new ContentKindInfo("邦画(CS)", "その他", 0x02, 0x0F));
            }
            if (ComponentKindDictionary == null)
            {
                ComponentKindDictionary = new Dictionary<UInt16, ComponentKindInfo>();
                ComponentKindDictionary.Add(0x0101, new ComponentKindInfo(0x01, 0x01, "480i(525i)、アスペクト比4:3"));
                ComponentKindDictionary.Add(0x0102, new ComponentKindInfo(0x01, 0x02, "480i(525i)、アスペクト比16:9 パンベクトルあり"));
                ComponentKindDictionary.Add(0x0103, new ComponentKindInfo(0x01, 0x03, "480i(525i)、アスペクト比16:9 パンベクトルなし"));
                ComponentKindDictionary.Add(0x0104, new ComponentKindInfo(0x01, 0x04, "480i(525i)、アスペクト比 > 16:9"));
                ComponentKindDictionary.Add(0x0191, new ComponentKindInfo(0x01, 0x91, "2160p、アスペクト比4:3"));
                ComponentKindDictionary.Add(0x0192, new ComponentKindInfo(0x01, 0x92, "2160p、アスペクト比16:9 パンベクトルあり"));
                ComponentKindDictionary.Add(0x0193, new ComponentKindInfo(0x01, 0x93, "2160p、アスペクト比16:9 パンベクトルなし"));
                ComponentKindDictionary.Add(0x0194, new ComponentKindInfo(0x01, 0x94, "2160p、アスペクト比 > 16:9"));
                ComponentKindDictionary.Add(0x01A1, new ComponentKindInfo(0x01, 0xA1, "480p(525p)、アスペクト比4:3"));
                ComponentKindDictionary.Add(0x01A2, new ComponentKindInfo(0x01, 0xA2, "480p(525p)、アスペクト比16:9 パンベクトルあり"));
                ComponentKindDictionary.Add(0x01A3, new ComponentKindInfo(0x01, 0xA3, "480p(525p)、アスペクト比16:9 パンベクトルなし"));
                ComponentKindDictionary.Add(0x01A4, new ComponentKindInfo(0x01, 0xA4, "480p(525p)、アスペクト比 > 16:9"));
                ComponentKindDictionary.Add(0x01B1, new ComponentKindInfo(0x01, 0xB1, "1080i(1125i)、アスペクト比4:3"));
                ComponentKindDictionary.Add(0x01B2, new ComponentKindInfo(0x01, 0xB2, "1080i(1125i)、アスペクト比16:9 パンベクトルあり"));
                ComponentKindDictionary.Add(0x01B3, new ComponentKindInfo(0x01, 0xB3, "1080i(1125i)、アスペクト比16:9 パンベクトルなし"));
                ComponentKindDictionary.Add(0x01B4, new ComponentKindInfo(0x01, 0xB4, "1080i(1125i)、アスペクト比 > 16:9"));
                ComponentKindDictionary.Add(0x01C1, new ComponentKindInfo(0x01, 0xC1, "720p(750p)、アスペクト比4:3"));
                ComponentKindDictionary.Add(0x01C2, new ComponentKindInfo(0x01, 0xC2, "720p(750p)、アスペクト比16:9 パンベクトルあり"));
                ComponentKindDictionary.Add(0x01C3, new ComponentKindInfo(0x01, 0xC3, "720p(750p)、アスペクト比16:9 パンベクトルなし"));
                ComponentKindDictionary.Add(0x01C4, new ComponentKindInfo(0x01, 0xC4, "720p(750p)、アスペクト比 > 16:9"));
                ComponentKindDictionary.Add(0x01D1, new ComponentKindInfo(0x01, 0xD1, "240p アスペクト比4:3"));
                ComponentKindDictionary.Add(0x01D2, new ComponentKindInfo(0x01, 0xD2, "240p アスペクト比16:9 パンベクトルあり"));
                ComponentKindDictionary.Add(0x01D3, new ComponentKindInfo(0x01, 0xD3, "240p アスペクト比16:9 パンベクトルなし"));
                ComponentKindDictionary.Add(0x01D4, new ComponentKindInfo(0x01, 0xD4, "240p アスペクト比 > 16:9"));
                ComponentKindDictionary.Add(0x01E1, new ComponentKindInfo(0x01, 0xE1, "1080p(1125p)、アスペクト比4:3"));
                ComponentKindDictionary.Add(0x01E2, new ComponentKindInfo(0x01, 0xE2, "1080p(1125p)、アスペクト比16:9 パンベクトルあり"));
                ComponentKindDictionary.Add(0x01E3, new ComponentKindInfo(0x01, 0xE3, "1080p(1125p)、アスペクト比16:9 パンベクトルなし"));
                ComponentKindDictionary.Add(0x01E4, new ComponentKindInfo(0x01, 0xE4, "1080p(1125p)、アスペクト比 > 16:9"));
                ComponentKindDictionary.Add(0x0201, new ComponentKindInfo(0x02, 0x01, "1/0モード（シングルモノ）"));
                ComponentKindDictionary.Add(0x0202, new ComponentKindInfo(0x02, 0x02, "1/0＋1/0モード（デュアルモノ）"));
                ComponentKindDictionary.Add(0x0203, new ComponentKindInfo(0x02, 0x03, "2/0モード（ステレオ）"));
                ComponentKindDictionary.Add(0x0204, new ComponentKindInfo(0x02, 0x04, "2/1モード"));
                ComponentKindDictionary.Add(0x0205, new ComponentKindInfo(0x02, 0x05, "3/0モード"));
                ComponentKindDictionary.Add(0x0206, new ComponentKindInfo(0x02, 0x06, "2/2モード"));
                ComponentKindDictionary.Add(0x0207, new ComponentKindInfo(0x02, 0x07, "3/1モード"));
                ComponentKindDictionary.Add(0x0208, new ComponentKindInfo(0x02, 0x08, "3/2モード"));
                ComponentKindDictionary.Add(0x0209, new ComponentKindInfo(0x02, 0x09, "3/2＋LFEモード（3/2.1モード）"));
                ComponentKindDictionary.Add(0x020A, new ComponentKindInfo(0x02, 0x0A, "3/3.1モード"));
                ComponentKindDictionary.Add(0x020B, new ComponentKindInfo(0x02, 0x0B, "2/0/0-2/0/2-0.1モード"));
                ComponentKindDictionary.Add(0x020C, new ComponentKindInfo(0x02, 0x0C, "5/2.1モード"));
                ComponentKindDictionary.Add(0x020D, new ComponentKindInfo(0x02, 0x0D, "3/2/2.1モード"));
                ComponentKindDictionary.Add(0x020E, new ComponentKindInfo(0x02, 0x0E, "2/0/0-3/0/2-0.1モード"));
                ComponentKindDictionary.Add(0x020F, new ComponentKindInfo(0x02, 0x0F, "0/2/0-3/0/2-0.1モード"));
                ComponentKindDictionary.Add(0x0210, new ComponentKindInfo(0x02, 0x10, "2/0/0-3/2/3-0.2モード"));
                ComponentKindDictionary.Add(0x0211, new ComponentKindInfo(0x02, 0x11, "3/3/3-5/2/3-3/0/0.2モード"));
                ComponentKindDictionary.Add(0x0240, new ComponentKindInfo(0x02, 0x40, "視覚障害者用音声解説"));
                ComponentKindDictionary.Add(0x0241, new ComponentKindInfo(0x02, 0x41, "聴覚障害者用音声"));
                ComponentKindDictionary.Add(0x0501, new ComponentKindInfo(0x05, 0x01, "H.264|MPEG-4 AVC、480i(525i)、アスペクト比4:3"));
                ComponentKindDictionary.Add(0x0502, new ComponentKindInfo(0x05, 0x02, "H.264|MPEG-4 AVC、480i(525i)、アスペクト比16:9 パンベクトルあり"));
                ComponentKindDictionary.Add(0x0503, new ComponentKindInfo(0x05, 0x03, "H.264|MPEG-4 AVC、480i(525i)、アスペクト比16:9 パンベクトルなし"));
                ComponentKindDictionary.Add(0x0504, new ComponentKindInfo(0x05, 0x04, "H.264|MPEG-4 AVC、480i(525i)、アスペクト比 > 16:9"));
                ComponentKindDictionary.Add(0x0591, new ComponentKindInfo(0x05, 0x91, "H.264|MPEG-4 AVC、2160p、アスペクト比4:3"));
                ComponentKindDictionary.Add(0x0592, new ComponentKindInfo(0x05, 0x92, "H.264|MPEG-4 AVC、2160p、アスペクト比16:9 パンベクトルあり"));
                ComponentKindDictionary.Add(0x0593, new ComponentKindInfo(0x05, 0x93, "H.264|MPEG-4 AVC、2160p、アスペクト比16:9 パンベクトルなし"));
                ComponentKindDictionary.Add(0x0594, new ComponentKindInfo(0x05, 0x94, "H.264|MPEG-4 AVC、2160p、アスペクト比 > 16:9"));
                ComponentKindDictionary.Add(0x05A1, new ComponentKindInfo(0x05, 0xA1, "H.264|MPEG-4 AVC、480p(525p)、アスペクト比4:3"));
                ComponentKindDictionary.Add(0x05A2, new ComponentKindInfo(0x05, 0xA2, "H.264|MPEG-4 AVC、480p(525p)、アスペクト比16:9 パンベクトルあり"));
                ComponentKindDictionary.Add(0x05A3, new ComponentKindInfo(0x05, 0xA3, "H.264|MPEG-4 AVC、480p(525p)、アスペクト比16:9 パンベクトルなし"));
                ComponentKindDictionary.Add(0x05A4, new ComponentKindInfo(0x05, 0xA4, "H.264|MPEG-4 AVC、480p(525p)、アスペクト比 > 16:9"));
                ComponentKindDictionary.Add(0x05B1, new ComponentKindInfo(0x05, 0xB1, "H.264|MPEG-4 AVC、1080i(1125i)、アスペクト比4:3"));
                ComponentKindDictionary.Add(0x05B2, new ComponentKindInfo(0x05, 0xB2, "H.264|MPEG-4 AVC、1080i(1125i)、アスペクト比16:9 パンベクトルあり"));
                ComponentKindDictionary.Add(0x05B3, new ComponentKindInfo(0x05, 0xB3, "H.264|MPEG-4 AVC、1080i(1125i)、アスペクト比16:9 パンベクトルなし"));
                ComponentKindDictionary.Add(0x05B4, new ComponentKindInfo(0x05, 0xB4, "H.264|MPEG-4 AVC、1080i(1125i)、アスペクト比 > 16:9"));
                ComponentKindDictionary.Add(0x05C1, new ComponentKindInfo(0x05, 0xC1, "H.264|MPEG-4 AVC、720p(750p)、アスペクト比4:3"));
                ComponentKindDictionary.Add(0x05C2, new ComponentKindInfo(0x05, 0xC2, "H.264|MPEG-4 AVC、720p(750p)、アスペクト比16:9 パンベクトルあり"));
                ComponentKindDictionary.Add(0x05C3, new ComponentKindInfo(0x05, 0xC3, "H.264|MPEG-4 AVC、720p(750p)、アスペクト比16:9 パンベクトルなし"));
                ComponentKindDictionary.Add(0x05C4, new ComponentKindInfo(0x05, 0xC4, "H.264|MPEG-4 AVC、720p(750p)、アスペクト比 > 16:9"));
                ComponentKindDictionary.Add(0x05D1, new ComponentKindInfo(0x05, 0xD1, "H.264|MPEG-4 AVC、240p アスペクト比4:3"));
                ComponentKindDictionary.Add(0x05D2, new ComponentKindInfo(0x05, 0xD2, "H.264|MPEG-4 AVC、240p アスペクト比16:9 パンベクトルあり"));
                ComponentKindDictionary.Add(0x05D3, new ComponentKindInfo(0x05, 0xD3, "H.264|MPEG-4 AVC、240p アスペクト比16:9 パンベクトルなし"));
                ComponentKindDictionary.Add(0x05D4, new ComponentKindInfo(0x05, 0xD4, "H.264|MPEG-4 AVC、240p アスペクト比 > 16:9"));
                ComponentKindDictionary.Add(0x05E1, new ComponentKindInfo(0x05, 0xE1, "H.264|MPEG-4 AVC、1080p(1125p)、アスペクト比4:3"));
                ComponentKindDictionary.Add(0x05E2, new ComponentKindInfo(0x05, 0xE2, "H.264|MPEG-4 AVC、1080p(1125p)、アスペクト比16:9 パンベクトルあり"));
                ComponentKindDictionary.Add(0x05E3, new ComponentKindInfo(0x05, 0xE3, "H.264|MPEG-4 AVC、1080p(1125p)、アスペクト比16:9 パンベクトルなし"));
                ComponentKindDictionary.Add(0x05E4, new ComponentKindInfo(0x05, 0xE4, "H.264|MPEG-4 AVC、1080p(1125p)、アスペクト比 > 16:9"));
            }
            if (DayOfWeekDictionary == null)
            {
                DayOfWeekDictionary = new Dictionary<byte, DayOfWeekInfo>();
                DayOfWeekDictionary.Add(0x00, new DayOfWeekInfo("日", 0x00));
                DayOfWeekDictionary.Add(0x01, new DayOfWeekInfo("月", 0x01));
                DayOfWeekDictionary.Add(0x02, new DayOfWeekInfo("火", 0x02));
                DayOfWeekDictionary.Add(0x03, new DayOfWeekInfo("水", 0x03));
                DayOfWeekDictionary.Add(0x04, new DayOfWeekInfo("木", 0x04));
                DayOfWeekDictionary.Add(0x05, new DayOfWeekInfo("金", 0x05));
                DayOfWeekDictionary.Add(0x06, new DayOfWeekInfo("土", 0x06));
            }
            if (HourDictionary == null)
            {
                HourDictionary = new Dictionary<UInt16, UInt16>();
                for (UInt16 i = 0; i <= 23; i++)
                {
                    HourDictionary.Add(i, i);
                }
            }
            if (MinDictionary == null)
            {
                MinDictionary = new Dictionary<UInt16, UInt16>();
                for (UInt16 i = 0; i <= 59; i++)
                {
                    MinDictionary.Add(i, i);
                }
            }
            if (RecModeDictionary == null)
            {
                RecModeDictionary = new Dictionary<byte, RecModeInfo>();
                RecModeDictionary.Add(0x00, new RecModeInfo("全サービス", 0x00));
                RecModeDictionary.Add(0x01, new RecModeInfo("指定サービス", 0x01));
                RecModeDictionary.Add(0x02, new RecModeInfo("全サービス（デコード処理なし）", 0x02));
                RecModeDictionary.Add(0x03, new RecModeInfo("指定サービス（デコード処理なし）", 0x03));
                RecModeDictionary.Add(0x04, new RecModeInfo("視聴", 0x04));
                RecModeDictionary.Add(0x05, new RecModeInfo("無効", 0x05));
            }
            if (YesNoDictionary == null)
            {
                YesNoDictionary = new Dictionary<byte, YesNoInfo>();
                YesNoDictionary.Add(0x00, new YesNoInfo("しない", 0x00));
                YesNoDictionary.Add(0x01, new YesNoInfo("する", 0x01));
            }
            if (PriorityDictionary == null)
            {
                PriorityDictionary = new Dictionary<byte, PriorityInfo>();
                PriorityDictionary.Add(0x01, new PriorityInfo("1 (低)", 0x01));
                PriorityDictionary.Add(0x02, new PriorityInfo("2", 0x02));
                PriorityDictionary.Add(0x03, new PriorityInfo("3", 0x03));
                PriorityDictionary.Add(0x04, new PriorityInfo("4", 0x04));
                PriorityDictionary.Add(0x05, new PriorityInfo("5 (高)", 0x05));
            }
            NWMode = false;
            if (NotifyLogList == null)
            {
                NotifyLogList = new List<NotifySrvInfo>();
            }
            if( CustContentColorList == null )
            {
                CustContentColorList = new List<Brush>();
            }
            if (CustTimeColorList == null)
            {
                CustTimeColorList = new List<Brush>();
            }
        }

        public static UInt64 Create64Key(UInt16 ONID, UInt16 TSID, UInt16 SID)
        {
            UInt64 key = ((UInt64)ONID) << 32 | ((UInt64)TSID) << 16 | (UInt64)SID;
            return key;
        }

        public static UInt64 Create64PgKey(UInt16 ONID, UInt16 TSID, UInt16 SID, UInt16 EventID)
        {
            UInt64 key = ((UInt64)ONID) << 48 | ((UInt64)TSID) << 32 | ((UInt64)SID) << 16 | (UInt64)EventID;
            return key;
        }

        public static UInt32 CreateARGBKey(byte a, byte r, byte g, byte b)
        {
            UInt32 key = ((UInt32)a) << 24 | ((UInt32)r) << 16 | ((UInt32)g) << 8 | (UInt32)b;
            return key;
        }

        public static EpgServiceInfo ConvertChSet5To(ChSet5Item item)
        {
            EpgServiceInfo info = new EpgServiceInfo();
            try
            {
                info.ONID = item.ONID;
                info.TSID = item.TSID;
                info.SID = item.SID;
                info.network_name = item.NetworkName;
                info.partialReceptionFlag = item.PartialFlag;
                info.remote_control_key_id = item.RemoconID;
                info.service_name = item.ServiceName;
                info.service_provider_name = item.NetworkName;
                info.service_type = (byte)item.ServiceType;
                info.ts_name = item.NetworkName;
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            } 
            return info;
        }

        public static String ReplaceUrl(String url)
        {
            string retText = url;

            retText = retText.Replace("ａ", "a");
            retText = retText.Replace("ｂ", "b");
            retText = retText.Replace("ｃ", "c");
            retText = retText.Replace("ｄ", "d");
            retText = retText.Replace("ｅ", "e");
            retText = retText.Replace("ｆ", "f");
            retText = retText.Replace("ｇ", "g");
            retText = retText.Replace("ｈ", "h");
            retText = retText.Replace("ｉ", "i");
            retText = retText.Replace("ｊ", "j");
            retText = retText.Replace("ｋ", "k");
            retText = retText.Replace("ｌ", "l");
            retText = retText.Replace("ｍ", "m");
            retText = retText.Replace("ｎ", "n");
            retText = retText.Replace("ｏ", "o");
            retText = retText.Replace("ｐ", "p");
            retText = retText.Replace("ｑ", "q");
            retText = retText.Replace("ｒ", "r");
            retText = retText.Replace("ｓ", "s");
            retText = retText.Replace("ｔ", "t");
            retText = retText.Replace("ｕ", "u");
            retText = retText.Replace("ｖ", "v");
            retText = retText.Replace("ｗ", "w");
            retText = retText.Replace("ｘ", "x");
            retText = retText.Replace("ｙ", "y");
            retText = retText.Replace("ｚ", "z");
            retText = retText.Replace("Ａ", "A");
            retText = retText.Replace("Ｂ", "B");
            retText = retText.Replace("Ｃ", "C");
            retText = retText.Replace("Ｄ", "D");
            retText = retText.Replace("Ｅ", "E");
            retText = retText.Replace("Ｆ", "F");
            retText = retText.Replace("Ｇ", "G");
            retText = retText.Replace("Ｈ", "H");
            retText = retText.Replace("Ｉ", "I");
            retText = retText.Replace("Ｊ", "J");
            retText = retText.Replace("Ｋ", "K");
            retText = retText.Replace("Ｌ", "L");
            retText = retText.Replace("Ｍ", "M");
            retText = retText.Replace("Ｎ", "N");
            retText = retText.Replace("Ｏ", "O");
            retText = retText.Replace("Ｐ", "P");
            retText = retText.Replace("Ｑ", "Q");
            retText = retText.Replace("Ｒ", "R");
            retText = retText.Replace("Ｓ", "S");
            retText = retText.Replace("Ｔ", "T");
            retText = retText.Replace("Ｕ", "U");
            retText = retText.Replace("Ｖ", "V");
            retText = retText.Replace("Ｗ", "W");
            retText = retText.Replace("Ｘ", "X");
            retText = retText.Replace("Ｙ", "Y");
            retText = retText.Replace("Ｚ", "Z");
            retText = retText.Replace("＃", "#");
            retText = retText.Replace("＄", "$");
            retText = retText.Replace("％", "%");
            retText = retText.Replace("＆", "&");
            retText = retText.Replace("’", "'");
            retText = retText.Replace("（", "(");
            retText = retText.Replace("）", ")");
            retText = retText.Replace("～", "~");
            retText = retText.Replace("＝", "=");
            retText = retText.Replace("｜", "|");
            retText = retText.Replace("＾", "^");
            retText = retText.Replace("￥", "\\");
            retText = retText.Replace("＠", "@");
            retText = retText.Replace("；", ";");
            retText = retText.Replace("：", ":");
            retText = retText.Replace("｀", "`");
            retText = retText.Replace("｛", "{");
            retText = retText.Replace("｝", "}");
            retText = retText.Replace("＜", "<");
            retText = retText.Replace("＞", ">");
            retText = retText.Replace("？", "?");
            retText = retText.Replace("＿", "_");
            retText = retText.Replace("＋", "+");
            retText = retText.Replace("－", "-");
            retText = retText.Replace("＊", "*");
            retText = retText.Replace("／", "/");
            retText = retText.Replace("．", ".");
            retText = retText.Replace("０", "0");
            retText = retText.Replace("１", "1");
            retText = retText.Replace("２", "2");
            retText = retText.Replace("３", "3");
            retText = retText.Replace("４", "4");
            retText = retText.Replace("５", "5");
            retText = retText.Replace("６", "6");
            retText = retText.Replace("７", "7");
            retText = retText.Replace("８", "8");
            retText = retText.Replace("９", "9");

            return retText;
        }

        public String ConvertReserveText(ReserveData reserveInfo)
        {
            String view = "";
            view = reserveInfo.StartTime.ToString("yyyy/MM/dd(ddd) HH:mm:ss ～ ");
            DateTime endTime = reserveInfo.StartTime + TimeSpan.FromSeconds(reserveInfo.DurationSecond);
            view += endTime.ToString("yyyy/MM/dd(ddd) HH:mm:ss") + "\r\n";

            String recMode = "";
            switch (reserveInfo.RecSetting.RecMode)
            {
                case 0:
                    recMode = "全サービス";
                    break;
                case 1:
                    recMode = "指定サービス";
                    break;
                case 2:
                    recMode = "全サービス（デコード処理なし）";
                    break;
                case 3:
                    recMode = "指定サービス（デコード処理なし）";
                    break;
                case 4:
                    recMode = "視聴";
                    break;
                case 5:
                    recMode = "無効";
                    break;
                default:
                    break;
            } 
            String tuijyu = "";
            if (reserveInfo.RecSetting.TuijyuuFlag == 0)
            {
                tuijyu = "しない";
            }
            else if (reserveInfo.RecSetting.TuijyuuFlag == 1)
            {
                tuijyu = "する";
            }
            String pittari = "";
            if (reserveInfo.RecSetting.PittariFlag == 0)
            {
                pittari = "しない";
            }
            else if (reserveInfo.RecSetting.PittariFlag == 1)
            {
                pittari = "する";
            }

            view += reserveInfo.StationName;
            if (0x7880 <= reserveInfo.OriginalNetworkID && reserveInfo.OriginalNetworkID <= 0x7FE8)
            {
                view += " (地デジ)";
            }
            else if (reserveInfo.OriginalNetworkID == 0x0004)
            {
                view += " (BS)";
            }
            else if (reserveInfo.OriginalNetworkID == 0x0006)
            {
                view += " (CS1)";
            }
            else if (reserveInfo.OriginalNetworkID == 0x0007)
            {
                view += " (CS2)";
            }
            else
            {
                view += " (その他)";
            }
            view += "\r\n";

            view += reserveInfo.Title + "\r\n\r\n";
            view += "録画モード : " + recMode + "\r\n";
            view += "優先度 : " + reserveInfo.RecSetting.Priority.ToString() + "\r\n";
            view += "追従 : " + tuijyu + "\r\n";
            view += "ぴったり（？） : " + pittari + "\r\n";
            if ((reserveInfo.RecSetting.ServiceMode & 0x01) == 0)
            {
                view += "指定サービス対象データ : デフォルト\r\n";
            }
            else
            {
                view += "指定サービス対象データ : ";
                if ((reserveInfo.RecSetting.ServiceMode & 0x10) > 0)
                {
                    view += "字幕含む ";
                }
                if ((reserveInfo.RecSetting.ServiceMode & 0x20) > 0)
                {
                    view += "データカルーセル含む";
                }
                view += "\r\n";
            }

            view += "録画実行bat : " + reserveInfo.RecSetting.BatFilePath + "\r\n";

            if (reserveInfo.RecSetting.RecFolderList.Count == 0)
            {
                view += "録画フォルダ : デフォルト\r\n";
            }
            else
            {
                view += "録画フォルダ : \r\n";
                foreach (RecFileSetInfo info in reserveInfo.RecSetting.RecFolderList)
                {
                    view += info.RecFolder + " (WritePlugIn:" + info.WritePlugIn + " ファイル名PlugIn:" + info.RecNamePlugIn + ")\r\n";
                }
            }

            if (reserveInfo.RecSetting.UseMargineFlag == 0)
            {
                view += "録画マージン : デフォルト\r\n";
            }
            else
            {
                view += "録画マージン : 開始 " + reserveInfo.RecSetting.StartMargine.ToString() +
                    " 終了 " + reserveInfo.RecSetting.EndMargine.ToString() + "\r\n";
            }

            if (reserveInfo.RecSetting.SuspendMode == 0)
            {
                view += "録画後動作 : デフォルト\r\n";
            }
            else
            {
                view += "録画後動作 : ";
                switch (reserveInfo.RecSetting.SuspendMode)
                {
                    case 1:
                        view += "スタンバイ";
                        break;
                    case 2:
                        view += "休止";
                        break;
                    case 3:
                        view += "シャットダウン";
                        break;
                    case 4:
                        view += "何もしない";
                        break;
                }
                if (reserveInfo.RecSetting.RebootFlag == 1)
                {
                    view += " 復帰後再起動する";
                }
                view += "\r\n";
            }
            if (reserveInfo.RecSetting.PartialRecFlag == 0)
            {
                view += "部分受信 : 同時出力なし\r\n";
            }
            else
            {
                view += "部分受信 : 同時出力あり\r\n";
                view += "部分受信　録画フォルダ : \r\n";
                foreach (RecFileSetInfo info in reserveInfo.RecSetting.PartialRecFolder)
                {
                    view += info.RecFolder + " (WritePlugIn:" + info.WritePlugIn + " ファイル名PlugIn:" + info.RecNamePlugIn + ")\r\n";
                }
            }
            if (reserveInfo.RecSetting.ContinueRecFlag == 0)
            {
                view += "連続録画動作 : 分割\r\n";
            }
            else
            {
                view += "連続録画動作 : 同一ファイル出力\r\n";
            }
            if (reserveInfo.RecSetting.TunerID == 0)
            {
                view += "使用チューナー強制指定 : 自動\r\n";
            }
            else
            {
                view += "使用チューナー強制指定 : ID:" + reserveInfo.RecSetting.TunerID.ToString("X8") + "\r\n";
            }

            view += "予約状況 : " + reserveInfo.Comment;
            view += "\r\n\r\n"; 
            
            view += "OriginalNetworkID : " + reserveInfo.OriginalNetworkID.ToString() + " (0x" + reserveInfo.OriginalNetworkID.ToString("X4") + ")\r\n";
            view += "TransportStreamID : " + reserveInfo.TransportStreamID.ToString() + " (0x" + reserveInfo.TransportStreamID.ToString("X4") + ")\r\n";
            view += "ServiceID : " + reserveInfo.ServiceID.ToString() + " (0x" + reserveInfo.ServiceID.ToString("X4") + ")\r\n";
            view += "EventID : " + reserveInfo.EventID.ToString() + " (0x" + reserveInfo.EventID.ToString("X4") + ")\r\n";

            return view;
        }

        public String ConvertProgramText(EpgEventInfo eventInfo, EventInfoTextMode textMode)
        {
            string retText = "";
            string basicInfo = "";
            string extInfo = "";
            if (eventInfo != null)
            {
                UInt64 key = ((UInt64)eventInfo.original_network_id) << 32 |
                    ((UInt64)eventInfo.transport_stream_id) << 16 |
                    ((UInt64)eventInfo.service_id);
                if (ChSet5.Instance.ChList.ContainsKey(key) == true)
                {
                    basicInfo += ChSet5.Instance.ChList[key].ServiceName + "(" + ChSet5.Instance.ChList[key].NetworkName + ")" + "\r\n";
                }

                if (eventInfo.StartTimeFlag == 1)
                {
                    basicInfo += eventInfo.start_time.ToString("yyyy/MM/dd(ddd) HH:mm:ss ～ ");
                }
                else
                {
                    basicInfo += "未定 ～ ";
                }
                if (eventInfo.DurationFlag == 1)
                {
                    DateTime endTime = eventInfo.start_time + TimeSpan.FromSeconds(eventInfo.durationSec);
                    basicInfo += endTime.ToString("yyyy/MM/dd(ddd) HH:mm:ss") + "\r\n";
                }
                else
                {
                    basicInfo += "未定\r\n";
                }

                if (eventInfo.ShortInfo != null)
                {
                    basicInfo += eventInfo.ShortInfo.event_name + "\r\n\r\n";
                    extInfo += eventInfo.ShortInfo.text_char + "\r\n\r\n";
                }

                if (eventInfo.ExtInfo != null)
                {
                    extInfo += eventInfo.ExtInfo.text_char + "\r\n\r\n";
                }

                //ジャンル
                extInfo += "ジャンル :\r\n";
                if (eventInfo.ContentInfo != null)
                {
                    foreach (EpgContentData info in eventInfo.ContentInfo.nibbleList)
                    {
                        String content = "";
                        int nibble1 = info.content_nibble_level_1;
                        int nibble2 = info.content_nibble_level_2;
                        if (nibble1 == 0x0E && nibble2 == 0x01)
                        {
                            nibble1 = info.user_nibble_1;
                            nibble2 = info.user_nibble_2;
                            UInt16 contentKey1 = (UInt16)(nibble1 << 8 | 0xFF);
                            UInt16 contentKey2 = (UInt16)(nibble1 << 8 | nibble2);
                            if (ContentKindDictionary2.ContainsKey(contentKey1) == true)
                            {
                                content += ContentKindDictionary2[contentKey1];
                            }
                            if (ContentKindDictionary2.ContainsKey(contentKey2) == true)
                            {
                                content += " - " + ContentKindDictionary2[contentKey2];
                            }
                            if (content.Length == 0 || nibble1 == 0x0F)
                            {
                                content += "(0x" + info.user_nibble_1.ToString("X2") + info.user_nibble_2.ToString("X2") + ")" + "(0x" + info.user_nibble_1.ToString("X2") + info.user_nibble_2.ToString("X2") + ")";
                            }
                            extInfo += content + "\r\n";
                        }
                        else
                        {
                            UInt16 contentKey1 = (UInt16)(nibble1 << 8 | 0xFF);
                            UInt16 contentKey2 = (UInt16)(nibble1 << 8 | nibble2);
                            if (ContentKindDictionary.ContainsKey(contentKey1) == true)
                            {
                                content += ContentKindDictionary[contentKey1];
                            }
                            if (ContentKindDictionary.ContainsKey(contentKey2) == true)
                            {
                                content += " - " + ContentKindDictionary[contentKey2];
                            }
                            if (content.Length == 0 || nibble1 == 0x0F)
                            {
                                content += "(0x" + nibble1.ToString("X2") + nibble2.ToString("X2") + ")" + "(0x" + info.user_nibble_1.ToString("X2") + info.user_nibble_2.ToString("X2") + ")";
                            }
                            extInfo += content + "\r\n";
                        }
                    }
                }
                extInfo += "\r\n";

                //映像
                extInfo += "映像 :";
                if (eventInfo.ComponentInfo != null)
                {
                    int streamContent = eventInfo.ComponentInfo.stream_content;
                    int componentType = eventInfo.ComponentInfo.component_type;
                    UInt16 componentKey = (UInt16)(streamContent << 8 | componentType);
                    if (ComponentKindDictionary.ContainsKey(componentKey) == true)
                    {
                        extInfo += ComponentKindDictionary[componentKey];
                    }
                    if (eventInfo.ComponentInfo.text_char.Length > 0)
                    {
                        extInfo += "\r\n";
                        extInfo += eventInfo.ComponentInfo.text_char;
                    }
                }
                extInfo += "\r\n";

                //音声
                extInfo += "音声 :\r\n";
                if (eventInfo.AudioInfo != null)
                {
                    foreach (EpgAudioComponentInfoData info in eventInfo.AudioInfo.componentList)
                    {
                        int streamContent = info.stream_content;
                        int componentType = info.component_type;
                        UInt16 componentKey = (UInt16)(streamContent << 8 | componentType);
                        if (ComponentKindDictionary.ContainsKey(componentKey) == true)
                        {
                            extInfo += ComponentKindDictionary[componentKey];
                        }
                        if (info.text_char.Length > 0)
                        {
                            extInfo += "\r\n";
                            extInfo += info.text_char;
                        }
                        extInfo += "\r\n";
                        extInfo += "サンプリングレート :";
                        switch (info.sampling_rate)
                        {
                            case 1:
                                extInfo += "16kHz";
                                break;
                            case 2:
                                extInfo += "22.05kHz";
                                break;
                            case 3:
                                extInfo += "24kHz";
                                break;
                            case 5:
                                extInfo += "32kHz";
                                break;
                            case 6:
                                extInfo += "44.1kHz";
                                break;
                            case 7:
                                extInfo += "48kHz";
                                break;
                            default:
                                break;
                        }
                        extInfo += "\r\n";
                    }
                }
                extInfo += "\r\n";

                //スクランブル
                if (!(0x7880 <= eventInfo.original_network_id && eventInfo.original_network_id <= 0x7FE8))
                {
                    if (eventInfo.FreeCAFlag == 0)
                    {
                        extInfo += "無料放送\r\n";
                    }
                    else
                    {
                        extInfo += "有料放送\r\n";
                    }
                    extInfo += "\r\n";
                }

                //イベントリレー
                if (eventInfo.EventRelayInfo != null)
                {
                    if (eventInfo.EventRelayInfo.eventDataList.Count > 0)
                    {
                        extInfo += "イベントリレーあり：\r\n";
                        foreach (EpgEventData info in eventInfo.EventRelayInfo.eventDataList)
                        {
                            key = ((UInt64)info.original_network_id) << 32 |
                                ((UInt64)info.transport_stream_id) << 16 |
                                ((UInt64)info.service_id);
                            if (ChSet5.Instance.ChList.ContainsKey(key) == true)
                            {
                                extInfo += ChSet5.Instance.ChList[key].ServiceName + "(" + ChSet5.Instance.ChList[key].NetworkName + ")" + " ";
                            }
                            else
                            {
                                extInfo += "OriginalNetworkID : " + info.original_network_id.ToString() + " (0x" + info.original_network_id.ToString("X4") + ") ";
                                extInfo += "TransportStreamID : " + info.transport_stream_id.ToString() + " (0x" + info.transport_stream_id.ToString("X4") + ") ";
                                extInfo += "ServiceID : " + info.service_id.ToString() + " (0x" + info.service_id.ToString("X4") + ") ";
                            }
                            extInfo += "EventID : " + info.event_id.ToString() + " (0x" + info.event_id.ToString("X4") + ")\r\n";
                            extInfo += "\r\n";
                        }
                        extInfo += "\r\n";
                    }
                }

                extInfo += "OriginalNetworkID : " + eventInfo.original_network_id.ToString() + " (0x" + eventInfo.original_network_id.ToString("X4") + ")\r\n";
                extInfo += "TransportStreamID : " + eventInfo.transport_stream_id.ToString() + " (0x" + eventInfo.transport_stream_id.ToString("X4") + ")\r\n";
                extInfo += "ServiceID : " + eventInfo.service_id.ToString() + " (0x" + eventInfo.service_id.ToString("X4") + ")\r\n";
                extInfo += "EventID : " + eventInfo.event_id.ToString() + " (0x" + eventInfo.event_id.ToString("X4") + ")\r\n";

            }

            if (textMode == EventInfoTextMode.All || textMode == EventInfoTextMode.BasicOnly)
            {
                retText = basicInfo;
            }
            if (textMode == EventInfoTextMode.All || textMode == EventInfoTextMode.ExtOnly)
            {
                retText += extInfo;
            }
            return retText;
        }

        public void FilePlay(uint reserveID)
        {
            if (Settings.Instance.FilePlayOnAirWithExe && (NWMode == false || Settings.Instance.FilePlayExe.Length != 0))
            {
                //ファイルパスを取得するため開いてすぐ閉じる
                var info = new NWPlayTimeShiftInfo();
                if (CtrlCmd.SendNwTimeShiftOpen(reserveID, ref info) == ErrCode.CMD_SUCCESS)
                {
                    CtrlCmd.SendNwPlayClose(info.ctrlID);
                    if (info.filePath != "")
                    {
                        FilePlay(info.filePath);
                        return;
                    }
                }
                MessageBox.Show("録画ファイルの場所がわかりませんでした。", "追っかけ再生", MessageBoxButton.OK, MessageBoxImage.Information);
            }
            else
            {
                TVTestCtrl.StartTimeShift(reserveID);
            }
        }

        public void FilePlay(String filePath)
        {
            try
            {
                if (NWMode == false || Settings.Instance.FilePlayExe.Length != 0)
                {
                    System.Diagnostics.Process process;
                    if (Settings.Instance.FilePlayExe.Length == 0)
                    {
                        process = System.Diagnostics.Process.Start(filePath);
                    }
                    else
                    {
                        String cmdLine = Settings.Instance.FilePlayCmd;
                        //'$'->'\t'は再帰的な展開を防ぐため
                        cmdLine = cmdLine.Replace("$FileNameExt$", Path.GetFileName(filePath).Replace('$', '\t'));
                        cmdLine = cmdLine.Replace("$FilePath$", filePath).Replace('\t', '$');
                        process = System.Diagnostics.Process.Start(Settings.Instance.FilePlayExe, cmdLine);

                    }
                }
                else
                {
                    TVTestCtrl.StartStreamingPlay(filePath, NW.ConnectedIP, NW.ConnectedPort);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        public void ReloadCustContentColorList()
        {
            try
            {
                CustContentColorList.Clear();
                for (int i = 0; i < Settings.Instance.ContentColorList.Count; i++)
                {
                    String name = Settings.Instance.ContentColorList[i];
                    Color color;
                    if (String.Compare(name, "カスタム") == 0)
                    {
                        UInt32 argb = Settings.Instance.ContentCustColorList[i];

                        byte r = (byte)((argb & 0x00FF0000) >> 16);
                        byte g = (byte)((argb & 0x0000FF00) >> 8);
                        byte b = (byte)(argb & 0x000000FF);

                        color = Color.FromArgb(0xFF, r, g, b);
                    }
                    else
                    {
                        color = ColorDef.ColorFromName(name);
                    }
                    if (Settings.Instance.EpgGradation == false)
                    {
                        CustContentColorList.Add(new SolidColorBrush(color));
                        CustContentColorList[CustContentColorList.Count - 1].Freeze();
                    }
                    else
                    {
                        CustContentColorList.Add(ColorDef.GradientBrush(color));
                    }
                }
                if (String.Compare(Settings.Instance.ReserveRectColorNormal, "カスタム") == 0)
                {
                    UInt32 argb = Settings.Instance.ContentCustColorList[0x11];

                    byte r = (byte)((argb & 0x00FF0000) >> 16);
                    byte g = (byte)((argb & 0x0000FF00) >> 8);
                    byte b = (byte)(argb & 0x000000FF);

                    Color item = Color.FromArgb(0xFF, r, g, b);
                    SolidColorBrush backColor = new SolidColorBrush();
                    backColor.Color = item;
                    backColor.Freeze();

                    CustContentColorList.Add(backColor);
                }
                else
                {
                    CustContentColorList.Add(ColorDef.Instance.ColorTable[Settings.Instance.ReserveRectColorNormal]);
                }
                if (String.Compare(Settings.Instance.ReserveRectColorNo, "カスタム") == 0)
                {
                    UInt32 argb = Settings.Instance.ContentCustColorList[0x12];

                    byte r = (byte)((argb & 0x00FF0000) >> 16);
                    byte g = (byte)((argb & 0x0000FF00) >> 8);
                    byte b = (byte)(argb & 0x000000FF);

                    Color item = Color.FromArgb(0xFF, r, g, b);
                    SolidColorBrush backColor = new SolidColorBrush();
                    backColor.Color = item;
                    backColor.Freeze();

                    CustContentColorList.Add(backColor);
                }
                else
                {
                    CustContentColorList.Add(ColorDef.Instance.ColorTable[Settings.Instance.ReserveRectColorNo]);
                }
                if (String.Compare(Settings.Instance.ReserveRectColorNoTuner, "カスタム") == 0)
                {
                    UInt32 argb = Settings.Instance.ContentCustColorList[0x13];

                    byte r = (byte)((argb & 0x00FF0000) >> 16);
                    byte g = (byte)((argb & 0x0000FF00) >> 8);
                    byte b = (byte)(argb & 0x000000FF);

                    Color item = Color.FromArgb(0xFF, r, g, b);
                    SolidColorBrush backColor = new SolidColorBrush();
                    backColor.Color = item;
                    backColor.Freeze();

                    CustContentColorList.Add(backColor);
                }
                else
                {
                    CustContentColorList.Add(ColorDef.Instance.ColorTable[Settings.Instance.ReserveRectColorNoTuner]);
                }
                if (String.Compare(Settings.Instance.ReserveRectColorWarning, "カスタム") == 0)
                {
                    UInt32 argb = Settings.Instance.ContentCustColorList[0x14];

                    byte r = (byte)((argb & 0x00FF0000) >> 16);
                    byte g = (byte)((argb & 0x0000FF00) >> 8);
                    byte b = (byte)(argb & 0x000000FF);

                    Color item = Color.FromArgb(0xFF, r, g, b);
                    SolidColorBrush backColor = new SolidColorBrush();
                    backColor.Color = item;
                    backColor.Freeze();

                    CustContentColorList.Add(backColor);
                }
                else
                {
                    CustContentColorList.Add(ColorDef.Instance.ColorTable[Settings.Instance.ReserveRectColorWarning]);
                }

                if (String.Compare(Settings.Instance.TitleColor1, "カスタム") == 0)
                {
                    UInt32 argb = Settings.Instance.TitleCustColor1;

                    byte r = (byte)((argb & 0x00FF0000) >> 16);
                    byte g = (byte)((argb & 0x0000FF00) >> 8);
                    byte b = (byte)(argb & 0x000000FF);

                    Color item = Color.FromArgb(0xFF, r, g, b);
                    SolidColorBrush backColor = new SolidColorBrush();
                    backColor.Color = item;
                    backColor.Freeze();

                    CustTitle1Color = backColor;
                }
                else
                {
                    CustTitle1Color = ColorDef.Instance.ColorTable[Settings.Instance.TitleColor1];
                }
                if (String.Compare(Settings.Instance.TitleColor2, "カスタム") == 0)
                {
                    UInt32 argb = Settings.Instance.TitleCustColor2;

                    byte r = (byte)((argb & 0x00FF0000) >> 16);
                    byte g = (byte)((argb & 0x0000FF00) >> 8);
                    byte b = (byte)(argb & 0x000000FF);

                    Color item = Color.FromArgb(0xFF, r, g, b);
                    SolidColorBrush backColor = new SolidColorBrush();
                    backColor.Color = item;
                    backColor.Freeze();

                    CustTitle2Color = backColor;
                }
                else
                {
                    CustTitle2Color = ColorDef.Instance.ColorTable[Settings.Instance.TitleColor2];
                }
                CustTimeColorList.Clear();
                for (int i = 0; i < Settings.Instance.TimeColorList.Count; i++)
                {
                    String name = Settings.Instance.TimeColorList[i];
                    Color color;
                    if (String.Compare(name, "カスタム") == 0)
                    {
                        UInt32 argb = Settings.Instance.TimeCustColorList[i];

                        byte r = (byte)((argb & 0x00FF0000) >> 16);
                        byte g = (byte)((argb & 0x0000FF00) >> 8);
                        byte b = (byte)(argb & 0x000000FF);

                        color = Color.FromArgb(0xFF, r, g, b);
                    }
                    else
                    {
                        color = ColorDef.ColorFromName(name);
                    }
                    if (Settings.Instance.EpgGradationHeader == false)
                    {
                        CustTimeColorList.Add(new SolidColorBrush(color));
                        CustTimeColorList[CustTimeColorList.Count - 1].Freeze();
                    }
                    else
                    {
                        CustTimeColorList.Add(ColorDef.GradientBrush(color, 0.9, 1.1));
                    }
                }
                Color serviceColor;
                if (String.Compare(Settings.Instance.ServiceColor, "カスタム") == 0)
                {
                    UInt32 argb = Settings.Instance.ServiceCustColor;

                    byte r = (byte)((argb & 0x00FF0000) >> 16);
                    byte g = (byte)((argb & 0x0000FF00) >> 8);
                    byte b = (byte)(argb & 0x000000FF);

                    serviceColor = Color.FromArgb(0xFF, r, g, b);
                }
                else
                {
                    serviceColor = ColorDef.ColorFromName(Settings.Instance.ServiceColor);
                }
                if (Settings.Instance.EpgGradationHeader == false)
                {
                    CustServiceColor = new SolidColorBrush(serviceColor);
                    CustServiceColor.Freeze();
                }
                else
                {
                    CustServiceColor = ColorDef.GradientBrush(serviceColor, 1.0, 2.0);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }


        private SolidColorBrush resDefBackColor = null;
        public SolidColorBrush ResDefBackColor
        {
            get
            {
                if (resDefBackColor == null)
                {
                    Color item = Color.FromArgb(0xFF, Settings.Instance.ResDefColorR, Settings.Instance.ResDefColorG, Settings.Instance.ResDefColorB);
                    resDefBackColor = new SolidColorBrush();
                    resDefBackColor.Color = item;
                    resDefBackColor.Freeze();
                }
                return resDefBackColor;
            }
        }
        private SolidColorBrush resErrBackColor = null;
        public SolidColorBrush ResErrBackColor
        {
            get
            {
                if (resErrBackColor == null)
                {
                    Color item = Color.FromArgb(0xFF, Settings.Instance.ResErrColorR, Settings.Instance.ResErrColorG, Settings.Instance.ResErrColorB);
                    resErrBackColor = new SolidColorBrush();
                    resErrBackColor.Color = item;
                    resErrBackColor.Freeze();
                }
                return resErrBackColor;
            }
        }
        private SolidColorBrush resWarBackColor = null;
        public SolidColorBrush ResWarBackColor
        {
            get
            {
                if (resWarBackColor == null)
                {
                    Color item = Color.FromArgb(0xFF, Settings.Instance.ResWarColorR, Settings.Instance.ResWarColorG, Settings.Instance.ResWarColorB);
                    resWarBackColor = new SolidColorBrush();
                    resWarBackColor.Color = item;
                    resWarBackColor.Freeze();
                }
                return resWarBackColor;
            }
        }
        private SolidColorBrush resNoBackColor = null;
        public SolidColorBrush ResNoBackColor
        {
            get
            {
                if (resNoBackColor == null)
                {
                    Color item = Color.FromArgb(0xFF, Settings.Instance.ResNoColorR, Settings.Instance.ResNoColorG, Settings.Instance.ResNoColorB);
                    resNoBackColor = new SolidColorBrush();
                    resNoBackColor.Color = item;
                    resNoBackColor.Freeze();
                }
                return resNoBackColor;
            }
        }

        private SolidColorBrush recEndDefBackColor = null;
        public SolidColorBrush RecEndDefBackColor
        {
            get
            {
                if (recEndDefBackColor == null)
                {
                    Color item = Color.FromArgb(0xFF, Settings.Instance.RecEndDefColorR, Settings.Instance.RecEndDefColorG, Settings.Instance.RecEndDefColorB);
                    recEndDefBackColor = new SolidColorBrush();
                    recEndDefBackColor.Color = item;
                    recEndDefBackColor.Freeze();
                }
                return recEndDefBackColor;
            }
        }
        
        private SolidColorBrush recEndErrBackColor = null;
        public SolidColorBrush RecEndErrBackColor
        {
            get
            {
                if( recEndErrBackColor == null ){
                    Color item = Color.FromArgb(0xFF, Settings.Instance.RecEndErrColorR, Settings.Instance.RecEndErrColorG, Settings.Instance.RecEndErrColorB);
                    recEndErrBackColor = new SolidColorBrush();
                    recEndErrBackColor.Color = item;
                    recEndErrBackColor.Freeze();
                }
                return recEndErrBackColor;
            }
        }

        private SolidColorBrush recEndWarBackColor = null;
        public SolidColorBrush RecEndWarBackColor
        {
            get
            {
                if (recEndWarBackColor == null)
                {
                    Color item = Color.FromArgb(0xFF, Settings.Instance.RecEndWarColorR, Settings.Instance.RecEndWarColorG, Settings.Instance.RecEndWarColorB);
                    recEndWarBackColor = new SolidColorBrush();
                    recEndWarBackColor.Color = item;
                    recEndWarBackColor.Freeze();
                }
                return recEndWarBackColor;
            }
        }

        private SolidColorBrush epgTipsBackColor = null;
        public SolidColorBrush EpgTipsBackColor
        {
            get
            {
                if (epgTipsBackColor == null)
                {
                    Color item = Color.FromArgb(0xFF, Settings.Instance.EpgTipsBackColorR, Settings.Instance.EpgTipsBackColorG, Settings.Instance.EpgTipsBackColorB);
                    epgTipsBackColor = new SolidColorBrush();
                    epgTipsBackColor.Color = item;
                    epgTipsBackColor.Freeze();
                }
                return epgTipsBackColor;
            }
        }
        private SolidColorBrush epgTipsForeColor = null;
        public SolidColorBrush EpgTipsForeColor
        {
            get
            {
                if (epgTipsForeColor == null)
                {
                    Color item = Color.FromArgb(0xFF, Settings.Instance.EpgTipsForeColorR, Settings.Instance.EpgTipsForeColorG, Settings.Instance.EpgTipsForeColorB);
                    epgTipsForeColor = new SolidColorBrush();
                    epgTipsForeColor.Color = item;
                    epgTipsForeColor.Freeze();
                }
                return epgTipsForeColor;
            }
        }

        public void AddNotifySave(NotifySrvInfo notifyInfo)
        {
            if (Settings.Instance.AutoSaveNotifyLog == 1)
            {
                String filePath = SettingPath.ModulePath;
                filePath += "\\Log";
                Directory.CreateDirectory(filePath);
                filePath += "\\EpgTimerNotify_" + DateTime.Now.ToString("yyyyMMdd") + ".txt";
                StreamWriter file = new StreamWriter(filePath, true, System.Text.Encoding.GetEncoding("shift_jis"));
                NotifySrvInfoItem info = new NotifySrvInfoItem();
                info.NotifyInfo = notifyInfo;
                file.Write(info.FileLogText);
                file.Close();
            }
        }
    }
}
