using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Diagnostics;
using System.Windows;
using System.Windows.Input;

using CtrlCmdCLI;
using CtrlCmdCLI.Def;

namespace EpgTimer
{
    class MenuUtil
    {
        public string TrimEpgKeyword(string txtKey, bool NotToggle = false)//NotToggleはショートカット用
        {
            string txtKey1 = txtKey;
            bool setting = Settings.Instance.CmEpgKeyword_Trim;
            if (Keyboard.Modifiers == ModifierKeys.Shift && NotToggle == false)
            {
                setting=!setting;
            }

            if (setting == true)
            {
                txtKey1 = TrimKeyword(txtKey1);
            }

            return txtKey1;
        }

        public string EpgKeyword_TrimMode()
        {
            return TrimModeTooltip(Settings.Instance.CmEpgKeyword_Trim);
        }

        private string TrimModeTooltip(bool mode)
        {
            string str_mode = (mode == true ? "オン" : "オフ");
            string str_mode_toggle = (mode == false ? "'オン'" : "'オフ'");
            return "記号除去モード : " + str_mode + " (Shift+クリックで一時的に" + str_mode_toggle + ")";
        }

        public void CopyTitle2Clipboard(string txtTitle, bool NotToggle = false)
        {
            string txtTitle1 = txtTitle;
            bool setting = Settings.Instance.CmCopyTitle_Trim;
            if (Keyboard.Modifiers == ModifierKeys.Shift && NotToggle == false)
            {
                setting = !setting;
            }

            if (setting == true)
            {
                txtTitle1 = TrimKeyword(txtTitle1);
            }

            Clipboard.SetDataObject(txtTitle1, true);
        }

        public string CopyTitle_TrimMode()
        {
            return TrimModeTooltip(Settings.Instance.CmCopyTitle_Trim);
        }

        public void CopyContent2Clipboard(EpgEventInfo eventInfo, bool NotToggle = false)
        {
            String text="";

            if (eventInfo != null)
            {
                bool setting = Settings.Instance.CmCopyContentBasic;
                if (Keyboard.Modifiers == ModifierKeys.Shift && NotToggle == false)
                {
                    setting = !setting;
                }

                if (setting == true)
                {
                    //text = eventInfo.ShortInfo.text_char;
                    text = CommonManager.Instance.ConvertProgramText(eventInfo, EventInfoTextMode.BasicOnly);
                }
                else
                {
                    text = CommonManager.Instance.ConvertProgramText(eventInfo, EventInfoTextMode.All);
                }

                text = text.TrimEnd() + "\r\n";
            }

            Clipboard.SetDataObject(text, true);
        }

        public void CopyContent2Clipboard(ReserveData resInfo, bool NotToggle = false)
        {
            CopyContent2Clipboard(GetEpgEventInfo(resInfo), NotToggle);
        }

        public void CopyContent2Clipboard(RecInfoItem recInfo, bool NotToggle = false)
        {
            string text = "";

            if (recInfo != null)
            {
                bool setting = Settings.Instance.CmCopyContentBasic;
                if (Keyboard.Modifiers == ModifierKeys.Shift && NotToggle == false)
                {
                    setting = !setting;
                }

                if (setting == true)
                {
                    string[] stArrayData = recInfo.RecInfo.ProgramInfo.Replace("\r\n", "\n").Split('\n');
                    Int32 endI = Math.Min(stArrayData.Length, 3);

                    for (Int32 i = 0; i < endI; i++)
                    {
                        text += stArrayData[i] + "\r\n";
                    }
                }
                else
                {
                    text = recInfo.RecInfo.ProgramInfo;
                }

                text = text.TrimEnd() + "\r\n";
            }

            Clipboard.SetDataObject(text, true);
        }

        public string CopyContent_Mode()
        {
            string mode = (Settings.Instance.CmCopyContentBasic == true ? "基本情報のみ" : "詳細情報");
            string mode_toggle = (Settings.Instance.CmCopyContentBasic == false ? "'基本情報のみ'" : "'詳細情報'");
            return "取得モード : " + mode + " (Shift+クリックで一時的に" + mode_toggle + ")";
        }

        public void SearchText(string txtKey, bool NotToggle = false)
        {
            string txtKey1 = txtKey;
            bool setting = Settings.Instance.CmSearchTitle_Trim;
            if (Keyboard.Modifiers == ModifierKeys.Shift && NotToggle == false)
            {
                setting = !setting;
            }

            if (setting == true)
            {
                txtKey1 = TrimKeyword(txtKey1);
            }

            string txtURI = Settings.Instance.CmSearchURI;
            txtURI += UrlEncode(txtKey1, System.Text.Encoding.UTF8);

            try
            {
                System.Diagnostics.Process.Start(txtURI);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                MessageBox.Show("'検索のURI'の設定を確認してください。");
            }
        }

        public string SearchText_TrimMode()
        {
            return TrimModeTooltip(Settings.Instance.CmSearchTitle_Trim);
        }

        public string TrimKeyword(string txtKey)
        {
            //
            // 前後の記号を取り除く
            //
            // [二][字]NHKニュース7
            // ５．１[SS][字]NHK歌謡コンサート「人生の旅路に　この歌を」
            // 午後のロードショー「私がウォシャウスキー」魅惑のヒロイン特集[字][S][二]
            // 【Ｍｏｔｈｅｒ’ｓ　Ｄａｙ　Ｓｐｅｃｉａｌ】【映】バンガー・シスターズ
            string markExp1 =
                "(" +
                    "(\\[[^\\]]+\\])+" +
                    "|" +
                    "(【[^】]+】)+" +
                    "|" +
                    "(［[^］]+］)+" +
                ")";
            string[] exp = {
                                "<[^>]+>", // NHK・フジテレビが使う補足
                                "(([#＃][\\d０-９]+)|(第[\\d０-９]+話)).*", // ドラマ等の話数から後ろ全て
                                "[\\(（][\\d０-９]+[\\)）]\\s*「[^」]+」.*", // ドラマ等の話数から後ろ全て その2
                                //"「[^」]+」.*", // ドラマ等のサブタイトルから後ろ全て、ちょっと強すぎるかも
                                "[◆▽].*", // タイトルに埋め込まれた番組説明
                                "[\\[［【\\(（][^\\]］】\\)）]*リマスター[^\\]］】\\)）]*[\\]］】\\)）]",// ときどき見かけるので1
                                "(((HD)|(ＨＤ)|(ハイビジョン)|(デジタル))リマスター((HD)|(ＨＤ))?版?)|(リマスター((HD)|(ＨＤ)|版)版?)",// 同上、括弧無しは特定パタンのみ
                                "^((５．１)|(5.1)|" + markExp1 + ")+", // 先頭の記号
                                markExp1 + "$" // 末尾の記号
                                };
            foreach (string str1 in exp)
            {
                txtKey = Regex.Replace(txtKey, str1, string.Empty).Trim();
            }
            // 映画のタイトル抽出
            // TODO:正規表現を設定ファイルで変更可能にする
            string[] titleExp = {
                                "^「(?<Title>[^」]+)」",
                                "((サタ☆シネ)|(シネマズ?)|(シアター)|(プレミア)|(ロードショー)|(ロードSHOW!)|(午後ロード)|(木曜デラックス)|(映画天国))\\s*「(?<Title>[^」]+)」",
                                "((シネマ)|(キネマ)).*『(?<Title>[^』]+)』"
                                };
            foreach (string str1 in titleExp)
            {
                Match m = Regex.Match(txtKey, str1);
                if (m.Success == true)
                {
                    txtKey = m.Groups["Title"].Value;
                    break;
                }
            }

            return txtKey;
        }

        //
        // HttpUtility を使わないUrlEncodeの実装
        // From http://d.hatena.ne.jp/kazuv3/20080605/1212656674
        //
        public string UrlEncode(string s, System.Text.Encoding enc)
        {
            System.Text.StringBuilder rt = new System.Text.StringBuilder();
            foreach (byte i in enc.GetBytes(s))
                if (i == 0x20)
                    rt.Append('+');
                else if (i >= 0x30 && i <= 0x39 || i >= 0x41 && i <= 0x5a || i >= 0x61 && i <= 0x7a)
                    rt.Append((char)i);
                else
                    rt.Append("%" + i.ToString("X2"));
            return rt.ToString();
        }
        
        public EpgEventInfo GetEpgEventInfo(ReserveData info)
        {
            CtrlCmdUtil cmd = CommonManager.Instance.CtrlCmd;
            EpgEventInfo eventInfo = null;

            if (info != null)
            {
                try
                {
                    if (info.EventID != 0xFFFF)
                    {
                        UInt64 key = CommonManager.Create64Key(info.OriginalNetworkID, info.TransportStreamID, info.ServiceID);
                        if (CommonManager.Instance.DB.ServiceEventList.ContainsKey(key) == true)
                        {
                            foreach (EpgEventInfo eventChkInfo in CommonManager.Instance.DB.ServiceEventList[key].eventList)
                            {
                                if (eventChkInfo.event_id == info.EventID)
                                {
                                    eventInfo = eventChkInfo;
                                    break;
                                }
                            }
                        }
                        if (eventInfo == null)
                        {
                            UInt64 pgId = CommonManager.Create64PgKey(info.OriginalNetworkID, info.TransportStreamID, info.ServiceID, info.EventID);
                            eventInfo = new EpgEventInfo();
                            cmd.SendGetPgInfo(pgId, ref eventInfo);
                        }
                    }
                }
                catch (Exception ex)
                {
                    MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                }

            }

            return eventInfo;
        }

    }
}
