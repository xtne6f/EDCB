using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Windows;
using System.Windows.Documents;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Collections;
using System.IO;
using System.Diagnostics;

namespace EpgTimer
{
    class CommonManager
    {
        public CtrlCmdUtil CtrlCmd
        { get; set; }
        public DBManager DB
        { get; set; }
        public TVTestCtrlClass TVTestCtrl
        { get; set; }
        public Dictionary<UInt16, ContentKindInfo> ContentKindDictionary
        { get; set; }
        public List<ContentKindInfo> ContentKindList
        { get; set; }
        public Dictionary<UInt16, ContentKindInfo> ContentKindDictionary2
        { get; set; }
        public Dictionary<UInt16, ComponentKindInfo> ComponentKindDictionary
        { get; set; }
        public Dictionary<byte, DayOfWeekInfo> DayOfWeekDictionary
        { get; set; }
        public Dictionary<UInt16, UInt16> HourDictionary
        { get; set; }
        public Dictionary<UInt16, UInt16> HourDictionary28
        { get; set; }
        public Dictionary<UInt16, UInt16> HourDictionarySelect
        {
            get { return Settings.Instance.LaterTimeUse == true ? HourDictionary28 : HourDictionary; }
        }
        public Dictionary<UInt16, UInt16> MinDictionary
        { get; set; }
        public Dictionary<byte, RecModeInfo> RecModeDictionary
        { get; set; }
        public Dictionary<byte, YesNoInfo> YesNoDictionary
        { get; set; }
        public Dictionary<byte, PriorityInfo> PriorityDictionary
        { get; set; }
        public bool NWMode
        { get; set; }
        public List<NotifySrvInfo> NotifyLogList
        { get; set; }
        public NWConnect NW
        { get; set; }
        public MenuUtil MUtil
        { get; set; }
        public ViewUtil VUtil
        { get; set; }

        MenuManager _mm;
        public MenuManager MM
        {
            get
            {
                //初期化に他のオブジェクトを使うので遅延させる
                if (_mm == null) _mm = new MenuManager();
                //
                return _mm;
            }
            set { _mm = value; }
        }

        public List<Brush> CustContentColorList { get; private set; }
        public SolidColorBrush CustTitle1Color { get; private set; }
        public SolidColorBrush CustTitle2Color { get; private set; }
        public SolidColorBrush CustTunerServiceColor { get; private set; }
        public SolidColorBrush CustTunerTextColor { get; private set; }
        public List<SolidColorBrush> CustTunerServiceColorPri { get; private set; }
        public List<Brush> CustTimeColorList { get; private set; }
        public Brush CustServiceColor { get; private set; }
        public SolidColorBrush ResDefBackColor { get; private set; }
        public SolidColorBrush ResErrBackColor { get; private set; }
        public SolidColorBrush ResWarBackColor { get; private set; }
        public SolidColorBrush ResNoBackColor { get; private set; }
        public SolidColorBrush ResAutoAddMissingBackColor { get; private set; }
        public SolidColorBrush ListDefForeColor { get; private set; }
        public List<SolidColorBrush> RecModeForeColor { get; private set; }
        public SolidColorBrush RecEndDefBackColor { get; private set; }
        public SolidColorBrush RecEndErrBackColor { get; private set; }
        public SolidColorBrush RecEndWarBackColor { get; private set; }
        public SolidColorBrush StatResForeColor { get; private set; }
        public SolidColorBrush StatRecForeColor { get; private set; }
        public SolidColorBrush StatOnAirForeColor { get; private set; }

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
            if (MUtil == null)
            {
                MUtil = new MenuUtil(CtrlCmd);
            }
            if (VUtil == null)
            {
                VUtil = new ViewUtil(CtrlCmd, MUtil);
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
                ContentKindDictionary.Add(0x0F0F, new ContentKindInfo("その他", "その他", 0x0F, 0x0F));

                ContentKindDictionary.Add(0xFEFF, new ContentKindInfo("不明なジャンル(未実装)", "", 0xFE, 0xFF));
                ContentKindDictionary.Add(0xFFFF, new ContentKindInfo("ジャンル情報なし", "", 0xFF, 0xFF));
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

                if (ContentKindDictionary != null)
                {
                    //CSもまとめて検索出来るようにする仮対応。
                    foreach (ContentKindInfo info in ContentKindDictionary2.Values)
                    {
                        ContentKindDictionary.Add((UInt16)(info.ID | 0x7000), new ContentKindInfo(info.ContentName, info.SubName, (Byte)(info.Nibble1 | 0x70), info.Nibble2));
                    }

                    //表示順が追加順にならないことの対策。OrderedDictionary使えるならその方が簡単かとは思うが
                    if (ContentKindList == null)
                    {
                        ContentKindList = ContentKindDictionary.Values.OrderBy(info => info.SortKey).ToList();
                    }
                }
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
            if (HourDictionary28 == null)
            {
                HourDictionary28 = new Dictionary<UInt16, UInt16>();
                for (UInt16 i = 0; i <= 36; i++)
                {
                    HourDictionary28.Add(i, i);
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
            if (CustTunerServiceColorPri == null)
            {
                CustTunerServiceColorPri = new List<SolidColorBrush>();
            }
            if (CustTimeColorList == null)
            {
                CustTimeColorList = new List<Brush>();
            }
            if (RecModeForeColor == null)
            {
                RecModeForeColor = new List<SolidColorBrush>();
            }
        }

        public bool IsConnected { get { return NWMode == false || NW.IsConnected == true; } }

        public static UInt64 Create64Key(UInt16 ONID, UInt16 TSID, UInt16 SID)
        {
            return ((UInt64)ONID) << 32 | ((UInt64)TSID) << 16 | (UInt64)SID;
        }

        public static UInt64 Create64PgKey(UInt16 ONID, UInt16 TSID, UInt16 SID, UInt16 EventID)
        {
            return ((UInt64)ONID) << 48 | ((UInt64)TSID) << 32 | ((UInt64)SID) << 16 | (UInt64)EventID;
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

        /// <summary>
        /// 良くある通信エラー(CMD_ERR_CONNECT,CMD_ERR_TIMEOUT)をMessageBoxで表示する。
        /// Owner(this)を指定するとDispatcher.BeginInvokeで実行する。
        /// </summary>
        public static bool CmdErrMsgTypical(ErrCode err, string caption = "通信エラー", Control Owner = null)
        {
            if (err == ErrCode.CMD_SUCCESS) return true;

            string msg;
            switch (err)
            {
                case ErrCode.CMD_ERR_CONNECT:
                    msg="サーバー または EpgTimerSrv に接続できませんでした。";
                    break;
                //case ErrCode.CMD_ERR_BUSY:  //もう表示しないことにしているようだ。
                //    msg = "データの読み込みを行える状態ではありません。\r\n（EPGデータ読み込み中。など）";
                //    break;
                case ErrCode.CMD_ERR_TIMEOUT:
                    msg = "EpgTimerSrvとの接続にタイムアウトしました。";
                    break;
                default:
                    msg = "通信エラーが発生しました。";
                    break;
            }

            if (Owner != null)
            {
                Owner.Dispatcher.BeginInvoke(new Action(() => MessageBox.Show(msg, caption)), null);
            }
            else
            {
                MessageBox.Show(msg, caption);
            }

            return false;
        }

        public static String ConvertTimeText(EpgEventInfo info)
        {
            if (info.StartTimeFlag != 1) return "未定 ～ 未定";
            //
            string reftxt = ConvertTimeText(info.start_time, info.PgDurationSecond, false, false, false, false);
            return info.DurationFlag == 1 ? reftxt : reftxt.Split(new char[] { '～' })[0] + "～ 未定";
        }
        public static String ConvertTimeText(EpgSearchDateInfo info)
        {
            //超手抜き。書式が変ったら、巻き込まれて死ぬ。
            var start = new DateTime(2000, 1, 2 + info.startDayOfWeek, info.startHour, info.startMin, 0);
            var end = new DateTime(2000, 1, 2 + info.endDayOfWeek, info.endHour, info.endMin, 0);
            if (end < start) end = end.AddDays(7);
            string reftxt = ConvertTimeText(start, (uint)(end - start).TotalSeconds, true, true, false, false);
            string[] src = reftxt.Split(new char[] { ' ', '～' });
            return src[0].Substring(6, 1) + " " + src[1] + " ～ " + src[2].Substring(6, 1) + " " + src[3];
        }
        public static String ConvertTimeText(DateTime start, uint duration, bool isNoYear, bool isNoSecond, bool isNoEndDay = true, bool isNoStartDay = false)
        {
            DateTime end = start + TimeSpan.FromSeconds(duration);

            if (Settings.Instance.LaterTimeUse == true)
            {
                bool over1Day = duration >= 24 * 60 * 60;
                bool? isStartLate = (isNoEndDay == false && over1Day == true) ? (bool?)false : null;
                bool isEndLate = (isNoEndDay == false || isNoStartDay == true && over1Day == false
                    ? over1Day == false && DateTime28.JudgeLateHour(end, start)
                    : DateTime28.JudgeLateHour(end, start, 99));
                DateTime28 ref_start = (isEndLate == true && isNoEndDay == true && isNoStartDay == false) ? new DateTime28(start) : null;

                return ConvertTimeText(start, isNoYear, isNoSecond, isNoStartDay, isStartLate)
                    + (isNoSecond == true ? "～" : " ～ ")
                    + ConvertTimeText(end, isNoYear, isNoSecond, isNoEndDay, isEndLate, ref_start);
            }
            else
            {
                return ConvertTimeText(start, isNoYear, isNoSecond, isNoStartDay)
                + (isNoSecond == true ? "～" : " ～ ")
                + ConvertTimeText(end, isNoYear, isNoSecond, isNoEndDay);
            }
        }
        public static String ConvertTimeText(DateTime time, bool isNoYear, bool isNoSecond, bool isNoDay = false, bool? isUse28 = null, DateTime28 ref_start = null)
        {
            if (Settings.Instance.LaterTimeUse == true)
            {
                var time28 = new DateTime28(time, isUse28, ref_start);
                return (isNoDay == true ? "" : time28.DateTimeMod.ToString((isNoYear == true ? "MM/dd(ddd) " : "yyyy/MM/dd(ddd) ")))
                + time28.HourMod.ToString("00:") + time.ToString(isNoSecond == true ? "mm" : "mm:ss");
            }
            else
            {
                return time.ToString((isNoDay == true ? "" :
                (isNoYear == true ? "MM/dd(ddd) " : "yyyy/MM/dd(ddd) ")) + (isNoSecond == true ? "HH:mm" : "HH:mm:ss"));
            }
        }
        public static String ConvertDurationText(uint duration, bool isNoSecond)
        {
            return (duration / 3600).ToString() 
                + ((duration % 3600) / 60).ToString(":00") 
                + (isNoSecond == true ? "" : (duration % 60).ToString(":00"));
        }

        public static String ConvertResModeText(ReserveMode? mode)
        {
            switch (mode)
            {
                case ReserveMode.KeywordAuto: return "キーワード予約";
                case ReserveMode.ManualAuto : return "プログラム自動予約";
                case ReserveMode.EPG        : return "個別予約(EPG)";
                case ReserveMode.Program    : return "個別予約(プログラム)";
                default                     : return "";
            }
        }

        public String ConvertProgramText(EpgEventInfo eventInfo, EventInfoTextMode textMode)
        {
            string retText = "";
            string basicInfo = "";
            string extInfo = "";
            if (eventInfo != null)
            {
                UInt64 key = eventInfo.Create64Key();
                if (ChSet5.Instance.ChList.ContainsKey(key) == true)
                {
                    basicInfo += ChSet5.Instance.ChList[key].ServiceName + "(" + ChSet5.Instance.ChList[key].NetworkName + ")" + "\r\n";
                }

                basicInfo += ConvertTimeText(eventInfo) + "\r\n";

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
                        UInt16 ID1 = (UInt16)(((UInt16)info.content_nibble_level_1) << 8 | 0xFF);
                        UInt16 ID2 = (UInt16)(((UInt16)info.content_nibble_level_1) << 8 | info.content_nibble_level_2);
                        if (ID2 == 0x0e01)//CS、仮対応データをそのまま使用。
                        {
                            ID1 = (UInt16)(((UInt16)info.user_nibble_1) << 8 | 0x70FF);
                            ID2 = (UInt16)(((UInt16)info.user_nibble_1) << 8 | 0x7000 | info.user_nibble_2);
                        }

                        String content = "";
                        ContentKindInfo kindInfo;
                        if (ContentKindDictionary.TryGetValue(ID1, out kindInfo) == true)
                        {
                            content += kindInfo.ContentName;
                        }
                        else
                        {
                            content += "不明" + "(0x" + info.content_nibble_level_1.ToString("X2") + info.content_nibble_level_2.ToString("X2") + ")";
                        }
                        if (ContentKindDictionary.TryGetValue(ID2, out kindInfo) == true)
                        {
                            content += " - " + kindInfo.SubName;
                        }
                        else
                        {
                            content += " - " + "不明" + "(0x" + info.user_nibble_1.ToString("X2") + info.user_nibble_2.ToString("X2") + ")";
                        }
                        extInfo += content + "\r\n";
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
                if (!ChSet5.IsTere(eventInfo.original_network_id))
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
                            key = info.Create64Key();
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

        public static String ConvertNetworkNameText(ushort originalNetworkID, bool IsSimple = false)
        {
            String retText = "";
            if (ChSet5.IsTere(originalNetworkID) == true)
            {
                retText = "地デジ";
            }
            else if (ChSet5.IsBS(originalNetworkID) == true)
            {
                retText = "BS";
            }
            else if (ChSet5.IsCS1(originalNetworkID) == true)
            {
                retText = IsSimple == true ? "CS" : "CS1";
            }
            else if (ChSet5.IsCS2(originalNetworkID) == true)
            {
                retText = IsSimple == true ? "CS" : "CS2";
            }
            else
            {
                retText = "その他";
            }
            return retText;
        }

        public String ConvertJyanruText(EpgEventInfo eventInfo)
        {
            if (eventInfo == null || eventInfo.ContentInfo == null)
            {
                return "";
            }
            else
            {
                return ConvertJyanruText(eventInfo.ContentInfo.nibbleList);
            }
        }
        public String ConvertJyanruText(EpgSearchKeyInfo searchKeyInfo)
        {
            if (searchKeyInfo == null)
            {
                return "";
            }
            else
            {
                return ConvertJyanruText(searchKeyInfo.contentList);
            }
        }
        public String ConvertJyanruText(List<EpgContentData> nibbleList)
        {
            String retText = "";
            if (nibbleList != null)
            {
                Dictionary<int, List<int>> nibbleDict1 = new Dictionary<int, List<int>>();  // 小ジャンルを大ジャンルでまとめる
                foreach (EpgContentData ecd1 in nibbleList)
                {
                    int nibble1 = ecd1.content_nibble_level_1;
                    int nibble2 = ecd1.content_nibble_level_2;
                    if (nibble1 == 0x0E && nibble2 == 0x01)//CS、仮対応データをそのまま使用
                    {
                        nibble1 = ecd1.user_nibble_1 | 0x70;
                        nibble2 = ecd1.user_nibble_2;
                    }

                    if (nibbleDict1.ContainsKey(nibble1))
                    {
                        nibbleDict1[nibble1].Add(nibble2);
                    }
                    else
                    {
                        nibbleDict1.Add(nibble1, new List<int>() { nibble2 });
                    }
                }
                foreach (KeyValuePair<int, List<int>> kvp1 in nibbleDict1)
                {
                    int nibble1 = kvp1.Key;
                    UInt16 contentKey1 = (UInt16)(nibble1 << 8 | 0xFF);
                    //
                    string smallCategory1 = "";
                    foreach (int nibble2 in kvp1.Value.Distinct())
                    {
                        UInt16 contentKey2 = (UInt16)(nibble1 << 8 | nibble2);
                        if (nibble2 != 0xFF)
                        {
                            if (smallCategory1 != "") { smallCategory1 += ", "; }
                            if (CommonManager.Instance.ContentKindDictionary.ContainsKey(contentKey2))
                            {
                                smallCategory1 += CommonManager.Instance.ContentKindDictionary[contentKey2].ToString().Trim();
                            }
                        }
                    }
                    //
                    if (retText != "") { retText += ", "; }
                    if (CommonManager.Instance.ContentKindDictionary.ContainsKey(contentKey1))
                    {
                        retText += "[" + CommonManager.Instance.ContentKindDictionary[contentKey1].ToString().Trim();
                        if (smallCategory1 != "") { retText += " - " + smallCategory1; }
                        retText += "]";
                    }
                }
            }
            return retText;
        }

        public String ConvertRecModeText(byte recMode)
        {
            RecModeInfo info;
            return RecModeDictionary.TryGetValue(recMode, out info) == true ? info.DisplayName : "";
        }

        public String ConvertTunerText(uint tunerID)
        {
            string tunerName = "";
            TunerReserveInfo info;
            if (DB.TunerReserveList.TryGetValue(tunerID, out info))
            {
                tunerName = info.tunerName;
            }
            else if (tunerID != 0)
            {
                tunerName = "不明なチューナー";
            }
            return new TunerSelectInfo(tunerName, tunerID).ToString();
        }

        public String ConvertViewModeText(int viewMode)
        {
            String retText = "";
            switch (viewMode)
            {
                case 0:
                    retText = "標準モード";
                    break;
                case 1:
                    retText = "1週間モード";
                    break;
                case 2:
                    retText = "リスト表示モード";
                    break;
                default:
                    break;
            }
            return retText;
        }

        public FlowDocument ConvertDisplayText(EpgEventInfo eventInfo)
        {
            String epgText = CommonManager.Instance.ConvertProgramText(eventInfo, EventInfoTextMode.All);
            if (epgText == "") epgText = "番組情報がありません。\r\n" + "またはEPGデータが読み込まれていません。";
            String text = epgText;
            FlowDocument flowDoc = new FlowDocument();
            Regex regex = new Regex("((http://|https://|ｈｔｔｐ：／／|ｈｔｔｐｓ：／／).*\r\n)");

            if (regex.IsMatch(text) == true)
            {
                try
                {
                    //Regexのsplitでやるとhttp://だけのも取れたりするので、１つずつ行う
                    Paragraph para = new Paragraph();

                    do
                    {
                        Match matchVal = regex.Match(text);
                        int index = text.IndexOf(matchVal.Value);

                        para.Inlines.Add(text.Substring(0, index));
                        text = text.Remove(0, index);

                        Hyperlink h = new Hyperlink(new Run(matchVal.Value.Replace("\r\n", "")));
                        h.MouseLeftButtonDown += new MouseButtonEventHandler(h_MouseLeftButtonDown);
                        h.Foreground = Brushes.Blue;
                        h.Cursor = Cursors.Hand;
                        String url = CommonManager.ReplaceUrl(matchVal.Value.Replace("\r\n", ""));
                        h.NavigateUri = new Uri(url);
                        para.Inlines.Add(h);
                        para.Inlines.Add("\r\n");

                        text = text.Remove(0, matchVal.Value.Length);
                    } while (regex.IsMatch(text) == true);
                    para.Inlines.Add(text);

                    flowDoc.Blocks.Add(para);
                }
                catch
                {
                    flowDoc = new FlowDocument(new Paragraph(new Run(epgText)));
                }
            }
            else
            {
                flowDoc.Blocks.Add(new Paragraph(new Run(epgText)));
            }

            return flowDoc;
        }

        public String ConvertTextSearchString(String s)
        {
            return ReplaceUrl(MUtil.TrimKeyword(s));
        }        

        //デフォルト番組表の情報作成
        public List<CustomEpgTabInfo> CreateDefaultTabInfo()
        {
            List<CustomEpgTabInfo> setInfo = Enumerable.Range(0, 4).Select(i => new CustomEpgTabInfo()).ToList();

            setInfo[0].TabName = "地デジ";
            setInfo[1].TabName = "BS";
            setInfo[2].TabName = "CS";
            setInfo[3].TabName = "その他";

            for (int i = 0; i < setInfo.Count; i++)
            {
                //再表示の際の認識用に、負の仮番号を与えておく。
                setInfo[i].ID = -1 * (i + 1);
            }

            foreach (ChSet5Item info in ChSet5.Instance.ChList.Values)
            {
                int i = 3;//その他
                if (info.IsTere == true)//地デジ
                {
                    i = 0;
                }
                else if (info.IsBS == true)//BS
                {
                    i = 1;
                }
                else if (info.IsCS == true)//CS
                {
                    i = 2;
                }

                setInfo[i].ViewServiceList.Add(info.Key);
            }

            return setInfo.Where(info => info.ViewServiceList.Count != 0).ToList();
        }
        
        void h_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            try
            {
                if (sender.GetType() == typeof(Hyperlink))
                {
                    Hyperlink h = sender as Hyperlink;
                    System.Diagnostics.Process.Start(h.NavigateUri.ToString());
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        public string GetFolderNameByDialog(string InitialPath = "", string Description = "")
        {
            try
            {
                if (Settings.Instance.OpenFolderWithFileDialog == true)
                {
                    System.Windows.Forms.OpenFileDialog dlg = new System.Windows.Forms.OpenFileDialog();
                    dlg.Title = Description;
                    dlg.CheckFileExists = false;
                    dlg.DereferenceLinks = false;
                    dlg.FileName = "(任意ファイル名)";
                    dlg.InitialDirectory = GetDirectoryName2(InitialPath);
                    if (dlg.ShowDialog() == System.Windows.Forms.DialogResult.OK)
                    {
                        return GetDirectoryName2(dlg.FileName);
                    }
                }
                else
                {
                    System.Windows.Forms.FolderBrowserDialog dlg = new System.Windows.Forms.FolderBrowserDialog();
                    dlg.Description = Description;
                    dlg.SelectedPath = GetDirectoryName2(InitialPath);
                    if (dlg.ShowDialog() == System.Windows.Forms.DialogResult.OK)
                    {
                        return dlg.SelectedPath;
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }

            return null;
        }

        public string GetFileNameByDialog(string InitialPath = "", string Title = "", string DefaultExt = "")
        {
            try
            {
                System.Windows.Forms.OpenFileDialog dlg = new System.Windows.Forms.OpenFileDialog();
                dlg.Title = Title;
                dlg.FileName = System.IO.Path.GetFileName(InitialPath);
                dlg.InitialDirectory = GetDirectoryName2(InitialPath);
                switch (DefaultExt)
                {
                    case ".exe":
                        dlg.DefaultExt = ".exe";
                        dlg.Filter = "exe Files (.exe)|*.exe;|all Files(*.*)|*.*";
                        break;
                    case ".bat":
                        dlg.DefaultExt = ".bat";
                        dlg.Filter = "bat Files (.bat)|*.bat;|all Files(*.*)|*.*";
                        break;
                    default:
                        dlg.Filter = "all Files(*.*)|*.*";
                        break;
                }

                if (dlg.ShowDialog() == System.Windows.Forms.DialogResult.OK)
                {
                    return dlg.FileName;
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }

            return null;
        }

        private string GetDirectoryName2(string folder_path)
        {
            string path = folder_path.Trim();
            while (path != "")
            {
                if (System.IO.Directory.Exists(path)) break;
                path = System.IO.Path.GetDirectoryName(path);
                path = (path != null ? path : "");
            }
            return path;
        }

        public String GetRecPath(String path)
        {
            var nwPath = "";
            try
            {
                if (String.IsNullOrWhiteSpace(path) == true) return "";
                if (CommonManager.Instance.NWMode != true) return path;
                CtrlCmd.SendGetRecFileNetworkPath(path, ref nwPath);
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            return nwPath;
        }

        public void OpenFolder(String folderPath, String title = "フォルダを開く")
        {
            try
            {
                String path1 = GetRecPath(folderPath);
                bool isFile = File.Exists(path1) == true;//録画結果から開く場合
                String path = isFile == true ? path1 : GetDirectoryName2(path1);//録画フォルダ未作成への対応
                bool noParent = path.TrimEnd('\\').CompareTo(path1.TrimEnd('\\')) != 0;//フォルダを遡った場合の特例

                if (String.IsNullOrWhiteSpace(path) == true)
                {
                    MessageBox.Show("パスが見つかりません。\r\n\r\n" + folderPath, title, MessageBoxButton.OK, MessageBoxImage.Information);
                }
                else
                {
                    //オプションに応じて一つ上のフォルダから対象フォルダを選択した状態で開く。
                    String cmd = isFile == true || noParent == false && Settings.Instance.MenuSet.OpenParentFolder == true ? "/select," : "";
                    Process.Start("EXPLORER.EXE", cmd + "\"" + path + "\"");
                }
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        public void FilePlay(ReserveData data)
        {
            if (data == null || data.RecSetting == null || data.IsEnabled == false) return;
            if (data.IsOnRec() == false)
            {
                MessageBox.Show("まだ録画が開始されていません。", "追っかけ再生", MessageBoxButton.OK, MessageBoxImage.Information);
                return;
            }
            if (data.RecSetting.RecMode == 4)//視聴モード
            {
                TVTestCtrl.SetLiveCh(data.OriginalNetworkID, data.TransportStreamID, data.ServiceID);
                return;
            }

            if (Settings.Instance.FilePlayOnAirWithExe && (NWMode == false || Settings.Instance.FilePlayOnNwWithExe == true))
            {
                //ファイルパスを取得するため開いてすぐ閉じる
                var info = new NWPlayTimeShiftInfo();
                if (CtrlCmd.SendNwTimeShiftOpen(data.ReserveID, ref info) == ErrCode.CMD_SUCCESS)
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
                TVTestCtrl.StartTimeShift(data.ReserveID);
            }
        }
        public void FilePlay(String filePath)
        {
            try
            {
                if (string.IsNullOrWhiteSpace(filePath) == true) return;

                if (NWMode == true && Settings.Instance.FilePlayOnNwWithExe == false)
                {
                    TVTestCtrl.StartStreamingPlay(filePath, NW.ConnectedIP, NW.ConnectedPort);
                }
                else
                {
                    //録画フォルダと保存・共有フォルダが異なる場合($FileNameExt$運用など)で、
                    //コマンドラインの一部になるときは、ファイルの確認を未チェックとする。
                    String path = GetRecPath(filePath);
                    String cmdLine = Settings.Instance.FilePlayCmd == "" ? "$FilePath$" : Settings.Instance.FilePlayCmd;
                    bool chkExist = cmdLine.Contains("$FilePath$") == true && cmdLine.Contains("$FileNameExt$") == false;

                    String title = "録画ファイルの再生";
                    String msg1 = "録画ファイルが見つかりません。\r\n\r\n" + filePath;
                    String msg2 = "再生アプリが見つかりません。\r\n設定を確認してください。\r\n\r\n" + Settings.Instance.FilePlayExe;

                    if (File.Exists(path) == false)
                    {
                        if (chkExist == true)
                        {
                            MessageBox.Show(msg1, title, MessageBoxButton.OK, MessageBoxImage.Information);
                            return;
                        }
                        path = filePath;
                    }

                    //'$'->'\t'は再帰的な展開を防ぐため
                    cmdLine = cmdLine.Replace("$FileNameExt$", Path.GetFileName(path).Replace('$', '\t'));
                    cmdLine = cmdLine.Replace("$FilePath$", path).Replace('\t', '$');

                    if (Settings.Instance.FilePlayExe.Length == 0)
                    {
                        if (File.Exists(cmdLine) == false)
                        {
                            MessageBox.Show(msg1, title, MessageBoxButton.OK, MessageBoxImage.Information);
                            return;
                        }
                        Process.Start(cmdLine);
                    }
                    else
                    {
                        if (File.Exists(Settings.Instance.FilePlayExe) == false)
                        {
                            MessageBox.Show(msg2, title, MessageBoxButton.OK, MessageBoxImage.Information);
                            return;
                        }
                        Process.Start(Settings.Instance.FilePlayExe, cmdLine);
                    }
                }
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }
        
        //ReloadCustContentColorList()用のコンバートメソッド
        private Brush _GetColorBrush(string colorName, uint colorValue = 0
            , bool gradation = false, double luminance = -1, double saturation = -1)
        {
            Color c = (colorName == "カスタム" ? ColorDef.FromUInt(colorValue) : ColorDef.FromName(colorName));
            if (gradation == false)
            {
                return ColorDef.SolidBrush(c);
            }
            else
            {
                if (luminance == -1)
                {
                    return ColorDef.GradientBrush(c);
                }
                else
                {
                    return ColorDef.GradientBrush(c, luminance, saturation);
                }
            }
        }
        public void ReloadCustContentColorList()
        {
            try
            {
                CustContentColorList.Clear();
                for (int i = 0; i < Settings.Instance.ContentColorList.Count; i++)
                {
                    CustContentColorList.Add(_GetColorBrush(Settings.Instance.ContentColorList[i], Settings.Instance.ContentCustColorList[i], Settings.Instance.EpgGradation));
                }
                CustContentColorList.Add(_GetColorBrush(Settings.Instance.ReserveRectColorNormal, Settings.Instance.ContentCustColorList[0x11]));
                CustContentColorList.Add(_GetColorBrush(Settings.Instance.ReserveRectColorNo, Settings.Instance.ContentCustColorList[0x12]));
                CustContentColorList.Add(_GetColorBrush(Settings.Instance.ReserveRectColorNoTuner, Settings.Instance.ContentCustColorList[0x13]));
                CustContentColorList.Add(_GetColorBrush(Settings.Instance.ReserveRectColorWarning, Settings.Instance.ContentCustColorList[0x14]));
                CustContentColorList.Add(_GetColorBrush(Settings.Instance.ReserveRectColorAutoAddMissing, Settings.Instance.ContentCustColorList[0x15]));

                CustTitle1Color = (SolidColorBrush)_GetColorBrush(Settings.Instance.TitleColor1, Settings.Instance.TitleCustColor1);
                CustTitle2Color = (SolidColorBrush)_GetColorBrush(Settings.Instance.TitleColor2, Settings.Instance.TitleCustColor2);
                CustTunerServiceColor = (SolidColorBrush)_GetColorBrush(Settings.Instance.TunerServiceColors[0], Settings.Instance.TunerServiceCustColors[0]);
                CustTunerTextColor = (SolidColorBrush)_GetColorBrush(Settings.Instance.TunerServiceColors[1], Settings.Instance.TunerServiceCustColors[1]);

                CustTunerServiceColorPri.Clear();
                for (int i = 2; i < 2 + 5; i++)
                {
                    CustTunerServiceColorPri.Add((SolidColorBrush)_GetColorBrush(Settings.Instance.TunerServiceColors[i], Settings.Instance.TunerServiceCustColors[i]));
                }

                CustTimeColorList.Clear();
                for (int i = 0; i < Settings.Instance.EpgEtcColors.Count; i++)
                {
                    CustTimeColorList.Add(_GetColorBrush(Settings.Instance.EpgEtcColors[i], Settings.Instance.EpgEtcCustColors[i], Settings.Instance.EpgGradationHeader));
                }

                CustServiceColor = _GetColorBrush(Settings.Instance.EpgEtcColors[4], Settings.Instance.EpgEtcCustColors[4], Settings.Instance.EpgGradationHeader, 1.0, 2.0);

                RecEndDefBackColor = (SolidColorBrush)_GetColorBrush(Settings.Instance.RecEndColors[0], Settings.Instance.RecEndCustColors[0]);
                RecEndErrBackColor = (SolidColorBrush)_GetColorBrush(Settings.Instance.RecEndColors[1], Settings.Instance.RecEndCustColors[1]);
                RecEndWarBackColor = (SolidColorBrush)_GetColorBrush(Settings.Instance.RecEndColors[2], Settings.Instance.RecEndCustColors[2]);


                ListDefForeColor = (SolidColorBrush)_GetColorBrush(Settings.Instance.ListDefColor, Settings.Instance.ListDefCustColor);

                RecModeForeColor.Clear();
                for (int i = 0; i < Settings.Instance.RecModeFontColors.Count; i++)
                {
                    RecModeForeColor.Add((SolidColorBrush)_GetColorBrush(Settings.Instance.RecModeFontColors[i], Settings.Instance.RecModeFontCustColors[i]));
                }

                ResDefBackColor = (SolidColorBrush)_GetColorBrush(Settings.Instance.ResBackColors[0], Settings.Instance.ResBackCustColors[0]);
                ResNoBackColor = (SolidColorBrush)_GetColorBrush(Settings.Instance.ResBackColors[1], Settings.Instance.ResBackCustColors[1]);
                ResErrBackColor = (SolidColorBrush)_GetColorBrush(Settings.Instance.ResBackColors[2], Settings.Instance.ResBackCustColors[2]);
                ResWarBackColor = (SolidColorBrush)_GetColorBrush(Settings.Instance.ResBackColors[3], Settings.Instance.ResBackCustColors[3]);
                ResAutoAddMissingBackColor = (SolidColorBrush)_GetColorBrush(Settings.Instance.ResBackColors[4], Settings.Instance.ResBackCustColors[4]);

                StatResForeColor = (SolidColorBrush)_GetColorBrush(Settings.Instance.StatColors[0], Settings.Instance.StatCustColors[0]);
                StatRecForeColor = (SolidColorBrush)_GetColorBrush(Settings.Instance.StatColors[1], Settings.Instance.StatCustColors[1]);
                StatOnAirForeColor = (SolidColorBrush)_GetColorBrush(Settings.Instance.StatColors[2], Settings.Instance.StatCustColors[2]);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        public void AddNotifySave(NotifySrvInfo notifyInfo)
        {
            if (Settings.Instance.AutoSaveNotifyLog == 1)
            {
                String filePath = SettingPath.ModulePath + "\\Log";
                Directory.CreateDirectory(filePath);
                filePath += "\\EpgTimerNotify_" + DateTime.Now.ToString("yyyyMMdd") + ".txt";
                using (var file = new StreamWriter(filePath, true))
                {
                    file.Write((new NotifySrvInfoItem(notifyInfo)).FileLogText);
                }
            }
        }

        public List<string> GetBonFileList()
        {
            var list = new List<string>();

            try
            {
                if (CommonManager.Instance.NWMode == false)
                {
                    foreach (string info in Directory.GetFiles(SettingPath.SettingFolderPath, "*.ChSet4.txt"))
                    {
                        list.Add(GetBonFileName(System.IO.Path.GetFileName(info)) + ".dll");
                    }
                }
                else
                {
                    //EpgTimerが作成したEpgTimerSrv.iniからBonDriverセクションを拾い出す
                    //将来にわたって確実なリストアップではないし、本来ならSendEnumPlugIn()あたりを変更して取得すべきだが、
                    //参考表示なので構わない
                    using (var reader = (new System.IO.StreamReader(SettingPath.TimerSrvIniPath, Encoding.Default)))
                    {
                        while (reader.Peek() >= 0)
                        {
                            string buff = reader.ReadLine();
                            int start = buff.IndexOf('[');
                            int end = buff.LastIndexOf(".dll]");
                            if (start >= 0 && end >= start + 2)
                            {
                                list.Add(buff.Substring(start + 1, end + 3 - start));
                            }
                        }
                    }
                }
            }
            catch { }

            return list;
        }

        private String GetBonFileName(String src)
        {
            int pos = src.LastIndexOf(")");
            if (pos < 1)
            {
                return src;
            }

            int count = 1;
            for (int i = pos - 1; i >= 0; i--)
            {
                if (src[i] == '(')
                {
                    count--;
                }
                else if (src[i] == ')')
                {
                    count++;
                }
                if (count == 0)
                {
                    return src.Substring(0, i);
                }
            }
            return src;
        }

        public bool? AutoAddViewOrderCheckAndSave(AutoAddListView view, out Dictionary<uint, uint> changeIDTable)
        {
            changeIDTable = null;
            try
            {
                if (view == null || view.IsVisible == false || view.dragMover.NotSaved == false) return false;
                //
                var cmdPrm = new EpgCmdParam(null);
                EpgCmds.SaveOrder.Execute(cmdPrm, view);
                changeIDTable = cmdPrm.Data as Dictionary<uint, uint>;
                return true;
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            return null;
        }

    }
}
