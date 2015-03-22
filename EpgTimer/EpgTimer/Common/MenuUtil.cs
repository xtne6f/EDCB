using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Diagnostics;
using System.Windows;
using System.Windows.Input;
using System.Windows.Controls;
using System.Collections;

using CtrlCmdCLI;
using CtrlCmdCLI.Def;

namespace EpgTimer
{
    class MenuUtil
    {
        private CtrlCmdUtil cmd = null;

        public MenuUtil(CtrlCmdUtil ctrlCmd)
        {
            cmd = ctrlCmd;
        }

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
            string text = "";

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
            CopyContent2Clipboard(CommonManager.Instance.GetEpgEventInfoFromReserveData(resInfo, true), NotToggle);
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
                    int endI = Math.Min(stArrayData.Length, 3);

                    for (int i = 0; i < endI; i++)
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
                                "^((５．１)|(5.1)|" + markExp1 + ")+", // 先頭の記号
                                "^(([#＃][\\d０-９]+)|(第[\\d０-９]+話))", // 先頭にある話数の除去
                                "^[\\(（][\\d０-９]+[\\)）]\\s*「[^」]+」", // 先頭にある話数の除去
                                "(([#＃][\\d０-９]+)|(第[\\d０-９]+話)).*", // ドラマ等の話数から後ろ全て
                                "[\\(（][\\d０-９]+[\\)）]\\s*「[^」]+」.*", // ドラマ等の話数から後ろ全て その2
                                //"「[^」]+」.*", // ドラマ等のサブタイトルから後ろ全て、ちょっと強すぎるかも
                                "<[^>]+>", // NHK・フジテレビが使う補足
                                "＜[^＞]+＞", // NHK・フジテレビが使う補足
                                "[◆▽].*", // タイトルに埋め込まれた番組説明
                                "[\\[［【\\(（][^\\]］】\\)）]*リマスター[^\\]］】\\)）]*[\\]］】\\)）]",// ときどき見かけるので1
                                "(((HD)|(ＨＤ)|(ハイビジョン)|(デジタル))リマスター((HD)|(ＨＤ))?版?)|(リマスター((HD)|(ＨＤ)|版)版?)",// 同上、括弧無しは特定パタンのみ
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
        
        public string MarginStartText(RecSettingData recSetting)
        {
            string view = "";
            if (recSetting != null)
            {
                int marginTime;
                if (recSetting.UseMargineFlag == 1)
                {
                    marginTime = recSetting.StartMargine;
                }
                else
                {
                    //TODO: ここでデフォルトマージンを確認するがEpgTimerNWでは無意味。根本的にはSendCtrlCmdの拡張が必要
                    marginTime = IniFileHandler.GetPrivateProfileInt("SET", "StartMargin", 0, SettingPath.TimerSrvIniPath);
                }
                view = CustomTimeFormat(marginTime * -1, recSetting.UseMargineFlag);
            }
            return view;
        }

        public string MarginEndText(RecSettingData recSetting)
        {
            string view = "";
            if (recSetting != null)
            {
                int marginTime;
                if (recSetting.UseMargineFlag == 1)
                {
                    marginTime = recSetting.EndMargine;
                }
                else
                {
                    //TODO: ここでデフォルトマージンを確認するがEpgTimerNWでは無意味。根本的にはSendCtrlCmdの拡張が必要
                    marginTime = IniFileHandler.GetPrivateProfileInt("SET", "EndMargin", 0, SettingPath.TimerSrvIniPath);
                }
                view = CustomTimeFormat(marginTime, recSetting.UseMargineFlag);
            }
            return view;
        }

        private string CustomTimeFormat(int span, byte useMarginFlag)
        {
            string hours;
            string minutes;
            string seconds = (span % 60).ToString("00;00");
            if (Math.Abs(span) < 3600)
            {
                hours = "";
                minutes = (span / 60).ToString("0;0") + ":";
            }
            else
            {
                hours = (span / 3600).ToString("0;0") + ":";
                minutes = ((span % 3600) / 60).ToString("00;00") + ":";
            }
            return span.ToString("+;-") + hours + minutes + seconds + CustomTimeMark(useMarginFlag);
        }

        private string CustomTimeMark(byte useMarginFlag)
        {
            //EpgtimerNWの場合、デフォルト値不明のため。不明でなくなったら要らない
            string mark = "";
            if (CommonManager.Instance.NWMode == true)
            {
                mark = (useMarginFlag == 1 ? " " : "?");
            }
            return mark;
        }
        
        public TextBlock GetTooltipBlockStandard(string text)
        {
            TextBlock block = new TextBlock();
            block.Text = text;
            block.MaxWidth = 400;
            block.TextWrapping = TextWrapping.Wrap;
            return block;
        }

        public static List<T> GetList<T>(T item)
        {
            var list = new List<T>();
            list.Add(item);
            return list;
        }

        public bool ReserveAdd(EpgEventInfo item, RecSettingView recSettingView = null, object sender = null)
        {
            return ReserveAdd(GetList(item), recSettingView, sender);
        }
        public bool ReserveAdd(List<EpgEventInfo> itemlist, RecSettingView recSettingView = null, object sender = null)
        {
            try
            {
                if (itemlist.Count == 1)
                {
                    if (IsEnableReserveAdd(itemlist[0]) == false) return false;
                }

                //Memo:未使用時は即破棄、使用時もEpgTimerSrv側に送信後破棄されるので、forループの外にいられる。
                var setInfo = new RecSettingData();
                if (recSettingView != null)
                {
                    //ダイアログからの予約、SearchWindowの簡易予約
                    recSettingView.GetRecSetting(ref setInfo);
                }
                else
                {
                    uint presetID = 0;  //EPG画面の簡易予約
                    if (sender != null)
                    {
                        //コンテキストメニューからのプリセット予約
                        presetID = ReadDataContext(sender, uint.MinValue, uint.MaxValue - 1);
                        if (presetID == 0xFF) return false;

                    }
                    Settings.GetDefRecSetting(presetID, ref setInfo);
                }

                var list = new List<ReserveData>();

                foreach (EpgEventInfo item in itemlist)
                {
                    if (item.StartTimeFlag != 0)
                    {
                        var resInfo = new ReserveData();
                        CommonManager.ConvertEpgToReserveData(item, ref resInfo);
                        resInfo.RecSetting = setInfo;
                        list.Add(resInfo);
                    }
                }

                return ReserveAdd(list);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                return false;
            }
        }
        public bool IsEnableReserveAdd(EpgEventInfo item)
        {
            if (item == null) return false;

            bool retv = (item.StartTimeFlag != 0);
            if (retv == false)
            {
                MessageBox.Show("開始時間未定のため予約できません");
            }
            return retv;
        }
        public bool ReserveAdd(ReserveData item)
        {
            return ReserveAdd(GetList(item));
        }
        public bool ReserveAdd(List<ReserveData> list)
        {
            return ReserveCmdSend(list, cmd.SendAddReserve, "予約追加");
        }

        private uint ReadDataContext(object sender, uint min, uint max)
        {
            try
            {
                if (sender == null || sender.GetType() != typeof(MenuItem))
                {
                    return 0xFF;
                }
                return Math.Max(Math.Min((uint)((sender as MenuItem).DataContext), max), min);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                return 0xFF;
            }
        }
        public bool ReserveChange(ReserveData item)
        {
            return ReserveChange(GetList(item));
        }
        public bool ReserveChange(List<ReserveData> list)
        {
            return ReserveCmdSend(list, cmd.SendChgReserve, "予約変更");
        }

        public bool ReserveDelete(ReserveData item)
        {
            return ReserveDelete(GetList(item));
        }
        public bool ReserveDelete(List<ReserveData> itemlist)
        {
            try
            {
                List<uint> list = itemlist.Select(item => item.ReserveID).ToList();
                return ReserveCmdSend(list, cmd.SendDelReserve, "予約削除");
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                return false;
            }
        }

        private bool ReserveCmdSend<T>(List<T> list, Func<List<T>, uint> cmdSend, string description="")
        {
            try
            {
                if (list.Count == 0) return false;
                ErrCode err = (ErrCode)cmdSend(list);
                CommonManager.CmdErrMsgTypical(err, description);
                return (err == ErrCode.CMD_SUCCESS ? true : false);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                return false;
            }
        }

    }

}
