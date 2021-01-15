using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Windows;
using System.Windows.Documents;
using System.IO;

namespace EpgTimer
{
    class CommonManager
    {
        public DBManager DB { get; private set; }
        public TVTestCtrlClass TVTestCtrl { get; private set; }
        public System.Diagnostics.Process SrvSettingProcess { get; set; }
        public IDictionary<ushort, string> ContentKindDictionary { get; private set; }
        public IEnumerable<ushort> ContentKindList
        {
            get
            {
                //「その他」をラストへ。各々大分類を前へ
                foreach (ushort id in ContentKindDictionary.Keys)
                {
                    if ((id & 0xFF) == 0)
                    {
                        yield return (ushort)(id | 0xFF);
                    }
                    if ((id & 0xFF) != 0xFF)
                    {
                        yield return id;
                    }
                }
                yield return 0x0FFF;
                yield return 0xFFFF;
            }
        }
        public IDictionary<ushort, string> ComponentKindDictionary { get; private set; }
        public string[] RecModeList { get; private set; }
        public bool NWMode { get; set; }
        public List<NotifySrvInfo> NotifyLogList { get; private set; }
        public System.Net.IPAddress NWConnectedIP { get; set; }
        public uint NWConnectedPort { get; set; }
        public Dictionary<char, List<KeyValuePair<string, string>>> ReplaceUrlDictionary { get; private set; }

        private static CommonManager _instance;
        public static CommonManager Instance
        {
            get
            {
                if (_instance == null)
                    _instance = new CommonManager();
                return _instance;
            }
        }

        public CommonManager()
        {
            DB = new DBManager();
            TVTestCtrl = new TVTestCtrlClass();

            ContentKindDictionary = new SortedList<ushort, string>(167)
            {
                { 0x0000, "定時・総合" },
                { 0x0001, "天気" },
                { 0x0002, "特集・ドキュメント" },
                { 0x0003, "政治・国会" },
                { 0x0004, "経済・市況" },
                { 0x0005, "海外・国際" },
                { 0x0006, "解説" },
                { 0x0007, "討論・会談" },
                { 0x0008, "報道特番" },
                { 0x0009, "ローカル・地域" },
                { 0x000A, "交通" },
                { 0x000F, "その他" },
                { 0x00FF, "ニュース／報道" },

                { 0x0100, "スポーツニュース" },
                { 0x0101, "野球" },
                { 0x0102, "サッカー" },
                { 0x0103, "ゴルフ" },
                { 0x0104, "その他の球技" },
                { 0x0105, "相撲・格闘技" },
                { 0x0106, "オリンピック・国際大会" },
                { 0x0107, "マラソン・陸上・水泳" },
                { 0x0108, "モータースポーツ" },
                { 0x0109, "マリン・ウィンタースポーツ" },
                { 0x010A, "競馬・公営競技" },
                { 0x010F, "その他" },
                { 0x01FF, "スポーツ" },

                { 0x0200, "芸能・ワイドショー" },
                { 0x0201, "ファッション" },
                { 0x0202, "暮らし・住まい" },
                { 0x0203, "健康・医療" },
                { 0x0204, "ショッピング・通販" },
                { 0x0205, "グルメ・料理" },
                { 0x0206, "イベント" },
                { 0x0207, "番組紹介・お知らせ" },
                { 0x020F, "その他" },
                { 0x02FF, "情報／ワイドショー" },

                { 0x0300, "国内ドラマ" },
                { 0x0301, "海外ドラマ" },
                { 0x0302, "時代劇" },
                { 0x030F, "その他" },
                { 0x03FF, "ドラマ" },

                { 0x0400, "国内ロック・ポップス" },
                { 0x0401, "海外ロック・ポップス" },
                { 0x0402, "クラシック・オペラ" },
                { 0x0403, "ジャズ・フュージョン" },
                { 0x0404, "歌謡曲・演歌" },
                { 0x0405, "ライブ・コンサート" },
                { 0x0406, "ランキング・リクエスト" },
                { 0x0407, "カラオケ・のど自慢" },
                { 0x0408, "民謡・邦楽" },
                { 0x0409, "童謡・キッズ" },
                { 0x040A, "民族音楽・ワールドミュージック" },
                { 0x040F, "その他" },
                { 0x04FF, "音楽" },

                { 0x0500, "クイズ" },
                { 0x0501, "ゲーム" },
                { 0x0502, "トークバラエティ" },
                { 0x0503, "お笑い・コメディ" },
                { 0x0504, "音楽バラエティ" },
                { 0x0505, "旅バラエティ" },
                { 0x0506, "料理バラエティ" },
                { 0x050F, "その他" },
                { 0x05FF, "バラエティ" },

                { 0x0600, "洋画" },
                { 0x0601, "邦画" },
                { 0x0602, "アニメ" },
                { 0x060F, "その他" },
                { 0x06FF, "映画" },

                { 0x0700, "国内アニメ" },
                { 0x0701, "海外アニメ" },
                { 0x0702, "特撮" },
                { 0x070F, "その他" },
                { 0x07FF, "アニメ／特撮" },

                { 0x0800, "社会・時事" },
                { 0x0801, "歴史・紀行" },
                { 0x0802, "自然・動物・環境" },
                { 0x0803, "宇宙・科学・医学" },
                { 0x0804, "カルチャー・伝統文化" },
                { 0x0805, "文学・文芸" },
                { 0x0806, "スポーツ" },
                { 0x0807, "ドキュメンタリー全般" },
                { 0x0808, "インタビュー・討論" },
                { 0x080F, "その他" },
                { 0x08FF, "ドキュメンタリー／教養" },

                { 0x0900, "現代劇・新劇" },
                { 0x0901, "ミュージカル" },
                { 0x0902, "ダンス・バレエ" },
                { 0x0903, "落語・演芸" },
                { 0x0904, "歌舞伎・古典" },
                { 0x090F, "その他" },
                { 0x09FF, "劇場／公演" },

                { 0x0A00, "旅・釣り・アウトドア" },
                { 0x0A01, "園芸・ペット・手芸" },
                { 0x0A02, "音楽・美術・工芸" },
                { 0x0A03, "囲碁・将棋" },
                { 0x0A04, "麻雀・パチンコ" },
                { 0x0A05, "車・オートバイ" },
                { 0x0A06, "コンピュータ・ＴＶゲーム" },
                { 0x0A07, "会話・語学" },
                { 0x0A08, "幼児・小学生" },
                { 0x0A09, "中学生・高校生" },
                { 0x0A0A, "大学生・受験" },
                { 0x0A0B, "生涯教育・資格" },
                { 0x0A0C, "教育問題" },
                { 0x0A0F, "その他" },
                { 0x0AFF, "趣味／教育" },

                { 0x0B00, "高齢者" },
                { 0x0B01, "障害者" },
                { 0x0B02, "社会福祉" },
                { 0x0B03, "ボランティア" },
                { 0x0B04, "手話" },
                { 0x0B05, "文字（字幕）" },
                { 0x0B06, "音声解説" },
                { 0x0B0F, "その他" },
                { 0x0BFF, "福祉" },

                { 0x0FFF, "その他" },

                { 0x6000, "中止の可能性あり" },
                { 0x6001, "延長の可能性あり" },
                { 0x6002, "中断の可能性あり" },
                { 0x6003, "別話数放送の可能性あり" },
                { 0x6004, "編成未定枠" },
                { 0x6005, "繰り上げの可能性あり" },
                { 0x60FF, "編成情報" },

                { 0x6100, "中断ニュースあり" },
                { 0x6101, "臨時サービスあり" },
                { 0x61FF, "特性情報" },

                { 0x6200, "3D映像あり" },
                { 0x62FF, "3D映像" },

                { 0x7000, "テニス" },
                { 0x7001, "バスケットボール" },
                { 0x7002, "ラグビー" },
                { 0x7003, "アメリカンフットボール" },
                { 0x7004, "ボクシング" },
                { 0x7005, "プロレス" },
                { 0x700F, "その他" },
                { 0x70FF, "スポーツ(CS)" },

                { 0x7100, "アクション" },
                { 0x7101, "SF／ファンタジー" },
                { 0x7102, "コメディー" },
                { 0x7103, "サスペンス／ミステリー" },
                { 0x7104, "恋愛／ロマンス" },
                { 0x7105, "ホラー／スリラー" },
                { 0x7106, "ウエスタン" },
                { 0x7107, "ドラマ／社会派ドラマ" },
                { 0x7108, "アニメーション" },
                { 0x7109, "ドキュメンタリー" },
                { 0x710A, "アドベンチャー／冒険" },
                { 0x710B, "ミュージカル／音楽映画" },
                { 0x710C, "ホームドラマ" },
                { 0x710F, "その他" },
                { 0x71FF, "洋画(CS)" },

                { 0x7200, "アクション" },
                { 0x7201, "SF／ファンタジー" },
                { 0x7202, "お笑い／コメディー" },
                { 0x7203, "サスペンス／ミステリー" },
                { 0x7204, "恋愛／ロマンス" },
                { 0x7205, "ホラー／スリラー" },
                { 0x7206, "青春／学園／アイドル" },
                { 0x7207, "任侠／時代劇" },
                { 0x7208, "アニメーション" },
                { 0x7209, "ドキュメンタリー" },
                { 0x720A, "アドベンチャー／冒険" },
                { 0x720B, "ミュージカル／音楽映画" },
                { 0x720C, "ホームドラマ" },
                { 0x720F, "その他" },
                { 0x72FF, "邦画(CS)" },

                { 0xFFFF, "なし" }
            };

            {
                ComponentKindDictionary = new SortedList<ushort, string>(75)
                {
                    { 0x0101, "480i(525i)、アスペクト比4:3" },
                    { 0x0102, "480i(525i)、アスペクト比16:9 パンベクトルあり" },
                    { 0x0103, "480i(525i)、アスペクト比16:9 パンベクトルなし" },
                    { 0x0104, "480i(525i)、アスペクト比 > 16:9" },
                    { 0x0191, "2160p、アスペクト比4:3" },
                    { 0x0192, "2160p、アスペクト比16:9 パンベクトルあり" },
                    { 0x0193, "2160p、アスペクト比16:9 パンベクトルなし" },
                    { 0x0194, "2160p、アスペクト比 > 16:9" },
                    { 0x01A1, "480p(525p)、アスペクト比4:3" },
                    { 0x01A2, "480p(525p)、アスペクト比16:9 パンベクトルあり" },
                    { 0x01A3, "480p(525p)、アスペクト比16:9 パンベクトルなし" },
                    { 0x01A4, "480p(525p)、アスペクト比 > 16:9" },
                    { 0x01B1, "1080i(1125i)、アスペクト比4:3" },
                    { 0x01B2, "1080i(1125i)、アスペクト比16:9 パンベクトルあり" },
                    { 0x01B3, "1080i(1125i)、アスペクト比16:9 パンベクトルなし" },
                    { 0x01B4, "1080i(1125i)、アスペクト比 > 16:9" },
                    { 0x01C1, "720p(750p)、アスペクト比4:3" },
                    { 0x01C2, "720p(750p)、アスペクト比16:9 パンベクトルあり" },
                    { 0x01C3, "720p(750p)、アスペクト比16:9 パンベクトルなし" },
                    { 0x01C4, "720p(750p)、アスペクト比 > 16:9" },
                    { 0x01D1, "240p アスペクト比4:3" },
                    { 0x01D2, "240p アスペクト比16:9 パンベクトルあり" },
                    { 0x01D3, "240p アスペクト比16:9 パンベクトルなし" },
                    { 0x01D4, "240p アスペクト比 > 16:9" },
                    { 0x01E1, "1080p(1125p)、アスペクト比4:3" },
                    { 0x01E2, "1080p(1125p)、アスペクト比16:9 パンベクトルあり" },
                    { 0x01E3, "1080p(1125p)、アスペクト比16:9 パンベクトルなし" },
                    { 0x01E4, "1080p(1125p)、アスペクト比 > 16:9" },
                    { 0x0201, "1/0モード（シングルモノ）" },
                    { 0x0202, "1/0＋1/0モード（デュアルモノ）" },
                    { 0x0203, "2/0モード（ステレオ）" },
                    { 0x0204, "2/1モード" },
                    { 0x0205, "3/0モード" },
                    { 0x0206, "2/2モード" },
                    { 0x0207, "3/1モード" },
                    { 0x0208, "3/2モード" },
                    { 0x0209, "3/2＋LFEモード（3/2.1モード）" },
                    { 0x020A, "3/3.1モード" },
                    { 0x020B, "2/0/0-2/0/2-0.1モード" },
                    { 0x020C, "5/2.1モード" },
                    { 0x020D, "3/2/2.1モード" },
                    { 0x020E, "2/0/0-3/0/2-0.1モード" },
                    { 0x020F, "0/2/0-3/0/2-0.1モード" },
                    { 0x0210, "2/0/0-3/2/3-0.2モード" },
                    { 0x0211, "3/3/3-5/2/3-3/0/0.2モード" },
                    { 0x0240, "視覚障害者用音声解説" },
                    { 0x0241, "聴覚障害者用音声" },
                    { 0x0501, "H.264|MPEG-4 AVC、480i(525i)、アスペクト比4:3" },
                    { 0x0502, "H.264|MPEG-4 AVC、480i(525i)、アスペクト比16:9 パンベクトルあり" },
                    { 0x0503, "H.264|MPEG-4 AVC、480i(525i)、アスペクト比16:9 パンベクトルなし" },
                    { 0x0504, "H.264|MPEG-4 AVC、480i(525i)、アスペクト比 > 16:9" },
                    { 0x0591, "H.264|MPEG-4 AVC、2160p、アスペクト比4:3" },
                    { 0x0592, "H.264|MPEG-4 AVC、2160p、アスペクト比16:9 パンベクトルあり" },
                    { 0x0593, "H.264|MPEG-4 AVC、2160p、アスペクト比16:9 パンベクトルなし" },
                    { 0x0594, "H.264|MPEG-4 AVC、2160p、アスペクト比 > 16:9" },
                    { 0x05A1, "H.264|MPEG-4 AVC、480p(525p)、アスペクト比4:3" },
                    { 0x05A2, "H.264|MPEG-4 AVC、480p(525p)、アスペクト比16:9 パンベクトルあり" },
                    { 0x05A3, "H.264|MPEG-4 AVC、480p(525p)、アスペクト比16:9 パンベクトルなし" },
                    { 0x05A4, "H.264|MPEG-4 AVC、480p(525p)、アスペクト比 > 16:9" },
                    { 0x05B1, "H.264|MPEG-4 AVC、1080i(1125i)、アスペクト比4:3" },
                    { 0x05B2, "H.264|MPEG-4 AVC、1080i(1125i)、アスペクト比16:9 パンベクトルあり" },
                    { 0x05B3, "H.264|MPEG-4 AVC、1080i(1125i)、アスペクト比16:9 パンベクトルなし" },
                    { 0x05B4, "H.264|MPEG-4 AVC、1080i(1125i)、アスペクト比 > 16:9" },
                    { 0x05C1, "H.264|MPEG-4 AVC、720p(750p)、アスペクト比4:3" },
                    { 0x05C2, "H.264|MPEG-4 AVC、720p(750p)、アスペクト比16:9 パンベクトルあり" },
                    { 0x05C3, "H.264|MPEG-4 AVC、720p(750p)、アスペクト比16:9 パンベクトルなし" },
                    { 0x05C4, "H.264|MPEG-4 AVC、720p(750p)、アスペクト比 > 16:9" },
                    { 0x05D1, "H.264|MPEG-4 AVC、240p アスペクト比4:3" },
                    { 0x05D2, "H.264|MPEG-4 AVC、240p アスペクト比16:9 パンベクトルあり" },
                    { 0x05D3, "H.264|MPEG-4 AVC、240p アスペクト比16:9 パンベクトルなし" },
                    { 0x05D4, "H.264|MPEG-4 AVC、240p アスペクト比 > 16:9" },
                    { 0x05E1, "H.264|MPEG-4 AVC、1080p(1125p)、アスペクト比4:3" },
                    { 0x05E2, "H.264|MPEG-4 AVC、1080p(1125p)、アスペクト比16:9 パンベクトルあり" },
                    { 0x05E3, "H.264|MPEG-4 AVC、1080p(1125p)、アスペクト比16:9 パンベクトルなし" },
                    { 0x05E4, "H.264|MPEG-4 AVC、1080p(1125p)、アスペクト比 > 16:9" }
                };
            }
            RecModeList = new string[] { "全サービス", "指定サービス", "全サービス(デコード処理なし)", "指定サービス(デコード処理なし)", "視聴" };
            NWMode = false;
            NotifyLogList = new List<NotifySrvInfo>();
            ReplaceUrlDictionary = CreateReplaceDictionary(",０,0,１,1,２,2,３,3,４,4,５,5,６,6,７,7,８,8,９,9" +
                ",Ａ,A,Ｂ,B,Ｃ,C,Ｄ,D,Ｅ,E,Ｆ,F,Ｇ,G,Ｈ,H,Ｉ,I,Ｊ,J,Ｋ,K,Ｌ,L,Ｍ,M,Ｎ,N,Ｏ,O,Ｐ,P,Ｑ,Q,Ｒ,R,Ｓ,S,Ｔ,T,Ｕ,U,Ｖ,V,Ｗ,W,Ｘ,X,Ｙ,Y,Ｚ,Z" +
                ",ａ,a,ｂ,b,ｃ,c,ｄ,d,ｅ,e,ｆ,f,ｇ,g,ｈ,h,ｉ,i,ｊ,j,ｋ,k,ｌ,l,ｍ,m,ｎ,n,ｏ,o,ｐ,p,ｑ,q,ｒ,r,ｓ,s,ｔ,t,ｕ,u,ｖ,v,ｗ,w,ｘ,x,ｙ,y,ｚ,z" +
                ",！,!,＃,#,＄,$,％,%,＆,&,’,',（,(,）,),～,~,￣,~,＝,=,＠,@,；,;,：,:,？,?,＿,_,＋,+,－,-,＊,*,／,/,．,.");
        }

        public static CtrlCmdUtil CreateSrvCtrl()
        {
            var cmd = new CtrlCmdUtil();
            if (Instance.NWMode)
            {
                cmd.SetSendMode(true);
                cmd.SetNWSetting(Instance.NWConnectedIP, Instance.NWConnectedPort);
            }
            return cmd;
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

        public static Dictionary<char, List<KeyValuePair<string, string>>> CreateReplaceDictionary(string pattern)
        {
            var ret = new Dictionary<char, List<KeyValuePair<string, string>>>();
            if (pattern.Length > 0)
            {
                string[] arr = pattern.Substring(1).Split(pattern[0]);
                for (int i = 0; i + 1 < arr.Length; i += 2)
                {
                    //先頭文字で仕分けする
                    if (arr[i].Length > 0)
                    {
                        List<KeyValuePair<string, string>> bucket;
                        if (ret.TryGetValue(arr[i][0], out bucket) == false)
                        {
                            ret[arr[i][0]] = bucket = new List<KeyValuePair<string, string>>();
                        }
                        bucket.Add(new KeyValuePair<string, string>(arr[i], arr[i + 1]));
                    }
                }
                foreach (var bucket in ret)
                {
                    //最長一致のため
                    bucket.Value.Sort((a, b) => b.Key.Length - a.Key.Length);
                }
            }
            return ret;
        }

        public static string ReplaceText(string text, Dictionary<char, List<KeyValuePair<string, string>>> replaceDictionary)
        {
            StringBuilder ret = null;
            if (replaceDictionary.Count > 0)
            {
                for (int i = 0; i < text.Length; i++)
                {
                    List<KeyValuePair<string, string>> bucket;
                    if (replaceDictionary.TryGetValue(text[i], out bucket))
                    {
                        int j = bucket.FindIndex(p => string.CompareOrdinal(text, i, p.Key, 0, p.Key.Length) == 0);
                        if (j >= 0)
                        {
                            if (ret == null)
                            {
                                ret = new StringBuilder(text, 0, i, text.Length);
                            }
                            ret.Append(bucket[j].Value);
                            i += bucket[j].Key.Length - 1;
                            continue;
                        }
                    }
                    if (ret != null)
                    {
                        ret.Append(text[i]);
                    }
                }
            }
            return ret != null ? ret.ToString() : text;
        }

        public static EpgServiceInfo ConvertChSet5To(ChSet5Item item)
        {
            EpgServiceInfo info = new EpgServiceInfo();
            {
                info.ONID = item.ONID;
                info.TSID = item.TSID;
                info.SID = item.SID;
                info.network_name = item.NetworkName;
                info.partialReceptionFlag = (byte)(item.PartialFlag ? 1 : 0);
                info.remote_control_key_id = 0;
                info.service_name = item.ServiceName;
                info.service_provider_name = item.NetworkName;
                info.service_type = (byte)item.ServiceType;
                info.ts_name = item.NetworkName;
            }
            return info;
        }

        public static string GetErrCodeText(ErrCode err)
        {
            switch (err)
            {
                case ErrCode.CMD_NON_SUPPORT:
                    return "EpgTimerSrvがサポートしていないコマンドです。";
                case ErrCode.CMD_ERR_CONNECT:
                    return "EpgTimerSrvに接続できませんでした。";
                case ErrCode.CMD_ERR_DISCONNECT:
                    return "EpgTimerSrvとの接続がリセットされた可能性があります。";
                case ErrCode.CMD_ERR_TIMEOUT:
                    return "EpgTimerSrvとの接続にタイムアウトしました。";
                case ErrCode.CMD_ERR_BUSY:
                    //このエラーはコマンドによって解釈が異なる
                    return null;
                default:
                    return null;
            }
        }

        public class TimeDuration : IComparable<TimeDuration>, IComparable
        {
            public TimeDuration(bool timeFlag, DateTime time, bool durationFlag, double durationSec)
            {
                _time = timeFlag ? time : DateTime.MinValue;
                _durationSec = durationFlag ? durationSec : double.MinValue;
            }
            public override string ToString()
            {
                if (_time != DateTime.MinValue)
                {
                    return _time.ToString("yyyy\\/MM\\/dd(ddd) HH\\:mm\\:ss") + (double.IsNaN(_durationSec) ? "" :
                           _durationSec != double.MinValue ? _time.AddSeconds(_durationSec).ToString(" ～ HH\\:mm\\:ss") : " ～ 未定");
                }
                return "未定";
            }
            public int CompareTo(TimeDuration other)
            {
                return other == null ? 1 : _time == other._time ? _durationSec.CompareTo(other._durationSec) : _time.CompareTo(other._time);
            }
            public int CompareTo(object other)
            {
                return CompareTo(other as TimeDuration);
            }
            private DateTime _time;
            private double _durationSec;
        }

        public String ConvertReserveText(ReserveData reserveInfo)
        {
            String view = new TimeDuration(true, reserveInfo.StartTime, true, reserveInfo.DurationSecond) + "\r\n";
            view += reserveInfo.StationName;
            view += " (" + ConvertNetworkNameText(reserveInfo.OriginalNetworkID) + ")" + "\r\n";

            view += reserveInfo.Title + "\r\n\r\n";
            view += ConvertRecSettingText(reserveInfo.RecSetting) + "\r\n" +
                    "予約状況 : " + reserveInfo.Comment +
                    "\r\n\r\n" +
                    "OriginalNetworkID : " + reserveInfo.OriginalNetworkID.ToString() + " (0x" + reserveInfo.OriginalNetworkID.ToString("X4") + ")\r\n" +
                    "TransportStreamID : " + reserveInfo.TransportStreamID.ToString() + " (0x" + reserveInfo.TransportStreamID.ToString("X4") + ")\r\n" +
                    "ServiceID : " + reserveInfo.ServiceID.ToString() + " (0x" + reserveInfo.ServiceID.ToString("X4") + ")\r\n" +
                    "EventID : " + reserveInfo.EventID.ToString() + " (0x" + reserveInfo.EventID.ToString("X4") + ")";
            return view;
        }

        public string ConvertRecSettingText(RecSettingData recSetting)
        {
            string view = "有効 : " + (recSetting.IsNoRec() ? "いいえ" : "はい") + "\r\n" +
                          "録画モード : " + RecModeList[recSetting.GetRecMode()] + "\r\n" +
                          "優先度 : " + recSetting.Priority.ToString() + "\r\n" +
                          "追従 : " + (recSetting.TuijyuuFlag == 0 ? "しない" : "する") + "\r\n" +
                          "ぴったり（？） : " + (recSetting.PittariFlag == 0 ? "しない" : "する") + "\r\n";
            if ((recSetting.ServiceMode & 0x01) == 0)
            {
                view += "指定サービス対象データ : デフォルト\r\n";
            }
            else
            {
                view += "指定サービス対象データ :";
                if ((recSetting.ServiceMode & 0x10) != 0)
                {
                    view += " 字幕含む";
                }
                if ((recSetting.ServiceMode & 0x20) != 0)
                {
                    view += " データカルーセル含む";
                }
                view += "\r\n";
            }

            view += "録画後実行bat : " + recSetting.BatFilePath + "\r\n";

            if (recSetting.RecFolderList.Count == 0)
            {
                view += "録画フォルダ : デフォルト\r\n";
            }
            else
            {
                view += "録画フォルダ : \r\n";
                foreach (RecFileSetInfo info in recSetting.RecFolderList)
                {
                    view += info.RecFolder + " (" + info.WritePlugIn + ", " +
                            (info.RecNamePlugIn.Length > 0 ? info.RecNamePlugIn : "ファイル名PlugInなし") + ")\r\n";
                }
            }

            if (recSetting.UseMargineFlag == 0)
            {
                view += "録画マージン : デフォルト\r\n";
            }
            else
            {
                view += "録画マージン : 開始 " + recSetting.StartMargine.ToString() +
                        " 終了 " + recSetting.EndMargine.ToString() + "\r\n";
            }

            if (recSetting.SuspendMode == 0)
            {
                view += "録画後動作 : デフォルト\r\n";
            }
            else
            {
                view += "録画後動作 : ";
                switch (recSetting.SuspendMode)
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
                if (recSetting.RebootFlag == 1)
                {
                    view += " 復帰後再起動する";
                }
                view += "\r\n";
            }
            if (recSetting.PartialRecFlag == 0)
            {
                view += "部分受信 : 同時出力なし\r\n";
            }
            else
            {
                view += "部分受信 : 同時出力あり\r\n" +
                        "部分受信 録画フォルダ : \r\n";
                foreach (RecFileSetInfo info in recSetting.PartialRecFolder)
                {
                    view += info.RecFolder + "(" + info.WritePlugIn + ", " +
                            (info.RecNamePlugIn.Length > 0 ? info.RecNamePlugIn : "ファイル名PlugInなし") + ")\r\n";
                }
            }
            view += "連続録画動作 : " + (recSetting.ContinueRecFlag == 0 ? "分割" : "同一ファイル出力") + "\r\n" +
                    "使用チューナー強制指定 : " + (recSetting.TunerID == 0 ? "自動" : "ID:" + recSetting.TunerID.ToString("X8"));
            return view;
        }

        public String ConvertProgramText(EpgEventInfo eventInfo, EventInfoTextMode textMode)
        {
            string retText = "";
            string basicInfo = "";
            string extInfo = "";
            if (eventInfo != null)
            {
                UInt64 key = Create64Key(eventInfo.original_network_id, eventInfo.transport_stream_id, eventInfo.service_id);
                if (ChSet5.Instance.ChList.ContainsKey(key) == true)
                {
                    basicInfo += ChSet5.Instance.ChList[key].ServiceName + "(" + ChSet5.Instance.ChList[key].NetworkName + ")" + "\r\n";
                }

                basicInfo += new TimeDuration(eventInfo.StartTimeFlag != 0, eventInfo.start_time,
                                              eventInfo.DurationFlag != 0, eventInfo.durationSec) + "\r\n";

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
                        if (nibble1 == 0x0E && nibble2 <= 0x01)
                        {
                            nibble1 = info.user_nibble_1 | (0x60 + nibble2 * 16);
                            nibble2 = info.user_nibble_2;
                        }
                        if (ContentKindDictionary.ContainsKey((ushort)(nibble1 << 8 | 0xFF)))
                        {
                            content += ContentKindDictionary[(ushort)(nibble1 << 8 | 0xFF)];
                        }
                        else
                        {
                            content += "(0x" + nibble1.ToString("X2") + ")";
                        }
                        if (ContentKindDictionary.ContainsKey((ushort)(nibble1 << 8 | nibble2)))
                        {
                            content += " - " + ContentKindDictionary[(ushort)(nibble1 << 8 | nibble2)];
                        }
                        else if (nibble1 != 0x0F)
                        {
                            content += " - (0x" + nibble2.ToString("X2") + ")";
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
                if (!ChSet5.IsDttv(eventInfo.original_network_id))
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
                            key = Create64Key(info.original_network_id, info.transport_stream_id, info.service_id);
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

        public static String ConvertNetworkNameText(ushort originalNetworkID)
        {
            String retText = "";
            if (ChSet5.IsDttv(originalNetworkID) == true)
            {
                retText = "地デジ";
            }
            else if (ChSet5.IsBS(originalNetworkID) == true)
            {
                retText = "BS";
            }
            else if (ChSet5.IsCS1(originalNetworkID) == true)
            {
                retText = "CS1";
            }
            else if (ChSet5.IsCS2(originalNetworkID) == true)
            {
                retText = "CS2";
            }
            else if (ChSet5.IsCS3(originalNetworkID) == true)
            {
                retText = "CS3";
            }
            else
            {
                retText = "その他";
            }
            return retText;
        }

        public static Paragraph ConvertDisplayText(string text)
        {
            int searchFrom = 0;
            var para = new Paragraph();
            string rtext = ReplaceText(text, CommonManager.Instance.ReplaceUrlDictionary);
            if (rtext.Length == text.Length)
            {
                for (Match m = Regex.Match(rtext, @"https?://[0-9A-Za-z!#$%&'()~=@;:?_+\-*/.]+"); m.Success; m = m.NextMatch())
                {
                    para.Inlines.Add(text.Substring(searchFrom, m.Index - searchFrom));
                    var h = new Hyperlink(new Run(text.Substring(m.Index, m.Length)));
                    h.MouseLeftButtonDown += (sender, e) =>
                    {
                        try
                        {
                            using (System.Diagnostics.Process.Start(((Hyperlink)sender).NavigateUri.ToString())) { }
                        }
                        catch (Exception ex)
                        {
                            MessageBox.Show(ex.ToString());
                        }
                    };
                    h.Foreground = SystemColors.HotTrackBrush;
                    h.Cursor = System.Windows.Input.Cursors.Hand;
                    h.NavigateUri = new Uri(m.Value);
                    para.Inlines.Add(h);
                    searchFrom = m.Index + m.Length;
                }
            }
            para.Inlines.Add(text.Substring(searchFrom));
            return para;
        }

        public void FilePlay(uint reserveID)
        {
            if (Settings.Instance.FilePlay && Settings.Instance.FilePlayOnAirWithExe)
            {
                //ファイルパスを取得するため開いてすぐ閉じる
                var info = new NWPlayTimeShiftInfo();
                try
                {
                    if (CreateSrvCtrl().SendNwTimeShiftOpen(reserveID, ref info) == ErrCode.CMD_SUCCESS)
                    {
                        CreateSrvCtrl().SendNwPlayClose(info.ctrlID);
                        if (info.filePath != "")
                        {
                            FilePlay(info.filePath);
                            return;
                        }
                    }
                }
                catch { }
                MessageBox.Show("録画ファイルの場所がわかりませんでした。", "追っかけ再生", MessageBoxButton.OK, MessageBoxImage.Information);
            }
            else
            {
                TVTestCtrl.StartStreamingPlay(null, reserveID);
            }
        }

        public void FilePlay(String filePath)
        {
            try
            {
                if (Settings.Instance.FilePlay)
                {
                    if (Settings.Instance.FilePlayExe.Length == 0)
                    {
                        using (System.Diagnostics.Process.Start(filePath)) { }
                    }
                    else
                    {
                        String cmdLine = Settings.Instance.FilePlayCmd;
                        //'$'->'\t'は再帰的な展開を防ぐため
                        cmdLine = cmdLine.Replace("$FileNameExt$", Path.GetFileName(filePath).Replace('$', '\t'));
                        cmdLine = cmdLine.Replace("$FilePath$", filePath).Replace('\t', '$');
                        using (System.Diagnostics.Process.Start(Settings.Instance.FilePlayExe, cmdLine)) { }
                    }
                }
                else
                {
                    TVTestCtrl.StartStreamingPlay(filePath, 0);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
            }
        }
    }
}
